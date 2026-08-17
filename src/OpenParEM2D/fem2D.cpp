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

#include "fem2D.hpp"

bool saveMat (ParBilinearForm &blf, string name, string directory, char *uniquifier, bool transpose) {
   stringstream ss;
   ss << directory << "/" << name << "." << uniquifier;

   HypreParMatrix *mat;
   try {
      mat=blf.ParallelAssemble();
   } catch (const std::exception& e) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2009: Out of memory in assembling finite-element matrix.\n"); 
      return true;
   }
   if (transpose) {
      HypreParMatrix *matT=mat->Transpose();
      delete mat;
      mat=matT;
   }
   mat->Print(ss.str().c_str());
   delete mat;
   return false;
}

bool saveMat (ParMixedBilinearForm &blf, string name, string directory, char *uniquifier, bool transpose) {
   stringstream ss;
   ss << directory << "/" << name << "." << uniquifier;

   HypreParMatrix *mat;
   try {
      mat=blf.ParallelAssemble();
   } catch (const std::exception& e) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2010: Out of memory in assembling finite-element matrix.\n");
      return true;
   }
   if (transpose) {
      HypreParMatrix *matT=mat->Transpose();
      delete mat;
      mat=matT;
   }
   mat->Print(ss.str().c_str());
   delete mat;
   return false;
}

int GetGlobalNE (ParMesh *pmesh)
{
   int localNE=pmesh->GetNE();
   int globalNE=0;

   MPI_Allreduce (&localNE,&globalNE,1,MPI_INT,MPI_SUM,PETSC_COMM_WORLD);

   return globalNE;
}

//--------------------------------------------------------------------------------------------------------------------
// Fields
//--------------------------------------------------------------------------------------------------------------------

Fields::Fields (fem2D *fem)
{
   grid_Et_re=new ParGridFunction(fem->fespace_ND);
   grid_Et_im=new ParGridFunction(fem->fespace_ND);
   grid_Ez_re=new ParGridFunction(fem->fespace_H1);
   grid_Ez_im=new ParGridFunction(fem->fespace_H1);

   grid_Ht_re=new ParGridFunction(fem->fespace_ND);
   grid_Ht_im=new ParGridFunction(fem->fespace_ND);
   grid_Hz_re=new ParGridFunction(fem->fespace_H1);
   grid_Hz_im=new ParGridFunction(fem->fespace_H1);
}

Fields::~Fields()
{
   if (grid_Et_re) {delete grid_Et_re; grid_Et_re=nullptr;}
   if (grid_Et_im) {delete grid_Et_im; grid_Et_im=nullptr;}
   if (grid_Ez_re) {delete grid_Ez_re; grid_Ez_re=nullptr;}
   if (grid_Ez_im) {delete grid_Ez_im; grid_Ez_im=nullptr;}

   if (grid_Ht_re) {delete grid_Ht_re; grid_Ht_re=nullptr;}
   if (grid_Ht_im) {delete grid_Ht_im; grid_Ht_im=nullptr;}
   if (grid_Hz_re) {delete grid_Hz_re; grid_Hz_re=nullptr;}
   if (grid_Hz_im) {delete grid_Hz_im; grid_Hz_im=nullptr;}

   if (grid_Pz_re) {delete grid_Pz_re; grid_Pz_re=nullptr;}
   if (grid_Pz_im) {delete grid_Pz_im; grid_Pz_im=nullptr;}
}

void Fields::flipSign ()
{

   (*grid_Et_re)*=-1;
   (*grid_Et_im)*=-1;
   (*grid_Ez_re)*=-1;
   (*grid_Ez_im)*=-1;

   (*grid_Ht_re)*=-1;
   (*grid_Ht_im)*=-1;
   (*grid_Hz_re)*=-1;
   (*grid_Hz_im)*=-1;
}

// field = false for Efield
// field = true  for Hfield
// allocates eVecRe and eVecIm, and these need to be freed elsewhere
bool Fields::loadEigenvector (fem2D *fem, long unsigned int mode, bool field, double **eVecRe, double **eVecIm)
{
   char filename[128];
   string line;
   vector<string> tokens;
   size_t current;

   if (field) { // Hfield
      sprintf (filename,"temp_%s/Hfield_mode_%ld.dat",fem->projData->project_name,mode+1);
   } else {     // Efield
      sprintf (filename,"temp_%s/Efield_mode_%ld.dat",fem->projData->project_name,mode+1);
   }

   ifstream eVecFile;
   eVecFile.open(filename,ifstream::in);

   if (eVecFile.is_open()) {
      bool fail=loadData (&eVecFile,eVecRe,eVecIm,&current,filename);
      eVecFile.close();
      if (fail) return true;
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2094: Unable to open file \"%s\" for reading.\n",filename);
      return true;
   }

   // check that the expected number of lines loaded
   if (current != fem->t_size+fem->z_size) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2135: Unexpected line load count of %ld vs. expected %ld in file \"%s\".\n",
                                             current,fem->t_size+fem->z_size,filename);
      return true;
   }

   // scale the eigenvector by the largest Et vector component
   // This is equivalent to scaling the amplitude and phase-shifting the solution.
   // Makes for nicer plots so that the real part dominates the imaginary part.

   if (current > 0) {
      double x1,x2,y1,y2;
      double mag2=0;
      double largest=0;
      int index=0;

      // get the scale
      x2=0;
      y2=0;
      if (field) {
         // must run for E-fields before running for H-fields
         x2=xScale;
         y2=yScale;
      } else {
         // largest Et component
         size_t i=0;
         while (i < fem->t_size) {
            mag2=(*eVecRe)[i]*(*eVecRe)[i]+(*eVecIm)[i]*(*eVecIm)[i];
            if (mag2 > largest) {largest=mag2; index=i;}
            i++;
         }
         x2=(*eVecRe)[index];
         y2=(*eVecIm)[index];

         // save the scale factor for use with H fields
         xScale=x2;
         yScale=y2;
      }

      // scale all
      if (fabs(x2) > 0 || fabs(y2) > 0) {
         size_t i=0;
         while (i < current) {
            x1=(*eVecRe)[i];
            y1=(*eVecIm)[i];

            // (x1+jy1)/(x2+jy2)
            (*eVecRe)[i]=(x1*x2+y1*y2)/(x2*x2+y2*y2);
            (*eVecIm)[i]=(x2*y1-x1*y2)/(x2*x2+y2*y2);

            i++;
         }
      }
   }

   return false;
}

bool Fields::build (fem2D *fem, long unsigned int mode)
{
   bool HFIELD=true;
   bool EFIELD=false;

   HYPRE_BigInt *offset_ND=fem->fespace_ND->GetTrueDofOffsets();
   HYPRE_BigInt *offset_H1=fem->fespace_H1->GetTrueDofOffsets();

   // must load E before H to get the scale factor

   // build the Efield
   if (! loadEigenvector (fem,mode,EFIELD,&EfieldRe,&EfieldIm)) {
      MPI_Barrier(PETSC_COMM_WORLD);

      Vector EtRe=Vector(EfieldRe+offset_ND[0],offset_ND[1]-offset_ND[0]);
      Vector EtIm=Vector(EfieldIm+offset_ND[0],offset_ND[1]-offset_ND[0]);
      Vector EzRe=Vector(EfieldRe+fem->t_size+offset_H1[0],offset_H1[1]-offset_H1[0]);
      Vector EzIm=Vector(EfieldIm+fem->t_size+offset_H1[0],offset_H1[1]-offset_H1[0]);

      grid_Et_re=new ParGridFunction(fem->fespace_ND);
      grid_Et_im=new ParGridFunction(fem->fespace_ND);
      grid_Ez_re=new ParGridFunction(fem->fespace_H1);
      grid_Ez_im=new ParGridFunction(fem->fespace_H1);

      grid_Et_re->Distribute(EtRe);
      grid_Et_im->Distribute(EtIm);
      grid_Ez_re->Distribute(EzRe);
      grid_Ez_im->Distribute(EzIm);

      if (EfieldRe) {free(EfieldRe); EfieldRe=nullptr;}
      if (EfieldIm) {free(EfieldIm); EfieldIm=nullptr;}
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2136: Failed to load Efield.\n");
      return true;
   }

   // build the Hfield
   if (! loadEigenvector (fem,mode,HFIELD,&HfieldRe,&HfieldIm)) {
      MPI_Barrier(PETSC_COMM_WORLD);

      Vector HtRe=Vector(HfieldRe+offset_ND[0],offset_ND[1]-offset_ND[0]);
      Vector HtIm=Vector(HfieldIm+offset_ND[0],offset_ND[1]-offset_ND[0]);
      Vector HzRe=Vector(HfieldRe+fem->t_size+offset_H1[0],offset_H1[1]-offset_H1[0]);
      Vector HzIm=Vector(HfieldIm+fem->t_size+offset_H1[0],offset_H1[1]-offset_H1[0]);

      grid_Ht_re=new ParGridFunction(fem->fespace_ND);
      grid_Ht_im=new ParGridFunction(fem->fespace_ND);
      grid_Hz_re=new ParGridFunction(fem->fespace_H1);
      grid_Hz_im=new ParGridFunction(fem->fespace_H1);

      grid_Ht_re->Distribute(HtRe);
      grid_Ht_im->Distribute(HtIm);
      grid_Hz_re->Distribute(HzRe);
      grid_Hz_im->Distribute(HzIm);

      if (HfieldRe) {free(HfieldRe); HfieldRe=nullptr;}
      if (HfieldIm) {free(HfieldIm); HfieldIm=nullptr;}

   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2137: Failed to load Hfield.\n");
      return true;
   }

   return false;
}

void Fields::updateGrids()
{
   if (grid_Et_re) grid_Et_re->Update();
   if (grid_Et_im) grid_Et_im->Update();
   if (grid_Ez_re) grid_Ez_re->Update();
   if (grid_Ez_im) grid_Ez_im->Update();

   if (grid_Ht_re) grid_Ht_re->Update();
   if (grid_Ht_im) grid_Ht_im->Update();
   if (grid_Hz_re) grid_Hz_re->Update();
   if (grid_Hz_im) grid_Hz_im->Update();

   if (grid_Pz_re) grid_Pz_re->Update();
   if (grid_Pz_im) grid_Pz_im->Update();
}

// save the results for ParaView
void Fields::saveParaView(fem2D *fem, long unsigned int index, bool isModal)
{
   if (!fem->projData->project_save_fields) return;

   stringstream ss;
   if (isModal) ss << fem->projData->project_name << "_frequency_" << fem->frequency << "_mode_" << index+1;
   else ss << fem->projData->project_name << "_frequency_" << fem->frequency << "_line_" << index+1;

   stringstream ssParaView;
   ssParaView << "ParaView_" << fem->projData->project_name;

   ParaViewDataCollection *pd=nullptr;
   pd=new ParaViewDataCollection(ss.str(),fem->pmesh);
   pd->SetOwnData(false);
   pd->SetPrefixPath(ssParaView.str());
   pd->RegisterField("Et_re",grid_Et_re);
   pd->RegisterField("Ez_re",grid_Ez_re);
   pd->RegisterField("Et_im",grid_Et_im);
   pd->RegisterField("Ez_im",grid_Ez_im);
   pd->RegisterField("Ht_re",grid_Ht_re);
   pd->RegisterField("Hz_re",grid_Hz_re);
   pd->RegisterField("Ht_im",grid_Ht_im);
   pd->RegisterField("Hz_im",grid_Hz_im);
   pd->RegisterField("Pz_re",grid_Pz_re);
   pd->RegisterField("Pz_im",grid_Pz_im);
   pd->SetLevelsOfDetail(fem->order);
   pd->SetDataFormat(VTKFormat::ASCII);
   pd->SetHighOrderOutput(true);
   pd->Save();
   delete pd;
}

