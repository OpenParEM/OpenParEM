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

#include "fileCleanup.hpp"

bool has_results_files (const char *baseName, int portCount)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank == 0) {
      stringstream ss;
      ss << ".s" << portCount << "p";

      if (exists_file(baseName,"","_results.csv")) return true;
      if (exists_file(baseName,"","_FarField_results.csv")) return true;
      if (exists_file(baseName,"","_FarField.csv")) return true;
      if (exists_file(baseName,"","_results.txt")) return true;
      if (exists_file(baseName,"","_iterations.txt")) return true;
      if (exists_file(baseName,"","_prototype_test_cases.csv")) return true;
      if (exists_file(baseName,"","_fields.csv")) return true;
      if (exists_file(baseName,"","_attributes.csv")) return true;
      if (exists_file(baseName,"",ss.str().c_str())) return true;
      if (exists_file(baseName,"temp_","")) return true;
      if (exists_file(baseName,"ParaView_","")) return true;
      if (exists_file(baseName,"ParaView_","_FarField")) return true;
      if (exists_file(baseName,"Report_","_FarField")) return true;
      if (exists_file(baseName,"ParaView_2D_port_","")) return true;
      if (exists_file(baseName,"ParaView_3D_port_","")) return true;
      if (exists_file(baseName,"ParaView_modal_2D_","")) return true;
      if (exists_file(baseName,"ParaView_solution_2D_","")) return true;
   }
   MPI_Barrier(PETSC_COMM_WORLD);
   return false;
}

void delete_stale_files (const char *baseName, int portCount)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank == 0) {
      stringstream ss;
      ss << ".s" << portCount << "p";

      delete_file(baseName,"","_results.csv");
      delete_file(baseName,"","_FarField_results.csv");
      delete_file(baseName,"","_FarField.csv");
      delete_file(baseName,"","_results.txt");
      delete_file(baseName,"","_iterations.txt");
      delete_file(baseName,"","_prototype_test_cases.csv");
      delete_file(baseName,"","_fields.csv");
      delete_file(baseName,"","_attributes.csv");
      delete_file(baseName,"",ss.str().c_str());
      delete_file(baseName,"temp_","");
      delete_file(baseName,"ParaView_","");
      delete_file(baseName,"ParaView_","_FarField");
      delete_file(baseName,"Report_","_FarField");
      delete_file(baseName,"ParaView_2D_port_","");
      delete_file(baseName,"ParaView_3D_port_","");
      delete_file(baseName,"ParaView_modal_2D_","");
      delete_file(baseName,"ParaView_solution_2D_","");
   }
   MPI_Barrier(PETSC_COMM_WORLD);
}

