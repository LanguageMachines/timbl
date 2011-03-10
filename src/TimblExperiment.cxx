/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2011
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

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <typeinfo>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cfloat>
#include <cctype>
#include <cassert>
#include <cstdarg>

#include <sys/time.h>

#include "config.h"
#include "timbl/MsgClass.h"
#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/Tree.h"
#include "timbl/Instance.h"
#include "timbl/Choppers.h"
#include "timbl/Metrics.h"
#include "timbl/Statistics.h"
#include "timbl/neighborSet.h"
#include "timbl/BestArray.h"
#include "timbl/IBtree.h"
#include "timbl/MBLClass.h"
#include "timbl/CommandLine.h"
#include "timbl/GetOptClass.h"
#include "timbl/TimblExperiment.h"
#include "timbl/XMLtools.h"

#ifdef HAVE_OPENMP
#include <omp.h>
#endif

namespace Timbl {
  using namespace std;
  using namespace Timbl;

  resultStore::~resultStore( ) {
    clear();
  }

  bool resultStore::reset( int _beam, normType _norm, 
			   double _factor, const Target *_targets ) {
    clear();
    beam = _beam;
    norm = _norm;
    factor = _factor;
    targets = _targets;
    bool result = true;
    if ( norm != noNorm &&
	 beam != 0 ){
      norm = noNorm;
      result = false;
    }
    return result;
  }
  
  void resultStore::clear( ) {
    delete dist;
    dist = 0;
    if ( disposable )
      delete rawDist; 
    rawDist = 0;
    beam = 0;
    isTop = false;
    resultCache.clear();
  }
  
  const WValueDistribution *resultStore::getResultDist() {
    if ( rawDist && !dist )
      prepare();
    return dist;
  }
  
  string resultStore::getResult() {
    if ( isTop ){
      if ( topCache.empty() ){
	if ( dist )
	  topCache = dist->DistToStringW( beam );
	else
	  topCache = "{}";
      }
      resultCache = topCache;
    }
    else if ( resultCache.empty() ){
      if ( dist )
	resultCache = dist->DistToStringW( beam );
      else
	resultCache = "{}";
    }
    return resultCache;
  }
  
  void resultStore::addConstant( const ValueDistribution *vd ) {
    rawDist = vd;
    disposable = false;
  }

  void resultStore::addTop( const ValueDistribution *vd ) {
    rawDist = vd;
    disposable = false;
    isTop = true;
  }

  void resultStore::addDisposable( ValueDistribution *vd ) {
    rawDist = vd;
    disposable = true;
  }

  void resultStore::prepare() {
    if ( isTop && !topCache.empty() )
      return;
    if ( !dist && rawDist ) {
      if ( !disposable ){
	dist = rawDist->to_WVD_Copy();
      }
      else {
	dist = dynamic_cast<WValueDistribution *>( const_cast<ValueDistribution *>(rawDist) );
	rawDist = 0;
      }
    }
  }
  
  void resultStore::normalize() {
    switch ( norm ){
    case probabilityNorm:
      dist->Normalize();
      break;
    case addFactorNorm:
      dist->Normalize_1( factor, targets );
      break;
    default:
      break;
    }
  }

  void TimblExperiment::normalizeResult(){
    bestResult.prepare();
    bestResult.normalize();
  }
  
  TimblExperiment::TimblExperiment( const AlgorithmType Alg,
				    const string& s ):
    MBLClass( s ),
    Initialized( false ),
    OptParams( NULL ),
    algorithm( Alg ),
    CurrentDataFile( "" ),
    WFileName( "" ),
    ibCount( 0 ),
    confusionInfo( 0 ),
    match_depth(-1),
    last_leaf(true),
    estimate( 0 ),
    numOfThreads( 1 )
  {
    Weighting = GR_w;
  }
  
  TimblExperiment::~TimblExperiment() {
    delete OptParams;
    delete confusionInfo;
  }
  
  TimblExperiment& TimblExperiment::operator=( const TimblExperiment&in ){
    if ( this != &in ){
      MBLClass::operator=(in);
      Initialized = false;
      OptParams = NULL;
      algorithm = in.algorithm;
      CurrentDataFile = in.CurrentDataFile;
      WFileName = in.WFileName;
      estimate = in.estimate;
      Weighting = in.Weighting;
      confusionInfo = 0;
      numOfThreads = in.numOfThreads;
    }
    return *this;
  }
  
  TimblExperiment *TimblExperiment::splitChild( ) const {
    TimblExperiment *result = 0;
    switch ( Algorithm() ){
    case IB1_a:
    case TRIBL_a: 
    case TRIBL2_a: 
    case IGTREE_a: 
      result = clone();
      break;
    default:
      FatalError( "You may not split experiments for Special cases like " + 
		  toString(algorithm) );
    }
    *result = *this;
    if ( OptParams ){
      result->OptParams = OptParams->Clone( 0 );
    }
    result->WFileName = WFileName;
    result->CurrentDataFile = "";
    result->InstanceBase->CleanPartition( false );
    result->InstanceBase = 0;
    result->is_synced = true;
    return result;
  }  

  void TimblExperiment::initExperiment( bool all_vd ){ 
    if ( !ExpInvalid() ){
      match_depth = NumOfFeatures();
      if ( !MBL_init ){  // do this only when necessary
	stats.clear();
	delete confusionInfo;
	confusionInfo = 0;
	if ( Verbosity(ADVANCED_STATS) ){
	  confusionInfo = new ConfusionMatrix( Targets->ValuesArray.size() );
	}
	initDecay();
	calculate_fv_entropy( true );
	if ( ib2_offset != 0 ){
	  //
	  // isn't this obsolete for the new IB2 implementation?
	  // You should think so, But I see small differences :{
	  // So leave this code for now
	  //
	  // invalidate MVDM matrices, they might be changing in size
	  for ( size_t j=0; j < NumOfFeatures(); ++j ){
	    if ( !Features[j]->Ignore() ){
	      Features[j]->clear_matrix();
	    }
	  }
	}
	if ( initProbabilityArrays( all_vd ) )
	  calculatePrestored();
	else {
	  Error( string("not enough memory for Probability Arrays")
		 + "' in (" 
		 + __FILE__  + "," + toString(__LINE__) + ")\n"
		 + "ABORTING now" );
	  throw std::bad_alloc();
	}
	if (!is_copy ){
	  InitWeights();
	  if ( do_diversify )
	    diverseWeights();
	}
	srand( random_seed );
	initTesters();
	MBL_init = true;
      }
    }
  }
  
  bool TimblExperiment::skipARFFHeader( istream& is ){
    string Buffer;
    while ( getline( is, Buffer ) &&
	    !compare_nocase_n( "@DATA", Buffer) )
      stats.addSkipped();
    return true;
  }
  
  bool TimblExperiment::nextLine( istream& datafile, string& Line ){
    int dummy;
    return nextLine( datafile, Line, dummy );
  }

  bool TimblExperiment::nextLine( istream& datafile, string& Line, int& cnt ){
    // Function that takes a line from a file, skipping comment
    // returns true if some line is found
    //
    bool found = false;
    cnt = 0;
    while ( !found && getline( datafile, Line ) ){
      ++cnt;
      if ( empty_line( Line, InputFormat() ) ){
	stats.addSkipped();
	continue;
      } 
      else 
	found = true;
    }
    return found;
  }  

  bool TimblExperiment::chopLine( const string& Line ){
    if ( !Chop( Line ) ){
      stats.addSkipped();
      return false;
    }
    else {
      stats.addLine();
      return true;
    }
  }
  
