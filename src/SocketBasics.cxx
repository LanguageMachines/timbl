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
#include <stdexcept>
#include <iostream>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <fcntl.h>

#include "config.h"
#include "timbl/Types.h"
#include "timbl/SocketBasics.h"

using namespace std;

namespace Sockets {
  const int TCP_BUFFER_SIZE = 2048;     // length of Internet inputbuffers

#ifndef PTHREADS
  // define stubs
  
  bool Socket::read( string& line ) {
    cerr << "No Socket operations available." << endl;
    return false;
  }

  bool Socket::read( string&, int ) {
    cerr << "No Socket operations available." << endl;
    return false;
  }

  bool Socket::write( const string& line ){
    cerr << "No Socket operations available." << endl;
    return false;
  }

#else
  
  Socket::~Socket() { 
    if ( sock >= 0 ) ::close(sock); 
  };

  bool Socket::read( string& line ) {
    if ( !isValid() ){
      mess = "read: socket invalid";
      return false;
    }
    char buf[TCP_BUFFER_SIZE];
    line = "";
    long int total_count = 0;
    char last_read = 0;
    char *current_position = buf;
    long int bytes_read = -1;
    while (last_read != 10) { // read 1 character at a time upto \lf
      bytes_read = ::read( sock, &last_read, 1 );
      if ( bytes_read <= 0) {
	// The other side may have closed unexpectedly 
	break;
      }
      if ( ( total_count < TCP_BUFFER_SIZE ) && 
	   ( last_read != 10) && (last_read !=13) ) {
	*current_position++ = last_read;
	total_count++;
      }
    }
    if ( bytes_read <= 0 ) {
      mess = "read: failed before a newline was found";
      return false;
    }
    else {
      *current_position = 0;
      line = buf;
      return true;
    }
  }

  //#define DEBUG

  void milli_wait( int m_secs ){
    struct timespec tv;
    ldiv_t div = ldiv( m_secs, 1000 );
    tv.tv_sec = div.quot;               // seconds
    tv.tv_nsec = div.rem * 1000000;     // nanoseconds
#ifdef DEBUG
    cerr << "sleeping for " << div.quot 
	 << " seconds and " << div.rem << " milli seconds" << endl;
#endif
    while ( nanosleep( &tv, &tv ) < 0 ){
#ifdef DEBUG
      cerr << "sleeping some more" << endl;
#endif
    }
  }

  bool Socket::read( string& result, unsigned int timeout ) {
    // a getline for nonblocking connections.
    // retry for a few special cases until timeout reached.
    // return false except when correctly terminated 
    // ( meaning \n or an EOF after at least some input)
    result = "";
    bool oldMode = mode;
    if ( !setNonBlocking(true) ){
      return false;
    }
    else {
      char buf[5];
      int count = 0;
      while ( timeout > 0 ){
	ssize_t res = ::read( sock, buf, 1 );
#ifdef DEBUG
	cerr << "read res = " << res  << " ( " << strerror(res) << ")" << endl;
#endif
	if ( res == 1 ){
	  char c = buf[0];
#ifdef DEBUG
	  cerr << "-'" << c << "'-" << endl;
#endif
	  if ( c == '\n' ){
	    return setNonBlocking( oldMode );
	  }
	  result += c;
	}
	else if ( res == EAGAIN || res == EWOULDBLOCK ){
	  milli_wait(100);
	  if ( ++count == 10 ){
	    --timeout;
	    count = 0;
	  }
	}
	else if ( res == -1 && !result.empty() ){
	  return setNonBlocking( oldMode );
	}
	else {
	  setNonBlocking( oldMode );
	  return false;
	}
      }
      mess = "timed out";
    }
    setNonBlocking( oldMode );
    return false;
  }


  bool Socket::write( const string& line ){
    if ( !isValid() ){
      mess = "write: socket invalid";
      return false;
    }
    if ( !line.empty() ){
      size_t bytes_sent = 0;
      long int this_write;
      size_t count = line.length();
      const char *str = line.c_str();
      while ( bytes_sent < count ){
	do {
	  this_write = ::write(sock, str, count - bytes_sent);
	} while ( (this_write < 0) && (errno == EINTR) );
	if (this_write <= 0)
	  break;
	bytes_sent += this_write;
	str += this_write;
      }
      if ( bytes_sent < count ) {
	mess = "write: failed to sent " + Timbl::toString(count - bytes_sent) +
	  " bytes out of " + Timbl::toString(count);
	return false;
      }
    }
    return true;
  }
  
