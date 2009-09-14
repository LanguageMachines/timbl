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
#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <cmath>
#include <cfloat>
#include <cassert>
#include <sys/time.h>

namespace Common {
  const double Epsilon = DBL_EPSILON;   // smallest x so that 1+x != 1
  const int DEFAULT_MAX_FEATS = 2500;   // default maximun number of Features
  
  std::string VersionInfo( bool = false );
  void ShowVersionInfo( std::ostream& );

  inline int look_ahead( std::istream &is ){
    int nc;
    while( is ){
      nc=is.peek();
      if ( !isspace(nc) )
	return nc;
      nc = is.get();
    }
    return -1;
  }
  
  inline void skip_spaces( std::istream &is ){
    int nc;
    while( is ){
      nc=is.peek();
      if ( !isspace(nc) )
	return;
      nc = is.get();
    }
  }
  
  inline double Log2(double number){
    // LOG base 2.
    if ( fabs(number) < Epsilon)
      return(0.0);
    return(log(number) / log(2.0));
  }

  class Timer {
  public:
    friend std::ostream& operator << ( std::ostream& os, const Timer& T );
    Timer(){ reset(); };
    void reset(){ myTime.tv_sec=0; myTime.tv_usec=0; };
    void start(){
      gettimeofday( &startTime, 0 );
    };
    void stop();
    unsigned int secs() { stop(); return myTime.tv_sec; };
    static std::string now();
  private:
    timeval startTime;
    timeval myTime;
  };

  
}
#endif
