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

#include <exception>
#include <list>
#include <vector>
#include <iosfwd>
#include <string>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cctype>

#include "timbl/TimblAPI.h"
#include "timbl/Options.h"
#include "timbl/ServerBase.h"
#include "timbl/TimblServerAPI.h"

using namespace std;
using namespace Timbl;

static list<string> ind_lines;

Algorithm algorithm;

bool Do_Server = false;
bool Do_Multi_Server = false;
int ServerPort = -1;
int Max_Connections = 10;

string I_Path;
string Q_value;
string dataFile;
string ServerConfigFile;
string MatrixInFile;
string TreeInFile;
string WgtInFile;
Weighting WgtType = UNKNOWN_W;
string ProbInFile;

inline void usage_full(void){
  cerr << "usage: TimblServer -f data-file {-t test-file} [options]" << endl;
  cerr << "Algorithm and Metric options:" << endl;
  cerr << "-a n      : algorithm" << endl;
  cerr << "     0 or IB1   : IB1     (default)" << endl;
  cerr << "     1 or IG    : IGTree" << endl;
  cerr << "     2 or TRIBL : TRIBL" << endl;
  cerr << "     3 or IB2   : IB2" << endl;
  cerr << "     4 or TRIBL2 : TRIBL2" << endl;
  cerr << "-m s      : use feature metrics as specified in string s:" 
       << endl
       << "            format: GlobalMetric:MetricRange:MetricRange" 
	 << endl
       << "            e.g.: mO:N3:I2,5-7" << endl;
  cerr << "     C: Cosine distance. (Global only. numeric features implied)" 
       << endl;
  cerr << "     D: Dot product. (Global only. numeric features implied)" 
       << endl;
  cerr << "     DC: Dice Coefficient" << endl;
  cerr << "     O: weighted Overlap (default)" << endl;
  cerr << "     L: Levenshtein distance" << endl;
  cerr << "     M: Modified value difference" << endl;
  cerr << "     J: Jeffrey Divergence" << endl;
  cerr << "     N: numeric values" << endl;
  cerr << "     I: Ignore named  values" << endl;
  cerr << "-w n      : Weighting" << endl;
  cerr << "     0: No Weighting" << endl;
  cerr << "     1: Weight using GainRatio (default)" << endl;
  cerr << "     2: Weight using InfoGain" << endl;
  cerr << "     3: Weight using Chi-square" << endl;
  cerr << "     4: Weight using Shared Variance" << endl;
  cerr << "-w f      : read Weights from file 'f'" << endl;
  cerr << "-w f:n    : read Weight n from file 'f'" << endl;
  cerr << "-b n      : number of lines used for bootstrapping (IB2 only)" 
       << endl;
  cerr << "--Diversify: rescale weight (see docs)" << endl;
  cerr << "-d val    : weight neighbors as function of their distance:" 
       << endl;
  cerr << "     Z      : equal weights to all (default)" << endl;
  cerr << "     ID     : Inverse Distance" << endl;
  cerr << "     IL     : Inverse Linear" << endl;
  cerr << "     ED:a   : Exponential Decay with factor a (no whitespace!)"
       << endl;
  cerr << "     ED:a:b : Exponential Decay with factor a and b (no whitespace!)"
       << endl;
  cerr << "-k n      : k nearest neighbors (default n = 1)" << endl;
  cerr << "-q n      : TRIBL treshold at level n" << endl;
  cerr << "-L n      : MVDM treshold at level n" << endl;
  cerr << "-R n      : solve ties at random with seed n" << endl;
  cerr << "Input options:" << endl;
  cerr << "-f f      : read from Datafile 'f'" << endl;
  cerr << "-F format : Assume the specified inputformat" << endl;
  cerr << "            (Compact, C4.5, ARFF, Columns, Binary, Sparse )" 
       << endl;
  cerr << "-l n      : length of Features (Compact format only)" << endl;
  cerr << "-i f      : read the InstanceBase from file 'f' "
       << "(skips phase 1 & 2 )"
       << endl;
  cerr << "--matrixin=<f> read ValueDifference Matrices from file 'f'" << endl;
  cerr << "-u f      : read value_class probabilities from file 'f'" 
       << endl;
  cerr << "-P d      : read data using path 'd'" << endl;
  cerr << "-s        : use exemplar weights from the input file" << endl;
  cerr << "-s0       : silently ignore the exemplar weights from the input file" << endl;
  cerr << "-T n      : use input field 'n' as the target. (default is: the last field)" << endl;
  cerr << "Output options:" << endl;
  cerr << "--Beam=<n> : limit +v db output to n highest-vote classes" << endl;
  cerr << "-V        : Show VERSION" << endl;
  cerr << "+v or -v level : set or unset verbosity level, where level is"
       << endl;
  cerr << "      s:  work silently" << endl;
  cerr << "      o:  show all options set" << endl;
  cerr << "      b:  show node/branch count and branching factor" << endl;
  cerr << "      f:  show Calculated Feature Weights (default)" 
       << endl;
  cerr << "      p:  show Value Difference matrices" << endl;
  cerr << "      e:  show exact matches" << endl;
  cerr << "      as: show advanced statistics (memory consuming)" << endl;
  cerr << "      cm: show Confusion Matrix (implies +vas)" << endl;
  cerr << "      cs: show per Class Statistics (implies +vas)" << endl;
  cerr << "      di: add distance to output file" << endl;
  cerr << "      db: add distribution of best matched to output file" 
       << endl;
  cerr << "      md: add matching depth to output file." << endl;
  cerr << "      k:  add a summary for all k neigbors to output file"
       << " (sets -x)" << endl;
  cerr << "      n:  add nearest neigbors to output file (sets -x)"
       << endl;
  cerr << "  You may combine levels using '+' e.g. +v p+db or -v o+di"
       << endl;
  cerr << "-G        : normalize distibutions (+vdb option only)" << endl
       << "    O     : normalize between 0 and 1" << endl
       << "    1:<f> : add f to all possible targets" << endl
       << "            then normalize between 0 and 1 (default f=1.0)" << endl;
  cerr << "Server options" << endl;
  cerr << "-S <port> : run as a server on <port>" << endl;
  cerr << "--pidfile=<f> store pid in file <f>" << endl; 
  cerr << "--logfile=<f> log server activity in file <f>" << endl; 
  cerr << "--serverconfig=<f> read server settings from file <f>" << endl; 
  cerr << "Internal representation options:" << endl;
  cerr << "-B n      : number of bins used for discretization of numeric " 
       << "feature values" << endl;
  cerr << "-c n      : clipping frequency for prestoring MVDM matrices"
       << endl;
  cerr << "-M n      : size of MaxBests Array" << endl;
  cerr << "-N n      : Number of features (default " 
       << TimblAPI::Default_Max_Feats() << ")" << endl;
  cerr << "-T n      : ordering of the Tree :" << endl;
  cerr << "       DO: none" << endl;
  cerr << "       GRO: using GainRatio" << endl;
  cerr << "       IGO: using InformationGain" << endl;
  cerr << "       1/V: using 1/# of Vals" << endl;
  cerr << "       G/V: using GainRatio/# of Vals" << endl;
  cerr << "       I/V: using InfoGain/# of Vals" << endl;
  cerr << "       X2O: using X-square" << endl;
  cerr << "       X/V: using X-square/# of Vals" << endl;
  cerr << "       SVO: using Shared Variance" << endl;
  cerr << "       S/V: using Shared Variance/# of Vals" << endl;
  cerr << "       GxE: using GainRatio * SplitInfo" << endl;
  cerr << "       IxE: using InformationGain * SplitInfo" << endl;
  cerr << "       1/S: using 1/SplitInfo" << endl;
  cerr << "+x or -x  : Do or don't use the exact match shortcut " << endl
       << "            (IB1 and IB2 only, default is -x)"
       << endl;
}

