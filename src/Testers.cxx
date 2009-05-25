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
#include "timbl/MsgClass.h"
#include "timbl/Tree.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"
#include "timbl/Testers.h"

using namespace std;
using Common::Epsilon;
using Common::Log2;

namespace Timbl{

  double lv_distance( const string& source, const string& target ){
    // code taken from: http://www.merriampark.com/ldcpp.htm
    //    Levenshtein Distance Algorithm: C++ Implementation
    //                  by Anders Sewerin Johansen
    // Step 1
    const size_t n = source.length();
    const size_t m = target.length();
    if (n == 0) {
      return (double)m;
    }
    if (m == 0) {
      return (double)n;
    }    
    // Good form to declare a TYPEDEF
    typedef std::vector< std::vector<size_t> > Tmatrix;     
    Tmatrix matrix(n+1);
    
    // Size the vectors in the 2.nd dimension. Unfortunately C++ doesn't
    // allow for allocation on declaration of 2.nd dimension of vec of vec
    
    for ( size_t i = 0; i <= n; ++i ) {
      matrix[i].resize(m+1);
    }
    // Step 2
    for ( size_t i = 0; i <= n; ++i ) {
      matrix[i][0]=i;
    }
    for ( size_t i = 0; i <= m; ++i ) {
      matrix[0][i]=i;
    }
    // Step 3
    for ( size_t i = 1; i <= n; ++i ) {
      const char s_i = source[i-1];
      // Step 4
      for ( size_t j = 1; j <= m; ++j ) {
	const char t_j = target[j-1];
	// Step 5
	int cost;
	if (s_i == t_j) {
	  cost = 0;
	}
	else {
	  cost = 1;
	}
	// Step 6
	const size_t above = matrix[i-1][j];
	const size_t left = matrix[i][j-1];
	const size_t diag = matrix[i-1][j-1];
	size_t cell = min( above + 1, min(left + 1, diag + cost));
	// Step 6A: Cover transposition, in addition to deletion,
	// insertion and substitution. This step is taken from:
	// Berghel, Hal ; Roach, David : "An Extension of Ukkonen's 
	// Enhanced Dynamic Programming ASM Algorithm"
	// (http://www.acm.org/~hlb/publications/asm/asm.html)
	if (i>2 && j>2) {
	  size_t trans=matrix[i-2][j-2]+1;
	  if (source[i-2]!=t_j) trans++;
	  if (s_i!=target[j-2]) trans++;
	  if (cell>trans) cell=trans;
	}
	matrix[i][j]=cell;
      }
    }
    return (double)matrix[n][m];
  }
  
  double dc_distance( const string& string1, const string& string2 ){
    // code taken from:
    // http://en.wikibooks.org/wiki/Algorithm_implementation/Strings/Dice's_coefficient
    multiset<string> string1_bigrams;
    multiset<string> string2_bigrams;
    
    for(unsigned int i = 0; i < (string1.length() - 1); i++) {      // extract character bigrams from string1
      string1_bigrams.insert(string1.substr(i, 2));
    }
    for(unsigned int i = 0; i < (string2.length() - 1); i++) {      // extract character bigrams from string2
      string2_bigrams.insert(string2.substr(i, 2));
    }
    
    int overlap = 0;
    
    for(unsigned int j = 0; j < (string2.length() - 1); j++) {      // extract character bigrams from string2 and overlap
      overlap += string1_bigrams.count(string2.substr(j, 2));
    }
    
    // calculate 1 - dice coefficient
    int total = string1_bigrams.size() + string2_bigrams.size();
    float dice = (float)(overlap * 2) / (float)total;
    dice = 1.0 - dice;
    return dice;
  }  

  double vd_distance( SparseValueProbClass *r, SparseValueProbClass *s ){
    double result = 0.0;
    SparseValueProbClass::IDiterator p1 = r->begin();
    SparseValueProbClass::IDiterator p2 = s->begin();
    while( p1 != r->end() &&
	   p2 != s->end() ){
      if ( p2->first < p1->first ){
	result += p2->second;
	++p2;
      }
      else if ( p2->first == p1->first ){
	result += fabs( p1->second - p2->second );
	++p1;
	++p2;
      }
      else {
	result += p1->second;
	++p1;
      }	  
    }
    while ( p1 != r->end() ){
      result += p1->second;
      ++p1;
    }
    while ( p2 != s->end() ){
      result += p2->second;
      ++p2;
    }
    result = result / 2.0;
    return result;
  }
  
  double k_log_k_div_m( double k, double l ) {
    if ( abs(k+l) < Epsilon )
      return 0;
    return k * Log2( (2.0 * k)/( k + l ) );
  }
  
  double jd_distance( SparseValueProbClass *r, SparseValueProbClass *s ){
    double result = 0.0;
    SparseValueProbClass::IDiterator p1 = r->begin();
    SparseValueProbClass::IDiterator p2 = s->begin();
    while( p1 != r->end() &&
	   p2 != s->end() ){
      if ( p2->first < p1->first ){
	result += p2->second;
	++p2;
      }
      else if ( p2->first == p1->first ){
	result += k_log_k_div_m( p1->second, p2->second );
	result += k_log_k_div_m( p2->second, p1->second );
	++p1;
	++p2;
      }
      else {
	result += p1->second;
	++p1;
      }	  
    }
    while ( p1 != r->end() ){
      result += p1->second;
      ++p1;
    }
    while ( p2 != s->end() ){
      result += p2->second;
      ++p2;
    }
    result = result / 2.0;
    return result;
  }
  

