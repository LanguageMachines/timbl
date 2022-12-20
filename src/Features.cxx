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

#include <vector>
#include <iosfwd>
#include <iomanip>
#include <algorithm> // for sort()
#include <numeric> // for accumulate()
#include <cmath> // for fabs()
#include "timbl/Common.h"
#include "timbl/Types.h"
#include "timbl/Metrics.h"
#include "timbl/Matrices.h"
#include "timbl/Instance.h"
#include "ticcutils/Unicode.h"
#include "ticcutils/UniHash.h"

namespace Timbl {

  using namespace std;
  using namespace Common;
  using icu::UnicodeString;

  FeatureValue::FeatureValue( const UnicodeString& value,
			      size_t hash_val ):
    ValueClass( value, hash_val ),
    ValueClassProb( 0 )
  {
  }

  FeatureValue::FeatureValue( const UnicodeString& s ):
    ValueClass( s, 0 ),
    ValueClassProb(0){
    _frequency = 0;
  }

  FeatureValue::~FeatureValue( ){
    delete ValueClassProb;
  }

  Feature::Feature( Hash::UnicodeHash *T ):
    metric_matrix( 0 ),
    TokenTree(T),
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
    n_min (0.0),
    n_max (0.0),
    weight(0.0),
    is_reference(false)
  {}