  string Socket::getMessage() const{
    string m;
    if ( isValid() )
      m = "socket " + Timbl::toString(sock);
    else
      m = "invalid socket ";
    if ( !mess.empty() )
      m += ": " + mess;
    return mess; 
  };

  bool Socket::setNonBlocking ( const bool b ) {
    int opts = fcntl( sock, F_GETFL );
#ifdef DEBUG
    cerr << "socket opts = " << opts << endl;
#endif
    if ( opts < 0 ) {
      mess = "fctl failed";
#ifdef DEBUG
    cerr << "fctl: " << mess << endl;
#endif
      return false;
    }
    else {
      if ( b )
	opts = ( opts | O_NONBLOCK );
      else
	opts = ( opts & ~O_NONBLOCK );
#ifdef DEBUG
      cerr << "try to set socket opts = " << opts << endl;
#endif
      if ( fcntl( sock, F_SETFL, opts ) < 0 ){
	mess = "fctl failed";
#ifdef DEBUG
	cerr << "fctl: " << mess << endl;
#endif
	return false;
      }
    }
    mode = !b;
#ifdef DEBUG
    cerr << "setNonBlocking done" << endl;
#endif
    return true;
  }

#ifdef HAVE_GETADDRINFO

  bool ClientSocket::connect( const string& hostString,
			      const string& portString ){
    struct addrinfo *res, *aip;
    struct addrinfo hints;
    memset( &hints, 0, sizeof(hints) );
    hints.ai_flags = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int eno;
    sock = -1;
    if ( (eno=getaddrinfo( hostString.c_str(), portString.c_str(),
			   &hints, &res ) ) != 0 ){
      mess = "ClientSocket connect: invalid hostname '" +
	hostString + "' (" + gai_strerror(eno) + ")";
    }
    else {
      aip = res;
      while( aip ){
	sock = socket( aip->ai_family, aip->ai_socktype, aip->ai_protocol);
	if ( sock > 0 )
	  break;
	sock = -1;
	aip = aip->ai_next;
      }
      if ( sock < 0 ){
	mess = string( "ClientSocket: Socket could not be created: (" )
	  + strerror(errno) + ")";
      }
      else {
	// connect the socket
	int val = 1;
	setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(val) );
	val = 1;
	setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (void *)&val, sizeof(val) );
	if ( ::connect( sock, aip->ai_addr, aip->ai_addrlen ) < 0 ){
	  close( sock );
	  mess = string( "ClientSocket: Connection on ") + hostString + ":"
	    + Timbl::toString(sock) + " failed (" + strerror(errno) + ")";
	}
      }
      freeaddrinfo( res ); // and delete all addr_info stuff
    }
    return isValid();
  }

  bool ServerSocket::connect( const string& port ){
    sock = -1;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags    = AI_PASSIVE;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    struct addrinfo *res;
    int status = getaddrinfo( 0, port.c_str(), &hints, &res);
    if ( status != 0) {
      mess = string("getaddrinfo error:: [") + gai_strerror(status) + "]";
    }
    else {
      struct addrinfo *resSave = res;
      // try to start up server
      // 
      while ( res ){
	sock = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
	if ( sock >= 0 ){
	  int val = 1;
	  if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, 
			   (void *)&val, sizeof(val) ) == 0 ){
	    val = 1;
	    if ( setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, 
			     (void *)&val, sizeof(val) ) == 0 ){
	      if ( bind( sock, res->ai_addr, res->ai_addrlen ) == 0 ){
		break;
	      }
	    }
	  }
	  mess = strerror( errno );
	  sock = -1;
	}
	res = res->ai_next;
      }
      freeaddrinfo( resSave );
    }
    return isValid();
  }

  bool ServerSocket::accept( ServerSocket& newSocket ){
    newSocket.sock = -1;
    struct sockaddr_storage cli_addr;
    TIMBL_SOCKLEN_T clilen = sizeof(cli_addr);
    int newsock = ::accept( sock, (struct sockaddr *)&cli_addr, &clilen );
    if( newsock < 0 ){
      mess = string("server-accept failed: (") + strerror(errno) + ")";
    }
    else {
      char host_name[NI_MAXHOST];
      int err = getnameinfo( (struct sockaddr *)&cli_addr,
			     clilen,
			     host_name, sizeof(host_name),
			     0, 0,
			     0 );
      string name;
      if ( err != 0 ){
	name = string(" failed: getnameinfo ") + strerror(errno);
      }
      else {
	name = host_name;
      }
      err = getnameinfo( (struct sockaddr *)&cli_addr,
			 clilen,
			 host_name, sizeof(host_name),
			 0, 0,
			 NI_NUMERICHOST );
      if ( err == 0 ){
	name += string(" [") + host_name + "]";
      }
      newSocket.sock = newsock;
      newSocket.clientName = name;
    }
    return newSocket.isValid();
  }

