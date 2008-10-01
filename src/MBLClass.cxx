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
#include <vector>
#include <set>
#include <fstream>
#include <string>
#include <limits>
#include <sstream>
#include <iomanip>
#include <typeinfo>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <climits>
#include <cfloat>
#include <cctype>
#include <cassert>

#include "timbl/MsgClass.h"
#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/Tree.h"
#include "timbl/Instance.h"
#include "timbl/neighborSet.h"
#include "timbl/Statistics.h"
#include "timbl/IBtree.h"
#ifdef USE_LOGSTREAMS
#include "timbl/LogStream.h"
#else
typedef std::ostream LogStream;
#define Log(X) (X)
#define Dbg(X) (X)
#endif
#include "timbl/SocketBasics.h"
#include "timbl/BestArray.h"
#include "timbl/Testers.h"
#include "timbl/Choppers.h"
#include "timbl/MBLClass.h"

using namespace std;

namespace Timbl {

  const int DefaultTargetNumber  = 20;
  const int DefaultFeatureNumber = 50;
  const int TargetIncrement      = 50;
  const int FeatureIncrement     = 100;  

  void MBLClass::fill_table(){
    Options.Add( new IntegerOption( "FLENGTH",
				    &F_length, 0, 1, 32 ) );
    Options.Add( new SizeOption( "MAXBESTS", 
				 &MaxBests, 500, 10, 100000 ) );
    Options.Add( new SizeOption( "TRIBL_OFFSET", 
				 &tribl_offset, 0, 0, MaxFeatures ) );
    Options.Add( new UnsignedOption( "IG_THRESHOLD", 
				     &igThreshold, 
				     1000, 0,
				     std::numeric_limits<unsigned>::max() ) );
    Options.Add( new InputFormatOption( "INPUTFORMAT",
					&input_format, 
					UnknownInputFormat ) ); 
    Options.Add( new OrdeningOption( "TREE_ORDER",
				     &TreeOrder, UnknownOrdening ) );
    Options.Add( new BoolOption( "ALL_WEIGHTS", 
				 &need_all_weights, false ) );
    Options.Add( new WeightOption( "WEIGHTING",
				   &Weighting, GR_w ) );
    Options.Add( new IntegerOption( "BIN_SIZE", 
				    &Bin_Size, 20, 2, 10000 ) );
    Options.Add( new UnsignedOption( "IB2_OFFSET", 
				    &ib2_offset, 0, 1, 10000000 ) );
    Options.Add( new BoolOption( "KEEP_DISTRIBUTIONS", 
				 &keep_distributions, false ) );
    Options.Add( new BoolOption( "DO_SLOPPY_LOO", 
				 &do_sloppy_loo, false ) );
    Options.Add( new SizeOption( "TARGET_POS", 
				 &target_pos, 
				 std::numeric_limits<size_t>::max(),
				 0, MaxFeatures ) );
    Options.SetFreezeMark();
    Options.Add( new BoolOption( "DO_SILLY", 
				 &do_silly_testing, false ) );
    Options.Add( new BoolOption( "DO_DIVERSIFY", 
				 &do_diversify, false ) );
    Options.Add( new DecayOption( "DECAY",
				  &decay_flag, Zero ) ); 
    Options.Add( new IntegerOption( "SEED",
				    &random_seed, -1, -1, RAND_MAX ) );
    Options.Add( new IntegerOption( "BEAM_SIZE", 
				    &beamSize, 0, 1, INT_MAX ) );
    Options.Add( new RealOption( "DECAYPARAM_A", 
				 &decay_alfa, 1.0, 0.0, DBL_MAX ) );
    Options.Add( new RealOption( "DECAYPARAM_B", 
				 &decay_beta, 1.0, 0.0, DBL_MAX ) );
    Options.Add( new NormalisationOption( "NORMALISATION",
					  &normalisation, noNorm ) );
    Options.Add( new RealOption( "NORM_FACTOR",
				 &norm_factor, 1.0, Epsilon, DBL_MAX ) );
    Options.Add( new BoolOption( "EXEMPLAR_WEIGHTS", 
				 &do_sample_weighting, false ) );
    Options.Add( new BoolOption( "IGNORE_EXEMPLAR_WEIGHTS", 
				 &do_ignore_samples, true ) );
    Options.Add( new BoolOption( "NO_EXEMPLAR_WEIGHTS_TEST", 
				 &no_samples_test, true ) );
    Options.Add( new VerbosityOption( "VERBOSITY", 
				      &verbosity, NO_VERB ) );
    Options.Add( new BoolOption( "EXACT_MATCH", 
				 &do_exact_match, false ) );
    Options.Add( new BoolOption( "DO_DOT_PRODUCT", 
				 &do_dot_product, false ) );
    Options.Add( new BoolOption( "DO_COSINE", 
				 &do_cos_metric, false ) );
    Options.Add( new BoolOption( "HASHED_TREE", 
				 &hashed_trees, true ) );
    Options.Add( new MetricOption( "GLOBAL_METRIC", 
				   &GlobalMetric, Overlap ) );
    Options.Add( new MetricArrayOption( "METRICS", 
					UserOptions, DefaultMetric, 
					MaxFeatures+1 ) );
    Options.Add( new IntegerOption( "MVD_LIMIT", 
				    &mvd_threshold, 1, 1, 100000 ) );
    Options.Add( new MetricOption( "MVD_DEFAULT_METRIC", 
				   &mvdDefaultMetric, Overlap ) );
    Options.Add( new SizeOption( "NEIGHBORS", 
				 &num_of_neighbors, 1, 1, 5000 ) );
    Options.Add( new IntegerOption( "PROGRESS", 
				    &progress, 10000, 1, INT_MAX ) );
    if ( !Options.Add( new IntegerOption( "CLIP_FACTOR", 
					  &clip_factor, 10, 0, 1000000 ) ) ){
      Error( "Too many options for OptionTable" );
    }
  }

  void MBLClass::InvalidMessage(void) const{
    if ( err_count++ == 1 )
      Warning( "A preceding error prevents any operation on this "
	       "Timbl Object\n"
	       "other experiments might not be influenced" );
    else 
      Warning( "This Experiment is invalid due to errors" );
  }
  
  bool MBLClass::SetOption( const string& line ){ 
    bool result = false;
    if ( !ExpInvalid() ){
      enum SetOptRes opt_res = Options.SetOption( line );
      switch ( opt_res ){
      case Opt_OK: // OK
	MBL_init = false; // To assure redoing initializing stuff
	result = true;
	break;
      case Opt_Frozen:
	Warning( "SetOption '" + line + "' ignored.\nThis option may not "
		 "be changed after an InstanceBase is already created" );
	break;
      case Opt_Unknown:
	Warning( "SetOption '" + line + "' failed.\nOption unknown" );
	break;
      case Opt_Ill_Val:
	Warning( "SetOption '" + line + 
		 "' failed.\nillegal value for this option" );
	break;
      }
    }
    return result;
  }
  
  void MBLClass::InitClass( const size_t Size ){
    is_copy = false;
    is_synced = false;
    tcp_socket = 0;
    Targets   = NULL;
    err_count = 0;
    MBL_init = false;
    need_all_weights = false;
    InstanceBase = NULL;
    TargetStrings = NULL;
    FeatureStrings = NULL;
    num_of_features = 0;
    target_pos = std::numeric_limits<size_t>::max();
    mvd_threshold = 1;
    mvdDefaultMetric = Overlap;
    effective_feats = 0;
    num_of_num_features = 0;
    DBEntropy = 0.0;
    ChopInput = 0;
    MaxFeatures = Size;
    runningPhase = LearnWords;
    do_sparse = false;
    do_sloppy_loo = false;
    do_silly_testing = false;
    do_diversify = false;
    keep_distributions = false;
    Permutation = new size_t[MaxFeatures];
    for ( size_t i=0; i< MaxFeatures; ++i ){
      Permutation[i] = i;
    }
    UserOptions.resize(MaxFeatures+1);
    tester = 0;
    fill_table();
    decay = 0;
#ifndef USE_LOGSTREAMS
    myerr = &cerr;
    mylog = &cout;
    mydebug = &cerr;
#else
    myerr = new LogStream( cerr, NULL, NoStamp );
    mylog = new LogStream( cout, NULL, NoStamp );
    mydebug = new LogStream( cout, "Debug", StampBoth );
    //    mylog->setlevel(LogSilent); 
    //    mydebug->settreshold(LogHeavy); 
#endif
    server_verbosity = &verbosity;
  }

  MBLClass::MBLClass( const string& name ){
    exp_name = name;
  }

