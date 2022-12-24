/*
  Copyright (c) 1998 - 2023
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
#include <ostream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <cassert>
#include "ticcutils/StringOps.h"
#include "ticcutils/Unicode.h"
#include "ticcutils/PrettyPrint.h"
#include "timbl/Types.h"
#include "timbl/Choppers.h"

using namespace std;
using namespace icu;

namespace Timbl{

  Chopper *Chopper::create( InputFormatType IF, bool doEx,
			    int fLen, bool doOcc ){
    Chopper *result = 0;
    switch ( IF ){
    case C4_5:
      if ( doOcc ){
	result = new C45_OccChopper();
      }
      else if ( doEx ){
	result = new C45_ExChopper();
      }
      else {
	result = new C45_Chopper();
      }
      break;
    case ARFF:
      if ( doOcc ){
	result = new ARFF_OccChopper();
      }
      else if ( doEx ){
	result = new ARFF_ExChopper();
      }
      else {
	result = new ARFF_Chopper();
      }
      break;
    case SparseBin:
      if ( doOcc ){
	result = new Bin_OccChopper();
      }
      else if ( doEx ){
	result = new Bin_ExChopper();
      }
      else {
	result = new Bin_Chopper();
      }
      break;
    case Sparse:
      if ( doOcc ){
	result = new Sparse_OccChopper();
      }
      else if ( doEx ){
	result = new Sparse_ExChopper();
      }
      else {
	result = new Sparse_Chopper();
      }
      break;
    case Columns:
      if ( doOcc ){
	result = new Columns_OccChopper();
      }
      else if ( doEx ){
	result = new Columns_ExChopper();
      }
      else {
	result = new Columns_Chopper();
      }
      break;
    case Tabbed:
      if ( doOcc ){
	result = new Tabbed_OccChopper();
      }
      else if ( doEx ){
	result = new Tabbed_ExChopper();
      }
      else {
	result = new Tabbed_Chopper();
      }
      break;
    case Compact:
      if ( doOcc ){
	result = new Compact_OccChopper( fLen );
      }
      else if ( doEx ) {
	result = new Compact_ExChopper( fLen );
      }
      else {
	result = new Compact_Chopper( fLen );
      }
      break;
    default:
      break;
    }
    return result;
  }

  void Chopper::init( const UnicodeString& s, size_t len, bool stripDot ) {
    vSize = len+1;
    choppedInput.resize(vSize);
    UnicodeString split = s;
    //    cerr << "    strip input:" << split << endl;
    // trim spaces at end
    split = TiCC::rtrim( split );
    if ( stripDot ){
      // now trim at most 1 trailing dot
      if ( split[split.length()-1] == '.' ){
	split.remove( split.length()-1 );
      }
    }
    // trim more spaces at end
    strippedInput = TiCC::rtrim( split );
    //    cerr << "stripped input:" << strippedInput << endl;
  }

  static UnicodeString extractWeight( const UnicodeString& buffer,
				      UnicodeString& wght ) {
    //    cerr << "extract weight from '" << buffer << "'" << endl;
    UnicodeString tmp = buffer;
    // first remove trailing whitespace and dots
    tmp = TiCC::rtrim( tmp, " ." );
    //    cerr << "step 1: '" << tmp << "'" << endl;
    int e_pos = tmp.length()-1;
    for ( ; e_pos >= 0; --e_pos ){
      if ( tmp[e_pos] == ' '
	   || tmp[e_pos] == '\t' ){
	break;
      }
    }
    if ( e_pos == 0 ){
      wght = "";
    }
    else {
      wght = UnicodeString( tmp, e_pos+1 );
      tmp.remove( e_pos );
    }
    tmp = TiCC::rtrim( tmp, "\t ." );
    //    cerr << "result='" << tmp << "' with weight: '" << wght << "'" << endl;
    return tmp;
  }

  static UnicodeString extractOcc( const UnicodeString& Buffer,
				   UnicodeString& occ ) {
    return extractWeight( Buffer, occ );
  }

  size_t Chopper::countFeatures( const UnicodeString& inBuffer,
				 InputFormatType IF,
				 int F_length,
				 bool chopTail ) {
    size_t result = 0;
    UnicodeString buffer = inBuffer;
    if ( chopTail ){
      UnicodeString dummy;
      buffer = extractWeight( buffer, dummy );
    }
    size_t len = buffer.length();
    switch ( IF ){
    case ARFF:
    case C4_5:
      for ( int i=0; i < buffer.length(); ++i ){
	if ( buffer[i] == ','){
	  ++result;
	}
      };
      break;
    case Compact:
      if ( F_length == 0 ){
	throw runtime_error( "-F Compact specified, but Feature Length not set."
			     " (-l option)" );
      }
      else {
	result = (len / F_length) - 1;
      }
      break;
    case Columns:
      {
	vector<UnicodeString> parts = TiCC::split( buffer );
	result = parts.size() - 1;
      };
      break;
    case Tabbed:
      {
	vector<UnicodeString> parts = TiCC::split_at( buffer, "\t" );
	result = parts.size() - 1;
      };
      break;
    default:
      throw logic_error( "CountFeatures: Illegal value in switch:" +
			 TiCC::toString(IF) );
    };
    return result;
  }


  InputFormatType Chopper::getInputFormat( const UnicodeString& inBuffer,
					   bool stripTail ) {
    InputFormatType IF = UnknownInputFormat;
    UnicodeString buffer = inBuffer;
    if ( stripTail ){
      UnicodeString dummy;
      buffer = extractWeight( buffer, dummy );
    }
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
    if ( columnCnt == 0 && c45Cnt == 0 ){
      IF = Compact;
    }
    else if ( c45Cnt >= columnCnt ){
      IF = C4_5;
    }
    else {
      IF = Columns;
    }
    return IF;
  }

  void ExChopper::init( const UnicodeString& s, size_t len, bool stripDot ) {
    exW = -1.0;
    UnicodeString split = s;
    vSize = len+1;
    choppedInput.resize(vSize);
    // trim trailing spaces
    split = TiCC::rtrim( split );
    UnicodeString wght;
    split = extractWeight( split, wght );
    if ( wght.isEmpty() ){
      throw logic_error( "Missing sample weight" );
    }
    else {
      double tmp;
      if ( !TiCC::stringTo<double>( wght, tmp ) ){
	throw runtime_error( "Wrong sample weight: '"
			     + TiCC::UnicodeToUTF8(wght) + "'" );
      }
      else {
	exW = tmp;
      }
    }
    if ( stripDot ){
      // now trim at most 1 trailing dot
      if ( split[split.length()-1] == '.' ){
	split.remove( split.length()-1 );
      }
    }
    // trim more trailing spaces
    strippedInput = TiCC::rtrim( split );
  }

  void OccChopper::init( const UnicodeString& s, size_t len, bool stripDot ) {
    UnicodeString split = s;
    occ = 1;
    vSize = len+1;
    choppedInput.resize(vSize);
    // first trim trailing spaces
    split = TiCC::rtrim( split );
    UnicodeString occS;
    // get occ
    split = extractOcc( split, occS );
    if ( occS.isEmpty() ){
      throw logic_error( "Missing occurrence" );
    }
    else {
      int tmp;
      if ( !TiCC::stringTo<int>( occS, tmp ) ){
	throw runtime_error( "Wrong (non-integer) occurrence value: '"
			     + TiCC::UnicodeToUTF8(occS) + "'" );
      }
      else {
	occ = tmp;
      }
    }
    if ( stripDot ){
      // now trim at most 1 trailing dot
      if ( split[split.length()-1] == '.' ){
	split.remove( split.length()-1 );
      }
    }
    // strip remaining trailing spaces
    strippedInput = TiCC::rtrim( split );
  }

  using TiCC::operator<<;
  bool C45_Chopper::chop( const UnicodeString& InBuf, size_t len ){
    // Function that takes a line, and chops it up into substrings,
    // which represent the feature-values and the target-value.
    init( InBuf, len, true );
    vector<UnicodeString> splits = TiCC::split_at( strippedInput, "," );
    size_t res = splits.size();
    if ( res != vSize ){
      return false;
    }
    for ( size_t i=0; i < res ; ++i ){
      choppedInput[i] = StrToCode( splits[i] );
    }
    //    cerr << "Chopped input=" << choppedInput << endl;
    return true;
  }

  UnicodeString C45_Chopper::getString() const{
    UnicodeString res;
    for ( const auto& chop : choppedInput ) {
      res += CodeToStr( chop ) + ",";
    }
    return res;
  }

  bool ARFF_Chopper::chop( const UnicodeString& InBuf, size_t len ){
    // Lines look like this:
    // one, two,   three , bla.
    // the termination dot is optional
    // WhiteSpace is skipped!
    return C45_Chopper::chop( InBuf, len );
  }

  bool Bin_Chopper::chop( const UnicodeString& InBuf, size_t len ) {
    // Lines look like this:
    // 12, 25, 333, bla.
    // the termination dot is optional
    init( InBuf, len, true );
    for ( size_t m = 0; m < vSize-1; ++m ){
      choppedInput[m] = "0";
    }
    vector<UnicodeString> parts = TiCC::split_exact_at( strippedInput, "," );
    for ( auto const& p : parts ){
      if ( &p == &parts.back() ){
	choppedInput[vSize-1] = p;
	break;
      }
      size_t k;
      if ( !TiCC::stringTo<size_t>( p, k ) ){
	return false;
      }
      if ( k < 1 || k > vSize ){
	return false;
      }
      else {
	choppedInput[k-1] = "1";
      }
    }
    return true;
  }

  UnicodeString Bin_Chopper::getString() const {
    UnicodeString res;
    int i = 1;
    for ( const auto& chop : choppedInput ){
      if ( &chop == &choppedInput.back() ){
	break;
      }
      if ( chop[0] == '1' ){
	res += TiCC::toUnicodeString(i) + ",";
      }
      ++i;
    }
    res += choppedInput.back() + ",";
    return res;
  }

  bool Compact_Chopper::chop( const UnicodeString& InBuf, size_t leng ){
    init( InBuf, leng, false );
    // Lines look like this:
    // ====AKBVAK
    // v1v2v3v4tt
    // Get & add the target.
    //
    size_t len = strippedInput.length();
    if ( len != vSize * fLen ){
      return false;
    }
    size_t i = 0;
    for ( auto& chop : choppedInput ){
      size_t index = i * fLen;
      // Scan the value.
      //
      chop.remove();
      for ( int j = 0; j < fLen; ++j ) {
	chop += strippedInput[index++];
      }
      ++i;
    }
    return ( i == vSize ); // Enough?
  }

  UnicodeString Compact_Chopper::getString() const {
    UnicodeString res;
    for ( const auto& chop : choppedInput ){
      res += CodeToStr( chop );
    }
    return res;
  }

  bool Columns_Chopper::chop( const UnicodeString& InBuf, size_t len ){
    // Lines look like this:
    // one  two three bla
    init( InBuf, len, false );
    vector<UnicodeString> splits = TiCC::split( strippedInput );
    size_t res = splits.size();
    if ( res != vSize ){
      return false;
    }
    for ( size_t i=0; i < res ; ++i ){
      choppedInput[i] = StrToCode( splits[i] );
    }
    return ( res == vSize ); // Enough?
  }

  UnicodeString Columns_Chopper::getString() const {
    UnicodeString res = TiCC::join( choppedInput );
    return res;
  }



  bool Tabbed_Chopper::chop( const UnicodeString& InBuf, size_t len ){
    // Lines look like this:
    // oneTABtwoTAB TABthreeTABbla
    init( InBuf, len, false );
    vector<UnicodeString> splits = TiCC::split_at( strippedInput, "\t" );
    size_t res = splits.size();
    if ( res != vSize ){
      return false;
    }
    for ( size_t i=0; i < res ; ++i ){
      choppedInput[i] = StrToCode( splits[i], false );
    }
    return ( res == vSize ); // Enough?
  }

  UnicodeString Tabbed_Chopper::getString() const {
    UnicodeString res;
    for ( const auto& chop : choppedInput ){
      res += CodeToStr( chop ) + "\t";
    }
    return res;
  }

  bool Sparse_Chopper::chop( const UnicodeString& InBuf, size_t len ){
    // Lines look like this:
    // (12,value1) (25,value2) (333,value3) bla.
    // the termination dot is optional
    init( InBuf, len, true );
    for ( size_t m = 0; m < vSize-1; ++m ){
      choppedInput[m] = DefaultSparseString;
    }
    choppedInput[vSize-1] = "";
    vector<UnicodeString> entries = TiCC::split_at_first_of( strippedInput,
							     "()" );
    size_t num_ent = entries.size();
    if ( num_ent < 1 ){
      return false;
    }
    for ( const auto& ent : entries ){
      --num_ent;
      vector<UnicodeString> parts = TiCC::split_at( ent, "," );
      size_t num = parts.size();
      if ( num != 2 ){
	if ( num == 1 && num_ent == 0 ){
	  // the target has no ','
	  parts[0].trim();
	  choppedInput[vSize-1] = parts[0];
	  return !choppedInput[vSize-1].isEmpty();
	}
	return false;
      }
      if ( num_ent == 0 ){
	// missing a target!
	return false;
      }
      size_t index;
      if ( !TiCC::stringTo( parts[0], index ) ){
	return false;
      }
      if ( index < 1 || index >= vSize ){
	return false;
      }
      choppedInput[index-1] = StrToCode( parts[1] );
    }
    return true;
  }

  UnicodeString Sparse_Chopper::getString() const {
    UnicodeString res;
    int i = 1;
    for ( const auto& chop : choppedInput ){
      if ( &chop == &choppedInput.back() ){
	break;
      }
      if ( chop != DefaultSparseString ){
	res += "(" + TiCC::toUnicodeString( i ) + ",";
	res += CodeToStr(chop);
	res += ")";
      }
      ++i;
    }
    res += choppedInput.back() + ",";
    return res;
  }

}
