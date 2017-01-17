/*
  Copyright (c) 1998 - 2017
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
#include <map>
#include <iostream>
#include <fstream>
#include <cassert>

#include "timbl/MsgClass.h"
#include "timbl/Common.h"
#include "timbl/Types.h"
#include "timbl/IBtree.h"
#include "timbl/TimblExperiment.h"
#include "ticcutils/Timer.h"

namespace Timbl {
  using namespace std;

  void IG_Experiment::InitInstanceBase(){
    srand( RandomSeed() );
    default_order();
    set_order();
    runningPhase = TrainWords;
    InstanceBase = new IG_InstanceBase( EffectiveFeatures(),
					ibCount,
					(RandomSeed()>=0),
					false, KeepDistributions() );
  }

  void IG_Experiment::initExperiment( bool ){
    if ( !ExpInvalid() ) {
      if ( !MBL_init ){  // do this only when necessary
	stats.clear();
	delete confusionInfo;
	confusionInfo = 0;
	if ( Verbosity(ADVANCED_STATS) )
	  confusionInfo = new ConfusionMatrix( Targets->ValuesArray.size() );
	if ( !is_copy ){
	  InitWeights();
	  if ( do_diversify )
	    diverseWeights();
	  srand( random_seed );
	}
	MBL_init = true;
      }
    }
  }

  bool IG_Experiment::checkTestFile(){
    if ( TimblExperiment::checkTestFile() )
      return sanityCheck();
    else
      return false;
  }

  ostream& operator<< ( ostream& os,
			const fileDoubleIndex& fmi ){
    os << "[";
    fileDoubleIndex::const_iterator fmIt = fmi.begin();
    while ( fmIt != fmi.end() ){
      os << fmIt->first << " " << fmIt->second << endl;
      ++fmIt;
    }
    os << "]";
    return os;
  }

  bool IG_Experiment::ClassicLearn( const string& FileName,
				    bool warnOnSingleTarget ){
    bool result = true;
    if ( is_synced ) {
      CurrentDataFile = FileName;
    }
    if ( CurrentDataFile == "" ){
      if ( FileName == "" ){
	Warning( "unable to build an InstanceBase: No datafile defined yet" );
	result = false;
      }
      else {
	if ( !Prepare( FileName, warnOnSingleTarget ) || ExpInvalid() ){
	  result = false;
	}
      }
    }
    else if ( FileName != "" &&
	      CurrentDataFile != FileName ){
      Error( "Unable to Learn from file '" + FileName + "'\n"
	     "while previously instantiated from file '" +
	     CurrentDataFile + "'" );
      result = false;
    }
    if ( result ) {
      TiCC::Timer learnT;
      learnT.start();
      InitInstanceBase();
      if ( ExpInvalid() )
	return false;
      if ( EffectiveFeatures() < 2 ){
	fileIndex fmIndex;
	result = build_file_index( CurrentDataFile, fmIndex );
	if ( result ){
	  stats.clear();
	  if ( !Verbosity(SILENT) ) {
	    Info( "\nPhase 3: Learning from Datafile: " + CurrentDataFile );
	    time_stamp( "Start:     ", 0 );
	  }
	  string Buffer;
	  IG_InstanceBase *outInstanceBase = 0;
	  TargetValue *TopTarget = Targets->MajorityClass();
	  //	cerr << "MAJORITY CLASS = " << TopTarget << endl;
	  // Open the file.
	  //
	  ifstream datafile( CurrentDataFile, ios::in);
	  //
	  fileIndex::const_iterator fit = fmIndex.begin();
	  while ( fit != fmIndex.end() ){
	    set<streamsize>::const_iterator sit = fit->second.begin();
	    while ( sit != fit->second.end() ){
	      datafile.clear();
	      datafile.seekg( *sit );
	      nextLine( datafile, Buffer );
	      chopLine( Buffer );
	      // Progress update.
	      //
	      if (( stats.dataLines() % Progress() ) == 0)
		time_stamp( "Learning:  ", stats.dataLines() );
	      chopped_to_instance( TrainWords );
	      if ( !outInstanceBase ){
		outInstanceBase = new IG_InstanceBase( EffectiveFeatures(),
						       ibCount,
						       (RandomSeed()>=0),
						       false,
						       true );
	      }
	      //		cerr << "add instance " << &CurrInst << endl;
	      outInstanceBase->AddInstance( CurrInst );
	      ++sit;
	    }
	    ++fit;
	  }
	  if ( outInstanceBase ){
	    //	      cerr << "Out Instance Base" << endl;
	    //	      time_stamp( "Start Pruning:    " );
	    //	      cerr << outInstanceBase << endl;
	    outInstanceBase->Prune( TopTarget );
	    //	      time_stamp( "Finished Pruning: " );
	    //	      cerr << outInstanceBase << endl;
	    //	      time_stamp( "Before Merge: " );
	    //	      cerr << InstanceBase << endl;
	    if ( !InstanceBase->MergeSub( outInstanceBase ) ){
	      FatalError( "Merging InstanceBases failed. PANIC" );
	      return false;
	    }
	    delete outInstanceBase;
	    outInstanceBase = 0;
	  }
	}
      }
      else {
	fileDoubleIndex fmIndex;
	result = build_file_multi_index( CurrentDataFile, fmIndex );
	//      cerr << "indexing took " << t << endl;
	if ( result ){
	  stats.clear();
	  if ( !Verbosity(SILENT) ) {
	    Info( "\nPhase 3: Learning from Datafile: " + CurrentDataFile );
	    time_stamp( "Start:     ", 0 );
	  }
	  string Buffer;
	  IG_InstanceBase *PartInstanceBase = 0;
	  IG_InstanceBase *outInstanceBase = 0;
	  TargetValue *TopTarget = Targets->MajorityClass();
	  //	cerr << "MAJORITY CLASS = " << TopTarget << endl;
	  // Open the file.
	  //
	  ifstream datafile( CurrentDataFile, ios::in);
	  //
	  fileDoubleIndex::const_iterator dit = fmIndex.begin();
	  while ( dit != fmIndex.end() ){
	    //	    FeatureValue *the_fv = (FeatureValue*)(dit->first);
	    //	  cerr << "handle feature '" << the_fv << "' met index " << the_fv->Index() << endl;
	    if ( dit->second.size() < 1 ){
	      FatalError( "panic" );
	    }
	    if ( igOffset() > 0 && dit->second.size() > igOffset() ){
	      //	    cerr << "within offset!" << endl;
	      IG_InstanceBase *TmpInstanceBase = 0;
	      TmpInstanceBase = new IG_InstanceBase( EffectiveFeatures(),
						     ibCount,
						     (RandomSeed()>=0),
						     false,
						     true );
	      fileIndex::const_iterator fit = dit->second.begin();
	      while ( fit !=  dit->second.end() ) {
		set<streamsize>::const_iterator sit = fit->second.begin();
		while ( sit != fit->second.end() ){
		  datafile.clear();
		  datafile.seekg( *sit );
		  nextLine( datafile, Buffer );
		  chopLine( Buffer );
		  // Progress update.
		  //
		  if (( stats.dataLines() % Progress() ) == 0)
		    time_stamp( "Learning:  ", stats.dataLines() );
		  chopped_to_instance( TrainWords );
		  if ( !PartInstanceBase ){
		    PartInstanceBase = new IG_InstanceBase( EffectiveFeatures(),
							    ibCount,
							    (RandomSeed()>=0),
							    false,
							    true );
		  }
		  //		cerr << "add instance " << &CurrInst << endl;
		  PartInstanceBase->AddInstance( CurrInst );
		  ++sit;
		}
		if ( PartInstanceBase ){
		  //		time_stamp( "Start Pruning:    " );
		  //		cerr << PartInstanceBase << endl;
		  PartInstanceBase->Prune( TopTarget, 2 );
		  //		time_stamp( "Finished Pruning: " );
		  //		cerr << PartInstanceBase << endl;
		  if ( !TmpInstanceBase->MergeSub( PartInstanceBase ) ){
		    FatalError( "Merging InstanceBases failed. PANIC" );
		    return false;
		  }
		  //		cerr << "after Merge: intermediate result" << endl;
		  //		cerr << TmpInstanceBase << endl;
		  delete PartInstanceBase;
		  PartInstanceBase = 0;
		}
		else {
		  //		cerr << "Partial IB is empty" << endl;
		}
		++fit;
	      }
	      //	    time_stamp( "Start Final Pruning: " );
	      //	    cerr << TmpInstanceBase << endl;
	      TmpInstanceBase->specialPrune( TopTarget );
	      //	    time_stamp( "Finished Final Pruning: " );
	      //	    cerr << TmpInstanceBase << endl;
	      if ( !InstanceBase->MergeSub( TmpInstanceBase ) ){
		FatalError( "Merging InstanceBases failed. PANIC" );
		return false;
	      }
	      //	    cerr << "finale Merge gave" << endl;
	      //	    cerr << InstanceBase << endl;
	      delete TmpInstanceBase;
	    }
	    else {
	      //	    cerr << "other case!" << endl;
	      fileIndex::const_iterator fit = dit->second.begin();
	      while ( fit != dit->second.end() ){
		set<streamsize>::const_iterator sit = fit->second.begin();
		while ( sit != fit->second.end() ){
		  datafile.clear();
		  datafile.seekg( *sit );
		  nextLine( datafile, Buffer );
		  chopLine( Buffer );
		  // Progress update.
		  //
		  if (( stats.dataLines() % Progress() ) == 0)
		    time_stamp( "Learning:  ", stats.dataLines() );
		  chopped_to_instance( TrainWords );
		  if ( !outInstanceBase )
		    outInstanceBase = new IG_InstanceBase( EffectiveFeatures(),
							   ibCount,
							   (RandomSeed()>=0),
							   false,
							   true );
		  //	      cerr << "add instance " << &CurrInst << endl;
		  outInstanceBase->AddInstance( CurrInst );
		  ++sit;
		}
		++fit;
	      }
	      if ( outInstanceBase ){
		//	      cerr << "Out Instance Base" << endl;
		//	      time_stamp( "Start Pruning:    " );
		//	      cerr << outInstanceBase << endl;
		outInstanceBase->Prune( TopTarget );
		//	      time_stamp( "Finished Pruning: " );
		//	      cerr << outInstanceBase << endl;
		//	      time_stamp( "Before Merge: " );
		//	      cerr << InstanceBase << endl;
		if ( !InstanceBase->MergeSub( outInstanceBase ) ){
		  FatalError( "Merging InstanceBases failed. PANIC" );
		  return false;
		}
		delete outInstanceBase;
		outInstanceBase = 0;
	      }
	    }
	    ++dit;
	  }
	}
      }
      if ( !Verbosity(SILENT) ){
	time_stamp( "Finished:  ", stats.dataLines() );
      }
      learnT.stop();
      if ( !Verbosity(SILENT) ){
	IBInfo( *mylog );
	Info( "Learning took " + learnT.toString() );
      }
#ifdef IBSTATS
      cerr << "final mismatches: " << InstanceBase->mismatch << endl;
#endif
    }
    return result;
  }


  bool IG_Experiment::checkLine( const string& line ){
    if ( TimblExperiment::checkLine( line ) )
      return sanityCheck();
    else
      return false;
  }

  bool IG_Experiment::sanityCheck() const {
    bool status = true;
    if ( IBStatus() != Pruned ){
      Warning( "you tried to apply the IGTree algorithm on a complete,"
	       "(non-pruned) Instance Base" );
      status = false;
    }
    if ( num_of_neighbors != 1 ){
      Warning( "number of neighbors must be 1 for IGTree test!" );
      status = false;
    }
    if ( decay_flag != Zero ){
      Warning( "Decay impossible for IGTree test, (while k=1)" );
      status = false;
    }
    if ( globalMetricOption != Overlap ){
      Warning( "Metric must be Overlap for IGTree test." );
      status = false;
    }
    return status;
  }

  const TargetValue *IG_Experiment::LocalClassify( const Instance& Inst,
						   double& Distance,
						   bool& exact ){
    match_depth = -1;
    last_leaf = false;
    exact = false;
    bool Tie = false;
    initExperiment();
    if ( !bestResult.reset( beamSize, normalisation, norm_factor, Targets ) ){
      Warning( "no normalisation possible because a BeamSize is specified\n"
	       "output is NOT normalized!" );
    }
    const TargetValue *TV = NULL;
    const ValueDistribution *ResultDist;
    ResultDist = InstanceBase->IG_test( Inst, match_depth, last_leaf, TV );
    if ( match_depth == 0 ){
      // when level 0, ResultDist == TopDistribution
      TV = InstanceBase->TopTarget( Tie );
    }
    Distance = sum_remaining_weights( match_depth );
    if ( ResultDist &&
	 InstanceBase && InstanceBase->PersistentD() ){
      if ( match_depth == 0 )
	bestResult.addTop( ResultDist );
      else
	bestResult.addConstant( ResultDist );
    }
    if ( confusionInfo )
      confusionInfo->Increment( Inst.TV, TV );
    bool correct = Inst.TV && ( TV == Inst.TV );
    if ( correct ){
      stats.addCorrect();
      if ( Tie )
	stats.addTieCorrect();
    }
    else if ( Tie )
      stats.addTieFailure();
    return TV;
  }

  void IG_Experiment::showTestingInfo( ostream& os ){
    if ( !Verbosity(SILENT)) {
      if ( Verbosity(OPTIONS) )
	ShowSettings( os );
      os << endl << "Starting to test, Testfile: " << testStreamName << endl
	 << "Writing output in:          " << outStreamName << endl
	 << "Algorithm     : IGTree" << endl;
      show_ignore_info( os );
      show_weight_info( os );
      os << endl;
    }
  }


  bool IG_Experiment::WriteInstanceBase( const string& FileName ){
    bool result = false;
    if ( ConfirmOptions() ){
      ofstream outfile( FileName, ios::out | ios::trunc );
      if (!outfile) {
	Warning( "can't open outputfile: " + FileName );
      }
      else {
	if ( !Verbosity(SILENT) )
	  Info( "Writing Instance-Base in: " + FileName );
	if ( PutInstanceBase( outfile ) ){
	  string tmp = FileName;
	  tmp += ".wgt";
	  ofstream wf( tmp );
	  if ( !wf ){
	    Error( "can't write default weightfile " + tmp );
	  }
	  else if ( writeWeights( wf ) ){
	    if ( !Verbosity(SILENT) ){
	      Info( "Saving Weights in " + tmp );
	    }
	    result = true;
	  }
	}
      }
    }
    return result;
  }

  bool IG_Experiment::GetInstanceBase( istream& is ){
    bool result = false;
    bool Pruned;
    bool Hashed;
    int Version;
    string range_buf;
    if ( !get_IB_Info( is, Pruned, Version, Hashed, range_buf ) ){
      return false;
    }
    else if ( !Pruned ){
      Error( "Instance-base is NOT Pruned!, invalid for " +
	     TiCC::toString(algorithm) + " Algorithm" );
    }
    else {
      TreeOrder = DataFile;
      Initialize();
      if ( !get_ranges( range_buf ) ){
	Warning( "couldn't retrieve ranges..." );
      }
      else {
	srand( RandomSeed() );
	InstanceBase = new IG_InstanceBase( EffectiveFeatures(),
					    ibCount,
					    (RandomSeed()>=0),
					    Pruned,
					    KeepDistributions() );
	int pos=0;
	for ( size_t i=0; i < NumOfFeatures(); ++i ){
	  Features[i]->SetWeight( 1.0 );
	  if ( Features[permutation[i]]->Ignore() )
	    PermFeatures[i] = NULL;
	  else
	    PermFeatures[pos++] = Features[permutation[i]];
	}
	if ( Hashed )
	  result = InstanceBase->ReadIB( is, PermFeatures,
					 Targets,
					 TargetStrings, FeatureStrings,
					 Version );
	else
	  result = InstanceBase->ReadIB( is, PermFeatures,
					 Targets,
					 Version );
	if ( result ){
	  if ( !InstanceBase->HasDistributions() ){
	    if ( KeepDistributions() )
	      Error( "Instance base doesn't contain Distributions, "
		     "+D option impossible" );
	    else if ( Verbosity(DISTRIB) ){
	      Info( "Instance base doesn't contain Distributions, "
		    "+vDB option disabled ...."  );
	      ResetVerbosityFlag(DISTRIB);
	    }
	  }
	}
      }
    }
    return result;
  }

  bool IG_Experiment::ReadInstanceBase( const string& FileName ){
    bool result = false;
    if ( ConfirmOptions() ){
      ifstream infile( FileName, ios::in );
      if ( !infile ) {
	Error( "can't open: " + FileName );
      }
      else {
	if ( !Verbosity(SILENT) )
	  Info( "Reading Instance-Base from: " + FileName );
	if ( GetInstanceBase( infile ) ){
	  if ( !Verbosity(SILENT) ){
	    writePermutation( cout );
	  }
	  string tmp = FileName;
	  tmp += ".wgt";
	  ifstream wf( tmp );
	  if ( !wf ){
	    Error( "cant't find default weightsfile " + tmp );
	  }
	  else if ( readWeights( wf, CurrentWeighting() ) ){
	    WFileName = tmp;
	    if ( !Verbosity(SILENT) ){
	      Info( "Reading weights from " + tmp );
	    }
	  }
	  result = true;
	}
      }
    }
    return result;
  }

}
