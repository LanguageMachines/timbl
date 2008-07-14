/*
  Copyright (c) 1998 - 2008
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

#include <algorithm>
#include <iostream>
#include <string>
#include <fstream>

#include "timbl/TimblAPI.h"

using namespace Timbl;

using std::ifstream;
using std::ofstream;
using std::ios;
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::istream;
using std::string;
using std::equal;
using std::getline;

#define MAX_EXP        10
#define MAX_PARAMS      256

bool nocase_cmp( char c1, char c2 ){
  return toupper(c1) == toupper(c2);
}

bool compare_nocase_n( const string& s1, const string& s2, size_t n ){
  if ( equal( s1.begin(), s1.begin()+n, s2.begin(), nocase_cmp ) ){
    return true;
  }
  else {
    return false;
  }
}

//
// here are the various scripting actions:
//
enum ActionType { UnknownAct, New, Free,         
		  Set,  Show, Classify,
		  Train, Test, Increment, Decrement,
		  Expand, Remove,
		  SaveTree, GetTree,      
		  SaveW, GetW,
		  Quit };
/*
  The following scripting commands are implemented:
  QUIT 
      stop all further actions.
  NEW name <algo>
      create an experiment with name 'name' of type 'algo'
      algo can be IB1, IB2, TRIBL or IGTREE. Default: IBL
  FREE name
      delete the experiment with name 'name'
  <name>.SET option value
      set option of experiment name to value
  <name>.SHOW OPTIONS
      show all posible options with their default and current
      values for experiment 'name'
  <name>.SHOW SETTINGS
      show all options with current settings of 'name'
  <name>.TRAIN file
      build an instancebase from file. 
  <name>.TEST file1 [file2]
      classify all lines from file1, write results to file2 or
      to file1.out if parameter file2 is not present
  <name>.EXPAND file1 
      increment the database with contents of file1
  <name>.REMOVE file1 
      decrement the database with contents of file1
  <name>.CLASSIFY line 
      classify this line
  <name>.ADD line 
      increment the database with line
  <name>.REM line 
      decrement the database with line
  <name>.SAVE file
      save the instancebase of experiment name to file.
  <name>.GET file
      get the instancebase for experiment name from file.
  <name>.SAVEW file
      save the current weights of experiment name to file.
  <name>.GETW file
      get new weights for experiment name from file.
 */

TimblAPI *experiments[MAX_EXP];
int exp_cnt = 0;


// the following functions implement a simple parser to parse the
// script file, recognize experiment names en actions to perform
// on those experiments

int fill_params( string *params, const string& line ){
  // chop line into a bunch of parameters.
  int i;
  for ( i=0; i < MAX_PARAMS; i++ )
    params[i] = "";
  i = 0;
  size_t len = line.length();
  if ( line[0] == '"' && line[len-1] == '"' ) {
    params[0] = string( line, 1, len-2 );
    return 1;
  }
  for ( size_t u_i = 0; u_i < len; u_i++) {
    if ( line[u_i] == ',' || line[u_i] == ' ' ){
      if ( params[i] != "" ) // Don't accept zero length strings !
	++i; 
      if ( i >= MAX_PARAMS )
	break;
    }
    else
      params[i] += line[u_i];
  } // u_i
  if ( i >= MAX_PARAMS ){
    cerr << "too many parameters!" << endl;
    return MAX_PARAMS+1;
  }
  if ( params[i] != "" ){ // last param ended at line end
    i++;
  }
  return i;
}

int lookup( const string& name ){
  // search for an experiment with the name 'name' in the list of
  // known experiments.
  int i;
  for ( i=0; i < exp_cnt; i++ ){
    if ( name == experiments[i]->ExpName() )
      return i;
  }
  return -1;
}

