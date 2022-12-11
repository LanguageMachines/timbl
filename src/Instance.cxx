/*
  Copyright (c) 1998 - 2022
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

#include <iostream>

#include "timbl/Types.h"
#include "timbl/Instance.h"

using namespace std;

namespace Timbl {

  Instance::Instance():
    TV(NULL),
    sample_weight(0.0),
    occ(1)
  {
  }

  Instance::~Instance(){
    clear();
  }

  void Instance::clear(){
    for ( auto& it : FV ){
      if ( it ){
	if ( it->isUnknown() ){
	  delete it;
	}
      }
      it = 0;
    }
    TV = 0;
    sample_weight = 0.0;
    occ = 1;
  }

  void Instance::Init( size_t len ){
    FV.resize( len, 0 );
  }

  ostream& operator<<( ostream& os, const Instance *I ){
    if ( I ){
      os << *I;
    }
    else {
      os << " Empty Instance";
    }
    return os;
  }

  ostream& operator<<( ostream& os, const Instance& I ){
    for ( const auto& it : I.FV ){
      os << it << ", ";
    }
    os << I.TV << " " << I.sample_weight;
    return os;
  }

}
