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
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cctype>
#include <ctime>
#include <cstdio>

#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/MsgClass.h"
#include "timbl/Tree.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"
#include "timbl/IBtree.h"

using namespace std;

namespace Timbl {
  using namespace Common;

  IBtree::IBtree():
    FValue(0), TValue(0), TDistribution(0), 
    link(0), next(0)
  { }
  
  IBtree::IBtree( FeatureValue *_fv ):
    FValue(_fv), TValue( 0 ), TDistribution( 0 ), 
    link(0), next(0)
  { }
  
  IBtree::~IBtree(){
    delete TDistribution;
    delete link;
    delete next;
  }
  
  inline IBtree *IBtree::add_feat_val( FeatureValue *FV, 
				       IBtree **tree,
				       unsigned long& cnt ){
    // Add a Featurevalue to the IB. 
    IBtree **pnt = tree;
    while ( *pnt ){
      if ( (*pnt)->FValue == FV ){
	// already there, so bail out.
	return *pnt;
      }
      else if ( (*pnt)->FValue->Index() < FV->Index() ){
	pnt = &((*pnt)->next);
      }
      else {
	// need to add a new node before the current one
	IBtree *tmp = *pnt;
	*pnt = new IBtree( FV );
	++cnt;
	(*pnt)->next = tmp;
	return *pnt;
      }
    }
    // add at the end.
    *pnt = new IBtree( FV );
    ++cnt;
    return *pnt;
  }
  
  static int IBtree_Indent = 0;

  ostream &operator<<( ostream &os, const IBtree& grap ){
    // output an IBtree somewhat orderly.
    const IBtree *pnt = &grap;
    while ( pnt ) {
      if ( pnt->link || pnt->FValue ){
	os << pnt->FValue;
	if ( pnt->TValue )
	  os << "(" << pnt->TValue << ")" ;
	if ( pnt->TDistribution ){
	  os << pnt->TDistribution ;
	}
	if ( pnt->link ){
	  os << "\t";
	  IBtree_Indent++;
	  os << pnt->link;
	  IBtree_Indent--;
	}
	else
	  os << endl;
      }
      else {
	if ( pnt->TValue ){
	  os << "(" << pnt->TValue << ")" ;
	  if ( pnt->link ){
	    os << "\t";
	    IBtree_Indent++;
	    os << pnt->link;
	    IBtree_Indent--;
	  }
	}
	if ( pnt->TDistribution ){
	  os << pnt->TDistribution ;
	}
	os << endl;
      }
      if (pnt->next){
	for ( int j=0; j<IBtree_Indent; ++j )
	  os << "\t";
      }
      pnt = pnt->next;
    }
    return os;
  }

  ostream &operator<<( ostream &os, const IBtree *grap ){
    if ( grap )
      os << *grap;
    else
      os << "null";
    return os;
  }

  ostream& operator<<( ostream &os, const InstanceBase_base& ib ){
    os << "INSTANCE BASE, tree:" << endl;
    os << ib.InstBase << endl;
    return os;
  }
  
  ostream& operator<<( ostream &os, const InstanceBase_base *ib ){
    if ( ib )
      os << *ib;
    else
      os << "empty INSTANCE BASE";
    return os;
  }
  
  unsigned long int InstanceBase_base::GetSizeInfo( unsigned long int& CurSize, 
						    double &Compression ) const {
    unsigned long int MaxSize = (Depth+1) * NumOfTails;
    CurSize = ibCount;
    Compression = 100*(1-(double)CurSize/(double)MaxSize);
    return CurSize * sizeof(IBtree);
  }
  
  void InstanceBase_base::write_tree( ostream &os, const IBtree *pnt ) const {
    // part of saving a tree in a recoverable manner
    os << " (" << pnt->TValue << " ";
    if ( pnt->link ){
      if ( PersistentDistributions && pnt->TDistribution ){
	os << pnt->TDistribution->Save();
      }
      pnt = pnt->link;
      if ( pnt->FValue ){
	os << "[";
	while ( pnt ){
	  os << pnt->FValue << " ";
	  write_tree( os, pnt );
	  pnt = pnt->next;
	  if ( pnt )
	    os << ",";
	}
	os << "]\n";
      }
      else if ( !PersistentDistributions && pnt->TDistribution ){
	os << pnt->TDistribution->Save();
      }
    }
    else if ( pnt->TDistribution ){
      os << pnt->TDistribution->Save();
    }
    os << ")\n";
  }
  
  void InstanceBase_base::write_tree_hashed( ostream &os, const IBtree *pnt ) const {
    // part of saving a tree in a recoverable manner
    os << "(" << pnt->TValue->Index();
    if ( pnt->link ){
      if ( PersistentDistributions && pnt->TDistribution ){
	os << pnt->TDistribution->SaveHashed();
      }
      pnt = pnt->link;
      if ( pnt->FValue ){
	os << "[";
	while ( pnt ){
	  os << pnt->FValue->Index();
	  write_tree_hashed( os, pnt );
	  pnt = pnt->next;
	  if ( pnt )
	    os << ",";
	}
	os << "]\n";
      }
      else if ( pnt->TDistribution && !PersistentDistributions ){
	os << pnt->TDistribution->SaveHashed();
      }
    }
    else if ( pnt->TDistribution ){
      os << pnt->TDistribution->SaveHashed();
    }
    os << ")\n";
  }
  
  const TargetValue *InstanceBase_base::TopTarget( bool &tie ) {
    if ( !DefaultsValid || !DefAss )
      TopT = 0;
    if ( TopT == 0 ){
      TopT = TopDistribution->BestTarget( tiedTop, Random); 
    }
    tie = tiedTop;
    return TopT;
  }

