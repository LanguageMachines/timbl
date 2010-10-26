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
#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <list>
#include <iosfwd>

namespace Timbl {
  
  class CL_item {
    friend std::ostream& operator<<( std::ostream&, const CL_item& );
  public:
  CL_item( const std::string& s, const std::string& o ): 
    opt_word(s), option( o ), mood( false ), longOpt(true) {};
  CL_item( char c, const std::string& o, bool m=false ): 
    option( o ), mood( m ), longOpt(false){ opt_word = c; };
  CL_item( const CL_item& in ):
    opt_word( in.opt_word ), option(in.option),
      mood(in.mood), longOpt(in.longOpt){
    };
    CL_item& operator=( const CL_item& );
    bool Mood() const { return mood; };
    char OptChar() const { return opt_word[0]; };
    const std::string& OptWord() const { return opt_word; };
    const std::string& Option() const { return option; };
    bool isLong() const { return longOpt; };
    bool getMood() const { return mood; };
  private:
    std::string opt_word;
    std::string option;
    bool mood;
    bool longOpt;
  };

  //  typedef std::list<CL_item> CommandLine;

  class CL_Options {
    friend std::ostream& operator<<( std::ostream&, const CL_Options& );
  public:
    CL_Options( const int, const char * const * );
    CL_Options( const std::string& );
    ~CL_Options();
    bool Present( const char ) const;
    bool Find( const char, std::string&, bool& ) const;
    bool Find( const std::string&, std::string& ) const;
    bool Delete( const char, bool = false );
    bool Delete( const std::string& );
    void Add( const char, const std::string&, bool );
    void Add( const std::string&, const std::string& );
    std::list<CL_item> Opts;
    void Split_Command_Line( const int, const char * const * ); 
  private:
    CL_Options( const CL_Options& );
    CL_Options& operator=( const CL_Options& );
  };
  
}
#endif
