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

#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <exception>
#include <stdexcept>
#include <sstream>

#include "StringOps.h"

namespace Timbl {

  template< typename T >
    T stringTo( const std::string& str ) {
    T result;
    std::stringstream dummy ( str );
    if ( !( dummy >> result ) ) {
      throw( std::runtime_error( "conversion from string '"
				 + str + "' failed" ) );
    }
    return result;
  }
  
  template <>
    inline bool stringTo<bool>( const std::string& str ) {
    bool result;
    std::stringstream dummy ( str );
    if ( !( dummy >> result ) ) {
      dummy.clear();
      dummy.setf(std::ios_base::boolalpha);
      if ( !( dummy >> result ) ) {
	throw( std::runtime_error( "conversion from string '"
				   + str + "' to bool failed" ) );
      }
   }
    return result;
  }
  
  template< typename T >
    bool stringTo( const std::string& str, T& result ) {
    try {
      result = stringTo<T>( str );
      return true;
    }
    catch( ... ){
     return false;
    }
  }
  
  template< typename T >
    std::string toString ( const T& obj, bool=false ) {
    std::stringstream dummy;
    if ( !( dummy << obj ) ) {
      throw( std::runtime_error( "conversion to long string failed" ) );
    }
   return dummy.str();
  }
  
  enum InputFormatType { UnknownInputFormat,
			 Compact, C4_5, Columns, ARFF, SparseBin,
			 Sparse,
			 MaxInputFormat };
  
  inline InputFormatType& operator++( InputFormatType &I ){
    return I = ( MaxInputFormat == I ) 
      ? UnknownInputFormat 
      : InputFormatType(I+1);
  }
  
  
  enum WeightType { Unknown_w,
		    No_w, GR_w, IG_w, X2_w, SV_w, UserDefined_w, 
		    Max_w };
  
  inline WeightType& operator++( WeightType &W ){
    return W = ( Max_w == W ) ? Unknown_w : WeightType(W+1);
  }
  
  enum AlgorithmType { Unknown_a,
		       IB1_a, IB2_a, IGTREE_a, TRIBL_a, TRIBL2_a,
		       LOO_a, CV_a,
		       Max_a };
  
  inline AlgorithmType& operator++( AlgorithmType &W ){
    return W = ( Max_a == W ) ? Unknown_a : AlgorithmType(W+1);
  }
  
  
  enum MetricType { UnknownMetric, Ignore, 
		    Numeric, DotProduct, Cosine, Overlap, Levenshtein, 
		    Dice, ValueDiff, JeffreyDiv, JSDiv, Euclidean, MaxMetric };
  
  inline MetricType& operator++( MetricType &W ){
    return W = ( MaxMetric == W ) ? UnknownMetric : MetricType(W+1);
  }
  
  enum OrdeningType { UnknownOrdening, DataFile, NoOrder,
		      GROrder, IGOrder, 
		      OneoverFeature, OneoverSplitInfo,
		      GRoverFeature, IGoverFeature,
		      GREntropyOrder, IGEntropyOrder,
		      X2Order, SVOrder, 
		      X2overFeature, SVoverFeature,
		      MaxOrdening };
  
  inline OrdeningType& operator++( OrdeningType &W ){
    return W = ( MaxOrdening == W ) ? UnknownOrdening : OrdeningType(W+1);
  }
  
  
  enum VerbosityFlags { NO_VERB=0, SILENT=1, OPTIONS=2, FEAT_W=4,
			VD_MATRIX=8, EXACT=16, DISTANCE=32, DISTRIB=64,
			NEAR_N=128, ADVANCED_STATS=256, CONF_MATRIX=512,
			CLASS_STATS=1024, CLIENTDEBUG=2048, ALL_K=4096,
			MATCH_DEPTH=8192, BRANCHING=16384,
                        MAX_VERB };

  inline VerbosityFlags operator~( VerbosityFlags V ){
    return (VerbosityFlags)( ~(int)V );
  }

  inline VerbosityFlags operator|( VerbosityFlags V1, VerbosityFlags V2 ){
    return (VerbosityFlags)( (int)V1|(int)V2 );
  }

  inline VerbosityFlags& operator|= ( VerbosityFlags& f, VerbosityFlags g ){
    f = (f | g);
    return f;
  }
  
  inline VerbosityFlags operator& ( VerbosityFlags f, VerbosityFlags g ){
    return (VerbosityFlags)((int)f & (int)g );
}
  
  inline VerbosityFlags& operator&= ( VerbosityFlags& f, VerbosityFlags g ){
    f = (f & g);
    return f;
  }
  
  enum OptType { UnknownOpt,
		 StringT, IntegerT, BooleanT, VerbosityT, IFormatT,
		 AlgoT, MetricT, WeightT, OrdeningT,
		 MaxOpt };
  
  inline OptType& operator++( OptType &W ){
    return W = ( MaxOpt == W ) ? UnknownOpt : OptType(W+1);
  }
  
  enum DecayType { UnknownDecay, 
		   Zero, InvDist, InvLinear, ExpDecay, MaxDecay };
   
