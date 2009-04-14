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

#include "config.h" // for TIMBL_SOCKLEN_T and HAVE_GETADDRINFO
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
#include "timbl/MBLClass.h"
#include "timbl/GetOptClass.h"
#include "timbl/TimblExperiment.h"
#include "timbl/SocketBasics.h"
#include "timbl/ServerProcs.h"

using namespace std;

namespace Timbl {
  using namespace SocketProcs;

#ifndef PTHREADS
  bool do_command( const string&, TimblExperiment *Exp, int ){
    *Log(Exp->my_err()) << "Server Mode not available" << endl;
    return false;
  }
  int ClassifyFromSocket( TimblExperiment *Exp, int  ){
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
  
  bool do_command( const string& Line, TimblExperiment *Exp, int Sock_Num ){
    bool go_on = true;
    if ( Exp->SetOptions( Line ) ){
      if ( Exp->ServerVerbosity() & CLIENTDEBUG )
	*Log(Exp->my_log()) << Sock_Num << ": Command :" << Line << endl;
      if ( Exp->ConfirmOptions() )
	go_on = write_line( Sock_Num, "OK\n" );
      else {
      }
    }
    else {
      if ( Exp->ServerVerbosity() & CLIENTDEBUG )
	*Log(Exp->my_log()) << Sock_Num << ": Don't understand '" 
			    << Line << "'" << endl;
    }
    return go_on;
  }
 
  int ClassifyFromSocket( TimblExperiment *Exp, int Sock_Num ){ 
    string Line, Command, Param;
    double Distance;
    int result = 0;
    bool go_on = true;
    while ( go_on && read_line( Sock_Num, Line, TCP_BUFFER_SIZE ) ){
      *Dbg(Exp->my_debug()) << "Line=" << Line << endl;
      Split( Line, Command, Param );
      switch ( check_command(Command) ){
      case Set:
	go_on = do_command( Param, Exp, Sock_Num );
	break;
      case Query:
	go_on = Exp->ShowSettings( *Log(Exp->my_log()) );
	break;
      case Exit:
	go_on = write_line( Sock_Num, "OK" ) && 
	  write_line( Sock_Num, " Closing\n" );
	return result;
	break;
      case Classify:{
	result++;
	string SLine = Param;
	string Distrib;
	string Answer;
	if ( Exp->Classify( SLine, Answer, Distrib, Distance ) ){
	  if ( Exp->ServerVerbosity() & CLIENTDEBUG )
	    *Log(Exp->my_log()) << Sock_Num << ": " << SLine << " --> " 
				<< Answer << " " << Distrib 
				<< " " << Distance << endl;
	  go_on = write_line( Sock_Num, "CATEGORY {" ) &&
	    write_line( Sock_Num, Answer ) &&
	    write_line( Sock_Num, "}" );
	  if ( go_on ){
	    if ( Exp->Verbosity(DISTRIB) ){
	      go_on = write_line( Sock_Num, " DISTRIBUTION " ) &&
		write_line( Sock_Num, Distrib );
	    }
	    if ( go_on ){
	      if ( Exp->Verbosity(DISTANCE) ){
		go_on = write_line( Sock_Num, " DISTANCE {" ) &&
		  write_line( Sock_Num, toString<double>(Distance) ) &&
		  write_line( Sock_Num, "}" );
	      }
	      if ( go_on ){
		if ( Exp->Verbosity(NEAR_N) ){
		  ostringstream tmp;
		  tmp << " NEIGHBORS\n";
		  Exp->showBestNeighbors( tmp );
		  tmp << "ENDNEIGHBORS";
		  go_on = write_line( Sock_Num, tmp.str() );
		}
	      }
	    }
	  }
	  if ( go_on )
	    go_on = write_line( Sock_Num, "\n" );
	}
	else {
	  if ( Exp->ServerVerbosity() & CLIENTDEBUG )
	    *Log(Exp->my_log()) << Sock_Num << ": Classify Failed on '" 
				<< SLine << "'" << endl;
	  
	}
      }
      break;
      case Comment:
	go_on = write_line( Sock_Num, "SKIP " ) &&
	  write_line( Sock_Num, Line ) &&
	  write_line( Sock_Num, "\n" );
	break;
      default:
	if ( Exp->ServerVerbosity() & CLIENTDEBUG )
	  *Log(Exp->my_log()) << Sock_Num << ": Don't understand '" 
			      << Line << "'" << endl;
	go_on = write_line( Sock_Num, "ERROR { Illegal instruction:'" ) &&
	  write_line( Sock_Num, Command ) &&
	  write_line( Sock_Num, "' in line:" ) &&
	  write_line( Sock_Num, Line ) &&
	  write_line( Sock_Num, "}\n" );
	break;
      }
    }
    return result;
  }
  
  void BrokenPipeChildFun( int Signal ){
    if ( Signal == SIGPIPE ){
      signal( SIGPIPE, BrokenPipeChildFun );
    }
  }


  void show_results(  ostream& os, time_t before, time_t after, int nw ){
    os << "Thread " << pthread_self() << ", terminated at: " 
       << asctime( localtime( &after ) )
       << "Total time used in this thread: " 
	   << after - before 
       << " sec, " << nw << " instances processed " ;
    if ( (after - before) > 0 )
      os << " (" << nw/(after - before) << " instances/sec)";
    os << endl;
  }


  // ***** This is the routine that is executed from a new thread *******
  void *do_chld( void *arg ){
    TimblExperiment *Exp = (TimblExperiment*)arg;
    int     mysockfd = Exp->TcpSocket();
    static int service_count=0;
#ifdef __sgi__
    static pthread_mutex_t my_lock = {PTHREAD_MUTEX_INITIALIZER};
#else
    static pthread_mutex_t my_lock = PTHREAD_MUTEX_INITIALIZER;
#endif
    pthread_mutex_lock(&my_lock);
    // use a mutex to update the global service counter
    service_count++;
    if ( service_count > Exp->Max_Connections() ){
      write_line( mysockfd, "Maximum connections exceeded\n" );
      write_line( mysockfd, "try again later...\n" );
      pthread_mutex_unlock( &my_lock );
      cerr << "Thread " << pthread_self() << " refused " << endl;
    }
    else {
      pthread_mutex_unlock( &my_lock );
      // Greeting message for the client
      //
      //
      write_line( mysockfd, "Welcome to the Timbl server.\n" );
      // process the test material
      // and do the timing
      //
      time_t timebefore, timeafter;
      time( &timebefore );
      // report connection to the server terminal
      //
      char line[256];
      sprintf( line, "Thread %u, on Socket %d", (unsigned int)pthread_self(), mysockfd );
      Exp->my_debug().message( line );
      *Log(Exp->my_log()) << line << ", started at: " 
			  << asctime( localtime( &timebefore) ) << endl;  
      
      signal( SIGPIPE, BrokenPipeChildFun );
      int nw = ClassifyFromSocket( Exp, mysockfd );
      time( &timeafter );
      show_results(  *Log(Exp->my_log()), timebefore, timeafter, nw );
      //
      pthread_mutex_lock(&my_lock);
      // use a mutex to update and display the global service counter
      *Log(Exp->my_log()) << "Socket Total = " << --service_count << endl;
      pthread_mutex_unlock(&my_lock);
      // close the socket and exit this thread
      //
      if ( close( mysockfd ) < 0 ){
	cerr << "closing problems on " << mysockfd
	     << " (" << strerror(errno) <<  ")" << endl;
      };
      delete Exp;
    }
    return NULL;
  }

  void BrokenPipeFun( int Signal ){
    if ( Signal == SIGPIPE ){
      signal( SIGPIPE, SIG_IGN );
    }
  }
  
#ifndef HAVE_GETADDRINFO
  void show_connection( ostream& os, struct hostent *host, int sock ){
    os << "Accepting Connection #" << sock
       << " from remote host: " << host->h_name;
    char **p;
    for (p = host->h_addr_list; *p != 0; p++) {
      struct in_addr in;
      (void) memcpy(&in.s_addr, *p, sizeof (in.s_addr));
      os << " [" << inet_ntoa(in) << "]";
    }
    os << endl;
  }

  void RunServer( TimblExperiment *Mother, int TCP_PORT ){
    string pidFile = Mother->pidFile;
    string logFile = Mother->logFile;
    if ( !pidFile.empty() ){
      // check validity of pidfile
      if ( pidFile[0] != '/' ) // make sure the path is absolute
	pidFile = '/' + pidFile;
      unlink( pidFile.c_str() );
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
#if defined( __sgi__ )
    int start = _daemonize( 0, -1, -1, -1 );
#else
    int start = daemon( 0, 0 );
#endif
    if ( start < 0 ){
      cerr << "failed to daemonize error= " << strerror(errno) << endl;
      exit(1);
    };

    if ( !pidFile.empty() ){
      // we have a liftoff!
      // signal it to the world
      ofstream pid_file( pidFile.c_str() ) ;
      if ( !pid_file ){
	*Log(Mother->my_err()) << "unable to create pidfile:"<< pidFile << endl;
	*Log(Mother->my_err()) << "TimblServer NOT Started" << endl;
	exit(1);
      }
      else {
	pid_t pid = getpid();
	pid_file << pid << endl;
      }
    }

    int    sockfd, newsockfd;
    TIMBL_SOCKLEN_T clilen, remlen;
    struct sockaddr_in cli_addr, serv_addr, rem_addr;
    struct hostent *host;
    pthread_t chld_thr;
    pthread_attr_t attr;
    clilen = sizeof(cli_addr);
    remlen = sizeof(rem_addr);
    // set the attributes
    if ( pthread_attr_init(&attr) ||
         pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED ) ){
      *Log(Mother->my_err()) << "Threads: couldn't set attributes" << endl;
      exit(0);
    }
    // start up server
    //
    *Log(Mother->my_log()) << "Starting Server on port:" << TCP_PORT << endl;
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
      *Log(Mother->my_err()) << "server: can't open stream socket" << endl;
      exit(0);
    }
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(TCP_PORT);

    int val = 1;
    setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR,
                (void *)&val, sizeof(val) );
    val = 1;
    setsockopt( sockfd, IPPROTO_TCP, TCP_NODELAY,
                (void *)&val, sizeof(val) );
    if( bind( sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr) ) < 0 ||
        listen(sockfd, 5) < 0 ) {
      int eno = errno;
      // maximum of 5 pending requests
      *Log(Mother->my_err()) << "server: can't bind local address"
                             << " (" << strerror(eno) << ")" << endl;
      exit(0);
    }

    int failcount = 0;
    while( true ){ // waiting for connections loop
      signal( SIGPIPE, SIG_IGN );
      newsockfd = accept( sockfd, (struct sockaddr *)&cli_addr, &clilen );
      if( newsockfd < 0 ){
        int err = errno;
        *Log(Mother->my_err()) << "accept fails, errno= " << err
                               << " (" << strerror(err) << ")" << endl;
        if ( ++failcount > 20 ){
          *Log(Mother->my_err()) << "accept failcount >20 " << endl;
          *Log(Mother->my_err()) << "server stopped." << endl;
          close(sockfd);
          exit(EXIT_FAILURE);
        }
        else {
          continue;
        }
      }
      else {
        failcount = 0;
        if ( getpeername( newsockfd,
                          (struct sockaddr *)&rem_addr,
                          &remlen ) < 0 ){
          int err = errno;
          *Log(Mother->my_err()) << "getpeername, "  << strerror(err) << endl;
        }else if ( (host = gethostbyaddr( (char *)&rem_addr.sin_addr,
                                          sizeof rem_addr.sin_addr,
                                          AF_INET) ) == NULL ){
          int err=errno;
          *Log(Mother->my_err()) << "gethostbyadd " << strerror(err) << endl;
        }
        else {
          show_connection( *Log(Mother->my_log()), host, newsockfd );
        }
        // create a new thread to process the incoming request
        // (The thread will terminate itself when done processing
        // and release its socket handle)
        //
        *Dbg(Mother->my_debug()) << " Voor Create Client " << endl;
        TimblExperiment *Chld = Mother->CreateClient( newsockfd );
        *Dbg(Mother->my_debug()) << " Na Create Client " << endl;
        *Dbg(Chld->my_debug()) << "voor pthread_create " << endl;
        pthread_create( &chld_thr, &attr, do_chld, (void *)Chld );
      }
      // the server is now free to accept another socket request
    }
    pthread_attr_destroy(&attr);
  }

