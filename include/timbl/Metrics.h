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

  class distanceMetricClass: public metricClass {
  public:
  distanceMetricClass( MetricType m ): metricClass(m){};
    virtual ~distanceMetricClass() {};
    bool isSimilarityMetric() const { return false; };
    virtual bool isNumerical() const = 0;
    virtual bool isStorable() const = 0;
    virtual double distance( FeatureValue *, FeatureValue *, size_t=1 ) const = 0;
  };

  class OverlapMetric: public distanceMetricClass {
  public:
  OverlapMetric(): distanceMetricClass( Overlap ){};
    bool isNumerical() const { return false; };
    bool isStorable() const { return false; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };
  
  class NumericMetric: public distanceMetricClass {
  public:
  NumericMetric(): distanceMetricClass( Numeric ){};
    bool isNumerical() const { return true; };
    bool isStorable() const { return false; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };

  class ValueDiffMetric: public distanceMetricClass {
  public:
  ValueDiffMetric(): distanceMetricClass( ValueDiff ){};
    bool isNumerical() const { return false; };
    bool isStorable() const { return true; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };
  
  class DiceMetric: public distanceMetricClass {
  public:
  DiceMetric(): distanceMetricClass( Dice ){};
    bool isNumerical() const { return false; };
    bool isStorable() const { return true; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };

  class JeffreyMetric: public distanceMetricClass {
  public:
  JeffreyMetric(): distanceMetricClass( JeffreyDiv ){};
    bool isNumerical() const { return false; };
    bool isStorable() const { return true; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };

  class LevenshteinMetric: public distanceMetricClass {
  public:
  LevenshteinMetric(): distanceMetricClass( Levenshtein ){};
    bool isNumerical() const { return false; };
    bool isStorable() const { return true; };
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };

  class similarityMetricClass: public metricClass {
  public:
  similarityMetricClass( MetricType m ): metricClass( m ){};
    bool isSimilarityMetric() const { return true; };
    bool isNumerical() const { return true; };
    bool isStorable() const { return false; };
  };

  class CosineMetric: public similarityMetricClass {
  public:
  CosineMetric(): similarityMetricClass( Cosine ){};
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };

  class DotProductMetric: public similarityMetricClass {
  public:
  DotProductMetric(): similarityMetricClass( DotProduct ){};
    double distance( FeatureValue *, FeatureValue *, size_t ) const;
  };

}  

#endif // METRICS_H
