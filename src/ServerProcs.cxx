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
#include <set>
#include <map>
#include <iosfwd>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <csignal>
#include <cerrno>

#include <sys/time.h>

#include "config.h" // for TIMBL_SOCKLEN_T
#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/MsgClass.h"
#ifdef USE_LOGSTREAMS
#include "timbl/LogStream.h"
#else
typedef std::ostream LogStream;
#define Log(X) (&X)
#define Dbg(X) (X)
#endif
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/Tree.h"
#include "timbl/IBtree.h"
#include "timbl/Tree.h"
#include "timbl/Instance.h"
#include "timbl/neighborSet.h"
#include "timbl/Statistics.h"
#include "timbl/BestArray.h"
#include "timbl/SocketBasics.h"
#include "timbl/MBLClass.h"
#include "timbl/GetOptClass.h"
#include "timbl/TimblExperiment.h"
#include "timbl/TimblAPI.h"
#include "timbl/ServerProcs.h"

using namespace std;
using namespace Sockets;

namespace Timbl {

#ifndef PTHREADS
  bool do_command( const string&, TimblExperiment *Exp ){
    *Log(Exp->my_err()) << "Server Mode not available" << endl;
    return false;
  }
  int ClassifyFromSocket( TimblExperiment *Exp ){
    *Log(Exp->my_err()) << "Server Mode not available" << endl;
    return -1;
  }
  void *do_chld( void *arg ){
    TimblExperiment *Tmp = (TimblExperiment*)arg;
    *Log(Tmp->my_err()) << "Server Mode not available" << endl;
    return NULL;
  }
  void RunServer( TimblExperiment *Mother, int ){
    *Log(Mother->my_err()) << "Server Mode not available" << endl;
  }
  void RunClient( istream&, ostream&, 
		  const string&, const string&, bool ){
    cerr << "Client Mode not available" << endl;
  }
#else // PTHREADS

const int TCP_BUFFER_SIZE = 2048;     // length of Internet inputbuffers,

#include <pthread.h>

  enum CommandType { UnknownCommand, Classify, Query, Set, Exit, Comment };
  enum CodeType { UnknownCode, Result, Error, OK, Skip,
		  Neighbors, EndNeighbors, Status, EndStatus };
  
  inline void Split( const string& line, string& com, string& rest ){
    string::const_iterator b_it = line.begin();
    while ( b_it != line.end() && isspace( *b_it ) ) ++b_it;
    string::const_iterator m_it = b_it;
    while ( m_it != line.end() && !isspace( *m_it ) ) ++m_it;
    com = string( b_it, m_it );
    while ( m_it != line.end() && isspace( *m_it) ) ++m_it;
    rest = string( m_it, line.end() );
  }
  
  CommandType check_command( const string& com ){
    CommandType result = UnknownCommand;
    if ( compare_nocase_n( com, "CLASSIFY" ) )
      result = Classify;
    else if ( compare_nocase_n( com, "QUERY" ) )
      result = Query;
    else if ( compare_nocase_n( com, "SET") )
      result = Set;
    else if ( compare_nocase_n( com, "EXIT" ) )
      result = Exit;
    else if ( com[0] == '#' )
      result = Comment;
    return result;
  }
  
