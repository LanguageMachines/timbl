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
#include <signal.h>
#include <iostream>
#include "timbl/SocketBasics.h"

using namespace std;

//
// a simple program to demonstrate and test the Timbl Socket interface
//

// ***** This is the routine that is executed from a new thread *******
void *do_child( void *arg ){
  Sockets::Socket *mysock = (Sockets::Socket*)arg;
  // Greeting message for the client
  //
  //
  mysock->write( "Welcome to the Timbl socket tester.\n" );
  // process the test material
  //
  char line[256];
  sprintf( line, "Server: Thread %u, on Socket %d", 
	   (unsigned int)pthread_self(),
	   mysock->getSockId() );
  cerr << line << ", started" << endl;
  string buf;
  while( mysock->read( buf ) ){
    cerr << "Server: read()" << buf << endl;
    string answer = string( "echo: " ) + buf + "\n"; 
    mysock->write( answer );
    cerr << "Server: wrote()" << answer << endl;
  }
  delete mysock;
  return NULL;
}


bool startServer( const string& portString ){
  Sockets::ServerSocket server;
  pthread_attr_t attr;
  if ( pthread_attr_init(&attr) ||
       pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED ) ){
    cerr  << "Threads: couldn't set attributes" << endl;
    return false;
  }
  if ( !server.connect( portString ) ){
    cerr << "failed to start Server: " << server.getMessage() << endl;
    return false;
  }
  
  if ( !server.listen( 5 ) ) {
    // maximum of 5 pending requests
    cerr << server.getMessage() << endl;
    return false;
  }
  
  cerr << "Started Server on port:" << portString << endl;
  int failcount = 0;
  while( true ){ // waiting for connections loop
    signal( SIGPIPE, SIG_IGN );
    Sockets::ServerSocket *newSocket = new Sockets::ServerSocket();
    if ( !server.accept( *newSocket ) ){
      cerr << server.getMessage() << endl;
      if ( ++failcount > 20 ){
	cerr << "accept failcount >20 " << endl;
	cerr << "server stopped." << endl;
	return false;
	}
      else {
	continue;  
      }
    }
    else {
      failcount = 0;
      cerr  << "Server: Accepting Connection #" << newSocket->getSockId()
	    << " from remote host: " << newSocket->getClientName() << endl;
      // create a new thread to process the incoming request 
      // (The thread will terminate itself when done processing
      // and release its socket handle)
      //
      pthread_t chld_thr;
      pthread_create( &chld_thr, &attr, do_child, (void *)newSocket );
    }
    // the server is now free to accept another socket request 
  }
  pthread_attr_destroy(&attr); 
  return true;
}

int main(){
  if ( !startServer("1234") ){
    cerr << "creating server failed" << endl;
  }
  cerr << "Server DONE" << endl;
  return 0;
}
