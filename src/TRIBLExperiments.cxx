/*
  Copyright (c) 1998 - 2009
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
#include "timbl/Statistics.h"
#include "timbl/neighborSet.h"
#include "timbl/BestArray.h"
#include "timbl/IBtree.h"

#ifdef USE_LOGSTREAMS
#include "timbl/LogStream.h"
#else
typedef std::ostream LogStream;
#define Log(X) (X)
#define Dbg(X) (X)
#endif

#include "timbl/SocketBasics.h"
#include "timbl/MBLClass.h"
#include "timbl/TimblExperiment.h"

namespace Timbl {
  using namespace std;

  void TRIBL_Experiment::InitInstanceBase(){
    srand( RandomSeed() );
    default_order();
    set_order();
    runningPhase = TrainWords;
    InstanceBase = new TRIBL_InstanceBase( EffectiveFeatures(), 
					   ibCount,
					   (RandomSeed()>=0),
					   KeepDistributions() );
  }
  
  void TRIBL2_Experiment::InitInstanceBase(){
    srand( RandomSeed() );
    default_order();
    set_order();
    runningPhase = TrainWords;
    InstanceBase = new TRIBL2_InstanceBase( EffectiveFeatures(), 
					    ibCount,
					    (RandomSeed()>=0),
					    KeepDistributions() );
  }
  
  bool TRIBL_Experiment::checkFile( const string& FileName ){
    if ( !TimblExperiment::checkFile( FileName ) )
      return false;
    else if ( IBStatus() == Pruned ){
      Warning( "you tried to apply the " + toString( algorithm) +
	       " algorithm on a pruned Instance Base" );
      return false;
    }
    else if ( TRIBL_offset() == 0 ){
      Error( "TRIBL algorithm impossible while treshold not set\n" );
      return false;
    }
    return true;
  }
  
  bool TRIBL2_Experiment::checkFile( const string& FileName ){
    if ( !TimblExperiment::checkFile( FileName ) )
      return false;
    else if ( IBStatus() == Pruned ){
      Warning( "you tried to apply the " + toString( algorithm) +
	       " algorithm on a pruned Instance Base" );
      return false;
    }
    return true;
  }
  
  const TargetValue *TRIBL_Experiment::LocalClassify( const Instance& Inst,
						      double& Distance,
						      bool& exact ){
    
    const TargetValue *Res = NULL;
    bool Tie = false;
    exact = false;
    bestResult.reset( beamSize, normalisation, norm_factor, Targets );
    const ValueDistribution *ExResultDist = ExactMatch( Inst );
    if ( ExResultDist ){
      Distance = 0.0;
      Res = ExResultDist->BestTarget( Tie, (RandomSeed() >= 0) );
      bestResult.addConstant( ExResultDist );
      exact = Do_Exact();
    }
    else {
      IB_InstanceBase *SubTree = NULL;
      size_t level = 0;
      const ValueDistribution *TrResultDist = 0;
      initExperiment();
      SubTree = InstanceBase->TRIBL_test( Inst, TRIBL_offset(), 
					  Res, TrResultDist,
					  level );
      if ( !SubTree ){
	match_depth = level;
	last_leaf = false;
	Distance = sum_remaining_weights(level);
	if ( TrResultDist ){
	  if ( level == 0 )
	    bestResult.addTop( TrResultDist );
	  else
	    bestResult.addConstant( TrResultDist );
	}
      }
      else {
	testInstance( Inst, SubTree, TRIBL_offset() );
	bestArray.initNeighborSet( nSet );
	WValueDistribution *ResultDist = getBestDistribution();
	Res = ResultDist->BestTarget( Tie, (RandomSeed() >= 0) );
	if ( Tie ){
	  ++num_of_neighbors;
	  testInstance( Inst, SubTree, TRIBL_offset() );
	  bestArray.addToNeighborSet( nSet, num_of_neighbors );
	  WValueDistribution *ResultDist2 = getBestDistribution();
	  bool Tie2 = false;
	  const TargetValue *Res2 = ResultDist2->BestTarget( Tie2, (RandomSeed() >= 0) );
	  --num_of_neighbors;
	  if ( !Tie2 ){
	    delete ResultDist;
	    bestResult.addDisposable( ResultDist2 );
	    Res = Res2;
	  }
	  else {
	    delete ResultDist2;
	    bestResult.addDisposable( ResultDist );
	  }
	}
	else {
	  bestResult.addDisposable( ResultDist );
	}
	SubTree->CleanPartition( true );
	Distance = getBestDistance();
      }
    }
    if ( confusionInfo )
      confusionInfo->Increment( Inst.TV, Res );
    bool correct = Inst.TV && ( Res == Inst.TV );
    if ( correct ){
      stats.addCorrect();
      if ( Tie )
	stats.addTieCorrect();
    }
    else if ( Tie )
      stats.addTieFailure();
    exact = exact || (fabs(Distance) < Epsilon );
    if ( exact )
      stats.addExact();
    return Res;
  }
  
  bool TRIBL_Experiment::checkLine( const string& line ){
    if ( !TimblExperiment::checkLine( line ) )
      return false;
    else if ( IBStatus() == Pruned ){
      Warning( "you tried to apply the TRIBL algorithm on a pruned "
	       " Instance Base" );
      return false;
    }
    return true;
  }
  
  bool TRIBL2_Experiment::checkLine( const string& line ){
    if ( !TimblExperiment::checkLine( line ) )
      return false;
    else if ( IBStatus() == Pruned ){
      Warning( "you tried to apply the TRIBL2 algorithm on a pruned "
	       " Instance Base" );
      return false;
    }
    return true;
  }
  
  const TargetValue *TRIBL2_Experiment::LocalClassify( const Instance& Inst,
						       double& Distance,
						       bool& exact ){
    const TargetValue *Res = NULL;
    exact = false;
    bestResult.reset( beamSize, normalisation, norm_factor, Targets );
    bool Tie = false;
    const ValueDistribution *ExResultDist = ExactMatch( Inst );
    if ( ExResultDist ){
      Distance = 0.0;
      Res = ExResultDist->BestTarget( Tie, (RandomSeed() >= 0) );
      bestResult.addConstant( ExResultDist );
      exact = Do_Exact();
    }
    else {
      IB_InstanceBase *SubTree = NULL;
      size_t level = 0;
      const ValueDistribution *TrResultDist = 0;
      SubTree = InstanceBase->TRIBL2_test( Inst, TrResultDist, level );
      if ( SubTree ){
	testInstance( Inst, SubTree, level );
	bestArray.initNeighborSet( nSet );
	WValueDistribution *ResultDist1 = getBestDistribution();
	Res = ResultDist1->BestTarget( Tie, (RandomSeed() >= 0) );
	if ( Tie ){
	  ++num_of_neighbors;
	  testInstance( Inst, SubTree, level );
	  bestArray.addToNeighborSet( nSet, num_of_neighbors );
	  WValueDistribution *ResultDist2 = getBestDistribution();
	  bool Tie2 = false;
	  const TargetValue *Res2 = ResultDist2->BestTarget( Tie2, (RandomSeed() >= 0) );
	  --num_of_neighbors;
	  if ( !Tie2 ){
	    delete ResultDist1;
	    bestResult.addDisposable( ResultDist2 );
	    Res = Res2;
	  }
	  else {
	    bestResult.addDisposable( ResultDist1 );
	  }
	}
	else {
	  bestResult.addDisposable( ResultDist1 );
	}
	SubTree->CleanPartition( true );
	match_depth = level;
	Distance = getBestDistance();
      }
      else {
	// an exact match
	Distance = 0.0;
	Res = TrResultDist->BestTarget( Tie, (RandomSeed() >= 0) );
	bestResult.addConstant( TrResultDist );
      }
    }
    if ( confusionInfo )
      confusionInfo->Increment( Inst.TV, Res );
    bool correct = Inst.TV && ( Res == Inst.TV );
    if ( correct ){
      stats.addCorrect();
      if ( Tie )
	stats.addTieCorrect();
    }
    else if ( Tie )
      stats.addTieFailure();
    exact = exact || ( fabs(Distance) < Epsilon );
    if ( exact )
      stats.addExact();
    return Res;
  }

  void TRIBL_Experiment::testing_info( ostream& os, 
				       const string& FileName,
				       const string& OutFile ){
    if ( Verbosity(OPTIONS) )
      ShowSettings( os );
    os << endl << "Starting to test, Testfile: " << FileName << endl
       <<	"Writing output in:          " << OutFile << endl
       << "Algorithm     : TRIBL, q = " << TRIBL_offset() << endl;
    show_metric_info( os );
    show_weight_info( os );
    os << decay << endl;
  }

  bool TRIBL_Experiment::GetInstanceBase( istream& is ){
    bool result = false;
    bool Pruned;
    bool Hashed;
    int Version;
    string range_buf;
    if ( !get_IB_Info( is, Pruned, Version, Hashed, range_buf ) ){
      Error( "Can'n retrieve Instance-Base\n" );
    }
    else if ( Pruned ){
      Error( "Instance-base is Pruned!, NOT valid for " +
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
	InstanceBase = new TRIBL_InstanceBase( EffectiveFeatures(), 
					       ibCount,
					       (RandomSeed()>=0),
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
      }
    }
    return result;
  }

  bool TRIBL2_Experiment::GetInstanceBase( istream& is ){
    bool result = false;
    bool Pruned;
    bool Hashed;
    int Version;
    string range_buf;
    if ( !get_IB_Info( is, Pruned, Version, Hashed, range_buf ) ){
      Error( "Can'n retrieve Instance-Base\n" );
    }
    else if ( Pruned ){
      Error( "Instance-base is Pruned!, NOT valid for " +
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
	InstanceBase = new TRIBL2_InstanceBase( EffectiveFeatures(), 
						ibCount,
						(RandomSeed()>=0),
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
      }
    }
    return result;
  }


}
