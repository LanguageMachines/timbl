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
#include "ticcutils/UniHash.h"

#include "timbl/Common.h"
#include "timbl/MsgClass.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"
#include "timbl/Matrices.h"
#include "timbl/Metrics.h"

using namespace std;
using namespace icu;

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

  void WValueDistribution::Normalize_1( double factor, const Targets *targ ) {
    for ( const auto& val : targ->values_array ){
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
    total_items += targ->num_of_values();
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

  Feature::Feature( Hash::UnicodeHash *T ):
    BaseFeatTargClass(T),
    metric_matrix( 0 ),
    metric( 0 ),
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
    standard_deviation(0.0),
    matrix_clip_freq(10),
    n_dot_j( 0 ),
    n_i_dot( 0 ),
    n_min (0.0),
    n_max (0.0),
    SaveSize(0),
    SaveNum(0),
    weight(0.0),
    is_reference(false)
  {}

  Feature::Feature( const Feature& in ): BaseFeatTargClass( in ){
    *this = in;
    is_reference = true;
  }

  Feature& Feature::operator=( const Feature& in ){
    if ( this != &in ){
      metric_matrix = in.metric_matrix;
      metric = in.metric;
      PrestoreStatus = in.PrestoreStatus;
      Prestored_metric = in.Prestored_metric;
      ignore = in.ignore;
      numeric = in.numeric;
      vcpb_read = in.vcpb_read;
      entropy = in.entropy;
      info_gain = in.info_gain;
      split_info = in.split_info;
      gain_ratio = in.gain_ratio;
      chi_square = in.chi_square;
      shared_variance = in.shared_variance;
      standard_deviation = in.standard_deviation;
      matrix_clip_freq = in.matrix_clip_freq;
      n_dot_j = in.n_dot_j;
      n_i_dot = in.n_i_dot;
      n_min = in.n_min;
      n_max = in.n_max;
      SaveSize = in.SaveSize;
      SaveNum = in.SaveNum;
      weight = in.weight;
      values_array = in.values_array;
      reverse_values = in.reverse_values;
    }
    return *this;
  }

  void Feature::InitSparseArrays(){
    if ( !is_reference ){
      // Loop over all values.
      //
      for ( const auto& FV : values_array ){
	size_t freq = FV->ValFreq();
	FV->ValueClassProb->Clear();
	if ( freq > 0 ){
	  // Loop over all present classes.
	  //
	  for ( const auto& tit : FV->TargetDist ){
	    FV->ValueClassProb->Assign( tit.second->Index(),
					tit.second->Freq()/(double)freq );
	  }
	}
      }
    }
  }

  struct D_D {
    D_D(): dist(0), value(0.0) {};
    explicit D_D( FeatureValue *fv ): value(0.0) {
      if ( !TiCC::stringTo( fv->Name(), value ) ){
	throw( logic_error("called DD with an non-numeric value" ) );
      }
      dist = &fv->TargetDist;
    }
    ValueDistribution *dist;
    double value;
  };

  bool dd_less( const D_D* dd1, const D_D* dd2 ){
    return dd1->value < dd2->value;
  }

  void Feature::NumStatistics( vector<FeatureValue *>& FVBin,
			       double DBentropy ){
    size_t BinSize = FVBin.size();
    double Prob, FVEntropy;
    size_t TotalVals = TotalValues();
    entropy = 0.0;
    vector<D_D*> ddv;
    ddv.reserve( values_array.size() );
    for ( const auto& FV : values_array ){
      if ( FV->ValFreq() > 0 ){
	ddv.push_back( new D_D( FV ) );
      }
    }
    sort( ddv.begin(), ddv.end(), dd_less );
    size_t dd_len = ddv.size();
    int num_per_bin = (int)floor( (double)dd_len / BinSize);
    size_t rest = dd_len - num_per_bin * BinSize;
    if ( rest ){
      num_per_bin++;
    }
    int jj = 0;
    int cnt = 0;
    for ( size_t m = 0; m < dd_len; ++m ){
      FVBin[jj]->TargetDist.Merge( *ddv[m]->dist );
      if ( ++cnt >= num_per_bin ){
	++jj;
	if ( --rest == 0 ){
	  --num_per_bin;
	}
	cnt = 0;
      }
    }
    for ( size_t j=0; j < dd_len; ++j ){
      delete ddv[j];
    }
    for ( size_t k=0; k < BinSize; k++ ){
      FeatureValue *pnt = FVBin[k];
      size_t Freq = pnt->TargetDist.totalSize();
      pnt->ValFreq( Freq );
      if ( Freq > 0 ){
	// Entropy for this FV pair.
	//
	FVEntropy = 0.0;
	for ( const auto& it : pnt->TargetDist ){
	  Prob = it.second->Freq()/(double)Freq;
	  FVEntropy += Prob * Log2(Prob);
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
    for ( size_t l=0; l < BinSize; ++l ){
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
    else {
      gain_ratio = info_gain / split_info;
    }
  }

  void Feature::Statistics( double DBentropy, Targets *Targs, bool full ){
    Statistics( DBentropy );
    if ( full ){
      ChiSquareStatistics( Targs );
      SharedVarianceStatistics( Targs, EffectiveValues() );
    }
  }

  void Feature::NumStatistics( double DBentropy, Targets *Targs,
			       int BinSize, bool full ){
    char dumname[80];
    vector<FeatureValue *> FVBin(BinSize);
    for ( int i=0; i < BinSize; ++i ){
      sprintf( dumname, "dum%d", i );
      FVBin[i] = new FeatureValue( dumname );
    }
    NumStatistics( FVBin, DBentropy );
    if ( full ){
      ChiSquareStatistics( FVBin, Targs );
      int cnt = 0;   // count effective values in Bin
      for ( int i=0; i < BinSize; ++i ){
	if ( FVBin[i]->ValFreq() > 0 ){
	  ++cnt;
	}
      }
      SharedVarianceStatistics( Targs, cnt );
    }
    for ( int i=0; i < BinSize; ++i ){
      delete FVBin[i];
    }
  }

  void Feature::Statistics( double DBentropy ){
    size_t TotalVals = TotalValues();
    entropy = 0.0;
    // Loop over the values.
    for ( const auto& fv : values_array ){
      // Entropy for this FV pair.
      size_t Freq = fv->ValFreq();
      if ( Freq > 0 ){
	double FVEntropy = 0.0;
	for ( const auto& tit : fv->TargetDist ){
	  double Prob = tit.second->Freq() / (double)Freq;
	  FVEntropy += Prob * Log2(Prob);
	}
	entropy += -FVEntropy * Freq / (double)TotalVals;
      }
    }

    entropy = fabs( entropy );
    // Info. gain.
    //
    info_gain = DBentropy - entropy;
    if ( info_gain < 0.0 ){
      info_gain = 0.0;
    }
    // And the split. info.
    //
    split_info = 0.0;
    for ( const auto& fv : values_array ){
      double Prob = fv->ValFreq() / (double)TotalVals;
      if ( Prob > 0 ) {
	split_info += Prob * Log2(Prob);
      }
    }
    split_info = -split_info;
    // Gain ratio.
    //
    if ( fabs(split_info) < Epsilon ){
      gain_ratio = 0.0;
    }
    else {
      gain_ratio = info_gain / split_info;
    }
  }

  void Feature::ChiSquareStatistics( vector<FeatureValue *>& FVA,
				     Targets *Targs ){
    size_t Num_Vals = FVA.size();
    chi_square = 0.0;
    long int n_dot_dot = 0;
    size_t Size = Targs->num_of_values();
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
    for ( size_t i = 0; i < Num_Vals; ++i ){
      n_i_dot[i] = 0;
      FeatureValue *fv = FVA[i];
      for ( const auto& tit : fv->TargetDist ){
	n_dot_j[tit.second->Index()-1] += tit.second->Freq();
	n_i_dot[i] += tit.second->Freq();
      }
      n_dot_dot += n_i_dot[i];
    }
    if ( n_dot_dot != 0 ){
      for ( size_t m = 0; m < Num_Vals; ++m ){
	FeatureValue *fv = FVA[m];
	size_t n = 0;
	for ( const auto& it : fv->TargetDist ){
	  if ( n >= Size ){
	    break;
	  }
	  while ( n < it.second->Index()-1 ){
	    double tmp = ((double)n_dot_j[n++] * (double)n_i_dot[m]) /
	      (double)n_dot_dot;
	    chi_square += tmp;
	  }
	  if ( n == it.second->Index()-1 ){
	    double tmp = ((double)n_dot_j[n++] * (double)n_i_dot[m]) /
	      (double)n_dot_dot;
	    if ( fabs(tmp) > Epsilon){
	      chi_square += ( (tmp - it.second->Freq()) *
			      (tmp - it.second->Freq()) ) / tmp;
	    }
	  }
	  else {
	    break;
	  }
	}
	while ( n < Size ){
	  double tmp = ((double)n_dot_j[n++] * (double)n_i_dot[m]) /
	    (double)n_dot_dot;
	  chi_square += tmp;
	}
      }
    }
  }

  void Feature::ChiSquareStatistics( Targets *Targs ){
    chi_square = 0.0;
    long int n_dot_dot = 0;
    size_t Size = Targs->num_of_values();
    size_t Num_Vals = values_array.size();
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
    int i = 0;
    for ( const auto& fv : values_array ){
      n_i_dot[i] = 0;
      for ( const auto& t_it : fv->TargetDist ){
	long int fr = t_it.second->Freq();
	n_dot_j[t_it.second->Index()-1] += fr;
	n_i_dot[i] += fr;
      }
      n_dot_dot += n_i_dot[i];
      ++i;
    }
    if ( n_dot_dot != 0 ){
      int m = 0;
      for ( const auto& fv : values_array ){
	size_t n = 0;
	for ( const auto& t_it : fv->TargetDist ){
	  if ( n >= Size ){
	    break;
	  }
	  size_t id = t_it.second->Index()-1;
	  long int fr = t_it.second->Freq();
	  while ( n < id ){
	    double tmp = ((double)n_dot_j[n++] * (double)n_i_dot[m]) /
	      (double)n_dot_dot;
	    chi_square += tmp;
	  }
	  if ( n == id ){
	    double tmp = ((double)n_dot_j[n++] * (double)n_i_dot[m]) /
	      (double)n_dot_dot;
	    if ( fabs(tmp) > Epsilon ){
	      chi_square += ( (tmp - fr ) * (tmp - fr ) ) / tmp;
	    }
	  }
	  else {
	    break;
	  }
	}
	while ( n < Size ){
	  double tmp = ((double)n_dot_j[n++] * (double)n_i_dot[m]) /
	    (double)n_dot_dot;
	  chi_square += tmp;
	}
	++m;
      }
    }
  }

  double Feature::fvDistance( FeatureValue *F, FeatureValue *G,
			      size_t limit ) const {
    double result = 0.0;
    if ( F != G ){
      bool dummy;
      if ( metric->isStorable() && matrixPresent( dummy ) &&
	   F->ValFreq() >= matrix_clip_freq &&
	   G->ValFreq() >= matrix_clip_freq ){
	result = metric_matrix->Extract( F, G );
      }
      else if ( metric->isNumerical() ) {
	result = metric->distance( F, G, limit, Max() - Min() );
      }
      else {
	result = metric->distance( F, G, limit );
      }
    }
    return result;
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
	is >> nextCh;   // skip }
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

  FeatureValue::FeatureValue( const UnicodeString& value,
			      size_t hash_val ):
    ValueClass( value, hash_val ), ValueClassProb( 0 ) {
  }

  FeatureValue::FeatureValue( const UnicodeString& s ):
    ValueClass( s, 0 ),
    ValueClassProb(0){
    Frequency = 0;
  }

  FeatureValue::~FeatureValue( ){
    delete ValueClassProb;
  }

  TargetValue::TargetValue( const UnicodeString& value,
			    size_t value_hash ):
    ValueClass( value, value_hash ){}

  size_t Targets::EffectiveValues() const {
    return count_if( values_array.begin(), values_array.end(),
		     [&]( const TargetValue* v ){
		       return (v->ValFreq() > 0); } );
  }

  size_t Feature::EffectiveValues() const {
    return count_if( values_array.begin(), values_array.end(),
		     [&]( const FeatureValue* v ){
		       return (v->ValFreq() > 0); } );
  }

  size_t Targets::TotalValues() const {
    return accumulate( values_array.begin(), values_array.end(),
		       0,
		       [&]( size_t r, const TargetValue *v ){
			 return r + v->ValFreq(); } );
  }

  size_t Feature::TotalValues() const {
    return accumulate( values_array.begin(), values_array.end(),
		       0,
		       [&]( size_t r, const FeatureValue *v ){
			 return r + v->ValFreq(); } );
  }

  FeatureValue *Feature::Lookup( const UnicodeString& str ) const {
    FeatureValue *result = NULL;
    unsigned int index = TokenTree->lookup( str );
    if ( index ) {
      auto const& it = reverse_values.find( index );
      if ( it != reverse_values.end() ){
	result = it->second;
      }
    }
    return result;
  }

  FeatureValue *Feature::add_value( const UnicodeString& valstr,
				    TargetValue *tv,
				    int freq ){
    unsigned int hash_val = TokenTree->hash( valstr );
    //    cerr << "hash(" << valstr << ") geeft: " << hash_val << endl;
    return add_value( hash_val, tv, freq );
  }

  FeatureValue *Feature::add_value( size_t index,
				    TargetValue *tv,
				    int freq ){
    auto const& it = reverse_values.find( index );
    if (  it == reverse_values.end() ){
      const UnicodeString& value = TokenTree->reverse_lookup( index );
      //      cerr << "lookup(" << index << ") geeft: " << value << endl;
      // we want to store the singleton value for this index
      // so we MUST reverse lookup the index
      FeatureValue *fv = new FeatureValue( value, index );
      fv->ValFreq( freq );
      reverse_values[index] = fv;
      values_array.push_back( fv );
    }
    else {
      it->second->IncValFreq( freq );
    }
    FeatureValue *result = reverse_values[index];
    if ( tv ){
      result->TargetDist.IncFreq(tv, freq );
    }
    return result;
  }

  FeatureValue *Feature_s::Lookup( size_t index,
				   const UnicodeString& str ) const {
    FeatureValue *result = NULL;
    unsigned int hash_val = feature_hash->lookup( str );
    if ( hash_val) {
      auto const& it = features[index]->reverse_values.find( hash_val );
      if ( it != features[index]->reverse_values.end() ){
	result = it->second;
      }
    }
    return result;
  }

  bool Feature::increment_value( FeatureValue *FV,
				 TargetValue *tv ){
    bool result = false;
    if ( FV ){
      FV->incr_val_freq();
      if ( tv ){
	FV->TargetDist.IncFreq(tv,1);
      }
      result = true;
    }
    return result;
  }

  bool Feature::decrement_value( FeatureValue *FV, TargetValue *tv ){
    bool result = false;
    if ( FV ){
      FV->decr_val_freq();
      if ( tv ){
	FV->TargetDist.DecFreq(tv);
      }
      result = true;
    }
    return result;
  }

  bool Feature::AllocSparseArrays( size_t Dim ){
    // Loop over all values.
    //
    for ( const auto& FV : values_array ){
      // Loop over all classes.
      if ( FV->ValueClassProb == NULL ){
	if ( !(FV->ValueClassProb = new SparseValueProbClass( Dim )) ){
	  return false;
	}
      }
    }
    return true;
  }

  bool Feature::isNumerical() const {
    if ( metric && metric->isNumerical() ){
      return true;
    }
    else {
      return false;
    }
  }

  bool Feature::isStorableMetric() const {
    if ( metric && metric->isStorable() ){
      return true;
    }
    else {
      return false;
    }
  }

  BaseFeatTargClass::BaseFeatTargClass( Hash::UnicodeHash *T ):
    TokenTree( T )
  {}

  BaseFeatTargClass::BaseFeatTargClass( const BaseFeatTargClass& in ):
    MsgClass( in ),
    TokenTree( in.TokenTree )
  {}

  BaseFeatTargClass::~BaseFeatTargClass(){
  }

  Targets::~Targets() {
    delete target_hash;
    for ( const auto& it : values_array ){
      delete it;
    }
    reverse_values.clear();
  }

  TargetValue *Targets::Lookup( const UnicodeString& str ) const {
    TargetValue *result = 0;
    size_t index = TokenTree->lookup( str );
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

  Feature::~Feature(){
    if ( !is_reference ){
      if ( n_dot_j ) {
	delete [] n_dot_j;
	delete [] n_i_dot;
      }
      delete_matrix();
      delete metric;
      for ( const auto& it : values_array ){
	delete it;
      }
    }
    reverse_values.clear();
  }

  bool Feature::matrixPresent( bool& isRead ) const {
    isRead = false;
    if ( metric_matrix != 0 ){
      if ( PrestoreStatus == ps_ok ){
	return true;
      }
      else if ( PrestoreStatus == ps_read ){
	isRead = true;
	return true;
      }
    }
    return false;
  }

  size_t Feature::matrix_byte_size() const {
    if ( metric_matrix ){
      return metric_matrix->NumBytes();
    }
    else {
      return 0;
    }
  }

  FeatVal_Stat Feature::prepare_numeric_stats(){
    bool first = true;
    for ( const auto& fv : values_array ){
      size_t freq = fv->ValFreq();
      if ( freq > 0 ){
	double tmp = -1;
	if ( !TiCC::stringTo( fv->Name(), tmp ) ){
	  Warning( "a Non Numeric value '" + fv->Name() +
		   "' in Numeric Feature!" );
	  return NotNumeric;
	}
	if ( first ){
	  first = false;
	  n_min = tmp;
	  n_max = tmp;
	}
	else if ( tmp < n_min ){
	  n_min = tmp;
	}
	else if ( tmp > n_max ){
	  n_max = tmp;
	}
      }
    }
    if ( fabs(n_max - n_min) < Epsilon ){
      return SingletonNumeric;
    }
    else {
      return NumericValue;
    }
  }

  inline int min( int i1, int i2 ) { return (i1>i2?i2:i1); }
  inline size_t min( size_t i1, size_t i2 ) { return (i1>i2?i2:i1); }

  void Feature::SharedVarianceStatistics( Targets *Targ, int eff_cnt ){
    size_t NumInst = Targ->TotalValues();
    int NumCats = Targ->EffectiveValues();
    int k = min( NumCats, eff_cnt ) - 1;
    if ( k == 0 || NumInst == 0 ){
      shared_variance = 0;
    }
    else {
      shared_variance = chi_square / (double)( NumInst * k );
    }
  }

  void Feature::StandardDeviationStatistics( ){
    double sum = 0.0;
    vector<double> store( values_array.size() );
    for ( unsigned int i=0; i < values_array.size(); ++i ){
      FeatureValue *FV = values_array[i];
      double val = TiCC::stringTo<double>( FV->Name() );
      store[i] = val;
      sum += val;
    }
    double total = 0.0;
    for ( unsigned int i=0; i < values_array.size(); ++i ){
      double diff = sum - store[i];
      total += diff*diff;
    }
    standard_deviation = sqrt( total / values_array.size() );
  }

  void Feature::clear_matrix(){
    if ( PrestoreStatus == ps_read ){
      return;
    }
    else {
      delete_matrix();
    }
  }

  void Feature::delete_matrix(){
    if ( metric_matrix ){
      metric_matrix->Clear();
      delete metric_matrix;
    }
    metric_matrix = 0;
    PrestoreStatus = ps_undef;
  }

  bool Feature::setMetricType( const MetricType M ){
    if ( !metric || M != metric->type() ){
      delete metric;
      metric = getMetricClass(M);
      return true;
    }
    else {
      return false;
    }
  }

  MetricType Feature::getMetricType() const { return metric->type(); }

  bool Feature::store_matrix( int limit){
    //
    // Store a complete distance matrix.
    //
    if ( PrestoreStatus == ps_read ){
      return true;
    }
    if ( !metric_matrix ){
      metric_matrix = new SparseSymetricMatrix<ValueClass*>();
    }
    if ( PrestoreStatus != ps_failed && metric->isStorable( ) ) {
      try {
	for ( const auto& FV_i : values_array ){
	  for ( const auto& FV_j : values_array ){
	    if ( FV_i->ValFreq() >= matrix_clip_freq &&
		 FV_j->ValFreq() >= matrix_clip_freq &&
		 ( Prestored_metric != metric->type() ||
		   fabs(metric_matrix->Extract(FV_i,FV_j)) < Epsilon ) ){
	      double dist = metric->distance( FV_i, FV_j, limit );
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
      Prestored_metric = metric->type();
    }
    return true;
  }

  ostream& operator<< (std::ostream& os, SparseValueProbClass *VPC ){
    if ( VPC ) {
      int old_prec = os.precision();
      os.precision(3);
      os.setf( std::ios::fixed );
      auto it = VPC->vc_map.begin();
      for ( size_t k = 1; k <= VPC->dimension; ++k ){
	os.setf(std::ios::right, std::ios::adjustfield);
	if ( it != VPC->vc_map.end() &&
	     it->first == k ){
	  os << "\t" << it->second;
	  ++it;
	}
	else {
	  os << "\t" << 0.0;
	}
      }
      os << setprecision( old_prec );
    }
    else {
      os << "(Null SA)";
    }
    return os;
  }

  void Feature::print_vc_pb_array( ostream &os ) const {
    for ( const auto& FV : values_array ){
      if ( FV->ValueClassProb ){
	os << FV << FV->ValueClassProb << endl;
      }
    }
  }

  bool Feature::read_vc_pb_array( istream &is ){
    unsigned int Num = 0;
    bool first = true;
    // clear all existing arrays
    for ( const auto& FV : values_array ){
      if ( FV->ValueClassProb ){
	delete FV->ValueClassProb;
	FV->ValueClassProb = NULL;
      }
    }
    UnicodeString buf;
    while ( TiCC::getline( is, buf ) ){
      if ( buf.length() < 8 ){ // "empty" line separates matrices
	break;
      }
      vector<UnicodeString> parts = TiCC::split( buf );
      if ( first ){
	Num = parts.size() - 1;
	first = false;
      }
      UnicodeString name = parts[0];
      FeatureValue *FV = Lookup( name );
      if ( !FV ){
	Warning( "Unknown FeatureValue '" + TiCC::UnicodeToUTF8(name)
		 + "' in file, (skipped) " );
	continue;
      }
      else {
	FV->ValueClassProb = new SparseValueProbClass( Num );
	for ( size_t i=0; i < Num; ++i ){
	  UnicodeString tname = parts[i+1];
	  double value;
	  if ( !TiCC::stringTo<double>( tname, value ) ){
	    Error( "Found illegal value '" + TiCC::UnicodeToUTF8(tname) + "'" );
	    return false;
	  }
	  else if ( value > Epsilon ) {
	    FV->ValueClassProb->Assign( i, value );
	  }
	}
      }
    }
    // check if we've got all the values, assign a default if not so
    for ( const auto& FV : values_array ){
      if ( FV->ValueClassProb == NULL ){
	FV->ValueClassProb = new SparseValueProbClass( Num );
      }
    }
    vcpb_read = true;
    return true;
  }

  bool Feature::fill_matrix( istream &is ) {
    if ( !metric_matrix ){
      metric_matrix = new SparseSymetricMatrix<ValueClass*>();
    }
    else {
      metric_matrix->Clear();
    }
    UnicodeString line;
    while ( TiCC::getline(is,line) ){
      if ( line.isEmpty() ){
	break;
      }
      vector<UnicodeString> arr = TiCC::split_at( line, " " );
      size_t num = arr.size();
      double d;
      if ( num != 2 ){
	Error( "wrong line in inputfile" );
	return false;
      }
      else if ( arr[0].length() < 2 ){
	Error( "wrong line in inputfile" );
	return false;
      }
      else if ( !TiCC::stringTo( arr[1], d ) ) {
	Error( "wrong line in inputfile" );
	return false;
      }
      else {
	UnicodeString stripped = UnicodeString( arr[0], 1,arr[0].length()-2) ;
	vector<UnicodeString> parts = TiCC::split_at( stripped, ",\t" );
	if ( parts.size() != 2 ){
	  Error( "wrong line in inputfile" );
	  return false;
	}
	else {
	  FeatureValue *F1 = Lookup(parts[0]);
	  FeatureValue *F2 = Lookup(parts[1]);
	  metric_matrix->Assign( F1, F2, d );
	}
      }
    }
    PrestoreStatus = ps_read;
    return true;
  }

  void Feature::print_matrix( ostream &os, bool full ) const {
    //
    // Print the matrix.
    //
    int old_prec = os.precision();
    ios::fmtflags old_flags = os.flags();
    os.unsetf(std::ios_base::floatfield);
    if ( full ){
      for ( const auto& FV_i : values_array ){
	os.width(6);
	os.setf(ios::left, ios::adjustfield);
	os << FV_i << ":";
	os.width(12);
	os.precision(3);
	os.setf(ios::right, ios::adjustfield);
	for ( const auto& FV_j : values_array ){
	  os.width(12);
	  os.precision(3);
	  os.setf(ios::right,ios::adjustfield );
	  if ( FV_i->ValFreq() < matrix_clip_freq ||
	       FV_j->ValFreq() < matrix_clip_freq ){
	    os << "*";
	  }
	  else {
	    os << metric_matrix->Extract(FV_i,FV_j);
	  }
	}
	os << endl;
      }
    }
    else {
      os << *metric_matrix << endl;
    }
    os << setprecision( old_prec );
    os.flags( old_flags );
  }

  TargetValue *Targets::add_value( const UnicodeString& valstr, int freq ){
    unsigned int hash_val = TokenTree->hash( valstr );
    //    cerr << "target hash(" << valstr << ") geeft: " << hash_val << endl;
    return add_value( hash_val, freq );
  }

  TargetValue *Targets::add_value( size_t index, int freq ){
    auto const& it = reverse_values.find( index );
    if (  it == reverse_values.end() ){
      const UnicodeString& name = TokenTree->reverse_lookup( index );
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

  Feature_s::~Feature_s(){
    //    delete feature_hash;
  }

  Instance::Instance():
    TV(NULL), sample_weight(0.0), occ(1) {
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
