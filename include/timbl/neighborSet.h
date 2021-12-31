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

#ifndef TIMBL_NEIGHBORSET_H
#define TIMBL_NEIGHBORSET_H
namespace Timbl{

  class decayStruct {
    friend std::ostream& operator<<( std::ostream&, const decayStruct& );
    friend std::ostream& operator<<( std::ostream&, const decayStruct * );
  public:
    decayStruct():alpha(0),beta(0){};
    decayStruct(double a, double b ):alpha(a),beta(b){};
    virtual ~decayStruct(){};
    virtual std::ostream& put( std::ostream& ) const = 0;
    virtual DecayType type() const = 0;
    double alpha;
    double beta;
  };

  class zeroDecay: public decayStruct {
  public:
    zeroDecay():decayStruct(){};
    std::ostream& put( std::ostream& ) const;
    DecayType type() const { return Zero;};
  };

  class invLinDecay: public decayStruct {
  public:
    invLinDecay():decayStruct(){};
    std::ostream& put( std::ostream& ) const;
    DecayType type() const { return InvLinear;};
  };

  class invDistDecay: public decayStruct {
  public:
    invDistDecay():decayStruct(){};
    std::ostream& put( std::ostream& ) const;
    DecayType type() const { return InvDist;};
  };

  class expDecay: public decayStruct {
  public:
    explicit expDecay( double alp ): decayStruct(alp,1.0){};
    expDecay( double alp, double bet ): decayStruct(alp,bet){};
    std::ostream& put( std::ostream& ) const;
    DecayType type() const { return ExpDecay;};
  };

  class neighborSet {
    friend std::ostream& operator<<( std::ostream&, const neighborSet& );
    friend std::ostream& operator<<( std::ostream&, const neighborSet * );
    friend class BestArray;
  public:
    neighborSet();
    ~neighborSet();
    neighborSet( const neighborSet& in );
    neighborSet& operator=( const neighborSet& );
    size_t size() const;
    void reserve( size_t );
    void clear();
    void truncate( size_t );
    void merge( const neighborSet& );
    double getDistance( size_t ) const;
    double bestDistance() const { return getDistance(0); };
    const ValueDistribution *getDistribution( size_t ) const;
    WValueDistribution *bestDistribution( const decayStruct * =0,
					  size_t =0 ) const ;
    double relativeWeight( const decayStruct *, size_t ) const;
    bool setShowDistance( bool b ) const {
      bool ret = showDistance;
      showDistance = b;
      return ret;
    }
    bool setShowDistribution( bool b ) const {
      bool ret = showDistribution;
      showDistribution = b;
      return ret;
    }
  private:
    mutable bool showDistance;
    mutable bool showDistribution;
    void push_back( double, const ValueDistribution & );
    std::vector<double> distances;
    std::vector<ValueDistribution *> distributions;
  };

}
#endif
