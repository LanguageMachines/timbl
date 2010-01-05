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
#include "timbl/Types.h"
#include "timbl/Tree.h"
#include "timbl/Instance.h"
#include "timbl/Common.h"
#include "timbl/SocketBasics.h"
#include "timbl/LogStream.h"
#include "timbl/Options.h"
#include "timbl/GetOptClass.h"
#include "timbl/ServerBase.h"

using namespace std;

namespace Timbl {

  TimblServer::TimblServer(){
    maxConn = 25;
    serverPort = -1;
    exp = 0;
    tcp_socket = 0;
  }  

  bool TimblServer::getConfig( const string& serverConfigFile ){
    maxConn = 25;
    serverPort = -1;
    serverProtocol = "http";
    ifstream is( serverConfigFile.c_str() );
    if ( !is ){
      Error( "problem reading " + serverConfigFile );
      return false;
    }
    else {
      string line;
      while ( getline( is, line ) ){
	if ( line.empty() || line[0] == '#' )
	  continue;
	string::size_type ispos = line.find('=');
	if ( ispos == string::npos ){
	  Error( "invalid entry in: " + serverConfigFile );
	  Error( "offending line: '" + line + "'" );
	  return false;
	}
	else {
	  string base = line.substr(0,ispos);
	  string rest = line.substr( ispos+1 );
	  if ( !rest.empty() ){
	    string tmp = base;
	    lowercase(tmp);
	    if ( tmp == "maxconn" ){
	      if ( !stringTo( rest, maxConn ) ){
		Error( "invalid value for maxconn" );
		return false;
	      }
	    }
	    else if ( tmp == "port" ){
	      if ( !stringTo( rest, serverPort ) ){
		Error( "invalid value for port" );
		return false;
	      }
	    }
	    else if ( tmp == "protocol" ){
	      string protocol = rest;
	      lowercase( protocol );
	      if ( protocol != "http" && protocol != "tcp" ){
		Error( "invalid protocol" );
		return false;
	      }
	      serverProtocol = protocol;
	    }
	    else {
	      string::size_type spos = 0;
	      if ( rest[0] == '"' )
		spos = 1;
	      string::size_type epos = rest.length()-1;
	      if ( rest[epos] == '"' ) 
		--epos;
	      serverConfig[base] = rest.substr( spos, epos );
	    }
	  }
	}
      }
      if ( serverPort < 0 ){
	Error( "missing 'port=' entry in config file" );
	return false;
      }
      else
	return true;
    }
  }
  
  bool TimblServer::startClassicServer( int port , int maxC ){
    serverPort = port;
    if ( maxC > 0 )
      maxConn = maxC;
    Info( "Starting a classic server on port " + toString( serverPort ) );
    if ( exp && exp->ConfirmOptions() ){
      exp->initExperiment( true );
      RunClassicServer( this );
    }
    else {
      Error( "invalid options" );
    }
    return false;
  }

  bool TimblServer::startMultiServer( const string& config ){
    if ( exp && exp->ConfirmOptions() ){
      if ( getConfig( config ) ){
	if ( serverProtocol == "http" ){
	  Info( "Starting a HTTP server on port " + toString( serverPort ) );
	  RunHttpServer( this );
	}
	else {
	  Info( "Starting a TCP server on port " + toString( serverPort ) );
	  RunClassicServer( this );
	}
      }
      else {
	Error( "invalid serverconfig" );
      }
    }
    else {
      Error( "invalid options" );
    }
    return false;
  }
      

  IB1_Server::IB1_Server( GetOptClass *opt ){
    exp = new IB1_Experiment( opt->MaxFeatures() );
    if (exp ){
      exp->UseOptions( opt );
      logFile = opt->getLogFile();
      pidFile = opt->getPidFile();
    }
  }

  IG_Server::IG_Server( GetOptClass *opt ){
    exp = new IG_Experiment( opt->MaxFeatures() );
    if (exp ){
      exp->UseOptions( opt );
      logFile = opt->getLogFile();
      pidFile = opt->getPidFile();
    }
  }

  TRIBL_Server::TRIBL_Server( GetOptClass *opt ){
    exp = new TRIBL_Experiment( opt->MaxFeatures() );
    if (exp ){
      exp->UseOptions( opt );
      logFile = opt->getLogFile();
      pidFile = opt->getPidFile();
    }
  }

  TRIBL2_Server::TRIBL2_Server( GetOptClass *opt ){
    exp = new TRIBL2_Experiment( opt->MaxFeatures() );
    if (exp ){
      exp->UseOptions( opt );
      logFile = opt->getLogFile();
      pidFile = opt->getPidFile();
    }
  }

}
