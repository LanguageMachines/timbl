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

#include "timbl/Types.h"
#include "timbl/Metrics.h"

using namespace std;

namespace Timbl {
  metricType* metricType::create( const string& s ){
    metricType *mt = 0;
    MetricType type = stringTo<MetricType>( s );
    switch ( type ){
    case DefaultMetric:
      mt = new defaultMetric();
      break;
    case Ignore:
      mt = new ignoreMetric();
      break;
    case Numeric:
      mt = new numericMetric();
      break;
    case DotProduct:
      mt = new dotproductMetric();
      break;
    case Cosine:
      mt = new cosineMetric();
      break;
    case Overlap:
      mt = new overLapMetric();
      break;
    case Levenshtein:
      mt = new levenshteinMetric();
      break;
    case ValueDiff:
      mt = new valueDiffMetric();
      break;
    case JeffreyDiv:
      mt = new jeffreyMetric();
      break;
    default:
      throw( "unhandled metric type in switch: " + toString(type,true) );
    }
    return mt;
  }

}
