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
#ifndef TIMBL_FEATURES_H
#define TIMBL_FEATURES_H

#include <stdexcept>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>
#include "unicode/unistr.h"
#include "timbl/MsgClass.h"
#include "ticcutils/Unicode.h"

namespace Hash {
  class UnicodeHash;
}

template<typename T>
class SparseSymetricMatrix;

namespace Timbl {

  class ValueClass;
  class TargetValue;
  class Targets;
  class FeatureValue;
  class metricClass;

  enum FeatVal_Stat { Unknown, Singleton, SingletonNumeric, NumericValue,
    NotNumeric };

  class Feature: public MsgClass {
    friend class MBLClass;
    friend class Feature_List;
  public:
    explicit Feature( Hash::UnicodeHash *T );
    ~Feature();
    bool Ignore() const { return ignore; };
    void Ignore( const bool val ){ ignore = val; };
    bool setMetricType( const MetricType );
    MetricType getMetricType() const;
    double Weight() const { return weight; };
    void SetWeight( const double w ) { weight = w; };
    double InfoGain() const { return info_gain; };
    void InfoGain( const double w ){ info_gain = w; };
    double SplitInfo() const { return split_info; };
    void SplitInfo( const double w ){ split_info = w; };
    double GainRatio() const { return gain_ratio; };
    void GainRatio( const double w ){ gain_ratio = w; };
    double ChiSquare() const { return chi_square; };
    void ChiSquare( const double w ){ chi_square = w; };
    double SharedVariance() const { return shared_variance; };
    void SharedVariance( const double w ){ shared_variance = w; };
    double StandardDeviation() const { return standard_deviation; };
    void StandardDeviation( const double w ){ standard_deviation = w; };
    double Min() const { return n_min; };
    void Min( const double val ){ n_min = val; };
    double Max() const { return n_max; };
    void Max( const double val ){ n_max = val; };
    double fvDistance( FeatureValue *, FeatureValue *, size_t=1 ) const;
    FeatureValue *add_value( const icu::UnicodeString&, TargetValue *, int=1 );
    FeatureValue *add_value( size_t, TargetValue *, int=1 );
    FeatureValue *Lookup( const icu::UnicodeString& ) const;
    bool decrement_value( FeatureValue *, TargetValue * );
    bool increment_value( FeatureValue *, TargetValue * );
    size_t EffectiveValues() const;
    size_t TotalValues() const;
    bool isNumerical() const;
    bool isStorableMetric() const;
    bool AllocSparseArrays( size_t );
    void InitSparseArrays();
    bool ArrayRead(){ return vcpb_read; };
    bool matrixPresent( bool& ) const;
    size_t matrix_byte_size() const;
    bool store_matrix( int = 1 );
    void clear_matrix();
    bool fill_matrix( std::istream& );
    void print_matrix( std::ostream&, bool = false ) const;
    void print_vc_pb_array( std::ostream& ) const;
    bool read_vc_pb_array( std::istream &  );
    FeatVal_Stat prepare_numeric_stats();
    void Statistics( double, const Targets&, bool );
    void NumStatistics( double, const Targets&, int, bool );
    void ClipFreq( size_t f ){ matrix_clip_freq = f; };
    size_t ClipFreq() const { return matrix_clip_freq; };
    SparseSymetricMatrix<ValueClass *> *metric_matrix;
  private:
    Feature( const Feature& );
    Feature& operator=( const Feature& );
    Hash::UnicodeHash *TokenTree;
    metricClass *metric;
    bool ignore;
    bool numeric;
    bool vcpb_read;
    enum ps_stat{ ps_undef, ps_failed, ps_ok, ps_read };
    enum ps_stat PrestoreStatus;
    MetricType Prestored_metric;
    void delete_matrix();
    double entropy;
    double info_gain;
    double split_info;
    double gain_ratio;
    double chi_square;
    double shared_variance;
    double standard_deviation;
    size_t matrix_clip_freq;
    long int *n_dot_j;
    long int* n_i_dot;
    double n_min;
    double n_max;
    size_t SaveSize;
    size_t SaveNum;
    double weight;
    void Statistics( double );
    void NumStatistics( std::vector<FeatureValue *>&, double );
    void ChiSquareStatistics( std::vector<FeatureValue *>&, const Targets& );
    void ChiSquareStatistics( const Targets& );
    void SharedVarianceStatistics( const Targets&, int );
    void StandardDeviationStatistics();
    std::vector<FeatureValue *> values_array;
    std::unordered_map< size_t, FeatureValue *> reverse_values;
    bool is_reference;
  };

  class Feature_List: public MsgClass {
  friend class MBLClass;
  public:
    Feature_List():
      _eff_feats(0),
      _num_of_feats(0),
      _num_of_num_feats(0),
      _feature_hash(0),
      _is_reference(false)
    {
    }
    explicit Feature_List( Hash::UnicodeHash *hash ):
      Feature_List()
    {
      _feature_hash = hash;
    }
    Feature_List &operator=( const Feature_List& );
    ~Feature_List();
    void init( size_t, const std::vector<MetricType>& );
    Hash::UnicodeHash *hash() const { return _feature_hash; };
    size_t effective_feats(){ return _eff_feats; };
    Feature *operator[]( size_t i ) const { return feats[i]; };
    void write_permutation( std::ostream & ) const;
    void calculate_permutation( const std::vector<double>& );
    size_t _eff_feats;
    size_t _num_of_feats;
    size_t _num_of_num_feats;
    std::vector<Feature *> feats;
    std::vector<Feature *> perm_feats;
    std::vector<size_t> permutation;
  private:
    Hash::UnicodeHash *_feature_hash;
    bool _is_reference;
  };

} // namespace Timbl

#endif // TIMBL_FEATURES_H
