/*
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
 
#include <iostream>
#include <fstream>

#include <cstdlib>
 
#include "timbl/TimblAPI.h"

using namespace std;
using namespace Timbl;

char inf[] = "./dimin.train";
char test_f[] = "./dimin.test";

int main(){
  string Bresult;
  double Distance;
  
  TimblAPI *Exp = new TimblAPI( "-a TRIBL" );
  Exp->SetOptions( "+vS +x -N30 -q2" );
  Exp->ShowOptions( cout );
  Exp->Learn( inf );
  ifstream testfile;
  string Buffer;
  testfile.open( test_f, ios::in );
  cout << "\nStart testing, using TRIBL" << endl;
  while ( getline( testfile, Buffer ) ){
    const TargetValue *tv = Exp->Classify( Buffer, Distance );
    if ( tv )
      cout << Buffer << "\t --> " << tv << " " << Distance << endl;
    else
      cout << Buffer << "\t --> (nill)" << endl;
  }
  testfile.close();
  delete Exp;
  Exp = new TimblAPI( "-a IB1" );
  Exp->SetOptions( "-vS" );
  Exp->ShowOptions( cout );
  Exp->Learn( inf );
  testfile.clear();
  testfile.open( test_f, ios::in );
  cout << "\nStart testing, using IB" << endl;
  while ( getline( testfile, Buffer ) ){
    if ( Exp->Classify( Buffer, Bresult, Distance ) ){
      cout << Buffer << "\t --> " << Bresult << " " << Distance << endl;
    } 
    else
      cout << Buffer << "\t --> (nill)" << endl;
  }
  testfile.close();
  delete Exp;
  Exp = new TimblAPI( "-a IGTREE" );
  Exp->SetOptions( "-vS -N40" );
  Exp->ShowOptions( cout );
  Exp->Learn( inf );
  Exp->WriteInstanceBase( "dimin.tree" );
  Exp->SaveWeights( "dimin.wgt" );
  cout << "\nStart testing, using IGTree, first run" << endl;
  testfile.clear();
  testfile.open( test_f, ios::in );
  while ( getline( testfile, Buffer ) ){ 
    if ( Exp->Classify( Buffer, Bresult, Distance ) ){ 
      cout << Buffer << "\t --> " << Bresult << " " << Distance << endl;
    } 
    else
      cout << Buffer << "\t --> (nill)" << endl;
  }
  testfile.close();
  delete Exp;
  Exp = new TimblAPI( "-a IGTREE" );
  Exp->SetOptions( "-vS" );
  Exp->ShowOptions( cout );
  Exp->GetInstanceBase( "dimin.tree" );
  Exp->GetWeights( "dimin.wgt" );
  cout << "\nStart testing, using IGTree, second run, (retrieved Tree)" << endl;
  testfile.clear();
  testfile.open( test_f, ios::in );
  while ( getline( testfile, Buffer ) ){
    if ( Exp->Classify( Buffer, Bresult, Distance ) ){ 
      cout << Buffer << "\t --> " << Bresult << " " << Distance << endl;
    } 
    else
      cout << Buffer << "\t --> (nill)" << endl;
  }
  testfile.close();
  exit(1);
}
