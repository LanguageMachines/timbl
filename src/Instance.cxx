/*
  Copyright (c) 1998 - 2008
  ILK  -  Tilburg University
  CNTS -  University of Antwerp
 
  This file is part of Timbl

  Timbl is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Timbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      http://ilk.uvt.nl/software.html
  or send mail to:
      Timbl@uvt.nl
*/
#include <list>
#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <typeinfo>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cfloat>
#include <cassert>

#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/MsgClass.h"
#include "timbl/Tree.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"

#ifdef PTHREADS
#include <pthread.h>
#endif

using namespace std;

namespace Timbl {
  using namespace Common;

  size_t Vfield::Index() { return value->Index(); }
  
  ostream& operator<<(ostream& os, const Vfield *vd ) {
    return vd->put( os );
  }
  
  ostream& operator<<(ostream& os, const Vfield& vd ) {
    return vd.put( os );
  }
  
  ostream& Vfield::put( ostream& os ) const {
    os << value << " " << weight;
    return os;
  }
  
  inline int random_number( int Min, int Max ){
    // calculate a random integer within the interval [min,max]
    if ( Min == Max )
      return Min;
    double randnum = (double)rand()/(double)RAND_MAX;
    randnum *= (Max-Min);
    randnum += Min;
    return (int)floor(randnum+0.5);
  }  

  void ValueDistribution::clear(){ 
    VDlist::iterator it;
    for ( it = distribution.begin(); it != distribution.end(); ++it )
      delete it->second;
    distribution.clear(); 
    total_items = 0;
  }
  
  void ValueDistribution::DistToEncodedString( string& DistStr ) const {
    ostringstream oss;
    VDlist::const_iterator it = distribution.begin();
    oss << "{ ";
    while ( it != distribution.end() ){
      Vfield *f = it->second;
      if ( f->Freq() <= 0 ){
	++it;
	continue;
      }
      oss << encode(f->Value()->Name())<< " " << f->Freq();
      ++it;
      if ( it != distribution.end() )
	oss << ", ";
    }
    oss << " }";
    DistStr = oss.str();
  }

  void WValueDistribution::DistToEncodedString( string& DistStr ) const {
    ostringstream oss;
    VDlist::const_iterator it = distribution.begin();
    oss << "{ ";
    while ( it != distribution.end() ){
      Vfield *f = it->second;
      if ( abs(f->weight) < Epsilon ){
	++it;
	continue;
      }
      oss << encode(f->value->Name()) << " " << f->weight;
      ++it;
      if ( it != distribution.end() )
	oss << ", ";
    }
    oss << " }";
    DistStr = oss.str();
  }

  void ValueDistribution::DistToString( string& DistStr, double minf ) const {
    ostringstream oss;
    VDlist::const_iterator it = distribution.begin();
    oss.setf(ios::showpoint);
    bool first = true;
    oss << "{ ";
    while ( it != distribution.end() ){
      Vfield *f = it->second;
      if ( f->frequency < minf ){
	++it;
	continue;
      }
      if ( abs(f->frequency) < Epsilon ){
	++it;
	continue;
      }
      if ( !first )
	oss << ", ";
      oss << f->value << " " << double(f->frequency);
      first = false;
      ++it;
    }
    oss << " }";
    DistStr = oss.str();
  }

  void WValueDistribution::DistToString( string& DistStr, double minw ) const {
    ostringstream oss;
    VDlist::const_iterator it = distribution.begin();
    oss.setf(ios::showpoint);
    bool first = true;
    oss << "{ ";
    while ( it != distribution.end() ){
      Vfield *f = it->second;
      if (  abs(f->weight) < minw ){
	++it;
	continue;
      }
      if ( abs(f->weight) < Epsilon ){
	++it;
	continue;
      }
      if ( !first )
	oss << ", ";
      oss << f->value << " " << f->weight;
      first = false;
      ++it;
    }
    oss << " }";
    DistStr = oss.str();
  }
  
  class dblCmp {
  public:
    bool operator() ( const double d1, const double d2 ) const {
      return d1 - d2 > Epsilon;
    }
  };
  
  void ValueDistribution::DistToStringWW( string& DistStr, int beam ) const {
    double minw = 0.0;
    if ( beam > 0 ){
      std::set<double, dblCmp> freqs;
      VDlist::const_iterator it = distribution.begin();
      while ( it != distribution.end() ){
	Vfield *f = it->second;
	freqs.insert( f->frequency );
	++it;
      }
      int cnt=0;
      std::set<double, dblCmp>::iterator rit = freqs.begin();
      while ( rit != freqs.end() && cnt < beam ) { 
	++cnt;
	if ( cnt < beam )
	  ++rit;
      }
      if ( cnt == beam )
	minw = *rit;
    }
    DistToString( DistStr, minw );
  }

  void WValueDistribution::DistToStringWW( string& DistStr, 
					   int beam ) const {
    double minw = 0.0;
    if ( beam > 0 ){
      std::set<double, dblCmp> wgths;
      VDlist::const_iterator it = distribution.begin();
      while ( it != distribution.end() ){
	Vfield *f = it->second;
	wgths.insert( f->weight );
	++it;
      }
      int cnt=0;
      std::set<double, dblCmp>::iterator rit = wgths.begin();
      while ( rit != wgths.end() && cnt < beam ) { 
	++cnt;
	if ( cnt < beam )
	  ++rit;
      }
      if ( cnt == beam )
	minw = *rit;
    }
    DistToString( DistStr, minw );
  }
  
  const string ValueDistribution::DistToString() const {
    string result;
    DistToString( result );
    return result;
  }
  
  const string ValueDistribution::DistToStringW( int beam ) const {
    string result;
    DistToStringWW( result, beam );
    return result;
  }

  const string ValueDistribution::ToEncodedString() const {
    string result;
    DistToEncodedString( result );
    return result;
  }  
  
  double ValueDistribution::Entropy() const {
    double Prob = 0.0;
    double entropy = 0.0;
    size_t TotalVals = total_items;
    if ( TotalVals > 0 ){
      VDlist::const_iterator it = distribution.begin();
      // Loop over the classes in the distibution
      while ( it != distribution.end() ){
	size_t Freq = it->second->Freq();
	if ( Freq > 0 ){
	  Prob = Freq / (double)TotalVals;
	  entropy += Prob * Log2(Prob);
	}
	++it;
      }
    }
    return fabs(entropy);
  }

  void WValueDistribution::Normalize() {
    double sum = 0.0;
    VDlist::iterator it = distribution.begin();
    while ( it != distribution.end() ){
      sum += it->second->Weight();
      ++it;
    }
    it = distribution.begin();
    while ( it != distribution.end() ){
      it->second->SetWeight( it->second->Weight() / sum );
      ++it;
    }
  }
  
  void WValueDistribution::Normalize_1( double factor, const Target *targ ) {
    for ( unsigned int i=0; i < targ->ValuesArray.size(); ++i ){
      // search for val, if not there: add entry with frequency factor;
      // otherwise increment the ExamplarWeight
      TargetValue *val = (TargetValue*)targ->ValuesArray[i];
      size_t id = val->Index();
      VDlist::const_iterator it = distribution.find( id );
      if ( it != distribution.end() ){
	it->second->SetWeight( it->second->Weight() + factor );
      }  
      else {
	distribution[id] = new Vfield( val, 1, factor );
      }
      ++it;
    }
    total_items += targ->ValuesArray.size();
    Normalize();
  }
  
