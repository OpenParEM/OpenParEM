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

#ifndef FEM2D_H
#define FEM2D_H

#include "mfem.hpp"
#include "mesh.hpp"
#include <slepceps.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <complex>
#include <cstdlib>
#include <filesystem>
#include <string>
#include "modes.hpp"
#include "project.h"
#include "convergence.hpp"
#include "fieldPoints.hpp"
#include "results.hpp"
#include "Hsolve.h"

using namespace std;
using namespace mfem;

#define lapack_int int
#define lapack_complex_double complex<double>

void test_is_point_on_line ();
extern "C" void matrixPrint(lapack_complex_double *, lapack_int);
extern "C" void matrixDiagonalPrint(lapack_complex_double *, lapack_int);
extern "C" void matrixSetValue(lapack_complex_double *, lapack_int, double, double);
extern "C" void matrixScaleValue(lapack_complex_double *, lapack_int, double, double);
extern "C" void  matrixScaleRow (lapack_complex_double *, lapack_int, lapack_int, double, double);
extern "C" double matrixGetRealValue (lapack_complex_double *, lapack_int);
extern "C" double matrixGetImagValue (lapack_complex_double *, lapack_int);
extern "C" double matrixGetRealScaleValue (lapack_complex_double *, lapack_int, double, double);
extern "C" double matrixGetImagScaleValue (lapack_complex_double *, lapack_int, double, double);
extern "C" void linearPrint(lapack_complex_double *, lapack_int);
extern "C" lapack_complex_double* matrixClone (lapack_complex_double *, lapack_int);
extern "C" void matrixZero (lapack_complex_double *, lapack_int);
extern "C" void matrixTranspose(lapack_complex_double *, lapack_int);
extern "C" void matrixConjugate (lapack_complex_double *, lapack_int);
extern "C" void matrixScale (lapack_complex_double *, double, double, lapack_int);
extern "C" void matrixCopy (lapack_complex_double *, lapack_complex_double *, lapack_int);
extern "C" int matrixInverse(lapack_complex_double *, lapack_int);
extern "C" void matrixMultiply(lapack_complex_double *, lapack_complex_double *, lapack_int);
extern "C" int is_modal_impedance (char *);
extern "C" int is_line_impedance (char *);
extern "C" int is_impedance_calculation (char *);
extern "C" void prefix ();
extern "C" char* get_prefix_text ();
extern "C" void set_prefix_text (char *);

void findPoints(ParMesh *, DenseMatrix &, Array<int>&, Array<IntegrationPoint>&, int);
int GetGlobalNE (ParMesh *);

class fem2D;

class Fields {
   private:
      ParGridFunction *grid_Et_re=nullptr;
      ParGridFunction *grid_Et_im=nullptr;
      ParGridFunction *grid_Ez_re=nullptr;
      ParGridFunction *grid_Ez_im=nullptr;

      ParGridFunction *grid_Ht_re=nullptr;
      ParGridFunction *grid_Ht_im=nullptr;
      ParGridFunction *grid_Hz_re=nullptr;
      ParGridFunction *grid_Hz_im=nullptr;

      ParGridFunction *grid_Pz_re=nullptr;
      ParGridFunction *grid_Pz_im=nullptr;

      double *EfieldRe=nullptr;
      double *EfieldIm=nullptr;
      double *HfieldRe=nullptr;
      double *HfieldIm=nullptr;

      double xScale;
      double yScale;

