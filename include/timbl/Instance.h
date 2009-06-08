/*
  Copyright (c) 1998 - 2009
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
#ifndef INSTANCE_H
#define INSTANCE_H

#include <stdexcept>
#include <list>
#include <vector>
#include <map>
#include "timbl/MsgClass.h"
#include "timbl/Matrices.h"

namespace Timbl {
  using Hash::StringHash;
  
  enum FeatVal_Stat { Unknown, Singleton, SingletonNumeric, NumericValue,
		      NotNumeric };

  class TargetValue;

  class Vfield{
    friend class ValueDistribution;
    friend class WValueDistribution;
    friend std::ostream& operator<<( std::ostream&, const Vfield& );
    friend std::ostream& operator<<( std::ostream&, const Vfield * );
  public:
    Vfield( const TargetValue *val, int freq, double w ):
      value(val), frequency(freq), weight(w) {};
    Vfield( const Vfield& in ):
      value(in.value), frequency(in.frequency), weight(in.weight) {};
    ~Vfield(){};
    std::ostream& put( std::ostream& ) const;
    const TargetValue *Value() const { return value; };
    void Value( const TargetValue *t ){  value = t; };
    int Freq() const { return frequency; };
    void IncFreq() {  frequency += 1; };
    void AddFreq( int f ) {  frequency += f; weight += f; };
    void DecFreq() {  frequency -= 1; };
    double Weight() const { return weight; };
    void SetWeight( double w ){ weight = w; };
    size_t Index();
  protected:
    const TargetValue *value;
    int frequency;
    double weight;
  private:
    Vfield& operator=( const Vfield& );
  };
  
  class Target;

  class WValueDistribution;

  class ValueDistribution{
    friend std::ostream& operator<<( std::ostream&, const ValueDistribution& );
    friend std::ostream& operator<<( std::ostream&, const ValueDistribution * );
    friend class WValueDistribution;
  public:
    typedef std::map<size_t, Vfield *> VDlist;
    typedef VDlist::const_iterator dist_iterator;
    ValueDistribution( ): total_items(0) {};
    ValueDistribution( const ValueDistribution& );
    virtual ~ValueDistribution(){ clear(); };
    size_t totalSize() const{ return total_items; };
    size_t size() const{ return distribution.size(); };
    bool empty() const{ return distribution.empty(); };
    void clear();
    dist_iterator begin() const { return distribution.begin(); };
    dist_iterator end() const { return distribution.end(); };
    virtual const TargetValue* BestTarget( bool &, bool = false ) const;
    void Merge( const ValueDistribution& );
    virtual void SetFreq( const TargetValue *, const int, const double=1.0 );
    virtual bool IncFreq( const TargetValue *, const double=1.0 );
    void DecFreq( const TargetValue * );
    static ValueDistribution *read_distribution( std::istream &, 
						  Target *, bool );
    static ValueDistribution *read_distribution_hashed( std::istream &, 
							Target *, bool );
    const std::string DistToString() const;
    const std::string DistToStringW( int ) const;
    virtual const std::string ToEncodedString() const;
    virtual const std::string SaveHashed() const;
    virtual const std::string Save() const;
    bool ZeroDist() const { return total_items == 0; };
    double Entropy() const;
    ValueDistribution *to_VD_Copy( ) const;
    virtual WValueDistribution *to_WVD_Copy() const;
  protected:
    virtual void DistToEncodedString( std::string& ) const;
    virtual void DistToString( std::string&, double=0 ) const;
    virtual void DistToStringWW( std::string&, int ) const;
    const TargetValue* BestTargetN( bool &, bool = false ) const;
    const TargetValue* BestTargetW( bool &, bool = false ) const;
    virtual ValueDistribution *clone( ) const { 
      return new ValueDistribution(); };
    size_t total_items;
    VDlist distribution;
  };

  class WValueDistribution: public ValueDistribution {
  public:
    WValueDistribution(): ValueDistribution() {};
    const TargetValue* BestTarget( bool &, bool = false ) const;
    void SetFreq( const TargetValue *, const int, const double );
    bool IncFreq( const TargetValue *, const double );
    WValueDistribution *to_WVD_Copy( ) const;
    const std::string SaveHashed() const;
    const std::string Save() const;
    void Normalize();
    void Normalize_1( double, const Target * );
    void MergeW( const ValueDistribution&, double );
  private:
    void DistToEncodedString( std::string& ) const;
    void DistToString( std::string&, double=0 ) const;
    void DistToStringWW( std::string&, int ) const;
    WValueDistribution *clone() const { 
      return new WValueDistribution; };
  };

  class ValueClass {
  public:
    ValueClass( const std::string& n, unsigned int i ):
      name( n ), index( i ), Frequency( 1 ) {};
    virtual ~ValueClass() {};
    void ValFreq( size_t f ){ Frequency = f; };
    void IncValFreq( int f ){ Frequency += f; };
    size_t ValFreq( ) const { return Frequency; };
    void incr_val_freq(){ Frequency++; };
    void decr_val_freq(){ Frequency--; };
    unsigned int Index() const { return index; };
    const std::string& Name() const { return name; };
    friend std::ostream& operator<<( std::ostream& os, ValueClass const *vc );
  protected:
    const std::string& name;
    unsigned int index;
    size_t Frequency;
    ValueClass( const ValueClass& );
    ValueClass& operator=( const ValueClass& );
  };
  
  class TargetValue: public ValueClass {
  public:
    TargetValue( const std::string&, unsigned int );
  };
  
  class SparseValueProbClass {
    friend std::ostream& operator<< ( std::ostream&, SparseValueProbClass * );
  public:
    typedef std::map< size_t, double > IDmaptype;
    typedef IDmaptype::const_iterator IDiterator;
    SparseValueProbClass( size_t d ): dimension(d) {};
    void Assign( const size_t i, const double d ) { vc_map[i] = d; };
    void Clear() { vc_map.clear(); };
    IDiterator begin() const { return vc_map.begin(); };
    IDiterator end() const { return vc_map.end(); };    
  private:
    IDmaptype vc_map;
    size_t dimension;
  };
  
  class FeatureValue: public ValueClass {
    friend class Feature;
    friend struct D_D;
  public:
    FeatureValue( const std::string& );
    FeatureValue( const std::string&, unsigned int );
    ~FeatureValue();
    void ReconstructDistribution( const ValueDistribution& vd ) { 
      TargetDist.Merge( vd );
      Frequency = TargetDist.totalSize();
    };
    bool isUnknown() const { return index == 0; };
    SparseValueProbClass *valueClassProb() const { return ValueClassProb; };
  private:
    SparseValueProbClass *ValueClassProb;
    ValueDistribution TargetDist;
    FeatureValue( const FeatureValue& );
    FeatureValue& operator=( const FeatureValue& );
  };
    
  typedef std::map< unsigned int, ValueClass *> IVCmaptype;
  typedef std::vector<ValueClass *> VCarrtype;

  class BaseFeatTargClass: public MsgClass {
  public:
    BaseFeatTargClass( int, int, StringHash * );
    virtual ~BaseFeatTargClass();
    size_t EffectiveValues() const;
    size_t TotalValues() const;
    VCarrtype ValuesArray;
    IVCmaptype ValuesMap;
    virtual ValueClass *Lookup( const std::string& ) const = 0;
  protected:
    int CurSize;
    int Increment;
    StringHash *TokenTree;
  private:
    BaseFeatTargClass( const BaseFeatTargClass& );
    BaseFeatTargClass& operator=( const BaseFeatTargClass& );
  };
  
  
  class Target: public BaseFeatTargClass {
  public:
    Target( int a, int b, StringHash *T ): BaseFeatTargClass(a,b,T) {};
    TargetValue *add_value( const std::string&, int freq = 1 );
    TargetValue *add_value( unsigned int, int freq = 1 );
    TargetValue *Lookup( const std::string& ) const;
    TargetValue *ReverseLookup( unsigned int ) const;
    bool decrement_value( TargetValue * );
    bool increment_value( TargetValue * );
    TargetValue *MajorityClass() const;
  };

  class metricClass;

  class Feature: public BaseFeatTargClass {
  public:
    Feature( int a, int b, StringHash *T );
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
    double Min() const { return n_min; };
    void Min( const double val ){ n_min = val; };
    double Max() const { return n_max; };
    void Max( const double val ){ n_max = val; };
    double distance( FeatureValue *, FeatureValue *, size_t=1 ) const;
    FeatureValue *add_value( const std::string&, TargetValue * );
    FeatureValue *add_value( unsigned int, TargetValue * );
    FeatureValue *Lookup( const std::string& ) const ;
    bool decrement_value( FeatureValue *, TargetValue * );
    bool increment_value( FeatureValue *, TargetValue * );
    bool isNumerical() const;
    bool isStorableMetric() const;
    bool AllocSparseArrays( size_t );
    void InitSparseArrays();
    bool ArrayRead(){ return vcpb_read; };
    bool matrix_present( ) const;
    unsigned int matrix_byte_size() const;
    bool store_matrix( int = 1 );
    void delete_matrix();
    void print_matrix( bool s = false ) const;
    void print_vc_pb_array( std::ostream& ) const;
    bool read_vc_pb_array( std::istream &  );
    FeatVal_Stat prepare_numeric_stats();
    void Statistics( double, Target *, bool );
    void NumStatistics( double, Target *, int, bool );
    void ClipFreq( size_t f ){ matrix_clip_freq = f; };  
    size_t ClipFreq() const { return matrix_clip_freq; };
    SparseSymetricMatrix<FeatureValue *> *metric_matrix;
 private:
    metricClass *metric;
    bool ignore;
    bool numeric;
    bool vcpb_read;
    enum ps_stat{ ps_undef, ps_failed, ps_ok };
    enum ps_stat PrestoreStatus;
    MetricType Prestored_metric;
    double entropy;
    double info_gain;
    double split_info;
    double gain_ratio;
    double chi_square;
    double shared_variance;
    size_t matrix_clip_freq;
    long int *n_dot_j;
    long int* n_i_dot;
    double n_min;
    double n_max;
    size_t SaveSize;
    size_t SaveNum;
    double weight;
    void Statistics( double );
    void NumStatistics( std::vector<FeatureValue *>&, double, int );
    void ChiSquareStatistics( std::vector<FeatureValue *>&, size_t, Target * );
    void ChiSquareStatistics( Target * );
    void SharedVarianceStatistics( Target *, int );
    Feature( const Feature& );
    Feature& operator=( const Feature& );
  };
  
  class Instance {
    friend std::ostream& operator<<(std::ostream&, const Instance * );
  public:
    Instance();
    Instance( size_t s ){ Init( s ); };
    ~Instance();
    void Init( size_t );
    void clear();
    double ExemplarWeight() const { return sample_weight; }; 
    void ExemplarWeight( const double sw ){ sample_weight = sw; }; 
    size_t size(){ return FV.size(); };
    std::vector<FeatureValue *> FV;
    TargetValue *TV;
  private:
    double sample_weight; // relative weight
    Instance( const Instance& );
    Instance& operator=( const Instance& );
  };
  
}
#endif
