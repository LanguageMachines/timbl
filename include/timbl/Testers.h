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

#ifndef TESTERS_H
#define TESTERS_H

namespace Timbl{
  class metricTestFunction {
  public:
    virtual ~metricTestFunction(){};
    virtual double test( FeatureValue *, 
			 FeatureValue *, 
			 Feature * ) const = 0;
  };

  class overlapTestFunction: public metricTestFunction {
  public:
    double test( FeatureValue *FV,
		 FeatureValue *G,
		 Feature *Feat ) const;
  };

  class valueDiffTestFunction: public metricTestFunction {
  public:
  valueDiffTestFunction( int t ): metricTestFunction(), threshold( t ){};
    double test( FeatureValue *,
		 FeatureValue *,
		 Feature * ) const;
  protected:
    int threshold;
  };
  
  class TesterClass {
  public:
    TesterClass( const std::vector<Feature*>&, 
		 const std::vector<size_t> & );
    virtual ~TesterClass(){};
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
    const std::vector<Feature *> &features;
    std::vector<Feature *> permFeatures;
    const std::vector<size_t> &permutation;
    std::vector<double> distances;
  };
  
  class DistanceTester: public TesterClass {
  public:
    DistanceTester( const std::vector<Feature*>&, 
		    const std::vector<size_t>&,
		    int );
    ~DistanceTester();
    double getDistance( size_t ) const;
    size_t test( std::vector<FeatureValue *>&, 
		 size_t,
		 double ); 
  private:
    metricTestFunction **metricTest;

  };
  
  class SimilarityTester: public TesterClass {
  public:
  SimilarityTester( const std::vector<Feature*>& pf,
		    const std::vector<size_t>& p ): 
    TesterClass( pf, p ){};
    ~SimilarityTester() {};
    double getDistance( size_t ) const;
    virtual size_t test( std::vector<FeatureValue *>&, 
			 size_t,
			 double ) = 0;
  };
  
  class CosineTester: public SimilarityTester {
  public:
  CosineTester( const std::vector<Feature*>& pf,
		const std::vector<size_t>& p ): 
    SimilarityTester( pf, p ){};  
    size_t test( std::vector<FeatureValue *>&, 
		 size_t,
		 double );
  };
  
  class DotProductTester: public SimilarityTester {
  public:
  DotProductTester( const std::vector<Feature*>& pf,
		    const std::vector<size_t>& p ): 
    SimilarityTester( pf, p ){};  
    size_t test( std::vector<FeatureValue *>&, 
		 size_t,
		 double );
  };

  TesterClass* getTester( MetricType,
			  const std::vector<Feature*>&, 
			  const std::vector<size_t>&,
			  int );

}  

#endif // TESTERS_H