void saveInitialGuess(HypreParVector *hpv, fem2D *fem, long unsigned int mode, string name)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   stringstream ss;
   ss << "_" << mode+1;

   ofstream out;

   int TXheader[3];
   int RXheader[3];

   if (rank == 0) {

      // rank 0 data

      const HYPRE_BigInt *hpi=hpv->Partitioning();

      TXheader[0]=rank;
      TXheader[1]=(int)hpi[0];
      TXheader[2]=(int)hpi[1];

      HYPRE_BigInt globalSize=hpv->GlobalSize();
      double *data=(double *)malloc(globalSize*sizeof(double));

      int i=0;
      while (i < TXheader[2]-TXheader[1]) {
         data[TXheader[1]+i]=hpv->Elem(i);
         i++;
      }

      // other rank data
      i=1;
      while (i < size) {
         MPI_Recv(RXheader,3,MPI_INT,i,0,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(data+RXheader[1],RXheader[2]-RXheader[1],MPI_DOUBLE,i,1,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         i++;
      }

      // save
      out.open((fem->get_temporaryDirectory()+"/initial_guess_"+name+ss.str()+".dat").c_str(),ofstream::out);
      if (out.is_open()) {
         int i=0;
         while (i < globalSize) {
            out << setprecision(15) << data[i] << endl;
            i++;
         }
         out.close();
      }

      if (data) free(data);
   } else {
      const HYPRE_BigInt *hpi=hpv->Partitioning();

      TXheader[0]=rank;
      TXheader[1]=(int)hpi[0];
      TXheader[2]=(int)hpi[1];

      MPI_Send(TXheader,3,MPI_INT,0,0,PETSC_COMM_WORLD);
      MPI_Send(hpv->GetData(),TXheader[2]-TXheader[1],MPI_DOUBLE,0,1,PETSC_COMM_WORLD);
   }
}

// write as initial guess for the next iteration
void Fields::writeInitialGuess (fem2D *fem, long unsigned int mode)
{
   HypreParVector *hpv_Et_re=grid_Et_re->GetTrueDofs();
   saveInitialGuess(hpv_Et_re,fem,mode,"Et_re");
   delete hpv_Et_re;

   HypreParVector *hpv_Et_im=grid_Et_im->GetTrueDofs();
   saveInitialGuess(hpv_Et_im,fem,mode,"Et_im");
   delete hpv_Et_im;

   HypreParVector *hpv_Ez_re=grid_Ez_re->GetTrueDofs();
   saveInitialGuess(hpv_Ez_re,fem,mode,"Ez_re");
   delete hpv_Ez_re;

   HypreParVector *hpv_Ez_im=grid_Ez_im->GetTrueDofs();
   saveInitialGuess(hpv_Ez_im,fem,mode,"Ez_im");
   delete hpv_Ez_im;
}

// calculate Pz from the E and H fields using the Poynting vector, so P=1/2 surface_integral Et x Ht*
complex<double> Fields::calculatePz(fem2D *fem)
{
   //**************************************************************************************************
   // real part
   //**************************************************************************************************

   // Et_re x Ht_re
   ParDiscreteLinearOperator Et_rexHt_re(fem->fespace_ND,fem->fespace_L2);
   VectorGridFunctionCoefficient Et_re_coef(grid_Et_re);
   Et_rexHt_re.AddDomainInterpolator(new ScalarCrossProductInterpolator(Et_re_coef));
   Et_rexHt_re.Assemble();
   Et_rexHt_re.Finalize();

   ParGridFunction grid_Pz1_re=ParGridFunction(fem->fespace_L2);
   Et_rexHt_re.Mult(*grid_Ht_re,grid_Pz1_re);

   GridFunctionCoefficient Pz1_re_coef(&grid_Pz1_re);

   // Et_im x Ht_im
   ParDiscreteLinearOperator Et_imxHt_im(fem->fespace_ND,fem->fespace_L2);
   VectorGridFunctionCoefficient Et_im_coef(grid_Et_im);
   Et_imxHt_im.AddDomainInterpolator(new ScalarCrossProductInterpolator(Et_im_coef));
   Et_imxHt_im.Assemble();
   Et_imxHt_im.Finalize();

   ParGridFunction grid_Pz2_re=ParGridFunction(fem->fespace_L2);
   Et_imxHt_im.Mult(*grid_Ht_im,grid_Pz2_re);

   GridFunctionCoefficient Pz2_re_coef(&grid_Pz2_re);

   // sum the two components

   SumCoefficient Pz_re_coef(Pz1_re_coef,Pz2_re_coef,0.5,0.5);

   // put it on the grid function

   if (grid_Pz_re) delete grid_Pz_re;
   grid_Pz_re=new ParGridFunction(fem->fespace_L2);
   grid_Pz_re->ProjectCoefficient(Pz_re_coef);

   //**************************************************************************************************
   // imaginary part
   //**************************************************************************************************

   // Et_im x Ht_re
   ParDiscreteLinearOperator Et_imxHt_re(fem->fespace_ND,fem->fespace_L2);
   Et_imxHt_re.AddDomainInterpolator(new ScalarCrossProductInterpolator(Et_im_coef));
   Et_imxHt_re.Assemble();
   Et_imxHt_re.Finalize();

   ParGridFunction grid_Pz1_im=ParGridFunction(fem->fespace_L2);
   Et_imxHt_re.Mult(*grid_Ht_re,grid_Pz1_im);

   GridFunctionCoefficient Pz1_im_coef(&grid_Pz1_im);

   // Et_re x Ht_im
   ParDiscreteLinearOperator Et_rexHt_im(fem->fespace_ND,fem->fespace_L2);
   Et_rexHt_im.AddDomainInterpolator(new ScalarCrossProductInterpolator(Et_re_coef));
   Et_rexHt_im.Assemble();
   Et_rexHt_im.Finalize();

   ParGridFunction grid_Pz2_im=ParGridFunction(fem->fespace_L2);
   Et_rexHt_im.Mult(*grid_Ht_im,grid_Pz2_im);

   GridFunctionCoefficient Pz2_im_coef(&grid_Pz2_im);

   // sum the two components

   SumCoefficient Pz_im_coef(Pz1_im_coef,Pz2_im_coef,0.5,-0.5);

   // put it on the grid function
   if (grid_Pz_im) delete grid_Pz_im;
   grid_Pz_im=new ParGridFunction(fem->fespace_L2);
   grid_Pz_im->ProjectCoefficient(Pz_im_coef);

   //**************************************************************************************************
   // integrate over the cross section
   //**************************************************************************************************

   ConstantCoefficient one(1.0);
   ParLinearForm Pz_lf(fem->fespace_L2);
   Pz_lf.AddDomainIntegrator(new DomainLFIntegrator(one));
   Pz_lf.Assemble();

   return complex<double>(Pz_lf(*grid_Pz_re),Pz_lf(*grid_Pz_im));
}

// get field values given a list of (x,y) coordinates
// set fieldValues == nullptr if calculating for Ez or Hz
bool getFieldValues (int numPoints, ParMesh *pmesh, DenseMatrix *points, ParGridFunction *grid_t_re, ParGridFunction *grid_t_im,
                     vector<complex<double>> *fieldValuesX, vector<complex<double>> *fieldValuesY)
{
   if ((int)fieldValuesX->size() != numPoints || (fieldValuesY && (int)fieldValuesY->size() != numPoints)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: fieldValues not properly allocated.\n");
      return true;
   }

   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // set up a datatype for data transfer
   // use an int for an array location then two doubles for a complex value
   // copied from Hsolve.c - ToDo - Make this reusable to avoid copying code.

   MPI_Datatype mpi_complex_int_type;
   int lengths[3]={1,1,1};

   MPI_Aint displacements[3];
   struct mpi_complex_int dummy;
   MPI_Aint base_address;
   MPI_Get_address(&dummy,&base_address);
   MPI_Get_address(&dummy.real,&displacements[0]);
   MPI_Get_address(&dummy.imag,&displacements[1]);
   MPI_Get_address(&dummy.location,&displacements[2]);
   displacements[0]=MPI_Aint_diff(displacements[0],base_address);
   displacements[1]=MPI_Aint_diff(displacements[1],base_address);
   displacements[2]=MPI_Aint_diff(displacements[2],base_address);

   MPI_Datatype types[3]={MPI_DOUBLE,MPI_DOUBLE,MPI_INT};
   MPI_Type_create_struct(3,lengths,displacements,types,&mpi_complex_int_type);
   MPI_Type_commit(&mpi_complex_int_type);

   // get element indices for each point
   Array<int> elementIndices(numPoints);
   Array<IntegrationPoint> integrationPoints(numPoints);
   findPoints(pmesh, *points, elementIndices, integrationPoints, 2);

   // space to hold transfer lengths and displacements
   int *counts_recv=(int *)malloc(size*sizeof(int));
   int *displacements_recv=(int *)malloc(size*sizeof(int));

   // space for data to gather
   struct mpi_complex_int *sendDataX=nullptr;
   struct mpi_complex_int *sendDataY=nullptr;
   struct mpi_complex_int *recvDataX=nullptr;
   struct mpi_complex_int *recvDataY=nullptr;

   sendDataX=(struct mpi_complex_int *)malloc(numPoints*sizeof(struct mpi_complex_int));
   if (fieldValuesY) sendDataY=(struct mpi_complex_int *)malloc(numPoints*sizeof(struct mpi_complex_int));
   recvDataX=(struct mpi_complex_int *)malloc(size*numPoints*sizeof(struct mpi_complex_int));                    // allocate more because a point may land in two triangles (edge)
   if (fieldValuesY) recvDataY=(struct mpi_complex_int *)malloc(size*numPoints*sizeof(struct mpi_complex_int));  // allocate more because a point may land in two triangles (edge)

   // field data
   // Will only be valid for this rank's section of the mesh
   // Store it to support gathering the data across ranks
   int count_from=0;
   int i=0;
   while (i < numPoints) {
      if (elementIndices[i] >= 0) {
         Vector tRe,tIm;
         grid_t_re->GetVectorValue(elementIndices[i],integrationPoints[i],tRe);
         grid_t_im->GetVectorValue(elementIndices[i],integrationPoints[i],tIm);

         sendDataX[count_from].location=i;
         sendDataX[count_from].real=tRe.Elem(0);
         sendDataX[count_from].imag=tIm.Elem(0);

         if (fieldValuesY) {
            sendDataY[count_from].location=i;
            sendDataY[count_from].real=tRe.Elem(1);
            sendDataY[count_from].imag=tIm.Elem(1);
         }

         count_from++;
      }
      i++;
   }

   // gather the transfer lengths 
   if (MPI_Gather(&count_from,1,MPI_INT,counts_recv,1,MPI_INT,0,PETSC_COMM_WORLD)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2138: Failed to gather transfer lengths.\n");
      return true;
   }

   // calculate displacements
   if (rank == 0) {
      displacements_recv[0]=0;
      i=1;
      while (i < size) {
         displacements_recv[i]=displacements_recv[i-1]+counts_recv[i-1];
         i++;
      }
   }

   // get the total number of points
   // Could be greater than numPoints due to duplicated points on a triangle boundary.

   int totalPoints=0;
   if (rank == 0) {
      i=0;
      while (i < size) {
         totalPoints+=counts_recv[i];
         i++;
      }
   }

   if (MPI_Bcast(&totalPoints,1,MPI_INT,0,PETSC_COMM_WORLD)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2139: Failed to broadcast totalPoints.\n");
      return true;
   }

   // send all to rank 0
   if (MPI_Gatherv(sendDataX,count_from,mpi_complex_int_type,recvDataX,counts_recv,displacements_recv,mpi_complex_int_type,0,PETSC_COMM_WORLD)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2140: Failed to gather sendDataX.\n");
      return true;
   }

   if (fieldValuesY && MPI_Gatherv(sendDataY,count_from,mpi_complex_int_type,recvDataY,counts_recv,displacements_recv,mpi_complex_int_type,0,PETSC_COMM_WORLD)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2141: Failed to gather sendDataY.\n");
      return true;
   }

   // cleanup
   if (counts_recv) {free(counts_recv); counts_recv=nullptr;}
   if (displacements_recv) {free(displacements_recv); displacements_recv=nullptr;}
   if (sendDataX) {free(sendDataX); sendDataX=nullptr;}
   if (fieldValuesY && sendDataY) {free(sendDataY); sendDataY=nullptr;}

   // broadcast

   if (MPI_Bcast(recvDataX,totalPoints,mpi_complex_int_type,0,PETSC_COMM_WORLD)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2142: Failed to broadcast recvDataX.\n");
      return true;
   }

   if (fieldValuesY && MPI_Bcast(recvDataY,totalPoints,mpi_complex_int_type,0,PETSC_COMM_WORLD)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2143: Failed to broadcast recvDataY.\n");
      return true;
   }

   // initialize data to enable detection of errors
   i=0;
   while (i < numPoints) {
      (*fieldValuesX)[i]=complex<double>(DBL_MAX,DBL_MAX);
      if (fieldValuesY) (*fieldValuesY)[i]=complex<double>(DBL_MAX,DBL_MAX);
      i++;
   }

   // transfer the results 
   i=0;
   while (i < totalPoints) {
      if (recvDataX[i].location >= 0) {
         (*fieldValuesX)[recvDataX[i].location]=complex<double>(recvDataX[i].real,recvDataX[i].imag);
         if (fieldValuesY) (*fieldValuesY)[recvDataY[i].location]=complex<double>(recvDataY[i].real,recvDataY[i].imag);
      }
      i++;
   }

   // cleanup
   if (recvDataX) {free(recvDataX); recvDataX=nullptr;}
   if (fieldValuesY && recvDataY) {free(recvDataY); recvDataY=nullptr;}

   // check for missing points
   i=0;
   while (i < numPoints) {
      if ((*fieldValuesX)[i] == complex<double>(DBL_MAX,DBL_MAX) || (fieldValuesY && (*fieldValuesY)[i] == complex<double>(DBL_MAX,DBL_MAX))) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"WARNING: Integration point (%g,%g) is not detected inside the mesh.\n",
                                                points->Elem(0,i),points->Elem(1,i));
      }
      i++;
   }

   MPI_Type_free(&mpi_complex_int_type);

   return false;
}

