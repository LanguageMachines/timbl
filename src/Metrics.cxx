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
#include <set>
#include <string>
#include <iosfwd>
#include <stdexcept>

#include "timbl/Common.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"
#include "timbl/Metrics.h"
#include "unicode/schriter.h"

using namespace std;
using Common::Epsilon;
using Common::Log2;

//#define METRIC_DEBUG

namespace Timbl{

  double lv_distance( const icu::UnicodeString& source,
		      const icu::UnicodeString& target ){
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
	  if (source[i-2]!=t_j) { trans++; };
	  if (s_i!=target[j-2]) { trans++; };
	  if (cell>trans) { cell=trans; };
	}
	matrix[i][j]=cell;
      }
    }
    return (double)matrix[n][m];
  }

  double dc_distance( const icu::UnicodeString& string1,
		      const icu::UnicodeString& string2 ){
    // code taken from:
    // http://en.wikibooks.org/wiki/Algorithm_implementation/Strings/Dice's_coefficient
    unsigned int ls1 = string1.length();
    unsigned int ls2 = string2.length();
    double dice;
    int overlap = 0;
    int total = 0;
    if ( ls1 <= 1 || ls2 <= 1 ){
      // back-off naar unigrammen
      set<UChar32> string1_unigrams;
      set<UChar32> string2_unigrams;

      icu::StringCharacterIterator it1(string1);
      while ( it1.hasNext() ){
	string1_unigrams.insert(it1.current32());
	it1.next32();
      }
      icu::StringCharacterIterator it2(string2);
      while ( it2.hasNext() ){
	string2_unigrams.insert(it2.current32());
	it2.next32();
      }

      for ( const auto& ug : string2_unigrams ){
	if ( string1_unigrams.find( ug ) != string1_unigrams.end() ){
	  ++overlap;
	}
      }
      total = string1_unigrams.size() + string2_unigrams.size();
    }
    else {
      set<icu::UnicodeString> string1_bigrams;
      set<icu::UnicodeString> string2_bigrams;

      for ( unsigned int i = 0; i < (ls1 - 1); ++i ) {
	// extract character bigrams from string1
	string1_bigrams.insert( icu::UnicodeString( string1, i, 2 ) );
      }
      for ( unsigned int i = 0; i < (ls2 - 1); ++i ) {
	// extract character bigrams from string2
	string2_bigrams.insert( icu::UnicodeString( string2, i, 2 ) );
      }

      for ( const auto& bg : string2_bigrams ){
	if ( string1_bigrams.find( bg ) != string1_bigrams.end() ){
	  ++overlap;
	}
      }
      total = string1_bigrams.size() + string2_bigrams.size();
    }
    dice = (double)(overlap * 2) / (double)total;
    // we will return 1 - dice coefficient as distance
    return 1.0 - dice;
  }

  double vd_distance( SparseValueProbClass *r, SparseValueProbClass *s ){
    double result = 0.0;
    if ( ! ( r && s ) ){
      return 1.0;
    }
    auto p1 = r->begin();
    auto p2 = s->begin();
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

  double p_log_p_div_q( double p, double q ) {
    if ( abs(q) < Epsilon ){
      return 0;
    }
    return p * Log2( p/q );
  }

  double jd_distance( SparseValueProbClass *r, SparseValueProbClass *s ){
    double part1 = 0.0;
    double part2 = 0.0;
    auto p1 = r->begin();
    auto p2 = s->begin();
    while( p1 != r->end() &&
	   p2 != s->end() ){
      if ( p2->first < p1->first ){
	part2 += p2->second;
	++p2;
      }
      else if ( p2->first == p1->first ){
	part1 += p_log_p_div_q( p1->second, p2->second );
	part2 += p_log_p_div_q( p2->second, p1->second );
	++p1;
	++p2;
      }
      else {
	part1 += p1->second;
	++p1;
      }
    }
    while ( p1 != r->end() ){
      part1 += p1->second;
      ++p1;
    }
    while ( p2 != s->end() ){
      part2 += p2->second;
      ++p2;
    }
    double result = part1 + part2;
    result = result / 2.0;
    return result;
  }

  double k_log_k_div_m( double k, double l ) {
    if ( abs(k+l) < Epsilon ){
      return 0;
    }
    return k * Log2( (2.0 * k)/( k + l ) );
  }

  double js_distance( SparseValueProbClass *r, SparseValueProbClass *s ){
    double part1 = 0.0;
    double part2 = 0.0;
    auto p1 = r->begin();
    auto p2 = s->begin();
    while( p1 != r->end() &&
	   p2 != s->end() ){
      if ( p2->first < p1->first ){
	part2 += p2->second;
	++p2;
      }
      else if ( p2->first == p1->first ){
	part1 += k_log_k_div_m( p1->second, p2->second );
	part2 += k_log_k_div_m( p2->second, p1->second );
	++p1;
	++p2;
      }
      else {
	part1 += p1->second;
	++p1;
      }
    }
    while ( p1 != r->end() ){
      part1 += p1->second;
      ++p1;
    }
    while ( p2 != s->end() ){
      part2 += p2->second;
      ++p2;
    }
    double result = part1 + part2;
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
    case Euclidean:
      return new EuclideanMetric();
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
    case JSDiv:
      return new JSMetric();
      break;
    case Levenshtein:
      return new LevenshteinMetric();
      break;
    case Dice:
      return new DiceMetric();
      break;
    case Ignore:
      return 0;
      break;
    default:
      throw logic_error("getMetricClass: unknown MetricType " + TiCC::toString(mt) );
    }
  }

  double OverlapMetric::distance( FeatureValue *F,
				  FeatureValue *G,
				  size_t,
				  double ) const {
    if ( F == G ){
      return 0.0;
    }
    else {
      return 1.0;
    }
  }


  inline bool FV_to_real( FeatureValue *FV, double &result ){
    if ( FV ){
      if ( TiCC::stringTo<double>( FV->name(), result ) ){
	return true;
      }
    }
    return false;
  }

  double JeffreyMetric::distance( FeatureValue *F, FeatureValue *G,
				  size_t limit,
				  double ) const {
    double result = 0.0;
    if ( G != F ){
      if ( F->ValFreq() < limit ||
	   G->ValFreq() < limit ){
#ifdef METRIC_DEBUG
	cerr << "result = 1.0 vanwege F.valFreq=" <<  F->ValFreq()
	     << " en G.valFreq()=" << G ->ValFreq()
	     << " met limiet= " << limit << endl;
#endif
	result = 1.0;
      }
      else {
	result = jd_distance( F->valueClassProb(), G->valueClassProb() );
      }
    }
    return result;
  }

  double JSMetric::distance( FeatureValue *F, FeatureValue *G,
			     size_t limit,
			     double ) const {
    double result = 0.0;
    if ( G != F ){
      if ( F->ValFreq() < limit ||
	   G->ValFreq() < limit ){
#ifdef METRIC_DEBUG
	cerr << "result = 1.0 vanwege F.valFreq=" <<  F->ValFreq()
	     << " en G.valFreq()=" << G ->ValFreq()
	     << " met limiet= " << limit << endl;
#endif
	result = 1.0;
      }
      else {
	result = js_distance( F->valueClassProb(), G->valueClassProb() );
      }
    }
    return result;
  }

  double LevenshteinMetric::distance( FeatureValue *F, FeatureValue *G,
				      size_t, double) const {
    double result = 0.0;
    if ( G != F ){
      result = lv_distance( F->name(), G->name() );
    }
    return result;
  }

  double DiceMetric::distance( FeatureValue *F, FeatureValue *G,
			       size_t, double ) const {
    double result = 0.0;
    if ( G != F ){
      result = dc_distance( F->name(), G->name() );
    }
    return result;
  }

  double ValueDiffMetric::distance( FeatureValue *F, FeatureValue *G,
				    size_t limit,
				    double ) const {
    double result = 0.0;
    if ( G != F ){
      if ( F->ValFreq() < limit ||
	   G->ValFreq() < limit ){
#ifdef METRIC_DEBUG
	cerr << "result = 1.0 vanwege F.valFreq=" <<  F->ValFreq()
	     << " en G.valFreq()=" << G ->ValFreq()
	     << " met limiet= " << limit << endl;
#endif
	result = 1.0;
      }
      else {
	result = vd_distance( F->valueClassProb(), G->valueClassProb() );
      }
    }
    return result;
  }

  double NumericMetric::distance( FeatureValue *F, FeatureValue *G,
				  size_t,
				  double scale ) const {
    double r1=0, r2=0, result;
    if ( FV_to_real( F, r1 ) &&
	 FV_to_real( G, r2 ) ){
      result = fabs( (r1-r2)/ ( scale ) );
    }
    else {
      result = 1.0;
    }
    return result;
  }

  double EuclideanMetric::distance( FeatureValue *F, FeatureValue *G,
				    size_t,
				    double scale ) const {
    double r1=0, r2=0, result;
    if ( FV_to_real( F, r1 ) &&
	 FV_to_real( G, r2 ) ){
      result = sqrt(fabs(r1*r1-r2*r2))/ ( scale );
    }
    else {
      result = 1.0;
    }
    return result;
  }

  double DotProductMetric::distance( FeatureValue *, FeatureValue *,
				     size_t, double ) const {
    throw( logic_error( "unimplemented distance() for Dotproduct metric!" ) );
  }

  double CosineMetric::distance( FeatureValue *, FeatureValue *,
				 size_t, double ) const {
    throw( logic_error( "unimplemented distance() for Cosine metric!" ) );
  }


}