  inline DecayType& operator++( DecayType &W ){
    return W = ( MaxDecay == W ) ? UnknownDecay : DecayType(W+1);
  }
  
  enum SmoothingType { UnknownSmoothing, Default, Lidstone, MaxSmoothing };  
  
  inline SmoothingType& operator++( SmoothingType &W ){
    return W = ( MaxSmoothing == W ) ? UnknownSmoothing : SmoothingType(W+1);
  }

  template <typename T>
    bool stringTo( const std::string& s, T &answer, T low, T upp ){
    try {
      T tmp = stringTo<T>( s );
      if ( (tmp >= low) && (tmp <= upp) ){
	answer = tmp;
	return true;
      }
      return false;
    }
    catch(...){
      return false;
    }
  }


  extern const char *DecayName[][2];
  extern const char *OrdeningName[][2];
  extern const char *WeightName[][2];
  extern const char *MetricName[][2];
  extern const char *InputFormatName[][2];
  extern const char *AlgorithmName[][2];
  extern const char *SmoothingName[][2];
  extern const char *VerbosityName[][2];
  extern const char *NormalisationName[][2];

  template <>
    inline DecayType stringTo<DecayType>( const std::string& str ) {
    DecayType d = UnknownDecay;
    for ( ++d; d < MaxDecay; ++d ){
      if ( compare_nocase( str, DecayName[d][0] ) ||
	   compare_nocase( str, DecayName[d][1] ) ){
	return d;
      }
    }
    throw( std::runtime_error( "conversion from string '"
			       + str + "' to decayType failed" ) );
    return UnknownDecay;
  }
  
  template <>
    inline std::string toString<DecayType>( const DecayType& W, bool b ){
    if ( b )
      return DecayName[W][1];
    else
      return DecayName[W][0];
  }

  template <>
    inline OrdeningType stringTo<OrdeningType>( const std::string& str ) {
    OrdeningType d = UnknownOrdening;
    for ( ++d; d < MaxOrdening; ++d ){
      if ( compare_nocase( str, OrdeningName[d][0] ) ||
	   compare_nocase( str, OrdeningName[d][1] ) ){
	return d;
      }
    }
    throw( std::runtime_error( "conversion from string '"
			       + str + "' to ordeningType failed" ) );
    return UnknownOrdening;
  }
  
  template <>
    inline std::string toString<OrdeningType>( const OrdeningType& W,
					       bool b ){
    return OrdeningName[W][(b?1:0)];
  }

  template <>
    inline MetricType stringTo<MetricType>( const std::string& str ) {
    MetricType d = UnknownMetric;
    for ( ++d; d < MaxMetric; ++d ){
      if ( compare_nocase( str, MetricName[d][0] ) ||
	   compare_nocase( str, MetricName[d][1] ) ){
	return d;
      }
    }
    throw( std::runtime_error( "conversion from string '"
			       + str + "' to metricType failed" ) );
    return UnknownMetric;
  }

  template <>
    inline std::string toString<MetricType>( const MetricType& W, bool b ){
    if ( b )
      return MetricName[W][1];
    else
      return MetricName[W][0];
  }

  WeightType charToWeig( char );

  template <>
    inline WeightType stringTo<WeightType>( const std::string& str ) {
    WeightType w = Unknown_w;
    if ( str.length() == 1 && isdigit(str[0]) ){
      w = charToWeig( str[0] );
    }
    if ( w != Unknown_w )
      return w;
    for ( ++w; w < Max_w; ++w ){
      if ( compare_nocase( str, WeightName[w][0] ) ||
	   compare_nocase( str, WeightName[w][1] ) ){
	return w;
      }
    }
    throw( std::runtime_error( "conversion from string '"
			       + str + "' to weightType failed" ) );
    return Unknown_w;
  }
  
  template <>
    inline std::string toString<WeightType>( const WeightType& W, bool b ){
    if ( b ) 
      return WeightName[W][1];
    else
      return WeightName[W][0];
  }

  AlgorithmType charToAlg( char  );

  template <>
    inline AlgorithmType stringTo<AlgorithmType>( const std::string& str ) {
    AlgorithmType a = Unknown_a;
    if ( str.length() == 1 && isdigit(str[0]) ){
      a = charToAlg( str[0] );
    }
    if ( a != Unknown_a )
      return a;
    for ( ++a; a < Max_a; ++a ){
      if ( compare_nocase( str, AlgorithmName[a][0] ) ||
	   compare_nocase( str, AlgorithmName[a][1] ) ){
	return a;
      }
    }
    throw( std::runtime_error( "conversion from string '"
			       + str + "' to algorithmType failed" ) );
    return Unknown_a;
  }

  template <>
    inline std::string toString<AlgorithmType>( const AlgorithmType& a,
						bool b ){
    if ( b )
      return AlgorithmName[a][1];
    else
      return AlgorithmName[a][0];
  }
  