  ValueDistribution *ValueDistribution::to_VD_Copy( ) const {
    ValueDistribution *res = new ValueDistribution();
    size_t key;
    Vfield *vdf;
    VDlist::const_iterator It = distribution.begin();
    while ( It != distribution.end() ){
      key = It->first;
      vdf = It->second;
      res->distribution[key] = new Vfield( vdf->Value(),
					   vdf->Freq(),
					   vdf->Freq() );
      ++It;
    }
    res->total_items = total_items;
    return res;
  }

  WValueDistribution *ValueDistribution::to_WVD_Copy() const {
    WValueDistribution *res = new WValueDistribution();
    size_t key;
    Vfield *vdf;
    VDlist::const_iterator It = distribution.begin();
    while ( It != distribution.end() ){
      key = It->first;
      vdf = It->second;
      res->distribution[key] = new Vfield( vdf->Value(),
					   vdf->Freq(),
					   vdf->Freq() );
      ++It;
    }
    res->total_items = total_items;
    return res;
  }

  WValueDistribution *WValueDistribution::to_WVD_Copy( ) const {
    WValueDistribution *result = new WValueDistribution();
    VDlist::const_iterator it = distribution.begin();
    Vfield *vdf;
    size_t key;
    while ( it != distribution.end() ){
      key = it->first;
      vdf = it->second;
      result->distribution[key] = new Vfield( vdf->Value(),
					      vdf->Freq(),
					      vdf->Weight() );
      ++it;
    }
    result->total_items = total_items;
    return result;
  }
  
  
  //
  // special functions to serialize distibutions including both frequency
  // AND weight information. Needed for store/retrieve InstanceBases
  //
  // First hashed variant:
  //
  
  const string ValueDistribution::SaveHashed() const{
    ostringstream oss;
    VDlist::const_iterator it = distribution.begin();
    oss << "{ ";
    while ( oss.good() && it != distribution.end() ){
      Vfield *f = it->second;
      if ( f->frequency > 0 ){
	oss << f->value->Index() << " " << f->frequency;
      }
      ++it;
      if ( it != distribution.end() )
	oss << ", ";
    }
    oss << " }";
    return oss.str();
  }

  const string WValueDistribution::SaveHashed() const{
    ostringstream oss;
    VDlist::const_iterator it = distribution.begin();
    oss << "{ ";
    while ( oss.good() && it != distribution.end() ){
      Vfield *f = it->second;
      if ( f->frequency > 0 ){
	oss << f->Value()->Index() << " " 
	    << f->frequency << " " << f->weight;
      }
      ++it;
      if ( it != distribution.end() )
	oss << ", ";
    }
    oss << " }";
    return oss.str();
  }

  //
  // non-hashed variant:
  //

  const string ValueDistribution::Save() const{
    ostringstream oss;
    VDlist::const_iterator it = distribution.begin();
    oss << "{ ";
    while ( oss.good() && it != distribution.end() ){
      Vfield *f = it->second;
      if ( f->frequency > 0 ){
	oss << f->value << " " << f->frequency;
      }
      ++it;
      if ( it != distribution.end() )
	oss << ", ";
    }
    oss << " }";
    return oss.str();
  }

  const string WValueDistribution::Save() const{
    ostringstream oss;
    VDlist::const_iterator it = distribution.begin();
    oss << "{ ";
    while ( oss.good() && it != distribution.end() ){
      Vfield *f = it->second;
      if ( f->frequency > 0 ){
	oss.setf(ios::showpoint);
	oss << f->value << " " << f->frequency << " " << f->weight;
      }
      ++it;
      if ( it != distribution.end() )
	oss << ", ";
    }
    oss << " }";
    return oss.str();
  }

  void ValueDistribution::SetFreq( const TargetValue *val, const int freq,
				   double ){
    // add entry with frequency freq;
    Vfield *temp = new Vfield( val, freq, freq );
    distribution[val->Index()] = temp;
    total_items += freq;
  }
  
  void WValueDistribution::SetFreq( const TargetValue *val, const int freq,
				    double sw ){
    // add entry with frequency freq;
    // also sets the sample_weight
    Vfield *temp = new Vfield( val, freq, sw );
    distribution[val->Index()] = temp;
    total_items += freq;
  }
  
  bool ValueDistribution::IncFreq( const TargetValue *val, double ){
    // search for val, if not there: add entry with frequency 1;
    // otherwise increment the freqency
    size_t id = val->Index(); 
    VDlist::const_iterator it = distribution.find( id );
    if ( it != distribution.end() ){
      it->second->IncFreq();
    }
    else
      distribution[id] = new Vfield( val, 1, 1.0 );
    total_items += 1;
    return true;
  }
  
  bool WValueDistribution::IncFreq( const TargetValue *val, double sw ){
    // search for val, if not there: add entry with frequency 1;
    // otherwise increment the freqency
    // also set sample weight
    size_t id = val->Index(); 
    VDlist::const_iterator it = distribution.find( id );
    if ( it != distribution.end() ){
      it->second->IncFreq();
    }  
    else {
      distribution[id] = new Vfield( val, 1, sw );
    }
    total_items += 1;
    return fabs( distribution[id]->Weight() - sw ) > Epsilon;
  }
  
  void ValueDistribution::DecFreq( const TargetValue *val ){
    // search for val, if not there, just forget
    // otherwise decrement the freqency
    VDlist::const_iterator it = distribution.find( val->Index() );
    if ( it != distribution.end() ){
      it->second->DecFreq();
      total_items -= 1;
    }  
  }
  
  void ValueDistribution::Merge( const ValueDistribution& VD ){
    VDlist::const_iterator It = VD.distribution.begin();
    size_t key;
    Vfield *vd;
    while ( It != VD.distribution.end() ){
      key = It->first;
      vd = It->second;
      if ( distribution.find(key) != distribution.end() ){
	distribution[key]->AddFreq( vd->Freq() );
      }
      else 
	// VD might be weighted. But we don't need/want that info here
	// Weight == Freq is more convenient
	distribution[key] = new Vfield( vd->Value(), vd->Freq(),
					vd->Freq() );
      ++It;
    }
    total_items += VD.total_items;
  }

  void WValueDistribution::MergeW( const ValueDistribution& VD, 
				   double Weight ){
    VDlist::const_iterator It = VD.distribution.begin();
    while ( It != VD.distribution.end() ){
      Vfield *vd = It->second;
      size_t key = It->first;
      if ( distribution.find(key) != distribution.end() ){
	distribution[key]->SetWeight( distribution[key]->Weight() + vd->Weight() *Weight );
      }
      else {
	distribution[key] = new Vfield( vd->Value(), 1,
					vd->Weight() * Weight);
      }
      ++It;
    }
    total_items += VD.total_items;
  }

