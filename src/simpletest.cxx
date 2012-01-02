/*
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

#include "timbl/TimblAPI.h"
#include <cstdlib>

int main(){
  std::string path = std::getenv( "topsrcdir" );
  std::cerr << path << std::endl;
  
  Timbl::TimblAPI exp( "+vdi+db", "test1" );
  if ( exp.isValid() ){
    exp.Learn( path + "/demos/dimin.train" );  
    if ( exp.isValid() ){
      exp.Test( path + "/demos/dimin.test", "dimin.out" );
      if ( exp.isValid() )
	return EXIT_SUCCESS;
    }
  }
  return EXIT_FAILURE;
}