  void InstanceBase_base::Save( ostream &os, bool persist ) {
    // save an IBtree for later use.
    bool temp_persist = PersistentDistributions;
    PersistentDistributions = persist;
    AssignDefaults();
    bool dummy;
    os << "# Version " << Version << "\n#\n(" 
       << TopTarget( dummy ) << " " << TopDistribution->Save();
    IBtree *pnt = InstBase;
    if ( pnt ){
      os << "[";
      while ( pnt ){
	os << pnt->FValue;
	write_tree( os, pnt );
	pnt = pnt->next;
	if ( pnt )
	  os << ",";
      }
      os << "]\n";
    }
    os << ")\n";
    PersistentDistributions = temp_persist;
  }
  
  string to_node( const FeatureValue *fv ){
    return "<feature>" + fv->EncodedName() + "</feature>";
  }

  string to_node( const TargetValue *tv ){
    return "<target>" + tv->EncodedName() + "</target>";
  }

  string to_node( const ValueDistribution *d ){
    return "<distribution>"+d->ToEncodedString()+"</distribution>";
  }
  
  string spaces( int id ){
    string res;
    for ( int j=0; j<id; ++j )
      res += " ";
    return res;
  }

  int count_next( const IBtree *pnt ){
    int cnt = 0;
    const IBtree *tmp = pnt;
    while ( tmp ){
      cnt++;
      tmp = tmp->next;
    }
    return cnt;
  }

  void write_xml( ostream& os, IBtree *pnt, int id ) {
    string ind = spaces(id);
    int cnt = count_next( pnt );
    os << ind << "<nodes nodecount=\"" << cnt << "\">" << endl;
    while ( pnt ){
      os << ind << " <node>" << endl;
      if ( pnt->FValue )
	os << ind << "  " << to_node( pnt->FValue ) << endl;
      if ( pnt->TValue )
	os << ind << "  " << to_node( pnt->TValue ) << endl;
      if ( pnt->link ){
	if ( pnt->link->FValue )
	  write_xml( os, pnt->link, id+3 );
	else if ( pnt->link->TDistribution )
	  os << ind << "  " << to_node( pnt->link->TDistribution ) << endl;
      }
      else if ( pnt->TDistribution )
	os << ind << "  " << to_node( pnt->TDistribution ) << endl;
      os << ind << " </node>" << endl;
      pnt = pnt->next;
    }
    os << ind << "</nodes>" << endl;
  }

  void InstanceBase_base::IBtoXML( ostream &os )  {
    // save an IBtree for later use.
    os << "<?xml version=\"1.0\"  encoding=\"UTF-8\" ?>" << endl;
    os << "<!-- Version " << Version << "\n#\n -->"  << endl;
    bool dummy;
    os << "<root>" << endl;
    os << to_node( TopTarget( dummy ) ) << endl;
    if ( PersistentDistributions )
      os << to_node( TopDistribution ) << endl;
    IBtree *pnt = InstBase;
    int indent = 1;
    write_xml( os, pnt, indent );
    os << "</root>" << endl;
  }  
  
