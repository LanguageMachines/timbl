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
#include <map>
#include <string>
#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/MsgClass.h"
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/Tree.h"
#include "timbl/Instance.h"
#include "timbl/neighborSet.h"
#include "timbl/BestArray.h"
#include "timbl/Statistics.h"
#include "timbl/MBLClass.h"
#include "timbl/GetOptClass.h"

using namespace std;

#include "timbl/TimblAPI.h"
#include "timbl/ServerBase.h"
#include "timbl/TimblServerAPI.h"
#include "timbl/TimblExperiment.h"
namespace Timbl {
  
  TimblServer *CreateServerPimpl( AlgorithmType algo, GetOptClass *opt ){
    TimblServer *result = NULL;
    switch ( algo ){
    case IB1_a:
      result = new IB1_Server( opt );
      break;
    case IGTREE_a:
      result = new IG_Server( opt );
      break;
    case TRIBL_a:
      result = new TRIBL_Server( opt );
      break;
    case TRIBL2_a:
      result = new TRIBL2_Server( opt );
      break;
    default:
      cerr << "wrong algorithm to create TimblServerAPI" << endl;
      return NULL;
    }
    if ( result->exp ){
      if ( opt->getVerbosity() & CLIENTDEBUG )
	result->setDebug(true);
      return result;
    }
    else {
      delete result;
      return NULL;
    }
  }

  TimblServerAPI::TimblServerAPI( ):
    pimpl( 0 ), i_am_fine(false) {
  }
  
  TimblServerAPI::TimblServerAPI( TimblOpts *T_Opts ):
    pimpl(), i_am_fine(false) {
    if ( T_Opts ){
      GetOptClass *OptPars = new GetOptClass( *T_Opts->getPimpl() );
      if ( !OptPars->parse_options( *T_Opts->getPimpl() ) )
	delete OptPars;
      else if ( OptPars->Algo() != Unknown_a ){
	pimpl = CreateServerPimpl( OptPars->Algo(), OptPars );
      }
      else {
	pimpl = CreateServerPimpl( IB1_a, OptPars );
      }
    }
    i_am_fine = (pimpl != NULL);
  }
  
  TimblServerAPI::~TimblServerAPI(){ 
    delete pimpl; 
  }
  
  bool TimblServerAPI::Valid() const {
    return i_am_fine && pimpl && pimpl->exp && !pimpl->exp->ExpInvalid();
  }  
  
  inline Weighting WT_to_W( WeightType wt ){
    Weighting w;
    switch ( wt ){
    case UserDefined_w: w = UD;
      break;
    case No_w: w = NW;
      break;
    case GR_w: w = GR;
      break;
    case IG_w: w = IG;
      break;
    case X2_w: w = X2;
      break;
    case SV_w: w = SV;
      break;
    default:
      w = UNKNOWN_W;
    }
    return w;
  }
  
  Algorithm TimblServerAPI::Algo() const {
    Algorithm result = UNKNOWN_ALG;
    if ( pimpl ){
      switch ( pimpl->exp->Algorithm() ){
      case IB1_a:
	result = IB1;
	break;
      case IB2_a:
	result = IB2;
	break;
      case IGTREE_a:
	result = IGTREE;
	break;
      case TRIBL_a:
	result = TRIBL;
	break;
      case TRIBL2_a:
	result = TRIBL2;
	break;
      case LOO_a:
	result = LOO;
	break;
      case CV_a:
	result = CV;
	break;
      default:
	cerr << "invalid algorithm in switch " << endl;
	break;
      }
    }
    return result;
  }
  
  bool TimblServerAPI::Learn( const string& s ){
    if ( Valid() )
      return pimpl->exp->Learn( s );
    else 
      return false;
  }
  
  bool TimblServerAPI::Prepare( const string& s ){
    if ( Valid() )
      return pimpl->exp->Prepare( s );
    else
      return false;
  }
  
  bool TimblServerAPI::initExperiment( ){
    if ( Valid() ){
      pimpl->exp->initExperiment( true );
      return true;
    }
    else
      return false;
  }
  
  InputFormatType TimblServerAPI::getInputFormat() const {
    if ( Valid() )
      return pimpl->exp->InputFormat();
    else
      return UnknownInputFormat;
  }
  
  bool TimblServerAPI::GetWeights( const string& f, Weighting w ){
    if ( Valid() ){
      WeightType tmp;
      switch ( w ){
      case UNKNOWN_W: tmp = Unknown_w;
	break;
      case NW: tmp = No_w;
	break;
      case GR: tmp = GR_w;
	break;
      case IG: tmp = IG_w;
	break;
      case X2: tmp = X2_w;
	break;
      case SV: tmp = SV_w;
	break;
      default:
	return false;
      }
      return pimpl->exp->GetWeights( f, tmp );
    }
  else 
    return false;
  }
  
  Weighting TimblServerAPI::CurrentWeighting() const{
    if ( Valid() )
      return WT_to_W( pimpl->exp->CurrentWeighting() );
    else
      return UNKNOWN_W;
  }
  
  Weighting TimblServerAPI::GetCurrentWeights( std::vector<double>& res ) const {
    res.clear();
    if ( Valid() ){
      if ( pimpl->exp->GetCurrentWeights( res ) )
	return CurrentWeighting();
    }
    return UNKNOWN_W;
  }
  
  string TimblServerAPI::ExpName() const {
    if ( pimpl && pimpl->exp ) // return the name, even when !Valid()
      return pimpl->exp->ExpName();
    else
      return "ERROR";
  }
  
  bool TimblServerAPI::GetInstanceBase( const string& f ){
    if ( Valid() ){
      if ( !pimpl->exp->ReadInstanceBase( f ) )
	i_am_fine = false;
      return Valid();
    }
    else
      return false;
  }
  
  bool TimblServerAPI::GetArrays( const string& f ){
    if ( Valid() ){
      return pimpl->exp->GetArrays( f );
    }
    else
      return false;
  }
  
  bool TimblServerAPI::GetMatrices( const string& f ){
    return Valid() && pimpl->exp->GetMatrices( f );
  }
  
  bool TimblServerAPI::StartServer( const int port, const int max_c ){
    return Valid() && pimpl->startClassicServer( port, max_c );
  }
  
  bool TimblServerAPI::StartMultiServer( const string& config ){
    return Valid() && pimpl->startMultiServer( config );
  }
  
  string TimblServerAPI::VersionInfo( bool full ){
    return Common::VersionInfo( full );
  }
  
  int TimblServerAPI::Default_Max_Feats() {
    return Common::DEFAULT_MAX_FEATS;
  }
  
}
