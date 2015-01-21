/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2015
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

#ifndef TIMBL_API_H
#define TIMBL_API_H

#include <string>
#include <vector>
#include "timbl/Common.h"
#include "timbl/MsgClass.h"
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/CommandLine.h"
#include "timbl/Instance.h"
#include "timbl/neighborSet.h"
#include "timbl/TimblExperiment.h"
#include "ticcutils/CommandLine.h"

namespace Timbl{

  inline std::string Version() { return Common::Version(); }
  inline std::string VersionName() { return Common::VersionName(); }
  inline std::string BuildInfo() { return Common::BuildInfo(); }

  enum Algorithm { UNKNOWN_ALG, IB1, IB2, IGTREE, TRIBL, TRIBL2, LOO, CV };
  enum Weighting { UNKNOWN_W, UD, NW, GR, IG, X2, SV, SD };

  class TimblOpts {
    friend class TimblAPI;
    friend std::ostream& operator<<( std::ostream&, const TimblOpts&  );
  public:
    TimblOpts( const int, const char * const * );
    TimblOpts( const std::string& );
    ~TimblOpts();
    bool Find( char, std::string&, bool& ) const;
    bool Find( const std::string&, std::string& ) const;
    bool Find( const std::string&, std::string&, bool& ) const;
    void Add( char, const std::string&, bool );
    void Add( const std::string&, const std::string& );
    bool Delete( char );
    bool Delete( const std::string& );
    CL_Options *getPimpl()const { return pimpl; };
  private:
    CL_Options *pimpl;
    TimblOpts( const TimblOpts& );
    TimblOpts& operator=( const TimblOpts& );
  };

  class TimblAPI {
    friend class TimblExperiment;
  public:
    TimblAPI( const TimblOpts *, const std::string& = "" );
    TimblAPI( const TiCC::CL_Options&, const std::string& = "" );
    TimblAPI( const std::string&,  const std::string& = "" );
    TimblAPI( const TimblAPI& );
    ~TimblAPI();
    bool isValid() const;
    bool Valid() const;
    bool StartServer( const int, const int=10 );
    bool StartMultiServer( const std::string& );
    TimblExperiment *grabAndDisconnectExp(){
      TimblExperiment *res = 0;
      if ( Valid() )
	res = pimpl;
      pimpl = 0;
      return res;
    }
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
    const Instance *lastHandledInstance() const;
    const Target *myTargets() const;
    bool Classify( const std::string&, std::string& );
    bool Classify( const std::string&, std::string&, double& );
    bool Classify( const std::string&, std::string&,
		   std::string&, double& );
    bool ShowBestNeighbors( std::ostream& ) const;
    size_t matchDepth() const;
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
    bool SetIndirectOptions( const TimblOpts&  );
    bool SetThreads( int c );
    Algorithm Algo() const;
    InputFormatType getInputFormat() const;
    static int Default_Max_Feats();
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
