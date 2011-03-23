/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2011
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
#ifndef OPTIONS_H
#define OPTIONS_H

#include <vector>
#include <climits>
#include <cstdio>

namespace Timbl {
  const int MAX_TABLE_SIZE =  50;
  
  class OptionClass {
    friend class OptionTableClass;
  public:
    OptionClass( const std::string& n ): Name( n ) {};
    virtual ~OptionClass() {};
    virtual bool set_option( const std::string& ) = 0;
    virtual std::ostream& show_opt( std::ostream & ) const = 0;
    virtual std::ostream& show_full( std::ostream & ) const = 0;
  protected:
    const std::string Name;
  private:
    OptionClass(const OptionClass&);
    OptionClass& operator = (const OptionClass&);
  };
  
  template <class Type>
    class OptionClassT: public OptionClass {
    public:
    OptionClassT( const std::string& n, Type *tp, Type t ):OptionClass(n),
      Content(tp) { *Content = t; };
    virtual bool set_option( const std::string& line ){ 
      Type T;
      bool result = stringTo<Type>( line, T );
      if ( result ) *Content = T;
      return result;
    };
    virtual std::ostream& show_opt( std::ostream &os ) const {
      os.width(20);
      os.setf( std::ios::left, std::ios::adjustfield );
      os << Name << " : " << toString<Type>(*Content);
      return os;
    };
    virtual std::ostream& show_full( std::ostream &os ) const {
      return show_opt( os );
    };
    private:
    Type *Content;
    OptionClassT(const OptionClassT&);
    OptionClassT& operator = (const OptionClassT&);
  };
  
  typedef OptionClassT<bool> BoolOption;
  
  template <>
    inline std::ostream& OptionClassT<bool>::show_opt( std::ostream &os ) const {
      os.width(20);
      os.setf( std::ios::left, std::ios::adjustfield );
      os.setf( std::ios::boolalpha );
      os << Name << " : " << *Content;
      return os;
  }
  
  template <>
    inline std::ostream& OptionClassT<bool>::show_full( std::ostream &os ) const {
    os.width(20);
    os.setf( std::ios::left, std::ios::adjustfield );
    os.setf( std::ios::boolalpha );
    os << Name << " :  false or true [" << *Content << "]";
    return os;
  }
  
  typedef OptionClassT<VerbosityFlags> VerbosityOption;
  
  template <>
    inline std::ostream& OptionClassT<VerbosityFlags>::show_full( std::ostream &os ) const {
    os.width(20);
    os.setf( std::ios::left, std::ios::adjustfield );
    os << Name << " : " << toString<VerbosityFlags>(*Content,true);
    return os;
  }
  
  typedef OptionClassT<InputFormatType> InputFormatOption;
  
  template <>
    inline std::ostream& InputFormatOption::show_full( std::ostream &os ) const {
    os.width(20);
    os.setf( std::ios::left, std::ios::adjustfield );
    os << Name << " : {";
    InputFormatType i = UnknownInputFormat;
    for ( ++i; i < MaxInputFormat-1; ++i )
      os << toString<InputFormatType>(i) << ", ";
    os << toString<InputFormatType>(i) << "}, [ "
       << toString<InputFormatType>(*Content) << "]";
    return os;
  }
  
  
  typedef OptionClassT<MetricType> MetricOption;
  
  template <>
    inline std::ostream& OptionClassT<MetricType>::show_full( std::ostream &os )const {
    os.width(20);
    os.setf( std::ios::left, std::ios::adjustfield );
    os << Name << " : {";
    MetricType i = UnknownMetric;
    for ( ++i; i < MaxMetric-1; ++i )
      os << toString<MetricType>(i) << ", ";
    os << toString<MetricType>(i) << "}, [ "
       << toString<MetricType>(*Content) << "]";
    return os;
  }

  typedef OptionClassT<AlgorithmType> AlgorithmOption;
  
  template <>
    inline std::ostream& OptionClassT<AlgorithmType>::show_full( std::ostream &os ) const {
    os.width(20);
    os.setf( std::ios::left, std::ios::adjustfield );
    os << Name << " : {";
    AlgorithmType i = Unknown_a;
    for ( ++i; i < Max_a-1; ++i )
      os << toString<AlgorithmType>(i) << ", ";
    os << toString<AlgorithmType>(i) << "}, [ "
       << toString<AlgorithmType>(*Content) << "]";
    return os;
  }
  
  typedef OptionClassT<DecayType> DecayOption;
  
