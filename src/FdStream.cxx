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

#include <cstring>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <iostream>
#include <streambuf>
#include <unistd.h>
#include "timbl/Common.h"
#include "timbl/FdStream.h"

using namespace std;

int fdoutbuf::overflow( int c ){
  if ( c != EOF ){
    char z = c;
    if ( write( fd, &z, 1 ) != 1 ) {
      return EOF;
    }
  }
  return c;
}

std::streamsize fdoutbuf::xsputn( const char *s, std::streamsize num ){
  return write( fd, s, num );
}


fdinbuf::fdinbuf( int _fd ): fd(_fd){ 
  setg( buffer + putbackSize, 
	buffer + putbackSize, 
	buffer + putbackSize );
}

int fdinbuf::underflow(){
  if ( gptr() < egptr() ){
    return traits_type::to_int_type(*gptr());
  }
  int numPutBack = gptr() - eback();
  if ( numPutBack > putbackSize ) {
    numPutBack = putbackSize;
  }
  
  std::memmove( buffer + putbackSize - numPutBack, 
	       gptr() - numPutBack,
	       numPutBack );
  int num = read( fd, buffer+putbackSize, bufferSize - putbackSize );
  if ( num <= 0 ){
    setg( 0, 0, 0 );
    return traits_type::eof();
  }
  setg( buffer + putbackSize - numPutBack, 
	buffer + putbackSize,
	buffer + putbackSize + num );
  return traits_type::to_int_type(*gptr());
}

// #define DEBUG

bool nb_getline( istream& is, string& result, int& timeout ){
  // a getline for nonblocking connections.
  // retry for a few special cases until timeout reached.
  // return false except when correctly terminated 
  // ( meaning \n or an EOF after at least some input)
  result = "";
  char c;
  int count = 0;
  while ( is && timeout > 0 ){
    if ( is.get( c ) ){
#ifdef DEBUG
      cerr << "-'" << c << "'-" << endl;
#endif
      if ( c == '\n' ){
	return true;
      }
      result += c;
    }
    else if ( errno == EAGAIN || errno == EWOULDBLOCK ){
#ifdef DEBUG
      cerr << "Blocked again" << endl;
#endif
      is.clear();
      errno = 0;
      Common::milli_wait(100);
      if ( ++count == 10 ){
	--timeout;
	count = 0;
      }
    }
    else if ( is.eof() && !result.empty() )
      return true;
    else
      return false;
  }
  return false;
}

bool nb_putline( ostream& os, const string& what, int& timeout ){
  // a putline for nonblocking connections.
  // retry for a few special cases until timeout reached.
  // return false except when correctly terminated 
  // Must handle SIGPIPE
  unsigned int i=0;
  int count = 0;
  bool result = true;
  typedef void (*sig_hndl)(int); 
  sig_hndl sig;
  // specify that the SIGPIPE signal is to be ignored
  sig=signal(SIGPIPE,SIG_IGN);
  while ( result && i < what.length() && os && timeout > 0 ){
    if ( os.put( what[i] ) ){
      ++i;
    }
    else if ( errno == EAGAIN || errno == EWOULDBLOCK ){
#ifdef DEBUG
      cerr << "Blocked again" << endl;
#endif
      os.clear();
      errno = 0;
      Common::milli_wait(100);
      if ( ++count == 10 ){
	--timeout;
	count = 0;
      }
    }
    else
      result = false;
    if ( os.eof() ){
      result = false;
    }
  }
  // restore old handler
  signal( SIGPIPE, sig );
  return result;
}
