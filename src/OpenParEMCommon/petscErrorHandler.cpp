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

#include "petscErrorHandler.hpp"

void exit_job_on_error (chrono::steady_clock::time_point, const char *, bool, int);

// tool: 2 for OpenParEM2D, 3 for OpenParEM3D
PetscErrorCode errorHandler (MPI_Comm comm, int line, const char *fun, const char *file, PetscErrorCode n, PetscErrorType p, const char *mess, void *ctx)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   PetscErrorCode ierr=0;

   if (rank == 0) {
      if (n == PETSC_ERR_MEM)                 cout << ((struct applicationContext *)ctx)->prefix_text << "ERROR1117: out of memory" << endl;
      else if (n == PETSC_ERR_FP)             cout << ((struct applicationContext *)ctx)->prefix_text << "ERROR1118: floating point exception" << endl;
      else if (n == PETSC_ERR_COR)            cout << ((struct applicationContext *)ctx)->prefix_text << "ERROR1119: corrupted data object" << endl;
      else if (n == PETSC_ERR_LIB)            cout << ((struct applicationContext *)ctx)->prefix_text << "ERROR1120: unspecified library error" << endl;
      else if (n == PETSC_ERR_PLIB)           cout << ((struct applicationContext *)ctx)->prefix_text << "ERROR1121: inconsistent data" << endl;
      else if (n == PETSC_ERR_MEMC)           cout << ((struct applicationContext *)ctx)->prefix_text << "ERROR1122: memory corruption" << endl;
      else if (n == PETSC_ERR_CONV_FAILED)    cout << ((struct applicationContext *)ctx)->prefix_text << "ERROR1123: failed convergence" << endl;
      else if (n == PETSC_ERR_POINTER)        cout << ((struct applicationContext *)ctx)->prefix_text << "ERROR1124: invalid pointer" << endl;
      else if (n == PETSC_ERR_NOT_CONVERGED)  cout << ((struct applicationContext *)ctx)->prefix_text << "ERROR1125: solver did not converge" << endl;
      else                                    cout << ((struct applicationContext *)ctx)->prefix_text << "ERROR1126: " << mess << endl;
   }

   exit_job_on_error (((struct applicationContext *)ctx)->job_start_time,((struct applicationContext *)ctx)->lockfile,true, ((struct applicationContext *)ctx)->tool);

   return ierr;
}