ActionType parse( const string& Buffer, int &pos, string *pars, int &len ){
  // here we parse lines of the script-file:
  // first we take the first part and see if it is a NEW or FREE
  // command which need special attention.
  // otherwise we asume it to be the name of an experiment.
  string Buf = compress( Buffer );
  len = 0;
  if ( compare_nocase_n( Buf, "NEW ", 4 ) ){
    len = fill_params( pars, Buf.substr(4) );
    if ( ( pos = lookup( pars[0] ) ) != -1 ){
      cerr << "you can't renew an experiment: " << Buf << endl;
      return UnknownAct;
    }
    return New;
  }
  else if ( compare_nocase_n( Buf, "FREE ", 5 ) ){
    len = fill_params( pars, Buf.substr(5) );
    if ( (pos = lookup( pars[0] ) ) == -1 ){
      cerr << "you can't free this unknown experiment: " << Buf << endl;
      return UnknownAct;
    }
    return Free;
  }
  else if ( compare_nocase_n( Buf, "QUIT", 4 ) ){
    return Quit;
  }
  else {
    string expname;
    string::size_type p = Buf.find( '.' );
    if ( p == string::npos ){
      cerr << "missing experiment reference!" << endl;
      return UnknownAct;
    }
    else{
      expname = Buf.substr(0, p );
      pos = lookup( expname ); // do we know it.
      if ( pos == -1 )
	return UnknownAct; // error
      Buf = Buf.substr( p+1 );
      // A well known experiment, so now we can see what we
      // must do.
      if ( compare_nocase_n( Buf, "SET ", 4 ) ){
	len = fill_params( pars, Buf.substr(4) );
	return Set;
      }
      else if ( compare_nocase_n( Buf, "SHOW ", 5 ) ){
	len = fill_params( pars, Buf.substr(5) );
	return Show;
      }
      else  if ( compare_nocase_n( Buf, "GET ", 4 ) ){
	len = fill_params( pars, Buf.substr(4) );
	return GetTree;
      }
      else if ( compare_nocase_n( Buf, "GETW ", 5 ) ){
	len = fill_params( pars, Buf.substr(5) );
	return GetW;
      }
      else if ( compare_nocase_n( Buf, "SAVE ", 5 ) ){
	len = fill_params( pars, Buf.substr(5) );
	return SaveTree;
      }
      else if ( compare_nocase_n( Buf, "SAVEW ", 6 ) ){
	len = fill_params( pars, Buf.substr(6) );
	return SaveW;
      }
      else if ( compare_nocase_n( Buf, "TRAIN ", 6 ) ){
	len = fill_params( pars, Buf.substr(6) );
	return Train;
      }
      else if ( compare_nocase_n( Buf, "EXPAND ", 7 ) ){
	len = fill_params( pars, Buf.substr(7) );
	return Expand;
      }
      else if ( compare_nocase_n( Buf, "REMOVE ", 7 ) ){
	len = fill_params( pars, Buf.substr(7) );
	return Remove;
      }
      else if ( compare_nocase_n( Buf, "TEST ", 5 ) ){
	len = fill_params( pars, Buf.substr(5) );
	return Test;
      }
      else if ( compare_nocase_n( Buf, "CLASSIFY ", 9 ) ){
	len = fill_params( pars, Buf.substr(9) );
	return Classify;
      }
      else if ( compare_nocase_n( Buf, "ADD ", 4 ) ){
	len = fill_params( pars, Buf.substr(4) );
	return Increment;
      }
      else if ( compare_nocase_n( Buf, "REM ", 4 ) ){
	len = fill_params( pars, Buf.substr(4) );
	return Decrement;
      }
      else
	return UnknownAct;
    }
  }
}

