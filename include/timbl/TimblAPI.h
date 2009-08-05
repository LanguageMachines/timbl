/*
  Copyright (c) 1998 - 2009
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

#ifndef TIMBLAPI_H
#define TIMBLAPI_H

#include <string>
#include <vector>
#include "StringOps.h"
#include "Common.h"
#include "MsgClass.h"
#include "Types.h"
#include "CommandLine.h"
#include "Tree.h"
#include "Instance.h"
#include "neighborSet.h"

namespace Timbl{
  class TimblExperiment;
  
  class Distribution;
  
  enum Algorithm { UNKNOWN_ALG, IB1, IB2, IGTREE, TRIBL, TRIBL2, LOO, CV };
  enum Weighting { UNKNOWN_W, UD, NW, GR, IG, X2, SV };
  
  class TimblOpts {
    friend class TimblAPI;
    friend std::ostream& operator<<( std::ostream&, const TimblOpts&  );
  public:
    TimblOpts( const int, const char * const * );
    TimblOpts( const std::string& );
    ~TimblOpts();
    bool Find( char, std::string&, bool& ) const;
    bool Find( const char *, std::string&, bool& ) const;
    bool Find( const std::string&, std::string&, bool& ) const;
    void Add( char, const std::string&, bool );
    bool Delete( char );
    bool Delete( const char * );
    bool Delete( const std::string& );
  private:
    CL_Options *pimpl;
    TimblOpts( const TimblOpts& );
    TimblOpts& operator=( const TimblOpts& );
  };
  
  class TimblAPI {
  public:
    TimblAPI( const TimblOpts *, const std::string& = "" );
    TimblAPI( const std::string&,  const std::string& = "" );
    TimblAPI( const TimblAPI& );
    ~TimblAPI();
    bool Valid() const;
    bool StartServer( const int, const int=10 );
    bool Prepare( const std::string& = "" );
    bool CVprepare( const std::string& = "",
		    Weighting = GR,
		    const std::string& = "" );
    bool Learn( const std::string& = "" );
    bool Increment( const std::string& );
    bool Decrement( const std::string& );
    bool Expand( const std::string& );
    bool Remove( const std::string& );
    bool Test( const std::string& = "", 
	       const std::string& = "",
	       const std::string& = "" );
    bool NS_Test( const std::string& = "", 
		  const std::string& = "" );
    const TargetValue *Classify( const std::string& );
    const TargetValue *Classify( const std::string&, 
				 const ValueDistribution *& );
    const TargetValue *Classify( const std::string&, double& );
    const TargetValue *Classify( const std::string&, 
				 const ValueDistribution *&, 
				 double& );
    const neighborSet *classifyNS( const std::string& );
    bool classifyNS( const std::string&, neighborSet& );
    bool Classify( const std::string&, std::string& );
    bool Classify( const std::string&, std::string&, double& );
    bool Classify( const std::string&, std::string&, 
		   std::string&, double& );
    bool ShowBestNeighbors( std::ostream& ) const;
    size_t matchDepth() const;
    bool matchedAtLeaf() const;
    std::string ExpName() const;
    static std::string VersionInfo( bool = false );
    bool SaveWeights( const std::string& = "" );
    bool GetWeights( const std::string& = "", Weighting = UNKNOWN_W  );
    Weighting CurrentWeighting() const;
    Weighting GetCurrentWeights( std::vector<double>& ) const;
    bool WriteInstanceBase( const std::string& = "" );
    bool WriteInstanceBaseXml( const std::string& = "" );
    bool WriteInstanceBaseLevels( const std::string& = "", unsigned int=0 );
    bool GetInstanceBase( const std::string& = "" );
    bool WriteArrays( const std::string& = "" );
    bool WriteMatrices( const std::string& = "" );
    bool GetArrays( const std::string& = "" );
    bool GetMatrices( const std::string& = "" );
    bool WriteNamesFile( const std::string& = "" );
    bool ShowWeights( std::ostream& ) const;
    bool ShowOptions( std::ostream& ) const;
    bool ShowSettings( std::ostream& ) const;
    bool ShowStatistics( std::ostream& ) const;
    bool SetOptions( const std::string& );
    bool SetIndirectOptions( const TimblOpts&  );
    bool Set_Single_Threaded();
    Algorithm Algo() const;
    InputFormatType getInputFormat() const;
    static int Default_Max_Feats();
    bool initExperiment();
  private:
    TimblAPI& operator=( const TimblAPI& ); // so nobody may use them
    TimblExperiment *pimpl;
    bool i_am_fine;
  }; 
  
  const std::string to_string( const Algorithm );
  const std::string to_string( const Weighting );
  bool string_to( const std::string&, Algorithm& );
  bool string_to( const std::string&, Weighting& );
  
}
#endif // TIMBLAPI_H