inline void usage(void){
  cerr << "usage:  Timbl -f data-file {-t test-file}"
       << endl;
  cerr << "or see: Timbl -h" << endl;
  cerr << "        for all possible options" << endl;
  cerr << endl;
}

void get_command_lines( const string& value, list<string>& result ){
  result.clear();
  ifstream ind( value.c_str()+1 ); // skip @ 
  if ( ind.bad() ){
    cerr << "Problem reading command-lines from file '" 
	 << value << "'" << endl;
    throw( "command line failure" );
  }
  string Buf;
  while ( getline( ind, Buf ) ){
    if ( Buf.empty() )
      continue;
    result.push_back( Buf );
  }
}

string correct_path( const string& filename,
		     const string& path,
		     bool keep_origpath ){  
  // if filename contains pathinformation, it is replaced with path, except
  // when keep_origpath is true. 
  // if filename contains NO pathinformation, path is always appended.
  // of course we don't append if the filename is empty or just '-' !
  
  if ( path != "" && filename != "" && filename[0] != '-' ){
    bool add_slash = path[path.length()] != '/';
    string tmp;
    string::size_type pos = filename.rfind( '/' );
    if ( pos == string::npos ){
      tmp = path;
      if ( add_slash )
	tmp += "/";
      tmp += filename;
    }
    else { 
      tmp = path;
      if ( add_slash )
	tmp += "/";
      if ( !keep_origpath ){
	tmp += filename.substr( pos+1 );
      }
      else 
	tmp += filename;
    }
    return tmp;
  }
  else
    return filename;
}

