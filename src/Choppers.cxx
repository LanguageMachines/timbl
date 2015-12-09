/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2015
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
#include <ostream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <cassert>
#include "ticcutils/StringOps.h"
#include "ticcutils/PrettyPrint.h"
#include "timbl/Types.h"
#include "timbl/Choppers.h"

using namespace std;
using namespace TiCC;

namespace Timbl{

  Chopper *Chopper::create( InputFormatType IF, bool doEx,
			    int fLen, bool doOcc ){
    Chopper *result = 0;
    switch ( IF ){
    case C4_5:
      if ( doOcc )
	result = new C45_OccChopper();
      else if ( doEx )
	result = new C45_ExChopper();
      else
	result = new C45_Chopper();
      break;
    case ARFF:
      if ( doOcc )
	result = new ARFF_OccChopper();
      else if ( doEx )
	result = new ARFF_ExChopper();
      else
	result = new ARFF_Chopper();
      break;
    case SparseBin:
      if ( doOcc )
	result = new Bin_OccChopper();
      else if ( doEx )
	result = new Bin_ExChopper();
      else
	result = new Bin_Chopper();
      break;
    case Sparse:
      if ( doOcc )
	result = new Sparse_OccChopper();
      else if ( doEx )
	result = new Sparse_ExChopper();
      else
	result = new Sparse_Chopper();
      break;
    case Columns:
      if ( doOcc )
	result = new Columns_OccChopper();
      else if ( doEx )
	result = new Columns_ExChopper();
      else
	result = new Columns_Chopper();
      break;
    case Tabbed:
      if ( doOcc )
	result = new Tabbed_OccChopper();
      else if ( doEx )
	result = new Tabbed_ExChopper();
      else
	result = new Tabbed_Chopper();
      break;
    case Compact:
      if ( doOcc )
	result = new Compact_OccChopper( fLen );
      else if ( doEx )
	result = new Compact_ExChopper( fLen );
      else
	result = new Compact_Chopper( fLen );
      break;
    default:
      break;
    }
    return result;
  }

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
  }

  static string stripExemplarWeight( const string& Buffer,
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

  static string stripOcc( const string& Buffer,
			  string& wght ) {
    return stripExemplarWeight( Buffer, wght );
  }

  size_t Chopper::countFeatures( const string& inBuffer,
				 InputFormatType IF,
				 int F_length,
				 bool chopTail ) {
    size_t result = 0;
    string buffer;
    if ( chopTail ){
      string dummy;
      buffer = stripExemplarWeight( inBuffer, dummy );
    }
    else
      buffer = inBuffer;
    size_t len = buffer.length();
    switch ( IF ){
    case ARFF:
    case C4_5:
      for ( auto const& c : buffer ){
	if ( c == ',')
	  ++result;
      };
      break;
    case Compact:
      if ( F_length == 0 ){
	throw runtime_error( "-F Compact specified, but Feature Length not set."
			     " (-l option)" );
	return result;
      }
      else
	result = (len / F_length) - 1;
      break;
    case Columns:
      {
	vector<string> parts;
	size_t num = split( buffer, parts );
	result = num - 1;
      };
      break;
    case Tabbed:
      {
	vector<string> parts;
	size_t num = split_at( buffer, parts, "\t" );
	result = num - 1;
      };
      break;
    default:
      throw logic_error( "CountFeatures: Illegal value in switch:" +
			 toString(IF) );
    };
    return result;
  }


  InputFormatType Chopper::getInputFormat( const string& inBuffer,
					   bool stripTail ) {
    InputFormatType IF = UnknownInputFormat;
    string buffer;
    if ( stripTail ){
      string dummy;
      buffer = stripExemplarWeight( inBuffer, dummy );
    }
    else
      buffer = inBuffer;
    size_t len = buffer.length();
    int c45Cnt = 0;
    int columnCnt = 0;
    for ( unsigned int i = 0; i < len; ++i ) {
      if ( buffer[i] == ',' ) {
	++c45Cnt;
      }
      else if ( isspace( buffer[i] ) ){
	++columnCnt;
	while ( i < len && isspace( buffer[i+1] ) ) ++i;
	if ( i >= len-1 ){ // just trailing spaces!
	  --columnCnt;
	}
      }
    }
    if ( columnCnt == 0 && c45Cnt == 0 )
      IF = Compact;
    else if ( c45Cnt >= columnCnt )
      IF = C4_5;
    else
      IF = Columns;
    return IF;
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
  }

  void OccChopper::init( const string& s, size_t len, bool stripDot ) {
    occ = 1;
    strippedInput = s;
    vSize = len+1;
    choppedInput.resize(vSize);
    string::iterator it = strippedInput.end();
    --it;
    // first trim trailing spaces
    while ( it != strippedInput.begin() &&
	    isspace(*it) ) --it;
    strippedInput.erase( ++it , strippedInput.end() );
    string occS;
    strippedInput = stripOcc( strippedInput, occS );
    if ( occS.empty() ){
      throw logic_error( "Missing occurence" );
    }
    else {
      int tmp;
      if ( !stringTo<int>( occS, tmp ) ){
	throw runtime_error( "Wrong (non-integer) occurence value: '" + occS + "'" );
      }
      else {
	occ = tmp;
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
  }

  bool C45_Chopper::chop( const string& InBuf, size_t len ){
    // Function that takes a line, and chops it up into substrings,
    // which represent the feature-values and the target-value.
    init( InBuf, len, true );
    vector<string> splits;
    size_t res = split_at( strippedInput, splits, "," );
    if ( res != vSize )
      return false;
    for ( size_t i=0; i < res ; ++i ){
      choppedInput[i] = StrToCode( splits[i] );
    }
    return true;
  }

  string C45_Chopper::getString() const{
    string res;
    for ( size_t i = 0; i < vSize; ++i ) {
      res += CodeToStr( choppedInput[i] ) + ",";
    }
    return res;
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

  string Bin_Chopper::getString() const {
    string res;
    for ( size_t i = 0; i < vSize-1; ++i ) {
      if ( choppedInput[i][0] == '1' )
	res += toString(i+1) + ",";
    }
    res += choppedInput[vSize-1] + ",";
    return res;
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

  string Compact_Chopper::getString() const {
    string res;
    for ( size_t i = 0; i < vSize; ++i ) {
      res += CodeToStr( choppedInput[i] );
    }
    return res;
  }

  bool Columns_Chopper::chop( const string& InBuf, size_t len ){
    // Lines look like this:
    // one  two three bla
    init( InBuf, len, false );
    vector<string> splits;
    size_t res = split( strippedInput, splits );
    if ( res != vSize )
      return false;
    for ( size_t i=0; i < res ; ++i ){
      choppedInput[i] = StrToCode( splits[i] );
    }
    return ( res == vSize ); // Enough?
  }

  string Columns_Chopper::getString() const {
    string res;
    for ( size_t i = 0; i < vSize; ++i ) {
      res += choppedInput[i] + " ";
    }
    return res;
  }



  bool Tabbed_Chopper::chop( const string& InBuf, size_t len ){
    // Lines look like this:
    // one  two three bla
    init( InBuf, len, false );
    vector<string> splits;
    size_t res = split_at( strippedInput, splits, "\t" );
    if ( res != vSize )
      return false;
    for ( size_t i=0; i < res ; ++i ){
      choppedInput[i] = StrToCode( splits[i] );
    }
    return ( res == vSize ); // Enough?
  }

  string Tabbed_Chopper::getString() const {
    string res;
    for ( size_t i = 0; i < vSize; ++i ) {
      res += CodeToStr( choppedInput[i] ) + "\t";
    }
    return res;
  }

  bool Sparse_Chopper::chop( const string& InBuf, size_t len ){
    // Lines look like this:
    // (12,value1) (25,value2) (333,value3) bla.
    // the termination dot is optional
    init( InBuf, len, true );
    for ( size_t m = 0; m < vSize-1; ++m )
      choppedInput[m] = DefaultSparseString;
    choppedInput[vSize-1] = "";
    vector<string> entries;
    size_t num_ent = split_at_first_of( strippedInput, entries, "()" );
    if ( num_ent < 1 )
      return false;
    for ( const auto& ent : entries ){
      --num_ent;
      vector<string> parts;
      size_t num = split_at( ent, parts, "," );
      if ( num != 2 ){
	if ( num == 1 && num_ent == 0 ){
	  // the target has no ','
	  choppedInput[vSize-1] = trim(parts[0]);
	  return !choppedInput[vSize-1].empty();
	}
	return false;
      }
      if ( num_ent == 0 ){
	// missing a target!
	return false;
      }
      size_t index;
      if ( !stringTo( parts[0], index ) )
	return false;
      choppedInput[index-1] = StrToCode( parts[1] );
    }
    return true;
  }

  string Sparse_Chopper::getString() const {
    string res;
    for ( size_t i = 0; i < vSize-1; ++i ) {
      if ( choppedInput[i] != DefaultSparseString )
	res += "(" + toString( i+1 ) + "," + CodeToStr(choppedInput[i]) + ")";
    }
    res += choppedInput[vSize-1] + ",";
    return res;
  }

}
