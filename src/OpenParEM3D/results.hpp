////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM3D - A fullwave 3D electromagnetic simulator.                  //
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


#ifndef RESULTS_H
#define RESULTS_H

#include "mfem.hpp"
#include <quadmath.h>
#include "petscsys.h"
#include <petsc.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include "fem3D.hpp"

extern "C" PetscErrorCode MatInvert (Mat *, int);
extern "C" PetscErrorCode MatInvertTest (int);
extern "C" int calculate_relative_error_on_S (char *);
extern "C" int calculate_absolute_error_on_S (char *);

using namespace std;
using namespace mfem;

class fem3D;

class Result
{
   private:
      bool active;                         // false means that a frequency was re-calculated and a newer result is available
      int iteration;                       // starts at 1
      double frequency;                    // Hz
      string type;                         // S, Y, or Z
      Mat *S=nullptr;                      // data matrix, NxN matrix where N is the number of S-ports
      vector<complex<double>> Zo;          // impedance at each S-port, NxN vector; valid only for type S
      PetscInt meshSize;
      PetscInt matrixSize;                 // global true dof size of the finite element space (keep the matrixSize variable name to align with fem3D.hpp)
      PetscInt sparseWidth;                // allocated width of the main compuational sparse matrices; memory usage is then matrixSize*sparseWidth*16
      double shortestPerWavelength;        // min edge length / wavelength in across all materials
      double longestPerWavelength;         // max edge length / wavelength in across all materials
      double maxRelativeError;             // iterative solution: relative error of S-parameters from prior iteration 
      double maxAbsoluteError;             //                     absolute error in the Hfield across the mesh in current iteration
      bool isRefined=false;                //                     indicates that this is part of an iterative solution (although it may only have the first iteration)
      bool isConverged=false;              //                     indicates if the iterative solution converged
      bool hasRadiation=false;             // indicates if radiation patterns are calculated
      double solve_time=0;
      double fem_setup_time=0;
      double mesh_error_time=0;
      double refine_time=0;
      double radiation_time=0;
      double magLimitdB=-150;              // breakover point for "equal" vs. "lessthan" comparisons for results
      double equalMagLimit=1e-12;
      double argLimitdeg=0.1;
      double equalArgLimit=1e-12;
   public:
      Result();
      ~Result();
      void set (string, double, double, double, int, ParMesh *);
      void set_iteration (int iteration_) {iteration=iteration_;}
      void set_frequency (double frequency_) {frequency=frequency_;}
      void set_active () {active=true;}
      void set_inactive () {active=false;}
      void set_matrixSize (PetscInt matrixSize_) {matrixSize=matrixSize_;}
      void set_sparseWidth (PetscInt sparseWidth_) {sparseWidth=sparseWidth_;}
      void set_type_S () {type="S";}
      void set_type_Y () {type="Y";}
      void set_type_Z () {type="Z";}
      void set_S (Mat *S_) {S=S_;}
      void set_maxRelativeError (double maxRelativeError_) {maxRelativeError=maxRelativeError_;}
      void set_maxAbsoluteError (double maxAbsoluteError_) {maxAbsoluteError=maxAbsoluteError_;}
      void set_isRefined () {isRefined=true;}
      void set_isConverged (bool isConverged_) {isConverged=isConverged_;}
      void set_hasRadiation (bool hasRadiation_) {hasRadiation=hasRadiation_;}
      int get_iteration() {return iteration;}
      void push_Zo (complex<double> Zo_) {Zo.push_back(Zo_);}
      double get_maxAbsoluteError () {return maxAbsoluteError;}
      double get_frequency() {return frequency;}
      Mat* get_S () {return S;}
      PetscScalar get_Sij (int, int);
      complex<double> get_Zo (int Sport) {return Zo[Sport];}
      bool is_active () {return active;}
      bool is_type_S () {if (type.compare("S") == 0) return true; return false;}
      bool is_type_Y () {if (type.compare("Y") == 0) return true; return false;}
      bool is_type_Z () {if (type.compare("Z") == 0) return true; return false;}
      bool extractS (string, string, int);
      void save (ostream *, struct projectData *, int);
      void saveFormatted (ostream *, double, double, double, double, double, double *, double);
      void saveCSV (ostream *, struct projectData *, double);
      void save_result_component (ofstream *, const char *, int, int *, const char *, long unsigned int);
      bool get_isRefined () {return isRefined;}
      bool get_isConverged () {return isConverged;}
      bool get_hasRadiation () {return hasRadiation;}
      double get_solve_time () {return solve_time;}
      double get_fem_setup_time () {return fem_setup_time;}
      double get_refine_time () {return refine_time;}
      double get_mesh_error_time () {return mesh_error_time;}
      double get_radiation_time () {return radiation_time;}
      void set_solve_time (double t) {solve_time+=t;}
      void set_fem_setup_time (double t) {fem_setup_time+=t;}
      void set_refine_time (double t) {refine_time+=t;}
      void set_mesh_error_time (double t) {mesh_error_time+=t;}
      void set_radiation_time (double t) {radiation_time+=t;}
      PetscErrorCode forceReciprocal();
      PetscErrorCode get_k (bool, complex<double>, Mat *);
      PetscErrorCode S2Z ();
      PetscErrorCode Z2S (bool, complex<double>);
      PetscErrorCode renormalize (complex<double>);
      PetscErrorCode SparameterConversion (BoundaryDatabase *, Mat *, Mat *, vector<complex<double>> *);
      void print ();
      PetscErrorCode save_as_test (ofstream *, const char *, int, int *);
};

class ResultDatabase
{
   private:
      vector<Result *> results;
      vector<double> unique_frequencies;  // sorted in increasing order
      int SportCount;
      double solve_time=0;
      double tol=1e-12;
      double frequencyScale=1;
   public:
      ResultDatabase () {}
      ~ResultDatabase ();
      void set_frequencyScale (struct projectData *);
      void push (Result *);
      Result* get_Result (double, int);
      Result* get_Result (double);
      vector<double>* get_unique_frequencies () {return &unique_frequencies;}
      int get_SportCount () {return SportCount;}
      double calculate_maxRelativeError (struct projectData *, double, int);
      double calculate_maxAbsoluteError (struct projectData *, double, int);
      void set_SportCount (int SportCount_) {SportCount=SportCount_;}
      bool save (struct projectData *);
      void saveFormatted(ostream *);
      bool saveFormatted(struct projectData *);
      void saveCSV (ostream *, struct projectData *, BoundaryDatabase *, vector<DifferentialPair *> *, bool);
      bool saveCSV (struct projectData *, BoundaryDatabase *, vector<DifferentialPair *> *, bool);
      bool saveTouchstone (struct projectData *, BoundaryDatabase *, vector<DifferentialPair *> *);
      void set_refine_time (double, double, int);
      void set_radiation_time (double, double, int);
      bool hasRefinement ();
      bool isAllConverged ();
      bool isSequentialConverged (struct projectData *, double);
      bool get_hasRadiation ();
      void set_solve_time (double t) {solve_time=t;}
      double get_solve_time () {return solve_time;}
      int get_lastIteration (double);
      bool SparameterConversion (int, double, BoundaryDatabase *, lapack_complex_double *, lapack_complex_double*);
      void print();
      bool save_as_test (string, struct projectData *, int *);
      bool loadCSV (const char *);
};

#endif

