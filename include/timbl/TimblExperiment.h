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

#ifndef TIMBLEXP_H
#define TIMBLEXP_H

#include <fstream>
#include "timbl/XMLtools.h"
#include "timbl/BestArray.h"
#include "timbl/Statistics.h"
#include "timbl/MBLClass.h"

namespace Timbl {

  class TimblAPI;
  class ConfusionMatrix;
  class CL_Options;
  class GetOptClass;
  
  class resultStore: public MsgClass {
  public:
  resultStore(): dist(0), disposable(false), isTop(false), beam(0) {};
    ~resultStore();
    void reset( int, normType, double, const Target * );
    void clear();
    void addConstant( const ValueDistribution * );
    void addTop( const ValueDistribution * );
    void addDisposable( ValueDistribution * );
    const WValueDistribution *getResultDist();
    std::string getResult();
    void prepare();
    void normalize();
  private:
    const ValueDistribution *rawDist;
    WValueDistribution *dist;
    bool disposable;
    bool isTop;
    int beam;
    normType norm;
    double factor;
    const Target *targets;
    std::string topCache;
    std::string resultCache;
  };

  typedef std::multimap<FeatureValue*,std::streamsize > MultiIndex;
  class fCmp {
  public:
    bool operator()( const FeatureValue* F, const FeatureValue* G ) const{
      return F->Index() > G->Index();
    }
  };  
  typedef std::map<FeatureValue*, MultiIndex, fCmp > featureMultiIndex;
  std::ostream& operator<< ( std::ostream&, const featureMultiIndex& );
  std::ostream& operator<< ( std::ostream&, const MultiIndex& );
  void compressIndex( const featureMultiIndex&,
		      featureMultiIndex& res, 
		      unsigned int );
  typedef std::multimap<FeatureValue*, std::streamsize, fCmp > featureIndex;
  std::ostream& operator<< ( std::ostream&, const featureIndex& );
  
  class TimblExperiment: public MBLClass {
    friend class TimblAPI;
  public:
    virtual ~TimblExperiment();
    virtual TimblExperiment *clone() const = 0;
    TimblExperiment& operator=( const TimblExperiment& );
    virtual bool Learn( const std::string& = "" );
    virtual bool Prepare( const std::string& = "" );
    virtual bool CVprepare( const std::string& = "",
			    WeightType = GR_w,
			    const std::string& = "" );
    virtual bool Increment( const std::string& )
    { FatalError( "Increment" ); return false; };
    virtual bool Decrement( const std::string& )
    { FatalError( "Decrement" ); return false; };
    virtual bool Expand( const std::string& ){
      FatalError( "Expand" ); return false; };
    virtual bool Remove( const std::string& ){
    FatalError( "Remove" ); return false;};
    virtual bool Test( const std::string&,
		       const std::string& );
    virtual bool NS_Test( const std::string&,
			  const std::string& );
    virtual void InitInstanceBase() = 0;
    virtual bool ReadInstanceBase( const std::string& );
    virtual bool WriteInstanceBase( const std::string& );
    bool chopLine( const std::string& );
    bool WriteInstanceBaseXml( const std::string& );
    bool WriteInstanceBaseLevels( const std::string&, unsigned int );
    bool WriteNamesFile( const std::string& ) const;
    int Estimate() const { return estimate; };
    void Estimate( int e ){ estimate = e; };
    void setOutPath( const std::string& s ){ outPath = s; };
    TimblExperiment *CreateClient( int  ) const;
    TimblExperiment *splitChild() const;
    bool SetOptions( int, const char ** );
    bool SetOptions( const std::string& );
    bool SetOptions( const CL_Options&  );
    bool IndirectOptions( const CL_Options&  );
    bool ConfirmOptions();
    bool DefaultOptions();
    GetOptClass *getOptParams() const { return OptParams; };
    void setOptParams( GetOptClass *op ) { OptParams = op; };
    bool WriteArrays( const std::string& );
    bool GetArrays( const std::string& );
    bool WriteMatrices( const std::string& );
    bool GetMatrices( const std::string& );
    bool SaveWeights( const std::string& );
    bool GetWeights( const std::string&, WeightType );
    bool GetCurrentWeights( std::vector<double>& );
    xmlNode *weightsToXML();
    bool ShowOptions( std::ostream& );
    bool ShowSettings( std::ostream& );
    xmlNode *settingsToXML();
    bool showBestNeighbors( std::ostream& ) const;
    xmlNode *bestNeighborsToXML() const;
    bool showStatistics( std::ostream& ) const;
    void showInputFormat( std::ostream& ) const;
    const std::string& ExpName() const { return exp_name; };
    void setExpName( const std::string& s ) { exp_name = s; };
    bool Classify( const std::string& , std::string& );
    bool Classify( const std::string& , std::string&, double& );
    bool Classify( const std::string& , std::string&, std::string&, double& );
    size_t matchDepth() const { return match_depth; };
    bool matchedAtLeaf() const { return last_leaf; };
    