  MBLClass &MBLClass::operator=( const MBLClass& m ){
    if ( this != &m ){
      is_copy = true;
      is_synced = false;
      MaxFeatures        = m.MaxFeatures;
      UserOptions.resize(MaxFeatures+1);
      fill_table();
      F_length           = m.F_length;
      MaxBests           = m.MaxBests;
      TreeOrder          = m.TreeOrder;
      decay_flag         = m.decay_flag;
      input_format       = m.input_format;
      random_seed        = m.random_seed;
      decay_alfa         = m.decay_alfa;
      decay_beta         = m.decay_beta;
      normalisation      = m.normalisation;
      do_sample_weighting = m.do_sample_weighting;
      do_ignore_samples  = m.do_ignore_samples;
      no_samples_test    = m.no_samples_test;
      keep_distributions = m.keep_distributions;
      verbosity          = m.verbosity;
      do_exact_match     = m.do_exact_match;
      do_dot_product     = m.do_dot_product;
      do_cos_metric          = m.do_cos_metric;
      GlobalMetric       = m.GlobalMetric;
      for ( size_t i=0; i < MaxFeatures+1; ++i )
	UserOptions[i]  = m.UserOptions[i];
      mvd_threshold       = m.mvd_threshold;
      mvdDefaultMetric   = m.mvdDefaultMetric;
      num_of_neighbors   = m.num_of_neighbors;
      dynamic_neighbors  = m.dynamic_neighbors;
      num_of_features    = m.num_of_features;
      target_pos         = m.target_pos;
      progress           = m.progress;
      tribl_offset       = m.tribl_offset;
      ib2_offset         = m.ib2_offset;
      runningPhase       = m.runningPhase;
      Weighting          = m.Weighting;
      do_sparse          = m.do_sparse;
      do_sloppy_loo      = m.do_sloppy_loo;
      do_silly_testing   = m.do_silly_testing;
      do_diversify       = m.do_diversify;
      Permutation = new size_t[MaxFeatures];
      for ( size_t j=0; j < MaxFeatures; ++j ){
	Permutation[j] = m.Permutation[j];
      }
      tester = 0;
      decay = 0;
      Features  = m.Features;
      PermFeatures = m.PermFeatures;
      Targets   = m.Targets;
      err_count = 0;
      MBL_init = false;
      need_all_weights = false;
      InstanceBase = m.InstanceBase->Clone();
      TargetStrings = m.TargetStrings;
      FeatureStrings = m.FeatureStrings;
      effective_feats = m.effective_feats;
      num_of_num_features    = m.num_of_num_features;
      DBEntropy = 0.0;
      ChopInput = 0;
      setInputFormat( m.input_format );
      //one extra to store the target!
      CurrInst.Init( num_of_features );
#ifndef USE_LOGSTREAMS
      myerr = m.myerr;
      mylog = m.mylog;
      mydebug = m.mydebug;
#else
      myerr = new LogStream( m.myerr );
      mylog = new LogStream( m.mylog );
      mydebug = new LogStream( m.mydebug );
#endif
      server_verbosity = &m.verbosity;
    }
    return *this;
  }

  MBLClass::~MBLClass(){
    CurrInst.clear();
    if ( !is_copy ){
      delete InstanceBase;
      for ( unsigned int i=0; i < Features.size(); ++i ){
	delete Features[i];
      }
      delete Targets;
      delete TargetStrings;
      delete FeatureStrings;
    }
    else if ( is_synced ){
      delete InstanceBase;
    }
    else {
      InstanceBase->CleanPartition();
    }
    delete tester;
    delete [] Permutation;
    delete decay;
    delete ChopInput;
#ifndef USE_LOGSTREAMS
#else
    delete mylog;
    delete myerr;
    delete mydebug;
#endif
  }

#ifdef PTHREADS
  using SocketProcs::write_line;
  
  void MBLClass::Info( const string& out_line ) const {
    // Info NEVER to socket !
    if ( exp_name != "" )
      *Log(mylog) << "-" << exp_name << "-" << out_line << endl;
    else
      *Log(mylog) << out_line << endl;
 }
  
  void MBLClass::Warning( const string& out_line ) const {
    if ( Socket() )
      write_line( Socket(), "ERROR { " ) &&
	write_line( Socket(), out_line ) &&
	write_line( Socket(), " }\n" );
    else {
      if ( exp_name != "" )
	*Log(myerr) << "Warning:-" << exp_name << "-" << out_line << endl;
      else 
	*Log(myerr) << "Warning:" << out_line << endl;
    }
  }
  
  void MBLClass::Error( const string& out_line ) const {
    if ( Socket() )
      write_line( Socket(), "ERROR { " ) &&
	write_line( Socket(), out_line ) &&
	write_line( Socket(), " }\n" );
    else {
      if ( exp_name != "" )
	*Log(myerr) << "Error:-" << exp_name << "-" << out_line << endl;
      else
	*Log(myerr) << "Error:" << out_line << endl;
    }
    err_count++;
  }
  
  void MBLClass::FatalError( const string& out_line ) const {
    if ( Socket() )
      write_line( Socket(), "ERROR { " ) &&
	write_line( Socket(), out_line ) &&
	write_line( Socket(), " }\n" );
    else {
      if ( exp_name != "" )
	*Log(myerr) << "-" << exp_name << "-";
      *Log(myerr) << out_line << endl;
      if ( exp_name != "" )
	*Log(myerr) << "Error:-" << exp_name << "-" << out_line
		    << "stopped" << endl;
      else 
	*Log(myerr) << "Error:-" << out_line << "stopped" << endl;
      throw( "Stopped" );
    }
  }
  
  bool MBLClass::ShowOptions( ostream& os ) const{
    bool result = true;
    if ( is_copy ){
      result = write_line( tcp_socket, "STATUS\n" );
    }
    else
      os << "Possible Experiment Settings (current value between []):" 
	 << endl;
    if ( result ){
      ostringstream tmp;
      Options.Show_Options( tmp );
      string tmp_s = tmp.str();
      if ( is_copy )
	result = write_line( tcp_socket, tmp_s ) &&
	  write_line( tcp_socket, "ENDSTATUS\n" );
      else
	os << tmp_s << endl;
    }
    return result;
  }
  
  bool MBLClass::ShowSettings( ostream& os ) const{
    bool result = true;
    if ( is_copy ){
      result = write_line( tcp_socket, "STATUS\n" );
    }
    else
      os << "Current Experiment Settings :" << endl;
    if ( result ){
      ostringstream tmp;
      Options.Show_Settings( tmp );
      string tmp_s = tmp.str();
      if ( is_copy )
	result = write_line( tcp_socket, tmp_s ) &&
	  write_line( tcp_socket, "ENDSTATUS\n" );
      else
	os << tmp_s << endl;
    }
    return result;
  }
  
#else
  
  
  void MBLClass::Info( const string& out_line ) const {
    if ( exp_name != "" )
      *Log(mylog) << "-" << exp_name << "-" << out_line << endl;
    else
      *Log(mylog) << out_line << endl;
  }
  
  void MBLClass::Warning( const string& out_line ) const {
    if ( exp_name != "" )
      *Log(myerr) << "Warning:-" << exp_name << "-" << out_line << endl;
    else 
      *Log(myerr) << "Warning:" << out_line << endl;
  }
  
  void MBLClass::Error( const string& out_line ) const {
    if ( exp_name != "" )
      *Log(myerr) << "Error:-" << exp_name << "-" << out_line << endl;
    else
      *Log(myerr) << "Error:" << out_line << endl;
    err_count++;
  }
  
  void MBLClass::FatalError( const string& out_line ) const {
    if ( exp_name != "" )
      *Log(myerr) << "Error:-" << exp_name << "-" << out_line
		  << "stopped" << endl;
    else 
      *Log(myerr) << "Error:-" << out_line << "stopped" << endl;
    throw( "Stopped" );
  }
  
  bool MBLClass::ShowOptions( ostream& os ) const {
    os << "Possible Experiment Settings (current value between []):" << endl;
    Options.Show_Options( os );
    os << endl;
    return true;
  }
 
  bool MBLClass::ShowSettings( ostream& os ) const {
    os << "Current Experiment Settings :" << endl;
    Options.Show_Settings( os );
    os << endl;
    return true;
  }
  
#endif // PTHREADS
  
  bool MBLClass::ShowWeights( ostream &os ) const {
    if ( ExpInvalid() )
      return false;
    else { 
      int OldPrec = os.precision(DBL_DIG);
      for ( size_t i=0; i< num_of_features; ++i ){
	os.precision(DBL_DIG);
	os << "Feature " << i+1 << "\t : " 
	   << Features[i]->Weight() << endl;
      }
      os.precision(OldPrec);
    }
    return true;
  }
  