  const TargetValue *ValueDistribution::BestTarget( bool& tie,
						    bool do_rand ) const {
    // get the most frequent target from the distribution.
    // In case of a tie take the one which is GLOBALLY the most frequent, 
    // and signal if this ties also!
    // OR (if do_rand) take random one of the most frequents
    const TargetValue *best = NULL;
    tie = false;
    VDlist::const_iterator It = distribution.begin();
    if ( It != distribution.end() ){
      Vfield *pnt = It->second;
      int Max = pnt->Freq();
      if ( do_rand ){
	int nof_best=1, pick=1;
	++It;
	while ( It != distribution.end() ){
	  pnt = It->second;
	  if ( pnt->Freq() > Max ){
	    Max = pnt->Freq();
	    nof_best = 1;
	  }
	  else
	    if ( pnt->Freq() == Max )
	      nof_best++;
	  ++It;
	}
	tie = nof_best > 1;
	pick = random_number( 1, nof_best );
	It = distribution.begin();
	nof_best = 0;
	while ( It != distribution.end() ){
	  pnt = It->second;
	  if ( pnt->Freq() == Max )
	    if ( ++nof_best == pick ){
	      return pnt->Value();
	    }
	  ++It;
	}
	return NULL;
      }
      else {
	best = pnt->Value();
	++It;
	while ( It != distribution.end() ){
	  pnt = It->second;
	  if ( pnt->Freq() > Max ){
	    tie = false;
	    best = pnt->Value();
	    Max = pnt->Freq();
	  }
	  else
	    if ( pnt->Freq() == Max ) {
	      tie = true;
	      if ( pnt->Value()->ValFreq() > best->ValFreq() ){
		best = pnt->Value();
	      }
	    }
	  ++It;
	}
	return best;
      }
    }
    return best;
  }
  
  const TargetValue *WValueDistribution::BestTarget( bool& tie,
						    bool do_rand ) const {
    // get the most frequent target from the distribution.
    // In case of a tie take the one which is GLOBALLY the most frequent, 
    // and signal if this ties also!
    // OR (if do_rand) take random one of the most frequents
    const TargetValue *best = NULL;
    VDlist::const_iterator It = distribution.begin();
    tie = false;
    if ( It != distribution.end() ){
      double Max = It->second->Weight();
      if ( do_rand ){
	int nof_best=1, pick=1;
	++It;
	while ( It != distribution.end() ){
	  if ( It->second->Weight() > Max ){
	    Max = It->second->Weight();
	    nof_best = 1;
	  }
	  else
	    if ( abs(It->second->Weight()- Max) < Epsilon )
	      nof_best++;
	  ++It;
	}
	tie = nof_best > 1;
	tie = nof_best > 1;
	pick = random_number( 1, nof_best );
	It = distribution.begin();
	nof_best = 0;
	while ( It != distribution.end() ){
	  if ( abs(It->second->Weight() - Max) < Epsilon )
	    if ( ++nof_best == pick ){
	      return It->second->Value();
	    }
	  ++It;
	}
	return NULL;
      }
      else { 
	best = It->second->Value();
	++It;
	while ( It != distribution.end() ){
	  if ( It->second->Weight() > Max ){
	    tie = false;
	    best = It->second->Value();
	    Max = It->second->Weight();
	  }
	  else 
	    if ( abs(It->second->Weight() - Max) < Epsilon ) {
	      tie = true;
	      if ( It->second->Value()->ValFreq() > best->ValFreq() ){
		best = It->second->Value();
	      }
	    }
	  ++It;
	}
	return best;
      }
    }
    return best;
  }

  Feature::Feature( int a, int b, StringHash *T ): 
    BaseFeatTargClass(a,b,T),
    metric_matrix( 0 ),
    metric( UnknownMetric ),
    ignore( false ),
    numeric( false ),
    vcpb_read( false ),
    PrestoreStatus(ps_undef),
    Prestored_metric( UnknownMetric ),
    entropy( 0.0 ),
    info_gain (0.0),
    split_info(0.0),
    gain_ratio(0.0),
    chi_square(0.0),
    shared_variance(0.0),
    matrix_clip_freq(10),
    n_dot_j(NULL),
    n_i_dot(NULL),
    n_min (0.0),
    n_max (0.0),
    SaveSize(0),
    SaveNum(0),
    weight(0.0)
  {}
  
  void Feature::InitSparseArrays(){
    // Loop over all values.
    //
    VCarrtype::const_iterator it = ValuesArray.begin();
    while ( it != ValuesArray.end() ){
      FeatureValue *FV = (FeatureValue*)*it;
      size_t freq = FV->ValFreq();
      FV->ValueClassProb->Clear();
      if ( freq > 0 ){
	// Loop over all present classes.
	//
	ValueDistribution::dist_iterator It = FV->TargetDist.begin();
	while ( It != FV->TargetDist.end() ){
	  FV->ValueClassProb->Assign( It->second->Index(), 
				      It->second->Freq()/(double)freq );
	  ++It;
	}
      }
      ++it;
    } 
  }

  struct D_D {
    D_D(){ dist = NULL; value = 0.0; };
    D_D( FeatureValue *fv ){
      stringTo<double>( fv->Name(), value );
      dist = &fv->TargetDist;
    }
    ValueDistribution *dist;
    double value;
  };

  bool dd_less( const D_D* dd1, const D_D* dd2 ){
    return dd1->value < dd2->value;
  }
  
  void Feature::NumStatistics( vector<FeatureValue *>& FVBin,
			       double DBentropy,
			       int BinSize ){
    double Prob, FVEntropy;
    size_t TotalVals = TotalValues();
    entropy = 0.0;
    vector<D_D*> ddv;
    VCarrtype::const_iterator it = ValuesArray.begin();
    ddv.reserve( ValuesArray.size() );
    while ( it != ValuesArray.end() ){
      FeatureValue *FV = (FeatureValue*)*it;
      if ( FV->ValFreq() > 0 ){
	ddv.push_back( new D_D( FV ) );
      }
      ++it;
    }
    sort( ddv.begin(), ddv.end(), dd_less );
    size_t dd_len = ddv.size();
    int num_per_bin = (int)floor( (double)dd_len / BinSize);
    size_t rest = dd_len - num_per_bin * BinSize;
    if ( rest )
      num_per_bin++;
    int jj = 0;
    int cnt = 0;
    for ( size_t m = 0; m < dd_len; ++m ){
      FVBin[jj]->TargetDist.Merge( *ddv[m]->dist );
      if ( ++cnt >= num_per_bin ){
	++jj;
	if ( --rest == 0 )
	  --num_per_bin;
	cnt = 0;
      }
    }
    for ( size_t j=0; j < dd_len; ++j ){
      delete ddv[j];
    }
    for ( int k=0; k < BinSize; k++ ){
      FeatureValue *pnt = FVBin[k];
      size_t Freq = pnt->TargetDist.totalSize();
      pnt->ValFreq( Freq );
      if ( Freq > 0 ){
	// Entropy for this FV pair.
	//
	FVEntropy = 0.0;
	ValueDistribution::dist_iterator It = pnt->TargetDist.begin();
	while ( It !=  pnt->TargetDist.end() ){
	  Prob = It->second->Freq()/(double)Freq;
	  FVEntropy += Prob * Log2(Prob);
	  ++It;
	}
	entropy += -FVEntropy * Freq / (double)TotalVals;
      }
    }
    entropy = fabs( entropy );
    // Info gain.
    //
    info_gain = DBentropy - entropy;
    
    // And the split info.
    //
    split_info = 0.0;
    for ( int l=0; l < BinSize; ++l ){
      size_t Freq = FVBin[l]->ValFreq();
      if ( Freq > 0 ){
	Prob = Freq / (double)TotalVals;
	split_info += Prob * Log2(Prob);
      } 
    }
    split_info = -split_info;
    // Gain ratio.
    //
    if ( fabs(split_info) <Epsilon ){
      gain_ratio = 0.0;
      info_gain  = 0.0;
      entropy = DBentropy;
    }
    else
      gain_ratio = info_gain / split_info;
  }
 
