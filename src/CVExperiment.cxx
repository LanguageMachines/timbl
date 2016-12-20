/*
  Copyright (c) 1998 - 2017
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
#include <sstream>
#include <iomanip>
#include <cassert>

#include <sys/time.h>

#include "timbl/MsgClass.h"
#include "timbl/Common.h"
#include "timbl/Types.h"
#include "timbl/TimblExperiment.h"

namespace Timbl {
  using namespace std;

  bool CV_Experiment::Prepare( const string& f, bool, bool ){
    cerr << "CV prepare " << f << endl;
    return true;
  }

  bool CV_Experiment::CVprepare( const string& wgtFile,
				 WeightType w,
				 const string& probFile ){
    CV_WfileName = wgtFile;
    CV_fileW = w;
    CV_PfileName = probFile;
    return true;
  }

  bool CV_Experiment::Learn( const string& f, bool ){
    cerr << "CV Learn " << f << endl;
    return true;
  }

  bool CV_Experiment::checkTestFile(){
    if ( !IB1_Experiment::checkTestFile() )
      return false;
    else if ( doSamples() ){
      FatalError( "Cannot Cross validate on a file with Examplar Weighting" );
      return false;
    }
    else if ( Verbosity(FEAT_W) ){
      LearningInfo( *mylog );
    }
    return true;
  }

  bool CV_Experiment::get_file_names( const string& FileName ){
    if ( !ExpInvalid() ){
      size_t size = 0;
      ifstream file_names( FileName, ios::in );
      if ( !file_names ){
	Error( "Unable to read CV filenames from " + FileName );
	return false;
      }
      string name;
      while ( getline( file_names, name ) ){
	size_t tmp = examineData( name );
	if ( tmp != 0 ){
	  if ( !Verbosity(SILENT) ){
	    *mylog << "Examine datafile '" << name
		   << "' gave the following results:"
		   << endl
		   << "Number of Features: " << tmp << endl;
	    showInputFormat( *mylog );
	  }
	  FileNames.push_back(name);
	  if ( size == 0 )
	    size = tmp;
	  else
	    if ( tmp != size ) {
	      Error( "mismatching number of features in file " +
		     name + "of CV filelist " + FileName );
	      return false;
	    }
	}
	else {
	  Error( "unable to determine number of features in file " +
		 name + "of CV filelist " + FileName );
	  return false;
	}
      }
      if ( FileNames.size() < 3 ){
	Error( "Not enough filenames found in CV filelist " + FileName
	       + " at least 3 required" );
	return false;
      }
      return true;
    }
    return false;
  }

  bool CV_Experiment::Test( const string& FileName,
			    const string& OutFile ){
    if ( !ConfirmOptions() )
      return false;
    (void)OutFile;
    bool result = false;
    VerbosityFlags keep = get_verbosity();
    set_verbosity( SILENT );
    if ( get_file_names( FileName ) ){
      *mylog << "Starting Cross validation test on files:" << endl;
      for ( const auto& name : FileNames ){
	*mylog << name << endl;
      }
      size_t NumOfFiles = FileNames.size();
      TimblExperiment::Prepare( FileNames[1], false );
      TimblExperiment::Learn( FileNames[1], false );
      for ( size_t filenum = 2; filenum < NumOfFiles; ++filenum )
	Expand( FileNames[filenum] );
      string outName;
      string percName;
      for ( size_t SkipFile = 0; SkipFile < NumOfFiles-1; ++SkipFile ) {
	outName = correct_path( FileNames[SkipFile], outPath, false );
	outName += ".cv";
	percName = outName;
	percName += ".%";
	set_verbosity( keep );
	if ( CV_WfileName != "" )
	  GetWeights( CV_WfileName, CV_fileW );
	if ( !CV_PfileName.empty() )
	  GetArrays( CV_PfileName );
	result = TimblExperiment::Test( FileNames[SkipFile], outName );
	if ( result )
	  result = createPercFile( percName );
	if ( !result )
	  return false;
	set_verbosity( SILENT );
	Expand( FileNames[SkipFile] );
	Remove( FileNames[SkipFile+1] );
      }
      outName = correct_path( FileNames[NumOfFiles-1], outPath, false );
      outName += ".cv";
      percName = outName;
      percName += ".%";
      set_verbosity( keep );
      if ( CV_WfileName != "" )
	GetWeights( CV_WfileName, CV_fileW );
      if ( !CV_PfileName.empty() )
	GetArrays( CV_PfileName );
      result = TimblExperiment::Test( FileNames[NumOfFiles-1], outName );
      if ( result )
	result = createPercFile( percName );
    }
    return result;
  }

}
