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

#if !defined __GNUC__ 
// no gcc 

#include <ctime>
#include <cstring>
#include <cstdio>

#include <typeinfo>
#ifdef __WIN32
#include <winsock2.h>
#include <sys/timeb.h>
void gettimeofday( timeval *tv, void *tz ){
  struct _timeb cur;
  _ftime(&cur);
  tv->tv_sec = cur.time;
  tv->tv_usec = cur.millitm * 1000;
}
#else
#include <sys/time.h>
#endif
#include "timbl/LogBuffer.h"


using std::ostream;
using std::streambuf;
using std::cerr;
using std::endl;

LogBuffer::LogBuffer( ostream& a, const char *mess, const LogFlag stamp ):
  ass_stream( &a ),
  stamp_flag( stamp ),
  in_sync( true ),
  level( LogNormal ),
  treshold_level( LogSilent ),
  ass_mess( NULL )
{
  if ( mess ){
    ass_mess = new char[strlen(mess)+1];
    strcpy( ass_mess, mess );
  }
}

LogBuffer::~LogBuffer(){
  sync();
  delete [] ass_mess;
}

inline long millitm() {
  struct timeval tp;
  gettimeofday(&tp,NULL);
  return tp.tv_usec/1000;
}

inline char *time_stamp( char *time_line, int size ){
  time_t lTime;
  struct tm *curtime;
  time(&lTime);
  struct tm tmp;
  curtime = localtime_r(&lTime,&tmp);
  strftime( time_line, size-5, "%Y%m%d:%H%M%S", curtime );
  sprintf( time_line+strlen(time_line), ":%03ld:", millitm() );
  return time_line;
}

//
// for a derived output stream, we must provide implementations for
// both overflow and sync.
//

int LogBuffer::overflow( int c ) {
  buffer_out();
  if ( level > treshold_level && c != '\r' )
    ass_stream->put( c );
  return c;
}

int LogBuffer::sync() {
  ass_stream->flush();
  in_sync = true;
  return 0;
}

void LogBuffer::buffer_out(){
  char time_line[50];
  if ( level > treshold_level ){
    // only output when we are on a high enough level
    // AND the usertest yields true
    if ( in_sync ) {
      // stamps and messages are only displayed when in sync
      // that is: when we have had a newline and NOT when we just
      // overflowed due to a long line
      if ( stamp_flag & StampTime ){
	*ass_stream << time_stamp( time_line, 50 );
      }
      if ( ass_mess && (stamp_flag & StampMessage) )
	*ass_stream << ass_mess << ":";
      in_sync = false;
    }
  }
}

//
// Getters and Setters for the private parts..
//

const char *LogBuffer::Message() const {
  return ass_mess;
}

void LogBuffer::Message( const char *s ){
  delete [] ass_mess;
  if ( s ){
    ass_mess = new char[strlen(s)+1];
    strcpy( ass_mess, s );
  }
  else
    ass_mess = NULL;
}

void LogBuffer::Treshold( LogLevel l ){ 
  if ( treshold_level != l ){
    treshold_level = l;
  }
}

LogLevel LogBuffer::Treshold() const { 
  return treshold_level;
}

void LogBuffer::Level( LogLevel l ){ 
  if ( level != l ){
    level = l;
  }
}

LogLevel LogBuffer::Level() const { 
  return level;
}

ostream& LogBuffer::AssocStream() const {
  return *ass_stream;
}

void LogBuffer::AssocStream( ostream& os ){
  ass_stream = &os;
}

void LogBuffer::StampFlag( const LogFlag b ){ 
  if ( stamp_flag != b ){
    stamp_flag = b;
  }
}

LogFlag LogBuffer::StampFlag() const { 
  return stamp_flag;
}

#else  // __GNUC__
// nothing!
#endif // __GNUC__
