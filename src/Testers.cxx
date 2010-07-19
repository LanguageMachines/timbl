/*
  Copyright (c) 1998 - 2010
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

  //#define DBGTEST

  double overlapTestFunction::test( FeatureValue *F,
				    FeatureValue *G,
				    Feature *Feat ) const {
#ifdef DBGTEST
    cerr << "overlap_distance(" << F << "," << G << ") = ";
#endif
    double result = Feat->fvDistance( F, G );
#ifdef DBGTEST
    cerr << result;
#endif
    result *= Feat->Weight();
#ifdef DBGTEST
    cerr << " gewogen " << result << endl;
#endif
    return result;
  }
    
  double numericOverlapTestFunction::test( FeatureValue *F,
					   FeatureValue *G,
					   Feature *Feat ) const {

#ifdef DBGTEST
    cerr << "numeric_distance(" << F << "," << G << ") = ";
#endif
    double result = Feat->fvDistance( F, G );
#ifdef DBGTEST
    cerr << result;
#endif
    result *= Feat->Weight();
#ifdef DBGTEST
    cerr << " gewogen " << result << endl;
#endif
    return result;
  }

  double euclideanTestFunction::test( FeatureValue *F,
				     FeatureValue *G,
				     Feature *Feat ) const {
#ifdef DBGTEST
    cerr << "euclidean_distance(" << F << "," << G << ") = ";
#endif
    double result = Feat->fvDistance( F, G );
#ifdef DBGTEST
    cerr << result;
#endif
    result *= Feat->Weight();
#ifdef DBGTEST
    cerr << " gewogen " << result << endl;
#endif
    return result;
  }

  double valueDiffTestFunction::test( FeatureValue *F,
				      FeatureValue *G,
				      Feature *Feat ) const {
#ifdef DBGTEST
    cerr << toString(Feat->getMetricType()) << "_distance(" << F << "," << G << ") = ";
#endif
    double result = Feat->fvDistance( F, G, threshold );
#ifdef DBGTEST
    cerr << result;
#endif
    result *= Feat->Weight();
#ifdef DBGTEST
    cerr << " gewogen " << result << endl;
#endif
    return result;
  }


  TesterClass* getTester( MetricType m, 
			  const std::vector<Feature*>& features, 
			  const std::vector<size_t>& permutation,
			  int mvdThreshold ){
    if ( m == Cosine )
      return new CosineTester( features, permutation );
    else if ( m == DotProduct )
      return new DotProductTester( features, permutation );
    else
      return new DistanceTester( features, permutation, mvdThreshold );
  }

  TesterClass::TesterClass( const vector<Feature*>& feat,
			    const vector<size_t>& perm ):
    _size(feat.size()), features(feat), permutation(perm) {
    permFeatures.resize(_size,0);
#ifdef DBGTEST
    cerr << "created TesterClass(" << _size << ")" << endl;
#endif
    for ( size_t j=0; j < _size; ++j ){
      permFeatures[j] = feat[perm[j]];
    }
    distances.resize(_size+1, 0.0);
  }
  
  void TesterClass::init( const Instance& inst,
			  size_t effective, 
			  size_t offset ){
#ifdef DBGTEST
    cerr << "tester Initialized!" << endl;
#endif
    effSize = effective-offset;
    offSet = offset;
    FV = &inst.FV;
  }

  DistanceTester::~DistanceTester(){
    for ( size_t i=0; i < _size; ++i ){
      delete metricTest[i];
    }
    delete [] metricTest;
  }
  
  DistanceTester::DistanceTester( const vector<Feature*>& feat,
				  const vector<size_t>& perm,
				  int mvdmThreshold ): 
    TesterClass( feat, perm ){
#ifdef DBGTEST
    cerr << "create a tester with treshold = " << mvdmThreshold << endl;
#endif
    metricTest = new metricTestFunction*[_size];
    for ( size_t i=0; i < _size; ++i ){
      metricTest[i] = 0;
#ifdef DBGTEST
      cerr << "set metric[" << i+1 << "]=" << toString(features[i]->getMetricType()) << endl;
#endif
      if ( features[i]->Ignore() )
	continue;
      if ( features[i]->isStorableMetric() ){
#ifdef DBGTEST
	cerr << "created  valueDiffTestFunction " << endl;
#endif
 	metricTest[i] = new valueDiffTestFunction( mvdmThreshold );
      }
      else if ( features[i]->isNumerical() ){
	if ( features[i]->getMetricType() == Euclidean ){
#ifdef DBGTEST
	  cerr << "created euclideanTestFunction " << endl;
#endif
	  metricTest[i] = new euclideanTestFunction();
	}
	else {
#ifdef DBGTEST
	  cerr << "created numericOverlapTestFunction " << endl;
#endif
	  metricTest[i] = new numericOverlapTestFunction();
	}
      }
      else {
#ifdef DBGTEST
	cerr << "created overlapFunction " << endl;
#endif
	metricTest[i] = new overlapTestFunction();
      }
    }
  }  

  size_t DistanceTester::test( vector<FeatureValue *>& G, 
			       size_t CurPos,
			       double Threshold ) {
    size_t i;
    size_t TrueF;
    for ( i=CurPos, TrueF = i + offSet; i < effSize; ++i,++TrueF ){
#ifdef DBGTEST
      cerr << "feature " << TrueF << " (perm=" << permutation[TrueF] 
	   << ")" << endl;
#endif
      double result = metricTest[permutation[TrueF]]->test( (*FV)[TrueF],
							    G[i],
							    permFeatures[TrueF] );
      distances[i+1] = distances[i] + result;
      if ( distances[i+1] > Threshold ){
#ifdef DBGTEST
	cerr << "threshold reached at " << i << " distance=" 
	     << distances[i+1] << endl;
#endif
	return i;
      }
    }
#ifdef DBGTEST
    	cerr << "threshold reached at end, distance=" << distances[effSize] << endl;
#endif
    return effSize;
  }

  double DistanceTester::getDistance( size_t pos ) const{
    return distances[pos];
  }

  inline bool FV_to_real( FeatureValue *FV, double &result ){
    if ( FV ){
      if ( stringTo<double>( FV->Name(), result ) )
	return true;
    }
    return false;
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

