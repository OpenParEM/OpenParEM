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

#include "Zsolve.h"

double tol=1e-14;

double lapack_complex_double_real (lapack_complex_double a) 
{
   return creal(a);
}

double lapack_complex_double_imag (lapack_complex_double a)
{
   return cimag(a);
}

void printComplex (lapack_complex_double a)
{
   double re, im;

   re=lapack_complex_double_real(a);
   im=lapack_complex_double_imag(a);

   if (fabs(re) < 1e-15) re=0;
   if (fabs(im) < 1e-15) im=0;

   PetscPrintf(PETSC_COMM_WORLD,"%#12.6g",re);
   if (im < 0) PetscPrintf(PETSC_COMM_WORLD,"-j%#11.6g",-im);
   else PetscPrintf(PETSC_COMM_WORLD,"+j%#-11.6g",im);
}

void linearPrint (lapack_complex_double *A, lapack_int n)
{
   lapack_int m=0;
   while (m < n*n) {
      printComplex(A[m]);
      PetscPrintf(PETSC_COMM_WORLD,"\n");
      m++;
   }
}

void matrixPrint (lapack_complex_double *A, lapack_int n)
{
   lapack_int i,j;

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"                   ");
   j=0;
   while (j < n) {
      PetscPrintf(PETSC_COMM_WORLD,"             %3d           ",j+1);
      j++;
   }
   PetscPrintf(PETSC_COMM_WORLD,"\n");

   i=0;
   while (i < n) {  // rows
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"                 %3d: ",i+1);
      j=0;
      while (j < n) {  // columns
         printComplex(A[i+j*n]);
         PetscPrintf(PETSC_COMM_WORLD,"  ");
         j++;
      }
      PetscPrintf(PETSC_COMM_WORLD,"\n");
      i++;
   }
   PetscPrintf(PETSC_COMM_WORLD,"\n");
}

void matrixDiagonalPrint (lapack_complex_double *A, lapack_int n)
{
   lapack_int i,j;

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"\n");

   i=0;
   while (i < n) {  // rows
      PetscPrintf(PETSC_COMM_WORLD,"                 %3d: ",i+1);
      j=0;
      while (j < n) {  // columns
         if (i == j) {
            printComplex(A[i+j*n]);
         }
         j++;
      }
      PetscPrintf(PETSC_COMM_WORLD,"\n");
      i++;
   }
   PetscPrintf(PETSC_COMM_WORLD,"\n");
}

void matrixSetValue (lapack_complex_double *A, lapack_int i, double realVal, double imagVal)
{
   A[i]=CMPLX(realVal,imagVal);
}

void matrixScaleValue (lapack_complex_double *A, lapack_int i, double realVal, double imagVal)
{
   A[i]*=CMPLX(realVal,imagVal);
}

void matrixScaleRow (lapack_complex_double *A, lapack_int n, lapack_int row, double realVal, double imagVal)
{
   lapack_int j;

   j=0;
   while (j < n) {
      A[row+j*n]*=CMPLX(realVal,imagVal);
      j++;
   }
}

double matrixGetRealValue (lapack_complex_double *A, lapack_int i)
{
   return lapack_complex_double_real(A[i]);
}

double matrixGetImagValue (lapack_complex_double *A, lapack_int i)
{
   return lapack_complex_double_imag(A[i]);
}

double matrixGetRealScaleValue (lapack_complex_double *A, lapack_int i, double realVal, double imagVal)
{
   return lapack_complex_double_real(A[i]*CMPLX(realVal,imagVal));
}

double matrixGetImagScaleValue (lapack_complex_double *A, lapack_int i, double realVal, double imagVal)
{
   return lapack_complex_double_imag(A[i]*CMPLX(realVal,imagVal));
}

lapack_complex_double* matrixClone (lapack_complex_double *A, lapack_int n)
{
   lapack_int m;
   lapack_complex_double *B;

   B=(lapack_complex_double *) malloc(n*n*sizeof(lapack_complex_double));

   m=0;
   while (m < n*n) {
      B[m]=A[m];
      m++;
   }

   return B;
}