  void MBLClass::calc_perm( double *W ){
    double *WR = new double[num_of_features];
    for ( size_t i=0; i< num_of_features; ++i ) 
      WR[i] = W[i];
    size_t IgnoredFeatures = 0;
    for ( size_t j=0; j < num_of_features; ++j ){
      Permutation[j] = j;
      if ( Features[j]->Ignore() ){
	WR[j] = -0.1;         // To be shure that they are placed AFTER
	// those which are realy Zero
	IgnoredFeatures++;
      }
    }
    if ( IgnoredFeatures == num_of_features ){
      Error( "All features seem to be ignored! Nothing to do" );
    }
    else {
      for ( size_t k=0; k < num_of_features; ++k ){
	size_t Max = 0;
	for ( size_t m=1; m < num_of_features; ++m ){
	  if ( WR[m] > WR[Max] )
	    Max = m;
	}
	WR[Max] = -1;
	Permutation[k] = Max;
      }
    }
    delete [] WR;
  }
  
  void MBLClass::writePermutation( ostream& os ) const {
    os << "Feature Permutation based on " 
       << ( Weighting==UserDefined_w?"weightfile":toString(TreeOrder, true))
       << " :" << endl << "< ";
    for ( size_t j=0; j < num_of_features-1; ++j ){
      os << Permutation[j]+1 << ", ";
    }
    os << Permutation[num_of_features-1]+1 << " >" << endl;
  }
  
  inline char *CurTime(){
    time_t lTime;
    struct tm *curtime;
    char *time_string;
    time(&lTime);
    curtime = localtime(&lTime);
    time_string = asctime(curtime);
    time_string[24] = '\0'; // defeat the newline!
    return time_string;
  }
  
  void MBLClass::time_stamp( const char *line, int number ) const {
    if ( !Verbosity(SILENT) ){
      ostringstream ostr;
      ostr << line;
      if ( number > -1 ){
	ostr.width(6);
	ostr.setf(ios::right, ios::adjustfield);
	ostr << number << " @ ";
      }
      else
	ostr << "        ";
      ostr << CurTime();
      Info( ostr.str() );
    }
  }
  
  void MBLClass::InitWeights(void){
    for ( size_t i=0; i< num_of_features; ++i ){
      if ( Features[i]->Ignore() )
	Features[i]->SetWeight( 0.0 );
      else
	switch ( Weighting ){
	case IG_w:
	  Features[i]->SetWeight( Features[i]->InfoGain() );
	  break;
	case GR_w:
	  Features[i]->SetWeight( Features[i]->GainRatio() );
	  break;
	case X2_w:
	  Features[i]->SetWeight( Features[i]->ChiSquare() );
	  break;
	case SV_w:
	  Features[i]->SetWeight( Features[i]->SharedVariance() );
	  break;
	case UserDefined_w:
	  break;
	case No_w:
	  Features[i]->SetWeight( 1.0 );
	  break;
	case Unknown_w:
	case Max_w:
	  FatalError( "InitWeights: Invalid Weight in switch: " +
		      toString( Weighting ) );
	  break;
	}
    }
  }

  void MBLClass::diverseWeights(void){
    double minW = DBL_MAX;
    for ( size_t i=0; i< num_of_features; ++i ){
      if ( Features[i]->Ignore() )
	continue;
      if ( Features[i]->Weight() < minW ){
	minW =  Features[i]->Weight();
      }
    }
    for ( size_t i=0; i< num_of_features; ++i ){
      if ( Features[i]->Ignore() )
	continue;
      Features[i]->SetWeight( (Features[i]->Weight() - minW ) + Epsilon );
    }
  }

  void MBLClass::default_order(){
    if ( TreeOrder == UnknownOrdening )
      switch ( Weighting ){
      case GR_w:
	TreeOrder = GROrder;
	break;
      case IG_w:
	TreeOrder = IGOrder;
	break;
      case X2_w:
	TreeOrder = X2Order;
	break;
      case SV_w:
	TreeOrder = SVOrder;
	break;
      case No_w:
	TreeOrder = NoOrder;
	break;
      case UserDefined_w:
	TreeOrder = GROrder;
	break;
      default:
	FatalError( "Illegal Weighting Value in Switch: " +
		    toString( Weighting ) );
	break;
      }
  }
  
  void MBLClass::set_order(){
    calculate_fv_entropy(false);
    double *Order = new double[num_of_features];
    for ( size_t i=0; i < num_of_features; ++i )
      switch( TreeOrder ){
      case DataFile:
	Order[i] = Features[i]->Weight();
	break;
      case NoOrder:
	Order[i] = (double)(num_of_features-i);
	break;
      case IGOrder:
	Order[i] = Features[i]->InfoGain();
	break;
      case GROrder:
	Order[i] = Features[i]->GainRatio();
	break;
      case IGEntropyOrder:
	Order[i] = Features[i]->InfoGain() * Features[i]->SplitInfo();
	break;
      case GREntropyOrder:
	Order[i] = Features[i]->GainRatio() * Features[i]->SplitInfo();
	break;
      case X2Order:
	Order[i] = Features[i]->ChiSquare();
	break;
      case SVOrder:
	Order[i] = Features[i]->SharedVariance();
	break;
      case OneoverFeature:
	Order[i] =  1.0 / Features[i]->ValuesArray.size();
	break;
      case GRoverFeature:
	Order[i] =  Features[i]->GainRatio() / Features[i]->ValuesArray.size();
	break;
      case IGoverFeature:
	Order[i] =  Features[i]->InfoGain() / Features[i]->ValuesArray.size();
	break;
      case X2overFeature:
	Order[i] =  Features[i]->ChiSquare() / Features[i]->ValuesArray.size();
	break;
      case SVoverFeature:
	Order[i] =  Features[i]->SharedVariance() / Features[i]->ValuesArray.size();
	break;
      case OneoverSplitInfo:
	Order[i] =  1.0 / Features[i]->SplitInfo();
	break;
      case UnknownOrdening:
      case MaxOrdening:
	FatalError( "Setorder: Illegal Order Value in Switch: " +
		    toString( TreeOrder ) );
	break;
      }
    calc_perm( Order );
    delete [] Order;
    if ( !Verbosity(SILENT) )
      writePermutation( *Log(mylog) );
    for ( size_t j=0; j < num_of_features; ++j ){
      if ( j < effective_feats )
	PermFeatures[j] = Features[Permutation[j]];
      else 
	PermFeatures[j] = NULL;
    }
  }
  
  void MBLClass::MatrixInfo( ostream& os ) const {
    unsigned int TotalCount = 0;
    for ( size_t f = 0; f < num_of_features; ++f ){
      if ( !Features[f]->Ignore() &&
	   Features[f]->matrix_present() &&
	   ( ( Features[f]->Metric() == ValueDiff ||
	       ( Features[f]->Metric() == DefaultMetric && 
		 GlobalMetric == ValueDiff ) ) ||
	     ( Features[f]->Metric() == JeffreyDiv ||
	       ( Features[f]->Metric() == DefaultMetric && 
		 GlobalMetric == JeffreyDiv ) ) ||
	     ( Features[f]->Metric() == Levenshtein ||
	       ( Features[f]->Metric() == DefaultMetric && 
		 GlobalMetric == Levenshtein ) ) ) ){
	unsigned int Count = Features[f]->matrix_byte_size();
	os << "Size of value-matrix[" << f+1 << "] = " 
	   << Count << " Bytes " << endl;
	TotalCount += Count;
      }
    }
    if ( TotalCount )
      os << "Total Size of value-matrices " << TotalCount << " Bytes " 
	 << endl << endl;
  }
  
