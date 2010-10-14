#ifndef NEW_IBTREE_H
#define NEW_IBTREE_H

#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <stdexcept>

namespace Timbl {

  class rfCmp {
  public:
    bool operator()( const FeatureValue* F, const FeatureValue* G ) const{
      return F->Index() < G->Index();
    }
  };  
  
  class NewIBroot;

  class NewIBTree {
    friend class NewIBroot;
    friend std::ostream& operator<< ( std::ostream&, const NewIBTree& );
    friend std::ostream& operator<< ( std::ostream&, const NewIBTree* );
  public:
    typedef std::map<FeatureValue *, NewIBTree *, rfCmp> IBmap;
  NewIBTree(): TValue(0), TDistribution(0){};
    virtual ~NewIBTree(){};
    virtual void put( std::ostream&, int ) const = 0;
    virtual void assign_defaults( bool, bool, bool, size_t ) = 0; 
    virtual ValueDistribution *sum_distributions( bool ) = 0;
    virtual void redoDistributions() = 0;
    virtual ValueDistribution *getDistribution( bool ) = 0;
    virtual const ValueDistribution *match( const Instance&,
					    unsigned int ) const = 0;
    virtual NewIBTree *find( FeatureValue * ) const = 0;
    virtual void save( std::ostream & ) const = 0;
    virtual void saveHashed( std::ostream & ) const = 0;
    virtual void prune( unsigned int& ) = 0;
    virtual bool addInst( const Instance &, 
			  unsigned int,
			  unsigned int,
			  unsigned int&,
			  unsigned int& ) =0;
    virtual void delInst( const Instance&, unsigned int, unsigned int& ) =0;
    virtual unsigned int size() const = 0;
    virtual IBmap *getMap() = 0;
    virtual bool isLeaf() const = 0;
    virtual void countBranches( unsigned int l, 
			   std::vector<unsigned int>& terminals,
			   std::vector<unsigned int>& nonTerminals ) = 0;
    const TargetValue *TValue;
    ValueDistribution *TDistribution;
  };

 class NewIBleaf: public NewIBTree {
  public:
    ~NewIBleaf();
    void put( std::ostream&, int ) const;
    ValueDistribution *getDistribution( bool );
    IBmap *getMap() { return 0; };
  private:
    bool addInst( const Instance &, unsigned int, unsigned int,
		  unsigned int&, unsigned int& );
    void delInst( const Instance&, unsigned int, unsigned int& );
    void save( std::ostream & ) const;
    void saveHashed( std::ostream & ) const;
    void assign_defaults( bool, bool, bool, size_t ){}; 
    void redoDistributions();
    ValueDistribution *sum_distributions( bool ){ return 0; };
    const ValueDistribution *match( const Instance&, unsigned int ) const;
    NewIBTree *find( FeatureValue * ) const { return 0; };
    void prune( unsigned int& ){ };
    unsigned int size() const { return 0; } ;
    bool isLeaf() const { return true; };
    void countBranches( unsigned int, 
			std::vector<unsigned int>&,
			std::vector<unsigned int>& );
  };
  
  class NewIBbranch: public NewIBTree {
    friend class IBiter;
  public:
    ~NewIBbranch();
    void put( std::ostream&, int ) const;
    ValueDistribution *getDistribution( bool );
    IBmap *getMap() { return &_mmap; };
    void assign( FeatureValue* fv, NewIBTree *t ) {
      _mmap[fv] = t; };
  private:
    void save( std::ostream & ) const;
    void saveHashed( std::ostream & ) const;
    bool addInst( const Instance&,
		  unsigned int, 
		  unsigned int, 
		  unsigned int&, 
		  unsigned int& );
    void delInst( const Instance&, unsigned int, unsigned int& );
    void assign_defaults( bool, bool, bool, size_t );
    void redoDistributions();
    ValueDistribution *sum_distributions( bool );
    const ValueDistribution *match( const Instance&, unsigned int ) const;
    NewIBTree *find( FeatureValue * ) const;
    void prune( unsigned int& );
    unsigned int size() const {return _mmap.size(); };
    bool isLeaf() const { return _mmap.empty(); };
    void countBranches( unsigned int, 
			std::vector<unsigned int>&,
			std::vector<unsigned int>& );
    IBmap _mmap;
  };

