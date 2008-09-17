#ifndef CHOPPERS_H
#define CHOPPERS_H

namespace Timbl{

  class Chopper {
  public:
    void init( size_t len ) { choppedInput.resize(len+2); };
    virtual ~Chopper() {};
    virtual bool chop( const std::string&,
		       size_t ) = 0;
    const std::string& getField( size_t i ) const { return choppedInput[i]; };
    void setField( size_t i, std::string s ){ choppedInput[i] = s; };
    const std::string& getExW() const { return exW; };
    void setExW( std::string& s){ exW = s; };
    virtual void print( std::ostream& ) = 0;
    void swapTarget( size_t target_pos ){
      std::string tmp = choppedInput[target_pos];
      for ( size_t i = target_pos+1; i <= size; ++i )
	choppedInput[i-1] = choppedInput[i];
      choppedInput[size] = tmp;
    }
  protected:
    std::vector<std::string> choppedInput;
    std::string exW;
    size_t size;
  };
  
  class C45_Chopper : public Chopper {
  public:
    bool chop( const std::string& InBuf, 
	       size_t Len ) {
      size = Len;
      // Function that takes a line InBuf, and chops it up into substrings,
      // which represent the feature-values and the target-value.
      std::vector<std::string> splits;
      size_t res = split_at( InBuf, splits, "," );
      if ( res != Len + 1 )
	return false;
      for ( size_t i=0; i <= Len ; ++i ){
	choppedInput[i] = StrToCode( compress(splits[i]) );
      }
      return true;
    }
    void print( std::ostream& os ){ 
      for ( size_t i = 0; i <= size; ++i ) {
	os << CodeToStr( choppedInput[i] ) << ",";
      }
      if ( !exW.empty() )
	os << " " << exW;
    };
  };


  class ARFF_Chopper : public C45_Chopper {
  public:
    bool chop( const std::string&InBuf,
	       size_t Len ) {
      // Lines look like this:
      // one, two,   three , bla.
      // the termination dot is optional
      // WhiteSpace is skipped!
      return C45_Chopper::chop( compress( InBuf ), Len );
    }
  };
  
  class Bin_Chopper : public Chopper {
  public:
    bool chop( const std::string& InBuf,
	       size_t Len ) {
      // Lines look like this:
      // 12, 25, 333, bla.
      // the termination dot is optional
      size = Len;
      for ( size_t m = 0; m < Len; ++m )
	choppedInput[m] = "0";
      std::string::size_type s_pos = 0;
      std::string::size_type e_pos = InBuf.find( ',' );
      while ( e_pos != std::string::npos ){
	std::string tmp = std::string( InBuf, s_pos, e_pos - s_pos );
	size_t k;
	if ( !stringTo<size_t>( tmp, k, 1, Len ) )
	  return false;
	else
	  choppedInput[k-1] = "1";
	s_pos = e_pos + 1;
	e_pos = InBuf.find( ',', s_pos );
      }
      choppedInput[Len] = std::string( InBuf, s_pos );
      return true;
    }
    void print( std::ostream& os ){ 
      for ( size_t i = 0; i < size; ++i ) {
	if ( choppedInput[i][0] == '1' )
	  os << i+1 << ",";
      }
      os << choppedInput[size] << ",";
      if ( !exW.empty() )
	os  << " " << exW;
    }
  };
  
  class Compact_Chopper : public Chopper {
  public:
  Compact_Chopper( int L ): fLen(L){};
    
    bool chop( const std::string& InBuf, 
	       size_t Len ) {
      size = Len;
      size_t i;
      // Lines look like this:
      // ====AKBVAK
      // v1v2v3v4tt
      // Get & add the target.
      //
      size_t len = InBuf.length();
      if ( len != (Len+1) * fLen ){
	return false;
      }
      for ( i = 0; i <= Len; ++i ) {
	size_t index = i * fLen; 
	// Scan the value.
	//
	choppedInput[i] = "";
	for ( int j = 0; j < fLen; ++j ) {
	  choppedInput[i] += InBuf[index++];
	}
      }
      return ( i == Len+1 ); // Enough?
    }
    void print( std::ostream& os ){ 
      for ( size_t i = 0; i <= size; ++i ) {
	os << CodeToStr( choppedInput[i] );
      }
      if ( !exW.empty() )
	os << " " << exW;
    };

