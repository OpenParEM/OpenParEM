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

#ifndef HANDLER_H
#define HANDLER_H

#include "petscsys.h"
#include <iostream>
#include <chrono>
#include "prefix.h"

using namespace std;

PetscErrorCode errorHandler (MPI_Comm, int, const char *, const char *, PetscErrorCode, PetscErrorType, const char *, void *);

struct applicationContext {
   chrono::steady_clock::time_point job_start_time;
   const char *lockfile;
   char *prefix_text;
   int tool;      // 2 for OpenParEM2D, 3 for OpenParEM3D
};

#endif

