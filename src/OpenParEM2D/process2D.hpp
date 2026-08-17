////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    process2D - processor to automation of regression testing of OpenParEM2D//
//    Copyright (C) 2026 Brian Young                                          //
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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <limits>
#include <unistd.h>
#include <cfloat>
#include <complex>
#include <filesystem>
#include "project.h"
#include "misc.hpp"

extern "C" void init_project (struct projectData *);
extern "C" int load_project_file(const char*, projectData*, const char*);
extern "C" void print_project (struct projectData *, const char *indent);
extern "C" void free_project(projectData*);
//extern "C" void prefix ();
//extern "C" char* get_prefix_text ();
//extern "C" void set_prefix_text (char *);

using namespace std;

class Result
{
   private:
      double frequency;
      int iteration;
      int mesh_size;
      int matrix_size;
      bool converged;
      float final_error;
      float elapsed_time;
      bool modalImpedanceCalculation;
      long unsigned int modeCount;
      vector<complex<double>> gamma;
      vector<complex<double>> impedance;
      vector<complex<double>> voltage;
      vector<complex<double>> current;
      vector<complex<double>> Pz;
   public:
      void set_frequency (double frequency_) {frequency=frequency_;}
      void set_iteration (int iteration_) {iteration=iteration_;}
      void set_mesh_size (int mesh_size_) {mesh_size=mesh_size_;}
      void set_matrix_size (int matrix_size_) {matrix_size=matrix_size_;}
      void set_converged (bool converged_) {converged=converged_;}
      void set_converted (int converged_) {if (converged_ == 0) converged=false; else converged=true;}
      void set_final_error (double final_error_) {final_error=final_error_;}
      void set_elapsed_time (double elapsed_time_) {elapsed_time=elapsed_time_;}
      void set_modalImpedanceCalculation (bool modalImpedanceCalculation_) {modalImpedanceCalculation=modalImpedanceCalculation_;}
      void set_modeCount (int modeCount_) {modeCount=modeCount_;}
      void push_gamma (complex<double> a) {gamma.push_back(a);}
      void push_impedance (complex<double> a) {impedance.push_back(a);}
      void push_voltage (complex<double> a) {voltage.push_back(a);}
      void push_current (complex<double> a) {current.push_back(a);}
      void push_Pz (complex<double> a) {Pz.push_back(a);}
      bool get_modalImpedanceCalculation() {return modalImpedanceCalculation;}
      long unsigned int get_modeCount() {return modeCount;}
      complex<double> get_gamma (double, unsigned long int);
      complex<double> get_impedance (double, unsigned long int);
      complex<double> get_voltage (double, unsigned long int);
      complex<double> get_current (double, unsigned long int);
      complex<double> get_Pz (double, unsigned long int);
      void print();
};

class ResultDatabase
{
   private:
      vector<Result *> resultList;
   public:
      ~ResultDatabase();
      bool loadResults (const char *);
      bool is_populated() {if (resultList.size() > 0) return true; return false;}
      complex<double> get_gamma(double, int);
      complex<double> get_impedance(double, long unsigned int);
      complex<double> get_voltage(double, long unsigned int);
      complex<double> get_current(double, long unsigned int);
      complex<double> get_Pz(double, long unsigned int);
      void print();
};

class EMfield
{
   private:
      double frequency;
      unsigned long int mode;
      double x,y;
      complex<double> Ex,Ey,Ez,Hx,Hy,Hz;
   public:
      void set_frequency (double frequency_) {frequency=frequency_;}
      void set_mode (int mode_) {mode=mode_;}
      void set_x (double x_) {x=x_;}
      void set_y (double y_) {y=y_;}
      void set_Ex (complex<double> Ex_) {Ex=Ex_;}
      void set_Ey (complex<double> Ey_) {Ey=Ey_;}
      void set_Ez (complex<double> Ez_) {Ez=Ez_;}
      void set_Hx (complex<double> Hx_) {Hx=Hx_;}
      void set_Hy (complex<double> Hy_) {Hy=Hy_;}
      void set_Hz (complex<double> Hz_) {Hz=Hz_;}
      bool get_EMfield (string, double, unsigned long int, double, double, double *);
      void print();
};

class EMfieldDatabase
{
   private:
      vector<EMfield *> EMfieldList;
   public:
      ~EMfieldDatabase();
      bool loadEMfields (const char *);
      bool is_populated() {if (EMfieldList.size() > 0) return true; return false;}
      bool get_EMfield (string, double, unsigned long int, double, double, double *);
      void print();
};

class TestCase
{
   private:
      string name;
      string caseType;
      double frequency;
      int mode;
      int column;   // for impedance matrix, if present; column=0 goes with modal impedance calculation
      double x,y;   // fields only
      string testVariable;
      string testFunction;          // equal or threshold
      double expectedValue;
      double foundValue;
      double error;                 // for testFunction "equal"
      double tolerance;             
      double threshold;             // for testFunction "lessthan"
      bool passed;
      bool evaluated;
      long unsigned int index;      // for sorting
   public:
      void errorEvaluation (bool);
      void set_name (string name_) {name=name_;}
      void set_frequency (double frequency_) {frequency=frequency_;}
      void set_mode (int mode_) {mode=mode_;}
      void set_column (int column_) {column=column_;}
      void set_x (double x_) {x=x_;}
      void set_y (double y_) {y=y_;}
      void set_testVariable (string testVariable_) {testVariable=testVariable_;}
      void set_testFunction (string testFunction_) {testFunction=testFunction_;}
      void set_expectedValue (double expectedValue_) {expectedValue=expectedValue_;}
      void set_tolerance (double tolerance_) {tolerance=tolerance_;}
      void set_threshold (double threshold_) {threshold=threshold_;}
      void set_passed (bool passed_) {passed=passed_;}
      void set_evaluated (bool evaluated_) {evaluated=evaluated_;}
      void set_index(long unsigned int i) {index=i;}
      string get_testFunction() {return testFunction;}
      double get_error_or_tolerance();
      bool get_evaluated() {return evaluated;}
      bool get_passed() {return passed;}
      long unsigned int get_index() {return index;}
      void evaluate(ResultDatabase *, EMfieldDatabase *);
      void show_evaluation(ostream *);
      void print();
      void printAllFormatted();
      void print_as_testcase();
      void audit(string, string, unsigned long int *, unsigned long int *, unsigned long int *, unsigned long int *,
                 double *, double *, double *, unsigned long int *, unsigned long int);
      void trim_error();
};

class TestCaseDatabase
{
   private:
      vector<TestCase *> testCaseList;
   public:
      ~TestCaseDatabase ();
      bool loadTestCases (const char *);
      bool is_populated () {if (testCaseList.size() > 0) return true; return false;}
      void evaluate (ResultDatabase *, EMfieldDatabase *);
      void show_evaluation (ostream *);
      void audit (string, string);
      void print ();
      void printAllFormatted ();
      void print_as_testcase ();
      void sort (bool);
      void trim_error ();
      bool all_evaluated ();
      bool all_passed ();
};