   public:
      Fields () {};
      Fields (fem2D *);
      ~Fields();
      void flipSign();
      bool loadEigenvector (fem2D *, long unsigned int, bool, double **, double **);
      bool build (fem2D *, long unsigned int);
      double* get_EfieldRe () {return EfieldRe;}
      double* get_EfieldIm () {return EfieldIm;}
      double* get_HfieldRe () {return HfieldRe;}
      double* get_HfieldIm () {return HfieldIm;}
      void saveParaView(fem2D *, long unsigned int, bool);
      ParGridFunction* get_grid_Et_re() {return grid_Et_re;}
      ParGridFunction* get_grid_Et_im() {return grid_Et_im;}
      ParGridFunction* get_grid_Ez_re() {return grid_Ez_re;}
      ParGridFunction* get_grid_Ez_im() {return grid_Ez_im;}
      ParGridFunction* get_grid_Ht_re() {return grid_Ht_re;}
      ParGridFunction* get_grid_Ht_im() {return grid_Ht_im;}
      ParGridFunction* get_grid_Hz_re() {return grid_Hz_re;}
      ParGridFunction* get_grid_Hz_im() {return grid_Hz_im;}
      complex<double> get_scale () {return complex<double>(xScale,yScale);}
      void set_scale (double xScale_, double yScale_) {xScale=xScale_; yScale=yScale_;}
      void updateGrids();
      void writeInitialGuess (fem2D *, long unsigned int);
      complex<double> calculatePz (fem2D *);
      complex<double> calculateLineIntegral (fem2D *, Boundary *, BoundaryDatabase *, BorderDatabase *, bool);
      void calculateFieldPoints (fem2D *, int, FieldPointDatabase *);
      double calculatePerturbationalLoss(fem2D *, BoundaryDatabase *, BorderDatabase *, MaterialDatabase *);
      void copyEH (Fields *);
      void weightEH (complex<double>);
      void weightAndAddEH (Fields *, complex<double>);
      void print();
      void save_grid_E(long unsigned int);
      void saveAsOne_grid_E(long unsigned int);
};

// all solutions to the eigenvalue problem are eigenmodes
class Mode {
   private:
      long unsigned int modeNumber;       // 0 based
      vector<complex<double>> current;    // currents computed for all current integration paths
      vector<complex<double>> voltage;    // voltages computed for all voltage integration paths
      complex<double> Pzavg;
      double alpha;
      double beta;
      double perturbationalLoss;
      Fields *fields=nullptr;
      complex<double> mode_current;       // the current for the mode
      bool validCurrent;
      complex<double> mode_voltage;       // the voltage for the mode
      bool validVoltage;
      complex<double> Zvi;
      complex<double> Zpv;
      complex<double> Zpi;
   public:
      Mode();
      ~Mode();
      void set_alpha (double alpha_) {alpha=alpha_;}
      void set_beta (double beta_) {beta=beta_;}
      void set_modeNumber (long unsigned int modeNumber_) {modeNumber=modeNumber_;}
      long unsigned int get_modeNumber() {return modeNumber;}
      complex<double> get_current (long unsigned int i) {return current[i];}
      complex<double> get_voltage (long unsigned int i) {return voltage[i];}
      double get_alpha() {return alpha;}
      double get_beta() {return beta;}
      complex<double> get_gamma() {return complex<double>(alpha,beta);}
      double get_perturbationalLoss() {return perturbationalLoss;}
      complex<double> get_Pzavg () {return Pzavg;}
      complex<double> get_mode_current () {return mode_current;}
      complex<double> get_mode_voltage () {return mode_voltage;}
      complex<double> get_Zvi () {return Zvi;}
      complex<double> get_Zpv () {return Zpv;}
      complex<double> get_Zpi () {return Zpi;}
      Fields* get_fields () {return fields;}
      bool buildFields (fem2D *);
      void updateGrids (fem2D *);
      void saveParaView (fem2D *);
      void writeInitialGuess (fem2D *fem) {fields->writeInitialGuess(fem,modeNumber);}
      void calculatePz (fem2D *);
      void calculateVoltages (fem2D *, BoundaryDatabase *, BorderDatabase *);
      void calculateCurrents (fem2D *, BoundaryDatabase *, BorderDatabase *);
      bool calculatePerturbationalLoss(fem2D *, BoundaryDatabase *, BorderDatabase *borderDatabase, MaterialDatabase *);
      void calculateFieldPoints (fem2D *, FieldPointDatabase *);
      void flipFieldSign ();
      bool calculateModalCurrent (fem2D *);
      bool calculateModalVoltage (fem2D *);
      void calculateImpedance (fem2D *);
      bool ZZrefineMesh (fem2D *, ConvergenceDatabase *);
      void print();
      void save_grid_E(long unsigned int);
      void saveAsOne_grid_E(long unsigned int);
};

