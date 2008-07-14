#ifndef TESTERS_H
#define TESTERS_H

namespace Timbl{
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
		 Feature *Feat ) const {
      double result = 0.0;
      if ( !FV->isUnknown() && FV->ValFreq() == 0 )
	result = 1.0;
      else if ( FV != G ){
	result = Feat->Weight();
      }
      return result;
    }
  };

  inline bool FV_to_real( FeatureValue *FV, double &result ){
    if ( FV ){
      if ( stringTo<double>( FV->Name(), result ) )
	return true;
    }
    return false;
  }  
  
  class numericOverlapTester: public metricTester {
  public:
    double test( FeatureValue *FV,
		 FeatureValue *G,
		 Feature *Feat ) const {
      double r1, r2, result;
      if ( FV_to_real( FV, r1 ) &&
	   FV_to_real( G, r2 ) )
	result = fabs( (r1-r2)/
		       (Feat->Max() - Feat->Min()));
      else
	result = 1.0;
      result *= Feat->Weight();
      return result;
    }
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
		 Feature *Feat ) const {
      double result = Feat->ValueDistance( F, G, threshold );
      result *= Feat->Weight();
      return result;
    }
  };
  
  class jeffreyDiffTester: public valueTester {
  public:
  jeffreyDiffTester( int t ): valueTester( t){};
    double test( FeatureValue *F,
		 FeatureValue *G,
		 Feature *Feat ) const {
      double result = Feat->JeffreyDistance( F, G, threshold );
      result *= Feat->Weight();
      return result;
    }
  };
  
  class levenshteinTester: public valueTester {
  public:
  levenshteinTester( int t ): valueTester( t){};
    double test( FeatureValue *F,
		 FeatureValue *G,
		 Feature *Feat ) const {
      double result = Feat->LevenshteinDistance( F, G, threshold );
      result *= Feat->Weight();
      return result;
    }
  };

}
#endif // TESTERS_H
