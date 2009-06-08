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
#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <climits>

#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/Tree.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"
#include "timbl/Metrics.h"
#include "timbl/Testers.h"

using namespace std;
using Common::Epsilon;
using Common::Log2;

namespace Timbl{


  double overlapTestFunction::test( FeatureValue *F,
				   FeatureValue *G,
				   Feature *Feat ) const {
    double result = Feat->distance( F, G );
    result *= Feat->Weight();
    return result;
  }
    
  inline bool FV_to_real( FeatureValue *FV, double &result ){
    if ( FV ){
      if ( stringTo<double>( FV->Name(), result ) )
	return true;
    }
    return false;
  }  

  double numericOverlapTestFunction::test( FeatureValue *F,
					   FeatureValue *G,
					   Feature *Feat ) const {
    double r1, r2, result;
    if ( FV_to_real( F, r1 ) &&
	 FV_to_real( G, r2 ) )
      result = fabs( (r1-r2)/ (Feat->Max() - Feat->Min()) );
    else
      result = 1.0;
    result *= Feat->Weight();
    return result;
  }

  double valueDiffTestFunction::test( FeatureValue *F,
				      FeatureValue *G,
				      Feature *Feat ) const {
    double result = Feat->distance( F, G, threshold );
    result *= Feat->Weight();
    return result;
  }


  TesterClass* getTester( MetricType m, 
			  const std::vector<Feature*>& features, 
			  const std::vector<size_t>& permutation,
			  int mvdThreshold ){
    if ( m == Cosine )
      return new CosineTester( features, permutation, mvdThreshold );
    else if ( m == DotProduct )
      return new DotProductTester( features, permutation, mvdThreshold );
    else
      return new DefaultTester( features, permutation, mvdThreshold );
  }

  TesterClass::TesterClass( const vector<Feature*>& feat,
			    const vector<size_t>& perm,
			    int mvdThreshold ):
    _size(feat.size()), features(feat), permutation(perm) {
    permFeatures.resize(_size,0);
    test_feature_val = new metricTestFunction*[_size];
    for ( size_t j=0; j < _size; ++j ){
      permFeatures[j] = feat[perm[j]];
      test_feature_val[j] = 0;
    }
    distances.resize(_size+1, 0.0);
    for ( size_t i=0; i < _size; ++i ){
      delete test_feature_val[i];
      test_feature_val[i] = 0;
      if ( features[i]->Ignore() )
	continue;
      if ( features[i]->isStorableMetric() )
 	test_feature_val[i] = new valueDiffTestFunction( mvdThreshold );
      else
	if ( features[i]->isNumerical() )
	  test_feature_val[i] = new numericOverlapTestFunction();
	else
	  test_feature_val[i] = new overlapTestFunction();
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

  double DefaultTester::getDistance( size_t pos ) const{
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
  
  double ExemplarTester::getDistance( size_t pos ) const{
    return distances[pos];
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

  double SimilarityTester::getDistance( size_t pos ) const{
    return maxSimilarity - distances[pos];
  }
  
}  

