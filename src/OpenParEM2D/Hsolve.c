////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM3D - A fullwave 2D electromagnetic simulator.                  //
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

#include "Hsolve.h"

PetscErrorCode printMatInfo (const char *mat_name, Mat *mat)
{
   MatInfo info;
   PetscInt m,n;
   MatType type;

   if (MatGetInfo(*mat,MAT_GLOBAL_SUM,&info)) return 1;
   if (MatGetSize(*mat,&m,&n)) return 1;
   if (MatGetType(*mat,&type)) return 1;

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s:\n",mat_name);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   rows=%ld, columns=%ld\n",m,n);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   type=%s\n",type);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   block_size=%g\n",info.block_size);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   nz_allocated=%g\n",info.nz_allocated);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   nz_used=%g\n",info.nz_used);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   nz_unneeded=%g\n",info.nz_unneeded);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   memory_allocated=%g\n",info.memory);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   number_of_assemblies=%g\n",info.assemblies);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   mallocs=%g\n",info.mallocs);

   return 0;
}

PetscErrorCode convertToIdentity (Mat *mat)
{
   PetscInt rows,cols;
   int i,j;

   if (MatGetSize(*mat,&rows,&cols)) return 1;
   i=0;
   while (i < rows) {
      j=0;
      while (j < cols) {
         if (MatSetValue(*mat, i, j, 0.0, INSERT_VALUES)) return 1;
         if (i == j) {
            if (MatSetValue(*mat, i, j, 1.0, INSERT_VALUES)) return 1;
         }
         j++;
      }
      i++;
   }

   if (MatAssemblyBegin(*mat,MAT_FINAL_ASSEMBLY)) return 1;
   if (MatAssemblyEnd(*mat,MAT_FINAL_ASSEMBLY)) return 1;

   return 0;
}

// insert A into B at location rowOffset, colOffset
PetscErrorCode InsertSubMatrix (Mat *A, Mat *B, PetscInt rowOffset, PetscInt colOffset, int show_stats, int show_data)
{
   PetscInt i,j,nc;
   const PetscInt *aj;
   const PetscScalar *aa;
   PetscInt Arows;
   PetscInt offsetRow[1];
   PetscInt *offsetCol;

   if (show_stats) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"InsertSubMatrix: rowOffset=%ld colOffset=%ld\n",rowOffset,colOffset);
      printMatInfo ("MatInfo on A:",A);
      if (show_data) {if (MatView(*A,PETSC_VIEWER_STDOUT_WORLD)) return 1;}
   }

   // rows
   if (MatGetSize(*A,&Arows,NULL)) return 1;
   i=0;
   while (i < Arows) {
      if (MatGetRow(*A,i,&nc,&aj,&aa)) return 1;

      // offset the columns
      if (PetscMalloc(nc*sizeof(PetscInt),&offsetCol)) return 1;
      j=0;
      while (j < nc) {
         offsetCol[j]=aj[j]+colOffset;
         j++;
      }

      offsetRow[0]=i+rowOffset;
      if (MatSetValues(*B,1,offsetRow,nc,offsetCol,aa,INSERT_VALUES)) return 1;
      if (MatRestoreRow(*A,i,&nc,&aj,&aa)) return 1;
      if (PetscFree(offsetCol)) return 1;
      i++;
   }

   if (show_stats) {
      printMatInfo ("MatInfo on B:",B);
      if (show_data) {if (MatView(*B,PETSC_VIEWER_STDOUT_WORLD)) return 1;}
   }

   return 0;
}