int isIdentityMatrix (lapack_complex_double *A, lapack_int n)
{
   lapack_int i,j;

   i=0;
   while (i < n) {  // rows
      j=0;
      while (j < n) {  // columns
         if (i == j) {
            if (cabs(A[i+j*n]-CMPLX(1,0)) > tol) return 1;
         } else {
            if (cabs(A[i+j*n]) > tol) return 1;
         }
         j++;
      }
      i++;
   }

   return 0;
}

int isEqualMatrix (lapack_complex_double *A, lapack_complex_double *B, lapack_int n)
{
   lapack_int i,j;

   i=0;
   while (i < n) {  // rows
      j=0;
      while (j < n) {  // columns
         if (cabs(A[i+j*n]-B[i+j*n]) > tol) return 1;
         j++;
      }
      i++;
   }

   return 0;
}

void matrixZero (lapack_complex_double *A, lapack_int n)
{
   lapack_int i,j;

   i=0;
   while (i < n) {  // rows
      j=0;
      while (j < n) {  // columns
         A[i+j*n]=CMPLX(0,0);
         j++;
      }
      i++;
   }
}

void matrixTranspose (lapack_complex_double *A, lapack_int n)
{
   lapack_int i,j;
   lapack_complex_double temp;

   i=0;
   while (i < n) {  // rows
      j=i+1;
      while (j < n) {  // columns
         temp=A[i+j*n];
         A[i+j*n]=A[i*n+j];
         A[i*n+j]=temp;
         j++;
      }
      i++;
   }
}

void matrixConjugate (lapack_complex_double *A, lapack_int n)
{
   lapack_int i,j;

   i=0;
   while (i < n) {
      j=0;
      while (j < n) {
         A[i+j*n]=CMPLX(+lapack_complex_double_real(A[i+j*n]),
                        -lapack_complex_double_imag(A[i+j*n]));
         j++;
      }
      i++;
   }
}

void matrixScale (lapack_complex_double *A, double ReVal, double ImVal, lapack_int n)
{
   lapack_int i,j;

   i=0;
   while (i < n) {
      j=0;
      while (j < n) {
         A[i+j*n]*=CMPLX(ReVal,ImVal);
         j++;
      }
      i++;
   }
}

// copy A to B
// n must be the same for each matrix
void matrixCopy (lapack_complex_double *A, lapack_complex_double *B, lapack_int n)
{
   lapack_int i=0;
   while (i < n*n) {
      B[i]=A[i];
      i++;
   }
}

void swap_rows (lapack_complex_double *A, lapack_int n, lapack_int i, lapack_int j)
{
   lapack_int k;
   lapack_complex_double temp;

   k=0; // column
   while (k < n) {
      temp=A[i+n*k];
      A[i+n*k]=A[j+n*k];
      A[j+n*k]=temp;
      k++;
   }
}

// matrix inverse using Gaussian elimination with pivoting
// adapted from the C example on Wikipedia's LU decomposition page
// keep the lapack variables since everything else is using it
//
// A is nxn stored in column major format
// P is nx1
int LUPDecompose(lapack_complex_double *A, lapack_int n, lapack_int *P) {

   lapack_int i,j,k,imax; 
   double maxA, absA;

   // initialize P
   i=0;
   while (i < n) {
      P[i]=i;
      i++;
   }

   // LU factorization
   i=0; // column
   while (i < n) {
      maxA=0;
      imax=i;

      // find the pivot
      k=i;  // row
      while (k < n) {
         absA=cabs(A[k+i*n]);
         if (absA > maxA) {
            maxA=absA;
            imax=k;
         }
         k++;
      }

      // check for singular matrix
      if (maxA < tol) return 1;

      // eliminate

      if (imax != i) {

         // apply the pivot
         j=P[i];
         P[i]=P[imax];
         P[imax]=j;

         // pivot A
         swap_rows(A,n,i,imax);
      }

      j=i+1; 
      while (j < n) {
         A[j+i*n]/=A[i+i*n];

         k=i+1; 
         while (k < n) {
            A[j+k*n]-=A[j+i*n]*A[i+k*n];
            k++;
         }
         j++;
      }

      i++;
   }

   return 0;
}

