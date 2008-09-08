#ifndef TESTERS_H
#define TESTERS_H

namespace Timbl{

  static const int maxSimilarity = INT_MAX;
  

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
  
  class TesterClass {
  public:
    TesterClass( const std::vector<Feature*>&, 
		 const size_t *, 
		 size_t );
    virtual ~TesterClass();
    void reset( size_t, MetricType, int );
    virtual size_t test( const std::vector<FeatureValue *>& FV,
		 std::vector<FeatureValue *>& G, 
		 std::vector<double>& Distances,
		 size_t CurPos,
		 size_t Size,
		 size_t ib_offset,
		 double Threshold_plus ) const =0;
    virtual size_t test_sim( const std::vector<FeatureValue *>& FV,
		 std::vector<FeatureValue *>& G, 
		 std::vector<double>& Distances,
		 size_t CurPos,
		 size_t Size,
		 size_t ib_offset ) const = 0;
    virtual size_t test_ex( const std::vector<FeatureValue *>& FV,
		 std::vector<FeatureValue *>& G, 
		 std::vector<double>& Distances,
		 size_t CurPos,
		 size_t Size,
		 size_t ib_offset,
		 double,
		 double& ) const = 0;
    virtual size_t test_sim_ex( const std::vector<FeatureValue *>& FV,
				std::vector<FeatureValue *>& G, 
				std::vector<double>& Distances,
				size_t CurPos,
				size_t Size,
				size_t ib_offset,
				double,
				double& ) const = 0;
  protected:
    size_t _size;
    metricTester **test_feature_val;
    const std::vector<Feature *> &features;
    std::vector<Feature *> permFeatures;
    const size_t *permutation;
    std::vector<double> distances;
  };
  
  class DefaultTester: public TesterClass {
  public:
  DefaultTester( std::vector<Feature*>& pf, size_t *p, size_t s ): 
    TesterClass( pf, p, s ){};  
    size_t test( const std::vector<FeatureValue *>& FV,
		 std::vector<FeatureValue *>& G, 
		 std::vector<double>& Distances,
		 size_t CurPos,
		 size_t Size,
		 size_t ib_offset,
		 double Threshold_plus ) const; 
    size_t test_ex( const std::vector<FeatureValue *>&,
		    std::vector<FeatureValue *>&, 
		    std::vector<double>& ,
		    size_t,
		    size_t,
		    size_t,
		    double,
		    double& ) const;
    size_t test_sim( const std::vector<FeatureValue *>&,
		     std::vector<FeatureValue *>&, 
		     std::vector<double>& ,
		     size_t,
		     size_t,
		     size_t ) const;
    size_t test_sim_ex( const std::vector<FeatureValue *>&,
			std::vector<FeatureValue *>&, 
			std::vector<double>& ,
			size_t,
			size_t,
			size_t,
			double,
			double& ) const;
  };
  
  class ExemplarTester: public TesterClass {
  public:
  ExemplarTester( std::vector<Feature*>& pf, size_t *p, size_t s ): 
    TesterClass( pf, p, s ){};  
    size_t test( const std::vector<FeatureValue *>&,
		 std::vector<FeatureValue *>&, 
		 std::vector<double>&,
		 size_t,
		 size_t,
		 size_t,
		 double ) const;
    size_t test_sim( const std::vector<FeatureValue *>&,
		     std::vector<FeatureValue *>&, 
		     std::vector<double>& ,
		     size_t,
		     size_t,
		     size_t ) const;
    size_t test_ex( const std::vector<FeatureValue *>& FV,
		    std::vector<FeatureValue *>& G, 
		    std::vector<double>& Distances,
		    size_t CurPos,
		    size_t Size,
		    size_t ib_offset,
		    double ExWeight,
		    double& Distance ) const;
    size_t test_sim_ex( const std::vector<FeatureValue *>&,
			std::vector<FeatureValue *>&, 
			std::vector<double>& ,
			size_t,
			size_t,
			size_t,
			double,
			double& ) const;
  };

  class CosineTester: public TesterClass {
  public:
  CosineTester( std::vector<Feature*>& pf, size_t *p, size_t s ): 
    TesterClass( pf, p, s ){};  
    size_t test( const std::vector<FeatureValue *>&,
		 std::vector<FeatureValue *>&, 
		 std::vector<double>&,
		 size_t,
		 size_t,
		 size_t,
		 double ) const;
    size_t test_sim( const std::vector<FeatureValue *>& FV,
		     std::vector<FeatureValue *>& G, 
		     std::vector<double>& Distances,
		     size_t CurPos,
		     size_t Size,
		     size_t ib_offset ) const;
    size_t test_ex( const std::vector<FeatureValue *>&,
		    std::vector<FeatureValue *>&, 
		    std::vector<double>&,
		    size_t,
		    size_t,
		    size_t,
		    double,
		    double& ) const;
    size_t test_sim_ex( const std::vector<FeatureValue *>&,
			std::vector<FeatureValue *>&, 
			std::vector<double>& ,
			size_t,
			size_t,
			size_t,
			double,
			double& ) const;
  };
  
  class DotProductTester: public TesterClass {
  public:
  DotProductTester( std::vector<Feature*>& pf, size_t *p, size_t s ): 
    TesterClass( pf, p, s ){};  
    size_t test( const std::vector<FeatureValue *>&,
		 std::vector<FeatureValue *>&, 
		 std::vector<double>&,
		 size_t,
		 size_t,
		 size_t,
		 double ) const;
    size_t test_sim( const std::vector<FeatureValue *>& FV,
		     std::vector<FeatureValue *>& G, 
		     std::vector<double>& Distances,
		     size_t CurPos,
		     size_t Size,
		     size_t ib_offset ) const;
    size_t test_ex( const std::vector<FeatureValue *>&,
		    std::vector<FeatureValue *>&, 
		    std::vector<double>&,
		    size_t,
		    size_t,
		    size_t,
		    double,
		    double& ) const;
    size_t test_sim_ex( const std::vector<FeatureValue *>& FV,
			std::vector<FeatureValue *>& G, 
			std::vector<double>& Distances,
			size_t CurPos,
			size_t Size,
			size_t ib_offset,
			double ExWeight,
			double& Distance ) const;
  };
  
}  

#endif // TESTERS_H
