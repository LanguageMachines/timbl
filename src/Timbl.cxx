/*
  Copyright (c) 1998 - 2022
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

#include <exception>
#include <list>
#include <vector>
#include <iosfwd>
#include <string>
#include <fstream>

#include "config.h"
#include "ticcutils/CommandLine.h"
#include "ticcutils/Timer.h"
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
bool Do_Limit = false;
size_t limit_val = 0;

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
  cerr << "usage: timbl -f data-file {-t test-file} [options]" << endl;
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
  cerr << "     E: Euclidean Distance" << endl;
  cerr << "     M: Modified value difference" << endl;
  cerr << "     J: Jeffrey Divergence" << endl;
  cerr << "     S: Jensen-Shannon Divergence" << endl;
  cerr << "     N: numeric values" << endl;
  cerr << "     I: Ignore named  values" << endl;
  cerr << "-w n      : Weighting" << endl;
  cerr << "     0 or nw: No Weighting" << endl;
  cerr << "     1 or gr: Weight using GainRatio (default)" << endl;
  cerr << "     2 or ig: Weight using InfoGain" << endl;
  cerr << "     3 or x2: Weight using Chi-square" << endl;
  cerr << "     4 or sv: Weight using Shared Variance" << endl;
  cerr << "     5 or sd: Weight using Standard Deviation. (all features must be numeric)" << endl;
  cerr << "-w f      : read Weights from file 'f'" << endl;
  cerr << "-w f:n    : read Weight n from file 'f'" << endl;
  cerr << "-b n      : number of lines used for bootstrapping (IB2 only)"
       << endl;
#ifdef HAVE_OPENMP
  cerr << "--clones=<num> : use 'n' threads for parallel testing" << endl;
#endif
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
  cerr << "-q n      : TRIBL threshold at level n" << endl;
  cerr << "-L n      : MVDM threshold at level n" << endl;
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
  cerr << "            (Compact, C4.5, ARFF, Columns, Tabbed, Binary, Sparse )"
       << endl;
  cerr << "-l n      : length of Features (Compact format only)" << endl;
  cerr << "-i f      : read the InstanceBase from file 'f' "
       << "(skips phase 1 & 2 )"
       << endl;
  cerr << "--matrixin=<f> read ValueDifference Matrices from file 'f'" << endl;
  cerr << "-u f      : read value_class probabilities from file 'f'"
       << endl;
  cerr << "--occurrences=train|test|both assume occurrence info in the files."
       << endl;
  cerr << "             (train: in the train file, test: in the test file, both: in both)" << endl;
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
  cerr << "-V or --version : Show VERSION" << endl;
  cerr << "+v or -v level  : set or unset verbosity level, where level is"
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
  cerr << "      cf: add confidence to the output file. (needs -G)" << endl;
  cerr << "      di: add distance to output file" << endl;
  cerr << "      db: add distribution of best matched to output file"
       << endl;
  cerr << "      md: add matching depth to output file." << endl;
  cerr << "      k:  add a summary for all k neighbors to output file"
       << " (sets -x)" << endl;
  cerr << "      n:  add nearest neighbors to output file (sets -x)"
       << endl;
  cerr << "  You may combine levels using '+' e.g. +v p+db or -v o+di"
       << endl;
  cerr << "-G        : normalize distibutions (+vdb option only)" << endl
       << "    Probability    : normalize between 0 and 1" << endl
       << "             0     : does the same " << endl
       << "    addFactor:<f>  : add f to all possible targets" << endl
       << "                     then normalize between 0 and 1 (default f=1.0)" << endl
       << "             1:<f> : does the same" << endl
       << "    logProbability : Add 1 to the target Weight, take the 10Log and" << endl
       << "                     then normalize between 0 and 1." << endl
       << "             2     : does the same" << endl;
  cerr << "-W f      : calculate and save all Weights in file 'f'" << endl;
  cerr << "+% or -%  : do or don't save test result (%) to file" << endl;
  cerr << "-o s      : use s as output filename" << endl;
  cerr << "-O d      : save output using path 'd'" << endl;
  cerr << "Internal representation options:" << endl;
  cerr << "-B n      : number of bins used for discretization of numeric "
       << "feature values (default B=20)" << endl;
  cerr << "-c n      : clipping frequency for prestoring MVDM matrices"
       << endl;
  cerr << "+D        : store distributions on all nodes" << endl
       << "            (necessary for using +v db with IGTree, but wastes memory otherwise)"
       << endl;
  cerr << "+H or -H  : write hashed trees (default +H)" << endl;
  cerr << "-M n      : size of MaxBests Array" << endl;
  cerr << "-N n      : Number of features (default "
       << TimblAPI::Default_Max_Feats() << ")" << endl;
  cerr << "--limit l : limit the number of features used to the 'l' with the highest weights." << endl;
  cerr << "            (will restart Timbl with an adapted -m option)" << endl;
  cerr << "--Treeorder=<value>      : ordering of the Tree :" << endl;
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
  cerr << "       SDO: using Standard Deviation" << endl;
  cerr << "       SD/V: using Standard Deviation/# of Vals" << endl;
  cerr << "       GxE: using GainRatio * SplitInfo" << endl;
  cerr << "       IxE: using InformationGain * SplitInfo" << endl;
  cerr << "       1/S: using 1/SplitInfo" << endl;
  cerr << "+x or -x  : Do or don't use the exact match shortcut " << endl
       << "            (IB1 and IB2 only, default is -x)"
       << endl;
}

inline void usage(void){
  cerr << "usage:  timbl -f data-file {-t test-file}"
       << endl;
  cerr << "or see: timbl -h" << endl;
  cerr << "        for all possible options" << endl;
  cerr << endl;
}

void get_command_lines( const string& value, list<string>& result ){
  result.clear();
  ifstream ind( value.substr(1) ); // skip @
  if ( ind.bad() ){
    cerr << "Problem reading command-lines from file '"
	 << value << "'" << endl;
    throw( "command line failure" );
  }
  string Buf;
  while ( getline( ind, Buf ) ){
    if ( Buf.empty() ){
      continue;
    }
    result.push_back( Buf );
  }
}

class softExit : public exception {};
class hardExit : public exception {};

void Preset_Values( TiCC::CL_Options& opts ){
  string value;
  if ( opts.is_present( 'h' )
       || opts.is_present( "help" ) ){
    usage_full();
    throw( softExit() );
  }
  if ( opts.is_present( 'V' )
       || opts.is_present( "version" ) ){
    cerr << "TiMBL " << Timbl::BuildInfo() << endl;
    throw( softExit() );
  }
  if ( opts.is_present( 'S' ) ){
    cerr << "Server mode is no longer available in timbl" << endl;
    cerr << "Please use the 'timblserver' command instead." << endl;
    throw( hardExit() );
  }
  if ( opts.extract( 'a', value ) ){
    // the user gave an algorithm
    if ( !string_to( value, algorithm ) ){
      cerr << "illegal -a value: " << value << endl;
      throw( hardExit() ); // no chance to proceed
    }
  }
  else {
    algorithm = IB1; // general default
  }
  opts.insert( 'a', to_string( algorithm ), false );
  if ( opts.extract( 'Z', value ) ){
    // Special case
    //    spitting neighborSets only
    Do_NS = true;
  }
  if ( opts.is_present( 't', value ) ){
    if ( value == "cross_validate" ){
      // Special case
      //    running Cross Validation
      Do_CV = true;
    }
    else if ( value == "leave_one_out" ){
      // Special case
      //    running Leave_one_out
      Do_LOO = true;
    }
    else if ( value != "" && value[0] == '@' ){
      Do_Indirect = true;
      opts.remove( 't' );
      get_command_lines( value, ind_lines );
    }
    if ( Do_LOO || Do_CV ){
      if ( algorithm != IB1 ){
	cerr << "Invalid Algorithm: Only IB1 possible for LOO and CV " << endl;
	throw( hardExit() ); // no chance to proceed
      }
    }
  }
  if ( opts.extract( "limit", value ) ){
    Do_Limit = true;
    if ( !TiCC::stringTo<size_t>( value, limit_val ) ){
      cerr << "illegal --limit value: " << value << endl;
      throw( hardExit() ); // no chance to proceed
    }
    if ( Do_CV ){
      cerr << "--limit is not implemented for --cross-validation" << endl;
      throw( hardExit() ); // no chance to proceed
    }
  }
  if ( opts.extract( 'P', value ) ){
    I_Path = value;
  }
  if ( opts.is_present( 'O', value ) ){
    // output path is needed for CV testing
    O_Path = value;
  }
  if ( opts.extract( 'f', value ) ){
    dataFile = correct_path( value, I_Path );
  }
  opts.is_present( 'q', Q_value );
  opts.insert( 'v', "F", true );
  opts.insert( 'v', "S", false );
  Weighting W = GR;
  // default Weighting = GainRatio
  if ( opts.is_present( 'w', value ) ){
    // user specified weighting
    if ( !string_to( value, W ) ){
      // no valid weight, hopefully a filename
      return;
    }
    else {
      // valid Weight, but maybe a number, so replace
      opts.remove( 'w' );
    }
  }
  opts.insert( 'w', to_string(W), false );
}

void Adjust_Default_Values( TiCC::CL_Options& opts ){
  if ( !opts.is_present( 'm' ) ){
    opts.insert( 'm', "O", false );
    // Default Metric = Overlap
  }
  if ( opts.extract( '%' ) ){
    Do_Save_Perc = true;
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

bool get_file_names( TiCC::CL_Options& opts ){
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
  if ( opts.extract( 'P', value ) || opts.extract( 'f', value ) ){
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
  else if ( opts.extract( 't', value ) ){
    TestFile = correct_path( value, I_Path );
  }
  if ( opts.extract( 'n', value ) ){
    NamesFile = correct_path( value, O_Path );
  }
  if ( opts.extract( "matrixout", value ) ){
    MatrixOutFile = correct_path( value, O_Path );
  }
  if ( opts.extract( "matrixin", value ) ){
    MatrixInFile = correct_path( value, I_Path );
  }
  if ( opts.extract( 'o', value ) ){
    if ( Do_CV ){
      cerr << "-o option not possible for Cross Validation testing" << endl;
      return false;
    }
    OutputFile = correct_path( value, O_Path );
  }
  if ( opts.extract( "IL", value ) ){
    vector<string> vec = TiCC::split_at( value, ":" );
    if ( vec.size() > 1 ){
      levelTreeOutFile = correct_path( vec[0], O_Path );
      levelTreeLevel = TiCC::stringTo<int>( vec[1] );
    }
    else {
      levelTreeOutFile = correct_path( value, O_Path );
    }
  }
  if ( opts.extract( 'I', value ) ){
    TreeOutFile = correct_path( value, O_Path );
  }
  if ( opts.extract( 'X', value ) ){
    XOutFile = correct_path( value, O_Path );
  }
  if ( opts.extract( 'i', value ) ){
    TreeInFile = correct_path( value, I_Path );
  }
  if ( opts.extract( 'U', value ) ){
    ProbOutFile = correct_path( value, O_Path );
  }
  if ( opts.extract( 'u', value ) ){
    if ( algorithm == IGTREE ){
      cerr << "-u option is useless for IGtree" << endl;
      return false;
    }
    ProbInFile = correct_path( value, I_Path );
  }
  if ( opts.is_present( 'W', value ) ){
    WgtOutFile = correct_path( value, O_Path );
    // leave the option, to signal that we need ALL feature weights
  }
  if ( opts.is_present( 'w', value ) ){
    Weighting W;
    if ( !string_to( value, W ) ){
      // No valid weighting, so assume it also has a filename
      vector<string> parts = TiCC::split_at( value, ":" );
      size_t num = parts.size();
      if ( num == 2 ){
	if ( !string_to( parts[1], W ) ){
	  cerr << "invalid weighting option: " << value << endl;
	  return false;
	}
	WgtInFile = correct_path( parts[0], I_Path );
	WgtType = W;
	opts.remove( 'w' );
      }
      else if ( num == 1 ){
	WgtInFile = correct_path( value, I_Path );
	opts.remove( 'w' );
      }
      else {
	cerr << "invalid weighting option: " << value << endl;
	return false;
      }
    }
  }
  return true;
}

bool Default_Output_Names( TiCC::CL_Options& opts ){
  if ( OutputFile == "" && TestFile != "" ){
    string value;
    string temp = correct_path( TestFile, O_Path, false );
    temp += ".";
    switch ( algorithm ){
    case IB1:
      if ( Do_LOO ){
	temp += "LOO";
      }
      else if ( Do_CV ){
	temp += "CV";
      }
      else {
	temp += "IB1";
      }
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
      else {
	temp +=  "-0";
      }
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
      if ( opts.is_present( 'm', value ) ){
	temp += value;
      }
      else {
	temp +=  "ErRoR";
      }
      if ( opts.is_present( 'L', value ) ){
	temp += ".L";
	temp += value;
      }
    }
    temp +=  ".";
    if ( opts.is_present( 'w', value ) ){
      temp += value;
    }
    else if ( !WgtInFile.empty() ){
      temp += "ud";
    }
    else {
      temp += "gr";
    }
    if ( algorithm != IGTREE ){
      if ( opts.is_present( 'k', value ) ){
	temp +=  ".k";
	temp +=  value;
      }
      else {
	temp +=  ".k1";
      }
      if ( opts.is_present( 'd', value ) ){
	temp +=  ".";
	temp +=  value;
      }
    }
    bool mood;
    if ( opts.is_present( 'x', value, mood ) ){
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
      if ( pos != string::npos ){
	PercFile.resize( pos );
      }
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
    if ( ProbInFile != "" ){
      Run->GetArrays( ProbInFile );
    }
    if ( MatrixInFile != "" ) {
      Run->GetMatrices( MatrixInFile );
    }
    if ( Do_NS ) {
      Run->NS_Test( TestFile, OutputFile );
    }
    else {
      Run->Test( TestFile,
		 OutputFile,
		 PercFile );
    }
  }
  else {
    // multiple tests from indirect file
    string tmp_line;
    while ( next_test( tmp_line) ){
      TiCC::CL_Options opts( timbl_indirect_opts, "" );
      try {
	opts.init( tmp_line );
	Adjust_Default_Values( opts );
      }
      catch ( TiCC::OptionError& e ){
	cerr << e.what() << endl;
	cerr << "Warning: Skipped a line from indirect testfile:\n'"
	     << tmp_line << "'" << endl;
	continue;
      }
      if ( !get_file_names( opts ) || TestFile == "" ){
	cerr << "Warning: Skipped a line from indirect testfile:\n'"
	     << tmp_line << "'" << endl;
	if ( TestFile == "" ){
	  cerr << "missing a Testfile name (-t option)" << endl;
	}
      }
      else if ( Run->SetIndirectOptions( opts ) ){
	Default_Output_Names( opts );
	if ( WgtInFile != "" ) {
	  if ( !Run->GetWeights( WgtInFile, WgtType ) ){
	    continue;
	  }
	}
	if ( ProbInFile != "" ){
	  Run->GetArrays( ProbInFile );
	}
	if ( MatrixInFile != "" ) {
	  Run->GetMatrices( MatrixInFile );
	}
	if ( Do_NS ){
	  Run->NS_Test( TestFile, OutputFile );
	}
	else {
	  Run->Test( TestFile,
		     OutputFile,
		     PercFile );
	}
      }
      else {
	cerr << "Warning: Skipped a line from indirect testfile:\n'"
	     << tmp_line << "'" << endl;
      }
    }
  }
}

bool checkInputFile( const string& name ){
  if ( !name.empty() ){
    ifstream is( name );
    if ( !is.good() ){
      cerr << "unable to find or use input file '" << name << "'" << endl;
      return false;
    }
  }
  return true;
}

bool checkOutputFile( const string& name ){
  if ( !name.empty() ){
    ofstream os( name );
    if ( !os.good() ) {
      cerr << "unable to find or use output file"  << name << "'" << endl;
      return false;
    }
  }
  return true;
}

int main(int argc, char *argv[]){
  try {
    // Start.
    //
    cerr << "TiMBL " << TimblAPI::VersionInfo()
	 << " (c) CLST/ILK/CLIPS 1998 - 2022.\n"
	 << "Tilburg Memory Based Learner\n"
	 << "Centre for Language and Speech Technology, Radboud University\n"
	 << "Induction of Linguistic Knowledge Research Group, Tilburg University\n"
	 << "CLiPS Computational Linguistics Group, University of Antwerp" << endl;
    cerr << TiCC::Timer::now() << endl << endl;
    if ( argc <= 1 ){
      usage();
      return 1;
    }
    TiCC::CL_Options opts( timbl_short_opts, timbl_long_opts );
    try {
      opts.init( argc, argv );
    }
    catch ( TiCC::OptionError& e ){
      cerr << e.what() << endl;
      usage();
      return 666;
    }
    Preset_Values( opts );
    Adjust_Default_Values( opts );
    if ( !get_file_names( opts ) ){
      return 2;
    }
    TimblAPI *Run = new TimblAPI( opts );
    if ( !Run->isValid() ){
      delete Run;
      usage();
      return 3;
    }
    Default_Output_Names( opts );
    vector<string> mas = opts.getMassOpts();
    if ( !mas.empty() ){
      cerr << "unknown value in option string: " << mas[0] << endl;
      usage();
      return 33;
    }
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
	if ( WgtOutFile != "" ) {
	  Run->SetOptions( "ALL_WEIGHTS: true" );
	}
	if ( Run->Prepare( dataFile ) ){
	  if ( Do_Limit ){
	    if ( Run->NumOfFeatures() < limit_val ){
	      cerr << "value of --limit is larger then the number of features!"
		   << endl;
	      return 32;
	    }
	    string m_val = Run->extract_limited_m( limit_val );
	    //	    cerr << endl << endl << "NEW M: " << m_val << endl << endl;
	    opts.extract( 'm' );
	    opts.insert( 'm', m_val, true );
	    cerr << "\t--limit=" << limit_val << " is specified, so we retrain "
		 << "the data with option: -m" << m_val << endl;
	    delete Run;
	    Run = new TimblAPI( opts );
	  }
	  if ( WgtOutFile != "" ) {
	    Run->SaveWeights( WgtOutFile );
	  }
	  // If we want to create a namesfile, do it here.
	  //
	  if ( NamesFile != "" ) {
	    Run->WriteNamesFile( NamesFile );
	  }
	  if ( ProbOutFile != "" ){
	    Run->WriteArrays( ProbOutFile );
	  }
	  do_test = TestFile != "" || Do_Indirect;
	  if ( do_test ||     // something to test ?
	       MatrixOutFile != "" || // or at least to produce
	       TreeOutFile != "" || // or at least to produce
	       levelTreeOutFile != "" || // or at least to produce
	       XOutFile != "" ){ // or at least to produce
	    bool ok = true;
	    if ( WgtInFile != "" ){
	      if ( Run->GetWeights( WgtInFile, WgtType ) ){
		cerr << "Calculated weights replaced by:" << endl;
		Run->ShowWeights( cerr );
	      }
	      else {
		cerr << "problems reading weights" << endl;
		ok = do_test = false;
	      }
	    }
	    if ( ok && Run->Learn( dataFile ) ){
	      if ( TreeOutFile != "" ){
		Run->WriteInstanceBase( TreeOutFile );
	      }
	      if ( levelTreeOutFile != "" ){
		Run->WriteInstanceBaseLevels( levelTreeOutFile,
					      levelTreeLevel );
	      }
	    }
	    else {
	      do_test = false; // no testing because of problems
	    }
	  }
	}
      }
      else if ( !dataFile.empty() &&
		!( TestFile.empty() && TreeOutFile.empty() && levelTreeOutFile.empty() ) ){
	// it seems we want to expand our tree
	do_test = false;
	if ( Run->GetInstanceBase( TreeInFile ) ) {
	  if ( Run->Expand( dataFile ) ){
	    if ( !TreeOutFile.empty() ){
	      Run->WriteInstanceBase( TreeOutFile );
	    }
	    if ( levelTreeOutFile != "" ){
	      Run->WriteInstanceBaseLevels( levelTreeOutFile,
					    levelTreeLevel );
	    }
	    do_test = !TestFile.empty();
	  }
	}
      }
      else {
	// normal case
	//   running a testing phase from recovered tree
	if ( TestFile.empty()
	     && XOutFile == ""
	     && !Do_Indirect ){
	  cerr << "reading an instancebase(-i option) without a testfile (-t option) is useless" << endl;
	  do_test = false;
	}
	else {
	  do_test = true;
	}
	if ( do_test ){
	  do_test = Run->GetInstanceBase( TreeInFile );
	}
      }
      if ( do_test ){
	Do_Test( Run );
      }
      if ( Run->isValid() ) {
	if ( XOutFile != "" ){
	  Run->WriteInstanceBaseXml( XOutFile );
	}
	if ( MatrixOutFile != "" ) {
	  Run->WriteMatrices( MatrixOutFile );
	}
      }
      if ( !do_test || !Run->isValid() ){
	delete Run;
	return EXIT_FAILURE;
      }
      delete Run;
    }
    return EXIT_SUCCESS;
  }
  catch( const softExit& e ){
    return EXIT_SUCCESS;
  }
  catch( const std::string& what ){
    cerr << what << ", sorry" << endl;
  }
  catch( const std::bad_alloc&){
    cerr << "ran out of memory somewhere" << endl;
    cerr << "timbl terminated, Sorry for that" << endl;
  }
  catch( const std::exception& e ){
    cerr << e.what() << ", sorry" << endl;
  }
  return EXIT_FAILURE;
}
