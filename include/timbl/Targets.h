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
#ifndef TIMBL_TARGETS_H
#define TIMBL_TARGETS_H

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

  class ValueClass {
  public:
    ValueClass( const icu::UnicodeString& n, size_t i ):
      _name( n ), _index( i ), _frequency( 1 ) {};
    ValueClass( const ValueClass& ) = delete; // forbid copies
    ValueClass& operator=( const ValueClass& ) = delete; // forbid copies
    virtual ~ValueClass() {};
    void ValFreq( size_t f ){ _frequency = f; };
    void IncValFreq( int f ){ _frequency += f; };
    size_t ValFreq( ) const { return _frequency; };
    void incr_val_freq(){ ++_frequency; };
    void decr_val_freq(){ --_frequency; };
    size_t Index() const { return _index; };
    const icu::UnicodeString& name() const { return _name; };
    const std::string name_string() const { return TiCC::UnicodeToUTF8(_name);};
    // temporary for backward compatability
    const icu::UnicodeString& name_u() const { return _name; }; // HACK
    const std::string Name() const { return TiCC::UnicodeToUTF8(_name); }; // HACK
    // REMOVE ^^^^
    friend std::ostream& operator<<( std::ostream& os, ValueClass const *vc );
  protected:
    const icu::UnicodeString& _name;
    size_t _index;
    size_t _frequency;
  };

  class TargetValue: public ValueClass {
  public:
    TargetValue( const icu::UnicodeString&, size_t );
  };

  class Targets: public MsgClass {
    friend class MBLClass;
    friend class WClassDistribution;
    friend class ConfusionMatrix;
  public:
    explicit Targets( Hash::UnicodeHash *T ):
      target_hash( T ),
      is_reference(false)
    {};
    ~Targets() override;
    Targets& operator=( const Targets& );
    void init();
    TargetValue *add_value( const icu::UnicodeString&, int freq = 1 );
    TargetValue *add_value( size_t, int freq = 1 );
    TargetValue *Lookup( const icu::UnicodeString& ) const;
    TargetValue *ReverseLookup( size_t ) const;
    bool decrement_value( TargetValue * );
    bool increment_value( TargetValue * );
    TargetValue *MajorityClass() const;
    size_t EffectiveValues() const;
    size_t TotalValues() const;
    size_t num_of_values() const { return values_array.size(); };
    Hash::UnicodeHash *hash() const { return target_hash; };
  private:
    Hash::UnicodeHash *target_hash;
    std::vector<TargetValue *> values_array;
    std::unordered_map< size_t, TargetValue *> reverse_values;
    bool is_reference;
  };

  class Vfield{
    friend class ClassDistribution;
    friend class WClassDistribution;
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

  class WClassDistribution;

  class ClassDistribution{
    friend std::ostream& operator<<( std::ostream&, const ClassDistribution& );
    friend std::ostream& operator<<( std::ostream&, const ClassDistribution * );
    friend class WClassDistribution;
  public:
    typedef std::map<size_t, Vfield *> VDlist;
    typedef VDlist::const_iterator dist_iterator;
    ClassDistribution( ): total_items(0) {};
    ClassDistribution( const ClassDistribution& );
    virtual ~ClassDistribution(){ clear(); };
    size_t totalSize() const{ return total_items; };
    size_t size() const{ return distribution.size(); };
    bool empty() const{ return distribution.empty(); };
    void clear();
    dist_iterator begin() const { return distribution.begin(); };
    dist_iterator end() const { return distribution.end(); };
    virtual const TargetValue* BestTarget( bool&, bool = false ) const;
    void Merge( const ClassDistribution& );
    virtual void SetFreq( const TargetValue *, int, double=1.0 );
    virtual bool IncFreq( const TargetValue *, size_t, double=1.0 );
    void DecFreq( const TargetValue * );
    static ClassDistribution *read_distribution( std::istream&,
						 Targets&,
						 bool );
    static ClassDistribution *read_distribution_hashed( std::istream&,
							Targets&,
							bool );
    const std::string DistToString() const;
    const std::string DistToStringW( int ) const;
    double Confidence( const TargetValue * ) const;
    virtual const std::string SaveHashed() const;
    virtual const std::string Save() const;
    bool ZeroDist() const { return total_items == 0; };
    double Entropy() const;
    ClassDistribution *to_VD_Copy( ) const;
    virtual WClassDistribution *to_WVD_Copy() const;
  protected:
    virtual void DistToString( std::string&, double=0 ) const;
    virtual void DistToStringWW( std::string&, int ) const;
    const TargetValue* BestTargetN( bool &, bool = false ) const;
    const TargetValue* BestTargetW( bool &, bool = false ) const;
    virtual ClassDistribution *clone( ) const {
      return new ClassDistribution(); };
    size_t total_items;
    VDlist distribution;
  };

  class WClassDistribution: public ClassDistribution {
  public:
    WClassDistribution(): ClassDistribution() {};
    const TargetValue* BestTarget( bool &, bool = false ) const override;
    void SetFreq( const TargetValue *, int, double ) override;
    bool IncFreq( const TargetValue *, size_t, double ) override;
    WClassDistribution *to_WVD_Copy( ) const override;
    const std::string SaveHashed() const override;
    const std::string Save() const override;
    void Normalize();
    void Normalize_1( double, const Targets& );
    void Normalize_2();
    void MergeW( const ClassDistribution&, double );
  private:
    void DistToString( std::string&, double=0 ) const override;
    void DistToStringWW( std::string&, int ) const override;
    WClassDistribution *clone() const override {
      return new WClassDistribution; };
  };

}
#endif // TINBL_TARGETS_H
