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

#include "config.h"
#include "timbl/Types.h"
#include "timbl/SocketBasics.h"

using namespace std;

namespace Timbl {
  const int TCP_BUFFER_SIZE = 2048;     // length of Internet inputbuffers

#ifndef PTHREADS
  // define stubs
  int make_connection( const string&, const string&, int ){
    cerr << "No Socket operations available." << endl;
    return -1;
  }
  bool read_line( int, string&, int){
    cerr << "No Socket operations available." << endl;
    return false;
  }
  bool write_line( int, const string& ){
    cerr << "No Socket operations available." << endl;
    return false;
  }
  
  bool Socket::read( string& line ){
    cerr << "No Socket operations available." << endl;
    return false;
  }

  bool Socket::write( const string& line ){
    cerr << "No Socket operations available." << endl;
    return false;
  }

#else

  bool Socket::read( string& line ){
    if ( !valid ){
      mess = "read: socket invalid";
      return false;
    }
    char buf[TCP_BUFFER_SIZE];
    line = "";
    long int total_count = 0;
    char last_read = 0;
    char *current_position = buf;
    long int bytes_read = -1;
    while (last_read != 10) { // read upto \lf
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
    if ( bytes_read  < 0 ) {
      mess = "read: failed before a newline was found";
      return false;
    }
    else {
      *current_position = 0;
      line = buf;
      return true;
    }
  }

  bool Socket::write( const string& line ){
    if ( !valid ){
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
  

#ifndef HAVE_GETADDRINFO
  /* Take a service name, and a service type, and return a port number.  If the
     service name is not found, it tries it as a decimal number.  The number
     returned is byte ordered for the network. */
  int atoport( const char *service ){
    int port;
    long int lport;
    struct servent *serv;
    char *errpos;
    
    /* First try to read it from /etc/services */
    serv = getservbyname(service, "tcp");
    if (serv != NULL)
      port = serv->s_port;
    else { /* Not in services, maybe a number? */
      lport = strtol(service,&errpos,0);
      if ( (errpos[0] != 0) || (lport < 1) || (lport > 65535) )
	return -1; /* Invalid port address */
      port = htons(lport);
   }
    return port;
  }

  /* Converts ascii text to in_addr struct.  NULL is returned if the address
     can not be found. */
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

  /* This is a generic function to make a connection to a given server/port.
     service is the port name/number,
     type is either SOCK_STREAM or SOCK_DGRAM, and
     netaddress is the host name to connect to.
     The function returns the socket, ready for action.*/
  int make_connection( const string& service,
		       const string& netaddress ){
    /* First convert service from a string, to a number... */
    int port = -1;
    struct in_addr *addr;
    int sock, connected;
    struct sockaddr_in address;
    
    port = atoport(service.c_str() );
    if (port == -1) {
      fprintf(stderr,"make_connection:  Invalid socket type.\n");
      return -1;
    }
    addr = atoaddr(netaddress.c_str());
    if (addr == NULL) {
      fprintf(stderr,"make_connection:  Invalid network address.\n");
      return -1;
    }
    
    memset((char *) &address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = (port);
    address.sin_addr.s_addr = addr->s_addr;
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    int val = 1;
    setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(val) );
    val = 1;
    setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (void *)&val, sizeof(val) );
    
    connected = connect(sock, (struct sockaddr *) &address,
			sizeof(address));
    if (connected < 0) {
      printf( "Failed connecting to %s on port %d.\n",
	      inet_ntoa(*addr),htons(port));
      perror("connect");
      return -1;
    }
    return sock;
  }
  
  //     This function reads from a socket, until it recieves a linefeed
  //     character.  It fills the buffer "str" up to the maximum size "count".
  //     This function will return -1 if the socket is closed during the read
  //     operation.
  //     Note that if a single line exceeds the length of count, the extra data
  //     will be read and discarded!  You have been warned. 
  long int sock_read(int sockfd,char *str, long int count){
    long int total_count = 0;
    char last_read = 0;
    char *current_position = str;
    while (last_read != 10) {
      long int bytes_read = read( sockfd, &last_read, 1 );
      if (bytes_read <= 0) {
	// The other side may have closed unexpectedly 
	return -1; 
      }
      if ( (total_count < count) && 
	   (last_read != 10) && (last_read !=13) ) {
	*current_position++ = last_read;
	total_count++;
      }
    }
    if (count > 0)
      *current_position = 0;
    return total_count;
  }

  long int sock_write( int sockfd, const char *str ){
    // This is just like the write() system call, accept that it will
    // make sure that all data is transmitted. 
    // return -1 if the connection is closed while it is trying to write.
    size_t bytes_sent = 0;
    long int this_write;
    size_t count = strlen( str );
    while (bytes_sent < count) {
      do {
	this_write = write(sockfd, str, count - bytes_sent);
      } while ( (this_write < 0) && (errno == EINTR) );
      if (this_write <= 0)
	return this_write;
      bytes_sent += this_write;
      str += this_write;
    }
    return count;
  }

  bool read_line( int socknum, string& line, int Size ){
    char *tmp = new char[Size];
    line = "";
    if ( sock_read( socknum, tmp, Size ) < 0 ) {
      delete [] tmp;
      return false;
    }
    else {
      line = tmp;
      delete [] tmp;
      return true;
    }
  }

  bool write_line( int socknum, const string& line ){
    // write a line to the socket
    if ( !line.empty() )
      if ( sock_write( socknum, line.c_str() ) < 0 ) {
	return false;
      }
    return true;
  }

  bool ClientSocket::connect( const string& host, const string& portNum ){
    /* First convert service from a string, to a number... */
    int port = -1;
    struct in_addr *addr;
    int sock, connected;
    
    port = atoport(portNum.c_str() );
    if (port == -1) {
      mess = string( "ClientSocket connect: Invalid socket type: (" )
	+ portNum + ")";
      return false;
    }
    addr = atoaddr( host.c_str());
    if (addr == NULL) {
      mess = string( "ClientSocket connect: Invalid hostname: (")
	+ host + ")";
      return false;
    }
    memset((char *) &address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = (port);
    address.sin_addr.s_addr = addr->s_addr;
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    int val = 1;
    setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(val) );
    val = 1;
    setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (void *)&val, sizeof(val) );
    
    connected = connect(sock, (struct sockaddr *) &address,
			sizeof(address));
    if (connected < 0) {
      mess = string( "Failed connecting to ") +  inet_ntoa(*addr) 
		     + " on port " + port + ".",
		     inet_ntoa(*addr),htons(port);
      return false;
    }
    else
      valid = true;
    return valid;
  }

  bool ServerSocket::connect( const string& port ){
    mess = "connect to do!" ;
    return false;
  }
  bool ServerSocket::listen( unsigned int num ){
    mess = "listen to do!" ;
    return false;
  }
  bool ServerSocket::accept( ServerSocket& newSocket ){
    mess = "accept to do!" ;
    return false;
  }

#else

  bool ClientSocket::connect( const string& hostString,
			      const string& portString ){
    sock = -1;
    valid = false;
    struct addrinfo *res, *aip;
    memset( &hints, 0, sizeof(hints) );
    hints.ai_flags = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int eno;
    sock = -1;
    if ( (eno=getaddrinfo( hostString.c_str(), portString.c_str(),
			   &hints, &res ) ) != 0 ){
      mess = "ClientSocket: getting address from '" +
	hostString + "' failed, err = " + gai_strerror(eno);
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
	else
	  valid = true;
      }
      freeaddrinfo( res ); // and delete all addr_info stuff
    }
    return valid;
  }

  bool ServerSocket::connect( const string& port ){
    sock = -1;
    valid = false;

    memset(&hints, 0, sizeof(struct addrinfo));
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
		valid = true;
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
    return valid;
  }

  bool ServerSocket::listen( unsigned int num ){
    if ( ::listen( sock, num) < 0 ) {
      // maximum of 5 pending requests
      mess = string("server-listen failed: (") + strerror(errno) + ")";
      return false;
    }
    else
      return true;
  }

  bool ServerSocket::accept( ServerSocket& newSocket ){
    newSocket.valid = false;
    newSocket.sock = -1;
    struct sockaddr_storage cli_addr;
    TIMBL_SOCKLEN_T clilen = sizeof(cli_addr);
    int newsock = ::accept( sock, (struct sockaddr *)&cli_addr, &clilen );
    if( newsock < 0 ){
      mess = string("server-accept failed: (") + strerror(errno) + ")";
      return false;
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
      newSocket.valid = true;
      return true;
    }
  }

#endif 

#endif // PTHREADS
}
