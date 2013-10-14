/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2013
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
#include <string>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <cctype>
#include <sys/time.h>
#include "timbl/Common.h"
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/Instance.h"
#include "timbl/Metrics.h"
#include "timbl/CommandLine.h"
#include "timbl/GetOptClass.h"
#include "timbl/TimblExperiment.h"

using namespace std;
using namespace TiCC;

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
    do_daemon = true;
    if ( MaxFeats == -1 ){
      MaxFeats = Max;
      LocalInputFormat = UnknownInputFormat; // InputFormat and verbosity
      myVerbosity = NO_VERB;   // are not reset!
    }
    target_pos = -1;
    metricsArray.resize(MaxFeats+1);
    for ( int i=0; i < MaxFeats+1; ++i ){
      metricsArray[i] = UnknownMetric;
    }
    outPath = "";
    logFile = "";
    pidFile = "";
    occIn = 0;
  }

  GetOptClass::GetOptClass( CL_Options& Opts ):
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
    bool the_mood;
    string optie;
    if ( Opts.Find( 'N', optie, the_mood ) ){
      N_present = true;
      MaxF = stringTo<int>( optie );
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
    do_daemon( in.do_daemon ),
    metricsArray( in.metricsArray ),
    parent_socket_os( in.parent_socket_os ),
    outPath( in.outPath ),
    logFile( in.logFile ),
    pidFile( in.pidFile ),
    occIn( in.occIn )
  {
  }

  GetOptClass *GetOptClass::Clone( ostream *sock_os ) const{
    GetOptClass *result = new GetOptClass(*this);
    result->parent_socket_os = sock_os;
    return result;
  }

  void GetOptClass::Error( const string& out_line ) const {
    if ( parent_socket_os )
      *parent_socket_os << "ERROR { " << out_line << " }" << endl;
    else {
      cerr << "Error:" << out_line << endl;
    }
  }

  bool GetOptClass::definitive_options( TimblExperiment *Exp ){
    if ( opt_changed || !opt_init ){
      opt_changed = false;
      bool first = !opt_init;
      if ( !opt_init )
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
	  optline = "INPUTFORMAT: " + toString(LocalInputFormat);
	  if (!Exp->SetOption( optline ))
	    return false;
	}
	if ( target_pos != -1 ){
	  optline = "TARGET_POS: " + toString<int>(target_pos-1);
	  if (!Exp->SetOption( optline ))
	    return false;
	}
	if ( keep_distributions ){
	  optline = "KEEP_DISTRIBUTIONS: true";
	  if (!Exp->SetOption( optline ))
	    return false;
	}
	if ( do_sloppy_loo ){
	  if ( local_algo != LOO_a ){
	    Error( "sloppy only valid for LOO algorithm" );
	    return false;
	  }
	  else {
	    optline = "DO_SLOPPY_LOO: true";
	    if (!Exp->SetOption( optline ))
	      return false;
	  }
	}
	if ( do_silly ){
	  optline = "DO_SILLY: true";
	  if (!Exp->SetOption( optline ))
	    return false;
	}
	if ( do_diversify ){
	  optline = "DO_DIVERSIFY: true";
	  if (!Exp->SetOption( optline ))
	    return false;
	}
	if ( f_length > 0 ){
	  optline = "FLENGTH: " + toString<int>(f_length);
	  if (!Exp->SetOption( optline ))
	    return false;
	}
	if ( local_weight != Unknown_w ){
	  optline = "WEIGHTING: " + toString(local_weight);
	  Exp->SetOption( optline );
	}
	if ( do_all_weights ){
	  optline = "ALL_WEIGHTS: true";
	  Exp->SetOption( optline );
	}
	optline = "MAXBESTS: " + toString<int>(maxbests);
	Exp->SetOption( optline );
	if ( BinSize > 0 ){
	  optline = "BIN_SIZE: " + toString<int>(BinSize);
	  Exp->SetOption( optline );
	}
	if ( BeamSize > 0 ){
	  optline = "BEAM_SIZE: " + toString<int>(BeamSize);
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
	  optline = "TRIBL_OFFSET: " + toString<int>(threshold);
	  Exp->SetOption( optline );
	}
	if ( igThreshold > 0 ){
	  optline = "IG_THRESHOLD: " + toString<int>(igThreshold);
	  Exp->SetOption( optline );
	}
	if ( local_order != UnknownOrdening ){
	  optline = "TREE_ORDER: " + toString(local_order);
	  Exp->SetOption( optline );
	}
	if ( !outPath.empty() ){
	  Exp->setOutPath( outPath );
	}
      }
      if ( clones > 0 )
	Exp->Clones( clones );
      if ( estimate < 10 )
	Exp->Estimate( 0 );
      else
	Exp->Estimate( estimate );
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
      if ( do_exact )
	Exp->SetOption(  "EXACT_MATCH: true" );
      else
	Exp->SetOption(  "EXACT_MATCH: false" );
      if ( do_hashed )
	Exp->SetOption(  "HASHED_TREE: true" );
      else
	Exp->SetOption(  "HASHED_TREE: false" );
      if ( occIn > 0 &&
	   do_sample_weights ){
	Error( "--occurrences and -s cannot be combined!" );
	return false;
      }
      if ( occIn > 0 ){
	Exp->SetOption( "HANDLE_OCCURRENCES: " + toString(occIn) );
      }
      else if ( do_sample_weights ){
	Exp->SetOption(  "EXEMPLAR_WEIGHTS: true" );
	if ( do_ignore_samples )
	  Exp->SetOption( "IGNORE_EXEMPLAR_WEIGHTS: true" );
	else
	  Exp->SetOption( "IGNORE_EXEMPLAR_WEIGHTS: false" );
	if ( do_ignore_samples_test )
	  Exp->SetOption( "NO_EXEMPLAR_WEIGHTS_TEST: true" );
	else
	  Exp->SetOption( "NO_EXEMPLAR_WEIGHTS_TEST: false" );
      }
      else
	Exp->SetOption(  "EXEMPLAR_WEIGHTS: false" );
      if ( local_metric == UnknownMetric ){
	// Ok, so NO defaults at all (API usage for instance)
	local_metric = Overlap;
	for ( size_t j=0; j < metricsArray.size(); ++j ){
	  metricsArray[j] = Overlap;
	}

      }
      optline = "GLOBAL_METRIC: " + toString(local_metric);
      Exp->SetOption( optline );
      if ( bootstrap_lines > 0 ){
	optline = "IB2_OFFSET: " + toString<int>(bootstrap_lines);
	Exp->SetOption( optline );
      }
      if ( local_normalisation != unknownNorm ){
	optline = "NORMALISATION: " + toString<normType>( local_normalisation );
	Exp->SetOption( optline );
	if ( local_normalisation == addFactorNorm ){
	  optline = "NORM_FACTOR: " + toString<double>( local_norm_factor );
	  Exp->SetOption( optline );
	}
      }
      optline = "MVD_LIMIT: " + toString<int>(mvd_limit);
      Exp->SetOption( optline );
      optline = "NEIGHBORS: " + toString<int>(no_neigh);
      if ( Exp->SetOption( optline ) ){
	optline = "DECAY: " + toString(local_decay);
	if ( Exp->SetOption( optline ) ){
	  optline = "DECAYPARAM_A: " + toString<double>(local_decay_alfa);
	  if ( Exp->SetOption( optline ) ){
	    optline = "DECAYPARAM_B: " + toString<double>(local_decay_beta);
	    if ( Exp->SetOption( optline ) ){
	      optline = "CLIP_FACTOR: " + toString<int>(clip_freq);
	      if ( Exp->SetOption( optline ) ){
		optline = "SEED: " + toString<int>(seed);
		if ( Exp->SetOption( optline ) ){
		  optline = "PROGRESS: " + toString<int>(local_progress);
		  if ( Exp->SetOption( optline ) ){
		    optline = "VERBOSITY: " +
		      toString(myVerbosity);
		    if ( Exp->SetOption( optline ) ){
		      for ( size_t i=0; i < metricsArray.size(); ++i ){
			// if ( !first ){
			//   if ( metricsArray[i] == Ignore ){
			//     Error( "-m:I is not possible at this stage" );
			//     return false;
			//   }
			// }
			optline = "METRICS: " + toString<int>( i ) + "=" +
			  toString(metricsArray[i]);
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
    string::iterator eit;
    while( it != line.end() && *it != ':' ){
      eit = it;
      while( eit != line.end() && isdigit( *eit ) ) ++eit;
      string tmp = string( it, eit );
      size_t k;
      if ( stringTo<size_t>( tmp, k, 1, metricsArray.size() ) ){
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
      else if ( *it == ',' )
	++it;
      else if ( *it == '-' ){
	++it;
	eit = it;
	while( eit != line.end() && isdigit( *eit ) ) ++eit;
	tmp = string( it, eit );
	m = stringTo<int>(tmp);
	if ( m <= 0 || m > metricsArray.size() ){
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
	  for ( size_t j=k+1; j <= m && j <= metricsArray.size(); ++j ){
	    if ( metricsArray[j] != UnknownMetric
		 && metricsArray[j] != Value ){
	      Error( "metric of feature " + toString<int>(j) +
		     " is multiply changed!" );
	      return false;
	    }
	    metricsArray[j] = Value;
	  }
	}
	if ( it != line.end() && *it == ',' ) ++it;
      }
    }
    return true;
  }

  inline bool GetOptClass::parse_metrics( const string& Mline,
					  MetricType& Def ){
    string line = TiCC::trim( Mline );
    TiCC::to_upper( line );
    string::iterator p = line.begin();
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
	if ( p == line.end() || *p == ':' )
	  Def = DotProduct;
	else
	  if ( *p == 'C' ){
	    Def = Dice;
	    ++p;
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
	  for ( vector<MetricType>::iterator it=metricsArray.begin();
		it != metricsArray.end();
		++it ){
	    *it = Def;
	  }
	  return true;
	}
      }
      else if ( *p != ':' ){
	Error( "missing ':' after default value in -m option" );
	return false;
      }
      else {
	// deviating options expected. reset the array
	for ( vector<MetricType>::iterator it=metricsArray.begin();
	      it != metricsArray.end();
	      ++it ){
	  *it = UnknownMetric;
	}
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
	    Error( "Similarity metric " + toString( Def )
		   + " only accepts -I specifications: -m " + Mline );
	    delete tmpMC;
	    return false;
	  }
	  delete tmpMC;
	  ++p;
	  if ( !parse_range( line, p, TmpMT ) )
	    return false;
	  if ( p == line.end() ){
	    break;
	  }
	  if ( *p != ':' ){
	    Error( "missing ':' in metric description" );
	    return false;
	  }
	  else
	    ++p;
	}
	if ( p != line.end() ){
	  Error( "illegal value in metric description: -m " + Mline );
	  return false;
	}
	else {
	  //
	  // set defaults for those still unset
	  //
	  for ( vector<MetricType>::iterator it=metricsArray.begin();
		it != metricsArray.end();
		++it ){
	    if ( *it == UnknownMetric )
	      *it = Def;
	  }
	}
      }
      return true;
    }
    else
      return false;
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

  bool GetOptClass::parse_options( const CL_Options& opts,
				   const int mode ){
    opt_changed = true;
    //    cerr << "options: " << opts << endl;
    //    cerr << "mode: " << mode << endl;
    const char *q;
    list<CL_item>::const_iterator curr_opt;
    curr_opt = opts.Opts.begin();
    if ( curr_opt == opts.Opts.end() ){
      return true;
    }
    const char *ok_opt;
    switch ( mode ){
    case 0:
      ok_opt = "a:b:B:c:C:d:De:F:G:Hk:l:L:m:M:n:N:o:O:p:q:QR:sS:t:T:v:w:Wx";
      break;
    case 1:
      // limited usage, for @t
      ok_opt = "d:e:G:k:L:m:p:QR:v:x";
      break;
    case 2:
      // limited usage, for Server
      ok_opt = "C:d:G:k:l:L:p:QS:v:x";
      break;
    default:
      ok_opt = NULL;
      string msg = string("Invalid value '") + toString(mode)
	+ "' in switch ("
	+ __FILE__  + "," + toString(__LINE__) + ")\n"
	+ "ABORTING now";
      throw std::logic_error( msg );
    }
    while ( curr_opt != opts.Opts.end()  ) {
      bool mood = false;
      bool longOpt = false;
      string long_option;
      string myoptarg = curr_opt->Option();
      char option = curr_opt->OptChar();

      if ( curr_opt->isLong() ){
	long_option = curr_opt->OptWord();
	longOpt = true;
      }
      else {
	mood = curr_opt->getMood();
      }
      //      cerr << "long option:" << long_option << endl;
      //      cerr << "   myoptarg:" << myoptarg << endl;
      if ( !strchr( ok_opt, option ) ){
	// invalid option
	switch ( mode ){
	case 1:
	case 2:{
	  string LongLine;
	  q = ok_opt;
	  while ( *q ){
	    if ( *q != ':'  ){
	      LongLine = LongLine + *q + ' ';
	    }
	    q++;
	  }
	  Error( string("Illegal option, -") + (char)option
		 + ", only the following options are allowed: "
		 + LongLine );
	}
	break;
	default:
	  break;
	}
	return false;
      };

      try {
	//	cerr << "try " << option << endl;
	switch (option) {
	case 'a':
	  if ( !stringTo<AlgorithmType>( myoptarg, local_algo ) ){
	    Error( "illegal -a value: " + myoptarg );
	    return false;
	  }
	  break;

	case 'b':
	  bootstrap_lines = stringTo<int>( myoptarg );
	  if ( bootstrap_lines < 1 ){
	    Error( "illegal value for -b option: " + myoptarg );
	    return false;
	  }
	  break;

	case 'B':
	  if ( longOpt ){
	    if ( long_option == "Beam" ){
	      if ( !stringTo<int>( myoptarg, BeamSize )
		   || BeamSize <= 0 ){
		Error( "illegal value for -Beam option: " + myoptarg );
		return false;
	      }
	    }
	    else {
	      Error( "invalid option: Did you mean '--Beam'?" );
	      return false;
	    }
	  }
	  else if ( myoptarg.find("eam") != string::npos ){
	    Error( "invalid option: Did you mean '--B" + myoptarg + "'?" );
	    return false;
	  }
	  else {
	    BinSize = stringTo<int>( myoptarg );
	    if ( BinSize <= 1 ){
	      Error( "illegal value for -B option: " + myoptarg );
	      return false;
	    }
	  }
	  break;

      case 'c':
	if ( longOpt ){
	  if ( long_option == "clones" ){
	    clones = stringTo<int>( myoptarg );
	    if ( clones <= 0 ){
	      Error( "invalid value for " + long_option + ": '"
		     + myoptarg + "'" );
	      return false;
	    }
	  }
	}
	else {
	  clip_freq = stringTo<int>( myoptarg );
	  if ( clip_freq < 0 ){
	    Error( "illegal value for -c option: " + myoptarg );
	    return false;
	  }
	}
	break;

      case 'd': {
	if ( longOpt ){
	  if ( long_option == "daemonize" ){
	    bool val;
	    if ( !isBoolOrEmpty(myoptarg,val) ){
	      Error( "invalid value for " + long_option + ": '"
		     + myoptarg + "'" );
	      return false;
	    }
	    do_daemon = val;
	  }
	  else {
	    Error( "invalid option: Did you mean '--daemonize'?" );
	    return false;
	  }
	}
	else if ( myoptarg.find("aemonize") != string::npos ){
	  Error( "invalid option: Did you mean '--daemonize' ?" );
	  return false;
	}
	else {
	  string::size_type pos1 = myoptarg.find( ":" );
	  if ( pos1 == string::npos ){
	    pos1 = myoptarg.find_first_of( "0123456789" );
	    if ( pos1 != string::npos ){
	      if ( ! ( stringTo<DecayType>( string( myoptarg, 0, pos1 ),
					    local_decay ) &&
		       stringTo<double>( string( myoptarg, pos1 ),
					 local_decay_alfa ) ) ){
		Error( "illegal value for -d option: " + myoptarg );
		return false;
	      }
	    }
	    else if ( !stringTo<DecayType>( myoptarg, local_decay ) ){
	      Error( "illegal value for -d option: " + myoptarg );
	      return false;
	    }
	  }
	  else {
	    string::size_type pos2 = myoptarg.find( ':', pos1+1 );
	    if ( pos2 == string::npos ){
	      pos2 = myoptarg.find_first_of( "0123456789", pos1+1 );
	      if ( pos2 != string::npos ){
		if ( ! ( stringTo<DecayType>( string( myoptarg, 0, pos1 ),
					      local_decay ) &&
			 stringTo<double>( string( myoptarg, pos2 ),
					   local_decay_alfa ) ) ){
		  Error( "illegal value for -d option: " + myoptarg );
		  return false;
		}
	      }
	      else {
		Error( "illegal value for -d option: " + myoptarg );
		return false;
	      }
	    }
	    else {
	      if ( ! ( stringTo<DecayType>( string( myoptarg, 0, pos1 ),
					    local_decay ) &&
		       stringTo<double>( string( myoptarg, pos1+1, pos2-pos1-1 ),
					 local_decay_alfa ) &&
		       stringTo<double>( string( myoptarg, pos2+1 ),
					 local_decay_beta ) ) ){
		Error( "illegal value for -d option: " + myoptarg );
		return false;
	      }
	    }
	  }
	}
	break;
      }

      case 'D':
	if ( longOpt ){
	  if ( long_option == "Diversify" )
	    do_diversify = true;
	  else {
	    Error( "invalid option: Did you mean '--Diversify' ?" );
	    return false;
	  }
	}
	else if ( myoptarg.size() > 1 ){
	  Error( "invalid option: Did you mean '--Diversify'?" );
	  return false;
	}
	else
	  keep_distributions = mood;
	break;

      case 'e':
	estimate = stringTo<int>( myoptarg );
	if ( estimate < 0 ){
	  Error( "illegal value for -e option: " + myoptarg );
	  return false;
	}
	break;

      case 'F':
	if ( !stringTo<InputFormatType>( myoptarg, LocalInputFormat ) ){
	  Error( "illegal value for -F option: " + myoptarg );
	  return false;
	}
	break;

      case 'G':
	if ( myoptarg.empty() )
	  local_normalisation = probabilityNorm;
	else {
	  string::size_type pos1 = myoptarg.find( ":" );
	  if ( pos1 == string::npos ){
	    local_normalisation = stringTo<normType>( myoptarg );
	    local_norm_factor = 1;
	  }
	  else {
	    local_normalisation = stringTo<normType>( string( myoptarg, 0, pos1 ) );
	    if ( !stringTo<double>( string( myoptarg, pos1+1 ),
				    local_norm_factor ) ||
		 local_norm_factor < Epsilon ){
	      Error( "illegal value for -G option: " + myoptarg );
	      return false;
	    }
	  }
	  if ( local_normalisation == unknownNorm ){
	    Error( "illegal value for -G option: " + myoptarg );
	    return false;
	  }
	}
	break;

      case 'H':
	do_hashed = mood;
	break;

      case 'k':
	no_neigh = stringTo<int>(myoptarg);
	if ( no_neigh <= 0 ){
	  Error( "illegal value for -k option: " + myoptarg );
	  return false;
	}
	break;

      case 'l':
	if ( longOpt ){
	  if ( long_option == "logfile" ){
	    if ( myoptarg.empty() ){
	      Error( "missing filename for '--logfile'" );
	      return false;
	    }
	    logFile = myoptarg;
	  }
	  else {
	    Error( "invalid option: Did you mean '--logfile' ?" );
	    return false;
	  }
	}
	else if ( myoptarg.find("ogfile") != string::npos ){
	  Error( "invalid option: Did you mean '--logfile' ?" );
	  return false;
	}
	else {
	  f_length = stringTo<int>( myoptarg );
	  if ( f_length <= 0 ){
	    Error( "illegal value for -l option: " + myoptarg );
	    return false;
	  }
	}
	break;

      case 'L': {
	string::size_type pos1 = myoptarg.find( ":" );
	if ( pos1 == string::npos ){
	  pos1 = myoptarg.find_first_of( "0123456789" );
	  if ( pos1 != string::npos ){
	    mvd_limit = stringTo<int>( myoptarg );
	    if ( mvd_limit <= 0 ){
	      Error( "illegal value for -L option: " + myoptarg );
	      return false;
	    }
	  }
	}
	else {
	  mvd_limit = stringTo<int>( string( myoptarg, pos1+1 ) );
	  if ( mvd_limit <= 0 ){
	    Error( "illegal value for -L option: " + myoptarg );
	    return false;
	  }
	}
	break;
      }
      case 'm':
	if ( !parse_metrics( myoptarg, local_metric ) )
	  return false;
	break;

      case 'M':
	maxbests = stringTo<int>( myoptarg );
	if ( maxbests <= 0 ){
	  Error( "illegal value for -M option: " + myoptarg );
	  return false;
	}
	break;

      case 'N':
	// skip previously parsed NumOfFeatures info.
	break;

      case 'O':
	outPath = myoptarg;
	break;

      case 'o':
	if ( longOpt ){
	  if ( long_option == "occurrences" ){
	    if ( myoptarg.empty() ){
	      Error( "missing value for '--occurrences'" );
	      return false;
	    }
	    if ( myoptarg == "train" )
	      occIn = 1;
	    else if ( myoptarg == "test" )
	      occIn = 2;
	    else if ( myoptarg == "both" )
	      occIn = 3;
	    else {
	      Error( "invalid --ocurrences value." );
	      return false;
	    }
	  }
	  else {
	    Error( "invalid option: Did you mean '--ocurrences' ?" );
	    return false;
	  }
	}
	else if ( myoptarg.find("ccurences") != string::npos ){
	  Error( "invalid option: Did you mean '--occurrences' ?" );
	  return false;
	}
	break;

      case 'p':
	if ( longOpt ){
	  if ( long_option == "pidfile" ){
	    if ( myoptarg.empty() ){
	      Error( "missing filename for '--pidfile'" );
	      return false;
	    }
	    pidFile = myoptarg;
	  }
	  else {
	    Error( "invalid option: Did you mean '--pidfile' ?" );
	    return false;
	  }
	}
	else if ( myoptarg.find("idfile") != string::npos ){
	  Error( "invalid option: Did you mean '--pidfile' ?" );
	  return false;
	}
	else {
	  local_progress = stringTo<int>( myoptarg );
	}
	break;

      case 'q':
	threshold = stringTo<int>( myoptarg );
	break;

      case 'Q':
	do_query = true;
	break;

      case 'R':
	if ( isdigit(myoptarg[0]) )
	  seed = stringTo<int>( myoptarg );
	else {
	  Error( "Integer argument for Random Seed expected (-R option)" );
	  return false;
	}
	break;

      case 's':
	if ( longOpt ){
	  if ( long_option == "sloppy" ){
	    bool val;
	    if ( !isBoolOrEmpty(myoptarg,val) ){
	      Error( "invalid value for sloppy: '"
		     + myoptarg + "'" );
	      return false;
	    }
	    do_sloppy_loo = val;
	  }
	  else if ( long_option == "silly" ){
	    bool val;
	    if ( !isBoolOrEmpty(myoptarg,val) ){
	      Error( "invalid value for silly: '"
		     + myoptarg + "'" );
	      return false;
	    }
	    do_silly = val;
	  }
	  else {
	    Error( "invalid option: Did you mean '--sloppy or --silly' ?" );
	    return false;
	  }
	}
	else {
	  if ( myoptarg.empty() ){
	    do_sample_weights = true;
	  }
	  else {
	    if ( isdigit(myoptarg[0]) ){
	      int val = stringTo<int>( myoptarg );
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
	      Error( "invalid value for -s: '" + myoptarg + "' (maybe you meant --s" + myoptarg + " ?)" );
	      return false;
	    }
	  }
	}
	break;

      case 't':
	if ( compare_nocase( myoptarg, "leave_one_out" ) )
	  local_algo = LOO_a;
	else if ( compare_nocase( myoptarg, "cross_validate" ) )
	  local_algo = CV_a;
	break;

      case 'T': {
	if ( longOpt ){
	  if ( long_option == "Threshold" ){
	    if ( !stringTo<int>(myoptarg, igThreshold )
		 || igThreshold < 0 ){
	      Error( "invalid value for Threshold: " + myoptarg );
	      return false;
	    }
	  }
	  else if ( long_option == "Treeorder" ){
	    if ( !stringTo<OrdeningType>( myoptarg, local_order ) ){
	      Error( "invalid value for Treeorder: " + myoptarg );
	      return false;
	    }
	  }
	  else {
	    Error( "invalid option: Did you mean '--Threshold' or --Treeorder ?" );
	    return false;
	  }
	}
	else if ( myoptarg.find("hreshold") != string::npos ||
		  myoptarg.find("reeorder") != string::npos ){
	  Error( "invalid option: Did you mean '--T" + myoptarg + "' ?" );
	  return false;
	}
	else if ( ! stringTo<int>( myoptarg, target_pos ) ){
	  Error( "invalid option: Did you mean '-T value ?" );
	  return false;
	}
	else if ( target_pos <= 0 ){
	  Error( "illegal value for -T option: " + myoptarg );
	  return false;
	}
      }
	break;

      case 'v':{
	VerbosityFlags Flag = NO_VERB;
	if ( !stringTo<VerbosityFlags>( myoptarg, Flag ) ){
	  Error( "illegal value for +/- v option: " + myoptarg );
	  return false;
	}
	else {
	  if ( mode == 2 &&
	       ( !(Flag & (SILENT|DISTANCE|DISTRIB|NEAR_N|CONF_MATRIX) ) ) )
	    return false;
	  else if ( Flag > 0 )
	    if ( mood ){
	      myVerbosity |= Flag;
	    }
	    else {
	      myVerbosity &= ~Flag;
	    }
	  else
	    myVerbosity = NO_VERB;
	}
      }
	break;

      case 'w': {
	if ( !stringTo<WeightType>( myoptarg, local_weight ) )
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

      }
      }
      catch( std::runtime_error& err ) {
	cerr << "invalid value for option '" << option << "'" << endl;
	return false;
      }
      ++curr_opt;
    }
    return true;
  }

}