  /*
    First learning Phase:
    Learning of the names of the FeatureValues and TargetValues
    also their distribution etc.
  */
  bool TimblExperiment::Prepare( const string& FileName,
				 bool warnOnSingleTarget,
				 bool expand ){
    assert( runningPhase == LearnWords );
    bool result = false;
    if ( FileName != "" && ConfirmOptions() ){
      if ( !ExpInvalid() ){
	if ( !expand &&
	     ( Options.TableFrozen() ||
	       NumOfFeatures() != 0 ) ){
	  Error( "couldn't learn from file '" + FileName +
		 "'\nInstanceBase already filled" );
	}
	else {
	  size_t Num = examineData( FileName );
	  if ( Num == 0 ){
	    Error( "Unable to initialize from file :'" + FileName + "'\n" );
	  }
	  else {
	    if ( !Verbosity(SILENT) ){
	      *mylog << "Examine datafile '" << FileName 
		     << "' gave the following results:"
		     << endl
		     << "Number of Features: " << Num << endl;
	      showInputFormat( *mylog );
	    }
	    if ( NumOfFeatures() == 0 ){
	      Initialize( Num );
	    }
	    CurrentDataFile = FileName;
	    if ( Verbosity(OPTIONS) ){
	      ShowSettings( *mylog );
	    }
	    // Open the file.
	    //
	    ifstream datafile( FileName.c_str(), ios::in);
	    stats.clear();
	    string Buffer;
	    if ( InputFormat() == ARFF )
	      skipARFFHeader( datafile );
	    if ( !nextLine( datafile, Buffer ) ){
	      Error( "no useful data in: " + FileName );
	      result = false;
	    }
	    else if ( !chopLine( Buffer ) ){
	      Error( "no useful data in: " + FileName );
	      result = false;
	    }
	    else {
	      Common::Timer prepT;
	      prepT.start();
	      bool found;
	      bool go_on = true;
	      if ( !Verbosity(SILENT) ){
		Info( "Phase 1: Reading Datafile: " + FileName );
		time_stamp( "Start:     ", 0 );
	      }
	      while( go_on ){
		chopped_to_instance( LearnWords );
		// Progress update.
		//
		if (( stats.dataLines() % Progress() ) == 0)
		  time_stamp( "Examining: ", stats.dataLines() );
		  found = false;
		  while ( !found && 
			  nextLine( datafile, Buffer ) ){
		    found = chopLine( Buffer );
		    if ( !found ){
		      Warning( "datafile, skipped line #" + 
			       toString<int>( stats.totalLines() ) +
			       "\n" + Buffer );
		    }
		  }
		  go_on = found;
	      }
	      if ( stats.dataLines() < 1 ){
		Error( "no useful data in: " + FileName );
	      }
	      else {
		time_stamp( "Finished:  ", stats.totalLines() );
		time_stamp( "Calculating Entropy " );
		if ( Verbosity(FEAT_W) && !Verbosity(SILENT) ){
		  *mylog << "Lines of data     : " 
			 << stats.dataLines() << endl;
		  if ( stats.skippedLines() != 0 )
		    *mylog << "SkippedLines      : "
			   << stats.skippedLines() << endl;
		  LearningInfo( *mylog );
		}
		else
		  calculate_fv_entropy( false );
		prepT.stop();
		Info( "Preparation took " + prepT.toString() );
		if ( warnOnSingleTarget && Targets->EffectiveValues() <=1 ){
		  Warning( "Training file contains only 1 class." );
		}
		result = true;
	      }
	    }
	  }
	}
      }
    }
    return result;
  }
  
  bool TimblExperiment::CVprepare( const string&,
				   WeightType,
				   const string& ){
    Error( "CVprepare called for NON CV experiment" );
    return false;
  }

  ostream& operator<< ( ostream& os, const set<streamsize>& s ){
    os << "{";
    set<streamsize>::const_iterator it = s.begin();
    while ( it != s.end() ){
      os << *it << ",";
      ++it;
    }
    os << "}";
    return os;
  }

  ostream& operator<< ( ostream& os, const fileIndex& fi ){
    fileIndex::const_iterator it = fi.begin();
    while ( it != fi.end() ){
      os << "<";
      os << it->first << "," << it->second;
      os << ">";
      ++it;
    }
    return os;
  }

  bool TimblExperiment::learnFromFileIndex( const fileIndex& fi, 
					    istream& datafile ){
    InstanceBase_base *outInstanceBase = 0;
    fileIndex::const_iterator fit = fi.begin();
    while ( fit != fi.end() ){
      set<streamsize>::const_iterator sit = fit->second.begin();
      while ( sit != fit->second.end() ){
	datafile.clear();
	datafile.seekg( *sit );
	string Buffer;
	nextLine( datafile, Buffer );
	chopLine( Buffer );
	// Progress update.
	//
	if (( stats.dataLines() % Progress() ) == 0) 
	  time_stamp( "Learning:  ", stats.dataLines() );
	chopped_to_instance( TrainWords );
	if ( !outInstanceBase )
	  outInstanceBase =InstanceBase->clone();
	//		  cerr << "add instance " << &CurrInst << endl;
	if ( !outInstanceBase->AddInstance( CurrInst ) ){
	  Warning( "deviating exemplar weight in:\n" + 
		   Buffer + "\nIgnoring the new weight" );
	}
	++sit;
      }
      ++fit;
    }
    if ( outInstanceBase ){
      if ( !InstanceBase->MergeSub( outInstanceBase ) ){
	FatalError( "Merging InstanceBases failed. PANIC" );
	return false;
      }
      delete outInstanceBase;
      outInstanceBase = 0;
    }
    return true;
  }

  bool TimblExperiment::ClassicLearn( const string& FileName, 
				      bool warnOnSingleTarget ){
    bool result = true;
    Common::Timer learnT;
    if ( is_synced ){
      CurrentDataFile = FileName; // assume magic!
    }
    if ( CurrentDataFile == "" )
      if ( FileName == "" ){
	Warning( "unable to build an InstanceBase: No datafile defined yet" );
	result = false;
      }
      else {
	if ( !Prepare( FileName, warnOnSingleTarget ) || ExpInvalid() ){
	  result = false;
	}
      }
    else if ( FileName != "" &&
	      CurrentDataFile != FileName ){
      Error( "Unable to Learn from file '" + FileName + "'\n"
	     "while previously instantiated from file '" + 
	     CurrentDataFile + "'" );
      result = false;
    }
    if ( result ) {
      Common::Timer learnT;
      learnT.start();
      InitInstanceBase();
      if ( ExpInvalid() )
	return false;
      if ( EffectiveFeatures() < 2 ) {
	fileIndex fmIndex;
	//      Common::Timer t;
	//      t.start();
	result = build_file_index( CurrentDataFile, fmIndex );
	//      t.stop();
	//      cerr << "indexing took " << t << endl;
	//      totalT.start();
	if ( result ){
	  //	  cerr << "index = " << fmIndex << endl;
	  stats.clear();
	  if ( !Verbosity(SILENT) ) {
	    Info( "\nPhase 3: Learning from Datafile: " + CurrentDataFile );
	    time_stamp( "Start:     ", 0 );
	  }
	  // Open the file.
	  //
	  ifstream datafile( CurrentDataFile.c_str(), ios::in);
	  //
	  learnFromFileIndex( fmIndex, datafile );
	}
      }
      else {
	fileDoubleIndex fIndex;
	//      Common::Timer t;
	//      t.start();
	result = build_file_multi_index( CurrentDataFile, fIndex );
	//	cerr << "index: " << fIndex << endl;
	//      t.stop();
	//      cerr << "indexing took " << t << endl;
	//      totalT.start();
	if ( result ){
	  stats.clear();
	  if ( !Verbosity(SILENT) ) {
	    Info( "\nPhase 3: Learning from Datafile: " + CurrentDataFile );
	    time_stamp( "Start:     ", 0 );
	  }
	  string Buffer;
	  // Open the file.
	  //
	  ifstream datafile( CurrentDataFile.c_str(), ios::in);
	  //
	  fileDoubleIndex::const_iterator mit = fIndex.begin();
	  while ( mit != fIndex.end() ){
	    learnFromFileIndex( mit->second, datafile );
	    ++mit;
	  }
	}
      }
      time_stamp( "Finished:  ", stats.dataLines() );
      learnT.stop();
      // cerr << "Endresult " << endl;
      // cerr << InstanceBase << endl;
      if ( !Verbosity(SILENT) ){
	IBInfo( *mylog );
	Info( "Learning took " + learnT.toString() );
      }
#ifdef IBSTATS
      cerr << "final mismatches: " << InstanceBase->mismatch << endl;
#endif
    }
    return result;
  }

  bool TimblExperiment::Learn( const std::string& s, bool warnOnSingleTarget ){
    if ( ExpInvalid() ||
	 !ConfirmOptions() ){
      return false;
    }
    return ClassicLearn( s, warnOnSingleTarget );
  }
  
  IB1_Experiment::IB1_Experiment( const size_t N,
				  const string& s,
				  const bool init ):
    TimblExperiment( IB1_a, s ){
    if ( init ) InitClass( N );
    TreeOrder = GRoverFeature;
  }
  
  /*
    Increment the Instancebase with one instance (IB1 Class only)
  */
  bool IB1_Experiment::Increment( const string& InstanceString ){
    bool result = true;
    if ( ExpInvalid() ){
      result = false;
    }
    else if ( IBStatus() == Invalid ){
      Warning( "unable to Increment, No InstanceBase available" );
      result = false;
    }
    else {
      if ( !Chop( InstanceString ) ){
	Error( "Couldn't convert to Instance: " + InstanceString );
	result = false;    // No more input
      }
      else {	
	chopped_to_instance( TrainLearnWords );
	MBL_init = false;
	bool happy;
	happy = InstanceBase->AddInstance( CurrInst );
	if ( !happy )
	  Warning( "deviating exemplar weight in:\n" + 
		   InstanceString + "\nIgnoring the new weight" );
      }
    }
    return result;
  }
  
  /*
    Decrement the Instancebase with one instance (IB1 Class only)
  */
  bool IB1_Experiment::Decrement( const string& InstanceString ){
    bool result = true;
    if ( ExpInvalid() ){
      result = false;
    }
    else if ( IBStatus() == Invalid ){
      Warning( "unable to Decrement, No InstanceBase available" );
      result = false;
    }
    else {
      if ( !Chop( InstanceString ) ){
	Error( "Couldn't convert to Instance: " + InstanceString );
	result = false;    // No more input
      }
      else {
	chopped_to_instance( TestWords );
	HideInstance( CurrInst );
      }
    }
    return result;
  }
  