  void save_hash( ostream &os, StringHash *cats, StringHash *feats ){
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
  
  void InstanceBase_base::Save( ostream &os, 
				StringHash *cats, 
				StringHash *feats,
				bool persist ) {
    // save an IBtree for later use.
    bool temp_persist =  PersistentDistributions;
    PersistentDistributions = persist;
    AssignDefaults();
    os << "# Version " << Version << " (Hashed)\n#" << endl;
    save_hash( os , cats, feats );
    bool dummy;
    os << "(" << TopTarget( dummy )->Index() << TopDistribution->SaveHashed();
    IBtree *pnt = InstBase;
    if ( pnt ){
      os << "[";
      while ( pnt ){
	os << pnt->FValue->Index();
	write_tree_hashed( os, pnt );
	pnt = pnt->next;
	if ( pnt )
	  os << ",";
      }
      os << "]\n";
    }
    os << ")\n";
    PersistentDistributions = temp_persist;
  }
  
  IBtree* InstanceBase_base::read_list( istream &is, 
					std::vector<Feature*>& Feats,
					Target  *Targ,
					int level ){
    IBtree *result = NULL, **pnt;
    pnt = &result;
    bool goon = true;
    char delim;
    while ( is && goon ) {
      is >> delim;    // skip the opening `[` or separating ','
      *pnt = read_local( is, Feats, Targ, level );
      if ( !(*pnt) ){
	delete result;
	return NULL;
      }
      pnt = &((*pnt)->next);
      goon = ( look_ahead(is) == ',' );
    }
    is >> delim;    // skip closing `]`
    return result;
  }
  
  IBtree* InstanceBase_base::read_list_hashed( istream &is, 
					       std::vector<Feature*>& Feats,
					       Target  *Targ,
					       int level ){
    IBtree *result = NULL, **pnt;
    pnt = &result;
    bool goon = true;
    char delim;
    while ( is && goon ) {
      is >> delim;    // skip the opening `[` or separating ','
      *pnt = read_local_hashed( is, Feats, Targ, level );
      if ( !(*pnt) ){
	delete result;
	return NULL;
      }
      pnt = &((*pnt)->next);
      goon = ( (look_ahead(is) == ',') );
    }
    is >> delim;    // skip closing `]`
    return result;
  }
  
  IBtree *InstanceBase_base::read_local( istream &is,
					 vector<Feature*>& Feats,
					 Target *Targ,
					 int level ){
    if ( !is ) 
      return NULL;
    IBtree *result = new IBtree();
    ++ibCount;
    string buf;
    char delim;
    is >> ws >> buf;
    result->FValue = Feats[level]->add_value( buf, NULL );
    is >> delim;
    if ( !is || delim != '(' ){
      Error( "missing `(` in Instance Base file" );
      delete result;
      return NULL;
    }
    is >> ws >> buf;
    result->TValue = Targ->Lookup( buf );
    int nxt = look_ahead(is);
    if ( nxt == '{' ){
      try {
	result->TDistribution
	  = ValueDistribution::read_distribution( is, Targ, false );
      }
      catch ( const string what ){
	Warning( what );
	Error( "problems reading a distribution from InstanceBase file" );
	delete result;
	return 0;
      }
      // also we have to update the targetinformation of the featurevalue
      // so we can recalculate the statistics later on.
      if ( result->FValue->ValFreq() > 0 )
	result->FValue->ReconstructDistribution( *(result->TDistribution) );
    }
    if ( look_ahead(is) == '[' ){
      result->link = read_list( is, Feats, Targ, level+1 );
      if ( !(result->link) ){
	delete result;
	return 0;
      }
    }
    else if ( look_ahead(is) == ')' && result->TDistribution ){
      result->link = new IBtree();
      ++ibCount;
      result->link->TValue = result->TValue;
      if ( PersistentDistributions )
	result->link->TDistribution = result->TDistribution->to_VD_Copy();
      else {
	result->link->TDistribution = result->TDistribution;
	result->TDistribution = NULL;
      }
      NumOfTails++;
    }
    is >> delim;
    if ( delim != ')' ){
      Error( "missing `)` in Instance Base file" );
      delete result;
      return NULL;
    }
    return result;
  }
  
  IBtree *InstanceBase_base::read_local_hashed( istream &is,
						vector<Feature*>& Feats,
						Target *Targ,
						int level ){
    if ( !is ) 
      return NULL;
    IBtree *result = new IBtree();
    ++ibCount;
    char delim;
    int index;
    is >> index;
    result->FValue = Feats[level]->add_value( index, NULL );
    is >> delim;
    if ( !is || delim != '(' ){
      Error( "missing `(` in Instance Base file" );
      delete result;
      return NULL;
    }
    is >> index;
    result->TValue = Targ->ReverseLookup( index );
    int nxt = look_ahead(is);
    if ( nxt == '{' ){
      //
      // A distribution is found, must be the last featurevalue
      // (the dummy node is not stored) 
      // OR we have Persistent Distributions
      try {
	result->TDistribution 
	  = ValueDistribution::read_distribution_hashed( is, Targ, false );
      }
      catch ( const string what ){
	Warning( what );
	Error( "problems reading a hashed distribution from InstanceBase file" );
	delete result;
	return 0;
      }
    }
    if ( look_ahead(is) == '[' ){
      result->link = read_list_hashed( is, Feats, Targ, level+1 );
      if ( !(result->link) ){
	delete result;
	return NULL;
      }
    }
    else if ( look_ahead(is) == ')' && result->TDistribution ){
      // 
      // make a dummy node for the targetdistributions just read
      //
      result->link = new IBtree();
      ++ibCount;
      result->link->TValue = result->TValue;
      if ( PersistentDistributions )
	result->link->TDistribution = result->TDistribution->to_VD_Copy();
      else {
	result->link->TDistribution = result->TDistribution;
	result->TDistribution = NULL;
      }
      NumOfTails++;
    }
    is >> delim;
    if ( delim != ')' ){
      Error( "missing `)` in Instance Base file" );
      delete result;
      return NULL;
    }
    return result;
  }
  
  bool InstanceBase_base::ReadIB( istream &is, 
				  vector<Feature *>& Feats, Target *Targs,
				  int expected_version ){
    if ( read_IB( is, Feats, Targs, expected_version ) ){
      InstBase->redo_distributions();
      ValueDistribution *Top 
	= InstBase->sum_distributions( PersistentDistributions );
      delete Top; // still a bit silly but the Top Distribution is known
      // but we need to cleanup behind us also
      return true;
    }
    else
      return false;
  }
  
  void InstanceBase_base::fill_index(){
    IBtree *pnt = InstBase;
    while ( pnt ){
      fast_index[pnt->FValue->Index()] = pnt;
      pnt = pnt->next;
    }
  }

  bool IG_InstanceBase::ReadIB( istream &is, 
				vector<Feature *>& Feats, Target *Targs,
				int expected_version ){
    if ( read_IB( is, Feats, Targs, expected_version ) ){
      if ( PersistentDistributions ){
	ValueDistribution *Top 
	  = InstBase->sum_distributions( PersistentDistributions );
	delete Top; // still a bit silly but the Top Distribution is known
	// but we need to cleanup behind us also
      }
      return true;
    }
    else
      return false;
  }
  
  bool InstanceBase_base::read_IB( istream &is, 
				   vector<Feature *>& Feats, Target *Targs,
				   int expected_version ){
    string buf;
    NumOfTails = 0;
    DefAss = true;  // always for a restored tree
    DefaultsValid = true; // always for a restored tree
    Version = expected_version;
    char delim;
    is >> delim;
    if ( !is || delim != '(' ){
      Error( "missing first `(` in Instance Base file" );
    }
    else {
      // first we get the value of the TopTarget. It's in the file
      // for backward compability
      is >> ws >> buf;
      delete TopDistribution;
      TopDistribution = 0;
      if ( look_ahead(is) == '{' ){
	// Now read the TopDistribution, to get the Targets
	// in the right order in Targ
	try {
	  TopDistribution 
	    = ValueDistribution::read_distribution( is, Targs, true );
	}
	catch ( const string& what ){
	  Warning( what );
	}
      }
      if ( !TopDistribution )
	Error( "problems reading Top Distribution from Instance Base file" );
      else {
	if ( look_ahead( is ) == '[' ){
	  InstBase = read_list( is, Feats, Targs, 0 );
	}
	if ( InstBase ){
	  is >> ws >> buf;
	  if ( buf.empty() || buf[0] != ')' )
	    Error( "missing last `)` in Instance base file, found " + buf );
	}
      }
    }
    return (InstBase != NULL);
  }
  
  bool InstanceBase_base::read_hash( istream &is, 
				     StringHash *cats,
				     StringHash *feats ) const {
    string line;
    is >> ws;
    is >> line;
    if ( !compare_nocase( line, "Classes" ) ){
      Error( "missing 'Classes' keyword in Hashinfo" );
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
      Error( "missing 'Features' keyword in Hashinfo" );
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
  
  bool InstanceBase_base::ReadIB( istream &is, 
				  vector<Feature *>& Feats, Target *Targs,
				  StringHash *cats, StringHash *feats,
				  int expected_version ){
    if ( read_IB( is, Feats, Targs, cats, feats, expected_version ) ){
      InstBase->redo_distributions();
      ValueDistribution *Top 
	= InstBase->sum_distributions( PersistentDistributions );
      delete Top; // still a bit silly but the Top Distribution is known
      // but we need to cleanup behind us also
      return true;
    }
    else
      return false;
  }
  
  bool IG_InstanceBase::ReadIB( istream &is, 
				vector<Feature *>& Feats, Target *Targs,
				StringHash *cats, StringHash *feats,
				int expected_version ){
    if ( read_IB( is, Feats, Targs, cats, feats, expected_version ) ){
      if ( PersistentDistributions ){
	ValueDistribution *Top 
	  = InstBase->sum_distributions( PersistentDistributions );
	delete Top; // still a bit silly but the Top Distribution is known
	// but we need to cleanup behind us also
      }
      return true;
    }
    else
      return false;
  }
  
  bool InstanceBase_base::read_IB( istream &is, 
				   vector<Feature *>& Feats, Target *Targs,
				   StringHash *cats, StringHash *feats,
				   int expected_version ){
    char delim;
    int dum;
    NumOfTails = 0;
    DefAss = true;  // always for a restored tree
    DefaultsValid = true; // always for a restored tree
    Version = expected_version;
    read_hash( is, cats, feats );
    is >> delim;
    if ( !is || delim != '(' ){
      Error( "missing first `(` in Instance Base file" );
    }
    else {
    // first we get the value of the TopTarget. It's in the file
      // for backward compability
      is >> dum;
      delete TopDistribution;
      TopDistribution = 0;
      if ( look_ahead(is) == '{' ){
	// Now read the TopDistribution, to get the Targets
	// in the right order in Targ
	try {
	  TopDistribution 
	    = ValueDistribution::read_distribution_hashed( is, Targs, true );
	}
	catch ( const string& what ){
	  Warning( what );
	}
	if ( !TopDistribution )
	  Error( "problems reading Top Distribution from Instance Base file" );
      }
      else {
	Error( "problems reading Top Distribution from Instance Base file" );
      }
      if ( look_ahead( is ) == '[' ){
	InstBase = read_list_hashed( is, Feats, Targs, 0 );
      }
      if ( InstBase ){
	is >> delim;
	if ( delim != ')' )
	  Error( "missing last `)` in Instance base file, found " + delim );
      }
    }
    return (InstBase != NULL);
  }
  
  bool InstanceBase_base::HasDistributions() const {
    if ( InstBase && InstBase->link )
      return InstBase->link->TDistribution != NULL;
    else
      return false;
  }

  inline ValueDistribution *IBtree::sum_distributions( bool keep ){
    // create a new distribution at this level by summing up the
    // distibutions of all branches.
    ValueDistribution *result;
    if ( !keep ){
      if ( TDistribution )
	if ( FValue ){
	  result = TDistribution;
	  TDistribution = NULL;
	}
	else
	  result = TDistribution->to_VD_Copy();
      else
	result = new ValueDistribution();
      IBtree *pnt = this->next;
      while ( pnt ){
	if ( pnt->TDistribution )
	  result->Merge( *(pnt->TDistribution) );
	if ( FValue ){
	  delete pnt->TDistribution;
	  pnt->TDistribution = NULL;
	}
	pnt = pnt->next;
      }
    }
    else {
      if ( TDistribution )
	result = TDistribution->to_VD_Copy();
      else
	result = new ValueDistribution();
      IBtree *pnt = this->next;
      while ( pnt ){
	if ( pnt->TDistribution ){
	  result->Merge( *(pnt->TDistribution) );
	}
	pnt = pnt->next;
      }
    }
    return result;
  }
  
  void IBtree::assign_defaults( bool Random, bool persist, size_t level ){
    // recursively gather Distribution information up to the top.
    // at each Node we use that info to calculate the Default target.
    // when level > 1 the info might be persistent for IGTREE use
    IBtree *pnt = this;
    bool dummy;
    while ( pnt ){
      if ( pnt->link ){
	if ( !pnt->TDistribution ){
	  pnt->link->assign_defaults( Random, persist, level-1 );
	  pnt->TDistribution = pnt->link->sum_distributions( level > 1
							     && persist );
	}
      }
      pnt->TValue = pnt->TDistribution->BestTarget( dummy, Random );
      pnt = pnt->next;
    }
  }
  
  void IBtree::re_assign_defaults( bool Random, 
				   bool persist ){
    // recursively gather Distribution information up to the top.
    // at each Node we use that info to calculate the Default target.
    IBtree *pnt = this;
    bool dummy;
    while ( pnt ){
      if ( pnt->link ){
	delete pnt->TDistribution;
	pnt->link->re_assign_defaults( Random, persist );
	pnt->TDistribution = pnt->link->sum_distributions( persist );
      }
      pnt->TValue = pnt->TDistribution->BestTarget( dummy, Random );
      pnt = pnt->next;
    }
  }
  
  void IBtree::redo_distributions(){
    // recursively gather Distribution information up to the top.
    // removing old info...
    // at each node we also Reconstruct Feature distributions
    // we keep the Target value that was given!
    IBtree *pnt = this;
    while ( pnt ){
      if ( pnt->link ){
	pnt->link->redo_distributions();
	delete pnt->TDistribution;
	pnt->TDistribution = pnt->link->sum_distributions( false );
	if ( pnt->FValue->ValFreq() > 0 )
	  pnt->FValue->ReconstructDistribution( *(pnt->TDistribution) );
      }
      pnt = pnt->next;
    }
  }

  inline IBtree *IBtree::make_unique( const TargetValue *Top, 
				      unsigned long& cnt ){
    // remove branches with the same target as the Top, except when they
    // still have a subbranch, which means that they are an exception.
    IBtree **tmp, *dead, *result;
    result = this;
    tmp = &result;
    while ( *tmp ){
      if ( (*tmp)->TValue == Top && (*tmp)->link == NULL ){
	dead = *tmp;
	*tmp = (*tmp)->next;
	dead->next=NULL;
	--cnt;
	delete dead;
      }
      else
	tmp = &((*tmp)->next);
    }
    return result;
  }
  
  inline IBtree *IBtree::Reduce( const TargetValue *Top,
				 unsigned long& cnt,
				 long depth ){
    // recursively cut default nodes, (with make unique,) starting at the
    // leaves of the Tree and moving back to the top.
    IBtree *pnt = this;
    while ( pnt ){
      if ( pnt->link != NULL )
	pnt->link = pnt->link->Reduce( pnt->TValue, cnt, depth-1 );
      pnt = pnt->next;
    }
    if ( depth <= 0 )
      return make_unique( Top, cnt );
    else
      return this;
  }
  
  const ValueDistribution *IBtree::exact_match( const Instance& Inst ) const {
    // Is there an exact match between the Instance and the IB
    // If so, return the best Distribution.
    const IBtree *pnt = this;
    int pos = 0;
    while ( pnt ){
      if ( pnt->link == NULL ){
	if ( pnt->TDistribution->ZeroDist() )
	  return NULL;
	else
	  return pnt->TDistribution;
      }
      else if ( pnt->FValue == Inst.FV[pos] ){
	if ( pnt->FValue->ValFreq() == 0 )
	  return NULL;
	else {
	  pnt = pnt->link;
	  pos++;
	}
      }
      else
	pnt = pnt->next;
    }
    return NULL;
  }
  
  InstanceBase_base::InstanceBase_base( size_t depth, 
					unsigned long int&cnt,
					bool Rand, 
					bool persist ):
    DefAss( false ),
    DefaultsValid( false ),
    Random( Rand ),
    PersistentDistributions( persist ),
    Version( 4 ),
    TopDistribution( new ValueDistribution ),
    WTop( 0 ),
    TopT( 0 ),
    InstBase( NULL ),
    RestartSearch( new const IBtree *[depth] ),
    SkipSearch( new const IBtree *[depth] ),
    InstPath( new const IBtree *[depth] ),
    ibCount( cnt ),
    Depth( depth ),
    NumOfTails( 0 )
    {}
  
  InstanceBase_base::~InstanceBase_base(){
    if ( InstPath ){
      delete [] InstPath;
    }
    if ( SkipSearch ){
      delete [] SkipSearch;
    }
    if ( RestartSearch ){
      delete [] RestartSearch;
    }
    delete InstBase;
    delete TopDistribution;
    delete WTop;
  }
  
  IB_InstanceBase *IB_InstanceBase::Clone() const {
    IB_InstanceBase *result =
      new IB_InstanceBase( Depth, ibCount, Random );
    result->DefAss = DefAss;
    result->DefaultsValid = DefaultsValid;
    result->NumOfTails = NumOfTails; // only usefull for Server???
    result->InstBase = InstBase;
    delete result->TopDistribution;
    result->TopDistribution = TopDistribution;
    return result;
  }
  
  IG_InstanceBase *IG_InstanceBase::Clone() const {
    IG_InstanceBase *result =
      new IG_InstanceBase( Depth, ibCount, 
			   Random, Pruned, PersistentDistributions );
    result->Pruned = Pruned;
    result->DefAss = DefAss;
    result->DefaultsValid = DefaultsValid;
    result->NumOfTails = NumOfTails; // only usefull for Server???
    result->InstBase = InstBase;
    delete result->TopDistribution;
    result->TopDistribution = TopDistribution;
    return result;
  }
  
  void IBtree::countBranches( unsigned int l, 
			      std::vector<unsigned int>& terminals,
			      std::vector<unsigned int>& nonTerminals ){
    if ( link && link->FValue != 0 ){
      ++nonTerminals[l];
      link->countBranches( l+1, terminals, nonTerminals );
    }
    else {
      ++terminals[l];
    }
    if ( next ){
      next->countBranches( l, terminals, nonTerminals );
    }
  }
  
  void InstanceBase_base::summarizeNodes( std::vector<unsigned int>& terminals,
					  std::vector<unsigned int>& nonTerminals ){
    terminals.clear();
    nonTerminals.clear();
    terminals.resize( Depth+1, 0 );
    nonTerminals.resize( Depth+1, 0 );
    if ( InstBase ){
      InstBase->countBranches( 0, terminals, nonTerminals );
    }
  }

  TRIBL_InstanceBase *TRIBL_InstanceBase::Clone() const {
    TRIBL_InstanceBase *result =
      new TRIBL_InstanceBase( Depth, ibCount, 
			      Random, PersistentDistributions );
    result->Treshold = Treshold;
    result->DefAss = DefAss;
    result->DefaultsValid = DefaultsValid;
    result->NumOfTails = NumOfTails; // only usefull for Server???
    result->InstBase = InstBase;
    delete result->TopDistribution;
    result->TopDistribution = TopDistribution;
    return result;
  }
  
  TRIBL2_InstanceBase *TRIBL2_InstanceBase::Clone() const {
    TRIBL2_InstanceBase *result =
      new TRIBL2_InstanceBase( Depth, ibCount,
			       Random, PersistentDistributions );
    result->DefAss = DefAss;
    result->DefaultsValid = DefaultsValid;
    result->NumOfTails = NumOfTails; // only usefull for Server???
    result->InstBase = InstBase;
    delete result->TopDistribution;
    result->TopDistribution = TopDistribution;
    return result;
  }
  
  IB_InstanceBase* TRIBL_InstanceBase::IBPartition( IBtree *sub ) const {
    int i=0;
    IBtree *tmp = sub;
    while ( tmp && tmp->link ){
      i++;
      tmp = tmp->link;
    }
    IB_InstanceBase *result =
      new IB_InstanceBase( i, ibCount, Random );
    result->DefAss = DefAss;
    result->DefaultsValid = DefaultsValid;
    result->NumOfTails = NumOfTails; // only usefull for Server???
    result->InstBase = sub;
    if ( sub ){
      delete result->TopDistribution;
      result->TopDistribution =
	sub->sum_distributions( false  );
    }
    return result;
  }

  IB_InstanceBase* TRIBL2_InstanceBase::IBPartition( IBtree *sub ) const {
    int i=0;
    IBtree *tmp = sub;
    while ( tmp && tmp->link ){
      i++;
      tmp = tmp->link;
    }
    IB_InstanceBase *result =
      new IB_InstanceBase( i, ibCount, Random );
    result->DefAss = DefAss;
    result->DefaultsValid = DefaultsValid;
    result->NumOfTails = NumOfTails; // only usefull for Server???
    result->InstBase = sub;
    if ( sub ){
      delete result->TopDistribution;
      result->TopDistribution =
	sub->sum_distributions( false  );
    }
    return result;
  }

  void InstanceBase_base::CleanPartition(){
    InstBase = 0; // prevent deletion of InstBase in next step!
    TopDistribution = 0;// same for TopDistribution
    delete this;
  }
  
  void InstanceBase_base::AssignDefaults(){
    if ( !DefaultsValid ){
      if ( !DefAss ){
	InstBase->assign_defaults( Random,
				   PersistentDistributions,
				   Depth );
      }
      else {
	InstBase->re_assign_defaults( Random, PersistentDistributions );
      }
      ValueDistribution *Top 
	= InstBase->sum_distributions( PersistentDistributions );
      delete Top; // still a bit silly but the Top Distribution is known
    }
    DefAss = true;
    DefaultsValid = true;
  }

  void TRIBL_InstanceBase::AssignDefaults( size_t treshold ){
    if ( Treshold != treshold ){
      Treshold = treshold;
      DefaultsValid = false;
    }
    if ( !DefaultsValid ){
      InstBase->assign_defaults( Random, PersistentDistributions, Treshold );
    }
    DefAss = true;
    DefaultsValid = true;
  }

  void InstanceBase_base::Prune( const TargetValue *, long ){
    FatalError( "You Cannot Prune this kind of tree! " );
  }
  
  void IG_InstanceBase::Prune( const TargetValue *top, long depth ){
    AssignDefaults( );
    if ( !Pruned ) {
      InstBase = InstBase->Reduce( top, ibCount, depth );
      Pruned = true;
    }
  }
  
  void IG_InstanceBase::specialPrune( const TargetValue *top ){
    IBtree *pnt = InstBase->link;
    // we have to fix the toptarget here, because the node 
    // is build incremental
    ValueDistribution dist;
    while ( pnt ){
      if ( pnt->TDistribution )
	dist.Merge( *pnt->TDistribution );
      pnt = pnt->next;
    }
    bool dummy;
    InstBase->TValue = dist.BestTarget( dummy, Random );
    InstBase = InstBase->Reduce( top, ibCount, 0 );
    Pruned = true;
  }
  
  bool InstanceBase_base::AddInstance( const Instance& Inst ){
    // add one instance to the IB
    IBtree *hlp, **pnt = &InstBase;
    if ( !InstBase ){
      for ( unsigned int i = 0; i < Depth; ++i ){
	*pnt = new IBtree( Inst.FV[i] );
	++ibCount;
	pnt = &((*pnt)->link);
      }
    }
    else {
      for ( unsigned int i = 0; i < Depth; ++i ){
	hlp = (*pnt)->add_feat_val( Inst.FV[i], pnt, ibCount );
	pnt = &(hlp->link);
      }
    }
    if ( *pnt == NULL ){
      *pnt = new IBtree();
      ++ibCount;
      if ( abs( Inst.ExemplarWeight() ) > Epsilon )
	(*pnt)->TDistribution = new WValueDistribution();
      else
	(*pnt)->TDistribution = new ValueDistribution;
      NumOfTails++;
    }
    bool sw_conflict = false;
    if ( abs( Inst.ExemplarWeight() ) > Epsilon ){
      sw_conflict = (*pnt)->TDistribution->IncFreq( Inst.TV, 
						    Inst.ExemplarWeight() );
    }
    else
      (*pnt)->TDistribution->IncFreq(Inst.TV);
    TopDistribution->IncFreq(Inst.TV);
    DefaultsValid = false;
    return !sw_conflict;
  }


  bool InstanceBase_base::MergeSub( InstanceBase_base * ){
    FatalError( "Merge" );
    return false;
  }  

  void IBtree::cleanDistributions() {
    IBtree *pnt = this;
    while ( pnt ){
      delete pnt->TDistribution;
      pnt->TDistribution = 0;
      if ( pnt->link )
	pnt->link->cleanDistributions();
      pnt = pnt->next;
    }
  }

  bool IG_InstanceBase::MergeSub( InstanceBase_base *ib ){
    if ( ib->InstBase ){
      if ( !PersistentDistributions ){
	ib->InstBase->cleanDistributions();
      }
      IBtree *ibPnt = ib->InstBase;
      while( ibPnt ){
	IBtree *ibPntNext = ibPnt->next;
	ibPnt->next = 0;
	FeatureValue *fv = ibPnt->FValue;
	if ( InstBase ){
	  IBtree **pnt = &InstBase;
	  while ( *pnt && (*pnt)->FValue->Index() < fv->Index() ){
	    pnt = &(*pnt)->next;
	  }
	  if ( *pnt ){
	    if ( (*pnt)->FValue->Index() == fv->Index() ){
	      // this may happen 
	      // snip the link and insert at our link
	      IBtree *snip = ibPnt->link;
	      ibPnt->link = 0;
	      delete ibPnt->TDistribution;
	      ibPnt->TDistribution = 0;
	      --ib->ibCount;
	      delete ibPnt;
	      while ( snip ){
		if ( PersistentDistributions )
		  (*pnt)->TDistribution->Merge( *snip->TDistribution ); 
		else
		  delete snip->TDistribution;
		IBtree **tmp = &(*pnt)->link;
		while ( *tmp && (*tmp)->FValue->Index() < snip->FValue->Index() ){
		  tmp = &(*tmp)->next;
		}
		IBtree *nxt = snip->next;
		snip->next = 0;
		if ( *tmp ){
		  if( (*tmp)->FValue->Index() == snip->FValue->Index() ){
		    return false;
		  }
		  snip->next = *tmp;
		}
		*tmp = snip;
		snip = nxt;
	      }
	    }
	    else {
	      ibPnt->next = *pnt;
	      *pnt = ibPnt;
	    }
	  }
	  else {
	    *pnt = ibPnt;
	  }
	}
	else {
	  InstBase = ibPnt;
	}
	ibPnt = ibPntNext;
      }
    }
    NumOfTails += ib->NumOfTails;
    TopDistribution->Merge( *ib->TopDistribution );
    Pruned = true;
    DefaultsValid = true;
    DefAss = true;
    ib->InstBase = 0;
    return true;
  }

  void InstanceBase_base::RemoveInstance( const Instance& Inst ){
    // remove an instance from the IB
    int pos = 0;
    IBtree *pnt = InstBase;
    while ( pnt ){
      if ( pnt->link == NULL ){
	pnt->TDistribution->DecFreq(Inst.TV);
	TopDistribution->DecFreq(Inst.TV);
	break;
      }
      else {
	if ( pnt->FValue == Inst.FV[pos] ){
	  pnt = pnt->link;
	  pos++;
	}
	else
	  pnt = pnt->next;
      }
    }
    DefaultsValid = false;
  }
  
  const ValueDistribution *InstanceBase_base::InitGraphTest( vector<FeatureValue *>&,
							     const vector<FeatureValue *>&,
							     size_t,
							     size_t ){
    FatalError( "InitGraphTest" );
    return 0;
  }

  const IBtree *IBtree::search_node( FeatureValue *fv ) const {
    const IBtree *pnt = 0;
    if ( fv ){
      pnt = this;
      while ( pnt ){
	if ( pnt->FValue == fv )
	  break;
	pnt = pnt->next;
      }
    }
    return pnt;
  }

  const IBtree *InstanceBase_base::fast_search_node( FeatureValue *fv ) {
    const IBtree *result = 0;
    if ( fast_index.empty() )
      fill_index();
    if ( fv ){
      FI_map::const_iterator It = fast_index.find( fv->Index() );
      if ( It != fast_index.end() )
	result = It->second;
    }
    return result;
  }

  const ValueDistribution *IB_InstanceBase::InitGraphTest( vector<FeatureValue *>& Path,
							   const vector<FeatureValue *>& Templ,
							   size_t off,
							   size_t eff ){
    const IBtree *pnt;
    const ValueDistribution *result = NULL;
    pnt = InstBase;
    for ( unsigned int i = 0; i < Depth; ++i ){
      InstPath[i] = pnt;
      RestartSearch[i] = pnt;
      if ( i == 0 )
	pnt = fast_search_node( Templ[off+i] );
      else
	pnt = pnt->search_node( Templ[off+i] );
      if ( pnt ){ // found an exact match, so mark restart position
	if ( RestartSearch[i] == pnt )
	  RestartSearch[i] = pnt->next;
	SkipSearch[i] = pnt;
	InstPath[i] = pnt;
      }
      else { // no exact match at this level. Just start with the first....
	RestartSearch[i] = NULL;
	SkipSearch[i] = NULL;
	pnt = InstPath[i];
      }
      Path[i] = pnt->FValue;
      pnt = pnt->link;
      if ( pnt && pnt->link == NULL ){
	result = pnt->TDistribution;
	break;
      }
    }
    while ( result && result->ZeroDist() ){
      // This might happen when doing LOO or CV tests
      size_t TmpPos = eff-1;
      result = NextGraphTest( Path, Templ, off, eff, TmpPos );
    }
    return result;
  }
  
  const ValueDistribution *InstanceBase_base::NextGraphTest( vector<FeatureValue *>&, 
							     const vector<FeatureValue *>&,
							     size_t,
							     size_t,
							     size_t& ){
    FatalError( "NextGraphTest" );
    return 0;
  }
  
  const ValueDistribution *IB_InstanceBase::NextGraphTest( vector<FeatureValue *>& Path, 
							   const vector<FeatureValue *>& Templ, 
							   size_t off,
							   size_t eff,
							   size_t& pos ){
    const IBtree *pnt = NULL;
    const ValueDistribution *result = NULL;
    bool goon = true;
    while ( !pnt && goon ){
      if ( RestartSearch[pos] == NULL ) {
	// No exact match here, so no real problems
	pnt = InstPath[pos]->next;
      }
      else {
	pnt = RestartSearch[pos];
	RestartSearch[pos] = NULL;
      }
      if ( pnt && pnt == SkipSearch[pos] ){
	pnt = pnt->next;
      }
      if ( !pnt ) {
	if ( pos == 0 )
	  goon = false;
	else
	  pos--;
      }
    }
    if ( pnt && goon ) {
      InstPath[pos] = pnt;
      Path[pos] = pnt->FValue;
      pnt = pnt->link;
      for (  size_t j=pos+1; j < Depth; ++j ){
	const IBtree *tmp = pnt->search_node( Templ[off+j] );
	if ( tmp ){ // we found an exact match, so mark Restart position
	  if ( pnt == tmp )
	    RestartSearch[j] = pnt->next;
	  else
	    RestartSearch[j] = pnt;
	  SkipSearch[j] = tmp;
	  InstPath[j] = tmp;
	  Path[j] = tmp->FValue;
	  pnt = tmp->link;
	}
	else { // no exact match at this level. Just start with the first....
	  RestartSearch[j] = NULL;
	  SkipSearch[j] = NULL;
	  InstPath[j] = pnt;
	  Path[j] = pnt->FValue;
	  pnt = pnt->link;
	}
      }
      if ( pnt )
	result = pnt->TDistribution;
    }
    if ( result && result->ZeroDist() ){
      // This might happen when doing LOO or CV tests
      size_t TmpPos = eff-1;
      result = NextGraphTest( Path, Templ,
			      off, eff, TmpPos );
      if ( TmpPos < pos ){
	pos = TmpPos;
      }
    }
    return result;
  }
  
  const ValueDistribution *InstanceBase_base::IG_test( const Instance& , 
						       int &,
						       bool &,
						       const TargetValue *& ){
    FatalError( "IG_test " );
    return NULL;
  }

  const ValueDistribution *IG_InstanceBase::IG_test( const Instance& Inst, 
						     int &end_level,
						     bool &leaf,
						     const TargetValue *&result ) {
    // The Test function for the IG algorithm, returns a pointer to the
    // distribution of the last matching position in the Tree, it's position
    // in the Instance Base and the default TargetValue
    const IBtree *pnt = InstBase;
    result = NULL;
    ValueDistribution *Dist = NULL;
    int pos = 0;
    leaf = false;
    pnt = fast_search_node( Inst.FV[pos] );
    while ( pnt ){
      result = pnt->TValue;
      if ( PersistentDistributions )
	Dist = pnt->TDistribution;
      pnt = pnt->link;
      if ( pnt && !pnt->FValue ) 
	pnt = NULL;
      leaf = (pnt == NULL);
      ++pos;
      if ( pnt )
	pnt = pnt->search_node( Inst.FV[pos] );
    }
    end_level = pos;
    if ( end_level == 0 ){
      if ( !WTop && TopDistribution )
	WTop = TopDistribution->to_WVD_Copy();
      Dist = WTop;
    }
    return Dist;
  }

  IB_InstanceBase *InstanceBase_base::TRIBL_test( const Instance& , 
						  size_t,
						  const TargetValue *&,
						  const ValueDistribution *&,
						  size_t & ){
    FatalError( "TRIBL_test " );
    return NULL;
  }

  IB_InstanceBase *InstanceBase_base::TRIBL2_test( const Instance& , 
						   const ValueDistribution *&,
						   size_t & ){
    FatalError( "TRIBL2_test " );
    return NULL;
  }

  IB_InstanceBase *TRIBL_InstanceBase::TRIBL_test( const Instance& Inst, 
						   size_t treshold,
						   const TargetValue *&TV,
						   const ValueDistribution *&dist,
						   size_t &level ) {
    // The Test function for the TRIBL algorithm, returns a pointer to the
    // Target at the last matching position in the Tree, 
    // or the subtree Instance Base necessary for IB1
    IBtree *pnt = InstBase;
    AssignDefaults( treshold );
    TV = NULL;
    dist = NULL;
    IB_InstanceBase *subt = NULL;
    size_t pos = 0;
    while ( pnt && pos < treshold ){
      if ( pnt->FValue == Inst.FV[pos] ){
	dist = pnt->TDistribution;
	TV = pnt->TValue;
	pnt = pnt->link;
	if ( pnt && !pnt->FValue ){
	  dist = pnt->TDistribution;
	  pnt = NULL;
	}
	pos++;
      }
      else
	pnt = pnt->next;
    }
    if ( pos == treshold ){
      if ( pnt ){
	subt = IBPartition( pnt );
	dist = NULL;
      }
      else {
	level = pos;
      }
    }      
    else {
      if ( pos == 0 && dist == NULL ){
	if ( !WTop && TopDistribution )
	  WTop = TopDistribution->to_WVD_Copy();
	dist = WTop;
	bool dummy;
	TV = TopTarget( dummy );
      }
      else
	level = pos;
    }
    return subt;
  }
  
  IB_InstanceBase *TRIBL2_InstanceBase::TRIBL2_test( const Instance& Inst, 
						     const ValueDistribution *& dist,
						     size_t &level ){
    // The Test function for the TRIBL2 algorithm, returns a pointer to the
    // the subtree Instance Base necessary for IB1
    IBtree *pnt = InstBase;
    dist = NULL;
    AssignDefaults();
    int pos = 0;
    IB_InstanceBase *subtree = NULL;
    IBtree *last_match = pnt;
    while ( pnt ){
      if ( pnt->FValue == Inst.FV[pos] ){
	// a match, go deeper
	pnt = pnt->link;
	last_match = pnt;
	pos++;
	if ( pnt && !pnt->FValue ){
	  // at the end, an exact match
	  dist = pnt->TDistribution;
	  last_match = NULL;
	  break;
	}
      }
      else
	pnt = pnt->next;
    }
    if ( last_match ){
      subtree = IBPartition( last_match );
      level = pos;
    }
    return subtree;
  }
  
} // namespace Timbl
