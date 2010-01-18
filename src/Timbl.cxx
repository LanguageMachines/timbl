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

using namespace std;
using namespace Timbl;

static list<string> ind_lines;

Algorithm algorithm;

bool Do_CV = false;
bool Do_LOO = false;
bool Do_NS = false;
bool Do_Indirect = false;
bool Do_Save_Perc = false;

string I_Path = "";
string O_Path = "";
string Q_value = "";
string dataFile = "";
string TestFile = "";
string OutputFile = "";
string PercFile = "";
string MatrixInFile = "";
string MatrixOutFile = "";
string TreeInFile = "";
string TreeOutFile = "";
string levelTreeOutFile = "";
int levelTreeLevel = 0;
string XOutFile = "";
string WgtInFile = "";
Weighting WgtType = UNKNOWN_W;
string WgtOutFile = "";
string ProbInFile = "";
string ProbOutFile = "";
string NamesFile = "";

inline void usage_full(void){
  cerr << "usage: Timbl -f data-file {-t test-file} [options]" << endl;
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
  cerr << "-t  f     : test using file 'f'" << endl;
  cerr << "-t leave_one_out:"
       << " test with Leave One Out,using IB1" << endl;
  cerr << " you may add -sloppy to speed up Leave One Out testing (see docs)" 
       << endl;
  cerr << "-t cross_validate:"
       << " Cross Validate Test,using IB1" << endl;
  cerr << "   @f     : test using files and options described in file 'f'" 
       << endl;
  cerr << "            Supported options: d e F k m o p q R t u v w x % -" 
       << endl;
  cerr << "            -t <file> is mandatory" << endl;
  cerr << "Input options:" << endl;
  cerr << "-f f      : read from Datafile 'f'" << endl;
  cerr << "-f f      : OR: use filenames from 'f' for CV test" << endl;
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
  cerr << "-e n      : estimate time until n patterns tested" << endl;
  cerr << "--Beam=<n> : limit +v db output to n highest-vote classes" << endl;
  cerr << "-I f      : dump the InstanceBase in file 'f'" << endl;
  cerr << "--matrixout=<f> store ValueDifference Matrices in file 'f'" << endl;
  cerr << "-X f      : dump the InstanceBase as XML in file 'f'" << endl;
  cerr << "-n f      : create names file 'f'" << endl;
  cerr << "-p n      : show progress every n lines (default p = 100,000)" 
       << endl;
  cerr << "-U f      : save value_class probabilities in file 'f'" << endl;
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
  cerr << "-W f      : calculate and save all Weights in file 'f'" << endl;
  cerr << "+% or -%  : do or don't save test result (%) to file" << endl;
  cerr << "-o s      : use s as output filename" << endl;
  cerr << "-O d      : save output using path 'd'" << endl;
  cerr << "Internal representation options:" << endl;
  cerr << "-B n      : number of bins used for discretization of numeric " 
       << "feature values" << endl;
  cerr << "-c n      : clipping frequency for prestoring MVDM matrices"
       << endl;
  cerr << "+D        : store distributions on all nodes" << endl
       << "            (necessary for using +v db with IGTree, but wastes memory otherwise)"
       << endl;
  cerr << "+H or -H  : write hashed trees (default +H)" << endl;
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
  if ( Opts.Find( 'S', value, mood ) ){
    cerr << "Server mode is no longer available in Timbl" << endl;
    cerr << "Please use the 'TimblServer' command instead." << endl;
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
  if ( Opts.Find( 'Z', value, mood ) ){
    // Special case
    //    spitting neigborSets only
    Do_NS = true;
    Opts.Delete( 'Z' );
  }
  if ( Opts.Find( 't', value, mood ) ){
    if ( value == "cross_validate" )
      // Special case
      //    running Cross Validation
      Do_CV = true;
    else if ( value == "leave_one_out" )
      // Special case
      //    running Leave_one_out
      Do_LOO = true;
    else if ( value != "" && value[0] == '@' ){
      Do_Indirect = true;
      get_command_lines( value, ind_lines );
      Opts.Delete( 't' );
    }
    if ( Do_LOO || Do_CV )
      if ( algorithm != IB1 ){
	cerr << "Invalid Algorithm: Only IB1 possible for LOO and CV " << endl;
	exit(1);
      }
  }
  if ( Opts.Find( 'P', value, mood ) ){
    I_Path = value;
    Opts.Delete( 'P' );
  }
  if ( Opts.Find( 'O', value, mood ) ){
    // output path is needed for CV testing
    O_Path = value;
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
  if ( Opts.Find( '%', value, mood ) ){
    Do_Save_Perc = true;
    Opts.Delete( '%' );
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
  TestFile = "";
  OutputFile = "";
  PercFile = "";
  MatrixInFile = "";
  MatrixOutFile = "";
  TreeInFile = "";
  TreeOutFile = "";
  levelTreeOutFile = "";
  levelTreeLevel = 0;
  XOutFile = "";
  WgtInFile = "";
  WgtType = UNKNOWN_W;
  WgtOutFile = "";
  ProbInFile = "";
  ProbOutFile = "";
  NamesFile = "";
  string value;
  bool mood;
  if ( Opts.Find( 'P', value, mood ) ||
       Opts.Find( 'f', value, mood ) ){
    cerr << "illegal option, value = " << value << endl;
    return false;
  }
  if ( Do_LOO ){
    if ( dataFile == "" ){
      cerr << "Missing datafile name for Leave One Out test" << endl;
      return false;
    }
    TestFile = dataFile;
  }
  else if ( Do_CV ){
    if ( dataFile == "" ){
      cerr << "Missing datafile name for Cross Validation test" << endl;
      return false;
    }
    TestFile = dataFile;
  }
  else if ( Opts.Find( 't', value, mood ) ){
    TestFile = correct_path( value, I_Path, true );
    Opts.Delete( 't' );
  }
  if ( Opts.Find( 'n', value, mood ) ){
    NamesFile = correct_path( value, O_Path, true );
    Opts.Delete( 'n' );
  }
  if ( Opts.Find( "matrixout", value, mood ) ){
    MatrixOutFile = correct_path( value, O_Path, true );
    Opts.Delete( "matrixout" );
  }
  if ( Opts.Find( "matrixin", value, mood ) ){
    MatrixInFile = correct_path( value, I_Path, true );
    Opts.Delete( "matrixin" );
  }
  if ( Opts.Find( 'o', value, mood ) ){
    if ( Do_CV ){
      cerr << "-o option not possible for Cross Validation testing" << endl;
      return false;
    }
    OutputFile = correct_path( value, O_Path, true );
    Opts.Delete( 'o' );
  }
  if ( Opts.Find( "IL", value, mood ) ){
    vector<string> vec;
    int num = split_at( value, vec, ":" );
    if ( num > 1 ){
      levelTreeOutFile = correct_path( vec[0], O_Path, true );
      levelTreeLevel = stringTo<int>( vec[1] );
    }
    else
      levelTreeOutFile = correct_path( value, O_Path, true );
    Opts.Delete( "IL" );
  }
  if ( Opts.Find( 'I', value, mood ) ){
    TreeOutFile = correct_path( value, O_Path, true );
    Opts.Delete( 'I' );
  }
  if ( Opts.Find( 'X', value, mood ) ){
    XOutFile = correct_path( value, O_Path, true );
    Opts.Delete( 'X' );
  }
  if ( Opts.Find( 'i', value, mood ) ){
    TreeInFile = correct_path( value, I_Path, true );
    Opts.Delete( 'i' );
  }
  if ( Opts.Find( 'U', value, mood ) ){
    ProbOutFile = correct_path( value, O_Path, true );
    Opts.Delete( 'U' );
  }
  if ( Opts.Find( 'u', value, mood ) ){
    if ( algorithm == IGTREE ){
      cerr << "-u option is useless for IGtree" << endl;
      return false;
    }
    ProbInFile = correct_path( value, I_Path, true );
    Opts.Delete( 'u' );
  }
  if ( Opts.Find( 'W', value, mood ) ){
    WgtOutFile = correct_path( value, O_Path, true );
    // leave the option, to signal that we need ALL feature weights
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

bool Default_Output_Names( TimblOpts& Opts ){
  string value;
  bool mood;
  if ( OutputFile == "" && TestFile != "" ){
    string temp = correct_path( TestFile, O_Path, false );
    temp += ".";
    switch ( algorithm ){
    case IB1:
      if ( Do_LOO )
	temp += "LOO";
      else if ( Do_CV )
	temp += "CV";
      else
	temp += "IB1";
      break;
    case IB2:
      temp +="IB2";
      break;
    case IGTREE:
      temp += "IGTree";
      break;
    case TRIBL:
      temp += "TRIBL";
      if ( Q_value != "" ){
	temp += "-";
	temp += Q_value;
      }
      else
	temp +=  "-0";
      break;
    case TRIBL2:
      temp +=  "TRIBL2";
      break;
    case LOO:
      temp +=  "LOO";
      break;
    case CV:
      temp +=  "CV";
      break;
    default:
      temp +=  "ERROR";
    }
    if ( algorithm != IGTREE ){
      temp +=  ".";
      if ( Opts.Find( 'm', value, mood ) )
	temp +=  value;
      else
	temp +=  "ErRoR";
      if ( Opts.Find( 'L', value, mood ) ){
	temp +=  ".L";
	temp +=  value;
      }
    }
    temp +=  ".";
    if ( Opts.Find( 'w', value, mood ) ){
      temp +=  value;
    }
    else 
      if ( !WgtInFile.empty() )
	temp += "ud";
      else
	temp += "gr";
    if ( algorithm != IGTREE ){
      if ( Opts.Find( 'k', value, mood ) ){
	temp +=  ".k";
	temp +=  value;
      }
      else
	temp +=  ".k1";
      if ( Opts.Find( 'd', value, mood ) ){
	temp +=  ".";
	temp +=  value;
      }
    }
    if ( Opts.Find( 'x', value, mood ) ){
      if ( mood ){
	temp +=  ".X";
      }
    }
    OutputFile = temp + ".out";
    if ( Do_Save_Perc ){
      PercFile = temp + ".%";
    }
  }
  else if ( OutputFile != "" ){
    if ( Do_Save_Perc ){
      PercFile = OutputFile;
      string::size_type pos = PercFile.rfind( '.' );
      if ( pos != string::npos )
	PercFile = PercFile.substr( 0, pos );
      PercFile += ".%";
    }
  }
  return true;
}

void Do_Test( TimblAPI *Run ){
  if ( WgtInFile != "" ) {
    Run->GetWeights( WgtInFile, WgtType );
  }
  if ( ind_lines.empty() ){
    // just one test...
    if ( ProbInFile != "" )
      Run->GetArrays( ProbInFile );
    if ( MatrixInFile != "" ) {
      Run->GetMatrices( MatrixInFile );
    }
    if ( Do_NS )
      Run->NS_Test( TestFile, OutputFile );
    else
      Run->Test( TestFile,
		 OutputFile,
		 PercFile );
  }
  else {
    // multiple tests from indirect file
    string tmp_line;
    while ( next_test( tmp_line) ){
      TimblOpts Opts( tmp_line );
      Adjust_Default_Values( Opts );
      if ( !get_file_names( Opts ) || TestFile == "" ){
	cerr << "Warning: Skipped a line from indirect testfile:\n'"	
	     << tmp_line << "'" << endl;
	if ( TestFile == "" )
	  cerr << "missing a Testfile name " << endl;
      }
      else if ( Run->SetIndirectOptions( Opts ) ){
	Default_Output_Names( Opts );
	if ( WgtInFile != "" ) {
	  if ( !Run->GetWeights( WgtInFile, WgtType ) )
	    continue;
	}
	if ( ProbInFile != "" )
	  Run->GetArrays( ProbInFile );
	if ( MatrixInFile != "" ) {
	  Run->GetMatrices( MatrixInFile );
	}
	if ( Do_NS )
	  Run->NS_Test( TestFile, OutputFile );
	else
	  Run->Test( TestFile,
		     OutputFile,
		     PercFile );
      }
      else
	cerr << "Warning: Skipped a line from indirect testfile:\n'"
	     << tmp_line << "'" << endl;
    }
  }
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

bool checkOutputFile( const string& name ){
  if ( !name.empty() ){
    ofstream os( name.c_str() );
    if ( !os.good() ) {
      cerr << "unable to find or use output file"  << name << "'" << endl;
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
    cerr << "TiMBL " << TimblAPI::VersionInfo()
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
    TimblAPI *Run = new TimblAPI( &Opts );
    if ( !Run->Valid() ){
      delete Run;
      usage();
      return 3;
    }
    Default_Output_Names( Opts );
    if ( Do_CV ){
      if ( checkInputFile( TestFile ) ){
	Run->CVprepare( WgtInFile, WgtType, ProbInFile );
	Run->Test( TestFile, "" );
      }
      delete Run;
    }
    else {
      bool do_test = false;
      if ( !checkInputFile( TreeInFile ) ||
	   !checkInputFile( dataFile ) ||
	   !checkInputFile( TestFile ) ||
	   !checkInputFile( WgtInFile ) ||
	   !checkInputFile( MatrixInFile ) ||
	   !checkInputFile( ProbInFile ) ||
	   !checkOutputFile( TreeOutFile ) ||
	   !checkOutputFile( levelTreeOutFile ) ||
	   !checkOutputFile( XOutFile ) ||
	   !checkOutputFile( NamesFile ) ||
	   !checkOutputFile( WgtOutFile ) ||
	   !checkOutputFile( MatrixOutFile ) ||
	   !checkOutputFile( ProbOutFile ) ){
	delete Run;
	return 3;
      }

      // normal cases....
      if ( TreeInFile == "" ){
	// normal case
	//   learning and maybe a testing phase
	if ( WgtOutFile != "" )
	  Run->SetOptions( "ALL_WEIGHTS: true" );
	if ( Run->Prepare( dataFile ) ){
	  if ( WgtOutFile != "" ) {
	    Run->SaveWeights( WgtOutFile );
	  }
	  // If we want to create a namesfile, do it here.
	  //
	  if ( NamesFile != "" ) {
	    Run->WriteNamesFile( NamesFile );
	  }
	  if ( ProbOutFile != "" )
	    Run->WriteArrays( ProbOutFile );
	  
	  do_test = TestFile != "" || Do_Indirect;
	  if ( do_test ||     // something to test ?
	       MatrixOutFile != "" || // or at least to produce
	       TreeOutFile != "" || // or at least to produce
	       levelTreeOutFile != "" || // or at least to produce
	       XOutFile != "" ){ // or at least to produce
	    if ( WgtInFile != "" ){
	      if ( Run->GetWeights( WgtInFile, WgtType ) ){
		cerr << "Calculated weights replaced by:" << endl;
		Run->ShowWeights( cerr );
	      }
	      else {
		cerr << "problems reading weights" << endl;
		do_test = false;
	      }
	    }
	    if ( Run->Learn( dataFile ) ){
	      if ( TreeOutFile != "" )
		Run->WriteInstanceBase( TreeOutFile );
	      if ( levelTreeOutFile != "" )
		Run->WriteInstanceBaseLevels( levelTreeOutFile, 
					      levelTreeLevel );
	      //	      if ( XOutFile != "" )
	      //		Run->WriteInstanceBaseXml( XOutFile );
	    }
	    else 
	      do_test = false; // no testing because of problems
	  }
	}
      }
      else  {
	// normal case
	//   running a testing phase from recovered tree
	do_test = Run->GetInstanceBase( TreeInFile );
      }
      if ( do_test ){
	Do_Test( Run );
      }
      if ( XOutFile != "" )
	Run->WriteInstanceBaseXml( XOutFile );
      if ( MatrixOutFile != "" ) {
	Run->WriteMatrices( MatrixOutFile );
      }
      delete Run;
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