  bool MBLClass::readArrays( istream& is ){
    bool result = true;
    size_t num;
    size_t index = 1;
    string buf;
    char kar;

    do {
      is >> ws >> buf;
      if ( compare_nocase_n( "feature", buf ) ){
	is >> ws >> kar; // skip #
	if ( kar != '#' ){
	  Error( "Input out-of-sync, a '#' was expected" );
	  result = false; 
	} 
	else {
	  is >> num;
	  if ( num != index ){
	    Error( "Wrong feature number " + toString<size_t>(num) +
		   " in file, " + toString<size_t>(index) + " expected" );
	    result = false;
	  }
	  else if ( index > num_of_features ){
	    Error( "Too many features matrices in this file " );
	    result = false;
	  }
	  else {
	    is >> ws >> buf;
	    if ( compare_nocase_n( "Ignored", buf ) ){
	      if ( Features[index-1]->Ignore() ){
		++index;
		continue;
	      }
	      else {
		Error( "Feature #" + toString<size_t>(index) + 
		       " may not be ignored...");
		result = false;
	      }
	    }
	    else if ( compare_nocase_n( "Numeric", buf ) ){
	      if ( Features[index-1]->Numeric() ){
		++index;
		continue;
	      }
	      else {
		Error( "Feature #" + toString<size_t>(index) + " is not Numeric..." );
		result = false;
	      }
	    }
	    else if ( !compare_nocase_n( "Matrix", buf ) ){
	      Error( "Problem in Probability file, missing matrix info" );
	      result = false;
	    }
	    else if ( Features[index-1]->Ignore() ||
		      Features[index-1]->Numeric() ){
	      Warning( "Matrix info found for feature #" 
		       + toString<size_t>(index)
		       + " (skipped)" );
	      ++index;
	    }
	    else {
	      is.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
	      result = Features[index-1]->read_vc_pb_array( is );
	      ++index;
	    }
	  }
	}
      }
    }
    while ( result && !is.eof() & !is.bad() );
    if ( index < num_of_features+1 ){
      Error( "Not enough features matrices in this file " );
      result = false;
    }
    return result;
  }
  
  
  bool MBLClass::writeArrays( ostream& os ) {
    if ( ExpInvalid() )
      return false;
    else if ( !initProbabilityArrays( false ) ){
      Warning( "couldn't Calculate probability Arrays's" );
      return false;
    }
    else {
      // Print the possible classes.
      //
      os << "Targets : ";
      VCarrtype::const_iterator it = Targets->ValuesArray.begin();
      while ( it != Targets->ValuesArray.end() ){
	os << (TargetValue *)*it;
	++it;
	if ( it != Targets->ValuesArray.end() )
	  os << ",";
      } 
      os << "." << endl << endl;
      for ( size_t i = 0; i < num_of_features; ++i )
	if ( Features[i]->Ignore() )
	  os << "feature # " << i+1 << " Ignored, (-s option)" << endl;
	else if (Features[i]->Numeric() )
	  os << "feature # " << i+1 << " Numeric, (-N option)" << endl;
	else {
	  os << "feature # " << i+1 << " Matrix: " << endl;
	  Features[i]->print_vc_pb_array( os );
	  os << endl;
	}
      return true;
    }
  }
  
  bool MBLClass::allocate_arrays(){
    size_t Dim = Targets->ValuesArray.size();
    bool result = true;
    for ( size_t j = 0; result && j < num_of_features; ++j ) {
      if ( !Features[j]->Ignore() &&
	   !Features[j]->Numeric() ) {
	result = Features[j]->AllocSparseArrays( Dim );
      }
    } // j
    return true;
  }
  
  bool MBLClass::initProbabilityArrays( bool force ){
    bool result = true;
    result = allocate_arrays();
    if ( result ){
      for ( size_t j = 0; j < num_of_features; ++j ) {
	if ( !Features[j]->Ignore() &&
	     !Features[j]->Numeric() ){
	  Features[j]->ClipFreq( (int)rint(clip_factor * 
					   log((double)Features[j]->EffectiveValues())));
	  if ( force ||
	       ( !Features[j]->ArrayRead() &&
		 ( ( Features[j]->Metric() == ValueDiff ||
		     Features[j]->Metric() == JeffreyDiv ||
		     Features[j]->Metric() == Levenshtein ||
		     ( Features[j]->Metric() == DefaultMetric && 
		       ( GlobalMetric == ValueDiff ||
			 GlobalMetric == JeffreyDiv ||
			 GlobalMetric == Levenshtein ) ) ) ) ) ){
	    Features[j]->InitSparseArrays();
	  } // if force
	} //if !Ignore
      } // j
    }  
    return result;
  }
  
  /*
    For mvd metric.
  */
  void MBLClass::calculatePrestored(){
    for ( size_t j = tribl_offset; j < effective_feats; ++j ) {
      if ( PermFeatures[j]->Metric() == ValueDiff ||
	   (PermFeatures[j]->Metric() == DefaultMetric && 
	    GlobalMetric == ValueDiff) ){
	PermFeatures[j]->store_matrix( ValueDiff,
				       mvd_threshold, mvdDefaultMetric );
      }
      else if ( PermFeatures[j]->Metric() == JeffreyDiv ||
		(PermFeatures[j]->Metric() == DefaultMetric && 
		 GlobalMetric == JeffreyDiv) ){
	PermFeatures[j]->store_matrix( JeffreyDiv, mvd_threshold,
				       mvdDefaultMetric );
      }
      else if ( PermFeatures[j]->Metric() == Levenshtein ||
		(PermFeatures[j]->Metric() == DefaultMetric && 
		 GlobalMetric == Levenshtein ) ){
	PermFeatures[j]->store_matrix( Levenshtein, mvd_threshold,
				       mvdDefaultMetric );
      }
    } // j
    if ( Verbosity(VD_MATRIX) ) 
      for ( size_t i = 0; i < num_of_features; ++i )
	if ( !Features[i]->Ignore() ){
	  if (Features[i]->matrix_present( ) ){
	    *Log(mylog) << "Value matrix of feature # " 
			<< i+1 << endl;
	    Features[i]->print_matrix();
	    *Log(mylog) << endl;
	  }
	  else {
	    *Log(mylog) << "Value Difference matrix of feature # " 
			<< i+1 << endl << "Not avaliable." << endl;
	  }
	}
  }
  
  const Instance *MBLClass::chopped_to_instance( PhaseValue phase ){
    CurrInst.clear();
    if ( num_of_features != target_pos ) {
      ChopInput->swapTarget( target_pos );
    }
    switch ( phase  ){
    case LearnWords:
      // Add the target.
      CurrInst.TV = Targets->add_value( ChopInput->getField( num_of_features ));
      // Now add the Feature values.
      for ( size_t i = 0; i < num_of_features; ++i ){
	// when learning, no need to bother about Permutation
	if ( Features[i]->Ignore() ) // but this might happen, take care!
	  CurrInst.FV[i] = NULL;
	else
	  // Add it to the Instance.
	  CurrInst.FV[i] = Features[i]->add_value( ChopInput->getField(i),
						   CurrInst.TV ); 
      } // i
      break;
    case TrainWords:
      // Lookup for TreeBuilding
      // First the Features
      for ( size_t k = 0; k < effective_feats; ++k ){
	size_t j = Permutation[k];
	CurrInst.FV[k] = Features[j]->Lookup( ChopInput->getField(j) );
      } // k
      // and the Target
      CurrInst.TV = Targets->Lookup( ChopInput->getField( num_of_features ) );
      break;
    case TrainLearnWords:
      // Lookup for Incremental TreeBuilding
      // Assumes that somehow Permutation and effective_feats are known
      // First the Target
      CurrInst.TV = Targets->add_value( ChopInput->getField(num_of_features ) );
      // Then the Features
      for ( size_t l = 0; l < effective_feats; ++l ){
	size_t j = Permutation[l];
	CurrInst.FV[l] = Features[j]->add_value( ChopInput->getField(j),
						 CurrInst.TV ); 
      } // k
      break;
    case TestWords:
      // Lookup for Testing
      // This might fail for unknown values, then we create a dummy value
      for ( size_t m = 0; m < effective_feats; ++m ){
	size_t j = Permutation[m];
	const string& fld =  ChopInput->getField(j);
	CurrInst.FV[m] = Features[j]->Lookup( fld );
	if ( !CurrInst.FV[m] ){
	  // for "unknown" values have to add a dummy value
	  CurrInst.FV[m] = new FeatureValue( fld );
	}

      } // i
      // the last string is the target
      CurrInst.TV = Targets->Lookup( ChopInput->getField(num_of_features) );
      break;
    default:
      FatalError( "Wrong value in Switch: " 
		  + toString<PhaseValue>(phase) );
    }
    if ( ( phase != TestWords ) && doSamples() ){
      double exW = ChopInput->getExW();
      if ( exW < 0 )
	exW = 1.0;
      CurrInst.ExemplarWeight( exW );
    }
    return &CurrInst;
  }
  
  bool empty_line( const string& Line, const InputFormatType IF ){
    // determine wether Line is empty or a commentline
    bool result = ( Line.empty() || 
		    ( IF == ARFF &&  // ARFF "comment"
		      ( Line[0] == '%' || Line[0] == '@' ) ) ||
		    ( Line.find_first_not_of( " \t" ) == string::npos ) );
    return result;
  }
  
  void MBLClass::show_org_input( ostream& out ) const {
    ChopInput->print( out );
  }
  