#else
  void show_connection( ostream& os, int sock, 
			const sockaddr *addr, TIMBL_SOCKLEN_T len ){

    string result;
    char host_name[NI_MAXHOST];
    int err = getnameinfo( addr,
			   len,
			   host_name, sizeof(host_name),
			   0, 0,
			   0 );
    if ( err != 0 ){
      result = string(" failed: getnameinfo ") + strerror(errno);
    }
    else {
      result = host_name;
    }
    err = getnameinfo( addr,
		       len,
		       host_name, sizeof(host_name),
		       0, 0,
		       NI_NUMERICHOST );
    if ( err == 0 ){
      result += string(" [") + host_name + "]";
    }
    os << "Accepting Connection #" << sock
       << " from remote host: " << result << endl;
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
#if defined( __sgi__ )
    int start = _daemonize( 0, -1, -1, -1 );
#else
    int start = daemon( 0, 0 );
#endif
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

    int newsockfd;
    pthread_t chld_thr;

    struct addrinfo hints, *res, *resSave;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags    = AI_PASSIVE;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    string service = toString<int>(TCP_PORT);
    int status = getaddrinfo( 0, service.c_str(), &hints, &res);
    if ( status != 0) {
      cerr << "getaddrinfo error:: [" << gai_strerror(status) << "]\n" << endl;
    }
    
    resSave = res;
    int sockfd = -1;
    // try to start up server
    // 
    while ( res ){
      sockfd = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
      if ( sockfd >= 0 ){
	int val = 1;
	if ( setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, 
			 (void *)&val, sizeof(val) ) == 0 ){
	  val = 1;
	  if ( setsockopt( sockfd, IPPROTO_TCP, TCP_NODELAY, 
			   (void *)&val, sizeof(val) ) == 0 ){
	    if ( bind( sockfd, res->ai_addr, res->ai_addrlen ) == 0 )
	      break;
	  }
	}
	status = errno;
	close( sockfd );
	sockfd = -1;
      }
      res = res->ai_next;
    }
    
    freeaddrinfo( resSave );
    if ( sockfd < 0 ){
      *Log(Mother->my_err()) << "failed to start Server " << endl;
      exit(0);
    }
    
    if ( listen(sockfd, 5) < 0 ) {
      int eno = errno;
      // maximum of 5 pending requests
      *Log(Mother->my_err()) << "server: can't bind local address" 
			     << " (" << strerror(eno) << ")" << endl; 
      exit(0);
    }
    
    int failcount = 0;
    while( true ){ // waiting for connections loop
      signal( SIGPIPE, SIG_IGN );
      struct sockaddr_storage cli_addr;
      TIMBL_SOCKLEN_T clilen = sizeof(cli_addr);
      newsockfd = accept( sockfd, (struct sockaddr *)&cli_addr, &clilen );
      if( newsockfd < 0 ){
	int err = errno;
	*Log(Mother->my_err()) << "accept fails, errno= " << err
			       << " (" << strerror(err) << ")" << endl;
	if ( ++failcount > 20 ){
	  *Log(Mother->my_err()) << "accept failcount >20 " << endl;
	  *Log(Mother->my_err()) << "server stopped." << endl;
	  close(sockfd);
	  exit(EXIT_FAILURE);
	}
	else {
	  continue;  
	}
      }
      else {
	failcount = 0;
	show_connection( *Log(Mother->my_log()), newsockfd, 
			 (struct sockaddr *)&cli_addr, clilen );
	// create a new thread to process the incoming request 
	// (The thread will terminate itself when done processing
	// and release its socket handle)
	//
	*Dbg(Mother->my_debug()) << " Voor Create Client " << endl;
	TimblExperiment *Chld = Mother->CreateClient( newsockfd );
	*Dbg(Mother->my_debug()) << " Na Create Client " << endl;
	*Dbg(Chld->my_debug()) << "voor pthread_create " << endl;
	pthread_create( &chld_thr, &attr, do_chld, (void *)Chld );
      }
      // the server is now free to accept another socket request 
    }
    pthread_attr_destroy(&attr); 
  }