void Preset_Values( TimblOpts& Opts ){
  bool mood;
  string value;
  if ( Opts.Find( 'h', value, mood ) ){
    usage_full();
    exit(1);
  }
  if ( Opts.Find( 'V', value, mood ) ){
    cerr << "TiMBL " << TimblAPI::VersionInfo( true ) << endl;
    exit(1);
  }
  if ( Opts.Find( 'a', value, mood ) ){
    // the user gave an algorithm
    if ( !string_to( value, algorithm ) ){
      cerr << "illegal -a value: " << value << endl;
      exit(1); // no chance to proceed
    }
    Opts.Delete( 'a' );
  }
  else
    algorithm = IB1; // general default
  Opts.Add( 'a', to_string( algorithm ), false );
  if ( Opts.Find( 'P', value, mood ) ){
    I_Path = value;
    Opts.Delete( 'P' );
  }
  if ( Opts.Find( 'f', value, mood ) ){
    dataFile = correct_path( value, I_Path, true );
    Opts.Delete( 'f' );
  }
  if ( Opts.Find( 'q', value, mood ) ){
    Q_value = value;
  }
  Opts.Add( 'v', "F", true );
  Opts.Add( 'v', "S", false );
  if ( Opts.Find( "serverconfig", value, mood ) ){
    ServerConfigFile = correct_path( value, I_Path, true );
    Opts.Delete( "serverconfig" );
    Do_Multi_Server = true;
  }
  if ( Opts.Find( 'S', value, mood ) ){
    if ( Do_Multi_Server ){
      cerr << "options -S conflicts with option --serverconfig" << endl;
      exit(3);
    }
    else {
      Do_Server = true;
      ServerPort = stringTo<int>( value );
      if ( ServerPort < 1 || ServerPort > 100000 ){
	cerr << "-S option, portnumber invalid: " << ServerPort << endl;
	exit(3);
      }
    }
  }
  else {
    if ( Do_Multi_Server ){
      Opts.Add( 'S', "0", true );
      // hack to signal GetOptClass that we are going into server mode
    }
  }
  if ( Opts.Find( 'C', value, mood ) ){
    if ( Do_Multi_Server ){
      cerr << "-C must be specified in the severconfigfile" << endl;
      exit(3);
    }
    if ( !Do_Server ){
      cerr << "-C option invalid without -S" << endl;
      exit(3);
    }
    Max_Connections = stringTo<int>( value );
    if ( Max_Connections < 1 || Max_Connections > 1000 ){
      cerr << "-C options, max number of connection invalid: " 
	   << Max_Connections << endl;
    }
    Opts.Delete( 'C' );
  }
  Weighting W = GR;
  // default Weighting = GainRatio
  if ( Opts.Find( 'w', value, mood ) ){
    // user specified weighting
    if ( !string_to( value, W ) )
      // no valid weight, hopefully a filename
      return;
    else
      // valid Weight, but maybe a number, so replace
      Opts.Delete( 'w' );
  }
  Opts.Add( 'w', to_string(W), false );
}

void Adjust_Default_Values( TimblOpts& Opts ){
  bool mood;
  string value;
  if ( !Opts.Find( 'm', value, mood ) ){
    Opts.Add( 'm', "O", false );
    // Default Metric = Overlap
  }
}

bool next_test( string& line ){
  bool result = false;
  line = "";
  if ( !ind_lines.empty() ){
    line = ind_lines.front();
    ind_lines.pop_front();
    result = true;
  }
  return result;
}