  void MBLClass::LearningInfo( ostream& os ) {
    if ( !ExpInvalid() ){
      calculate_fv_entropy( !MBL_init );
      os.setf(ios::showpoint );
      int OldPrec = os.precision(8);
      os << "DB Entropy        : " << DBEntropy << endl;
      os << "Number of Classes : " << Targets->EffectiveValues() << endl;
      os << endl;
      if ( Verbosity(FEAT_W) ){
	if (  need_all_weights ){
	  os << "Feats\tVals\tX-square\tVariance\tInfoGain\tGainRatio" << endl;
	  for ( size_t i = 0; i < num_of_features; ++i ) {
	    os << setw(5) << i+1;
	    os.setf(ios::right, ios::adjustfield);
	    if ( Features[i]->Ignore() ){
	      os << " (ignored) " << endl;
	    }
	    else {
	      os.setf(ios::right, ios::adjustfield);
	      os << setw(7) << Features[i]->EffectiveValues()
		 << "\t" << Features[i]->ChiSquare()
		 << "\t" << Features[i]->SharedVariance()
		 << "\t" << Features[i]->InfoGain()
		 << "\t" << Features[i]->GainRatio();
	      if ( Features[i]->Numeric() )
		os << " NUMERIC";
	      os << endl;
	    }
	  }
	  os << endl;
	  os.precision(OldPrec);
	}
	else {
	  os << "Feats\tVals\tInfoGain\tGainRatio" << endl;
	  for ( size_t i = 0; i < num_of_features; ++i ) {
	    os << setw(5) << i+1;
	    os.setf(ios::right, ios::adjustfield);
	    if ( Features[i]->Ignore() ){
	      os << " (ignored) " << endl;
	    }
	    else {
	      os.setf(ios::right, ios::adjustfield);
	      os << setw(7) << Features[i]->EffectiveValues()
		 << "\t" << Features[i]->InfoGain()
		 << "\t" << Features[i]->GainRatio();
	      if ( Features[i]->Numeric() )
		os << " NUMERIC";
	      os << endl;
	    }
	  }
	  os << endl;
	  os.precision(OldPrec);
	}
      }
    }
  }
  
  bool MBLClass::writeWeights( ostream& os ) const {
    bool result = false;
    if ( !ExpInvalid() ){
      if ( Features[0] == NULL ){
	Warning( "unable to save Weights, nothing learned yet" );
      }
      else {
	os << "# DB Entropy: " << DBEntropy << endl;
	os << "# Classes: " << Targets->ValuesArray.size() << endl;
	os << "# Lines of data: " << Targets->TotalValues() << endl;
	int OldPrec = os.precision(DBL_DIG);
	os << "# " << toString( No_w ) << endl;
	os << "# Fea." << "\t" << "Weight" << endl;
	for (  size_t i = 0; i < num_of_features; ++i ) {
	  os.precision(DBL_DIG);
	  if ( Features[i]->Ignore() )
	    os << i+1 << "\t" << "Ignore" << endl;
	  else
	    os << i+1 << "\t" << 1.0 << endl;
	}
	os << "#" << endl;
	os << "# " << toString( GR_w ) << endl;
	os << "# Fea." << "\t" << "Weight" << endl;
	for (  size_t i = 0; i < num_of_features; ++i ) {
	  os.precision(DBL_DIG);
	  if ( Features[i]->Ignore() )
	    os << i+1 << "\t" << "Ignore" << endl;
	  else
	    os << i+1 << "\t" << Features[i]->GainRatio() << endl;
	}
	os << "#" << endl;
	os << "# " << toString( IG_w ) << endl;
	os << "# Fea." << "\t" << "Weight" << endl;
	for (  size_t i = 0; i < num_of_features; ++i ) {
	  os.precision(DBL_DIG);
	  if ( Features[i]->Ignore() )
	    os << i+1 << "\t" << "Ignore" << endl;
	  else
	    os << i+1 << "\t" << Features[i]->InfoGain() << endl;
	}
	if ( need_all_weights ){
	  os << "#" << endl;
	  os << "# " << toString( SV_w ) << endl;
	  os << "# Fea." << "\t" << "Weight" << endl;
	  for (  size_t i = 0; i < num_of_features; ++i ) {
	    os.precision(DBL_DIG);
	    if ( Features[i]->Ignore() )
	      os << i+1 << "\t" << "Ignore" << endl;
	    else
	      os << i+1 << "\t" << Features[i]->SharedVariance() << endl;
	  }
	  os << "#" << endl;
	  os << "# " << toString( X2_w ) << endl;
	  os << "# Fea." << "\t" << "Weight" << endl;
	  for (  size_t i = 0; i < num_of_features; ++i ) {
	    os.precision(DBL_DIG);
	    if ( Features[i]->Ignore() )
	      os << i+1 << "\t" << "Ignore" << endl;
	    else
	      os << i+1 << "\t" << Features[i]->ChiSquare() << endl;
	  }
	  os << "#" << endl;
	  os.precision(OldPrec);
	}
	result = true;
      }
    }
    return result;
  }
  
  bool MBLClass::read_the_vals( istream& is ){
    bool result = true;
    bool *done = new bool[num_of_features];
    for ( size_t i=0; i < num_of_features; ++i )
      done[i] = false;
    string Buffer;
    while ( getline( is, Buffer) ){
      if ( !Buffer.empty() ){
	if ( Buffer[0] == '#'){
	  break;
	}
	// Line looks like:
	// 28      0.445481
	// or:
	// 13      Ignore
	//
	vector<string> vals;
	if ( split( Buffer, vals ) == 2 ){
	  size_t i_f = stringTo<size_t>( vals[0] );
	  if ( i_f > num_of_features ){
	    Error( "in weightsfile, Feature index > Maximum, (" +
		   toString<size_t>(num_of_features) + ")" );
	  }
	  else if ( done[i_f-1] ){
	    Error( "in weightsfile, Feature index " + vals[0] +
		   " is mentioned twice" );
	  }
	  else {
	    done[i_f-1] = true;
	    if ( !compare_nocase( vals[1], "Ignore" ) ){
	      double w;
	      if ( !stringTo<double>( vals[1], w  ) ){
		Error( "in weightsfile, Feature " + vals[0] +
		       " has illegal value: " + vals[1] );
	      }
	      else {
		Features[i_f-1]->SetWeight( w );
		if ( Features[i_f-1]->Ignore() )
		  Warning( "in weightsfile, "
			   "Feature " + vals[0] + " has value: " +
			   toString<double>( w ) +
			   " assigned, but will be ignored" );
	      }
	    }
	    else {
	      Features[i_f-1]->SetWeight( 0.0 );
	      if ( !Features[i_f-1]->Ignore() )
		Warning( "in weightsfile, Feature " + vals[0] +
			 " has value: 'Ignore', we will use: 0.0 " );
	    }
	  }
	}
      }
    }
    if ( result ){
      for ( size_t j=0; j < num_of_features; ++j )
	if ( !done[j] ) {
	  Error( "in weightsfile, Feature index " + toString<size_t>(j+1) +
		 " is not mentioned" );
	  result = false;
	}
    }
    delete [] done;
    return result;
  }

  bool MBLClass::readWeights( istream& is, WeightType wanted ){
    set<WeightType> ret_weights;
    bool result = false;
    bool old_style = true;
    if ( !ExpInvalid() ){
      string Buffer;
      while( getline( is, Buffer ) ) {
	// A comment starts with '#'
	//
	if ( Buffer.empty() )
	  continue;
	else {
	  if ( Buffer[0] == '#'){
	    vector<string> vals;
	    if ( split_at( Buffer, vals, " " ) == 2  ){
	      WeightType tmp_w = Unknown_w;
	      if ( !stringTo<WeightType>( vals[1], tmp_w ) )
		continue;
	      else {
		old_style = false;
		if ( tmp_w == wanted ){
		  getline( is, Buffer );
		  result = read_the_vals( is );
		  break;
		}
	      }
	    }
	  }
	}
      }
      if ( is.eof() ){
	if ( old_style ){
	  // wanted weighting not found
	  // Old style weightsfile?
	  //	  Warning( "Old Style weightsfile. Please update" );
	  is.clear();
	  is.seekg(0);
	  size_t pos = 0;
	  while( getline( is, Buffer ) ) {
	    // A comment starts with '#'
	    //
	    if ( Buffer.empty() ){
	      pos = is.tellg();
	      continue;
	    }
	    else {
	      if ( Buffer[0] == '#'){
		pos = is.tellg();
		continue;
	      }
	      is.seekg(pos);
	      result = read_the_vals( is );
	      break;
	    }
	  }
	}
      }
      if ( !result ){
	Warning( "Unable to retrieve " 
		 + toString( wanted ) + " Weights" );
	Warning( "unable to continue" );
	return false;
      }
      // make shure all weights are correct
      // Paranoid?
      for ( size_t i=0; i< num_of_features; ++i ){
	Features[i]->InfoGain( Features[i]->Weight() );
	Features[i]->GainRatio( Features[i]->Weight() );
	Features[i]->ChiSquare( Features[i]->Weight() );
	Features[i]->SharedVariance( Features[i]->Weight() );
      }
      Weighting = UserDefined_w;
    }
    return true;
  }
  
