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
#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <cstdlib>

#include <sys/time.h>

#include "timbl/MsgClass.h"
#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/Tree.h"
#include "timbl/Instance.h"
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
      initExperiment();
      stats.clear();
      delete confusionInfo;
      confusionInfo = 0;
      if ( Verbosity(ADVANCED_STATS) )
	confusionInfo = new ConfusionMatrix( Targets->ValuesArray.size() );
      showTestingInfo( *Log(mylog) );
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
	  Decrement( CurrInst );
	  bool exact = LocalTest( CurrInst, outStream );
	  if ( exact ){ // remember that a perfect match may be incorrect!
	    if ( Verbosity(EXACT) ) {
	      *Log(mylog) << "Exacte match:\n";
	      show_org_input( *Log(mylog) );
	      *Log(mylog) << endl;
	    }
	  }
	  // Display progress counter.
	  show_progress( *Log(mylog), lStartTime );
	  Increment( CurrInst );
	}
      }// end while.
      time_stamp( "Ready:  ", stats.dataLines() );
      show_speed_summary( *Log(mylog), startTime );
      showStatistics( *Log(mylog) );
      result = true;
    }
    return result;
  }

  bool LOO_Experiment::ReadInstanceBase( const string& ){
    Error( "cannot combine Leave One Out with retrieving an Instancebase " );
    return false;
  }
  
}
