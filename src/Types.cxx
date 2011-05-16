/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2011
  ILK   - Tilburg University
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
      http://ilk.uvt.nl/software.html
  or send mail to:
      timbl@uvt.nl
*/

#include <vector>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cstring>

#include "timbl/StringOps.h"
#include "timbl/Types.h"

namespace Timbl {
  using std::string;
  using std::vector;

// initializers

  const char *AlgorithmName[][2] = { { "Unknown", "Unknown Algorithm" },
				     { "IB1", "Memory Based Learning" },
				     { "IB2", "Adapted Memory Based Learning"},
				     { "IGTree", "Information Gain Tree" },
				     { "TRIBL", "Tree IB1" },
				     { "TRIBL2", "Tribl 2" },
				     { "LOO", "Leave One Out" },
				     { "CV", "Cross Validate" } };
  
  const char *MetricName[][2] = { { "U", "Unknown Metric" },
				  { "I", "Ignore" },
				  { "N", "Numeric" },
				  { "D", "Dot product" },
				  { "C", "Cosine metric" },
				  { "O", "Overlap" }, 
				  { "L", "Levenshtein" },
				  { "DC", "Dice coefficient" },
				  { "M", "Value Difference" },
				  { "J", "Jeffrey Divergence" },
				  { "S", "Jensen-Shannon Divergence" },
				  { "E", "Euclidean Distance" } };
  
  const char *WeightName[][2] = { { "un", "Unknown Weighting" },
				  { "nw", "No Weighting" },
				  { "gr", "GainRatio" },
				  { "ig", "InfoGain" },
				  { "x2", "Chi-square" }, 
				  { "sv", "Shared Variance" }, 
				  { "sd", "Standard Deviation" }, 
				  { "ud", "User Defined"} };
  
  const char *DecayName[][2] = { { "Unknown", "Unknown Decay" },
				 { "Z", "Zero Decay" },
				 { "ID", "Inverse Distance" },
				 { "IL", "Inverse Linear Distance" },
				 { "ED", "Exponential Decay" } };
  
  const char *SmoothingName[][2] = { { "Unknown", "Unknown Smoothing" },
				     { "Default", "Default Smoothing" },
				     { "L", "Lidstone Smoothing" } };
  
  const char *OrdeningName[][2] = { { "Unknown", "Unknown Ordering" },
				    { "UDO", "Data File Ordering" },
				    { "DO", "Default Ordering" },
				    { "GRO", "GainRatio" },
				    { "IGO", "InformationGain" },
				    { "1/V", "Inverse Values" },
				    { "1/S", "Inverse SplitInfo" },
				    { "G/V", "GainRatio/Values" },
				    { "I/V", "InformationGain/Values" },
				    { "GxE", "GainRatio*Entropy" },
				    { "IxE", "InformationGain*Entropy" },
				    { "X2O", "Chi-Squared" },
				    { "SVO", "Shared Variance" },
				    { "SDO", "Standard Deviation" },
				    { "X/V", "Chi-Squared/Values" },
				    { "S/V", "Shared Variance/Values" },
				    { "SD/V", "Standard Deviation/Values" } };
  
  const char *InputFormatName[][2] = {
    { "Unknown", "Unknown Input Format" }, 
    { "Compact", "Compact" },
    { "C45", "C4.5" },
    { "Column", "Columns" },
    { "ARFF", "ARFF" },
    { "BINARY", "Sparse Binary" },
    { "SPARSE", "Sparse" } };
  
  const char *VerbosityName[][2] = { { "Unknown", "erronous" },
				     { "S", "Silent" },
				     { "O", "Options" },
				     { "F", "Feature_Statistics" },
				     { "P", "Probability_arrays" },
				     { "E", "Exact_match" },
				     { "DI", "Distances" },
				     { "DB", "Distribution" },
				     { "N", "Nearest_Neighbours" },
				     { "AS", "Advanced_Statistics" },
				     { "CM", "Confusion_Matrix" },
				     { "CS", "Class_Statistics" },
				     { "CD", "Client_Debug" },
				     { "K", "All_K_values" },
				     { "MD", "MatchingDepth" },
				     { "B", "BranchingFactor" },
				     // Verbosity is special!
				     // should end with "" strings!
				     { "", "" } };

  const char *NormalisationName[][2] = {
    { "Unknown", "Unknown normalisation" },
    { "None", "No Normalisation" },
    { "Probability", "Normalise to 100%" },
    { "AddFactor", "Add a factor to all targets, then normalise to 100%" },
    { "LogProbability", "Take 10log, then Normalise to 100%" }
  };

  WeightType charToWeig( char w ){
    switch ( w ){
    case '0':
      return No_w;
    case '1':
	return GR_w;
    case '2':
      return IG_w;
    case '3':
      return X2_w;
    case '4':
      return SV_w;
    case '5':
      return SD_w;
    default:
      return Unknown_w;
    }
  }
  
  AlgorithmType charToAlg( char a ){
    switch ( a ){
    case '0':
      return IB1_a;
    case '1':
      return IGTREE_a;
    case '2':
      return TRIBL_a;
    case '3':
      return IB2_a;
    case '4':
      return TRIBL2_a;
    default:
      return Unknown_a;
    }
  }

  normType charToNorm( char a ){
    switch ( a ){
    case '0':
      return probabilityNorm;
    case '1':
      return addFactorNorm;
    case '2':
      return logProbNorm;
    default:
      return unknownNorm;
    }
  }

}
