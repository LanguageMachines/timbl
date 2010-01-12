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

#ifndef TIMBLSERVER_H
#define TIMBLSERVER_H

#include "timbl/TimblExperiment.h"

namespace Timbl {

  class TimblServer : public MsgClass {
    friend class TimblServerAPI;
    friend TimblServer *CreateServerPimpl( AlgorithmType, GetOptClass * );
  protected:
    TimblServer();
    bool getConfig( const std::string& );
    bool startClassicServer( int, int=0 );
    bool startMultiServer( const std::string& );
    bool SetSingleThreaded();
    void setOutPath( const std::string& s ){ outPath = s; };
    void setServerConfigFile( const std::string& s ){ serverConfigFile = s; };
    void RunClassicServer();
    void RunHttpServer();
    TimblExperiment *CreateClient( Sockets::ServerSocket* ) const;
    TimblExperiment *splitChild() const;
    //    VerbosityFlags ServerVerbosity() { return exp->get_s_verbosity(); };
    Sockets::ServerSocket *TcpSocket() const { return tcp_socket; };
    int Max_Connections() const { return maxConn; };
    TimblExperiment *exp;
    std::string logFile;
    std::string pidFile;
  private:
    int maxConn;
    int serverPort;
    Sockets::ServerSocket *tcp_socket;
    std::string outPath;
    std::string serverProtocol;
    std::string serverConfigFile;
    std::map<std::string, std::string> serverConfig;
  };

  class IB1_Server: public TimblServer {
  public:
    IB1_Server( GetOptClass * );
  };
  
  class IG_Server: public TimblServer {
  public:
    IG_Server( GetOptClass * );
  };
 
  class TRIBL_Server: public TimblServer {
  public:
    TRIBL_Server( GetOptClass * );
  };
  
  class TRIBL2_Server: public TimblServer {
  public:
    TRIBL2_Server( GetOptClass * );
  };

}
#endif // TIMBLSERVER_H
