////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM3D - A fullwave 3D electromagnetic simulator.                  //
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

#ifndef FEM3D_H
#define FEM3D_H

#include "mfem.hpp"
#include "petscsys.h"
#include <petsc.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <complex>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <_hypre_parcsr_mv.h>
#include "port.hpp"
#include "project.h"
#include "csr.h"
#include "OPEM_L2ZZErrorEstimator.hpp"
#include "pattern.hpp"

using namespace std;
using namespace mfem;

#define lapack_int int
#define lapack_complex_double std::complex<double>

void test_is_point_on_line ();
extern "C" void matrixPrint (lapack_complex_double *, lapack_int);
extern "C" void matrixDiagonalPrint (lapack_complex_double *, lapack_int);
extern "C" void linearPrint (lapack_complex_double *, lapack_int);
extern "C" void matrixZero (lapack_complex_double *, lapack_int);
extern "C" void matrixTranspose (lapack_complex_double *, lapack_int);
extern "C" void matrixConjugate (lapack_complex_double *, lapack_int);
extern "C" void matrixScale (lapack_complex_double *, double, double, lapack_int);
extern "C" int matrixInverse (lapack_complex_double *, lapack_int);
extern "C" void matrixMultiply (lapack_complex_double *, lapack_complex_double *, lapack_int);
extern "C" void matrixSum (lapack_complex_double *, lapack_complex_double *, lapack_int);
extern "C" lapack_complex_double* matrixClone (lapack_complex_double *, lapack_int);
extern "C" void show_memory (int, const char *);
extern "C" PetscErrorCode MatInvert (Mat *, int);

int getGlobalNE (ParMesh *);
int getGlobalNBE (ParMesh *);

void process_mem_usage(double&, double&);
void show_memory (int, string);

class BoundaryDatabase;

class fem3D {
   private:

      ND_FECollection *fec_ND;
      ParFiniteElementSpace *fespace_ND;

      RT_FECollection *fec_RT;
      ParFiniteElementSpace *fespace_RT;

      H1_FECollection *fec_H1;
      ParFiniteElementSpace *fespace_H1;

      L2_FECollection *fec_L2;
      ParFiniteElementSpace *fespace_L2;

      Mat A;                                  // for E, the electric field: volume integrations
      Vec X;                                  //                            port eigenvector for the driven mode
      Vec Xdofs;                              //                            mark the driven port dofs
      Vec x;                                  // solved E dofs; x from the Ax=b problem
      Mat P,Q;                                // for H, the magnetic field; volume integrations
      Mat R;                                  // for E re-calculated from H as part of the mesh error calculation

      Vector *e_re=nullptr;                   // for E-field grids
      Vector *e_im=nullptr;
      Vector *h_re=nullptr;                   // for H-field grids
      Vector *h_im=nullptr;
      PetscInt nPEC,*PEC;                     // PEC dofs

      double frequency;
      int drivingSet;

      struct projectData *projData;           // not owned
      ParMesh **pmesh;

      // mesh elements for refinement - cummulative across driven Sports
      int refinementCount;   // per element of refinementList - all the same
      vector<double> errors; // local mesh errors
      vector<int> elements;  // local element numbers
      vector<int> ranks;     // ranks for each mesh error

      // results for the last driven Sport

      ParGridFunction *gridReE=nullptr;         // full 3D solution
      ParGridFunction *gridImE=nullptr;
      ParGridFunction *gridReH=nullptr;
      ParGridFunction *gridImH=nullptr;
      ParGridFunction *gridReExH=nullptr;       // 1/2ExH*
      ParGridFunction *gridImExH=nullptr;

      //ParGridFunction *gridReEz=nullptr;      // component normal to the port from the 3D solution
      //ParGridFunction *gridImEz=nullptr;
      //ParGridFunction *gridReHz=nullptr;
      //ParGridFunction *gridImHz=nullptr;

