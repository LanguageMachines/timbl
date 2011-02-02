/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2011
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

#ifndef STRING_OPS_H
#define STRING_OPS_H

#include <string>
#include <vector>
#include <sstream>
#include <exception>
#include <stdexcept>

namespace Timbl {
  std::string compress( const std::string& );
  size_t split_at( const std::string&, std::vector<std::string>&, 
		   const std::string& );
  size_t split_at_first_of( const std::string&, std::vector<std::string>&, 
			    const std::string& );
  inline size_t split( const std::string& s, std::vector<std::string>& vec ){
    return split_at_first_of( s, vec, " \r\t" );
  }
  
  std::string string_tok( const std::string&, 
			  std::string::size_type&,
			  const std::string& );
  
  bool compare_nocase( const std::string&, const std::string& );
  bool compare_nocase_n( const std::string&, const std::string& );
  
  std::string StrToCode( const std::string& );
  std::string CodeToStr( const std::string& );
  
  void lowercase( std::string& );
  void uppercase( std::string& );
  
  std::string encode( const std::string& );
  
  std::string format_nonascii( const std::string& );
  
}

#endif
