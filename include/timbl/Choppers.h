#ifndef CHOPPERS_H
#define CHOPPERS_H
/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2012
  ILK   - Tilburg University
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
      http://ilk.uvt.nl/software.html
  or send mail to:
      timbl@uvt.nl
*/

namespace Timbl{

  static const std::string DefaultSparseString = "0.0000E-17";

  class Chopper {
  public:
    virtual ~Chopper() {};
    virtual bool chop( const std::string&, size_t ) = 0;
    const std::string& getField( size_t i ) const { return choppedInput[i]; };
    virtual double getExW() const { return -1; };
    virtual int getOcc() const { return 1; };
    virtual std::string getString() const = 0;
    void print( std::ostream& os ){
      os << getString();
    };
    void swapTarget( size_t target_pos ){
      std::string tmp = choppedInput[target_pos];
      for ( size_t i = target_pos+1; i < vSize; ++i )
	choppedInput[i-1] = choppedInput[i];
      choppedInput[vSize-1] = tmp;
    }
    static Chopper *create( InputFormatType , bool, int, bool );
    static InputFormatType getInputFormat( const std::string&,
					   bool=false );
    static size_t countFeatures( const std::string&, 
				 InputFormatType,  
				 int,
				 bool=false );
  protected:
    virtual void init( const std::string&, size_t, bool );
    size_t vSize;
    std::string strippedInput;
    std::vector<std::string> choppedInput;
  };
  
  class ExChopper: public virtual Chopper {
  public:
    double getExW() const { return exW; };
  protected:
    void init( const std::string&, size_t, bool );
    double exW;
  };
  
  class OccChopper: public virtual Chopper {
  public:
    int getOcc() const { return occ; };
  protected:
    void init( const std::string&, size_t, bool );
    int occ;
  };
  
  class C45_Chopper : public virtual Chopper {
  public:
    bool chop( const std::string&, size_t );
    std::string getString() const;
  };

  class C45_ExChopper : public C45_Chopper, public ExChopper {
  };

  class C45_OccChopper : public C45_Chopper, public OccChopper {
  };

  class ARFF_Chopper : public C45_Chopper {
  public:
    bool chop( const std::string&, size_t );
  };
  
  class ARFF_ExChopper : public C45_ExChopper {
  };
  
  class ARFF_OccChopper : public C45_OccChopper {
  };
  
  class Bin_Chopper : public virtual Chopper {
  public:
    bool chop( const std::string&, size_t );
    std::string getString() const;
  };
  
  class Bin_ExChopper : public Bin_Chopper, public ExChopper {
  };
  
  class Bin_OccChopper : public Bin_Chopper, public OccChopper {
  };
  
  class Compact_Chopper : public virtual Chopper {
  public:
  Compact_Chopper( int L ): fLen(L){};
    bool chop( const std::string&, size_t );
    std::string getString() const;
  private:
    int fLen;
    Compact_Chopper();
  };
  
  class Compact_ExChopper : public Compact_Chopper, public ExChopper {
  public:
  Compact_ExChopper( int L ): Compact_Chopper( L ){};
  private:
    Compact_ExChopper();
  };
  
  class Compact_OccChopper : public Compact_Chopper, public OccChopper {
  public:
  Compact_OccChopper( int L ): Compact_Chopper( L ){};
  private:
    Compact_OccChopper();
  };
  
  class Columns_Chopper : public virtual Chopper {
  public:
    bool chop( const std::string&, size_t );
    std::string getString() const;
  };

  class Columns_ExChopper : public Columns_Chopper, public ExChopper {
  };

  class Columns_OccChopper : public Columns_Chopper, public OccChopper {
  };

  class Sparse_Chopper : public virtual Chopper {
  public:
    bool chop( const std::string&, size_t );
    std::string getString() const;
  };  

  class Sparse_ExChopper : public Sparse_Chopper, public ExChopper {
  };  

  class Sparse_OccChopper : public Sparse_Chopper, public OccChopper {
  };  

}
#endif // CHOPPERS_H
