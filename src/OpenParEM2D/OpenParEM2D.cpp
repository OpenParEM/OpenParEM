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

// Run:          mpirun -np N OpenParEM2D project.proj
//               mpirun -np N --quiet OpenParEM2D project.proj
//
// Description:  This code solves Maxwell's equations in 2D for waveguide/transmission line 
//               eigenvalue/eigenvector pairs for the propagating modes.
//
//               The code is an implementation of the formulation in the paper:
//                   Lee, Jin-Fa, "Finite Element Analysis of Lossy Dielectric Waveguides,"
//                   IEEE Trans. Microwave Theory Techniques, vol. 42, no. 6, June 1994, pp. 1025-1031.
//

#include "mfem.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <complex>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <chrono>
#include <unistd.h>
#include "modes.hpp"
#include "results.hpp"
#include "convergence.hpp"
#include "fem2D.hpp"
#include "project.h"
#include "fieldPoints.hpp"
#include "jobrelated.hpp"
#include "frequencyPlan.hpp"
#include "petscErrorHandler.hpp"
#include "license.hpp"

using namespace std;
using namespace mfem;

int version_major=2;
int version_minor=1;
int version_patch=0;

extern "C" int eigensolve (struct projectData *, int, double, int, PetscInt *, int, PetscInt *, double **, double **, int *, PetscMPIInt);
extern "C" void init_project (struct projectData *);
extern "C" int load_project_file(const char*, projectData*, const char*);
extern "C" void print_project (struct projectData *, struct projectData *, const char *indent);
extern "C" void free_project(projectData*);
extern "C" void matrixTest();
extern "C" void colMajorTest();
extern "C" char* get_project_name (const char *);
extern "C" int is_modal_impedance (char *);
extern "C" void prefix ();
extern "C" char* get_prefix_text ();
extern "C" void set_prefix_text (char *);

void help () {
   PetscPrintf(PETSC_COMM_WORLD,"usage: OpenParEM2D [-h] filename\n");
   PetscPrintf(PETSC_COMM_WORLD,"       -h          : Print this help text\n");
   PetscPrintf(PETSC_COMM_WORLD,"       filename    : Filename of an OpenParEM setup file.\n");
   PetscPrintf(PETSC_COMM_WORLD,"\nOpenParEM2D is a full-wave 2D electromagnetic solver.\n");
   PetscPrintf(PETSC_COMM_WORLD,"Version %d.%d.%d\n",version_major,version_minor,version_patch);
}

// load the mesh - either serial or parallel
bool load_mesh (struct projectData *projData, Mesh **mesh, ParMesh **pmesh, MeshMaterialList **meshMaterials, int *dim)
{
   PetscMPIInt size,rank;
   MPI_Comm_size(PETSC_COMM_WORLD, &size);
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   bool foundSerialMesh=false;
   bool foundParallelMesh=false;

   // look for serial mesh
   if (std::filesystem::exists(projData->mesh_file)) {
      foundSerialMesh=true;
      ifstream meshFile;
      meshFile.open(projData->mesh_file);
      if (meshFile.is_open()) {
         *mesh=new Mesh;
         (*mesh)->Load(meshFile, 1, projData->mesh_enable_refine);
         meshFile.close();
         reset_attributes (*mesh,*pmesh,*meshMaterials);
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2206: Failed to open mesh file \"%s\" for reading.\n",projData->mesh_file);
         return true;
      }
      *dim=(*mesh)->Dimension();

      int i=0;
      while (i < projData->mesh_uniform_refinement_count) {
         (*mesh)->UniformRefinement();
         i++;
      }
   }

   // look for parallel mesh

   int mesh_count=0;
   while (true) {
      stringstream ss;
      ss << projData->mesh_file << "." << setfill('0') << setw(6) << mesh_count;
      if (!std::filesystem::exists(ss.str().c_str())) break;
      ss.str("");
      ss.clear();
      mesh_count++;
   }

   if (mesh_count > 0) {
      if (mesh_count == size) foundParallelMesh=true;
      else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2207: Improperly sized parallel mesh found.\n");
         return true;
      }
   }

   if (foundParallelMesh) {
      stringstream ssName;
      ssName << projData->mesh_file << ".";

      string fname(MakeParFilename(ssName.str(),rank));
      ifstream ifs(fname);
      if (ifs.is_open()) {
         *pmesh=new ParMesh(PETSC_COMM_WORLD,ifs,projData->mesh_enable_refine);
         ifs.close();
         reset_attributes (*mesh,*pmesh,*meshMaterials);
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2208: Failed to open mesh file \"%s\" for reading.\n",fname.c_str());
         return true;
      }

      int i=0;
      while (i < projData->mesh_uniform_refinement_count) {
         (*pmesh)->UniformRefinement();
         i++;
      }
      *dim=(*pmesh)->Dimension();
   }

   // final check for valid mesh
   if (!foundSerialMesh && !foundParallelMesh) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2209: Missing valid mesh file.\n");
      return true;
   }

   // if both meshes are present, prefer pmesh over mesh for now
   if (*mesh && *pmesh) {delete *mesh; *mesh=nullptr;}

   return false;
}