    virtual AlgorithmType Algorithm() const = 0;
    const TargetValue *Classify( const std::string& Line, 
				 const ValueDistribution *& db,
				 double& di ){
      const TargetValue *res = classifyString( Line, di );
      if ( res ){
	normalizeResult();
	db = bestResult.getResultDist();
      }
      return res;
    }
    const TargetValue *Classify( const std::string& Line ){
      double dum_d;
      return classifyString( Line, dum_d  );
    }
    
    const TargetValue *Classify( const std::string& Line, 
				 const ValueDistribution *& db ){
      double dum_d;
      const TargetValue *res = classifyString( Line, dum_d );
      if ( res ){
	normalizeResult();
	db = bestResult.getResultDist();
      }
      return res;
    }
    const TargetValue *Classify( const std::string& Line, 
				 double& di ){
      return classifyString( Line, di );
    }
    
    const neighborSet *NB_Classify( const std::string& );
    
    bool LocalTest( const Instance& , std::ostream& );    
    virtual void initExperiment( bool = false );
    
  protected:
    TimblExperiment( const AlgorithmType, const std::string& = "" );
    virtual bool checkLine( const std::string& );
    virtual const TargetValue *LocalClassify( const Instance& , 
					      double&,
					      bool& );
    
    virtual bool GetInstanceBase( std::istream& ) = 0;
    virtual void showTestingInfo( std::ostream& );
    virtual bool checkTestFile();
    bool initTestFiles( const std::string&, const std::string& );
    void show_results( std::ostream&,
		       const std::string&, 
		       const TargetValue *,
		       const double ) ;
    void testInstance( const Instance&,
		       InstanceBase_base *,
		       size_t = 0 );
    void normalizeResult();
    const neighborSet *LocalClassify( const Instance&  );
    bool nextLine( std::istream &, std::string& );
    bool skipARFFHeader( std::istream & );
    
    void show_progress( std::ostream& os, time_t );
    bool createPercFile( const std::string& = "" ) const;
    
    void show_speed_summary( std::ostream& os,
			     const timeval& ) const;
    
    void show_ignore_info( std::ostream& os ) const;
    void show_weight_info( std::ostream& os ) const;
    void show_metric_info( std::ostream& os ) const;
    double sum_remaining_weights( size_t ) const;

    bool build_file_index( const std::string&, featureIndex&  );

    bool Initialized;
    GetOptClass *OptParams;
    AlgorithmType algorithm;
    std::string CurrentDataFile;
    std::string WFileName;
    std::string outPath;
    std::string testStreamName;
    std::string outStreamName;
    std::ifstream testStream;
    std::ofstream outStream;
    unsigned long ibCount; 
    ConfusionMatrix *confusionInfo;
    StatisticsClass stats;
    resultStore bestResult;
    size_t match_depth;
    bool last_leaf;

  private:
    TimblExperiment( const TimblExperiment& );
    int estimate;
    const TargetValue *classifyString( const std::string& , double& );
  }; 

  class IB1_Experiment: public TimblExperiment {
  public:
    IB1_Experiment( const size_t N = DEFAULT_MAX_FEATS, 
		    const std::string& s= "",
		    const bool init = true );
    bool Increment( const std::string& );
    bool Decrement( const std::string& );
    bool Expand( const std::string& );
    bool Remove( const std::string& );
    AlgorithmType Algorithm() const { return IB1_a; };
    void InitInstanceBase();
    bool NS_Test( const std::string&,
		  const std::string& );
  protected:
    TimblExperiment *clone() const { 
      return new IB1_Experiment( MaxFeats(), "", false ); 
    };
    bool checkTestFile();
    bool checkLine( const std::string& );
    bool Increment( const Instance& I ) { return UnHideInstance( I ); };
    bool Decrement( const Instance& I ) { return HideInstance( I ); };
  private:
    bool GetInstanceBase( std::istream& );
  };
  
