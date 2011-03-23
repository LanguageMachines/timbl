/*
  Copyright (c) 1998 - 2011
  ILK  -  Tilburg University
  CNTS -  University of Antwerp
 
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

#include <cstdlib>

#include <iostream>
#include "timbl/TimblAPI.h"
using namespace Timbl;

int main(){
  TimblAPI *My_Experiment = new TimblAPI( "-a IB1 +vDI+DB +mM" , 
                                          "test4" );
  My_Experiment->ShowSettings( std::cout );
  My_Experiment->Learn( "dimin.train" );  
  My_Experiment->Test( "dimin.test", "inc1.out" );
  My_Experiment->SaveWeights( "wg.1.wgt" );  
  My_Experiment->WriteArrays( "arr.1.arr" );  
  My_Experiment->Increment( "=,=,=,=,+,k,e,=,-,r,@,l,T" );  
  My_Experiment->Test( "dimin.test", "inc2.out" );
  My_Experiment->SaveWeights( "wg.2.wgt" );  
  My_Experiment->WriteArrays( "arr.2.arr" );  
  My_Experiment->Increment( "+,zw,A,rt,-,k,O,p,-,n,O,n,E" );  
  My_Experiment->Test( "dimin.test", "inc3.out" );
  My_Experiment->SaveWeights( "wg.3.wgt" );  
  My_Experiment->WriteArrays( "arr.3.arr" );  
  My_Experiment->Decrement( "+,zw,A,rt,-,k,O,p,-,n,O,n,E" );  
  My_Experiment->Test( "dimin.test", "inc4.out" );
  My_Experiment->SaveWeights( "wg.4.wgt" );  
  My_Experiment->WriteArrays( "arr.4.arr" );  
  My_Experiment->Decrement( "=,=,=,=,+,k,e,=,-,r,@,l,T" );  
  My_Experiment->Test( "dimin.test", "inc5.out" );
  My_Experiment->SaveWeights( "wg.5.wgt" );  
  My_Experiment->WriteArrays( "arr.5.arr" );  
  delete My_Experiment;
  exit(1);
}
