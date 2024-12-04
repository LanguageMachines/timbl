/*
  Copyright (c) 1998 - 2024
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
#ifndef TIMBL_IBTREE_H
#define TIMBL_IBTREE_H

#include <unordered_map>

#include "ticcutils/XMLtools.h"
#include "timbl/MsgClass.h"

//#define IBSTATS

namespace Hash {
  class UnicodeHash;
}

namespace Timbl {

  class IB_InstanceBase;
  class IG_InstanceBase;
  class TRIBL_InstanceBase;
  class TRIBL2_InstanceBase;
  class Feature;
  class FeatureValue;
  class Instance;
  class Feature_List;
  class Targets;
  class TargetValue;
  class ClassDistribution;
  class WClassDistribution;

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
    ClassDistribution *TDistribution;
    IBtree *link;
    IBtree *next;

    IBtree();
    explicit IBtree( FeatureValue * );
    IBtree( const IBtree& ) = delete; // forbid copies
    IBtree& operator=( const IBtree& ) = delete; // forbid copies
    ~IBtree();
    IBtree *Reduce( const TargetValue *, unsigned long&, long );
#ifdef IBSTATS
    static inline IBtree *add_feat_val( FeatureValue *,
					unsigned int&,
					IBtree *&,
					unsigned long& );
#else
    static inline IBtree *add_feat_val( FeatureValue *,
					IBtree *&,
					unsigned long& );
#endif
    inline ClassDistribution *sum_distributions( bool );
    inline IBtree *make_unique( const TargetValue *, unsigned long& );
    void cleanDistributions();
    void re_assign_defaults( bool, bool );
    void assign_defaults( bool, bool, size_t );
    void redo_distributions();
    void countBranches( unsigned int,
			std::vector<unsigned int>&,
			std::vector<unsigned int>& );
    const ClassDistribution *exact_match( const Instance&  ) const;
  protected:
    const IBtree *search_node( const FeatureValue * ) const;
  };

  using FI_map = std::unordered_map<size_t, const IBtree*>;

  class InstanceBase_base: public MsgClass {
    friend class IG_InstanceBase;
    friend class TRIBL_InstanceBase;
    friend class TRIBL2_InstanceBase;
    InstanceBase_base( const InstanceBase_base& ) = delete; // forbid copies
    InstanceBase_base& operator=( const InstanceBase_base& ) = delete; // forbid copies
    friend std::ostream& operator<<( std::ostream &os,
				     const InstanceBase_base& );
    friend std::ostream& operator<<( std::ostream &os,
				     const InstanceBase_base * );
  public:
    InstanceBase_base( size_t, unsigned long&, bool, bool );
    virtual ~InstanceBase_base( void ) override;
    void AssignDefaults( void );
    void RedoDistributions();
    bool AddInstance( const Instance&  );
    void RemoveInstance( const Instance&  );
    void summarizeNodes( std::vector<unsigned int>&,
			 std::vector<unsigned int>& );
    virtual bool MergeSub( InstanceBase_base * );
    const ClassDistribution *ExactMatch( const Instance& I ) const {
      return InstBase->exact_match( I ); };
    virtual const ClassDistribution *InitGraphTest( std::vector<FeatureValue *>&,
						    const std::vector<FeatureValue *> *,
						    const size_t,
						    const size_t );
    virtual const ClassDistribution *NextGraphTest( std::vector<FeatureValue *>&,
						    size_t& );
    unsigned long int GetDistSize( ) const { return NumOfTails; };
    virtual const ClassDistribution *IG_test( const Instance& , size_t&, bool&,
					      const TargetValue *& );
    virtual IB_InstanceBase *TRIBL_test( const Instance& , size_t,
					 const TargetValue *&,
					 const ClassDistribution *&,
					 size_t& );
    virtual IB_InstanceBase *TRIBL2_test( const Instance& ,
					  const ClassDistribution *&,
					  size_t& );
    bool read_hash( std::istream&,
		    Hash::UnicodeHash&,
		    Hash::UnicodeHash& ) const;
    virtual InstanceBase_base *Copy() const = 0;
    virtual InstanceBase_base *clone() const = 0;
    void Save( std::ostream&,
	       bool=false );
    void Save( std::ostream&,
	       const Hash::UnicodeHash&,
	       const Hash::UnicodeHash&,
	       bool=false );
    void toXML( std::ostream& );
    void printStatsTree( std::ostream&, unsigned int startLevel );
    virtual bool ReadIB( std::istream&,
			 Feature_List&,
			 Targets&,
			 int );
    virtual bool ReadIB_hashed( std::istream&,
				Feature_List&,
				Targets&,
				int );
    virtual void Prune( const TargetValue *, long = 0 );
    virtual bool IsPruned() const { return false; };
    void CleanPartition(  bool );
    unsigned long int GetSizeInfo( unsigned long int&, double & ) const;
    const ClassDistribution *TopDist() const { return TopDistribution; };
    bool HasDistributions() const;
    const TargetValue *TopTarget( bool & );
    bool PersistentD() const { return PersistentDistributions; };
    unsigned long int nodeCount() const { return ibCount;} ;
    size_t depth() const { return Depth;} ;
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
    ClassDistribution *TopDistribution;
    WClassDistribution *WTop;
    const TargetValue *TopT;
    FI_map fast_index;
    bool tiedTop;
    IBtree *InstBase;
    IBtree *LastInstBasePos;
    std::vector<const IBtree *> RestartSearch;
    std::vector<const IBtree *> SkipSearch;
    std::vector<const IBtree *> InstPath;
    unsigned long int& ibCount;

    size_t Depth;
    unsigned long int NumOfTails;
    IBtree *read_list( std::istream&,
		       Feature_List&,
		       Targets&,
		       int );
    IBtree *read_local( std::istream&,
			Feature_List&,
			Targets&,
			int );
    IBtree *read_list_hashed( std::istream&,
			      Feature_List&,
			      Targets&,
			      int );
    IBtree *read_local_hashed( std::istream&,
			       Feature_List&,
			       Targets&,
			       int );
    void write_tree( std::ostream &os, const IBtree * ) const;
    void write_tree_hashed( std::ostream &os, const IBtree * ) const;
    bool read_IB( std::istream&,
		  Feature_List& ,
		  Targets&,
		  int );
    bool read_IB_hashed( std::istream&,
			 Feature_List& ,
			 Targets&,
			 int );
    void fill_index();
    const IBtree *fast_search_node( const FeatureValue * );
  };

  class IB_InstanceBase: public InstanceBase_base {
  public:
  IB_InstanceBase( size_t size, unsigned long& cnt, bool rand ):
    InstanceBase_base( size, cnt, rand , false ),
      offSet(0),
      effFeat(0),
      testInst(0)
	{};
    IB_InstanceBase *Copy() const override;
    IB_InstanceBase *clone() const override;
    const ClassDistribution *InitGraphTest( std::vector<FeatureValue *>&,
					    const std::vector<FeatureValue *> *,
					    const size_t,
					    const size_t ) override;
    const ClassDistribution *NextGraphTest( std::vector<FeatureValue *>&,
					    size_t& ) override;
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
    IG_InstanceBase *clone() const override;
    IG_InstanceBase *Copy() const override;
    void Prune( const TargetValue *, long = 0 ) override;
    void specialPrune( const TargetValue * );
    bool IsPruned() const override { return Pruned; };
    const ClassDistribution *IG_test( const Instance& ,
				      size_t&,
				      bool&,
				      const TargetValue *& ) override;
    bool ReadIB( std::istream&,
		 Feature_List&,
		 Targets&,
		 int ) override;
    bool ReadIB_hashed( std::istream&,
			Feature_List&,
			Targets&,
			int ) override;
    bool MergeSub( InstanceBase_base * ) override;
  protected:
    bool Pruned;
  };

  class TRIBL_InstanceBase: public InstanceBase_base {
  public:
    TRIBL_InstanceBase( size_t size, unsigned long& cnt,
			bool rand, bool keep_dists ):
      InstanceBase_base( size, cnt, rand, keep_dists ), Threshold(0) {};
    TRIBL_InstanceBase *clone() const override;
    TRIBL_InstanceBase *Copy() const override;
    IB_InstanceBase *TRIBL_test( const Instance&,
				 size_t,
				 const TargetValue *&,
				 const ClassDistribution *&,
				 size_t& ) override;
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
    TRIBL2_InstanceBase *clone() const override;
    TRIBL2_InstanceBase *Copy() const override;
    IB_InstanceBase *TRIBL2_test( const Instance& ,
				  const ClassDistribution *&,
				  size_t& ) override;
  private:
    IB_InstanceBase *IBPartition( IBtree * ) const;
  };

}
#endif