// field = false => electric field
// field = true  => magnetic field
complex<double> Fields::calculateLineIntegral (fem2D *fem, Boundary *boundary, BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase, bool field)
{
   long unsigned int max=-1;
   ParGridFunction *grid_t_re, *grid_t_im;
   vector<complex<double>> fieldValuesX,fieldValuesY;
   double xi1,yi1,xi2,yi2,segmentLength;
   complex<double> valueX,valueY;
   bool has_printed_integrator=false;
   bool has_printed_numerical=false;

   fieldValuesX.resize(fem->pointsCount);
   fieldValuesY.resize(fem->pointsCount);

   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   // set the field type
   if (field) {
      grid_t_re=grid_Ht_re;
      grid_t_im=grid_Ht_im;
   } else {
      grid_t_re=grid_Et_re;
      grid_t_im=grid_Et_im;
   }

   // sum the results over the entire path
   complex<double> integral=complex<double>(0,0);

   // see if this boundary is on a mesh border
   struct EdgeAttribute test_attribute;
   test_attribute.boundary=boundary->get_attribute();
   test_attribute.path=-1;
   test_attribute.segment=-1;
   long unsigned int boundary_index=borderDatabase->exists_test_boundary_only(test_attribute);
   bool on_border=true;
   if (boundary_index == max) on_border=false;
   //on_border=false;  // to force numerical integration

   // loop through the paths
   long unsigned int iPath=0;
   while (iPath < boundary->get_path_size()) {

      bool reverse=boundary->get_reverse(iPath);

      vector<Path *> *pathList=boundaryDatabase->get_pathList();
      Path *path=(*pathList)[boundary->get_path(iPath)];

      // loop through the path segments

      long unsigned int limit=path->get_points_size();
      if (path->is_closed()) limit++;

      struct point p1;
      struct point p2=path->get_point_value(0);

      long unsigned int j=1;
      while (j < limit) {

         p1=point_copy(p2);
         p2=path->get_point_value(j);

         // get a unit vector in the direction of the supplied line
         struct point p2mp1=point_subtraction(p2,p1);
         double length=point_magnitude(p2mp1);
         struct point nhat=point_scale(1/length,p2mp1);

         if (on_border) {

            // integrate with an MFEM integrator

            has_printed_integrator=true;
            if (! has_printed_integrator) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"            calculating integral with MFEM integrator\n");
               has_printed_integrator=true;
            }

            // restrict to the needed border elements

            // knock out all
            fem->border_attributes=0;

            // add back in this border, path, and segment
            long unsigned int k=0;
            while (k < borderDatabase->get_size()) {
               Border *border=borderDatabase->get_border(k);
               if ((int)border->get_mode(boundary->get_mode()).boundary == boundary->get_attribute() && 
                   border->get_mode(boundary->get_mode()).path == iPath &&
                   border->get_mode(boundary->get_mode()).segment == j-1) {
                  fem->border_attributes[border->get_global_attribute()-1]=1;
               }
               k++;
            }

            Vector unit(2);
            unit.Elem(0)=nhat.x;
            unit.Elem(1)=nhat.y;
            VectorConstantCoefficient unitVec(unit);

            ParLinearForm lf(fem->fespace_ND);
            lf.AddBoundaryIntegrator(new VectorFEDomainLFIntegrator(unitVec),fem->border_attributes);
            lf.Assemble();

            double I_re=lf(*grid_t_re);
            double I_im=lf(*grid_t_im);
            if (reverse) integral-=complex<double>(I_re,I_im);
            else         integral+=complex<double>(I_re,I_im);

         } else {

            // numerically integrate

            has_printed_numerical=true;
            if (! has_printed_numerical) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"            calculating integral with numerical integration\n");
               has_printed_numerical=true;
            }

            // get points along the integration line
            DenseMatrix points(2,fem->pointsCount);
            double theta=atan2(p2mp1.y,p2mp1.x);
            int i=0;
            while (i < fem->pointsCount) {
               points.Elem(0,i)=p1.x+cos(theta)*length*(double)i/((double)(fem->pointsCount-1));
               points.Elem(1,i)=p1.y+sin(theta)*length*(double)i/((double)(fem->pointsCount-1));
               i++;
            }

            // get the field values
            if (getFieldValues (fem->pointsCount,fem->pmesh,&points,grid_t_re,grid_t_im,&fieldValuesX,&fieldValuesY)) {
               return complex<double>(DBL_MAX,DBL_MAX);  // fatal error
            }

            // calculate the integral
            // ToDo - break this up into sections by ranks to parallel process; not really worth it for small pointsCount

            i=0;
            while (i < fem->pointsCount-1) {

               xi1=points.Elem(0,i);
               yi1=points.Elem(1,i);

               xi2=points.Elem(0,i+1);
               yi2=points.Elem(1,i+1);

               segmentLength=sqrt((xi2-xi1)*(xi2-xi1)+(yi2-yi1)*(yi2-yi1));

               // enable the calculation to continue in case a point lies just outside the mesh
               if (fieldValuesX[i] == complex<double>(DBL_MAX,DBL_MAX) || fieldValuesX[i+1] == complex<double>(DBL_MAX,DBL_MAX) ||
                   fieldValuesY[i] == complex<double>(DBL_MAX,DBL_MAX) || fieldValuesY[i+1] == complex<double>(DBL_MAX,DBL_MAX)) {i++; continue;}

               valueX=(fieldValuesX[i]+fieldValuesX[i+1])/2;
               valueY=(fieldValuesY[i]+fieldValuesY[i+1])/2;

               if (reverse) integral-=(valueX*nhat.x+valueY*nhat.y)*segmentLength;
               else         integral+=(valueX*nhat.x+valueY*nhat.y)*segmentLength;

               i++;
            }
         }

         j++;
      }
      iPath++;
   }

   return integral;
}

void Fields::calculateFieldPoints (fem2D *fem, int mode, FieldPointDatabase *fieldPointDatabase)
{
   FieldPoint fieldPoint;
   vector<complex<double>> Etx,Ety,Etz,Htx,Hty,Htz;

   // skip if there are no frequency points to calculate
   if (fem->projData->field_points_count == 0) return;

   // intermediate storage to call getFieldValues
   Etx.resize(fem->projData->field_points_count);
   Ety.resize(fem->projData->field_points_count);
   Etz.resize(fem->projData->field_points_count);

   Htx.resize(fem->projData->field_points_count);
   Hty.resize(fem->projData->field_points_count);
   Htz.resize(fem->projData->field_points_count);


   // transfer the (x,y) locations to a new data structure
   DenseMatrix points(2,fem->projData->field_points_count);
   int i=0;
   while (i < fem->projData->field_points_count) {
      points.Elem(0,i)=fem->projData->field_points_x[i];
      points.Elem(1,i)=fem->projData->field_points_y[i];
      i++;
   }

   // This setup is inefficient because each call to getFieldValues re-calculates all the information about the mesh.
   // It is assumed that the field point calculation is only used for validation work, so the loss of efficiency is acceptable.

   if (getFieldValues(fem->projData->field_points_count,fem->pmesh,&points,grid_Et_re,grid_Et_im,&Etx,&Ety)) return;
   if (getFieldValues(fem->projData->field_points_count,fem->pmesh,&points,grid_Ez_re,grid_Ez_im,&Etz,nullptr)) return;
   if (getFieldValues(fem->projData->field_points_count,fem->pmesh,&points,grid_Ht_re,grid_Ht_im,&Htx,&Hty)) return;
   if (getFieldValues(fem->projData->field_points_count,fem->pmesh,&points,grid_Hz_re,grid_Hz_im,&Htz,nullptr)) return;

   i=0;
   while (i < fem->projData->field_points_count) {

      fieldPoint.set_frequency(fem->frequency);
      fieldPoint.set_mode(mode);
      fieldPoint.set_dim(2);
      fieldPoint.set_x(fem->projData->field_points_x[i]);
      fieldPoint.set_y(fem->projData->field_points_y[i]);

      fieldPoint.set_Ex_re(real(Etx[i]));
      fieldPoint.set_Ex_im(imag(Etx[i]));
      fieldPoint.set_Ey_re(real(Ety[i]));
      fieldPoint.set_Ey_im(imag(Ety[i]));
      fieldPoint.set_Ez_re(real(Etz[i]));
      fieldPoint.set_Ez_im(imag(Etz[i]));

      fieldPoint.set_Hx_re(real(Htx[i]));
      fieldPoint.set_Hx_im(imag(Htx[i]));
      fieldPoint.set_Hy_re(real(Hty[i]));
      fieldPoint.set_Hy_im(imag(Hty[i]));
      fieldPoint.set_Hz_re(real(Htz[i]));
      fieldPoint.set_Hz_im(imag(Htz[i]));

      fieldPointDatabase->push(&fieldPoint);

      i++;
   }
}

