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

#include <string>
#include "timbl/Common.h"
#include "timbl/MsgClass.h"
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/Instance.h"
#include "timbl/neighborSet.h"
#include "timbl/BestArray.h"
#include "timbl/Statistics.h"
#include "timbl/MBLClass.h"
#include "timbl/GetOptClass.h"
#include "ticcutils/CommandLine.h"
#include "timbl/TimblAPI.h"
#include "timbl/TimblExperiment.h"


using namespace std;
using namespace icu;

namespace Timbl {

  TimblExperiment *Create_Pimpl( AlgorithmType algo, const string& ex_name,
				 GetOptClass *opt ){
    TimblExperiment *result = NULL;
    switch ( algo ){
    case IB1_a:
      result = new IB1_Experiment( opt->MaxFeatures(), ex_name );
      break;
    case IB2_a:
      result = new IB2_Experiment( opt->MaxFeatures(), ex_name );
      break;
    case IGTREE_a:
      result = new IG_Experiment( opt->MaxFeatures(), ex_name );
      break;
    case TRIBL_a:
      result = new TRIBL_Experiment( opt->MaxFeatures(), ex_name );
      break;
    case TRIBL2_a:
      result = new TRIBL2_Experiment( opt->MaxFeatures(), ex_name );
      break;
    case LOO_a:
      result = new LOO_Experiment( opt->MaxFeatures(), ex_name );
      break;
    case CV_a:
      result = new CV_Experiment( opt->MaxFeatures(), ex_name );
      break;
    default:
      cerr << "wrong algorithm to create TimblAPI" << endl;
      return NULL;
    }
    result->setOptParams( opt );
    return result;
  }

  TimblAPI::TimblAPI( const TimblAPI& exp ):
    pimpl( exp.pimpl->splitChild() ), i_am_fine(true) {
  }

  TimblAPI::TimblAPI( ):
    pimpl( 0 ), i_am_fine(false) {
  }

  TimblAPI::TimblAPI( const TiCC::CL_Options& opts,
		      const string& name ):
    pimpl(), i_am_fine(false) {
    GetOptClass *OptPars = new GetOptClass( opts );
    if ( !OptPars->parse_options( opts ) ){
      delete OptPars;
    }
    else if ( OptPars->Algo() != Unknown_a ){
      pimpl = Create_Pimpl( OptPars->Algo(), name, OptPars );
    }
    else {
      pimpl = Create_Pimpl( IB1_a, name, OptPars );
    }
    i_am_fine = (pimpl != NULL);
  }

  TimblAPI::TimblAPI( const string& pars,
		      const string& name ):
    pimpl(), i_am_fine(false){
    TiCC::CL_Options Opts;
    Opts.init( pars );
    GetOptClass *OptPars = new GetOptClass( Opts );
    if ( OptPars->parse_options( Opts ) ){
      if ( OptPars->Algo() != Unknown_a ){
	pimpl = Create_Pimpl( OptPars->Algo(), name, OptPars );
      }
      else {
	pimpl = Create_Pimpl( IB1_a, name, OptPars );
      }
    }
    i_am_fine = (pimpl != NULL);
  }

  TimblAPI::~TimblAPI(){
    delete pimpl;
  }

  bool TimblAPI::Valid() const {
    return i_am_fine && pimpl && !pimpl->ExpInvalid();
  }

  bool TimblAPI::isValid() const {
    return i_am_fine && pimpl && !pimpl->ExpInvalid(false);
  }

  const string to_string( const Algorithm A ) {
    string result;
    switch ( A ){
    case IB1:
      result = "IB1";
      break;
    case IB2:
      result = "IB2";
      break;
    case IGTREE:
      result = "IGTREE";
      break;
    case TRIBL:
      result = "TRIBL";
      break;
    case TRIBL2:
      result = "TRIBL2";
      break;
    case LOO:
      result = "LOO";
      break;
    case CV:
      result = "CV";
      break;
    default:
      cerr << "invalid algorithm in switch " << endl;
      result = "Unknown Algorithm";
    }
    return result;
  }

