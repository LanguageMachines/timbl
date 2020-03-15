/*
  Copyright (c) 1998 - 2020
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

#include "timbl/TimblAPI.h"
#include <cstdlib>

int main(){
  std::string path = std::getenv( "topsrcdir" );
  std::cerr << path << std::endl;
  std::cerr << "version: " << Timbl::VersionName() << std::endl;
  Timbl::AlgorithmType alg = Timbl::charToAlg( '0'  );
  assert( alg == Timbl::IB1_a );
  Timbl::normType nor = Timbl::charToNorm( '0'  );
  assert( nor == Timbl::probabilityNorm );
  Timbl::WeightType w = Timbl::charToWeig( '0'  );
  assert( w == Timbl::No_w );
  Timbl::TimblAPI exp( "+vdi+db", "test1" );
  if ( exp.isValid() ){
    exp.Learn( path + "/demos/dimin.train" );
    if ( exp.isValid() ){
      exp.Test( path + "/demos/dimin.test", "dimin.out" );
      if ( exp.isValid() ){
	return EXIT_SUCCESS;
      }
    }
  }
  return EXIT_FAILURE;
}
