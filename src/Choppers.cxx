/*
  Copyright (c) 1998 - 2008
  ILK  -  Tilburg University
  CNTS -  University of Antwerp
 
  This file is part of Timbl

  Timbl is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Timbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      http://ilk.uvt.nl/software.html
  or send mail to:
      Timbl@uvt.nl
*/
#include <ostream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <cassert>
#include "timbl/StringOps.h"
#include "timbl/Types.h"
#include "timbl/Choppers.h"

using namespace std;

namespace Timbl{

  void Chopper::init( const string& s, size_t len, bool stripDot ) {
    strippedInput = s;
    vSize = len+1;
    choppedInput.resize(vSize);
    string::iterator it = strippedInput.end();
    --it;
    // first trim trailing spaces 
    while ( it != strippedInput.begin() && 
	    isspace(*it) ) --it;
    strippedInput.erase( ++it , strippedInput.end() );
    it = strippedInput.end();
    --it;
    if ( stripDot ){
      // first trim trailing dot
      if ( it != strippedInput.begin() && *it == '.' )
	--it;
    }
    // strip remaining trailing spaces
    while ( it != strippedInput.begin() && 
	    isspace(*it) ) --it;
    strippedInput.erase( ++it , strippedInput.end() );
  };

  string Chopper::stripExemplarWeight( const string& Buffer,
				       string& wght ) {
    string::size_type t_pos, e_pos = Buffer.length();
    // first remove trailing whitespace and dot
    e_pos = Buffer.find_last_not_of( ". \t", e_pos );
    // now some non-space
    t_pos = Buffer.find_last_of( " \t", e_pos );
    if ( t_pos != string::npos ){
      // found white space
      wght = string( Buffer, t_pos+1, e_pos - t_pos );
    }
    else {
      wght = "";
    }
    // and some more space...
    e_pos = Buffer.find_last_not_of( " \t", t_pos );
    return string( Buffer, 0, e_pos+1 );
  }


  void ExChopper::init( const string& s, size_t len, bool stripDot ) {
    exW = -1.0;
    strippedInput = s;
    vSize = len+1;
    choppedInput.resize(vSize);
    string::iterator it = strippedInput.end();
    --it;
    // first trim trailing spaces 
    while ( it != strippedInput.begin() && 
	    isspace(*it) ) --it;
    strippedInput.erase( ++it , strippedInput.end() );
    string wght;
    strippedInput = stripExemplarWeight( strippedInput, wght );
    if ( wght.empty() ){
      throw logic_error( "Missing sample weight" );
    }
    else {
      double tmp;
      if ( !stringTo<double>( wght, tmp ) ){
	throw runtime_error( "Wrong sample weight: '" + wght + "'" );
      }
      else {
	exW = tmp;
      }
    }
    it = strippedInput.end();
    --it;
    if ( stripDot ){
      // first trim trailing dot
      if ( it != strippedInput.begin() && *it == '.' )
	--it;
    }
    // strip remaining trailing spaces
    while ( it != strippedInput.begin() && 
	    isspace(*it) ) --it;
    strippedInput.erase( ++it , strippedInput.end() );
  };

  bool C45_Chopper::chop( const string& InBuf, size_t len ){
    // Function that takes a line, and chops it up into substrings,
    // which represent the feature-values and the target-value.
    init( InBuf, len, true );
    vector<string> splits;
    size_t res = split_at( strippedInput, splits, "," );
    if ( res != vSize )
      return false;
    for ( size_t i=0; i < res ; ++i ){
      choppedInput[i] = StrToCode( compress(splits[i]) );
    }
    return true;
  }
  
  void C45_Chopper::print( ostream& os ){ 
    for ( size_t i = 0; i < vSize; ++i ) {
      os << CodeToStr( choppedInput[i] ) << ",";
    }
  }

  bool ARFF_Chopper::chop( const string& InBuf, size_t len ){
    // Lines look like this:
    // one, two,   three , bla.
    // the termination dot is optional
    // WhiteSpace is skipped!
    return C45_Chopper::chop( InBuf, len );
  }
  
  bool Bin_Chopper::chop( const string& InBuf, size_t len ) {
    // Lines look like this:
    // 12, 25, 333, bla.
    // the termination dot is optional
    init( InBuf, len, true );
    for ( size_t m = 0; m < vSize-1; ++m )
      choppedInput[m] = "0";
    string::size_type s_pos = 0;
    string::size_type e_pos = strippedInput.find( ',' );
    while ( e_pos != string::npos ){
      string tmp = string( strippedInput, s_pos, e_pos - s_pos );
      size_t k;
      if ( !stringTo<size_t>( tmp, k, 1, vSize-1 ) )
	return false;
      else
	choppedInput[k-1] = "1";
      s_pos = e_pos + 1;
      e_pos = strippedInput.find( ',', s_pos );
    }
    choppedInput[vSize-1] = string( strippedInput, s_pos );
    return true;
  }
  