class ModeDatabase {
   private:
      vector<Mode *> modeList;
   public:
      ~ModeDatabase();
      long unsigned int get_size() {return modeList.size();}
      Mode* get_mode (long unsigned int i) {return modeList[i];}
      bool buildFields(fem2D *, double *, double *);
      void saveParaView(fem2D *);
      bool ZZrefineMeshes(fem2D *, ConvergenceDatabase *);
      void writeInitialGuess (fem2D *);
      void calculatePz (fem2D *);
      void calculateVandI (fem2D *, BoundaryDatabase *, BorderDatabase *);
      bool calculateModalVandI (fem2D *);
      void calculateImpedance (fem2D *, BoundaryDatabase *, BorderDatabase *);
      bool calculatePerturbationalLoss(fem2D *, BoundaryDatabase *, BorderDatabase *, MaterialDatabase *);
      void calculateFieldPoints (fem2D *, FieldPointDatabase *);
      void sort (fem2D *);
      void print();
      void save_grid_E();
      void saveAsOne_grid_E();
};

class fem2D {
   private:

      // Et, Ht
      ND_FECollection *fec_ND=nullptr;
      ParFiniteElementSpace *fespace_ND=nullptr;

      // Ez, Hz
      H1_FECollection *fec_H1=nullptr;
      ParFiniteElementSpace *fespace_H1=nullptr;

      // Pz
      L2_FECollection *fec_L2=nullptr;
      ParFiniteElementSpace *fespace_L2=nullptr;

      int ess_tdof_size_ND;
      int ess_tdof_size_H1;

      struct projectData *projData;  // not owned
      int iteration;

      ParMesh *pmesh;                // not owned
      string projectName;
      string temporaryDirectory;
      double frequency;
      int order;
      ModeDatabase modeDatabase;
      Array<int> border_attributes;

      size_t t_size;
      size_t z_size;
      double meshScale;
      int startingMeshSize;
      int pointsCount=100;  // for line integrals

      lapack_complex_double* Ti;   // for conversion between modal and line currents
      lapack_complex_double* Tv;   // for conversion between modal and line voltages

      friend class Fields;
      friend class Mode;
      friend class ModeDatabase;

   public:
      fem2D (struct projectData *, ParMesh *, int, double, int, 
             PWConstCoefficient *, PWConstCoefficient *, PWConstCoefficient *, PWConstCoefficient *, string, bool *);
      ~fem2D();
      struct projectData * get_projectData () {return projData;}
      void set_t_size () {t_size=fespace_ND->GlobalTrueVSize();}
      void set_z_size () {z_size=fespace_H1->GlobalTrueVSize();}
      size_t get_ND_size () {return fespace_ND->GlobalTrueVSize();}
      size_t get_H1_size () {return fespace_H1->GlobalTrueVSize();}
      PetscInt* get_ess_tdof_ND (BoundaryDatabase *, BorderDatabase *);
      PetscInt* get_ess_tdof_H1 (BoundaryDatabase *, BorderDatabase *);
      bool buildFields (double *, double *);
      bool ZZrefineMeshes (ConvergenceDatabase *);
      void writeInitialGuess ();
      void calculateVandI (BoundaryDatabase *, BorderDatabase *);
      void buildTiTv ();
      bool saveTiTv ();
      bool calculateModalVandI ();
      void calculateImpedance (BoundaryDatabase *, BorderDatabase *);
      bool calculatePerturbationalLoss (BoundaryDatabase *, BorderDatabase *, MaterialDatabase *);
      void calculateFieldPoints (FieldPointDatabase *);
      void saveParaView ();
      Result* updateResults (ResultDatabase *, ConvergenceDatabase *, chrono::steady_clock::time_point, chrono::steady_clock::time_point);
      int get_ess_tdof_size_ND () {return ess_tdof_size_ND;}
      int get_ess_tdof_size_H1 () {return ess_tdof_size_H1;}
      string get_temporaryDirectory() {return temporaryDirectory;}
      void sort () {modeDatabase.sort(this);}
      void save_grid_E () {modeDatabase.save_grid_E();}
      void saveAsOne_grid_E () {modeDatabase.saveAsOne_grid_E();}
      void dumpDof2DData ();
      void printBoundaryAttributes ();
};

#endif