void one_command( istream &in_file, int &line_count ) {
  // the actual "engine"
  // get a line from in_file, parse it and take appropiate action
  // Most of the time by directly calling a MBL Class function.
  // of course some sanity checking is done here and there
  static string *params = NULL;
  int pos = -1, len;
  if ( params == 0 ){
    params = new string[MAX_PARAMS+1];
  } 
  string Buffer;
  getline( in_file, Buffer );
  line_count++;
  if ( Buffer == "" || Buffer[0] == '#' ){
    return;
  }
  cerr << "TSE script, executing line: " << line_count<< endl
       << "=== " << Buffer << endl;
  ActionType action = parse( Buffer, pos, params, len );
  if ( len >= MAX_PARAMS ){
    cerr << "Too many parameters, skipping....." << endl;
    return;
  }
  switch ( action ){
  case Quit:
    exit(1);
    break;
  case New: {
    if ( exp_cnt == MAX_EXP ){
      cerr << "To many different experiments in one run" << endl;
      exit(1);
    }
    if ( len == 0 ){
      cerr << " Wrong number of parameters for New" << endl;
      exit(1);
    }
    string cmnd; 
    if ( len == 1 ){
      cerr << "1 parameters " << params[0] << endl;
      cmnd = "-a IB1";
    }
    else {
      for ( int i=1; i < len; ++i )
	cmnd += params[i] + " ";
    }
    experiments[exp_cnt++] = new TimblAPI( cmnd, params[0] );
    cerr << "Created a new experiment: " 
	 << experiments[exp_cnt-1]->ExpName() << endl;
    break;
  }
  case Free:
    delete experiments[pos];
    exp_cnt--;
    for ( ; pos < exp_cnt; pos++ ){
      experiments[pos] = experiments[pos+1];
    }
    experiments[exp_cnt] = 0;
    break;
  case GetTree:
    if ( len == 0 )
      cerr << "missing filename to retrieve InstanceBase" << endl;
    else
      experiments[pos]->GetInstanceBase( params[0] );
    break;
  case SaveTree:
    if ( len == 0 ){
      params[0] = experiments[pos]->ExpName() + ".tree";
    }
    else
      experiments[pos]->WriteInstanceBase(params[0]);
    break;
  case GetW:
    if ( len == 0 ) {
      params[0] = experiments[pos]->ExpName() + ".weights";
    }
    else
      experiments[pos]->GetWeights(params[0]);
    break;
  case SaveW:
    if ( len == 0 ){
      params[0] = experiments[pos]->ExpName() + ".weights";
    }
    else
      experiments[pos]->SaveWeights(params[0]);
    break;
  case Show:
    if ( len != 1 )
      cerr << "missing information about WHAT to show" << endl;
    else {
      if ( compare_nocase( params[0], "OPTIONS" ) )
	experiments[pos]->ShowOptions( cerr );
      else if ( compare_nocase( params[0], "SETTING" ) )
	experiments[pos]->ShowSettings( cerr );
      else
	cerr << "don't know how to show '" << params[0] << "'" << endl;
    }
    break;
  case Train:
    if ( len == 1 )
      experiments[pos]->Learn(params[0]);
    else
      cerr << "missing filename for Train" << endl;
    break;
  case Expand:
    if ( len == 1 )
      experiments[pos]->Expand(params[0]);
    else
      cerr << "missing filename for Expand" << endl;
    break;
  case Remove:
    if ( len == 1 )
      experiments[pos]->Remove(params[0]);
    else
      cerr << "missing filename for Remove" << endl;
    break;
  case Test: {
    switch ( len ){
    case 0:
      cerr << "missing filename for Test" << endl;
      return;
      break;
    case 1:
      params[1] = params[0] + ".out";
      break;
    case 2:
      break;
    default:
      cerr << "too many parameters for Test, (ignored)" << endl;
    }
    experiments[pos]->Test( params[0], params[1] );
    break;
  }
  case Classify: 
    if ( len == 1 ){
      const TargetValue *tv = experiments[pos]->Classify(params[0]);
      cout << "classify: " << params[0] << " ==> " << tv << endl;
    }
    else
      cerr << "missing instancestring for Add" << endl;
    break;
  case Increment: 
    if ( len == 1 )
      experiments[pos]->Increment(params[0]);
    else
      cerr << "missing instancestring for Add" << endl;
    break;
  case Decrement: 
    if ( len == 1 )
      experiments[pos]->Decrement(params[0]);
    else
      cerr << "missing instancestring for Remove" << endl;
    break;
  case Set:
    if ( len != 1 ){
      for ( int j=1; j < len; j++ )
	params[0] += params[j];
    }
    if ( !experiments[pos]->SetOptions( params[0] ) )
      cerr << "problem with Set " << params[0] << endl;
    break;
  case UnknownAct:
    if ( pos < 0 )
      cerr << "[" << line_count << "]" << Buffer 
	   << "  ==> Unknown experiment, skipped\n" << endl; 
    else
      cerr << "[" << line_count << "] " << Buffer
	   << "  ==> Unknown action, skipped\n" << endl;
    break;
  }
}

int main(int argc, char *argv[] ){
  // the following trick makes it possible to parse lines from cin
  // as well from a user supplied file.
  istream *script_file;
  ifstream test_file;
  if ( argc > 1 ){
    if ( (test_file.open( argv[1], ios::in ), !test_file.good() ) ){
      cerr << argv[0] << " - couldn't open scriptfile " << argv[1] << endl;
      exit(1);
    }
    cout << "reading script from: " << argv[1] << endl;
    script_file = &test_file;
  }    
  else
    script_file = &cin;
  int line = 0;
  while ( !(*script_file).eof() )
    one_command( *script_file, line );
  exit(0);
}

