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
#ifndef LOGBUFFER_H
#define LOGBUFFER_H

enum LogLevel{ LogSilent, LogNormal, LogDebug, LogHeavy, LogExtreme };
enum LogFlag { NoStamp=0, StampTime=1, StampMessage=2, StampBoth=3 };

#if !defined __GNUC__ 
// braindead gcc
#include <iomanip>
#include <iostream>

class LogBuffer : public std::streambuf {
 public:
  LogBuffer( std::ostream& , const char * = NULL, 
	     const LogFlag = StampBoth );
  ~LogBuffer();
  //
  // setters/getters
  LogLevel Level() const;
  void Level( const LogLevel l );
  LogLevel Treshold() const;
  void Treshold( const LogLevel l );
  const char *Message() const;
  void Message( const char* );
  LogFlag StampFlag() const;
  void StampFlag( const LogFlag );
  std::ostream& AssocStream() const;
  void AssocStream( std::ostream & );
 protected:
  int sync();
  int overflow( int );
 private:
  std::ostream *ass_stream;
  LogFlag stamp_flag;
  bool in_sync;
  LogLevel level;
  LogLevel treshold_level;
  char *ass_mess;
  void buffer_out();
  // prohibit copying and assignment
  LogBuffer( const LogBuffer& );
  LogBuffer& operator=( const LogBuffer& );
};

#else // __GNUC__

#include <ctime>
#include <cstring>
#include <cstdio>
#include <typeinfo>
#include <iomanip>
#include <iostream>
#include <sys/time.h>

template <class charT, class traits = std::char_traits<charT> >
class basic_log_buffer : public std::basic_streambuf<charT, traits> {
 public:
  basic_log_buffer( std::basic_ostream<charT,traits>&, const char * = NULL, 
		    const LogFlag = StampBoth );
  ~basic_log_buffer();
  //
  // setters/getters
  LogLevel Level() const;
  void Level( const LogLevel l );
  LogLevel Treshold() const;
  void Treshold( const LogLevel l );
  const char *Message() const;
  void Message( const char* );
  std::basic_ostream<charT,traits>& AssocStream() const;
  void AssocStream( std::basic_ostream<charT,traits>& );
  LogFlag StampFlag() const;
  void StampFlag( const LogFlag );
 protected:
  int sync();
  int overflow( int );
 private:
  std::basic_ostream<charT,traits> *ass_stream;
  LogFlag stamp_flag;
  bool in_sync;
  LogLevel level;
  LogLevel treshold_level;
  char *ass_mess;
  void buffer_out();
  // prohibit copying and assignment
  basic_log_buffer( const basic_log_buffer& );
  basic_log_buffer& operator=( const basic_log_buffer& );
};

typedef basic_log_buffer<char, std::char_traits<char> > LogBuffer;
typedef basic_log_buffer<wchar_t, std::char_traits<wchar_t> > wLogBuffer;

template <class charT, class traits >
basic_log_buffer<charT,traits>::basic_log_buffer( std::basic_ostream<charT,traits>& a,
						  const char *mess, 
						  const LogFlag stamp ) {
  ass_stream = &a;
  if ( mess ){
    ass_mess = new char[strlen(mess)+1];
    strcpy( ass_mess, mess );
  }
  else
    ass_mess = NULL;
  stamp_flag = stamp;
  in_sync = true;
  level = LogNormal;
  treshold_level = LogSilent;
}

template <class charT, class traits >
basic_log_buffer<charT,traits>::~basic_log_buffer(){
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
// both use a helper function buffer_out to do the real work.
//

template <class charT, class traits >
int basic_log_buffer<charT,traits>::overflow( int c ) {
  buffer_out();
  if ( level > treshold_level && c != '\r' ){
    if ( c != EOF ){
      char z = static_cast<char>(c);
      ass_stream->put( z );
    }
    else
      return EOF;
  }
  return c;
}

template <class charT, class traits >
int basic_log_buffer<charT,traits>::sync() {
  ass_stream->flush();
  in_sync = true;
  return 0;
}

template <class charT, class traits >
void basic_log_buffer<charT,traits>::buffer_out(){
  char time_line[50];
  if ( level > treshold_level ){
    // only output when we are on a high enough level
    if ( in_sync ) {
      // stamps and messages are only displayed when in sync
      // that is: when we have had a newline and NOT when we just
      // overflowed due to a long line
      if ( stamp_flag & StampTime ){
	*ass_stream << time_stamp( time_line, 50 );
      }
      if ( ass_mess && ( stamp_flag & StampMessage ) )
	*ass_stream << ass_mess << ":";
      in_sync = false;
    }
  }
}

//
// Getters and Setters for the private parts..
//

template <class charT, class traits >
const char *basic_log_buffer<charT,traits>::Message() const {
  return ass_mess;
}

template <class charT, class traits >
void basic_log_buffer<charT,traits>::Message( const char *s ){
  delete [] ass_mess;
  if ( s ){
    ass_mess = new char[strlen(s)+1];
    strcpy( ass_mess, s );
  }
  else
    ass_mess = NULL;
}

template <class charT, class traits >
void basic_log_buffer<charT,traits>::Treshold( LogLevel l ){ 
  if ( treshold_level != l ){
    treshold_level = l;
  }
}

template <class charT, class traits >
LogLevel basic_log_buffer<charT,traits>::Treshold() const { 
  return treshold_level;
}

template <class charT, class traits >
void basic_log_buffer<charT,traits>::Level( LogLevel l ){ 
  if ( level != l ){
    level = l;
  }
}

template <class charT, class traits >
LogLevel basic_log_buffer<charT,traits>::Level() const { 
  return level;
}

template <class charT, class traits >
std::basic_ostream<charT,traits>& basic_log_buffer<charT,traits>::AssocStream() const {
  return *ass_stream;
}

template <class charT, class traits >
void basic_log_buffer<charT,traits>::AssocStream( std::basic_ostream<charT,traits>& os ){
  ass_stream = &os;
}

template <class charT, class traits >
void basic_log_buffer<charT,traits>::StampFlag( const LogFlag b ){ 
  if ( stamp_flag != b ){
    stamp_flag = b;
  }
}

template <class charT, class traits >
LogFlag basic_log_buffer<charT,traits>::StampFlag() const { 
  return stamp_flag;
}

#endif // __GNUC__

#endif // LOGBUFFER_H
