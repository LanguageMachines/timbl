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

#include <iostream>
#include <string>
#include <exception>
#include "timbl/Common.h"
#include "timbl/MsgClass.h"
#include "timbl/StringOps.h"
#include "timbl/Tree.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"
#include "timbl/Statistics.h"

namespace Timbl {

  using std::bad_alloc;
  using std::ostream;
  using std::ios;
  using std::ios_base;
  using std::endl;
  using Common::Epsilon;

  ConfusionMatrix::ConfusionMatrix( size_t s ): size(s){
    try {
      mat.resize(size+1);
      for ( size_t i=0; i <= size; ++i ){
	mat[i].resize(size,0);
      }
    }
    catch( bad_alloc ){
      Error ( "Not enough memory for ConfusionMatrix" );
      throw;
      
    }
  }
  
  ConfusionMatrix::~ConfusionMatrix(){
    for ( unsigned int i=0; i <= size; ++i )
      mat[i].clear();
    mat.clear();
  }
  
  void ConfusionMatrix::Increment( const TargetValue *t1, 
				   const TargetValue *t2 ){
    if ( t2 ){
      if ( t1 )
	++mat[t1->Index()-1][t2->Index()-1];
      else
	++mat[size][t2->Index()-1];
    }
    else
      throw std::out_of_range( "ConfusionMatrix, index out of range" );
  }
  
  void ConfusionMatrix::Print( ostream& os, const Target *tg ) const {
    os << "Confusion Matrix:" << endl;
    os << "        ";
    for ( unsigned int i=0; i < tg->ValuesArray.size(); ++i ){
      // Print the class names.
      os.width(6);
      os.setf(ios::right, ios::adjustfield);
      os << tg->ValuesArray[i] << " ";
    }
    os << endl;
    os << "        ";
    for ( unsigned int i=0; i < size; ++i )
      os << "-------";
    os << endl;
    for ( unsigned int i=0; i < tg->ValuesArray.size(); ++i ){
      os.width(6);
      os.setf(ios::right, ios::adjustfield);
      os << tg->ValuesArray[i] << " | ";
      for ( unsigned int j=0; j < size; ++j ){
	os.width(6);
	os.setf(ios::right, ios::adjustfield);
	os << mat[i][j] << " ";
      }
      os << endl;
      if ( i == tg->ValuesArray.size() - 1 ){
	os <<  "   -*- | ";
	for ( unsigned int j=0; j < size; ++j ){
	  os.width(6);
	  os.setf(ios::right, ios::adjustfield);
	  os << mat[size][j] << " ";
	}
	os << endl;
      }
    }
    os << endl;
  }
  
  void pf( ostream& os, size_t d ){
    os.width(4);
    os << " \t" << d;
  }
  
  void pf( ostream& os, double d ){
    if ( d < 0 )
      os << " \t (nan)\t";
    else {
      os.setf(ios::showpoint);
      os << " \t" << d;
    }
  }
  
  void ConfusionMatrix::FScore( ostream& os, 
				const Target* tg, bool cs_too ) const {
    double maf = 0.0;
    double mif = 0.0;
    double maa = 0.0;
    double mia = 0.0;
    ios_base::fmtflags flags = os.flags(ios::fixed);
    int oldPrec =  os.precision(5);
    size_t effF = 0;
    size_t totF = 0;
    size_t effA = 0;
    size_t totA = 0;
    if ( cs_too ){
      os << "Scores per Value Class:" << endl;
      os << "class  |\tTP\tFP\tTN\tFN\tprecision\trecall(TPR)\tFPR\t\tF-score\t\tAUC" << endl;
    }
    for ( unsigned int i=0; i < tg->ValuesArray.size(); ++i ){
      size_t TP = 0;
      size_t FP = 0;
      size_t FN = 0;
      size_t TN = 0;
      ValueClass *tv = tg->ValuesArray[i];
      size_t valFreq = tv->ValFreq();
      for ( unsigned int j=0; j < size; ++j ){
	if ( i == j ){
	  TP = mat[i][j];
	}
	else
	  FN += mat[i][j];
      }
      for ( unsigned int j=0; j <= size; ++j ){
	if ( j != i )
	  FP += mat[j][i];
      }
      for ( unsigned int j=0; j <= size; ++j ){
	if ( j != i )
	  for ( unsigned int k=0; k < size; ++k ){
	    if ( k != i )
	      TN += mat[j][k];
	  }
      }
      double precision;
      if ( TP + FP == 0 )
	precision = -1;
      else
	precision = TP / double(TP + FP);
      double TPR;
      if ( TP + FN == 0 )
	TPR = -1;
      else
	TPR = TP / double(TP + FN);
      double FPR;
      if ( FP + TN == 0 )
	FPR = -1;
      else
	FPR = FP / double(FP + TN);
      double FScore;
      if ( precision < 0 || TPR < 0 ||
	   fabs(precision + TPR) < Epsilon ){
	FScore = -1;
      }
      else {
	FScore = ( 2 * precision * TPR ) / (precision + TPR );
	++effF;
	maf += FScore;
	totF += valFreq;
	mif += (FScore * valFreq);
      }
      double AUC;
      if ( TPR < 0 || FPR < 0 ){
	AUC = -1;
      }
      else {
	AUC = ( 0.5 * TPR * FPR ) + ( TPR * ( 1.0 - FPR ) ) + 
	  ( 0.5 * ( ( 1.0 - TPR ) * ( 1.0 - FPR ) ) );
	++effA;
	maa += AUC;
	totA += valFreq;
	mia += (AUC * valFreq);
      }
      if ( cs_too ){
	os.width( 6 );
	os << tv << " | ";
	os.width(0);
	pf(os,TP);
	pf(os,FP);
	pf(os,TN);
	pf(os,FN);
	pf(os,precision);
	pf(os,TPR);
	pf(os,FPR);
	pf(os,FScore);
	pf(os,AUC);
	os << endl;
      }
    }
    maf = maf / effF;
    mif = mif / totF;
    maa = maa / effA;
    mia = mia / totA;
    os.precision( oldPrec );
    os.flags( flags );
    os << "F-Score beta=1, microav: " << mif << endl;
    os << "F-Score beta=1, macroav: " << maf << endl;
    os << "AUC, microav:            " << mia << endl;
    os << "AUC, macroav:            " << maa << endl;
  }
  
}
