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
  NewIBTree(): TValue(0), TDistribution(0){};
    virtual ~NewIBTree(){};
    virtual void put( std::ostream&, int ) const;
    virtual void assign_defaults( bool, bool, bool, size_t ) = 0; 
    virtual ValueDistribution *sum_distributions( bool ) = 0;
    virtual ValueDistribution *getDistribution( bool ) = 0;
    virtual const ValueDistribution *match( const Instance&,
					    unsigned int ) const = 0;
    virtual void save( std::ostream & ) const = 0;
    virtual void prune( const TargetValue *, unsigned int& ) = 0;
    virtual void addInst( const Instance &, 
			  unsigned int,
			  unsigned int& ) =0;
    virtual void delInst( const Instance&, unsigned int, unsigned int& ) =0;
    virtual unsigned int size() const = 0;
    const TargetValue *TValue;
    ValueDistribution *TDistribution;
  };

 class NewIBleaf: public NewIBTree {
  public:
    ~NewIBleaf();
    void put( std::ostream&, int ) const;
    ValueDistribution *getDistribution( bool );
  private:
    void addInst( const Instance &, unsigned int, unsigned int& );
    void delInst( const Instance&, unsigned int, unsigned int& );
    void save( std::ostream & ) const;
    void assign_defaults( bool, bool, bool, size_t ){}; 
    ValueDistribution *sum_distributions( bool ){};
    const ValueDistribution *match( const Instance&, unsigned int ) const;
    void prune( const TargetValue *, unsigned int& );
    unsigned int size() const { return 0; } ;
  };
  
  class NewIBbranch: public NewIBTree {
  public:
    ~NewIBbranch();
    void put( std::ostream&, int ) const;
    ValueDistribution *getDistribution( bool );
  protected:
    mutable std::map<FeatureValue *, NewIBTree *, rfCmp>::const_iterator mit;
  private:
    void save( std::ostream & ) const;
    void addInst( const Instance&, unsigned int, unsigned int& );
    void delInst( const Instance&, unsigned int, unsigned int& );
    void assign_defaults( bool, bool, bool, size_t );
    ValueDistribution *sum_distributions( bool );
    const ValueDistribution *match( const Instance&, unsigned int ) const;
    void prune( const TargetValue *, unsigned int& );
    unsigned int size() const {return _mmap.size(); };
    std::map<FeatureValue *, NewIBTree *, rfCmp> _mmap;
  };
  
  class NewIBroot {
  public:
  NewIBroot( int depth, bool random, bool keep ): _depth(depth),
      _random(random), _keepDist(keep), _root(0), _version(4), 
      _defValid(false), _defAss(false), _pruned(false), _nodeCount(1),
      TopTarget(0), TopDist(0) {};
    ~NewIBroot();
    void assignDefaults();
    void addInstance( const Instance & );
    void deleteInstance( const Instance & );
    void Save( std::ostream &, bool );
    void Put( std::ostream &  ) const;
    void Prune( const TargetValue * = 0 );
    const ValueDistribution *exactMatch( const Instance& ) const;
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
    const TargetValue *TopTarget;
    ValueDistribution *TopDist;
  };

}
#endif
