#ifndef INDEXCLASS_H
#define INDEXCLASS_H

#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <stdexcept>

// #define DEBUG
template< class KEY, class VALUE, class Compare = std::less<KEY> >
  class IndexClass;

template< class KEY, class VALUE, class Compare >
  IndexClass<KEY,VALUE,Compare>* createIndexClass( const std::vector<KEY>& , 
						   unsigned int,
						   int, VALUE );

template< class KEY, class VALUE, class Compare>
  class IndexClass {
  friend IndexClass<KEY,VALUE,Compare>* createIndexClass<>( const std::vector<KEY>& , 
							    unsigned int,
							    int, VALUE );
 public:
  virtual ~IndexClass(){};
  virtual void put( std::ostream&, int ) const;
  virtual IndexClass *addNext( const std::vector<KEY>&, 
			       unsigned int, int, VALUE ) =0;
  virtual VALUE next() const = 0;
};

template< class KEY, class VALUE, class Compare = std::less<KEY> >
  class msIndexClass: public IndexClass<KEY,VALUE,Compare> {
 public:
 void put( std::ostream&, int ) const;
 IndexClass<KEY,VALUE,Compare> *addNext( const std::vector<KEY>&, unsigned int, 
					 int, VALUE );
 VALUE next() const;
 protected:
 mutable typename std::map<KEY, std::set<VALUE>, Compare >::const_iterator mit;
 mutable typename std::set<VALUE>::const_iterator sit;
 std::map<KEY, std::set<VALUE>, Compare > _map;
};

template< class KEY, class VALUE, class Compare = std::less<KEY> >
  class mmIndexClass: public IndexClass<KEY,VALUE,Compare> {
 public:
 ~mmIndexClass();
 void put( std::ostream&, int ) const;
 IndexClass<KEY,VALUE,Compare> *addNext( const std::vector<KEY>&, 
					 unsigned int, int, VALUE );
 VALUE next() const;
 protected:
 mutable typename std::map<KEY, IndexClass<KEY,VALUE,Compare> *, Compare >::const_iterator mit;
 std::map<KEY, IndexClass<KEY,VALUE,Compare> *, Compare> _mmap;
};

template< class KEY, class VALUE, class Compare = std::less<KEY> >
  class rootIndexClass: public IndexClass<KEY,VALUE,Compare> {
 public:
 rootIndexClass( int depth ): _depth(depth),_root(0) {};
 ~rootIndexClass();
 void add( const std::vector<KEY>&, VALUE );
 void put( std::ostream&, int ) const;
 IndexClass<KEY, VALUE, Compare> *addNext( const std::vector<KEY>&, unsigned int, int, VALUE ){ throw std::logic_error("addNext()");};
 VALUE next() const;
 protected:
 int _depth;
 IndexClass<KEY,VALUE,Compare> *_root;
};

template< class KEY, class VALUE, class Compare>
inline void IndexClass<KEY,VALUE,Compare>::put( std::ostream& os, int level ) const {
}

template< class KEY, class VALUE, class Compare>
inline std::ostream& operator<< ( std::ostream& os, 
				  const IndexClass<KEY,VALUE, Compare>& o ){
  o.put( os, 0 );
  return os;
}

template< class KEY, class VALUE, class Compare>
inline std::ostream& operator<< ( std::ostream& os, 
				  const IndexClass<KEY,VALUE,Compare> *o ){
  if ( o )
    o->put( os, 0 );
  else
    os << "nO";
  return os;
}

template< class KEY, class VALUE, class Compare >
inline IndexClass<KEY,VALUE,Compare> 
*msIndexClass<KEY,VALUE,Compare>::addNext( const std::vector<KEY>& v, 
					   unsigned int start_pos,
					   int depth,
					   VALUE val  ){
  if ( start_pos < v.size() ){
    typename std::map<KEY,std::set<VALUE>, Compare >::iterator it = _map.find( v[start_pos] );
    if ( it != _map.end() ){
      it->second.insert( val );
    }
    else {
      std::set<VALUE> st;
      st.insert( val );
      _map[v[start_pos]] = st;
      mit = _map.begin();
    }
    sit = mit->second.begin();
  }
  return this;
}

