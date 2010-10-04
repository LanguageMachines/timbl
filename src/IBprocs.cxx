/*
  Copyright (c) 1998 - 2010
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
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <cassert>

#include "timbl/Tree.h"
#include "timbl/MsgClass.h"
#include "timbl/IBtree.h"
#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"
#include "timbl/NewIBtree.h"
#include "timbl/Options.h"
#include "timbl/BestArray.h"
#include "timbl/MBLClass.h"
#include "timbl/Metrics.h"

using namespace std;

namespace Timbl {

  bool MBLClass::HideInstance( const Instance& Inst ){
    bool result = true;
    if ( NewIB ){
      NewIB->deleteInstance( Inst );
    }
    else {
      InstanceBase->RemoveInstance( Inst );
    }
    MBL_init = do_sloppy_loo; // must be only true if you are REALY sure
    for ( size_t i=0; i < effective_feats && result; ++i ){
      PermFeatures[i]->clear_matrix();
      if ( !PermFeatures[i]->decrement_value( Inst.FV[i], 
					      Inst.TV ) ){
	FatalError( "Unable to Hide an Instance!" );
	result = false;
      }
    }
    if ( result )
      Targets->decrement_value( Inst.TV );
    return result;
  }
  
  bool MBLClass::UnHideInstance( const Instance& Inst ){
    bool result = true;
    if ( NewIB ){
      NewIB->addInstance( Inst );
    }
    else {
      InstanceBase->AddInstance( Inst );
    }
    MBL_init = do_sloppy_loo; // must be only true if you are REALY sure
    for ( size_t i=0; i < effective_feats && result; ++i ){
      PermFeatures[i]->clear_matrix();
      if ( !PermFeatures[i]->increment_value( Inst.FV[i], 
					      Inst.TV ) ){
	FatalError( "Unable to UnHide this Instance!" );
	result = false;
      }
    }
    if ( result )
      Targets->increment_value( Inst.TV );
    return result;
  }
  
  MBLClass::IB_Stat MBLClass::IBStatus() const {
    if ( NewIB ){
      if ( NewIB->isPruned() )
	return Pruned;
      else
	return Normal;
    }
    else if (!InstanceBase )
      return Invalid;
    else if (InstanceBase->IsPruned() )
      return Pruned;
    else
      return Normal;
  }
  
  void MBLClass::IBInfo( ostream& os ) const {
    double Compres;
    unsigned long int CurSize;
    unsigned long int CurBytes;
    if ( NewIB )
      CurBytes = NewIB->getSizeInfo( CurSize, Compres );
    else
      CurBytes = InstanceBase->GetSizeInfo( CurSize, Compres );
    ios::fmtflags OldFlg = os.setf( ios::fixed, ios::floatfield );
    int OldPrec = os.precision(2);
    os << "\nSize of InstanceBase = " << CurSize << " Nodes, (" << CurBytes
       << " bytes), " << Compres << " % compression" << endl;
    if ( !NewIB && Verbosity(BRANCHING) ) {
      vector<unsigned int> terminals;
      vector<unsigned int> nonTerminals;
      InstanceBase->summarizeNodes( terminals, nonTerminals );
      os << "branching info:" << endl;
      unsigned int i = 0;
      vector<unsigned int>::const_iterator nIt = nonTerminals.begin();
      vector<unsigned int>::const_iterator tIt = terminals.begin();
      unsigned int summedNodes = 0;
      unsigned int endNodes = 0;
      os << "   level | feature |     nodes |  nonterms | terminals |  b-factor | b-factor-n" << endl;
      while ( nIt != nonTerminals.end() ){
	endNodes += *tIt;
	int nodes;
	if ( i == 0 ){
	  nodes = 1;
	  os << setw(8) << 0 << " |" << setw(8) << "top" << " |" 
	     << setw(10) << 1 << " |"
	     << setw(10) << 1 << " |" << setw(10) << 0 << " |"
	     << setw(10) << double(*nIt + *tIt) << " |"
	     << setw(10) << double(*nIt + *tIt) << endl;
	}
	else {
	  nodes = *(nIt-1) + *(tIt-1);
	  if ( nodes == 0 )
	    break;
	  os << setw(8) << i << " |"<< setw(8) << permutation[i-1] + 1 << " |"
	     << setw(10) << nodes << " |"
	     << setw(10) << *(nIt-1) << " |" << setw(10) << *(tIt-1) << " |"
	     << setw(10) << (*nIt + *tIt)/double(nodes) << " |"
	     << setw(10) << (*nIt?(*nIt)/double(*(nIt-1)):0) << endl;
	}
 	summedNodes += nodes;
	++i;
	++nIt;
	++tIt;
      }
      os << "total: nodes = " << summedNodes 
	 << " endnodes = " << endNodes 
	 << " factor = " << summedNodes/double(endNodes) << endl;
    }
    os.precision( OldPrec );
    os.setf( OldFlg );
  }

  bool MBLClass::get_IB_Info( istream& is,
			      bool& Pruned,
			      int& Version,
			      bool& Hashed,
			      string& range_buf ){
    if ( ExpInvalid() ){
      Error( "Can't retrieve Instance-Base\n" );
      return false;
    }
    if ( Options.TableFrozen() ||
	 num_of_features != 0 ){
      Warning( "unable to read an Instance Base while another"
	       " experiment is already loaded" );
      return false;
    }
    
    bool info_ok = true;
    bool more = true;
    size_t depth = 0;
    int version = -1;
    Hashed = false;
    range_buf = "";
    string buffer;
    vector<string> splits;
    more = ( look_ahead(is) == '#' &&
	     getline( is, buffer ) );
    while ( info_ok && more ){
      size_t num = split( buffer, splits );
      if ( num > 2 ){
	if ( compare_nocase_n( "Status:", splits[1] ) ){
	  version = 2;
	  if ( splits[2] == "pruned" )
	    Pruned = true;
	  else if ( splits[2] == "complete" )
	    Pruned = false;
	  else {
	    Error( "Unknown Status Information in Instance-Base file." );
	    info_ok = false;
	  }
	}
	else if ( compare_nocase_n( "Algorithm:", splits[1] ) ) {
	  version = 1;
	  if ( compare_nocase( splits[2], "IG-tree" ) )
	    Pruned = true;
	  else if ( compare_nocase( splits[2], "MBL" ) )
	    Pruned = false;
	  else {
	    Error( "Unknown Algorithm Information in Instance-Base file." );
	    info_ok = false;
	  }
	}
	else if ( compare_nocase_n( "Permutation:", splits[1] ) ){
	  if ( splits[2][0] != '<' ){
	    Error( "missing `<` while reading permutation" );
	    info_ok = false;
	  }
	  else if ( splits[num-1][0] != '>' ){
	    Error( "missing `>` while reading permutation" );
	    info_ok = false;
	  }
	  else {
	    string perms;
	    for ( size_t i=3; i < num-1; ++i ){
	      perms = perms + splits[i]; // Maybe we could use splits directly?
	    }
	    bool excl = false;
	    effective_feats = 0;
	    size_t i = 0;
	    size_t index;
	    string::size_type pos = 0; // skip <
	    while ( info_ok && pos != string::npos &&
		    i < MaxFeatures ){
	      i++;
	      if ( !excl )
		effective_feats++;
	      string tmp = string_tok( perms, pos, ", !" );
	      index = stringTo<size_t>( tmp );
	      permutation.push_back( --index );
	      if ( index >= MaxFeatures ){
		Error ( "illegal value " + toString<size_t>(index) + 
			" in permutation, not between 1 and " +
			toString<size_t>( MaxFeatures ) );
		info_ok = false;
		break;
	      }
	      if ( excl )
		UserOptions[index+1] = Ignore;
	      if ( pos == string::npos ){
		break;
	      }
	      while ( isspace(perms[pos]) ) ++pos;
	      switch ( perms[pos] ){
	      case ',':
		++pos;
		break;
	      case '!':
		++pos;
		excl = true;
		break;
	      default:
		Error ( "missing `,` while reading permutation" );
		info_ok = false;
	      }
	    }
	    if ( info_ok )
	      depth = i;
	  }
	}
	else if ( compare_nocase_n( "Numeric:", splits[1] ) ){
	  if ( splits[2][0] != '.' ){
	    string::size_type pos = 0;
	    while ( pos != string::npos ){
	      string tmp = string_tok( splits[2], pos, ",. " );
	      if ( tmp != "" ){
		int k = stringTo<int>( tmp );
		UserOptions[k] = Numeric;
	      }
	    }
	    getline( is, range_buf );
	  }
	}
	else if ( compare_nocase_n( "Bin_Size:", splits[1] ) ){
	  int siz = stringTo<int>( splits[2] );
	  if ( siz < 2 || siz > 1000000 ){
	    Error( "invalid Bin_Size found: " + splits[2] );
	    info_ok = false;
	  }
	  else
	    Bin_Size = siz;
	}
	else if ( compare_nocase_n( "Version", splits[1] ) ){
	  version = stringTo<int>( splits[2] );
	  if ( version >= 3  && num > 3 ){
	    if ( compare_nocase_n( "(Hashed)", splits[3] ) )
	      Hashed = true;
	  }
	}
      }
      more = ( look_ahead(is) == '#' && 
	       getline( is, buffer ) );
    }
    if ( version < 0 ) {
      Error( "missing Version information in Instance-Base file" );
      info_ok = false;
    }
    else if ( version < 4 ) {
      Error( "A Version " + toString<int>(version) + 
	     " type InstanceBase file is found:\n"
	     "        You should recreate it as it is no longer supported"
	     "\n        in this version of the Timbl package" );
    }
    Version = version;
    if ( info_ok ){
      num_of_features = depth;
      return true;
    }
    else {
      num_of_features = 0;
      Error( "Can't retrieve Instance-Base\n" );
      return false;
    }
  }
  
  bool MBLClass::get_ranges( const string& rangeline ){
    if ( NumNumFeatures() == 0 )
      return true;
    istringstream is( rangeline );
    string buf;
    char kar;
    bool result = false;
    is >> kar; // skip #
    is >> ws >> buf;
    if ( !compare_nocase_n( "Ranges:", buf ) )
      Error( "missing Ranges line in Instance-Base file" );
    else {
      is >> ws;
      int k;
      if ( look_ahead(is) == '.' ){
	result = true;
      }
      else {
	do {
	  is >> k; 
	  if ( UserOptions[k] != Numeric ){
	    Error( "Found range info for feature " + toString<int>(k) +
		   ", which is Not defined as Numeric!" );
	    result = false;
	  }
	  else {
	    is >> ws >> buf;
	    double min, max;
	    int scancount = sscanf( buf.c_str(), "[%lf-%lf]", &min, &max );
	    if ( scancount == 2 ){
	      Features[k-1]->Min( min );
	      Features[k-1]->Max( max );
	      if ( is ){
		is >> ws >> buf;
		if ( !buf.empty() && (buf[0] == '.' || buf[0] == ',' ) )
		  result = true;
		else
		  result = false;
	      }
	      else {
		buf = ".";
		result = true;
	      }
	    }
	    else
	      result = false;
	  }
	} while ( result && buf[0] != '.' );
      }
    }
    return result;
  }
  
  inline void MBLClass::writePermSpecial( ostream &os ) const{
    // write out the permutation and mark the last feature which is
    // NOT to be ignored with an exclamation mark, for instance: 
    // < 5, 2, 3! 1, 4 >
    bool excl = false;
    os << "< ";
    for ( size_t j=0; j < num_of_features-1; ++j ){
      if ( !excl && Features[permutation[j+1]]->Ignore() ){
	excl = true;
	os << permutation[j]+1 << "! ";
      }
      else
	os << permutation[j]+1 << ", ";
    }
    os << permutation[num_of_features-1]+1 << " >" << endl;
  }  

  bool MBLClass::PutInstanceBase( ostream& os ) const {
    bool result = true;
    if ( ExpInvalid() ){
      result = false;
    }
    else if ( NewIB == 0 && InstanceBase == 0 ){
      Warning( "unable to write an Instance Base, nothing learned yet" );
    }
    else if ( NewIB ){
      os << "# Status: " 
	      << (NewIB->isPruned()?"pruned":"complete") << endl;
      os << "# Permutation: "; 
      writePermSpecial( os );
      os << "# Numeric: "; 
      bool first = true;
      for ( size_t i=0; i < num_of_features; ++i )
	if ( !Features[i]->Ignore() &&
	     Features[i]->isNumerical() ){
	  if ( !first )
	    os << ", ";
	  else
	    first = false;
	  os << i+1;
	}
      os << '.' << endl;
      if ( NumNumFeatures() > 0 ){
	os << "# Ranges: "; 
	first = true;
	for ( size_t j=0; j < num_of_features; ++j )
	  if ( !Features[j]->Ignore() &&
	       Features[j]->isNumerical() ){
	    if ( !first )
	      os << " , ";
	    else
	      first = false;
	    os << j+1 << " [" << Features[j]->Min() 
		    << "-" << Features[j]->Max() << "]";
	  }
	os << " ." << endl;
      }
      os << "# Bin_Size: " << Bin_Size << endl;
      if ( hashed_trees ){
	NewIB->saveHashed( os, 
			   TargetStrings, FeatureStrings, 
			   keep_distributions );
      }
      else
	NewIB->save( os, keep_distributions );
    }
    else {
      os << "# Status: " 
	      << (InstanceBase->IsPruned()?"pruned":"complete") << endl;
      os << "# Permutation: "; 
      writePermSpecial( os );
      os << "# Numeric: "; 
      bool first = true;
      for ( size_t i=0; i < num_of_features; ++i )
	if ( !Features[i]->Ignore() &&
	     Features[i]->isNumerical() ){
	  if ( !first )
	    os << ", ";
	  else
	    first = false;
	  os << i+1;
	}
      os << '.' << endl;
      if ( NumNumFeatures() > 0 ){
	os << "# Ranges: "; 
	first = true;
	for ( size_t j=0; j < num_of_features; ++j )
	  if ( !Features[j]->Ignore() &&
	       Features[j]->isNumerical() ){
	    if ( !first )
	      os << " , ";
	    else
	      first = false;
	    os << j+1 << " [" << Features[j]->Min() 
		    << "-" << Features[j]->Max() << "]";
	  }
	os << " ." << endl;
      }
      os << "# Bin_Size: " << Bin_Size << endl;
      if ( hashed_trees ){
	InstanceBase->Save( os,
			    TargetStrings, FeatureStrings, 
			    keep_distributions );
      }
      else
	InstanceBase->Save( os, keep_distributions );
    }
    return result;
  }
  
  
}
