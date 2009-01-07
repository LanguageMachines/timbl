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
#include <sstream>
#include "timbl/Common.h"
#include "config.h"

extern "C" {
  int timbl_present(){ return 1; }
}

using namespace std;

namespace Common {

  void ShowVersionInfo( std::ostream& os, bool full ){
    os << VERSION;
    if ( full )
      os << ", compiled on " << __DATE__ << ", " << __TIME__;
  }
  
  string VersionInfo( bool full ){
    string result;
    ostringstream oss;
    ShowVersionInfo( oss, full );
    result = oss.str();
    return result; 
  }

}