bool get_file_names( TimblOpts& Opts ){
  MatrixInFile = "";
  TreeInFile = "";
  WgtInFile = "";
  WgtType = UNKNOWN_W;
  ProbInFile = "";
  string value;
  bool mood;
  if ( Opts.Find( 'P', value, mood ) ||
       Opts.Find( 'f', value, mood ) ){
    cerr << "illegal option, value = " << value << endl;
    return false;
  }
  if ( Opts.Find( "matrixin", value, mood ) ){
    MatrixInFile = correct_path( value, I_Path, true );
    Opts.Delete( "matrixin" );
  }
  if ( Opts.Find( 'i', value, mood ) ){
    TreeInFile = correct_path( value, I_Path, true );
    Opts.Delete( 'i' );
  }
  if ( Opts.Find( 'u', value, mood ) ){
    if ( algorithm == IGTREE ){
      cerr << "-u option is useless for IGtree" << endl;
      return false;
    }
    ProbInFile = correct_path( value, I_Path, true );
    Opts.Delete( 'u' );
  }
  if ( Opts.Find( 'w', value, mood ) ){
    Weighting W;
    if ( !string_to( value, W ) ){
      // No valid weighting, so assume it also has a filename
      vector<string> parts;
      size_t num = split_at( value, parts, ":" );
      if ( num == 2 ){
	if ( !string_to( parts[1], W ) ){
	  cerr << "invalid weighting option: " << value << endl;
	  return false;
	}
	WgtInFile = correct_path( parts[0], I_Path, true );
	WgtType = W;
	Opts.Delete( 'w' );
      }
      else if ( num == 1 ){
	WgtInFile = correct_path( value, I_Path, true );
	Opts.Delete( 'w' );
      } 
      else {
	cerr << "invalid weighting option: " << value << endl;
	return false;
      }
    }
  }
  return true;
}

bool checkInputFile( const string& name ){
  if ( !name.empty() ){
    ifstream is( name.c_str() );
    if ( !is.good() ){
      cerr << "unable to find or use input file '" << name << "'" << endl;
      return false;
    }
  }
  return true;
}

int main(int argc, char *argv[]){
  try {
    struct tm *curtime;
    time_t Time;
    // Start.
    //
    cerr << "TiMBL Server" << TimblAPI::VersionInfo()
	 << " (c) ILK 1998 - 2010.\n" 
	 << "Tilburg Memory Based Learner\n"
	 << "Induction of Linguistic Knowledge Research Group, Tilburg University\n"
	 << "CLiPS Computational Linguistics Group, University of Antwerp" << endl;
    time(&Time);
    curtime = localtime(&Time);
    cerr << asctime(curtime) << endl;
    if ( argc <= 1 ){
      usage();
      return 1;
    }
    TimblOpts Opts( argc, argv );
    Preset_Values( Opts );
    Adjust_Default_Values( Opts );
    if ( !get_file_names( Opts ) )
      return 2;
    TimblServerAPI *Run = new TimblServerAPI( &Opts );
    if ( !Run->Valid() ){
      delete Run;
      usage();
      return 3;
    }
    if ( Do_Server ){
      // Special case:   running a classic Server
      if ( !checkInputFile( TreeInFile ) ||
	   !checkInputFile( dataFile ) ||
	   !checkInputFile( WgtInFile ) ||
	   !checkInputFile( MatrixInFile ) ||
	   !checkInputFile( ProbInFile ) ){
	delete Run;
	return 3;
      }
      if ( TreeInFile != "" ){
	if ( !Run->GetInstanceBase( TreeInFile ) ){
	  return 3;
	}
      }
      else {
	if ( !Run->Learn( dataFile ) ){
	  return 3;
	}
      }
      if ( WgtInFile != "" ) {
	Run->GetWeights( WgtInFile, WgtType );
      }
      if ( ProbInFile != "" )
	Run->GetArrays( ProbInFile );
      if ( MatrixInFile != "" ) {
	Run->GetMatrices( MatrixInFile );
      }
      if ( Run->StartServer( ServerPort, Max_Connections ) )
	return 0;
      else
	cerr << "starting a server failed" << endl;
    }
    else if ( Do_Multi_Server ){
      if ( !checkInputFile( ServerConfigFile ) ){
	delete Run;
	return 3;
      }
      if ( !Run->StartMultiServer( ServerConfigFile ) ){
	cerr << "starting a MultiServer failed" << endl;
      }
    }
    else {
    }
    return 0;
  }
  catch(std::bad_alloc){
    cerr << "ran out of memory somewhere" << endl;
    cerr << "Timbl terminated, Sorry for that" << endl;
  }
  catch(std::exception& e){
    cerr << "a standard exception was raised: '" << e.what() << "'" << endl;
    cerr << "Timbl terminated, Sorry for that" << endl; 
  }
  catch(std::string& what){
    cerr << "an exception was raised: '" << what << "'" << endl;
    cerr << "Timbl terminated, Sorry for that" << endl; 
  }
  catch(...){
    cerr << "some exception was raised" << endl;
    cerr << "Timbl terminated, Sorry for that" << endl; 
  }
}
