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

#ifndef EIGENSOLVE_H
#define EIGENSOLVE_H

#include <slepceps.h>
#include <complex.h>
#include "project.h"
#include "triplet.h"

double* allocReaddof (char *, char *, size_t *);
void printdof (double *, size_t);
FILE* openDataFile (char *, const char *, char *, int);
int loadDataLine (FILE *, struct dataTriplet *, int);
int loadDataFileStats (const char *, char *, char *, PetscInt *, PetscInt *, PetscInt *);
int loadDataFile (const char *, char *, char *, Mat *, PetscInt, PetscInt, int, int, double, PetscMPIInt);
int Hsetup (struct projectData *, Mat *, Mat *, Mat *, Mat *, Mat *, PetscMPIInt);
int Hsolve (struct projectData *, Mat *, Mat *, Mat *, Mat *, Mat *, PetscInt, PetscInt, Vec *, PetscScalar *, Vec *);

#endif

