/*
  Copyright (c) 1998 - 2008
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

#include <ctime>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <typeinfo>
#include "timbl/LogStream.h"
#ifdef PTHREADS
#include <pthread.h>
#endif

#if defined __GNUC__
#define DARE_TO_OPTIMIZE
#endif

using std::ostream;
using std::streambuf;
using std::cerr;
using std::endl;
using std::bad_cast;

LogStream::LogStream( int ) : 
  ostream( static_cast<streambuf *>(0) ), 
  buf( cerr ),
  single_threaded_mode(false){
}

LogStream null_stream( 0 );

LogStream::LogStream() : 
  ostream( &buf ),
  buf( cerr, NULL, StampBoth ),
  single_threaded_mode(false) {
}

LogStream::LogStream( const char *message, LogFlag stamp ) : 
  ostream( &buf ),
  buf( cerr, message, stamp ),
  single_threaded_mode(false) {
}

LogStream::LogStream( ostream& as, const char *message, LogFlag stamp ) : 
  ostream( &buf ), 
  buf( as, message, stamp ),
  single_threaded_mode(false){
}

LogStream::LogStream( const LogStream& ls, 
		      const char *message, LogFlag stamp ): 
  ostream( &buf ),  
  buf( ls.buf.AssocStream(), 
       ls.buf.Message(), 
       stamp ),
  single_threaded_mode( ls.single_threaded_mode ){
  buf.Level( ls.buf.Level() );
  buf.Treshold( ls.buf.Treshold() );
  addmessage( message );
}

LogStream::LogStream( const LogStream& ls, const char *message ): 
  ostream( &buf ), 
  buf( ls.buf.AssocStream(), 
       ls.buf.Message(), 
       ls.buf.StampFlag() ),
  single_threaded_mode( ls.single_threaded_mode ){
  buf.Level( ls.buf.Level() );
  buf.Treshold( ls.buf.Treshold() );
  addmessage( message );
}


LogStream::LogStream( const LogStream *ls ):
  ostream( &buf ),
  buf( ls->buf.AssocStream(), 
       ls->buf.Message(), 
       ls->buf.StampFlag() ), 
  single_threaded_mode( ls->single_threaded_mode ){
  buf.Level( ls->buf.Level() );
  buf.Treshold( ls->buf.Treshold() );
}

void LogStream::addmessage( const char *s ){
  if ( s ){
    const char *tmp = buf.Message();
    if ( tmp ){
      char *new_m = (char *)malloc( (strlen(tmp) + strlen(s) + 1 ) );
      strcpy( new_m, tmp );
      strcat( new_m, s );
      buf.Message( new_m );
      free( new_m );
    }
  }
}

void LogStream::addmessage( const int i ){
  char m[32];
  sprintf( m, "-%d", i );
  addmessage( m );
}

static bool static_init = false;

bool LogStream::set_single_threaded_mode( ){
  if ( !static_init ){
    single_threaded_mode = true;
    return true;
  }
  else
    return false;
}

ostream& setlevel_sup( ostream& os, LogLevel l ){
  try {
    LogStream& tmp = dynamic_cast<LogStream&>(os);
    tmp.setlevel( l );
  }
  catch ( bad_cast ){
  }
  return os;
}

o_manip<LogLevel> setlevel( LogLevel l ){
  return o_manip<LogLevel>( &setlevel_sup, l );
}

ostream& settreshold_sup( ostream& os, LogLevel l ){
  try {
    LogStream& tmp = dynamic_cast<LogStream&>(os);
    tmp.settreshold( l );
  }
  catch ( bad_cast ){
  }
  return os;
}

o_manip<LogLevel> settreshold( LogLevel l ){
  return o_manip<LogLevel>( &settreshold_sup, l );
}

ostream& setstamp_sup( ostream& os, LogFlag f ){
  try {
    LogStream& tmp = dynamic_cast<LogStream&>(os);
    tmp.setstamp( f );
    }
  catch ( bad_cast ){
  }
  return os;
}

o_manip<LogFlag> setstamp( LogFlag f ){
  return o_manip<LogFlag>( &setstamp_sup, f );
}

ostream& setmess_sup( ostream& os, const char *m ){
  try {
    LogStream& tmp = dynamic_cast<LogStream&>(os);
    tmp.message( m );
    }
  catch ( bad_cast ){
  }
  return os;
}

o_manip<const char *> setmessage( const char *m ){
  return o_manip<const char*>( &setmess_sup, m );
}

ostream& addmess_sup( ostream& os, const char *m ){
  try {
    LogStream& tmp = dynamic_cast<LogStream&>(os);
    tmp.addmessage( m );
    }
  catch ( bad_cast ){
  }
  return os;
}

o_manip<const char *> addmessage( const char *m ){
  return o_manip<const char*>( &addmess_sup, m );
}

o_manip<const char *> addmessage( const int i ){
  static char m[32]; // assume we are within the mutex here
  sprintf( m, "-%d", i );
  return o_manip<const char*>( &addmess_sup, m );
}

ostream& write_sup( ostream& os, const char *m, const int l ){
  try {
    LogStream& tmp = dynamic_cast<LogStream&>(os);
    tmp.write( m, l );
  }
  catch ( bad_cast ){
  }
  return os;
}

o_manip_2<const char *, const int> write_buf( const char *m, const int l ){
  return o_manip_2<const char*, const int>( &write_sup, m, l );
}

#ifdef PTHREADS
#ifdef DO_SIMPLE
#ifdef __sgi__
static pthread_mutex_t global_logging_mutex = {PTHREAD_MUTEX_INITIALIZER};
#else
static pthread_mutex_t global_logging_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

inline bool init_mutex(){ 
  pthread_mutex_lock( &global_logging_mutex );
  return true;
}

inline void mutex_release(){ 
  pthread_mutex_unlock( &global_logging_mutex );
}
#else // not DO_SIMPLE
#ifdef __sgi__
pthread_mutex_t global_logging_mutex = {PTHREAD_MUTEX_INITIALIZER};
pthread_mutex_t global_lock_mutex = {PTHREAD_MUTEX_INITIALIZER};
#else
pthread_mutex_t global_logging_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t global_lock_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

struct lock_s { pthread_t id; int cnt; time_t tim; };

#define MAX_LOCKS 500

lock_s locks[MAX_LOCKS];

bool LogStream::Problems(){
  //  cerr << "test for problems" << endl;
  bool result = false;
  time_t lTime;
  time(&lTime);
  pthread_mutex_lock( &global_lock_mutex );
  for ( int i=0; i < MAX_LOCKS; i++ ){
    if ( locks[i].id != 0 &&
	 lTime - locks[i].tim > 30 ){
      result = true;
      cerr << "ALERT" << endl;
      cerr << "ALERT" << endl;
      cerr << "Thread " << locks[i].id 
	   << "is blocking our LogStreams since " << lTime - locks[i].tim
	   << " seconds!" << endl;
      cerr << "ALERT" << endl;
      cerr << "ALERT" << endl;
    }
  }
  pthread_mutex_unlock( &global_lock_mutex );
  return result;
}

inline int get_lock( pthread_t ID ){
  time_t lTime;
  time(&lTime);
  pthread_mutex_lock( &global_lock_mutex );
  int free_lock = -1;
  for ( int i=0; i < MAX_LOCKS; i++ ){
    if ( pthread_equal( locks[i].id, ID ) ){
      pthread_mutex_unlock( &global_lock_mutex );
      return i;
    }
    else if ( free_lock < 0 && locks[i].id == 0 ){
        free_lock = i;
    }
  }
  if ( free_lock < 0 ){
    throw( "LogStreams FATAL error: get_lock() failed " );
  }
  locks[free_lock].id = ID;
  locks[free_lock].cnt = 0;
  locks[free_lock].tim = lTime;
  pthread_mutex_unlock( &global_lock_mutex );
  return free_lock;
}

inline bool init_mutex(){
  if ( !static_init ){
    for (int i=0; i < MAX_LOCKS; i++ ) {
      locks[i].id = 0;
      locks[i].cnt = 0;
    }
    static_init = true;
  }
  //  cerr << "voor Lock door thread " << pthread_self() << endl;
  int pos = get_lock( pthread_self() );
  if ( locks[pos].cnt == 0 ){
    pthread_mutex_lock( &global_logging_mutex );
//      cerr << "Thread " << pthread_self()  << " locked [" << pos 
//  	 << "]" << endl;
  }
  locks[pos].cnt++;
  if ( locks[pos].cnt > 1 ){
//      cerr << "Thread " << pthread_self()  << " regained [" << pos 
//  	 << "] cnt = " << locks[pos].cnt << endl;
  }
  return static_init;
}

inline void mutex_release(){
  //  cerr << "voor UnLock door thread " << pthread_self() << endl;
  int pos = get_lock( pthread_self() );
  locks[pos].cnt--;
  if ( locks[pos].cnt < 0 ){
    throw( "LogStreams FATAL error: mutex_release() failed" );
  }
  if ( locks[pos].cnt > 0 ){
//      cerr << "Thread " << pthread_self()  << " still owns [" << pos 
//  	 << "] cnt = "<< locks[pos].cnt << endl;
  }
  if ( locks[pos].cnt == 0 ){
    locks[pos].id = 0;
    //    cerr << "Thread " << pthread_self()  << " unlocked [" << pos << "]" << endl;
    pthread_mutex_unlock( &global_logging_mutex );
  }
}

#endif // DO_SIMPLE
#else // no PTHREADS (unwise)
inline bool init_mutex(){ return true; }
inline void mutex_release(){ return; }
bool LogStream::Problems(){
  return false;
}
#endif

bool LogStream::IsBlocking(){
  if ( !bad() ){
    return getlevel() <= gettreshold();
  }
  else
    return true;
}

bool IsActive( LogStream &ls ){
  return !ls.IsBlocking();
}

bool IsActive( LogStream *ls ){
  return ls && !ls->IsBlocking();
}


Log::Log( LogStream *os ){
  if ( !os ){
    throw( "LogStreams FATAL error: No Stream supplied! " );
  }
  if ( os->single_threaded() || init_mutex() ){
    my_level = os->gettreshold();
    my_stream = os;
    os->settreshold( LogSilent );
  }
}

Log::Log( LogStream& os ){
  if ( os.single_threaded() || init_mutex() ){
    my_level = os.gettreshold();
    my_stream = &os;
    os.settreshold( LogSilent );
  }
}

Log::~Log(){
  my_stream->flush();
  my_stream->settreshold( my_level );
  if ( !my_stream->single_threaded() )
    mutex_release();
} 

LogStream& Log::operator *(){
#ifdef DARE_TO_OPTIMIZE
  if ( my_stream->getlevel() > my_stream->gettreshold() )
    return *my_stream; 
  else
    return null_stream;
#else
  return *my_stream;
#endif
}

Dbg::Dbg( LogStream *os ){
  if ( !os ){
    throw( "LogStreams FATAL error: No Stream supplied! " );
  }
  if ( os->single_threaded() || init_mutex() ){
    my_stream = os;
    my_level = os->gettreshold();
    os->settreshold( LogNormal );
  }
}

Dbg::Dbg( LogStream& os ){
  if ( os.single_threaded() || init_mutex() ){
    my_stream = &os;
    my_level = os.gettreshold();
    os.settreshold( LogNormal );
  }
}

Dbg::~Dbg(){
  my_stream->flush();
  my_stream->settreshold( my_level );
  if ( !my_stream->single_threaded() )
    mutex_release();
}

LogStream& Dbg::operator *() { 
#ifdef DARE_TO_OPTIMIZE
  if ( my_stream->getlevel() > my_stream->gettreshold() )
    return *my_stream; 
  else
    return null_stream;
#else
  return *my_stream;
#endif
}

xDbg::xDbg( LogStream *os ){
  if ( !os ){
    throw( "LogStreams FATAL error: No Stream supplied! " );
  }
  if ( os->single_threaded() || init_mutex() ){
    my_stream = os;
    my_level = os->gettreshold();
    os->settreshold( LogDebug );
  }
}

xDbg::xDbg( LogStream& os ){
  if ( os.single_threaded() || init_mutex() ){
    my_stream = &os;
    my_level = os.gettreshold();
    os.settreshold( LogDebug );
  }
}

xDbg::~xDbg(){
  my_stream->flush();
  my_stream->settreshold( my_level );
  if ( !my_stream->single_threaded() )
    mutex_release();
} 

LogStream& xDbg::operator *(){
#ifdef DARE_TO_OPTIMIZE
  if ( my_stream->getlevel() > my_stream->gettreshold() )
    return *my_stream; 
  else
    return null_stream;
#else
  return *my_stream;
#endif
}

xxDbg::xxDbg( LogStream *os ){
  if ( !os ){
    throw( "LogStreams FATAL error: No Stream supplied! " );
  }
  if ( os->single_threaded() || init_mutex() ){
    my_stream = os;
    my_level = os->gettreshold();
    os->settreshold( LogHeavy );
  }
}

xxDbg::xxDbg( LogStream& os ){
  if ( os.single_threaded() || init_mutex() ){
    my_stream = &os;
    my_level = os.gettreshold();
    os.settreshold( LogHeavy );
  }
}

xxDbg::~xxDbg(){
  my_stream->flush();
  my_stream->settreshold( my_level );
  if ( !my_stream->single_threaded() )
    mutex_release();
}
 
LogStream& xxDbg::operator *(){
#ifdef DARE_TO_OPTIMIZE
  if ( my_stream->getlevel() > my_stream->gettreshold() )
    return *my_stream; 
  else
    return null_stream;
#else
  return *my_stream;
#endif
}