  void MBLClass::calculate_fv_entropy( bool always ){
    bool realy_first =  DBEntropy < Epsilon;
    bool nothing_changed = true;
    if ( always || realy_first ){
      // if it's the first time (DBEntropy == 0 ) or
      // if always, we have to (re)calculate everything
      double Entropy = 0.0, Ratio;
      // first get the Database Entropy
      size_t totval = Targets->TotalValues();
      VCarrtype::const_iterator it = Targets->ValuesArray.begin();
      while ( it != Targets->ValuesArray.end() ){
	Ratio = (*it)->ValFreq() / 
	  (double)totval;
	if ( Ratio > 0 )
	  Entropy += Ratio * Log2(Ratio);
	++it;
      }
      DBEntropy = fabs(-Entropy);
      allocate_arrays(); // create ValueClassProb arrays..
    }
    // Loop over the Features, see if the numerics are non-singular
    // and do the statistics for those features where the metric is changed.
    MetricType TmpMetric;
    FeatVal_Stat *feat_status = new FeatVal_Stat[num_of_features];
    for ( size_t g = 0; g < num_of_features; ++g ) {
      feat_status[g] = Unknown;
      if ( Features[g]->Ignore() )
	continue;
      TmpMetric = UserOptions[g+1];
      if ( Features[g]->Numeric() ){
	feat_status[g] = Features[g]->prepare_numeric_stats();
	if ( feat_status[g] == SingletonNumeric &&
	     ( input_format == SparseBin 
	       && ( do_dot_product || do_cos_metric ) ) ){
	  // ok
	}
	else {
	  if ( feat_status[g] != NumericValue ){
	    Features[g]->Numeric( false );
	    nothing_changed = false;
	    if ( GlobalMetric == Numeric )
	      TmpMetric = Overlap;
	    else
	      TmpMetric = DefaultMetric;
	    Features[g]->Metric(TmpMetric);
	  }
	  else
	    TmpMetric = Numeric;
	} 
      }
      else if ( Features[g]->TotalValues() == 1 )
	feat_status[g] = Singleton;
      if ( always || realy_first ){
	if ( Weighting != UserDefined_w ){
	  if ( TmpMetric == Numeric ){
	    Features[g]->NumStatistics( DBEntropy, Targets, Bin_Size,
					need_all_weights );
	  }
	  else {
	    Features[g]->Statistics( DBEntropy, Targets,
				     need_all_weights );
	  }
	}
	if ( TmpMetric != Numeric && 
	     !( TmpMetric == DefaultMetric && GlobalMetric == Numeric ) &&
	     TmpMetric !=  Features[g]->Metric() ){
	  nothing_changed = false;
	  Features[g]->Metric(TmpMetric);
	}
      }
    } // end g
    if ( (do_dot_product || do_cos_metric ) && !nothing_changed ){
      // check to see if ALL features are still Numeric.
      // otherwise we can't do Inner product!
      bool first = true;
      ostringstream ostr1;
      ostringstream ostr2;
      for ( size_t ff = 0; ff < num_of_features; ++ff ) 
	if ( feat_status[ff] == NotNumeric ){
	  if ( first ){
	    ostr1 << "The following feature(s) have non numeric value: ";
	    first = false;
	  }
	  else
	    ostr1 << ", ";
	  size_t n = ff;
	  while ( ff < num_of_features-1 && 
		  feat_status[ff+1] == NotNumeric )
	    ff++;
	  if ( n != ff ){
	    ostr1 << n+1 << "-" << ff+1;
	  }
	  else
	    ostr1 << ff+1;
	}
      if ( !first  ){
	Error( ostr1.str() );
	Error( "Therefore InnerProduct/Cosine operations are impossible" );
	return;
      }
    }
    // Give a warning for singular features, except when it's
    // a result of a forced recalculation or when the input format is
    // Sparse ??
    if ( !nothing_changed &&
	 ( input_format != Sparse && input_format != SparseBin ) &&
	 ( realy_first || !always ) ){
      bool first = true;
      ostringstream ostr1;
      ostringstream ostr2;
      for ( size_t ff = 0; ff < num_of_features; ++ff ) 
	if ( feat_status[ff] == Singleton ||
	     feat_status[ff] == SingletonNumeric ){
	  if ( first ){
	    ostr1 << "The following feature(s) have only 1 value: ";
	    first = false;
	  }
	  else
	    ostr1 << ", ";
	  size_t n = ff;
	  while ( ff < num_of_features-1 && 
		  ( feat_status[ff+1] == Singleton ||
		    feat_status[ff+1] == SingletonNumeric ) )
	    ff++;
	  if ( n != ff ){
	    ostr1 << n+1 << "-" << ff+1;
	  }
	  else
	    ostr1 << ff+1;
	}
      if ( !first  ){
	Warning( ostr1.str() );
      }
      first = true;
      for ( size_t ff = 0; ff < num_of_features; ++ff ) 
	if ( feat_status[ff] == NotNumeric ){
	  if ( first ){
	    ostr2 << "The following feature(s) contained non-numeric values and\nwill be treated as NON-Numeric: ";
	    first = false;
	  }
	  else
	    ostr2 << ", ";
	  size_t n = ff;
	  while ( ff < num_of_features-1 && 
		  feat_status[ff+1] == NotNumeric ) ff++;
	  if ( n != ff ){
	    ostr2 << n+1 << "-" << ff+1;
	  }
	  else
	    ostr2 << ff+1;
	}
      if ( !first  ){
	Warning( ostr2.str() );
      }
    }
    delete [] feat_status;
  }
  
  bool MBLClass::writeNamesFile( ostream& os ) const {
    bool result = true;
    if ( ExpInvalid() ){
      result = false;
    }
    else {
      // Print the possible classes.
      //
      VCarrtype::const_iterator it = Targets->ValuesArray.begin();
      while ( it != Targets->ValuesArray.end() ){
	os << (TargetValue *)*it;
	++it;
	if ( it != Targets->ValuesArray.end() )
	  os << ",";
      } 
      os << "." << endl << endl;
      
      // Loop over the Features.
      //
      for ( size_t f = 0; f < num_of_features; ++f ) {
	
	// Print the Feature name, and a colon.
	//
	os << "a" << f+1 << ": ";
	if ( Features[f]->Ignore() )
	  os << "Ignore" << endl;
	else if ( Features[f]->Numeric() )
	  os << "Numeric" << endl;
	else {
	  // Loop over the values.
	  //
	  VCarrtype::const_iterator it2 = Features[f]->ValuesArray.begin();
	  while( it2 != Features[f]->ValuesArray.end() ){
	    os << (FeatureValue *)*it2;
	    ++it2;
	    if ( it2 != Features[f]->ValuesArray.end() )
	      os << ",";
	  }
	  os << "." << endl;
	}
      }
    }
    return result;
  }

  bool MBLClass::Chop( const string& line ) {
    try {
      return ChopInput->chop( line, num_of_features, chopExamples() );
    }
    catch ( const exception& e ){
      Warning( e.what() );
      return false;
    }
  }
  
  bool MBLClass::setInputFormat( const InputFormatType IF ){
    if ( ChopInput ){
      delete ChopInput;
      ChopInput = 0;
    }
    if ( do_sparse && ( IF != SparseBin && IF != Sparse ) )
      return false;
    switch ( IF ){
    case C4_5:
      ChopInput = new C45_Chopper();
      break;
    case ARFF:
      ChopInput = new ARFF_Chopper();
      break;
    case SparseBin:
      ChopInput = new Bin_Chopper();
      do_sparse = true;
      break;
    case Sparse:
      ChopInput = new Sparse_Chopper();
      do_sparse = true;
      break;
    case Columns:
      ChopInput = new Columns_Chopper();
      break;
    case Compact:
      ChopInput = new Compact_Chopper(F_length );
      break;
    default:
      return false;
    }
    input_format = IF;
    return true;
  }
  
  const ValueDistribution *MBLClass::ExactMatch( const Instance& inst ) const {
    const ValueDistribution *result = NULL;
    if ( !(do_dot_product || do_cos_metric) &&
	 ( do_exact_match || 
	   ( num_of_neighbors == 1 &&
	     !( Verbosity( NEAR_N | ALL_K) ) ) ) ){
      result = InstanceBase->ExactMatch( inst );
    }
    return result;
  }

  double MBLClass::getBestDistance() const {
    return nSet.bestDistance();
  }

  WValueDistribution *MBLClass::getBestDistribution( unsigned int k ){
    return nSet.bestDistribution( decay, k );
  }