  template <>
    inline std::ostream& DecayOption::show_full( std::ostream &os ) const {
    os.width(20);
    os.setf( std::ios::left, std::ios::adjustfield );
    os << Name << " : {";
    DecayType i = UnknownDecay;
    for ( ++i; i < MaxDecay-1; ++i )
      os << toString<DecayType>(i) << ", ";
    os << toString<DecayType>(i) << "}, [ "
       << toString<DecayType>(*Content) << "]";
    return os;
  }
  
  typedef OptionClassT<SmoothingType> SmoothOption;
  
  template <>
    inline std::ostream& SmoothOption::show_full( std::ostream &os ) const {
    os.width(20);
    os.setf( std::ios::left, std::ios::adjustfield );
    os << Name << " : {";
    SmoothingType i = UnknownSmoothing;
    for ( ++i; i < MaxSmoothing-1; ++i )
      os << toString<SmoothingType>(i) << ", ";
    os << toString<SmoothingType>(i) << "}, [ "
       << toString<SmoothingType>(*Content) << "]";
    return os;
  }
  
  typedef OptionClassT<WeightType> WeightOption;
  
  template <>
    inline std::ostream& OptionClassT<WeightType>::show_full( std::ostream &os ) const {
    os.width(20);
    os.setf( std::ios::left, std::ios::adjustfield );
    os << Name << " : {";
    WeightType i = Unknown_w;
    for ( ++i; i < Max_w-1; ++i )
      os << toString<WeightType>(i) << ", ";
    os << toString<WeightType>(i) << "}, [ "
       << toString<WeightType>(*Content) << "]";
    return os;
  }
  
  typedef OptionClassT<OrdeningType> OrdeningOption;
  
  template <>
    inline std::ostream& OptionClassT<OrdeningType>::show_full( std::ostream &os ) const {
    os.width(20);
    os.setf( std::ios::left, std::ios::adjustfield );
    os << Name << " : {";
    OrdeningType i = UnknownOrdening;
    for ( ++i; i < MaxOrdening-1; ++i )
      os << toString<OrdeningType>(i) << ", ";
    os << toString<OrdeningType>(i) << "}, [ "
       << toString<OrdeningType>(*Content) << "]";
    return os;
  }

  typedef OptionClassT<normType> NormalisationOption;
  
  template <>
    inline std::ostream& NormalisationOption::show_full( std::ostream &os ) const {
    os.width(20);
    os.setf( std::ios::left, std::ios::adjustfield );
    os << Name << " : {";
    normType i = unknownNorm;
    for ( ++i; i < maxNorm-1; ++i )
      os << toString<normType>(i) << ", ";
    os << toString<normType>(i) << "}, [ "
       << toString<normType>(*Content) << "]";
    return os;
  }

  //
  // Array of options types
  //  
  template <class Type>
    class OptionArrayClass: public OptionClass {
    public:
    OptionArrayClass( const std::string& n, 
		      std::vector<Type>& ta,
		      const size_t size ): 
      OptionClass( n ), TA(ta), Size(size ){};
    protected:
    std::vector<Type>& TA;
    size_t Size;
    private:
    OptionArrayClass(const OptionArrayClass&);
    OptionArrayClass& operator = (const OptionArrayClass&);
  };
  
  
  class MetricArrayOption: public OptionArrayClass<MetricType> {
  public:
    MetricArrayOption( const std::string& n, 
		       std::vector<MetricType>& mp, 
		       MetricType& m,
		       size_t s ):
    OptionArrayClass<MetricType>( n, mp, s ), def(m){ 
      for ( size_t i=0; i < s; i++ )
	TA[i] = m;
    };
    bool set_option( const std::string& line );
    std::ostream& show_opt( std::ostream &os ) const;
    std::ostream& show_full( std::ostream &os ) const;
  private:
    const MetricType& def;
  };

  inline bool MetricArrayOption::set_option( const std::string& line ){ 
    MetricType m = UnknownMetric;
    size_t i=0;
    std::vector<std::string> res;
    bool result = split_at( line, res, "=" ) == 2 &&
      stringTo<MetricType>( res[1], m ) && 
      stringTo<size_t>( res[0], i, 0, Size );
    if ( result ) 
      TA[i] = m;
    return result;
  }
  
  inline std::ostream& MetricArrayOption::show_opt( std::ostream &os ) const {
    os.width(20);
    os.setf( std::ios::left, std::ios::adjustfield );
    os << Name << " : ";
    for ( size_t i=0; i < Size; i++ )
      if ( TA[i] != def )
	os << i << ":" << toString<MetricType>(TA[i]) << ", ";
    return os;
  }
  