  bool string_to( const string& s, Algorithm& A ){
    A = UNKNOWN_ALG;
    AlgorithmType tmp;
    if ( TiCC::stringTo<AlgorithmType>( s, tmp ) ){
      switch ( tmp ){
      case IB1_a: A = IB1;
	break;
      case IB2_a: A = IB2;
	break;
      case IGTREE_a: A = IGTREE;
	break;
      case TRIBL_a: A = TRIBL;
	break;
      case TRIBL2_a: A = TRIBL2;
	break;
      case LOO_a: A = LOO;
	break;
      case CV_a: A = CV;
	break;
      default:
	return false;
      }
      return true;
    }
    return false;
  }

  const string to_string( const Weighting W ) {
    string result;
    switch ( W ){
    case UD:
      result = "ud";
      break;
    case NW:
      result = "nw";
      break;
    case GR:
      result = "gr";
      break;
    case IG:
      result = "ig";
      break;
    case X2:
      result = "x2";
      break;
    case SV:
      result = "sv";
      break;
    case SD:
      result = "sd";
      break;
    default:
      cerr << "invalid Weighting in switch " << endl;
      result = "Unknown Weight";
    }
    return result;
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
    case SD_w: w = SD;
      break;
    default:
      w = UNKNOWN_W;
    }
    return w;
  }

  bool string_to( const string& s, Weighting& w ){
    w = UNKNOWN_W;
    WeightType tmp;
    if ( TiCC::stringTo<WeightType>( s, tmp ) ){
      w = WT_to_W( tmp );
      if ( w == UNKNOWN_W ){
	return false;
      }
      return true;
    }
    return false;
  }

  Algorithm TimblAPI::Algo() const {
    Algorithm result = UNKNOWN_ALG;
    if ( pimpl ){
      switch ( pimpl->Algorithm() ){
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

  bool TimblAPI::Learn( const string& s ){
    if ( Valid() ){
      return pimpl->Learn( s );
    }
    else {
      return false;
    }
  }

  bool TimblAPI::Prepare( const string& s ){
    if ( Valid() ){
      return pimpl->Prepare( s );
    }
    else {
      return false;
    }
  }

  bool TimblAPI::CVprepare( const string& wf, Weighting w, const string& pf ){
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
      case SD: tmp = SD_w;
	break;
      default:
	return false;
      }
      return pimpl->CVprepare( wf, tmp, pf );
    }
    else {
      return false;
    }
  }


  bool TimblAPI::Increment_u( const UnicodeString& us ){
    return Valid() && pimpl->Increment( us );
  }

  bool TimblAPI::Increment( const string& s ){
    return Valid() && pimpl->Increment( TiCC::UnicodeFromUTF8(s) );
  }

  bool TimblAPI::Decrement_u( const UnicodeString& us ){
    return Valid() && pimpl->Decrement( us );
  }
  bool TimblAPI::Decrement( const string& s ){
    return Valid() && pimpl->Decrement( TiCC::UnicodeFromUTF8(s) );
  }

  bool TimblAPI::Expand( const string& s ){
    return Valid() && pimpl->Expand( s );
  }

  bool TimblAPI::Remove( const string& s ){
    return Valid() && pimpl->Remove( s );
  }

  bool TimblAPI::Test( const string& in,
		       const string& out,
		       const string& p ){
    if ( !Valid() ){
      return false;
    }
    else {
      if ( in.empty() ){
	return false;
      }
      if ( out.empty() && Algo() != CV ){
	return false;
      }
      if ( !pimpl->Test( in, out ) ){
	return false;
      }
      return pimpl->createPercFile( p );
    }
  }

  bool TimblAPI::NS_Test( const string& in,
			  const string& out ){
    if ( !Valid() ){
      return false;
    }
    else {
      if ( in.empty() ){
	return false;
      }
      if ( out.empty() && Algo() != CV ){
	return false;
      }
      return pimpl->NS_Test( in, out );
    }
  }

  const TargetValue *TimblAPI::Classify( const string& s,
					 const ClassDistribution *& db,
					 double& di ){
    if ( Valid() ){
      return pimpl->Classify( s, db, di );
    }
    else {
      db = NULL;
      di = DBL_MAX;
    }
    return NULL;
  }

  const TargetValue *TimblAPI::Classify( const string& s ){
    if ( Valid() ){
      return pimpl->Classify( s );
    }
    return NULL;
  }

  const TargetValue *TimblAPI::Classify( const string& s,
					 const ClassDistribution *& db ){
    if ( Valid() ){
      return pimpl->Classify( s, db  );
    }
    else {
      db = NULL;
    }
    return NULL;
  }

  const TargetValue *TimblAPI::Classify( const string& s,
					 double& di ){
    if ( Valid() ){
      return pimpl->Classify( s, di );
    }
    else {
      di = DBL_MAX;
    }
    return NULL;
  }

  const neighborSet *TimblAPI::classifyNS( const string& s ){
    const neighborSet *ns = 0;
    if ( Valid() ){
      ns = pimpl->NB_Classify( s );
    }
    return ns;
  }

  bool TimblAPI::classifyNS( const string& s, neighborSet& ns ){
    const neighborSet *b = classifyNS( s );
    if ( b != 0 ){
      ns = *b;
      return true;
    }
    return false;
  }

  const Instance *TimblAPI::lastHandledInstance() const {
    if ( Valid() ){
      return &pimpl->CurrInst;
    }
    return 0;
  }

  const Targets& TimblAPI::myTargets() const{
    if ( Valid() ){
      return pimpl->targets;
    }
    abort();
  }

  bool TimblAPI::Classify( const string& s, string& cls ){
    return Valid() && pimpl->Classify( s, cls );
  }

  bool TimblAPI::Classify( const string& s, string& cls, double &f ) {
    return Valid() && pimpl->Classify( s, cls, f );
  }

  bool TimblAPI::Classify( const string& s, string& cls,
			   string& dist, double &f ){
    return Valid() && pimpl->Classify( s, cls, dist, f );
  }

  bool TimblAPI::Classify( const string& s, UnicodeString& cls ){
    return Valid() && pimpl->Classify( s, cls );
  }

  bool TimblAPI::Classify( const string& s, UnicodeString& cls, double &f ) {
    return Valid() && pimpl->Classify( s, cls, f );
  }

  bool TimblAPI::Classify( const string& s,
			   UnicodeString& cls,
			   string& dist, double &f ){
    return Valid() && pimpl->Classify( s, cls, dist, f );
  }

  size_t TimblAPI::matchDepth() const {
    if ( Valid() ){
      return pimpl->matchDepth();
    }
    else {
      return -1;
    }
  }

  double TimblAPI::confidence() const {
    if ( Valid() ){
      return pimpl->confidence();
    }
    else {
      return -1;
    }
  }

  bool TimblAPI::matchedAtLeaf() const {
    return  Valid() && pimpl->matchedAtLeaf();
  }

  bool TimblAPI::initExperiment( ){
    if ( Valid() ){
      pimpl->initExperiment( true );
      return true;
    }
    else {
      return false;
    }
  }

  InputFormatType TimblAPI::getInputFormat() const {
    if ( Valid() ){
      return pimpl->InputFormat();
    }
    else {
      return UnknownInputFormat;
    }
  }

  bool TimblAPI::SaveWeights( const string& f ){
    if ( Valid() ){
      return pimpl->SaveWeights( f );
    }
    else {
      return false;
    }
  }

  bool TimblAPI::GetWeights( const string& f, Weighting w ){
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
      case SD: tmp = SD_w;
	break;
      default:
	return false;
      }
      return pimpl->GetWeights( f, tmp );
    }
    else {
      return false;
    }
  }

  double TimblAPI::GetAccuracy() {
    if (Valid()) {
        return pimpl->stats.testedCorrect()/(double) pimpl->stats.dataLines();
    }
    else {
      return -1;
    }
  }

  Weighting TimblAPI::CurrentWeighting() const{
    if ( Valid() ){
      return WT_to_W( pimpl->CurrentWeighting() );
    }
    else {
      return UNKNOWN_W;
    }
  }

  Weighting TimblAPI::GetCurrentWeights( std::vector<double>& res ) const {
    res.clear();
    if ( Valid() &&
	 pimpl->GetCurrentWeights( res ) ){
      return CurrentWeighting();
    }
    return UNKNOWN_W;
  }

  bool TimblAPI::SetOptions( const string& argv ){
    return Valid() && pimpl->SetOptions( argv );
  }

  bool TimblAPI::SetIndirectOptions( const TiCC::CL_Options& opts ){
    return Valid() && pimpl->IndirectOptions( opts );
  }

  string TimblAPI::ExpName() const {
    if ( pimpl ) {
      // return the name, even when !Valid()
      return pimpl->ExpName();
    }
    else {
      return "ERROR";
    }
  }

  bool TimblAPI::WriteNamesFile( const string& f ){
    if ( Valid() ) {
      return pimpl->WriteNamesFile( f );
    }
    else {
      return false;
    }
  }

  bool TimblAPI::WriteInstanceBase( const string& f ){
    if ( Valid() ){
      return pimpl->WriteInstanceBase( f );
    }
    else {
      return false;
    }
  }

  bool TimblAPI::WriteInstanceBaseXml( const string& f ){
    if ( Valid() ){
      return pimpl->WriteInstanceBaseXml( f );
    }
    else {
      return false;
    }
  }

  bool TimblAPI::WriteInstanceBaseLevels( const string& f, unsigned int l ){
    if ( Valid() ){
      return pimpl->WriteInstanceBaseLevels( f, l );
    }
    else {
      return false;
    }
  }

  bool TimblAPI::GetInstanceBase( const string& f ){
    if ( Valid() ){
      if ( !pimpl->ReadInstanceBase( f ) ){
	i_am_fine = false;
      }
      return Valid();
    }
    else {
      return false;
    }
  }

  bool TimblAPI::WriteArrays( const string& f ){
    if ( Valid() ){
      return pimpl->WriteArrays( f );
    }
    else {
      return false;
    }
  }

  bool TimblAPI::GetArrays( const string& f ){
    if ( Valid() ){
      return pimpl->GetArrays( f );
    }
    else {
      return false;
    }
  }

  bool TimblAPI::WriteMatrices( const string& f ){
    return Valid() && pimpl->WriteMatrices( f );
  }

  bool TimblAPI::GetMatrices( const string& f ){
    return Valid() && pimpl->GetMatrices( f );
  }

  bool TimblAPI::ShowBestNeighbors( ostream& os ) const{
    return Valid() && pimpl->showBestNeighbors( os );
  }

  bool TimblAPI::ShowWeights( ostream& os ) const{
    return Valid() && pimpl->ShowWeights( os );
  }

  bool TimblAPI::ShowOptions( ostream& os ) const{
    return Valid() && pimpl->ShowOptions( os );
  }

  bool TimblAPI::ShowSettings( ostream& os ) const{
    return Valid() && pimpl->ShowSettings( os );
  }

  bool TimblAPI::ShowIBInfo( ostream& os ) const{
    if ( Valid() ){
      pimpl->IBInfo( os );
      return true;
    }
    else {
      return false;
    }
  }

  bool TimblAPI::ShowStatistics( ostream& os ) const{
    return Valid() && pimpl->showStatistics( os );
  }

  string TimblAPI::extract_limited_m( int lim ) const {
    if ( Valid() ){
      return pimpl->extract_limited_m( lim );
    }
    else {
      return "error";
    }
  }

  string TimblAPI::VersionInfo( bool full ){
    // obsolete
    return Common::VersionInfo( full );
  }

  size_t TimblAPI::Default_Max_Feats(){
    return Common::DEFAULT_MAX_FEATS;
  }

  size_t TimblAPI::NumOfFeatures() const {
    if ( Valid() ){
      return pimpl->NumOfFeatures();
    }
    else {
      return -1;
    }
  }

}