  template <>
    inline InputFormatType stringTo<InputFormatType>( const std::string& str ){
    InputFormatType d = UnknownInputFormat;
    for ( ++d; d < MaxInputFormat; ++d ){
      if ( compare_nocase( str, InputFormatName[d][0] ) ||
	   compare_nocase( str, InputFormatName[d][1] ) ){
	return d;
      }
    }
    throw( std::runtime_error( "conversion from string '"
			       + str + "' to weightType failed" ) );
    return UnknownInputFormat;
  }
  
  template <>
    inline std::string toString<InputFormatType>( const InputFormatType& i,
						  bool b ){
    if ( b )
      return InputFormatName[i][1];
    else
      return InputFormatName[i][0];
  }

  template <>
    inline SmoothingType stringTo<SmoothingType>( const std::string& str ) { 
    SmoothingType d = UnknownSmoothing;
    for ( ++d; d < MaxSmoothing; ++d ){
      if ( compare_nocase( str, SmoothingName[d][0] ) ||
	   compare_nocase( str, SmoothingName[d][1] ) ){
	return d;
      }
    }
    throw( std::runtime_error( "conversion from string '"
			       + str + "' to smoothingType failed" ) );
    return UnknownSmoothing;
  }
  
  template <>
    inline std::string toString<SmoothingType>( const SmoothingType& s,
						bool b ){
    if ( b )
      return SmoothingName[s][1];
    else
      return SmoothingName[s][0];
  }

  enum normType { unknownNorm, noNorm, probabilityNorm, addFactorNorm, maxNorm };
  
  inline normType& operator++( normType &W ){
    return W = ( maxNorm == W ) ? noNorm : normType(W+1);
  }
  
  normType charToNorm( char  );

  template <>
    inline normType stringTo<normType>( const std::string& str ) { 
    normType d = unknownNorm;
    if ( str.length() == 1 && isdigit(str[0]) ){
      d = charToNorm( str[0] );
    }
    if ( d != unknownNorm )
      return d;
    for ( ++d; d < maxNorm; ++d ){
      if ( compare_nocase( str, NormalisationName[d][0] ) ||
	   compare_nocase( str, NormalisationName[d][1] ) ){
	return d;
      }
    }
    throw( std::runtime_error( "conversion from string '"
			       + str + "' to normalisationType failed" ) );
    return unknownNorm;
  }
  
  template <>
    inline std::string toString<normType>( const normType& s,
					   bool b ){
    if ( b )
      return NormalisationName[s][1];
    else
      return NormalisationName[s][0];
  }

  inline bool string_to_verbflag( const std::string& line, 
				  VerbosityFlags &a ){
    unsigned int i;
    for ( i=0; VerbosityName[i][0][0] != '\0'; i++ )
      if ( compare_nocase( line, VerbosityName[i][0] ) ||
	   compare_nocase( line, VerbosityName[i][1] ) ){
	if ( i==0 ){	
	  a = NO_VERB;
	}
	else{
	  a = (VerbosityFlags)(1<<(i-1));
	}
	return true;
      }
    return false;
  }
  
  template <>
    inline VerbosityFlags stringTo<VerbosityFlags>( const std::string& str ) { 
    std::vector<std::string> tmp;
    size_t cnt = split_at( str, tmp, "+" ); 
    VerbosityFlags V = NO_VERB;
    for ( size_t i=0; i < cnt; ++i ){
      VerbosityFlags Flag;
      if ( string_to_verbflag( tmp[i], Flag ) ){
	V |= Flag;
      }
      else {
	throw( std::runtime_error( "conversion from string '"
				   + str + "' to verbosityFlag failed" ) );
      }
    }
    return V;
  }

  inline std::string verbosity_to_string( int v, bool full ){
    if ( v == 0 )
      return VerbosityName[0][(full?1:0)];
    else {
      std::string OutLine;
      bool first = true;
      for ( unsigned int i=1; VerbosityName[i][0][0] != '\0'; ++i )
	if ( v & (1<<(i-1)) ){
	  if (first)
	    first = false;
	  else
	    OutLine += '+';
	  OutLine += VerbosityName[i][(full?1:0)];
	}
      return OutLine;
    }
  }
  
  template <>
    inline std::string toString<VerbosityFlags>( const VerbosityFlags& v,
						 bool full ){
    return verbosity_to_string( (int)v, full );
  }

  inline std::string toString( const AlgorithmType& a, bool b=false ){
    return toString<AlgorithmType>( a, b );
  }

  inline std::string toString( const MetricType& a, bool b=false ){
    return toString<MetricType>( a, b );
  }

  inline std::string toString( const WeightType& a, bool b=false ){
    return toString<WeightType>( a, b );
  }

  inline std::string toString( const InputFormatType& a, bool b=false ){
    return toString<InputFormatType>( a, b );
  }

  inline std::string toString( const DecayType& a, bool b=false ){
    return toString<DecayType>( a, b );
  }

  inline std::string toString( const OrdeningType& a, bool b=false ){
    return toString<OrdeningType>( a, b );
  }

  inline std::string toString( const VerbosityFlags& a, bool b=false ){
    return toString<VerbosityFlags>( a, b );
  }


}
#endif