  private:
    int fLen;
  };
  
  class Columns_Chopper : public Chopper {
  public:
    bool chop( const std::string& InBuf,
	       size_t Len ) {
      // Lines look like this:
      // one  two three bla
      size = Len;
      unsigned int i = 0;
      std::string::size_type s_pos = 0;
      std::string::size_type e_pos = InBuf.find_first_of( " \t" );
      while ( e_pos != s_pos && e_pos != std::string::npos && i <= Len+1 ){
	// stop if a zero length string is found or if too many entries show up
	choppedInput[i++] = std::string( InBuf, s_pos, e_pos - s_pos );
	s_pos = InBuf.find_first_not_of( " \t", e_pos );
	e_pos = InBuf.find_first_of( " \t", s_pos );
      }
      if ( s_pos != std::string::npos && i <= Len+1 ){
	choppedInput[i++] = std::string( InBuf, s_pos );
      }
      return ( i == Len+1 ); // Enough?
    }
    void print( std::ostream& os ){ 
      for ( size_t i = 0; i <= size; ++i ) {
	os << choppedInput[i] << " ";
      }
      if ( !exW.empty() )
	os << " " << exW;
    };

  };

  static const std::string ThisDefaultSparseString = "0.0000E-17";

  class Sparse_Chopper : public Chopper {
  public:
    bool chop( const std::string& InBuf, 
	       size_t Len ) {
      // Lines look like this:
      // (12,value1) (25,value2) (333,value3) bla.
      // the termination dot is optional
      
      size = Len;
      for ( size_t m = 0; m < Len; ++m )
	choppedInput[m] = ThisDefaultSparseString;
      choppedInput[Len] = "";
      std::string::size_type s_pos = InBuf.find( "(" );
      if ( s_pos == std::string::npos )
	choppedInput[Len] = compress(InBuf);
      else {
	std::string::size_type m_pos, e_pos = InBuf.find( ")" );
	while ( s_pos < e_pos &&
		s_pos != std::string::npos  && e_pos != std::string::npos ){
	  m_pos = InBuf.find( ',', s_pos );
	  std::string temp = std::string( InBuf, s_pos + 1, m_pos - s_pos - 1 );
	  size_t k = 0;
	  if ( !stringTo<size_t>( temp, k, 1, Len ) )
	    return false;
	  else {
	    choppedInput[k-1] = std::string( InBuf, m_pos + 1, e_pos - m_pos -1 );
	    choppedInput[k-1] = StrToCode( compress(choppedInput[k-1]) );
	  }
	  s_pos = InBuf.find( '(', e_pos );
	  if ( s_pos == std::string::npos ){
	    e_pos = InBuf.find_first_not_of( ") \t", e_pos );
	    if ( e_pos != std::string::npos ){
	      choppedInput[Len] = std::string( InBuf, e_pos );
	      choppedInput[Len] = compress( choppedInput[Len] );
	    }
	  }
	  else
	    e_pos = InBuf.find( ')', s_pos );
	}
      }
      return !choppedInput[Len].empty();
    }
    void print( std::ostream& os ){ 
      for ( size_t i = 0; i < size; ++i ) {
	if ( choppedInput[i] != ThisDefaultSparseString )
	  os << "(" << i+1 << "," << CodeToStr(choppedInput[i]) << ")";
      }
      os << choppedInput[size] << ",";
      if ( !exW.empty() )
	os << " " << exW;
    }
  };
  

}
#endif // CHOPPERS_H
