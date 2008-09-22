#ifndef CHOPPERS_H
#define CHOPPERS_H
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

namespace Timbl{

  class Chopper {
  public:
    void init( size_t len ) { choppedInput.resize(len+2); };
    virtual ~Chopper() {};
    virtual bool chop( const std::string&,
		       size_t ) = 0;
    const std::string& getField( size_t i ) const { return choppedInput[i]; };
    void setField( size_t i, std::string s ){ choppedInput[i] = s; };
    const std::string& getExW() const { return exW; };
    void setExW( std::string& s){ exW = s; };
    virtual void print( std::ostream& ) = 0;
    void swapTarget( size_t target_pos ){
      std::string tmp = choppedInput[target_pos];
      for ( size_t i = target_pos+1; i <= size; ++i )
	choppedInput[i-1] = choppedInput[i];
      choppedInput[size] = tmp;
    }
  protected:
    std::vector<std::string> choppedInput;
    std::string exW;
    size_t size;
  };
  
  class C45_Chopper : public Chopper {
  public:
    bool chop( const std::string&, size_t );
    void print( std::ostream& );
  };


  class ARFF_Chopper : public C45_Chopper {
  public:
    bool chop( const std::string&, size_t );
  };
  
  class Bin_Chopper : public Chopper {
  public:
    bool chop( const std::string&, size_t );
    void print( std::ostream& os );
  };
  
  class Compact_Chopper : public Chopper {
  public:
  Compact_Chopper( int L ): fLen(L){};
    bool chop( const std::string&, size_t );
    void print( std::ostream& );
  private:
    int fLen;
  };
  
  class Columns_Chopper : public Chopper {
  public:
    bool chop( const std::string&, size_t );
    void print( std::ostream& );
  };

  class Sparse_Chopper : public Chopper {
  public:
    bool chop( const std::string&, size_t );
    void print( std::ostream& );
  };  

}
#endif // CHOPPERS_H