  /*
    Expand  an Instance Base (IB1 only)
  */
  bool IB1_Experiment::Expand( const string& FileName ){
    bool result = true;
    if ( ExpInvalid() ){
      result = false;
    }
    else if ( IBStatus() == Invalid ){
      Warning( "unable to expand the InstanceBase: Not there" );
      result = false;
    }
    else if ( FileName == "" ){
      Warning( "unable to expand the InstanceBase: No inputfile specified" );
      result = false;
    }
    else {
      if ( InputFormat() == UnknownInputFormat ){
	// we may expand from 'nothing'
	if ( !Prepare( FileName, false, true ) ){
	  Error( "Unable to expand from file :'" + FileName + "'\n" );
	  return false;
	}
      }
      string Buffer;
      stats.clear();
      // Open the file.
      //
      ifstream datafile( FileName.c_str(), ios::in);
      if ( InputFormat() == ARFF )
	skipARFFHeader( datafile );
      if ( !nextLine( datafile, Buffer ) ){
	Error( "no useful data in: " + FileName );
	result = false;    // No more input
      }
      else if ( !chopLine( Buffer ) ){
	Error( "no useful data in: " + FileName );
	result = false;    // No more input
      }
      else {
	MBL_init = false;
	if ( !Verbosity(SILENT) ) {
	  Info( "Phase 2: Expanding from Datafile: " + FileName );
	  time_stamp( "Start:     ", 0 );
	}
	bool found;
	do {
	  // The next Instance to store. 
	  chopped_to_instance( TrainLearnWords );
	  bool happy;
	  happy = InstanceBase->AddInstance( CurrInst );
	  if ( !happy ){
	    Warning( "deviating exemplar weight in line #" + 
		     toString<int>(stats.totalLines() ) + ":\n" +
		     Buffer + "\nIgnoring the new weight" );	
	  }
	  // Progress update.
	  //
	  if ((stats.dataLines() % Progress() ) == 0) 
	    time_stamp(  "Learning:  ", stats.dataLines() );
	  found = false;
	  while ( !found && nextLine( datafile, Buffer ) ){
	    found = chopLine( Buffer );
	    if ( !found ){
	      Warning( "datafile, skipped line #" + 
		       toString<int>( stats.totalLines() ) +
		       "\n" + Buffer );
	    }
	  }
	} while( found );
	time_stamp( "Finished:  ", stats.dataLines() );
	if ( !Verbosity(SILENT) )
	  IBInfo( *mylog );
      }
    }
    return result;
  }

  /*
    Remove Instances from an Instance Base (IB1 only )
  */
  bool IB1_Experiment::Remove( const string& FileName ){
    bool result = true;
    if ( ExpInvalid() ){
      result = false;
    }
    else if ( IBStatus() == Invalid ){
      Warning( "unable to remove from InstanceBase: Not there" );
      result = false;
    }
    else if ( FileName == "" ){
      Warning( "unable to remove from InstanceBase: No input specified" );
      result = false;
    }
    else {
      string Buffer;
      stats.clear();
      // Open the file.
      //
      ifstream datafile( FileName.c_str(), ios::in);
      if ( InputFormat() == ARFF )
	skipARFFHeader( datafile );
      if ( !nextLine( datafile, Buffer ) ){
	Error( "no useful data in: " + FileName );
	result = false;    // No more input
      }
      else if ( !chopLine( Buffer ) ){
	Error( "no useful data in: " + FileName );
	result = false;    // No more input
      }
      else {
	if ( !Verbosity(SILENT) ) {
	  Info( "Phase 2: Removing using Datafile: " + FileName );
	  time_stamp( "Start:     ", 0 );
	}
	bool found;
	do {
	  // The next Instance to remove. 
	  chopped_to_instance( TestWords );
	  HideInstance( CurrInst );
	  // Progress update.
	  //
	  if ((stats.dataLines() % Progress() ) == 0) 
	    time_stamp( "Removing:  ", stats.dataLines() );
	  found = false;
	  while ( !found && 
		  nextLine( datafile, Buffer ) ){
	    found = chopLine( Buffer );
	    if ( !found ){
	      Warning( "datafile, skipped line #" + 
		       toString<int>( stats.totalLines() ) +
		       "\n" + Buffer );
	    }
	  }
	} while( found );
	time_stamp( "Finished:  ", stats.dataLines() );
	if ( !Verbosity(SILENT) )
	  IBInfo( *mylog );
      }
    }
    return result;
  }
  
