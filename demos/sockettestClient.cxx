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

#include <cstdlib>
#include <string>
#include <signal.h>
#include <iostream>
#include "timbl/Types.h"
#include "timbl/SocketBasics.h"

using namespace std;

int globalTimeOut;
//
// a simple program to demonstrate and test the Timbl Socket interface
//

const string lines[] = { "eerste regel", "tweede regel", "derde regel",
			 "laatste regel", "" };

bool runClient( const string& sock, int id ){
  cerr << "Starting Client " << id << " on localhost, port:" << sock << endl;
  Sockets::ClientSocket client;
  if ( client.connect( "localhost", sock) ){
    string resultLine;
    if ( client.read( resultLine ) ){
      cerr << "Client " << id << " read():\t\t\t\t" << resultLine << endl;
      int i = 0;
      string testLine = lines[i];
      while ( !testLine.empty() ){
	if ( client.write( testLine + "\n" ) ){
	  cerr << "Client " << id << " wrote():\t\t\t\t" << testLine << endl;
	  if ( client.read( resultLine ) ){
	    if ( resultLine == "" ) 
	      continue;
	    cerr << "Client " << id << " read() \t\t\t\t" << resultLine << endl;
	  }
	  else {
	    cerr << "read failed: "  + client.getMessage() << endl;
	    return false;
	  }
	}
	else {
	  cerr << "write failed: "  + client.getMessage() << endl;
	  return false;
	}
	testLine = lines[++i];
      }
      cerr << "all lines processed" << endl;
    }
    else {
      cerr << "connection failed: " + client.getMessage() << endl;
      return false;
    }
  }
  return true;
}

int randomSecs(){
  long int r = random();
  ldiv_t dif = ldiv( r, 5 );
  return abs(dif.rem);
}

bool runToClient( const string& sock, int id ){
  cerr << "Starting Client " << id << " on localhost, port:" << sock << endl;
  Sockets::ClientSocket client;
  if ( client.connect( "localhost", sock) ){
    client.setNonBlocking();
    string resultLine;
    int timeOut = globalTimeOut;
    int snorr = randomSecs();
    cerr << "client " << id << " sleeps " << snorr << " seconds" << endl;
    sleep( snorr);
    if ( client.read( resultLine, timeOut ) ){
      cerr << "Client " << id << " read():\t\t\t\t" << resultLine << endl;
      int i = 0;
      string testLine = lines[i];
      while ( !testLine.empty() ){
	int snorr = randomSecs();
	cerr << "client " << id << " sleeps " << snorr << " seconds" << endl;
	sleep( snorr);
	if ( client.write( testLine + "\n" ) ){
	  cerr << "Client " << id << " wrote():\t\t\t\t" << testLine << endl;
	  int snorr = randomSecs();
	  cerr << "client " << id << " sleeps " << snorr << " seconds" << endl;
	  sleep( snorr);
	  if ( client.read( resultLine, timeOut ) ){
	    if ( resultLine == "" ) 
	      continue;
	    cerr << "Client " << id << " read() \t\t\t\t" << resultLine << endl;
	  }
	  else {
	    cerr << "read failed: "  + client.getMessage() << endl;
	    return false;
	  }
	}
	else {
	  cerr << "write failed: "  + client.getMessage() << endl;
	  return false;
	}
	testLine = lines[++i];
      }
      cerr << "all lines processed" << endl;
    }
    else {
      cerr << "connection failed: " + client.getMessage() << endl;
      return false;
    }
  }
  return true;
}

int main( int argc, const char *argv[] ){
  string port = "1234";
  string tos = "0";
  if ( argc > 1 )
    port = argv[1];
  if ( argc > 2 )
    tos = argv[2];
  int timeOut;
  if ( !Timbl::stringTo<int>( tos, timeOut ) ){
    cerr << "invalid timeout" << endl;
    cerr << "usage: " << argv[0] << " <port> <timeout>" << endl;
    return 1;
  }
  else
    globalTimeOut = timeOut;
  int i;
#pragma omp parallel for
  for ( i=0; i < 6; ++i ){
    cerr << "creating client: " << i+1 << endl;
    if ( globalTimeOut > 0 ){
      if ( !runToClient( port, i+1 ) )
	cerr << "runClient failed" << endl;
    }
    else if ( !runClient( port, i+1 ) )
      cerr << "runClient failed" << endl;
    cerr << "Client "<< i+1 << " Done" << endl;
  }
  cerr << "clientTest done" << endl;
  return 0;
}
