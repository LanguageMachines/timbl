#ifndef TIMBL_CHOPPERS_H
#define TIMBL_CHOPPERS_H
/*
  Copyright (c) 1998 - 2021
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

namespace Timbl{

  static const icu::UnicodeString DefaultSparseString = "0.0000E-17";

  class Chopper {
  public:
    virtual ~Chopper() {};
    virtual bool chop( const icu::UnicodeString&, size_t ) = 0;
    const icu::UnicodeString& getField( size_t i ) const {
      return choppedInput[i];
    };
    virtual double getExW() const { return -1; };
    virtual int getOcc() const { return 1; };
    virtual icu::UnicodeString getString() const = 0;
    void print( std::ostream& os ){
      os << getString();
    };
    void swapTarget( size_t target_pos ){
      icu::UnicodeString tmp = choppedInput[target_pos];
      for ( size_t i = target_pos+1; i < vSize; ++i ){
	choppedInput[i-1] = choppedInput[i];
      }
      choppedInput[vSize-1] = tmp;
    }
    static Chopper *create( InputFormatType , bool, int, bool );
    static InputFormatType getInputFormat( const icu::UnicodeString&,
					   bool=false );
    static size_t countFeatures( const icu::UnicodeString&,
				 InputFormatType,
				 int,
				 bool=false );
  protected:
    virtual void init( const icu::UnicodeString&, size_t, bool );
    size_t vSize;
    icu::UnicodeString strippedInput;
    std::vector<icu::UnicodeString> choppedInput;
  };

  class ExChopper: public virtual Chopper {
  public:
    double getExW() const { return exW; };
  protected:
    void init( const icu::UnicodeString&, size_t, bool );
    double exW;
  };

  class OccChopper: public virtual Chopper {
  public:
    int getOcc() const { return occ; };
  protected:
    void init( const icu::UnicodeString&, size_t, bool );
    int occ;
  };

  class C45_Chopper : public virtual Chopper {
  public:
    bool chop( const icu::UnicodeString&, size_t );
    icu::UnicodeString getString() const;
  };

  class C45_ExChopper : public C45_Chopper, public ExChopper {
  };

  class C45_OccChopper : public C45_Chopper, public OccChopper {
  };

  class ARFF_Chopper : public C45_Chopper {
  public:
    bool chop( const icu::UnicodeString&, size_t );
  };

  class ARFF_ExChopper : public C45_ExChopper {
  };

  class ARFF_OccChopper : public C45_OccChopper {
  };

  class Bin_Chopper : public virtual Chopper {
  public:
    bool chop( const icu::UnicodeString&, size_t );
    icu::UnicodeString getString() const;
  };

  class Bin_ExChopper : public Bin_Chopper, public ExChopper {
  };

  class Bin_OccChopper : public Bin_Chopper, public OccChopper {
  };

  class Compact_Chopper : public virtual Chopper {
  public:
    explicit Compact_Chopper( int L ): fLen(L){};
    bool chop( const icu::UnicodeString&, size_t );
    icu::UnicodeString getString() const;
  private:
    int fLen;
    Compact_Chopper();
  };

  class Compact_ExChopper : public Compact_Chopper, public ExChopper {
  public:
    explicit Compact_ExChopper( int L ): Compact_Chopper( L ){};
  private:
    Compact_ExChopper();
  };

  class Compact_OccChopper : public Compact_Chopper, public OccChopper {
  public:
    explicit Compact_OccChopper( int L ): Compact_Chopper( L ){};
  private:
    Compact_OccChopper();
  };

  class Columns_Chopper : public virtual Chopper {
  public:
    bool chop( const icu::UnicodeString&, size_t );
    icu::UnicodeString getString() const;
  };

  class Columns_ExChopper : public Columns_Chopper, public ExChopper {
  };

  class Columns_OccChopper : public Columns_Chopper, public OccChopper {
  };

  class Tabbed_Chopper : public virtual Chopper {
  public:
    bool chop( const icu::UnicodeString&, size_t );
    icu::UnicodeString getString() const;
  };

  class Tabbed_ExChopper : public Tabbed_Chopper, public ExChopper {
  };

  class Tabbed_OccChopper : public Tabbed_Chopper, public OccChopper {
  };


  class Sparse_Chopper : public virtual Chopper {
  public:
    bool chop( const icu::UnicodeString&, size_t );
    icu::UnicodeString getString() const;
  };

  class Sparse_ExChopper : public Sparse_Chopper, public ExChopper {
  };

  class Sparse_OccChopper : public Sparse_Chopper, public OccChopper {
  };

}
#endif // TIMBL_CHOPPERS_H