// adapted from the C example on Wikipedia's LU decomposition page
int matrixInverse (lapack_complex_double *A, lapack_int n)
{
   lapack_int i,j,k;
   lapack_int *P;
   lapack_complex_double *IA;

   if (A == NULL) return 1;
   if (n <= 0) return 1;

   // allocate memory

   P=(lapack_int *)malloc(n*sizeof(lapack_int));
   if (P == NULL) return 1;

   IA=(lapack_complex_double *)malloc(n*n*sizeof(lapack_complex_double));
   if (IA == NULL) {
      free(P);
      return 1;
   }

   // decompose
   if (LUPDecompose(A,n,P)) return 1;

   // inverse

   j=0;
   while (j < n) {
      i=0;
      while (i < n) {
         IA[i+j*n]=CMPLX(0,0);
         if (P[i] == j) IA[i+j*n]=CMPLX(1,0);

         k=0;
         while (k < i) {
            IA[i+j*n]-=A[i+k*n]*IA[k+j*n];
            k++;
         }
         i++;
      }

      i=n-1;
      while (i >= 0) {
         k=i+1;
         while (k < n) {
            IA[i+j*n]-=A[i+k*n]*IA[k+j*n];
            k++;
         }

         IA[i+j*n]/=A[i+i*n];

         i--;
      }

      j++;
   }

   free(P);

   i=0;
   while (i < n) {
      j=0;
      while (j < n) {
         A[i+j*n]=IA[i+j*n];
         j++;
      }
      i++;
   }

   free(IA);

   return 0;
}


/* Version using Lapack
int matrixInverse (lapack_complex_double *A, lapack_int n)
{
   if (n == 1) {
      A[0]=1/A[0];
      return 0;
   }

   lapack_int info;
   lapack_int *ipiv = (lapack_int *) malloc (n*n*sizeof(lapack_int));

   info=LAPACKE_zgetrf(LAPACK_COL_MAJOR,n,n,A,n,ipiv);
   if (info > 0) return info;

   info=LAPACKE_zgetri(LAPACK_COL_MAJOR,n,A,n,ipiv);
   if (info > 0) return info;
// adapted from the C example on Wikipedia's LU decomposition page
   free (ipiv);

   return 0;
}
*/

// A*B, result returned in B
void matrixMultiply (lapack_complex_double *A, lapack_complex_double *B, lapack_int N)
{
   lapack_int i,j,n;
   lapack_complex_double *C;

   // temp space
   C=(lapack_complex_double *) malloc(N*N*sizeof(lapack_complex_double));

   matrixZero (C,N);

   // C=A*B
   i=0;
   while (i < N) {
      j=0;
      while (j < N) {
         n=0;
         while (n < N) {
            C[i+j*N]+=A[i+n*N]*B[n+j*N];
            n++;
         }
         j++;
      }
      i++;
   }

   // copy to B
   i=0;
   while (i < N) {
      j=0;
      while (j < N) {
         B[i+j*N]=C[i+j*N];
         j++;
      }
      i++;
   }

   free(C); 
}

// A+B, result return in B
void matrixSum (lapack_complex_double *A, lapack_complex_double *B, lapack_int n)
{
   lapack_int i,j;

   i=0;
   while (i < n) {
      j=0;
      while (j < n) {
         B[i+j*n]=A[i+j*n]+B[i+j*n];
         j++;
      }
      i++;
   }
}