void process_mem_usage(double& vm_usage, double& resident_set)
{
    vm_usage     = 0.0;
    resident_set = 0.0;

    // the two fields we want
    unsigned long vsize;
    long rss;
    {
        std::string ignore;
        std::ifstream ifs("/proc/self/stat", std::ios_base::in);
        ifs >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
                >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
                >> ignore >> ignore >> vsize >> rss;
    }

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    vm_usage = vsize / 1024.0;
    resident_set = rss * page_size_kb;
}

void printSummary(double frequency, ResultDatabase *resultDatabase, int iteration)
{
   double NpTodB=20*log10(exp(1));
   double eps0=8.8541878176e-12;
   double ko=2*M_PI*frequency*sqrt(4.0e-7*M_PI*eps0);

   Result *result=resultDatabase->get_Result(frequency,iteration);

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"      Results:\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         -------------------------------------------------------------------------------------------------------------------------\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"          mode     frequency           er,eff       alpha,dB/m  beta/1000,rad/m          beta/ko         Zo(real)         Zo(imag)\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         -------------------------------------------------------------------------------------------------------------------------\n");

   long unsigned int mode=0;
   while (mode < result->get_modeCount()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"      %5ld",mode+1);
      PetscPrintf(PETSC_COMM_WORLD,"%17.8g",frequency);

      PetscPrintf(PETSC_COMM_WORLD,"%17.8g",pow(imag(result->get_gamma(mode))/ko,2));
      PetscPrintf(PETSC_COMM_WORLD,"%17.8g",real(result->get_gamma(mode))*NpTodB+result->get_alpha_perturbation(mode)/real(result->get_Pz(mode))/2*NpTodB);
      PetscPrintf(PETSC_COMM_WORLD,"%17.8g",imag(result->get_gamma(mode))/1000);
      PetscPrintf(PETSC_COMM_WORLD,"%17.8g",imag(result->get_gamma(mode))/ko);

      if (real(result->get_Z(mode)) != DBL_MAX) PetscPrintf(PETSC_COMM_WORLD,"%17.8g",real(result->get_Z(mode)));
      else PetscPrintf(PETSC_COMM_WORLD,"                 ");

      if (imag(result->get_Z(mode)) != DBL_MAX) PetscPrintf(PETSC_COMM_WORLD,"%17.8g",imag(result->get_Z(mode)));
      else PetscPrintf(PETSC_COMM_WORLD,"                 ");

      PetscPrintf(PETSC_COMM_WORLD,"\n");

      mode++;
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         -------------------------------------------------------------------------------------------------------------------------\n");

   return;
}

void show_memory (int show, string space)
{
   double vm_usage, resident_set;

   if (show) {
      process_mem_usage(vm_usage, resident_set);
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%smemory (kB): %g  page size (kB): %g\n",space.c_str(),vm_usage,resident_set);
   }
}

