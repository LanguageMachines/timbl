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
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <climits>

#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/MsgClass.h"
#include "timbl/Tree.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"
#include "timbl/Testers.h"

using namespace std;

namespace Timbl{
  double overlapTester::test( FeatureValue *FV,
			      FeatureValue *G,
			      Feature *Feat ) const {
    double result = 0.0;
    if ( !FV->isUnknown() && FV->ValFreq() == 0 )
      result = 1.0;
    else if ( FV != G ){
      result = Feat->Weight();
    }
    return result;
  }

  inline bool FV_to_real( FeatureValue *FV, double &result ){
    if ( FV ){
      if ( stringTo<double>( FV->Name(), result ) )
	return true;
    }
    return false;
  }  
  
  double numericOverlapTester::test( FeatureValue *FV,
				     FeatureValue *G,
				     Feature *Feat ) const {
    double r1, r2, result;
    if ( FV_to_real( FV, r1 ) &&
	 FV_to_real( G, r2 ) )
      result = fabs( (r1-r2)/
		     (Feat->Max() - Feat->Min()));
    else
      result = 1.0;
    result *= Feat->Weight();
    return result;
  }
  
  double valueDiffTester::test( FeatureValue *F,
				FeatureValue *G,
				Feature *Feat ) const {
    double result = Feat->ValueDistance( F, G, threshold );
    result *= Feat->Weight();
    return result;
  }

  
  double jeffreyDiffTester::test( FeatureValue *F,
				  FeatureValue *G,
				  Feature *Feat ) const {
    double result = Feat->JeffreyDistance( F, G, threshold );
    result *= Feat->Weight();
    return result;
  }
  
  double levenshteinTester::test( FeatureValue *F,
				  FeatureValue *G,
				  Feature *Feat ) const {
    double result = Feat->LevenshteinDistance( F, G, threshold );
    result *= Feat->Weight();
    return result;
  }
  
  TesterClass::TesterClass( const vector<Feature*>& feat,
			    const vector<size_t>& perm ):
    _size(feat.size()), features(feat), permutation(perm) {
    permFeatures.resize(_size,0);
    test_feature_val = new metricTester*[_size];
    for ( size_t j=0; j < _size; ++j ){
      permFeatures[j] = feat[perm[j]];
      test_feature_val[j] = 0;
    }
    distances.resize(_size+1, 0.0);
  }
  
  void TesterClass::reset( MetricType globalMetric, int threshold ){
    distances.resize(_size+1, 0.0);
    for ( size_t i=0; i < _size; ++i ){
      delete test_feature_val[i];
      test_feature_val[i] = 0;
      if ( features[i]->Ignore() )
	continue;
      if ( features[i]->Numeric() ){
	test_feature_val[i] = new numericOverlapTester();
      }
      else {
	MetricType TM = features[i]->Metric();
	if ( TM == DefaultMetric )
	  TM = globalMetric;
	switch ( TM ){
	case Overlap:
	case Cosine:
	case DotProduct:
	  test_feature_val[i] = new overlapTester();
	  break;
	case Levenshtein:
	  test_feature_val[i] = new levenshteinTester( threshold );
	  break;
	case ValueDiff:
	  test_feature_val[i] = new valueDiffTester( threshold );
	  break;
	case JeffreyDiv:
	  test_feature_val[i] = new jeffreyDiffTester( threshold );
	  break;
	default:
	  string msg = string("Invalid value '") + toString( TM, true ) 
	    + "' in switch (" 
	    + __FILE__  + "," + toString(__LINE__) + ")\n"
	    + "ABORTING now";
	  throw logic_error( msg );
	}
      }
    }
  }

  void TesterClass::init( const Instance& inst,
			  size_t effective, 
			  size_t offset ){
    effSize = effective-offset;
    offSet = offset;
    FV = &inst.FV;
  }


  TesterClass::~TesterClass(){
    for ( size_t i=0; i < _size; ++i ){
      delete test_feature_val[i];
    }
    delete [] test_feature_val;
  }

  double DefaultTester::getDistance( size_t pos ) const{
    return distances[pos];
  }

  size_t DefaultTester::test( vector<FeatureValue *>& G, 
			      size_t CurPos,
			      double Threshold ) {
    size_t i;
    size_t TrueF;
    for ( i=CurPos, TrueF = i + offSet; i < effSize; ++i,++TrueF ){
      double result = test_feature_val[permutation[TrueF]]->test( (*FV)[TrueF],
								  G[i],
								  permFeatures[TrueF] );
      distances[i+1] = distances[i] + result;
      if ( distances[i+1] > Threshold ){
	return i;
      }
    }
    return effSize;
  }

  double ExemplarTester::getDistance( size_t pos ) const{
    return distances[pos];
  }

  size_t ExemplarTester::test( vector<FeatureValue *>& G, 
			       size_t CurPos,
			       double ){
    double result;
    size_t TrueF;
    size_t i;
    for ( i=CurPos, TrueF = i + offSet; i < effSize; ++i,++TrueF ){
      result = test_feature_val[permutation[TrueF]]->test( (*FV)[TrueF],
							   G[i],
							   permFeatures[TrueF] );
      distances[i+1] = distances[i] + result;
    }
    return effSize;
  }
 
  
  double innerProduct( FeatureValue *FV,
		       FeatureValue *G ) {
    double r1, r2, result;
    if ( FV_to_real( FV, r1 ) &&
	 FV_to_real( G, r2 ) ){
      result = r1 * r2;
    }
    else
      result = 0.0;
    return result;
  }

  double CosineTester::getDistance( size_t pos ) const{
    return maxSimilarity - distances[pos];
  }

  size_t CosineTester::test( vector<FeatureValue *>& G, 
			     size_t CurPos,
			     double ){
    double denom1 = 0.0;
    double denom2 = 0.0;
    double result = 0.0;
    size_t TrueF;
    size_t i;
    for ( i=CurPos, TrueF = i + offSet; i < effSize; ++i,++TrueF ){
      double W = permFeatures[TrueF]->Weight();
      denom1 +=  innerProduct( (*FV)[TrueF], (*FV)[TrueF] ) * W;
      denom2 += innerProduct( G[i], G[i] ) * W;
      result += innerProduct( (*FV)[TrueF], G[i] ) * W;
    }
    double denom = sqrt( denom1 * denom2 );
    distances[effSize] = result/ (denom + Common::Epsilon);
    return effSize;
  }  

  double DotProductTester::getDistance( size_t pos ) const{
    return maxSimilarity - distances[pos];
  }


  size_t DotProductTester::test( vector<FeatureValue *>& G, 
				 size_t CurPos,
				 double ) {
    double result;
    size_t TrueF;
    size_t i;
    for ( i=CurPos, TrueF = i + offSet; i < effSize; ++i,++TrueF ){
      result = innerProduct( (*FV)[TrueF], G[i] );
      result *= permFeatures[TrueF]->Weight();
      distances[i+1] = distances[i] + result;
    }
    return effSize;
  }
  
}  

