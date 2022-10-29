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

#ifndef TIMBL_API_H
#define TIMBL_API_H

#include <string>
#include <vector>
#include "ticcutils/CommandLine.h"
#include "timbl/Common.h"
#include "timbl/MsgClass.h"
#include "timbl/Types.h"
#include "timbl/Instance.h"
#include "timbl/neighborSet.h"
#include "timbl/TimblExperiment.h"

namespace Timbl{

  inline std::string Version() { return Common::Version(); }
  inline std::string VersionName() { return Common::VersionName(); }
  inline std::string BuildInfo() { return Common::BuildInfo(); }

  enum Algorithm { UNKNOWN_ALG, IB1, IB2, IGTREE, TRIBL, TRIBL2, LOO, CV };
  enum Weighting { UNKNOWN_W, UD, NW, GR, IG, X2, SV, SD };

  class TimblAPI {
    friend class TimblExperiment;
  public:
    TimblAPI( const TiCC::CL_Options&, const std::string& = "" );
    TimblAPI( const std::string&,  const std::string& = "" );
    TimblAPI( const TimblAPI& );
    ~TimblAPI();
    bool isValid() const;
    bool Valid() const;
    TimblExperiment *grabAndDisconnectExp(){
      TimblExperiment *res = 0;
      if ( Valid() ){
	res = pimpl;
	pimpl = 0;
      }
      return res;
    }
    bool Prepare( const std::string& = "" );
    bool CVprepare( const std::string& = "",
		    Weighting = GR,
		    const std::string& = "" );
    bool Learn( const std::string& = "" );
    bool Increment_u( const icu::UnicodeString& );
    bool Increment( const std::string& );
    bool Decrement_u( const icu::UnicodeString& );
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
    const Instance *lastHandledInstance() const;
    const Targets *myTargets() const;
    bool Classify( const std::string&,
		   std::string& );
    bool Classify( const std::string&,
		   std::string&,
		   double& );
    bool Classify( const std::string&,
		   std::string&,
		   std::string&,
		   double& );
    bool Classify( const std::string&,
		   icu::UnicodeString& );
    bool Classify( const std::string&,
		   icu::UnicodeString&,
		   double& );
    bool Classify( const std::string&,
		   icu::UnicodeString&,
		   std::string&,
		   double& );
    bool ShowBestNeighbors( std::ostream& ) const;
    size_t matchDepth() const;
    double confidence() const;
    bool matchedAtLeaf() const;
    std::string ExpName() const;
    static std::string VersionInfo( bool = false ); //obsolete
    bool SaveWeights( const std::string& = "" );
    bool GetWeights( const std::string& = "", Weighting = UNKNOWN_W  );
    double GetAccuracy();
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
    bool ShowIBInfo( std::ostream& ) const;
    bool ShowStatistics( std::ostream& ) const;
    bool SetOptions( const std::string& );
    bool SetIndirectOptions( const TiCC::CL_Options&  );
    bool SetThreads( int c );
    std::string extract_limited_m( int ) const;
    Algorithm Algo() const;
    InputFormatType getInputFormat() const;
    size_t NumOfFeatures() const;
    static size_t Default_Max_Feats();
    bool initExperiment();
  private:
    TimblAPI();
    TimblAPI& operator=( const TimblAPI& ); // so nobody may use them
    TimblExperiment *pimpl;
    bool i_am_fine;
  };

  const std::string to_string( const Algorithm );
  const std::string to_string( const Weighting );
  bool string_to( const std::string&, Algorithm& );
  bool string_to( const std::string&, Weighting& );

}
#endif // TIMBL_API_H
