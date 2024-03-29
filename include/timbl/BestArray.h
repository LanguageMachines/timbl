/*
  Copyright (c) 1998 - 2024
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
#ifndef TIMBL_BESTARRAY_H
#define TIMBL_BESTARRAY_H

#include <vector>
#include <iosfwd>
#include <cstddef>

#include "unicode/unistr.h"
#include "libxml/parser.h"
#include "ticcutils/json.hpp"
#include "timbl/Targets.h"

namespace Timbl {

  class neighborSet;

  class BestRec {
    friend std::ostream& operator<< ( std::ostream&, const BestRec * );
  public:
    BestRec();
    BestRec( const BestRec& ) = delete; // forbid copies
    BestRec& operator=( const BestRec& ) = delete; // forbid copies
    ~BestRec();
    size_t totalBests() const { return aggregateDist.totalSize(); };
    double bestDistance;
    ClassDistribution aggregateDist;
    std::vector<ClassDistribution*> bestDistributions;
    std::vector<icu::UnicodeString> bestInstances;
  private:
  };

  class BestArray {
    friend std::ostream& operator<< ( std::ostream&, const BestArray& );
  public:
  BestArray(): _storeInstances(false),
      _showDi(false),
      _showDb(false),
      size(0),
      maxBests(0)
	{};
    ~BestArray();
    void init( unsigned int, unsigned int, bool, bool, bool );
    double addResult( double,
		      const ClassDistribution *,
		      const icu::UnicodeString& );
    void initNeighborSet( neighborSet& ) const;
    void addToNeighborSet( neighborSet& , size_t ) const;
    xmlNode *toXML() const;
    nlohmann::json to_JSON() const;
    nlohmann::json record_to_json( const BestRec *, size_t ) const;
  private:
    bool _storeInstances;
    bool _showDi;
    bool _showDb;
    unsigned int size;
    unsigned int maxBests;
    std::vector<BestRec *> bestArray;
  };

}
#endif // TIMBL_BESTARRAY_H
