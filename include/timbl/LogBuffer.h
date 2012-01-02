/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2012
  ILK   - Tilburg University
  CLiPS - University of Antwerp
 
  This file is part of timbl

  timbl is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  timbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      http://ilk.uvt.nl/software.html
  or send mail to:
      timbl@uvt.nl
*/
#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include <ctime>
#include <cstdio>
#include <string>
#include <cstring>
#include <typeinfo>
#include <iomanip>
#include <iostream>
#include <sys/time.h>

enum LogLevel{ LogSilent, LogNormal, LogDebug, LogHeavy, LogExtreme };
enum LogFlag { NoStamp=0, StampTime=1, StampMessage=2, StampBoth=3 };

template <class charT, class traits = std::char_traits<charT> >
  class basic_log_buffer : public std::basic_streambuf<charT, traits> {
 public:
 basic_log_buffer( std::basic_ostream<charT,traits>&, const std::string& = "", 
		   const LogFlag = StampBoth );
  ~basic_log_buffer();
  //
  // setters/getters
  LogLevel Level() const;
  void Level( const LogLevel l );
  LogLevel Threshold() const;
  void Threshold( const LogLevel l );
 const std::string& Message() const;
 void Message( const std::string& );
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
  LogLevel threshold_level;
  std::string ass_mess;
  void buffer_out();
  // prohibit copying and assignment
  basic_log_buffer( const basic_log_buffer& );
  basic_log_buffer& operator=( const basic_log_buffer& );
};

typedef basic_log_buffer<char, std::char_traits<char> > LogBuffer;
typedef basic_log_buffer<wchar_t, std::char_traits<wchar_t> > wLogBuffer;

template <class charT, class traits >
basic_log_buffer<charT,traits>::basic_log_buffer( std::basic_ostream<charT,traits>& a,
						  const std::string& mess, 
						  const LogFlag stamp ) {
  ass_stream = &a;
  ass_mess = mess;
  stamp_flag = stamp;
  in_sync = true;
  level = LogNormal;
  threshold_level = LogSilent;
}

template <class charT, class traits >
basic_log_buffer<charT,traits>::~basic_log_buffer(){
  sync();
}

inline long millitm() {
  struct timeval tp;
  gettimeofday(&tp,NULL);
  return tp.tv_usec/1000;
}

inline std::string time_stamp(){
  char time_line[50];
  time_t lTime;
  struct tm *curtime;
  time(&lTime);
  struct tm tmp;
  curtime = localtime_r(&lTime,&tmp);
  strftime( time_line, 45, "%Y%m%d:%H%M%S", curtime );
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
  if ( level > threshold_level && c != '\r' ){
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
  if ( level > threshold_level ){
    // only output when we are on a high enough level
    if ( in_sync ) {
      // stamps and messages are only displayed when in sync
      // that is: when we have had a newline and NOT when we just
      // overflowed due to a long line
      if ( stamp_flag & StampTime ){
	*ass_stream << time_stamp();
      }
      if ( !ass_mess.empty() && ( stamp_flag & StampMessage ) )
	*ass_stream << ass_mess << ":";
      in_sync = false;
    }
  }
}

//
// Getters and Setters for the private parts..
//

template <class charT, class traits >
  const std::string& basic_log_buffer<charT,traits>::Message() const {
  return ass_mess;
}

template <class charT, class traits >
  void basic_log_buffer<charT,traits>::Message( const std::string& s ){
  ass_mess = s;
}

template <class charT, class traits >
void basic_log_buffer<charT,traits>::Threshold( LogLevel l ){ 
  if ( threshold_level != l ){
    threshold_level = l;
  }
}

template <class charT, class traits >
LogLevel basic_log_buffer<charT,traits>::Threshold() const { 
  return threshold_level;
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

#endif // LOGBUFFER_H