  void Feature::Statistics( double DBentropy, Target *Targets, bool full ){
    Statistics( DBentropy );
    if ( full ){
      ChiSquareStatistics( Targets );
      SharedVarianceStatistics( Targets, EffectiveValues() );
    }
  }
  
  void Feature::NumStatistics( double DBentropy, Target *Targets,
			       int BinSize, bool full ){
    char dumname[80];
    vector<FeatureValue *> FVBin(BinSize);;
    for ( int i=0; i < BinSize; ++i ){
      sprintf( dumname, "dum%d", i );
      FVBin[i] = new FeatureValue( dumname );
    }
    NumStatistics( FVBin, DBentropy, BinSize );
    if ( full ){
      ChiSquareStatistics( FVBin, BinSize, Targets );
      int cnt = 0;   // count effective values in Bin 
      for( int i=0; i < BinSize; ++i ){
	if ( FVBin[i]->ValFreq() > 0 )
	  cnt++;
      }
      SharedVarianceStatistics( Targets, cnt );
    }
    for( int i=0; i < BinSize; ++i ){
      delete FVBin[i];
    }
  }

  void Feature::Statistics( double DBentropy ){
    double Prob = 0.0;
    size_t TotalVals = TotalValues();
    entropy = 0.0;
    // Loop over the values.
    VCarrtype::const_iterator it = ValuesArray.begin();
    while ( it != ValuesArray.end() ){
      FeatureValue *fv = (FeatureValue*)*it;
      // Entropy for this FV pair.
      double FVEntropy = 0.0;
      size_t Freq = fv->ValFreq();
      if ( Freq > 0 ){
	ValueDistribution::dist_iterator It = fv->TargetDist.begin();
	while ( It != fv->TargetDist.end() ){
	  Prob = It->second->Freq() / (double)Freq;
	  FVEntropy += Prob * Log2(Prob);
	  ++It;
	}
	entropy += -FVEntropy * Freq / (double)TotalVals;
      }
      ++it;
    } 
    
    entropy = fabs( entropy );
    // Info. gain.
    //
    info_gain = DBentropy - entropy;
    // And the split. info.
    //
    split_info = 0.0;
    it = ValuesArray.begin();
    while( it != ValuesArray.end() ){
      FeatureValue *fv = (FeatureValue*)*it;
      Prob = fv->ValFreq() / (double)TotalVals;
      if ( Prob > 0 ) {
	split_info += Prob * Log2(Prob);
      }
      ++it;
    } 
    split_info = -split_info;
    // Gain ratio.
    //
    if ( fabs(split_info) < Epsilon )
      gain_ratio = 0.0;
    else
      gain_ratio = info_gain / split_info;
  }

  void Feature::ChiSquareStatistics( vector<FeatureValue *>& FVA, 
				     size_t Num_Vals,
				     Target *Targets ){
    chi_square = 0.0;
    long int n_dot_dot = 0;
    double tmp;
    size_t Size = Targets->ValuesArray.size();
    if ( !n_dot_j ) {
      n_dot_j = new long int[Size]; 
      n_i_dot = new long int[Num_Vals]; 
      SaveSize = Size;
      SaveNum = Num_Vals;
    }
    else {
      if ( SaveSize < Size ){
	delete [] n_dot_j;
	n_dot_j = new long int[Size];
	SaveSize = Size;
      }
      if ( SaveNum < Num_Vals ){
	delete [] n_i_dot;
	n_i_dot = new long int[Num_Vals];
	SaveNum = Num_Vals;
      }
    }
    for ( size_t j = 0; j < Size; ++j ){
      n_dot_j[j] = 0;
    }
    ValueDistribution::dist_iterator It;
    for ( size_t i = 0; i < Num_Vals; ++i ){
      n_i_dot[i] = 0;
      FeatureValue *fv = FVA[i];
      It = fv->TargetDist.begin();
      while ( It != fv->TargetDist.end()  ){
	n_dot_j[It->second->Index()-1] += It->second->Freq();
	n_i_dot[i] += It->second->Freq();
	++It;
      }
      n_dot_dot += n_i_dot[i];
    }
    if ( n_dot_dot != 0 ){
      for ( size_t m = 0; m < Num_Vals; ++m ){
	FeatureValue *fv = FVA[m];
	It = fv->TargetDist.begin();
	size_t n = 0;
	while ( It != fv->TargetDist.end() && n < Size ){
	  while ( n < It->second->Index()-1 ){
	    tmp = ((double)n_dot_j[n++] * (double)n_i_dot[m]) /
	      (double)n_dot_dot;
	    chi_square += tmp;
	  }
	  if ( n == It->second->Index()-1 ){
	    tmp = ((double)n_dot_j[n++] * (double)n_i_dot[m]) /
	      (double)n_dot_dot;
	    if ( fabs(tmp) > Epsilon){
	      chi_square += ( (tmp - It->second->Freq()) *
			      (tmp - It->second->Freq()) ) / tmp;
	    }
	    ++It;
	  }
	  else
	    break;
	}
	while ( n < Size ){
	  tmp = ((double)n_dot_j[n++] * (double)n_i_dot[m]) /
	    (double)n_dot_dot;
	  chi_square += tmp;
	}
      }
    }
  }

  void Feature::ChiSquareStatistics( Target *Targets ){
    chi_square = 0.0;
    long int n_dot_dot = 0;
    double tmp;
    size_t Size = Targets->ValuesArray.size();
    size_t Num_Vals = ValuesArray.size();
    if ( !n_dot_j ) {
      n_dot_j = new long int[Size]; 
      n_i_dot = new long int[Num_Vals]; 
      SaveSize = Size;
      SaveNum = Num_Vals;
    }
    else {
      if ( SaveSize < Size ){
	delete [] n_dot_j;
	n_dot_j = new long int[Size];
	SaveSize = Size;
      }
      if ( SaveNum < Num_Vals ){
	delete [] n_i_dot;
	n_i_dot = new long int[Num_Vals];
	SaveNum = Num_Vals;
      }
    }
    for ( size_t j = 0; j < Size; ++j ){
      n_dot_j[j] = 0;
    }
    ValueDistribution::dist_iterator It;
    VCarrtype::const_iterator it = ValuesArray.begin();
    int i = 0;
    while ( it != ValuesArray.end() ){
      n_i_dot[i] = 0;
      FeatureValue *fv = (FeatureValue *)*it;
      It = fv->TargetDist.begin();
      while ( It != fv->TargetDist.end()  ){
	long int fr = It->second->Freq();
	n_dot_j[It->second->Index()-1] += fr;
	n_i_dot[i] += fr;
	++It;
      }
      n_dot_dot += n_i_dot[i];
      ++it;
      ++i;
    }
    if ( n_dot_dot != 0 ){
      it = ValuesArray.begin();
      int m = 0;
      while ( it != ValuesArray.end() ){
	FeatureValue *fv = (FeatureValue *)*it;
	It = fv->TargetDist.begin();
	size_t n = 0;
	while ( It != fv->TargetDist.end() && n < Size ){
	  size_t id = It->second->Index()-1;
	  long int fr = It->second->Freq();
	  while ( n < id ){
	    tmp = ((double)n_dot_j[n++] * (double)n_i_dot[m]) /
	      (double)n_dot_dot;
	    chi_square += tmp;
	  }
	  if ( n == id ){
	    tmp = ((double)n_dot_j[n++] * (double)n_i_dot[m]) /
	      (double)n_dot_dot;
	    if ( fabs(tmp) > Epsilon ){
	      chi_square += ( (tmp - fr ) * (tmp - fr ) ) / tmp;
	    }
	    ++It;
	  }
	  else
	    break;
	}
	while ( n < Size ){
	  tmp = ((double)n_dot_j[n++] * (double)n_i_dot[m]) /
	    (double)n_dot_dot;
	  chi_square += tmp;
	}
	++it;
	++m;
      }
    }
  }

