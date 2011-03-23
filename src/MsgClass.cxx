/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2011
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

#include <iostream>
#include <string>
#include <stdexcept>
#include "timbl/MsgClass.h"

using std::cerr;
using std::endl;
using std::string;

namespace Timbl {
  
  void MsgClass::Info( const string& out_line ) const {
    cerr << out_line << endl;
  }

  void MsgClass::Warning( const string& out_line ) const {
    cerr << "Warning:" << out_line << endl;
  }

  void MsgClass::Error( const string& out_line ) const {
    cerr << "Error:" << out_line << endl;
  }

  void MsgClass::FatalError( const string& out_line ) const {
    cerr << "Fatal timbl Error:"
	 << out_line << endl
	 << "Please send a bugreport to timbl@uvt.nl" << endl
	 << "include enough information, like:" << endl
	 << "- Type of computer, type and version of OS, "
	 << "and type and version of the compiler" << endl
	 << "- Which Commands and switches were used" << endl
	 << "- Which input was used, and which output was produced" << endl;
    throw std::runtime_error( "aborted" );
  }
  
}
