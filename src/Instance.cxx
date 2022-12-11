/*
  Copyright (c) 1998 - 2022
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
#include <list>
#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <algorithm> // for sort()
#include <numeric> // for accumulate()
#include <iomanip>
#include <cassert>

#include "ticcutils/StringOps.h"
#include "ticcutils/PrettyPrint.h"
#include "ticcutils/UniHash.h"

#include "timbl/Common.h"
#include "timbl/Types.h"
#include "timbl/Targets.h"
#include "timbl/Instance.h"

using namespace std;
using namespace icu;


namespace Timbl {
  using namespace Common;
  using TiCC::operator<<;

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
    if ( Min == Max ){
      return Min;
    }
    double randnum = (double)rand()/(double)RAND_MAX;
    randnum *= (Max-Min);
    randnum += Min;
    return (int)floor(randnum+0.5);
  }

  void ValueDistribution::clear(){
    for ( const auto& d : distribution ){
      delete d.second;
    }
    distribution.clear();
    total_items = 0;
  }

  double ValueDistribution::Confidence( const TargetValue *tv ) const {
    auto it = find_if( distribution.begin(), distribution.end(),
		       [tv]( const std::pair<const long unsigned int, Timbl::Vfield*>& v ){
			 return v.second->Value() == tv ; } );
    if ( it != distribution.end() ){
      return it->second->Weight();
    }
    return 0.0;
  }

  void ValueDistribution::DistToString( string& DistStr, double minf ) const {
    ostringstream oss;
    oss.setf(ios::showpoint);
    bool first = true;
    oss << "{ ";
    for ( const auto& it : distribution ){
      Vfield *f = it.second;
      if ( f->frequency >= minf ){
	if ( !first ){
	  oss << ", ";
	}
	oss << f->value << " " << double(f->frequency);
	first = false;
      }
    }
    oss << " }";
    DistStr = oss.str();
  }

  void WValueDistribution::DistToString( string& DistStr, double minw ) const {
    ostringstream oss;
    oss.setf(ios::showpoint);
    bool first = true;
    oss << "{ ";
    for( const auto& it : distribution ){
      Vfield *f = it.second;
      if ( abs(f->weight) < minw ){
	continue;
      }
      if ( abs(f->weight) < Epsilon ){
	continue;
      }
      if ( !first ){
	oss << ", ";
      }
      oss << f->value << " " << f->weight;
      first = false;
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
      for ( const auto& it : distribution ){
	Vfield *f = it.second;
	freqs.insert( f->frequency );
      }
      int cnt=0;
      for ( const auto& rit : freqs ){
	if ( ++cnt == beam ) {
	  minw = rit;
	  break;
	}
      }
    }
    DistToString( DistStr, minw );
  }

  void WValueDistribution::DistToStringWW( string& DistStr,
					   int beam ) const {
    double minw = 0.0;
    if ( beam > 0 ){
      std::set<double, dblCmp> wgths;
      for ( const auto& it : distribution ){
	Vfield *f = it.second;
	wgths.insert( f->weight );
      }
      int cnt=0;
      for ( const auto& rit : wgths ){
	if ( ++cnt == beam ) {
	  minw = rit;
	  break;
	}
      }
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

  double ValueDistribution::Entropy() const {
    double entropy = 0.0;
    size_t TotalVals = total_items;
    if ( TotalVals > 0 ){
      // Loop over the classes in the distibution
      for ( const auto& it : distribution ){
	size_t Freq = it.second->Freq();
	if ( Freq > 0 ){
	  double Prob = Freq / (double)TotalVals;
	  entropy += Prob * Log2(Prob);
	}
      }
    }
    return fabs(entropy);
  }

  void WValueDistribution::Normalize() {
    double sum = accumulate( distribution.begin(), distribution.end(),
			     0.0,
			     []( double r, const std::pair<const long unsigned int, Timbl::Vfield*>& v ){
			       return r + v.second->Weight(); } );
    for ( auto& it : distribution ){
      it.second->SetWeight( it.second->Weight() / sum );
    }
  }

  void WValueDistribution::Normalize_1( double factor,
					const Targets& targ ) {
    for ( const auto& val : targ.values_array ){
      // search for val, if not there: add entry with frequency factor;
      // otherwise increment the ExamplarWeight
      size_t id = val->Index();
      auto const& it = distribution.find( id );
      if ( it != distribution.end() ){
	it->second->SetWeight( it->second->Weight() + factor );
      }
      else {
	distribution[id] = new Vfield( val, 1, factor );
      }
    }
    total_items += targ.num_of_values();
    Normalize();
  }

  void WValueDistribution::Normalize_2( ) {
    for ( const auto& d : distribution ){
      d.second->SetWeight( log1p( d.second->Weight() ) );
    }
    Normalize();
  }

  ValueDistribution *ValueDistribution::to_VD_Copy( ) const {
    ValueDistribution *res = new ValueDistribution();
    for ( const auto& d : distribution ){
      size_t key = d.first;
      Vfield *vdf = d.second;
      res->distribution[key] = new Vfield( vdf->Value(),
					   vdf->Freq(),
					   vdf->Freq() );
    }
    res->total_items = total_items;
    return res;
  }

  WValueDistribution *ValueDistribution::to_WVD_Copy() const {
    WValueDistribution *res = new WValueDistribution();
    for ( const auto& d : distribution ){
      size_t key = d.first;
      Vfield *vdf = d.second;
      res->distribution[key] = new Vfield( vdf->Value(),
					   vdf->Freq(),
					   vdf->Freq() );
    }
    res->total_items = total_items;
    return res;
  }

  WValueDistribution *WValueDistribution::to_WVD_Copy( ) const {
    WValueDistribution *result = new WValueDistribution();
    for ( const auto& d : distribution ){
      size_t key = d.first;
      Vfield *vdf = d.second;
      result->distribution[key] = new Vfield( vdf->Value(),
					      vdf->Freq(),
					      vdf->Weight() );
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
    oss << "{ ";
    bool first = true;
    for ( const auto& it : distribution ){
      Vfield *f = it.second;
      if ( f->frequency > 0 ){
	if ( !first ){
	  oss << ", ";
	}
	oss << f->value->Index() << " " << f->frequency;
	first = false;
      }
    }
    oss << " }";
    return oss.str();
  }

  const string WValueDistribution::SaveHashed() const{
    ostringstream oss;
    bool first = true;
    oss << "{ ";
    for ( const auto& it : distribution ){
      Vfield *f = it.second;
      if ( f->frequency > 0 ){
	if ( !first ){
	  oss << ", ";
	}
	oss << f->Value()->Index() << " "
	    << f->frequency << " " << f->weight;
	first = false;
      }
    }
    oss << " }";
    return oss.str();
  }

  //
  // non-hashed variant:
  //

  const string ValueDistribution::Save() const{
    ostringstream oss;
    oss << "{ ";
    bool first = true;
    for ( const auto& it : distribution ){
      Vfield *f = it.second;
      if ( f->frequency > 0 ){
	if ( !first ){
	  oss << ", ";
	}
	oss << f->value << " " << f->frequency;
	first = false;
      }
    }
    oss << " }";
    return oss.str();
  }

  const string WValueDistribution::Save() const{
    ostringstream oss;
    oss << "{ ";
    bool first = true;
    for ( const auto& it : distribution ){
      Vfield *f = it.second;
      if ( f->frequency > 0 ){
	if ( !first ){
	  oss << ", ";
	}
	oss.setf(ios::showpoint);
	oss << f->value << " " << f->frequency << " " << f->weight;
	first = false;
      }
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

  bool ValueDistribution::IncFreq( const TargetValue *val,
				   size_t occ,
				   double ){
    // search for val, if not there: add entry with frequency 'occ';
    // otherwise increment the freqency
    size_t id = val->Index();
    auto const& it = distribution.find( id );
    if ( it != distribution.end() ){
      it->second->IncFreq( occ );
    }
    else {
      distribution[id] = new Vfield( val, occ, 1.0 );
    }
    total_items += occ;
    return true;
  }

  bool WValueDistribution::IncFreq( const TargetValue *val,
				    size_t occ,
				    double sw ){
    // search for val, if not there: add entry with frequency 'occ';
    // otherwise increment the freqency
    // also set sample weight
    size_t id = val->Index();
    auto const& it = distribution.find( id );
    if ( it != distribution.end() ){
      it->second->IncFreq( occ );
    }
    else {
      distribution[id] = new Vfield( val, occ, sw );
    }
    total_items += occ;
    return fabs( distribution[id]->Weight() - sw ) > Epsilon;
  }

  void ValueDistribution::DecFreq( const TargetValue *val ){
    // search for val, if not there, just forget
    // otherwise decrement the freqency
    auto const& it = distribution.find( val->Index() );
    if ( it != distribution.end() ){
      it->second->DecFreq();
      total_items -= 1;
    }
  }

  void ValueDistribution::Merge( const ValueDistribution& VD ){
    for ( const auto& it : VD.distribution ){
      size_t key = it.first;
      Vfield *vd = it.second;
      if ( distribution.find(key) != distribution.end() ){
	distribution[key]->AddFreq( vd->Freq() );
      }
      else {
	// VD might be weighted. But we don't need/want that info here
	// Weight == Freq is more convenient
	distribution[key] = new Vfield( vd->Value(), vd->Freq(),
					vd->Freq() );
      }
    }
    total_items += VD.total_items;
  }

  void WValueDistribution::MergeW( const ValueDistribution& VD,
				   double Weight ){
    for ( const auto& it : VD.distribution ){
      Vfield *vd = it.second;
      size_t key = it.first;
      if ( distribution.find(key) != distribution.end() ){
	distribution[key]->SetWeight( distribution[key]->Weight() + vd->Weight() *Weight );
      }
      else {
	distribution[key] = new Vfield( vd->Value(), 1,
					vd->Weight() * Weight);
      }
    }
    total_items += VD.total_items;
  }

  const TargetValue *ValueDistribution::BestTarget( bool& tie,
						    bool do_rand ) const {
    // get the most frequent target from the distribution.
    // In case of a tie take the one which is GLOBALLY the most frequent,
    // OR (if do_rand) take random one of the most frequents
    // and signal if this ties also!
    const TargetValue *best = NULL;
    tie = false;
    auto It = distribution.begin();
    if ( It != distribution.end() ){
      Vfield *pnt = It->second;
      size_t Max = pnt->Freq();
      if ( do_rand ){
	int nof_best=1, pick=1;
	++It;
	while ( It != distribution.end() ){
	  pnt = It->second;
	  if ( pnt->Freq() > Max ){
	    Max = pnt->Freq();
	    nof_best = 1;
	  }
	  else {
	    if ( pnt->Freq() == Max ){
	      nof_best++;
	    }
	  }
	  ++It;
	}
	tie = ( nof_best > 1 );
	pick = random_number( 1, nof_best );
	It = distribution.begin();
	nof_best = 0;
	while ( It != distribution.end() ){
	  pnt = It->second;
	  if ( pnt->Freq() == Max ){
	    if ( ++nof_best == pick ){
	      return pnt->Value();
	    }
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
	  else {
	    if ( pnt->Freq() == Max ) {
	      tie = true;
	      if ( pnt->Value()->ValFreq() > best->ValFreq() ){
		best = pnt->Value();
	      }
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
    // OR (if do_rand) take random one of the most frequents
    // and signal if this ties also!
    const TargetValue *best = NULL;
    auto It = distribution.begin();
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
	  else {
	    if ( abs(It->second->Weight()- Max) < Epsilon ){
	      nof_best++;
	    }
	  }
	  ++It;
	}
	tie = ( nof_best > 1 );
	pick = random_number( 1, nof_best );
	It = distribution.begin();
	nof_best = 0;
	while ( It != distribution.end() ){
	  if ( abs(It->second->Weight() - Max) < Epsilon ){
	    if ( ++nof_best == pick ){
	      return It->second->Value();
	    }
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
	  else {
	    if ( abs(It->second->Weight() - Max) < Epsilon ) {
	      tie = true;
	      if ( It->second->Value()->ValFreq() > best->ValFreq() ){
		best = It->second->Value();
	      }
	    }
	  }
	  ++It;
	}
	return best;
      }
    }
    return best;
  }

  ostream& operator<<(ostream& os, const ValueDistribution& vd ) {
    string tmp;
    vd.DistToString( tmp );
    os << tmp;
    return os;
  }

  ostream& operator<<(ostream& os, const ValueDistribution *vd ) {
    string tmp = "{null}";
    if ( vd ){
      vd->DistToString( tmp );
    }
    os << tmp;
    return os;
  }

  ValueDistribution *ValueDistribution::read_distribution( istream &is,
							   Targets& Targ,
							   bool do_fr ){
    // read a distribution from stream is into Target
    // if do_f we also adjust the value of Frequency of the Target, which is
    // otherwise 1. Special case when reading the TopDistribution.
    //
    ValueDistribution *result = 0;
    char nextCh;
    is >> nextCh;   // skip {
    if ( nextCh != '{' ){
      throw runtime_error( "missing '{' in distribution string." );
    }
    else {
      int next;
      do {
	size_t freq;
	UnicodeString buf;
	is >> ws >> buf;
	is >> freq;
	TargetValue *target;
	if ( do_fr ){
	  target = Targ.add_value( buf, freq );
	}
	else {
	  target = Targ.Lookup( buf );
	}
	if ( !target ){
	  delete result;
	  result = 0;
	  break;
	}
	next = look_ahead(is);
	if ( next == ',' ){
	  if ( !result ) {
	    result = new ValueDistribution();
	  }
	  result->SetFreq( target, freq );
	  is >> nextCh;
	  next = look_ahead(is);
	}
	else if ( next == '}' ){
	  if ( !result ){
	    result = new ValueDistribution();
	  }
	  result->SetFreq( target, freq );
	}
	else if ( isdigit(next) ){
	  if ( !result ){
	    result = new WValueDistribution();
	  }
	  double sw;
	  is >> sw;
	  result->SetFreq( target, freq, sw );
	  next = look_ahead(is);
	  if ( next == ',' ){
	    is >> nextCh;
	    next = look_ahead(is);
	  }
	}
      } while ( is && next != '}' );
      if ( is ){
	is >> nextCh;   // skip }
      }
      else {
	delete result;
	throw runtime_error( "missing '}' in distribution string." );
      }
    }
    return result;
  }


  ValueDistribution *ValueDistribution::read_distribution_hashed( istream &is,
								  Targets& Targ,
								  bool do_fr ){

    ValueDistribution *result = 0;
    // read a distribution from stream is into Target
    // if do_f we also adjust the value of Frequency of the Target, which is
    // otherwise 1. Special case when reading the TopDistribution.
    //
    char nextCh;
    is >> nextCh;   // skip {
    if ( nextCh != '{' ){
      throw runtime_error( "missing '{' in distribution string." );
    }
    else {
      int next;
      do {
	unsigned int index;
	size_t freq;
	is >> index;
	is >> freq;
	TargetValue *target;
	if ( do_fr ){
	  target = Targ.add_value( index, freq );
	}
	else {
	  target = Targ.ReverseLookup( index );
	}
	if ( !target ){
	  delete result;
	  result = 0;
	  break;
	}
	next = look_ahead(is);
	if ( next == ',' ){
	  if ( !result ){
	    result = new ValueDistribution();
	  }
	  result->SetFreq( target, freq );
	  is >> nextCh;
	  next = look_ahead(is);
	}
	else if ( next == '}' ){
	  if ( !result ){
	    result = new ValueDistribution();
	  }
	  result->SetFreq( target, freq );
	}
	else if ( isdigit(next) ){
	  double sw;
	  is >> sw;
	  if ( !result ){
	    result = new WValueDistribution();
	  }
	  result->SetFreq( target, freq, sw );
	  next = look_ahead(is);
	  if ( next == ',' ){
	    is >> nextCh;
	    next = look_ahead(is);
	  }
	}
      } while ( is && next != '}' );
      if ( is ){
	is >> nextCh;   // skip thr '}'
      }
      else {
	delete result;
	throw runtime_error( "missing '}' in distribution string" );
      }
    }
    return result;
  }


  ostream& operator<<( std::ostream& os, ValueClass const *vc ){
    if ( vc ){
      os << vc->Name();
    }
    else {
      os << "*FV-NF*";
    }
    return os;
  }

  TargetValue::TargetValue( const UnicodeString& value,
			    size_t value_hash ):
    ValueClass( value, value_hash ){}

  size_t Targets::EffectiveValues() const {
    return count_if( values_array.begin(), values_array.end(),
		     [&]( const TargetValue* v ){
		       return (v->ValFreq() > 0); } );
  }

  size_t Targets::TotalValues() const {
    return accumulate( values_array.begin(), values_array.end(),
		       0,
		       [&]( size_t r, const TargetValue *v ){
			 return r + v->ValFreq(); } );
  }

  Targets &Targets::operator=( const Targets& t ){
    if ( this != &t ){
      values_array = t.values_array;
      reverse_values = t.reverse_values;
      target_hash = t.target_hash; // shared ??
      is_reference =true;
    }
    return *this;
  }


  Targets::~Targets() {
    if ( !is_reference ){
      for ( const auto& it : values_array ){
	delete it;
      }
      delete target_hash;
    }
    reverse_values.clear();
  }

  void Targets::init(){
    target_hash = new Hash::UnicodeHash();
  }

  TargetValue *Targets::Lookup( const UnicodeString& str ) const {
    TargetValue *result = 0;
    size_t index = target_hash->lookup( str );
    if ( index ) {
      auto const& it = reverse_values.find( index );
      result = it->second;
    }
    return result;
  }

  TargetValue *Targets::ReverseLookup( size_t index ) const {
    auto const& it = reverse_values.find( index );
    return it->second;
  }

  TargetValue *Targets::add_value( const UnicodeString& valstr, int freq ){
    unsigned int hash_val = target_hash->hash( valstr );
    //    cerr << "target hash(" << valstr << ") geeft: " << hash_val << endl;
    return add_value( hash_val, freq );
  }

  TargetValue *Targets::add_value( size_t index, int freq ){
    auto const& it = reverse_values.find( index );
    if (  it == reverse_values.end() ){
      const UnicodeString& name = target_hash->reverse_lookup( index );
      //      cerr << "target lookup(" << index << ") geeft: " << name << endl;
      // we want to store the singleton value for this index
      // so we MUST reverse lookup the index
      TargetValue *tv = new TargetValue( name, index );
      tv->ValFreq( freq );
      reverse_values[index] = tv;
      values_array.push_back( tv );
    }
    else {
      it->second->IncValFreq( freq );
    }
    return reverse_values[index];
  }

  TargetValue *Targets::MajorityClass() const {
    TargetValue *result = 0;
    size_t freq = 0;
    for ( const auto& it : values_array ){
      if ( it->ValFreq() > freq ){
	result = it;
	freq = result->ValFreq();
      }
    }
    return result;
  }

  bool Targets::increment_value( TargetValue *TV ){
    bool result = false;
    if ( TV ){
      TV->incr_val_freq();
      result = true;
    }
    return result;
  }

  bool Targets::decrement_value( TargetValue *TV ){
    bool result = false;
    if ( TV ){
      TV->decr_val_freq();
      result = true;
    }
    return result;
  }

  Instance::Instance():
    TV(NULL),
    sample_weight(0.0),
    occ(1)
  {
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
    occ = 1;
  }

  void Instance::Init( size_t len ){
    FV.resize( len, 0 );
  }

  ostream& operator<<(ostream& os, const Instance *I ){
    if ( I ){
      os << *I;
    }
    else {
      os << " Empty Instance";
    }
    return os;
  }

  ostream& operator<<(ostream& os, const Instance& I ){
    for ( unsigned int i=0; i < I.FV.size(); ++i ){
      os << I.FV[i] << ", ";
    }
    os << I.TV << " " << I.sample_weight;
    return os;
  }

}
