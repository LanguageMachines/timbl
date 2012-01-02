/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2012
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

#include <cassert>
#include <string>
#include <vector>
#include <ostream>
#include <iostream>
#include <stdexcept>

#include "timbl/CommandLine.h"

using namespace std;

namespace Timbl {

  CL_Options::CL_Options( const int argc, const char * const *argv ){
    Split_Command_Line( argc, argv );
  }
  
  CL_Options::CL_Options( const string& args ){
    const char *argstr = args.c_str();
    Split_Command_Line( 0, &argstr );
  }
  
  CL_Options::~CL_Options(){
  }
  
  ostream& operator<<( ostream& os, const CL_item& it ){
    if ( it.longOpt ){
      os << "--" << it.opt_word;
      if ( !it.option.empty() )
	os << "=" << it.option;
    }
    else
      os << (it.mood ? "+": "-" ) << it.opt_word << it.option;
    return os;
  }

  ostream& operator<<( ostream& os, const CL_Options& cl ){
    list<CL_item>::const_iterator pos = cl.Opts.begin();
    while ( pos != cl.Opts.end() ){
      os << *pos << " ";
      ++pos;
    }
    return os;
  }

  bool CL_Options::Present( const char c ) const {
    list<CL_item>::const_iterator pos;
    for ( pos = Opts.begin(); pos != Opts.end(); ++pos ){
      if ( pos->OptChar() == c ){
	return true;
      }
    }
    return false;
  }
  
  bool CL_Options::Find( const char c, string &opt, bool& mood ) const {
    list<CL_item>::const_iterator pos;
    for ( pos = Opts.begin(); pos != Opts.end(); ++pos ){
      if ( pos->OptChar() == c ){
	opt = pos->Option();
	mood = pos->Mood();
	return true;
      }
    }
    return false;
  }
  
  bool CL_Options::Find( const string& w, string &opt ) const {
    list<CL_item>::const_iterator pos;
    for ( pos = Opts.begin(); pos != Opts.end(); ++pos ){
      if ( pos->OptWord() == w ){
	opt = pos->Option();
	return true;
      }
    }
    return false;
  }
  
  bool CL_Options::Delete( const char c, bool all ){
    list<CL_item>::iterator pos;
    for ( pos = Opts.begin(); pos != Opts.end(); ){
      if ( pos->OptChar() == c ){
	pos = Opts.erase(pos);
	if ( !all )
	  return true;
      }
      ++pos;
    }
    return false;
  }
  
  bool CL_Options::Delete( const string& w ){
    list<CL_item>::iterator pos;
    for ( pos = Opts.begin(); pos != Opts.end(); ++pos ){
      if ( pos->OptWord() == w ){
	Opts.erase(pos);
	return true;
      }
    }
    return false;
  }
  
  void CL_Options::Add( const string& s, const string& line ){
    CL_item cl( s, line );
    Opts.push_front( cl );
  }

  void CL_Options::Add( const char c, const string& line, bool mood ){
    CL_item cl( c, line, mood );
    Opts.push_front( cl );
  }

  inline bool p_or_m( char k )
  { return ( k == '+' || k == '-' ); }

  inline int opt_split( const char *line, vector<string>& new_argv ){ 
    int k=0;
    const char *p = line;
    int argc = 0;
    while ( *p ){
      if ( ( p_or_m(*p) && argc == 0 ) || 
	   ( isspace(*p++) && p_or_m(*p) ) ){
	argc++;
      }
    }
    string res;
    if ( argc != 0 ){
      new_argv.reserve(argc);
      p = line;
      while ( isspace( *p ) ){ p++; };
      while ( *p ){
	int skip = 0;
	while ( isspace( *p ) ){ p++; skip++; };
	if ( !*p )
	  break;
	if ( skip != 0 && p_or_m(*p) && k != 1 ){
	  new_argv.push_back( res );
	  k = 0;
	  res = "";
	}
	res += *p++;
      }
      new_argv.push_back( res );
    }
    return argc;
  }
  
  void CL_Options::Split_Command_Line( const int Argc, 
				       const char * const *Argv ){
    Opts.clear();
    int local_argc = 0;
    vector<string> local_argv;
    char Optchar;
    string Optword;
    string Option;
    bool Mood = false;
    if ( Argc == 0 ) 
      if ( Argv != 0 &&
	   Argv[0] != 0 ){
	local_argc = opt_split( Argv[0], local_argv );
      }
      else
	return;
    else {
      local_argc = Argc-1;
      for( int i=1; i < Argc; ++i ){
	// start at 1 to skip the program name
	local_argv.push_back( Argv[i] );
      }
    }
    for ( int arg_ind=0; arg_ind < local_argc; ++arg_ind ){
      bool longOpt = false;
      Option = local_argv[arg_ind];
      if ( !p_or_m(Option[0]) ){
	Optchar = '?';
	Optword = Option;
	Mood = false;
      }
      else {
	Mood = Option[0] == '+';
	if ( Option.size() > 1 ){
	  longOpt = Option[1] == '-';
	  if ( longOpt ){
	    if ( Mood )
	      throw std::runtime_error("invalid option: " + Option );
	    string::size_type pos = Option.find( "=" );
	    if ( pos == string::npos ){
	      Optword = Option.erase(0,2);
	      Option = "";
	    }
	    else {
	      Optword = Option.substr( 2, pos-2 );
	      Option = Option.substr( pos+1 );
	    }
	    Optchar = Optword[0];
	  }
	  else {
	    Optchar = Option[1];
	    Optword = Optchar;
	    Option = Option.erase(0,2);
	  }
	}
	else {
	  Optchar = 0;
	  Optword = Option;
	  Option = Option.erase(0,1);
	}
	if ( (!Optchar || Option.empty() ) && arg_ind+1 < local_argc ) {
	  string tmpOption = local_argv[arg_ind+1];
	  if ( !p_or_m(tmpOption[0]) ){
	    Option = tmpOption;
	    ++arg_ind;
	    if ( !Optchar ){
	      Optchar = Optword[0];
	    }
	  }
	}
      }
      if ( longOpt ){
	CL_item cl( Optword, Option );
	Opts.push_front( cl );
      }
      else {
	CL_item cl( Optchar, Option, Mood );
	Opts.push_front( cl );
      }
    }
  }    
}


