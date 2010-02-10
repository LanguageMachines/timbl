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

#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>

using namespace std;

#include <map>
#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/MsgClass.h"
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/Tree.h"
#include "timbl/IBtree.h"
#include "timbl/Instance.h"
#include "timbl/neighborSet.h"
#include "timbl/Statistics.h"
#include "timbl/BestArray.h"
#include "timbl/SocketBasics.h"
#include "timbl/MBLClass.h"
#include "timbl/GetOptClass.h"
#include "timbl/TimblExperiment.h"

using namespace std;
using namespace Timbl;
using namespace Sockets;

inline void usage( char *name ){
  cerr << "Timbl Client V0.10" << endl
       << "For demonstration purposes only!" << endl
       << "Usage:" << endl
       << name << " NodeName PortNumber [InputFile [OutputFile [BATCH]]]"
       << endl;
}

const int TCP_BUFFER_SIZE = 2048;     // length of Internet inputbuffers,
enum CodeType { UnknownCode, Result, Error, OK, Echo, Skip,
		Neighbors, EndNeighbors, Status, EndStatus };


bool check_for_neigbors( const string& line ){
  return line.find( "NEIGHBORS" ) != string::npos;
}

inline void Split( const string& line, string& com, string& rest ){
  string::const_iterator b_it = line.begin();
  while ( b_it != line.end() && isspace( *b_it ) ) ++b_it;
  string::const_iterator m_it = b_it;
  while ( m_it != line.end() && !isspace( *m_it ) ) ++m_it;
  com = string( b_it, m_it );
  while ( m_it != line.end() && isspace( *m_it) ) ++m_it;
  rest = string( m_it, line.end() );
}  

CodeType get_code( const string& com ){
  CodeType result = UnknownCode;
  if ( compare_nocase( com, "CATEGORY" ) )
    result = Result;
  else if ( compare_nocase( com, "ERROR" ) )
    result = Error;
  else if ( compare_nocase( com, "OK" ) )
    result = OK;
  else if ( compare_nocase( com, "AVAILABLE" ) )
    result = Echo;
  else if ( compare_nocase( com, "SELECTED" ) )
    result = Echo;
  else if ( compare_nocase( com, "SKIP" ) )
    result = Skip;
  else if ( compare_nocase( com, "NEIGHBORS" ) )
    result = Neighbors;
  else if ( compare_nocase( com, "ENDNEIGHBORS" ) )
    result = EndNeighbors;
  else if ( compare_nocase( com, "STATUS" ) )
    result = Status;
  else if ( compare_nocase( com, "ENDSTATUS" ) )
    result = EndStatus;
  return result;
}


void RunClient( istream& Input, ostream& Output, 
		const string& NODE, const string& TCP_PORT, 
		bool classify_mode, const string& base ){
  bool Stop_C_Flag = false;
  cout << "Starting Client on node:" << NODE << ", port:" 
       << TCP_PORT << endl;
  ClientSocket client;
  if ( client.connect(NODE, TCP_PORT) ){
    string TestLine, ResultLine;
    string Code, Rest;
    if ( client.read( ResultLine ) ){
      cout << ResultLine << endl;
      cout << "Start entering commands please:" << endl;
      if ( !base.empty() ){
	client.write( "base " + base + "\n" );
      }
      while( !Stop_C_Flag &&
	     getline( Input, TestLine ) ){ 
	if ( classify_mode )
	  client.write( "c " );
	if ( client.write( TestLine + "\n" ) ){
	repeat:
	  if ( client.read( ResultLine ) ){
	    if ( ResultLine == "" ) goto repeat;
	    Split( ResultLine, Code, Rest );
	    switch ( get_code( Code ) ){
	    case OK:
	      Output << "OK" << endl;
	      break;
	    case Echo:
	      Output << ResultLine << endl;
	      break;
	    case Skip:
	      Output << "Skipped " << Rest << endl;
	      break;
	    case Error:
	      Output << ResultLine << endl;
	      break;
	    case Result: {
	      bool also_neighbors = check_for_neigbors( ResultLine );
	      if ( classify_mode )
		Output << TestLine << " --> ";
	      Output << ResultLine << endl;
	      if ( also_neighbors )
		while ( client.read( ResultLine ) ){
		  Split( ResultLine, Code, Rest );
		  Output << ResultLine << endl;
		  if ( get_code( Code ) == EndNeighbors )
		    break;
		}
	      break;
	    }
	    case Status:
	      Output << ResultLine << endl;
	      while ( client.read( ResultLine ) ){
		Split( ResultLine, Code, Rest );
		Output << ResultLine << endl;
		if ( get_code( Code ) == EndStatus )
		  break;
	      }
	      break;
	    default:
	      Output << "Client is confused?? " << ResultLine << endl;
	      Output << "Code was '" << Code << "'" << endl;
	      break;
	    }
	  }
	  else
	    Stop_C_Flag = true;
	}
	else 
	  Stop_C_Flag = true;
      }
    }
  }
  else {
    cerr << "connection failed: " + client.getMessage() << endl;
  }
}

int main(int argc, char *argv[] ){
  // the following trick makes it possible to parse lines from cin
  // as well from a user supplied file.
  istream *Input = &cin;
  ostream *Output = &cout;
  ifstream input_file;
  ofstream output_file;
  bool c_mode = false;
  string base;
  if ( argc > 3 ){
    if ( (input_file.open( argv[3], ios::in ), !input_file.good() ) ){
      cerr << argv[0] << " - couldn't open inputfile " << argv[3] << endl;
      exit(1);
    }
    cout << "reading input from: " << argv[3] << endl;
    Input = &input_file;
    if ( argc > 4 ){
      if ( (output_file.open( argv[4], ios::out ), !output_file.good() ) ){
	cerr << argv[0] << " - couldn't open outputfile " << argv[4] << endl;
	exit(1);
      }
      cout << "writing output to: " << argv[4] << endl;
      Output = &output_file;
      if ( argc > 5 )
	c_mode = compare_nocase_n( "BATCH", argv[5] );
      if ( argc > 6 )
	base = argv[6];
    }
  }
  else if ( argc < 3 ){
    usage( argv[0] );
    exit(1);
  }
  RunClient( *Input, *Output, argv[1], argv[2], c_mode, base );
  exit(0);
}
