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

#ifndef TIMBL_METRICS_H
#define TIMBL_METRICS_H

#include <exception>
#include <limits>

namespace Timbl{

  class FeatureValue;

  class metricClass {
  public:
    explicit metricClass( MetricType m ): _type(m){};
    virtual ~metricClass() {};
    MetricType type() const { return _type; };
    virtual bool isSimilarityMetric() const = 0;
    virtual bool isNumerical() const = 0;
    virtual bool isStorable() const = 0;
    virtual double distance( const FeatureValue *,
			     const FeatureValue *,
			     size_t=1, double = 1.0 ) const = 0;
    virtual double get_max_similarity() const {
      throw std::logic_error( "get_max_similarity not implemented for " +
			      TiCC::toString( _type ) );
    }
  private:
    MetricType _type;
  };

  metricClass *getMetricClass( MetricType );

  class distanceMetricClass: public metricClass {
  public:
    explicit distanceMetricClass( MetricType m ): metricClass(m){};
    virtual ~distanceMetricClass() {};
    bool isSimilarityMetric() const override { return false; };
  };

  class OverlapMetric: public distanceMetricClass {
  public:
  OverlapMetric(): distanceMetricClass( Overlap ){};
    bool isNumerical() const override { return false; };
    bool isStorable() const override { return false; };
    double distance( const FeatureValue *,
		     const FeatureValue *,
		     size_t,
		     double ) const override;
  };

  class NumericMetricClass: public distanceMetricClass {
  public:
    explicit NumericMetricClass( MetricType m ): distanceMetricClass( m ){};
    virtual ~NumericMetricClass() {};
    bool isNumerical() const override { return true; };
    bool isStorable() const override { return false; };
  };

  class NumericMetric: public NumericMetricClass {
  public:
  NumericMetric(): NumericMetricClass( Numeric ){};
    double distance( const FeatureValue *,
		     const FeatureValue *,
		     size_t,
		     double ) const override;
  };

  class EuclideanMetric: public NumericMetricClass {
  public:
  EuclideanMetric(): NumericMetricClass( Euclidean ){};
    double distance( const FeatureValue *,
		     const FeatureValue *,
		     size_t,
		     double ) const override;
  };

  class ValueDiffMetric: public distanceMetricClass {
  public:
  ValueDiffMetric(): distanceMetricClass( ValueDiff ){};
    bool isNumerical() const override { return false; };
    bool isStorable() const override { return true; };
    double distance( const FeatureValue *,
		     const FeatureValue *,
		     size_t,
		     double ) const override;
  };

  class DiceMetric: public distanceMetricClass {
  public:
  DiceMetric(): distanceMetricClass( Dice ){};
    bool isNumerical() const override { return false; };
    bool isStorable() const override { return true; };
    double distance( const FeatureValue *,
		     const FeatureValue *,
		     size_t,
		     double ) const override;
  };

  class JeffreyMetric: public distanceMetricClass {
  public:
  JeffreyMetric(): distanceMetricClass( JeffreyDiv ){};
    bool isNumerical() const override{ return false; };
    bool isStorable() const override { return true; };
    double distance( const FeatureValue *,
		     const FeatureValue *,
		     size_t,
		     double ) const override;
  };

  class JSMetric: public distanceMetricClass {
  public:
  JSMetric(): distanceMetricClass( JSDiv ){};
    bool isNumerical() const override { return false; };
    bool isStorable() const override { return true; };
    double distance( const FeatureValue *,
		     const FeatureValue *,
		     size_t,
		     double ) const override;
  };

  class LevenshteinMetric: public distanceMetricClass {
  public:
  LevenshteinMetric(): distanceMetricClass( Levenshtein ){};
    bool isNumerical() const override { return false; };
    bool isStorable() const override { return true; };
    double distance( const FeatureValue *,
		     const FeatureValue *,
		     size_t,
		     double ) const override;
  };

  class similarityMetricClass: public metricClass {
  public:
    explicit similarityMetricClass( MetricType m ): metricClass( m ){};
    virtual ~similarityMetricClass() {};
    bool isSimilarityMetric() const override { return true; };
    bool isNumerical() const override { return true; };
    bool isStorable() const override { return false; };
  };

  class CosineMetric: public similarityMetricClass {
  public:
  CosineMetric(): similarityMetricClass( Cosine ){};
    double distance( const FeatureValue *,
		     const FeatureValue *,
		     size_t,
		     double ) const override;
    double get_max_similarity() const override { return 1.0; };
  };

  class DotProductMetric: public similarityMetricClass {
  public:
  DotProductMetric(): similarityMetricClass( DotProduct ){};
    double distance( const FeatureValue *,
		     const FeatureValue *,
		     size_t,
		     double ) const override;
    double get_max_similarity() const override {
      return std::numeric_limits<int>::max();
    };
  };

}

#endif // TIMBL_METRICS_H