// perturbational loss calculated over each boundary segment
double Fields::calculatePerturbationalLoss(fem2D *fem, BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase, MaterialDatabase *materialDatabase)
{
   double tolerance=1e-12;
   string indent="   ";
   double perturbationalLoss=0;

   long unsigned int i=0;
   while (i < borderDatabase->get_size()) {
      // find the boundary associated with this border
      Border *border=borderDatabase->get_border(i);
      if (border->has_boundary()) {
         long unsigned int boundary_index=border->get_boundary().boundary;
         Boundary *boundary=boundaryDatabase->get_boundary(boundary_index);
         if (boundary->is_surface_impedance()) {
            // get the surface impedance
            double Rs=0;
            Material *material=materialDatabase->get(boundary->get_material());
            if (material) {
               Rs=material->get_Rs(fem->projData->solution_temperature, fem->frequency, tolerance, indent);
               if (Rs == -DBL_MAX) return -DBL_MAX;
            } else {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2144: Material \"%s\" does not exist in the materials database.  Defaulting to PEC.\n",
                                                      boundary->get_material().c_str());
            }

            // knock out all boundaries and modes
            fem->border_attributes=0;

            // put this one back
            fem->border_attributes[border->get_global_attribute()-1]=1;

            // integrate the Ht contribution

            // real
            VectorGridFunctionCoefficient Ht_re_coef(grid_Ht_re);
            ParLinearForm Ht_re_lf(fem->fespace_ND);
            Ht_re_lf.AddBoundaryIntegrator(new VectorFEDomainLFIntegrator(Ht_re_coef),fem->border_attributes);
            Ht_re_lf.Assemble();
            double Ht_re_mag2=Ht_re_lf(*grid_Ht_re);

            // imag
            VectorGridFunctionCoefficient Ht_im_coef(grid_Ht_im);
            ParLinearForm Ht_im_lf(fem->fespace_ND);
            Ht_im_lf.AddBoundaryIntegrator(new VectorFEDomainLFIntegrator(Ht_im_coef),fem->border_attributes);
            Ht_im_lf.Assemble();
            double Ht_im_mag2=Ht_im_lf(*grid_Ht_im);

            // integrate the Hz contribution

            // real
            GridFunctionCoefficient Hz_re_coef(grid_Hz_re);
            ParLinearForm Hz_re_lf(fem->fespace_H1);
            Hz_re_lf.AddBoundaryIntegrator(new DomainLFIntegrator(Hz_re_coef),fem->border_attributes);
            Hz_re_lf.Assemble();
            double Hz_re_mag2=Hz_re_lf(*grid_Hz_re);

            // imag
            GridFunctionCoefficient Hz_im_coef(grid_Hz_im);
            ParLinearForm Hz_im_lf(fem->fespace_H1);
            Hz_im_lf.AddBoundaryIntegrator(new DomainLFIntegrator(Hz_im_coef),fem->border_attributes);
            Hz_im_lf.Assemble();
            double Hz_im_mag2=Hz_im_lf(*grid_Hz_im);

            perturbationalLoss+=0.5*Rs*(Ht_re_mag2+Ht_im_mag2+Hz_re_mag2+Hz_im_mag2);
         }
      }
      i++;
   }

   return perturbationalLoss;
}

void Fields::copyEH (Fields *a)
{
   *grid_Et_re=*(a->grid_Et_re);
   *grid_Et_im=*(a->grid_Et_im);
   *grid_Ez_re=*(a->grid_Ez_re);
   *grid_Ez_im=*(a->grid_Ez_im);

   *grid_Ht_re=*(a->grid_Ht_re);
   *grid_Ht_im=*(a->grid_Ht_im);
   *grid_Hz_re=*(a->grid_Hz_re);
   *grid_Hz_im=*(a->grid_Hz_im);
}

void weight_grid (ParGridFunction *grid_re, ParGridFunction *grid_im, complex<double> weight)
{
   Vector dofs_re;
   grid_re->GetTrueDofs(dofs_re);

   Vector dofs_im;
   grid_im->GetTrueDofs(dofs_im);

   Vector weighted_dofs_re(dofs_re.Size());
   Vector weighted_dofs_im(dofs_im.Size());

   int i=0;
   while (i < dofs_re.Size()) {
      weighted_dofs_re(i)=dofs_re(i)*real(weight)-dofs_im(i)*imag(weight);
      weighted_dofs_im(i)=dofs_re(i)*imag(weight)+dofs_im(i)*real(weight);
      i++;
   }

   grid_re->Distribute(weighted_dofs_re);
   grid_im->Distribute(weighted_dofs_im);
}

void weight_and_add_grid (ParGridFunction *grid_re, ParGridFunction *grid_im, 
                          ParGridFunction *add_grid_re, ParGridFunction *add_grid_im, complex<double> weight)
{
   // weight the grid to add

   Vector add_dofs_re;
   add_grid_re->GetTrueDofs(add_dofs_re);

   Vector add_dofs_im;
   add_grid_im->GetTrueDofs(add_dofs_im);

   Vector weighted_dofs_re(add_dofs_re.Size());
   Vector weighted_dofs_im(add_dofs_im.Size());

   int i=0;
   while (i < add_dofs_re.Size()) {
      weighted_dofs_re(i)=add_dofs_re(i)*real(weight)-add_dofs_im(i)*imag(weight);
      weighted_dofs_im(i)=add_dofs_re(i)*imag(weight)+add_dofs_im(i)*real(weight);
      i++;
   }

   // add

   Vector dofs_re;
   grid_re->GetTrueDofs(dofs_re);

   Vector dofs_im;
   grid_im->GetTrueDofs(dofs_im);

   i=0;
   while (i < dofs_re.Size()) {
      dofs_re(i)+=weighted_dofs_re(i);
      dofs_im(i)+=weighted_dofs_im(i);
      i++;
   }

   grid_re->Distribute(dofs_re);
   grid_im->Distribute(dofs_im);
}

void Fields::weightEH (complex<double> weight)
{
   weight_grid(grid_Et_re,grid_Et_im,weight);
   weight_grid(grid_Ez_re,grid_Ez_im,weight);
   weight_grid(grid_Ht_re,grid_Ht_im,weight);
   weight_grid(grid_Hz_re,grid_Hz_im,weight);
}

void Fields::weightAndAddEH (Fields *a, complex<double> weight)
{
   weight_and_add_grid(grid_Et_re,grid_Et_im,a->grid_Et_re,a->grid_Et_im,weight);
   weight_and_add_grid(grid_Ez_re,grid_Ez_im,a->grid_Ez_re,a->grid_Ez_im,weight);
   weight_and_add_grid(grid_Ht_re,grid_Ht_im,a->grid_Ht_re,a->grid_Ht_im,weight);
   weight_and_add_grid(grid_Hz_re,grid_Hz_im,a->grid_Hz_re,a->grid_Hz_im,weight);
}

Vector* getGlobalVector (HypreParVector *a)
{
   int rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   if (a == nullptr) return nullptr;

   Vector *b=new Vector(a->GlobalSize());
   if (b == nullptr) return nullptr;

   const HYPRE_BigInt *partition=a->Partitioning();

   // collect at zero

   if (rank == 0) {
      int low=partition[0];
      int high=partition[1];

      int i=low;
      while (i < high) {
         b->Elem(i)=a->Elem(i-low);
         i++;
      }

      i=1;
      while (i < size) {
         int low,high;
         MPI_Recv(&low,1,MPI_INT,i,100,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&high,1,MPI_INT,i,101,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         double *pack=(double *)malloc((high-low)*sizeof(double));
         MPI_Recv(pack,high-low,MPI_DOUBLE,i,102,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         int j=low;
         while (j < high) {
            b->Elem(j)=pack[j-low];
            j++;
         }
         if (pack) free(pack);

         i++;
      }
   } else {
      int low=partition[0];
      int high=partition[1];
      MPI_Send(&low,1,MPI_INT,0,100,PETSC_COMM_WORLD);
      MPI_Send(&high,1,MPI_INT,0,101,PETSC_COMM_WORLD);

      double *pack=(double *)malloc((high-low)*sizeof(double));
      int j=low;
      while (j < high) {
         pack[j-low]=a->Elem(j-low);
         j++;
      }
      MPI_Send(pack,high-low,MPI_DOUBLE,0,102,PETSC_COMM_WORLD);
      if (pack) free(pack);
   }

   // distribute

   if (rank == 0) {
      int i=1;
      while (i < size) {
         double *pack=(double *)malloc(b->Size()*sizeof(double));
         int j=0;
         while (j < b->Size()) {
            pack[j]=b->Elem(j);
            j++;
         }
         MPI_Send(pack,b->Size(),MPI_DOUBLE,i,103,PETSC_COMM_WORLD);
         if (pack) free(pack);
         i++;
      }
   } else {
      double *pack=(double *)malloc(b->Size()*sizeof(double));
      MPI_Recv(pack,b->Size(),MPI_DOUBLE,0,103,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      int i=0;
      while (i < b->Size()) {
         b->Elem(i)=pack[i];
         i++;
      }
      if (pack) free(pack);
   } 

   return b;
}

void Fields::print()
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"      Fields:\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         this=%p\n",this);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         grid_Et_re=%p\n",grid_Et_re);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         grid_Et_im=%p\n",grid_Et_im);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         grid_Ez_re=%p\n",grid_Ez_re);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         grid_Ez_im=%p\n",grid_Ez_im);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         grid_Ht_re=%p\n",grid_Ht_re);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         grid_Ht_im=%p\n",grid_Ht_im);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         grid_Hz_re=%p\n",grid_Hz_re);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         grid_Hz_im=%p\n",grid_Hz_im);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         grid_Pz_re=%p\n",grid_Pz_re);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         grid_Pz_im=%p\n",grid_Pz_im);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         EfieldRe=%p\n",EfieldRe);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         EfieldIm=%p\n",EfieldIm);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         HfieldRe=%p\n",HfieldRe);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         HfieldIm=%p\n",HfieldIm);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         xScale=%g\n",xScale);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         yScale=%g\n",yScale);
}

void Fields::save_grid_E(long unsigned int i)
{
   char filename[128];

   sprintf (filename,"%s_mode_%ld","grid_Et_re",i);
   grid_Et_re->Save(filename);

   sprintf (filename,"%s_mode_%ld","grid_Et_im",i);
   grid_Et_im->Save(filename);

   sprintf (filename,"%s_mode_%ld","grid_Ez_re",i);
   grid_Ez_re->Save(filename);

   sprintf (filename,"%s_mode_%ld","grid_Ez_im",i);
   grid_Ez_im->Save(filename);

}

void Fields::saveAsOne_grid_E(long unsigned int i)
{
   char filename[128];

   sprintf (filename,"%s_mode_%ld_asOne","grid_Et_re",i);
   grid_Et_re->SaveAsOne(filename);

   sprintf (filename,"%s_mode_%ld_asOne","grid_Et_im",i);
   grid_Et_im->SaveAsOne(filename);

   sprintf (filename,"%s_mode_%ld_asOne","grid_Ez_re",i);
   grid_Ez_re->SaveAsOne(filename);

   sprintf (filename,"%s_mode_%ld_asOne","grid_Ez_im",i);
   grid_Ez_im->SaveAsOne(filename);

}


//--------------------------------------------------------------------------------------------------------------------
// Mode
//--------------------------------------------------------------------------------------------------------------------

Mode::Mode()
{
   modeNumber=0;
   Pzavg=complex<double>(0,0);
   alpha=0;
   beta=0;
   perturbationalLoss=0;
   fields=new Fields();
   mode_current=complex<double>(DBL_MAX,DBL_MAX);
   mode_voltage=complex<double>(DBL_MAX,DBL_MAX);
   validCurrent=false;
   validVoltage=false;
}

bool Mode::buildFields(fem2D *fem)
{
   if (fields->build(fem,modeNumber)) return true;
   calculatePz(fem);
   return false;
}

void Mode::updateGrids(fem2D *fem)
{
   fields->updateGrids();
}

void Mode::saveParaView(fem2D *fem)
{
   fields->saveParaView(fem,modeNumber,true);
}

