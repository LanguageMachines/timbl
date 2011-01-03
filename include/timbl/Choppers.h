#ifndef CHOPPERS_H
#define CHOPPERS_H
/*
  Copyright (c) 1998 - 2010
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

namespace Timbl{

  static const std::string DefaultSparseString = "0.0000E-17";

  class Chopper {
  public:
    virtual ~Chopper() {};
    virtual bool chop( const std::string&, size_t ) = 0;
    const std::string& getField( size_t i ) const { return choppedInput[i]; };
    virtual double getExW() const { return -1; };
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
    static Chopper *create( InputFormatType , bool, int );
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
  
  class C45_Chopper : public virtual Chopper {
  public:
    bool chop( const std::string&, size_t );
    std::string getString() const;
  };

  class C45_ExChopper : public C45_Chopper, public ExChopper {
  };

  class ARFF_Chopper : public C45_Chopper {
  public:
    bool chop( const std::string&, size_t );
  };
  
  class ARFF_ExChopper : public C45_ExChopper {
  };
  
  class Bin_Chopper : public virtual Chopper {
  public:
    bool chop( const std::string&, size_t );
    std::string getString() const;
  };
  
  class Bin_ExChopper : public Bin_Chopper, public ExChopper {
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
  
  class Columns_Chopper : public virtual Chopper {
  public:
    bool chop( const std::string&, size_t );
    std::string getString() const;
  };

  class Columns_ExChopper : public Columns_Chopper, public ExChopper {
  };

  class Sparse_Chopper : public virtual Chopper {
  public:
    bool chop( const std::string&, size_t );
    std::string getString() const;
  };  

  class Sparse_ExChopper : public Sparse_Chopper, public ExChopper {
  };  

}
#endif // CHOPPERS_H