void load_project_file (const char *projFile, struct projectData *defaultData, struct projectData *projData, char *lockfile, chrono::steady_clock::time_point job_start_time)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   loading project file \"%s\"\n",projFile);

   init_project (defaultData);
   init_project (projData);

   if (load_project_file (projFile,projData,"   ")) {
      if (projData->debug_show_project) {print_project (projData,defaultData,"      ");}
      exit_job_on_error (job_start_time,lockfile,true);
   }
   if (projData->debug_show_project) {print_project (projData,defaultData,"      ");}
}

void delete_stale_files (const char *baseName)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank == 0) {
      delete_file(baseName,"temp_","");
      delete_file(baseName,"ParaView_","");
      delete_file(baseName,"","_results.csv");
      delete_file(baseName,"","_fields.csv");
      delete_file(baseName,"","_prototype_test_cases.csv");
      delete_file(baseName,"","_test_prototype_test_cases.csv");
   }
   MPI_Barrier(PETSC_COMM_WORLD);
}

char* create_temp_directory (struct projectData *projData, chrono::steady_clock::time_point job_start_time, char *lockfile)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   stringstream ss;
   ss << "temp_" << projData->project_name;

   char *tempdir;
   tempdir=(char *) malloc((strlen(ss.str().c_str())+1)*sizeof(char));
   sprintf (tempdir,"%s",ss.str().c_str());

   int is_created=1;
   if (rank == 0) {
      if (! std::filesystem::create_directory(tempdir)) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2210: Failed to create results directory \"%s\".\n",tempdir);
         is_created=0;
      }
   }

   MPI_Bcast(&is_created,1,MPI_INT,0,PETSC_COMM_WORLD);
   if (! is_created) {
         exit_job_on_error (job_start_time,lockfile,true);
   }

   return tempdir;
}