  void TimblExperiment::showInputFormat( ostream& os ) const {
    switch ( InputFormat() ){
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
  
  void TimblExperiment::show_progress( ostream& os, 
				       time_t start, unsigned int line ){
    char time_string[26];
    struct tm *curtime;
    time_t Time;
    time_t SecsUsed;
    time_t EstimatedTime;
    double Estimated;
    int local_progress = Progress();
    if ( ( (line % local_progress ) == 0) || ( line <= 10 ) ||
	 ( line == 100 || line == 1000 || line == 10000 ) ){
      time(&Time);
      if ( line == 1000 ){
	// check if we are slow, if so, change progress value
	if ( Time - start > 120 ) // more then two minutes
	  // very slow !
	  Progress( 1000 );
      }
      else if ( line == 10000 ){
	if ( Time - start > 600 ) // more then ten minutes
	  // quit slow !
	  Progress( 10000 );
      }
      curtime = localtime(&Time);
      if ( exp_name != "" )
	os  << "-" << exp_name << "-";
      os << "Tested: ";
      os.width(6);
      os.setf(ios::right, ios::adjustfield);
      strcpy( time_string, asctime(curtime));
      time_string[24] = '\0';
      os << line << " @ " << time_string;
      
      // Estime time until Estimate.
      //
      if ( Estimate() > 0 &&  (unsigned int)Estimate() < line ) {
	SecsUsed = Time - start;
	if ( SecsUsed > 0 ) {
	  Estimated = (SecsUsed / (float)line) * 
	    (float)Estimate();
	  EstimatedTime = (long)Estimated + start;
	  os << ", ";
	  strcpy(time_string, ctime(&EstimatedTime));
	  time_string[24] = '\0';
	  os << Estimate() << ": " << time_string;
	} 
      }
      os << endl;
    }
  }
  
  bool IB2_Experiment::show_learn_progress( ostream& os,
					    time_t start,
					    size_t added ){
    char time_string[26];
    struct tm *curtime;
    time_t Time;
    time_t SecsUsed;
    time_t EstimatedTime;
    double Estimated;
    int local_progress = Progress();
    unsigned int lines = stats.dataLines();
    unsigned int line = lines - IB2_offset() ;
    if ( ( (line % local_progress ) == 0) || ( line <= 10 ) ||
	 ( line == 100 || line == 1000 || line == 10000 ) ){
      time(&Time);
      if ( line == 100 ){
	// check if we are slow, if so, change progress value
	if ( Time - start > 120 && local_progress > 100 )
	  // very slow !
	  Progress( 100 );
      }
      else if ( line == 1000 ){
	// check if we are slow, if so, change progress value
	if ( Time - start > 120 && local_progress > 1000 )
	  // very slow !
	  Progress( 1000 );
      }
      else if ( line == 10000 ){
	if ( Time - start > 120 && local_progress > 10000)
	  // quit slow !
	  Progress( 10000 );
      }
      curtime = localtime(&Time);
      if ( exp_name != "" )
	os  << "-" << exp_name << "-";
      os << "Learning:  ";
      os.width(6);
      os.setf(ios::right, ios::adjustfield);
      strcpy( time_string, asctime(curtime));
      time_string[24] = '\0';
      os << lines << " @ " << time_string;
      os << "\t added:" << added;
      // Estime time until Estimate.
      //
      if ( Estimate() > 0 && (unsigned int)Estimate() < lines ) {
	SecsUsed = Time - start;
	if ( SecsUsed > 0 ) {
	  Estimated = (SecsUsed / (float)line) * 
	    ( (float)Estimate() - IB2_offset() );
	  EstimatedTime = (long)Estimated + start;
	  os << "\t, ";
	  strcpy(time_string, ctime(&EstimatedTime));
	  time_string[24] = '\0';
	  os << Estimate() << ": " << time_string;
	} 
      }
      os << endl;
      return true;
    }
    else
      return false;
  }
  
  void TimblExperiment::show_speed_summary( ostream& os,
					    const timeval& Start ) const {
    timeval Time;
    gettimeofday( &Time, 0 );
    long int uSecsUsed = (Time.tv_sec - Start.tv_sec) * 1000000 +
      (Time.tv_usec - Start.tv_usec);
    double secsUsed = (double)uSecsUsed / 1000000 + Epsilon;
    int oldPrec = os.precision(4);
    os << setprecision(4);
    os.setf( ios::fixed, ios::floatfield );
    os << "Seconds taken: " << secsUsed << " (";
    os << setprecision(2);
    os << stats.dataLines() / secsUsed << " p/s)" << endl;
    os << setprecision(oldPrec);
  }
  
  bool TimblExperiment::showStatistics( ostream& os ) const {
    os << endl;
    if ( confusionInfo )
      confusionInfo->FScore( os, Targets, Verbosity(CLASS_STATS) );
    os << "overall accuracy:        " 
       << stats.testedCorrect()/(double) stats.dataLines() 
       << "  (" << stats.testedCorrect() << "/" << stats.dataLines()  << ")" ;
    if ( stats.exactMatches() != 0 )
      os << ", of which " << stats.exactMatches() << " exact matches " ;
    os << endl;
    int totalTies =  stats.tiedCorrect() + stats.tiedFailure();
    if ( totalTies > 0 ){
      if ( totalTies == 1 )
	os << "There was 1 tie";
      else
	os << "There were " << totalTies << " ties";
      double tie_perc = 100 * ( stats.tiedCorrect() / (double)totalTies);
      int oldPrec = os.precision(2);
      os << " of which " << stats.tiedCorrect()
	 << " (" << setprecision(2)
	 << tie_perc << setprecision(6) << "%)";
      if ( totalTies == 1 )
	os << " was correctly resolved" << endl;
      else
	os << " were correctly resolved" << endl;
      os.precision(oldPrec);
    }
    if ( confusionInfo && Verbosity(CONF_MATRIX) ){
      os << endl;
      confusionInfo->Print( os, Targets );
    }
    return true;
  }

  bool TimblExperiment::createPercFile( const string& fileName ) const {
    if ( fileName != "" ) {
      ofstream outfile( fileName.c_str(), ios::out | ios::trunc);
      if (!outfile) {
	Warning( "can't open: " + fileName );
	return false;
      }
      else {
	outfile 
	  << (stats.testedCorrect() / (float)stats.dataLines()) * 100.0
	  << endl
	  << "tested " << stats.dataLines() << " lines " << endl
	  << "correct " << stats.testedCorrect() << " lines " << endl;
	outfile.close();
      }
    }
    return true;
  }
  
  bool TimblExperiment::showBestNeighbors( ostream& outfile ) const {
    if ( Verbosity( NEAR_N | ALL_K) ){
      outfile << bestArray;
      return true;
    }
    else
      return false;
  }
    
  xmlNode *TimblExperiment::bestNeighborsToXML() const {
    if ( Verbosity( NEAR_N | ALL_K) ){
      return bestArray.toXML();
    }
    else
      return 0;
  }
    
  void TimblExperiment::show_results( ostream& outfile, 
				      const string& dString,
				      const TargetValue *Best,
				      const double Distance ) {
    outfile << get_org_input() << CodeToStr(Best->Name());
    if ( Verbosity(DISTRIB) ){
      outfile << " " << dString;
    }
    if ( Verbosity(DISTANCE) ) {
      int OldPrec = outfile.precision(DBL_DIG-1);
      outfile.setf(ios::showpoint);
      outfile.width(8);
      if ( GlobalMetric->isSimilarityMetric() )
	outfile << " " << maxSimilarity-Distance;
      else
	outfile << " " << Distance;
      outfile.precision(OldPrec);
    }
    if ( Verbosity(MATCH_DEPTH) ){
      outfile << " " << matchDepth() << ":" << (matchedAtLeaf()?"L":"N");
    }
    outfile << endl;
    showBestNeighbors( outfile );
  }
  
  bool IB2_Experiment::Prepare( const string& FileName, 
				bool warnOnSingleTarget,
				bool expand ){
    if ( !ConfirmOptions() ||
	 ( IB2_offset() == 0 && InstanceBase == 0 ) ){
      Error( "IB2 learning failed, invalid bootstrap option?" );
      return false;
    }
    else
      return TimblExperiment::Prepare( FileName, false, expand );
  }
  
  bool IB2_Experiment::Learn( const string& FileName, bool warnOnSingleTarget ){
    if ( IB2_offset() == 0 ){
      Error( "IB2 learning failed, invalid bootstrap option?" );
      return false;
    }
    else {
      bool result = true;
      Common::Timer learnT;
      if ( ExpInvalid() ||
	   !ConfirmOptions() ){
	result = false;
      }
      else {
	if ( is_synced ){
	  CurrentDataFile = FileName; // assume magic!
	}
	if ( CurrentDataFile == "" )
	  if ( FileName == "" ){
	    Warning( "unable to build an InstanceBase: No datafile defined yet" );
	    result = false;
	  }
	  else {
	    if ( !Prepare( FileName, false ) || ExpInvalid() ){
	      result = false;
	    }
	  }
	else if ( FileName != "" &&
		  CurrentDataFile != FileName ){
	  Error( "Unable to Learn from file '" + FileName + "'\n"
		 "while previously instantiated from file '" + 
		 CurrentDataFile + "'" );
	  result = false;
	}
      }
      if ( result ) {
	string Buffer;
	stats.clear();
	// Open the file.
	//
	ifstream datafile( CurrentDataFile.c_str(), ios::in);
	if ( InputFormat() == ARFF )
	  skipARFFHeader( datafile );
	if ( !nextLine( datafile, Buffer ) ){
	  Error( "cannot start learning from in: " + CurrentDataFile );
	  result = false;    // No more input
	}
	else if ( !chopLine( Buffer ) ){
	  Error( "no useful data in: " + CurrentDataFile );
	  result = false;    // No more input
	}
	else {
	  learnT.start();
	  InitInstanceBase( );
	  if ( ExpInvalid() )
	    return false;
	  MBL_init = false;
	  if ( !Verbosity(SILENT) ) {
	    Info( "Phase 2: Learning from Datafile: " + CurrentDataFile );
	    time_stamp( "Start:     ", 0 );
	  }
	  bool found;
	  bool go_on = ( stats.dataLines() <= IB2_offset() );
	  while( go_on ){ 
	    // The next Instance to store. 
	    chopped_to_instance( TrainWords );
	    bool happy;
	    happy = InstanceBase->AddInstance( CurrInst );
	    if ( !happy ){
	      Warning( "deviating exemplar weight in line #" +
		       toString<int>(stats.totalLines()) + ":\n" +
		       Buffer + "\nIgnoring the new weight" );
	    }
	    // Progress update.
	    //
	    if ((stats.dataLines() % Progress() ) == 0) 
	      time_stamp( "Learning:  ", stats.dataLines() );
	    if ( stats.dataLines() >= IB2_offset() )
	      go_on = false;
	    else {
	      found = false;
	      while ( !found && 
		      nextLine( datafile, Buffer ) ){
		found = chopLine( Buffer );
		if ( !found ){
		  Warning( "datafile, skipped line #" + 
			   toString<int>( stats.totalLines() ) +
			   "\n" + Buffer );
		}
	      }
	      go_on = found;
	    }
	  }
	  time_stamp( "Finished:  ", stats.dataLines() );
	  learnT.stop();
	  if ( !Verbosity(SILENT) ){
	    IBInfo( *mylog );
	    Info( "Learning took " + learnT.toString() );
	  }
#ifdef IBSTATS
	  cerr << "IB2 mismatches: " << InstanceBase->mismatch << endl;
#endif
	}
	if ( result )
	  result = Expand_N( FileName );
      }
      return result;
    }
  }
  
  bool IB2_Experiment::Expand( const string& FileName ){
    bool result = false;
    if ( CurrentDataFile == "" && InstanceBase == 0 ){
      Warning( "IB2, cannot Append data: No datafile bootstrapped yet" );
    }
    else {
      IB2_offset( 0 );
      if ( InputFormat() == UnknownInputFormat ){
	// we may expand from 'nothing'
	if ( !Prepare( FileName, false, true ) ){
	  Error( "Unable to expand from file :'" + FileName + "'\n" );
	  return false;
	}
      }
      result = Expand_N( FileName );
    }
    return result;
  }
  
  bool IB2_Experiment::Remove( const string& ){
    Warning( "IB2, remove impossible, (ignored) " );
    return false;
  }
  
  bool IB2_Experiment::Expand_N( const string& FileName ){
    bool result = true;
    size_t Added = 0;
    size_t TotalAdded = 0;
    if ( ExpInvalid() ){
      result = false;
    }
    else if ( CurrentDataFile == "" && InstanceBase == 0 ){
      Warning( "IB2, cannot Append data: No datafile bootstrapped yet" );
      result = false;
    }
    else if ( IBStatus() == Invalid ){
      Warning( "unable to expand the InstanceBase: Not there" );
      result = false;
    }
    else {
      string file_name;
      if ( FileName == "" )
	file_name = CurrentDataFile;
      else
	file_name = FileName;
      string Buffer;
      stats.clear();
      // Open the file.
      //
      ifstream datafile( file_name.c_str(), ios::in);
      if ( InputFormat() == ARFF )
	skipARFFHeader( datafile );
      if ( !nextLine( datafile, Buffer ) ){
	Error( "no useful data in: " + file_name );
	result = false;    // No more input
      }
      else if ( !chopLine( Buffer ) ){
	Error( "no useful data in: " + file_name );
	result = false;    // No more input
      }
      else {
	while ( stats.dataLines() <= IB2_offset() ){
	  if ( !nextLine( datafile, Buffer ) ){
	    Error( "not enough lines to skip in " + FileName );
	    result = false;
	    break;
	  }
	  else if ( !chopLine( Buffer ) ){
	    Warning( "datafile, skipped line #" + 
		     toString<int>( stats.totalLines() ) +
		     "\n" + Buffer );
	  }
	}
	if ( result ){
	  time_t lStartTime;
	  time(&lStartTime);
	  if ( !Verbosity(SILENT) ) {
	    Info( "Phase 2: Appending from Datafile: " + FileName + 
		  " (starting at line " + toString<int>( stats.dataLines() ) + ")" );
	    time_stamp( "Start:     ", stats.dataLines() );
	  }
	  bool found;
	  initExperiment();
	  do {
	    // The next Instance to store. 
	    chopped_to_instance( TestWords );
	    double final_distance;
	    bool dummy = false;
	    StatisticsClass stats_keep = stats;
	    const TargetValue *ResultTarget = LocalClassify( CurrInst, 
							     final_distance,
							     dummy );
	    stats = stats_keep;
	    if ( ResultTarget != CurrInst.TV ) {
	      chopped_to_instance( TrainLearnWords );
	      bool happy;
	      happy = InstanceBase->AddInstance( CurrInst );
	      if ( !happy ){
		Warning( "deviating exemplar weight in line #" + 
			 toString<int>(stats.totalLines() ) + ":\n" +
			 Buffer + "\nIgnoring the new weight" );
	      }
	      ++Added;
	      ++TotalAdded;
	      MBL_init = true; // avoid recalculations in LocalClassify
	      
	    }
	    // Progress update.
	    //
	    if ( show_learn_progress( *mylog, lStartTime, Added ) ){
	      Added = 0;
	    }
	    found = false;
	    while ( !found &&
		    nextLine( datafile, Buffer ) ){
	      found = chopLine( Buffer );
	      if ( !found ){
		Warning( "datafile, skipped line #" + 
			 toString<int>( stats.totalLines() ) +
			 "\n" + Buffer );
	      }
	    }
	  } while( found );
	  if ( result ){
	    time_stamp( "Finished:  ", stats.dataLines() );
	    *mylog << "in total added " << TotalAdded << " new entries" << endl;
	    if ( !Verbosity(SILENT) ){
	      IBInfo( *mylog );
	      LearningInfo( *mylog );
	    }
	    MBL_init = false; // force recalculations when testing
	  }
	}
      }
    }
    return result;
  }
  
  bool TimblExperiment::initTestFiles( const string& InFileName,
				       const string& OutFileName ){
    if ( !ExpInvalid() &&
	 ConfirmOptions() ){
      testStream.close();
      testStream.clear(); // just to be shure. old G++ libraries are in error here
      testStream.open( InFileName.c_str(), ios::in);
      if ( !testStream ) {
	Error( "can't open: " + InFileName );
      }
      else {
	outStream.close();
	outStream.clear(); // just to be shure. old G++ libraries are in error here
	// first we check if the outFile is writable.
	// We don't write it though, because we don't want to have
	// it mangled when checkTestFile fails
	outStream.open( OutFileName.c_str(), ios::app );
	if ( !outStream ) {
	  Error( "can't open: " + OutFileName );
	}
	else {
	  testStreamName = InFileName;
	  outStreamName = OutFileName;
	  if ( checkTestFile() ){
	    outStream.close();
	    outStream.clear(); // just to be shure. old G++ libraries are in error here
	    outStream.open( OutFileName.c_str(), ios::out | ios::trunc );
	    return true;
	  }
	}
      }
    }
    return false;
  }

  bool TimblExperiment::checkTestFile(){
    if ( IBStatus() == Invalid )
      Warning( "you tried to apply the " + toString( algorithm ) +
	       " algorithm, but no Instance Base is available yet" );
    else {
      runningPhase = TestWords;
      size_t numF =0;
      if ( (numF = examineData( testStreamName )) != NumOfFeatures() ){
	if ( numF == 0 ){
	  Error( "unable to use the data from '" + testStreamName +
		 "', wrong Format?" );
	}
	else
	  Error( "mismatch between number of features in Testfile " +
		 testStreamName + " and the Instancebase (" +
		 toString<size_t>(numF) + " vs. " + 
		 toString<size_t>(NumOfFeatures()) + ")" ); 
	return false;
      }
      if ( !Verbosity(SILENT) ){
	*mylog << "Examine datafile '" << testStreamName 
	       << "' gave the following results:"
	       << endl
	       << "Number of Features: " << numF << endl;
	showInputFormat( *mylog );
      }
    }	
    return true;
  }
  
  bool IB1_Experiment::checkTestFile(){
    if ( !TimblExperiment::checkTestFile() )
      return false;
    else if ( IBStatus() == Pruned ){
      Warning( "you tried to apply the " + toString( algorithm) +
	       " algorithm on a pruned Instance Base" );
      return false;
    }
    return true;
  }
  
  bool IB2_Experiment::checkTestFile(){
    if ( !IB1_Experiment::checkTestFile() )
      return false;
    else if ( IB2_offset() == 0 ){
      Error( "missing bootstrap information for IB2 algorithm." );
      return false;
    }
    return true;
  }
  
  bool TimblExperiment::checkLine( const string& line ){
    size_t i;
    bool result = false;
    if ( !ExpInvalid() &&
	 ConfirmOptions() ) {
      runningPhase = TestWords;
      InputFormatType IF = InputFormat();
      if ( IF == UnknownInputFormat )
	IF = getInputFormat( line );
      if ( (i = countFeatures( line, IF )) != NumOfFeatures() ){
	if ( i > 0 )
	  Warning( "mismatch between number of features in testline " +
		   line + " and the Instancebase (" + toString<size_t>(i)
		   + " vs. " + toString<size_t>(NumOfFeatures()) + ")" ); 
      }
      else { 
	if ( Initialized ){
	  result = true;
	}
	else if ( IBStatus() == Invalid )
	  Warning( "no Instance Base is available yet" );
	else if ( !setInputFormat( IF ) ){
	  Error( "Couldn't set input format to " + toString( IF ) );
	}
	else {
	  if ( Verbosity(NEAR_N) ){
	    Do_Exact( false );
	  }
	  initExperiment();
	  Initialized = true;
	  result = true;
	}
      }
    }
    return result;
  }
    
    
  bool IB1_Experiment::checkLine( const string& line ){
    if ( !TimblExperiment::checkLine( line ) )
      return false;
    else if ( IBStatus() == Pruned ){
      Warning( "you tried to apply the IB1 algorithm on a pruned"
	       " Instance Base" );
      return false;
    }
    else if ( TRIBL_offset() != 0 ){
      Error( "IB1 algorithm impossible while treshold > 0\n"
	     "Please use TRIBL" );
      return false;
    }
    return true;
  }
    
  bool TimblExperiment::Classify( const string& Line, 
				  string& Result,
				  string& Dist,
				  double& Distance ){
    Result.clear();
    Dist.clear();
    const TargetValue *targ = classifyString( Line, Distance );
    if ( targ ){
      Result = targ->Name();
      normalizeResult();
      Dist = bestResult.getResult();
      return true;
    }
    return false;
  }
  
  bool TimblExperiment::Classify( const string& Line, 
				  string& Result,
				  double& Distance ){
    Result.clear();
    const TargetValue *targ = classifyString( Line, Distance );
    if ( targ ){
      Result = targ->Name();
      return true;
    }
    return false;
  }
  
  bool TimblExperiment::Classify( const string& Line, 
				  string& Result ) {
    Result.clear();
    double dummy;
    const TargetValue *targ = classifyString( Line, dummy );
    if ( targ ){
      Result = targ->Name();
      return true;
    }
    return false;
  }

  void TimblExperiment::testInstance( const Instance& Inst,
				      InstanceBase_base *base,
				      size_t offset ) {
    initExperiment();
    bestArray.init( num_of_neighbors, MaxBests,
		    Verbosity(NEAR_N), Verbosity(DISTANCE),
		    Verbosity(DISTRIB) ); 
    TestInstance( Inst, base, offset );
  }

  const TargetValue *TimblExperiment::LocalClassify( const Instance& Inst,
						     double& Distance,
						     bool& exact ){
    bool recurse = true;
    bool Tie = false; 
    exact = false;
    if ( !bestResult.reset( beamSize, normalisation, norm_factor, Targets ) ){
      Warning( "no normalisation possible because a BeamSize is specified\n"
	       "output is NOT normalized!" );
    }
    const ValueDistribution *ExResultDist = ExactMatch( Inst );
    WValueDistribution *ResultDist = 0;
    nSet.clear();
    const TargetValue *Res;
    if ( ExResultDist ){
      Distance = 0.0;
      recurse = !Do_Exact();
      // no retesting when exact match and the user ASKED for them..
      Res = ExResultDist->BestTarget( Tie, (RandomSeed() >= 0) );
      //
      // add the exact match to bestArray. It should be taken into account
      // for Tie resolution. this fixes bug 44
      //
      bestArray.init( num_of_neighbors, MaxBests,
		      Verbosity(NEAR_N), Verbosity(DISTANCE),
		      Verbosity(DISTRIB) ); 
      bestArray.addResult( Distance, ExResultDist, get_org_input() );
      bestArray.initNeighborSet( nSet );
    }
    else {
      testInstance( Inst, InstanceBase );
      bestArray.initNeighborSet( nSet );
      ResultDist = getBestDistribution( );
      Res = ResultDist->BestTarget( Tie, (RandomSeed() >= 0) );
      Distance = getBestDistance();
    }
    if ( Tie && recurse ){
      bool Tie2 = true;
      ++num_of_neighbors;
      testInstance( Inst, InstanceBase );
      bestArray.addToNeighborSet( nSet, num_of_neighbors );
      WValueDistribution *ResultDist2 = getBestDistribution();
      const TargetValue *Res2 = ResultDist2->BestTarget( Tie2, (RandomSeed() >= 0) );
      --num_of_neighbors;
      if ( !Tie2 ){
	Res = Res2;
	delete ResultDist;
	ResultDist = ResultDist2; 
      }
      else
	delete ResultDist2;
    }

    exact = fabs(Distance) < Epsilon ;
    if ( ResultDist ){
      bestResult.addDisposable( ResultDist );
    }
    else {
      bestResult.addConstant( ExResultDist );
      exact = exact || Do_Exact();
    }
    if ( exact )
      stats.addExact();
    if ( confusionInfo ){
      confusionInfo->Increment( Inst.TV, Res );
    }
    bool correct = Inst.TV && ( Res == Inst.TV );
    if ( correct ){
      stats.addCorrect();
      if ( Tie )
	stats.addTieCorrect();
    }
    else if ( Tie )
      stats.addTieFailure();
    return Res;
  }
  
  const TargetValue *TimblExperiment::classifyString( const string& Line, 
						      double& Distance ){
    Distance = -1.0;
    const TargetValue *BestT = NULL;
    if ( checkLine( Line ) &&
	 chopLine( Line ) ){
      chopped_to_instance( TestWords );
      bool exact = false;
      BestT = LocalClassify( CurrInst, Distance, exact );
    }
    return BestT;
  }

  const neighborSet *TimblExperiment::NB_Classify( const string& Line ){
    initExperiment();
    if ( checkLine( Line ) &&
	 chopLine( Line ) ){
      chopped_to_instance( TestWords );
      return LocalClassify( CurrInst );
    }
    return 0;
  }
  
  const neighborSet *TimblExperiment::LocalClassify( const Instance& Inst ){
    testInstance( Inst, InstanceBase );
    bestArray.initNeighborSet( nSet );
    nSet.setShowDistance( Verbosity(DISTANCE) );
    nSet.setShowDistribution( Verbosity(DISTRIB) );
    return &nSet;
  }
  
  double TimblExperiment::sum_remaining_weights( size_t level ) const {
    double result = 0.0;
    for ( size_t i = level; i < EffectiveFeatures(); ++i ){
      result += PermFeatures[i]->Weight();
    }
    return result;
  }
    
  void TimblExperiment::show_metric_info( ostream& os ) const {
    os << "Global metric : " << toString( globalMetricOption, true);
    if ( GlobalMetric->isStorable() ){
      os << ", Prestored matrix";
    }
    if ( Do_Exact() )
      os << ", prefering exact matches";
    os << endl;
    os << "Deviant Feature Metrics:";
    int cnt = 0;
    size_t *InvPerm = new size_t[NumOfFeatures()];
    for ( size_t i = 0; i < NumOfFeatures(); ++i )
      InvPerm[permutation[i]] = i;
    for ( size_t i = 0; i < NumOfFeatures(); ++i ){
      if ( !Features[i]->Ignore() &&
	   InvPerm[i]+1 > TRIBL_offset() ){
	MetricType mt =  Features[i]->getMetricType();
	if ( mt != globalMetricOption ){
	  ++cnt;
	  os << endl << "   Feature[" << i+1 << "] : " << toString( mt, true );
	  if ( Features[i]->isStorableMetric() ){
	    bool readM = false;
	    if ( Features[i]->matrixPresent( readM ) )
	      if ( readM )
		os << " (User Defined)";
	      else
		os << " (Prestored)";
	    else
	      os << " (Not Prestored)";
	  }
	}
      }
    }
    delete [] InvPerm;
    if ( cnt )
      os << endl;
    else
      os << "(none)" << endl;
    MatrixInfo( os );
    show_ignore_info( os );
  }

  void TimblExperiment::show_weight_info( ostream& os ) const {
    os << "Weighting     : " << toString(CurrentWeighting(), true);
    if ( CurrentWeighting() == UserDefined_w ){
      if ( WFileName != "" )
	os << "  (" << WFileName << ")";
      else
	os << " (no weights loaded, using No Weighting)" ;
    }
    os << endl;
    if ( Verbosity( FEAT_W ) && CurrentWeighting() != No_w )
      ShowWeights( os );
  }

  void TimblExperiment::show_ignore_info( ostream& os ) const{
    bool first = true;
    for ( size_t i=0; i< NumOfFeatures(); ++i ){
      if ( Features[i]->Ignore() ){
	if ( first ){
	  first = false;
	  os << "Ignored features : { ";
	}
	else
	  os << ", ";
	os << i+1;
      }
    }
    if ( !first )
      os << " } " << endl;
  }

  void TimblExperiment::showTestingInfo( ostream& os ) {
    if ( !Verbosity(SILENT) ){
      if ( Verbosity(OPTIONS ) )
	ShowSettings( os );
      os << endl << "Starting to test, Testfile: " << testStreamName << endl
	 << "Writing output in:          " << outStreamName << endl
	 << "Algorithm     : " << toString( Algorithm() ) << endl;
      show_metric_info( os );
      show_weight_info( os );
      os << decay << endl;
    }
  }

  class threadData {
  public:
    threadData():exp(0), lineNo(0), resultTarget(0), 
		 exact(false), distance(-1){};
    bool exec();
    void show( ostream& ) const;
    TimblExperiment *exp;
    string Buffer;
    unsigned int lineNo;
    const TargetValue *resultTarget;
    bool exact;
    string distrib;
    double distance;
  };

  bool threadData::exec(){
    resultTarget = 0;
// #pragma omp critical
//     cerr << "exec " << lineNo << " '" << Buffer << "'" << endl;
    if ( Buffer.empty() ){
      return false;
    }
    if ( !exp->chopLine( Buffer ) ){
      exp->Warning( "testfile, skipped line #" + 
		    toString<int>( lineNo ) +
		    "\n" + Buffer );
      return false;
    }
    else {
      exp->chopped_to_instance( TimblExperiment::TestWords );
      exact = false;
      resultTarget = exp->LocalClassify( exp->CurrInst,
					 distance,
					 exact );
      exp->normalizeResult();
      distrib = exp->bestResult.getResult();
      return true;
    }
  }

  void threadData::show( ostream& os ) const {
    if ( resultTarget != 0 ){
      exp->show_results( os, distrib, resultTarget, distance );
      if ( exact ){ // remember that a perfect match may be incorrect!
	if ( exp->Verbosity(EXACT) ) {
	  *exp->mylog << "Exacte match:\n" << exp->get_org_input() << endl;
	}
      }
    }
  }

  class threadBlock {
  public:
    threadBlock( TimblExperiment *, int = 1 );
    bool readLines( istream& );
    void finalize();
    vector<threadData> exps;
  private:
    size_t size;
  };

  threadBlock::threadBlock( TimblExperiment *parent, int num ){
    if ( num <= 0 )
      throw range_error( "threadBlock size cannot be <=0" );
    size = num;
    exps.resize( size );
    exps[0].exp = parent;
    for ( int i = 1; i < size; ++i ){
      exps[i].exp = parent->clone();
      *exps[i].exp = *parent;
      exps[i].exp->initExperiment();
    };
  }

  bool threadBlock::readLines( istream& is  ){
    bool result = true;
    bool goon = true;
    for ( int i=0; i < size; ++i ){
      exps[i].Buffer = "";
      int cnt;
      goon = exps[0].exp->nextLine( is, exps[i].Buffer, cnt );
      exps[i].lineNo += cnt;
      if ( !goon && i == 0 )
	result = false;
    }
    return result;
  }

  void threadBlock::finalize(){
    for ( int i=1; i < size; ++i ){
      exps[0].exp->stats.merge( exps[i].exp->stats );
      if ( exps[0].exp->confusionInfo ){
	exps[0].exp->confusionInfo->merge( exps[i].exp->confusionInfo );
      }
      delete exps[i].exp;
    }
  }

#ifdef HAVE_OPENMP
  bool TimblExperiment::Test( const string& FileName,
			      const string& OutFile ){
    bool result = false;
    if ( initTestFiles( FileName, OutFile ) ){
      initExperiment();
      stats.clear();
      showTestingInfo( *mylog );
      if ( numOfThreads > 1 ){
	omp_set_num_threads( numOfThreads );
      }
      threadBlock experiments( this, numOfThreads );
      // Start time.
      //
      time_t lStartTime;
      time(&lStartTime);
      timeval startTime;
      gettimeofday( &startTime, 0 );
      if ( InputFormat() == ARFF )
	skipARFFHeader( testStream );
      unsigned int dataCount = stats.dataLines();
      while ( experiments.readLines( testStream ) ){
	if ( numOfThreads > 1 ){
#pragma omp parallel for shared( experiments, dataCount )
	  for ( int i=0; i < numOfThreads; ++i ){
	    if ( experiments.exps[i].exec() &&
		 !Verbosity(SILENT) )
	      // Display progress counter.
#pragma omp critical
	      show_progress( *mylog, lStartTime, ++dataCount );
	  }
	  
	  for ( int i=0; i < numOfThreads; ++i ){
	    // Write it to the output file for later analysis.
	    experiments.exps[i].show( outStream );
	  }
	}
	else {
	  if ( experiments.exps[0].exec() &&
	       !Verbosity(SILENT) ){
	    // Display progress counter.
	    show_progress( *mylog, lStartTime, ++dataCount );
	  }
	  // Write it to the output file for later analysis.
	  experiments.exps[0].show( outStream );
	}
      }
      experiments.finalize();
      if ( !Verbosity(SILENT) ){
	time_stamp( "Ready:  ", stats.dataLines() );
	show_speed_summary( *mylog, startTime );
	showStatistics( *mylog );
      }
      result = true;
    }
    return result;
  }
#else
  bool TimblExperiment::Test( const string& FileName,
			      const string& OutFile ){
    bool result = false;
    if ( initTestFiles( FileName, OutFile ) ){
      initExperiment();
      stats.clear();
      showTestingInfo( *mylog );
      // Start time.
      //
      time_t lStartTime;
      time(&lStartTime);
      timeval startTime;
      gettimeofday( &startTime, 0 );
      if ( InputFormat() == ARFF )
	skipARFFHeader( testStream );
      string Buffer;
      while ( nextLine( testStream, Buffer ) ){
	if ( !chopLine( Buffer ) ) {
	  Warning( "testfile, skipped line #" + 
		   toString<int>( stats.totalLines() ) +
		   "\n" + Buffer );
	}
	else {
	  chopped_to_instance( TestWords );
	  bool exact = false;
	  string distrib;
	  double distance;
	  const TargetValue *resultTarget = LocalClassify( CurrInst,
							   distance,
							   exact );
	  normalizeResult();
	  distrib = bestResult.getResult();
	  show_results( outStream, distrib, resultTarget, distance );
	  if ( exact ){ // remember that a perfect match may be incorrect!
	    if ( Verbosity(EXACT) ) {
	      *mylog << "Exacte match:\n" << get_org_input() << endl;
	    }
	  }
	  if ( !Verbosity(SILENT) )
	    // Display progress counter.
	    show_progress( *mylog, lStartTime, stats.dataLines() );
	}
      }
      if ( !Verbosity(SILENT) ){
	time_stamp( "Ready:  ", stats.dataLines() );
	show_speed_summary( *mylog, startTime );
	showStatistics( *mylog );
      }
      result = true;
    }
    return result;
  }
#endif

  bool TimblExperiment::NS_Test( const string& ,
				 const string& ){
    FatalError( "wrong algorithm" );
    return false;
  }

  bool IB1_Experiment::NS_Test( const string& FileName,
				const string& OutFile ){
    bool result = false;
    if ( initTestFiles( FileName, OutFile ) ){
      initExperiment();
      stats.clear();
      showTestingInfo( *mylog );
      // Start time.
      //
      time_t lStartTime;
      time(&lStartTime);
      timeval startTime;
      gettimeofday( &startTime, 0 );
      if ( InputFormat() == ARFF )
	skipARFFHeader( testStream );
      string Buffer;
      while ( nextLine( testStream, Buffer ) ){
	if ( !chopLine( Buffer ) ) {
	  Warning( "testfile, skipped line #" + 
		   toString<int>( stats.totalLines() ) +
		   "\n" + Buffer );
	}
	else {
	  chopped_to_instance( TestWords );
	  const neighborSet *res = LocalClassify( CurrInst );
	  outStream << get_org_input() << endl << *res;
	  if ( !Verbosity(SILENT) )
	    // Display progress counter.
	    show_progress( *mylog, lStartTime, stats.dataLines() );
	}
      }// end while.
      if ( !Verbosity(SILENT) ){
	time_stamp( "Ready:  ", stats.dataLines() );
	show_speed_summary( *mylog, startTime );
      }
      result = true;
    }
    return result;
  }
  
  bool TimblExperiment::SetOptions( int i, const char **argv ){
    CL_Options *Opts = new CL_Options( i, argv );
    bool result = SetOptions( *Opts  );
    delete Opts;
    return result;
  }

  bool TimblExperiment::SetOptions( const string& arg ){
    CL_Options *Opts = new CL_Options( arg );
    bool result = SetOptions( *Opts  );
    delete Opts;
    return result;
  }

  bool TimblExperiment::SetOptions( const CL_Options& Opts ){
    bool result;
    if ( IsClone() )
      result = OptParams->parse_options( Opts, 2 );
    else
      result = OptParams->parse_options( Opts, 0 );
    return result;
  }

  bool TimblExperiment::IndirectOptions( const CL_Options& Opts ){
    OptParams->set_default_options();
    return OptParams->parse_options( Opts, 1 );
  }

  bool TimblExperiment::ConfirmOptions(){
    return OptParams->definitive_options( this );
  }

  bool TimblExperiment::DefaultOptions(){
    OptParams->set_default_options();
    return true;
  }

  bool TimblExperiment::ShowOptions( ostream& os ){
    return ( ConfirmOptions() &&
	     MBLClass::ShowOptions( os ) );
  }

  bool TimblExperiment::ShowSettings( ostream& os ){
    return ( ConfirmOptions() &&
	     MBLClass::ShowSettings( os ) );
  }

  xmlNode *TimblExperiment::settingsToXML(){
    if ( ConfirmOptions() )
      return MBLClass::settingsToXml( );
    else
      return 0;
  }

  xmlNode *TimblExperiment::weightsToXML(){
    xmlNode *result = XmlNewNode( "currentWeights" );
    XmlSetAttribute( result, "weighting", toString( CurrentWeighting() ) );
    vector<double> wghts;
    GetCurrentWeights( wghts );
    for ( unsigned int i=0; i < wghts.size(); ++i ){
      xmlNode *n = XmlNewChild( result, "feature", toString(wghts[i]) );
      XmlSetAttribute( n, "index", toString(i+1) );
    }
    return result;
  }

  bool TimblExperiment::WriteArrays( const std::string& FileName ){
    ofstream out( FileName.c_str(), ios::out | ios::trunc );
    if ( !out ) {
      Warning( "Problem opening Probability file '" + 
	       FileName + "' (not written)" );
      return false;
    }
    else {
      if ( !Verbosity(SILENT) )
	Info( "Saving Probability Arrays in " + FileName );
      return MBLClass::writeArrays( out );
    }
  }
  
  bool TimblExperiment::GetArrays( const std::string& FileName ){
    ifstream inf( FileName.c_str(), ios::in );
    if ( !inf ){
      Error( "Problem opening Probability file " + FileName );
      return false;
    }
    else {
      if ( !Verbosity(SILENT) )
	Info( "Reading Probability Arrays from " + FileName );
      if ( !readArrays( inf ) ){
	Error( "Errors found in file " + FileName );
	return false;
      }
      else
	return true;
    }
  }

  bool TimblExperiment::WriteMatrices( const std::string& FileName ){
    ofstream out( FileName.c_str(), ios::out | ios::trunc );
    if ( !out ) {
      Warning( "Problem opening matrices file '" + 
	       FileName + "' (not written)" );
      return false;
    }
    else {
      if ( !Verbosity(SILENT) )
	Info( "Saving Matrices in " + FileName );
      initExperiment( );
      return writeMatrices( out );
    }
    }
    
  bool TimblExperiment::GetMatrices( const std::string& FileName ){
    ifstream inf( FileName.c_str(), ios::in );
    if ( !inf ){
      Error( "Problem opening matrices file " + FileName );
      return false;
    }
    else {
      if ( !Verbosity(SILENT) )
	Info( "Reading matrices from " + FileName );
      if ( !readMatrices( inf ) ){
	Error( "Errors found in file " + FileName );
	return false;
      }
      else
	return true;
    }
  }

  bool TimblExperiment::SaveWeights( const std::string& FileName ){
    if ( ConfirmOptions() ){
      // Open the output file.
      //
      ofstream outfile( FileName.c_str(), ios::out | ios::trunc);
      if (!outfile) {
	Warning( "can't open Weightsfile: " + FileName );
	return false;
      }
      else {
	if ( !Verbosity(SILENT) )
	  Info( "Saving Weights in " + FileName );
	if ( writeWeights( outfile ) )
	  return true;
	else {
	  Error( "failed to store weights in file " + FileName );
	  return false;
	}
      }
    }
    else
      return false;
  }

  bool TimblExperiment::GetWeights( const std::string& FileName, WeightType w ){
    if ( ConfirmOptions() ){
      // Open the file.
      //
      ifstream weightsfile( FileName.c_str(), ios::in);
      if ( !weightsfile) {
	Error( "can't open WeightsFile " + FileName );
	return false;
      }
      else {
	if ( w == Unknown_w ){
	  w = GR_w;
	  // 	Warning( "Unspecified weighting, using default: " 
	  // 		 + toString(w) );
	}
	if ( !Verbosity(SILENT) )
	  //	  Info( "Reading " + toString(w) + " weights from " + FileName );
	  Info( "Reading weights from " + FileName );
	if ( readWeights( weightsfile, w ) ){
	  WFileName = FileName;
	  return true;
	}
	else {
	  Warning( "Errors in Weightsfile " + FileName );
	  return false;
	}
      }
    }
    else
      return false;
  }

  bool TimblExperiment::WriteInstanceBase( const std::string& FileName ){
    bool result = false;
    if ( ConfirmOptions() ){
      ofstream outfile( FileName.c_str(), ios::out | ios::trunc );
      if (!outfile) {
	Warning( "can't open outputfile: " + FileName );
      }
      else {
	if ( !Verbosity(SILENT) )
	  Info( "Writing Instance-Base in: " + FileName );
	if ( PutInstanceBase( outfile ) )
	  result = true;
      }
    }
    return result;
  }
  
  bool TimblExperiment::WriteInstanceBaseXml( const std::string& FileName ) {
    bool result = false;
    if ( ConfirmOptions() ){
      ofstream os( FileName.c_str(), ios::out | ios::trunc );
      if (!os) {
	Warning( "can't open outputfile: " + FileName );
      }
      else {
	if ( !Verbosity(SILENT) )
	  Info( "Writing Instance-Base in: " + FileName );
	if ( ExpInvalid() ){
	  result = false;
	}
	else if ( InstanceBase == NULL ){
	  Warning( "unable to write an Instance Base, nothing learned yet" );
	}
	else {
	  InstanceBase->toXML( os );
	}
      }
    }
    return result;
  }

  bool TimblExperiment::WriteInstanceBaseLevels( const std::string& FileName,
						 unsigned int levels ) {
    bool result = false;
    if ( ConfirmOptions() ){
      ofstream os( FileName.c_str(), ios::out | ios::trunc );
      if (!os) {
	Warning( "can't open outputfile: " + FileName );
      }
      else {
	if ( !Verbosity(SILENT) )
	  Info( "Writing Instance-Base in: " + FileName );
	if ( ExpInvalid() ){
	  result = false;
	}
	else if ( InstanceBase == NULL ){
	  Warning( "unable to write an Instance Base, nothing learned yet" );
	}
	else {
	  InstanceBase->printStatsTree( os, levels );
	}
      }
    }
    return result;
  }

  bool IB1_Experiment::GetInstanceBase( istream& is ){
    bool result = false;
    bool Pruned;
    bool Hashed;
    int Version;
    string range_buf;
    if ( !get_IB_Info( is, Pruned, Version, Hashed, range_buf ) ){
      return false;
    }
    else if ( Pruned ){
      Error( "Instance-base is Pruned!, NOT valid for " + 
	     toString(algorithm) + " Algorithm" );
    }
    else {
      TreeOrder = DataFile;
      Initialize();
      if ( !get_ranges( range_buf ) ){
	Warning( "couldn't retrieve ranges..." );
      }
      else {
	srand( RandomSeed() );
	int pos=0;
	for ( size_t i=0; i < NumOfFeatures(); ++i ){
	  Features[i]->SetWeight( 1.0 );
	  if ( Features[permutation[i]]->Ignore() )
	    PermFeatures[i] = NULL;
	  else 
	    PermFeatures[pos++] = Features[permutation[i]];
	}
	InstanceBase = new IB_InstanceBase( EffectiveFeatures(), 
					    ibCount,
					    (RandomSeed()>=0) );
	if ( Hashed )
	  result = InstanceBase->ReadIB( is, PermFeatures,
					 Targets, 
					 TargetStrings, FeatureStrings,
					 Version ); 
	else
	  result = InstanceBase->ReadIB( is, PermFeatures, 
					 Targets, 
					 Version ); 
      }
    }
    return result;
  }
  
  bool TimblExperiment::ReadInstanceBase( const string& FileName ){
    bool result = false;
    if ( ConfirmOptions() ){
      ifstream infile( FileName.c_str(), ios::in );
      if ( !infile ) {
	Error( "can't open: " + FileName );
      }
      else {
	if ( !Verbosity(SILENT) )
	  Info( "Reading Instance-Base from: " + FileName );
	if ( GetInstanceBase( infile ) ){
	  if ( !Verbosity(SILENT) ){
	    IBInfo( cout );
	    writePermutation( cout );
	  }
	  result = true;
	}
      }
    }
    return result;
  }
  
  bool TimblExperiment::WriteNamesFile( const string& FileName ) const {
    // Open the file.
    //
    ofstream namesfile( FileName.c_str(), ios::out | ios::trunc);
    if (!namesfile) {
      Warning( "can't open NamesFile: '" +
	       FileName + "' (not written)" );
      return false;
    }
    else {
      if ( !Verbosity(SILENT) )
	Info( "Saving names in " + FileName );
      MBLClass::writeNamesFile( namesfile );
      return true;
    }
  }
  
  void IB1_Experiment::InitInstanceBase(){
    srand( RandomSeed() );
    set_order();
    runningPhase = TrainWords;
    InstanceBase = new IB_InstanceBase( EffectiveFeatures(), 
					ibCount,
					(RandomSeed()>=0) );
  }
  
  bool TimblExperiment::GetCurrentWeights( vector<double>& res ) {
    res.clear();
    if ( ExpInvalid() )
      return false;
    else { 
      initExperiment( );
      for ( size_t i=0; i< NumOfFeatures(); ++i ){
	res.push_back( Features[i]->Weight() );
      }
    }
    return true;
  }

  bool TimblExperiment::build_file_index( const string& file_name, 
					  fileIndex& fmIndex ){
    bool result = true;
    string Buffer;
    stats.clear();
    size_t cur_pos = 0;
    // Open the file.
    //
    ifstream datafile( file_name.c_str(), ios::in);
    if ( InputFormat() == ARFF )
      skipARFFHeader( datafile );
    cur_pos = datafile.tellg();
    if ( !nextLine( datafile, Buffer ) ){
      Error( "cannot start learning from in: " + file_name );
      result = false;    // No more input
    }
    else if ( !chopLine( Buffer ) ){
      Error( "no useful data in: " + file_name );
      result = false;    // No more input
    }
    else {
      if ( !Verbosity(SILENT) ) {
	Info( "Phase 2: Building index on Datafile: " + file_name );
	time_stamp( "Start:     ", 0 );
      }
      bool found;
      bool go_on = true;
      while( go_on ){ 
	// The next Instance to store. 
	//	cerr << "line at pos " << cur_pos << " : " << Buffer << endl;
	chopped_to_instance( TrainWords );
	//	cerr << "gives Instance " << &CurrInst << endl;
	FeatureValue *fv0 = CurrInst.FV[0];
	fileIndex::iterator it = fmIndex.find( fv0 );
	if ( it == fmIndex.end() ){
	  set<streamsize> st;
	  st.insert(cur_pos);
	  fmIndex[fv0] = st;
	}
	else {
	  it->second.insert( cur_pos );
	}
	if ((stats.dataLines() % Progress() ) == 0) 
	  time_stamp( "Indexing:  ", stats.dataLines() );
	found = false;
	while ( !found && 
		( cur_pos = datafile.tellg(),
		  nextLine( datafile, Buffer ) ) ){
	  found = chopLine( Buffer );
	  if ( !found ){
	    Warning( "datafile, skipped line #" + 
		     toString<int>( stats.totalLines() ) +
		     "\n" + Buffer );
	  }
	}
	go_on = found;
      }
      time_stamp( "Finished:  ", stats.dataLines() );
    }
    return result;
  }
  
  bool TimblExperiment::build_file_multi_index( const string& file_name, 
						fileDoubleIndex& fmIndex ){
    bool result = true;
    string Buffer;
    stats.clear();
    size_t cur_pos = 0;
    // Open the file.
    //
    ifstream datafile( file_name.c_str(), ios::in);
    if ( InputFormat() == ARFF )
      skipARFFHeader( datafile );
    cur_pos = datafile.tellg();
    if ( !nextLine( datafile, Buffer ) ){
      Error( "cannot start learning from in: " + file_name );
      result = false;    // No more input
    }
    else if ( !chopLine( Buffer ) ){
      Error( "no useful data in: " + file_name );
      result = false;    // No more input
    }
    else {
      if ( !Verbosity(SILENT) ) {
	Info( "Phase 2: Building multi index on Datafile: " + file_name );
	time_stamp( "Start:     ", 0 );
      }
      bool found;
      bool go_on = true;
      while( go_on ){ 
	// The next Instance to store. 
	//	cerr << "line at pos " << cur_pos << " : " << Buffer << endl;
	chopped_to_instance( TrainWords );
	//	cerr << "gives Instance " << &CurrInst << endl;
	FeatureValue *fv0 = CurrInst.FV[0];
	FeatureValue *fv1 = CurrInst.FV[1];
	fileDoubleIndex::iterator it = fmIndex.find( fv0 );
	if ( it != fmIndex.end() )
	  it->second[fv1].insert( cur_pos );
	else {
	  fileIndex mi;
	  mi[fv1].insert( cur_pos );
	  fmIndex[fv0] = mi;
	}
	if ((stats.dataLines() % Progress() ) == 0) 
	  time_stamp( "Indexing:  ", stats.dataLines() );
	found = false;
	while ( !found && 
		( cur_pos = datafile.tellg(),
		  nextLine( datafile, Buffer ) ) ){
	  found = chopLine( Buffer );
	  if ( !found ){
	    Warning( "datafile, skipped line #" + 
		     toString<int>( stats.totalLines() ) +
		     "\n" + Buffer );
	  }
	}
	go_on = found;
      }
      time_stamp( "Finished:  ", stats.dataLines() );
    }
    return result;
  }
  
  
}
