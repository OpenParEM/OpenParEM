////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM2D - A fullwave 2D electromagnetic simulator.                  //
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

#ifndef RESULTS_H
#define RESULTS_H

#include "mfem.hpp"
#include <slepceps.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include "misc.hpp"
#include "project.h"

extern "C" void prefix ();
extern "C" char* get_prefix_text ();
extern "C" void set_prefix_text (char *);

using namespace std;
using namespace mfem;

class run_statistics
{
   private:
      double error;
      int meshSize;
      int matrixSize;
      chrono::steady_clock::time_point start_time;
      chrono::steady_clock::time_point end_solve_time;
      chrono::steady_clock::time_point end_refine_time;
      bool converged;
   public:
      double get_error() {return error;}
      int get_meshSize() {return meshSize;}
      int get_matrixSize() {return matrixSize;}
      chrono::steady_clock::time_point get_start_time() {return start_time;}
      chrono::steady_clock::time_point get_end_solve_time() {return end_solve_time;}
      chrono::steady_clock::time_point get_end_refine_time() {return end_refine_time;}
      chrono::duration<double> get_solve_elapsed() {return end_solve_time-start_time;}
      chrono::duration<double> get_refine_elapsed() {return end_refine_time-end_solve_time;}
      bool get_converged() {return converged;}
      void set_error(double error_) {error=error_;}
      void set_meshSize(int meshSize_) {meshSize=meshSize_;}
      void set_matrixSize(int matrixSize_) {matrixSize=matrixSize_;}
      void set_start_time(chrono::steady_clock::time_point start_time_) {start_time=start_time_;}
      void set_end_solve_time(chrono::steady_clock::time_point end_solve_time_) {end_solve_time=end_solve_time_;}
      void set_end_refine_time(chrono::steady_clock::time_point end_refine_time_) {end_refine_time=end_refine_time_;}
      void set_converged(bool converged_) {converged=converged_;}
};

class Result
{
   private:
      int iteration;
      double frequency;
      long unsigned int modeCount;
      bool modalImpedanceCalculation;         // true for modal, false for line
      vector<complex<double>> gamma;          // array, modeCount x 1
      vector<double> alpha_perturbation;      // array, modeCount x 1
      vector<complex<double>> Z;              // array, modeCount x 1, impedance: VI, PV, or PI
      vector<complex<double>> V;              // array, modeCount x 1, mode voltage
      vector<complex<double>> I;              // array, modeCount x 1, mode current
      vector<complex<double>> Pz;             // array, modeCount x 1, average Pz
      run_statistics *run_stats;
      bool active;                            // false means that a frequency was re-calculated and a newer result is available
      double resultMagLimit=1e-8;             // breakover point for "equal" vs. "lessthan" comparisons for results
      double equalErrorLimit=1e-12;           // see also waveguide.c, results.cpp, fieldPoints.cpp
      double lessthanErrorLimit=1e-12;
   public:
      ~Result();
      void set_active() {active=true;}
      void set_inactive() {active=false;}
      void set_iteration (int iteration_) {iteration=iteration_;}
      void set_frequency (double frequency_) {frequency=frequency_;}
      void set_modeCount (int modeCount_) {modeCount=modeCount_;}
      void set_modalImpedanceCalculation (int modalImpedanceCalculation_) {modalImpedanceCalculation=modalImpedanceCalculation_;}
      void set_end_refine_time (chrono::steady_clock::time_point end_refine_time_) {run_stats->set_end_refine_time(end_refine_time_);}
      void push_gamma (complex<double> gamma_) {gamma.push_back(gamma_);}
      void push_alpha_perturbation (double alpha_perturbation_) {alpha_perturbation.push_back(alpha_perturbation_);}
      void push_Pz (complex<double> Pz_) {Pz.push_back(Pz_);}
      void push_Z (complex<double> Z_) {Z.push_back(Z_);}
      void push_V (complex<double> V_) {V.push_back(V_);}
      void push_I (complex<double> I_) {I.push_back(I_);}
      void set_run_stats (run_statistics *run_stats_) {run_stats=run_stats_;}

      int get_iteration() {return iteration;}
      double get_frequency() {return frequency;}
      long unsigned int get_modeCount() {return modeCount;}
      bool get_modalImpedanceCalculation() {return modalImpedanceCalculation;}
      complex<double> get_gamma (long unsigned int i) {return gamma[i];}
      double get_alpha_perturbation (long unsigned int i) {return alpha_perturbation[i];}
      complex<double> get_Z (long unsigned int i) {return Z[i];}
      complex<double> get_V (long unsigned int i) {return V[i];}
      complex<double> get_I (long unsigned int i) {return I[i];}
      complex<double> get_Pz (long unsigned int i) {return Pz[i];}
      run_statistics* get_run_stats() {return run_stats;}
      bool is_active() {return active;}

      void print();
      void save_as_test (ofstream *, const char *, int, int *);
      void save_result_component (ofstream *, const char *, int, int *, const char *);
};

class ResultDatabase
{
   private:
      vector<Result *> results;
      vector<double> unique_frequencies;  // sorted in increasing order
      double tol=1e-12;
   public:
      ResultDatabase(){}
      ~ResultDatabase();
      void push(Result *);
      long unsigned int get_mode_count(double);
      Result* get_Result(double);
      Result* get_Result(double, int);
      int get_iterationTotal(double);
      chrono::duration<double> frequency_elapsed(double);
      void update_convergence(double, bool, double);
      void save(struct projectData *, const char *);
      void save_as_test(const char *, const char *);
      void print();
};

#endif
