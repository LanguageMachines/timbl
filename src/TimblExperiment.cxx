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

#ifdef USE_LOGSTREAMS
#include "timbl/LogStream.h"
#else
typedef std::ostream LogStream;
#define Log(X) (X)
#define Dbg(X) (X)
#endif

#include "timbl/SocketBasics.h"
#include "timbl/MBLClass.h"
#include "timbl/CommandLine.h"
#include "timbl/GetOptClass.h"
#include "timbl/TimblExperiment.h"
#include "timbl/ServerProcs.h"

namespace Timbl {
  using namespace std;
  using namespace Timbl;

  resultStore::~resultStore( ) {
    clear();
  }

  void resultStore::reset( int _beam, normType _norm, 
			   double _factor, const Target *_targets ) {
    clear();
    beam = _beam;
    norm = _norm;
    factor = _factor;
    targets = _targets;
    if ( norm != noNorm &&
	 beam != 0 ){
      Warning( "no normalisation possible because a BeamSize is specified" );
      Info( "output is NOT normalized!" );
      norm = noNorm;
    }
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
    estimate( 0 ),
    max_conn( 10 )
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
      max_conn = in.max_conn;
      Weighting = in.Weighting;
      confusionInfo = 0;
    }
    return *this;
  }
  
  TimblExperiment *TimblExperiment::CreateClient( ServerSocket *socket ) const {
    TimblExperiment *result = 0;
    switch ( Algorithm() ){
    case IB1_a:
    case TRIBL_a: 
    case TRIBL2_a: 
    case IGTREE_a: 
      result = clone();
      break;
    default:
      FatalError( "You may not Create Clients for Special cases like " + 
		  toString(algorithm) );
    }
    *result = *this;
    string line = "Client on socket: " + toString<int>( socket->getSockId() );
#ifdef USE_LOGSTREAMS
    result->myerr->message( line );
    result->mydebug->message( line );
#endif
    if ( OptParams ){
      result->OptParams = OptParams->Clone( socket );
    }
    result->Socket( socket );
    result->WFileName = WFileName;
    result->CurrentDataFile = CurrentDataFile;
    *Dbg(mydebug) << "created a new Client" << endl;
    *Dbg(result->mydebug) <<  "I am the new client" << endl;
    return result;
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
    string line = "child-";
#ifdef USE_LOGSTREAMS
    result->myerr->message( line );
    result->mydebug->message( line );
#endif
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
      if ( !MBL_init ){  // do this only when necessary
	stats.clear();
	delete confusionInfo;
	confusionInfo = 0;
	if ( Verbosity(ADVANCED_STATS) )
	  confusionInfo = new ConfusionMatrix( Targets->ValuesArray.size() );
	initDecay();
	if ( !is_copy ){
	  calculate_fv_entropy( true );
	  if ( ib2_offset != 0 ){
	    // invalidate MVDM matrices, they might be changing in size
	    for ( size_t j=0; j < NumOfFeatures(); ++j ){
	      if ( !Features[j]->Ignore() ){
		Features[j]->delete_matrix();
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
	  InitWeights();
	  if ( do_diversify )
	    diverseWeights();
	  srand( random_seed );
	}
	initTesters();
	MBL_init = true;
      }
    }
  }
  
  bool TimblExperiment::StartServer( const int port, const int max_c ){
    initExperiment( true );
    RunServer( this, port );
    max_conn = max_c;
    return true;
  }
  
  bool TimblExperiment::SetSingleThreaded(){
#ifdef USE_LOGSTREAMS
    if ( !mylog->set_single_threaded_mode() ||
	 !myerr->set_single_threaded_mode() ||
	 !mydebug->set_single_threaded_mode() ){
      cerr << "problem setting to single threaded mode" << endl;
      return false;
    }
#endif
    return true;
  }

  bool TimblExperiment::skipARFFHeader( istream& is ){
    string Buffer;
    while ( getline( is, Buffer ) &&
	    !compare_nocase_n( "@DATA", Buffer) )
      stats.addSkipped();
    return true;
  }
  
  bool TimblExperiment::nextLine( istream& datafile, string& Line ){
    // Function that takes a line from a file, skipping comment
    // returns true if some line is found
    //
    bool found = false;
    while ( !found && getline( datafile, Line ) ){
      if ( empty_line( Line, InputFormat() ) ){
	stats.addSkipped();
	continue;
      } else 
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
  bool TimblExperiment::Prepare( const string& FileName ){
    assert( runningPhase == LearnWords );
    bool result = false;
    if ( FileName != "" && ConfirmOptions() ){
      if ( !ExpInvalid() ){
	if ( Options.TableFrozen() ||
	     NumOfFeatures() != 0 ){
	  Error( "couldn't learn from file '" + FileName +
		 "'\nInstanceBase already filled" );
	}
	else {
	  size_t Num = examineData( FileName );
	  if ( Num == 0 ){
	    Error( "Unable to initialize from file :'" + FileName + "'\n" );
	  }
	  else {
	    if ( NumOfFeatures() == 0 ){
	      Initialize( Num );
	    }
	    CurrentDataFile = FileName;
	    if ( Verbosity(OPTIONS) ){
	      ShowSettings( *Log(mylog) );
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
		if ( Verbosity(FEAT_W) ){
		  *Log(mylog) << "Lines of data     : " 
			      << stats.dataLines() << endl;
		  if ( stats.skippedLines() != 0 )
		    *Log(mylog) << "SkippedLines      : "
				<< stats.skippedLines() << endl;
		  LearningInfo( *Log(mylog) );
		}
		else
		  calculate_fv_entropy( false );
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

  bool TimblExperiment::Learn( const string& FileName ){
    bool result = true;
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
	  if ( !Prepare( FileName ) || ExpInvalid() ){
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
	InitInstanceBase( );
	if ( !Verbosity(SILENT) ) {
	  Info( "Phase 2: Learning from Datafile: " + CurrentDataFile );
	  time_stamp( "Start:     ", 0 );
	}
	bool found;
	bool go_on = ( IB2_offset() == 0 || stats.dataLines() <= IB2_offset() );
	while( go_on ){ 
	  // The next Instance to store. 
	  chopped_to_instance( TrainWords );
	  if ( !IBAdd( CurrInst ) ){
	    Warning( "deviating exemplar weight in line #" +
		     toString<int>(stats.totalLines()) + ":\n" +
		     Buffer + "\nIgnoring the new weight" );
	  }
	  // Progress update.
	  //
	  if ((stats.dataLines() % Progress() ) == 0) 
	    time_stamp( "Learning:  ", stats.dataLines() );
	  if ( IB2_offset() > 0 && stats.dataLines() >= IB2_offset() )
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
	if ( !Verbosity(SILENT) )
	  IBInfo( *Log(mylog) );
      }
    }
    return result;
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
	if ( !IBAdd( CurrInst ) ){
	  Warning( "deviating exemplar weight in:\n" + 
		   InstanceString + "\nIgnoring the new weight" );
	}
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
	  Info( "Phase 2: Expanding from Datafile: " + FileName );
	  time_stamp( "Start:     ", 0 );
	}
	bool found;
	do {
	  // The next Instance to store. 
	  chopped_to_instance( TrainLearnWords );
	  if ( !IBAdd( CurrInst ) ){
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
	  IBInfo( *Log(mylog) );
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
	  IBInfo( *Log(mylog) );
      }
    }
    return result;
  }
  
  void TimblExperiment::show_progress( ostream& os,time_t start ){
    char time_string[26];
    struct tm *curtime;
    time_t Time;
    time_t SecsUsed;
    time_t EstimatedTime;
    double Estimated;
    int local_progress = Progress();
    unsigned int line = stats.dataLines();
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

  void TimblExperiment::createPercFile( const string& fileName ) const {
    if ( fileName != "" ) {
      ofstream outfile( fileName.c_str(), ios::out | ios::trunc);
      if (!outfile) {
	Warning( "can't open: " + fileName );
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
  }
  
  bool TimblExperiment::showBestNeighbors( ostream& outfile ) const {
    if ( Verbosity( NEAR_N | ALL_K) ){
      outfile << bestArray;
      return true;
    }
    else
      return false;
  }
    
  void TimblExperiment::show_results( ostream& outfile, 
				      const string& dString,
				      const TargetValue *Best,
				      const double Distance ) {
    show_org_input( outfile );
    outfile << CodeToStr(Best->Name());
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
    outfile << endl;
    showBestNeighbors( outfile );
  }
  
  bool IB2_Experiment::Prepare( const string& FileName  ){
    if ( ConfirmOptions() && IB2_offset() == 0 ){
      Error( "IB2 learning failed, invalid bootstrap option?" );
      return false;
    }
    else
      return TimblExperiment::Prepare( FileName );
  }
  
  bool IB2_Experiment::Learn( const string& FileName ){
    bool result = false;
    if ( IB2_offset() == 0 )
      Error( "IB2 learning failed, invalid bootstrap option?" );
    else if ( TimblExperiment::Learn( FileName ) )
      result = Expand_N( FileName );
    return result;
  }
  
  bool IB2_Experiment::Expand( const string& FileName ){
    bool result = false;
    if ( CurrentDataFile == "" ){
      Warning( "IB2, cannot Append data: No datafile bootstrapped yet" );
    }
    else {
      IB2_offset( 0 );
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
    else if ( CurrentDataFile == "" ){
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
	      if ( !IBAdd( CurrInst ) ){
		Warning( "deviating exemplar weight in line #" + 
			 toString<int>(stats.totalLines() ) + ":\n" +
			 Buffer + "\nIgnoring the new weight" );
	      }
	      *Dbg(mydebug) << "adding " << &CurrInst << endl;
	      ++Added;
	      ++TotalAdded;
	      MBL_init = true; // avoid recalculations in LocalClassify
	      
	    }
	    // Progress update.
	    //
	    if ( show_learn_progress( *Log(mylog), lStartTime, Added ) ){
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
	    *Log(mylog) << "in total added " << TotalAdded << " new entries" << endl;
	    if ( !Verbosity(SILENT) ){
	      IBInfo( *Log(mylog) );
	      LearningInfo( *Log(mylog) );
	    }
	    MBL_init = false; // force recalculations when testing
	  }
	}
    }
  }
    return result;
  }
  
  bool TimblExperiment::checkFile( const string& FileName ){
    bool result = false;
    if ( !ExpInvalid() &&
	 ConfirmOptions() ){
      size_t i;
      runningPhase = TestWords;
      if ( IBStatus() == Invalid )
	Warning( "you tried to apply the " + toString( algorithm ) +
		 " algorithm, but no Instance Base is available yet" );
      else if ( FileName != "" &&
		( i = examineData( FileName )) != NumOfFeatures() ){
	if ( i == 0 ){
	  Error( "unable to use the data from '" + FileName +
		 "', wrong Format?" );
	}
	else
	  Error( "mismatch between number of features in Testfile " +
		 FileName + " and the Instancebase (" +
		 toString<size_t>(i) + " vs. " + 
		 toString<size_t>(NumOfFeatures()) + ")" ); 
      }
      else {
	initExperiment();
	result = true;
      }
    }
    return result;
  }
  
  bool IB1_Experiment::checkFile( const string& FileName ){
    if ( !TimblExperiment::checkFile( FileName ) )
      return false;
    else if ( IBStatus() == Pruned ){
      Warning( "you tried to apply the " + toString( algorithm) +
	       " algorithm on a pruned Instance Base" );
      return false;
    }
    return true;
  }
  
  bool IB2_Experiment::checkFile( const string& FileName ){
    if ( !IB1_Experiment::checkFile( FileName ) )
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
    TestInstance( Inst, base, offset );
  }

  const TargetValue *TimblExperiment::LocalClassify( const Instance& Inst,
						     double& Distance,
						     bool& exact ){
    bool recurse = true;
    bool Tie = false; 
    exact = false;
    bestResult.reset( beamSize, normalisation, norm_factor, Targets );
    const ValueDistribution *ExResultDist = ExactMatch( Inst );
    WValueDistribution *ResultDist = 0;
    nSet.clear();
    const TargetValue *Res;
    if ( ExResultDist ){
      Distance = 0.0;
      recurse = !Do_Exact();
      // no retesting when exact match and the user ASKED for them..
      Res = ExResultDist->BestTarget( Tie, (RandomSeed() >= 0) );
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
    if ( confusionInfo )
      confusionInfo->Increment( Inst.TV, Res );
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
  
  bool TimblExperiment::LocalTest( const Instance& Inst, ostream& outfile ){
    double final_distance = 0.0;
    bool exact = false;
    const TargetValue *ResultTarget = LocalClassify( Inst, 
						     final_distance,
						     exact );
    normalizeResult();
    string dString = bestResult.getResult();
    // Write it to the output file for later analysis.
    show_results( outfile, dString, ResultTarget, final_distance );
    return exact;
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
	    if ( Features[i]->matrix_present() )
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

  void TimblExperiment::testing_info( ostream& os, 
				      const string& FileName, 
				      const string& OutFile ){
    if ( Verbosity(OPTIONS ) )
      ShowSettings( os );
    os << endl << "Starting to test, Testfile: " << FileName << endl
       << "Writing output in:          " << OutFile << endl
       << "Algorithm     : " << toString( Algorithm() ) << endl;
    show_metric_info( os );
    show_weight_info( os );
    os << decay << endl;
  }

  bool TimblExperiment::Test( const string& FileName,
			      const string& OutFile, 
			      const string& PercFile ){
    bool result = false;
    if ( checkFile( FileName ) ){
      // Open the files
      //
      istream *testfile;
      ostream *outfile = NULL;;
      ifstream inp_file;
      ofstream out_file;
      if ( FileName == "-" )
	testfile = &cin;
      else {
	inp_file.open( FileName.c_str(), ios::in);
	testfile = &inp_file;
      }
      if ( OutFile == "-" )
	outfile = &cout;
      else {
	out_file.open( OutFile.c_str(), ios::out | ios::trunc );
	if ( out_file )
	  outfile = &out_file;
      }
      if (!outfile) {
	Error( "can't open: " + OutFile );
      }
      else {
	string Buffer;
	stats.clear();
	if ( !Verbosity(SILENT) )
	  testing_info( *Log(mylog), FileName, OutFile );
      
	// Start time.
	//
	time_t lStartTime;
	time(&lStartTime);
	timeval startTime;
	gettimeofday( &startTime, 0 );
	if ( InputFormat() == ARFF )
	  skipARFFHeader( *testfile );
	while ( nextLine( *testfile, Buffer ) ){
	  if ( !chopLine( Buffer ) ) {
	    Warning( "testfile, skipped line #" + 
		     toString<int>( stats.totalLines() ) +
		     "\n" + Buffer );
	  }
	  else {
	    chopped_to_instance( TestWords );
	    bool exact = LocalTest( CurrInst, *outfile );
	    if ( exact ){ // remember that a perfect match may be incorrect!
	      if ( Verbosity(EXACT) ) {
		*Log(mylog) << "Exacte match:\n";
		show_org_input( *Log(mylog) );
		*Log(mylog) << endl;
	      }
	    }
	    if ( !Verbosity(SILENT) )
	      // Display progress counter.
	      show_progress( *Log(mylog), lStartTime );
	  }
	}// end while.
	if ( OutFile != "-" )
	  out_file.close();
	if ( !Verbosity(SILENT) ){
	  time_stamp( "Ready:  ", stats.dataLines() );
	  show_speed_summary( *Log(mylog), startTime );
	  showStatistics( *Log(mylog) );
	}
	createPercFile( PercFile );
	result = true;
      }
    }
    return result;
  }

  bool TimblExperiment::NS_Test( const string& ,
				 const string& ){
    FatalError( "wrong algorithm" );
    return false;
  }

  bool IB1_Experiment::NS_Test( const string& FileName,
				const string& OutFile ){
    bool result = false;
    if ( checkFile( FileName ) ){
      // Open the files
      //
      istream *testfile;
      ostream *outfile = NULL;;
      ifstream inp_file;
      ofstream out_file;
      if ( FileName == "-" )
	testfile = &cin;
      else {
	inp_file.open( FileName.c_str(), ios::in);
	testfile = &inp_file;
      }
      if ( OutFile == "-" )
	outfile = &cout;
      else {
	out_file.open( OutFile.c_str(), ios::out | ios::trunc );
	if ( out_file )
	  outfile = &out_file;
      }
      if (!outfile) {
	Error( "can't open: " + OutFile );
      }
      else {
	string Buffer;
	stats.clear();
	if ( !Verbosity(SILENT) )
	  testing_info( *Log(mylog), FileName, OutFile );
      
	// Start time.
	//
	time_t lStartTime;
	time(&lStartTime);
	timeval startTime;
	gettimeofday( &startTime, 0 );
	if ( InputFormat() == ARFF )
	  skipARFFHeader( *testfile );
	while ( nextLine( *testfile, Buffer ) ){
	  if ( !chopLine( Buffer ) ) {
	    Warning( "testfile, skipped line #" + 
		     toString<int>( stats.totalLines() ) +
		     "\n" + Buffer );
	  }
	  else {
	    chopped_to_instance( TestWords );
	    const neighborSet *res = LocalClassify( CurrInst );
	    show_org_input( *outfile );
	    *outfile << endl << *res;
	    if ( !Verbosity(SILENT) )
	      // Display progress counter.
	      show_progress( *Log(mylog), lStartTime );
	  }
	}// end while.
	if ( OutFile != "-" )
	  out_file.close();
	if ( !Verbosity(SILENT) ){
	  time_stamp( "Ready:  ", stats.dataLines() );
	  show_speed_summary( *Log(mylog), startTime );
	}
	result = true;
      }
    }
    return result;
  }
  
  void TimblExperiment::UseOptions( GetOptClass *gec ){
    OptParams = gec;
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
	  InstanceBase->IBtoXML( os );
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
      Error( "Can'n retrieve Instance-Base\n" );
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
	InstanceBase = new IB_InstanceBase( EffectiveFeatures(), 
					    ibCount,
					    (RandomSeed()>=0) );
	int pos=0;
	for ( size_t i=0; i < NumOfFeatures(); ++i ){
	  Features[i]->SetWeight( 1.0 );
	  if ( Features[permutation[i]]->Ignore() )
	    PermFeatures[i] = NULL;
	  else 
	    PermFeatures[pos++] = Features[permutation[i]];
	}
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
	    writePermutation( my_log() );
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
  
  
}