  ostream& operator<<(ostream& os, const ValueDistribution& vd ) {
    string tmp;
    vd.DistToString( tmp );
    os << tmp;
    return os;
  }
  
  ostream& operator<<(ostream& os, const ValueDistribution *vd ) {
    string tmp = "{null}";
    if( vd )
      vd->DistToString( tmp );
    os << tmp;
    return os;
  }
  
  ValueDistribution *ValueDistribution::read_distribution( istream &is, 
							   Target *Targ,
							   bool do_fr ){
    // read a distribution from stream is into Target
    // if do_f we also adjust the value of Frequency of the Target, which is
    // otherwise 1. Special case when reading the TopDistribution.
    //
    ValueDistribution *result = 0;
    TargetValue *target;
    string buf;
    int freq;
    double sw;
    char nextCh;
    is >> nextCh;   // skip {
    if ( nextCh != '{' ){
      throw string( "missing '{' " );
    }
    else {
      int next;
      do {
	is >> ws >> buf;
	is >> freq;
	if ( do_fr ){
	  target = Targ->add_value( buf, freq );
	}
	else
	  target = Targ->Lookup( buf );
	if ( !target ){
	  delete result;
	  result = 0;
	  break;
	}
	next = look_ahead(is);
	if ( next == ',' ){
	  if ( !result )
	    result = new ValueDistribution();
	  result->SetFreq( target, freq );
	  is >> nextCh;
	  next = look_ahead(is);
	}
	else if ( next == '}' ){
	  if ( !result )
	    result = new ValueDistribution();
	  result->SetFreq( target, freq );
	}
	else if ( isdigit(next) ){
	  if ( !result )
	    result = new WValueDistribution();
	  is >> sw;
	  result->SetFreq( target, freq, sw );
	  next = look_ahead(is);
	  if ( next == ',' ){
	    is >> nextCh;
	    next = look_ahead(is);
	  }
	}
      } while ( is && next != '}' );
      if ( is )
	is >> nextCh;   // skip }
      else {
	delete result;
	throw string( "missing '}' " );
      }
    }
    return result;
  }
  
  
  ValueDistribution *ValueDistribution::read_distribution_hashed( istream &is, 
								  Target *Targ,
								  bool do_fr ){

    ValueDistribution *result = 0;
    // read a distribution from stream is into Target
    // if do_f we also adjust the value of Frequency of the Target, which is
    // otherwise 1. Special case when reading the TopDistribution.
    //
    TargetValue *target;
    int freq;
    unsigned int index;
    double sw;
    char nextCh;
    is >> nextCh;   // skip {
    if ( nextCh != '{' ){
      throw string( "missing '{' " );
    }
    else {
      int next;
      do {
	is >> index;
	is >> freq;
	if ( do_fr ){
	  target = Targ->add_value( index, freq );
	}
	else
	  target = Targ->ReverseLookup( index );
	if ( !target ){
	  delete result;
	  result = 0;
	  break;
	}
	next = look_ahead(is);
	if ( next == ',' ){
	  if ( !result )
	    result = new ValueDistribution();
	  result->SetFreq( target, freq );
	  is >> nextCh;
	  next = look_ahead(is);
	}
	else if ( next == '}' ){
	  if ( !result )
	    result = new ValueDistribution();
	  result->SetFreq( target, freq );
	}
	else if ( isdigit(next) ){
	  is >> sw;
	  if ( !result )
	    result = new WValueDistribution();
	  result->SetFreq( target, freq, sw );
	  next = look_ahead(is);
	  if ( next == ',' ){
	    is >> nextCh;
	    next = look_ahead(is);
	  }
	}
      } while ( is && next != '}' );
      if ( is )
	is >> nextCh;   // skip }
      else {
	delete result;
	throw string( "missing '}' " );
      }
    }
    return result;
  }
  
   
  ostream& operator<<( std::ostream& os, ValueClass const *vc ){
    if ( vc ){
      os << vc->Name();
    }
    else
      os << "*FV-NF*";
    return os;
  }
  
  FeatureValue::FeatureValue( const std::string& value,
			      unsigned int hash_val ):
    ValueClass( value, hash_val ), ValueClassProb( NULL ) {
  }
  
  FeatureValue::FeatureValue( const string& s ):
    ValueClass( s, 0 ),
    ValueClassProb(NULL){ 
    Frequency = 0; 
  }
  
  FeatureValue::~FeatureValue( ){
    delete ValueClassProb;
  }
  
  TargetValue::TargetValue( const std::string& value,
			    unsigned int value_hash ):
    ValueClass( value, value_hash ){}
  
  size_t BaseFeatTargClass::EffectiveValues() const {
    int result = 0;
    VCarrtype::const_iterator it = ValuesArray.begin();
    while( it != ValuesArray.end() ){
      if ( (*it)->ValFreq() > 0 )
	++result;
      ++it;
    }
    return result;
  }
  
  size_t BaseFeatTargClass::TotalValues() const {
    size_t result = 0;
    VCarrtype::const_iterator it = ValuesArray.begin();
    while( it != ValuesArray.end() ){
      result += (*it)->ValFreq();
      ++it;
    }
    return result;
  }
  
  FeatureValue *Feature::Lookup( const string& str ) const {
    FeatureValue *result = NULL;
    unsigned int index = TokenTree->Lookup( str );
    if ( index ) {
      IVCmaptype::const_iterator it = ValuesMap.find( index );
      if ( it != ValuesMap.end() )
	result = (FeatureValue *)(it->second);
    }
    return result;
  }

  FeatureValue *Feature::add_value( const string& valstr, 
				    TargetValue *tv ){
    unsigned int hash_val = TokenTree->Hash( valstr );
    return add_value( hash_val, tv );
  }
  
  FeatureValue *Feature::add_value( unsigned int index, 
				    TargetValue *tv ){
    IVCmaptype::const_iterator it = ValuesMap.find( index );
    if(  it == ValuesMap.end() ){
      const string& value = TokenTree->ReverseLookup( index );
      // we want to store the singleton value for this index
      // so we MUST reverse lookup the index
      FeatureValue *fv = new FeatureValue( value, index );
      ValuesMap[index] = fv;
      ValuesArray.push_back( fv );
    }
    else {
      it->second->incr_val_freq();
    }
    FeatureValue *result = (FeatureValue *)ValuesMap[index];
    if ( tv )
      result->TargetDist.IncFreq(tv);
    return result;
  }
  
