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
			      unsigned int& ncnt, unsigned int& lcnt ){
    NewIBTree *result = 0;
    if ( pos == I.FV.size() ){
      result = new NewIBleaf( );
      ++lcnt;
    }
    else { 
      result = new NewIBbranch( );
    }
    ++ncnt;
    result->addInst( I, pos, ncnt, lcnt );
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
  
  bool NewIBbranch::addInst( const Instance& I, 
			     unsigned int pos,
			     unsigned int& ncnt,
			     unsigned int& lcnt ){
    bool result = true;
    if ( pos < I.FV.size() ){
      std::map<FeatureValue *,NewIBTree*,rfCmp>::iterator it = _mmap.find( I.FV[pos] );
      if ( it != _mmap.end() ){
	result = it->second->addInst( I, pos+1, ncnt, lcnt );
      }
      else {
	NewIBTree *o = createNewIBTree( I, pos+1, ncnt, lcnt );
	_mmap[I.FV[pos]] = o;
      }
    }
    return result;
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
  
  bool NewIBleaf::addInst( const Instance& v, 
			   unsigned int,
			   unsigned int&,
			   unsigned int& ){
    if ( !TDistribution )
      TDistribution = new ValueDistribution();
    return TDistribution->IncFreq( v.TV, v.ExemplarWeight() );
  }
  
  void NewIBleaf::delInst( const Instance& v, 
			   unsigned int,
			   unsigned int& ){
    TDistribution->DecFreq( v.TV );
  }

  const NewIBTree *NewIBbranch::find( FeatureValue *fv ) const {
    IBmap::const_iterator mit = _mmap.find( fv );
    if ( mit != _mmap.end() )
      return mit->second;
    else
      return 0;
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
  
  bool NewIBroot::addInstance( const Instance& I ){
    bool result = true;
    if ( !_root ){
      _root = createNewIBTree( I, 0, _nodeCount, _leafCount );
    }
    else
      result = _root->addInst( I, 0, _nodeCount, _leafCount );
    return result;
  }

  void NewIBroot::deleteInstance( const Instance& I ){
    if ( _root ){
      _root->delInst( I, 0, _nodeCount );
      if ( topDist )
	topDist->DecFreq(I.TV);
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
	topTV = _root->TValue;
	topDist = _root->TDistribution;
    }
    }
    _defAss = true;
    _defValid = true;
  }
  
  const TargetValue *NewIBroot::topTarget( bool &tie ) {
    if ( !_defValid || !_defAss )
      topTV = 0;
    if ( topTV == 0 ){
      topTV = topDist->BestTarget( tiedTop, _random); 
    }
    tie = tiedTop;
    return topTV;
  }

  NewIBroot::NewIBroot( int depth, bool random, bool keep ): 
    _depth(depth), _random(random), _keepDist(keep), _root(0), _version(4), 
    _defValid(false), _defAss(false), _pruned(false), _nodeCount(1),
    _leafCount(0), topTV(0), tiedTop( false ), topDist(0), WTop(0) {
    RestartSearch = new bool[depth];
    SkipSearch = new IBiter[depth];
    InstPath = new IBiter[depth];
  };
  
  NewIBroot::~NewIBroot(){
    delete _root;
    delete WTop;
    delete [] RestartSearch;
    delete [] SkipSearch;
    delete [] InstPath;
  }

  unsigned long int NewIBroot::getSizeInfo( unsigned long int& CurSize, 
					    double &Compression ) const {
    unsigned long int MaxSize = (_depth+1) * _leafCount;
    CurSize = _nodeCount;
    Compression = 100*(1-(double)CurSize/(double)MaxSize);
    return CurSize * sizeof(NewIBbranch);
  }
  
  void NewIBroot::put( ostream& os ) const{
    os << "TOPTARGET = " << topTV << endl;
    os << "TOPDIST = " << topDist << endl;
    os << "\n[" << _root << "]" << endl;
  }

  void NewIBroot::save( ostream &os, bool persist ) {
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
  
  void NewIBroot::prune( const TargetValue *top ) {
    assignDefaults( );
    if ( !_pruned ) {
      if ( _root ){
	if ( top )
	  _root->prune( top, _nodeCount );
	else
	  _root->prune( topTV, _nodeCount );	  
      }
      _pruned = true;
    }
  }

  NewIBroot *NewIBroot::copy() const {
    NewIBroot *result = new NewIBroot( _depth, _random, _keepDist );
    result->_defAss = _defAss;
    result->_defValid = _defValid;
    result->_nodeCount = _nodeCount;
    result->_leafCount = _leafCount; // only usefull for Server???
    result->_pruned = _pruned;
    result->_root = _root;
    result->topDist = topDist;
    return result;
  }

  void NewIBroot::deleteCopy( bool distToo ){
    _root = 0; // prevent deletion of InstBase in next step!
    if ( !distToo )
      topDist = 0; // save TopDist for deletion
    delete this;
  }
  
  const ValueDistribution *NewIBroot::exactMatch( const Instance& I ) const {  
    if ( _root )
      return _root->match( I, 0 );
    else
      return 0;
  }
  
  void IBiter::init( NewIBTree *tree ){
    if ( tree ){
      _map = tree->getMap();
      if ( _map )
	mit = _map->begin();
    }
    else {
      _map = 0;
    }
  }
  
  NewIBTree *IBiter::value() {
    if ( _map && mit != _map->end() )
      return mit->second;
    else
      return 0;
  }

  FeatureValue *IBiter::FValue() {
    if ( _map && mit != _map->end() )
      return mit->first;
    else
      return 0;
  }

  NewIBTree *IBiter::find( FeatureValue *fv ) {
    if ( _map ){
      //      cerr << "iter search " << fv << endl;
      mit = _map->find( fv );
      if ( mit != _map->end() ){
	//	cerr << "found! " << mit->first << endl;
	return mit->second;
      }
    }
    return 0;
  }
  
  void IBiter::reset( ) {
    if ( _map ){
      mit = _map->begin();
    }
  }

  const ValueDistribution *NewIBroot::initTest( vector<FeatureValue *>& Path,
						const Instance *inst,
						size_t off,
						size_t eff ){
    const ValueDistribution *result = NULL;
    testInst = inst;
    offSet = off;
    InstPath[0].init( _root );
    for ( unsigned int i = 0; i < _depth; ++i ){
      NewIBTree *pnt = InstPath[i].find( testInst->FV[i+offSet] );
      if ( pnt ){
	//	cerr << "found " << testInst->FV[i+offSet] << endl;
	SkipSearch[i] = InstPath[i];
	Path[i] = testInst->FV[i+offSet];
	RestartSearch[i] = true;
      }
      else {
	//	cerr << "didn't find " << testInst->FV[i+offSet] << endl;	
	RestartSearch[i] = false;
	SkipSearch[i].init(0);
	InstPath[i].reset();
	Path[i] = InstPath[i].FValue();
      }
      //      cerr << "set Path[" << i << "] to " << Path[i] << endl;
      if ( i == _depth-1 ){
	result = InstPath[i].value()->TDistribution;
      }
      else {
	InstPath[i+1].init( InstPath[i].value() );
      }
    }
    if ( result && result->ZeroDist() ){
      // This might happen when doing LOO or CV tests
      size_t TmpPos = effFeat-1;
      result = nextTest( Path, TmpPos );
    }
    return result;
  }

  const ValueDistribution *NewIBroot::nextTest( vector<FeatureValue *>& Path, 
						size_t& pos ){
    const ValueDistribution *result = 0;
    const NewIBTree *pnt = 0;
    while ( !pnt  ){
      if ( !RestartSearch[pos] ) {
	// No exact match here, so no real problems
	InstPath[pos].increment();
	//	cerr << "no match before so just increment " << InstPath[pos].FValue() << endl;
      }
      else {
	InstPath[pos].reset();
	//	cerr << "restart with " << InstPath[pos].FValue() << endl;	
	RestartSearch[pos] = false;
      }
      if ( InstPath[pos].value() != 0 &&
	   InstPath[pos].value() == SkipSearch[pos].value() ){
	//	cerr << "hit on Skip" << endl;
	InstPath[pos].increment();
	SkipSearch[pos].init(0);
	//	cerr << "go to next: " << InstPath[pos].FValue() << endl;	
      }
      pnt = InstPath[pos].value();
      if ( !pnt ) {
	if ( pos == 0 )
	  break;
	else {
	  pos--;
	  //	  cerr << "decremented POS to " << pos << endl;
	}
      }
    }
    if ( pnt ) {
      Path[pos] = InstPath[pos].FValue();
      //      cerr << "set Path[" << pos << "] to " << Path[pos] << endl;
      if ( pos < _depth-1 ){
	InstPath[pos+1].init( InstPath[pos].value() );
	for (  size_t j=pos+1; j < _depth; ++j ){
	  NewIBTree *tmp = InstPath[j].find( testInst->FV[j+offSet] );
	  if ( tmp ){ // we found an exact match, so mark Restart
	    SkipSearch[j] = InstPath[j];
	    RestartSearch[j] = true;
	  }
	  else { // no exact match at this level. Just start with the first....
	    RestartSearch[j] = false;
	    SkipSearch[j].init(0);
	    InstPath[j].reset();
	  }
	  Path[j] = InstPath[j].FValue();
	  //	  cerr << "set Path[" << j << "] to " << Path[j] << endl;
	  //	  cerr << "using InstPath[" << j << "] = " << InstPath[j].value() << endl;
	  if ( j == _depth-1 ){
	    result = InstPath[j].value()->TDistribution;
	    //	    cerr << "assign result:" << result << endl;
	  }
	  else {
	    InstPath[j+1].init( InstPath[j].value() );
	  }
	}
      }
      else {
	result = InstPath[_depth-1].value()->TDistribution;
      }
    }
    if ( result && result->ZeroDist() ){
      // This might happen when doing LOO or CV tests
      size_t TmpPos = effFeat-1;
      result = nextTest( Path, TmpPos );
      if ( TmpPos < pos ){
	pos = TmpPos;
      }
    }
    return result;
  }

  const ValueDistribution *NewIBroot::IG_test( const Instance& Inst, 
					       size_t &end_level,
					       bool &leaf,
					       const TargetValue *&result ) {
    // The Test function for the IG algorithm, returns a pointer to the
    // distribution of the last matching position in the Tree, it's position
    // in the Instance Base and the default TargetValue
    result = 0;
    ValueDistribution *Dist = 0;
    int pos = 0;
    leaf = false;
    if ( _root ){
      const NewIBTree *pnt = _root->find( Inst.FV[0] );
      while ( pnt ){
	result = pnt->TValue;
	if ( _keepDist )
	  Dist = pnt->TDistribution;
	++pos;
	if ( pos < _depth )
	  pnt = pnt->find( Inst.FV[pos] );
	else
	  pnt = 0;
	leaf = (pnt == 0 );
      }
      end_level = pos;
      if ( end_level == 0 ){
	if ( !WTop && topDist )
	  WTop = topDist->to_WVD_Copy();
	Dist = WTop;
      }
    }
    return Dist;
  }

  
}
