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

#ifndef SOCKET_BASICS_H
#define SOCKET_BASICS_H

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

namespace Sockets {

  class Socket {
  public: 
  Socket(): nonBlocking(false),sock(-1){};
    virtual ~Socket();
    bool isValid() const { return sock != -1 ; };
    std::string getMessage() const;
    int getSockId() const { return sock; }
    bool read( std::string& );
    bool read( std::string&, unsigned int );
    bool write( const std::string& );
    bool write( const std::string&, unsigned int );
    bool setNonBlocking();
    bool setBlocking();
  protected:
    bool nonBlocking;
    int sock;
    std::string mess;
  };

  class ClientSocket: public Socket {
  public:
    bool connect( const std::string&, const std::string& );
  };
  
  class ServerSocket: public Socket {
  public:
    bool connect( const std::string& );
    bool listen( unsigned int = 5 );
    bool accept( ServerSocket& );
    std::string getClientName() const { return clientName; };
  private:
    std::string clientName;
  };
}

#endif
