////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM2D - A fullwave 2D electromagnetic simulator.                  //
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

#include "jobrelated.hpp"

void exit_job_on_error (chrono::steady_clock::time_point job_start_time, const char *lockfile, bool removeLock)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Job Complete\n");

   // remove the lock - not 100% safe
   if (rank == 0 && removeLock) {
      if (std::filesystem::exists(lockfile)) {
         std::filesystem::remove(lockfile);
      }
   }

   chrono::steady_clock::time_point job_end_time=chrono::steady_clock::now();
   chrono::duration<double> elapsed = job_end_time - job_start_time;
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Elapsed time: %g s\n",elapsed.count());

   // send an exit code to the parent with tag 100000
   MPI_Comm parent;
   MPI_Comm_get_parent (&parent);
   if (parent != MPI_COMM_NULL) {
      int retval=1;
      MPI_Send(&retval,1,MPI_INT,0,100000,parent);
      MPI_Comm_free(&parent);
   }

   PetscFinalize();

   exit(1);
}

// this method is not guaranteed
char* create_lock_file (const char *baseName)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   stringstream ssLock;
   ssLock << "." << baseName << ".lock";

   char *lockfile;
   lockfile=(char *) malloc((strlen(ssLock.str().c_str())+1)*sizeof(char));
   sprintf (lockfile,"%s",ssLock.str().c_str());

   int is_locked=0;
   if (rank == 0) {
      if (std::filesystem::exists(lockfile)) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1015: Project \"%s.proj\" is locked with file \"%s\".\n",baseName,ssLock.str().c_str());
         is_locked=1;
      }
   }

   MPI_Bcast(&is_locked,1,MPI_INT,0,PETSC_COMM_WORLD);
   if (is_locked) {
      PetscFinalize();
      exit(1);
   }

   // assume that the file does not exist
   if (rank == 0) {
      ofstream lock;
      lock.open(lockfile,ofstream::out);
      if (lock.is_open()) {
         lock << "locked" << endl;
         lock.flush();
         lock.close();
         is_locked=1;
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1016: Cannot open \"%s\" for writing.\n",lockfile);
      }

      // wait for the lock file to appear
      chrono::duration<double> elapsed;
      chrono::steady_clock::time_point start;
      chrono::steady_clock::time_point current;
      start=chrono::steady_clock::now();
      current=chrono::steady_clock::now();
      elapsed=current-start;
      while (!std::filesystem::exists(lockfile)) {
         current=chrono::steady_clock::now();
         elapsed=current-start;
         if (elapsed.count() > 60) { // wait up to 60 seconds before proceeding anyway
            break;
         }
      }
   }

   MPI_Bcast(&is_locked,1,MPI_INT,0,PETSC_COMM_WORLD);
   if (! is_locked) {
      PetscFinalize();
      exit(1);
   }

   return lockfile;
}

// not 100% safe
void remove_lock_file (const char *lockfile)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   if (rank == 0) {
      if (std::filesystem::exists(lockfile)) {
        std::filesystem::remove(lockfile);
      }
   }
}

void delete_file (const char *baseName, string pre, string post)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank == 0) {
      stringstream ss;
      ss << pre << baseName << post;
      if (std::filesystem::exists(ss.str().c_str())) {
        std::filesystem::remove_all(ss.str().c_str());
      }
   }
}

double elapsed_time (chrono::steady_clock::time_point start, chrono::steady_clock::time_point finish)
{
   chrono::duration<double> elapsed=finish-start;
   return elapsed.count();
}


