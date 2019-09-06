/*
  Copyright (c) 1998 - 2019
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

#include <string>
#include <map>
#include <fstream>
#include <cassert>

#include <sys/time.h>

#include "timbl/MsgClass.h"
#include "timbl/Common.h"
#include "timbl/Types.h"
#include "timbl/IBtree.h"
#include "timbl/MBLClass.h"
#include "timbl/TimblExperiment.h"

namespace Timbl {
  using namespace std;

  void LOO_Experiment::initExperiment( bool all_vd ){
    if ( !ExpInvalid() ){
      if ( !MBL_init ){  // do this only when necessary
	initDecay();
	if ( !is_copy ){
	  calculate_fv_entropy( true );
	  if ( initProbabilityArrays( all_vd ) )
	    calculatePrestored();
	  else {
	    Error( string("not enough memory for Probability Arrays")
		   + "' in ("
		   + __FILE__  + "," + TiCC::toString(__LINE__) + ")\n"
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

  bool LOO_Experiment::checkTestFile(){
    // no need to test the Testfile
    // it is the same as the trainfile, so already checked
    if ( doSamples() ){
      FatalError( "Cannot Leave One Out on a file with Examplar Weighting" );
      return false;
    }
    return true;
  }

  void LOO_Experiment::showTestingInfo( ostream& os ){
    if ( !Verbosity(SILENT) ){
      if ( Verbosity(OPTIONS ) )
	ShowSettings( os );
      os << endl << "Starting to test using Leave One Out";
      if ( Do_Sloppy_LOO() )
	os << " using SLOPPY metric calculations" << endl;
      else
	os << endl;
      os   << "Writing output in:          " << outStreamName << endl
	   << "Algorithm     : LOO" << endl;
      show_metric_info( os );
      show_weight_info( os );
      os << decay << endl;
    }
  }

  bool LOO_Experiment::Test( const string& FileName,
			     const string& OutFile ){
    bool result = false;
    if ( initTestFiles( FileName, OutFile ) ){
      if ( InstanceBase->nodeCount() == InstanceBase->depth() + 1 ){
	// protect ourselves against 1-line trainfiles
	FatalError( "the file '" + FileName + "' contains only 1 usable line. LOO impossible!" );
      }
      initExperiment();
      stats.clear();
      delete confusionInfo;
      confusionInfo = 0;
      if ( Verbosity(ADVANCED_STATS) )
	confusionInfo = new ConfusionMatrix( Targets->ValuesArray.size() );
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
		   TiCC::toString<int>( stats.totalLines() ) +
		   "\n" + Buffer );
	}
	else {
	  chopped_to_instance( TestWords );
	  Decrement( CurrInst );
	  double final_distance = 0.0;
	  bool exact = false;
	  const TargetValue *ResultTarget = LocalClassify( CurrInst,
							   final_distance,
							   exact );
	  normalizeResult();
	  string dString = bestResult.getResult();
	  double confi = 0;
	  if ( Verbosity(CONFIDENCE) ){
	    confi = confidence();
	  }
	  // Write it to the output file for later analysis.
	  show_results( outStream, confi, dString,
			ResultTarget, final_distance );
	  if ( exact ){ // remember that a perfect match may be incorrect!
	    if ( Verbosity(EXACT) ) {
	      *mylog << "Exacte match:\n" << get_org_input() << endl;
	    }
	  }
	  if ( !Verbosity(SILENT) ){
	    // Display progress counter.
	    show_progress( *mylog, lStartTime, stats.dataLines() );
	  }
	  Increment( CurrInst );
	}
      }// end while.
      if ( !Verbosity(SILENT) ){
	time_stamp( "Ready:  ", stats.dataLines() );
	show_speed_summary( *mylog, startTime );
	showStatistics( *mylog );
      }
      result = true;
    }
    return result;
  }

  bool LOO_Experiment::ReadInstanceBase( const string& ){
    Error( "cannot combine Leave One Out with retrieving an Instancebase " );
    return false;
  }

}
