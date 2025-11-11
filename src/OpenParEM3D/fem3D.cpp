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

#include "fem3D.hpp"

extern "C" PetscErrorCode eliminatePEC (Mat *, PetscInt, PetscInt *);
extern "C" PetscErrorCode solveComplexLinearSystem (const char *, struct projectData *, PetscMPIInt,
    PetscInt, PetscInt *, Mat *, Vec *, PetscInt *, PetscReal *, PetscInt *);
extern "C" PetscErrorCode buildHb (Mat *, Vec *, Vec *);
extern "C" PetscErrorCode solveHfield (const char *, struct projectData *, PetscMPIInt, Mat *, Vec *, Vec*, PetscReal *, PetscInt *);
extern "C" PetscErrorCode hypre_ParCSRMatrixToMat(hypre_ParCSRMatrix *, Mat *, PetscInt, int, int, int);
extern "C" PetscErrorCode hypre_getSparseWidth (hypre_ParCSRMatrix *, PetscInt *);

int getGlobalNE (ParMesh *pmesh)
{
   int localNE=pmesh->GetNE();
   int globalNE=0;

   MPI_Allreduce (&localNE,&globalNE,1,MPI_INT,MPI_SUM,PETSC_COMM_WORLD);

   return globalNE;
}

int getGlobalNBE (ParMesh *pmesh)
{
   int localNBE=pmesh->GetNBE();
   int globalNBE=0;

   MPI_Allreduce (&localNBE,&globalNBE,1,MPI_INT,MPI_SUM,PETSC_COMM_WORLD);

   return globalNBE;
}

//--------------------------------------------------------------------------------------------------------------------
// fem3D
//--------------------------------------------------------------------------------------------------------------------

void fem3D::set_data (ParMesh **pmesh_, struct projectData *projData_, double frequency_)
{
   pmesh=pmesh_;
   projData=projData_;
   frequency=frequency_;
   radius=0;
   radiation_beta=0;
   wave_impedance=0;
}

void fem3D::build_fe_spaces ()
{
   fec_ND=new ND_FECollection(projData->fem_order,(*pmesh)->Dimension());
   fespace_ND=new ParFiniteElementSpace(*pmesh,fec_ND);

   fec_RT=new RT_FECollection(projData->fem_order,(*pmesh)->Dimension());
   fespace_RT=new ParFiniteElementSpace(*pmesh,fec_RT);

   fec_H1=new H1_FECollection(projData->fem_order,(*pmesh)->Dimension());
   fespace_H1=new ParFiniteElementSpace(*pmesh,fec_H1);

   fec_L2=new L2_FECollection(projData->fem_order,(*pmesh)->Dimension(),BasisType::GaussLobatto);
   fespace_L2=new ParFiniteElementSpace(*pmesh,fec_L2);
}

void fem3D::build_PEC_dofs ()
{
   Array<int> ess_bdr_PEC;
   ess_bdr_PEC.SetSize((*pmesh)->bdr_attributes.Max());
   ess_bdr_PEC=0;       // disable all
   ess_bdr_PEC[0]=1;    // enable PEC

   Array<int> ess_dofs_ND;
   fespace_ND->GetEssentialTrueDofs(ess_bdr_PEC, ess_dofs_ND);
   HYPRE_BigInt *offset=fespace_ND->GetTrueDofOffsets();

   nPEC=ess_dofs_ND.Size();
   PetscMalloc(nPEC*sizeof(PetscInt),&PEC);

   int i=0;
   while (i < ess_dofs_ND.Size()) {
      PEC[i]=ess_dofs_ND[i]+offset[0];
      i++;
   }
}

// A is a complex matrix that is matrixSize x sparseWidth.
// Building A costs 50% more memory ReA or ReB must be allocated at the same time as A.
bool fem3D::build_A (BoundaryDatabase *boundaryDatabase, MaterialDatabase *materialDatabase, double temperature,
                     PWConstCoefficient *Inv_mur, PWConstCoefficient *neg_ko2_Re_er, PWConstCoefficient *neg_ko2_Im_er,
                     bool solution_check_homogeneous, string indent)
{
   vector<Array<int> *> borderAttributeList;
   vector<ConstantCoefficient *> ReC1ConstList,ImC1ConstList,ReC2ConstList,ImC2ConstList,ZconstList;

   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   // real

   ParBilinearForm *pmblfReA=new ParBilinearForm(fespace_ND);
   pmblfReA->AddDomainIntegrator(new CurlCurlIntegrator (*Inv_mur));
   pmblfReA->AddDomainIntegrator(new VectorFEMassIntegrator (*neg_ko2_Re_er));
   if (boundaryDatabase->addPortIntegrators(*pmesh,pmblfReA,Inv_mur,neg_ko2_Re_er,neg_ko2_Im_er,borderAttributeList,
                                            ReC1ConstList,ImC1ConstList,ReC2ConstList,ImC2ConstList,
                                            true,drivingSet,solution_check_homogeneous,indent)) return true;
   pmblfReA->Assemble();
   pmblfReA->Finalize();
   HypreParMatrix *ReA;
   try {
      ReA=pmblfReA->ParallelAssemble();
   } catch (const std::exception& e) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3009: Out of memory in assembling finite-element matrix.\n");
      return true;
   }

   long unsigned int i=0;
   while (i < borderAttributeList.size()) {
      if (borderAttributeList[i]) delete borderAttributeList[i];
      i++;
   }
   borderAttributeList.clear();

   i=0;
   while (i < ReC1ConstList.size()) {
      if (ReC1ConstList[i]) delete ReC1ConstList[i];
      if (ImC1ConstList[i]) delete ImC1ConstList[i];
      if (ReC2ConstList[i]) delete ReC2ConstList[i];
      if (ImC2ConstList[i]) delete ImC2ConstList[i];
      i++;
   }
   ReC1ConstList.clear();
   ImC1ConstList.clear();
   ReC2ConstList.clear();
   ImC2ConstList.clear();

   delete pmblfReA;
   hypre_getSparseWidth((hypre_ParCSRMatrix *) *ReA,&sparseWidth);
   if (hypre_ParCSRMatrixToMat((hypre_ParCSRMatrix *) *ReA, &A, sparseWidth, 1, 0, 0)) return true;
   delete ReA;

   // imag

   ParBilinearForm *pmblfImA=new ParBilinearForm(fespace_ND);
   pmblfImA->AddDomainIntegrator(new VectorFEMassIntegrator (*neg_ko2_Im_er));
   if (boundaryDatabase->addPortIntegrators(*pmesh,pmblfImA,Inv_mur,neg_ko2_Re_er,neg_ko2_Im_er,
                                            borderAttributeList,ReC1ConstList,ImC1ConstList,ReC2ConstList,ImC2ConstList,
                                            false,drivingSet,solution_check_homogeneous,indent)) return true;
   boundaryDatabase->addImpedanceIntegrators(frequency,temperature,*pmesh,pmblfImA,materialDatabase,
                                             borderAttributeList,ZconstList,false);
   pmblfImA->Assemble(); 
   pmblfImA->Finalize();
   HypreParMatrix *ImA;
   try {
      ImA=pmblfImA->ParallelAssemble();
   } catch (const std::exception& e) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3009: Out of memory in assembling finite-element matrix.\n");
      return true;
   }

   i=0;
   while (i < borderAttributeList.size()) {
      if (borderAttributeList[i]) delete borderAttributeList[i];
      i++;
   }

   i=0;
   while (i < ReC1ConstList.size()) {
      if (ReC1ConstList[i]) delete ReC1ConstList[i];
      if (ImC1ConstList[i]) delete ImC1ConstList[i];
      if (ReC2ConstList[i]) delete ReC2ConstList[i];
      if (ImC2ConstList[i]) delete ImC2ConstList[i];
      i++;
   }

   i=0;
   while (i < ZconstList.size()) {
      if (ZconstList[i]) delete ZconstList[i];
      i++;
   }

   delete pmblfImA;
   if (hypre_ParCSRMatrixToMat((hypre_ParCSRMatrix *) *ImA, &A, sparseWidth, 0, 1, 1)) return true;
   delete ImA;

   return false;
}

void fem3D::build_P ()
{
   ConstantCoefficient one(1.0);
   ParBilinearForm *pblfReP=new ParBilinearForm(fespace_ND);
   pblfReP->AddDomainIntegrator(new VectorFEMassIntegrator(one));
   pblfReP->Assemble();
   pblfReP->Finalize();
   HypreParMatrix *ReP=pblfReP->ParallelAssemble();
   delete pblfReP;

   if (hypre_ParCSRMatrixToMat((hypre_ParCSRMatrix *) *ReP, &P, sparseWidth, 1, 0, 1)) {
      cout << "ASSERT: fem3D::build_P failed to build Mat P." << endl;
   }

   delete ReP;
}

void fem3D::build_Q (PWConstCoefficient *Inv_w_mu)
{
   ParBilinearForm *pmblfReQ=new ParBilinearForm(fespace_ND);
   pmblfReQ->AddDomainIntegrator(new MixedVectorCurlIntegrator (*Inv_w_mu));
   pmblfReQ->Assemble();
   pmblfReQ->Finalize();

   HypreParMatrix *ReQ=pmblfReQ->ParallelAssemble();
   delete pmblfReQ;

   // anticipate solving PH=jQE and put Q in the imaginary position
   if (hypre_ParCSRMatrixToMat((hypre_ParCSRMatrix *) *ReQ, &Q, sparseWidth, 1, 1, 1)) {
      cout << "ASSERT: fem3D::build_Q failed to build Mat Q." << endl;
   }

   delete ReQ;
}

void fem3D::build_R (PWConstCoefficient *ReInv_w_er, PWConstCoefficient *ImInv_w_er)
{
   // real

   ParBilinearForm *pmblfReR=new ParBilinearForm(fespace_ND);
   pmblfReR->AddDomainIntegrator(new MixedVectorCurlIntegrator (*ReInv_w_er));
   pmblfReR->Assemble();
   pmblfReR->Finalize();
   HypreParMatrix *ReR=pmblfReR->ParallelAssemble();
   delete pmblfReR;

   // anticipate solving PE=-jRH and put ReR in the imaginary position as a negative
   (*ReR)*=-1.0;
   if (hypre_ParCSRMatrixToMat((hypre_ParCSRMatrix *) *ReR, &R, sparseWidth, 1, 1, 0)) {
      cout << "ASSERT: fem3D::build_R failed to build Mat ReR." << endl;
   }

   delete ReR;

   // imag

   ParBilinearForm *pmblfImR=new ParBilinearForm(fespace_ND);
   pmblfImR->AddDomainIntegrator(new MixedVectorCurlIntegrator (*ImInv_w_er));
   pmblfImR->Assemble();
   pmblfImR->Finalize();
   HypreParMatrix *ImR=pmblfImR->ParallelAssemble();
   delete pmblfImR;

   // anticipate solving PE=-jRH and put ImR in the real position
   if (hypre_ParCSRMatrixToMat((hypre_ParCSRMatrix *) *ImR, &R, sparseWidth, 0, 0, 1)) {
      cout << "ASSERT: fem3D::build_R failed to build Mat ImR." << endl;
   }

   delete ImR;
}

PetscErrorCode fem3D::build_X()
{
   PetscErrorCode ierr=0;
   ierr=MatCreateVecs(A,&X,NULL); if (ierr) return ierr;
   ierr=VecZeroEntries(X);
   return ierr;
}

PetscErrorCode fem3D::build_x()
{
   PetscErrorCode ierr=0;
   ierr=MatCreateVecs(A,&x,NULL); if (ierr) return ierr;
   ierr=VecZeroEntries(x);
   return ierr;
}

void fem3D::build_grids()
{
   if (gridReE) delete gridReE;
   if (gridImE) delete gridImE;
   if (gridReH) delete gridReH;
   if (gridImH) delete gridImH;

   if (gridReExH) delete gridReExH;
   if (gridImExH) delete gridImExH;

//   if (gridReEz) delete gridReEz;
//   if (gridImEz) delete gridImEz;
//   if (gridReHz) delete gridReHz;
//   if (gridImHz) delete gridImHz;

   gridReE=new ParGridFunction(fespace_ND);
   gridImE=new ParGridFunction(fespace_ND);
   gridReH=new ParGridFunction(fespace_ND);
   gridImH=new ParGridFunction(fespace_ND);

   gridReExH=new ParGridFunction(fespace_RT);
   gridImExH=new ParGridFunction(fespace_RT);

//   gridReEz=new ParGridFunction(fespace_L2);
//   gridImEz=new ParGridFunction(fespace_L2);
//   gridReHz=new ParGridFunction(fespace_L2);
//   gridImHz=new ParGridFunction(fespace_L2);
}