  CodeType get_code( const string& com ){
    CodeType result = UnknownCode;
    if ( compare_nocase( com, "CATEGORY" ) )
      result = Result;
    else if ( compare_nocase( com, "ERROR" ) )
      result = Error;
    else if ( compare_nocase( com, "OK" ) )
      result = OK;
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
  
  bool check_for_neigbors( const string& line ){
    return line.find( "NEIGHBORS" ) != string::npos;
  }
  
  bool do_command( const string& Line, TimblExperiment *Exp ){
    bool go_on = true;
    if ( Exp->SetOptions( Line ) ){
      if ( Exp->ServerVerbosity() & CLIENTDEBUG )
	*Log(Exp->my_log()) << Exp->TcpSocket()->getSockId()
			    << ": Command :" << Line << endl;
      if ( Exp->ConfirmOptions() )
	go_on = Exp->TcpSocket()->write( "OK\n" );
      else {
      }
    }
    else {
      if ( Exp->ServerVerbosity() & CLIENTDEBUG )
	*Log(Exp->my_log()) << Exp->TcpSocket()->getSockId()
			    << ": Don't understand '" 
			    << Line << "'" << endl;
    }
    return go_on;
  }

  bool classifyOneLine( TimblExperiment *Exp, const string& params ){
    double Distance;
    string Distrib;
    string Answer;
    ServerSocket *sock = Exp->TcpSocket();
    if ( Exp->Classify( params, Answer, Distrib, Distance ) ){
      if ( Exp->ServerVerbosity() & CLIENTDEBUG )
	*Log(Exp->my_log()) << sock->getSockId() << ": " 
			    << params << " --> " 
			    << Answer << " " << Distrib 
			    << " " << Distance << endl;
      bool go_on = sock->write( "CATEGORY {" ) &&
	sock->write( Answer ) &&
	sock->write( "}" );
      if ( go_on ){
	if ( Exp->Verbosity(DISTRIB) ){
	  go_on = sock->write( " DISTRIBUTION " ) &&
	    sock->write( Distrib );
	}
	if ( go_on ){
	  if ( Exp->Verbosity(DISTANCE) ){
	    go_on = sock->write( " DISTANCE {" ) &&
	      sock->write( toString<double>(Distance) ) &&
	      sock->write( "}" );
	  }
	  if ( go_on ){
	    if ( Exp->Verbosity(NEAR_N) ){
	      ostringstream tmp;
	      tmp << " NEIGHBORS\n";
	      Exp->showBestNeighbors( tmp );
	      tmp << "ENDNEIGHBORS";
	      go_on = sock->write( tmp.str() );
	    }
	  }
	}
      }
      if ( go_on ){
	go_on = sock->write( "\n" );
      }
      return go_on;
    }
    else {
      if ( Exp->ServerVerbosity() & CLIENTDEBUG )
	*Log(Exp->my_log()) << sock->getSockId()
			    << ": Classify Failed on '" 
			    << params << "'" << endl;
      return false;
    }
  }
  
  int classicClassify( const string& firstLine, TimblExperiment *Exp ){ 
    string Line = firstLine, Command, Param;
    int result = 0;
    bool go_on = true;
    ServerSocket *sock = Exp->TcpSocket();
    *Dbg(Exp->my_debug()) << "ClassifyFromSocket: " 
			  << sock->getSockId() << endl;
    do {
      *Dbg(Exp->my_debug()) << "Line='" << Line << "'" << endl;
      Split( Line, Command, Param );
      switch ( check_command(Command) ){
      case Set:
	go_on = do_command( Param, Exp );
	break;
      case Query:
	go_on = Exp->ShowSettings( *Log(Exp->my_log()) );
	break;
      case Exit:
	go_on = sock->write( "OK" ) && sock->write( " Closing\n" );
	return result;
	break;
      case Classify:
	if ( classifyOneLine( Exp, Line ) )
	  result++;
	go_on = true; // HACK?
	break;
      case Comment:
	go_on = sock->write( "SKIP " ) &&
	  sock->write( Line ) &&
	  sock->write( "\n" );
	break;
      default:
	if ( Exp->ServerVerbosity() & CLIENTDEBUG )
	  *Log(Exp->my_log()) << sock->getSockId() << ": Don't understand '" 
			      << Line << "'" << endl;
	go_on = sock->write( "ERROR { Illegal instruction:'" ) &&
	  sock->write( Command ) &&
	  sock->write( "' in line:" ) &&
	  sock->write( Line ) &&
	  sock->write( "}\n" );
	break;
      }
    }
    while ( go_on && sock->read( Line ) );
    return result;
  }

#define IS_DIGIT(x) (((x) >= '0') && ((x) <= '9'))
#define IS_HEX(x) ((IS_DIGIT(x)) || (((x) >= 'a') && ((x) <= 'f')) || \
            (((x) >= 'A') && ((x) <= 'F')))


  string urlDecode( const string& s ) {
    int cc;
    string result;
    int len=s.size();
    for (int i=0; i<len ; ++i ) {
      cc=s[i];
      if (cc == '+') {
	result += ' ';
      } 
      else if ( cc == '%' &&
		( i < len-2 &&
		  ( IS_HEX(s[i+1]) ) &&
		  ( IS_HEX(s[i+2]) ) ) ){
	std::istringstream ss( "0x"+s.substr(i+1,2) );
	int tmp;
	ss >> std::showbase >> std::hex;
	ss >> tmp;
      result = result + (char)tmp;
      i += 2;
      }
      else {
	result += cc;
      }
    }
    return result;
  }
  