// refine on a partial component of the tangengial magnetic field
// ToDo: write a custom integrator to enable adaptive meshing on the total electric field
bool Mode::ZZrefineMesh (fem2D *fem, ConvergenceDatabase *convergenceDatabase)
{
   long unsigned int i;
   int j;
   bool use_initial_guess=true;
   int rank,size;

   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // skip refinement if the mode is converged to avoid unnecessary mesh creation
   if (! fem->projData->refinement_refine_converged_modes && convergenceDatabase->is_converged(modeNumber)) {
      if (fem->projData->output_show_refining_mesh) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"         mode %ld skipped\n",modeNumber+1);}
      return use_initial_guess;
   }

   // continuing

   if (fem->projData->output_show_refining_mesh) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"         mode %ld:\n",modeNumber+1);}

   // real errors
   L2_FECollection *fec_L2_flux=new L2_FECollection(fem->order-1,fem->pmesh->Dimension());  // flux
   H1_FECollection *fec_H1_flux=new H1_FECollection(fem->order,fem->pmesh->Dimension());    // smoothed flux
   CurlCurlIntegrator *CCinteg=new CurlCurlIntegrator ();                                      // integrator
   ParFiniteElementSpace *fespace_L2_flux=new ParFiniteElementSpace(fem->pmesh,fec_L2_flux);
   ParFiniteElementSpace *fespace_H1_flux=new ParFiniteElementSpace(fem->pmesh,fec_H1_flux);
   L2ZienkiewiczZhuEstimator *estimator=new L2ZienkiewiczZhuEstimator(*CCinteg,*fields->get_grid_Ht_re(),fespace_L2_flux,fespace_H1_flux);
   Vector localErrors;
   localErrors=estimator->GetLocalErrors();
   delete CCinteg;
   delete fec_H1_flux;
   delete fec_L2_flux;
   delete estimator;

   // imag errors
   // Fields::loadEigenvector normalizes the field to make the real part dominant.  Including the imag part here
   // is probably not doing anything while doubling the time it takes to calculate the element errors.
   fec_L2_flux=new L2_FECollection(fem->order-1,fem->pmesh->Dimension());  // flux
   fec_H1_flux=new H1_FECollection(fem->order,fem->pmesh->Dimension());    // smoothed flux
   CCinteg=new CurlCurlIntegrator ();                                      // integrator
   fespace_L2_flux=new ParFiniteElementSpace(fem->pmesh,fec_L2_flux);
   fespace_H1_flux=new ParFiniteElementSpace(fem->pmesh,fec_H1_flux);
   estimator=new L2ZienkiewiczZhuEstimator(*CCinteg,*fields->get_grid_Ht_im(),fespace_L2_flux,fespace_H1_flux);
   Vector localErrorsIm;
   localErrorsIm=estimator->GetLocalErrors();
   delete CCinteg;
   delete fec_H1_flux;
   delete fec_L2_flux;
   delete estimator;

   int local_error_size=localErrors.Size();

   // combine errors
   j=0;
   while (j < local_error_size) {
      localErrors[j]=sqrt(localErrors[j]*localErrors[j]+localErrorsIm[j]*localErrorsIm[j]);
      j++;
   }

   // element centers and local error limits
   DenseMatrix centers(2,local_error_size);
   Vector center(2);
   j=0;
   while (j < local_error_size) {
      fem->pmesh->GetElementCenter (j,center);
      centers.Elem(0,j)=center.Elem(0);
      centers.Elem(1,j)=center.Elem(1);
      j++;
   }

   // get element indices for each point
   Array<int> localElements(local_error_size);
   Array<IntegrationPoint> integrationPoints(local_error_size);
   findPoints(fem->pmesh,centers,localElements,integrationPoints,2);

   // merge into a global list at rank 0

   vector<double> errors;
   vector<int> elements;
   vector<int> ranks;

   if (rank == 0) {

      // local
      int i=0;
      while (i < local_error_size) {
         errors.push_back(localErrors[i]);
         elements.push_back(localElements[i]);
         ranks.push_back(rank);
         i++;
      }

      // collected
      i=1;
      while (i < size) {
         int transfer_count=0;
         MPI_Recv(&transfer_count,1,MPI_INT,i,100,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int k=0;
         while (k < transfer_count) {
            double transfer_error=0;
            int transfer_element=0;
            int transfer_rank=0;
            MPI_Recv(&transfer_error,1,MPI_DOUBLE,i,101,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(&transfer_element,1,MPI_INT,i,102,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(&transfer_rank,1,MPI_INT,i,103,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

            errors.push_back(transfer_error);
            elements.push_back(transfer_element);
            ranks.push_back(transfer_rank);
            k++;
         }
         i++;
      }
   } else {
      MPI_Send(&local_error_size,1,MPI_INT,0,100,PETSC_COMM_WORLD);

      int i=0;
      while (i < local_error_size) {
         double transfer_error=localErrors[i];
         int transfer_element=localElements[i];
         int transfer_rank=rank;

         MPI_Send(&transfer_error,1,MPI_DOUBLE,0,101,PETSC_COMM_WORLD);
         MPI_Send(&transfer_element,1,MPI_INT,0,102,PETSC_COMM_WORLD);
         MPI_Send(&transfer_rank,1,MPI_INT,0,103,PETSC_COMM_WORLD);

         i++;
      }
   }

   //  get the global refinement count
   int refinementCount=GetGlobalNE(fem->pmesh)*fem->projData->mesh_refinement_fraction/fem->meshScale;
   if (refinementCount == 0) refinementCount=1;

   // sort the top errors
   if (rank == 0) {
      long unsigned int i=0;
      while (i < (long unsigned int)refinementCount) {
         long unsigned int k=i+1;
         while (k < errors.size()) {
            if (errors[i] < errors[k]) {
               double temp_error=errors[i];
               errors[i]=errors[k];
               errors[k]=temp_error;

               int temp_element=elements[i];
               elements[i]=elements[k];
               elements[k]=temp_element;

               int temp_rank=ranks[i];
               ranks[i]=ranks[k];
               ranks[k]=temp_rank;
            }
            k++;
         }
         i++;
      }

      cout << "         max error=" << errors[0] << endl;
   }

   // distribute the errors
   if (rank == 0) {

      int i=1;
      while (i < size) {

         // count the number to send
         int count=0;
         int j=0;
         while (j < refinementCount) {
            if (ranks[j] == i) count++;
            j++;
         }

         MPI_Send(&count,1,MPI_INT,i,200,PETSC_COMM_WORLD);

         // send the data 
         j=0;
         while (j < refinementCount) {
            if (ranks[j] == i) {
               MPI_Send(&(elements[j]),1,MPI_INT,i,201,PETSC_COMM_WORLD);
            } 
            j++;
         } 
         i++;
      }

      // restrict the rank 0 data
      vector<int> temp_elements=elements;
      elements.clear();
      i=0;
      while (i < refinementCount) {
         if (ranks[i] == rank) elements.push_back(temp_elements[i]);
         i++;
      }

   } else {
      int transfer_count=0;
      MPI_Recv(&transfer_count,1,MPI_INT,0,200,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      int i=0;
      while (i < transfer_count) {
         int transfer_element=0;
         MPI_Recv(&transfer_element,1,MPI_INT,0,201,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         elements.push_back(transfer_element);
         i++;
      }
   }

   // transfer for use with GeneralRefinement
   Array<int> localRefineList(elements.size());
   i=0;
   while (i < elements.size()) {
      localRefineList[i]=elements[i];
      i++;
   }

   if (refinementCount == 1) {
      if (fem->projData->output_show_refining_mesh) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"            refining %d element ...\n",refinementCount);}
   } else {
      if (fem->projData->output_show_refining_mesh) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"            refining %d elements ...\n",refinementCount);}
   }

   fem->pmesh->GeneralRefinement(localRefineList);

   if (fem->projData->output_show_refining_mesh) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"            new mesh size: %d\n",GetGlobalNE(fem->pmesh));}

   return use_initial_guess;
}

void Mode::calculatePz (fem2D *fem)
{
   Pzavg=fields->calculatePz(fem);
}

void Mode::calculateVoltages (fem2D *fem, BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase)
{
   voltage.clear();
   long unsigned int i=0;
   while (i < (long unsigned int)fem->projData->solution_active_mode_count) {

      // loop through the boundary definitions
      bool found=false;
      long unsigned int j=0;
      while (j < boundaryDatabase->get_boundary_size()) {

         // pick off voltage definitions for the given mode
         if (boundaryDatabase->get_boundary(j)->is_mode_voltage() && boundaryDatabase->get_boundary(j)->get_mode()-1 == (int)i) {
            // calculate line integrals
            voltage.push_back(-boundaryDatabase->get_boundary(j)->get_scale()*
               fields->calculateLineIntegral(fem,boundaryDatabase->get_boundary(j),boundaryDatabase,borderDatabase,false));
            found=true;
            break;
         }
         j++;
      }
      if (!found) voltage.push_back(complex<double>(DBL_MAX,DBL_MAX));
      i++;
   }
}

void Mode::calculateCurrents (fem2D *fem, BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase)
{
   current.clear();
   long unsigned int i=0;
   while (i < (long unsigned int)fem->projData->solution_active_mode_count) {

      // loop through the boundary definitions
      bool found=false;
      long unsigned int j=0;
      while (j < boundaryDatabase->get_boundary_size()) {

         // pick off current definitions for the given mode
         if (boundaryDatabase->get_boundary(j)->is_mode_current() && boundaryDatabase->get_boundary(j)->get_mode()-1 == (int)i) {

            // calculate line integrals
            current.push_back(boundaryDatabase->get_boundary(j)->get_scale()*
               fields->calculateLineIntegral(fem,boundaryDatabase->get_boundary(j),boundaryDatabase,borderDatabase,true));
            found=true;
            break;
         }
         j++;
      }
      if (!found) current.push_back(complex<double>(DBL_MAX,DBL_MAX));
      i++;
   }
}

void Mode::flipFieldSign ()
{
   fields->flipSign();

   long unsigned int i=0;
   while (i < voltage.size()) {
      if (voltage[i] != complex<double>(DBL_MAX,DBL_MAX)) voltage[i]=-voltage[i];
      i++;
   }

   i=0;
   while (i < current.size()) {
      if (current[i] != complex<double>(DBL_MAX,DBL_MAX)) current[i]=-current[i];
      i++;
   }
}

