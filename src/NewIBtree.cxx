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
			      unsigned int _depth,
			      unsigned int& ncnt, unsigned int& lcnt ){
    NewIBTree *result = 0;
    if ( pos == _depth ){
      result = new NewIBleaf( );
      ++lcnt;
    }
    else { 
      result = new NewIBbranch( );
    }
    ++ncnt;
    result->addInst( I, pos, _depth, ncnt, lcnt );
    return result;
  }
  
  ValueDistribution *NewIBleaf::getDistribution( bool keep ){
    if ( keep )
      return TDistribution;
    else
      return TDistribution->to_VD_Copy();
  }  

  ValueDistribution *NewIBbranch::getDistribution( bool keep ){
    ValueDistribution *result = TDistribution;
    if ( !keep ){
      TDistribution = 0;
    }
    return result;
  }  

  ValueDistribution *NewIBbranch::sum_distributions( bool keep ){
    // create a new distribution at this level by summing up the
    // distibutions of all branches.
    ValueDistribution *result = 0;
    std::map<FeatureValue *, NewIBTree*, rfCmp>::iterator it = _mmap.begin();
    while ( it != _mmap.end() ){
      ValueDistribution *tmp = it->second->getDistribution( keep );
      if ( tmp ){
	if ( !result )
	  result = new ValueDistribution();
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
			     unsigned int _depth,
			     unsigned int& ncnt,
			     unsigned int& lcnt ){
    bool result = true;
    if ( pos < _depth ){
      std::map<FeatureValue *,NewIBTree*,rfCmp>::iterator it = _mmap.find( I.FV[pos] );
      if ( it != _mmap.end() ){
	result = it->second->addInst( I, pos+1, _depth, ncnt, lcnt );
      }
      else {
	NewIBTree *o = createNewIBTree( I, pos+1, _depth, ncnt, lcnt );
	assign( I.FV[pos], o );      }
    }
    return result;
  }

  void NewIBleaf::delInst( const Instance& I,
			   unsigned int,
			   unsigned int& ){
    TDistribution->DecFreq( I.TV );
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
			   unsigned int,
			   unsigned int&,
			   unsigned int& ){
    if ( abs( v.ExemplarWeight() ) > Common::Epsilon ){
      if ( !TDistribution ){
	TDistribution = new WValueDistribution();
      }
      return !TDistribution->IncFreq( v.TV, v.ExemplarWeight() );
    }
    else {
      if ( !TDistribution ){
	TDistribution = new ValueDistribution();
      }
      TDistribution->IncFreq( v.TV );
      return true;
    }
  }
  
  NewIBTree *NewIBbranch::find( FeatureValue *fv ) const {
    IBmap::const_iterator mit = _mmap.find( fv );
    if ( mit != _mmap.end() )
      return mit->second;
    else
      return 0;
  }

  const ValueDistribution *NewIBleaf::match( const Instance& , 
					     unsigned int ) const {
    if ( TDistribution->ZeroDist() ) // a deleted instance
      return 0;
    return TDistribution;
  }
  
  const ValueDistribution *NewIBbranch::match( const Instance& I, 
					       unsigned int pos ) const {
    std::map<FeatureValue *,NewIBTree*,rfCmp>::const_iterator it = _mmap.find( I.FV[pos] );
    if ( it != _mmap.end() ){
      if ( it->second->FValue &&
	   it->second->FValue->ValFreq() == 0 ) // a deleted Instance
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
  
  void NewIBleaf::saveHashed( std::ostream& os ) const {
    // part of saving a tree in a recoverable manner
    os << TValue->Index() << " " << TDistribution->SaveHashed();
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
	if ( it->second->FValue )
	  os << it->second->FValue;
	os << " (";
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

  void NewIBbranch::saveHashed( std::ostream& os ) const {
    // part of saving a tree in a recoverable manner
    os << TValue->Index() << " ";
    if ( TDistribution )
      os << TDistribution->SaveHashed();
    if ( _mmap.size() > 0 ){
      os << "[";
      std::map<FeatureValue*,NewIBTree *,rfCmp>::const_iterator it = _mmap.begin();
      while ( it != _mmap.end() ){
	if ( it->second->FValue )
	  os << it->second->FValue->Index();
	os << " (";
	it->second->saveHashed( os );
	os << " )";
	++it;
	if ( it != _mmap.end() ){
	  os << "\n,";
	}
      }
      os << "\n]\n";
    }
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

  void NewIBleaf::put( std::ostream& os, int level ) const{
    int l = level;
    while ( l-- > 0 )
      os << "\t";
    os << TValue << " " << TDistribution->Save() << endl;
  }

  void NewIBbranch::put( std::ostream& os, int level ) const{
    int l = level;
    while ( l-- > 0 )
      os << "\t";
    os << "[" << TValue << " ";
    if ( TDistribution )
      os << TDistribution->Save();
    else
      os << "{null}";
    std::map<FeatureValue*,NewIBTree *,rfCmp>::const_iterator it = _mmap.begin();
    while ( it != _mmap.end() ){
      os << it->second->FValue << " ";
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
      if ( it->second->TDistribution && level > 1 ){
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
  
  void NewIBleaf::redoDistributions(){
  }

  void NewIBbranch::redoDistributions(){
    delete TDistribution;
    TDistribution = 0;
    std::map<FeatureValue*,NewIBTree*, rfCmp>::iterator it = _mmap.begin();
    while ( it != _mmap.end() ){
      it->second->redoDistributions();
      if ( it->second->FValue &&
	   it->second->FValue->ValFreq() > 0 )
	// also we have to update the targetinformation of the featurevalue
	// so we can recalculate the statistics later on.
	it->second->FValue->ReconstructDistribution( *(it->second->TDistribution) );
      ++it;
    }
    TDistribution = sum_distributions( false );
  }

  void NewIBbranch::prune( unsigned int& cnt ){
    std::map<FeatureValue*,NewIBTree*, rfCmp>::iterator it = _mmap.begin();
    while ( it != _mmap.end() ){
      it->second->prune( cnt );
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

  std::ostream& operator<< ( std::ostream& os, 
			     const NewIBroot& o ){
    o.put( os );
    return os;
  }
  
  std::ostream& operator<< ( std::ostream& os, 
			     const NewIBroot *o ){
    if ( o )
      o->put( os );
    else
      os << "røøt";
    return os;
  }

  bool NewIBroot::addInstance( const Instance& I ){
    bool result = true;
    if ( !_root ){
      _root = createNewIBTree( I, 0, _depth, _nodeCount, _leafCount );
    }
    else
      result = _root->addInst( I, 0, _depth, _nodeCount, _leafCount );
    _defValid = false;    
    return result;
  }

  void NewIBroot::deleteInstance( const Instance& I ){
    if ( _root ){
      _defValid = false;
      _root->delInst( I, 0, _nodeCount );
      if ( topDist )
	topDist->DecFreq(I.TV);
    }
  }
  
  void NewIBroot::assignDefaults(){
    if ( !_defValid ){
      bool dummy;
      if ( _root ){
	if ( _root->TDistribution ){
	  delete _root->TDistribution;
	  _root->TDistribution = 0;
	}
	_root->assign_defaults( _defAss, _random, _keepDist, _depth );
	_root->TDistribution = _root->sum_distributions( _keepDist );
	_root->TValue = _root->TDistribution->BestTarget( dummy, _random );
	topTV = _root->TValue;
	topDist = _root->TDistribution;
      }
    }
    _defAss = true;
    _defValid = true;
  }

  void NewIBroot::redoDistributions(){
    // recursively gather Distribution information up to the top.
    // removing old info...
    // at each node we also Reconstruct Feature distributions
    // we keep the Target value that was given!
    if ( _root ){
      _root->redoDistributions( );
    }
  }

  void NewIBroot::assignDefaults( size_t treshold ){
    if ( _treshold != treshold ){
      _treshold = treshold;
      _defValid = false;    
    }
    if ( !_defValid ){
      bool dummy;
      if ( _root ){
	if ( !_root->TDistribution ){
	  _root->assign_defaults( _defAss, _random, _keepDist, _depth );
	  _root->TDistribution = _root->sum_distributions( true );
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
    _defValid(false), _defAss(false), _pruned(false), _treshold(depth),
    _nodeCount(0), _leafCount(0), topTV(0), tiedTop( false ), topDist(0), 
    WTop(0) {
    RestartSearch = new bool[depth];
    SkipSearch = new NewIBTree*[depth];
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
    ios::fmtflags OldFlg = os.setf( ios::fixed, ios::floatfield );
    os << "# Version " << _version << "\n#\n(";
    if ( _root ){
      _root->save( os );
    }
    os << ")\n";
    os.setf( OldFlg );
    _keepDist = temp_persist;
  }
  
  void saveHash( ostream &os, StringHash *cats, StringHash *feats ){
    int Size = cats->NumOfEntries();
    os << "Classes" << endl;
    for ( int i=1; i <= Size; ++i )
      os << i << "\t" << cats->ReverseLookup( i ) << endl;
    Size = feats->NumOfEntries();
    os << "Features" << endl;
    for ( int i=1; i <= Size; ++i )
      os << i << "\t" << feats->ReverseLookup( i ) << endl;
    os << endl;
  }  

  void NewIBroot::saveHashed( ostream &os, 
			      StringHash *cats, 
			      StringHash *feats,
			      bool persist ) {
    // save an IBtree for later use.
    bool temp_persist = _keepDist;
    _keepDist = persist;
    assignDefaults();
    ios::fmtflags OldFlg = os.setf( ios::fixed, ios::floatfield );
    os << "# Version " << _version << " (Hashed)\n#\n";
    saveHash( os , cats, feats );
    os << "(";
    if ( _root ){
      _root->saveHashed( os );
    }
    os << ")\n";
    os.setf( OldFlg );
    _keepDist = temp_persist;
  }
  
  void NewIBroot::readMap( istream &is,
			   NewIBTree *node,
			   std::vector<Feature*>& Feats,
			   Target  *Targ,
			   int level ){
    bool goon = true;
    char delim;
    while ( is && goon ) {
      is >> delim;    // skip the opening `[` or separating ','
      string buf;
      is >> ws >> buf;
      FeatureValue *FV = Feats[level]->add_value( buf, NULL );
      NewIBTree *tmp = readTree( is, Feats, Targ, level );
      if ( tmp ){
	node->assign(FV, tmp );
      }
      goon = ( Common::look_ahead(is) == ',' );
    }
    is >> delim;    // skip closing `]`
  }
  
  void NewIBroot::readMapHashed( istream &is,
				 NewIBTree *node,
				 std::vector<Feature*>& Feats,
				 Target  *Targ,
				 int level ){
    bool goon = true;
    char delim;
    while ( is && goon ) {
      is >> delim;    // skip the opening `[` or separating ','
      string buf;
      int index;
      is >> index;
      FeatureValue *FV = Feats[level]->add_value( index, NULL );
      NewIBTree *tmp = readTreeHashed( is, Feats, Targ, level );
      if ( tmp ){
	node->assign( FV, tmp );
      }
      goon = ( Common::look_ahead(is) == ',' );
    }
    is >> delim;    // skip closing `]`
  }
  
  NewIBTree *NewIBroot::readTree( std::istream& is,
				  vector<Feature*>& Feats,
				  Target *Targ,
				  int level ){
    if ( !is ) 
      return 0;
    string buf;
    char delim;
    is >> delim;
    if ( !is || delim != '(' ){
      cerr << "missing `(` in Instance Base file" << endl;
      return 0;
    }
    is >> ws >> buf;
    TargetValue *TV = Targ->Lookup( buf );
    ValueDistribution *TD = 0;
    int nxt = Common::look_ahead(is);
    if ( nxt == '{' ){
      try {
	TD = ValueDistribution::read_distribution( is, Targ, false );
      }
      catch ( const string what ){
	cerr << "problems reading a distribution from InstanceBase file"
	     << what << endl;
	return 0;
      }
    }
    NewIBTree *result = 0;
    if ( Common::look_ahead(is) == '[' ){
      //  a branch
      NewIBbranch *tmp = new NewIBbranch();
      ++_nodeCount;
      tmp->TValue = TV;
      readMap( is, tmp, Feats, Targ, level+1 );
      result = tmp;
    }
    else if ( Common::look_ahead(is) == ')' && TD ){
      result = new NewIBleaf();
      ++_nodeCount;
      ++_leafCount;
      result->TValue = TV;
      result->TDistribution = TD;
    }
    is >> delim;
    if ( delim != ')' ){
      cerr << "missing `)` in Instance Base file" << endl;
      delete result;
      return 0;
    }
    return result;
  }
  

  NewIBTree *NewIBroot::readTreeHashed( std::istream &is, 
					std::vector<Feature *>& Feats, 
					Target *Targ, 
					int level ){
    if ( !is ) 
      return 0;
    string buf;
    char delim;
    int index;
    is >> delim;
    if ( !is || delim != '(' ){
      cerr << "missing `(` in Instance Base file" << endl;
      return 0;
    }
    is >> index;
    TargetValue *TV = Targ->ReverseLookup( index );
    ValueDistribution *TD = 0;
    int nxt = Common::look_ahead(is);
    if ( nxt == '{' ){
      try {
	TD = ValueDistribution::read_distribution_hashed( is, Targ, false );
      }
      catch ( const string what ){
	cerr << "problems reading a distribution from InstanceBase file"
	     << what << endl;
	return 0;
      }
    }
    NewIBTree *result = 0;
    if ( Common::look_ahead(is) == '[' ){
      //  a branch
      NewIBbranch *tmp = new NewIBbranch();
      ++_nodeCount;
      tmp->TValue = TV;
      readMapHashed( is, tmp, Feats, Targ, level+1 );
      result = tmp;
    }
    else if ( Common::look_ahead(is) == ')' && TD ){
      result = new NewIBleaf();
      ++_nodeCount;
      ++_leafCount;
      result->TValue = TV;
      result->TDistribution = TD;
    }
    is >> delim;
    if ( delim != ')' ){
      cerr << "missing `)` in Instance Base file" << endl;
      delete result;
      return 0;
    }
    return result;
  }
  

  bool NewIBroot::read( std::istream& is, std::vector<Feature *>& Feats, 
			Target *Targs, int expectedVersion ){
    string buf;
    _defAss = true;  // always for a restored tree
    _defValid = true; // always for a restored tree
    _version = expectedVersion;
    char delim;
    is >> delim;
    if ( !is || delim != '(' ){
      cerr << "missing first `(` in Instance Base file" << endl;
    }
    else {
      // first we get the value of the TopTarget. It's in the file
      // for backward compability
      is >> ws >> buf;
      delete topDist;
      topDist = 0;
      if ( Common::look_ahead(is) == '{' ){
	// Now read the TopDistribution, to get the Targets
	// in the right order in Targ
	try {
	  topDist 
	    = ValueDistribution::read_distribution( is, Targs, true );
	}
	catch ( const string& what ){
	  cerr << "read_distribution failed: " << what << endl;
	}
      }
      if ( !topDist )
	cerr << "problems reading Top Distribution from Instance Base file" 
	     << endl;
      else {
	topTV = Targs->Lookup( buf );
	if ( Common::look_ahead( is ) == '[' ){
	  NewIBbranch *tmp = new NewIBbranch();
	  ++_nodeCount;
	  tmp->TDistribution = topDist;
	  tmp->TValue = topTV;
	  readMap( is, tmp, Feats, Targs, 0 );
	  _root = tmp;
	}
	if ( _root ){
	  is >> ws >> buf;
	  if ( buf.empty() || buf[0] != ')' )
	    cerr << "missing last `)` in Instance base file, found "
		 << buf << endl;
	  else {
	    redoDistributions();
	    return true;
	  }
	}
      }
    }
    return false;
  }
  
  bool readHash( istream &is, 
		 StringHash *cats,
		 StringHash *feats ){
    string line;
    is >> ws;
    is >> line;
    if ( !compare_nocase( line, "Classes" ) ){
      cerr << "missing 'Classes' keyword in Hashinfo" << endl;
      return false;
    }
    is >> ws;
    vector<string> vals;
    while ( getline( is, line ) ){
      size_t i = split( line, vals );
      if ( i == 2 )
	// just ignore index!
	cats->Hash( vals[1] );
      else
	break;
      is >> ws;
    }
    if ( !compare_nocase( line, "Features" ) ){
      cerr << "missing 'Features' keyword in Hashinfo" << endl;
      return false;
    }
    while ( getline( is, line ) ){
      size_t i = split( line, vals );
      if ( i == 2 )
	// just ignore index!
	feats->Hash( vals[1] );
      else
	break;
    }
    return true;
  }

  bool NewIBroot::readHashed( std::istream &is, std::vector<Feature *>& Feats, 
			      Target *Targs,
			      StringHash *cats, StringHash *feats, 
			      int expectedVersion ){
    char delim;
    _defAss = true;  // always for a restored tree
    _defValid = true; // always for a restored tree
    _version = expectedVersion;
    readHash( is, cats, feats );
    is >> delim;
    if ( !is || delim != '(' ){
      cerr << "missing first `(` in Instance Base file" << endl;
    }
    else {
      // first we get the value of the TopTarget. 
      int index;
      is >> index;
      delete topDist;
      topDist = 0;
      if ( Common::look_ahead(is) == '{' ){
	// Now read the TopDistribution, to get the Targets
	// in the right order in Targ
	try {
	  topDist 
	    = ValueDistribution::read_distribution_hashed( is, Targs, true );
	}
	catch ( const string& what ){
	  cerr << "read_distribution failed: " << what << endl;
	}
      }
      if ( !topDist ){
	cerr << "problems reading Top Distribution from Instance Base file" 
	     << endl;
      }
      else {
	topTV = Targs->ReverseLookup( index );
	if ( Common::look_ahead( is ) == '[' ){
	  NewIBbranch *tmp = new NewIBbranch();
	  ++_nodeCount;
	  tmp->TDistribution = topDist;
	  tmp->TValue = topTV;
	  readMapHashed( is, tmp, Feats, Targs, 0 );
	  _root = tmp;
	}
	if ( _root ){
	  is >> delim;
	  if ( delim != ')' )
	    cerr << "missing last `)` in Instance base file, found '"
		 << delim << "'" << endl;
	  else {
	    redoDistributions();
	    return true;
	  }
	}
      }
    }
    return false;
  }

  void NewIBbranch::countBranches( unsigned int l, 
				   std::vector<unsigned int>& terminals,
				   std::vector<unsigned int>& nonTerminals ){
    
    if ( _mmap.empty() )
      ++terminals[l];
    else {
      ++nonTerminals[l];
      IBmap::const_iterator it = _mmap.begin();
      while ( it != _mmap.end() ){
	it->second->countBranches( l+1, terminals, nonTerminals );
	++it;
      }
    }
  }

  void NewIBleaf::countBranches( unsigned int l, 
				 std::vector<unsigned int>& terminals,
				 std::vector<unsigned int>& ){
    ++terminals[l];
  }
  
  void NewIBroot::summarizeNodes( std::vector<unsigned int>& terminals,
				  std::vector<unsigned int>& nonTerminals ){
    terminals.clear();
    nonTerminals.clear();
    terminals.resize( _depth+1, 0 );
    nonTerminals.resize( _depth+1, 0 );
    if ( _root ){
      _root->countBranches( 0, terminals, nonTerminals );
    }
  }
  
  void NewIBroot::prune() {
    assignDefaults( );
    if ( !_pruned ) {
      if ( _root ){
	_root->prune( _nodeCount );
      }
      _pruned = true;
    }
  }

  NewIBroot *NewIBroot::copy() const {
    NewIBroot *result = new NewIBroot( _depth, _random, _keepDist );
    result->_defAss = _defAss;
    result->_defValid = _defValid;
    result->_nodeCount = _nodeCount;
    result->_leafCount = _leafCount;
    result->_pruned = _pruned;
    result->_root = _root;
    result->topDist = topDist;
    return result;
  }

  NewIBroot* NewIBroot::IBPartition( NewIBTree *sub, size_t dep ) const {
    NewIBroot *result = new NewIBroot( _depth-dep, _random, _keepDist );
    result->_nodeCount = _nodeCount;
    result->_leafCount = _leafCount;
    result->_root = sub;
    if ( result->_root ){
      if ( !result->_root->TDistribution )
	result->_root->TDistribution = result->_root->sum_distributions( _keepDist );
      result->topDist = result->_root->TDistribution;
    }
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

  void NewIBbranch::initIt( IBiter& uit ){
    uit._map = &_mmap;
    uit.mit = uit._map->begin();
  }
  
  void NewIBbranch::assign( FeatureValue* fv, NewIBTree *t ){
    t->FValue = fv;
    _mmap[fv] = t;
  }

  inline NewIBTree *IBiter::find( FeatureValue *fv ) {
    //      cerr << "iter:map_find zoek " <<  fv << endl;
    mit = _map->find( fv );
    if ( mit != _map->end() ){
      //	cerr << "FOUND! " << endl;
      return mit->second;
    }
    else {
      //	cerr << "missed! " << endl;
      return 0;
    }
  }

  const ValueDistribution *NewIBroot::initTest( vector<FeatureValue *>& Path,
						const Instance *inst,
						size_t off,
						size_t eff ){
    const ValueDistribution *result = NULL;
    testInst = inst;
    //    cerr << "initTest for '" << inst << "'" << endl;
    // cerr << "offset = " << off << endl;
    // cerr << "effFeat = " << eff << endl;
    // cerr << "_depth = " << _depth << endl;
    offSet = off;
    effFeat = eff;
    _root->initIt( InstPath[0] );
    for ( unsigned int i = 0; i < _depth; ++i ){
      //      cerr << "voor find " << testInst->FV[i+offSet] << endl;
      NewIBTree *pnt = InstPath[i].find( testInst->FV[i+offSet] );
      if ( pnt ){
	//	cerr << "NewIBroot found " << testInst->FV[i+offSet] << endl;
	SkipSearch[i] = pnt;
	RestartSearch[i] = true;
      }
      else {
	//	cerr << "NewIBroot missed " << testInst->FV[i+offSet] << endl;
	RestartSearch[i] = false;
	SkipSearch[i] = 0;
	InstPath[i].reset();
	pnt = InstPath[i].value();
      }
      Path[i] = pnt->FValue;
      //      cerr << "set Path[" << i << "] to " << Path[i] << endl;
      if ( i == _depth-1 ){
	result = pnt->TDistribution;
      }
      else {
	pnt->initIt( InstPath[i+1] );
      }
    }
    if ( result && result->ZeroDist() ){
      // This might happen when doing LOO or CV tests
      size_t TmpPos = effFeat-1;
      result = nextTest( Path, TmpPos );
    }
    //    cerr << "Start " << Path << endl;
    return result;
  }

  const ValueDistribution *NewIBroot::nextTest( vector<FeatureValue *>& Path, 
						size_t& pos ){
    const ValueDistribution *result = 0;
    NewIBTree *pnt = 0;
    while ( !pnt  ){
      if ( !RestartSearch[pos] ) {
	// No exact match here, so no real problems
	InstPath[pos].increment();
	//	cerr << "no match before so just increment ";
	// if ( InstPath[pos]->value() )
	//   cerr << InstPath[pos]->value()->FValue;
	// cerr << endl;
      }
      else {
	InstPath[pos].reset();
	//	cerr << "restart at " << InstPath[pos]->value()->FValue << endl;	
	RestartSearch[pos] = false;
      }
      pnt = InstPath[pos].value();
      if ( pnt && pnt == SkipSearch[pos] ){
	//	cerr << "hit on Skip" << endl;
	InstPath[pos].increment();
	pnt = InstPath[pos].value();
	SkipSearch[pos] = 0;
	//	cerr << "go to next: " << InstPath[pos].FValue() << endl;	
      }
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
      Path[pos] = pnt->FValue;
      //      cerr << "set Path[" << pos << "] to " << Path[pos] << endl;
      if ( pos < _depth-1 ){
	pnt->initIt( InstPath[pos+1] );
	//	cerr << "Initialised pos = " << pos+1 << endl;
	for (  size_t j=pos+1; j < _depth; ++j ){
	  //	  cerr << "loop j = " << j << endl;
	  //	  cerr << InstPath[j]->value() << endl;
	  NewIBTree *pnt2 = InstPath[j].find( testInst->FV[j+offSet] );
	  if ( pnt2 ){ // we found an exact match, so mark Restart
	    SkipSearch[j] = pnt2;
	    RestartSearch[j] = true;
	  }
	  else { // no exact match at this level. Just start with the first....
	    SkipSearch[j] = 0;
	    InstPath[j].reset();
	    pnt2 = InstPath[j].value();
	    RestartSearch[j] = false;
	  }
	  Path[j] = pnt2->FValue;
	  //	  cerr << "set Path[" << j << "] to " << Path[j] << endl;
	  //	  cerr << "using InstPath[" << j << "] = " << InstPath[j].value() << endl;
	  if ( j == _depth-1 ){
	    result = pnt2->TDistribution;
	    //	    cerr << "assign result:" << result << endl;
	  }
	  else {
	    pnt2->initIt( InstPath[j+1] );
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
    //    cerr << "try next " << Path << " pos = " << pos << endl;
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
    unsigned int pos = 0;
    leaf = false;
    if ( _root ){
      const NewIBTree *pnt = _root->find( Inst.FV[0] );
      while ( pnt ){
	result = pnt->TValue;
	if ( _keepDist )
	  Dist = pnt->TDistribution;
	++pos;
	if ( pos < _depth ){
	  leaf = pnt->isLeaf();
	  pnt = pnt->find( Inst.FV[pos] );
	}
	else {
	  pnt = 0;
	  leaf = true;
	}
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

  NewIBroot *NewIBroot::TRIBL_test( const Instance& Inst, 
				    size_t treshold,
				    const TargetValue *&TV,
				    const ValueDistribution *&dist,
				    size_t &level ) {
    // The Test function for the TRIBL algorithm, returns a pointer to the
    // Target at the last matching position in the Tree, 
    // or the subtree Instance Base necessary for IB1
    assignDefaults( treshold );
    NewIBroot *subt = 0;
    TV = 0;
    dist = 0;
    if ( _root ){
      NewIBTree *pnt = _root->find( Inst.FV[0] );
      dist = topDist;
      TV = topTV;
      size_t pos = 0;
      while ( pnt && pos < treshold-1 ){
	dist = pnt->TDistribution;
	TV = pnt->TValue;
	++pos;
	if ( pos < _depth )
	  pnt = pnt->find( Inst.FV[pos] );
	else
	  pnt = 0;
      }
      if ( pos == treshold-1 ){
	if ( pnt ){
	  subt = IBPartition( pnt, treshold );
	  dist = 0;
	}
	else {
	  level = pos;
	}
      }      
      else {
	if ( pos == 0 && dist == 0 ){
	  if ( !WTop && topDist )
	    WTop = topDist->to_WVD_Copy();
	  dist = WTop;
	  bool dummy;
	  TV = topTarget(dummy);
	}
	else
	  level = pos;
      }
    }
    return subt;
  }

  NewIBroot *NewIBroot::TRIBL2_test( const Instance& Inst, 
				     const ValueDistribution *& dist,
				     size_t &level ){
    // The Test function for the TRIBL2 algorithm, returns a pointer to the
    // the subtree Instance Base necessary for IB1
    assignDefaults();
    dist = 0;
    unsigned int pos = 0;
    NewIBroot *subtree = 0;
    NewIBTree *pnt = _root;
    NewIBTree *last_match = 0;;
    while ( pnt ){
      last_match = pnt;
      // a match, go deeper
      if ( pos < _depth ){
	pnt = pnt->find( Inst.FV[pos] );
      }
      else {
	// at the end, an exact match
	dist = pnt->TDistribution;
	pnt = 0;
	last_match = 0;
      }
      if ( pnt ){
	//	cerr << "matched " <<  Inst.FV[pos]<< endl;
	pos++;
      }
    }
    if ( last_match ){
      subtree = IBPartition( last_match, pos );
      level = pos;
    }
    return subtree;
  }
    
}