  string MBLClass::formatInstance( const vector<FeatureValue *>& OrgFV,
				   vector<FeatureValue *>& RedFV,
				   size_t OffSet,
				   size_t Size ) const {
    string result;
    Instance inst( Size );
    for ( size_t i=0; i< OffSet; ++i )
      inst.FV[i] = OrgFV[i];
    for ( size_t j=OffSet; j< Size; ++j )
      inst.FV[j] = RedFV[j-OffSet];
    size_t *InvPerm = new size_t[num_of_features];
    for ( size_t i=0; i< num_of_features; ++i )
      InvPerm[Permutation[i]] = i;
    for ( size_t j=0; j< num_of_features; ++j ){
      switch ( input_format ) {
      case C4_5:
	// fall through
      case ARFF:
	if ( Features[j]->Ignore() )
	  result += "-*-,";
	else
	  result += inst.FV[InvPerm[j]]->Name() + ",";
	break;
      case Sparse:
	if ( inst.FV[InvPerm[j]]->Name() != DefaultSparseString )
	  result += string("(")  + toString<size_t>(j+1) + ","
	    + CodeToStr( inst.FV[InvPerm[j]]->Name() ) + ")";
	break;
      case SparseBin:
	if ( inst.FV[InvPerm[j]]->Name()[0] == '1' )
	  result += toString<size_t>( j+1 ) + ",";
	break;
      case Columns:
	if ( Features[j]->Ignore() )
	  result += "-*- ";
	else
	  result += inst.FV[InvPerm[j]]->Name() + " ";
	break;
      default:
	if ( Features[j]->Ignore() )
	  result += string( F_length, '*' );
	else
	  result += inst.FV[InvPerm[j]]->Name();
	break;
      }
    }
    delete [] InvPerm;
    return result;
  }

  inline double WeightFun( double D, double W ){
    return D / (W + Common::Epsilon);
  }
  
  
  void MBLClass::test_instance_ex( const Instance& Inst,
				   InstanceBase_base *IB,
				   size_t ib_offset ){
    vector<FeatureValue *> CurrentFV(num_of_features);
    size_t EffFeat = effective_feats - ib_offset;
    const ValueDistribution *best_distrib = IB->InitGraphTest( CurrentFV, 
							       &Inst.FV,
							       ib_offset,
							       effective_feats );
    tester->init( Inst, effective_feats, ib_offset );
    ValueDistribution::dist_iterator lastpos;
    Vfield *Bpnt = NULL;
    if ( best_distrib ){
      lastpos = best_distrib->begin();
      if ( lastpos != best_distrib->end() )
	Bpnt = lastpos->second;
    }
    size_t CurPos = 0;
    while ( Bpnt ) {
      double dummy = -0.0;
      size_t EndPos  = tester->test( CurrentFV,
				     CurPos,
				     dummy );
      if ( EndPos == EffFeat ){
	// we finished with a certain amount of succes
	ValueDistribution ResultDist;
	ResultDist.SetFreq( Bpnt->Value(), Bpnt->Freq() );
	string origI;
	if ( Verbosity(NEAR_N) ){
	  origI = formatInstance( Inst.FV, CurrentFV, 
				  ib_offset, 
				  num_of_features );
	}
	double Distance = WeightFun( tester->getDistance(EndPos),
				     Bpnt->Weight() );
	bestArray.addResult( Distance, &ResultDist, origI );
      }
      else {
	EndPos++; // out of luck, compensate for roll-back
      }
      CurPos = EndPos-1;
      ++lastpos;
      if ( lastpos != best_distrib->end() ){
	Bpnt = lastpos->second;
      }
      else {
	best_distrib = IB->NextGraphTest( CurrentFV, 
					  CurPos );
	Bpnt = NULL;
	if ( best_distrib ){
	  lastpos = best_distrib->begin();
	  if ( lastpos != best_distrib->end() ){
	    Bpnt = lastpos->second;  
	  }
	}
      }
    }
  }
  
  void MBLClass::initDecay(){
    if ( decay ){
      delete decay;
      decay = 0;
    }
    switch ( decay_flag ){
    case InvDist:
      decay = new invDistDecay();
      break;
    case InvLinear:
      decay = new invLinDecay();
      break;
    case ExpDecay:
      decay = new expDecay( decay_alfa, decay_beta );
      break;
    case Zero: // fall through
    default:
      break;
    }
  }

  void MBLClass::initTesters() {
    delete tester;
    if ( doSamples() )
      tester = new ExemplarTester( Features, Permutation );
    else if ( do_cos_metric )
      tester = new CosineTester( Features, Permutation );
    else if ( do_dot_product )
      tester = new DotProductTester( Features, Permutation );
    else
      tester = new DefaultTester( Features, Permutation );
    tester->reset( GlobalMetric, mvd_threshold );
  }

  ostream&  operator<< ( ostream& os, const vector<FeatureValue*>& fv ){
    vector<FeatureValue*>::const_iterator it= fv.begin();
    os << "vector<";
    while( it != fv.end() ){
      os << *it;
      ++it;
      if ( it != fv.end() )
	os << ",";
    }
    os << ">";
    return os;
  }

  ostream&  operator<< ( ostream& os, const vector<double>& fv ){
    vector<double>::const_iterator it= fv.begin();
    os << "vector<";
    while( it != fv.end() ){
      os << *it;
      ++it;
      if ( it != fv.end() )
	os << ",";
    }
    os << ">";
    return os;
  }

  
  void MBLClass::test_instance( const Instance& Inst,
				InstanceBase_base *IB,
				size_t ib_offset ){
    vector<FeatureValue *> CurrentFV(num_of_features);
    double Threshold = DBL_MAX;
    size_t EffFeat = effective_feats - ib_offset;
    const ValueDistribution *best_distrib = IB->InitGraphTest( CurrentFV, 
							       &Inst.FV,
							       ib_offset,
							       effective_feats );
    tester->init( Inst, effective_feats, ib_offset );
    size_t CurPos = 0;
    while ( best_distrib ){
      size_t EndPos = tester->test( CurrentFV,
				    CurPos,
				    Threshold + Epsilon );
      if ( EndPos == EffFeat ){
	// we finished with a certain amount of succes
	double Distance = tester->getDistance(EndPos);
	if ( Distance >= 0.0 ){
	  string origI;
	  if ( Verbosity(NEAR_N) ){
	    origI = formatInstance( Inst.FV, CurrentFV, 
				    ib_offset, 
				    num_of_features );
	  }
	  Threshold = bestArray.addResult( Distance, best_distrib, origI );
	  if ( do_silly_testing )
	    Threshold = DBL_MAX;
	}
	else {
	  Error( "DISTANCE == " + toString<double>(Distance) );
	  FatalError( "we are dead" );
	}
      }
      else {
	EndPos++; // out of luck, compensate for roll-back
      }
      size_t pos=EndPos-1;
      while ( true ){
	if ( tester->getDistance(pos) <= Threshold ){
	  CurPos = pos;
	  best_distrib = IB->NextGraphTest( CurrentFV, 
					    CurPos );
	  break;
	}
	if ( pos == 0 )
	  break;
	--pos;
      }
    }
  }

  void MBLClass::test_instance_sim( const Instance& Inst,
				    InstanceBase_base *IB,
				    size_t ib_offset ){
    vector<FeatureValue *> CurrentFV(num_of_features);
    size_t EffFeat = effective_feats - ib_offset;
    const ValueDistribution *best_distrib = IB->InitGraphTest( CurrentFV, 
							       &Inst.FV,
							       ib_offset,
							       effective_feats );
    tester->init( Inst, effective_feats, ib_offset );
    size_t CurPos = 0;
    while ( best_distrib ){
      double dummy = -1.0;
      size_t EndPos = tester->test( CurrentFV,
				    CurPos,
				    dummy );
      if ( EndPos == EffFeat ){
	// we finished with a certain amount of succes
	double Distance = tester->getDistance(EndPos);
	if ( Distance >= 0.0 ){
	  string origI;
	  if ( Verbosity(NEAR_N) ){
	    origI = formatInstance( Inst.FV, CurrentFV, 
				    ib_offset, 
				    num_of_features );
	  }
	  bestArray.addResult( Distance, best_distrib, origI );

	}
	else {
	  Error( "DISTANCE == " + toString<double>(Distance) );
	  FatalError( "we are dead" );
	}
      }
      else {
	EndPos++; // out of luck, compensate for roll-back
      }
      if ( EndPos > 0 ){
	CurPos = EndPos-1;
	best_distrib = IB->NextGraphTest( CurrentFV, 
					  CurPos );
      }
    }
  }
  
  void MBLClass::TestInstance( const Instance& Inst, 
			       InstanceBase_base *SubTree,
			       size_t level ){
    bestArray.init( num_of_neighbors, MaxBests,
		    Verbosity(NEAR_N), Verbosity(DISTANCE),
		    Verbosity(DISTRIB) ); 
    // must be cleared for EVERY test
    if (  doSamples() ){
      test_instance_ex( Inst, SubTree, level );
    }
    else {
      if ( do_dot_product || do_cos_metric ){
	test_instance_sim( Inst, SubTree, level );
      }
      else
	test_instance( Inst, SubTree, level );
    }
  }
  
