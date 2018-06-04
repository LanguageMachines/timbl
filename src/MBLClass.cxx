/*
  Copyright (c) 1998 - 2018
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
#include <set>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include <iomanip>
#include <typeinfo>

#include <ctime>
#include <cassert>

#include "ticcutils/StringOps.h"
#include "ticcutils/PrettyPrint.h"
#include "ticcutils/Timer.h"
#include "ticcutils/TreeHash.h"

#include "timbl/MsgClass.h"
#include "timbl/Common.h"
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/Instance.h"
#include "timbl/IBtree.h"
#include "timbl/BestArray.h"
#include "timbl/Testers.h"
#include "timbl/Metrics.h"
#include "timbl/Choppers.h"

#include "timbl/MBLClass.h"

using namespace std;

namespace Timbl {

  void MBLClass::fill_table(){
    if ( tableFilled )
      return;
    else
      tableFilled = true;
    //cerr << "fill table() for " << (void*)this << endl;
    bool stat =
      Options.Add( new IntegerOption( "FLENGTH",
				      &F_length, 0, 1, 32 ) )
      && Options.Add( new SizeOption( "MAXBESTS",
				      &MaxBests, 500, 10, 100000 ) )
      && Options.Add( new SizeOption( "TRIBL_OFFSET",
				      &tribl_offset, 0, 0, MaxFeatures ) )
      && Options.Add( new UnsignedOption( "IG_THRESHOLD",
					  &igThreshold,
					  1000, 0,
					  std::numeric_limits<unsigned>::max() ) )
      && Options.Add( new InputFormatOption( "INPUTFORMAT",
					     &input_format,
					     UnknownInputFormat ) )
      && Options.Add( new OrdeningOption( "TREE_ORDER",
					  &TreeOrder, UnknownOrdening ) )
      && Options.Add( new BoolOption( "ALL_WEIGHTS",
				      &need_all_weights, false ) )
      && Options.Add( new WeightOption( "WEIGHTING",
					&Weighting, GR_w ) )
      && Options.Add( new IntegerOption( "BIN_SIZE",
					 &Bin_Size, 20, 2, 10000 ) )
      && Options.Add( new UnsignedOption( "IB2_OFFSET",
					  &ib2_offset, 0, 1, 10000000 ) )
      && Options.Add( new BoolOption( "KEEP_DISTRIBUTIONS",
				      &keep_distributions, false ) )
      && Options.Add( new BoolOption( "DO_SLOPPY_LOO",
				      &do_sloppy_loo, false ) )
      && Options.Add( new SizeOption( "TARGET_POS",
				      &target_pos,
				      std::numeric_limits<size_t>::max(),
				      0, MaxFeatures ) );
    if ( stat ){
      Options.SetFreezeMark();
      stat =
	Options.Add( new BoolOption( "DO_SILLY",
				     &do_silly_testing, false ) )
	&& Options.Add( new BoolOption( "DO_DIVERSIFY",
					&do_diversify, false ) )
	&& Options.Add( new DecayOption( "DECAY",
					 &decay_flag, Zero ) )
	&& Options.Add( new IntegerOption( "SEED",
					   &random_seed, -1, -1, RAND_MAX ) )
	&& Options.Add( new IntegerOption( "BEAM_SIZE",
					   &beamSize, 0, 1, INT_MAX ) )
	&& Options.Add( new RealOption( "DECAYPARAM_A",
					&decay_alfa, 1.0, 0.0, DBL_MAX ) )
	&& Options.Add( new RealOption( "DECAYPARAM_B",
					&decay_beta, 1.0, 0.0, DBL_MAX ) )
	&& Options.Add( new NormalisationOption( "NORMALISATION",
						 &normalisation, noNorm ) )
	&& Options.Add( new RealOption( "NORM_FACTOR",
					&norm_factor, 1.0, Epsilon, DBL_MAX ) )
	&& Options.Add( new BoolOption( "EXEMPLAR_WEIGHTS",
					&do_sample_weighting, false ) )
	&& Options.Add( new BoolOption( "IGNORE_EXEMPLAR_WEIGHTS",
					&do_ignore_samples, true ) )
	&& Options.Add( new BoolOption( "NO_EXEMPLAR_WEIGHTS_TEST",
					&no_samples_test, true ) )
	&& Options.Add( new VerbosityOption( "VERBOSITY",
					     &verbosity, NO_VERB ) )
	&& Options.Add( new BoolOption( "EXACT_MATCH",
					&do_exact_match, false ) )
	&& Options.Add( new BoolOption( "HASHED_TREE",
					&hashed_trees, true ) )
	&& Options.Add( new MetricOption( "GLOBAL_METRIC",
					  &globalMetricOption, Overlap ) )
	&& Options.Add( new MetricArrayOption( "METRICS",
					       UserOptions, globalMetricOption,
					       MaxFeatures+1 ) )
	&& Options.Add( new IntegerOption( "MVD_LIMIT",
					   &mvd_threshold, 1, 1, 100000 ) )
	&& Options.Add( new SizeOption( "NEIGHBORS",
					&num_of_neighbors, 1, 1, 100000 ) )
	&& Options.Add( new IntegerOption( "PROGRESS",
					   &progress, 10000, 1, INT_MAX ) )
	&& Options.Add( new IntegerOption( "HANDLE_OCCURRENCES",
					   &doOcc, 0, 0, 3 ) )
	&& Options.Add( new IntegerOption( "CLIP_FACTOR",
					   &clip_factor, 10, 0, 1000000 ) );
    }
    if ( !stat ){
      FatalError( "Too many options for OptionTable" );
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
      //      Info( "set Option:" + line );
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
	Error( "SetOption '" + line +
	       "' failed.\nIllegal value for this option" );
	break;
      }
    }
    return result;
  }

  void MBLClass::InitClass( const size_t Size ){
    GlobalMetric = 0;
    is_copy = false;
    is_synced = false;
    sock_os = 0;
    Targets   = NULL;
    err_count = 0;
    MBL_init = false;
    tableFilled = false;
    need_all_weights = false;
    InstanceBase = NULL;
    TargetStrings = NULL;
    FeatureStrings = NULL;
    num_of_features = 0;
    target_pos = std::numeric_limits<size_t>::max();
    mvd_threshold = 1;
    effective_feats = 0;
    num_of_num_features = 0;
    DBEntropy = -1.0;
    ChopInput = 0;
    MaxFeatures = Size;
    runningPhase = LearnWords;
    do_sloppy_loo = false;
    do_silly_testing = false;
    do_diversify = false;
    keep_distributions = false;
    UserOptions.resize(MaxFeatures+1);
    tester = 0;
    //    cerr << "call fill table() in InitClass()" << endl;
    fill_table();
    decay = 0;
    myerr = &cerr;
    mylog = &cout;
  }

  MBLClass::MBLClass( const string& name ){
    tableFilled = false;
    exp_name = name;
  }

  MBLClass &MBLClass::operator=( const MBLClass& m ){
    if ( this != &m ){
      is_copy = true;
      is_synced = false;
      MaxFeatures        = m.MaxFeatures;
      UserOptions.resize(MaxFeatures+1);
      //      cerr << "call fill table() in assign" << endl;
      fill_table();
      F_length           = m.F_length;
      MaxBests           = m.MaxBests;
      TreeOrder          = m.TreeOrder;
      decay_flag         = m.decay_flag;
      input_format       = m.input_format;
      random_seed        = m.random_seed;
      beamSize           = m.beamSize;
      decay_alfa         = m.decay_alfa;
      decay_beta         = m.decay_beta;
      normalisation      = m.normalisation;
      norm_factor        = m.norm_factor;
      do_sample_weighting = m.do_sample_weighting;
      do_ignore_samples  = m.do_ignore_samples;
      no_samples_test    = m.no_samples_test;
      keep_distributions = m.keep_distributions;
      verbosity          = m.verbosity;
      do_exact_match     = m.do_exact_match;
      sock_os            = 0;
      globalMetricOption = m.globalMetricOption;
      if ( m.GlobalMetric )
	GlobalMetric     = getMetricClass( m.GlobalMetric->type() );
      UserOptions        = m.UserOptions;
      mvd_threshold      = m.mvd_threshold;
      num_of_neighbors   = m.num_of_neighbors;
      dynamic_neighbors  = m.dynamic_neighbors;
      num_of_features    = m.num_of_features;
      target_pos         = m.target_pos;
      progress           = m.progress;
      Bin_Size           = m.Bin_Size;
      tribl_offset       = m.tribl_offset;
      ib2_offset         = m.ib2_offset;
      clip_factor        = m.clip_factor;
      runningPhase       = m.runningPhase;
      Weighting          = m.Weighting;
      do_sloppy_loo      = m.do_sloppy_loo;
      do_silly_testing   = m.do_silly_testing;
      do_diversify       = m.do_diversify;
      permutation = m.permutation;
      tester = 0;
      decay = 0;
      Features  = m.Features;
      PermFeatures = m.PermFeatures;
      for ( unsigned int i=0; i < Features.size(); ++i ){
	Features[i] = new Feature( *m.Features[i] );
	if ( m.PermFeatures[i] )
	  PermFeatures[i] = Features[permutation[i]];
	else
	  PermFeatures[i] = 0;
      }
      Targets   = m.Targets;
      err_count = 0;
      MBL_init = false;
      need_all_weights = false;
      InstanceBase = m.InstanceBase->Copy();
      TargetStrings = m.TargetStrings;
      FeatureStrings = m.FeatureStrings;
      effective_feats = m.effective_feats;
      num_of_num_features    = m.num_of_num_features;
      DBEntropy = -1.0;
      ChopInput = 0;
      setInputFormat( m.input_format );
      //one extra to store the target!
      CurrInst.Init( num_of_features );
      myerr = m.myerr;
      mylog = m.mylog;
    }
    return *this;
  }

  MBLClass::~MBLClass(){
    CurrInst.clear();
    if ( !is_copy ){
      delete InstanceBase;
      delete Targets;
      delete TargetStrings;
      delete FeatureStrings;
    }
    else {
      if ( is_synced ){
	delete InstanceBase;
      }
      else {
	InstanceBase->CleanPartition( false );
      }
    }
    for ( auto const& feat : Features ){
      delete feat;
    }
    delete GlobalMetric;
    delete tester;
    delete decay;
    delete ChopInput;
  }


  void MBLClass::Info( const string& out_line ) const {
#pragma omp critical
    {
      // Info NEVER to socket !
      if ( exp_name != "" )
	*mylog << "-" << exp_name << "-" << out_line << endl;
      else
	*mylog << out_line << endl;
    }
  }

  void MBLClass::Warning( const string& out_line ) const {
#pragma omp critical
    {
      if ( sock_os )
	*sock_os << "ERROR { " << out_line << " }" << endl;
      else {
	if ( exp_name != "" )
	  *myerr << "Warning:-" << exp_name << "-" << out_line << endl;
	else
	*myerr << "Warning: " << out_line << endl;
      }
    }
  }

  void MBLClass::Error( const string& out_line ) const {
    if ( sock_os )
      *sock_os << "ERROR { " << out_line << " }"  << endl;
    else {
      if ( exp_name != "" )
	*myerr << "Error:-" << exp_name << "-" << out_line << endl;
      else
	*myerr << "Error: " << out_line << endl;
    }
    err_count++;
  }

  void MBLClass::FatalError( const string& out_line ) const {
    if ( sock_os )
      *sock_os << "ERROR { " << out_line << " }" << endl;
    else {
      if ( exp_name != "" )
	*myerr << "-" << exp_name << "-";
      if ( exp_name != "" )
	*myerr << "FatalError:-" << exp_name << "-" << out_line << endl;
      else
	*myerr << "FatalError: " << out_line << endl;
      throw( runtime_error("Stopped") );
    }
  }

  bool MBLClass::ShowOptions( ostream& os ) const {
    os << "Possible Experiment Settings (current value between []):" << endl;
    Options.Show_Options( os );
    os << endl;
    return true;
  }

  bool MBLClass::ShowSettings( ostream& os ) const{
    os << "Current Experiment Settings :" << endl;
    Options.Show_Settings( os );
    os << endl;
    return true;
  }

  bool MBLClass::connectToSocket( ostream *ss ){
    if ( sock_os ){
      throw( logic_error( "connectToSocket:: already connected!" ) );
    }
    else {
      sock_os = ss;
      if ( sock_os && sock_os->good() ){
	return true;
      }
      else {
	FatalError( "connecting streams to socket failed" );
      }
    }
    return false;
  }

  xmlNode *MBLClass::settingsToXml() const{
    ostringstream tmp;
    Options.Show_Settings( tmp );
    vector<string> lines;
    int num = TiCC::split_at( tmp.str(), lines, "\n" );
    xmlNode *result = TiCC::XmlNewNode("settings");
    for ( int i=0; i < num; ++i ){
      vector<string> parts;
      if ( TiCC::split_at( lines[i], parts, ":" ) ==2 ){
	string tag = TiCC::trim( parts[0] );
	string val = TiCC::trim( parts[1] );
	TiCC::XmlNewTextChild( result, tag, val );
      }
    }
    return result;
  }

  bool MBLClass::ShowWeights( ostream &os ) const {
    if ( ExpInvalid() )
      return false;
    else {
      int OldPrec = os.precision(DBL_DIG);
      size_t pos = 0;
      for ( auto const& feat : Features ){
	os.precision(DBL_DIG);
	os << "Feature " << ++pos << "\t : " << feat->Weight() << endl;
      }
      os.precision(OldPrec);
    }
    return true;
  }

  void MBLClass::calculatePermutation( const vector<double>& W ){
    vector<double> WR = W;
    size_t IgnoredFeatures = 0;
    permutation.resize(num_of_features);
    for ( size_t j=0; j < num_of_features; ++j ){
      permutation[j] = j;
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
	permutation[k] = Max;
      }
    }
  }

  string MBLClass::extract_limited_m( size_t lim ){
    default_order();
    set_order();
    string result;
    MetricType gm = globalMetricOption;
    result += TiCC::toString( gm );
    set<size_t> ignore;
    map<string,set<size_t>> metrics;
    // using TiCC::operator<<;
    // cerr << "permutation: " << permutation << endl;
    for ( size_t k=0; k < num_of_features; ++k ){
      if ( Features[permutation[k]]->Ignore() ){
	//	cerr << "Add " << k+1 << " to ignore" << endl;
	ignore.insert(k+1);
      }
      else {
	MetricType m = Features[permutation[k]]->getMetricType();
	if ( m != gm ){
	  metrics[TiCC::toString( m )].insert(k+1);
	}
      }
    }
    // cerr << "lim=" << lim << " so start ignoring at: "
    //  	 << lim + ignore.size()<< endl;

    for ( size_t i=lim+ignore.size(); i < num_of_features; ++i ){
      //      cerr << "Add " << permutation[i]+1 << " to ignore" << endl;
      ignore.insert( permutation[i]+1 );
    }
    if ( !ignore.empty() ){
      result += ":I";
      // using TiCC::operator<<;
      // cerr << "IGNORE bevat:" << ignore << endl;
      for ( auto it = ignore.begin(); it != ignore.end(); ++it ){
	size_t value = *it;
	//	cerr << "START it=" << *it << " value = " << value << endl;
	size_t steps = 0;
	for ( ; value <= *ignore.rbegin(); ++value ){
	  //	  cerr << "value = " << value << endl;
	  if ( ignore.find(value) == ignore.end() ){
	    break;
	  }
	  ++steps;
	}
	// cerr << "END it=" << *it << " value = " << value << endl;
	// cerr << "STEPS =" << steps << endl;
	if ( value == *it+1 ){
	  // so only one value, output it
	  if ( *it != *ignore.begin() ){
	    result += ",";
	  }
	  result += TiCC::toString(*it) + ",";
	}
	else if ( value == *it+2 ){
	  // so only two values, output them , separated
	  result += TiCC::toString(*it);
	  ++it;
	  result += "," + TiCC::toString(*it);
	}
	else {
	  // a range. output with a hyphen
	  result += TiCC::toString(*it) + "-" + TiCC::toString( value-1) + ",";
	  //	  cerr << "advance it met " << steps-1 << endl;
	  for ( size_t j=0; j < steps-1;++j){
	    ++it;
	    if ( it == ignore.end() ){
	      --it;
	      break;
	    }
	  }
	}
      }
      result.pop_back();
    }
    result += ":";
    for ( const auto& it : metrics ){
      bool first = true;
      for ( const auto& ig : it.second ){
	if ( ignore.find( ig ) == ignore.end() ){
	  if ( first ){
	    result += it.first;
	    first = false;
	  }
	  result += TiCC::toString( ig ) + ",";
	}
      }
      if ( result.back() == ',' ){
	result.pop_back();
	result.push_back(':');
      }
    }
    while ( result.back() == ':'
	    || result.back() == ',' ){
      result.pop_back();
    }
    return result;
  }

  void MBLClass::writePermutation( ostream& os ) const {
    os << "Feature Permutation based on "
       << ( Weighting==UserDefined_w?"weightfile":TiCC::toString(TreeOrder, true))
       << " :" << endl << "< ";
    for ( size_t j=0; j < num_of_features-1; ++j ){
      os << permutation[j]+1 << ", ";
    }
    os << permutation[num_of_features-1]+1 << " >" << endl;
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
      ostr << TiCC::Timer::now();
      Info( ostr.str() );
    }
  }

  void MBLClass::InitWeights(void){
    for ( auto const& feat : Features ){
      if ( feat->Ignore() )
	feat->SetWeight( 0.0 );
      else
	switch ( Weighting ){
	case IG_w:
	  feat->SetWeight( feat->InfoGain() );
	  break;
	case GR_w:
	  feat->SetWeight( feat->GainRatio() );
	  break;
	case X2_w:
	  feat->SetWeight( feat->ChiSquare() );
	  break;
	case SV_w:
	  feat->SetWeight( feat->SharedVariance() );
	  break;
	case SD_w:
	  feat->SetWeight( feat->StandardDeviation() );
	  break;
	case UserDefined_w:
	  break;
	case No_w:
	  feat->SetWeight( 1.0 );
	  break;
	case Unknown_w:
	case Max_w:
	  FatalError( "InitWeights: Invalid Weight in switch: " +
		      TiCC::toString( Weighting ) );
	  break;
	}
    }
  }

  void MBLClass::diverseWeights(void){
    double minW = DBL_MAX;
    for ( auto const& feat : Features ){
      if ( feat->Ignore() )
	continue;
      if ( feat->Weight() < minW ){
	minW =  feat->Weight();
      }
    }
    for ( auto const& feat : Features ){
      if ( feat->Ignore() )
	continue;
      feat->SetWeight( (feat->Weight() - minW ) + Epsilon );
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
      case SD_w:
	TreeOrder = SDOrder;
	break;
      case No_w:
	TreeOrder = NoOrder;
	break;
      case UserDefined_w:
	TreeOrder = GROrder;
	break;
      default:
	FatalError( "Illegal Weighting Value in Switch: " +
		    TiCC::toString( Weighting ) );
	break;
      }
  }

  void MBLClass::set_order(){
    calculate_fv_entropy(false);
    vector<double> Order(num_of_features);
    size_t i = 0;
    for ( auto const& feat : Features ){
      switch( TreeOrder ){
      case DataFile:
	Order[i] = feat->Weight();
	break;
      case NoOrder:
	Order[i] = (double)(num_of_features-i);
	break;
      case IGOrder:
	Order[i] = feat->InfoGain();
	break;
      case GROrder:
	Order[i] = feat->GainRatio();
	break;
      case IGEntropyOrder:
	Order[i] = feat->InfoGain() * feat->SplitInfo();
	break;
      case GREntropyOrder:
	Order[i] = feat->GainRatio() * feat->SplitInfo();
	break;
      case X2Order:
	Order[i] = feat->ChiSquare();
	break;
      case SVOrder:
	Order[i] = feat->SharedVariance();
	break;
      case SDOrder:
	Order[i] = feat->StandardDeviation();
	break;
      case OneoverFeature:
	Order[i] =  1.0 / feat->ValuesArray.size();
	break;
      case GRoverFeature:
	Order[i] =  feat->GainRatio() / feat->ValuesArray.size();
	break;
      case IGoverFeature:
	Order[i] =  feat->InfoGain() / feat->ValuesArray.size();
	break;
      case X2overFeature:
	Order[i] =  feat->ChiSquare() / feat->ValuesArray.size();
	break;
      case SVoverFeature:
	Order[i] =  feat->SharedVariance() / feat->ValuesArray.size();
	break;
      case SDoverFeature:
	Order[i] =  feat->StandardDeviation() / feat->ValuesArray.size();
	break;
      case OneoverSplitInfo:
	Order[i] =  1.0 / feat->SplitInfo();
	break;
      case UnknownOrdening:
      case MaxOrdening:
	FatalError( "Setorder: Illegal Order Value in Switch: " +
		    TiCC::toString( TreeOrder ) );
	break;
      }
      ++i;
    }
    calculatePermutation( Order );
    if ( !Verbosity(SILENT) )
      writePermutation( *mylog );
    for ( size_t j=0; j < num_of_features; ++j ){
      if ( j < effective_feats )
	PermFeatures[j] = Features[permutation[j]];
      else
	PermFeatures[j] = NULL;
    }
  }

  void MBLClass::MatrixInfo( ostream& os ) const {
    unsigned int TotalCount = 0;
    bool dummy;
    size_t m = 1;
    for ( const auto& feat : Features ){
      if ( !feat->Ignore() &&
	   feat->isStorableMetric() &&
	   feat->matrixPresent( dummy ) ){
	unsigned int Count = feat->matrix_byte_size();
	os << "Size of value-matrix[" << m << "] = "
	   << Count << " Bytes " << endl;
	TotalCount += Count;
      }
      ++m;
    }
    if ( TotalCount )
      os << "Total Size of value-matrices " << TotalCount << " Bytes "
	 << endl << endl;
  }

  bool MBLClass::readMatrices( istream& is ){
    string line;
    bool skip = false;
    bool anything = false;
    while ( getline( is, line ) ){
      line = TiCC::trim( line );
      if ( line.empty() )
	continue;
      if ( line.compare( 0, 7, "Feature" ) != 0 ){
	if ( skip )
	  continue;
	else
	  return false;
      }
      else {
	skip = false;
	line = line.substr( 8 );
	string::size_type pos = line.find_first_not_of("0123456789");
	string nums = line.substr( 0, pos );
	size_t num;
	if ( !TiCC::stringTo( nums, num ) ){
	  FatalError( "no feature index found in the inputfile" );
	}
	else {
	  if ( pos == string::npos )
	    line = "";
	  else {
	    line = TiCC::trim( line.substr( pos ) );
	  }
	  if ( line.empty() ){
	    if ( !Features[num-1]->isStorableMetric() ){
	      Warning( "Ignoring entry for feature " + nums
		       + " which is NOT set to a storable metric type."
		       + " use -m commandline option to set metrics" );
	      skip = true;
	    }
	    else if ( !Features[num-1]->fill_matrix( is ) )
	      return false;
	    else {
	      Info( "read ValueMatrix for feature " + nums );
	      anything = true;
	    }
	  }
	}
      }
    }
    if ( !anything ){
      Error( "NO metric values found" );
      return false;
    }
    return true;
  }

  bool MBLClass::writeMatrices( ostream& os ) const {
    size_t pos = 0;
    for ( const auto& feat : Features ){
      os << "Feature " << ++pos;
      bool dummy;
      if ( !feat->matrixPresent(  dummy ) ){
	os << " not available.\n" << endl;
      }
      else {
	os << endl;
	feat->print_matrix( os );
      }
    }
    return os.good();
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
	    Error( "Wrong feature number " + TiCC::toString<size_t>(num) +
		   " in file, " + TiCC::toString<size_t>(index) + " expected" );
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
		Error( "Feature #" + TiCC::toString<size_t>(index) +
		       " may not be ignored...");
		result = false;
	      }
	    }
	    else if ( compare_nocase_n( "Numeric", buf ) ){
	      if ( Features[index-1]->isNumerical() ){
		++index;
		continue;
	      }
	      else {
		Error( "Feature #" + TiCC::toString<size_t>(index) + " is not Numeric..." );
		result = false;
	      }
	    }
	    else if ( !compare_nocase_n( "Matrix", buf ) ){
	      Error( "Problem in Probability file, missing matrix info" );
	      result = false;
	    }
	    else if ( Features[index-1]->Ignore() ||
		      Features[index-1]->isNumerical() ){
	      Warning( "Matrix info found for feature #"
		       + TiCC::toString<size_t>(index)
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
    while ( result && !is.eof() && !is.bad() );
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
      size_t pos = 0;
      for ( const auto& feat : Features ){
	os << "feature # " << ++pos ;
	if ( feat->Ignore() ){
	  os << " Ignored, (-s option)" << endl;
	}
	else if ( feat->isNumerical() ){
	  os << " Numeric, (-N option)" << endl;
	}
	else {
	  os << " Matrix: " << endl;
	  feat->print_vc_pb_array( os );
	  os << endl;
	}
      }
      return true;
    }
  }

  bool MBLClass::allocate_arrays(){
    size_t Dim = Targets->ValuesArray.size();
    for ( const auto& feat : Features ){
      if ( !feat->Ignore() &&
	   !feat->isNumerical() ) {
	if ( !feat->AllocSparseArrays( Dim ) )
	  return false;
      }
    }
    return true;
  }

  bool MBLClass::initProbabilityArrays( bool force ){
    bool result = true;
    if ( !is_copy ){
      result = allocate_arrays();
      if ( result ){
	for ( const auto& feat : Features ){
	  if ( !feat->Ignore() &&
	       !feat->isNumerical() ){
	    feat->ClipFreq( (int)rint(clip_factor *
				      log((double)feat->EffectiveValues())));
	    if ( !feat->ArrayRead() &&
		 ( force ||
		   feat->isStorableMetric() ) ){
	      feat->InitSparseArrays();
	    }
	  }
	} // j
      }
    }
    return result;
  }

  /*
    For mvd metric.
  */
  void MBLClass::calculatePrestored(){
    if ( !is_copy ){
      for ( size_t j = tribl_offset; j < effective_feats; ++j ) {
	if ( !PermFeatures[j]->Ignore() &&
	     PermFeatures[j]->isStorableMetric() ){
	  PermFeatures[j]->store_matrix( mvd_threshold );
	}
      }
      if ( Verbosity(VD_MATRIX) ){
	size_t pos = 0;
	for ( auto const& feat : Features ){
	  ++pos;
	  if ( !feat->Ignore() ){
	    bool dummy;
	    *mylog << "Value Difference matrix of feature # "
		   << pos << endl;
	    if ( feat->matrixPresent( dummy ) ){
	      feat->print_matrix( *mylog, true );
	      *mylog << endl;
	    }
	    else {
	      *mylog << "Not available." << endl;
	    }
	  }
	}
      }
    }
  }

  const Instance *MBLClass::chopped_to_instance( PhaseValue phase ){
    CurrInst.clear();
    if ( num_of_features != target_pos ) {
      ChopInput->swapTarget( target_pos );
    }
    int occ = ChopInput->getOcc();
    if ( occ > 1 ){
      CurrInst.Occurrences( occ );
    }
    switch ( phase  ){
    case LearnWords:
      // Add the target.
      CurrInst.TV = Targets->add_value( ChopInput->getField( num_of_features ),
					occ );
      // Now add the Feature values.
      for ( size_t i = 0; i < num_of_features; ++i ){
	// when learning, no need to bother about Permutation
	if ( Features[i]->Ignore() ) // but this might happen, take care!
	  CurrInst.FV[i] = NULL;
	else {
	  // Add it to the Instance.
	  CurrInst.FV[i] = Features[i]->add_value( ChopInput->getField(i),
						   CurrInst.TV, occ );
	}
      } // i
      break;
    case TrainWords:
      // Lookup for TreeBuilding
      // First the Features
      for ( size_t k = 0; k < effective_feats; ++k ){
	size_t j = permutation[k];
	CurrInst.FV[k] = Features[j]->Lookup( ChopInput->getField(j) );
      } // k
      // and the Target
      CurrInst.TV = Targets->Lookup( ChopInput->getField( num_of_features ) );
      break;
    case TrainLearnWords:
      // Lookup for Incremental TreeBuilding
      // Assumes that somehow Permutation and effective_feats are known
      // First the Target
      CurrInst.TV = Targets->add_value( ChopInput->getField(num_of_features ),
					occ );
      // Then the Features
      for ( size_t l = 0; l < effective_feats; ++l ){
	size_t j = permutation[l];
	CurrInst.FV[l] = Features[j]->add_value( ChopInput->getField(j),
						 CurrInst.TV, occ );
      } // for l
      break;
    case TestWords:
      // Lookup for Testing
      // This might fail for unknown values, then we create a dummy value
      for ( size_t m = 0; m < effective_feats; ++m ){
	size_t j = permutation[m];
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
		  + TiCC::toString<PhaseValue>(phase) );
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

  string MBLClass::get_org_input( ) const {
    return ChopInput->getString();
  }

  void MBLClass::LearningInfo( ostream& os ) {
    if ( !ExpInvalid() && !Verbosity(SILENT) ){
      calculate_fv_entropy( !MBL_init );
      os.setf(ios::showpoint );
      int OldPrec = os.precision(8);
      os << "DB Entropy        : " << DBEntropy << endl;
      os << "Number of Classes : " << Targets->EffectiveValues() << endl;
      os << endl;
      if ( Verbosity(FEAT_W) ){
	if (  CurrentWeighting() == SD_w ){
	  os << "Feats\tVals\tStandard Deviation" << endl;
	  size_t pos = 0;
	  for ( const auto& feat : Features ){
	    os << setw(5) << ++pos;
	    os.setf(ios::right, ios::adjustfield);
	    if ( feat->Ignore() ){
	      os << " (ignored) " << endl;
	    }
	    else {
	      os.setf(ios::right, ios::adjustfield);
	      os << setw(7) << feat->EffectiveValues()
		 << "\t" << feat->StandardDeviation();
	      if ( feat->isNumerical() )
		os << " NUMERIC";
	      os << endl;
	    }
	  }
	  os << endl;
	  os.precision(OldPrec);
	}
	else if ( need_all_weights ){
	  os << "Feats\tVals\tX-square\tVariance\tInfoGain\tGainRatio" << endl;
	  size_t pos = 0;
	  for ( const auto& feat : Features ) {
	    os << setw(5) << ++pos;
	    os.setf(ios::right, ios::adjustfield);
	    if ( feat->Ignore() ){
	      os << " (ignored) " << endl;
	    }
	    else {
	      os.setf(ios::right, ios::adjustfield);
	      os << setw(7) << feat->EffectiveValues()
		 << "\t" << feat->ChiSquare()
		 << "\t" << feat->SharedVariance()
		 << "\t" << feat->InfoGain()
		 << "\t" << feat->GainRatio();
	      if ( feat->isNumerical() )
		os << " NUMERIC";
	      os << endl;
	    }
	  }
	  os << endl;
	  os.precision(OldPrec);
	}
	else {
	  os << "Feats\tVals\tInfoGain\tGainRatio" << endl;
	  size_t pos = 0;
	  for ( const auto& feat : Features ) {
	    os << setw(5) << ++pos;
	    os.setf(ios::right, ios::adjustfield);
	    if ( feat->Ignore() ){
	      os << " (ignored) " << endl;
	    }
	    else {
	      os.setf(ios::right, ios::adjustfield);
	      os << setw(7) << feat->EffectiveValues()
		 << "\t" << feat->InfoGain()
		 << "\t" << feat->GainRatio();
	      if ( feat->isNumerical() )
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
	if ( CurrentWeighting() == SD_w ){
	  os << "#" << endl;
	  os << "# " << TiCC::toString( SD_w ) << endl;
	  os << "# Fea." << "\t" << "Weight" << endl;
	  size_t pos = 0;
	  for ( const auto& feat : Features ){
	    os.precision(DBL_DIG);
	    os << ++pos << "\t";
	    if ( feat->Ignore() )
	      os << "Ignore" << endl;
	    else
	      os << feat->StandardDeviation() << endl;
	  }
	  os << "#" << endl;
	}
	else {
	  os << "# " << TiCC::toString( No_w ) << endl;
	  os << "# Fea." << "\t" << "Weight" << endl;
	  size_t pos = 0;
	  for (  const auto& feat : Features ){
	    os.precision(DBL_DIG);
	    os << ++pos << "\t";
	    if ( feat->Ignore() )
	      os << "Ignore" << endl;
	    else
	      os << 1.0 << endl;
	  }
	  os << "#" << endl;
	  os << "# " << TiCC::toString( GR_w ) << endl;
	  os << "# Fea." << "\t" << "Weight" << endl;
	  pos = 0;
	  for (  const auto& feat : Features ){
	    os.precision(DBL_DIG);
	    os << ++pos << "\t";
	    if ( feat->Ignore() )
	      os << "Ignore" << endl;
	    else
	      os << feat->GainRatio() << endl;
	  }
	  os << "#" << endl;
	  os << "# " << TiCC::toString( IG_w ) << endl;
	  os << "# Fea." << "\t" << "Weight" << endl;
	  pos = 0;
	  for (  const auto& feat : Features ){
	    os.precision(DBL_DIG);
	    os << ++pos << "\t";
	    if ( feat->Ignore() )
	      os << "Ignore" << endl;
	    else
	      os << feat->InfoGain() << endl;
	  }
	  if ( need_all_weights ){
	    os << "#" << endl;
	    os << "# " << TiCC::toString( SV_w ) << endl;
	    os << "# Fea." << "\t" << "Weight" << endl;
	    pos = 0;
	    for (  const auto& feat : Features ){
	      os.precision(DBL_DIG);
	      os << ++pos << "\t";
	      if ( feat->Ignore() )
		os << "Ignore" << endl;
	      else
		os << feat->SharedVariance() << endl;
	    }
	    os << "#" << endl;
	    os << "# " << TiCC::toString( X2_w ) << endl;
	    os << "# Fea." << "\t" << "Weight" << endl;
	    pos = 0;
	    for (  const auto& feat : Features ){
	      os.precision(DBL_DIG);
	      os << ++pos << "\t";
	      if ( feat->Ignore() )
		os << "Ignore" << endl;
	      else
		os << feat->ChiSquare() << endl;
	    }
	    os << "#" << endl;
	  }
	}
	os.precision(OldPrec);
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
	if ( TiCC::split( Buffer, vals ) == 2 ){
	  size_t i_f = TiCC::stringTo<size_t>( vals[0] );
	  if ( i_f > num_of_features ){
	    Error( "in weightsfile, Feature index > Maximum, (" +
		   TiCC::toString<size_t>(num_of_features) + ")" );
	  }
	  else if ( done[i_f-1] ){
	    Error( "in weightsfile, Feature index " + vals[0] +
		   " is mentioned twice" );
	  }
	  else {
	    done[i_f-1] = true;
	    if ( !compare_nocase( vals[1], "Ignore" ) ){
	      double w;
	      if ( !TiCC::stringTo<double>( vals[1], w  ) ){
		Error( "in weightsfile, Feature " + vals[0] +
		       " has illegal value: " + vals[1] );
	      }
	      else {
		Features[i_f-1]->SetWeight( w );
		if ( Features[i_f-1]->Ignore() )
		  Warning( "in weightsfile, "
			   "Feature " + vals[0] + " has value: " +
			   TiCC::toString<double>( w ) +
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
	  Error( "in weightsfile, Feature index " + TiCC::toString<size_t>(j+1) +
		 " is not mentioned" );
	  result = false;
	}
    }
    delete [] done;
    return result;
  }

  bool MBLClass::readWeights( istream& is, WeightType wanted ){
    if ( !ExpInvalid() ){
      bool old_style = true;
      bool result = false;
      string Buffer;
      while( getline( is, Buffer ) ) {
	// A comment starts with '#'
	//
	if ( Buffer.empty() )
	  continue;
	else {
	  if ( Buffer[0] == '#'){
	    vector<string> vals;
	    if ( TiCC::split_at( Buffer, vals, " " ) == 2  ){
	      WeightType tmp_w = Unknown_w;
	      if ( !TiCC::stringTo<WeightType>( vals[1], tmp_w ) )
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
		 + TiCC::toString( wanted ) + " Weights" );
	Warning( "unable to continue" );
	return false;
      }
      // make shure all weights are correct
      // Paranoid?
      for ( const auto& feat : Features ){
	feat->InfoGain( feat->Weight() );
	feat->GainRatio( feat->Weight() );
	feat->ChiSquare( feat->Weight() );
	feat->SharedVariance( feat->Weight() );
	feat->StandardDeviation( 0.0 );
      }
      Weighting = UserDefined_w;
    }
    return true;
  }

  void MBLClass::calculate_fv_entropy( bool always ){
    bool realy_first =  DBEntropy < 0.0;
    if ( always || realy_first ){
      // if it's the first time (DBEntropy == 0 ) or
      // if always, we have to (re)calculate everything
      double Entropy = 0.0;
      // first get the Database Entropy
      size_t totval = Targets->TotalValues();
      VCarrtype::const_iterator it = Targets->ValuesArray.begin();
      while ( it != Targets->ValuesArray.end() ){
	double Ratio = (*it)->ValFreq() /
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
    vector<FeatVal_Stat> feat_status(num_of_features);
    bool nothing_changed = true;
    for ( size_t g = 0; g < num_of_features; ++g ) {
      feat_status[g] = Unknown;
      if ( Features[g]->Ignore() )
	continue;
      bool metricChanged = false;
      MetricType TmpMetricType = UserOptions[g+1];
      metricClass *tmpMetric = getMetricClass( TmpMetricType );
      if ( tmpMetric->isNumerical() ){
	feat_status[g] = Features[g]->prepare_numeric_stats();
	if ( feat_status[g] == SingletonNumeric &&
	     input_format == SparseBin &&
	     GlobalMetric->isSimilarityMetric( ) ){
	  // ok
	}
	else if ( feat_status[g] != NumericValue ){
	  if ( GlobalMetric->isNumerical() ){
	    TmpMetricType = Overlap;
	  }
	  else {
	    TmpMetricType = globalMetricOption;
	  }
	}
      }
      else if ( Features[g]->ValuesArray.size() == 1 )
	feat_status[g] = Singleton;
      delete tmpMetric;
      if ( always || realy_first ){
	bool isRead;
	if ( Features[g]->metric &&
	     Features[g]->getMetricType() != TmpMetricType &&
	     Features[g]->isStorableMetric() &&
	     Features[g]->matrixPresent( isRead ) &&
	     isRead ){
	  Error( "The metric " + TiCC::toString(Features[g]->getMetricType()) +
		 " for feature " + TiCC::toString( g+1 ) +
		 " is set from a file. It cannot be changed!" );
	  return;
	}
	metricChanged = !Features[g]->setMetricType(TmpMetricType);
      }
      if ( metricChanged )
	nothing_changed = false;
    } // end g
    if ( ( CurrentWeighting() == SD_w ||
	   GlobalMetric->isSimilarityMetric() )
	 && !nothing_changed ){
      // check to see if ALL features are still Numeric.
      // otherwise we can't do Standard Deviation weighting,
      // or Similarity Metrics!
      bool first = true;
      ostringstream ostr1;
      for ( size_t ff = 0; ff < num_of_features; ++ff ){
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
      }
      if ( !first  ){
	Error( ostr1.str() );
	if ( GlobalMetric->isSimilarityMetric() )
	  Error( "Therefore InnerProduct/Cosine operations are impossible" );
	else
	  Error( "Therefore " + TiCC::toString(CurrentWeighting()) + " weighting is impossible" );
	return;
      }
    }
    // Give a warning for singular features, except when it's
    // a result of a forced recalculation
    if ( realy_first ){
      bool first = true;
      ostringstream ostr1;
      ostringstream ostr2;
      for ( size_t ff = 0; ff < num_of_features; ++ff ) {
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
      }
      if ( !first && !is_copy ){
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
    if ( always || realy_first ){
      for ( const auto& feat : Features ){
	if ( Weighting != UserDefined_w ){
	  if ( CurrentWeighting() == SD_w )
	    feat->StandardDeviationStatistics( );
	  else if ( feat->isNumerical() ){
	    feat->NumStatistics( DBEntropy, Targets, Bin_Size,
				 need_all_weights );
	  }
	  else {
	    feat->Statistics( DBEntropy, Targets, need_all_weights );
	  }
	}
      }
    }
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
      size_t pos = 0;
      for ( auto const& feat : Features ){
	os << "a" << ++pos << ": ";
	if ( feat->Ignore() )
	  os << "Ignore" << endl;
	else if ( feat->isNumerical() )
	  os << "Numeric" << endl;
	else {
	  // Loop over the values.
	  //
	  auto it2 = feat->ValuesArray.begin();
	  while( it2 != feat->ValuesArray.end() ){
	    os << (FeatureValue *)*it2;
	    ++it2;
	    if ( it2 != feat->ValuesArray.end() )
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
      return ChopInput->chop( line, num_of_features );
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
    ChopInput = Chopper::create( IF, chopExamples(), F_length, chopOcc() );
    if ( ChopInput ){
      input_format = IF;
      return true;
    }
    return false;
  }

  const ValueDistribution *MBLClass::ExactMatch( const Instance& inst ) const {
    const ValueDistribution *result = NULL;
    if ( !GlobalMetric->isSimilarityMetric() &&
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
      InvPerm[permutation[i]] = i;
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
	  result += string("(")  + TiCC::toString<size_t>(j+1) + ","
	    + CodeToStr( inst.FV[InvPerm[j]]->Name() ) + ")";
	break;
      case SparseBin:
	if ( inst.FV[InvPerm[j]]->Name()[0] == '1' )
	  result += TiCC::toString<size_t>( j+1 ) + ",";
	break;
      case Columns:
	if ( Features[j]->Ignore() )
	  result += "-*- ";
	else
	  result += inst.FV[InvPerm[j]]->Name() + " ";
	break;
      case Tabbed:
	if ( Features[j]->Ignore() )
	  result += "-*- ";
	else
	  result += inst.FV[InvPerm[j]]->Name() + "\t";
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
      // call test() with a maximum threshold, to prevent stepping out early
      size_t EndPos  = tester->test( CurrentFV,
				     CurPos,
				     DBL_MAX );
      if ( EndPos != EffFeat ){
	throw( logic_error( "Exemplar testing: test should not stop before last feature" ) );
      }
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
    delete GlobalMetric;
    GlobalMetric = getMetricClass( globalMetricOption );
    delete tester;
    tester = getTester( globalMetricOption, Features, permutation, mvd_threshold );
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
    //    cerr << "start test Instance = " << &Inst << " met " << TiCC::toString(CurrentFV) << endl;
    //    cerr << "BA at start = " << bestArray << endl;
    size_t CurPos = 0;
    while ( best_distrib ){
      //      cerr << "test:" << TiCC::toString(CurrentFV) << endl;
      size_t EndPos = tester->test( CurrentFV,
				    CurPos,
				    Threshold + Epsilon );
      //      cerr << "EndPos = " << EndPos << endl;
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
	  //	  cerr << "Ok add " << best_distrib << "at distance " << Distance << endl;
	  Threshold = bestArray.addResult( Distance, best_distrib, origI );
	  //	  cerr << "BA = " << bestArray << endl;
	  if ( do_silly_testing )
	    Threshold = DBL_MAX;
	}
	else {
	  Error( "DISTANCE == " + TiCC::toString<double>(Distance) );
	  FatalError( "we are dead" );
	}
      }
      else {
	EndPos++; // out of luck, compensate for roll-back
      }
      size_t pos=EndPos-1;
      //      cerr << "start rollback " << pos << endl;
      while ( true ){
	//	cerr << "rollback " << pos << endl;
	if ( tester->getDistance(pos) <= Threshold ){
	  CurPos = pos;
	  //	  cerr << "voor next test " << endl;
	  best_distrib = IB->NextGraphTest( CurrentFV,
					    CurPos );
	  //	  cerr << "na next test, curpos=" << CurPos << "-" << TiCC::toString(CurrentFV) << endl;
	  break;
	}
	if ( pos == 0 )
	  break;
	--pos;
      }
    }
    //    cerr << "BA at end = " << bestArray << endl;
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
    while ( best_distrib ){
      double dummy_t = -1.0;
      size_t dummy_p = 0;
      // similarity::test() doesn't need CurPos, nor a Threshold
      // it recalculates the whole vector
      size_t EndPos = tester->test( CurrentFV,
				    dummy_p,
				    dummy_t );
      if ( EndPos == EffFeat ){
	// this should always be true!
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
	else if ( GlobalMetric->type() == DotProduct ){
	  Error( "The Dot Product metric fails on your data: intermediate result too big to handle," );
	  Info( "you might consider using the Cosine metric '-mC' " );
	  FatalError( "timbl terminated" );
	}
	else {
	  Error( "negative similarity DISTANCE: " + TiCC::toString<double>(Distance) );
	  FatalError( "we are dead" );
	}
      }
      else {
	throw( logic_error( "Similarity testing: test should consider all features" ) );
      }
      --EndPos;
      best_distrib = IB->NextGraphTest( CurrentFV, EndPos );
    }
  }

  void MBLClass::TestInstance( const Instance& Inst,
			       InstanceBase_base *SubTree,
			       size_t level ){
    // must be cleared for EVERY test
    if (  doSamples() ){
      test_instance_ex( Inst, SubTree, level );
    }
    else {
      if ( GlobalMetric->isSimilarityMetric( ) )
	test_instance_sim( Inst, SubTree, level );
      else
	test_instance( Inst, SubTree, level );
    }
  }

  size_t MBLClass::countFeatures( const string& inBuffer,
				  const InputFormatType IF ) const {
    size_t result = 0;
    if ( IF == Sparse  || IF == SparseBin )
      return num_of_features;
    else {
      try {
	result = Chopper::countFeatures( inBuffer, IF, F_length,
					 chopExamples() || chopOcc() );
      }
      catch( const runtime_error& e ){
	Error( e.what() );
      }
      catch( const exception& e ){
	FatalError( e.what() );
      }
    }
    return result;
  }

  InputFormatType MBLClass::getInputFormat( const string& inBuffer ) const {
    return Chopper::getInputFormat( inBuffer, chopExamples() || chopOcc() );
  }

  size_t MBLClass::examineData( const string& FileName ){
    // Looks at the data files, counts num_of_features.
    // and sets input_format variables.
    //
    size_t NumF = 0;
    InputFormatType IF = UnknownInputFormat;
    // Open the file.
    //
    if ( FileName == "" ) {
      Warning( "couldn't initialize: No FileName specified " );
      return 0;
    }
    else {
      string Buffer;
      ifstream datafile( FileName, ios::in);
      if (!datafile) {
	Warning( "can't open DataFile: " + FileName );
	return 0;
      }
      else if ( input_format != UnknownInputFormat ){
	// The format is somehow already known, so use that
	if ( input_format == SparseBin || input_format == Sparse )
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
	if ( NumF > MaxFeatures ){
	  Error( "Number of Features exceeds the maximum number. "
		 "(currently " + TiCC::toString<size_t>(MaxFeatures) +
		 ")\nPlease increase.\n" );
	  return 0;
	}
	setInputFormat( IF );
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
		  TiCC::toString<size_t>( num_of_features+1 ) );
    Features.resize(num_of_features,NULL);
    PermFeatures.resize(num_of_features,NULL);
    FeatureStrings = new Hash::StringHash(); // all features share the same hash
    TargetStrings = new Hash::StringHash(); // targets has it's own hash
    Targets = new Target( TargetStrings );
    for ( size_t i=0; i< num_of_features; ++i ){
      Features[i] = new Feature( FeatureStrings );
      PermFeatures[i] = NULL;
    }
    CurrInst.Init( num_of_features );
    effective_feats = num_of_features;
    num_of_num_features = 0;
    delete GlobalMetric;
    GlobalMetric = getMetricClass( globalMetricOption );
    // the user thinks about features running from 1 to Num
    // we know better, so shift the UserOptions one down.
    for ( size_t j = 0; j < num_of_features; ++j ){
      MetricType m = UserOptions[j+1];
      if ( m == Ignore ){
	Features[j]->Ignore( true );
	effective_feats--;
      }
      else {
	Features[j]->setMetricType( m );
	if ( Features[j]->isNumerical() ){
	  num_of_num_features++;
	}
      }
    }
    Options.FreezeTable();
    if ( Weighting > IG_w ||
	 TreeOrder >= X2Order )
      need_all_weights = true;
  }

} // namespace