  bool Feature::increment_value( FeatureValue *FV, 
				 TargetValue *tv ){
    bool result = false;
    if ( FV ){
      FV->incr_val_freq();
      if ( tv )
	FV->TargetDist.IncFreq(tv);
      result = true;
    }
    return result;
  }
  
  bool Feature::decrement_value( FeatureValue *FV, TargetValue *tv ){
    bool result = false;
    if ( FV ){
      FV->decr_val_freq();
      if ( tv )
	FV->TargetDist.DecFreq(tv);
      result = true;
    }
    return result;
  }
  
  bool Feature::AllocSparseArrays( size_t Dim ){
    // Loop over all values.
    //
    VCarrtype::const_iterator it = ValuesArray.begin();
    while ( it != ValuesArray.end() ){
      FeatureValue *FV = (FeatureValue*)*it;
      // Loop over all classes.
      if ( FV->ValueClassProb == NULL ){
	if ( !(FV->ValueClassProb = new SparseValueProbClass( Dim )) ){
	  return false;
	}
      }
      ++it;
    } 
    return true;
  }
  
  BaseFeatTargClass::BaseFeatTargClass( int Size, int Inc, StringHash *T ):
    CurSize( Size ),
    Increment( Inc ),
    TokenTree( T ){
  }
  
  BaseFeatTargClass::~BaseFeatTargClass(){
    VCarrtype::iterator it = ValuesArray.begin();
    while ( it != ValuesArray.end() ){
      delete *it;
      ++it;
    }
  }
  
  TargetValue *Target::Lookup( const string& str ) const {
    TargetValue *result = 0;
    unsigned int index = TokenTree->Lookup( str );
    if ( index ) {
      IVCmaptype::const_iterator it = ValuesMap.find( index );
      result = (TargetValue *)it->second;
    }
    return result;
  }

  TargetValue *Target::ReverseLookup( unsigned int index ) const {
    IVCmaptype::const_iterator it = ValuesMap.find( index );
    return dynamic_cast<TargetValue *>(it->second);
  }

  Feature::~Feature(){
    delete_matrix();
    if ( n_dot_j ) {
      delete [] n_dot_j;
      delete [] n_i_dot;
    }
  }

  bool isStorable( MetricType m ){
    return m == ValueDiff || m == JeffreyDiv || m == Levenshtein;
  }
  
  bool Feature::storableMetric( MetricType gm ){
    return isStorable( metric) ||
      ( metric == DefaultMetric && isStorable( gm ) );
  }  

  bool Feature::matrix_present() const {
    return metric_matrix != 0 && PrestoreStatus == ps_ok;
  }
  
  unsigned int Feature::matrix_byte_size() const {
    if ( metric_matrix )
      return metric_matrix->NumBytes();
    else
      return 0;
  }
  
  FeatVal_Stat Feature::prepare_numeric_stats(){
    double tmp; 
    size_t freq;
    bool first = true;
    VCarrtype::const_iterator it = ValuesArray.begin();
    while ( it != ValuesArray.end() ){
      FeatureValue *fv = (FeatureValue*)*it;
      freq = fv->ValFreq();
      if ( freq > 0 ){
	if ( !stringTo<double>( fv->Name(), tmp ) ){
	  Warning( "a Non Numeric value '" + 
		   string(fv->Name()) +
		   "' in Numeric Feature!" );
	  return NotNumeric;
	}
	if ( first ){
	  first = false;
	  n_min = tmp;
	  n_max = tmp;
	}
	else if ( tmp < n_min )
	  n_min = tmp;
	else if ( tmp > n_max )
	  n_max = tmp;
      }
      ++it;
    }
    if ( fabs(n_max - n_min) < Epsilon )
      return SingletonNumeric;
    else
      return NumericValue;
  }

  inline int min( int i1, int i2 ) { return (i1>i2?i2:i1); }
  inline size_t min( size_t i1, size_t i2 ) { return (i1>i2?i2:i1); }

  void Feature::SharedVarianceStatistics( Target *Targ, int eff_cnt ){
    size_t NumInst = Targ->TotalValues();
    int NumCats = Targ->EffectiveValues();
    int k = min( NumCats, eff_cnt ) - 1 ;
    if ( k == 0 || NumInst == 0 )
      shared_variance = 0;
    else
      shared_variance = chi_square / (double)( NumInst * k );
  }
  
  double SparseValueProbClass::vd_distance( SparseValueProbClass *s ) const {
    double result = 0.0;
    IDmaptype::const_iterator p1 = vc_map.begin();
    IDmaptype::const_iterator p2 = s->vc_map.begin();
    while( p1 != vc_map.end() &&
	   p2 != s->vc_map.end() ){
      if ( p2->first < p1->first ){
	result += p2->second;
	++p2;
      }
      else if ( p2->first == p1->first ){
	result += fabs( p1->second - p2->second );
	++p1;
	++p2;
      }
      else {
	result += p1->second;
	++p1;
      }	  
    }
    while ( p1 != vc_map.end() ){
      result += p1->second;
      ++p1;
    }
    while ( p2 != s->vc_map.end() ){
      result += p2->second;
      ++p2;
    }
    result = result / 2.0;
    return result;
  }
  
  double lv_distance( const string& source, const string& target ){
    // code taken from: http://www.merriampark.com/ldcpp.htm
    //    Levenshtein Distance Algorithm: C++ Implementation
    //                  by Anders Sewerin Johansen
    // Step 1
    const size_t n = source.length();
    const size_t m = target.length();
    if (n == 0) {
      return (double)m;
    }
    if (m == 0) {
      return (double)n;
    }    
    // Good form to declare a TYPEDEF
    typedef std::vector< std::vector<size_t> > Tmatrix;     
    Tmatrix matrix(n+1);
    
    // Size the vectors in the 2.nd dimension. Unfortunately C++ doesn't
    // allow for allocation on declaration of 2.nd dimension of vec of vec
    
    for ( size_t i = 0; i <= n; ++i ) {
      matrix[i].resize(m+1);
    }
    // Step 2
    for ( size_t i = 0; i <= n; ++i ) {
      matrix[i][0]=i;
    }
    for ( size_t i = 0; i <= m; ++i ) {
      matrix[0][i]=i;
    }
    // Step 3
    for ( size_t i = 1; i <= n; ++i ) {
      const char s_i = source[i-1];
      // Step 4
      for ( size_t j = 1; j <= m; ++j ) {
	const char t_j = target[j-1];
	// Step 5
	int cost;
	if (s_i == t_j) {
	  cost = 0;
	}
	else {
	  cost = 1;
	}
	// Step 6
	const size_t above = matrix[i-1][j];
	const size_t left = matrix[i][j-1];
	const size_t diag = matrix[i-1][j-1];
	size_t cell = min( above + 1, min(left + 1, diag + cost));
	// Step 6A: Cover transposition, in addition to deletion,
	// insertion and substitution. This step is taken from:
	// Berghel, Hal ; Roach, David : "An Extension of Ukkonen's 
	// Enhanced Dynamic Programming ASM Algorithm"
	// (http://www.acm.org/~hlb/publications/asm/asm.html)
	if (i>2 && j>2) {
	  size_t trans=matrix[i-2][j-2]+1;
	  if (source[i-2]!=t_j) trans++;
	  if (s_i!=target[j-2]) trans++;
	  if (cell>trans) cell=trans;
	}
	matrix[i][j]=cell;
      }
    }
    return (double)matrix[n][m];
  }
  
