/*
  Copyright (c) 1998 - 2017
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

#include "ticcutils/XMLtools.h"
#include "timbl/neighborSet.h"

namespace Timbl {

  class BestRec {
    friend std::ostream& operator<< ( std::ostream&, const BestRec * );
  public:
    BestRec();
    ~BestRec();
    size_t totalBests() { return aggregateDist.totalSize(); };
    double bestDistance;
    ValueDistribution aggregateDist;
    std::vector<ValueDistribution*> bestDistributions;
    std::vector<std::string> bestInstances;
  private:
    BestRec( const BestRec& );
    BestRec& operator=( const BestRec& );
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
    double addResult( double, const ValueDistribution *, const std::string& );
    void initNeighborSet( neighborSet& ) const;
    void addToNeighborSet( neighborSet& , size_t ) const;
    xmlNode *toXML() const;
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