  inline std::ostream& MetricArrayOption::show_full( std::ostream &os ) const {
    os.width(20);
    os.setf( std::ios::left, std::ios::adjustfield );
    os << Name << " : comma separated metricvalues, [";
    bool first = true;
    for ( size_t i=0; i < Size; i++ ){
      if ( TA[i] != def ){
	if ( !first )
	  os << ",";
	else
	  first = false;
	os << i << ":" << toString<MetricType>(TA[i]);
      }
    }
    os << "]";
    return os;
  }
  
  //
  // Limited Type, with min and maxVal
  //  
  template <class Type>
    class OptionClassLT: public OptionClass {
    public:
    OptionClassLT( const std::string& n, Type *tp, Type t,
		   Type Min, Type Max ):OptionClass(n),
      Content( tp), minVal( Min ), maxVal( Max )
      { *Content = t; };
    
    virtual bool set_option( const std::string& line ){ 
      Type T;
      bool result = stringTo<Type>( line, T, minVal, maxVal );
      if ( result ) *Content = T;
      return result;
    };
    virtual std::ostream& show_opt( std::ostream &os ) const {
      os.width(20);
      os.setf( std::ios::showpoint );
      os.setf( std::ios::left, std::ios::adjustfield );
      os << Name << " : " << *Content;
      return os;
    };
    virtual std::ostream& show_full( std::ostream &os ) const {
      os.width(20);
      os.setf( std::ios::showpoint );
      os.setf( std::ios::left, std::ios::adjustfield );
      os << Name << " :  { " 
	 << minVal << " - " << maxVal << "}, [" << *Content << "]";
      return os;
    };
    private:
    Type *Content;
    Type minVal;
    Type maxVal;
    OptionClassLT(const OptionClassLT&);
    OptionClassLT& operator = (const OptionClassLT&);
  };
  
  typedef OptionClassLT<int> IntegerOption;
  typedef OptionClassLT<unsigned int> UnsignedOption;
  typedef OptionClassLT<size_t> SizeOption;
  typedef OptionClassLT<double> RealOption;
  
  enum SetOptRes { Opt_OK, Opt_Frozen, Opt_Unknown, Opt_Ill_Val};
  
  class OptionTableClass {
  public:
    bool Add( OptionClass *opt ){
      Table[table_size++] = opt;
      return table_size < MAX_TABLE_SIZE;
    };
    void SetFreezeMark(void){ table_start = table_size; };
    void FreezeTable(void){ table_frozen = true; };
    bool TableFrozen(void){ return table_frozen; };
    SetOptRes SetOption( const std::string& );
    void Show_Settings( std::ostream& ) const;
    void Show_Options( std::ostream& ) const;
    OptionTableClass():
      table_start(0), table_size(0), table_frozen(false),Table(0){
      Table = new OptionClass *[MAX_TABLE_SIZE]; };
    ~OptionTableClass(){
      for ( int i=0; i < table_size; i++ )
	delete Table[i];
      delete [] Table;
    };
  private:
    int table_start;
    int table_size;
    bool table_frozen;
    OptionClass **Table;
    inline OptionClass *look_up( const std::string&, bool & );
    OptionTableClass( const OptionTableClass& );
    OptionTableClass& operator=( const OptionTableClass& );
  };
  
  inline void OptionTableClass::Show_Settings( std::ostream& os ) const{
    for ( int i=0; i <table_size; i++)
      Table[i]->show_opt( os ) << std::endl;
  }
  
  inline void OptionTableClass::Show_Options( std::ostream& os ) const {
    for ( int i=0; i <table_size; i++)
      Table[i]->show_full( os ) << std::endl;
  }

  inline void split_line( const std::string& line, 
			  std::string& name, 
			  std::string& value ){
    std::vector<std::string> results;
    size_t i = split_at( line, results, ":" );
    switch (i){
    case 2:
      name = compress(results[0]);
    case 1:
      value = compress(results[1]);
    default:
      break;
    }
  }  

  inline OptionClass *OptionTableClass::look_up( const std::string& option_name, 
						 bool &runtime ){
    for ( int i=0; i < table_size; i++ )
      if ( compare_nocase( option_name, Table[i]->Name ) ){
	runtime = (i >= table_start || !table_frozen );
	return Table[i];
      }
    return NULL;
  }
  
  inline SetOptRes OptionTableClass::SetOption( const std::string& line ){ 
    SetOptRes result = Opt_OK;
    bool runtime = false;
    std::string option_name;
    std::string value;
    split_line( line, option_name, value );
    OptionClass *option = look_up( option_name, runtime );
    if ( option ){
      if ( !runtime )
	result = Opt_Frozen; // may not be changed at this stage
      else
	if ( !option->set_option( value ) )
	  result = Opt_Ill_Val; // illegal value
    }
    else 
      result = Opt_Unknown; // What the hell ???
    return result;
  }  


}

#endif

