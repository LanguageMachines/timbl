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
    friend std::ostream& operator<< ( std::ostream&, const NewIBTree * );
  public:
    typedef std::map<FeatureValue *, NewIBTree *, rfCmp> IBmap;
  NewIBTree(): TValue(0), TDistribution(0){};
    virtual ~NewIBTree(){};
    virtual void put( std::ostream&, int ) const;
    virtual void assign_defaults( bool, bool, bool, size_t ) = 0; 
    virtual ValueDistribution *sum_distributions( bool ) = 0;
    virtual ValueDistribution *getDistribution( bool ) = 0;
    virtual const ValueDistribution *match( const Instance&,
					    unsigned int ) const = 0;
    virtual const NewIBTree *find( FeatureValue * ) const = 0;
    virtual void save( std::ostream & ) const = 0;
    virtual void prune( const TargetValue *, unsigned int& ) = 0;
    virtual bool addInst( const Instance &, 
			  unsigned int,
			  unsigned int&,
			  unsigned int& ) =0;
    virtual void delInst( const Instance&, unsigned int, unsigned int& ) =0;
    virtual unsigned int size() const = 0;
    virtual IBmap *getMap() = 0;
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
    bool addInst( const Instance &, unsigned int, 
		  unsigned int&, unsigned int& );
    void delInst( const Instance&, unsigned int, unsigned int& );
    void save( std::ostream & ) const;
    void assign_defaults( bool, bool, bool, size_t ){}; 
    ValueDistribution *sum_distributions( bool ){};
    const ValueDistribution *match( const Instance&, unsigned int ) const;
    const NewIBTree *find( FeatureValue * ) const { return 0; };
    void prune( const TargetValue *, unsigned int& );
    unsigned int size() const { return 0; } ;
  };
  
  class NewIBbranch: public NewIBTree {
    friend class IBiter;
  public:
    ~NewIBbranch();
    void put( std::ostream&, int ) const;
    ValueDistribution *getDistribution( bool );
    IBmap *getMap() { return &_mmap; };
  private:
    void save( std::ostream & ) const;
    bool addInst( const Instance&, unsigned int, unsigned int&, unsigned int& );
    void delInst( const Instance&, unsigned int, unsigned int& );
    void assign_defaults( bool, bool, bool, size_t );
    ValueDistribution *sum_distributions( bool );
    const ValueDistribution *match( const Instance&, unsigned int ) const;
    const NewIBTree *find( FeatureValue * ) const;
    void prune( const TargetValue *, unsigned int& );
    unsigned int size() const {return _mmap.size(); };
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
  public:
    NewIBroot( int, bool, bool );
    ~NewIBroot();
    void assignDefaults();
    bool addInstance( const Instance & );
    void deleteInstance( const Instance & );
    void save( std::ostream &, bool );
    void put( std::ostream &  ) const;
    void prune( const TargetValue * = 0 ); 
    bool isPruned() const { return _pruned; };
    bool persistentD() const { return _keepDist; };
    unsigned long int getSizeInfo( unsigned long int&, double & ) const;
    void deleteCopy( bool );
    NewIBroot *copy() const;
    const ValueDistribution *exactMatch( const Instance& ) const;
    const TargetValue *topTarget( bool& );
    const ValueDistribution *IG_test( const Instance&, 
				      size_t&,
				      bool&,
				      const TargetValue *& );

    const ValueDistribution *initTest( std::vector<FeatureValue *>&,
				       const Instance *,
				       size_t, size_t );
    const ValueDistribution *nextTest( std::vector<FeatureValue *>&, 
				       size_t& );

  protected:
    int _depth;
    bool _random;
    bool _keepDist;
    NewIBTree *_root;
  private:
    int _version;
    bool _defValid;
    bool _defAss;
    bool _pruned;
    unsigned int _nodeCount;
    unsigned int _leafCount;
    const TargetValue *topTV;
    bool tiedTop;
    ValueDistribution *topDist;
    ValueDistribution *WTop;

    const Instance *testInst;
    unsigned int offSet;
    unsigned int effFeat;
    unsigned int depth;
    bool *RestartSearch;
    IBiter *SkipSearch;
    IBiter *InstPath;
  };

}
#endif