int main(int argc, char *argv[])
{
   double eps0=8.8541878176e-12;
   const char *projFile;
   char* prefix_text;
   struct projectData projData,defaultData;
   BoundaryDatabase boundaryDatabase;
   BorderDatabase borderDatabase;
   ResultDatabase resultDatabase;
   ConvergenceDatabase *convergenceDatabase;
   FieldPointDatabase fieldPointDatabase;
   FrequencyPlan frequencyPlan;
   MeshMaterialList meshMaterials;
   MaterialDatabase localMaterialDatabase;
   MaterialDatabase materialDatabase;

   // Initialize Petsc and MPI
   SlepcInitializeNoArguments();
   PetscMPIInt size,rank;
   MPI_Comm_size(PETSC_COMM_WORLD, &size);
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   MPI_Barrier(PETSC_COMM_WORLD);

   MPI_Comm parent;
   MPI_Comm_get_parent (&parent);
   prefix_text=(char *)malloc(256*sizeof(char));
   if (parent == MPI_COMM_NULL) snprintf(prefix_text,256,"%s","");
   else snprintf(prefix_text,256,"%s","         | ");
   set_prefix_text(prefix_text);

   // trap PETSc errors to enable graceful exit, primarily for out-of-memory errors
   struct applicationContext appCtx;
   PetscPushErrorHandler(errorHandler,(struct applicationContext *) &appCtx);

   chrono::steady_clock::time_point job_start_time=chrono::steady_clock::now();

   // parse inputs
   int retVal=1;
   int printHelp=0;
   if (argc <= 1) printHelp=1;
   else {
      if (strcmp(argv[1],"-h") == 0) {printHelp=1; retVal=0;}
      else projFile=argv[1];
   }
   if (printHelp) {help(); /*PetscFinalize()*/ SlepcFinalize();; exit(retVal);}

   char *baseName=get_project_name(projFile);
   print_copyright_notice ("OpenParEM2D",version_major,version_minor,version_patch);
   char *lockfile=create_lock_file(baseName);

   appCtx.job_start_time=job_start_time;
   appCtx.lockfile=lockfile;
   appCtx.prefix_text=strdup(get_prefix_text());

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Setting up ...\n");

   // project
   load_project_file(projFile,&defaultData,&projData,lockfile,job_start_time);
   projData.version_major=version_major;
   projData.version_minor=version_minor;
   projData.version_patch=version_patch;
   delete_stale_files(baseName);
   char *tempdir=create_temp_directory(&projData,job_start_time,lockfile);
   if (projData.output_show_license) {print_license(); exit_job_on_error (job_start_time,lockfile,true);}
   show_memory (projData.debug_show_memory, "   "); 

   // materials
   if (materialDatabase.load_materials(projData.materials_global_path,projData.materials_global_name,
                                       projData.materials_local_path,projData.materials_local_name,
                                       projData.materials_check_limits)) exit_job_on_error (job_start_time,lockfile,true);
   if (projData.debug_show_materials) {materialDatabase.print("   ");}

   show_memory (projData.debug_show_memory, "   ");

   // get the boundary database indicating impedance integration paths and boundary conditions
   if (boundaryDatabase.load(projData.mode_definition_file,projData.solution_check_closed_loop)) exit_job_on_error (job_start_time,lockfile,true);
   if (projData.debug_show_mode_definitions) boundaryDatabase.print();

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Loading mesh and assigning materials ...\n");
   if (!projData.materials_check_limits) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"    Skipping limit checks on material values\n");
   }

   // get the mapping from the region number to the material name
   if (meshMaterials.load(projData.mesh_file,2)) exit_job_on_error (job_start_time,lockfile,true);
   //meshMaterials.print();

   // load the mesh - either serial or parallel 
   int dim;
   Mesh *mesh=nullptr;
   ParMesh *pmesh=nullptr;
   MeshMaterialList *p=&meshMaterials;
   if (load_mesh (&projData,&mesh,&pmesh,&p,&dim)) exit_job_on_error (job_start_time,lockfile,true);
   if (! (dim == 2)) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2211: Mesh must be 2-dimensional.\n"); exit_job_on_error (job_start_time,lockfile,true);}

   // check field points paths for scale
   if (check_field_points (projFile,mesh,pmesh,projData.mesh_order,2,
                           projData.field_points_count,projData.field_points_x,projData.field_points_y,nullptr)) {
      exit_job_on_error (job_start_time,lockfile,true);
   }
   if (boundaryDatabase.check_scale(mesh,pmesh,projData.mesh_order)) exit_job_on_error (job_start_time,lockfile,true);

   // mark the boundaries
   boundaryDatabase.mark_boundaries (mesh,pmesh,&borderDatabase);
   if (pmesh) {
      borderDatabase.merge(); // local->global
      borderDatabase.reassign_mesh_attributes(pmesh);
   }
   if (mesh) mesh->SetAttributes();    // recalulates the support data structures
   if (pmesh) pmesh->SetAttributes();

   // copy the pmesh for simulations requiring a restart from the original mesh
   ParMesh *restart_pmesh=nullptr;
   if (pmesh) restart_pmesh=new ParMesh(*pmesh);

   // construct coefficient vectors to hold material properties per FEM element
   int attributes_max=-1;
   if (mesh) attributes_max=mesh->attributes.Max();
   if (pmesh) attributes_max=pmesh->attributes.Max();
   Vector ko2RePermittivity(attributes_max);
   Vector ko2ImPermittivity(attributes_max);
   Vector InvPermeability(attributes_max);  // inverse of the permeability for use in (12)
   Vector InvOmegaMu(attributes_max);
   Vector OmegaMu(attributes_max);
   Vector InvOmegaMuEps(attributes_max);

   // assign materials

   //if (attributes_max != meshMaterials.size()) {
   //   prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2212: Mesh file \"%s\" does not include the correct number of regions for material definitions.\n",projData.mesh_file);
   //   prefix(); PetscPrintf(PETSC_COMM_WORLD,"       The $PhysicalNames block should have %d entries, but %d were found.\n",
   //                                        attributes_max+1,meshMaterials.size()+1);
   //   exit_job_on_error (job_start_time,lockfile,true);
   //}

   double *alphaList=(double *)malloc (projData.solution_modes*sizeof(double));
   double *betaList=(double *)malloc (projData.solution_modes*sizeof(double));

   // loop for all frequencies

   show_memory (projData.debug_show_memory, "");

   // set up the frequency plan
   if (frequencyPlan.assemble(projData.refinement_frequency,projData.inputFrequencyPlansCount,projData.inputFrequencyPlans))
        exit_job_on_error (job_start_time,lockfile,true);
   if (projData.debug_show_frequency_plan) frequencyPlan.print();
   double lastFrequency=0;  // for scaling the initial guess

   // set up pmesh when not using adaptive mesh refinement

   if (! frequencyPlan.is_refining() && mesh) {
      pmesh=new ParMesh(MPI_COMM_WORLD,*mesh);
   }
   // starting use_initial_guess state
   int use_initial_guess=0;
   if (projData.solution_initial_alpha > 0 || projData.solution_initial_beta > 0) use_initial_guess=1;

   // loop
   FrequencyPlanPoint *frequencyPlanPoint;
   double frequency=-1;
   bool refineMesh=false;
   bool restartMesh=false;
   int meshSize=0;
   while ((frequencyPlanPoint=frequencyPlan.get_frequency(projData.refinement_frequency,&frequency,&refineMesh,&restartMesh,&meshSize))) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"Frequency: %g\n",frequency);

      double ko=2*M_PI*frequency*sqrt(4.0e-7*M_PI*eps0);
      double ko2=pow(ko,2);

      // for initial guess scaling with frequency
      if (lastFrequency == 0) lastFrequency=frequency;
      double betaScale=frequency/lastFrequency;

      // set up pmesh when using adaptive mesh refinement
      if (refineMesh && restartMesh) {
         if (!pmesh) delete pmesh;
         if (mesh) pmesh=new ParMesh(MPI_COMM_WORLD,*mesh);
         else pmesh=new ParMesh(*restart_pmesh);

         // new mesh means that the existing solution is invalid unless an initial guess has been provided
         if (projData.solution_initial_alpha == 0 && projData.solution_initial_beta == 0) use_initial_guess=0;
      }

      if (refineMesh) {
         if (restartMesh) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Refining mesh starting with the initial mesh.\n");}
         else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Refining mesh starting with the last mesh.\n");}
      }

      if (!pmesh) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2213: Setup is inconsistent leading to undefined internal mesh definition.\n");
         exit_job_on_error (job_start_time,lockfile,true);
      }

      int j=0;
      while (j < pmesh->attributes.Max()) {

         Material *useMaterial=materialDatabase.get(meshMaterials.get_name(meshMaterials.get_index(j)));

         if (useMaterial != nullptr) {

            // permittivity
            complex<double> e=useMaterial->get_eps(projData.solution_temperature,frequency,materialDatabase.get_tol(),materialDatabase.get_indent());
            if (e == complex<double>(-DBL_MAX,0)) exit_job_on_error (job_start_time,lockfile,true);

            ko2RePermittivity[j]=ko2*real(e)/eps0;
            ko2ImPermittivity[j]=ko2*imag(e)/eps0;

            // permeability
            double mu=useMaterial->get_mu(projData.solution_temperature,frequency,materialDatabase.get_tol(),materialDatabase.get_indent());
            if (mu == -DBL_MAX) exit_job_on_error (job_start_time,lockfile,true);

            InvPermeability[j]=1.0/(mu/(4e-7*M_PI));    // 1/relative permeability
            InvOmegaMu[j]=1.0/(2*M_PI*frequency*mu);    // 1/(w*permeability)
            OmegaMu[j]=2*M_PI*frequency*mu;    // w*permeability

            InvOmegaMuEps[j]=1/(2*M_PI*frequency*mu*real(e));

         } else {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2214: Material \"%s\" for region %d is not present in the material database.\n",
                                         meshMaterials.get_name(j).c_str(),j+1);
            exit_job_on_error (job_start_time,lockfile,true);
         }

         j++;
      }

      PWConstCoefficient ko2Re_e(ko2RePermittivity);
      PWConstCoefficient ko2Im_e(ko2ImPermittivity);
      PWConstCoefficient Inv_mu(InvPermeability);
      PWConstCoefficient Inv_w_mu(InvOmegaMu);
      PWConstCoefficient w_mu(OmegaMu);
      PWConstCoefficient Inv_w_mu_e(InvOmegaMuEps);

      show_memory (projData.debug_show_memory, "   ");

      // set up for convergence testing
      convergenceDatabase=new ConvergenceDatabase();
      convergenceDatabase->initialize(projData.solution_modes,projData.refinement_required_passes,projData.refinement_variable,projData.refinement_tolerance);

      int iteration=0;
      bool iterate=true;
      while (iterate) {
         chrono::steady_clock::time_point solve_start_time=chrono::steady_clock::now();

         if (refineMesh) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Iteration %d ...\n",iteration+1);}
         else {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Using existing mesh ...\n");
            iterate=false;
         }
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"      mesh size: %d\n",GetGlobalNE(pmesh));
         show_memory (projData.debug_show_memory, "      ");

         double h_min,h_max,kappa_min,kappa_max;
         pmesh->GetCharacteristics(h_min, h_max, kappa_min, kappa_max);
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"      mesh worst element aspect ratio: %g",kappa_max);
         if (kappa_max < 5) {PetscPrintf(PETSC_COMM_WORLD," < target: 5\n");}
         else {PetscPrintf(PETSC_COMM_WORLD," > target: 5\n");}

         // create the finite element spaces and matrices on the parallel mesh
         fem2D *fem=new fem2D(&projData,pmesh,projData.mesh_order,frequency,iteration,&ko2Re_e,&ko2Im_e,&Inv_mu,&w_mu,tempdir);
         //fem->dumpDof2DData();

         // scale beta to approximate the change due to a shift in frequency
         if (use_initial_guess && projData.solution_active_mode_count > 0) {
            betaList[0]*=betaScale*2;  // align with similar option in OpenParEM3D
            betaScale=1;                      // will update when the frequency changes
         }

         // solve the eigenvalue problem, Ax=kBx
         int matrixSize=-1;
         PetscInt *ess_tdof_ND=fem->get_ess_tdof_ND(&boundaryDatabase,&borderDatabase);
         PetscInt *ess_tdof_H1=fem->get_ess_tdof_H1(&boundaryDatabase,&borderDatabase);
         if (projData.debug_skip_solve || eigensolve (&projData,use_initial_guess,frequency,
                                                      fem->get_ess_tdof_size_ND(),ess_tdof_ND,
                                                      fem->get_ess_tdof_size_H1(),ess_tdof_H1,
                                                      &alphaList,&betaList,&matrixSize,rank) != 0) 
         {
            // failed or canceled - stop iterating
            iterate=false;
         } else {

            // process the eigenvalue solutions
            fem->set_t_size();
            fem->set_z_size();
            if (fem->buildFields(alphaList,betaList)) exit_job_on_error (job_start_time,lockfile,true);
            fem->sort();
            fem->calculateVandI(&boundaryDatabase,&borderDatabase);
            if (fem->calculateModalVandI()) exit_job_on_error (job_start_time,lockfile,true);
            fem->calculateImpedance(&boundaryDatabase,&borderDatabase);
            if (fem->calculatePerturbationalLoss(&boundaryDatabase,&borderDatabase,&materialDatabase)) exit_job_on_error (job_start_time,lockfile,true);
            fem->calculateFieldPoints(&fieldPointDatabase);
            fem->saveTiTv();
            fem->saveParaView();
            chrono::steady_clock::time_point solve_end_time=chrono::steady_clock::now();
            Result *result=fem->updateResults(&resultDatabase,convergenceDatabase,solve_start_time,solve_end_time);

            chrono::duration<double> elapsed = solve_end_time - solve_start_time;
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"         solve elapsed time: %g s\n",elapsed.count());

            // stop if no solutions were found
            if (projData.solution_active_mode_count == 0) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2215: Cannot continue.  Modify setup to increase chances of finding a solution.\n");
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"           - check frequency\n");
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"           - check dimensions\n");
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"           - reduce solution.tolerance\n");
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"           - decrease mesh.order\b");
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"           - apply uniform mesh refinement by setting mesh.uniform_refinement.count\n");
               exit_job_on_error (job_start_time,lockfile,true);
            }

            // print a summary to the console
            printSummary(frequency,&resultDatabase,iteration);

            // show convergence
            if (iteration > 0) convergenceDatabase->show_progress();

            if (iteration < projData.refinement_iteration_min-1) convergenceDatabase->set_not_converged();

            if (refineMesh && convergenceDatabase->is_converged()) iterate=false;

            // refine the mesh using all modes
            if (refineMesh && ! convergenceDatabase->is_converged() && projData.solution_active_mode_count > 0) {
               fem->ZZrefineMeshes(convergenceDatabase);
            }

            // write the fields as the initial guess for the next calculation
            if (projData.solution_use_initial_guess) fem->writeInitialGuess();

            prefix(); PetscPrintf(PETSC_COMM_WORLD,"      Finished\n");

            chrono::steady_clock::time_point iteration_end_time=chrono::steady_clock::now();
            elapsed = iteration_end_time - job_start_time;
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"      cummulative elapsed time: %g s\n",elapsed.count());

            result->set_end_refine_time(iteration_end_time);
         }

         if (ess_tdof_ND) {PetscFree(ess_tdof_ND); ess_tdof_ND=nullptr;}
         if (ess_tdof_H1) {PetscFree(ess_tdof_H1); ess_tdof_H1=nullptr;}

         // update stats with convergence and information
         resultDatabase.update_convergence(frequency,convergenceDatabase->is_converged(),convergenceDatabase->get_last_error());

         // save the results to a results csv file
         if (rank == 0) resultDatabase.save(&projData,baseName);

         // initial guess
         if (projData.solution_use_initial_guess) use_initial_guess=1;

         delete fem;
         ++iteration;

         if (iteration == projData.refinement_iteration_max) iterate=false;
      }

      if (refineMesh && convergenceDatabase->is_converged()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Converged\n");}
      if (refineMesh && ! convergenceDatabase->is_converged()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   NOT CONVERGED\n");}

      // keep track of mesh sizes to check whether a frequency needs to be recalculated due to refinement at another frequency
      meshSize=GetGlobalNE(pmesh);
      frequencyPlanPoint->set_meshSize(meshSize);

      delete convergenceDatabase;
      lastFrequency=frequency;
   }

   // save the results and field point data as test cases
   if (projData.test_create_cases && rank == 0) {
      fieldPointDatabase.normalize();
      fieldPointDatabase.save(&projData,baseName);

      resultDatabase.save_as_test(baseName,projData.project_name);
      fieldPointDatabase.save_as_test(baseName,projData.project_name);
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Job Complete\n");

   MPI_Barrier(PETSC_COMM_WORLD);
   if (rank == 0) {
      if (! projData.debug_tempfiles_keep && std::filesystem::exists(tempdir)) {
        std::filesystem::remove_all(tempdir);
      }
   }

   if (mesh) delete mesh;
   if (pmesh) delete pmesh;
   if (restart_pmesh) delete restart_pmesh;
   free_project (&projData);
   if (alphaList) free(alphaList);
   if (betaList) free(betaList);

   chrono::steady_clock::time_point job_end_time=chrono::steady_clock::now();
   chrono::duration<double> elapsed = job_end_time - job_start_time;
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Elapsed time: %g s\n",elapsed.count());

   show_memory (projData.debug_show_memory, "");

   remove_lock_file (lockfile);

   // notifiy the barrier and send the exit code in case OpenParEM2D was spawned by OpenParEM3D
   if (parent != MPI_COMM_NULL) {
      MPI_Barrier(parent);
      int retval[1]={0};
      MPI_Send(retval,1,MPI_INT,0,0,parent);
   }

   SlepcFinalize();

   return 0;
}

