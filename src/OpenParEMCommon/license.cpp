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

#include "license.hpp"

void print_copyright_notice (string program_name, int major, int minor, int patch)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s %d.%d.%d Copyright (C) 2025 Brian Young\n",program_name.c_str(),major,minor,patch);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"This program comes with ABSOLUTELY NO WARRANTY.  This is free software,\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"and you are welcome to distribute it under certain conditions. To see\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"the full license, set\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"  output.show.license true\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"in the setup file.\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"\n");
}

void print_license()
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Printing license text ...\n\n");
   if (rank == 0) {
      #include "formatted_license.txt"
   }
}