// the basic matrices are only dependent on frequency
int Hsetup (struct projectData *projData, Mat *Mt, Mat *Cz, Mat *Zt, Mat *Mz, Mat *Ct)
{
   PetscInt ioffset,joffset;
   int location,transpose,sign;
   int iMt,iCz,iZt,iMz,iCt;
   char *resultsDir;
   long int MtHeight,MtWidth,MtSparseWidth;
   long int CzHeight,CzWidth,CzSparseWidth;
   long int ZtHeight,ZtWidth,ZtSparseWidth;
   long int MzHeight,MzWidth,MzSparseWidth;
   long int CtHeight,CtWidth,CtSparseWidth;
   PetscMPIInt rank,size;

   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   if (projData->output_show_postprocessing) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"            setting up for H field calculation ...\n");}

   // directory to hold results
   resultsDir=(char *) malloc ((5+strlen(projData->project_name)+1+1)*sizeof(char));
   sprintf (resultsDir,"temp_%s/",projData->project_name);  // align this with OpenParEM2D.cpp, eigensolve.c, Hsolve.c, and Fields::saveFields

   // run through the files to get counts for allocating matrices

   int fail=0;
   if (rank == 0) {
      if (loadDataFileStats("Mt_mat",resultsDir,projData->project_name,&MtHeight,&MtWidth,&MtSparseWidth) != 0) {
         prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2151: Failed to scan \"Mt_mat\".\n");
         fail=1;
      }

      if (loadDataFileStats("Cz_mat",resultsDir,projData->project_name,&CzHeight,&CzWidth,&CzSparseWidth) != 0) {
         prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2152: Failed to scan \"Cz_mat\".\n");
         fail=1;
      }

      if (loadDataFileStats("Zt_mat",resultsDir,projData->project_name,&ZtHeight,&ZtWidth,&ZtSparseWidth) != 0) {
         prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2153: Failed to scan \"Zt_mat\".\n");
         fail=1;
      }

      if (loadDataFileStats("Mz_mat",resultsDir,projData->project_name,&MzHeight,&MzWidth,&MzSparseWidth) != 0) {
         prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2154: Failed to scan \"Mz_mat\".\n");
         fail=1;
      }

      if (loadDataFileStats("Ct_mat",resultsDir,projData->project_name,&CtHeight,&CtWidth,&CtSparseWidth) != 0) {
         prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2155: Failed to scan \"Ct_mat\".\n");
         fail=1;
      }

      int i=1;
      while (i < size) {
         MPI_Send(&fail,1,MPI_INT,i,2,PETSC_COMM_WORLD);
         i++;
      }
   } else {
      MPI_Recv(&fail,1,MPI_INT,0,2,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   }
   if (fail) return 1;

   if (MPI_Bcast(&MtHeight,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;
   if (MPI_Bcast(&MtWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;
   if (MPI_Bcast(&MtSparseWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;

   if (MPI_Bcast(&CzHeight,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;
   if (MPI_Bcast(&CzWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;
   if (MPI_Bcast(&CzSparseWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;

   if (MPI_Bcast(&ZtHeight,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;
   if (MPI_Bcast(&ZtWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;
   if (MPI_Bcast(&ZtSparseWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;

   if (MPI_Bcast(&MzHeight,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;
   if (MPI_Bcast(&MzWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;
   if (MPI_Bcast(&MzSparseWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;

   if (MPI_Bcast(&CtHeight,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;
   if (MPI_Bcast(&CtWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;
   if (MPI_Bcast(&CtSparseWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) return 1;

   if (MPI_Barrier(PETSC_COMM_WORLD)) return 1;

   //prefix(); PetscPrintf (PETSC_COMM_WORLD,"MtHeight=%zu  MtWidth=%zu  MtSparseWidth=%zu\n",MtHeight,MtWidth,MtSparseWidth);
   //prefix(); PetscPrintf (PETSC_COMM_WORLD,"CzHeight=%zu  CzWidth=%zu  CzSparseWidth=%zu\n",CzHeight,CzWidth,CzSparseWidth);
   //prefix(); PetscPrintf (PETSC_COMM_WORLD,"ZtHeight=%zu  ZtWidth=%zu  ZtSparseWidth=%zu\n",ZtHeight,ZtWidth,ZtSparseWidth);
   //prefix(); PetscPrintf (PETSC_COMM_WORLD,"MzHeight=%zu  MzWidth=%zu  MzSparseWidth=%zu\n",MzHeight,MzWidth,MzSparseWidth);
   //prefix(); PetscPrintf (PETSC_COMM_WORLD,"CtHeight=%zu  CtWidth=%zu  CtSparseWidth=%zu\n",CtHeight,CtWidth,CtSparseWidth);
   //prefix(); PetscPrintf (PETSC_COMM_WORLD,"matrix size: Ht=%zu, Hz=%zu\n",MtHeight,MzHeight);

   // Build up the matrics for the standard linear problem Ax=b.

   // Mt
   if (MatCreate(PETSC_COMM_WORLD,Mt)) return 1;
   if (MatSetSizes(*Mt,PETSC_DECIDE,PETSC_DECIDE,MtHeight,MtWidth)) return 1;
   if (MatSetFromOptions(*Mt)) return 1;
   if (MatSeqAIJSetPreallocation(*Mt,MtSparseWidth,NULL)) return 1;
   if (MatMPIAIJSetPreallocation(*Mt,MtSparseWidth,NULL,MtSparseWidth,NULL)) return 1;
   if (MatZeroEntries(*Mt)) return 1;

   ioffset=0; joffset=0; location=1; transpose=0; sign=0;
   iMt=loadDataFile ("Mt_mat",resultsDir,projData->project_name,Mt,ioffset,joffset,location,transpose,sign,rank);
   if (iMt) {prefix(); if (PetscPrintf (PETSC_COMM_WORLD,"ERROR2156: Failed to load \"Mt_mat\" data file.\n")) return 1;}

   if (MatAssemblyBegin(*Mt,MAT_FINAL_ASSEMBLY)) return 1;
   if (MatAssemblyEnd(*Mt,MAT_FINAL_ASSEMBLY)) return 1;

   // Cz
   if (MatCreate(PETSC_COMM_WORLD,Cz)) return 1;
   if (MatSetSizes(*Cz,PETSC_DECIDE,PETSC_DECIDE,MtHeight,CzWidth)) return 1;
   if (MatSetFromOptions(*Cz)) return 1;
   if (MatSeqAIJSetPreallocation(*Cz,CzSparseWidth,NULL)) return 1;
   if (MatMPIAIJSetPreallocation(*Cz,CzSparseWidth,NULL,CzSparseWidth,NULL)) return 1;
   if (MatZeroEntries(*Cz)) return 1;

   ioffset=0; joffset=0; location=0; transpose=0; sign=0;
   iCz=loadDataFile ("Cz_mat",resultsDir,projData->project_name,Cz,ioffset,joffset,location,transpose,sign,rank);
   if (iCz) {prefix(); if (PetscPrintf (PETSC_COMM_WORLD,"ERROR2157: Failed to load \"Cz_mat\" data file.\n")) return 1;}

   if (MatAssemblyBegin(*Cz,MAT_FINAL_ASSEMBLY)) return 1;
   if (MatAssemblyEnd(*Cz,MAT_FINAL_ASSEMBLY)) return 1;

   // Zt
   if (MatCreate(PETSC_COMM_WORLD,Zt)) return 1;
   if (MatSetSizes(*Zt,PETSC_DECIDE,PETSC_DECIDE,ZtHeight,ZtWidth)) return 1;
   if (MatSetFromOptions(*Zt)) return 1;
   if (MatSeqAIJSetPreallocation(*Zt,ZtSparseWidth,NULL)) return 1;
   if (MatMPIAIJSetPreallocation(*Zt,ZtSparseWidth,NULL,ZtSparseWidth,NULL)) return 1;
   if (MatZeroEntries(*Zt)) return 1;

   ioffset=0; joffset=0; location=0; transpose=0; sign=0;
   iZt=loadDataFile ("Zt_mat",resultsDir,projData->project_name,Zt,ioffset,joffset,location,transpose,sign,rank);
   if (iZt) {prefix(); if (PetscPrintf (PETSC_COMM_WORLD,"ERROR2158: Failed to load \"Zt_mat\" data file.\n")) return 1;}

   if (MatAssemblyBegin(*Zt,MAT_FINAL_ASSEMBLY)) return 1;
   if (MatAssemblyEnd(*Zt,MAT_FINAL_ASSEMBLY)) return 1;

   // Mz
   if (MatCreate(PETSC_COMM_WORLD,Mz)) return 1;
   if (MatSetSizes(*Mz,PETSC_DECIDE,PETSC_DECIDE,MzHeight,MzWidth)) return 1;
   if (MatSetFromOptions(*Mz)) return 1;
   if (MatSeqAIJSetPreallocation(*Mz,MzSparseWidth,NULL)) return 1;
   if (MatMPIAIJSetPreallocation(*Mz,MzSparseWidth,NULL,MzSparseWidth,NULL)) return 1;
   if (MatZeroEntries(*Mz)) return 1;

   ioffset=0; joffset=0; location=1; transpose=0; sign=0;
   iMz=loadDataFile ("Mz_mat",resultsDir,projData->project_name,Mz,ioffset,joffset,location,transpose,sign,rank);
   if (iMz) {prefix(); if (PetscPrintf (PETSC_COMM_WORLD,"ERROR2159: Failed to load \"Mz_mat\" data file.\n")) return 1;}

   if (MatAssemblyBegin(*Mz,MAT_FINAL_ASSEMBLY)) return 1;
   if (MatAssemblyEnd(*Mz,MAT_FINAL_ASSEMBLY)) return 1;

   // Ct
   if (MatCreate(PETSC_COMM_WORLD,Ct)) return 1;
   if (MatSetSizes(*Ct,PETSC_DECIDE,PETSC_DECIDE,CtHeight,CtWidth)) return 1;
   if (MatSetFromOptions(*Ct)) return 1;
   if (MatSeqAIJSetPreallocation(*Ct,CtSparseWidth,NULL)) return 1;
   if (MatMPIAIJSetPreallocation(*Ct,CtSparseWidth,NULL,CtSparseWidth,NULL)) return 1;
   if (MatZeroEntries(*Ct)) return 1;

   ioffset=0; joffset=0; location=0; transpose=0; sign=1;
   iCt=loadDataFile ("Ct_mat",resultsDir,projData->project_name,Ct,ioffset,joffset,location,transpose,sign,rank);
   if (iCt) {prefix(); if (PetscPrintf (PETSC_COMM_WORLD,"ERROR2160: Failed to load \"Ct_mat\" data file.\n")) return 1;}
   if (MatAssemblyBegin(*Ct,MAT_FINAL_ASSEMBLY)) return 1;
   if (MatAssemblyEnd(*Ct,MAT_FINAL_ASSEMBLY)) return 1;

   free(resultsDir);

   return 0;
}

void convergence (struct projectData *projData, KSPConvergedReason reason)
{
   if (projData->output_show_postprocessing) {
      if (reason < 0) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"                  NOT CONVERGED: ");}
      else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"                  Converged: ");}

      if (reason == KSP_CONVERGED_ITERATING) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"CONVERGED_ITERATING");}
      if (reason == KSP_CONVERGED_RTOL_NORMAL) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"RTOL_NORMAL");}
      if (reason == KSP_CONVERGED_ATOL_NORMAL) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ATOL_NORMAL");}
      if (reason == KSP_CONVERGED_RTOL) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"RTOL");}
      if (reason == KSP_CONVERGED_ATOL) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ATOL");}
      if (reason == KSP_CONVERGED_ITS) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ITS");}
      if (reason == KSP_CONVERGED_NEG_CURVE) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"NEG_CURVE");}
      if (reason == KSP_CONVERGED_STEP_LENGTH) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"STEP_LENGTH");}
      if (reason == KSP_CONVERGED_HAPPY_BREAKDOWN) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"HAPPY_BREAKDOWN");}
      if (reason == KSP_DIVERGED_NULL) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"NULL");}
      if (reason == KSP_DIVERGED_ITS) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ITS");}
      if (reason == KSP_DIVERGED_DTOL) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"DTOL");}
      if (reason == KSP_DIVERGED_BREAKDOWN) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"BREAKDOWN - Generic breakdown during solution.");}
      if (reason == KSP_DIVERGED_BREAKDOWN_BICG) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"BREAKDOWN_BICG");}
      if (reason == KSP_DIVERGED_NONSYMMETRIC) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"NONSYMMETRIC");}
      if (reason == KSP_DIVERGED_INDEFINITE_PC) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"INDEFINITE_PC");}
      if (reason == KSP_DIVERGED_NANORINF) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"NANORINF");}
      if (reason == KSP_DIVERGED_INDEFINITE_MAT) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"INDEFINITE_MAT");}
      if (reason == KSP_DIVERGED_PC_FAILED) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"PC_FAILED - Could not build or use the requested preconditioner.");}
   } else {
      if (reason < 0) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"           Hfield NOT CONVERGED");}
   }
}

