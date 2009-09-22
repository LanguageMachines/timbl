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

#ifndef FD_STREAM_H
#define FD_STREAM_H

#include <iostream>
#include <streambuf>

class fdoutbuf: public std::streambuf {
 protected:
  int fd; // file descriptor
 public:
 fdoutbuf( int _fd ): fd(_fd){};
 protected:
  virtual int overflow( int );
  virtual std::streamsize xsputn( const char *, std::streamsize );
};

class fdostream: public std::ostream {
 protected:
  fdoutbuf buf;
 public:
 fdostream( int fd ): std::ostream(&buf), buf(fd) {};
};

class fdinbuf: public std::streambuf {
 protected:
  int fd; // file descriptor
  static const int putbackSize = 4;
  static const int bufferSize = 512;
  char buffer[bufferSize];
 public:
  fdinbuf( int _fd );
 protected:
  virtual int underflow();
};

class fdistream: public std::istream {
 protected:
  fdinbuf buf;
 public:
 fdistream( int fd ): std::istream(&buf), buf(fd) {};
};

bool nb_getline( std::istream&, std::string&, int& );
bool nb_putline( std::ostream&, std::string&, int& );
#endif
