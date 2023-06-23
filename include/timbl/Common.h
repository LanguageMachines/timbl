/*
  Copyright (c) 1998 - 2023
  ILK   - Tilburg University
  CLST  - Radboud University
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
      https://github.com/LanguageMachines/timbl/issues
  or send mail to:
      lamasoftware (at ) science.ru.nl

*/
#ifndef TIMBL_COMMON_H
#define TIMBL_COMMON_H

#include <iostream>
#include <limits>
#include <cmath>
#include <cassert>
#include <ctype.h>   // for isspace
#include <string>    // for string

namespace Common {
  const double Epsilon = std::numeric_limits<double>::epsilon();
  // smallest x so that 1+x != 1
  const int DEFAULT_MAX_FEATS = 2500;   // default maximun number of Features

  std::string Version();
  std::string VersionName();
  std::string BuildInfo();
  std::string VersionInfo( bool ); // obsolete

  inline int look_ahead( std::istream &is ){
    while( is ){
      int nc=is.peek();
      if ( !isspace(nc) )
	return nc;
      is.get();
    }
    return -1;
  }

  inline void skip_spaces( std::istream &is ){
    while( is ){
      int nc=is.peek();
      if ( !isspace(nc) )
	return;
      is.get();
    }
  }

  inline double Log2(double number){
    // LOG base 2.
    if ( fabs(number) < Epsilon)
      return(0.0);
    return log2(number);
  }

}
#endif
