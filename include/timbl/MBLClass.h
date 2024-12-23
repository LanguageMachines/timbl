/*
  Copyright (c) 1998 - 2024
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
#ifndef TIMBL_MBLCLASS_H
#define TIMBL_MBLCLASS_H

#include "timbl/Instance.h"
#include "timbl/BestArray.h"
#include "timbl/neighborSet.h"
#include "timbl/Options.h"

using xmlNode = struct _xmlNode;

namespace Hash {
  class UnicodeHash;
}

namespace Timbl {
  using namespace Common;

  class InstanceBase_base;
  class TesterClass;
  class Chopper;
  class neighborSet;

  class MBLClass: public MsgClass {
  public:
    bool SetOption( const std::string& );
    xmlNode *settingsToXml() const;
    virtual nlohmann::json settings_to_JSON();
    bool ShowWeights( std::ostream& ) const;
    bool Verbosity( VerbosityFlags v ) const {
      return verbosity & v; };
    void SetVerbosityFlag( VerbosityFlags v ) { verbosity |= v; };
    void ResetVerbosityFlag( VerbosityFlags v ) { verbosity &= ~v; };
    bool MBLInit() const { return MBL_init; };
    void MBLInit( bool b ) { MBL_init = b; };
    bool ExpInvalid( bool b = true ) const {
      if ( err_cnt > 0 ){
	if ( b ){
	  InvalidMessage();
	}
	return true;
      }
      else
	return false;
    };
    WeightType CurrentWeighting() const { return Weighting; };
    InputFormatType InputFormat() const { return input_format; };
    bool connectToSocket( std::ostream *, bool = false );
    std::ostream *sock_os;
    bool sock_is_json;
    mutable nlohmann::json last_error;
    int getOcc() const { return doOcc; };
  protected:
    explicit MBLClass( const std::string& = "" );
    void init_options_table( size_t );
    MBLClass& operator=( const MBLClass& );
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
    virtual bool ShowOptions( std::ostream& );
    virtual bool ShowSettings( std::ostream& );
    void writePermutation( std::ostream& ) const;
    void LearningInfo( std::ostream& );
    virtual ~MBLClass() override;
    void Initialize( size_t );
    bool PutInstanceBase( std::ostream& ) const;
    VerbosityFlags get_verbosity() const { return verbosity; };
    void set_verbosity( VerbosityFlags v ) { verbosity = v; };
    const Instance *chopped_to_instance( PhaseValue );
    bool Chop( const icu::UnicodeString& );
    bool HideInstance( const Instance& );
    bool UnHideInstance( const Instance&  );
    icu::UnicodeString formatInstance( const std::vector<FeatureValue *>&,
				       const std::vector<FeatureValue *>&,
				       size_t,	size_t ) const;
    bool setInputFormat( const InputFormatType );
    size_t countFeatures( const icu::UnicodeString&,
			  const InputFormatType ) const;
    InputFormatType getInputFormat( const icu::UnicodeString& ) const;
    size_t examineData( const std::string& );
    void time_stamp( const char *, int =-1 ) const;
    void TestInstance( const Instance& ,
		       InstanceBase_base * = NULL,
		       size_t = 0 );
    icu::UnicodeString get_org_input( ) const;
    const ClassDistribution *ExactMatch( const Instance& ) const;
    void fillNeighborSet( neighborSet& ) const;
    void addToNeighborSet( neighborSet& ns, size_t n ) const;
    double getBestDistance() const;
    WClassDistribution *getBestDistribution( unsigned int =0 );
    IB_Stat IBStatus() const;
    bool get_ranges( const std::string& );
    size_t get_IB_Info( std::istream&, bool&, int&, bool&, std::string& );
    size_t NumOfFeatures() const { return features._num_of_feats; };
    size_t targetPos() const { return target_pos; };
    size_t NumNumFeatures() const { return features._num_of_num_feats; };
    size_t EffectiveFeatures() const { return features._eff_feats; };
    void IBInfo( std::ostream& os ) const;
    void MatrixInfo( std::ostream& ) const;
    int RandomSeed() const { return random_seed; };
    void Info( const std::string& ) const override;
    void Warning( const std::string& ) const override;
    void Error( const std::string& ) const override;
    void FatalError( const std::string& ) const override;
    size_t MaxFeats() const { return MaxFeatures; };
    int Progress() const { return progress; };
    void Progress( int p ){ progress =  p; };
    std::string extract_limited_m( size_t );
    Targets targets;
    Feature_List features;
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
    bool recalculate_stats( Feature_List&,
			    std::vector<FeatVal_Stat>&,
			    bool );
    OptionTableClass Options;
    PhaseValue runningPhase;
    WeightType Weighting;
    metricClass *GlobalMetric;
    OrdeningType TreeOrder;
    size_t num_of_neighbors;
    bool dynamic_neighbors;
    DecayType decay_flag;
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
    bool tableFilled;
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
    size_t target_pos;
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
    bool keep_distributions;
    double DBEntropy;
    TesterClass *tester;
    int doOcc;
    bool chopExamples() const {
      return do_sample_weighting &&
	!( runningPhase == TestWords && no_samples_test ); }
    bool chopOcc() const {
      switch( runningPhase ) {
      case TrainWords:
      case LearnWords:
      case TrainLearnWords:
	return doOcc == 1 || doOcc == 3;
      case TestWords:
	return doOcc > 1;
      default:
	return false;
      }
    };
    void InvalidMessage() const ;

    void do_numeric_statistics( );

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

  bool empty_line( const icu::UnicodeString& , const InputFormatType );
}

#endif // TIMBL_MBLCLASS_H