bool Mode::calculateModalCurrent (fem2D *fem)
{
   bool fail=false;
   int n=fem->projData->solution_modes;

   bool use_modal_calculation=false;
   if (is_modal_impedance(fem->projData->solution_impedance_calculation)) use_modal_calculation=true;
   if (is_line_impedance(fem->projData->solution_impedance_calculation)) {
      use_modal_calculation=false;
      // For single-mode ports, the modal and line calculations are identical.
      if (n == 1) use_modal_calculation=true;
   } 

   // For modal calculation, the user must set up the voltage and current paths to calculate the correct
   // voltage and current for each specific mode.  For anything except symmetric differential pairs,
   // the setup is challenging.
   //
   if (use_modal_calculation) {
      mode_current=get_current(modeNumber);
      validCurrent=false;
      if (mode_current != complex<double>(DBL_MAX,DBL_MAX)) {
         validCurrent=true;
         if (real(mode_current) < 0) {
            flipFieldSign();
            mode_current=-mode_current;
         }
      }

      if (validCurrent) {
         if (!fem->Ti) {
            fem->Ti=(lapack_complex_double *) malloc(n*n*sizeof(lapack_complex_double));
            matrixZero(fem->Ti,n);
         }

         // unity for modes
         matrixSetValue(fem->Ti,modeNumber+n*modeNumber,1,0);
      }
   }

   // For line calculation, the user simply provides a voltage line from ground to each line or a closed
   // current path around each conductor or both.  The user does not need to specifically set up the paths
   // unique to each mode.  The algorithm here attempts to combine the voltages and currents to extract
   // the modal voltages and currents.  It is proven against the known modal solution for the symmetric
   // differential pair, but the algorithm may have issues with arbitrary multi-conductor setups.
   //
   if (!use_modal_calculation) {

      // check for valid currents
      validCurrent=true;
      long unsigned int k=0;
      while (k < current.size()) {
         if (current[k] == complex<double>(DBL_MAX,DBL_MAX)) {
            validCurrent=false;
            break;
         }
         k++;
      }

      if (validCurrent) {

         if (!fem->Ti) {
            fem->Ti=(lapack_complex_double *) malloc(n*n*sizeof(lapack_complex_double));
            matrixZero(fem->Ti,n);
         }

         // fill Ti for this row

         complex<double> return_current=0;
         mode_current=0;

         long unsigned int k=0;
         while (k < current.size()) {
            double sign=-1;
            if (real(current[k]) >= 0) sign=1;
            matrixSetValue(fem->Ti,modeNumber+k*n,sign,0);

            mode_current+=sign*current[k];
            return_current+=current[k];

            k++;
         }

         // scale to account for the return current
         complex<double> scale=0.5*(abs(mode_current)+abs(return_current))/abs(mode_current);
         k=0;
         while (k < current.size()) {
            matrixScaleValue(fem->Ti,modeNumber+k*n,real(scale),imag(scale));
            k++;
         }
         mode_current*=scale;

         // set the first column to be positive to ensure that the fields have the same polarity across ports
         if (matrixGetRealValue(fem->Ti,modeNumber) < 0) {
            flipFieldSign();
            matrixScaleRow (fem->Ti,n,modeNumber,-1,0);
            mode_current=-mode_current;
         }

         mode_current=0;
         k=0;
         while (k < current.size()) {
            double realVal=matrixGetRealValue(fem->Ti,modeNumber+n*k);
            double imagVal=matrixGetImagValue(fem->Ti,modeNumber+n*k);
            mode_current+=complex<double>(realVal,imagVal)*current[k];
            k++;
         }
      } else {
         if (fem->Ti) {free(fem->Ti); fem->Ti=nullptr;}
         mode_current=complex<double>(DBL_MAX,DBL_MAX);
         fail=true;
      }
   }

   return fail;
}

bool Mode::calculateModalVoltage (fem2D *fem)
{
   bool fail=false;
   int n=fem->projData->solution_modes;

   bool use_modal_calculation=false;
   if (is_modal_impedance(fem->projData->solution_impedance_calculation)) use_modal_calculation=true;
   if (is_line_impedance(fem->projData->solution_impedance_calculation)) {
      use_modal_calculation=false;
      if (n == 1) use_modal_calculation=true;
   } 

   if (use_modal_calculation) {
      mode_voltage=get_voltage(modeNumber);
      validVoltage=false;
      if (mode_voltage != complex<double>(DBL_MAX,DBL_MAX)) {
         validVoltage=true;
         if (! validCurrent && real(mode_voltage) < 0) {
            flipFieldSign();
            mode_voltage=-mode_voltage;
         }
      }

      if (validVoltage) {
         if (!fem->Tv) {
            fem->Tv=(lapack_complex_double *) malloc(n*n*sizeof(lapack_complex_double));
            matrixZero(fem->Tv,n);
         }

         // unity for modes
         matrixSetValue(fem->Tv,modeNumber+n*modeNumber,1,0);
      }
   }

   if (!use_modal_calculation) {
    
      validVoltage=false;

      // line voltages require line currents
      if (validCurrent) {

         // check for valid voltages
         validVoltage=true;
         long unsigned int k=0;
         while (k < voltage.size()) {
            if (voltage[k] == complex<double>(DBL_MAX,DBL_MAX)) {
               validVoltage=false;
               break;
            }
            k++;
         }

         if (validVoltage) {
            if (!fem->Tv) {
               fem->Tv=matrixClone(fem->Ti,n);
               matrixConjugate(fem->Tv,n);
               matrixInverse(fem->Tv,n);
               matrixTranspose(fem->Tv,n);
            }

            mode_voltage=0;
            long unsigned int k=0;
            while (k < voltage.size()) {
               double realVal=matrixGetRealValue(fem->Tv,modeNumber+n*k);
               double imagVal=matrixGetImagValue(fem->Tv,modeNumber+n*k);
               mode_voltage+=complex<double>(realVal,imagVal)*voltage[k];
               k++;
            }

            if (real(mode_voltage) < 0) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"INFO: Mode voltage for Sport %ld is negative.\n",modeNumber);
            }
         }
      } else {
         if (fem->Ti) {free(fem->Ti); fem->Ti=nullptr;}
         if (fem->Tv) {free(fem->Tv); fem->Tv=nullptr;}
         mode_voltage=complex<double>(DBL_MAX,DBL_MAX);
         fail=true;
      }
   }

   return fail;
}

void Mode::calculateImpedance (fem2D *fem)
{
   if (fem->projData->debug_show_impedance_details) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"            Mode\n");
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"               %3ld",modeNumber+1);
      if (validVoltage) {          PetscPrintf(PETSC_COMM_WORLD,"  voltage (V): (%g,%g)\n", real(mode_voltage),imag(mode_voltage));}
      else              {          PetscPrintf(PETSC_COMM_WORLD,"  voltage (V): not defined\n");}
      if (validCurrent) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"                    current (I): (%g,%g)\n",real(mode_current),imag(mode_current));}
      else              {prefix(); PetscPrintf(PETSC_COMM_WORLD,"                    current (I): not defined\n");}
                         prefix(); PetscPrintf(PETSC_COMM_WORLD,"                    Pz (Pz,avg): (%g,%g)\n", real(Pzavg),imag(Pzavg));
   }

   if (validVoltage && validCurrent) Zvi=mode_voltage/mode_current;
   else Zvi=complex<double>(DBL_MAX,DBL_MAX);

   if (validVoltage) Zpv=0.5*mode_voltage*conj(mode_voltage)/conj(Pzavg);
   else Zpv=complex<double>(DBL_MAX,DBL_MAX);

   if (validCurrent) Zpi=2*Pzavg/(mode_current*conj(mode_current));
   else Zpi=complex<double>(DBL_MAX,DBL_MAX);
}

bool Mode::calculatePerturbationalLoss(fem2D *fem, BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase, MaterialDatabase *materialDatabase)
{
   perturbationalLoss=fields->calculatePerturbationalLoss(fem,boundaryDatabase,borderDatabase,materialDatabase);
   if (perturbationalLoss == -DBL_MAX) return true;
   return false;
}

void Mode::print()
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Mode:\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   this=%p\b",this);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   modeNumber=%ld\n",modeNumber);

   unsigned long int i=0;
   while (i < current.size()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"   current[%ld]=(%g,%g)\n",i,real(current[i]),imag(current[i]));
      i++;
   }

   i=0;
   while (i < voltage.size()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"   voltage[%ld]=(%g,%g)\n",i,real(voltage[i]),imag(voltage[i]));
      i++;
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Pzavg=(%g,%g)\n",real(Pzavg),imag(Pzavg));
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   alpha=%g\n",alpha);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   beta=%g\n",beta);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   perturbationalLoss=%g\n",perturbationalLoss);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   mode_current=(%g,%g)\n",real(mode_current),imag(mode_voltage));
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   validCurrent=%d\n",validCurrent);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   mode_voltage=(%g,%g)\n",real(mode_voltage),imag(mode_voltage));
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   validVoltage=%d\n",validVoltage);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Zvi=(%g,%g)\n",real(Zvi),imag(Zvi));
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Zpv=(%g,%g)\n",real(Zpv),imag(Zpv));
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Zpi=(%g,%g)\n",real(Zpi),imag(Zpi));
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   fields=%p\n",fields);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   fields:\n");
   fields->print();
}

void Mode::save_grid_E (long unsigned int i)
{
   if (fields == nullptr) return;
   fields->save_grid_E(i);
}

void Mode::saveAsOne_grid_E (long unsigned int i)
{
   if (fields == nullptr) return;
   fields->saveAsOne_grid_E(i);
}

void Mode::calculateFieldPoints (fem2D *fem, FieldPointDatabase *fieldPointDatabase)
{
   fields->calculateFieldPoints(fem,modeNumber,fieldPointDatabase);
}

Mode::~Mode()
{
   if (fields) {delete fields; fields=nullptr;}
}

//--------------------------------------------------------------------------------------------------------------------
// ModeDatabase
//--------------------------------------------------------------------------------------------------------------------

bool ModeDatabase::buildFields(fem2D *fem, double *alpha, double *beta)
{
   long unsigned int i=0;
   while (i < (long unsigned int)fem->projData->solution_active_mode_count) {
      Mode *mode=new Mode();
      modeList.push_back(mode);

      mode->set_modeNumber(i);    // zero based
      mode->set_alpha(alpha[i]);
      mode->set_beta(beta[i]);
      if (mode->buildFields(fem)) return true;

      i++;
   }
   return false;
}

void ModeDatabase::saveParaView(fem2D *fem)
{
   long unsigned int i=0;
   while (i < (long unsigned int)fem->projData->solution_active_mode_count) {
      modeList[i]->saveParaView(fem);
      i++;
   }
}

bool ModeDatabase::ZZrefineMeshes (fem2D *fem, ConvergenceDatabase *convergenceDatabase)
{
   bool use_initial_guess=false;

   long unsigned int i=0;
   while (i < (long unsigned int)fem->projData->solution_active_mode_count) {
      use_initial_guess=modeList[i]->ZZrefineMesh(fem,convergenceDatabase);

      // update fespaces
      fem->fespace_ND->Update();
      fem->fespace_H1->Update();
      fem->fespace_L2->Update();

      // update the grids
      long unsigned int j=0;
      while (j < (long unsigned int)fem->projData->solution_active_mode_count) {
         modeList[j]->updateGrids(fem);
         j++;
      }

      if (! use_initial_guess) break;

      i++;
   }

   return use_initial_guess;
}

void ModeDatabase::writeInitialGuess (fem2D *fem)
{
   long unsigned int i=0;
   while (i < (long unsigned int)fem->projData->solution_active_mode_count) {
      modeList[i]->writeInitialGuess (fem);
      i++;
   }
}

void ModeDatabase::calculatePz(fem2D *fem)
{
   long unsigned int i=0;
   while (i < (long unsigned int)fem->projData->solution_active_mode_count) {
      modeList[i]->calculatePz(fem);
      i++;
   }
}

void ModeDatabase::calculateVandI (fem2D *fem, BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase)
{
   long unsigned int n=fem->projData->solution_active_mode_count;

   long unsigned int i=0;
   while (i < n) {
      modeList[i]->calculateCurrents(fem,boundaryDatabase,borderDatabase);
      modeList[i]->calculateVoltages(fem,boundaryDatabase,borderDatabase);
      i++;
   }
}

bool ModeDatabase::calculateModalVandI (fem2D *fem)
{
   long unsigned int i=0;
   while (i < (long unsigned int)fem->projData->solution_active_mode_count) {
      if (modeList[i]->calculateModalCurrent(fem)) return true;
      i++;
   }

   i=0;
   while (i < (long unsigned int)fem->projData->solution_active_mode_count) {
      if (modeList[i]->calculateModalVoltage(fem)) return true;
      i++;
   }

   return false;
}