#endif  
  
  void RunClient( istream& Input, ostream& Output, 
		  const string& NODE, const string& TCP_PORT, 
		  bool classify_mode ){
    bool Stop_C_Flag = false;
    int    sockfd;
    cout << "Starting Client on node:" << NODE << ", port:" 
	 << TCP_PORT << endl;

    sockfd = make_connection( NODE, TCP_PORT );
    if ( sockfd > 0 ){
      string TestLine, ResultLine;
      string Code, Rest;
      if ( read_line( sockfd, ResultLine, TCP_BUFFER_SIZE ) ){
	cout << ResultLine << endl;
	cout << "Start entering commands please:" << endl;
	while( !Stop_C_Flag &&
	       getline( Input, TestLine ) ){ 
	  if ( classify_mode )
	    write_line( sockfd, "c " );
	  if ( write_line( sockfd, TestLine ) &&
	       write_line( sockfd, "\n" ) ){
	  repeat:
	    if ( read_line( sockfd, ResultLine, TCP_BUFFER_SIZE ) ){
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
		  while ( read_line( sockfd, ResultLine, TCP_BUFFER_SIZE ) ){
		    Split( ResultLine, Code, Rest );
		    Output << ResultLine << endl;
		    if ( get_code( Code ) == EndNeighbors )
		      break;
		  }
		break;
	      }
	      case Status:
		Output << ResultLine << endl;
		while ( read_line( sockfd, ResultLine, TCP_BUFFER_SIZE ) ){
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
      close( sockfd );
    }
  }
#endif // PTHREADS
} // namespace
