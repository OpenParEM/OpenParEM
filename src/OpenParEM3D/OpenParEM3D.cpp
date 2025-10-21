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
#include <HYPRE.h>
#include <csignal>
#include "port.hpp"
#include "fem3D.hpp"
#include "results.hpp"
#include "project.h"
#include "jobrelated.hpp"
#include "OpenParEMmaterials.hpp"
#include "frequencyPlan.hpp"
#include "license.hpp"
#include "mesh.hpp"
#include "petscErrorHandler.hpp"

using namespace std;
using namespace mfem;

int version_major=2;
int version_minor=1;
int version_patch=0;

extern "C" void init_project (struct projectData *);
extern "C" int load_project_file(const char*, projectData*, const char*);
extern "C" void print_project (struct projectData *, struct projectData *, const char *);
extern "C" void free_project(projectData*);
extern "C" char* get_project_name (const char *filename);
extern "C" int calculate_S (char *);
extern "C" int is_converged (struct projectData *, double, double);
extern "C" void prefix ();
extern "C" char* get_prefix_text ();
extern "C" void set_prefix_text (char *);

// globals to work with the signal handler
vector<int> pidList;
char *lockfile;
std::filesystem::path currentPath;

void signalHandler (int signum)
{
   if (PETSC_COMM_WORLD == MPI_COMM_NULL) {
      cout.flush();
      cout << "OpenParEM3D Job Aborted" << endl;
      exit(0);
   }

   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   MPI_Comm parent;
   MPI_Comm_get_parent (&parent);

   std::filesystem::current_path(currentPath);

   // kill any OpenParEM2D jobs that may be running
   if (rank == 0) {
      int i=0;
      while (i < pidList.size()) {
         //std::string command="kill -9 ";
         std::string command="kill ";
         command.append(std::to_string(pidList[i]));
         system(command.c_str());
         i++;
      }
      pidList.clear();
   }

   if (parent != MPI_COMM_NULL) {
      int retval=1;
      MPI_Send(&retval,1,MPI_INT,0,100000,parent);
      MPI_Comm_free(&parent);
   }

   remove_lock_file(lockfile);

   cout.flush();
   if (rank == 0) cout << "OpenParEM3D Job Aborted" << endl;

   MPI_Barrier(PETSC_COMM_WORLD);
   PetscFinalize();
   exit(0); // 1
}

double edgeLength (double *a, double *b)
{
   return sqrt((a[0]-b[0])*(a[0]-b[0])+
               (a[1]-b[1])*(a[1]-b[1])+
               (a[2]-b[2])*(a[2]-b[2]));
}

void get_edgeLengthsPerWavelength (ParMesh *pmesh, double frequency, Vector er, Vector mur, double *shortest, double *longest)
{
   double eps0=8.8541878176e-12;

   *shortest=DBL_MAX;
   *longest=0;

   int i=0;
   while (i < pmesh->GetNE()) {

      int attribute=pmesh->GetAttribute(i);
      double velocity=1/sqrt(eps0*er[attribute-1]*4e-7*M_PI*mur[attribute-1]);
      double wavelength=velocity/frequency;

      Array<int> vertices;
      pmesh->GetElementVertices(i,vertices);

      int j=0;
      while (j < vertices.Size()) {

         double *coordsj=pmesh->GetVertex(vertices[j]);

         int k=0;
         while (k < vertices.Size()) {
            if (k != j) {
               double *coordsk=pmesh->GetVertex(vertices[k]);
               double length=edgeLength(coordsj,coordsk)/wavelength;
               if (length > *longest) *longest=length;
               if (length < *shortest) *shortest=length;
            }
            k++;
         }
         j++;
      }
      i++;
   }
}