void ModeDatabase::calculateImpedance (fem2D *fem, BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase)
{
   long unsigned int i=0;
   while (i < (long unsigned int)fem->projData->solution_active_mode_count) {
      modeList[i]->calculateImpedance(fem);
      i++;
   }

   if (fem->projData->debug_show_impedance_details) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"            Mode\n");
   }

   if (fem->projData->debug_show_impedance_details) {
      long unsigned int i=0;
      while (i < (long unsigned int)fem->projData->solution_active_mode_count) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"               %3ld",i+1);
         Mode *mode=modeList[i];

         PetscPrintf(PETSC_COMM_WORLD,"  Impedance (VI):");
         if (mode->get_Zvi() != complex<double>(DBL_MAX,DBL_MAX)) {PetscPrintf(PETSC_COMM_WORLD," (%g,%g)\n",real(mode->get_Zvi()),imag(mode->get_Zvi()));}
         else {PetscPrintf(PETSC_COMM_WORLD," not defined\n");}

         prefix(); PetscPrintf(PETSC_COMM_WORLD,"                              (PV):");
         if (mode->get_Zpv() != complex<double>(DBL_MAX,DBL_MAX)) {PetscPrintf(PETSC_COMM_WORLD," (%g,%g)\n",real(mode->get_Zpv()),imag(mode->get_Zpv()));}
         else {PetscPrintf(PETSC_COMM_WORLD," not defined\n");}

         prefix(); PetscPrintf(PETSC_COMM_WORLD,"                              (PI):");
         if (mode->get_Zpi() != complex<double>(DBL_MAX,DBL_MAX)) {PetscPrintf(PETSC_COMM_WORLD," (%g,%g)\n",real(mode->get_Zpi()),imag(mode->get_Zpi()));}
         else {PetscPrintf(PETSC_COMM_WORLD," not defined\n");}

         i++;
      }
   }
}

bool ModeDatabase::calculatePerturbationalLoss(fem2D *fem, BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase, MaterialDatabase *materialDatabase)
{
   long unsigned int i=0;
   while (i < (long unsigned int)fem->projData->solution_active_mode_count) {
      if (modeList[i]->calculatePerturbationalLoss(fem,boundaryDatabase,borderDatabase,materialDatabase)) return true;
      i++;
   }
   return false;
}

void ModeDatabase::calculateFieldPoints (fem2D *fem, FieldPointDatabase *fieldPointDatabase)
{
   long unsigned int i=0;
   while (i < (long unsigned int)fem->projData->solution_active_mode_count) {
      modeList[i]->calculateFieldPoints (fem,fieldPointDatabase);
      i++;
   }
}

// sort degenerate modes by power
void ModeDatabase::sort (fem2D *fem)
{
   long unsigned int i=0;
   while (i < modeList.size()-1) {
      bool found=true;
      while (found) {
         found=false;
         long unsigned int j=i+1;
         while (j < modeList.size()) {
            complex<double> gamma_i=modeList[i]->get_gamma();
            complex<double> gamma_j=modeList[j]->get_gamma();

            if (! modeList[i]->get_fields()) continue;
            if (! modeList[j]->get_fields()) continue;

            complex<double> scale_i=modeList[i]->get_fields()->get_scale();
            complex<double> scale_j=modeList[j]->get_fields()->get_scale();

            // scale Pzavg by the scale factor used in Fields::loadEigenvector
            // This is an absolute comparison, and the modes use unique scale factors.
            // To get a valid comparison, the scaling has to be reversed.
            // Note that for use in Z calculations, the calculation cancels the scaling,
            // so unique scale factors per mode is ok there.
            double Pzavg_i=abs(modeList[i]->get_Pzavg()*scale_i*scale_i);
            double Pzavg_j=abs(modeList[j]->get_Pzavg()*scale_j*scale_j);

            // check if degenerate
            if (abs((gamma_i-gamma_j)/gamma_i) < 1e-4) {

               // swap if j has more power than i
               if (Pzavg_i < Pzavg_j) {

                  Mode *temp=modeList[i];
                  modeList[i]=modeList[j];
                  modeList[j]=temp;

                  long unsigned int mode=modeList[i]->get_modeNumber();
                  modeList[i]->set_modeNumber(modeList[j]->get_modeNumber());
                  modeList[j]->set_modeNumber(mode);

                  found=true;
               }
            }
            j++;
         }
      }
      i++;
   }
}

void ModeDatabase::print()
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"ModeDatabase:\n");
             PetscPrintf(PETSC_COMM_WORLD,"   this=%p\n",this);
             PetscPrintf(PETSC_COMM_WORLD,"   modeList.size=%ld\n",modeList.size());

   long unsigned int i=0;
   while (i < (long unsigned int)modeList.size()) {
      modeList[i]->print();
      i++;
   }
}

void ModeDatabase::save_grid_E()
{
   long unsigned int i=0;
   while (i < (long unsigned int)modeList.size()) {
      modeList[i]->save_grid_E(i);
      i++;
   }
}

void ModeDatabase::saveAsOne_grid_E()
{
   long unsigned int i=0;
   while (i < (long unsigned int)modeList.size()) {
      modeList[i]->saveAsOne_grid_E(i);
      i++;
   }
}

ModeDatabase::~ModeDatabase()
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      delete modeList[i];
      i++;
   }
}

//--------------------------------------------------------------------------------------------------------------------
// fem2D
//--------------------------------------------------------------------------------------------------------------------

