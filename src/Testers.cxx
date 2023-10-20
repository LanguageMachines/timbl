/*
  Copyright (c) 1998 - 2023
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
#include <vector>
#include <string>
#include <iosfwd>

#include "timbl/Common.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"
#include "timbl/Metrics.h"
#include "timbl/Testers.h"

using namespace std;
using Common::Epsilon;
using Common::Log2;

namespace Timbl{

  //#define DBGTEST
  //#define DBGTEST_DOT

  double overlapTestFunction::test( const FeatureValue *F,
				    const FeatureValue *G,
				    const Feature *Feat ) const {
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

  double valueDiffTestFunction::test( const FeatureValue *F,
				      const FeatureValue *G,
				      const Feature *Feat ) const {
#ifdef DBGTEST
    cerr << TiCC::toString(Feat->getMetricType()) << "_distance(" << F << "," << G << ") = ";
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
			  const Feature_List& features,
			  int mvdThreshold ){
    if ( m == Cosine ){
      return new CosineTester( features );
    }
    else if ( m == DotProduct ){
      return new DotProductTester( features );
    }
    else {
      return new DistanceTester( features, mvdThreshold );
    }
  }

  TesterClass::TesterClass( const Feature_List& features ):
    _size(features.feats.size()),
    effSize(_size),
    offSet(0),
    FV(0),
    features(features.feats),
    permutation(features.permutation)
  {
    permFeatures.resize(_size,0);
#ifdef DBGTEST
    cerr << "created TesterClass(" << _size << ")" << endl;
#endif
    for ( size_t j=0; j < _size; ++j ){
      permFeatures[j] = features.feats[features.permutation[j]];
    }
    distances.resize(_size+1, 0.0);
  }

  void TesterClass::init( const Instance& inst,
			  size_t effective,
			  size_t oset ){
#ifdef DBGTEST
    cerr << "tester Initialized!" << endl;
#endif
    effSize = effective-oset;
    offSet = oset;
    FV = &inst.FV;
  }

  DistanceTester::~DistanceTester(){
    for ( const auto& it : metricTest ){
      delete it;
    }
  }

  DistanceTester::DistanceTester( const Feature_List& features,
				  int mvdmThreshold ):
    TesterClass( features ){
#ifdef DBGTEST
    cerr << "create a tester with threshold = " << mvdmThreshold << endl;
#endif
    metricTest.resize(_size,0);
    for ( size_t i=0; i < _size; ++i ){
#ifdef DBGTEST
      cerr << "set metric[" << i+1 << "]=" << TiCC::toString(features.feats[i]->getMetricType()) << endl;
#endif
      if ( features[i]->Ignore() )
	continue;
      if ( features[i]->isStorableMetric() ){
#ifdef DBGTEST
	cerr << "created  valueDiffTestFunction " << endl;
#endif
 	metricTest[i] = new valueDiffTestFunction( mvdmThreshold );
      }
      else {
#ifdef DBGTEST
	cerr << "created overlapFunction " << endl;
#endif
	metricTest[i] = new overlapTestFunction();
      }
    }
  }

  size_t DistanceTester::test( const vector<FeatureValue *>& G,
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

  inline bool FV_to_real( const FeatureValue *FV,
			  double &result ){
    if ( FV ){
      if ( TiCC::stringTo<double>( FV->name(), result ) ){
	return true;
      }
    }
    return false;
  }

  double innerProduct( const FeatureValue *FV,
		       const FeatureValue *G ) {
    double r1=0, r2=0, result;
#ifdef DBGTEST_DOT
    cerr << "innerproduct " << FV << " x " << G << endl;
#endif
    if ( FV_to_real( FV, r1 ) &&
	 FV_to_real( G, r2 ) ){
#ifdef DBGTEST_DOT
      cerr << "innerproduct " << r1 << " x " << r2 << endl;
#endif
      result = r1 * r2;
    }
    else {
      result = 0.0;
    }
#ifdef DBGTEST_DOT
    cerr << " resultaat == " << result << endl;
#endif
    return result;
  }

  size_t CosineTester::test( const vector<FeatureValue *>& G,
			     size_t,
			     double ){
    double denom1 = 0.0;
    double denom2 = 0.0;
    double result = 0.0;
    size_t TrueF;
    size_t i;
    for ( i=0, TrueF = i + offSet; i < effSize; ++i,++TrueF ){
      double W = permFeatures[TrueF]->Weight();
      denom1 += innerProduct( (*FV)[TrueF], (*FV)[TrueF] ) * W;
      denom2 += innerProduct( G[i], G[i] ) * W;
      result += innerProduct( (*FV)[TrueF], G[i] ) * W;
    }
    double denom = sqrt( denom1 * denom2 );
    distances[effSize] = result/ (denom + Common::Epsilon);
#ifdef DBGTEST
    cerr << "denom1 " << denom1 << endl;
    cerr << "denom2 " << denom2 << endl;
    cerr << "denom  " << denom << endl;
    cerr << "result " << result << endl;
    cerr << "cosine::test() distance " <<  distances[effSize] << endl;
#endif
    return effSize;
  }

  size_t DotProductTester::test( const vector<FeatureValue *>& G,
				 size_t,
				 double ) {
    size_t TrueF;
    size_t i;
    for ( i=0, TrueF = i + offSet; i < effSize; ++i,++TrueF ){
      double result = innerProduct( (*FV)[TrueF], G[i] );
      result *= permFeatures[TrueF]->Weight();
      distances[i+1] = distances[i] + result;
#ifdef DBGTEST
      cerr << "gewogen result " << result << endl;
      cerr << "dot::test() distance[" << i+1 << "]=" <<  distances[i+1] << endl;
#endif
    }
    return effSize;
  }

  double CosineTester::getDistance( size_t pos ) const{
#ifdef DBGTEST
    cerr << "getDistance, maxSim = " << 1.0  << endl;
    cerr << " distances[" << pos << "]= " <<  distances[pos] << endl;
#endif
    return 1.0 - distances[pos];
  }

  double DotProductTester::getDistance( size_t pos ) const{
#ifdef DBGTEST_DOT
    cerr << "getDistance, maxSim = " << std::numeric_limits<int>::max() << endl;
    cerr << " distances[" << pos << "]= " <<  distances[pos] << endl;
#endif
    return (std::numeric_limits<int>::max() - distances[pos])/std::numeric_limits<int>::max();;
  }

}
