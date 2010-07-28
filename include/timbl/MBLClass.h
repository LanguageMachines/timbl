/*
  Copyright (c) 1998 - 2010
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
#ifndef MBLCLASS_H
#define MBLCLASS_H

typedef struct _xmlNode xmlNode;

namespace Timbl {
  using namespace Common;

  class InstanceBase_base;
  class TesterClass;
  class Chopper;

  class MBLClass {
  public:
    bool SetOption( const std::string& );
    xmlNode *settingsToXml() const;
    bool ShowWeights( std::ostream& ) const;
    bool Verbosity( VerbosityFlags v ) const { 
      return verbosity & v; };
    void SetVerbosityFlag( VerbosityFlags v ) { verbosity |= v; };
    void ResetVerbosityFlag( VerbosityFlags v ) { verbosity &= ~v; };
    bool MBLInit() const { return MBL_init; };
    void MBLInit( bool b ) { MBL_init = b; };
    bool ExpInvalid() const { 
      if ( err_count > 0 ){
	InvalidMessage();
	return true;
      }
      else
	return false;
    };
    WeightType CurrentWeighting() const { return Weighting; };
    InputFormatType InputFormat() const { return input_format; };
    bool connectToSocket( std::ostream * );
    std::ostream *sock_os;
  protected:
    enum PhaseValue { TrainWords, LearnWords, TestWords, TrainLearnWords };
    friend std::ostream& operator<< ( std::ostream&, const PhaseValue& );
    enum IB_Stat { Invalid, Normal, Pruned };

    bool writeArrays( std::ostream& );
    bool readArrays( std::istream& );
    bool writeMatrices( std::ostream& ) const;
    bool readMatrices( std::istream& );
    bool writeWeights( std::ostream& ) const;
    bool readWeights( std::istream&, WeightType );
    bool writeNamesFile( std::ostream& ) const;
    bool ShowOptions( std::ostream& ) const;
    bool ShowSettings( std::ostream& ) const;
    void writePermutation( std::ostream& ) const;
    void LearningInfo( std::ostream& );
    MBLClass( const std::string& = "" );
    virtual ~MBLClass();
    void InitClass( const size_t );
    MBLClass& operator=( const MBLClass& );
    void Initialize( size_t = 0 );
    bool PutInstanceBase( std::ostream& ) const;
    VerbosityFlags get_verbosity() const { return verbosity; };
    void set_verbosity( VerbosityFlags v ) { verbosity = v; };
    const Instance *chopped_to_instance( PhaseValue );
    bool Chop( const std::string& );
    bool HideInstance( const Instance& );
    bool UnHideInstance( const Instance&  );
    std::string formatInstance( const std::vector<FeatureValue *>&,
				std::vector<FeatureValue *>&,
				size_t,	size_t ) const;
    bool setInputFormat( const InputFormatType );
    size_t countFeatures( const std::string&, 
			  const InputFormatType ) const;
    InputFormatType getInputFormat( const std::string& ) const;
    size_t examineData( const std::string& );
    void time_stamp( const char *, int =-1 ) const;
    void TestInstance( const Instance& ,
		       InstanceBase_base * = NULL,
		       size_t = 0 );
    void show_org_input( std::ostream & ) const;
    const ValueDistribution *ExactMatch( const Instance& ) const;
    void fillNeighborSet( neighborSet& ) const;
    void addToNeighborSet( neighborSet& ns, size_t n ) const;
    double getBestDistance() const;
    WValueDistribution *getBestDistribution( unsigned int =0 );
    IB_Stat IBStatus() const;
    bool get_ranges( const std::string& );
    bool get_IB_Info( std::istream&, bool&, int&, bool&, std::string& );
    size_t NumOfFeatures() const { return num_of_features; };
    size_t targetPos() const { return target_pos; };
    size_t NumNumFeatures() const { return num_of_num_features; };
    size_t EffectiveFeatures() const { return effective_feats; };
    void IBInfo( std::ostream&, InstanceBase_base * ) const;
    void IBInfo( std::ostream& os ) const { 
      return IBInfo( os, InstanceBase) ; };
    void MatrixInfo( std::ostream& ) const;
    int RandomSeed() const { return random_seed; };
    void Info( const std::string& ) const;
    void Warning( const std::string& ) const;
    void Error( const std::string& ) const;
    void FatalError( const std::string& ) const;
    size_t MaxFeats() const { return MaxFeatures; };
    int Progress() const { return progress; };
    void Progress( int p ){ progress =  p; };
    Target   *Targets;
    std::vector<Feature *> Features;
    std::vector<Feature *> PermFeatures;
    std::vector<size_t> permutation;
    InstanceBase_base *InstanceBase;
    std::ostream *mylog;
    std::ostream *myerr;
    size_t TRIBL_offset() const { return tribl_offset; };
    unsigned int igOffset() const { return igThreshold; };
    unsigned int IB2_offset() const { return ib2_offset; };
    void IB2_offset( unsigned int n ) { ib2_offset = n; };
    bool Do_Sloppy_LOO() const { return do_sloppy_loo; };
    bool doSamples() const { 
      return do_sample_weighting && !do_ignore_samples; };
    bool Do_Exact() const { return do_exact_match; };
    void Do_Exact( bool b ) { do_exact_match = b; };
    void InitWeights();
    void diverseWeights();
    bool KeepDistributions() const { return keep_distributions; };
    void KeepDistributions( bool f ){ keep_distributions = f; };

    bool IsClone() const { return is_copy; };
    void default_order();
    void set_order(void);
    void calculatePermutation( const std::vector<double>& );
    void  calculate_fv_entropy( bool );
    OptionTableClass Options;
    PhaseValue runningPhase;
    WeightType Weighting;
    metricClass *GlobalMetric;
    OrdeningType TreeOrder;
    size_t num_of_neighbors;
    bool dynamic_neighbors;
    DecayType decay_flag;
    StringHash *TargetStrings;
    StringHash *FeatureStrings;
    std::string exp_name;
    Instance CurrInst;
    BestArray bestArray;
    size_t MaxBests;
    neighborSet nSet;
    decayStruct *decay;
    int beamSize;
    normType normalisation;
    double norm_factor;
    bool is_copy;
    bool is_synced;
    unsigned int ib2_offset;
    int random_seed;
    double decay_alfa;
    double decay_beta;
    bool MBL_init;
    MetricType globalMetricOption;
    bool do_diversify;
    bool initProbabilityArrays( bool );
    void calculatePrestored();
    void initDecay();
    void initTesters();
    Chopper *ChopInput;
    int F_length;
  private:
    size_t MaxFeatures;
    std::vector<MetricType> UserOptions;
    InputFormatType input_format;
    VerbosityFlags verbosity;
    mutable int err_count;
    size_t num_of_features;
    size_t num_of_num_features;
    size_t target_pos;
    size_t effective_feats;
    int clip_factor;
    int Bin_Size;
    int progress;
    size_t tribl_offset;
    unsigned igThreshold;
    int mvd_threshold;
    bool do_sloppy_loo;
    bool do_exact_match;
    bool do_silly_testing;
    bool hashed_trees;
    bool need_all_weights;
    bool do_sample_weighting;
    bool do_ignore_samples;
    bool no_samples_test;
    bool chopExamples() const {
      return do_sample_weighting && 
	!( runningPhase == TestWords && no_samples_test ); }
    bool keep_distributions;
    double DBEntropy;
    void fill_table();
    void InvalidMessage() const ;
    double calculate_db_entropy( Target * );
    void do_numeric_statistics( );
    TesterClass *tester;
    

    void test_instance( const Instance& ,
			InstanceBase_base * = NULL,
			size_t = 0 );
    void test_instance_sim( const Instance& ,
			    InstanceBase_base * = NULL,
			    size_t = 0 );
    
    void test_instance_ex( const Instance&,
			   InstanceBase_base * = NULL,
			   size_t = 0 );

    bool allocate_arrays();

    double RelativeWeight( unsigned int ) const;    
    void writePermSpecial(std::ostream&) const;
    bool read_the_vals( std::istream& );
    MBLClass( const MBLClass& );
  };
 
  inline std::ostream& operator<< ( std::ostream& os, 
				    const MBLClass::PhaseValue& ph ){
    switch( ph ){
    case MBLClass::TrainWords: 
      os << "TrainWords";
      break;
    case MBLClass::LearnWords: 
      os << "LearnWords";
      break;
    case MBLClass::TestWords:
      os << "TestWords";
      break;
    case MBLClass::TrainLearnWords: 
      os << "TrainlearnWords";
      break;
    default:
      os << "unknown phase";
    }
    return os;
  }
  
  bool empty_line( const std::string& , const InputFormatType );
}

#endif // MBLCLASS_H