template< class KEY, class VALUE, class Compare >
inline void msIndexClass<KEY,VALUE,Compare>::put( std::ostream& os, int level ) const{
  int l = level;
  while ( l-- > 0 )
    os << "\t";
  os << "[";
  typename std::map<KEY,std::set<VALUE>, Compare >::const_iterator it = _map.begin();
  while ( it != _map.end() ){
    os << it->first << " ";
    typename std::set<VALUE>::const_iterator sit=it->second.begin();
    os << "{";
    while ( sit != it->second.end() ){
      os << *sit;
      ++sit;
      if ( sit != it->second.end() )
	os << ",";
    }
    os << "}";
    ++it;
    if ( it != _map.end() ){
      os << std::endl << "  ";
      int l = level;
      while ( l-- > 0 )
	os << "\t\t";
    }
  }
  os << "]";
}
 
 template< class KEY, class VALUE, class Compare>
 inline VALUE msIndexClass<KEY,VALUE,Compare>::next() const {
   VALUE v = VALUE();
   if ( mit != _map.end() ){
#ifdef DEBUG
     std::cerr << "msNext()" << mit->first << std::endl;
#endif
     if ( sit != mit->second.end() ){
       v = *sit;
       ++sit;
       if ( sit == mit->second.end() ){
#ifdef DEBUG
	 std::cerr << "sit at end 1" << std::endl;
#endif
	 ++mit;
	 if ( mit != _map.end() ){
	   sit = mit->second.begin();
	 }
#ifdef DEBUG
	 else {
	   std::cerr << "mit at end 1" << std::endl;
	 }
#endif   
       }
     }
     else {
       ++mit;
       if ( mit != _map.end() ){
	 sit = mit->second.begin();
	 v = *sit;
	 ++sit;
	 if ( sit == mit->second.end() ){
#ifdef DEBUG
	   std::cerr << "sit at end 2" << std::endl;
#endif
	   ++mit;
	   if ( mit != _map.end() ){
	     sit = mit->second.begin();
	   }
#ifdef DEBUG
	   else {
	     std::cerr << "mit at end 2" << std::endl;
	   }
#endif
	 }
       }
#ifdef DEBUG
       else {
	 std::cerr << "mit at end 3" << std::endl;
       }
#endif
     }
   }
   return v;
 }
 
template< class KEY, class VALUE, class Compare >
inline mmIndexClass<KEY,VALUE,Compare>::~mmIndexClass<KEY,VALUE,Compare>(){
  typename std::map<KEY,IndexClass<KEY,VALUE,Compare>*, Compare>::const_iterator it = _mmap.begin();  
  while ( it != _mmap.end() ){
    delete it->second;
    ++it;
  }
}

template< class KEY, class VALUE, class Compare >
inline void mmIndexClass<KEY,VALUE,Compare>::put( std::ostream& os, int level ) const{
  int l = level;
  while ( l-- > 0 )
    os << "\t";
  os << "[";
  typename std::map<KEY,IndexClass<KEY,VALUE,Compare>*,Compare>::const_iterator it = _mmap.begin();
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

 template< class KEY, class VALUE, class Compare>
inline VALUE mmIndexClass<KEY,VALUE,Compare>::next() const {
  VALUE v = VALUE();
  if ( mit != _mmap.end() ){
    VALUE nv = mit->second->next();
    while ( v == nv ){
      ++mit;
      if ( mit != _mmap.end() ){
	nv = mit->second->next();
      }
      else
	return v;
    }
    v = nv;
  }
  return v;
}

template< class KEY, class VALUE, class Compare >
inline IndexClass<KEY,VALUE,Compare> 
*createIndexClass( const std::vector<KEY>& v, 
		   unsigned int start_pos,
		   int depth,
		   VALUE val ){
  IndexClass<KEY,VALUE,Compare> *result = 0;
  if ( start_pos+1 >= depth || start_pos == v.size() - 1 ){
    result = new msIndexClass<KEY,VALUE,Compare>( );
  }
  else if ( start_pos < v.size() - 1 ) {
    result = new mmIndexClass<KEY,VALUE,Compare>( );
  }
  result = result->addNext( v, start_pos, depth, val );
  return result;
}

template< class KEY, class VALUE, class Compare>
inline void rootIndexClass<KEY,VALUE,Compare>::add( const std::vector<KEY>& vec, VALUE val ){
  if ( !_root )
    _root = createIndexClass<KEY,VALUE,Compare>( vec, 0, _depth, val );
  else
    _root = _root->addNext( vec, 0, _depth, val );
}

template< class KEY, class VALUE, class Compare>
inline rootIndexClass<KEY,VALUE,Compare>::~rootIndexClass(){
  delete _root;
}

template< class KEY, class VALUE, class Compare >
inline void rootIndexClass<KEY,VALUE,Compare>::put( std::ostream& os, int ) const{
  os << "\n[" << _root << "]" << std::endl;
}

template< class KEY, class VALUE, class Compare>
inline VALUE rootIndexClass<KEY,VALUE,Compare>::next() const {
  if ( _root )
    return _root->next();
  else
    return VALUE();
}

template< class KEY, class VALUE, class Compare >
inline IndexClass<KEY,VALUE,Compare>
*mmIndexClass<KEY,VALUE,Compare>::addNext( const std::vector<KEY>& vec,
					   unsigned int start_pos,
					   int depth, 
					   VALUE val  ){
  IndexClass<KEY,VALUE,Compare> *result = this;
  if ( start_pos < vec.size() ){
    typename std::map<KEY,IndexClass<KEY,VALUE,Compare>*,Compare>::iterator it = 
      _mmap.find( vec[start_pos] );
    if ( it != _mmap.end() ){
      it->second = it->second->addNext( vec, start_pos+1, depth, val );
    }
    else {
      IndexClass<KEY,VALUE,Compare> *o = createIndexClass<KEY,VALUE,Compare>( vec, start_pos+1, depth, val );
      _mmap[vec[start_pos]] = o;
      mit = _mmap.begin();
    }
  }
  if ( result != this ){
    std::cerr << "it changed!" << std::endl;
    delete this;
  }
  return result;
}


#endif
