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

#ifndef LINEARSYSTEM_H
#define LINEARSYSTEM_H

#include <petsc.h>
#include <complex.h>
#include "project.h"
#include <_hypre_parcsr_mv.h>
#include "triplet.h"
#include <time.h>

#define lapack_int int
#define lapack_complex_double double complex

double lapack_complex_double_real ();
double lapack_complex_double_imag ();

//double* allocReaddof (char *, char *, size_t *);
//void printdof (double *, size_t);
FILE* openDataFile (const char *, const char *, char *, int);
int loadDataLine (FILE *, struct dataTriplet *, int);
int loadDataFileStats (const char *, const char *, char *, PetscInt *, PetscInt *, PetscInt *);
int loadDataFile (const char *, const char *, char *, Mat *, PetscInt, PetscInt, int, int, double, PetscMPIInt);

void matrixPrint(lapack_complex_double *, lapack_int);
void matrixDiagonalPrint(lapack_complex_double *, lapack_int);
void linearPrint(lapack_complex_double *, lapack_int);
lapack_complex_double* matrixClone (lapack_complex_double *, lapack_int);
void matrixTranspose(lapack_complex_double *, lapack_int);
void matrixConjugate (lapack_complex_double *, lapack_int);
void matrixScale (lapack_complex_double *, lapack_complex_double *, lapack_int);
void matrixCopy (lapack_complex_double *, lapack_complex_double *, lapack_int);
int matrixInverse(lapack_complex_double *, lapack_int);
void matrixMultiply(lapack_complex_double *, lapack_complex_double *, lapack_int);

void remove_lock_file (const char *);

#endif