  double FeatureValue::VDDistance( FeatureValue *G, 
				   size_t limit, MetricType mt ) const {
    double result = 0.0;
    if ( G != this ){
      if ( ValFreq() < limit ||
	   G->ValFreq() < limit ){
	switch ( mt ){
	case Overlap:
	  result = 1.0;
	  break;
	case Levenshtein:
	  result = lv_distance( Name(), G->Name() );
	  break;
	default:
	  string msg = string("Invalid value '") + toString(mt) 
	    + "' in switch (" 
	    + __FILE__  + "," + toString(__LINE__) + ")\n"
	    + "ABORTING now";
	  throw std::logic_error( msg );
	}
      }
      else {
	result = ValueClassProb->vd_distance( G->ValueClassProb );
      }
    }
    return result;
  }

  double k_log_k_div_m( double k, double l ) {
    if ( abs(k+l) < Epsilon )
      return 0;
    return k * Log2( (2.0 * k)/( k + l ) );
  }
  
  double SparseValueProbClass::jd_distance( SparseValueProbClass *s ) const {
    double result = 0.0;
    IDmaptype::const_iterator p1 = vc_map.begin();
    IDmaptype::const_iterator p2 = s->vc_map.begin();
    while( p1 != vc_map.end() &&
	   p2 != s->vc_map.end() ){
      if ( p2->first < p1->first ){
	result += p2->second;
	++p2;
      }
      else if ( p2->first == p1->first ){
	result += k_log_k_div_m( p1->second, p2->second );
	result += k_log_k_div_m( p2->second, p1->second );
	++p1;
	++p2;
      }
      else {
	result += p1->second;
	++p1;
      }	  
    }
    while ( p1 != vc_map.end() ){
      result += p1->second;
      ++p1;
    }
    while ( p2 != s->vc_map.end() ){
      result += p2->second;
      ++p2;
    }
    result = result / 2.0;
    return result;
  }
  
  double FeatureValue::JDDistance( FeatureValue *G, 
				   size_t limit,
				   MetricType mt ) const {
    double result = 0.0;
    if ( G != this ){
      if ( ValFreq() < limit ||
	   G->ValFreq() < limit ){
	switch ( mt ){
	case Overlap:
	  result = 1.0;
	  break;
	case Levenshtein:
	  result = lv_distance( Name(), G->Name() );
	  break;
	default:
	  string msg = string("Invalid value '") + toString(mt) 
	    + "' in switch (" 
	    + __FILE__  + "," + toString(__LINE__) + ")\n"
	    + "ABORTING now";
	  throw std::logic_error( msg );
	}
      }
      else {
	result = ValueClassProb->jd_distance( G->ValueClassProb );
      }
    }
    return result;
  }
  
  double FeatureValue::LDDistance( FeatureValue *G, 
				   size_t limit,
				   MetricType mt ) const {
    // LD is ALWAYS calculated!
    double result = 0.0;
    if ( G != this ){
      if ( ValFreq() < limit ||
	   G->ValFreq() < limit ){
	switch ( mt ){
	case Overlap:
	  result = 1.0;
	  break;
	case Levenshtein:
	  result = lv_distance( Name(), G->Name() );
	  break;
	default:
	  string msg = string("Invalid value '") + toString(mt) 
	    + "' in switch (" 
	    + __FILE__  + "," + toString(__LINE__) + ")\n"
	    + "ABORTING now";
	  throw std::logic_error( msg );
	}
      }
      else
	result = lv_distance( Name(), G->Name() );
    }
    return result;
  }
  
  void Feature::delete_matrix(){
    if ( metric_matrix ){
      metric_matrix->Clear();
      delete metric_matrix;
    }
    metric_matrix = 0;
    PrestoreStatus = ps_undef;
  }
  
  bool Feature::store_matrix( MetricType gmt, int limit, MetricType df ){
    //
    // Store a complete distance matrix.
    //
    if ( !metric_matrix )
      metric_matrix = new SparseSymetricMatrix<FeatureValue*>();

    MetricType mt = metric;
    if ( mt == DefaultMetric )
      mt = gmt;
    
    if ( PrestoreStatus != ps_failed && isStorable( mt ) ) {
      try {
	for ( unsigned int ii=0; ii < ValuesArray.size(); ++ii ){
	  FeatureValue *FV_i = (FeatureValue *)ValuesArray[ii];
	  for ( unsigned int jj=0; jj < ValuesArray.size(); ++jj ){
	    FeatureValue *FV_j = (FeatureValue *)ValuesArray[jj];
	    if ( FV_i->ValFreq() >= matrix_clip_freq &&
		 FV_j->ValFreq() >= matrix_clip_freq &&
		 ( Prestored_metric != mt ||
		   fabs(metric_matrix->Extract(FV_i,FV_j)) < Epsilon ) ){
	      double dist;
	      switch ( mt ){
	      case ValueDiff:
		dist = FV_i->VDDistance( FV_j, limit, df );
		break;
	      case JeffreyDiv:
		dist = FV_i->JDDistance( FV_j, limit, df );
		break;
	      case Levenshtein:
		dist = FV_i->LDDistance( FV_j, limit, df );
		break;
	      default:
		FatalError( "invalid value " + toString( mt ) + "in switch" );
	      }
	      metric_matrix->Assign( FV_i, FV_j, dist );
	    }
	  }
	}
      }
      catch( ... ){
	cout << "hit the ground!" << endl;
	PrestoreStatus = ps_failed;
	return false;
      };
      PrestoreStatus = ps_ok;
    }
    if ( PrestoreStatus == ps_ok ){
      Prestored_metric = mt;
    }
    return true;
  }

  double Feature::ValueDistance( FeatureValue *F, FeatureValue *G, 
				 int th ) const {
    double result = 1.0;
    if ( F ){
      if ( matrix_present() &&
	   F->ValFreq() >= matrix_clip_freq &&
	   G->ValFreq() >= matrix_clip_freq )
	result = metric_matrix->Extract( F, G );
      else
	result = F->VDDistance( G, th );
    }
    return result;
  }
  
  double Feature::JeffreyDistance( FeatureValue *F, FeatureValue *G,
				   int th ) const {
    double result = 1.0;
    if ( F ){
      if ( matrix_present() &&
	   F->ValFreq() >= matrix_clip_freq &&
	   G->ValFreq() >= matrix_clip_freq )
	result = metric_matrix->Extract( F, G );
      else
	result = F->JDDistance( G, th );
    }
    return result;
  }
  
  double Feature::LevenshteinDistance( FeatureValue *F, FeatureValue *G,
				       int th ) const {
    double result = 1.0;
    if ( matrix_present() &&
	 F->ValFreq() >= matrix_clip_freq &&
	 G->ValFreq() >= matrix_clip_freq )
      result = metric_matrix->Extract( F, G );
    else
      result = F->LDDistance( G, th );
    return result;
  }  

  ostream& operator<< (std::ostream& os, SparseValueProbClass *VPC ){
    if ( VPC ) {
      int old_prec = os.precision();
      os.precision(3);
      os.setf( std::ios::fixed );
      SparseValueProbClass::IDmaptype::const_iterator it = VPC->vc_map.begin();
      for ( size_t k = 1; k <= VPC->dimension; ++k ){
	os.setf(std::ios::right, std::ios::adjustfield);
	if ( it != VPC->vc_map.end() &&
	     it->first == k ){
	  os << "\t" << it->second;
	  ++it;
	}
	else
	  os << "\t" << 0.0;
      }
      os << setprecision( old_prec );
    }
    else
      os << "(Null SA)";
    return os;
  }
  
