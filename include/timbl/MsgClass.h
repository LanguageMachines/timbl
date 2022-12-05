/*
  Copyright (c) 1998 - 2022
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
#ifndef TIMBL_MSGCLASS_H
#define TIMBL_MSGCLASS_H

namespace Timbl {
  class MsgClass {
  public:
    MsgClass():
      err_cnt(0)
    {};
    virtual ~MsgClass() {};
    virtual void Info( const std::string&  ) const;
    virtual void Warning( const std::string& ) const ;
    virtual void Error( const std::string& ) const ;
    virtual void FatalError( const std::string& ) const ;
    mutable int err_cnt;
  };

}
#endif
