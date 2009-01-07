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

#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>

using namespace std;

#ifdef PTHREADS
#include <map>
#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/MsgClass.h"
#include "timbl/LogStream.h"
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/Tree.h"
#include "timbl/IBtree.h"
#include "timbl/Instance.h"
#include "timbl/neighborSet.h"
#include "timbl/Statistics.h"
#include "timbl/BestArray.h"
#include "timbl/MBLClass.h"
#include "timbl/GetOptClass.h"
#include "timbl/TimblExperiment.h"
#include "timbl/ServerProcs.h"

using namespace Timbl;

inline void usage( char *name ){
  cerr << "Timbl Client V0.10" << endl
       << "For demonstration purposes only!" << endl
       << "Usage:" << endl
       << name << " NodeName PortNumber [InputFile [OutputFile [BATCH]]]"
       << endl;
}

int main(int argc, char *argv[] ){
  // the following trick makes it possible to parse lines from cin
  // as well from a user supplied file.
  istream *Input = &cin;
  ostream *Output = &cout;
  ifstream input_file;
  ofstream output_file;
  bool c_mode = false;
  if ( argc > 3 ){
    if ( (input_file.open( argv[3], ios::in ), !input_file.good() ) ){
      cerr << argv[0] << " - couldn't open inputfile " << argv[3] << endl;
      exit(1);
    }
    cout << "reading input from: " << argv[3] << endl;
    Input = &input_file;
    if ( argc > 4 ){
      if ( (output_file.open( argv[4], ios::out ), !output_file.good() ) ){
	cerr << argv[0] << " - couldn't open outputfile " << argv[4] << endl;
	exit(1);
      }
      cout << "writing output to: " << argv[4] << endl;
      Output = &output_file;
      if ( argc > 5 )
	c_mode = compare_nocase_n( "BATCH", argv[5] );
    }
  }
  else if ( argc < 3 ){
    usage( argv[0] );
    exit(1);
  }
  RunClient( *Input, *Output, argv[1], argv[2], c_mode );
  exit(0);
}

#else

int main(int argc, char *argv[] ){
  (void)argc;
  (void)argv;
  cerr << "No Client software is build." << endl;
}

#endif // PTHREADS

