/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2015
  ILK   - Tilburg University
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
      http://ilk.uvt.nl/software.html
  or send mail to:
      timbl@uvt.nl
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
#include "timbl/Options.h"
#include "timbl/Instance.h"
#include "timbl/Statistics.h"
#include "timbl/neighborSet.h"
#include "timbl/BestArray.h"
#include "timbl/IBtree.h"
#include "timbl/MBLClass.h"
#include "ticcutils/CommandLine.h"
#include "timbl/TimblExperiment.h"

using namespace TiCC;
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
    bool result = false;
    if ( !ExpInvalid() ){
      NumOfFiles = 0;
      ifstream file_names( FileName, ios::in );
      string name;
      if ( file_names.good() ) {
	while ( getline( file_names, name ) )
	  ++NumOfFiles;
	file_names.close();
	FileNames = new string[NumOfFiles];
	ifstream file_names2( FileName, ios::in );
	size_t size = 0;
	int pos = 0;
	while ( getline( file_names2, name ) ){
	  size_t tmp = examineData( name );
	  if ( tmp != 0 ){
	    if ( !Verbosity(SILENT) ){
	      *mylog << "Examine datafile '" << FileName
		     << "' gave the following results:"
		     << endl
		     << "Number of Features: " << tmp << endl;
	      showInputFormat( *mylog );
	    }
	    FileNames[pos++] = name;
	    if ( size == 0 )
	      size = tmp;
	    else
	      if ( tmp != size ) {
		Error( "mismatching number of features in file " +
		       name + "of CV filelist " + FileName );
		return false;
	      }
	  }
	  else
	    return false;
	}
	if ( pos != NumOfFiles ){
	  Error( "Unable to read all " + toString<int>(NumOfFiles) +
		 " CV filenames from " + FileName );
	  return false;
	}
	else
	  result = true;
      }
      else
	Error( "Unable to read CV filenames from " + FileName );
    }
    return result;
  }

  static string fixPath( const string& fileName,
			 const string& path ){
    // if fileName contains pathinformation, it is replaced with path
    // if filename contains NO pathinformation, path is always appended.
    // of course we don't append if the path is empty

    if ( !path.empty() ){
      bool addSlash = path[path.length()] != '/';
      string tmp;
      string::size_type pos = fileName.rfind( '/' );
      if ( pos == string::npos ){
	tmp = path;
	if ( addSlash )
	  tmp += "/";
	tmp += fileName;
      }
      else {
	tmp = path;
	if ( addSlash )
	tmp += "/";
	tmp += fileName.substr( pos+1 );
      }
      return tmp;
    }
    else
      return fileName;
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
      for ( int i = 0; i < NumOfFiles; ++i )
	*mylog << FileNames[i] << endl;
      TimblExperiment::Prepare( FileNames[1], false );
      TimblExperiment::Learn( FileNames[1], false );
      for ( int filenum = 2; filenum < NumOfFiles; ++filenum )
	Expand( FileNames[filenum] );
      string outName;
      string percName;
      for ( int SkipFile = 0; SkipFile < NumOfFiles-1; ++SkipFile ) {
	outName = fixPath( FileNames[SkipFile], outPath );
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
      outName = fixPath( FileNames[NumOfFiles-1], outPath );
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