  class IBiter {
  public:
  IBiter(): _map(0){};
    void init( NewIBTree * );
    void reset();
    void increment() { ++mit; };
    NewIBTree* find( FeatureValue * );
    NewIBTree* value();
    FeatureValue* FValue();
  private:
    NewIBTree::IBmap::const_iterator mit;
    NewIBTree::IBmap *_map;
  };
  
  class NewIBroot {
    friend std::ostream& operator<< ( std::ostream&, const NewIBTree& );
    friend std::ostream& operator<< ( std::ostream&, const NewIBTree* );
  public:
    NewIBroot( int, bool, bool );
    ~NewIBroot();
    NewIBroot* IBPartition( NewIBTree *sub, size_t dep ) const;
    void assignDefaults();
    void redoDistributions();
    void assignDefaults( size_t );
    bool addInstance( const Instance & );
    void deleteInstance( const Instance & );
    void save( std::ostream &, bool );
    void saveHashed( std::ostream &, StringHash *, StringHash *, bool );
    bool read( std::istream&, std::vector<Feature *>&, 
	       Target *, int );
    bool readHashed( std::istream &, std::vector<Feature *>&, Target *,
		     StringHash *, StringHash *, int );
    void put( std::ostream & ) const;
    void prune( ); 
    bool isPruned() const { return _pruned; };
    bool persistentD() const { return _keepDist; };
    unsigned long int getSizeInfo( unsigned long int&, double & ) const;
    void summarizeNodes( std::vector<unsigned int>&, std::vector<unsigned int>& );
    void deleteCopy( bool );
    NewIBroot *copy() const;
    const ValueDistribution *exactMatch( const Instance& ) const;
    const TargetValue *topTarget( bool& );
    const ValueDistribution *IG_test( const Instance&, 
				      size_t&,
				      bool&,
				      const TargetValue *& );
    NewIBroot *TRIBL_test( const Instance&,
			   size_t,
			   const TargetValue *&,
			   const ValueDistribution *&, 
			   size_t& ); 

    NewIBroot *TRIBL2_test( const Instance&,
			    const ValueDistribution *&, 
			    size_t& ); 

    const ValueDistribution *initTest( std::vector<FeatureValue *>&,
				       const Instance *,
				       size_t, size_t );
    const ValueDistribution *nextTest( std::vector<FeatureValue *>&, 
				       size_t& );

  protected:
    unsigned int _depth;
    bool _random;
    bool _keepDist;
    NewIBTree *_root;
  private:
    NewIBTree *readTree( std::istream&, std::vector<Feature *>&, 
			 Target *, int );
    NewIBTree *readTreeHashed( std::istream &, 
			       std::vector<Feature *>&, Target *, int );
    void readMapHashed( std::istream &, NewIBbranch *, 
			std::vector<Feature*>&, Target *, int );
    void readMap( std::istream &, NewIBbranch *,
		  std::vector<Feature*>&, Target *, int );
    int _version;
    bool _defValid;
    bool _defAss;
    bool _pruned;
    size_t _treshold;
    unsigned int _nodeCount;
    unsigned int _leafCount;
    const TargetValue *topTV;
    bool tiedTop;
    ValueDistribution *topDist;
    ValueDistribution *WTop;

    const Instance *testInst;
    unsigned int offSet;
    unsigned int effFeat;
    bool *RestartSearch;
    NewIBTree **SkipSearch;
    IBiter *InstPath;
  };

  std::ostream& operator<< ( std::ostream&, const NewIBroot& );
  std::ostream& operator<< ( std::ostream&, const NewIBroot* );

}
#endif
