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
#ifndef TIMBL_INSTANCE_H
#define TIMBL_INSTANCE_H

#include "ticcutils/Unicode.h"
#include "timbl/Targets.h"
#include "timbl/Features.h"

namespace Hash {
  class UnicodeHash;
}

namespace Timbl {

  class TargetValue;
  class FeatureValue;

  class Instance {
    friend std::ostream& operator<<(std::ostream&, const Instance& );
    friend std::ostream& operator<<(std::ostream&, const Instance * );
  public:
    Instance();
    explicit Instance( size_t s ): Instance() { Init( s ); };
    Instance( const Instance& ) = delete; // inhibit copies
    Instance& operator=( const Instance& ) = delete; // inhibit copies
    ~Instance();
    void Init( size_t );
    void clear();
    double ExemplarWeight() const { return sample_weight; };
    void ExemplarWeight( const double sw ){ sample_weight = sw; };
    int Occurrences() const { return occ; };
    void Occurrences( const int o ) { occ = o; };
    size_t size() const { return FV.size(); };
    std::vector<FeatureValue *> FV;
    TargetValue *TV;
  private:
    double sample_weight; // relative weight
    int occ;
  };

}
#endif
