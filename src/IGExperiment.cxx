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

#include <string>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <cstdlib>

#include "timbl/MsgClass.h"
#include "timbl/Common.h"
#include "timbl/StringOps.h"
#include "timbl/Types.h"
#include "timbl/Options.h"
#include "timbl/Tree.h"
#include "timbl/Instance.h"
#include "timbl/IBtree.h"
#include "timbl/TimblExperiment.h"

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

  void compressIndex( const featureFileMultiIndex& fmIndex,
		      featureFileMultiIndex& res, 
		      unsigned int offset ){
    res.clear();
    featureFileMultiIndex::const_iterator fmIt = fmIndex.begin();
    while ( fmIt != fmIndex.end() ){
      if ( fmIt->second.size() < offset ){
	res[fmIt->first] = fmIt->second;
      }
      else {
	fileMultiIndex out;
	typedef fileMultiIndex::const_iterator Mit;
	Mit mit = fmIt->second.begin();
	FeatureValue *fv = 0;
	size_t totalCnt = 0;
	while ( mit != fmIt->second.end() ){
	  pair<Mit,Mit> range = fmIt->second.equal_range( mit->first );
	  size_t cnt = fmIt->second.count( mit->first );
	  if ( cnt > offset ){
	    res[fmIt->first].insert( range.first, range.second );
	  }
	  else {
	    if ( !fv )
	      fv = mit->first;
	    for ( Mit rit=range.first; rit != range.second; ++rit)
	      out.insert( make_pair(fv, rit->second ) );
	    totalCnt += cnt;
	    if ( totalCnt > offset ){
	      fv = 0;
	      totalCnt = 0;
	      res[fmIt->first].insert( out.begin(), out.end() );
	      out.clear();
	    }
	  }
	  mit = range.second;
	}
	if ( !out.empty() ){
	  res[fmIt->first].insert( out.begin(), out.end() );
	}
      }
      ++fmIt;
    }
  }

  ostream& operator<< ( ostream& os, 
			const fileMultiIndex& mi ){
    fileMultiIndex::const_iterator mIt = mi.begin();
    while ( mIt != mi.end() ){
      os << "<";
      os << mIt->first << "," << mIt->second;
	os << ">";
      ++mIt;
    }
    return os;
  }

  ostream& operator<< ( ostream& os, 
			const featureFileMultiIndex& fmi ){
    os << "[";
    featureFileMultiIndex::const_iterator fmIt = fmi.begin();
    while ( fmIt != fmi.end() ){
      os << fmIt->first << " " << fmIt->second << endl;
      ++fmIt;
    }
    os << "]";
    return os;
  }  

  bool IG_Experiment::SpeedLearn( const string& FileName ){
    bool result = true;
//     Common::Timer mergeT;
//     Common::Timer subMergeT;
//     Common::Timer totMergeT;
//     Common::Timer pruneT;
//     Common::Timer subPruneT;
//     Common::Timer specialPruneT;
//     Common::Timer totalT;
    if ( ExpInvalid() ||
	 !ConfirmOptions() ){
      result = false;
    }
    else {
      if ( is_synced ) {
	CurrentDataFile = FileName;
      }
      if ( CurrentDataFile == "" ){
	if ( FileName == "" ){
	  Warning( "unable to build an InstanceBase: No datafile defined yet" );
	  result = false;
	}
	else {
	  if ( !Prepare( FileName ) || ExpInvalid() ){
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
    }
    if ( result ) {
      Common::Timer learnT;
      learnT.start();
      InitInstanceBase();
      if ( ExpInvalid() )
	return false;
      unsigned int totalDone = 0;
      if ( EffectiveFeatures() < 2 ) {
	fileIndex fmIndex;
	//      Common::Timer t;
	//      t.start();
	result = build_speed_index( fmIndex );
	//      t.stop();
	//      cerr << "indexing took " << t << endl;
	//      totalT.start();
	if ( result ){
	  //	  cerr << "index = " << fmIndex << endl;
	  if ( !Verbosity(SILENT) ) {
	    Info( "\nPhase 3: Learning from Datafile: " + CurrentDataFile );
	    time_stamp( "Start:     ", 0 );
	  }
	  IG_InstanceBase *PartInstanceBase = 0;
	  TargetValue *TopTarget = Targets->MajorityClass();
	  //	  cerr << "MAJORITY CLASS = " << TopTarget << endl;
	  fileIndex::const_iterator it = fmIndex.begin();
	  unsigned int partialDone = 0;
	  while ( it != fmIndex.end() ){
	    //	  FeatureValue *the_fv = (FeatureValue*)(it->first);
	    //	  cerr << "handle feature " << the_fv << " met index " << the_fv->Index() << endl;
	    if ( PartInstanceBase &&
		 (partialDone + it->second.size()) > igOffset() ){
	      //	      cerr << "start prune " << endl;
	      //	      cerr << PartInstanceBase << endl;
	      PartInstanceBase->Prune( TopTarget );
	      //		subPruneT.stop();
	      //		time_stamp( "Finished Pruning: " );
	      //	      cerr << "finished prune:" << endl;
	      //	      cerr << PartInstanceBase << endl;
	      //		subMergeT.start();
	      if ( !InstanceBase->MergeSub( PartInstanceBase ) ){
		FatalError( "Merging InstanceBases failed. PANIC" );
		return false;
	      }
	      else {
		//		subMergeT.stop();
		//		cerr << "after Merge: intermediate result" << endl;
		//		cerr << InstanceBase << endl;
		cerr << "intermediate mismatch: " << PartInstanceBase->mismatch << endl;
		delete PartInstanceBase;
		PartInstanceBase = 0;
		partialDone = 0;
	      }
	    }
	    set<streamsize>::const_iterator fit = it->second.begin();
	    while ( fit != it->second.end() ){
	      if (( totalDone % Progress() ) == 0) 
		time_stamp( "Learning:  ", totalDone );
	      if ( !PartInstanceBase ){
		PartInstanceBase = new IG_InstanceBase( EffectiveFeatures(), 
							ibCount,
							(RandomSeed()>=0), 
							false, 
							true );
	      }
	      //		cerr << "add instance " << &CurrInst << endl;
	      Instance tmp = instances[*fit]; 
	      tmp.permute( permutation );
	      PartInstanceBase->AddInstance( tmp );
	      ++fit;
	      ++partialDone;
	      ++totalDone;
	    }
	    ++it;
	  }
	  if ( PartInstanceBase ){
	    time_stamp( "Final  Pruning:    " );
	    //	    cerr << PartInstanceBase << endl;
	    //		subPruneT.start();
	    PartInstanceBase->Prune( TopTarget );
	    //		subPruneT.stop();
	    time_stamp( "Finished Pruning: " );
	    //	    cerr << PartInstanceBase << endl;
	    //		subMergeT.start();
	    //	    cerr << "merge into " << endl;
	    //	    cerr << InstanceBase << endl;
	    if ( !InstanceBase->MergeSub( PartInstanceBase ) ){
	      FatalError( "Merging InstanceBases failed. PANIC" );
	      return false;
	    }
	    //		subMergeT.stop();
	    //	    cerr << "Final result" << endl;
	    cerr << "intermediate mismatch: " << PartInstanceBase->mismatch << endl;
	    delete PartInstanceBase;
	    PartInstanceBase = 0;
	  }
	}
      }
      else {
	featureFileMultiIndex fmIndexRaw;
	//      Common::Timer t;
	//      t.start();
	result = build_speed_multi_index( fmIndexRaw );
	//      t.stop();
	//      cerr << "indexing took " << t << endl;
	//      totalT.start();
	if ( result ){
	  featureFileMultiIndex fmIndex;
	  //	Common::Timer t;
	  //	t.start();
	  //	cerr << "compressing index " << fmIndexRaw << endl;
	  compressIndex( fmIndexRaw, fmIndex, igOffset() );
	  //	cerr << "resulting index " << fmIndex << endl;
	  //	t.stop();
	  //	cerr << "compressing took " << t << endl;
	  if ( !Verbosity(SILENT) ) {
	    Info( "\nPhase 3: Learning from Datafile: " + CurrentDataFile );
	    time_stamp( "Start:     ", 0 );
	  }
	  string Buffer;
	  IG_InstanceBase *PartInstanceBase = 0;
	  IG_InstanceBase *outInstanceBase = 0;
	  unsigned int totalDone = 0;
	  TargetValue *TopTarget = Targets->MajorityClass();
	  //	cerr << "MAJORITY CLASS = " << TopTarget << endl;
	  featureFileMultiIndex::const_iterator it = fmIndex.begin();
	  while ( it != fmIndex.end() ){
	    //	  FeatureValue *the_fv = (FeatureValue*)(it->first);
	    //	  cerr << "handle feature " << the_fv << " met index " << the_fv->Index() << endl;
	    fileMultiIndex::const_iterator fmIt = it->second.begin();
	    if ( fmIt == it->second.end() ){
	      FatalError( "panic" );
	    }
	    if ( igOffset() > 0 && it->second.size() > igOffset() ){
	      //	    cerr << "within offset!" << endl;
	      IVCmaptype::const_iterator it2
		= Features[permutation[1]]->ValuesMap.begin();
	      IG_InstanceBase *TmpInstanceBase = 0;
	      TmpInstanceBase = new IG_InstanceBase( EffectiveFeatures(), 
						     ibCount,
						     (RandomSeed()>=0), 
						     false, 
						     true );
	      while ( it2 != Features[permutation[1]]->ValuesMap.end() ){
		FeatureValue *the2fv = (FeatureValue*)(it2->second);
		//	      cerr << "handle secondary feature " << the2fv << endl;
		typedef fileMultiIndex::const_iterator mit;
		pair<mit,mit> b = it->second.equal_range( the2fv );
		for ( mit i = b.first; i != b.second; ++i ){
		  if (( totalDone % Progress() ) == 0) 
		    time_stamp( "Learning:  ", totalDone );
		  if ( !PartInstanceBase ){
		    PartInstanceBase = new IG_InstanceBase( EffectiveFeatures(), 
							    ibCount,
							    (RandomSeed()>=0), 
							    false, 
							    true );
		  }
		  //		cerr << "add instance " << &CurrInst << endl;
		  Instance tmp =  instances[i->second];
		  tmp.permute( permutation );
		  PartInstanceBase->AddInstance( tmp );
		  ++totalDone;
		}
		if ( PartInstanceBase ){
		  //  		cerr << "finished handling secondary feature:" << the2fv << endl;
		  //		time_stamp( "Start Pruning:    " );
		  //		cerr << PartInstanceBase << endl;
		  //		subPruneT.start();
		  PartInstanceBase->Prune( TopTarget, 2 );
		  //		subPruneT.stop();
		  //		time_stamp( "Finished Pruning: " );
		  //		cerr << PartInstanceBase << endl;
		  //		subMergeT.start();
		  if ( !TmpInstanceBase->MergeSub( PartInstanceBase ) ){
		    FatalError( "Merging InstanceBases failed. PANIC" );
		    return false;
		  }
		  //		subMergeT.stop();
		  //		cerr << "after Merge: intermediate result" << endl;
		  //		cerr << TmpInstanceBase << endl;
		  cerr << "intermediate mismatch: " << PartInstanceBase->mismatch << endl;
		  delete PartInstanceBase;
		  PartInstanceBase = 0;
		}
		else {
		  //		cerr << "Partial IB is empty" << endl;
		}
		++it2;
	      }
	      //	    time_stamp( "Start Final Pruning: " );
	      //	    cerr << TmpInstanceBase << endl;
	      //	    specialPruneT.start();
	      TmpInstanceBase->specialPrune( TopTarget );
	      //	    specialPruneT.start();
	      //	    time_stamp( "Finished Final Pruning: " );
	      //	    cerr << TmpInstanceBase << endl;
	      //	    totMergeT.start();
	      if ( !InstanceBase->MergeSub( TmpInstanceBase ) ){
		FatalError( "Merging InstanceBases failed. PANIC" );
		return false;
	      }
	      //	    totMergeT.start();
	      //	    cerr << "finale Merge gave" << endl;
	      //	    cerr << InstanceBase << endl;
	      delete TmpInstanceBase;
	    }
	    else {
	      //	    cerr << "other case!" << endl;
	      fileMultiIndex::const_iterator mIt = it->second.begin();
	      while ( mIt != it->second.end() ){
		// Progress update.
		//
		if (( totalDone % Progress() ) == 0) 
		  time_stamp( "Learning:  ", totalDone );
		if ( !outInstanceBase )
		  outInstanceBase = new IG_InstanceBase( EffectiveFeatures(), 
							 ibCount,
							 (RandomSeed()>=0), 
							 false, 
							 true );
		//	      cerr << "add instance " << &CurrInst << endl;
		Instance tmp =  instances[mIt->second];
		tmp.permute( permutation );
		outInstanceBase->AddInstance( tmp );
		++totalDone;
		++mIt;
	      }
	      if ( outInstanceBase ){
		//	      cerr << "Out Instance Base" << endl;
		//	      time_stamp( "Start Pruning:    " );
		//	      cerr << outInstanceBase << endl;
		//	      pruneT.start();
		outInstanceBase->Prune( TopTarget );
		//	      pruneT.stop();
		//	      time_stamp( "Finished Pruning: " );
		//	      cerr << outInstanceBase << endl;
		//	      time_stamp( "Before Merge: " );
		//	      cerr << InstanceBase << endl;
		//	      mergeT.start();
		if ( !InstanceBase->MergeSub( outInstanceBase ) ){
		  FatalError( "Merging InstanceBases failed. PANIC" );
		  return false;
		}
		//	      mergeT.stop();
		cerr << "intermediate mismatch: " << outInstanceBase->mismatch << endl;
		delete outInstanceBase;
		outInstanceBase = 0;
	      }
	    }
	    ++it;
	  }
	}
      }
      time_stamp( "Finished:  ", totalDone );
      instances.clear();
      learnT.stop();
      if ( !Verbosity(SILENT) ){
	IBInfo( *mylog );
	Info( "Learning took " + learnT.toString() );
      }
      cerr << "finale mismatch:                                        " << InstanceBase->mismatch << endl;
      //     totalT.stop();
      //     cerr << "normal pruning took " << pruneT << endl;
      //     cerr << "sub pruning took " << subPruneT << endl;
      //     cerr << "special pruning took " <<  specialPruneT << endl;
      //     cerr << "normal merging took " << mergeT << endl;
      //     cerr << "submerging took " << subMergeT << endl;
      //     cerr << "final merging took " << totMergeT << endl;
      //     cerr << "In total learning took " << totalT << endl;
    }
    return result;
  }
  
  bool IG_Experiment::ClassicLearn( const string& FileName ){
    bool result = true;
//     Common::Timer mergeT;
//     Common::Timer subMergeT;
//     Common::Timer totMergeT;
//     Common::Timer pruneT;
//     Common::Timer subPruneT;
//     Common::Timer specialPruneT;
//     Common::Timer totalT;
    if ( ExpInvalid() ||
	 !ConfirmOptions() ){
      result = false;
    }
    else {
      if ( is_synced ) {
	CurrentDataFile = FileName;
      }
      if ( CurrentDataFile == "" ){
	if ( FileName == "" ){
	  Warning( "unable to build an InstanceBase: No datafile defined yet" );
	  result = false;
	}
	else {
	  if ( !Prepare( FileName ) || ExpInvalid() ){
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
    }
    if ( result ) {
      Common::Timer learnT;
      learnT.start();
      InitInstanceBase();
      if ( ExpInvalid() )
	return false;
      if ( EffectiveFeatures() < 2 ) {
	fileIndex fmIndex;
	//      Common::Timer t;
	//      t.start();
	result = build_file_index( CurrentDataFile, fmIndex );
	//      t.stop();
	//      cerr << "indexing took " << t << endl;
	//      totalT.start();
	if ( result ){
	  //	  cerr << "index = " << fmIndex << endl;
	  stats.clear();
	  if ( !Verbosity(SILENT) ) {
	    Info( "\nPhase 3: Learning from Datafile: " + CurrentDataFile );
	    time_stamp( "Start:     ", 0 );
	  }
	  IG_InstanceBase *PartInstanceBase = 0;
	  TargetValue *TopTarget = Targets->MajorityClass();
	  //	  cerr << "MAJORITY CLASS = " << TopTarget << endl;
	  // Open the file.
	  //
	  ifstream datafile( CurrentDataFile.c_str(), ios::in);
	  //
	  fileIndex::const_iterator it = fmIndex.begin();
	  unsigned int partialDone = 0;
	  while ( it != fmIndex.end() ){
	    //	  FeatureValue *the_fv = (FeatureValue*)(it->first);
	    //	  cerr << "handle feature " << the_fv << " met index " << the_fv->Index() << endl;
	    if ( PartInstanceBase &&
		 (partialDone + it->second.size()) > igOffset() ){
	      //	      cerr << "start prune " << endl;
	      //	      cerr << PartInstanceBase << endl;
	      PartInstanceBase->Prune( TopTarget );
	      //		subPruneT.stop();
	      //		time_stamp( "Finished Pruning: " );
	      //	      cerr << "finished prune:" << endl;
	      //	      cerr << PartInstanceBase << endl;
	      //		subMergeT.start();
	      if ( !InstanceBase->MergeSub( PartInstanceBase ) ){
		FatalError( "Merging InstanceBases failed. PANIC" );
		return false;
	      }
	      else {
		//		subMergeT.stop();
		//		cerr << "after Merge: intermediate result" << endl;
		//		cerr << InstanceBase << endl;
		delete PartInstanceBase;
		PartInstanceBase = 0;
		partialDone = 0;
	      }
	    }
	    set<streamsize>::const_iterator fit = it->second.begin();
	    while ( fit != it->second.end() ){
	      datafile.clear();
	      datafile.seekg( *fit );
	      string Buffer;
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
	      ++fit;
	      ++partialDone;
	    }
	    ++it;
	  }
	  if ( PartInstanceBase ){
	    time_stamp( "Final  Pruning:    " );
	    //	    cerr << PartInstanceBase << endl;
	    //		subPruneT.start();
	    PartInstanceBase->Prune( TopTarget );
	    //		subPruneT.stop();
	    time_stamp( "Finished Pruning: " );
	    //	    cerr << PartInstanceBase << endl;
	    //		subMergeT.start();
	    //	    cerr << "merge into " << endl;
	    //	    cerr << InstanceBase << endl;
	    if ( !InstanceBase->MergeSub( PartInstanceBase ) ){
	      FatalError( "Merging InstanceBases failed. PANIC" );
	      return false;
	    }
	    //		subMergeT.stop();
	    //	    cerr << "Final result" << endl;
	    cerr << "intermediate mismatch: " << PartInstanceBase->mismatch << endl;	    delete PartInstanceBase;
	    PartInstanceBase = 0;
	  }
	}
      }
      else {
	featureFileMultiIndex fmIndexRaw;
	//      Common::Timer t;
	//      t.start();
	result = build_file_multi_index( CurrentDataFile, fmIndexRaw );
	//      t.stop();
	//      cerr << "indexing took " << t << endl;
	//      totalT.start();
	if ( result ){
	  featureFileMultiIndex fmIndex;
	  //	Common::Timer t;
	  //	t.start();
	  //	cerr << "compressing index " << fmIndexRaw << endl;
	  compressIndex( fmIndexRaw, fmIndex, igOffset() );
	  //	cerr << "resulting index " << fmIndex << endl;
	  //	t.stop();
	  //	cerr << "compressing took " << t << endl;
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
	  ifstream datafile( CurrentDataFile.c_str(), ios::in);
	  featureFileMultiIndex::const_iterator it = fmIndex.begin();
	  while ( it != fmIndex.end() ){
	    //	  FeatureValue *the_fv = (FeatureValue*)(it->first);
	    //	  cerr << "handle feature " << the_fv << " met index " << the_fv->Index() << endl;
	    fileMultiIndex::const_iterator fmIt = it->second.begin();
	    if ( fmIt == it->second.end() ){
	      FatalError( "panic" );
	    }
	    if ( igOffset() > 0 && it->second.size() > igOffset() ){
	      //	    cerr << "within offset!" << endl;
	      IVCmaptype::const_iterator it2
		= Features[permutation[1]]->ValuesMap.begin();
	      IG_InstanceBase *TmpInstanceBase = 0;
	      TmpInstanceBase = new IG_InstanceBase( EffectiveFeatures(), 
						     ibCount,
						     (RandomSeed()>=0), 
						     false, 
						     true );
	      while ( it2 != Features[permutation[1]]->ValuesMap.end() ){
		FeatureValue *the2fv = (FeatureValue*)(it2->second);
		//	      cerr << "handle secondary feature " << the2fv << endl;
		typedef fileMultiIndex::const_iterator mit;
		pair<mit,mit> b = it->second.equal_range( the2fv );
		for ( mit i = b.first; i != b.second; ++i ){
		  datafile.clear();
		  datafile.seekg( i->second );
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
		}
		if ( PartInstanceBase ){
		  //  		cerr << "finished handling secondary feature:" << the2fv << endl;
		  //		time_stamp( "Start Pruning:    " );
		  //		cerr << PartInstanceBase << endl;
		  //		subPruneT.start();
		  PartInstanceBase->Prune( TopTarget, 2 );
		  //		subPruneT.stop();
		  //		time_stamp( "Finished Pruning: " );
		  //		cerr << PartInstanceBase << endl;
		  //		subMergeT.start();
		  if ( !TmpInstanceBase->MergeSub( PartInstanceBase ) ){
		    FatalError( "Merging InstanceBases failed. PANIC" );
		    return false;
		  }
		  //		subMergeT.stop();
		  //		cerr << "after Merge: intermediate result" << endl;
		  //		cerr << TmpInstanceBase << endl;
		  cerr << "intermediate mismatch: " << PartInstanceBase->mismatch << endl;
		  delete PartInstanceBase;
		  PartInstanceBase = 0;
		}
		else {
		  //		cerr << "Partial IB is empty" << endl;
		}
		++it2;
	      }
	      //	    time_stamp( "Start Final Pruning: " );
	      //	    cerr << TmpInstanceBase << endl;
	      //	    specialPruneT.start();
	      TmpInstanceBase->specialPrune( TopTarget );
	      //	    specialPruneT.start();
	      //	    time_stamp( "Finished Final Pruning: " );
	      //	    cerr << TmpInstanceBase << endl;
	      //	    totMergeT.start();
	      if ( !InstanceBase->MergeSub( TmpInstanceBase ) ){
		FatalError( "Merging InstanceBases failed. PANIC" );
		return false;
	      }
	      //	    totMergeT.start();
	      //	    cerr << "finale Merge gave" << endl;
	      //	    cerr << InstanceBase << endl;
	      delete TmpInstanceBase;
	    }
	    else {
	      //	    cerr << "other case!" << endl;
	      fileMultiIndex::const_iterator mIt = it->second.begin();
	      while ( mIt != it->second.end() ){
		datafile.clear();
		datafile.seekg( mIt->second );
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
		++mIt;
	      }
	      if ( outInstanceBase ){
		//	      cerr << "Out Instance Base" << endl;
		//	      time_stamp( "Start Pruning:    " );
		//	      cerr << outInstanceBase << endl;
		//	      pruneT.start();
		outInstanceBase->Prune( TopTarget );
		//	      pruneT.stop();
		//	      time_stamp( "Finished Pruning: " );
		//	      cerr << outInstanceBase << endl;
		//	      time_stamp( "Before Merge: " );
		//	      cerr << InstanceBase << endl;
		//	      mergeT.start();
		if ( !InstanceBase->MergeSub( outInstanceBase ) ){
		  FatalError( "Merging InstanceBases failed. PANIC" );
		  return false;
		}
		//	      mergeT.stop();
		cerr << "intermediate mismatch: " << outInstanceBase->mismatch << endl;
		delete outInstanceBase;
		outInstanceBase = 0;
	      }
	    }
	    ++it;
	  }
	}
      }
      time_stamp( "Finished:  ", stats.dataLines() );
      learnT.stop();
      if ( !Verbosity(SILENT) ){
	IBInfo( *mylog );
	Info( "Learning took " + learnT.toString() );
      }
      cerr << "mismatches: " << InstanceBase->mismatch << endl;
      //     totalT.stop();
      //     cerr << "normal pruning took " << pruneT << endl;
      //     cerr << "sub pruning took " << subPruneT << endl;
      //     cerr << "special pruning took " <<  specialPruneT << endl;
      //     cerr << "normal merging took " << mergeT << endl;
      //     cerr << "submerging took " << subMergeT << endl;
      //     cerr << "final merging took " << totMergeT << endl;
      //     cerr << "In total learning took " << totalT << endl;
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
    bestResult.reset( beamSize, normalisation, norm_factor, Targets );
    const TargetValue *TV = NULL;
    const ValueDistribution *ResultDist 
      = InstanceBase->IG_test( Inst, match_depth, last_leaf, TV );
    if ( match_depth == 0 ){
      // when level 0, ResultDist == TopDistribution
      TV = InstanceBase->TopTarget( Tie );
    }
    Distance = sum_remaining_weights( match_depth );
    if ( InstanceBase->PersistentD() && ResultDist ){
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
      ofstream outfile( FileName.c_str(), ios::out | ios::trunc );
      if (!outfile) {
	Warning( "can't open outputfile: " + FileName );
      }
      else {
	if ( !Verbosity(SILENT) )
	  Info( "Writing Instance-Base in: " + FileName );
	if ( PutInstanceBase( outfile ) ){
	  string tmp = FileName;
	  tmp += ".wgt";
	  ofstream wf( tmp.c_str() );
	  if ( !wf ){
	    Error( "can't write default weightfile " + tmp );
	    result = false;
	  }
	  else if ( !writeWeights( wf ) )
	    result = false;
	  else if ( !Verbosity(SILENT) )
	    Info( "Saving Weights in " + tmp );
	  result = true;
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
	     toString(algorithm) + " Algorithm" );
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
      ifstream infile( FileName.c_str(), ios::in );
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
	  ifstream wf( tmp.c_str() );
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
