#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <stdexcept>

#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/MsgClass.h"
#include "timbl/Tree.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"
#include "timbl/NewIBtree.h"

using namespace std;

// #define DEBUG

namespace Timbl{

  void NewIBTree::put( std::ostream& os, int level ) const {
    os << "raar!" << std::endl;
  }
  
  std::ostream& operator<< ( std::ostream& os, 
			     const NewIBTree& o ){
    o.put( os, 0 );
    return os;
  }
  
  std::ostream& operator<< ( std::ostream& os, 
			     const NewIBTree *o ){
    if ( o )
      o->put( os, 0 );
    else
      os << "nO";
    return os;
  }

  NewIBTree *createNewIBTree( const Instance& I, unsigned int pos,
			      unsigned int& cnt ){
    NewIBTree *result = 0;
    if ( pos == I.FV.size() ){
      result = new NewIBleaf( );
    }
    else { 
      result = new NewIBbranch( );
    }
    ++cnt;
    result->addInst( I, pos,  cnt );
    return result;
  }
  
  ValueDistribution *NewIBleaf::getDistribution( bool keep ){
    if ( keep )
      return TDistribution;
    else
      return TDistribution->to_VD_Copy();
  }  

  ValueDistribution *NewIBbranch::getDistribution( bool keep ){
    ValueDistribution *result;
    if ( !TDistribution ){
      result = new ValueDistribution();
    }
    else {
      result = TDistribution;
    }
    if ( !keep ){
      TDistribution = 0;
    }
    return result;
  }  
  
  ValueDistribution *NewIBbranch::sum_distributions( bool keep ){
    // create a new distribution at this level by summing up the
    // distibutions of all branches.
    ValueDistribution *result = getDistribution( keep );
    std::map<FeatureValue *, NewIBTree*, rfCmp>::iterator it = _mmap.begin();
    while ( it != _mmap.end() ){
      ValueDistribution *tmp = it->second->getDistribution( keep );
      if ( tmp ){
	result->Merge( *tmp );
	if ( !keep )
	  delete tmp;
      }
      ++it;
    }
    return result;
  }  
  
  void NewIBbranch::addInst( const Instance& I, 
			     unsigned int pos,
			     unsigned int& cnt ){
    if ( pos < I.FV.size() ){
      std::map<FeatureValue *,NewIBTree*,rfCmp>::iterator it = _mmap.find( I.FV[pos] );
      if ( it != _mmap.end() ){
	it->second->addInst( I, pos+1, cnt );
      }
      else {
	NewIBTree *o = createNewIBTree( I, pos+1, cnt );
	_mmap[I.FV[pos]] = o;
	mit = _mmap.begin();
      }
    }
  }

  void NewIBbranch::delInst( const Instance& I, 
			     unsigned int pos,
			     unsigned int& cnt ){
    if ( pos < I.FV.size() ){
      std::map<FeatureValue *,NewIBTree*,rfCmp>::iterator it = _mmap.find( I.FV[pos] );
      if ( it != _mmap.end() ){
	it->second->delInst( I, pos+1, cnt );
      }
      else {
	throw logic_error("attemt to delete an instance which isn't there!");
      }
    }
  }
  
  void NewIBleaf::addInst( const Instance& v, 
			   unsigned int,
			   unsigned int& ){
    if ( !TDistribution )
      TDistribution = new ValueDistribution();
    TDistribution->IncFreq( v.TV, v.ExemplarWeight() );
  }
  
  void NewIBleaf::delInst( const Instance& v, 
			   unsigned int,
			   unsigned int& ){
    TDistribution->DecFreq( v.TV );
  }

  const ValueDistribution *NewIBleaf::match( const Instance& I, 
					     unsigned int pos ) const {
    if ( TDistribution->ZeroDist() ) // a deleted instance
      return 0;
    return TDistribution;
  }

  const ValueDistribution *NewIBbranch::match( const Instance& I, 
					       unsigned int pos ) const {
    std::map<FeatureValue *,NewIBTree*,rfCmp>::const_iterator it = _mmap.find( I.FV[pos] );
    if ( it != _mmap.end() ){
      if ( it->first->ValFreq() == 0 ) // a deleted Instance
	return 0;
      else
	return it->second->match( I, pos+1 );
    }
    else
      return 0;
  }
  
  void NewIBleaf::save( std::ostream& os ) const {
    // part of saving a tree in a recoverable manner
    os << TValue << " " << TDistribution->Save();
  }
  
  void NewIBbranch::save( std::ostream& os ) const {
    // part of saving a tree in a recoverable manner
    os << TValue << " ";
    if ( TDistribution )
      os << TDistribution->Save();
    if ( _mmap.size() > 0 ){
      os << "[";
      std::map<FeatureValue*,NewIBTree *,rfCmp>::const_iterator it = _mmap.begin();
      while ( it != _mmap.end() ){
	os << it->first << " (";
	it->second->save( os );
	os << " )";
	++it;
	if ( it != _mmap.end() ){
	  os << "\n,";
	}
      }
      os << "\n]\n";
    }
  }