  void Bin_Chopper::print( ostream& os ){ 
    for ( size_t i = 0; i < vSize-1; ++i ) {
      if ( choppedInput[i][0] == '1' )
	os << i+1 << ",";
    }
    os << choppedInput[vSize-1] << ",";
  }
  
  bool Compact_Chopper::chop( const string& InBuf, size_t leng ){
    init( InBuf, leng, false );
    size_t i;
    // Lines look like this:
    // ====AKBVAK
    // v1v2v3v4tt
    // Get & add the target.
    //
    size_t len = strippedInput.length();
    if ( len != vSize * fLen ){
      return false;
    }
    for ( i = 0; i < vSize; ++i ) {
      size_t index = i * fLen; 
      // Scan the value.
      //
      choppedInput[i] = "";
      for ( int j = 0; j < fLen; ++j ) {
	choppedInput[i] += strippedInput[index++];
      }
    }
    return ( i == vSize ); // Enough?
  }
  
  void Compact_Chopper::print( ostream& os ){ 
    for ( size_t i = 0; i < vSize; ++i ) {
      os << CodeToStr( choppedInput[i] );
    }
  };
  
  bool Columns_Chopper::chop( const string& InBuf, size_t len ){
    // Lines look like this:
    // one  two three bla
    init( InBuf, len, false );
    unsigned int i = 0;
    string::size_type s_pos = 0;
    string::size_type e_pos = strippedInput.find_first_of( " \t" );
    while ( e_pos != s_pos && e_pos != string::npos && i < vSize ){
      // stop if a zero length string is found or if too many entries show up
      choppedInput[i++] = string( strippedInput, s_pos, e_pos - s_pos );
      s_pos = strippedInput.find_first_not_of( " \t", e_pos );
      e_pos = strippedInput.find_first_of( " \t", s_pos );
    }
    if ( e_pos != string::npos )
      return false;
    if ( s_pos != string::npos && i < vSize ){
      choppedInput[i++] = string( strippedInput, s_pos );
    }
    return ( i == vSize ); // Enough?
  }
  
  void Columns_Chopper::print( ostream& os ){ 
    for ( size_t i = 0; i < vSize; ++i ) {
      os << choppedInput[i] << " ";
    }
  };
  
  bool Sparse_Chopper::chop( const string& InBuf, size_t len ){
    // Lines look like this:
    // (12,value1) (25,value2) (333,value3) bla.
    // the termination dot is optional
    init( InBuf, len, true );
    for ( size_t m = 0; m < vSize-1; ++m )
      choppedInput[m] = DefaultSparseString;
    choppedInput[vSize-1] = "";
    string::size_type s_pos = strippedInput.find( "(" );
    if ( s_pos == string::npos )
      choppedInput[vSize-1] = compress(strippedInput);
    else {
      string::size_type m_pos, e_pos = strippedInput.find( ")" );
      while ( s_pos < e_pos &&
	      s_pos != string::npos  && e_pos != string::npos ){
	m_pos = strippedInput.find( ',', s_pos );
	string temp = string( strippedInput, s_pos + 1, m_pos - s_pos - 1 );
	size_t k = 0;
	if ( !stringTo<size_t>( temp, k, 1, vSize-1 ) )
	  return false;
	else {
	  choppedInput[k-1] = string( strippedInput, m_pos + 1, e_pos - m_pos -1 );
	  choppedInput[k-1] = StrToCode( compress(choppedInput[k-1]) );
	}
	s_pos = strippedInput.find( '(', e_pos );
	if ( s_pos == string::npos ){
	  e_pos = strippedInput.find_first_not_of( ") \t", e_pos );
	  if ( e_pos != string::npos ){
	    choppedInput[vSize-1] = string( strippedInput, e_pos );
	    choppedInput[vSize-1] = compress( choppedInput[vSize-1] );
	  }
	}
	else
	  e_pos = strippedInput.find( ')', s_pos );
      }
    }
    return !choppedInput[vSize-1].empty();
  }
  
  void Sparse_Chopper::print( ostream& os ){ 
    for ( size_t i = 0; i < vSize-1; ++i ) {
      if ( choppedInput[i] != DefaultSparseString )
	os << "(" << i+1 << "," << CodeToStr(choppedInput[i]) << ")";
    }
    os << choppedInput[vSize-1] << ",";
  }  
  
}
