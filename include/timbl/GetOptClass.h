/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2011
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
#ifndef GETOPTCLASS_H
#define GETOPTCLASS_H

#include <list>
#include <iosfwd>

#include "Types.h"

namespace Timbl {
  class CL_Options;
  class TimblExperiment;

  class GetOptClass: public MsgClass {
  public:
    GetOptClass( CL_Options&  );
    virtual ~GetOptClass();
    GetOptClass *Clone( std::ostream * = 0 ) const;
    bool parse_options( const CL_Options&, const int=0 );
    void set_default_options( const int=0 );
    bool definitive_options( TimblExperiment * );
    AlgorithmType Algo() const { return local_algo; };
    int MaxFeatures() const { return MaxFeats; };
    std::string getLogFile() const { return logFile; };
    std::string getPidFile() const { return pidFile; };
    bool daemonizeFlag() const { return do_daemon; };
    VerbosityFlags getVerbosity() { return myVerbosity; };
  private:  
    AlgorithmType local_algo;
    MetricType local_metric;
    OrdeningType local_order;
    WeightType local_weight;
    InputFormatType LocalInputFormat;
    DecayType local_decay;
    double local_decay_alfa;
    double local_decay_beta;
    normType local_normalisation;
    double local_norm_factor;
    int MaxFeats;
    int target_pos;
    int no_neigh;
    int mvd_limit;
    int estimate;
    int maxbests;
    int clip_freq;
    int clones;
    int BinSize;
    int BeamSize;
    int bootstrap_lines;
    int f_length;
    int local_progress;
    int seed;
    int threshold;
    int igThreshold;
    VerbosityFlags myVerbosity;
    bool opt_init;
    bool opt_changed;
    bool do_exact;
    bool do_hashed;
    bool min_present;
    bool N_present;
    bool keep_distributions;
    bool do_sample_weights;
    bool do_ignore_samples;
    bool do_ignore_samples_test;
    bool do_query;
    bool do_all_weights;
    bool do_sloppy_loo;
    bool do_silly;
    bool do_diversify;
    bool do_daemon;
    std::vector<MetricType>metricsArray;
    std::ostream *parent_socket_os;
    std::string inPath;
    std::string outPath;
    std::string logFile;
    std::string pidFile;
    void Error( const std::string& ) const;
    inline bool parse_range( std::string&, 
			     std::string::iterator&,
			     MetricType );
    inline bool parse_metrics( const std::string&,
			       MetricType& );
    GetOptClass( const GetOptClass& );
    GetOptClass& operator=( const GetOptClass& );
  };

}
#endif
