/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2013
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

#include <algorithm>
#include <string>
#include <iostream>

#include <cerrno>
#include <cfloat>
#include "ticcutils/StringOps.h"
#include "timbl/StringOps.h"

using namespace std;
namespace Timbl {
  
  string StrToCode( const string &par ){
    string In = TiCC::trim(par);
    string Out;
    string::const_iterator it = In.begin();
    while ( it != In.end() ){
      switch ( *it ){
      case ' ':
	Out += '\\';
	Out += '_';
	break;
      case '\t':
	Out += '\\';
	Out += 't';
	break;
      case '\\':
	Out += '\\';
	Out += '\\';
	break;
      default:
	Out += *it;
      }
      ++it;
    }
    return Out;
  }
  
  string CodeToStr( const string& in ){
    string out;
    string::const_iterator it = in.begin();
    while ( it != in.end() ){
      if ( *it == '\\' ){
	++it;
	if ( it == in.end() ){
	  out += '\\';
	  break;
	}
	else {
	  switch ( *it ){
	  case  '_':
	    out += ' ';
	    break;
	  case '\\':
	    out += '\\';
	    break;
	  case 't':
	    out += '\t';
	    break;
	  default:
	    out += '\\';
	    out += *it;
	  }
	  ++it;
	}
      }
      else
	out += *it++;
    }
    return out;
  }
  
  string string_tok( const string& s, 
		     string::size_type& pos,
		     const string& seps ){
    string::size_type b_pos = s.find_first_not_of( seps, pos ); 
    if ( b_pos != string::npos ){
      pos = s.find_first_of( seps, b_pos ); 
      if ( pos == string::npos )
	return string( s, b_pos );
      else
	return string( s, b_pos, pos - b_pos );
    }
    else {
      pos = string::npos;
    }
    return "";
  }
  
  bool nocase_cmp( char c1, char c2 ){
    return toupper(c1) == toupper(c2);
  }
  
  bool compare_nocase( const string& s1, const string& s2 ){
    if ( s1.size() == s2.size() &&
	 equal( s1.begin(), s1.end(), s2.begin(), nocase_cmp ) )
      return true;
    else
      return false;
  }
  
  bool compare_nocase_n( const string& s1, const string& s2 ){
    if ( s1.size() <= s2.size() &&
	 equal( s1.begin(), s1.end(), s2.begin(), nocase_cmp ) )
      return true;
    else
      return false;
  }

} // namespace Timbl
