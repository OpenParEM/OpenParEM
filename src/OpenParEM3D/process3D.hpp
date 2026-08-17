///////////////////////////////////////////////////////////////////////////////////
//                                                                               //
//    process3D - processor for automation of regression testing of OpenParEM3D  //
//    Copyright (C) 2026 Brian Young                                             //
//                                                                               //
//    This program is free software: you can redistribute it and/or modify       //
//    it under the terms of the GNU General Public License as published by       //
//    the Free Software Foundation, either version 3 of the License, or          //
//    (at your option) any later version.                                        //
//                                                                               //
//    This program is distributed in the hope that it will be useful,            //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of             //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              //
//    GNU General Public License for more details.                               //
//                                                                               //
//    You should have received a copy of the GNU General Public License          //
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.      //
//                                                                               //
///////////////////////////////////////////////////////////////////////////////////

#ifndef PROCESS_H
#define PROCESS_H

//#include "mfem.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <limits>
#include <unistd.h>
#include <complex>
#include <filesystem>
#include "results.hpp"
#include "project.h"

extern "C" void init_project (struct projectData *);
extern "C" PetscErrorCode load_project_file(const char*, projectData*, const char*);
extern "C" void print_project (struct projectData *, const char *indent);
extern "C" void free_project(projectData*);

using namespace std;
using namespace mfem;

class TestCase
{
   private:
      string name;
      double frequency;
      bool type;                    // false for S-parameters; true for antenna metrics
      string testVariable;
      int portOut;                  // S-parameters
      int portIn;                   // S-parameters and antenna metrics
      double gain;                  // antenna metrics
      double directivity;
      double radiationEfficiency;
      string testFunction;          // equal or threshold
      double expectedValue;
      double foundValue;
      double error;                 // for testFunction "equal"
      double tolerance;
      double threshold;             // for testFunction "lessthan"
      bool passed;
      bool evaluated;
      long unsigned int index;  // for sorting
   public:
      void set_name (string name_) {name=name_;}
      void set_type (bool type_) {type=type_;}
      void set_frequency (double frequency_) {frequency=frequency_;}
      void set_testVariable (string testVariable_) {testVariable=testVariable_;}
      void set_portOut (int portOut_) {portOut=portOut_;}
      void set_portIn (int portIn_) {portIn=portIn_;}
      void set_gain (double gain_) {gain=gain_;}
      void set_directivity (double directivity_) {directivity=directivity_;}
      void set_radiationEfficiency (double radiationEfficiency_) {radiationEfficiency=radiationEfficiency_;}
      void set_testFunction (string testFunction_) {testFunction=testFunction_;}
      void set_expectedValue (double expectedValue_) {expectedValue=expectedValue_;}
      void set_threshold (double threshold_) {threshold=threshold_;}
      void set_tolerance (double tolerance_) {tolerance=tolerance_;}
      void set_passed (bool passed_) {passed=passed_;}
      void set_evaluated (bool evaluated_) {evaluated=evaluated_;}
      void set_index(long unsigned int i) {index=i;}
      string get_testFunction () {return testFunction;}
      bool get_passed () {return passed;}
      bool get_evaluated () {return evaluated;}
      void print_as_testcase ();
      void printAllFormatted();
      void evaluate (ResultDatabase *, PatternDatabase *);
      double get_error_or_tolerance();
      long unsigned int get_index() {return index;}
      void show_evaluation(ostream *);
};

class TestCaseDatabase
{
   private:
      vector<TestCase *> testCaseList;
   public:
      bool load (const char *);
      void print_as_testcase ();
      void printAllFormatted ();
      void evaluate (ResultDatabase *, PatternDatabase *);
      void sort (bool);
      void show_evaluation (ostream *);
      bool all_evaluated ();
      bool all_passed ();
};

#endif

