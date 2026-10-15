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

#include "csr.h"

PetscErrorCode printMatInfo (const char *mat_name, Mat *mat)
{
   PetscErrorCode ierr=0;
   MatInfo info;
   PetscInt m,n;
   MatType type;

   ierr=MatGetInfo(*mat,MAT_GLOBAL_SUM,&info); CHKERRQ(ierr);
   ierr=MatGetSize(*mat,&m,&n); CHKERRQ(ierr);
   ierr=MatGetType(*mat,&type); CHKERRQ(ierr);

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

   return ierr;
}

void show_memory (int show, const char *description)
{
   struct rusage usage;

   if (show) {
      getrusage(RUSAGE_SELF,&usage);
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s memory (MB): %g\n",description,(double)usage.ru_maxrss/1024.);
   }
}

PetscErrorCode hypre_getSparseWidth (hypre_ParCSRMatrix *a, PetscInt *maxSparseWidth)
{
   PetscErrorCode ierr=0;
   PetscMPIInt size,rank;
   MPI_Comm_size(PETSC_COMM_WORLD, &size);
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   PetscInt i,n,sparseWidth;

   HYPRE_Int *diag_i;
   HYPRE_Int *offd_i;
   HYPRE_Int num_nonzeros_offd;

   diag_i=hypre_CSRMatrixI(a->diag);
   num_nonzeros_offd=hypre_CSRMatrixNumNonzeros(a->offd);
   if (num_nonzeros_offd) offd_i=hypre_CSRMatrixI(a->offd);

   sparseWidth=0;
   i=a->first_row_index;
   while (i <= a->last_row_index) {
      n=diag_i[i-a->first_row_index+1]-diag_i[i-a->first_row_index];
      if (num_nonzeros_offd) n+=offd_i[i-a->first_row_index+1]-offd_i[i-a->first_row_index];
      if (n > sparseWidth) sparseWidth=n;
      i++;
   }

   ierr=MPI_Allreduce(&sparseWidth,maxSparseWidth,1,MPI_LONG,MPI_MAX,PETSC_COMM_WORLD);

   return ierr;
}

// Extract one row of the CSR data from the hypre_ParCSRMatrix for putting it into a Petsc Mat.
PetscErrorCode hypre_extractRow (hypre_ParCSRMatrix *matrix, PetscInt row, PetscInt *n, PetscInt **idxn, PetscScalar **v, int imaginary)
{
   PetscErrorCode ierr=0;
   PetscInt j,index;

   HYPRE_BigInt first_col_diag;
   HYPRE_BigInt *col_map_offd;
   HYPRE_Complex *diag_data;
   HYPRE_Int *diag_i;
   HYPRE_Int *diag_j;
   HYPRE_Complex *offd_data;
   HYPRE_Int *offd_i;
   HYPRE_Int *offd_j;
   HYPRE_BigInt J;
   HYPRE_Int num_nonzeros_offd;

   // column count and storage

   diag_data=hypre_CSRMatrixData(matrix->diag);
   diag_i=hypre_CSRMatrixI(matrix->diag);
   diag_j=hypre_CSRMatrixJ(matrix->diag);

   num_nonzeros_offd = hypre_CSRMatrixNumNonzeros(matrix->offd);
   if (num_nonzeros_offd)
   {
      offd_data=hypre_CSRMatrixData(matrix->offd);
      offd_i=hypre_CSRMatrixI(matrix->offd);
      offd_j=hypre_CSRMatrixJ(matrix->offd);
   }

   (*n)=diag_i[row+1]-diag_i[row];                          // diag
   if (num_nonzeros_offd) (*n)+=offd_i[row+1]-offd_i[row];  // off diag

   ierr=PetscMalloc((*n)*sizeof(PetscInt),idxn);  if (ierr) return ierr;

   // data storage
   ierr=PetscMalloc((*n)*sizeof(PetscScalar),v); if (ierr) return ierr;

   // transfer data

   first_col_diag=hypre_ParCSRMatrixFirstColDiag(matrix);
   col_map_offd=hypre_ParCSRMatrixColMapOffd(matrix);

   index=0;

   // diag columns
   j=diag_i[row];
   while (j < diag_i[row+1]) {
      J=first_col_diag+(HYPRE_BigInt)diag_j[j];
      if (diag_data) {
         (*idxn)[index]=J;
         (*v)[index]=diag_data[j];
         if (imaginary) (*v)[index]*=PETSC_i;
         index++;
      }
      j++;
   }

   // offd columns
   if (num_nonzeros_offd) {
      j=offd_i[row];
      while (j < offd_i[row+1]) {
         J=col_map_offd[offd_j[j]];
         if (offd_data) {
            (*idxn)[index]=J;
            (*v)[index]=offd_data[j];
            if (imaginary) (*v)[index]*=PETSC_i;
            index++;
         }
         j++;
      }
   }

   return ierr;
}