  int ClassifyFromSocket( TimblExperiment *Exp ){ 
    string Line;
    ServerSocket *sock = Exp->TcpSocket();
    sock->write( "Welcome to the Timbl Server\n", 5 );
    if ( sock->read( Line ) ){
      *Log(Exp->my_log()) << "FirstLine='" << Line << "'" << endl;
      return classicClassify( Line, Exp );
    }
    return 0;
  }

  
  void BrokenPipeChildFun( int Signal ){
    if ( Signal == SIGPIPE ){
      signal( SIGPIPE, BrokenPipeChildFun );
    }
  }

  void show_results(  ostream& os, Timer& timeDone, int nw ){
    os << "Thread " << (uintptr_t)pthread_self() << ", terminated at: " 
       << Timer::now()
       << "Total time used in this thread: " 
       << timeDone << ", " << nw << " instances processed " ;
    if ( timeDone.secs() > 0 )
      os << " (" << nw/timeDone.secs() << " instances/sec)";
    os << endl;
  }


  // ***** This is the routine that is executed from a new thread *******
  void *do_chld( void *arg ){
    TimblExperiment *Exp = (TimblExperiment*)arg;
    ServerSocket *mysock = Exp->TcpSocket();
    static int service_count=0;

    static pthread_mutex_t my_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&my_lock);
    // use a mutex to update the global service counter
    service_count++;
    if ( service_count > Exp->Max_Connections() ){
      mysock->write( "Maximum connections exceeded\n" );
      mysock->write( "try again later...\n" );
      pthread_mutex_unlock( &my_lock );
      cerr << "Thread " << (uintptr_t)pthread_self() << " refused " << endl;
    }
    else {
      pthread_mutex_unlock( &my_lock );
      // process the test material
      // and do the timing
      //
      Timer timeDone;
      // report connection to the server terminal
      //
      char line[256];
      sprintf( line, "Thread %lu, on Socket %d", (uintptr_t)pthread_self(),
	       mysock->getSockId() );
      Exp->my_debug().message( line );
      *Log(Exp->my_log()) << line << ", started at: " 
			  << Timer::now() << endl;  
      signal( SIGPIPE, BrokenPipeChildFun );
      timeDone.start();
      int nw = ClassifyFromSocket( Exp );
      show_results(  *Log(Exp->my_log()), timeDone, nw );
      //
      pthread_mutex_lock(&my_lock);
      // use a mutex to update and display the global service counter
      *Log(Exp->my_log()) << "Socket Total = " << --service_count << endl;
      pthread_mutex_unlock(&my_lock);
      // close the socket and exit this thread
      //
      delete Exp;
    }
    return NULL;
  }


  struct childArgs{
    TimblExperiment *Exp;
    ServerSocket *socket;
    map<string, TimblAPI*> *experiments;
  };

  // ***** This is the routine that is executed from a new thread *******
  void *do_chld2( void *arg ){
    childArgs *args = (childArgs *)arg;
    TimblExperiment *Mother = args->Exp;
    ServerSocket *mysock = args->socket;
    map<string, TimblAPI*> *experiments = args->experiments;
    static int service_count=0;

    static pthread_mutex_t my_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&my_lock);
    // use a mutex to update the global service counter
    service_count++;
    if ( service_count > Mother->Max_Connections() ){
      mysock->write( "Maximum connections exceeded\n" );
      mysock->write( "try again later...\n" );
      pthread_mutex_unlock( &my_lock );
      cerr << "Thread " << (uintptr_t)pthread_self() << " refused " << endl;
    }
    else {
      pthread_mutex_unlock( &my_lock );
      // process the test material
      // and do the timing
      //
      Timer timeDone;
      // report connection to the server terminal
      //
      char line[256];
      sprintf( line, "Thread %lu, on Socket %d", (uintptr_t)pthread_self(),
	       mysock->getSockId() );
      Mother->my_debug().message( line );
      *Log(Mother->my_log()) << line << ", started at: " 
			     << Timer::now() << endl;  
      signal( SIGPIPE, BrokenPipeChildFun );
      timeDone.start();
      string Line;
      int timeout = 1;
      int nw=0;
      mysock->setNonBlocking();
      if ( mysock->read( Line, timeout ) ){
	///	*Log(Mother->my_debug()) << "FirstLine='" << Line << "'" << endl;
	if ( Line.find( "HTTP" ) != string::npos ){
	  // skip HTTP header
	  string tmp;
	  while ( (mysock->read( tmp, 1 ), !tmp.empty()) ){
	    //	    cerr << "skip: read:'" << tmp << "'" << endl;;
	  }
	  string::size_type spos = Line.find( "GET" );
	  if ( spos != string::npos ){
	    string::size_type epos = Line.find( " HTTP" );
	    string line = Line.substr( spos+3, epos - spos - 3 );
	    //	    *Log(Mother->my_debug()) << "Line='" << line << "'" << endl;
	    epos = line.find( "?" );
	    string basename, command;
	    if ( epos != string::npos ){
	      basename = line.substr( 0, epos );
	      command = line.substr( epos+1 );
	      epos = basename.find( "/" );
	      if ( epos != string::npos ){
		basename = basename.substr( epos+1 );
		epos = command.find( "=" );
		if ( epos != string::npos ){
		  string params = command.substr( epos+1 );
		  params = urlDecode(params);
		  command = command.substr( 0, epos );
		  //		  *Log(Mother->my_debug()) << "base='" << basename << "'" << endl;
		  //		  *Log(Mother->my_debug()) << "command='" << command << "'" << endl;
		  map<string,TimblAPI*>::const_iterator it= experiments->find(basename);
		  if ( it != experiments->end() ){
		    TimblAPI *api = it->second->cloneExp();
		    // cloneExp doesn't set the Exp socket. is that a problem?
		    if ( api ){
		      if ( command == "classify" ){
			string distrib, answer; 
			double distance;
			*Log(Mother->my_debug()) << "Classify(" << params << ")" << endl;
			if ( api->Classify( params, answer, distrib, distance ) ){
			  
			  *Log(Mother->my_debug()) << "resultaat: " << answer 
						   << ", distrib: " << distrib
						   << ", distance " << distance
						 << endl;
			  string result = "<?xml version=\"1.0\"?>\r\n";
			  result += string("<classification>\r\n <category>") 
			    + answer + "</category>\r\n";
			  if ( api->OptIsSet(DISTRIB) ){
			    result += string(" <distribution>") +
			      distrib + "</distribution>\r\n";
			  }
			  if ( api->OptIsSet(DISTANCE) ){
			    result += string( " <distance>" ) 
			      + toString<double>(distance) 
			      + "</distance>\r\n";
			  }
			  if ( api->OptIsSet(NEAR_N) ){
			    result += api->BestNeighborsToXML();
			  }
			  result += "</classification>";
			  *Log(Mother->my_log()) << "RESULT:" << result << endl;
			  mysock->write( result + "\r\n" );
			  
			  ++nw;
			}
			else {
			  *Log(Mother->my_log()) << "FAILED" << endl;
			}
		      }
		    }
		  }
		  else {
		    *Log(Mother->my_log()) << "invalid BASE! '" << basename 
					   << "'" << endl;
		    mysock->write( "invalid basename: '" + basename + "'\n", 5 );
		  }
		  mysock->write( "\r\n" );
		}
	      }
	    } 
	  }
	}
      }
      show_results(  *Log(Mother->my_log()), timeDone, nw );
      //
      pthread_mutex_lock(&my_lock);
      // use a mutex to update and display the global service counter
      *Log(Mother->my_log()) << "Socket Total = " << --service_count << endl;
      pthread_mutex_unlock(&my_lock);
      // close the socket and exit this thread
      delete mysock;
      //
    }
    return NULL;
  }

  void BrokenPipeFun( int Signal ){
    if ( Signal == SIGPIPE ){
      signal( SIGPIPE, SIG_IGN );
    }
  }
  
  void RunServer( TimblExperiment *Mother, int TCP_PORT ){
    string logFile =  Mother->logFile;
    string pidFile =  Mother->pidFile;
    if ( !pidFile.empty() ){
      // check validity of pidfile
      if ( pidFile[0] != '/' ) // make sure the path is absolute
	pidFile = '/' + pidFile;
      unlink( pidFile.c_str() ) ;
      ofstream pid_file( pidFile.c_str() ) ;
      if ( !pid_file ){
	*Log(Mother->my_err())<< "unable to create pidfile:"<< pidFile << endl;
	*Log(Mother->my_err())<< "TimblServer NOT Started" << endl;
	exit(1);
      }
    }
    if ( !logFile.empty() ){
      if ( logFile[0] != '/' ) // make sure the path is absolute
	logFile = '/' + logFile;
      ostream *tmp = new ofstream( logFile.c_str() );
      if ( tmp && tmp->good() ){
	*Log(Mother->my_err()) << "switching logging to file " 
			       << logFile << endl;
	Mother->my_log().associate( *tmp );
	Mother->my_err().associate( *tmp );
	*Log(Mother->my_log())  << "Started logging " << endl;	
	*Log(Mother->my_log())  << "Server verbosity " << toString<VerbosityFlags>( Mother->ServerVerbosity()) << endl;	
      }
      else {
	*Log(Mother->my_err()) << "unable to create logfile: " << logFile << endl;
	*Log(Mother->my_err()) << "not started" << endl;
	exit(1);
      }
    }

    int start = daemon( 0, logFile.empty() );

    if ( start < 0 ){
      cerr << "failed to daemonize error= " << strerror(errno) << endl;
      exit(1);
    };
    if ( !pidFile.empty() ){
      // we have a liftoff!
      // signal it to the world
      ofstream pid_file( pidFile.c_str() ) ;
      if ( !pid_file ){
	*Log(Mother->my_err())<< "unable to create pidfile:"<< pidFile << endl;
	*Log(Mother->my_err())<< "TimblServer NOT Started" << endl;
	exit(1);
      }
      else {
	pid_t pid = getpid();
	pid_file << pid << endl;
      }
    }
    // set the attributes 
    pthread_attr_t attr;
    if ( pthread_attr_init(&attr) ||
	 pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED ) ){
      *Log(Mother->my_err()) << "Threads: couldn't set attributes" << endl;
      exit(0);
    }
    *Log(Mother->my_log()) << "Starting Server on port:" << TCP_PORT << endl;

    pthread_t chld_thr;

    ServerSocket server;
    string portString = toString<int>(TCP_PORT);
    if ( !server.connect( portString ) ){
      *Log(Mother->my_err()) << "failed to start Server: " 
			     << server.getMessage() << endl;
      exit(0);
    }

    if ( !server.listen( 5 ) ) {
      // maximum of 5 pending requests
      *Log(Mother->my_err()) << server.getMessage() << endl;
      exit(0);
    }
    
    int failcount = 0;
    while( true ){ // waiting for connections loop
      signal( SIGPIPE, SIG_IGN );
      ServerSocket *newSocket = new ServerSocket();
      if ( !server.accept( *newSocket ) ){
	*Log(Mother->my_err()) << server.getMessage() << endl;
	if ( ++failcount > 20 ){
	  *Log(Mother->my_err()) << "accept failcount >20 " << endl;
	  *Log(Mother->my_err()) << "server stopped." << endl;
	  exit(EXIT_FAILURE);
	}
	else {
	  continue;  
	}
      }
      else {
	failcount = 0;
	*Log(Mother->my_log()) << "Accepting Connection #" 
			       << newSocket->getSockId()
			       << " from remote host: " 
			       << newSocket->getClientName() << endl;
	// create a new thread to process the incoming request 
	// (The thread will terminate itself when done processing
	// and release its socket handle)
	//
	*Dbg(Mother->my_debug()) << " Voor Create Client " << endl;
	TimblExperiment *Chld = Mother->CreateClient( newSocket );
	*Dbg(Mother->my_debug()) << " Na Create Client " << endl;
	*Dbg(Chld->my_debug()) << "voor pthread_create " << endl;
	pthread_create( &chld_thr, &attr, do_chld, (void *)Chld );
      }
      // the server is now free to accept another socket request 
    }
    pthread_attr_destroy(&attr); 
  }
  
  void RunAdvancedServers( TimblExperiment *Mother, int TCP_PORT ){
    string logFile =  Mother->logFile;
    string pidFile =  Mother->pidFile;
    if ( !pidFile.empty() ){
      // check validity of pidfile
      if ( pidFile[0] != '/' ) // make sure the path is absolute
	pidFile = '/' + pidFile;
      unlink( pidFile.c_str() ) ;
      ofstream pid_file( pidFile.c_str() ) ;
      if ( !pid_file ){
	*Log(Mother->my_err())<< "unable to create pidfile:"<< pidFile << endl;
	*Log(Mother->my_err())<< "TimblServer NOT Started" << endl;
	exit(1);
      }
    }
    if ( !logFile.empty() ){
      if ( logFile[0] != '/' ) // make sure the path is absolute
	logFile = '/' + logFile;
      ostream *tmp = new ofstream( logFile.c_str() );
      if ( tmp && tmp->good() ){
	*Log(Mother->my_err()) << "switching logging to file " 
			       << logFile << endl;
	Mother->my_log().associate( *tmp );
	Mother->my_err().associate( *tmp );
	*Log(Mother->my_log())  << "Started logging " << endl;	
	*Log(Mother->my_log())  << "Server verbosity " << toString<VerbosityFlags>( Mother->ServerVerbosity()) << endl;	
      }
      else {
	*Log(Mother->my_err()) << "unable to create logfile: " << logFile << endl;
	*Log(Mother->my_err()) << "not started" << endl;
	exit(1);
      }
    }

    map<string, TimblAPI*> experiments;
    map<string,string>::const_iterator it = Mother->serverConfig.begin();
    while ( it != Mother->serverConfig.end() ){
      TimblOpts opts( it->second );
      string treeName;
      string trainName;
      bool mood;
      if ( opts.Find( 'f', trainName, mood ) )
	opts.Delete( 'f' );
      else if ( opts.Find( 'i', treeName, mood ) )
	opts.Delete( 'i' );
      if ( !( treeName.empty() && trainName.empty() ) ){
	TimblAPI *run = new TimblAPI( &opts, it->first );
	bool result = false;
	if ( run && run->Valid() ){
	  if ( treeName.empty() ){
	    //	    cerr << "trainName = " << trainName << endl;
	    result = run->Learn( trainName );
	  }
	  else {
	    //	    cerr << "treeName = " << treeName << endl;
	    result = run->GetInstanceBase( treeName );
	  }
	}
	if ( result ){
	  experiments[it->first] = run;
	  cerr << "started experiment " << it->first 
	       << " with parameters: " << it->second << endl;
	}
	else {
	  cerr << "FAILED to start experiment " << it->first 
	       << " with parameters: " << it->second << endl;
	}
      }
      else {
	cerr << "missing '-i' or '-f' optiion in serverconfig file" << endl;
      }
      ++it;
    }
    int start = daemon( 0, logFile.empty() );

    if ( start < 0 ){
      cerr << "failed to daemonize error= " << strerror(errno) << endl;
      exit(1);
    };
    if ( !pidFile.empty() ){
      // we have a liftoff!
      // signal it to the world
      ofstream pid_file( pidFile.c_str() ) ;
      if ( !pid_file ){
	*Log(Mother->my_err())<< "unable to create pidfile:"<< pidFile << endl;
	*Log(Mother->my_err())<< "TimblServer NOT Started" << endl;
	exit(1);
      }
      else {
	pid_t pid = getpid();
	pid_file << pid << endl;
      }
    }
    // set the attributes 
    pthread_attr_t attr;
    if ( pthread_attr_init(&attr) ||
	 pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED ) ){
      *Log(Mother->my_err()) << "Threads: couldn't set attributes" << endl;
      exit(0);
    }
    *Log(Mother->my_log()) << "Starting Server on port:" << TCP_PORT << endl;

    pthread_t chld_thr;

    ServerSocket server;
    string portString = toString<int>(TCP_PORT);
    if ( !server.connect( portString ) ){
      *Log(Mother->my_err()) << "failed to start Server: " 
			     << server.getMessage() << endl;
      exit(0);
    }

    if ( !server.listen( 5 ) ) {
      // maximum of 5 pending requests
      *Log(Mother->my_err()) << server.getMessage() << endl;
      exit(0);
    }
    
    int failcount = 0;
    while( true ){ // waiting for connections loop
      signal( SIGPIPE, SIG_IGN );
      ServerSocket *newSocket = new ServerSocket();
      if ( !server.accept( *newSocket ) ){
	*Log(Mother->my_err()) << server.getMessage() << endl;
	if ( ++failcount > 20 ){
	  *Log(Mother->my_err()) << "accept failcount >20 " << endl;
	  *Log(Mother->my_err()) << "server stopped." << endl;
	  exit(EXIT_FAILURE);
	}
	else {
	  continue;  
	}
      }
      else {
	failcount = 0;
	*Log(Mother->my_log()) << "Accepting Connection #" 
			       << newSocket->getSockId()
			       << " from remote host: " 
			       << newSocket->getClientName() << endl;
	// create a new thread to process the incoming request 
	// (The thread will terminate itself when done processing
	// and release its socket handle)
	//
	childArgs args;
	args.Exp = Mother;
	args.socket = newSocket;
	args.experiments = &experiments;
	pthread_create( &chld_thr, &attr, do_chld2, (void *)&args );
      }
      // the server is now free to accept another socket request 
    }
    pthread_attr_destroy(&attr); 
  }
  
  void RunClient( istream& Input, ostream& Output, 
		  const string& NODE, const string& TCP_PORT, 
		  bool classify_mode ){
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
		Output << "Server is confused?? " << ResultLine << endl;
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
#endif // PTHREADS
} // namespace
