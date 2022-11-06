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
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <cctype>
#include <algorithm>
#include "timbl/Common.h"
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/MsgClass.h"
#include "timbl/Metrics.h"
#include "ticcutils/CommandLine.h"
#include "timbl/GetOptClass.h"
#include "timbl/TimblExperiment.h"

using namespace std;

namespace Timbl {

  void GetOptClass::set_default_options( int Max ){
    local_algo = IB1_a;
    local_metric = UnknownMetric;
    local_order = UnknownOrdening;
    local_weight = Unknown_w;
    local_decay = Zero;
    local_decay_alfa = 1.0;
    local_decay_beta = 1.0;
    local_normalisation = unknownNorm;
    local_norm_factor = 1;
    no_neigh = 1;
    mvd_limit = 1;
    estimate = 0;
    maxbests = 500;
    BinSize = 0;
    BeamSize = 0;
    clip_freq = 10;
    clones = 1;
    bootstrap_lines = -1;
    local_progress = 100000;
    seed = -1;
    do_exact = false;
    do_hashed = true;
    min_present = false;
    keep_distributions = false;
    do_sample_weights = false;
    do_ignore_samples = false;
    do_ignore_samples_test = false;
    do_query = false;
    do_all_weights = false;
    do_sloppy_loo = false;
    do_silly = false;
    do_diversify = false;
    if ( MaxFeats == -1 ){
      MaxFeats = Max;
      LocalInputFormat = UnknownInputFormat; // InputFormat and verbosity
      myVerbosity = NO_VERB;   // are not reset!
    }
    target_pos = -1;
    metricsArray.resize(MaxFeats+1, UnknownMetric );
    outPath = "";
    occIn = 0;
  }

  GetOptClass::GetOptClass( const TiCC::CL_Options& opts ):
    LocalInputFormat( UnknownInputFormat ),
    MaxFeats(-1),
    target_pos(-1),
    f_length( 0 ),
    threshold( -1 ),
    igThreshold( -1 ),
    myVerbosity( NO_VERB ),
    opt_init( false ),
    opt_changed( false ),
    N_present( false ),
    parent_socket_os( 0 ) {
    int MaxF = DEFAULT_MAX_FEATS;
    string optie;
    if ( opts.is_present( 'N', optie ) ){
      N_present = true;
      MaxF = TiCC::stringTo<int>( optie );
    }
    set_default_options( MaxF );
  }

  GetOptClass::~GetOptClass( ){
  }

  GetOptClass::GetOptClass( const GetOptClass& in ):
    MsgClass(in),
    local_algo( in.local_algo ),
    local_metric( in.local_metric ),
    local_order( in.local_order ),
    local_weight( in.local_weight ),
    LocalInputFormat( in.LocalInputFormat ),
    local_decay( in.local_decay ),
    local_decay_alfa( in.local_decay_alfa ),
    local_decay_beta( in.local_decay_beta ),
    local_normalisation( in.local_normalisation ),
    local_norm_factor( in.local_norm_factor ),
    MaxFeats( in.MaxFeats ),
    target_pos( in.target_pos ),
    no_neigh( in.no_neigh ),
    mvd_limit( in.mvd_limit ),
    estimate( in.estimate ),
    maxbests( in.maxbests ),
    clip_freq( in.clip_freq ),
    clones( in.clones ),
    BinSize( in.BinSize ),
    BeamSize( in.BeamSize ),
    bootstrap_lines( in.bootstrap_lines ),
    f_length( in.f_length ),
    local_progress( in.local_progress ),
    seed( in.seed ),
    threshold( in.threshold ),
    igThreshold( in.igThreshold ),
    myVerbosity( in.myVerbosity ),
    opt_init( in.opt_init ),
    opt_changed( in.opt_changed ),
    do_exact( in.do_exact ),
    do_hashed( in.do_hashed ),
    min_present( in.min_present ),
    N_present(false),
    keep_distributions( in.keep_distributions ),
    do_sample_weights( in.do_sample_weights ),
    do_ignore_samples( in.do_ignore_samples ),
    do_ignore_samples_test( in.do_ignore_samples_test ),
    do_query( in.do_query ),
    do_all_weights( false ),
    do_sloppy_loo( false ),
    do_silly( in.do_silly ),
    do_diversify( in.do_diversify ),
    metricsArray( in.metricsArray ),
    parent_socket_os( in.parent_socket_os ),
    outPath( in.outPath ),
    occIn( in.occIn )
  {
  }