  void NewIBleaf::put( std::ostream& os, int level ) const{
    int l = level;
    while ( l-- > 0 )
      os << "\t";
    os << TValue << " " << TDistribution->Save() << endl;
  }

  NewIBleaf::~NewIBleaf(){
    delete TDistribution;
  }
  
  NewIBbranch::~NewIBbranch(){
    std::map<FeatureValue*,NewIBTree*, rfCmp>::const_iterator it = _mmap.begin();  
    while ( it != _mmap.end() ){
      delete it->second;
      ++it;
    }
    delete TDistribution;
  }
  
  void NewIBbranch::put( std::ostream& os, int level ) const{
    int l = level;
    while ( l-- > 0 )
      os << "\t";
    os << "[" << TValue << " ";
    if ( TDistribution )
      os << TDistribution->Save();
    else
      os << "{}";
    std::map<FeatureValue*,NewIBTree *,rfCmp>::const_iterator it = _mmap.begin();
    while ( it != _mmap.end() ){
      os << it->first << " ";
      it->second->put( os, level+1 );
      ++it;
      if ( it != _mmap.end() ){
	os << std::endl << "  ";
	int l = level;
	while ( l-- > 0 )
	  os << "\t\t";
      }
    }
    os << "]";
  }

  void NewIBbranch::assign_defaults( bool redo, bool random, bool persist, size_t level ){
    // recursively gather Distribution information up to the top.
    // at each Node we use that info to calculate the Default target.
    // when level > 1 the info might be persistent for IGTREE use
    std::map<FeatureValue*,NewIBTree*, rfCmp>::iterator it = _mmap.begin();
    bool dummy;
    while ( it != _mmap.end() ){
      if ( redo ){
	delete it->second->TDistribution;
	it->second->TDistribution = 0;
      }
      if ( !it->second->TDistribution ){
	it->second->assign_defaults( redo, random, persist, level-1 );
	it->second->TDistribution = it->second->sum_distributions( level > 1
								   && persist );
      }
      it->second->TValue = it->second->TDistribution->BestTarget( dummy, random );
      ++it;
    }
  }

  void NewIBleaf::prune( const TargetValue* top, unsigned int& cnt ){ }

  void NewIBbranch::prune( const TargetValue* top, unsigned int& cnt ){
    std::map<FeatureValue*,NewIBTree*, rfCmp>::iterator it = _mmap.begin();
    while ( it != _mmap.end() ){
      it->second->prune( TValue, cnt );
      ++it;
    }
    it = _mmap.begin();
    while ( it != _mmap.end() ){
      if ( it->second->TValue == TValue && it->second->size() == 0 ){
	delete it->second;
	--cnt;
	_mmap.erase(it++);
      }
      else
	++it;
    }
  }
  
  void NewIBroot::addInstance( const Instance& I ){
    if ( !_root ){
      _root = createNewIBTree( I, 0, _nodeCount );
    }
    else
      _root->addInst( I, 0, _nodeCount );
  }

  void NewIBroot::deleteInstance( const Instance& I ){
    if ( _root ){
      _root->delInst( I, 0, _nodeCount );
      if ( TopDist )
	TopDist->DecFreq(I.TV);
    }
  }
  
  void NewIBroot::assignDefaults(){
    if ( !_defValid ){
      bool dummy;
      if ( _root ){
	if ( !_root->TDistribution ){
	  _root->assign_defaults( _defAss, _random, _keepDist, _depth );
	  _root->TDistribution = _root->sum_distributions( _keepDist );
	}
	_root->TValue = _root->TDistribution->BestTarget( dummy, _random );
	TopTarget = _root->TValue;
	TopDist = _root->TDistribution;
    }
    }
    _defAss = true;
    _defValid = true;
  }
  
  NewIBroot::~NewIBroot(){
    delete _root;
  }
  
  void NewIBroot::Put( ostream& os ) const{
    os << "TOPTARGET = " << TopTarget << endl;
    os << "TOPDIST = " << TopDist << endl;
    os << "\n[" << _root << "]" << endl;
  }

  void NewIBroot::Save( ostream &os, bool persist ) {
    // save an IBtree for later use.
    bool temp_persist = _keepDist;
    _keepDist = persist;
    assignDefaults();
    bool dummy;
    ios::fmtflags OldFlg = os.setf( ios::fixed, ios::floatfield );
    os << "# Version " << _version << "\n#\n(";
    if ( _root ){
      _root->save( os );
    }
    os << ")\n";
    os.setf( OldFlg );
    _keepDist = temp_persist;
  }
  
  void NewIBroot::Prune( const TargetValue *top ) {
    cerr << "start prune with " << _nodeCount << " nodes" << endl;
    assignDefaults( );
    if ( !_pruned ) {
      if ( _root ){
	if ( top )
	  _root->prune( top, _nodeCount );
	else
	  _root->prune( TopTarget, _nodeCount );	  
      }
      _pruned = true;
    }
    cerr << "end prune with " << _nodeCount << " nodes" << endl;
  }
  
  const ValueDistribution *NewIBroot::exactMatch( const Instance& I ) const {  
    if ( _root )
      return _root->match( I, 0 );
    else
      return 0;
  }

}