// split out a subvector of A from low to high-1 and return in B starting at index 0
// B is allocated and must be destroyed elsewhere
int VecSplit (Vec *A, PetscInt low, PetscInt high, Vec *B)
{
   PetscInt i;
   PetscInt *idx_from,low_from,high_from,count_from;
   PetscInt *idx_to,low_to,high_to,count_to;
   PetscScalar *data_from,*data_to;
   struct mpi_complex_int *sendData,*recvData;
   PetscMPIInt size,rank;

   MPI_Comm_size(PETSC_COMM_WORLD, &size);
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   // set up a datatype for data transfer
   // use an int for an array location then two doubles for a complex value

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

   // space to hold transfer lengths and displacements
   int *counts_recv,*displacements_recv;
   counts_recv=(int *)malloc(size*sizeof(int));
   displacements_recv=(int *)malloc(size*sizeof(int));

   // data from A
   if (VecGetOwnershipRange(*A,&low_from,&high_from)) return 1;
   if (PetscMalloc((high-low)*sizeof(PetscInt),&idx_from)) return 1;
   if (PetscMalloc((high-low)*sizeof(PetscScalar),&data_from)) return 1;
   count_from=0;
   i=low;
   while (i < high) {
      if (i >= low_from && i < high_from) {
         idx_from[count_from]=i;
         count_from++;
      }
      i++;
   }

   // gather the transfer lengths
   if (MPI_Gather(&count_from,1,MPI_INT,counts_recv,1,MPI_INT,0,PETSC_COMM_WORLD)) {
      prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2161: Failed to gather data.\n");
      return 1;
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

   // get the data
   if (VecGetValues(*A,count_from,idx_from,data_from)) return 1;

   // assemble the data for transfer
   if (PetscMalloc(count_from*sizeof(struct mpi_complex_int),&sendData)) return 1;
   i=0;
   while (i < count_from) {
      sendData[i].location=idx_from[i];
      sendData[i].real=PetscRealPart(data_from[i]);
      sendData[i].imag=PetscImaginaryPart(data_from[i]);
      i++;
   }
   if (PetscFree(data_from)) return 1;
   if (PetscFree(idx_from)) return 1;

   // space for the received data
   if (PetscMalloc((high-low)*sizeof(struct mpi_complex_int),&recvData)) return 1;

   // send all to rank 0
   if (MPI_Gatherv(sendData,count_from,mpi_complex_int_type,recvData,counts_recv,displacements_recv,mpi_complex_int_type,0,PETSC_COMM_WORLD)) {
      prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2162: Failed to gather data.\n");
      return 1;
   }

   if (counts_recv) {free(counts_recv); counts_recv=NULL;}
   if (displacements_recv) {free(displacements_recv); displacements_recv=NULL;}
    if (PetscFree(sendData)) return 1; 

   // broadcast recvData
   if (MPI_Bcast(recvData,high-low,mpi_complex_int_type,0,PETSC_COMM_WORLD)) {
      prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2163: Failed to broadcast data.\n");
      return 1;
   }

   // transfer data to B
    if (VecCreate(PETSC_COMM_WORLD,B)) return 1;
    if (VecSetType(*B,VECSTANDARD)) return 1;
    if (VecSetSizes(*B,PETSC_DECIDE,high-low)) return 1;

    if (VecGetOwnershipRange(*B,&low_to,&high_to)) return 1;
    if (PetscMalloc((high_to-low_to)*sizeof(PetscInt),&idx_to)) return 1;
    if (PetscMalloc((high_to-low_to)*sizeof(PetscScalar),&data_to)) return 1;

   // setup for transfer
   count_to=0;
   i=0;
   while (i < high-low) {
      if (recvData[i].location-low >= low_to && recvData[i].location-low < high_to) {
         idx_to[count_to]=recvData[i].location-low;
         data_to[count_to]=recvData[i].real+PETSC_i*recvData[i].imag;
         count_to++;
      }
      i++;
   }

   // transfer
    if (VecSetValues(*B,count_to,idx_to,data_to,INSERT_VALUES)) return 1;
    if (VecAssemblyBegin(*B)) return 1;
    if (VecAssemblyEnd(*B)) return 1;

   // cleanup
    if (PetscFree(recvData)) return 1;
    if (PetscFree(idx_to)) return 1;
    if (PetscFree(data_to)) return 1;
   MPI_Type_free(&mpi_complex_int_type);

   return 0;
}

// Solve Ax=b
int Hsolve (struct projectData *projData, Mat *Mt, Mat *Cz, Mat *Zt, Mat *Mz, Mat *Ct,
            PetscInt EtSize, PetscInt EzSize, Vec *Efield, PetscScalar *gamma, Vec *Hfield, PetscMPIInt rank)
{
   Mat Ztgamma;
   Vec Et,Ez;
   Vec Ht,Hz;
   Vec btt,btz,bz;
   KSP ksp;
   PetscInt its,maxits;
   PetscReal rtol,atol,dtol;
   KSPConvergedReason reason;
   //PC pc;

   if (projData->output_show_postprocessing) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"               solving for H field ...\n");}

   // space for Ht and Hz

    if (VecCreate(PETSC_COMM_WORLD,&Ht)) return 1;
    if (VecSetType(Ht,VECSTANDARD)) return 1;
    if (VecSetSizes(Ht,PETSC_DECIDE,EtSize)) return 1;

    if (VecCreate(PETSC_COMM_WORLD,&Hz)) return 1;
    if (VecSetType(Hz,VECSTANDARD)) return 1;
    if (VecSetSizes(Hz,PETSC_DECIDE,EzSize)) return 1;

   // split out Et from Efield
   if (VecSplit(Efield,0,EtSize,&Et)) return 1;

   // split out Ez from Efield
   if (VecSplit(Efield,EtSize,EtSize+EzSize,&Ez)) return 1;

   //*******************************************************************************************
   // Ht
   //*******************************************************************************************

   // copy Zt and scale by gamma
    if (MatDuplicate(*Zt, MAT_COPY_VALUES, &Ztgamma)) return 1;
    if (MatScale(Ztgamma, *gamma)) return 1;

   // calculate btt

    if (VecCreate(PETSC_COMM_WORLD,&btt)) return 1;
    if (VecSetType(btt,VECSTANDARD)) return 1;
    if (VecSetSizes(btt,PETSC_DECIDE,EtSize)) return 1;

    if (MatMult(Ztgamma, Et, btt)) return 1;

    if (VecAssemblyBegin(btt)) return 1;
    if (VecAssemblyEnd(btt)) return 1;

   // calculate btz

    if (VecCreate(PETSC_COMM_WORLD,&btz)) return 1;
    if (VecSetType(btz,VECSTANDARD)) return 1;
    if (VecSetSizes(btz,PETSC_DECIDE,EtSize)) return 1;

    if (MatMult(*Cz, Ez, btz)) return 1;

    if (VecAssemblyBegin(btz)) return 1;
    if (VecAssemblyEnd(btz)) return 1;

   // get the sum
   if (VecAXPY(btt,1,btz)) return 1;

   // solve Ax=b

   // setup
    if (KSPCreate(PETSC_COMM_WORLD, &ksp)) return 1;
   // if (KSPSetType(ksp,KSPLSQR)) return 1;
    if (KSPSetOperators(ksp, *Mt, *Mt)) return 1;
   // if (KSPGetPC(ksp,&pc)) return 1;
   // if (PCSetType(pc,PCNONE)) return 1;
    if (KSPSetFromOptions(ksp)) return 1;

   maxits=projData->solution_iteration_limit;       // PETSc default = 1e4
   rtol=projData->solution_tolerance;               // PETSc default = 1e-5
   //atol=projData->solution_tolerance;             // PETSC default = 1e-50
   atol=1e-50;                                      // PETSC default = 1e-50
   dtol=1e5;                                        // PETSC default = 1e5
    if (KSPSetTolerances(ksp, rtol, atol, dtol, maxits)) return 1;

   // solve
    if (KSPSolve(ksp, btt, Ht)) return 1;

   // get stats
    if (KSPGetIterationNumber(ksp, &its)) return 1;
    if (KSPGetConvergedReason(ksp, &reason)) return 1;

   convergence (projData,reason);
   if (projData->output_show_postprocessing) {prefix();  if (PetscPrintf (PETSC_COMM_WORLD,", number of Ht iterations: %ld\n",its)) return 1;}

   // cleanup
    if (MatDestroy(&Ztgamma)) return 1;
    if (VecDestroy(&btt)) return 1;
    if (VecDestroy(&btz)) return 1;
    if (VecDestroy(&Ez)) return 1;
    if (KSPDestroy(&ksp)) return 1;

   //*******************************************************************************************
   // Hz
   //*******************************************************************************************

   // calculate bz

    if (VecCreate(PETSC_COMM_WORLD,&bz)) return 1;
    if (VecSetType(bz,VECSTANDARD)) return 1;
    if (VecSetSizes(bz,PETSC_DECIDE,EzSize)) return 1;

    if (MatMult(*Ct, Et, bz)) return 1;

    if (VecAssemblyBegin(bz)) return 1;
    if (VecAssemblyEnd(bz)) return 1;

   // solve Ax=b

   // setup
    if (KSPCreate(PETSC_COMM_WORLD, &ksp)) return 1;
   // if (KSPSetType(ksp,KSPLSQR)) return 1;
    if (KSPSetOperators(ksp, *Mz, *Mz)) return 1;
   // if (KSPGetPC(ksp,&pc)) return 1;
   // if (PCSetType(pc,PCNONE)) return 1;
    if (KSPSetFromOptions(ksp)) return 1;

   maxits=projData->solution_iteration_limit;       // PETSc default = 1e4
   rtol=projData->solution_tolerance;               // PETSc default = 1e-5
   //atol=projData->solution_tolerance;             // PETSC default = 1e-50
   atol=1e-50;                                      // PETSC default = 1e-50
   dtol=1e5;                                        // PETSC default = 1e5
    if (KSPSetTolerances(ksp, rtol, atol, dtol, maxits)) return 1;

   // solve
    if (KSPSolve(ksp, bz, Hz)) return 1;

   // get stats
    if (KSPGetIterationNumber(ksp, &its)) return 1;
    if (KSPGetConvergedReason(ksp, &reason)) return 1;

   convergence (projData,reason);
   if (projData->output_show_postprocessing) {prefix();  if (PetscPrintf (PETSC_COMM_WORLD,", number of Hz iterations: %ld\n",its)) return 1;}

   // cleanup
    if (VecDestroy(&bz)) return 1;
    if (KSPDestroy(&ksp)) return 1;

   //*******************************************************************************************
   // combine Ht and Hz into one vector
   //*******************************************************************************************

   Vec vecList[2];
   vecList[0]=Ht;
   vecList[1]=Hz;
   if (VecConcatenate(2,vecList,Hfield,NULL)) return 1;  // allocates memory

   // cleanup
    if (VecDestroy(&Et)) return 1;
    if (VecDestroy(&Ez)) return 1;
    if (VecDestroy(&Ht)) return 1;
    if (VecDestroy(&Hz)) return 1;

   return 0;
}