void help () {
   PetscPrintf(PETSC_COMM_WORLD,"usage: OpenParEM3D [-h] filename\n");
   PetscPrintf(PETSC_COMM_WORLD,"       -h          : Print this help text\n");
   PetscPrintf(PETSC_COMM_WORLD,"       filename    : Filename of an OpenParEM setup file.\n");
   PetscPrintf(PETSC_COMM_WORLD,"\nOpenParEM3D is a full-wave 3D electromagnetic solver.\n");
   PetscPrintf(PETSC_COMM_WORLD,"Version %d.%d.%d\n",version_major,version_minor,version_patch);
}

bool print_mesh_quality_message (ParMesh *pmesh, struct projectData *projData)
{
   double h_min,h_max,kappa_min,kappa_max;
   pmesh->GetCharacteristics(h_min, h_max, kappa_min, kappa_max);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"      mesh worst element aspect ratio: %g",kappa_max);
   if (kappa_max < 5) {PetscPrintf(PETSC_COMM_WORLD," < target: 5\n");}
   else {PetscPrintf(PETSC_COMM_WORLD," > target: 5\n");}

   if (kappa_max > projData->mesh_quality_limit) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"         ERROR3005: aspect ratio > limit: %g\n",projData->mesh_quality_limit);
      return true;
   }

   return false;
}


void isOpenParEM2Dreachable ()
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank == 0) {
      if (system("which OpenParEM2D > /dev/null")) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3008: OpenParEM2D not found.\n");
         PetscFinalize();
         exit(1);
      }
   }
   MPI_Barrier(PETSC_COMM_WORLD);
}

void load_project_file (const char *projFile, struct projectData *defaultData, struct projectData *projData,
                        char *lockfile, chrono::steady_clock::time_point job_start_time)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   loading project file \"%s\"\n",projFile);

   init_project (defaultData);
   init_project (projData);

   if (load_project_file (projFile,projData,"   ")) {
      MPI_Bcast (&(projData->debug_show_project),1,MPI_INT,0,PETSC_COMM_WORLD);
      if (projData->debug_show_project) {print_project (projData,defaultData,"      ");}
      exit_job_on_error (job_start_time,lockfile,true);
   }
   if (projData->debug_show_project) {print_project (projData,defaultData,"      ");}
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

bool saveSerialMesh (struct projectData *projData, MeshMaterialList *meshMaterials, ParMesh *pmesh)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   stringstream ssMesh;
   ssMesh << "refined_" << projData->mesh_file;

   Mesh mesh=pmesh->GetSerialMesh(0);
   if (rank == 0) {
      mesh.Save(ssMesh.str());
   }

   stringstream ssRegions;
   ssRegions << "materials_for_refined_" << projData->mesh_file;
   bool retval=meshMaterials->saveRegionsFile (ssRegions.str().c_str());
   if (retval) return true;

   return false;
}