  size_t MBLClass::countFeatures( const string& inBuffer,
				  const InputFormatType IF ) const {
    size_t result = 0;
    string buffer;
    if ( chopExamples() ){
      string dummy;
      buffer = Chopper::stripExemplarWeight( inBuffer, dummy );
    }
    else
      buffer = inBuffer;
    size_t len = buffer.length();
    switch ( IF ){
    case ARFF:
    case C4_5:
      for ( size_t i = 0; i < len; ++i ) {
	if (buffer[i] == ',')
	  result++;
      };
      break;
    case Compact:
      if ( F_length == 0 ){
	Error( "-F Compact specified, but Feature Length not set."
	       " (-l option)" );
	return result;
      }
      else
	result = (len / F_length) - 1;
      break;
    case Columns:
      for ( size_t j = 0; j < len; ++j ) {
	if ( isspace(buffer[j]) ){
	  result++;
	  while ( isspace( buffer[++j] ) ){};
	  if ( buffer[j] == '\0' )
	    result--; // we had some trailing spaces
	}
      };
      break;
    default:
      FatalError( "CountFeatures: Illegal value in switch:" +
		  toString(IF) );
    };
    return result;
  }
  
  InputFormatType MBLClass::getInputFormat( const string& inBuffer ) const {
    InputFormatType IF = UnknownInputFormat;
    string buffer;
    if ( chopExamples() ){
      string dummy;
      buffer = Chopper::stripExemplarWeight( inBuffer, dummy );
    }
    else
      buffer = inBuffer;
    size_t len = buffer.length();
    int c45Cnt = 0;
    int columnCnt = 0;
    for ( unsigned int i = 0; i < len; ++i ) {
      if ( buffer[i] == ',' ) {
	++c45Cnt;
      }
      else if ( isspace( buffer[i] ) ){
	++columnCnt;
	while ( i < len && isspace( buffer[i+1] ) ) ++i;
	if ( i >= len-1 ){ // just trailing spaces!
	  --columnCnt;
	}
      }
    }
    if ( columnCnt == 0 && c45Cnt == 0 )
      IF = Compact;
    else if ( c45Cnt >= columnCnt )
      IF = C4_5;
    else
      IF = Columns;
    return IF;
  }
  
  inline void show_input_format( ostream& os,
				 InputFormatType IF,
				 int F_length ) {
    switch ( IF ){
    case C4_5:
      os << "InputFormat       : C4.5";
      break;
    case SparseBin:
      os << "InputFormat       : Sparse Binary";
      break;
    case Sparse:
      os << "InputFormat       : Sparse";
      break;
    case ARFF:
      os << "InputFormat       : ARFF";
      break;
    case Columns:
      os << "InputFormat       : Columns";
      break;
    case Compact:
      os << "InputFormat       : Compact, (Feature Length = "
	 << F_length << ")";
      break;
    default:
      os << "InputFormat unknown\n";
    }
    os << endl << endl;
  }
  
  size_t MBLClass::examineData( const string& FileName ){
    // Looks at the data files, counts num_of_features.
    // and sets input_format variables.
    //
    size_t NumF = 0;
    InputFormatType IF = UnknownInputFormat;
    // Open the file.
    //
    if ( FileName == "-" )
      return num_of_features;
    else if ( FileName == "" ) {
      Warning( "couldn't initialize: No FileName specified " );
      return 0;
    }
    else {
      string Buffer;
      ifstream datafile( FileName.c_str(), ios::in);
      if (!datafile) {
	Warning( "can't open DataFile: " + FileName );
	return 0;
      }
      else if ( input_format != UnknownInputFormat ){
	// The format is somehow already known, so use that
	if ( do_sparse || input_format == SparseBin || input_format == Sparse )
	  NumF = MaxFeatures;
	else {
	  if ( !getline( datafile, Buffer ) ) {
	    Warning( "empty data file" );
	  }
	  else {
	    bool more = true;
	    if ( input_format == ARFF ){
	      while ( !compare_nocase_n( "@DATA", Buffer ) ){
		if ( !getline( datafile, Buffer ) ){
		  Warning( "empty data file" );
		  more = false;
		  break;
		};
	      }
	      if ( more && !getline( datafile, Buffer ) ){
		Warning( "empty data file" ); 
		more = false;
	      };
	    }
	    while ( more && empty_line( Buffer, input_format ) ){
	      if ( !getline( datafile, Buffer ) ){
		Warning( "empty data file" ); 
		more = false;
	      };
	    }
	    // now we have a usable line, 
	    //analyze it using the User defined input_format
	    NumF = countFeatures( Buffer, input_format );
	  }
	}
	IF = input_format;
      }
      else if ( !getline( datafile, Buffer ) ){
	Warning( "empty data file: " + FileName );
      }
      // We start by reading the first line so we can figure out the number
      // of Features, and see if the file is comma seperated or not,  etc.
      //
      else { 
	if ( IF == ARFF ){
	  // Remember, we DON't want to auto-detect ARFF
	  while ( !compare_nocase_n( "@DATA", Buffer ) ){
	    if ( !getline( datafile, Buffer ) ) {
	      Warning( "no ARRF data after comments: " + FileName );
	      return 0;
	    }
	  }
	  do {
	    if ( !getline( datafile, Buffer ) ) {
	      Warning( "no ARRF data after comments: " + FileName );
	      return 0;
	    }
	  } while ( empty_line( Buffer, input_format ) );
	}
	else {
	  while ( empty_line( Buffer, input_format ) ) {
	    if ( !getline( datafile, Buffer ) ) {
	      Warning( "no data after comments: " + FileName );
	      return 0;
	    }
	  }
	  // We found a useful line!
	  // Now determine the input_format (if not already known,
	  // and Count Features as well.
	}
	IF = getInputFormat( Buffer );
	NumF = countFeatures( Buffer, IF );
      }
    }
    if ( NumF > 0 ){
      if ( input_format != UnknownInputFormat &&
	   input_format != IF ){
	Warning( "assumed inputformat differs from specified!" );
	return 0;
      }
      else {
	if ( !Verbosity(SILENT) ){
	  *Log(mylog) << "Examine datafile '" << FileName 
		      << "' gave the following results:"
		      << endl
		      << "Number of Features: " << NumF << endl;
	}
	if ( NumF > MaxFeatures ){
	  Error( "Number of Features exceeds the maximum number. "
		 "(currently " + toString<size_t>(MaxFeatures) +
		 ")\nPlease increase.\n" );
	  return 0;
	}
	setInputFormat( IF );
	if ( !Verbosity(SILENT) )
	  show_input_format( *Log(mylog), IF, F_length );
      }
    }
    return NumF;
  }

  void MBLClass::Initialize( size_t n ){ 
    if ( n > 0 )
      num_of_features = n;
    // Allocate memory. Will be reused again and again ....
    //
    if ( target_pos == std::numeric_limits<size_t>::max() )
      target_pos = num_of_features; // the default
    else if ( target_pos > num_of_features )
      FatalError( "Initialize: TARGET_POS cannot exceed NUM_OF_FEATURES+1 " +
		  toString<size_t>( num_of_features+1 ) );
    Features.resize(num_of_features,NULL);
    PermFeatures.resize(num_of_features,NULL);
    FeatureStrings = new StringHash(); // all features share the same hash
    TargetStrings = new StringHash(); // targets has it's own hash
    Targets = new Target( DefaultTargetNumber, 
			  TargetIncrement,
			  TargetStrings );
    for ( size_t i=0; i< num_of_features; ++i ){
      Features[i] = new Feature( DefaultFeatureNumber,
				 FeatureIncrement,
				 FeatureStrings );
      PermFeatures[i] = NULL; //Features[i];
    }
    CurrInst.Init( num_of_features );
    // the user thinks about features running from 1 to Num
    // we know better, so shift one down.
    effective_feats = num_of_features;
    num_of_num_features = 0;
    for ( size_t j = 0; j < num_of_features; ++j ){
      switch (UserOptions[j+1] ){
      case Ignore:
	Features[j]->Ignore( true );
	effective_feats--;
	break;
      case Numeric:
	Features[j]->Numeric( true );
	num_of_num_features++;
	break;
      case DefaultMetric:
	if ( GlobalMetric == Numeric ){
	  Features[j]->Numeric( true );
	  num_of_num_features++;
	}
	break;
      case Overlap:
      case ValueDiff:
      case JeffreyDiv:
	break;
      default:
	FatalError( "Initialize: Illegal value in switch " +
		    toString( UserOptions[j+1] ) );
      }
    }
    Options.FreezeTable();
    if ( Weighting > IG_w ||
	 TreeOrder >= X2Order )
      need_all_weights = true;
  }
  
} // namespace