  GetOptClass *GetOptClass::Clone( ostream *sock_os ) const{
    GetOptClass *result = new GetOptClass(*this);
    result->parent_socket_os = sock_os;
    return result;
  }

  void GetOptClass::Error( const string& out_line ) const {
    if ( parent_socket_os ){
      *parent_socket_os << "ERROR { " << out_line << " }" << endl;
    }
    else {
      cerr << "Error:" << out_line << endl;
    }
  }

  bool GetOptClass::definitive_options( TimblExperiment *Exp ){
    if ( opt_changed || !opt_init ){
      opt_changed = false;
      bool first = !opt_init;
      opt_init = true;
      string optline;
      if ( first ){
	// the following options can only be set once!
	// If you try it anyway, you should get a MblClass warning...
	if ( LocalInputFormat == SparseBin ){
	  if ( !N_present ){
	    Error( "Missing -N option, mandatory for -F Binary" );
	    return false;
	  }
	}
	if ( LocalInputFormat == Sparse ){
	  if ( !N_present ){
	    Error( "Missing -N option, mandatory for -F Sparse" );
	    return false;
	  }
	}
	if ( LocalInputFormat != UnknownInputFormat ){
	  optline = "INPUTFORMAT: " + TiCC::toString(LocalInputFormat);
	  if ( !Exp->SetOption( optline ) ){
	    return false;
	  }
	}
	if ( target_pos != -1 ){
	  optline = "TARGET_POS: " + TiCC::toString<int>(target_pos-1);
	  if ( !Exp->SetOption( optline ) ){
	    return false;
	  }
	}
	if ( keep_distributions ){
	  optline = "KEEP_DISTRIBUTIONS: true";
	  if ( !Exp->SetOption( optline ) ){
	    return false;
	  }
	}
	if ( do_sloppy_loo ){
	  if ( local_algo != LOO_a ){
	    Error( "sloppy only valid for LOO algorithm" );
	    return false;
	  }
	  else {
	    optline = "DO_SLOPPY_LOO: true";
	    if ( !Exp->SetOption( optline ) ){
	      return false;
	    }
	  }
	}
	if ( do_silly ){
	  optline = "DO_SILLY: true";
	  if ( !Exp->SetOption( optline ) ){
	    return false;
	  }
	}
	if ( do_diversify ){
	  optline = "DO_DIVERSIFY: true";
	  if ( !Exp->SetOption( optline ) ){
	    return false;
	  }
	}
	if ( f_length > 0 ){
	  optline = "FLENGTH: " + TiCC::toString<int>(f_length);
	  if ( !Exp->SetOption( optline ) ){
	    return false;
	  }
	}
	if ( local_weight != Unknown_w ){
	  optline = "WEIGHTING: " + TiCC::toString(local_weight);
	  Exp->SetOption( optline );
	}
	if ( do_all_weights ){
	  optline = "ALL_WEIGHTS: true";
	  Exp->SetOption( optline );
	}
	optline = "MAXBESTS: " + TiCC::toString<int>(maxbests);
	Exp->SetOption( optline );
	if ( BinSize > 0 ){
	  optline = "BIN_SIZE: " + TiCC::toString<int>(BinSize);
	  Exp->SetOption( optline );
	}
	if ( BeamSize > 0 ){
	  optline = "BEAM_SIZE: " + TiCC::toString<int>(BeamSize);
	  Exp->SetOption( optline );
	}
	if ( local_algo == TRIBL_a && threshold < 0 ){
	  Error( "-q is missing for TRIBL algorithm" );
	  return false;
	}
	if ( threshold >= 0 ){
	  if ( local_algo != TRIBL_a ){
	    Error( "-q option only valid for TRIBL algorithm" );
	    return false;
	  }
	  if ( threshold == 0 ){
	    Error( "invalid -q option. Must be > 0 " );
	    return false;
	  }
	  optline = "TRIBL_OFFSET: " + TiCC::toString<int>(threshold);
	  Exp->SetOption( optline );
	}
	if ( igThreshold > 0 ){
	  optline = "IG_THRESHOLD: " + TiCC::toString<int>(igThreshold);
	  Exp->SetOption( optline );
	}
	if ( local_order != UnknownOrdening ){
	  optline = "TREE_ORDER: " + TiCC::toString(local_order);
	  Exp->SetOption( optline );
	}
	if ( !outPath.empty() ){
	  Exp->setOutPath( outPath );
	}
      } //first
      if ( clones > 0 ){
	Exp->Clones( clones );
      }
      if ( estimate < 10 ){
	Exp->Estimate( 0 );
      }
      else {
	Exp->Estimate( estimate );
      }
      if ( myVerbosity & CONFIDENCE ){
	if ( local_normalisation == unknownNorm ){
	  Error( "Invalid option +vcf, while -G is missing!" );
	  return false;
	}
      }
      if ( myVerbosity & DISTRIB ){
	if ( !keep_distributions && local_algo == IGTREE_a ){
	  myVerbosity &= ~DISTRIB;
	  Error( "Invalid option +vdb, while +D is missing!" );
	  return false;
	}
      }
      if ( myVerbosity & ALL_K ){
	if ( local_algo == IGTREE_a ){
	  Error( "Invalid option +vk, impossible with IGtree algorithm" );
	  return false;
	}
	else if ( !(myVerbosity & DISTRIB) ){
	  // silently add +vdb when +vk is set
	  myVerbosity |= DISTRIB;
	}
      }
      if ( myVerbosity & NEAR_N ){
	if ( local_algo == IGTREE_a ){
	  Error( "Invalid option +vn, impossible with IGtree algorithm" );
	  return false;
	}
      }
      if ( myVerbosity & CONF_MATRIX ||
	   myVerbosity & CLASS_STATS )
	myVerbosity |= ADVANCED_STATS;
      if ( do_exact ){
	Exp->SetOption(  "EXACT_MATCH: true" );
      }
      else {
	Exp->SetOption(  "EXACT_MATCH: false" );
      }
      if ( do_hashed ) {
	Exp->SetOption(  "HASHED_TREE: true" );
      }
      else {
	Exp->SetOption(  "HASHED_TREE: false" );
      }
      if ( occIn > 0 &&
	   do_sample_weights ){
	Error( "--occurrences and -s cannot be combined!" );
	return false;
      }
      if ( occIn > 0 ){
	Exp->SetOption( "HANDLE_OCCURRENCES: " + TiCC::toString(occIn) );
      }
      else if ( do_sample_weights ){
	Exp->SetOption(  "EXEMPLAR_WEIGHTS: true" );
	if ( do_ignore_samples ){
	  Exp->SetOption( "IGNORE_EXEMPLAR_WEIGHTS: true" );
	}
	else {
	  Exp->SetOption( "IGNORE_EXEMPLAR_WEIGHTS: false" );
	}
	if ( do_ignore_samples_test ){
	  Exp->SetOption( "NO_EXEMPLAR_WEIGHTS_TEST: true" );
	}
	else {
	  Exp->SetOption( "NO_EXEMPLAR_WEIGHTS_TEST: false" );
	}
      }
      else {
	Exp->SetOption(  "EXEMPLAR_WEIGHTS: false" );
      }
      if ( local_metric == UnknownMetric ){
	// Ok, so NO defaults at all (API usage for instance)
	local_metric = Overlap;
	fill( metricsArray.begin(), metricsArray.end(), Overlap );
      }
      optline = "GLOBAL_METRIC: " + TiCC::toString(local_metric);
      Exp->SetOption( optline );
      if ( bootstrap_lines > 0 ){
	optline = "IB2_OFFSET: " + TiCC::toString<int>(bootstrap_lines);
	Exp->SetOption( optline );
      }
      if ( local_normalisation != unknownNorm ){
	optline = "NORMALISATION: " + TiCC::toString<normType>( local_normalisation );
	Exp->SetOption( optline );
	if ( local_normalisation == addFactorNorm ){
	  optline = "NORM_FACTOR: " + TiCC::toString<double>( local_norm_factor );
	  Exp->SetOption( optline );
	}
      }
      optline = "MVD_LIMIT: " + TiCC::toString<int>(mvd_limit);
      Exp->SetOption( optline );
      optline = "NEIGHBORS: " + TiCC::toString<int>(no_neigh);
      if ( Exp->SetOption( optline ) ){
	optline = "DECAY: " + TiCC::toString(local_decay);
	if ( Exp->SetOption( optline ) ){
	  optline = "DECAYPARAM_A: " + TiCC::toString<double>(local_decay_alfa);
	  if ( Exp->SetOption( optline ) ){
	    optline = "DECAYPARAM_B: " + TiCC::toString<double>(local_decay_beta);
	    if ( Exp->SetOption( optline ) ){
	      optline = "CLIP_FACTOR: " + TiCC::toString<int>(clip_freq);
	      if ( Exp->SetOption( optline ) ){
		optline = "SEED: " + TiCC::toString<int>(seed);
		if ( Exp->SetOption( optline ) ){
		  optline = "PROGRESS: " + TiCC::toString<int>(local_progress);
		  if ( Exp->SetOption( optline ) ){
		    optline = "VERBOSITY: " +
		      TiCC::toString(myVerbosity);
		    if ( Exp->SetOption( optline ) ){
		      for ( size_t i=0; i < metricsArray.size(); ++i ){
			optline = "METRICS: " + TiCC::toString<int>( i ) + "=" +
			  TiCC::toString(metricsArray[i]);
			if (!Exp->SetOption( optline ) ){
			  Error( "changing metric is not possible at this stage" );
			  return false;
			}
		      }
		      if ( do_query ){
			Exp->ShowSettings( cerr );
			do_query = false;
		      }
		      return true;
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
      return false;
    }
    return true;
  }

  inline bool GetOptClass::parse_range( string& line,
					string::iterator& it,
					MetricType Value ){
    size_t m;
    while( it != line.end() && *it != ':' ){
      auto eit = it;
      while( eit != line.end() && isdigit( *eit ) ) ++eit;
      string tmp = string( it, eit );
      size_t k;
      if ( TiCC::stringTo<size_t>( tmp, k, 1, metricsArray.size() ) ){
	if ( metricsArray[k] != UnknownMetric && metricsArray[k] != Value ){
	  Error( "metric of feature " + tmp +
		 " is multiply changed!" );
	  return false;
	}
	metricsArray[k] = Value;
      }
      else {
	Error( "illegal value in metric description: -m " + line );
	return false;
      }
      it = eit;
      if ( it == line.end() ){
	return true;
      }
      else if ( *it == ',' ){
	++it;
      }
      else if ( *it == '-' ){
	++it;
	eit = it;
	while( eit != line.end() && isdigit( *eit ) ) ++eit;
	tmp = string( it, eit );
	m = TiCC::stringTo<int>(tmp);
	if ( m == 0 || m > metricsArray.size() ){
	  Error( "illegal value in metric description: -m " + line );
	  return false;
	}
	it = eit;
	if ( it != line.end() && (*it != ',' && *it != ':' ) ){
	  Error( "illegal value in metric description: -m " + line );
	  return false;
	}
	if ( m < k ){
	  Error( "illegal value in metric description: -m " + line );
	  return false;
	}
	else {
	  for ( size_t j=k+1; j <= m && j < metricsArray.size(); ++j ){
	    if ( metricsArray[j] != UnknownMetric
		 && metricsArray[j] != Value ){
	      Error( "metric of feature " + TiCC::toString<int>(j) +
		     " is multiply changed!" );
	      return false;
	    }
	    metricsArray[j] = Value;
	  }
	}
	if ( it != line.end() && *it == ',' ) {
	  ++it;
	}
      }
    }
    return true;
  }

  inline bool GetOptClass::parse_metrics( const string& Mline,
					  MetricType& Def ){
    string line = TiCC::trim( Mline );
    TiCC::to_upper( line );
    auto p = line.begin();
    if ( p != line.end() ){
      switch ( *p++ ){
      case 'O' :
	Def = Overlap;
	break;
      case 'J' :
	Def = JeffreyDiv;
	break;
      case 'S' :
	Def = JSDiv;
	break;
      case 'M' :
	Def = ValueDiff;
	break;
      case 'N' :
	Def = Numeric;
	break;
      case 'E' :
	Def = Euclidean;
	break;
      case 'D' :
	if ( p == line.end() || *p == ':' ){
	  Def = DotProduct;
	}
	else {
	  if ( *p == 'C' ){
	    Def = Dice;
	    ++p;
	  }
	}
	break;
      case 'C' :
	Def = Cosine;
	break;
      case 'L' :
	Def = Levenshtein;
	break;
      case 'I' :
	Def = Ignore;
	break;
      default:
	Error( "illegal default value for metric: -m " + Mline );
	return false;
      }
      if ( p == line.end() ){
	//
	// only -m options, no further specifications
	//
	if ( Def == Ignore ){
	  Error( "Ignore without further specification for metric: -m " + Mline );
	  return false;
	}
	else {
	  // set the defaults
	  fill( metricsArray.begin(), metricsArray.end(), Def );
	  return true;
	}
      }
      else if ( *p != ':' ){
	Error( "missing ':' after default value in -m option" );
	return false;
      }
      else {
	// deviating options expected. reset the array
	fill( metricsArray.begin(), metricsArray.end(), UnknownMetric );
	++p;
	MetricType TmpMT;
	while( p != line.end() ){
	  switch ( *p ){
	  case 'O' :
	    TmpMT = Overlap;
	    break;
	  case 'S' :
	    TmpMT = JSDiv;
	    break;
	  case 'J' :
	    TmpMT = JeffreyDiv;
	    break;
	  case 'D' :
	    if ( *(p+1) && *(p+1) == 'C' ){
	      ++p;
	      TmpMT = Dice;
	    }
	    else {
	      Error( "illegal value in metric description: -m " + Mline );
	      return false;
	    }
	    break;
	  case 'M' :
	    TmpMT = ValueDiff;
	    break;
	  case 'E' :
	    TmpMT = Euclidean;
	    break;
	  case 'N' :
	    TmpMT = Numeric;
	    break;
	  case 'I' :
	    TmpMT = Ignore;
	    break;
	  default:
	    Error( "illegal value in metric description: -m " + Mline );
	    return false;
	  }
	  metricClass *tmpMC = getMetricClass(Def);
	  if ( TmpMT != Ignore && tmpMC->isSimilarityMetric() ){
	    Error( "Similarity metric " + TiCC::toString( Def )
		   + " only accepts -I specifications: -m " + Mline );
	    delete tmpMC;
	    return false;
	  }
	  delete tmpMC;
	  ++p;
	  if ( !parse_range( line, p, TmpMT ) ){
	    return false;
	  }
	  if ( p == line.end() ){
	    break;
	  }
	  if ( *p != ':' ){
	    Error( "missing ':' in metric description" );
	    return false;
	  }
	  else {
	    ++p;
	  }
	}
	if ( p != line.end() ){
	  Error( "illegal value in metric description: -m " + Mline );
	  return false;
	}
	else {
	  //
	  // set defaults for those still unset
	  //
	  replace( metricsArray.begin(), metricsArray.end(),
		   UnknownMetric, Def );
	}
      }
      return true;
    }
    else {
      return false;
    }
  }

  inline bool isBoolOrEmpty( const string& in, bool& val ){
    if ( in.empty() ){
      val = true;
      return true;
    }
    else {
      string s = TiCC::uppercase( in );
      if ( s == "TRUE" || s == "YES" || s == "FALSE" || s == "NO" ){
	val = ( s == "TRUE" || s == "YES" );
	return true;
      }
    }
    return false;
  }

  bool GetOptClass::parse_options( const TiCC::CL_Options& opts,
				   const int mode ){
    opt_changed = true;
    //    cerr << "options: " << opts << endl;
    //    cerr << "mode: " << mode << endl;
    for ( auto const& curr_opt: opts ){
      //      cerr << "process " << curr_opt << endl;
      bool mood = curr_opt.get_mood();
      bool longOpt = curr_opt.is_long();
      string value = curr_opt.value();
      char opt_char = curr_opt.opt_char();
      string option = curr_opt.option();
      try {
	//	cerr << "try " << opt_char << endl;
	switch (opt_char) {
	case 'a':
	  {
	    AlgorithmType tmp_a = IB1_a;
	    if ( !TiCC::stringTo<AlgorithmType>( value, tmp_a ) ){
	      Error( "illegal -a value: " + value );
	      return false;
	    }
	    else if ( tmp_a != IB1_a ){
	      if ( local_algo == LOO_a || local_algo == CV_a ){
		Error( "only IB1 algorithm is allowed for: " + TiCC::toString(local_algo)  );
		return false;
	      }
	      else {
		local_algo = tmp_a;
	      }
	    }
	  }
	  break;

	case 'b':
	  bootstrap_lines = TiCC::stringTo<int>( value );
	  if ( bootstrap_lines < 1 ){
	    Error( "illegal value for -b option: " + value );
	    return false;
	  }
	  break;

	case 'B':
	  if ( longOpt ){
	    if ( option == "Beam" ){
	      if ( !TiCC::stringTo<int>( value, BeamSize )
		   || BeamSize <= 0 ){
		Error( "illegal value for --Beam option: " + value );
		return false;
	      }
	    }
	  }
	  else if ( value.find("eam") != string::npos ){
	    Error( "invalid option: Did you mean '--B" + value + "'?" );
	    return false;
	  }
	  else {
	    BinSize = TiCC::stringTo<int>( value );
	    if ( BinSize <= 1 ){
	      Error( "illegal value for -B option: " + value );
	      return false;
	    }
	  }
	  break;

	case 'c':
	  if ( longOpt ){
	    if ( option == "clones" ){
	      if ( !TiCC::stringTo<int>( value, clones )
		   || clones <= 0 ){
		Error( "invalid value for --clones option: '"
		       + value + "'" );
		return false;
	      }
	    }
	  }
	  else {
	    if ( !TiCC::stringTo<int>( value, clip_freq )
		 || clip_freq < 0 ){
	      Error( "illegal value for -c option: " + value );
	      return false;
	    }
	  }
	  break;

	case 'd': {
	  string::size_type pos1 = value.find( ":" );
	  if ( pos1 == string::npos ){
	    pos1 = value.find_first_of( "0123456789" );
	    if ( pos1 != string::npos ){
	      if ( ! ( TiCC::stringTo<DecayType>( string( value, 0, pos1 ),
					    local_decay ) &&
		       TiCC::stringTo<double>( string( value, pos1 ),
					 local_decay_alfa ) ) ){
		Error( "illegal value for -d option: " + value );
		return false;
	      }
	    }
	    else if ( !TiCC::stringTo<DecayType>( value, local_decay ) ){
	      Error( "illegal value for -d option: " + value );
	      return false;
	    }
	  }
	  else {
	    string::size_type pos2 = value.find( ':', pos1+1 );
	    if ( pos2 == string::npos ){
	      pos2 = value.find_first_of( "0123456789", pos1+1 );
	      if ( pos2 != string::npos ){
		if ( ! ( TiCC::stringTo<DecayType>( string( value, 0, pos1 ),
					      local_decay ) &&
			 TiCC::stringTo<double>( string( value, pos2 ),
					   local_decay_alfa ) ) ){
		  Error( "illegal value for -d option: " + value );
		  return false;
		}
	      }
	      else {
		Error( "illegal value for -d option: " + value );
		return false;
	      }
	    }
	    else {
	      if ( ! ( TiCC::stringTo<DecayType>( string( value, 0, pos1 ),
					    local_decay ) &&
		       TiCC::stringTo<double>( string( value, pos1+1, pos2-pos1-1 ),
					 local_decay_alfa ) &&
		       TiCC::stringTo<double>( string( value, pos2+1 ),
					 local_decay_beta ) ) ){
		Error( "illegal value for -d option: " + value );
		return false;
	      }
	    }
	  }
	  break;
	}

	case 'D':
	  if ( longOpt ){
	    if ( option == "Diversify" ){
	      do_diversify = true;
	    }
	    else {
	      Error( "invalid option: Did you mean '--Diversify' ?" );
	      return false;
	    }
	  }
	  else {
	    keep_distributions = mood;
	  }
	  break;

	case 'e':
	  if ( !TiCC::stringTo<int>( value, estimate )
	       || estimate < 0 ){
	    Error( "illegal value for -e option: " + value );
	    return false;
	  }
	  break;

	case 'F':
	  if ( !TiCC::stringTo<InputFormatType>( value, LocalInputFormat ) ){
	    Error( "illegal value for -F option: " + value );
	    return false;
	  }
	  break;

	case 'G':
	  if ( value.empty() ){
	    local_normalisation = probabilityNorm;
	  }
	  else {
	    string::size_type pos1 = value.find( ":" );
	    if ( pos1 == string::npos ){
	      local_normalisation = TiCC::stringTo<normType>( value );
	      local_norm_factor = 1;
	    }
	    else {
	      local_normalisation = TiCC::stringTo<normType>( string( value, 0, pos1 ) );
	      if ( !TiCC::stringTo<double>( string( value, pos1+1 ),
				      local_norm_factor ) ||
		   local_norm_factor < Epsilon ){
		Error( "illegal value for -G option: " + value );
		return false;
	      }
	    }
	    if ( local_normalisation == unknownNorm ){
	      Error( "illegal value for -G option: " + value );
	      return false;
	    }
	  }
	  break;

	case 'H':
	  do_hashed = mood;
	  break;

	case 'k':
	  if ( !TiCC::stringTo<int>( value, no_neigh )
	       || no_neigh <= 0 ){
	    Error( "illegal value for -k option: " + value );
	    return false;
	  }
	  break;

	case 'l':
	  if ( !TiCC::stringTo<int>( value, f_length )
	       || f_length <= 0 ){
	    Error( "illegal value for -l option: " + value );
	    return false;
	  }
	  break;

	case 'L': {
	  string::size_type pos1 = value.find( ":" );
	  if ( pos1 == string::npos ){
	    pos1 = value.find_first_of( "0123456789" );
	    if ( pos1 != string::npos ){
	      if ( !TiCC::stringTo<int>( value, mvd_limit )
		   || mvd_limit <= 0 ){
		Error( "illegal value for -L option: " + value );
		return false;
	      }
	    }
	  }
	  else {
	    if ( !TiCC::stringTo<int>( string( value, pos1+1 ), mvd_limit )
		 || mvd_limit <= 0 ){
	      Error( "illegal value for -L option: " + value );
	      return false;
	    }
	  }
	  break;
	}
	case 'm':
	  if ( !parse_metrics( value, local_metric ) ){
	    return false;
	  }
	  break;

	case 'M':
	  if ( !TiCC::stringTo<int>( value, maxbests )
	       || maxbests <= 0 ){
	    Error( "illegal value for -M option: " + value );
	    return false;
	  }
	  break;

	case 'N':
	  // skip previously parsed NumOfFeatures info.
	  break;

	case 'O':
	  outPath = value;
	  break;

	case 'o':
	  if ( longOpt ){
	    if ( option == "occurrences" ){
	      if ( value == "train" ){
		occIn = 1;
	      }
	      else if ( value == "test" ){
		occIn = 2;
	      }
	      else if ( value == "both" ){
		occIn = 3;
	      }
	      else {
		Error( "invalid --ocurrences value. (expected train,test or both)" );
		return false;
	      }
	    }
	  }
	  break;

	case 'p':
	  local_progress = TiCC::stringTo<int>( value );
	  break;

	case 'q':
	  threshold = TiCC::stringTo<int>( value );
	  break;

	case 'Q':
	  do_query = true;
	  break;

	case 'R':
	  if ( !TiCC::stringTo<int>( value, seed ) ){
	    Error( "Integer argument for Random Seed expected (-R option)" );
	    return false;
	  }
	  break;

	case 's':
	  if ( longOpt ){
	    if ( option == "sloppy" ){
	      bool val;
	      if ( !isBoolOrEmpty(value,val) ){
		Error( "invalid value for sloppy: '"
		       + value + "'" );
		return false;
	      }
	      do_sloppy_loo = val;
	    }
	    else if ( option == "silly" ){
	      bool val;
	      if ( !isBoolOrEmpty(value,val) ){
		Error( "invalid value for silly: '"
		       + value + "'" );
		return false;
	      }
	      do_silly = val;
	    }
	  }
	  else { //short opt, so -s
	    if ( value.empty() ){
	      do_sample_weights = true;
	    }
	    else {
	      int val;
	      if ( TiCC::stringTo<int>( value, val ) ){
		if ( val == 0 ){
		  do_ignore_samples = true;
		  do_ignore_samples_test = false;
		  do_sample_weights = true;
		}
		else if ( val == 1 ){
		  do_ignore_samples_test = true;
		  do_sample_weights = true;
		}
	      }
	      if ( !do_sample_weights) {
		Error( "invalid value for -s: '" + value + "' (maybe you meant --s" + value + " ?)" );
		return false;
	      }
	    }
	  }
	  break;

	case 't':
	  {
	    AlgorithmType tmp_a = IB1_a;
	    if ( compare_nocase( value, "leave_one_out" ) ){
	      tmp_a = LOO_a;
	    }
	    else if ( compare_nocase( value, "cross_validate" ) ){
	      tmp_a = CV_a;
	    }
	    if ( local_algo != IB1_a && tmp_a != IB1_a ){
	      Error( "only IB1 algorithm is allowed for: " + TiCC::toString(tmp_a)  );
	      return false;
	    }
	    local_algo = tmp_a;
	  }
	  break;
	case 'T': {
	  if ( longOpt ){
	    if ( option == "Threshold" ){
	      if ( !TiCC::stringTo<int>(value, igThreshold )
		   || igThreshold < 0 ){
		Error( "invalid value for Threshold: " + value );
		return false;
	      }
	    }
	    else if ( option == "Treeorder" ){
	      if ( !TiCC::stringTo<OrdeningType>( value, local_order ) ){
		Error( "invalid value for Treeorder: " + value );
		return false;
	      }
	    }
	  }
	  else if ( value.find("hreshold") != string::npos ||
		    value.find("reeorder") != string::npos ){
	    Error( "invalid option: Did you mean '--T" + value + "' ?" );
	    return false;
	  }
	  else if ( !TiCC::stringTo<int>( value, target_pos )
		    || target_pos <= 0 ){
	    Error( "illegal value for -T option: " + value );
	    return false;
	  }
	}
	  break;

	case 'v':{
	  VerbosityFlags Flag = NO_VERB;
	  if ( !TiCC::stringTo<VerbosityFlags>( value, Flag ) ){
	    Error( "illegal value for +/- v option: " + value );
	    return false;
	  }
	  else {
	    if ( mode == 2 &&
		 ( !(Flag & (SILENT|DISTANCE|DISTRIB|NEAR_N|CONF_MATRIX) ) ) ){
	      Error( "-v option: " + TiCC::toString(Flag) +
		     " is not allowed at this stage." );
	      return false;
	    }
	    else if ( Flag > 0 ){
	      if ( mood ){
		myVerbosity |= Flag;
	      }
	      else {
		myVerbosity &= ~Flag;
	      }
	    }
	    else {
	      myVerbosity = NO_VERB;
	    }
	  }
	}
	  break;

	case 'w': {
	  if ( !TiCC::stringTo<WeightType>( value, local_weight ) )
	    return false;
	};
	  break;

	case 'W': {
	  do_all_weights = true;
	};
	  break;

	case 'x':
	  do_exact = mood;
	  break;

	default:
	  Warning( string("unhandled option: ") + opt_char + " " + value );
	}
      }
      catch( std::runtime_error& err ) {
	Error( string("invalid value for option '-") + opt_char + "' ("
	       + value + ")" );
	return false;
      }
    }
    return true;
  }

}
