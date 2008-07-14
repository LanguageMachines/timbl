#ifndef CHOPPERS_H
#define CHOPPERS_H

namespace Timbl{

  class Chopper {
  public:
    virtual ~Chopper() {};
    virtual bool chop( const std::string&,
		       std::vector<std::string>&,
		       size_t ) const = 0;
  };
  
  class C45_Chopper : public Chopper {
  public:
    bool chop( const std::string& InBuf, 
	       std::vector<std::string>& OutBuf, 
	       size_t Len ) const {
      // Function that takes a line InBuf, and chops it up into substrings,
      // which represent the feature-values and the target-value.
      std::vector<std::string> splits;
      size_t res = split_at( InBuf, splits, "," );
      if ( res != Len + 1 )
	return false;
      for ( size_t i=0; i <= Len ; ++i ){
	OutBuf[i] = StrToCode( compress(splits[i]) );
      }
      return true;
    }
  };


  class ARFF_Chopper : public C45_Chopper {
  public:
    bool chop( const std::string&InBuf,
	       std::vector<std::string>& OutBuf, 
	       size_t Len ) const {
      // Lines look like this:
      // one, two,   three , bla.
      // the termination dot is optional
      // WhiteSpace is skipped!
      return C45_Chopper::chop( compress( InBuf ), OutBuf, Len );
    }
  };
  
  class Bin_Chopper : public Chopper {
  public:
    bool chop( const std::string&InBuf,
	       std::vector<std::string>& OutBuf,
	       size_t Len ) const {
      // Lines look like this:
      // 12, 25, 333, bla.
      // the termination dot is optional
      for ( size_t m = 0; m < Len; ++m )
	OutBuf[m] = "0";
      std::string::size_type s_pos = 0;
      std::string::size_type e_pos = InBuf.find( ',' );
      while ( e_pos != std::string::npos ){
	std::string tmp = std::string( InBuf, s_pos, e_pos - s_pos );
	size_t k;
	if ( !stringTo<size_t>( tmp, k, 1, Len ) )
	  return false;
	else
	  OutBuf[k-1] = "1";
	s_pos = e_pos + 1;
	e_pos = InBuf.find( ',', s_pos );
      }
      OutBuf[Len] = std::string( InBuf, s_pos );
      return true;
    }
  };
  
  class Compact_Chopper : public Chopper {
  public:
  Compact_Chopper( int L ): Chopper(), fLen(L){};
    
    bool chop( const std::string& InBuf, 
	       std::vector<std::string>& OutBuf, 
	       size_t Len ) const {
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
	OutBuf[i] = "";
	for ( int j = 0; j < fLen; ++j ) {
	  OutBuf[i] += InBuf[index++];
	}
      }
      return ( i == Len+1 ); // Enough?
    }
  private:
    int fLen;
  };
  
  class Columns_Chopper : public Chopper {
  public:
    bool chop( const std::string& InBuf,
	       std::vector<std::string>& OutBuf,
	       size_t Len ) const {
      // Lines look like this:
      // one  two three bla
      unsigned int i = 0;
      std::string::size_type s_pos = 0;
      std::string::size_type e_pos = InBuf.find_first_of( " \t" );
      while ( e_pos != s_pos && e_pos != std::string::npos && i <= Len+1 ){
	// stop if a zero length string is found or if too many entries show up
	OutBuf[i++] = std::string( InBuf, s_pos, e_pos - s_pos );
	s_pos = InBuf.find_first_not_of( " \t", e_pos );
	e_pos = InBuf.find_first_of( " \t", s_pos );
      }
      if ( s_pos != std::string::npos && i <= Len+1 ){
	OutBuf[i++] = std::string( InBuf, s_pos );
      }
      return ( i == Len+1 ); // Enough?
    }
  };

  static const std::string ThisDefaultSparseString = "0.0000E-17";

  class Sparse_Chopper : public Chopper {
  public:
    bool chop( const std::string& InBuf, 
	       std::vector<std::string>& OutBuf,
	       size_t Len ) const {
      // Lines look like this:
      // (12,value1) (25,value2) (333,value3) bla.
      // the termination dot is optional
      
      for ( size_t m = 0; m < Len; ++m )
	OutBuf[m] = ThisDefaultSparseString;
      OutBuf[Len] = "";
      std::string::size_type s_pos = InBuf.find( "(" );
      if ( s_pos == std::string::npos )
	OutBuf[Len] = compress(InBuf);
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
	    OutBuf[k-1] = std::string( InBuf, m_pos + 1, e_pos - m_pos -1 );
	    OutBuf[k-1] = StrToCode( compress(OutBuf[k-1]) );
	  }
	  s_pos = InBuf.find( '(', e_pos );
	  if ( s_pos == std::string::npos ){
	    e_pos = InBuf.find_first_not_of( ") \t", e_pos );
	    if ( e_pos != std::string::npos ){
	      OutBuf[Len] = std::string( InBuf, e_pos );
	      OutBuf[Len] = compress( OutBuf[Len] );
	    }
	  }
	  else
	    e_pos = InBuf.find( ')', s_pos );
	}
      }
      return !OutBuf[Len].empty();
    }
  };
  

}
#endif // CHOPPERS_H