int main(int argc, char *argv[])
{
   double eps0=8.8541878176e-12;
   const char *projFile;
   char *prefix_text;
   struct projectData projData,defaultData;
   BoundaryDatabase boundaryDatabase;
   FrequencyPlan frequencyPlan;
   MeshMaterialList meshMaterials;
   MaterialDatabase materialDatabase;
   vector<ParSubMesh> parSubMeshesPort;      // for ports
   ResultDatabase resultDatabase;
   PatternDatabase patternDatabase;
   GammaDatabase gammaDatabase;
   vector<DifferentialPair *> aggregateList;
   bool gracefulExit=false;

   signal(SIGINT,signalHandler);
   signal(SIGTERM,signalHandler);

   currentPath=std::filesystem::current_path();

   // Initialize Petsc and MPI
   PetscInitializeNoArguments();
   PetscMPIInt size,rank;
   MPI_Comm_size(PETSC_COMM_WORLD, &size);
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   MPI_Barrier(PETSC_COMM_WORLD);

   // send pid to the parent, if it exists
   MPI_Comm parent;
   MPI_Comm_get_parent (&parent);
   if (parent != MPI_COMM_NULL) {
      int pid=getpid();
      MPI_Send(&pid,1,MPI_INT,0,10000,parent);
   }

   // look for a stop signal
   MPI_Request request;
   int signal;
   if (parent != MPI_COMM_NULL && rank == 0) { 
      MPI_Irecv(&signal,1,MPI_INT,0,300000,parent,&request);
   }

   prefix_text=(char *)malloc(256*sizeof(char));
   snprintf(prefix_text,256,"%s","");
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
   if (printHelp) {help(); PetscFinalize(); exit(retVal);}
   
   print_copyright_notice ("OpenParEM3D",version_major,version_minor,version_patch);
   char *baseName=get_project_name(projFile);
   lockfile=create_lock_file(baseName);
   isOpenParEM2Dreachable();

   appCtx.job_start_time=job_start_time;
   appCtx.lockfile=lockfile;

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Setting up ...\n");

   // project
   load_project_file(projFile,&defaultData,&projData,lockfile,job_start_time);
   projData.version_major=version_major;
   projData.version_minor=version_minor;
   projData.version_patch=version_patch;
   if (projData.output_show_license) {print_license(); exit_job_on_error (job_start_time,lockfile,true);}
   show_memory (projData.debug_show_memory, "   ");

   // materials
   if (materialDatabase.load_materials(projData.materials_global_path,projData.materials_global_name,
                                       projData.materials_local_path,projData.materials_local_name,
                                       projData.materials_check_limits)) exit_job_on_error (job_start_time,lockfile,true);
   if (projData.debug_show_materials) {materialDatabase.print("   ");}

   // mesh
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Loading mesh and assigning materials ...\n");
   if (!projData.materials_check_limits) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Skipping limit checks on material values\n");}
   if (meshMaterials.load(projData.mesh_file,3)) exit_job_on_error (job_start_time,lockfile,true);
   //meshMaterials.print();
   Mesh mesh(projData.mesh_file, 1, 1);
   //   mesh.ScaleElements(0.001);    // hard coded for now to convert from mm to m.  - ToDo - Generalize, but ScaleElements may be broken.
   int dim=mesh.Dimension();
   if (! (dim == 3)) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3011: Mesh must be 3-dimensional.\n"); exit_job_on_error (job_start_time,lockfile,true);}
   reset_attributes(&mesh,nullptr,&meshMaterials);

   // boundaries

   if (boundaryDatabase.load(projData.port_definition_file,projData.solution_check_closed_loop)) exit_job_on_error (job_start_time,lockfile,true);
   if (boundaryDatabase.checkSportNumbering()) exit_job_on_error (job_start_time,lockfile,true);
   boundaryDatabase.assignAttributes(&mesh);
   if (boundaryDatabase.snapToMeshBoundary(&mesh)) exit_job_on_error (job_start_time,lockfile,true);
   if (boundaryDatabase.check_overlaps()) exit_job_on_error (job_start_time,lockfile,true);
   if (boundaryDatabase.alignRadiationNormals()) exit_job_on_error (job_start_time,lockfile,true);
   boundaryDatabase.set2DModeNumbers();
   boundaryDatabase.aggregateDifferentialPairList(&aggregateList);

   boundaryDatabase.fillIntegrationPoints();

   if (boundaryDatabase.hasRadiationBoundary() && projData.inputAntennaPatternsCount == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"INFO: A radiation boundary is specified, but no antenna plot patterns are defined.\n");
   }

   if (!boundaryDatabase.hasRadiationBoundary() && projData.inputAntennaPatternsCount > 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3012: A radiation boundary is not specified, but an antenna plot pattern is defined.\n");
      exit_job_on_error (job_start_time,lockfile,true);
   }

   // clean up from any prior run
   delete_stale_files(baseName,boundaryDatabase.get_SportCount());

   // scale checks for basic error detection
   ParMesh *pmesh=nullptr;
   if (check_field_points (projFile,&mesh,pmesh,projData.mesh_order,3,
                           projData.field_points_count,projData.field_points_x,projData.field_points_y,projData.field_points_z)) {
      exit_job_on_error (job_start_time,lockfile,true);
   }
   if (boundaryDatabase.check_scale(&mesh,projData.mesh_order)) exit_job_on_error (job_start_time,lockfile,true);

   // boundary and mesh linkage
   if (boundaryDatabase.createDefaultBoundary(&projData,&mesh,&materialDatabase,&boundaryDatabase)) exit_job_on_error (job_start_time,lockfile,true);
   if (boundaryDatabase.markMeshBoundaries (&mesh)) exit_job_on_error (job_start_time,lockfile,true);
   if (projData.debug_show_port_definitions) boundaryDatabase.print();

   // mesh and mesh materials consistency check
   if (mesh.attributes.Max() != meshMaterials.size()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3068: Mesh file \"%s\" does not include the correct number of regions for material definitions.\n",projData.mesh_file);
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"       The $PhysicalNames block should have %d entries, but only %d were found\n.",
                                           mesh.attributes.Max()+1,meshMaterials.size()+1);
      exit_job_on_error (job_start_time,lockfile,true);
   }

   // vectors to hold material properties per distinct material region
   Vector Negko2ReRelativePermittivity(mesh.attributes.Max());
   Vector Negko2ImRelativePermittivity(mesh.attributes.Max());
   Vector InvMur(mesh.attributes.Max());
   Vector InvOmegaMu(mesh.attributes.Max());
   Vector mur(mesh.attributes.Max());
   Vector er(mesh.attributes.Max());
   Vector ReInvOmegaEr(mesh.attributes.Max());
   Vector ImInvOmegaEr(mesh.attributes.Max());

   show_memory (projData.debug_show_memory,"");

   // set up the frequency plan
   if (frequencyPlan.assemble(projData.refinement_frequency,projData.inputFrequencyPlansCount,projData.inputFrequencyPlans))
       exit_job_on_error (job_start_time,lockfile,true);
   if (projData.debug_show_frequency_plan) frequencyPlan.print();
   double lastFrequency=0;

   // set up pmesh when not using adaptive mesh refinement
   if (! frequencyPlan.is_refining()) pmesh=new ParMesh(PETSC_COMM_WORLD, mesh);

   // loop over frequencies

   FrequencyPlanPoint *frequencyPlanPoint;
   double frequency=-1;
   bool refineMesh=false;
   bool restartMesh=false;
   bool saveFieldsHeader=true;
   int meshSize=0;
   while ((frequencyPlanPoint=frequencyPlan.get_frequency(projData.refinement_frequency,&frequency,&refineMesh,&restartMesh,&meshSize))) {

      prefix(); PetscPrintf(PETSC_COMM_WORLD,"*******************************************************************************************************************************************************\n");
      prefix(); PetscPrintf(PETSC_COMM_WORLD," Frequency: %g\n",frequency);
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"*******************************************************************************************************************************************************\n");

      double ko=2*M_PI*frequency*sqrt(4.0e-7*M_PI*eps0);

      if (lastFrequency == 0) lastFrequency=frequency;

      // set up pmesh when using adaptive mesh refinement
      if (refineMesh && restartMesh) {
         if (pmesh != NULL) delete pmesh;
         pmesh=new ParMesh(PETSC_COMM_WORLD,mesh);
         boundaryDatabase.resetElementNumbers();
      }

      if (refineMesh) {
         if (restartMesh) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Refining mesh starting with the initial mesh.\n");}
         else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Refining mesh starting with the last mesh.\n");}
      }

      if (! pmesh) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: pmesh is not defined.\n");
         exit_job_on_error (job_start_time,lockfile,true);
      }

      // create frequency-dependent material constants for integration across the finite-element space

      int j=0;
      while (j < pmesh->attributes.Max()) {
         Material *useMaterial=materialDatabase.get(meshMaterials.get_name(meshMaterials.get_index(j)));

         if (useMaterial != NULL) {

            // permittivity
            complex<double> e=useMaterial->get_eps(projData.solution_temperature,frequency,materialDatabase.get_tol(),materialDatabase.get_indent());
            if (e == complex<double>(-DBL_MAX,0)) exit_job_on_error (job_start_time,lockfile,true);

            Negko2ReRelativePermittivity[j]=-ko*ko*real(e)/eps0;
            Negko2ImRelativePermittivity[j]=-ko*ko*imag(e)/eps0;

            er[j]=real(e)/eps0;

            ReInvOmegaEr[j]=real(1/(2*M_PI*frequency*e));
            ImInvOmegaEr[j]=imag(1/(2*M_PI*frequency*e));

            // permeability
            double mu=useMaterial->get_mu(projData.solution_temperature,frequency,materialDatabase.get_tol(),materialDatabase.get_indent());
            if (mu == -DBL_MAX) exit_job_on_error (job_start_time,lockfile,true);

            InvMur[j]=1.0/(mu/(4e-7*M_PI));
            InvOmegaMu[j]=1.0/(2*M_PI*frequency*mu);    // 1/(w*permeability)

            mur[j]=mu/(4e-7*M_PI);

         } else {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3013: Material \"%s\" for region %d is not present in the material database.\n",
                                         meshMaterials.get_name(j).c_str(),j+1);
            exit_job_on_error (job_start_time,lockfile,true);
         }

         j++;
      }

      PWConstCoefficient neg_ko2_Re_er(Negko2ReRelativePermittivity);
      PWConstCoefficient neg_ko2_Im_er(Negko2ImRelativePermittivity);
      PWConstCoefficient Inv_mur(InvMur);
      PWConstCoefficient Inv_w_mu(InvOmegaMu);
      PWConstCoefficient ReInv_w_er(ReInvOmegaEr);
      PWConstCoefficient ImInv_w_er(ImInvOmegaEr);

      // solve 

      int iteration=resultDatabase.get_lastIteration(frequency);
      bool iterate=true;
      while (iterate) {

         iterate=refineMesh;
 
         if (iterate) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Iteration %d ...\n",iteration+1);}
         else         {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Using existing mesh ...\n");}

         chrono::steady_clock::time_point fem_setup_start_time=chrono::steady_clock::now();

         if (print_mesh_quality_message (pmesh,&projData)) exit_job_on_error (job_start_time,lockfile,true);
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"      mesh size: %d\n",getGlobalNE(pmesh));

         double shortestPerWavelength=-1;
         double longestPerWavelength=-1;
         get_edgeLengthsPerWavelength(pmesh,frequency,er,mur,&shortestPerWavelength,&longestPerWavelength);
         double global_shortestPerWavelength,global_longestPerWavelength;
         MPI_Allreduce (&shortestPerWavelength,&global_shortestPerWavelength,1,MPI_DOUBLE,MPI_MIN,PETSC_COMM_WORLD);
         MPI_Allreduce (&longestPerWavelength,&global_longestPerWavelength,1,MPI_DOUBLE,MPI_MAX,PETSC_COMM_WORLD);
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"      mesh edge length/wavelength in element: shortest=%g longest=%g\n",
                                                global_shortestPerWavelength,global_longestPerWavelength);

         // solve the 2D ports
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"      solving 2D ports ...\n");
         if (boundaryDatabase.solve2Dports(pmesh,&parSubMeshesPort,&projData,frequency,&meshMaterials,&gammaDatabase,&pidList)) {
            exit_job_on_error (job_start_time,lockfile,true);
         }

         // save the propagation constant to use as an initial guess in OpenParEM2D
         boundaryDatabase.populateGamma(frequency,&gammaDatabase);

         prefix(); PetscPrintf(PETSC_COMM_WORLD,"      building finite element spaces ...\n");
         fem3D *fem=new fem3D();
         fem->set_data(&pmesh,&projData,frequency);
         fem->build_fe_spaces();
         fem->build_portEssTdofLists(&boundaryDatabase);
         boundaryDatabase.buildGrids(fem);
         boundaryDatabase.save2DModalParaView(&parSubMeshesPort,&projData,frequency,false);
         boundaryDatabase.save3DParaView(pmesh,&projData,frequency,false);
         boundaryDatabase.createDrivingSets();
         boundaryDatabase.calculateLineIntegrals(pmesh,fem);
         boundaryDatabase.alignDirections(pmesh,fem);

         chrono::steady_clock::time_point fem_setup_end_time=chrono::steady_clock::now();

         Result *result=new Result();
         result->set("S",frequency,shortestPerWavelength,longestPerWavelength,iteration,pmesh);
         result->set_fem_setup_time(elapsed_time(fem_setup_start_time,fem_setup_end_time));
         resultDatabase.push(result);

         // loop through the driving sets

         int drivingSet=1;
         int SportCount=boundaryDatabase.get_SportCount();
         resultDatabase.set_SportCount(SportCount);
         while (drivingSet <= SportCount) {

            chrono::steady_clock::time_point solve_start_time=chrono::steady_clock::now();

            stringstream setname;
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"      Driving %s %d ...\n",boundaryDatabase.get_drivingSetName().c_str(),drivingSet);

            prefix(); PetscPrintf(PETSC_COMM_WORLD,"         solving E field in 3D volume ...\n");
            if (fem->solve(&boundaryDatabase,&materialDatabase,projData.solution_temperature,
                &neg_ko2_Re_er,&neg_ko2_Im_er,&Inv_mur,&Inv_w_mu,
                drivingSet,true,projData.solution_check_homogeneous,"   ")) {
               exit_job_on_error (job_start_time,lockfile,true);
            }
            result->set_matrixSize(fem->get_matrixSize());
            result->set_sparseWidth(fem->get_sparseWidth());

            boundaryDatabase.transfer_3Dsolution_3Dgrids_to_2Dgrids(fem);
            boundaryDatabase.save2DSolutionParaView(&parSubMeshesPort,&projData,frequency,drivingSet,false);

            prefix(); PetscPrintf(PETSC_COMM_WORLD,"         post-processing ...\n");
            boundaryDatabase.calculateSplits();
            complex<double> acceptedPower=0;
            if(boundaryDatabase.calculateAcceptedPower(drivingSet,&acceptedPower)) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"            ASSERT: failure in calculating accepted power\n");
               exit_job_on_error (job_start_time,lockfile,true);
            }

            chrono::steady_clock::time_point solve_end_time=chrono::steady_clock::now();
            result->set_solve_time(elapsed_time(solve_start_time,solve_end_time));

            if (iterate) {
               chrono::steady_clock::time_point mesh_error_start_time=chrono::steady_clock::now();
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"         calculating mesh errors ...\n");
               double maxMeshError;
               if (fem->calculateMeshErrors(&projData,&boundaryDatabase,&Inv_mur,&maxMeshError)) {
                  exit_job_on_error (job_start_time,lockfile,true);
               }
               result->set_maxAbsoluteError(maxMeshError);
               chrono::steady_clock::time_point mesh_error_end_time=chrono::steady_clock::now();
               result->set_mesh_error_time(elapsed_time(mesh_error_start_time,mesh_error_end_time));
            }

            if (saveFieldsHeader) {fem->saveFieldValuesHeader(&projData); saveFieldsHeader=false;}
            fem->saveFieldValues(&projData,pmesh,iteration,drivingSet);

            chrono::steady_clock::time_point radiation_start_time=chrono::steady_clock::now();
            if (projData.inputAntennaPatternsCount > 0) {
               result->set_hasRadiation(true);
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"         radiation boundary currents ...\n");
               fem->calculateRadiationCurrents(&boundaryDatabase);
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"         radiation patterns ...\n");
               fem->calculateRadiationPatterns(drivingSet,acceptedPower,iteration,&boundaryDatabase,&patternDatabase);
            }
            chrono::steady_clock::time_point radiation_end_time=chrono::steady_clock::now();
            result->set_radiation_time(elapsed_time(radiation_start_time,radiation_end_time));

            drivingSet++;
         }

         // calculate S-parameters
         if(boundaryDatabase.calculateS(result)) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"            ERROR3189: failed to calculate S-parameters.\n");
            exit_job_on_error (job_start_time,lockfile,true);
         }

         // force the matrix to be reciprocal
         if (!projData.debug_skip_forced_reciprocity) {
            if (result->forceReciprocal()) cout << "ASSERT: forced reciprocity failed." << endl;
         }

         // S-parameter renormalization
         if (projData.reference_impedance > 0) {
            PetscPrintf (PETSC_COMM_WORLD,"         renormalizing S-parameters ...\n");
            if (boundaryDatabase.has_Ti() && boundaryDatabase.has_Tv()) {
               // If single-ended, then this operation does a renormalization identical to the method in ResultDatabase::renormalize called below.
               // If modal, then this operation converts modal S-parameters to single-ended S-parameters.
               fem->build_Mc_Ms(projData.reference_impedance,&boundaryDatabase,&aggregateList,0);
               if (result->SparameterConversion(&boundaryDatabase,fem->get_Mc(),fem->get_Ms(),fem->get_SportZoList()))
                  exit_job_on_error (job_start_time,lockfile,true);
            } else {
               // Performs renormalization without doing any recombinations of ports.
               if (result->renormalize(projData.reference_impedance)) cout << "ASSERT: renomalization failed." << endl;
            }

            // mixed mode conversion
            if (aggregateList.size() > 0 && !projData.debug_skip_mixed_conversion) {
               PetscPrintf (PETSC_COMM_WORLD,"         mixed-mode conversion on S-parameters ...\n");
               fem->build_Mc_Ms(projData.reference_impedance,&boundaryDatabase,&aggregateList,1);
               if (result->SparameterConversion(&boundaryDatabase,fem->get_Mc(),fem->get_Ms(),fem->get_SportZoList()))
                  exit_job_on_error (job_start_time,lockfile,true);
            }
         }

         if (iterate) {
            chrono::steady_clock::time_point refine_start_time=chrono::steady_clock::now();

            // mark as having iterations
            result->set_isRefined();

            // save the relative error
            double maxRelativeError=resultDatabase.calculate_maxRelativeError(&projData,frequency,iteration+1);
            result->set_maxRelativeError(maxRelativeError);

            // save the absolute error
            double maxAbsoluteError=resultDatabase.calculate_maxAbsoluteError(&projData,frequency,iteration+1);
            result->set_maxAbsoluteError(maxAbsoluteError);

            // check if converged - preliminary (can be over-ridden)
            int isConverged=is_converged(&projData,maxRelativeError,maxAbsoluteError);

            // check for minimum iterations
            if (iteration < projData.refinement_iteration_min-1) {isConverged=0;}

            // save the convergence status
            result->set_isConverged(isConverged);

            // check for convergence
            if (resultDatabase.isSequentialConverged(&projData,frequency)) iterate=false;

            // check for maximum iterations
            if (iteration == projData.refinement_iteration_max-1) iterate=false;

            // refine
            if (iterate) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"         refining mesh ...\n");
               fem->refineMesh();

               if (projData.mesh_save_refined) {
                  prefix(); PetscPrintf(PETSC_COMM_WORLD,"         saving refined mesh ...\n");
                  if (saveSerialMesh(&projData,&meshMaterials,pmesh)) exit_job_on_error (job_start_time,lockfile,true);
               }
            }

            chrono::steady_clock::time_point refine_end_time=chrono::steady_clock::now();
            resultDatabase.set_refine_time(elapsed_time(refine_start_time,refine_end_time),frequency,iteration+1);
         }

         resultDatabase.save(&projData);
         resultDatabase.saveFormatted(&projData);
         resultDatabase.saveCSV (&projData,&boundaryDatabase,&aggregateList,true);
         resultDatabase.saveTouchstone (&projData,&boundaryDatabase,&aggregateList);

         // calculate antenna metrics
         patternDatabase.calculateIsotropicGain(frequency,iteration);
         patternDatabase.calculateDirectivity(frequency,iteration);
         patternDatabase.calculateRadiationEfficiency(frequency,iteration);

         patternDatabase.saveCSV(&projData,resultDatabase.get_unique_frequencies(),SportCount,true);
         patternDatabase.saveParaView(&projData,frequency,iteration);
         patternDatabase.save(&projData);

         // clean up
         delete fem; fem=nullptr;
         boundaryDatabase.reset();

         // for benchmarking
         //chrono::steady_clock::time_point iteration_end_time=chrono::steady_clock::now();
         //if (rank == 0) cout << "CUMMULATIVETIME," << iteration+1 << "," << elapsed_time(job_start_time,iteration_end_time) << endl;

         ++iteration;

         // check for signaled stop
         if (parent != MPI_COMM_NULL) {
            if (rank == 0) {
               // look for a non-blocking message to stop
               int test;
               MPI_Test(&request,&test,MPI_STATUS_IGNORE); 

               // send a blocking message to the other ranks
               int i=1;
               while (i < size) {
                  MPI_Send(&test,1,MPI_INT,i,300001,PETSC_COMM_WORLD);
                  i++;
               }
               if (test) gracefulExit=true;
            } else {
               int test;
               MPI_Recv(&test,1,MPI_INT,0,300001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
               if (test) gracefulExit=true;  
            }
         }
         if (gracefulExit) break;
      }

      // keep track of mesh sizes to check whether a frequency needs to be recalculated due to refinement at another frequency
      meshSize=getGlobalNE(pmesh);
      frequencyPlanPoint->set_meshSize(meshSize);

      lastFrequency=frequency;
      if (gracefulExit) break;
   }

   // save the results as test cases
   if (projData.test_create_cases) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"Generating test cases ...\n");
      int casenumber=0;

      stringstream ssTests;
      ssTests << projData.project_name << "_prototype_test_cases.csv";

      resultDatabase.save_as_test(ssTests.str(),&projData,&casenumber);
      patternDatabase.save_as_test(ssTests.str(),&projData,resultDatabase.get_SportCount(),&casenumber);
   }

   // print convergence status
   if (resultDatabase.hasRefinement()) {
      if (resultDatabase.isAllConverged()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"Converged\n");}
      else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"NOT CONVERGED\n");}
   }

   if (gracefulExit) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"Job Stopped\n");}
   else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"Job Complete\n");}

   if (rank == 0) {
      stringstream ss;
      ss << "temp_" << projData.project_name;
      if (! projData.debug_tempfiles_keep && std::filesystem::exists(ss.str().c_str())) {
        std::filesystem::remove_all(ss.str().c_str());
      }
   }

   write_attributes (baseName,pmesh);
   delete pmesh;

   chrono::steady_clock::time_point job_end_time=chrono::steady_clock::now();
   resultDatabase.set_solve_time(elapsed_time(job_start_time,job_end_time));
   resultDatabase.save(&projData);

   free_project (&projData);

   show_memory (projData.debug_show_memory, "");

   remove_lock_file(lockfile);

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Elapsed time: %g s\n",resultDatabase.get_solve_time());

   if (parent != MPI_COMM_NULL) {
      int retval=0;
      MPI_Send(&retval,1,MPI_INT,0,100000,parent);
      MPI_Comm_free(&parent);
   }

   MPI_Barrier(PETSC_COMM_WORLD);
   PetscFinalize();

   return 0;
}