  void Feature::print_vc_pb_array( ostream &os ) const {
    VCarrtype::const_iterator it = ValuesArray.begin();
    while ( it != ValuesArray.end() ){
      FeatureValue *FV = (FeatureValue*)*it;
      if ( FV->ValueClassProb ){
	os << FV << FV->ValueClassProb << endl;
      }
      ++it;
    }
  }
  
  bool Feature::read_vc_pb_array( istream &is ){
    const char*p;
    unsigned int Num = 0;
    FeatureValue *FV;
    bool first = true;
    // clear all existing arrays
    VCarrtype::const_iterator it = ValuesArray.begin();
    while ( it != ValuesArray.end() ){
      FV = (FeatureValue*)*it;
      if ( FV->ValueClassProb ){
	delete FV->ValueClassProb;
	FV->ValueClassProb = NULL;
      }
      ++it;
    }
    string name;
    string buf;
    while ( getline( is, buf ) ){
      if ( buf.length() < 8 ){ // "empty" line separates matrices
	break;
      }
      if ( first ){
	p = buf.c_str();
	while ( *p && isspace(*p) ) ++p; // skip whitespace
	while ( *p && !isspace(*p) ) ++p; // skip the featurename
	while ( *p && isspace(*p) ){ // move along counting non space items
	  p++;   // found something non-space
	  Num++; // so increment counter
	  while( *p && !isspace(*p) ) p++; // skip it
	}
	first = false;
      }
      p = buf.c_str();  // restart
      name = "";
      while ( *p && isspace(*p) ) ++p; // skip whitespace
      while ( *p && !isspace(*p) )
	name += *p++;
      FV = Lookup( name );
      if ( !FV ){
	Warning( "Unknown FeatureValue '" + name + "' in file, (skipped) " );
	continue;
      }
      else {
	FV->ValueClassProb = new SparseValueProbClass( Num );
	size_t ui = 0;
	while ( *p && isspace( *p ) ){
	  while ( *p && isspace(*p) ) ++p; // skip trailing whitespace
	  if ( *p ){
	    if ( ui == Num ){
	      FatalError( "Running out range: " + toString<size_t>(ui) );
	      return false;
	    }
	    name = "";
	    while( *p && !isspace( *p ) )
	      name += *p++;
	    double value = 0.0;
	    if ( !stringTo<double>( name, value ) ){
	      Error( "Found illegal value '" + name + "'" );
	      return false;
	    }
	    else if ( value > Epsilon ) {
	      FV->ValueClassProb->Assign( ui, value );
	    }
	    ui++;
	  }
	}
      }
    }
    // check if we've got all the values, assign a default if not so
    it = ValuesArray.begin();
    while ( it != ValuesArray.end() ){
      FV = (FeatureValue *)*it;
      if ( FV->ValueClassProb == NULL ){
	FV->ValueClassProb = new SparseValueProbClass( Num );
      }
      ++it;
    }
    vcpb_read = true;
    return true;
  }

  void Feature::print_matrix( bool shrt ) const {
    //
    // Print the matrix.
    //
    if ( shrt ){
      cout << " a " << ValuesArray.size() << "x" << ValuesArray.size()
	   << " matrix" << endl;
    }
    else {
      int old_prec = cout.precision();
      cout.setf( ios::scientific );
      VCarrtype::const_iterator it1 = ValuesArray.begin();
      while ( it1 != ValuesArray.end() ){
	FeatureValue *FV_i = (FeatureValue *)(*it1);
	cout.width(6);
	cout.setf(ios::left, ios::adjustfield);
	cout << FV_i << ":";
	cout.width(12); 
	cout.precision(3);
	cout.setf(ios::right, ios::adjustfield);
	VCarrtype::const_iterator it2 = ValuesArray.begin();
	while ( it2 !=  ValuesArray.end() ){
	  FeatureValue *FV_j = (FeatureValue *)(*it2);
	  cout.width(12); 
	  cout.precision(3);
	  cout.setf(ios::right, ios::adjustfield);
	  if ( FV_i->ValFreq() < matrix_clip_freq ||
	       FV_j->ValFreq() < matrix_clip_freq )
	    cout << "*";
	  else
	    cout << metric_matrix->Extract(FV_i,FV_j);
	  ++it2;
	} 
	cout << endl;
	++it1;
      } 
      cout << setprecision( old_prec );
    }
  }
  
  TargetValue *Target::add_value( const string& valstr, int freq ){
    unsigned int hash_val = TokenTree->Hash( valstr );
    return add_value( hash_val, freq );
  }
  
  TargetValue *Target::add_value( unsigned int index, int freq ){
    //    cerr << "Add Target index=" << index << endl;
    IVCmaptype::const_iterator it = ValuesMap.find( index );
    if(  it == ValuesMap.end() ){
      //      cerr << "it is NEW " << endl;
      const string& name = TokenTree->ReverseLookup( index );
      //      cerr << "reverse terurns " << name << endl;
      // we want to store the singleton value for this index
      // so we MUST reverse lookup the index
      TargetValue *tv =  new TargetValue( name, index );
      tv->ValFreq( freq );
      //      cerr << "Created: " << tv << endl;
      ValuesMap[index] = tv;
      //      cerr << "and inserted at index " << index << endl;
      ValuesArray.push_back( tv );
    }
    else {
      it->second->IncValFreq( freq );
    }
    return (TargetValue *)ValuesMap[index];
  }
  
  TargetValue *Target::MajorityClass() const {
    ValueClass *result = 0;
    size_t freq = 0;
    VCarrtype::const_iterator it = ValuesArray.begin();
    while ( it != ValuesArray.end() ){
      if ( (*it)->ValFreq() > freq ){
	result = *it;
	freq = result->ValFreq();
      }
      ++it;
    }
    return (TargetValue*)result;
  }

  bool Target::increment_value( TargetValue *TV ){
    bool result = false;
    if ( TV ){
      TV->incr_val_freq();
      result = true;
    }
    return result;
  }
  
  bool Target::decrement_value( TargetValue *TV ){
    bool result = false;
    if ( TV ){
      TV->decr_val_freq();
      result = true;
    }
    return result;
  }
  
  Instance::Instance():
    TV(NULL), sample_weight(0.0) {
  }
  
  Instance::~Instance(){
    clear();
  }
  
  void Instance::clear(){
    for ( unsigned int i=0; i < FV.size(); ++i ){
      if ( FV[i] ){
	if ( FV[i]->isUnknown() ){
	  delete FV[i];
	}
      }
      FV[i] = 0;
    }
    TV = 0;
    sample_weight = 0.0;
  }
  
  void Instance::Init( size_t len ){
    FV.resize( len, 0 );
  }
  
  ostream& operator<<(ostream& os, const Instance *I ){
    if ( I ){
      for ( unsigned int i=0; i < I->FV.size(); ++i )
	os << I->FV[i] << ", ";
      os << I->TV << " " << I->sample_weight;
    }
    else
      os << " Empty Instance";
    return os;
  }

}