// hypre_ParCSRMatrix defined at */hypre-2.22.0/src/parcsr_mv/par_csr_matrix.h
//                   routines at */hypre-2.22.0/src/parcsr_mv/par_csr_matrix.c
// hypre_CSRMatrix defined at */hypre-2.22.0/src/seq_mv/csr_matrix.h
//                routines at */hypre-2.22.0/src/seq_mv/csr_matrix.c
// ReA or ImA can be passed as nullptr for a real- or imag- valued Mat
PetscErrorCode hypre_ParCSRMatrixToMat (hypre_ParCSRMatrix *a, Mat *A, PetscInt sparseWidth, int create_A, int imaginary, int assemble)
{
   PetscErrorCode ierr=0;
   PetscInt i;
   PetscInt Height,Width;
   PetscMPIInt size;
   PetscInt n,*idxn=NULL;
   PetscScalar *v=NULL;

   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   Height=(PetscInt)a->global_num_rows;
   Width=(PetscInt)a->global_num_cols;

   // create the matrix
   if (create_A) {
      //ierr=MatDestroy(A); if (ierr) return ierr;
      ierr=MatCreate(PETSC_COMM_WORLD,A); if (ierr) return ierr;

      if (size == 1) {
         ierr=MatSetType(*A,MATSEQAIJ); if (ierr) return ierr;
         ierr=MatSetSizes(*A,PETSC_DECIDE,PETSC_DECIDE,Height,Width); if (ierr) return ierr;
         ierr=MatSeqAIJSetPreallocation(*A,sparseWidth,NULL); if (ierr) return ierr;
      } else {
         // By using global sizes, the local sizes do not match ReA and ReB.
         // Later, the computed result from x must be re-generated to re-align the local divisions.
         // Note that forcing the forcing the local sizes of A to align with ReA and ReB could not be made to work.
         ierr=MatSetType(*A,MATMPIAIJ); if (ierr) return ierr;
         ierr=MatSetSizes(*A,PETSC_DECIDE,PETSC_DECIDE,Height,Width); if (ierr) return ierr;
         ierr=MatMPIAIJSetPreallocation(*A,sparseWidth,NULL,sparseWidth,NULL); if (ierr) return ierr;
      }

      ierr=MatZeroEntries(*A); if (ierr) return ierr;
   }

   i=a->first_row_index;
   while (i <= a->last_row_index) {
      hypre_extractRow (a,i-a->first_row_index,&n,&idxn,&v,imaginary);
      ierr=MatSetValues(*A,1,&i,n,idxn,v,ADD_VALUES); if (ierr) return ierr;
      if (idxn) {PetscFree(idxn); idxn=NULL;}
      if (v) {PetscFree(v); v=NULL;}
      i++;
   } 

   if (assemble) {
      ierr=MatAssemblyBegin(*A,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
      ierr=MatAssemblyEnd(*A,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   }

   return ierr;
}