  Feature::Feature( const Feature& in ): MsgClass( in ){
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
      weight = in.weight;
      values_array = in.values_array;
      reverse_values = in.reverse_values;
      TokenTree = in.TokenTree;
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

  size_t Feature::EffectiveValues() const {
    return count_if( values_array.begin(), values_array.end(),
		     [&]( const FeatureValue* v ){
		       return (v->ValFreq() > 0); } );
  }

  size_t Feature::TotalValues() const {
    return accumulate( values_array.begin(), values_array.end(),
		       0,
		       [&]( size_t r, const FeatureValue *v ){
			 return r + v->ValFreq(); } );
  }

  FeatureValue *Feature::Lookup( const UnicodeString& str ) const {
    FeatureValue *result = NULL;
    unsigned int hash_val = TokenTree->lookup( str );
    if ( hash_val > 0 ) {
      auto const& it = reverse_values.find( hash_val );
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

  FeatureValue *Feature::add_value( size_t hash_val,
				    TargetValue *tv,
				    int freq ){
    auto const& it = reverse_values.find( hash_val );
    if (  it == reverse_values.end() ){
      const UnicodeString& value = TokenTree->reverse_lookup( hash_val );
      //      cerr << "lookup(" << index << ") geeft: " << value << endl;
      // we want to store the singleton value for this index
      // so we MUST reverse lookup the index
      FeatureValue *fv = new FeatureValue( value, hash_val );
      fv->ValFreq( freq );
      reverse_values[hash_val] = fv;
      values_array.push_back( fv );
    }
    else {
      it->second->IncValFreq( freq );
    }
    FeatureValue *result = reverse_values[hash_val];
    if ( tv ){
      result->TargetDist.IncFreq(tv, freq );
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

  struct D_D {
    D_D(): dist(0), value(0.0) {};
    explicit D_D( FeatureValue *fv ): value(0.0) {
      if ( !TiCC::stringTo( fv->name(), value ) ){
	throw( logic_error("called DD with an non-numeric value" ) );
      }
      dist = &fv->TargetDist;
    }
    ClassDistribution *dist;
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
    size_t dd_len = values_array.size();
    ddv.reserve( dd_len );
    for ( const auto& FV : values_array ){
      if ( FV->ValFreq() > 0 ){
	ddv.push_back( new D_D( FV ) );
      }
    }
    sort( ddv.begin(), ddv.end(), dd_less );
    int num_per_bin = (int)floor( (double)dd_len / BinSize);
    size_t rest = dd_len - num_per_bin * BinSize;
    if ( rest ){
      num_per_bin++;
    }
    int jj = 0;
    int cnt = 0;
    for ( const auto& it: ddv ){
      FVBin[jj]->TargetDist.Merge( *it->dist );
      if ( ++cnt >= num_per_bin ){
	++jj;
	if ( --rest == 0 ){
	  --num_per_bin;
	}
	cnt = 0;
      }
    }
    for ( auto const& it: ddv ){
      delete it;
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

  void Feature::Statistics( double DBentropy,
			    const Targets& Targs,
			    bool full ){
    Statistics( DBentropy );
    if ( full ){
      ChiSquareStatistics( Targs );
      SharedVarianceStatistics( Targs, EffectiveValues() );
    }
  }

  void Feature::NumStatistics( double DBentropy,
			       const Targets& Targs,
			       int BinSize,
			       bool full ){
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
    for ( const auto& it : FVBin ){
      delete it;
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
				     const Targets& Targs ){
    size_t Num_Vals = FVA.size();
    chi_square = 0.0;
    long int n_dot_dot = 0;
    size_t Size = Targs.num_of_values();
    n_dot_j.resize(Size,0);
    n_i_dot.resize(Num_Vals,0);
    for ( size_t j = 0; j < Size; ++j ){
      // ALL values should be zeroed
      n_dot_j[j] = 0;
    }
    for ( size_t i = 0; i < Num_Vals; ++i ){
      n_i_dot[i] = 0;      // ALL values should be zeroed
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

  void Feature::ChiSquareStatistics( const Targets& Targs ){
    chi_square = 0.0;
    long int n_dot_dot = 0;
    size_t Size = Targs.num_of_values();
    size_t Num_Vals = values_array.size();
    n_dot_j.resize(Size,0);
    n_i_dot.resize(Num_Vals,0);
    for ( size_t j = 0; j < Size; ++j ){
      // ALL values should be zeroed
      n_dot_j[j] = 0;
    }
    int i = 0;
    for ( const auto& fv : values_array ){
      n_i_dot[i] = 0;       // ALL values should be zeroed
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

  double Feature::fvDistance( FeatureValue *F,
			      FeatureValue *G,
			      size_t limit ) const {
    double result = 0.0;
    if ( F != G ){
      bool dummy;
      if ( metric->isStorable()
	   && matrixPresent( dummy )
	   && F->ValFreq() >= matrix_clip_freq
	   && G->ValFreq() >= matrix_clip_freq ){
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

  Feature_List &Feature_List::operator=( const Feature_List& l ){
    if ( this != &l ){
      _num_of_feats = l._num_of_feats;
      feats.resize(_num_of_feats);
      perm_feats.resize(_num_of_feats);
      permutation = l.permutation;
      _feature_hash = l._feature_hash; // shared ??
      for ( unsigned int i=0; i < _num_of_feats; ++i ){
	feats[i] = new Feature( *l.feats[i] );
      }
      for ( unsigned int i=0; i < _num_of_feats; ++i ){
	if ( l.perm_feats[i] ) {
	  perm_feats[i] = feats[permutation[i]];
	}
	else {
	  perm_feats[i] = 0;
	}
      }
      _is_reference = true;
      _eff_feats = l._eff_feats;
      _num_of_num_feats = l._num_of_num_feats;
    }
    return *this;
  }

  Feature_List::~Feature_List(){
    if ( !_is_reference ){
      delete _feature_hash;
    }
  }

  void Feature_List::init( size_t size,
			   const vector<MetricType>& UserOptions )
  {
    _num_of_feats = size;
    _feature_hash = new Hash::UnicodeHash(); // all features share the same hash
    feats.resize(_num_of_feats,NULL);
    perm_feats.resize(_num_of_feats,NULL);
    for ( size_t i=0; i< _num_of_feats; ++i ){
      feats[i] = new Feature( _feature_hash );
    }
    _eff_feats = _num_of_feats;
    _num_of_num_feats = 0;
    // the user thinks about features running from 1 to _num_of_feats+1
    // we know better, so shift the UserOptions one down.
    for ( size_t j = 0; j < _num_of_feats; ++j ){
      MetricType m = UserOptions[j+1];
      if ( m == Ignore ){
	feats[j]->Ignore( true );
	--_eff_feats;
      }
      else {
	feats[j]->setMetricType( m );
	if ( feats[j]->isNumerical() ){
	  ++_num_of_num_feats;
	}
      }
    }
  }

  void Feature_List::write_permutation( ostream &os ) const {
    os << "< ";
    for ( const auto& it : permutation ){
      os << it + 1;
      if ( &it != &permutation.back())
	os << ", ";
    }
    os << " >";
  }

  void Feature_List::calculate_permutation( const vector<double>& W ){
    vector<double> WR = W;
    size_t IgnoredFeatures = 0;
    permutation.resize(_num_of_feats);
    for ( size_t j=0; j < _num_of_feats; ++j ){
      permutation[j] = j;
      if ( feats[j]->Ignore() ){
	WR[j] = -0.1;         // To be shure that they are placed AFTER
	// those which are realy Zero
	IgnoredFeatures++;
      }
    }
    if ( IgnoredFeatures == _num_of_feats ){
      Error( "All features seem to be ignored! Nothing to do" );
      exit(1);
    }
    else {
      for ( size_t k=0; k < _num_of_feats; ++k ){
	size_t Max = 0;
	for ( size_t m=1; m < _num_of_feats; ++m ){
	  if ( WR[m] > WR[Max] ){
	    Max = m;
	  }
	}
	WR[Max] = -1;
	permutation[k] = Max;
      }
    }
    for ( size_t j=0; j < _num_of_feats; ++j ){
      if ( j < _eff_feats ){
	perm_feats[j] = feats[permutation[j]];
      }
      else {
	perm_feats[j] = NULL;
      }
    }
  }
  Feature::~Feature(){
    if ( !is_reference ){
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
	if ( !TiCC::stringTo( fv->name(), tmp ) ){
	  Warning( "a Non Numeric value '" + fv->s_name() +
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

  void Feature::SharedVarianceStatistics( const Targets& Targ,
					  int eff_cnt ){
    size_t NumInst = Targ.TotalValues();
    int NumCats = Targ.EffectiveValues();
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
      double val = TiCC::stringTo<double>( FV->name() );
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

} // namespace Timbl