void colMajorTest()
{
   int n=3;
   lapack_complex_double *A;
   A=(lapack_complex_double *) malloc(n*n*sizeof(lapack_complex_double));

   /*
   matrix:
       0 1 2
       3 4 5
       6 7 8
   linear for column major:
       0 3 6 1 4 7 2 5 8
   */

   A[0]=CMPLX(0,0);
   A[1]=CMPLX(3,3);
   A[2]=CMPLX(6,6);
   A[3]=CMPLX(1,1);
   A[4]=CMPLX(4,4);
   A[5]=CMPLX(7,7);
   A[6]=CMPLX(2,2);
   A[7]=CMPLX(5,5);
   A[8]=CMPLX(8,8);

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"linear:\n");
   linearPrint(A,n);

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"matrix:\n");
   matrixPrint(A,n);

   free(A);
}

void matrixTest()
{
   lapack_int i,j,n;
   lapack_complex_double *A,*B,*C;

   srand(time(0));

   n=30;
   A=(lapack_complex_double *) malloc(n*n*sizeof(lapack_complex_double));
   B=(lapack_complex_double *) malloc(n*n*sizeof(lapack_complex_double));
   C=(lapack_complex_double *) malloc(n*n*sizeof(lapack_complex_double));

   // generate a random test matrix
   // good for n up to about 40
   i=0;
   while (i < n) {
      j=0;
      while (j < n) {
         A[i+j*n]=CMPLX((double)rand()/(double)RAND_MAX-0.5,(double)rand()/(double)RAND_MAX-0.5);
         B[i+j*n]=A[i+j*n];
         C[i+j*n]=A[i+j*n];
         j++;
      }
      i++;
   }

   matrixTranspose(A,n);
   matrixTranspose(A,n);
   if (isEqualMatrix (A,B,n)) PetscPrintf(PETSC_COMM_WORLD,"testMatrix: (A^T)^T==A FAILED\n");
   else PetscPrintf(PETSC_COMM_WORLD,"testMatrix: (A^T)^T==A passed\n");

   if (matrixInverse(A,n)) {PetscPrintf(PETSC_COMM_WORLD,"ERROR1128: matrix inversion error.\n"); return;}

   matrixMultiply(A,B,n);
   if (isIdentityMatrix(B,n)) PetscPrintf(PETSC_COMM_WORLD,"testMatrix: A^-1*A==I FAILED\n");
   else PetscPrintf(PETSC_COMM_WORLD,"testMatrix: A^-1*A==I passed\n");

   if (matrixInverse(A,n)) {PetscPrintf(PETSC_COMM_WORLD,"ERROR1129: matrix inversion error.\n"); return;}
   if (isEqualMatrix (A,C,n)) PetscPrintf(PETSC_COMM_WORLD,"testMatrix: (A^-1)^-1==A FAILED\n");
   else PetscPrintf(PETSC_COMM_WORLD,"testMatrix: (A^-1)^-1==A passed\n");
 

   free(A);
   free(B);
   free(C);
}

void vectorZero (lapack_complex_double *b, lapack_int n)
{
   lapack_int i;

   i=0;
   while (i < n) {
      b[i]=CMPLX(0,0);
      i++;
   }
}

void vectorSetValue (lapack_complex_double *b, lapack_int i, double realVal, double imagVal)
{
   b[i]=CMPLX(realVal,imagVal);
}

// x=A*b
void matrixVectorMultiply (lapack_complex_double *A, lapack_complex_double *b, lapack_complex_double *x, lapack_int n)
{
   lapack_int i,j;

   vectorZero(x,n);

   i=0;
   while (i < n) {
      j=0;
      while (j < n) {
         x[i]+=A[i+j*n]*b[j];
         j++;
      }
      i++;
   }
}

double vectorGetRealValue (lapack_complex_double *b, lapack_int i)
{
   return lapack_complex_double_real(b[i]);
}

double vectorGetImagValue (lapack_complex_double *b, lapack_int i)
{
   return lapack_complex_double_imag(b[i]);
}



