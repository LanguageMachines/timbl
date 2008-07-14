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

#ifndef NEIGHBORSET_H
#define NEIGHBORSET_H
namespace Timbl{

  class decayStruct {
  public:
    decayStruct():alpha(0),beta(0){};
    decayStruct(double a, double b ):alpha(a),beta(b){};
    virtual ~decayStruct(){};
    enum decay_t { Zero_d, InvLin_d, InvDist_d, Exp_d };
    virtual decay_t type() const = 0;
    double alpha;
    double beta;
  };
  
  class zeroDecay: public decayStruct {
  public:
    zeroDecay():decayStruct(){};
    decay_t type() const { return Zero_d;};
  };
  
  class invLinDecay: public decayStruct {
  public:
    invLinDecay():decayStruct(){};
    decay_t type() const { return InvLin_d;};
  };
  
  class invDistDecay: public decayStruct {
  public:
    invDistDecay():decayStruct(){};
    decay_t type() const { return InvDist_d;};
  };
  
  class expDecay: public decayStruct {
  public:
    expDecay( double alp ): decayStruct(alp,1.0){};
    expDecay( double alp, double bet ): decayStruct(alp,bet){};
    decay_t type() const { return Exp_d;};
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
