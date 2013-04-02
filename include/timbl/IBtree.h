/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2013
  ILK   - Tilburg University
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
      http://ilk.uvt.nl/software.html
  or send mail to:
      timbl@uvt.nl
*/
#ifndef TIMBL_IBTREE_H
#define TIMBL_IBTREE_H

#include "ticcutils/TreeHash.h"
#include "ticcutils/XMLtools.h"

//#define IBSTATS

namespace Timbl {

  class IB_InstanceBase;
  class IG_InstanceBase;
  class TRIBL_InstanceBase;
  class TRIBL2_InstanceBase;
  class Feature;
  class FeatureValue;
  class Instance;
  class Target;
  class TargetValue;
  class ValueDistribution;
  class WValueDistribution;

  class IBtree {
    friend class InstanceBase_base;
    friend class IB_InstanceBase;
    friend class IG_InstanceBase;
    friend class TRIBL_InstanceBase;
    friend class TRIBL2_InstanceBase;
    friend std::ostream &operator<<( std::ostream&, const IBtree& );
    friend std::ostream &operator<<( std::ostream&, const IBtree * );
    friend xmlNode *to_xml( IBtree *pnt );
    friend int count_next( const IBtree * );
  public:
    const TargetValue* targetValue() const { return TValue; };
  private:
    FeatureValue *FValue;
    const TargetValue *TValue;
    ValueDistribution *TDistribution;
    IBtree *link;
    IBtree *next;
    
    IBtree();
    IBtree( FeatureValue * );
    ~IBtree();
    IBtree *Reduce( const TargetValue *, unsigned long&, long );
#ifdef IBSTATS
    inline IBtree *add_feat_val( FeatureValue *, unsigned int&, IBtree **, unsigned long& );
#else
    inline IBtree *add_feat_val( FeatureValue *, IBtree **, unsigned long& );
#endif
    inline ValueDistribution *sum_distributions( bool );
    inline IBtree *make_unique( const TargetValue *, unsigned long& );
    void cleanDistributions();
    void re_assign_defaults( bool, bool ); 
    void assign_defaults( bool, bool, size_t ); 
    void redo_distributions();
    void countBranches( unsigned int,
			std::vector<unsigned int>&,
			std::vector<unsigned int>& );
    const ValueDistribution *exact_match( const Instance&  ) const;
  protected:
    const IBtree *search_node( FeatureValue * ) const;
    IBtree( const IBtree& );
    IBtree& operator=( const IBtree& );
  };

  typedef std::map<size_t, const IBtree*> FI_map;
  
  class InstanceBase_base: public MsgClass {
    friend class IG_InstanceBase;
    friend class TRIBL_InstanceBase;
    friend class TRIBL2_InstanceBase;
    InstanceBase_base( const InstanceBase_base& );
    InstanceBase_base& operator=( const InstanceBase_base& );
    friend std::ostream& operator<<( std::ostream &os, 
				     const InstanceBase_base& );
    friend std::ostream& operator<<( std::ostream &os, 
				     const InstanceBase_base * );
  public:
    InstanceBase_base( size_t, unsigned long&, bool, bool );
    virtual ~InstanceBase_base( void );
    void AssignDefaults( void );
    void RedoDistributions();
    bool AddInstance( const Instance&  );
    void RemoveInstance( const Instance&  );
    void summarizeNodes( std::vector<unsigned int>&,
			 std::vector<unsigned int>& );
    virtual bool MergeSub( InstanceBase_base * );
    const ValueDistribution *ExactMatch( const Instance& I ) const {
      return InstBase->exact_match( I ); };
    virtual const ValueDistribution *InitGraphTest( std::vector<FeatureValue *>&, 
						    const std::vector<FeatureValue *> *,
						    size_t,
						    size_t );
    virtual const ValueDistribution *NextGraphTest( std::vector<FeatureValue *>&, 
					      size_t& );
    unsigned long int GetDistSize( ) const { return NumOfTails; };
    virtual const ValueDistribution *IG_test( const Instance& , size_t&, bool&,
					      const TargetValue *& );
    virtual IB_InstanceBase *TRIBL_test( const Instance& , size_t,
					 const TargetValue *&,
					 const ValueDistribution *&, 
					 size_t& );
    virtual IB_InstanceBase *TRIBL2_test( const Instance& , 
					  const ValueDistribution *&,
					  size_t& );
    bool read_hash( std::istream &, Hash::StringHash *, Hash::StringHash * ) const;
    virtual InstanceBase_base *Copy() const = 0;
    virtual InstanceBase_base *clone() const = 0;
    void Save( std::ostream &, bool=false );
    void Save( std::ostream &, Hash::StringHash *, Hash::StringHash *, bool=false );
    void toXML( std::ostream& );
    void printStatsTree( std::ostream&, unsigned int startLevel );
    virtual bool ReadIB( std::istream&, std::vector<Feature *>&, 
			 Target *, int );
    virtual bool ReadIB( std::istream &, std::vector<Feature *>&, Target *,
			 Hash::StringHash *, Hash::StringHash *, int );
    virtual void Prune( const TargetValue *, long = 0 );
    virtual bool IsPruned() const { return false; };
    void CleanPartition(  bool );
    unsigned long int GetSizeInfo( unsigned long int&, double & ) const;
    const ValueDistribution *TopDist() const { return TopDistribution; };
    bool HasDistributions() const;
    const TargetValue *TopTarget( bool & );
    bool PersistentD() const { return PersistentDistributions; };
    unsigned long int nodeCount() const { return ibCount;} ;
    const IBtree *instBase() const { return InstBase; };
    
#ifdef IBSTATS
    std::vector<unsigned int> mismatch;
#endif
  protected:
    bool DefAss;
    bool DefaultsValid;
    bool Random;
    bool PersistentDistributions;
    int Version;
    ValueDistribution *TopDistribution;
    WValueDistribution *WTop;
    const TargetValue *TopT;
    FI_map fast_index;
    bool tiedTop;
    IBtree *InstBase;
    IBtree *LastInstBasePos;
    const IBtree **RestartSearch;
    const IBtree **SkipSearch;
    const IBtree **InstPath;
    unsigned long int tree_size;
    unsigned long int& ibCount;
    
