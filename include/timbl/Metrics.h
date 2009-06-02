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

#ifndef METRICS_H
#define METRICS_H

namespace Timbl{
#include <climits> // for INT_MAX

  double lv_distance( const std::string&, const std::string& );
  double dc_distance( const std::string&, const std::string& );

  static const int maxSimilarity = INT_MAX;
  
  metricClass *getMetricClass( MetricType );

  class metricClass {
  public:
  metricClass( MetricType m ): _type(m){};
    virtual ~metricClass() {};
    MetricType type() const { return _type; };
    virtual bool isSimilarityMetric() const = 0;
    virtual bool isNumerical() const = 0;
    virtual bool isStorable() const = 0;
    virtual double distance( FeatureValue *, FeatureValue *, size_t=1 ) const = 0;
    metricClass *clone() const{ return getMetricClass(_type); };
  private:
    MetricType _type;
  };

  class OverlapMetric: public metricClass {
  public:
  OverlapMetric(): metricClass( Overlap ){};
    bool isSimilarityMetric() const { return false; };
    bool isNumerical() const { return false; };
    bool isStorable() const { return false; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };
  
  class ValueDiffMetric: public metricClass {
  public:
  ValueDiffMetric(): metricClass( ValueDiff ){};
    bool isSimilarityMetric() const { return false; };
    bool isNumerical() const { return false; };
    bool isStorable() const { return true; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };
  
  class NumericMetric: public metricClass {
  public:
  NumericMetric(): metricClass( Numeric ){};
    bool isSimilarityMetric() const { return false; };
    bool isNumerical() const { return true; };
    bool isStorable() const { return false; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };

  class CosineMetric: public metricClass {
  public:
  CosineMetric(): metricClass( Cosine ){};
    bool isSimilarityMetric() const { return true; };
    bool isNumerical() const { return true; };
    bool isStorable() const { return false; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };

  class DotProductMetric: public metricClass {
  public:
  DotProductMetric(): metricClass( DotProduct ){};
    bool isSimilarityMetric() const { return true; };
    bool isNumerical() const { return true; };
    bool isStorable() const { return false; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };

  class DiceMetric: public metricClass {
  public:
  DiceMetric(): metricClass( Dice ){};
    bool isSimilarityMetric() const { return false; };
    bool isNumerical() const { return false; };
    bool isStorable() const { return true; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };

  class JeffreyMetric: public metricClass {
  public:
  JeffreyMetric(): metricClass( JeffreyDiv ){};
    bool isSimilarityMetric() const { return false; };
    bool isNumerical() const { return false; };
    bool isStorable() const { return true; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };

  class LevenshteinMetric: public metricClass {
  public:
  LevenshteinMetric(): metricClass( Levenshtein ){};
    bool isSimilarityMetric() const { return false; };
    bool isNumerical() const { return false; };
    bool isStorable() const { return true; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };


}  

#endif // METRICS_H