// fill out the dofs on the ports
void fem3D::build_portEssTdofLists(BoundaryDatabase *boundaryDatabase)
{
   boundaryDatabase->build_portEssTdofLists(fespace_ND,*pmesh);
}

PetscErrorCode fem3D::build_Xdofs()
{
   PetscErrorCode ierr=0;
   ierr=MatCreateVecs(A,&Xdofs,NULL);  if (ierr) return ierr;
   ierr=VecZeroEntries(Xdofs);
   return ierr;
}

//xxx
// x uses a local distribution on A (which is destroyed before here)
// e_re and e_im must use a local distribution on fespace_ND
// The two local spaces do not exactly line up, so they must be realigned from A to fespace_ND for e_re and e_im.
void fem3D::build_e_re_e_im ()
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // local distribution of fespace_ND - for e_re and e_im

   HYPRE_BigInt *fespace_ND_offsets=fespace_ND->GetTrueDofOffsets();

   if (e_re) delete e_re;
   if (e_im) delete e_im;

   e_re=new Vector(fespace_ND_offsets[1]-fespace_ND_offsets[0]);
   e_im=new Vector(fespace_ND_offsets[1]-fespace_ND_offsets[0]);

   // local distribution of A - for x

   PetscInt i,low,high;
   VecGetOwnershipRange(x,&low,&high);
   int local_x_size=high-low;

   PetscInt *ixvals;
   PetscMalloc(local_x_size*sizeof(PetscInt),&ixvals);

   i=low;
   while (i < high) {
      ixvals[i-low]=i;
      i++;
   }

   PetscScalar *local_x_vals;
   PetscMalloc(local_x_size*sizeof(PetscScalar),&local_x_vals);

   VecGetValues(x,local_x_size,ixvals,local_x_vals);

   // convert to two double arrays

   double *local_x_re=(double *)malloc(local_x_size*sizeof(double));
   double *local_x_im=(double *)malloc(local_x_size*sizeof(double));

   i=0;
   while (i < local_x_size) {
      local_x_re[i]=real(local_x_vals[i]);
      local_x_im[i]=imag(local_x_vals[i]);
      i++;
   }

   PetscFree(ixvals);
   PetscFree(local_x_vals);

   // global x size

   int global_x_size=0;
   MPI_Allreduce (&local_x_size,&global_x_size,1,MPI_INT,MPI_SUM,PETSC_COMM_WORLD);

   // collect x at rank 0
   double *global_x_re,*global_x_im;

   if (rank == 0) {

      // global x
      global_x_re=(double *)malloc(global_x_size*sizeof(double));
      global_x_im=(double *)malloc(global_x_size*sizeof(double));

      // local
      int i=0;
      while (i < local_x_size) {
         global_x_re[low+i]=local_x_re[i];
         global_x_im[low+i]=local_x_im[i];
         i++;
      }

      // collected
      i=1;
      while (i < size) {
         int transfer_size=0;
         MPI_Recv(&transfer_size,1,MPI_INT,i,10000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int offset=0;
         MPI_Recv(&offset,1,MPI_INT,i,10001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         // real

         double *transfer=(double *)malloc(transfer_size*sizeof(double));
         MPI_Recv(transfer,transfer_size,MPI_DOUBLE,i,10002,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int k=0;
         while (k < transfer_size) {
            global_x_re[offset+k]=transfer[k];
            k++;
         }

         // imag

         MPI_Recv(transfer,transfer_size,MPI_DOUBLE,i,10003,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         k=0;
         while (k < transfer_size) {
            global_x_im[offset+k]=transfer[k];
            k++;
         }

         if (transfer) free(transfer);

         i++;
      }
   } else {
      MPI_Send(&local_x_size,1,MPI_INT,0,10000,PETSC_COMM_WORLD);
      MPI_Send(&low,1,MPI_INT,0,10001,PETSC_COMM_WORLD);
      MPI_Send(local_x_re,local_x_size,MPI_DOUBLE,0,10002,PETSC_COMM_WORLD);
      MPI_Send(local_x_im,local_x_size,MPI_DOUBLE,0,10003,PETSC_COMM_WORLD);
   }

   // collect the new partitioning data
   std::vector<int> newLow,newHigh;
   if (rank == 0) {
      newLow.push_back(fespace_ND_offsets[0]);
      newHigh.push_back(fespace_ND_offsets[1]);

      int i=1;
      while (i < size) {
         int lowPart,highPart;
         MPI_Recv(&lowPart,1,MPI_INT,i,20010,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&highPart,1,MPI_INT,i,20011,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         newLow.push_back(lowPart);
         newHigh.push_back(highPart);
         i++;
      }
   } else {
      int lowPart=fespace_ND_offsets[0];
      int highPart=fespace_ND_offsets[1];
      MPI_Send(&lowPart,1,MPI_INT,0,20010,PETSC_COMM_WORLD);
      MPI_Send(&highPart,1,MPI_INT,0,20011,PETSC_COMM_WORLD);
   }

   // send global x to all ranks

   if (rank == 0) {
      int i=newLow[0];
      while (i < newHigh[0]) {
         e_re->Elem(i-newLow[0])=global_x_re[i];
         e_im->Elem(i-newLow[0])=global_x_im[i];
         i++;
      }

      i=1;
      while (i < size) {
         MPI_Send(global_x_re+newLow[i],newHigh[i]-newLow[i],MPI_DOUBLE,i,20000,PETSC_COMM_WORLD);
         MPI_Send(global_x_im+newLow[i],newHigh[i]-newLow[i],MPI_DOUBLE,i,20001,PETSC_COMM_WORLD);
         i++;
      }

   } else {

      double *transfer=(double *)malloc((fespace_ND_offsets[1]-fespace_ND_offsets[0])*sizeof(double));

      // real

      MPI_Recv(transfer,fespace_ND_offsets[1]-fespace_ND_offsets[0],MPI_DOUBLE,0,20000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      int i=0;
      while (i < fespace_ND_offsets[1]-fespace_ND_offsets[0]) {
         e_re->Elem(i)=transfer[i];
         i++;
      }

      // imag

      MPI_Recv(transfer,fespace_ND_offsets[1]-fespace_ND_offsets[0],MPI_DOUBLE,0,20001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      i=0;
      while (i < fespace_ND_offsets[1]-fespace_ND_offsets[0]) {
         e_im->Elem(i)=transfer[i];
         i++;
      }

      if (transfer) free(transfer);
   }

   // clean up
   if (rank == 0) {
      if (local_x_re) free(local_x_re);
      if (local_x_im) free(local_x_im);
      if (global_x_re) free(global_x_re);
      if (global_x_im) free(global_x_im);
   }
}

//xxx
/* avoid sending global vector to all ranks
// x uses a local distribution on A (which is destroyed before here)
// e_re and e_im must use a local distribution on fespace_ND
// The two local spaces do not exactly line up, so they must be realigned from A to fespace_ND for e_re and e_im.
void fem3D::build_e_re_e_im ()
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // local distribution of fespace_ND - for e_re and e_im

   HYPRE_BigInt *fespace_ND_offsets=fespace_ND->GetTrueDofOffsets();

   if (e_re) delete e_re;
   if (e_im) delete e_im;

   e_re=new Vector(fespace_ND_offsets[1]-fespace_ND_offsets[0]);
   e_im=new Vector(fespace_ND_offsets[1]-fespace_ND_offsets[0]);

   // local distribution of A - for x

   PetscInt i,low,high;
   VecGetOwnershipRange(x,&low,&high);
   int local_x_size=high-low;

   PetscInt *ixvals;
   PetscMalloc(local_x_size*sizeof(PetscInt),&ixvals);

   i=low;
   while (i < high) {
      ixvals[i-low]=i;
      i++;
   }

   PetscScalar *local_x_vals;
   PetscMalloc(local_x_size*sizeof(PetscScalar),&local_x_vals);

   VecGetValues(x,local_x_size,ixvals,local_x_vals);

   // global x size

   int global_x_size=0;
   MPI_Allreduce (&local_x_size,&global_x_size,1,MPI_INT,MPI_SUM,PETSC_COMM_WORLD);

   // collect x at rank 0
   PetscScalar *global_x_vals;

   if (rank == 0) {

      // global x
      PetscMalloc(global_x_size*sizeof(PetscScalar),&global_x_vals);

      // local
      int i=0;
      while (i < local_x_size) {
         global_x_vals[low+i]=local_x_vals[i];
         i++;
      }

      // collected
      i=1;
      while (i < size) {
         int transfer_size=0;
         MPI_Recv(&transfer_size,1,MPI_INT,i,10000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int k=0;
         while (k < transfer_size) {
            int location=0;
            double transfer_real=0;
            double transfer_imag=0;
            MPI_Recv(&location,1,MPI_INT,i,10001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(&transfer_real,1,MPI_DOUBLE,i,10002,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(&transfer_imag,1,MPI_DOUBLE,i,10003,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

            global_x_vals[location]=transfer_real+PETSC_i*transfer_imag;

            k++;
         }
         i++;
      }
   } else {
      MPI_Send(&local_x_size,1,MPI_INT,0,10000,PETSC_COMM_WORLD);

      int i=0;
      while (i < local_x_size) {
         int location=low+i;
         double transfer_real=real(local_x_vals[i]);
         double transfer_imag=imag(local_x_vals[i]);
         MPI_Send(&location,1,MPI_INT,0,10001,PETSC_COMM_WORLD);
         MPI_Send(&transfer_real,1,MPI_DOUBLE,0,10002,PETSC_COMM_WORLD);
         MPI_Send(&transfer_imag,1,MPI_DOUBLE,0,10003,PETSC_COMM_WORLD);
         i++;
      }
   }

   // collect the new partitioning data
   std::vector<int> newLow,newHigh;
   if (rank == 0) {
      newLow.push_back(fespace_ND_offsets[0]);
      newHigh.push_back(fespace_ND_offsets[1]);

      int i=1;
      while (i < size) {
         int lowPart,highPart;
         MPI_Recv(&lowPart,1,MPI_INT,i,20010,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&highPart,1,MPI_INT,i,20011,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         newLow.push_back(lowPart);
         newHigh.push_back(highPart);
         i++;
      }
   } else {
      int lowPart=fespace_ND_offsets[0];
      int highPart=fespace_ND_offsets[1];
      MPI_Send(&lowPart,1,MPI_INT,0,20010,PETSC_COMM_WORLD);
      MPI_Send(&highPart,1,MPI_INT,0,20011,PETSC_COMM_WORLD);
   }

   // send global x to all ranks

   if (rank == 0) {
      int i=newLow[0];
      while (i < newHigh[0]) {
         e_re->Elem(i-newLow[0])=real(global_x_vals[i]);
         e_im->Elem(i-newLow[0])=imag(global_x_vals[i]);
         i++;
      }

      i=1;
      while (i < size) {
         int k=newLow[i];
         while (k < newHigh[i]) {
            double transfer_real=real(global_x_vals[k]);
            double transfer_imag=imag(global_x_vals[k]);
            MPI_Send(&transfer_real,1,MPI_DOUBLE,i,20001,PETSC_COMM_WORLD);
            MPI_Send(&transfer_imag,1,MPI_DOUBLE,i,20002,PETSC_COMM_WORLD);
            k++;
         }
         i++;
      }
   } else {
      int k=fespace_ND_offsets[0];
      while (k < fespace_ND_offsets[1]) {
         double transfer_real=0;
         double transfer_imag=0;
         MPI_Recv(&transfer_real,1,MPI_DOUBLE,0,20001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&transfer_imag,1,MPI_DOUBLE,0,20002,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         e_re->Elem(k-fespace_ND_offsets[0])=transfer_real;
         e_im->Elem(k-fespace_ND_offsets[0])=transfer_imag;

         k++;
      }
   }

   // clean up
   PetscFree(ixvals);
   PetscFree(local_x_vals);
   if (rank == 0) PetscFree(global_x_vals);
}
*/

//xxx
/* original - very slow in MPICH
// x uses a local distribution on A (which is destroyed before here)
// e_re and e_im must use a local distribution on fespace_ND
// The two local spaces do not exactly line up, so they must be realigned from A to fespace_ND for e_re and e_im.
void fem3D::build_e_re_e_im ()
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // local distribution of fespace_ND - for e_re and e_im
   HYPRE_BigInt *fespace_ND_offsets=fespace_ND->GetTrueDofOffsets();
   int local_ND_size=fespace_ND_offsets[1]-fespace_ND_offsets[0];

   if (e_re) delete e_re;
   if (e_im) delete e_im;

   e_re=new Vector(local_ND_size);
   e_im=new Vector(local_ND_size);

   // local distribution of A - for x

   PetscInt i,low,high;
   VecGetOwnershipRange(x,&low,&high);
   int local_x_size=high-low;

   PetscInt *ixvals;
   PetscMalloc(local_x_size*sizeof(PetscInt),&ixvals);

   i=low;
   while (i < high) {
      ixvals[i-low]=i;
      i++;
   }

   PetscScalar *local_x_vals;
   PetscMalloc(local_x_size*sizeof(PetscScalar),&local_x_vals);

   VecGetValues(x,local_x_size,ixvals,local_x_vals);

   // global x

   int global_x_size=0;
   MPI_Allreduce (&local_x_size,&global_x_size,1,MPI_INT,MPI_SUM,PETSC_COMM_WORLD);

   PetscScalar *global_x_vals;
   PetscMalloc(global_x_size*sizeof(PetscScalar),&global_x_vals);

   // collect x at rank 0

   if (rank == 0) {

      // local
      int i=0;
      while (i < local_x_size) {
         global_x_vals[low+i]=local_x_vals[i];
         i++;
      }

      // collected
      i=1;
      while (i < size) {
         int transfer_size=0;
         MPI_Recv(&transfer_size,1,MPI_INT,i,10000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int k=0;
         while (k < transfer_size) {
            int location=0;
            double transfer_real=0;
            double transfer_imag=0;
            MPI_Recv(&location,1,MPI_INT,i,10001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(&transfer_real,1,MPI_DOUBLE,i,10002,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(&transfer_imag,1,MPI_DOUBLE,i,10003,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

            global_x_vals[location]=transfer_real+PETSC_i*transfer_imag;

            k++;
         }
         i++;
      }
   } else {
      MPI_Send(&local_x_size,1,MPI_INT,0,10000,PETSC_COMM_WORLD);

      int i=0;
      while (i < local_x_size) {
         int location=low+i;
         double transfer_real=real(local_x_vals[i]);
         double transfer_imag=imag(local_x_vals[i]);
         MPI_Send(&location,1,MPI_INT,0,10001,PETSC_COMM_WORLD);
         MPI_Send(&transfer_real,1,MPI_DOUBLE,0,10002,PETSC_COMM_WORLD);
         MPI_Send(&transfer_imag,1,MPI_DOUBLE,0,10003,PETSC_COMM_WORLD);
         i++;
      }
   }

   // send global x to all ranks

   if (rank == 0) {
      int i=1;
      while (i < size) {
         int k=0;
         while (k < global_x_size) {
            double transfer_real=real(global_x_vals[k]);
            double transfer_imag=imag(global_x_vals[k]);
            MPI_Send(&transfer_real,1,MPI_DOUBLE,i,20001,PETSC_COMM_WORLD);
            MPI_Send(&transfer_imag,1,MPI_DOUBLE,i,20002,PETSC_COMM_WORLD);
            k++;
         }
         i++;
      }
   } else {
      int k=0;
      while (k < global_x_size) {
         double transfer_real=0;
         double transfer_imag=0;
         MPI_Recv(&transfer_real,1,MPI_DOUBLE,0,20001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&transfer_imag,1,MPI_DOUBLE,0,20002,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         global_x_vals[k]=transfer_real+PETSC_i*transfer_imag;

         k++;
      }
   }

   // transfer data to e_re and e_im - completes the re-alignment

   int index=0;
   i=0;
   while (i < global_x_size) {
      if (i >= fespace_ND_offsets[0] && i < fespace_ND_offsets[1]) {
         e_re->Elem(index)=real(global_x_vals[i]);
         e_im->Elem(index)=imag(global_x_vals[i]);
         index++;
      }
      i++;
   }

   // clean up
   PetscFree(ixvals);
   PetscFree(local_x_vals);
   PetscFree(global_x_vals);
}
*/

void fem3D::buildEgrids (BoundaryDatabase *boundaryDatabase)
{
   gridReE->Distribute(*e_re);
   gridImE->Distribute(*e_im);
}

// see build_e_re_e_im for details
void fem3D::build_h_re_h_im (Vec *hdofs)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // local distribution of fespace_ND - for e_re and e_im

   HYPRE_BigInt *fespace_ND_offsets=fespace_ND->GetTrueDofOffsets();

   if (h_re) delete h_re;
   if (h_im) delete h_im;

   h_re=new Vector(fespace_ND_offsets[1]-fespace_ND_offsets[0]);
   h_im=new Vector(fespace_ND_offsets[1]-fespace_ND_offsets[0]);

   // local distribution of A - for hdofs

   PetscInt i,low,high;
   VecGetOwnershipRange(*hdofs,&low,&high);
   int local_hdof_size=high-low;

   PetscInt *ixvals;
   PetscMalloc(local_hdof_size*sizeof(PetscInt),&ixvals);

   i=low;
   while (i < high) {
      ixvals[i-low]=i;
      i++;
   }

   PetscScalar *local_hdof_vals;
   PetscMalloc(local_hdof_size*sizeof(PetscScalar),&local_hdof_vals);

   VecGetValues(*hdofs,local_hdof_size,ixvals,local_hdof_vals);

   // convert to two double arrays

   double *local_hdof_re=(double *)malloc(local_hdof_size*sizeof(double));
   double *local_hdof_im=(double *)malloc(local_hdof_size*sizeof(double));

   i=0;
   while (i < local_hdof_size) {
      local_hdof_re[i]=real(local_hdof_vals[i]);
      local_hdof_im[i]=imag(local_hdof_vals[i]);
      i++;
   }

   PetscFree(ixvals);
   PetscFree(local_hdof_vals);

   // global hdof size

   int global_hdof_size=0;
   MPI_Allreduce (&local_hdof_size,&global_hdof_size,1,MPI_INT,MPI_SUM,PETSC_COMM_WORLD);

   // collect hdof at rank 0
   double *global_hdof_re,*global_hdof_im;

   if (rank == 0) {

      // global hdof
      global_hdof_re=(double *)malloc(global_hdof_size*sizeof(double));
      global_hdof_im=(double *)malloc(global_hdof_size*sizeof(double));

      // local
      int i=0;
      while (i < local_hdof_size) {
         global_hdof_re[low+i]=local_hdof_re[i];
         global_hdof_im[low+i]=local_hdof_im[i];
         i++;
      }

      // collected
      i=1;
      while (i < size) {
         int transfer_size=0;
         MPI_Recv(&transfer_size,1,MPI_INT,i,10000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int offset=0;
         MPI_Recv(&offset,1,MPI_INT,i,10001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         // real

         double *transfer=(double *)malloc(transfer_size*sizeof(double));
         MPI_Recv(transfer,transfer_size,MPI_DOUBLE,i,10002,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int k=0;
         while (k < transfer_size) {
            global_hdof_re[offset+k]=transfer[k];
            k++;
         }

         // imag

         MPI_Recv(transfer,transfer_size,MPI_DOUBLE,i,10003,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         k=0;
         while (k < transfer_size) {
            global_hdof_im[offset+k]=transfer[k];
            k++;
         }

         if (transfer) free(transfer);

         i++;
      }
   } else {
      MPI_Send(&local_hdof_size,1,MPI_INT,0,10000,PETSC_COMM_WORLD);
      MPI_Send(&low,1,MPI_INT,0,10001,PETSC_COMM_WORLD);
      MPI_Send(local_hdof_re,local_hdof_size,MPI_DOUBLE,0,10002,PETSC_COMM_WORLD);
      MPI_Send(local_hdof_im,local_hdof_size,MPI_DOUBLE,0,10003,PETSC_COMM_WORLD);
   }

   // collect the new partitioning data
   std::vector<int> newLow,newHigh;
   if (rank == 0) {
      newLow.push_back(fespace_ND_offsets[0]);
      newHigh.push_back(fespace_ND_offsets[1]);

      int i=1;
      while (i < size) {
         int lowPart,highPart;
         MPI_Recv(&lowPart,1,MPI_INT,i,20010,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&highPart,1,MPI_INT,i,20011,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         newLow.push_back(lowPart);
         newHigh.push_back(highPart);
         i++;
      }
   } else {
      int lowPart=fespace_ND_offsets[0];
      int highPart=fespace_ND_offsets[1];
      MPI_Send(&lowPart,1,MPI_INT,0,20010,PETSC_COMM_WORLD);
      MPI_Send(&highPart,1,MPI_INT,0,20011,PETSC_COMM_WORLD);
   }

   // send global hdof to all ranks

   if (rank == 0) {
      int i=newLow[0];
      while (i < newHigh[0]) {
         h_re->Elem(i-newLow[0])=global_hdof_re[i];
         h_im->Elem(i-newLow[0])=global_hdof_im[i];
         i++;
      }

      i=1;
      while (i < size) {
         MPI_Send(global_hdof_re+newLow[i],newHigh[i]-newLow[i],MPI_DOUBLE,i,20000,PETSC_COMM_WORLD);
         MPI_Send(global_hdof_im+newLow[i],newHigh[i]-newLow[i],MPI_DOUBLE,i,20001,PETSC_COMM_WORLD);
         i++;
      }

   } else {

      double *transfer=(double *)malloc((fespace_ND_offsets[1]-fespace_ND_offsets[0])*sizeof(double));

      // real

      MPI_Recv(transfer,fespace_ND_offsets[1]-fespace_ND_offsets[0],MPI_DOUBLE,0,20000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      int i=0;
      while (i < fespace_ND_offsets[1]-fespace_ND_offsets[0]) {
         h_re->Elem(i)=transfer[i];
         i++;
      }

      // imag

      MPI_Recv(transfer,fespace_ND_offsets[1]-fespace_ND_offsets[0],MPI_DOUBLE,0,20001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      i=0;
      while (i < fespace_ND_offsets[1]-fespace_ND_offsets[0]) {
         h_im->Elem(i)=transfer[i];
         i++;
      }

      if (transfer) free(transfer);
   }

   // clean up
   if (rank == 0) {
      if (local_hdof_re) free(local_hdof_re);
      if (local_hdof_im) free(local_hdof_im);
      if (global_hdof_re) free(global_hdof_re);
      if (global_hdof_im) free(global_hdof_im);
   }
}

//xxx
/* original with poor MPICH performance
// see notes for build_e_re_e_im
void fem3D::build_h_re_h_im (Vec *hdofs)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // local distribution of fespace_ND - for h_re and h_im
   HYPRE_BigInt *fespace_ND_offsets=fespace_ND->GetTrueDofOffsets();
   int local_ND_size=fespace_ND_offsets[1]-fespace_ND_offsets[0];

   if (h_re) delete h_re;
   if (h_im) delete h_im;

   h_re=new Vector(local_ND_size);
   h_im=new Vector(local_ND_size);

   // local distribution of A - for hdofs

   PetscInt i,low,high;
   VecGetOwnershipRange(*hdofs,&low,&high);
   int local_hdof_size=high-low;

   PetscInt *ixvals;
   PetscMalloc(local_hdof_size*sizeof(PetscInt),&ixvals);

   i=low;
   while (i < high) {
      ixvals[i-low]=i;
      i++;
   }

   PetscScalar *local_hdof_vals;
   PetscMalloc(local_hdof_size*sizeof(PetscScalar),&local_hdof_vals);

   VecGetValues(*hdofs,local_hdof_size,ixvals,local_hdof_vals);

   // global hdofs

   int global_hdof_size=0;
   MPI_Allreduce (&local_hdof_size,&global_hdof_size,1,MPI_INT,MPI_SUM,PETSC_COMM_WORLD);

   PetscScalar *global_hdof_vals;
   PetscMalloc(global_hdof_size*sizeof(PetscScalar),&global_hdof_vals);

   // collect hdofs at rank 0

   if (rank == 0) {

      // local
      int i=0;
      while (i < local_hdof_size) {
         global_hdof_vals[low+i]=local_hdof_vals[i];
         i++;
      }

      // collected
      i=1;
      while (i < size) {
         int transfer_size=0;
         MPI_Recv(&transfer_size,1,MPI_INT,i,10000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int k=0;
         while (k < transfer_size) {
            int location=0;
            double transfer_real=0;
            double transfer_imag=0;
            MPI_Recv(&location,1,MPI_INT,i,10001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(&transfer_real,1,MPI_DOUBLE,i,10002,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(&transfer_imag,1,MPI_DOUBLE,i,10003,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

            global_hdof_vals[location]=transfer_real+PETSC_i*transfer_imag;

            k++;
         }
         i++;
      }
   } else {
      MPI_Send(&local_hdof_size,1,MPI_INT,0,10000,PETSC_COMM_WORLD);

      int i=0;
      while (i < local_hdof_size) {
         int location=low+i;
         double transfer_real=real(local_hdof_vals[i]);
         double transfer_imag=imag(local_hdof_vals[i]);
         MPI_Send(&location,1,MPI_INT,0,10001,PETSC_COMM_WORLD);
         MPI_Send(&transfer_real,1,MPI_DOUBLE,0,10002,PETSC_COMM_WORLD);
         MPI_Send(&transfer_imag,1,MPI_DOUBLE,0,10003,PETSC_COMM_WORLD);
         i++;
      }
   }

   // send global hdofs to all ranks

   if (rank == 0) {
      int i=1;
      while (i < size) {
         int k=0;
         while (k < global_hdof_size) {
            double transfer_real=real(global_hdof_vals[k]);
            double transfer_imag=imag(global_hdof_vals[k]);
            MPI_Send(&transfer_real,1,MPI_DOUBLE,i,20001,PETSC_COMM_WORLD);
            MPI_Send(&transfer_imag,1,MPI_DOUBLE,i,20002,PETSC_COMM_WORLD);
            k++;
         }
         i++;
      }
   } else {
      int k=0;
      while (k < global_hdof_size) {
         double transfer_real=0;
         double transfer_imag=0;
         MPI_Recv(&transfer_real,1,MPI_DOUBLE,0,20001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&transfer_imag,1,MPI_DOUBLE,0,20002,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         global_hdof_vals[k]=transfer_real+PETSC_i*transfer_imag;

         k++;
      }
   }

   // transfer data to h_re and h_im - completes the re-alignment

   int index=0;
   i=0;
   while (i < global_hdof_size) {
      if (i >= fespace_ND_offsets[0] && i < fespace_ND_offsets[1]) {
         h_re->Elem(index)=real(global_hdof_vals[i]);
         h_im->Elem(index)=imag(global_hdof_vals[i]);
         index++;
      }
      i++;
   }

   // clean up
   PetscFree(ixvals);
   PetscFree(local_hdof_vals);
   PetscFree(global_hdof_vals);
}
*/

void fem3D::buildHgrids(BoundaryDatabase *boundaryDatabase, PWConstCoefficient *Inv_w_mu)
{
   int ierr=0;
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   build_Q(Inv_w_mu);

   // hdofs
   Vec hdofs;
   VecDuplicate(x,&hdofs);

   // rhs b
   Vec b;
   buildHb(&Q,&x,&b);

   MatDestroy(&Q);

   build_P();

   // solve for hofs
   ierr=solveHfield (boundaryDatabase->get_tempDirectory().c_str(),projData,rank,&P,&b,&hdofs,&HfieldError,&HfieldConverged);
   if (ierr) {
      cout << "fem3D::buildHgrids: ASSERT: solveHfield returned error code " <<  ierr << endl;
   }

   MatDestroy(&P);

   // Vec hdofs to HypreParVector h_re,h_im
   build_h_re_h_im(&hdofs);

   // fill grids
   gridReH->Distribute(*h_re);
   gridImH->Distribute(*h_im);

   // clean up
   VecDestroy(&hdofs);
   VecDestroy(&b);
}

// Re(E x H*)
void fem3D::build_ReExHgrid()
{
   VectorGridFunctionCoefficient ReEcoef(gridReE);
   VectorGridFunctionCoefficient ImEcoef(gridImE);
   VectorGridFunctionCoefficient ReHcoef(gridReH);
   VectorGridFunctionCoefficient ImHcoef(gridImH);

   // ReE x ReH
   ParDiscreteLinearOperator ReExReH (fespace_ND,fespace_RT);
   ReExReH.AddDomainInterpolator(new VectorCrossProductInterpolator(ReEcoef));
   ReExReH.Assemble();
   ReExReH.Finalize();
   ParGridFunction gridReExReH=ParGridFunction(fespace_RT);
   ReExReH.Mult(*gridReH,gridReExReH);
   VectorGridFunctionCoefficient ReExReHcoef(&gridReExReH);

   // ImE x ImH
   ParDiscreteLinearOperator ImExImH (fespace_ND,fespace_RT);
   ImExImH.AddDomainInterpolator(new VectorCrossProductInterpolator(ImEcoef));
   ImExImH.Assemble();
   ImExImH.Finalize();
   ParGridFunction gridImExImH=ParGridFunction(fespace_RT);
   ImExImH.Mult(*gridImH,gridImExImH);
   VectorGridFunctionCoefficient ImExImHcoef(&gridImExImH);

   VectorSumCoefficient ReExH=VectorSumCoefficient(ReExReHcoef,ImExImHcoef,0.5,0.5);

   gridReExH->ProjectCoefficient(ReExH);
}

// Im(E x H*)
void fem3D::build_ImExHgrid()
{
   VectorGridFunctionCoefficient ReEcoef(gridReE);
   VectorGridFunctionCoefficient ImEcoef(gridImE);
   VectorGridFunctionCoefficient ReHcoef(gridReH);
   VectorGridFunctionCoefficient ImHcoef(gridImH);

   // ReE x ImH
   ParDiscreteLinearOperator ReExImH (fespace_ND,fespace_RT);
   ReExImH.AddDomainInterpolator(new VectorCrossProductInterpolator(ReEcoef));
   ReExImH.Assemble();
   ReExImH.Finalize();
   ParGridFunction gridReExImH=ParGridFunction(fespace_RT);
   ReExImH.Mult(*gridImH,gridReExImH);
   VectorGridFunctionCoefficient ReExImHcoef(&gridReExImH);

   // ImE x ReH
   ParDiscreteLinearOperator ImExReH (fespace_ND,fespace_RT);
   ImExReH.AddDomainInterpolator(new VectorCrossProductInterpolator(ImEcoef));
   ImExReH.Assemble();
   ImExReH.Finalize();
   ParGridFunction gridImExReH=ParGridFunction(fespace_RT);
   ImExReH.Mult(*gridReH,gridImExReH);
   VectorGridFunctionCoefficient ImExReHcoef(&gridImExReH);

   VectorSumCoefficient ImExH=VectorSumCoefficient(ReExImHcoef,ImExReHcoef,-0.5,0.5);

   gridImExH->ProjectCoefficient(ImExH);
}

bool fem3D::solve(BoundaryDatabase *boundaryDatabase, MaterialDatabase *materialDatabase, double temperature,
                  PWConstCoefficient *neg_ko2_Re_er, PWConstCoefficient *neg_ko2_Im_er,
                  PWConstCoefficient *Inv_mur, PWConstCoefficient *Inv_w_mu,
                  int drivingSet_, bool calculateH,
                  bool solution_check_homogeneous, string indent)
{
   PetscInt i,low,high;
   PetscScalar value;
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   drivingSet=drivingSet_;

   if (build_A(boundaryDatabase,materialDatabase,temperature,Inv_mur,neg_ko2_Re_er,neg_ko2_Im_er,
      solution_check_homogeneous,indent)) return true;

   build_PEC_dofs();
   eliminatePEC(&A,nPEC,PEC);
   build_X();
   build_x();
   build_grids();
   build_Xdofs();
   boundaryDatabase->fillX(&X,&Xdofs,drivingSet);
   VecCopy(X,x);

   // port dofs

   PetscInt nPortDof=0;
   VecGetOwnershipRange(Xdofs,&low,&high);
   i=low;
   while (i < high) {
      VecGetValues(Xdofs,1,&i,&value);
      if (value != 0) nPortDof++;
      i++;
   }

   PetscInt *PortDof;
   PetscMalloc(nPortDof*sizeof(PetscInt),&PortDof); 

   i=0;
   while (i < nPortDof) {
      PortDof[i]=0;
      i++;
   }

   PetscInt index=0;
   i=low;
   while (i < high) {
      VecGetValues(Xdofs,1,&i,&value);
      if (value != 0) {
         PortDof[index]=i; index++;
      }
      i++;
   }
   solveComplexLinearSystem (boundaryDatabase->get_tempDirectory().c_str(),projData,rank,
                                  nPortDof,PortDof,&A,&x,&matrixSize,&EfieldError,&EfieldConverged);

   MatDestroy(&A);
   VecDestroy(&X);
   VecDestroy(&Xdofs);
   PetscFree(PortDof); PortDof=nullptr;
   PetscFree(PEC); PEC=nullptr;

   build_e_re_e_im();
   buildEgrids(boundaryDatabase);

   if (calculateH) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"         solving H field in 3D volume ...\n");
      buildHgrids (boundaryDatabase,Inv_w_mu);
   }

   VecDestroy(&x);

   //buildZgrids(boundaryDatabase);

   if (projData->project_calculate_poynting) {
      if (calculateH) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"         calculating Poynting vector field in 3D volume ...\n");
         build_ReExHgrid();
         build_ImExHgrid();
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"         skipping Poynting vector field in 3D volume due to omitted H-field calculation ...\n");
      }
   }

   saveParaView(calculateH,projData->project_calculate_poynting,boundaryDatabase->get_drivingSetName());

   return false;
}

void fem3D::printElementVertices ()
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   Array<int> vertices;
   real_t *coord;
   int i=0;
   while (i < (*pmesh)->GetNE()) {
      (*pmesh)->GetElementVertices(i,vertices);

      cout << rank << ": element " << i << ":" << endl;
      int j=0;
      while (j < vertices.Size()) {
         coord=(*pmesh)->GetVertex(j);
         cout << rank << ":   vertex j x=" << coord[0] << " y=" << coord[1] << " z=" << coord[2] << endl;
         j++;
      }
      i++;
   }
}

// complex(Re_hpv,Im_hpv) -> v
// copy and convert data from partitions on ParFiniteElementSpace to partitions on Vec
void hypreParVectorToVec (HypreParVector *Re_hpv, HypreParVector *Im_hpv, Vec *v)
{
   Vector *Re_gv=Re_hpv->GlobalVector();
   Vector *Im_gv=Im_hpv->GlobalVector();

   PetscInt i,low,high;
   VecGetOwnershipRange(*v,&low,&high);

   PetscInt *ixvals;
   PetscMalloc((high-low)*sizeof(PetscInt),&ixvals);

   i=low;
   while (i < high) {
      ixvals[i-low]=i;
      i++;
   }

   PetscScalar *xvals;
   PetscMalloc((high-low)*sizeof(PetscScalar),&xvals);

   i=low;
   while (i < high) {
      xvals[i-low]=Re_gv->Elem(i)+PETSC_i*Im_gv->Elem(i);
      i++;
   }

   VecSetValues(*v,high-low,ixvals,xvals,INSERT_VALUES);

   VecAssemblyBegin(*v);
   VecAssemblyEnd(*v);

   delete Re_gv;
   delete Im_gv;
   PetscFree(ixvals);
   PetscFree(xvals);
}

// v -> complex(Re_hpv,Im_hpv) 
// copy and convert data from partitions on Vec to partitions on ParFiniteElementSpace
void vecToHypreParVector (Vec *v, HypreParVector *Re_hpv, HypreParVector *Im_hpv)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   PetscInt i,v_size,low,high;

   VecGetSize(*v,&v_size);
   VecGetOwnershipRange(*v,&low,&high);

   Vector Re_global_v,Im_global_v;
   Re_global_v.SetSize(v_size);
   Im_global_v.SetSize(v_size);

   PetscInt *ixvals;
   PetscMalloc((high-low)*sizeof(PetscInt),&ixvals);

   i=low;
   while (i < high) {
      ixvals[i-low]=i;
      i++;
   }

   PetscScalar *xvals;
   PetscMalloc((high-low)*sizeof(PetscScalar),&xvals);

   VecGetValues(*v,high-low,ixvals,xvals);

   // collect at zero
   if (rank == 0) {
      int i=low;
      while (i < high) {
         Re_global_v[i]=real(xvals[i-low]);
         Im_global_v[i]=imag(xvals[i-low]);
         i++;
      }

      i=1;
      while (i < size) {
         int count=0;
         MPI_Recv(&count,1,MPI_INT,i,300,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int offset=0;
         MPI_Recv(&offset,1,MPI_INT,i,301,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         double *pack=(double *)malloc(2*count*sizeof(double));
         MPI_Recv(pack,2*count,MPI_DOUBLE,i,302,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int j=0;
         while (j < count) {
            Re_global_v[offset+j]=pack[j];
            Im_global_v[offset+j]=pack[count+j];
            j++;
         }

         if (pack) free(pack);

         i++;
      }
   } else {
      int count=high-low;
      MPI_Send(&count,1,MPI_INT,0,300,PETSC_COMM_WORLD);

      int offset=low;
      MPI_Send(&offset,1,MPI_INT,0,301,PETSC_COMM_WORLD);

      double *pack=(double *)malloc(2*count*sizeof(double));
      int i=low;
      while (i < high) {
         pack[i-low]=real(xvals[i-low]);
         pack[count+i-low]=imag(xvals[i-low]);
         i++;
      }
      MPI_Send(pack,2*count,MPI_DOUBLE,0,302,PETSC_COMM_WORLD);
      if (pack) free(pack);
   }

   // send to other ranks
   if (rank == 0) {

      double *pack=(double *)malloc(2*v_size*sizeof(double));
      int i=0;
      while (i < v_size) {
         pack[i]=Re_global_v.Elem(i);
         pack[i+v_size]=Im_global_v.Elem(i);
         i++;
      }

      i=1;
      while (i < size) {
         MPI_Send(pack,2*v_size,MPI_DOUBLE,i,303,PETSC_COMM_WORLD);
         i++;
      }

      if (pack) free(pack);
   } else {
      double *pack=(double *)malloc(2*v_size*sizeof(double));
      MPI_Recv(pack,2*v_size,MPI_DOUBLE,0,303,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      int i=0;
      while (i < v_size) {
         Re_global_v[i]=pack[i];
         Im_global_v[i]=pack[v_size+i];
         i++;
      }

      if (pack) free(pack);
   }

   // transfer to the HypreParVectors
   HYPRE_BigInt *hpv_offsets=Re_hpv->Partitioning();

   i=hpv_offsets[0];
   while (i < hpv_offsets[1]) {
      Re_hpv->Elem(i-hpv_offsets[0])=Re_global_v.Elem(i);
      Im_hpv->Elem(i-hpv_offsets[0])=Im_global_v.Elem(i);
      i++;
   }

   PetscFree(ixvals);
   PetscFree(xvals);
}

// This is a heavily modifed version of the OPEM_L2ZZErrorEstimator in OPEM_L2ZZErrorEstimator.cpp
// to convert to operation with complex fields and to solve the complex problem with Petsc.  Switching 
// here to reflect a change in the software license, which seems warranted given the extensive changes.
// Testing shows that this produces the same results as OPEM_L2ZZErrorEstimator in OPEM_L2ZZErrorEstimator.cpp
// but runs noticeably slower.  Leaving here for future reference.
bool OPEM_L2ZZErrorEstimator (BilinearFormIntegrator &flux_integrator,
                              const ParGridFunction &Re_x, const ParGridFunction &Im_x,
                              ParFiniteElementSpace &smooth_flux_fes,
                              ParFiniteElementSpace &flux_fes,
                              Vector &errors,
                              double norm_p, double solver_tol, int solver_max_it,
                              double &finalResidualNorm)
{
   PetscErrorCode ierr=0;
   bool fail=false;

   // Compute fluxes in discontinuous space
   GridFunction Re_flux(&flux_fes);
   GridFunction Im_flux(&flux_fes);

   Re_flux=0.0;
   Im_flux=0.0;

   ParFiniteElementSpace *Re_xfes=Re_x.ParFESpace();
   ParFiniteElementSpace *Im_xfes=Im_x.ParFESpace();

   Array<int> Re_xdofs,Re_fdofs,Im_xdofs,Im_fdofs;
   Vector Re_el_x,Re_el_f,Im_el_x,Im_el_f;

   int i=0;
   while (i < Re_xfes->GetNE()) {
      const DofTransformation* const Re_xtrans=Re_xfes->GetElementVDofs(i,Re_xdofs);
      const DofTransformation* const Im_xtrans=Im_xfes->GetElementVDofs(i,Im_xdofs);

      Re_x.GetSubVector(Re_xdofs,Re_el_x);
      Im_x.GetSubVector(Im_xdofs,Im_el_x);

      if (Re_xtrans) Re_xtrans->InvTransformPrimal(Re_el_x);
      if (Im_xtrans) Im_xtrans->InvTransformPrimal(Im_el_x);

      ElementTransformation *Re_Transf=Re_xfes->GetElementTransformation(i);
      ElementTransformation *Im_Transf=Im_xfes->GetElementTransformation(i);

      flux_integrator.ComputeElementFlux(*Re_xfes->GetFE(i),*Re_Transf,Re_el_x,*flux_fes.GetFE(i),Re_el_f,false);
      flux_integrator.ComputeElementFlux(*Im_xfes->GetFE(i),*Im_Transf,Im_el_x,*flux_fes.GetFE(i),Im_el_f,false);

      const DofTransformation* const Re_ftrans=flux_fes.GetElementVDofs(i,Re_fdofs);
      const DofTransformation* const Im_ftrans=flux_fes.GetElementVDofs(i,Im_fdofs);

      if (Re_ftrans) Re_ftrans->TransformPrimal(Re_el_f);
      if (Im_ftrans) Im_ftrans->TransformPrimal(Im_el_f);

      Re_flux.SetSubVector(Re_fdofs,Re_el_f);
      Im_flux.SetSubVector(Im_fdofs,Im_el_f);

      i++;
   }

   // Assemble the linear system for L2 projection into the "smooth" space

   ParBilinearForm *a=new ParBilinearForm(&smooth_flux_fes);

   ParLinearForm *Re_b=new ParLinearForm(&smooth_flux_fes);
   ParLinearForm *Im_b=new ParLinearForm(&smooth_flux_fes);

   VectorGridFunctionCoefficient Re_f(&Re_flux);
   VectorGridFunctionCoefficient Im_f(&Im_flux);

   if (Re_xfes->GetNE()) {
      MFEM_VERIFY(smooth_flux_fes.GetFE(0) != NULL,
                  "Could not obtain FE of smooth flux space.");

      if (smooth_flux_fes.GetFE(0)->GetRangeType() == FiniteElement::SCALAR) {
         VectorMassIntegrator *vmass=new VectorMassIntegrator;
         vmass->SetVDim(smooth_flux_fes.GetVDim());
         a->AddDomainIntegrator(vmass);

         Re_b->AddDomainIntegrator(new VectorDomainLFIntegrator(Re_f));
         Im_b->AddDomainIntegrator(new VectorDomainLFIntegrator(Im_f));
      } else {
         a->AddDomainIntegrator(new VectorFEMassIntegrator);

         Re_b->AddDomainIntegrator(new VectorFEDomainLFIntegrator(Re_f));
         Im_b->AddDomainIntegrator(new VectorFEDomainLFIntegrator(Im_f));
      }
   }

   Re_b->Assemble();
   Im_b->Assemble();
   a->Assemble();
   a->Finalize();

   // The destination of the projected discontinuous flux
   ParGridFunction smooth_flux(&smooth_flux_fes);
   smooth_flux=0.0;

   HypreParMatrix* Re_A=a->ParallelAssemble();
   HypreParVector* Re_B=Re_b->ParallelAssemble();
   HypreParVector* Im_B=Im_b->ParallelAssemble();

   delete a;
   delete Re_b;
   delete Im_b;

   // solve with Petsc

   Mat A;
   PetscInt sparseWidth;
   hypre_getSparseWidth((hypre_ParCSRMatrix *) *Re_A,&sparseWidth);
   if (hypre_ParCSRMatrixToMat((hypre_ParCSRMatrix *) *Re_A,&A,sparseWidth,1,0,1)) return true;
   delete Re_A;

   Vec b,x;
   MatCreateVecs(A,&b,&x);
   hypreParVectorToVec(Re_B,Im_B,&b);

   KSP ksp;
   PC pc;

   ierr=KSPCreate(PETSC_COMM_WORLD, &ksp); if (ierr) return 12;
//xxx
//   ierr=KSPSetType(ksp,KSPCG); if (ierr) return 13;
ierr=KSPSetType(ksp,KSPGMRES); if (ierr) return 13;
//ierr=KSPSetType(ksp,KSPFGMRES); if (ierr) return 13;
//ierr=KSPSetType(ksp,KSPLGMRES); if (ierr) return 13;
//ierr=KSPSetType(ksp,KSPBCGS); if (ierr) return 13;
   ierr=KSPSetOperators(ksp,A,A); if (ierr) return 14;
   ierr=KSPGetPC(ksp,&pc); if (ierr) return 15;
//   ierr=PCSetType(pc,PCSOR); if (ierr) return 19;  // 2X  with CG and GMRES
ierr=PCSetType(pc,PCCHOLESKY); if (ierr) return 19;  // 1/2X with GC and GMRES
//ierr=PCSetType(pc,PCLU); if (ierr) return 19; // slightly more that 1/2X with GMRES
//ierr=PCSetType(pc,PCJACOBI); if (ierr) return 19;  // almost 2X with GMRES
//ierr=PCSetType(pc,PCILU); // fails with GMRES
//ierr=PCSetType(pc,PCICC); // fails with GMRES
   ierr=PCFactorSetShiftType(pc,MAT_SHIFT_NONZERO); if (ierr) return 16;
   ierr=KSPSetFromOptions(ksp); if (ierr) return 17;

   PetscInt maxits;
   PetscReal rtol,atol,dtol;
   maxits=solver_max_it;                            // PETSc default = 1e4
   rtol=solver_tol;                                 // PETSc default = 1e-5
   //atol=projData->solution_3D_tolerance;          // PETSC default = 1e-50
   atol=1e-50;                                      // PETSC default = 1e-50
   dtol=1e5;                                        // PETSC default = 1e5
   ierr=KSPSetTolerances(ksp,rtol,atol,dtol,maxits); if (ierr) return 18;

   ierr=KSPSolve(ksp,b,x); if (ierr) return 24;

   // get stats
   KSPConvergedReason reason;
   PetscReal rnorm;
   ierr=KSPGetResidualNorm(ksp,&rnorm); if (ierr) return 25;
   ierr=KSPGetConvergedReason(ksp,&reason); if (ierr) return 26;
   ierr=KSPDestroy(&ksp); if (ierr) return 27;
   if (reason < 0) fail=true;

   // move x back onto ParFiniteElementSpace partitioning
   vecToHypreParVector (&x,Re_B,Im_B);

   //xxx
   //ToDo: fix the return values
   // cleanup
   ierr=MatDestroy(&A);  if (ierr) return ierr;
   ierr=VecDestroy(&x);  if (ierr) return 27;
   ierr=VecDestroy(&b);  if (ierr) return 27;

   // put on grid functions

   ParGridFunction Re_smooth_flux(&smooth_flux_fes);
   ParGridFunction Im_smooth_flux(&smooth_flux_fes);

   Re_smooth_flux.Distribute(Re_B);
   Im_smooth_flux.Distribute(Im_B);

   delete Re_B;
   delete Im_B;

   // Proceed through the elements one by one, and find the Lp norm differences
   // between the flux as computed per element and the flux projected onto the
   // smooth_flux_fes space.
   errors.SetSize(Re_xfes->GetNE());
   i=0;
   while (i < Re_xfes->GetNE()) {
      errors(i)=sqrt(pow(ComputeElementLpDistance(norm_p,i,Re_smooth_flux,Re_flux),2)+
                     pow(ComputeElementLpDistance(norm_p,i,Im_smooth_flux,Im_flux),2));
      i++;
   }

   return fail;
}


// calculate the mesh errors using a Zienkiewicz-Zhu estimator
//
// Calculates the mesh errors on the H fields.  Since these are calculated from E, any error in E is magnified, and adaptive refinement
// does a better job of refining the mesh than if the errors are calculated on E.  To switch to calculating the error on E, change
// gridReH and gridImH to gridReE and gridImE in the two calls to OPEM_L2ZZErrorEstimator.
bool fem3D::calculateMeshErrors (struct projectData *projData, BoundaryDatabase *boundaryDatabase, PWConstCoefficient *Inv_mur, double *maxMeshError_)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   CurlCurlIntegrator flux_integrator(*Inv_mur);

   RT_FECollection flux_fec(projData->fem_order-1, (*pmesh)->SpaceDimension());
   ParFiniteElementSpace flux_fes(*pmesh, &flux_fec);

   ND_FECollection smooth_flux_fec(projData->fem_order,(*pmesh)->Dimension());
   ParFiniteElementSpace smooth_flux_fes(*pmesh, &smooth_flux_fec);

   double solution_tolerance=1e-12;
   double solution_tolerance_message_limit=1e-3;
   int iteration_limit=2000;
   double norm_p=1;
   Vector ReLocalErrors;

   bool use_complex=false;
   if (use_complex) {
      double FinalResidualNorm=0;
      bool printError=false;
      if (OPEM_L2ZZErrorEstimator(flux_integrator,*gridReH,*gridImH,smooth_flux_fes,flux_fes,ReLocalErrors,norm_p,
                                  solution_tolerance,iteration_limit,FinalResidualNorm)) printError=true;
      if (printError) {
         if (FinalResidualNorm > solution_tolerance_message_limit) {
            prefix(); PetscPrintf (PETSC_COMM_WORLD,"            INFO: Calculated mesh errors are approximate with the final error of %g > the target of %g.\n",
                                                     FinalResidualNorm,solution_tolerance_message_limit);
         }
      }
   } else {
      double ReFinalResidualNorm=0;
      double ImFinalResidualNorm=0;
      bool printError=false;
      Vector ImLocalErrors;
      int show_real_iterations=0;
      int show_imag_iterations=0;
      if (projData->output_show_iterations) {show_real_iterations=1; show_imag_iterations=2;}
      if (OPEM_L2ZZErrorEstimator(flux_integrator,*gridReH,smooth_flux_fes,flux_fes,ReLocalErrors,norm_p,
                                  solution_tolerance,iteration_limit,ReFinalResidualNorm,projData->debug_refine_preconditioner,show_real_iterations)) printError=true;
      if (OPEM_L2ZZErrorEstimator(flux_integrator,*gridImH,smooth_flux_fes,flux_fes,ImLocalErrors,norm_p,
                                  solution_tolerance,iteration_limit,ImFinalResidualNorm,projData->debug_refine_preconditioner,show_imag_iterations)) printError=true;
      if (printError) {
         if (ImFinalResidualNorm > ReFinalResidualNorm) ReFinalResidualNorm=ImFinalResidualNorm;
         if (ReFinalResidualNorm > solution_tolerance_message_limit) {
            prefix(); PetscPrintf (PETSC_COMM_WORLD,"            INFO: Calculated mesh errors are approximate with the final error of %g > the target of %g.\n",
                                                     ReFinalResidualNorm,solution_tolerance_message_limit);
         }
      }

      int i=0; 
      while (i < ReLocalErrors.Size()) {
         ReLocalErrors[i]=sqrt(ReLocalErrors[i]*ReLocalErrors[i]+ImLocalErrors[i]*ImLocalErrors[i]);
         i++;
      }
   }

   /* Does not work as well as with no scaling
   // scale the local errors by the longest element edge
   Array<int> vertices;
   real_t *coord1,*coord2;
   int i=0;
   while (i < (*pmesh)->GetNE()) {
      (*pmesh)->GetElementVertices(i,vertices);
      real_t maxLength=0;
      int j=0;
      while (j < vertices.Size()) {
         coord1=(*pmesh)->GetVertex(j);
         int k=0;
         while (k < vertices.Size()) {
            if (k != j) {
               coord2=(*pmesh)->GetVertex(k);
               real_t length=sqrt(pow(coord1[0]-coord2[0],2)+pow(coord1[1]-coord2[1],2)+pow(coord1[2]-coord2[2],2));
               if (length > maxLength) maxLength=length;
            }
            k++;
         }
         j++;
      }
      //ReLocalErrors[i]*=pow(maxLength,3);
      ReLocalErrors[i]*=maxLength;
      i++;
   }
   */

   /* Does not work as well as with no scaling
   // scale the local errors by the element volume
   int i=0;
   while (i < (*pmesh)->GetNE()) {
      real_t volume=(*pmesh)->GetElementVolume(i);
      ReLocalErrors[i]*=volume;
      //ReLocalErrors[i]*=pow(volume,0.3333);
      i++;
   }
   */

   // merge in the errors from the prior pass (i.e. different driven port) - keep the larger error
   int i=0;
   while (i < (int)errors.size()) {
      if (errors[i] > ReLocalErrors[elements[i]]) ReLocalErrors[elements[i]]=errors[i];
      i++;
   }

   // transfer the data
   errors.clear();
   elements.clear();
   ranks.clear();
   i=0;
   while (i < ReLocalErrors.Size()) {
      errors.push_back(ReLocalErrors[i]);
      elements.push_back(i);
      ranks.push_back(rank);
      i++;
   }

   // collect the errors at rank 0

   if (rank == 0) {
      int i=1;
      while (i < size) {
         int transfer_size;
         MPI_Recv(&transfer_size,1,MPI_INT,i,1000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         double *pack=(double *)malloc(transfer_size*sizeof(double));
         MPI_Recv(pack,transfer_size,MPI_DOUBLE,i,1001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         int j=0;
         while (j < transfer_size) {
            errors.push_back(pack[j]);
            j++;
         }
         if (pack) free(pack);

         int *packi=(int *)malloc(transfer_size*sizeof(int));

         MPI_Recv(packi,transfer_size,MPI_INT,i,1002,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         j=0;
         while (j < transfer_size) {
            elements.push_back(packi[j]);
            j++;
         }

         MPI_Recv(packi,transfer_size,MPI_INT,i,1003,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         j=0;
         while (j < transfer_size) {
            ranks.push_back(packi[j]);
            j++;
         }

         if (packi) free(packi);

         i++;
      }
   } else {
      int local_size=errors.size();
      MPI_Send(&local_size,1,MPI_INT,0,1000,PETSC_COMM_WORLD);

      double *pack=(double *)malloc(local_size*sizeof(double));
      int i=0;
      while (i < local_size) {
         pack[i]=errors[i];
         i++;
      }
      MPI_Send(pack,local_size,MPI_DOUBLE,0,1001,PETSC_COMM_WORLD);
      if (pack) free(pack);

      int *packi=(int *)malloc(local_size*sizeof(int));

      i=0;
      while (i < local_size) {
         packi[i]=elements[i];
         i++;
      }
      MPI_Send(packi,local_size,MPI_INT,0,1002,PETSC_COMM_WORLD);

      i=0;
      while (i < local_size) {
         packi[i]=ranks[i];
         i++;
      }
      MPI_Send(packi,local_size,MPI_INT,0,1003,PETSC_COMM_WORLD);

      if (packi) free(packi);
   }

   // sort and find the top errors

   refinementCount=getGlobalNE(*pmesh)*projData->mesh_3D_refinement_fraction;
   if (refinementCount == 0) refinementCount=1;

   double maxMeshError=-1;
   if (rank == 0) {
      int i=0;
      while (i < refinementCount) {
         int j=i+1;
         while (j < (int)errors.size()) {
            if (errors[i] < errors[j]) {
               double temp_error=errors[i];
               errors[i]=errors[j];
               errors[j]=temp_error;

               int temp_element=elements[i];
               elements[i]=elements[j];
               elements[j]=temp_element;

               int temp_rank=ranks[i];
               ranks[i]=ranks[j];
               ranks[j]=temp_rank;
            }
            j++;
         }
         i++;
      }
      maxMeshError=errors[0];
   }

   MPI_Bcast(&maxMeshError,1,MPI_DOUBLE,0,PETSC_COMM_WORLD);
   *maxMeshError_=maxMeshError;

   // send back to the ranks

   if (rank == 0) {

      int i=1;
      while (i < size) {

         // count the elements to send
         int count=0;
         long unsigned int j=0;
         while (j < (long unsigned int)refinementCount) {
            if (ranks[j] == i) count++;
            j++;
         }

         MPI_Send(&count,1,MPI_INT,i,2000,PETSC_COMM_WORLD);

         j=0;
         while (j < (long unsigned int)refinementCount) {
            if (ranks[j] == i) {
               MPI_Send(&(errors[j]),1,MPI_DOUBLE,i,2001,PETSC_COMM_WORLD);
               MPI_Send(&(elements[j]),1,MPI_INT,i,2002,PETSC_COMM_WORLD);
               MPI_Send(&(ranks[j]),1,MPI_INT,i,2003,PETSC_COMM_WORLD);
            }
            j++;
         }

         i++;
      }

      // reduce the rank 0 elements to just the local elements
      vector<double> errors_copy=errors; errors.clear();
      vector<int> elements_copy=elements; elements.clear();
      vector<int> ranks_copy=ranks; ranks.clear();
      int j=0;
      while (j < (int)refinementCount) {
         if (ranks[j] == 0) {
            errors.push_back(errors_copy[j]);
            elements.push_back(elements_copy[j]);
            ranks.push_back(ranks_copy[j]);
         }
         j++;
      }

   } else {
      errors.clear();
      elements.clear();
      ranks.clear();

      int transfer_count=0;
      MPI_Recv(&transfer_count,1,MPI_INT,0,2000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      int i=0;
      while (i < transfer_count) {
         double local_error=0;
         MPI_Recv(&local_error,1,MPI_DOUBLE,0,2001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         errors.push_back(local_error);

         int local_element=0;
         MPI_Recv(&local_element,1,MPI_INT,0,2002,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         elements.push_back(local_element);

         int local_rank=0;
         MPI_Recv(&local_rank,1,MPI_INT,0,2003,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         ranks.push_back(local_rank);

         i++;
      }
   }

   return false;
}

void fem3D::refineMesh ()
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   // transfer the data for use with GeneralRefinement
   Array<int> localRefineList(elements.size());
   int i=0;
   while (i < (int)elements.size()) {
      localRefineList[i]=elements[i];
      i++;
   }

   // refine the mesh
   int previousMeshSize=getGlobalNE(*pmesh);
   (*pmesh)->GeneralRefinement(localRefineList);
   int newMeshSize=getGlobalNE(*pmesh);

   if (projData->output_show_refining_mesh) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"             new mesh size: %d\n",newMeshSize);}
   if (newMeshSize > 3*previousMeshSize) {prefix(); PetscPrintf(PETSC_COMM_WORLD,
      "             Warning: The mesh size jumped from %d to %d.\n",previousMeshSize,newMeshSize);}
}

void fem3D::saveParaView (bool plot_H, bool plot_poynting, string set_name)
{
   if (!projData->project_save_fields) return;

   stringstream ss;
   ss << projData->project_name << "_frequency_" << frequency << "_" << set_name << "_" << drivingSet;

   stringstream ssParaView;
   ssParaView << "ParaView_" << projData->project_name;

   ParaViewDataCollection *pd=nullptr;
   pd=new ParaViewDataCollection(ss.str(),*pmesh);
   pd->SetOwnData(false);
   pd->SetPrefixPath(ssParaView.str());
   pd->RegisterField("gridReE",gridReE);
   pd->RegisterField("gridImE",gridImE);
   if (plot_H) pd->RegisterField("gridReH",gridReH);
   if (plot_H) pd->RegisterField("gridImH",gridImH);
   if (plot_poynting) pd->RegisterField("gridReExH",gridReExH);
   if (plot_poynting) pd->RegisterField("gridImExH",gridImExH);
   pd->SetLevelsOfDetail(3);
   pd->SetDataFormat(VTKFormat::ASCII);
   pd->SetHighOrderOutput(true);
   pd->Save();
   delete pd;
}

void fem3D::saveFieldValuesHeader (struct projectData *projData)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   // nothing to do
   if (projData->field_points_count == 0) return;

   if (rank == 0) {
      stringstream ss;
      ss << projData->project_name << "_fields.csv";

      ofstream out;
      out.open(ss.str().c_str(),ofstream::app);

      if (out.is_open()) {
         out << "#frequency,iteration,driving Sport,"
             << "real(Ex),real(Ey),real(Ez),"
             << "imag(Ex),imag(Ey),imag(Ez),"
             << "real(Hx),real(Hy),real(Hz),"
             << "imag(Hx),imag(Hy),imag(Hz)"
             << endl;

         out.close();
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3009: Failed to open file \"%s\" for writing.\n",ss.str().c_str());
      }
   }
}

void fem3D::saveFieldValues (struct projectData *projData, ParMesh *pmesh, int iteration, int drivingSport)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   // nothing to do
   if (projData->field_points_count == 0) return;

   // transfer the data into a useful format - make a copy for E and a copy for H
   OPEMIntegrationPointList Elist,Hlist;
   int i=0;
   while (i < projData->field_points_count) {

      OPEMIntegrationPoint *point=new OPEMIntegrationPoint(i,
         projData->field_points_x[i],projData->field_points_y[i],projData->field_points_z[i]);
      Elist.push(point);

      point=new OPEMIntegrationPoint(i,
         projData->field_points_x[i],projData->field_points_y[i],projData->field_points_z[i]);
      Hlist.push(point);

      i++;
   }

   // get the values

   Elist.update(pmesh);
   Elist.get_fieldValues(gridReE,gridImE);
   Elist.assemble();

   Hlist.update(pmesh);
   Hlist.get_fieldValues(gridReH,gridImH);
   Hlist.assemble();

   if (rank == 0) {

      stringstream ss;
      ss << projData->project_name << "_fields.csv";

      ofstream out;
      out.open(ss.str().c_str(),ofstream::app);

      if (out.is_open()) {
         long unsigned int i=0;
         while (i < Elist.get_size()) {
            double x1,y1,z1,x2,y2,z2;
            Elist.get_point(i)->get_location(&x1,&y1,&z1);
            Hlist.get_point(i)->get_location(&x2,&y2,&z2);
            if (!double_compare(x1,x2,1e-12) || !double_compare(y1,y2,1e-12) || !double_compare(z1,z2,1e-12)) {
               cout << "ASSERT: fem3D::saveFieldValues: Mismatched coordinates." << endl;
            }

            complex<double> Ex,Ey,Ez,Hx,Hy,Hz;
            Elist.get_point(i)->get_fields(&Ex,&Ey,&Ez);
            Hlist.get_point(i)->get_fields(&Hx,&Hy,&Hz);

            // match ParaView's "probe" output format
            // Note that the results do not exactly match Paraview.
            out << frequency << ","
                << iteration << ","
                << drivingSport << ", "
                << x1 << "," << y1 << "," << z1 << ", "
                << real(Ex) << "," << real(Ey) << "," << real(Ez) << ", "
                << imag(Ex) << "," << imag(Ey) << "," << imag(Ez) << ", "
                << real(Hx) << "," << real(Hy) << "," << real(Hz) << ", "
                << imag(Hx) << "," << imag(Hy) << "," << imag(Hz) 
                << endl;

            i++;
         }

         out.close();
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3004: Failed to open file \"%s\" for writing.\n",ss.str().c_str());
      }
   }
}

// conversion = 0 => conversion from modal to single-ended
// conversion = 1 => conversion from single-ended to mixed-mode
PetscErrorCode fem3D::build_Mc_Ms (double referenceZ, BoundaryDatabase *boundaryDatabase, vector<DifferentialPair *> *differentialPairList, int conversion)
{
   PetscErrorCode ierr=0;
   bool SINGLEENDED=false,MIXEDMODE=false;

   if (conversion == 0) SINGLEENDED=true;
   if (conversion == 1) MIXEDMODE=true;

   // for MIXEDMODE
   double Zoe=referenceZ/2;
   double Zoo=referenceZ*2;

   int SportCount=boundaryDatabase->get_SportCount();

   // make a list of the modes across all ports - ordered in terms of increasing Sport to align with the S-parameters
   vector<Mode *> modeList;
   if (boundaryDatabase->buildAggregateModeList(&modeList)) return true;
   if (SportCount != (int)modeList.size()) {
      PetscPrintf(PETSC_COMM_WORLD,"ASSERT: fem3D::build_Mc_Ms found inconsistent number of Sports.\n");
      return true;
   }

   // for convenience, make lists of the ports for each mode
   vector<Port *> portList;
   long unsigned int i=0;
   while (i < modeList.size()) {
      Port *port;
      long unsigned int index;
      if (boundaryDatabase->get_port_from_mode(modeList[i],&port,&index)) {
         PetscPrintf(PETSC_COMM_WORLD,"ASSERT: fem3D::build_Mc_Ms failed to find a port.\n");
         return true;
      }
      portList.push_back(port);
      i++;
   }

   // allocate space
   Mat Kv;
   ierr=MatCreate(PETSC_COMM_WORLD,&Kv); if (ierr) return ierr;
   ierr=MatSetType(Kv,MATDENSE); if (ierr) return ierr;
   ierr=MatSetSizes(Kv,PETSC_DECIDE,PETSC_DECIDE,SportCount,SportCount); if (ierr) return ierr;
   ierr=MatZeroEntries(Kv); if (ierr) return ierr;
   PetscInt Kvlow,Kvhigh;
   ierr=MatGetOwnershipRange(Kv,&Kvlow,&Kvhigh); if (ierr) return ierr;

   Mat Ki;
   ierr=MatCreate(PETSC_COMM_WORLD,&Ki); if (ierr) return ierr;
   ierr=MatSetType(Ki,MATDENSE); if (ierr) return ierr;
   ierr=MatSetSizes(Ki,PETSC_DECIDE,PETSC_DECIDE,SportCount,SportCount); if (ierr) return ierr;
   ierr=MatZeroEntries(Ki); if (ierr) return ierr;
   PetscInt Kilow,Kihigh;
   ierr=MatGetOwnershipRange(Ki,&Kilow,&Kihigh); if (ierr) return ierr;

   Mat rootZa;
   ierr=MatCreate(PETSC_COMM_WORLD,&rootZa); if (ierr) return ierr;
   ierr=MatSetType(rootZa,MATDENSE); if (ierr) return ierr;
   ierr=MatSetSizes(rootZa,PETSC_DECIDE,PETSC_DECIDE,SportCount,SportCount); if (ierr) return ierr;
   ierr=MatZeroEntries(rootZa); if (ierr) return ierr;
   PetscInt rootZalow,rootZahigh;
   ierr=MatGetOwnershipRange(rootZa,&rootZalow,&rootZahigh); if (ierr) return ierr;

   Mat rootZb;
   ierr=MatCreate(PETSC_COMM_WORLD,&rootZb); if (ierr) return ierr;
   ierr=MatSetType(rootZb,MATDENSE); if (ierr) return ierr;
   ierr=MatSetSizes(rootZb,PETSC_DECIDE,PETSC_DECIDE,SportCount,SportCount); if (ierr) return ierr;
   ierr=MatZeroEntries(rootZb); if (ierr) return ierr;
   PetscInt rootZblow,rootZbhigh;
   ierr=MatGetOwnershipRange(rootZb,&rootZblow,&rootZbhigh); if (ierr) return ierr;

   Mat invRootZa;
   ierr=MatCreate(PETSC_COMM_WORLD,&invRootZa); if (ierr) return ierr;
   ierr=MatSetType(invRootZa,MATDENSE); if (ierr) return ierr;
   ierr=MatSetSizes(invRootZa,PETSC_DECIDE,PETSC_DECIDE,SportCount,SportCount); if (ierr) return ierr;
   ierr=MatZeroEntries(invRootZa); if (ierr) return ierr;
   PetscInt invRootZalow,invRootZahigh;
   ierr=MatGetOwnershipRange(invRootZa,&invRootZalow,&invRootZahigh); if (ierr) return ierr;

   Mat invRootZb;
   ierr=MatCreate(PETSC_COMM_WORLD,&invRootZb); if (ierr) return ierr;
   ierr=MatSetType(invRootZb,MATDENSE); if (ierr) return ierr;
   ierr=MatSetSizes(invRootZb,PETSC_DECIDE,PETSC_DECIDE,SportCount,SportCount); if (ierr) return ierr;
   ierr=MatZeroEntries(invRootZb); if (ierr) return ierr;
   PetscInt invRootZblow,invRootZbhigh;
   ierr=MatGetOwnershipRange(invRootZb,&invRootZblow,&invRootZbhigh); if (ierr) return ierr;


   // fill rootZa and rootZb

   PetscScalar value;

   if (SINGLEENDED) {
      PetscInt i=0;
      while (i < (PetscInt)modeList.size()) {
         value=real(sqrt(modeList[i]->get_impedance()))+PETSC_i*imag(sqrt(modeList[i]->get_impedance()));
         if (i >= rootZalow && i < rootZahigh) {ierr=MatSetValue(rootZa,i,i,value,INSERT_VALUES); if (ierr) return ierr;}
         SportZoList.push_back(referenceZ);
         value=sqrt(referenceZ);
         if (i >= rootZblow && i < rootZbhigh) {ierr=MatSetValue(rootZb,i,i,value,INSERT_VALUES); if (ierr) return ierr;}
         i++;
      }
   }

   if (MIXEDMODE) {
      PetscInt i=0;
      while (i < (PetscInt)modeList.size()) {
         value=sqrt(referenceZ);
         if (i >= rootZalow && i < rootZahigh) {ierr=MatSetValue(rootZa,i,i,value,INSERT_VALUES); if (ierr) return ierr;}
         SportZoList.push_back(referenceZ);
         value=sqrt(referenceZ);
         if (i >= rootZblow && i < rootZbhigh) {ierr=MatSetValue(rootZb,i,i,value,INSERT_VALUES); if (ierr) return ierr;} // default to single-ended
         i++;
      }

      i=0;
      while (i < (PetscInt)differentialPairList->size()) {
         PetscInt p=(*differentialPairList)[i]->get_Sport_P()-1;
         PetscInt n=(*differentialPairList)[i]->get_Sport_N()-1;

         // set to mixed mode
         SportZoList[p]=Zoe;
         value=sqrt(Zoe);
         if (p >= rootZblow && p < rootZbhigh) {ierr=MatSetValue(rootZb,p,p,value,INSERT_VALUES); if (ierr) return ierr;}
         SportZoList[n]=Zoo;
         value=sqrt(Zoo);
         if (n >= rootZblow && n < rootZbhigh) {ierr=MatSetValue(rootZb,n,n,value,INSERT_VALUES); if (ierr) return ierr;}
         i++;
      }
   }

   ierr=MatAssemblyBegin(rootZa,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   ierr=MatAssemblyEnd(rootZa,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;

   ierr=MatAssemblyBegin(rootZb,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   ierr=MatAssemblyEnd(rootZb,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;

   // fill invRootZa and invRootZb

   if (SINGLEENDED) {
      PetscInt i=0;
      while (i < (PetscInt)modeList.size()) {
         value=real(1/sqrt(modeList[i]->get_impedance()))+PETSC_i*imag(1/sqrt(modeList[i]->get_impedance()));
         if (i >= invRootZalow && i < invRootZahigh) {ierr=MatSetValue(invRootZa,i,i,value,INSERT_VALUES); if (ierr) return ierr;}
         value=1/sqrt(referenceZ);
         if (i >= invRootZblow && i < invRootZbhigh) {ierr=MatSetValue(invRootZb,i,i,value,INSERT_VALUES); if (ierr) return ierr;}
         i++;
      }
   }

   if (MIXEDMODE) {
      PetscInt i=0;
      while (i < (PetscInt)modeList.size()) {
         value=1/sqrt(referenceZ);
         if (i >= invRootZalow && i < invRootZahigh) {ierr=MatSetValue(invRootZa,i,i,value,INSERT_VALUES); if (ierr) return ierr;}
         value=1/sqrt(referenceZ);
         if (i >= invRootZblow && i < invRootZbhigh) {ierr=MatSetValue(invRootZb,i,i,value,INSERT_VALUES); if (ierr) return ierr;} // default to single-ended
         i++;
      }

      i=0;
      while (i < (PetscInt)differentialPairList->size()) {
         PetscInt p=(*differentialPairList)[i]->get_Sport_P()-1;
         PetscInt n=(*differentialPairList)[i]->get_Sport_N()-1;

         // set to mixed mode
         value=1/sqrt(Zoe);
         if (p >= invRootZblow && p < invRootZbhigh) {ierr=MatSetValue(invRootZb,p,p,value,INSERT_VALUES); if (ierr) return ierr;} // common mode
         value=1/sqrt(Zoo);
         if (n >= invRootZblow && n < invRootZbhigh) {ierr=MatSetValue(invRootZb,n,n,value,INSERT_VALUES); if (ierr) return ierr;} // differential mode
         i++;
      }
   }

   ierr=MatAssemblyBegin(invRootZa,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   ierr=MatAssemblyEnd(invRootZa,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;

   ierr=MatAssemblyBegin(invRootZb,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   ierr=MatAssemblyEnd(invRootZb,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;

   // Kv and Ki

   // Note: From OpenParEM2D for line calculations, Ti/Tv goes from single-ended voltages to modal voltages.

   if (SINGLEENDED) {
      PetscInt i=0;
      while (i < (PetscInt)portList.size()) {

         // select the row from Ti/Tv to insert
         // ports may be in sequential order
         int row=-1;
         PetscInt j=0;
         while (j <= i) {
            if (portList[j] == portList[i]) row++;
            j++;
         }

         // fill Ki/Kv from Ti/Tv

         int TiTvSize=portList[i]->get_TiTvSize();
         j=0;
         while (j < (PetscInt) TiTvSize) {

            // get the mode number for this TiTv value
            int count=-1;
            PetscInt k=0;
            while (k < (PetscInt)portList.size()) {
               if (portList[k] == portList[i]) count++;
               if (count == (int)j) break;
               k++;
            }

            value=portList[i]->get_ReTi(row,j)+PETSC_i*portList[i]->get_ImTi(row,j);
            if (i >= Kilow && i < Kihigh) {ierr=MatSetValue(Ki,i,k,value,INSERT_VALUES); if (ierr) return ierr;} 

            value=portList[i]->get_ReTv(row,j)+PETSC_i*portList[i]->get_ImTv(row,j);
            if (i >= Kvlow && i < Kvhigh) {ierr=MatSetValue(Kv,i,k,value,INSERT_VALUES); if (ierr) return ierr;}

            j++;
         }
         i++;
      }
   }

   if (MIXEDMODE) {

      // default for single ended
      PetscInt i=0;
      while (i < (PetscInt)modeList.size()) {
         value=1;
         if (i >= Kvlow && i < Kvhigh) {ierr=MatSetValue(Kv,i,i,value,INSERT_VALUES); if (ierr) return ierr;}
         value=1;
         if (i >= Kilow && i < Kihigh) {ierr=MatSetValue(Ki,i,i,value,INSERT_VALUES); if (ierr) return ierr;}
         i++;
      }

      // mixed mode
      i=0;
      while (i < (PetscInt)differentialPairList->size()) {
         PetscInt p=(*differentialPairList)[i]->get_Sport_P()-1;
         PetscInt n=(*differentialPairList)[i]->get_Sport_N()-1;

         // common-mode voltage
         value=0.5;
         if (p >= Kvlow && p < Kvhigh) {ierr=MatSetValue(Kv,p,p,value,INSERT_VALUES); if (ierr) return ierr;}
         value=0.5;
         if (p >= Kvlow && p < Kvhigh) {ierr=MatSetValue(Kv,p,n,value,INSERT_VALUES); if (ierr) return ierr;}

         // differential voltage
         value=+1;
         if (n >= Kvlow && n < Kvhigh) {ierr=MatSetValue(Kv,n,p,value,INSERT_VALUES); if (ierr) return ierr;}
         value=-1;
         if (n >= Kvlow && n < Kvhigh) {ierr=MatSetValue(Kv,n,n,value,INSERT_VALUES); if (ierr) return ierr;}

         // common-mode current
         value=1;
         if (p >= Kilow && p < Kihigh) {ierr=MatSetValue(Ki,p,p,value,INSERT_VALUES); if (ierr) return ierr;}
         value=1;
         if (p >= Kilow && p < Kihigh) {ierr=MatSetValue(Ki,p,n,value,INSERT_VALUES); if (ierr) return ierr;}

         // differential current
         value=+0.5;
         if (n >= Kilow && n < Kihigh) {ierr=MatSetValue(Ki,n,p,value,INSERT_VALUES); if (ierr) return ierr;}
         value=-0.5;
         if (n >= Kilow && n < Kihigh) {ierr=MatSetValue(Ki,n,n,value,INSERT_VALUES); if (ierr) return ierr;}

         i++;
      }
   }

   ierr=MatAssemblyBegin(Kv,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   ierr=MatAssemblyEnd(Kv,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;

   ierr=MatAssemblyBegin(Ki,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   ierr=MatAssemblyEnd(Ki,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;

   // The calculation is going from modal to single-ended, so invert since
   // Ti/Tv goes from single-ended to modal.
   if (SINGLEENDED) {
      ierr=MatInvert(&Kv,0); if (ierr) return ierr;
      ierr=MatInvert(&Ki,0); if (ierr) return ierr;
   }

   // intermediate terms

   Mat term1a,term1;
   ierr=MatMatMult(invRootZb,Kv,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&term1a); if (ierr) return ierr;
   ierr=MatMatMult(term1a,rootZa,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&term1); if (ierr) return ierr;
   ierr=MatDestroy(&term1a);  if (ierr) return ierr;

   Mat term2a,term2;
   ierr=MatMatMult(rootZb,Ki,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&term2a); if (ierr) return ierr;
   ierr=MatMatMult(term2a,invRootZa,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&term2); if (ierr) return ierr;
   ierr=MatDestroy(&term2a);  if (ierr) return ierr;

   // Ms
   ierr=MatConvert(term1,MATSAME,MAT_INITIAL_MATRIX,&Ms); if (ierr) return ierr;
   ierr=MatAXPY(Ms,1,term2,SAME_NONZERO_PATTERN); if (ierr) return ierr;
   ierr=MatScale(Ms,0.5); if (ierr) return ierr;

   // Mc
   ierr=MatConvert(term1,MATSAME,MAT_INITIAL_MATRIX,&Mc); if (ierr) return ierr;
   ierr=MatAXPY(Mc,-1,term2,SAME_NONZERO_PATTERN); if (ierr) return ierr;
   ierr=MatScale(Mc,0.5); if (ierr) return ierr;

   // clean up
   ierr=MatDestroy(&Ki); if (ierr) return ierr;
   ierr=MatDestroy(&Kv); if (ierr) return ierr;
   ierr=MatDestroy(&rootZa); if (ierr) return ierr;
   ierr=MatDestroy(&rootZb); if (ierr) return ierr;
   ierr=MatDestroy(&invRootZa); if (ierr) return ierr;
   ierr=MatDestroy(&invRootZb); if (ierr) return ierr;
   ierr=MatDestroy(&term1); if (ierr) return ierr;
   ierr=MatDestroy(&term2); if (ierr) return ierr;

   return ierr;
}

void fem3D::calculateRadiationCurrents (BoundaryDatabase *boundaryDatabase)
{
   // get the far-field wave impedance

   wave_impedance=0;
   Array<int> ess_bdr_rad;
   ess_bdr_rad.SetSize((*pmesh)->bdr_attributes.Max());
   ess_bdr_rad=0;       // disable all
   long unsigned int j=0;
   while (j < boundaryDatabase->get_boundaryListSize()) {
      Boundary *boundary=boundaryDatabase->get_boundary(j);
      if (boundary->is_radiation()) {
         ess_bdr_rad[boundary->get_attribute()-1]=1;
         wave_impedance=boundary->get_wave_impedance();  // already verified that all have the same wave impedance
      }
      j++;
   }

   if (wave_impedance == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"fem3D::calculateRadiationCurrents: ASSERT: Cannot calculate radiation patterns since no radiation boundary conditions are defined.\n");
      return;
   }

   // beta assuming relative permeability = 1
   radiation_beta=2*M_PI*frequency*4e-7*M_PI/wave_impedance;

   // wavelength assuming relative permeability = 1
   double lambda=wave_impedance/(4e-7*M_PI*frequency);

   // length of r
   radius=1e5*lambda;

   // mesh limits
   Vector lowerLeft,upperRight;
   (*pmesh)->GetBoundingBox(lowerLeft,upperRight,max(projData->fem_order,1));

   // find a center
   Vector center(3);
   center(0)=(lowerLeft(0)+upperRight(0))/2;
   center(1)=(lowerLeft(1)+upperRight(1))/2;
   center(2)=(lowerLeft(2)+upperRight(2))/2;

   // surface currents on the radiation boundaries
   boundaryDatabase->calculateRadiationCurrents(*pmesh,projData,center,radiation_beta,projData->antenna_plot_current_resolution,gridReE,gridImE,gridReH,gridImH);
   boundaryDatabase->collectRadiationCurrents();
}

void fem3D::calculateRadiationPatterns (int Sport, complex<double> acceptedPower, int iteration, BoundaryDatabase *boundaryDatabase, PatternDatabase *patternDatabase)
{
   // mesh limits
   Vector lowerLeft,upperRight;
   (*pmesh)->GetBoundingBox(lowerLeft,upperRight,max(projData->fem_order,1));

   // find a center
   Vector center(3);
   center(0)=(lowerLeft(0)+upperRight(0))/2;
   center(1)=(lowerLeft(1)+upperRight(1))/2;
   center(2)=(lowerLeft(2)+upperRight(2))/2;

   // increment around the circle
   int nPoints=360/projData->antenna_plot_2D_resolution+0.5;

   // cycle through the patterns
   int i=0;
   while (i < projData->inputAntennaPatternsCount) {

      // convert inputs to strings
      string quantity1="";
      if (projData->inputAntennaPatterns[i].quantity1) quantity1=projData->inputAntennaPatterns[i].quantity1;
      string quantity2="";
      if (projData->inputAntennaPatterns[i].quantity2) quantity2=projData->inputAntennaPatterns[i].quantity2;
      string plane="";
      if (projData->inputAntennaPatterns[i].plane) plane=projData->inputAntennaPatterns[i].plane;

      if (projData->inputAntennaPatterns[i].dim == 2) {

         // get the fields on the circle
         Circle *circle=new Circle();
         circle->set_radius(radius);
         circle->set_center(center(0),center(1),center(2));
         circle->set_plane(plane);
         circle->set_angles(projData->inputAntennaPatterns[i].theta,
                            projData->inputAntennaPatterns[i].phi,
                            projData->inputAntennaPatterns[i].latitude,
                            projData->inputAntennaPatterns[i].rotation);
         circle->set_nPoints(nPoints);

        // see if this circle already exists
        Circle *test=patternDatabase->get_circle(frequency,Sport,iteration,circle);
        if (test) {
           // re-use the existing data
           delete circle;
           circle=test;
        } else {
           // create the circle and compute the fields
           circle->create();
           boundaryDatabase->calculateFarField(radius,center,radiation_beta,wave_impedance,circle->get_pointList());
        }

         // save
         patternDatabase->addPattern(frequency,Sport,iteration,acceptedPower,2,quantity1,quantity2,plane,
                                     projData->inputAntennaPatterns[i].theta,projData->inputAntennaPatterns[i].phi,
                                     projData->inputAntennaPatterns[i].latitude,projData->inputAntennaPatterns[i].rotation,
                                     circle,nullptr);
      } else {

         // get the fields on the sphere

         Sphere *sphere=new Sphere();
         sphere->set_radius(radius);
         sphere->set_center (center(0),center(1),center(2));

         // see if this sphere already exists
         Sphere *test=patternDatabase->get_sphere(frequency,Sport,iteration,sphere);
         if (test) {
            // re-use the existing data
            delete sphere;
            sphere=test;
         } else {
            sphere->create();
            sphere->refine(projData->antenna_plot_3D_refinement);
            sphere->findNeighbors();
            //sphere->checkNeighbors();
            sphere->setMetrics();
            sphere->allocateAreasToPoints();
            boundaryDatabase->calculateFarField(radius,center,radiation_beta,wave_impedance,sphere->get_pointList());
         }

         // save
         patternDatabase->addPattern(frequency,Sport,iteration,acceptedPower,3,quantity1,quantity2,plane,0,0,0,0,nullptr,sphere);
     }

      i++;
   }

   patternDatabase->calculateTotalArea(frequency,iteration);
   patternDatabase->populate_sphere(frequency,Sport,iteration);

   patternDatabase->calculateRadiatedPower(frequency,iteration);
   patternDatabase->calculateIsotropicGain(frequency,iteration);
   patternDatabase->calculateDirectivity(frequency,iteration);
   patternDatabase->calculateRadiationEfficiency(frequency,iteration);

   boundaryDatabase->deleteRadiationCurrents ();
}

fem3D::~fem3D()
{
   if (fespace_ND) {delete fespace_ND; fespace_ND=nullptr;}
   if (fespace_RT) {delete fespace_RT; fespace_RT=nullptr;}
   if (fespace_H1) {delete fespace_H1; fespace_H1=nullptr;}
   if (fespace_L2) {delete fespace_L2; fespace_L2=nullptr;}
   if (fec_ND) {delete fec_ND; fec_ND=nullptr;}
   if (fec_RT) {delete fec_RT; fec_RT=nullptr;}
   if (fec_H1) {delete fec_H1; fec_H1=nullptr;}
   if (fec_L2) {delete fec_L2; fec_L2=nullptr;}

   if (e_re) {delete e_re; e_re=nullptr;}
   if (e_im) {delete e_im; e_im=nullptr;}
   if (h_re) {delete h_re; h_re=nullptr;}
   if (h_im) {delete h_im; h_im=nullptr;}

   if (gridReE) {delete gridReE; gridReE=nullptr;}
   if (gridImE) {delete gridImE; gridImE=nullptr;}
   if (gridReH) {delete gridReH; gridReH=nullptr;}
   if (gridImH) {delete gridImH; gridImH=nullptr;}
   if (gridReExH) {delete gridReExH; gridReExH=nullptr;}
   if (gridImExH) {delete gridImExH; gridImExH=nullptr;}

//   if (gridReEz) {delete gridReEz; gridReEz=nullptr;}
//   if (gridImEz) {delete gridImEz; gridImEz=nullptr;}
//   if (gridReHz) {delete gridReHz; gridReHz=nullptr;}
//   if (gridImHz) {delete gridImHz; gridImHz=nullptr;}

   MatDestroy(&Mc);
   MatDestroy(&Ms);
}