fem2D::fem2D(struct projectData *projData_, ParMesh *pmesh_, int order_, double frequency_, int iteration_,
             PWConstCoefficient *ko2Re_e, PWConstCoefficient *ko2Im_e, PWConstCoefficient *Inv_mu, // for E-field
             PWConstCoefficient *w_mu,                                                             // for H-field
             string temporaryDirectory_,
             bool *error) 
{
   projData=projData_;
   pmesh=pmesh_;
   order=order_;
   frequency=frequency_;
   iteration=iteration_;
   temporaryDirectory=temporaryDirectory_;
   Ti=nullptr;
   Tv=nullptr;

   // Array for holding border element selections
   border_attributes.SetSize(pmesh->bdr_attributes.Max());

   //*****************************************************************************************************
   // finite element spaces
   //*****************************************************************************************************

   fec_ND=new ND_FECollection(order,pmesh->Dimension());
   fespace_ND=new ParFiniteElementSpace(pmesh,fec_ND);

   fec_H1=new H1_FECollection(order,pmesh->Dimension());
   fespace_H1=new ParFiniteElementSpace(pmesh,fec_H1);

   fec_L2=new L2_FECollection(order-1,pmesh->Dimension());
   fespace_L2=new ParFiniteElementSpace(pmesh,fec_L2);

   //*****************************************************************************************************
   // matrices for the solution of the eigenvalue problem for the E-field and gamma
   //*****************************************************************************************************

   // bilinear forms - Implements the integrals over the finite element spaces.
   // These are defined as (13) in the paper.  The constants from (12) are pulled in so that MFEM can integrate
   // material changes over the cross section.  The dielectric constant can be complex, while the relative permeability
   // is only real.
   //
   // There are 5 equations in (12).
   // Tt (1st instance) - complex eps constant - Done in real and imaginary parts using real math since MFEM doesn't support complex.
   // Tt (2nd instance) - real mur constant
   // G  - real mur constant
   // Sz - real mur constant
   // Tz - complex eps constant - Done in real and imaginary parts using real math since MFEM doesn't support complex.
   // St - real mur constant
   //
   // So there are a total of 8 integrals to calculate.

   // Tt

   // instance with 1/mur

   ParBilinearForm Tt_mur=ParBilinearForm(fespace_ND);
   Tt_mur.AddDomainIntegrator(new VectorFEMassIntegrator (*Inv_mu));
   Tt_mur.Assemble();
   Tt_mur.Finalize();
   if (saveMat (Tt_mur,"Tt_mur_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

   // instance with ko^2*Re(er)

   ParBilinearForm Tt_eps_re=ParBilinearForm(fespace_ND);
   Tt_eps_re.AddDomainIntegrator(new VectorFEMassIntegrator (*ko2Re_e));
   Tt_eps_re.Assemble();
   Tt_eps_re.Finalize();
   if (saveMat (Tt_eps_re,"Tt_eps_re_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

   // instance with ko^2*Im(er)

   ParBilinearForm Tt_eps_im=ParBilinearForm(fespace_ND);
   Tt_eps_im.AddDomainIntegrator(new VectorFEMassIntegrator (*ko2Im_e));
   Tt_eps_im.Assemble();
   Tt_eps_im.Finalize();
   if (saveMat (Tt_eps_im,"Tt_eps_im_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

   // G

   ParMixedBilinearForm G=ParMixedBilinearForm(fespace_H1,fespace_ND);
   G.AddDomainIntegrator(new MixedVectorGradientIntegrator(*Inv_mu));
   G.Assemble();
   G.Finalize();
   if (saveMat (G,"G_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

   // GT - transpose of G

   //ToDo - put back to just being a transpose of G?
   ParMixedBilinearForm GT=ParMixedBilinearForm(fespace_H1,fespace_ND);
   GT.AddDomainIntegrator(new MixedVectorGradientIntegrator(*Inv_mu));
   GT.Assemble();
   GT.Finalize();
   if (saveMat (GT,"GT_mat",temporaryDirectory,projData->project_name,true)) {*error=true; return;}

   // Sz

   ParBilinearForm Sz=ParBilinearForm(fespace_H1);
   Sz.AddDomainIntegrator(new DiffusionIntegrator (*Inv_mu));
   Sz.Assemble();
   Sz.Finalize();
   if (saveMat (Sz,"Sz_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

   // Tz

   // instance with ko^2*Re(er)

   ParBilinearForm Tz_eps_re=ParBilinearForm(fespace_H1);
   Tz_eps_re.AddDomainIntegrator(new MassIntegrator (*ko2Re_e));
   Tz_eps_re.Assemble();
   Tz_eps_re.Finalize();
   if (saveMat (Tz_eps_re,"Tz_eps_re_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

   // instance with ko^2*Im(er)

   ParBilinearForm Tz_eps_im=ParBilinearForm(fespace_H1);
   Tz_eps_im.AddDomainIntegrator(new MassIntegrator (*ko2Im_e));
   Tz_eps_im.Assemble();
   Tz_eps_im.Finalize();
   if (saveMat (Tz_eps_im,"Tz_eps_im_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

   // St

   ParBilinearForm St=ParBilinearForm(fespace_ND);
   St.AddDomainIntegrator(new CurlCurlIntegrator (*Inv_mu));
   St.Assemble();
   St.Finalize();
   if (saveMat (St,"St_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

   //*****************************************************************************************************
   // matrices for the solution of the H-field
   //*****************************************************************************************************

   // Mt

   ParBilinearForm Mt=ParBilinearForm(fespace_ND);
   Mt.AddDomainIntegrator(new VectorFEMassIntegrator(*w_mu));
   Mt.Assemble();
   Mt.Finalize();
   if (saveMat (Mt,"Mt_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

   // for cross product with zHat
   DenseMatrix *m=new DenseMatrix(2,2);
   m->Elem(0,0)=0;
   m->Elem(0,1)=-1;
   m->Elem(1,0)=1;
   m->Elem(1,1)=0;
   MatrixCoefficient *mmat=new MatrixConstantCoefficient (*m);

   // Cz
   ParMixedBilinearForm Cz=ParMixedBilinearForm(fespace_H1,fespace_ND);
   Cz.AddDomainIntegrator(new MixedVectorGradientIntegrator(*mmat));
   Cz.Assemble();
   Cz.Finalize();
   if (saveMat (Cz,"Cz_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

   // Zt

   ParBilinearForm Zt=ParBilinearForm(fespace_ND);
   Zt.AddDomainIntegrator(new VectorFEMassIntegrator(*mmat));
   Zt.Assemble();
   Zt.Finalize();
   if (saveMat (Zt,"Zt_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

   // clean up for zHat x vector
   delete mmat;
   delete m;

   // Mz

   ParBilinearForm Mz=ParBilinearForm(fespace_H1);
   Mz.AddDomainIntegrator(new MassIntegrator(*w_mu));
   Mz.Assemble();
   Mz.Finalize();
   if (saveMat (Mz,"Mz_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

   // Ct

   ParMixedBilinearForm Ct=ParMixedBilinearForm(fespace_ND,fespace_H1);
   Ct.AddDomainIntegrator(new MixedScalarCurlIntegrator());
   Ct.Assemble();
   Ct.Finalize();
   if (saveMat (Ct,"Ct_mat",temporaryDirectory,projData->project_name,false)) {*error=true; return;}

}

// for Et
PetscInt* fem2D::get_ess_tdof_ND(BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase)
{
   int i;
   PetscInt *ess_tdof;

   Array<int> ess_tdof_list_ND;

   // set all to PEC
   // surface impedances are computed with PEC fields, then a perturbational calculation is used to get the loss
   border_attributes=1;

   // loop through the borders to find and eliminate the PMC boundaries
   long unsigned int j=0;
   while (j < borderDatabase->get_size()) {
      Border *border=borderDatabase->get_border(j);
      if (border->has_boundary()) {
         if (boundaryDatabase->get_boundary(border->get_boundary().boundary)->is_perfect_magnetic_conductor()) {
            border_attributes[border->get_global_attribute()-1]=0;  // offset down by 1 to align with MFEM convention
         }
      }
      j++;
   }

   // Get dofs for which border_attributes == 1.
   // Zeroing out the row and column provides for a PEC boundary.
   // The natural boundary condition is PMC, so not zeroing out the row and column provides for a PMC boundary.
   fespace_ND->GetEssentialTrueDofs(border_attributes, ess_tdof_list_ND);
   ess_tdof_size_ND=ess_tdof_list_ND.Size();

   // convert to a PetscInt array for passing to the C-language eigensolve

   PetscMalloc(ess_tdof_size_ND*sizeof(PetscInt),&ess_tdof);
   HYPRE_BigInt *offset=fespace_ND->GetTrueDofOffsets();

   i=0;
   while (i < ess_tdof_size_ND) {
      ess_tdof[i]=ess_tdof_list_ND[i]+offset[0];
      i++;
   }

   return ess_tdof;
}

// for Ez
PetscInt* fem2D::get_ess_tdof_H1(BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase)
{
   int i;
   PetscInt *ess_tdof;

   Array<int> ess_tdof_list_H1;

   // set all to PEC
   // surface impedances are computed with PEC fields, then a perturbational calculation is used to get the loss
   border_attributes=1;

   // loop through the borders to find and eliminate the PMC boundaries
   long unsigned int j=0;
   while (j < borderDatabase->get_size()) {
      Border *border=borderDatabase->get_border(j);
      if (border->has_boundary()) {
         if (boundaryDatabase->get_boundary(border->get_boundary().boundary)->is_perfect_magnetic_conductor()) {
            border_attributes[border->get_global_attribute()-1]=0;  // offset down by 1 to align with MFEM convention
         }
      }
      j++;
   }

   // Get dofs for which border_attributes == 1.
   // Zeroing out the row and column provides for a PEC boundary.
   // The natural boundary condition is PMC, so not zeroing out the row and column provides for a PMC boundary.
   fespace_H1->GetEssentialTrueDofs(border_attributes, ess_tdof_list_H1);
   ess_tdof_size_H1=ess_tdof_list_H1.Size();

   // convert to a PetscInt array for passing to the C-language eigensolve

   PetscMalloc(ess_tdof_size_H1*sizeof(PetscInt),&ess_tdof);
   HYPRE_BigInt *offset=fespace_H1->GetTrueDofOffsets();

   i=0;
   while (i < ess_tdof_size_H1) {
      ess_tdof[i]=ess_tdof_list_H1[i]+offset[0];
      i++;
   }

   return ess_tdof;
}

bool fem2D::buildFields (double *alpha, double *beta)
{
   return modeDatabase.buildFields(this,alpha,beta);
}

bool fem2D::saveTiTv ()
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (Ti == nullptr && Tv == nullptr) return 0;

   int n=projData->solution_modes;

   if (rank == 0) {
      stringstream ss;
      ss << temporaryDirectory << "/TiTv.dat";

      ofstream out;
      out.open(ss.str().c_str(),ofstream::out);
      if (out.is_open()) {

         if (Ti) {
            out << "Ti:" << endl;
            out << "   " << n << endl;
            int i=0;
            while (i < n) {
               int j=0;
               while (j < n) {
                  out << "   " << i << "," << j << "," << matrixGetRealValue(Ti,i+j*n) << "," << matrixGetImagValue(Ti,i+j*n) << endl;
                  j++;
               }
               i++;
            }
         }

         if (Tv) {
            out << "Tv:" << endl;
            out << "   " << n << endl;
            int i=0;
            while (i < n) {
               int j=0;
               while (j < n) {
                  out << "   " << i << "," << j << "," << matrixGetRealValue(Tv,i+j*n) << "," << matrixGetImagValue(Tv,i+j*n) << endl;
                  j++;
               }
               i++;
            }
         }

         out.close();
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2134: Unable to open file \"%s\" for writing.\n",ss.str().c_str());
         return 1;
      }
   }

   return 0;
}

void fem2D::calculateVandI (BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase)
{
   modeDatabase.calculateVandI(this,boundaryDatabase,borderDatabase);
}

bool fem2D::calculateModalVandI ()
{
   if (modeDatabase.calculateModalVandI(this)) return true;
   return false;
}

void fem2D::calculateImpedance (BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase)
{
   modeDatabase.calculateImpedance(this,boundaryDatabase,borderDatabase);
}

bool fem2D::calculatePerturbationalLoss(BoundaryDatabase *boundaryDatabase, BorderDatabase *borderDatabase, MaterialDatabase *materialDatabase)
{
   if (modeDatabase.calculatePerturbationalLoss(this,boundaryDatabase,borderDatabase,materialDatabase)) return true;
   return false;
}

void fem2D::calculateFieldPoints (FieldPointDatabase *fieldPointDatabase)
{
   modeDatabase.calculateFieldPoints(this,fieldPointDatabase);
}

void fem2D::saveParaView()
{
   modeDatabase.saveParaView(this);
}

bool fem2D::ZZrefineMeshes (ConvergenceDatabase *convergenceDatabase)
{
   startingMeshSize=GetGlobalNE(pmesh);

   // see how many modes will be refined and calculate a scale factor
   if (projData->refinement_refine_converged_modes) meshScale=projData->solution_active_mode_count;
   else {
      meshScale=projData->solution_active_mode_count-convergenceDatabase->is_converged_count();
      if (meshScale == 0) meshScale=1;  // should not happen
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"      Refining mesh ...\n");

   return modeDatabase.ZZrefineMeshes(this,convergenceDatabase);
}

void fem2D::writeInitialGuess ()
{
   modeDatabase.writeInitialGuess(this);
}

Result* fem2D::updateResults(ResultDatabase *resultDatabase, ConvergenceDatabase *convergenceDatabase,
                          chrono::steady_clock::time_point solve_start_time, chrono::steady_clock::time_point solve_end_time)
{
   Result *result=new Result();

   result->set_active();
   result->set_iteration(iteration);
   result->set_frequency(frequency);
   result->set_modeCount(projData->solution_active_mode_count);
   result->set_modalImpedanceCalculation(is_modal_impedance(projData->solution_impedance_calculation));

   long unsigned int i=0;
   while (i < modeDatabase.get_size()) {
      Mode *mode=modeDatabase.get_mode(i);
      if (mode) {
         result->push_gamma(complex<double>(mode->get_alpha(),mode->get_beta()));
         result->push_alpha_perturbation(mode->get_perturbationalLoss());
         result->push_Pz(mode->get_Pzavg());
         result->push_V(mode->get_mode_voltage());
         result->push_I(mode->get_mode_current());
         if (strcmp(projData->solution_impedance_definition,"VI") == 0) result->push_Z(mode->get_Zvi());
         if (strcmp(projData->solution_impedance_definition,"PV") == 0) result->push_Z(mode->get_Zpv());
         if (strcmp(projData->solution_impedance_definition,"PI") == 0) result->push_Z(mode->get_Zpi());
         if (strcmp(projData->solution_impedance_definition,"none") == 0) result->push_Z(complex<double>(DBL_MAX,DBL_MAX));
      }
      i++;
   }

   run_statistics *run_stats=new run_statistics;
   run_stats->set_error(0);
   run_stats->set_meshSize(GetGlobalNE(pmesh));
   run_stats->set_matrixSize(t_size+z_size);
   run_stats->set_start_time(solve_start_time);
   run_stats->set_end_solve_time(solve_end_time);
   run_stats->set_end_refine_time(solve_end_time);  // update later if there is refinement
   run_stats->set_converged(false);

   result->set_run_stats(run_stats);

   resultDatabase->push(result);

   // convergence
   i=0;
   while (i < modeDatabase.get_size()) {
      Mode *mode=modeDatabase.get_mode(i);
      if (mode) {
         complex<double> Z;
         if (strcmp(projData->solution_impedance_definition,"VI") == 0) Z=mode->get_Zvi();
         if (strcmp(projData->solution_impedance_definition,"PV") == 0) Z=mode->get_Zpv();
         if (strcmp(projData->solution_impedance_definition,"PI") == 0) Z=mode->get_Zpi();
         convergenceDatabase->push(i,Z,mode->get_alpha(),mode->get_beta(),
                                mode->get_perturbationalLoss(),real(mode->get_Pzavg()));
      }
      i++;
   }

   return result;
}

void fem2D::dumpDof2DData()
{
   int count=0;

   cout << "OpenParEM2D Dof data from 2D mesh:" << endl;

   // loop over elements
   int i=0;
   while (i < fespace_ND->GetNE()) {
      cout << "   element " << i << ":" << endl;

      // coordinates

      Array<int> vert;
      pmesh->GetElementVertices(i,vert);
      cout << "      vertex indices and coordinates: " << endl;
      int j=0;
      while (j < vert.Size()) {
         double *coords=pmesh->GetVertex(vert[j]);
         cout << "         " << vert[j] << ": (" << coords[0] << "," << coords[1] << ")" << endl;
         j++;
      }

      // Vdofs

      Array<int> vdofs;
      fespace_ND->GetElementVDofs(i,vdofs);

      cout << "      ElementVDofs:" << endl;
      j=0;
      while (j < vdofs.Size()) {
         cout << "         " << vdofs[j] << endl;
         count++;
         j++;
      }

      i++;
   }

   cout << "   dof count=" << count << endl;
}

void fem2D::printBoundaryAttributes ()
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   int i=0;
   while (i < pmesh->GetNBE()) {
      Array<int> vertices;
      pmesh->GetBdrElementVertices(i,vertices);
      double *vertex0,*vertex1;
      vertex0=pmesh->GetVertex(vertices[0]);
      vertex1=pmesh->GetVertex(vertices[1]);
      cout << rank << ": element=" << i << "  attribute=" << pmesh->GetBdrAttribute(i) 
                   << "  boundary=(" << vertex0[0] << "," << vertex0[1] << ")-("
                                     << vertex1[0] << "," << vertex1[1] << ")" << endl;
      i++;
   }
}

fem2D::~fem2D()
{
   if (fespace_ND) {delete fespace_ND; fespace_ND=nullptr;}
   if (fec_ND) {delete fec_ND; fec_ND=nullptr;}

   if (fespace_H1) {delete fespace_H1; fespace_H1=nullptr;}
   if (fec_H1) {delete fec_H1; fec_H1=nullptr;}

   if (fespace_L2) {delete fespace_L2; fespace_L2=nullptr;}
   if (fec_L2) {delete fec_L2; fec_L2=nullptr;}

   if (Ti) {free(Ti); Ti=nullptr;}
   if (Tv) {free(Tv); Tv=nullptr;}
}

