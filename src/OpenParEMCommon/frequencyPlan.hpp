////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM2D - A fullwave 2D electromagnetic simulator.                  //
//    Copyright (C) 2025 Brian Young                                          //
//                                                                            //
//    This program is free software: you can redistribute it and/or modify    //
//    it under the terms of the GNU General Public License as published by    //
//    the Free Software Foundation, either version 3 of the License, or       //
//    (at your option) any later version.                                     //
//                                                                            //
//    This program is distributed in the hope that it will be useful,         //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of          //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           //
//    GNU General Public License for more details.                            //
//                                                                            //
//    You should have received a copy of the GNU General Public License       //
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.   //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef FREQUENCYPLAN_H
#define FREQUENCYPLAN_H

#include "petscsys.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "inputFrequency.h"
#include "prefix.h"
#include <quadmath.h>

using namespace std;

extern "C" void prefix ();

// meshRefinementType:
// 0 - refine at each frequency individually
// 1 - refine at a given refinementFrequency [must be > 0]
// 2 - do not refine at any frequencies

class FrequencyPlanPoint {
   private:
      double frequency;
      int refinementPriority;                // 0 means no refinement
      bool restart;                          // true: restart refinement with initial mesh
      bool simulated;                        // true: used for simulation
      bool active;
      int meshSize;                          // used to determine when a re-simulation is required
   public:
      void set_frequency (double frequency_) {frequency=frequency_;}
      void set_refinementPriority (int refinementPriority_) {refinementPriority=refinementPriority_;}
      void set_restart (bool restart_) {restart=restart_;}
      void set_simulated (bool simulated_) {simulated=simulated_;}
      void set_active (bool active_) {active=active_;}
      void set_meshSize (int meshSize_) {meshSize=meshSize_;}
      double get_frequency () {return frequency;}
      int get_refinementPriority () {return refinementPriority;}
      bool get_restart () {return restart;}
      bool get_simulated () {return simulated;}
      bool get_active () {return active;}
      int get_meshSize () {return meshSize;}
      void print ();
};

class FrequencyPlan {
   private:
      vector<FrequencyPlanPoint *> planList;
      int refinedCount;
      bool hasRefined;
   public:
      ~FrequencyPlan ();
      bool assemble (char *, unsigned long int, struct inputFrequencyPlan *);
      FrequencyPlanPoint* get_frequency (char *, double *, bool *, bool *, int *);
      long unsigned int get_plan_size () {return planList.size();}
      FrequencyPlanPoint* get_frequency (long unsigned int i) {return planList[i];}
      bool is_refining ();
      void sort ();
      void eliminateDuplicates ();
      void setAllRefineRestart ();
      void setLowRefinementPriority (int);
      void setHighRefinementPriority (int);
      void print ();
};


#endif
