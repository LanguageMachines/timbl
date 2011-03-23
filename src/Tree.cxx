/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2011
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

#include <iostream>
#include <cstdlib>
#include <cstring>

#include "timbl/Common.h"
#include "timbl/Tree.h"

using namespace std;

namespace Hash {
  using namespace Common;
  using namespace Tries;
  
  HashInfo::HashInfo( const string& Tname, unsigned int Indx ):
    name(Tname), ID(Indx){}
  
  HashInfo::~HashInfo(){
  }
  
  ostream& operator<<( ostream& os, const HashInfo& tok ){ 
    os << tok.ID << " " << tok.name;
    return os;
  }

  StringHash::StringHash():
    NumOfTokens(0)
  {}

  StringHash::~StringHash( ){
  }

  unsigned int StringHash::Hash( const string& name ){
    HashInfo *info = StringTree.Retrieve( name );
    if ( !info ){
      info = new HashInfo( name, ++NumOfTokens );
      info = (HashInfo *)StringTree.Store( name, info );
    }
    unsigned int idx = info->Index();
    if ( idx >= rev_index.size() ){
      rev_index.resize( rev_index.size() + 1000 );
    }
    rev_index[idx] = info;
    return info->Index();
  }
  
  unsigned int StringHash::Lookup( const string& name ) const {
    HashInfo *info = StringTree.Retrieve( name );
    if ( info )
      return info->Index(); 
    else
      return 0;
  }
  
  const string& StringHash::ReverseLookup( unsigned int index ) const {
    return rev_index[index]->Name();
  }
  
  ostream& operator << ( ostream& os, const StringHash& S){
    return os << &S.StringTree; }
  
  LexInfo::LexInfo( const string& Tname, const string& Tran ):
    name(Tname),trans(Tran){}
  
  LexInfo::~LexInfo(){}
  
  ostream& operator<<( ostream& os, const LexInfo& LI ){ 
    os << " " << LI.name << " - " << LI.trans;
    return os;
  }
  
  Lexicon::Lexicon(){}
 
  Lexicon::~Lexicon(){}
  
  LexInfo *Lexicon::Lookup( const string& name ) const {
    return (LexInfo *)LexTree.Retrieve( name ); 
  }
  
  LexInfo *Lexicon::Store( const string& name, const string& translation ){
    LexInfo *info = LexTree.Retrieve( name );
    if ( !info ){
      info = new LexInfo( name, translation );
      return LexTree.Store( name, info );
    }
    return info;
  }
  
  ostream& operator<<( ostream& os, const Lexicon& L )
  { return os << &L.LexTree; }
  
} 
