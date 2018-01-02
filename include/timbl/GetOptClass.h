/*
  Copyright (c) 1998 - 2018
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
#ifndef TIMBL_GETOPTCLASS_H
#define TIMBL_GETOPTCLASS_H

#include <list>
#include <iosfwd>

namespace Timbl {
  class TimblExperiment;

  class GetOptClass: public MsgClass {
  public:
    explicit GetOptClass( const TiCC::CL_Options&  );
    virtual ~GetOptClass();
    GetOptClass *Clone( std::ostream * = 0 ) const;
    bool parse_options( const TiCC::CL_Options&, const int=0 );
    void set_default_options( const int=0 );
    bool definitive_options( TimblExperiment * );
    AlgorithmType Algo() const { return local_algo; };
    int MaxFeatures() const { return MaxFeats; };
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
    std::vector<MetricType>metricsArray;
    std::ostream *parent_socket_os;
    std::string inPath;
    std::string outPath;
    int occIn;
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
