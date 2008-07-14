/*
  Copyright (c) 1998 - 2008
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
#include <string>
#include "timbl/TimblAPI.h"

using std::endl;
using std::cout;
using std::string;
using namespace Timbl;

int main(){
  TimblAPI *My_Experiment = new TimblAPI( "-a IB1 +vDI+DB+n +mM +k4 " , 
                                          "test5" );
  My_Experiment->Learn( "dimin.train" );  
  {
    string line =  "=,=,=,=,+,k,e,=,-,r,@,l,T";
    const neighborSet *neighbours1 = My_Experiment->classifyNS( line );
    if ( neighbours1 ){
      cout << "Classify OK on " << line << endl;
      cout << neighbours1;
    } else
      cout << "Classify failed on " << line << endl;
    neighborSet neighbours2;
    line = "+,zw,A,rt,-,k,O,p,-,n,O,n,E";
    if ( My_Experiment->classifyNS( line, neighbours2 ) ){
      cout << "Classify OK on " << line << endl;
      cout << neighbours2;
    } else
      cout << "Classify failed on " << line << endl;
    line = "+,z,O,n,-,d,A,xs,-,=,A,rm,P";
    const neighborSet *neighbours3 = My_Experiment->classifyNS( line );
    if ( neighbours3 ){
      cout << "Classify OK on " << line << endl;
      cout << neighbours3;
    } else
      cout << "Classify failed on " << line << endl;
    neighborSet uit2;
    {
      neighborSet uit;
      uit.setShowDistance(true);
      uit.setShowDistribution(true);
      cout << " before first merge " << endl;
      cout << uit;
      uit.merge( *neighbours1 );
      cout << " after first merge " << endl;
      cout << uit;
      uit.merge( *neighbours3 );
      cout << " after second merge " << endl;
      cout << uit;
      uit.merge( neighbours2 );
      cout << " after third merge " << endl;
      cout << uit;
      uit.truncate( 3 );
      cout << " after truncate " << endl;
      cout << uit;
      cout << " test assignment" << endl;
      uit2 = *neighbours1;
    }
    cout << "assignment result: " << endl;
    cout << uit2;
    {
      cout << " test copy construction" << endl;
      neighborSet uit(uit2);
      cout << "result: " << endl;
      cout << uit;
    }
    cout << "almost done!" << endl;
  }
  delete My_Experiment;
  cout << "done!" << endl;
}