#else

  // Converts ascii text to in_addr struct.
  // NULL is returned if the address can not be found.
  struct in_addr *atoaddr( const char *address){
    struct hostent *host;
    static struct in_addr saddr;
    
    /* First try it as aaa.bbb.ccc.ddd. */
    saddr.s_addr = inet_addr(address);
    if (saddr.s_addr != (unsigned int)-1) {
      return &saddr;
    }
    host = gethostbyname(address);
    if (host != NULL) {
      return (struct in_addr *) *host->h_addr_list;
    }
    return NULL;
  }

  bool ClientSocket::connect( const string& host, const string& portNum ){
    int port = stringTo<int>(portNum );
    if (port == -1) {
      mess = "ClientSocket connect: invalid port number";
      return false;
    }
    struct in_addr *addr = atoaddr( host.c_str() );
    if (addr == NULL) {
      mess = "ClientSocket connect:  Invalid host.";
      return false;
    }
    
    struct sockaddr_in address;
    memset((char *) &address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = addr->s_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if ( sock < 0 ){
      mess = "ClientSocket connect: socket failed";
    }
    else {
      int val = 1;
      setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(val) );
      val = 1;
      setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (void *)&val, sizeof(val) );
      int connected = ::connect( sock, (struct sockaddr *) &address,
				 sizeof(address));
      if (connected < 0) {
	mess = string( "ClientSocket connect: ") + host + ":" + portNum +
	  " failed (" + strerror( errno ) + ")";
      }
    }
    return isValid();
  }
  
  bool ServerSocket::connect( const string& port ){
    sock = -1;
    sock = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sock < 0 ){
      mess = string("ServerSocket connect: socket failed (" )
	+ strerror( errno ) + ")";
    }
    else {
      int val = 1;
      setsockopt( sock, SOL_SOCKET, SO_REUSEADDR,
		  (void *)&val, sizeof(val) );
      val = 1;
      setsockopt( sock, IPPROTO_TCP, TCP_NODELAY,
		  (void *)&val, sizeof(val) );
      struct sockaddr_in serv_addr;
      memset((char *) &serv_addr, 0, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      int TCP_PORT = stringTo<int>(port);
      serv_addr.sin_port = htons(TCP_PORT);
      if ( bind( sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr) ) < 0 ){
	mess = string( "ServerSocket connect: bind failed (" )
	  + strerror( errno ) + ")";
      }
    }
    return isValid();
  }

  bool ServerSocket::accept( ServerSocket& newSocket ){
    newSocket.sock = -1;
    struct sockaddr_storage cli_addr;
    TIMBL_SOCKLEN_T clilen = sizeof(cli_addr);
    int newsock = ::accept( sock, (struct sockaddr *)&cli_addr, &clilen );
    if( newsock < 0 ){
      mess = string("ServerSocket: accept failed: (") + strerror(errno) + ")";
    }
    else {
      string clientname;
      struct sockaddr_in rem;
      TIMBL_SOCKLEN_T remlen = sizeof(rem);
      if ( getpeername( newsock, (struct sockaddr *)&rem, &remlen ) >= 0 ){
	struct hostent *host = gethostbyaddr( (char *)&rem.sin_addr,
					      sizeof rem.sin_addr,
					      AF_INET );
	if ( host ){
	  clientname = host->h_name;
	  char **p;
	  for (p = host->h_addr_list; *p != 0; p++) {
	    struct in_addr in;
	    (void) memcpy(&in.s_addr, *p, sizeof (in.s_addr));
	    clientname += string(" [") + inet_ntoa(in) + "]";
	  }
	}
      }
      newSocket.clientName = clientname;
      newSocket.sock = newsock;
    }
    return newSocket.isValid();
  }

#endif 

  bool ServerSocket::listen( unsigned int num ){
    if ( ::listen( sock, num) < 0 ) {
      // maximum of num pending requests
      mess = string("server-listen failed: (") + strerror(errno) + ")";
      return false;
    }
    else
      return true;
  }


#endif // PTHREADS
}
