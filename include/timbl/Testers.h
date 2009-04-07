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

#ifndef TESTERS_H
#define TESTERS_H

namespace Timbl{

  static const int maxSimilarity = INT_MAX;
  
  inline bool isSimilarityMetric( MetricType m ){
    return m == Cosine || m == DotProduct;
  }
  
  inline bool isNumericalMetric( MetricType m ){
    return m == Numeric || m == Cosine || m == DotProduct;
  }


  class metricTester {
  public:
    virtual ~metricTester(){};
    virtual double test( FeatureValue *, 
			 FeatureValue *, 
			 Feature * ) const = 0;
  };

  class overlapTester: public metricTester {
  public:
    double test( FeatureValue *FV,
		 FeatureValue *G,
		 Feature *Feat ) const;
  };

  class numericOverlapTester: public metricTester {
  public:
    double test( FeatureValue *FV,
		 FeatureValue *G,
		 Feature *Feat ) const;
  };
  

  class valueTester: public metricTester {
  public:
  valueTester( int t ): metricTester(), threshold( t ){};
  protected:
    int threshold;
  private:
    valueTester();
  };
  
  class valueDiffTester: public valueTester {
  public:
  valueDiffTester( int t ): valueTester( t){};
    double test( FeatureValue *F,
		 FeatureValue *G,
		 Feature *Feat ) const;
  };
  
  class jeffreyDiffTester: public valueTester {
  public:
  jeffreyDiffTester( int t ): valueTester( t){};
    double test( FeatureValue *F,
		 FeatureValue *G,
		 Feature *Feat ) const;
  };
  
  class levenshteinTester: public valueTester {
  public:
  levenshteinTester( int t ): valueTester( t){};
    double test( FeatureValue *F,
		 FeatureValue *G,
		 Feature *Feat ) const;
  };
  
  class diceTester: public valueTester {
  public:
  diceTester( int t ): valueTester( t){};
    double test( FeatureValue *F,
		 FeatureValue *G,
		 Feature *Feat ) const;
  };
  
  class TesterClass {
  public:
    TesterClass( const std::vector<Feature*>&, 
		 const std::vector<size_t> & );
    virtual ~TesterClass();
    void reset( MetricType, int );
    void init( const Instance&, size_t, size_t );
    virtual size_t test( std::vector<FeatureValue *>&, 
			 size_t,
			 double ) = 0;
    virtual double getDistance( size_t ) const = 0;
  protected:
    size_t _size;
    size_t effSize;
    size_t offSet;
    const std::vector<FeatureValue *> *FV;
    metricTester **test_feature_val;
    const std::vector<Feature *> &features;
    std::vector<Feature *> permFeatures;
    const std::vector<size_t> &permutation;
    std::vector<double> distances;
  };
  
  class DefaultTester: public TesterClass {
  public:
  DefaultTester( const std::vector<Feature*>& pf, 
		 const std::vector<size_t>& p ): 
    TesterClass( pf, p ){};  
    double getDistance( size_t ) const;
    size_t test( std::vector<FeatureValue *>&, 
		 size_t,
		 double ); 
  };
  
  class ExemplarTester: public TesterClass {
  public:
  ExemplarTester( const std::vector<Feature*>& pf,
		  const std::vector<size_t>& p ): 
    TesterClass( pf, p ){};  
    double getDistance( size_t ) const;
    size_t test( std::vector<FeatureValue *>&, 
		 size_t,
		 double );
  };

  class CosineTester: public TesterClass {
  public:
  CosineTester( const std::vector<Feature*>& pf,
		const std::vector<size_t>& p ): 
    TesterClass( pf, p ){};  
    double getDistance( size_t ) const;
    size_t test( std::vector<FeatureValue *>&, 
		 size_t,
		 double );
  };
  
  class DotProductTester: public TesterClass {
  public:
  DotProductTester( const std::vector<Feature*>& pf,
		    const std::vector<size_t>& p ): 
    TesterClass( pf, p ){};  
    double getDistance( size_t ) const;
    size_t test( std::vector<FeatureValue *>&, 
		 size_t,
		 double );
  };

}  

#endif // TESTERS_H
