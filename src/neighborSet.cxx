/*
  Copyright (c) 1998 - 2020
  ILK   - Tilburg University
  CLST  - Radboud University
  CLiPS - University of Antwerp

  This file is part of timbl

  timbl is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  timbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      https://github.com/LanguageMachines/timbl/issues
  or send mail to:
      lamasoftware (at ) science.ru.nl
*/

#include <cmath>
#include <stdexcept>
#include <vector>

#include "timbl/Common.h"
#include "timbl/MsgClass.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"
#include "timbl/neighborSet.h"

namespace Timbl {

  using namespace std;
  using namespace Common;

  neighborSet::neighborSet(): showDistance(false),showDistribution(false){}

  neighborSet::~neighborSet(){
    clear();
  }

  neighborSet::neighborSet( const neighborSet& in ){
    showDistance = in.showDistance;
    showDistribution = in.showDistribution;
    merge( in );
  }

  neighborSet& neighborSet::operator=( const neighborSet& in ){
    if ( this != &in ){
      clear();
      showDistance = in.showDistance;
      showDistribution = in.showDistribution;
      merge( in );
    }
    return *this;
  }

  size_t neighborSet::size() const{
    return distances.size();
  }

  void neighborSet::clear(){
    distances.clear();
    for ( auto const& db : distributions ){
      delete db;
    }
    distributions.clear();
  }

  void neighborSet::reserve( size_t s ){
    distances.reserve( s );
    distributions.reserve( s );
  }

  void neighborSet::truncate( size_t len ){
    if ( len < distributions.size() ){
      for ( size_t i=len; i < distributions.size(); ++i ){
	delete distributions[i];
      }
      distributions.resize( len );
      distances.resize( len);
    }
  }

  void neighborSet::push_back( double d, const ValueDistribution &dist ){
    distances.push_back( d );
    distributions.push_back( dist.to_VD_Copy() );
  }

  void neighborSet::merge( const neighborSet& s ){
    // reserve enough space to avoid reallocations
    // reallocation invalidates pointers!
    reserve( size() + s.size() );
    vector<double>::iterator dit1 = distances.begin();
    vector<double>::const_iterator dit2 = s.distances.begin();
    vector<ValueDistribution *>::iterator dis1 = distributions.begin();
    vector<ValueDistribution *>::const_iterator dis2 = s.distributions.begin();
    while ( dit1 != distances.end() ){
      if ( dit2 != s.distances.end() ){
	if (fabs(*dit1 - *dit2) < Epsilon) {
	  // equal
	  (*dis1)->Merge( **dis2 );
	  ++dit1;
	  ++dis1;
	  ++dit2;
	  ++dis2;
	}
	else if ( *dit1 < *dit2 ){
	  ++dit1;
	  ++dis1;
	}
	else {
	  dit1 = distances.insert( dit1, *dit2 );
	  ++dit1;
	  ++dit2;
	  dis1 = distributions.insert( dis1, (*dis2)->to_VD_Copy() );
	  ++dis1;
	  ++dis2;
	}
      }
      else {
	break;
      }
    }
    while ( dit2 != s.distances.end() ){
      distances.push_back( *dit2 );
      ++dit2;
      distributions.push_back( (*dis2)->to_VD_Copy() );
      ++dis2;
    }
  }

  double neighborSet::relativeWeight( const decayStruct *d,
				      size_t k ) const{
    double result = 1.0;
    if ( !d )
      return result;
    switch ( d->type() ){
    case Zero:
      break;
    case InvDist:
      result = 1.0/(distances[k] + Epsilon);
      break;
    case InvLinear:
      if ( k > 0 && size() != 1 ){
	double nearest_dist, furthest_dist;
	nearest_dist = distances[0];
	furthest_dist = distances[size()-1];
	result = (furthest_dist - distances[k]) /
	  (furthest_dist-nearest_dist);
      }
      break;
    case ExpDecay:
      result = exp(-d->alpha*pow(distances[k], d->beta));
      if ( result == 0 ){
	// A result of zero is undesirable. (bug 89)
	// We optimisticly replace it with Epsilon
	result = Epsilon;
      }
      break;
    default:
      throw "wrong value in switch";
    }
    return result;
  }

  double neighborSet::getDistance( size_t n ) const {
    if ( size() <= n ){
      throw std::range_error( "getDistance() parameter exceeds size of neighborSet" );
    }
    return distances[n];
  }

  const ValueDistribution *neighborSet::getDistribution( size_t n ) const {
    if ( size() <= n ){
      throw std::range_error( "getDistribution() parameter exceeds size of neighborSet" );
    }
    return distributions[n];
  }

  WValueDistribution *neighborSet::bestDistribution( const decayStruct *d,
						     size_t max ) const {
    // Analyse the set to find THE best ValueDistribution.
    // For each neighbor, we loop over the number of bests in that
    // bin, and merge that distribution into the result
    //
    WValueDistribution *result = new WValueDistribution();
    size_t stop = distributions.size();
    stop = ( max > 0 && max < stop ? max : stop );
    for ( size_t k = 0; k < stop; ++k ) {
      result->MergeW( *distributions[k], relativeWeight( d, k ) );
    }
    return result;
  }

  ostream& operator<<( ostream& os, const neighborSet& set ){
    for ( unsigned int i=0; i < set.size(); ++i ){
      os << "# k=" << i+1;
      if ( set.showDistribution )
	os << "\t"  << set.distributions[i]->DistToStringW(0);
      if ( set.showDistance ){
	int OldPrec = os.precision(DBL_DIG-1);
	os.setf(ios::showpoint);
	os << "\t" << set.distances[i];
	os.precision(OldPrec);
      }
      os << endl;
    }
    return os;
  }

  ostream& operator<<( ostream& os, const neighborSet *Set ){
    os << *Set;
    return os;
  }

  ostream& operator<<( ostream& os, const decayStruct& dc ){
    return dc.put( os );
  }

  ostream& zeroDecay::put( ostream& os ) const {
    return os;
  }

  ostream& invLinDecay::put( ostream& os ) const {
    os << "Decay         : " << TiCC::toString( type(), true);
    return os;
  }

  ostream& invDistDecay::put( ostream& os ) const {
    os << "Decay         : " << TiCC::toString( type(), true);
    return os;
  }

  ostream& expDecay::put( ostream& os ) const {
    os << "Decay         : " << TiCC::toString( type(), true);
    os << " a=" << alpha << " b= " << beta;
    return os;
  }

  ostream& operator<<( ostream& os, const decayStruct *dc ){
    if ( dc )
      os << *dc;
    return os;
  }

}