    size_t Depth;
    unsigned long int NumOfTails;
    IBtree *read_list( std::istream &, 
		       std::vector<Feature*>&, Target *,
		       int );
    IBtree *read_local( std::istream &,
			std::vector<Feature*>&, Target *,
			int );
    IBtree *read_list_hashed( std::istream &, 
			      std::vector<Feature*>&, Target *, 
			      int );
    IBtree *read_local_hashed( std::istream &, 
			       std::vector<Feature*>&, Target *, 
			       int );
    void write_tree( std::ostream &os, const IBtree * ) const;
    void write_tree_hashed( std::ostream &os, const IBtree * ) const;
    bool read_IB( std::istream &, std::vector<Feature *>&, Target *, int );
    bool read_IB( std::istream &, std::vector<Feature *>&, Target *,
		  Hash::StringHash *, Hash::StringHash *, int );
    void fill_index();
    const IBtree *fast_search_node( FeatureValue * );
  };
  
  class IB_InstanceBase: public InstanceBase_base {
  public:
    IB_InstanceBase( size_t size, unsigned long& cnt, bool rand ):
      InstanceBase_base( size, cnt, rand , false ) {
    };
    IB_InstanceBase *Copy() const;
    IB_InstanceBase *clone() const;
    const ValueDistribution *InitGraphTest( std::vector<FeatureValue *>&, 
					    const std::vector<FeatureValue *> *,
					    size_t,
					    size_t );
    const ValueDistribution *NextGraphTest( std::vector<FeatureValue *>&, 
				      size_t& );
  private:
    size_t offSet;
    size_t effFeat;
    const std::vector<FeatureValue *> *testInst;
  };

  class IG_InstanceBase: public InstanceBase_base {
  public:
    IG_InstanceBase( size_t size, unsigned long& cnt, 
		     bool rand, bool pruned, bool keep_dists ):
      InstanceBase_base( size, cnt, rand, keep_dists ), Pruned( pruned ) {};
    IG_InstanceBase *clone() const;
    IG_InstanceBase *Copy() const;
    void Prune( const TargetValue *, long = 0 );
    void specialPrune( const TargetValue * );
    bool IsPruned() const { return Pruned; };
    const ValueDistribution *IG_test( const Instance& , size_t&, bool&,
				      const TargetValue *& );
    bool ReadIB( std::istream &, std::vector<Feature *>&, Target *, int );
    bool ReadIB( std::istream &, std::vector<Feature *>&, Target *,
		 Hash::StringHash *, Hash::StringHash *, int );
    bool MergeSub( InstanceBase_base * );
  protected:
    bool Pruned;
  };

  class TRIBL_InstanceBase: public InstanceBase_base {
  public:
    TRIBL_InstanceBase( size_t size, unsigned long& cnt, 
			bool rand, bool keep_dists ):
      InstanceBase_base( size, cnt, rand, keep_dists ), Threshold(0) {};
    TRIBL_InstanceBase *clone() const;
    TRIBL_InstanceBase *Copy() const;
    IB_InstanceBase *TRIBL_test( const Instance&,
				 size_t,
				 const TargetValue *&,
				 const ValueDistribution *&, 
				 size_t& ); 
  private:
    IB_InstanceBase *IBPartition( IBtree * ) const;
    void AssignDefaults( size_t );
    size_t Threshold;
  };

  class TRIBL2_InstanceBase: public InstanceBase_base {
  public:
    TRIBL2_InstanceBase( size_t size, unsigned long& cnt, 
			 bool rand, bool keep_dists ):
      InstanceBase_base( size, cnt, rand, keep_dists ) {
    };
    TRIBL2_InstanceBase *clone() const;
    TRIBL2_InstanceBase *Copy() const;
    IB_InstanceBase *TRIBL2_test( const Instance& , 
				  const ValueDistribution *&,
				  size_t& );
  private:
    IB_InstanceBase *IBPartition( IBtree * ) const;
  };

}
#endif
