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

#ifndef TIMBL_TESTERS_H
#define TIMBL_TESTERS_H

namespace Timbl{
  class metricTestFunction {
  public:
    virtual ~metricTestFunction(){};
    virtual double test( const FeatureValue *,
			 const FeatureValue *,
			 const Feature * ) const = 0;
  };

  class overlapTestFunction: public metricTestFunction {
  public:
    double test( const FeatureValue *FV,
		 const FeatureValue *G,
		 const Feature *Feat ) const override;
  };

  class valueDiffTestFunction: public metricTestFunction {
  public:
    explicit valueDiffTestFunction( int t ):
    metricTestFunction(),
      threshold( t )
      {};
    double test( const FeatureValue *,
		 const FeatureValue *,
		 const Feature * ) const override;
  protected:
    int threshold;
  };

  class TesterClass {
  public:
    TesterClass( const Feature_List& );
    TesterClass( const TesterClass& ) = delete; // inhibit copies
    TesterClass& operator=( const TesterClass& ) = delete; // inhibit copies
    virtual ~TesterClass(){};
    void init( const Instance&, size_t, size_t );
    virtual size_t test( const std::vector<FeatureValue *>&,
			 size_t,
			 double ) = 0;
    virtual double getDistance( size_t ) const = 0;
  protected:
    size_t _size;
    size_t effSize;
    size_t offSet;
    const std::vector<FeatureValue *> *FV;
    const std::vector<Feature *> &features;
    const std::vector<size_t> &permutation;
    std::vector<Feature *> permFeatures;
    std::vector<double> distances;
  private:
  };

  class DistanceTester: public TesterClass {
  public:
    DistanceTester( const Feature_List&,
		    int );
    ~DistanceTester();
    double getDistance( size_t ) const override;
    size_t test( const std::vector<FeatureValue *>&,
		 size_t,
		 double ) override;
  private:
    std::vector<metricTestFunction*> metricTest;
  };

  class SimilarityTester: public TesterClass {
  public:
    explicit SimilarityTester( const Feature_List& pf ):
      TesterClass( pf ){};
    ~SimilarityTester() {};
    virtual size_t test( const std::vector<FeatureValue *>&,
			 size_t,
			 double ) override = 0;
  protected:
  private:
  };

  class CosineTester: public SimilarityTester {
  public:
    explicit CosineTester( const Feature_List& pf ):
      SimilarityTester( pf ){};
    double getDistance( size_t ) const override;
    size_t test( const std::vector<FeatureValue *>&,
		 size_t,
		 double ) override;
  private:
  };

  class DotProductTester: public SimilarityTester {
  public:
    explicit DotProductTester( const Feature_List& pf ):
      SimilarityTester( pf ){};
    double getDistance( size_t ) const override;
    size_t test( const std::vector<FeatureValue *>&,
		 size_t,
		 double ) override;
  private:
  };

  TesterClass* getTester( MetricType,
			  const Feature_List&,
			  int );

}

#endif // TIMBL_TESTERS_H
