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
#ifndef TIMBL_INSTANCE_H
#define TIMBL_INSTANCE_H

#include <stdexcept>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>
#include "unicode/unistr.h"
#include "timbl/MsgClass.h"
#include "ticcutils/Unicode.h"

namespace Hash {
  class UnicodeHash;
}

namespace Timbl {

  class TargetValue;
  class FeatureValue;

  class Vfield{
    friend class ValueDistribution;
    friend class WValueDistribution;
    friend std::ostream& operator<<( std::ostream&, const Vfield& );
    friend std::ostream& operator<<( std::ostream&, const Vfield * );
  public:
    Vfield( const TargetValue *val, int freq, double w ):
      value(val), frequency(freq), weight(w) {};
    Vfield( const Vfield& in ):
      value(in.value), frequency(in.frequency), weight(in.weight) {};
    Vfield& operator=( const Vfield& ) = delete; // forbid copies
    ~Vfield(){};
    std::ostream& put( std::ostream& ) const;
    const TargetValue *Value() const { return value; };
    void Value( const TargetValue *t ){  value = t; };
    size_t Freq() const { return frequency; };
    void IncFreq( int inc=1 ) {  frequency += inc; };
    void AddFreq( int f ) {  frequency += f; weight += f; };
    void DecFreq() {  frequency -= 1; };
    double Weight() const { return weight; };
    void SetWeight( double w ){ weight = w; };
    size_t Index();
  protected:
    const TargetValue *value;
    size_t frequency;
    double weight;
  private:
  };

  class Targets;

  class WValueDistribution;

  class ValueDistribution{
    friend std::ostream& operator<<( std::ostream&, const ValueDistribution& );
    friend std::ostream& operator<<( std::ostream&, const ValueDistribution * );
    friend class WValueDistribution;
  public:
    typedef std::map<size_t, Vfield *> VDlist;
    typedef VDlist::const_iterator dist_iterator;
    ValueDistribution( ): total_items(0) {};
    ValueDistribution( const ValueDistribution& );
    virtual ~ValueDistribution(){ clear(); };
    size_t totalSize() const{ return total_items; };
    size_t size() const{ return distribution.size(); };
    bool empty() const{ return distribution.empty(); };
    void clear();
    dist_iterator begin() const { return distribution.begin(); };
    dist_iterator end() const { return distribution.end(); };
    virtual const TargetValue* BestTarget( bool&, bool = false ) const;
    void Merge( const ValueDistribution& );
    virtual void SetFreq( const TargetValue *, int, double=1.0 );
    virtual bool IncFreq( const TargetValue *, size_t, double=1.0 );
    void DecFreq( const TargetValue * );
    static ValueDistribution *read_distribution( std::istream&,
						 Targets&,
						 bool );
    static ValueDistribution *read_distribution_hashed( std::istream&,
							Targets&,
							bool );
    const std::string DistToString() const;
    const std::string DistToStringW( int ) const;
    double Confidence( const TargetValue * ) const;
    virtual const std::string SaveHashed() const;
    virtual const std::string Save() const;
    bool ZeroDist() const { return total_items == 0; };
    double Entropy() const;
    ValueDistribution *to_VD_Copy( ) const;
    virtual WValueDistribution *to_WVD_Copy() const;
  protected:
    virtual void DistToString( std::string&, double=0 ) const;
    virtual void DistToStringWW( std::string&, int ) const;
    const TargetValue* BestTargetN( bool &, bool = false ) const;
    const TargetValue* BestTargetW( bool &, bool = false ) const;
    virtual ValueDistribution *clone( ) const {
      return new ValueDistribution(); };
    size_t total_items;
    VDlist distribution;
  };

  class WValueDistribution: public ValueDistribution {
  public:
    WValueDistribution(): ValueDistribution() {};
    const TargetValue* BestTarget( bool &, bool = false ) const override;
    void SetFreq( const TargetValue *, int, double ) override;
    bool IncFreq( const TargetValue *, size_t, double ) override;
    WValueDistribution *to_WVD_Copy( ) const override;
    const std::string SaveHashed() const override;
    const std::string Save() const override;
    void Normalize();
    void Normalize_1( double, const Targets& );
    void Normalize_2();
    void MergeW( const ValueDistribution&, double );
  private:
    void DistToString( std::string&, double=0 ) const override;
    void DistToStringWW( std::string&, int ) const override;
    WValueDistribution *clone() const override {
      return new WValueDistribution; };
  };

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