  class IB2_Experiment: public IB1_Experiment {
  public:
  IB2_Experiment( size_t N, const std::string& s="" ): 
    IB1_Experiment( N, s ) {
      IB2_offset( 0 );
    }; 
    bool Prepare( const std::string& = "" );
    bool Expand( const std::string& );
    bool Remove( const std::string& );
    bool Learn( const std::string& = "" );
    AlgorithmType Algorithm() const { return IB2_a; };
  protected:
    bool checkTestFile( );
    TimblExperiment *clone() const { return new IB2_Experiment( MaxFeats() ); };
    bool Expand_N( const std::string& );
    bool show_learn_progress( std::ostream& os, time_t, size_t );
  };
  
  class LOO_Experiment: public IB1_Experiment {
  public:
  LOO_Experiment( int N, const std::string& s = "" ): 
    IB1_Experiment( N, s ) {
    };
    bool Test( const std::string&,
	       const std::string& );
    AlgorithmType Algorithm() const { return LOO_a; };
    bool ReadInstanceBase( const std::string& );
    void initExperiment( bool = false );
  protected:
    bool checkTestFile( );
    void showTestingInfo( std::ostream& );
  };
  
  class CV_Experiment: public IB1_Experiment {
  public:
  CV_Experiment( int N = DEFAULT_MAX_FEATS, const std::string& s = "" ): 
    IB1_Experiment( N, s ), NumOfFiles( 0 ), FileNames( NULL )
      { };
    ~CV_Experiment(){ delete [] FileNames; };
    bool Learn( const std::string& = "" );
    bool Prepare( const std::string& = "" );
    bool Test( const std::string&,
	       const std::string& );
    bool CVprepare( const std::string& = "",
		    WeightType = GR_w,
		    const std::string& = "" );
    AlgorithmType Algorithm() const { return CV_a; };
  protected:
    bool checkTestFile();
    bool get_file_names( const std::string& );
  private:
    CV_Experiment( const CV_Experiment& );
    CV_Experiment& operator=( const CV_Experiment& );
    int NumOfFiles;
    std::string *FileNames;
    std::string CV_WfileName;
    std::string CV_PfileName;
    WeightType CV_fileW;
  };
  
  class TRIBL_Experiment: public TimblExperiment {
  public:
  TRIBL_Experiment( const size_t N = DEFAULT_MAX_FEATS, 
		    const std::string& s = "",
		    const bool init = true ): 
    TimblExperiment( TRIBL_a, s ) {
      if ( init ) InitClass( N );
    };
    void InitInstanceBase();
  protected:
    TimblExperiment *clone() const { 
      return new TRIBL_Experiment( MaxFeats(), "", false ); };
    void showTestingInfo( std::ostream& );
    bool checkTestFile();
    AlgorithmType Algorithm() const { return TRIBL_a; };
    bool checkLine( const std::string& );
    const TargetValue *LocalClassify( const Instance& , 
				      double&,
				      bool& );
  private:
    bool GetInstanceBase( std::istream& );
  };
  
  class TRIBL2_Experiment: public TimblExperiment {
  public:
  TRIBL2_Experiment( const size_t N = DEFAULT_MAX_FEATS, 
		     const std::string& s = "",
		     const bool init = true ): 
    TimblExperiment( TRIBL2_a, s ) {
      if ( init ) InitClass( N );
    };
    void InitInstanceBase();
  protected:
    TimblExperiment *clone() const {
      return new TRIBL2_Experiment( MaxFeats(), "", false ); };
    bool checkTestFile();
    AlgorithmType Algorithm() const { return TRIBL2_a; };
    bool checkLine( const std::string& );
    const TargetValue *LocalClassify( const Instance& ,
				      double&,
				      bool& );
  private:
    bool GetInstanceBase( std::istream& );
  };

  class IG_Experiment: public TimblExperiment {
  public:
  IG_Experiment( const size_t N = DEFAULT_MAX_FEATS,
		 const std::string& s = "",
		 const bool init = true ): 
    TimblExperiment( IGTREE_a, s ) { 
      if ( init ) InitClass( N );
    };
    bool Learn( const std::string& f = "" );
    AlgorithmType Algorithm() const { return IGTREE_a; };
    void InitInstanceBase();
    bool WriteInstanceBase( const std::string& );
    bool ReadInstanceBase( const std::string& );
    void initExperiment( bool = false );
  protected:
    TimblExperiment *clone() const { 
      return new IG_Experiment( MaxFeats(), "", false ); };
    bool checkTestFile();
    void showTestingInfo( std::ostream& );
    bool checkLine( const std::string& );
    bool sanityCheck() const;
    bool build_file_multi_index( const std::string&, featureMultiIndex&  );
    const TargetValue *LocalClassify( const Instance&,
				      double&,
				      bool& );
  private:
    
    bool GetInstanceBase( std::istream& );
  };
  
}

#endif // TIMBLEXP_H