      PetscReal EfieldError,HfieldError;
      PetscInt EfieldConverged,HfieldConverged;
      PetscInt matrixSize;
      PetscInt sparseWidth;                     // 3D solution matrix size is matrixSize*sparseWidth*16

      Mat Mc;                                   // for S-parameter conversions
      Mat Ms; 
      vector<complex<double>> SportZoList;

      double radius;                            // radius of the sphere for far-field calculations
      double radiation_beta;                    // propagation constant for the volume surrounding the antenna in the far field
      double wave_impedance;                    // wave impedance for the volume surrounding the antenna in the far field

   public:
      ~fem3D();
      void set_data(ParMesh **, struct projectData *, double);
      PetscReal get_EfieldError() {return EfieldError;}
      PetscReal get_HfieldError() {return HfieldError;}
      PetscInt get_EfieldConverged() {return EfieldConverged;}
      PetscInt get_HfieldConverged() {return HfieldConverged;}
      PetscInt get_matrixSize() {return matrixSize;}
      PetscInt get_sparseWidth() {return sparseWidth;}
      void build_fe_spaces ();
      void build_PEC_dofs ();
      bool build_A (BoundaryDatabase *, MaterialDatabase *, double, PWConstCoefficient *,
         PWConstCoefficient *, PWConstCoefficient *, bool, string);
      void build_P ();
      void build_Q (PWConstCoefficient *);
      void build_R (PWConstCoefficient *, PWConstCoefficient *);
      PetscErrorCode build_X ();
      PetscErrorCode build_x ();
      PetscErrorCode build_b ();
      void build_grids ();
      void build_portEssTdofLists (BoundaryDatabase *);
      PetscErrorCode build_Xdofs ();
      void build_e_re_e_im ();
      void build_h_re_h_im (Vec *);
      void save_ReXd_ImXd (BoundaryDatabase *);
      void buildEgrids (BoundaryDatabase *);
      void buildHgrids (BoundaryDatabase *, PWConstCoefficient *);
      void buildZgrids (BoundaryDatabase *);
      void build_ReExHgrid ();
      void build_ImExHgrid ();
      bool solve (BoundaryDatabase *, MaterialDatabase *, double, PWConstCoefficient *, PWConstCoefficient *,
                  PWConstCoefficient *, PWConstCoefficient *, int, bool, bool, string);
      void printElementVertices ();
      bool calculateMeshErrors (struct projectData *, BoundaryDatabase *, PWConstCoefficient *, double *);
      void refineMesh ();
      ParFiniteElementSpace* get_fespace_ND () {return fespace_ND;}
      ParFiniteElementSpace* get_fespace_H1 () {return fespace_H1;}
      void saveParaView (bool, bool, string);
      void saveFieldValuesHeader (struct projectData *);
      void saveFieldValues (struct projectData *, ParMesh *, int, int);
      void saveFieldValues2 (struct projectData *, ParMesh *, int, int);
      ParGridFunction* get_gridReE () {return gridReE;}
      ParGridFunction* get_gridImE () {return gridImE;}
      ParGridFunction* get_gridReH () {return gridReH;}
      ParGridFunction* get_gridImH () {return gridImH;}
      //ParGridFunction* get_gridReEz () {return gridReEz;}
      //ParGridFunction* get_gridImEz () {return gridImEz;}
      //ParGridFunction* get_gridReHz () {return gridReHz;}
      //ParGridFunction* get_gridImHz () {return gridImHz;}
      PetscErrorCode build_Mc_Ms (double, BoundaryDatabase *, vector<DifferentialPair*> *, int);
      Mat* get_Mc () {return &Mc;}
      Mat* get_Ms () {return &Ms;}
      vector<complex<double>>* get_SportZoList () {return &SportZoList;}
      void calculateRadiationCurrents (BoundaryDatabase *);
      void calculateRadiationPatterns (int, complex<double>, int, BoundaryDatabase *, PatternDatabase *);
};

#endif