  metricClass *getMetricClass( MetricType mt ){
    switch ( mt ){
    case Overlap:
      return new OverlapMetric();
      break;
    case Numeric:
      return new NumericMetric();
      break;
    case Cosine:
      return new CosineMetric();
      break;
    case DotProduct:
      return new DotProductMetric();
      break;
    case ValueDiff:
      return new ValueDiffMetric();
      break;
    case JeffreyDiv:
      return new JeffreyMetric();
      break;
    case Levenshtein:
      return new LevenshteinMetric();
      break;
    case Dice:
      return new DiceMetric();
      break;
    case Ignore:
    case DefaultMetric:
      return 0;
      break;
    default:
      throw logic_error("getMetricClass: unknown MetricType " + toString(mt) );
    }
  }

  double OverlapMetric::distance( FeatureValue *, FeatureValue *, 
				  size_t ) const {
    cerr << "unimplemented distance() for Overlap metric!" << endl;
    assert( type() == MaxMetric );
    return -1.0;
  }

  double JeffreyMetric::distance( FeatureValue *F, FeatureValue *G, 
				  size_t limit ) const {
    double result = 0.0;
    if ( G != F ){
      if ( F->ValFreq() < limit ||
	   G->ValFreq() < limit ){
	result = 1.0;
      }
      else {
	result = jd_distance( F->valueClassProb(), G->valueClassProb() );
      }
    }
    return result;
  }

  double LevenshteinMetric::distance( FeatureValue *F, FeatureValue *G, 
				      size_t limit ) const {
    double result = 0.0;
    if ( G != F ){
      if ( F->ValFreq() < limit ||
	   G->ValFreq() < limit ){
	result = 1.0;
      }
      else
	result = lv_distance( F->Name(), G->Name() );
    }
    return result;
  }  

  double DiceMetric::distance( FeatureValue *F, FeatureValue *G, 
			       size_t limit ) const {
    double result = 0.0;
    if ( F->ValFreq() < limit ||
	 G->ValFreq() < limit ){
      result = 1.0;
    }
    else
      result = dc_distance( F->Name(), G->Name() );
    return result;
  }
  
  double DotProductMetric::distance( FeatureValue *, FeatureValue *, 
				     size_t ) const {
    cerr << "unimplemented distance() for Dotproduct metric!" << endl;
    assert( type() == MaxMetric );
    return -1.0;
  }

  double ValueDiffMetric::distance( FeatureValue *F, FeatureValue *G, 
				    size_t limit ) const {
    double result = 0.0;
    if ( G != F ){
      if ( F->ValFreq() < limit ||
	   G->ValFreq() < limit ){
	result = 1.0;
      }
      else {
	if ( F->valueClassProb() && G->valueClassProb() )
	  result = vd_distance( F->valueClassProb(), G->valueClassProb() );
	else
	  result = 1.0;
      }
    }
    return result;
  }

  double CosineMetric::distance( FeatureValue *, FeatureValue *, 
				 size_t ) const {
    cerr << "unimplemented distance() for Cosine metric!" << endl;
    assert( type() == MaxMetric );
    return -1.0;
  }

  double NumericMetric::distance( FeatureValue *, FeatureValue *, 
				  size_t ) const {
    cerr << "unimplemented distance() for Numeric metric!" << endl;
    assert( type() == MaxMetric );
    return -1.0;
  }

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
    double result = Feat->Metric()->distance( F, G, threshold );
    result *= Feat->Weight();
    return result;
  }

  
  double jeffreyDiffTester::test( FeatureValue *F,
				  FeatureValue *G,
				  Feature *Feat ) const {
    double result = Feat->Metric()->distance( F, G, threshold );
    result *= Feat->Weight();
    return result;
  }
  
  double levenshteinTester::test( FeatureValue *F,
				  FeatureValue *G,
				  Feature *Feat ) const {
    double result = Feat->Metric()->distance( F, G, threshold );
    result *= Feat->Weight();
    return result;
  }
  
  double diceTester::test( FeatureValue *F,
			   FeatureValue *G,
			   Feature *Feat ) const {
    double result = Feat->Metric()->distance( F, G, threshold );
    result *= Feat->Weight();
    return result;
  }
  
  TesterClass::TesterClass( const vector<Feature*>& feat,
			    const vector<size_t>& perm,
			    int mvdThreshold ):
    _size(feat.size()), features(feat), permutation(perm) {
    permFeatures.resize(_size,0);
    test_feature_val = new metricTester*[_size];
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
      MetricType TM = features[i]->Metric()->type();
      switch ( TM ){
      case Overlap:
      case Cosine:
      case DotProduct:
	test_feature_val[i] = new overlapTester();
	break;
      case Numeric:
	test_feature_val[i] = new numericOverlapTester();
	break;
      case Levenshtein:
	test_feature_val[i] = new levenshteinTester( mvdThreshold );
	break;
      case Dice:
	test_feature_val[i] = new diceTester( mvdThreshold );
	break;
      case ValueDiff:
	test_feature_val[i] = new valueDiffTester( mvdThreshold );
	break;
      case JeffreyDiv:
	test_feature_val[i] = new jeffreyDiffTester( mvdThreshold );
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

