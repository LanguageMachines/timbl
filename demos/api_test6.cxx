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

#include <iostream>
#include "timbl/TimblAPI.h"

using std::cout;
using std::endl;
using namespace Timbl;

int main(){
  TimblAPI My_Experiment( "-a IB1 +vDI+DB -k3", "test6" );
  My_Experiment.Learn( "dimin.train" ); 
  const ValueDistribution *vd;
  const TargetValue *tv
    = My_Experiment.Classify( "-,=,O,m,+,h,K,=,-,n,I,N,K", vd );
  cout << "resulting target: " << tv << endl;
  cout << "resulting Distribution: " << vd << endl;
  ValueDistribution::dist_iterator it=vd->begin();
  while ( it != vd->end() ){
    cout << it->second << " OR ";
    cout << it->second->Value() << " " << it->second->Weight() << endl;
    ++it;
  }

  cout << "the same with neighborSets" << endl;
  const neighborSet *nb = My_Experiment.classifyNS( "-,=,O,m,+,h,K,=,-,n,I,N,K" );
  ValueDistribution *vd2 = nb->bestDistribution();
  cout << "default answer " << vd2 << endl;
  decayStruct *dc = new  expDecay(0.3);
  delete vd2;
  vd2 = nb->bestDistribution( dc );
  delete dc;
  cout << "with exponenial decay, alpha = 0.3 " << vd2 << endl;  
  delete vd2;
}
