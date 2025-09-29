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

#include "eigensolve.h"

// globals for use in monitorEM2D since the function call is fixed
double NpTodB=20*log10(exp(1));
double koMonitor=0;
double modesMonitor=0;
double errorMonitor=0;
double errorLimitMonitor=0;
int workingModeMonitor=1;
PetscScalar monitorTarget;
int use_shift_invert;

double solution_null_threshold=1e-8;

double* Readdof (char *resultsDir, char *type, int mode, size_t *count) {
   FILE *fp;
   char *filename=NULL;
   char *line=NULL;
   size_t len=0;
   ssize_t read;
   int lineCount,blockSize,allocated;
   double *dof=NULL;
   char test[16];

   sprintf(test,"_%d",mode);
   filename=(char *)malloc((strlen(resultsDir)+strlen(type)+23+strlen(test))*sizeof(char));

   if (filename != NULL) {
      sprintf(filename,"%s/initial_guess_%s_%d.dat",resultsDir,type,mode);

      fp=fopen(filename,"r");
      if (fp) {

         blockSize=512;
         allocated=512;
         dof=(double *)malloc(allocated*sizeof(double));
         if (dof == NULL) {
            if(line) {free(line); line=NULL;}
            if (filename) {free(filename); filename=NULL;}
            return NULL;
         }

         *count=0;
         lineCount=1;

         read = getline (&line,&len,fp);
         while (read != -1) {
            if (lineCount > 0) {
               if (*count == allocated) {
                  allocated+=blockSize;
                  dof=(double *)realloc(dof,allocated*sizeof(double));
                  if (dof == NULL) {if(line) free(line); line=NULL; return NULL;}
               }
               dof[*count]=atof(line);
               (*count)++;
            }
            lineCount++;
            if (line) {free(line); line=NULL;}
            read = getline (&line,&len,fp);
         }

         fclose (fp);
         free (filename); 
      } else {
         free (filename);
         return NULL;
      }
   } else {return NULL;}

   if (line) {free(line); line=NULL;}
   return dof;
}

void printdof (double *a, size_t size) {
   size_t i;

   if (a == NULL) return;
   i=0;
   while (i < size) {
      prefix(); PetscPrintf (PETSC_COMM_WORLD,"%g\n",a[i]);
      i++;
   }
}

FILE* openDataFile (char *resultsDir, const char *type, char *project, int parIndex) {
  
   FILE *fp; 
   char *filename=NULL;

   filename=(char *)malloc((strlen(resultsDir)+strlen(type)+strlen(project)+2+5+1)*sizeof(char));

   if (filename != NULL) {
      sprintf(filename,"%s%s.%s.%05d",resultsDir,type,project,parIndex);
      removeQuotes(filename);
      fp=fopen(filename,"r");

      if (fp) {
         //printf ("Processing %s ...\n",filename);
         free(filename);
      } else {
         free(filename);
         return NULL;
      }

   } else return NULL;

   return fp;
}

// return 0 on successful read of a line with data returned in loadedData
//          Do not return data if skip=1.
// return 1 if no data is read (i.e. at the end of the file)
// return 2 if fp is NULL
// return 3 if the dataTriplet is incomplete [a fatal error]
int loadDataLine (FILE *fp, struct dataTriplet *loadedData, int skip) {
    char *line=NULL;
    char *ptr=NULL;
    size_t len=0;
    ssize_t read;
    char *token=NULL;

    if (fp == NULL) return 2;

    read = getline (&line,&len,fp);
    if (read == -1) {if(line) free(line); line=NULL; return 1;}
    if (skip) {if(line) free(line); line=NULL; return 0;}

    line=removeNewLineChar(line);

    // i
    token=strtok(line," ");
    if (token != NULL) {
       loadedData->i=(size_t)strtoull(token,&ptr,0);
    
       // j
       token=strtok(NULL," ");
       if (token != NULL) {
          loadedData->j=(size_t)strtoull(token,&ptr,0);

          // value
          token=strtok(NULL," ");
          if (token != NULL) loadedData->value=atof(token);
          else {if(line) free(line); line=NULL; return 3;}
       } else {if(line) free(line); line=NULL; return 3;}
    } else {if(line) free(line); line=NULL; return 3;}

    if (line) {free(line); line=NULL;}
    return 0;
}


// count the number of rows, columns, and sparse column usage to enable accurate allocations of sparse matrices
int loadDataFileStats (const char *type, char *resultsDir, char *project,
                        PetscInt *rowCount, PetscInt *colCount, PetscInt *sparseColCount)
{
   int count=0;  // counts the number of loaded files and returns that count
   int skip,retval;
   FILE *fp=NULL;
   struct dataTriplet loadedData;
   size_t current_row,column_count,max_column_count;

   *rowCount=0;
   *colCount=0;

   // keep track of the maximum column width for allocating sparse matrices
   current_row=SIZE_MAX;
   column_count=0;
   max_column_count=0;

   // loop over the files
   while (1) {

      fp=openDataFile (resultsDir,type,project,count);
      if (fp) {
         skip=1;  // skip the first line

         // loop over the data
         while (1) {
            retval=loadDataLine (fp,&loadedData,skip);
            if (retval == 0) {                   // data is good
               if (!skip) {

                  // max counts
                  if (loadedData.i > *rowCount) *rowCount=loadedData.i;
                  if (loadedData.j > *colCount) *colCount=loadedData.j;

                  // sparse column count
                  if (loadedData.i == current_row) {
                     column_count++;
                     if (column_count > max_column_count) max_column_count=column_count;
                  } else {
                     current_row=loadedData.i;
                     column_count=1;
                  }
               }
            } else if (retval == 1) {  // no data read, eof
               break;
            } else if (retval == 2) {
               prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2112: Reached unreachable point 1.\n");
               return 1;
            } else if (retval == 3) {
               prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2113: Incomplete data triplet.\n");
               return 1;
            } else {
               prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2114: Reached unreachable point 2.\n");
               return 1;
            }
            skip=0;
         }
         fclose(fp);
      } else break;
      count++;
   }

   *sparseColCount=(PetscInt) max_column_count;
   *rowCount+=1;
   *colCount+=1;

   return 0;
}


// load real data from OpenParEM2D for processing into complex numbers

// location=0     - load in the real part
// location=else  - load in the imaginary part
// transpose=0    - no transpose
// transpose=else - transpose
// _sign=0         - load as positive
// _sign=else      - load as negative
int loadDataFile (const char *type, char *resultsDir, char *project, Mat *data, PetscInt ioffset, PetscInt joffset, int location, int transpose, double _sign, PetscMPIInt rank)
{
   int skip,retval;
   FILE *fp=NULL;
   struct dataTriplet loadedData;
   PetscInt one,idxm[1],idxn[1];
   PetscComplex v[1];
   PetscErrorCode ierr=0;
   double sign=1;

   if (data == NULL) return 1;

   if (_sign != 0) sign=-1;

   one=1;

   fp=openDataFile (resultsDir,type,project,rank);
   if (fp) {
      skip=1;  // skip the first line

      // loop over the data
      while (1) {
         retval=loadDataLine (fp,&loadedData,skip);
         if (retval == 0) {  // data is good
            if (!skip) {

               idxm[0]=loadedData.i+ioffset;
               idxn[0]=loadedData.j+joffset;

               // skip the operation if the value is zero to save a little time
               if (loadedData.value != 0) {
                  if (location == 0) v[0]=PetscCMPLX(loadedData.value*sign,0);      // load in real part
                  else v[0]=PetscCMPLX(0,loadedData.value*sign);                    // load in imaginary part

                  if (transpose == 0) {ierr=MatSetValues(*data,one,idxm,one,idxn,v,ADD_VALUES); CHKERRQ(ierr);}   // no transpose
                  else {ierr=MatSetValues(*data,one,idxn,one,idxm,v,ADD_VALUES); CHKERRQ(ierr);}                  // transpose
               }
            }
         } else if (retval == 1) {  // no data read, eof
            break;
         } else if (retval == 2) {
            prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2115: Reached unreachable point 1.\n");
            return 1;
         } else if (retval == 3) {
            prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2116: Incomplete data triplet.\n");
            return 1;
         } else {
            prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2117: Reached unreachable point 2.\n");
            return 1;
         }
         skip=0;
      }
      fclose(fp);
   }

   return 0;
}

PetscErrorCode saveFields(char *resultsDir, Vec *Efield, Vec *Hfield, int mode)
{
   PetscErrorCode ierr=0;
   PetscViewer viewer;
   char filename[128];

   // save Efield to a file
   sprintf(filename,"%sEfield_mode_%d.dat",resultsDir,mode+1);
   ierr=PetscViewerASCIIOpen(PETSC_COMM_WORLD,filename,&viewer); CHKERRQ(ierr);
   ierr=PetscViewerPushFormat(viewer,PETSC_VIEWER_ASCII_MATLAB); CHKERRQ(ierr);
   ierr=VecView(*Efield,viewer); CHKERRQ(ierr);
   ierr=PetscViewerPopFormat(viewer); CHKERRQ(ierr);
   ierr=PetscViewerDestroy(&viewer); CHKERRQ(ierr);

   // save Hfield to a file
   sprintf(filename,"%sHfield_mode_%d.dat",resultsDir,mode+1);
   ierr=PetscViewerASCIIOpen(PETSC_COMM_WORLD,filename,&viewer); CHKERRQ(ierr);
   ierr=PetscViewerPushFormat(viewer,PETSC_VIEWER_ASCII_MATLAB); CHKERRQ(ierr);
   ierr=VecView(*Hfield,viewer); CHKERRQ(ierr);
   ierr=PetscViewerPopFormat(viewer); CHKERRQ(ierr);
   ierr=PetscViewerDestroy(&viewer); CHKERRQ(ierr);

   return ierr;
}

// Post-process the eigenvalue solution for the eigenpairs and save.
// The eigenvectors are the E-fields.
//
int postProcess (struct projectData *projData, char *resultsDir, EPS *eps, double ko, 
                 double **alphaList, double **betaList, PetscInt EtSize, PetscInt EzSize, PetscMPIInt rank)
{
   PetscErrorCode ierr=0;
   PetscInt i,j,nconv,low,high,index,indexCount,*ivals;
   PetscScalar gamma,gamma2,dotProduct,norm_Efield,norm_Efields,norm_max,*vals;
   PetscScalar *gamma2s;
   Vec Efield,*Efields,Hfield;
   Mat Mt,Cz,Zt,Mz,Ct;
   int fail=0;

   int keep,nullEigenvalueCount;

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         post processing ...\n");

   // get eigenvalues and eigenvectors
   // nconv is the number of successfully solved eigenpairs
   ierr=EPSGetConverged(*eps,&nconv); CHKERRQ(ierr);

   // stop if there are no solved modes
   if (nconv <= 0) {
      projData->solution_active_mode_count=0;
      fail=1;
      return fail;
   }

   // space for eigenvalues
   ierr=PetscMalloc(nconv*sizeof(PetscScalar),&gamma2s); CHKERRQ(ierr);

   // space for eigenvectors
   VecCreate(PETSC_COMM_WORLD,&Efield);
   VecSetType(Efield,VECSTANDARD);
   VecSetSizes(Efield,PETSC_DECIDE,EtSize+EzSize);
   ierr=VecDuplicateVecs(Efield,nconv,&Efields); CHKERRQ(ierr);

   // get the eigenpairs then keep only the ones with non-null eigenvectors

   nullEigenvalueCount=0;
   i=0; j=0;
   while (i < nconv) {

      ierr=EPSGetEigenpair(*eps,i,&gamma2,NULL,Efield,NULL); CHKERRQ(ierr);  // eigenpair

      ierr=VecAssemblyBegin(Efield); CHKERRQ(ierr);
      ierr=VecAssemblyEnd(Efield); CHKERRQ(ierr);

      // save the non-nullspace eigenpairs
      if (cabs(gamma2) > ko*ko*solution_null_threshold) {

         // check to see if this is the same solution as the prior solution
         keep=1;
         if (j > 0) {
            ierr=VecDot(Efield,Efield,&norm_Efield); CHKERRQ(ierr);
            ierr=VecDot(Efields[j-1],Efields[j-1],&norm_Efields); CHKERRQ(ierr);

            norm_max=norm_Efield;
            if (cabs(norm_Efields) > cabs(norm_max)) norm_max=norm_Efields;
            norm_max=sqrt(norm_max);

            ierr=VecDot(Efield,Efields[j-1],&dotProduct); CHKERRQ(ierr);

            if (dotProduct != 0) {
               if (cabs((norm_max-dotProduct)/dotProduct) < 0.00001) keep=0;
            }
         }

         if (keep) {
            ierr=VecCopy(Efield,Efields[j]); CHKERRQ(ierr);
            ierr=VecAssemblyBegin(Efields[j]); CHKERRQ(ierr);  // probably not needed
            ierr=VecAssemblyEnd(Efields[j]); CHKERRQ(ierr);    // probably not needed

            gamma2s[j]=gamma2;
            j++;
         }
      } else {
         nullEigenvalueCount++;
      }

      i++;
   }

   EPSDestroy(eps);

   projData->solution_active_mode_count=j;
   if (projData->solution_active_mode_count > projData->solution_modes) projData->solution_active_mode_count=projData->solution_modes;

   // print out status on the number of eigenpairs

   if (nullEigenvalueCount > 0) {prefix(); ierr=PetscPrintf (PETSC_COMM_WORLD,"         null space eigenvalue count: %d\n",nullEigenvalueCount); CHKERRQ(ierr);}

   if (projData->solution_active_mode_count < projData->solution_modes) {
      if (projData->solution_active_mode_count == 0) {
         prefix(); ierr=PetscPrintf (PETSC_COMM_WORLD,
            "         INFO: None the requested %d modes were found.\n",
            projData->solution_modes); CHKERRQ(ierr);
      } else if (projData->solution_active_mode_count == 1) {
         prefix(); ierr=PetscPrintf (PETSC_COMM_WORLD,
            "         INFO: Only %d mode of the requested %d were found.\n",
            projData->solution_active_mode_count,projData->solution_modes); CHKERRQ(ierr);
      } else {
         prefix(); ierr=PetscPrintf (PETSC_COMM_WORLD,
            "         INFO: Only %d modes of the requested %d were found.\n",
            projData->solution_active_mode_count,projData->solution_modes); CHKERRQ(ierr);
      }
   }

   // jump to end if there are no non-null solutions to work with
   if (projData->solution_active_mode_count <= 0) {fail=1; goto EXIT;}

   // move the eigenvalues to arrays to pass out - avoid complex since this is going to c++
   // alphaList and betaList need to be freed elsewhere

   if (*alphaList != NULL) free(*alphaList);
   if (*betaList != NULL) free(*betaList);

   *alphaList=(double *)malloc (projData->solution_active_mode_count*sizeof(double));
   *betaList=(double *)malloc (projData->solution_active_mode_count*sizeof(double));

   if (*alphaList == NULL || *betaList == NULL) {
      if (*alphaList != NULL) {free(*alphaList); *alphaList=NULL;}
      if (*betaList != NULL) {free(*betaList); *betaList=NULL;}
   } else {
      i=0;
      while (i < projData->solution_active_mode_count) {
         (*alphaList)[i]=creal(csqrt(gamma2s[i]));
         (*betaList)[i]=cimag(csqrt(gamma2s[i]));

         // adjust the sign
         // the solution is in gamma^2, so the square root can be + or -
         // the math setup is for positive beta and alpha, so set that
         if (fabs((*betaList)[i]) > fabs((*alphaList)[i])) {
            if ((*betaList)[i] < 0) {
               (*betaList)[i]=-(*betaList)[i];
               (*alphaList)[i]=-(*alphaList)[i];
            }
         } else {
            if ((*alphaList)[i] < 0) {
               (*betaList)[i]=-(*betaList)[i];
               (*alphaList)[i]=-(*alphaList)[i];
            }
         }
         i++;
      }
   }

   // Hfield - pre-calculate matrices that are only dependent on frequency
   Hsetup (projData, &Mt, &Cz, &Zt, &Mz, &Ct, rank);

   // Solve for H and save the the E and H fields to files

   i=0;
   while (i < projData->solution_active_mode_count) {
      if (projData->output_show_postprocessing) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"            mode %ld:\n",i+1);}

      gamma=(*alphaList)[i]+PETSC_i*(*betaList)[i];

      // Scale the Et part of the eigenvector result to get the true electric field value.

      ierr=VecGetOwnershipRange(Efields[i],&low,&high); CHKERRQ(ierr);

      // count the number of values to update
      index=0;
      j=0;
      while (j < EtSize) {
         if (j >= low && j < high) {
            index++;
         }
         j++;
      }
      indexCount=index;

      // allocate space for indices and values
      ierr=PetscMalloc(indexCount*sizeof(PetscInt),&ivals); CHKERRQ(ierr);
      ierr=PetscMalloc(indexCount*sizeof(PetscScalar),&vals); CHKERRQ(ierr);

      // set the indices
      index=0;
      j=0;
      while (j < EtSize) {
         if (j >= low && j < high) {
            ivals[index]=j;
            index++;
         }
         j++;
      }

      // get the values
      ierr=VecGetValues(Efields[i],indexCount,ivals,vals); CHKERRQ(ierr);

      // scale the values
      j=0;
      while (j < indexCount) {
         vals[j]/=gamma;
         j++;
      }

      // put the values back
      ierr=VecSetValues(Efields[i],indexCount,ivals,vals,INSERT_VALUES); CHKERRQ(ierr);

      ierr=VecAssemblyBegin(Efields[i]); CHKERRQ(ierr);
      ierr=VecAssemblyEnd(Efields[i]); CHKERRQ(ierr);

      // clean up
      if (ivals) {PetscFree(ivals); ivals=NULL;}
      if (vals) {PetscFree(vals); vals=NULL;}

      // solve for H
      Hsolve (projData, &Mt, &Cz, &Zt, &Mz, &Ct, EtSize, EzSize, &Efields[i], &gamma, &Hfield, rank); 

      // save E and H
      saveFields(resultsDir,&Efields[i],&Hfield,i);

      // allocated in Hsolve with VecConcatenate
      ierr=VecDestroy(&Hfield); CHKERRQ(ierr);

      i++;
   }

   if (Mt) {ierr=MatDestroy(&Mt); CHKERRQ(ierr);}
   if (Cz) {ierr=MatDestroy(&Cz); CHKERRQ(ierr);}
   if (Zt) {ierr=MatDestroy(&Zt); CHKERRQ(ierr);}
   if (Mz) {ierr=MatDestroy(&Mz); CHKERRQ(ierr);}
   if (Ct) {ierr=MatDestroy(&Ct); CHKERRQ(ierr);}

   EXIT:

   // clean up
   ierr=VecDestroy(&Efield); CHKERRQ(ierr);
   if (Efields) {ierr=VecDestroyVecs(nconv,&Efields); CHKERRQ(ierr);}
   if (gamma2s) {ierr=PetscFree(gamma2s); CHKERRQ(ierr);}

   return fail;
}

PetscErrorCode monitorEM2D(EPS eps,PetscInt its,PetscInt nconv,PetscScalar *eigr,PetscScalar *eigi,PetscReal *errest,PetscInt nest,void *vf)
{
   int i,ilimit;
   PetscScalar gamma;
   PetscReal maxErr;
   PetscFunctionBegin;

   if (nconv == workingModeMonitor+1) {
      workingModeMonitor++;
      errorMonitor=1e300;
   }

   // set the output limit
   ilimit=nconv+1;
   if (modesMonitor <= nconv) ilimit=modesMonitor;
   if (nest < ilimit) ilimit=nest;

   // find the maximum error
   maxErr=0;
   i=0;
   while (i < ilimit) {
      if (errest[i] > maxErr) maxErr=errest[i];
      i++;
   }

   if (maxErr < errorMonitor/10 || maxErr < errorLimitMonitor) {
      errorMonitor=maxErr;
      i=0;
      while (i < ilimit) {

         if (use_shift_invert) gamma=csqrt(monitorTarget*monitorTarget+1/eigr[i]);
         else gamma=csqrt(eigr[i]);

         // adjust the sign
         if (fabs(cimag(gamma)) > fabs(creal(gamma))) {
            if (cimag(gamma) < 0) gamma=-gamma;
         } else {
            if (creal(gamma) < 0) gamma=-gamma;
         }

         if (i == 0) {
            prefix(); PetscPrintf (PETSC_COMM_WORLD,"            %-12ld %5ld %21.10g %17.10g %17.10g\n",
                         its,nconv,creal(gamma)*NpTodB,cimag(gamma)/koMonitor,errest[i]);
         } else {
            prefix(); PetscPrintf (PETSC_COMM_WORLD,"                               %21.10g %17.10g %17.10g\n",
                         creal(gamma)*NpTodB,cimag(gamma)/koMonitor,errest[i]);
         }
         i++;
      }
   }
   PetscFunctionReturn(0);
}


// for applying boundary conditions: ess_tdof_size_Et, Et_ess_tdof, ess_tdof_size_Ez, Ez_ess_tdof
// for passing out eigenvalue results: alphaList, betaList
// alphaList and betaList are used from the prior iteration as part of the initial guess

int eigensolve (struct projectData *projData, int use_initial_guess, double frequency, int order,
                int ess_tdof_size_Et, PetscInt *Et_ess_tdof, int ess_tdof_size_Ez, PetscInt *Ez_ess_tdof, 
                double **alphaList, double **betaList, int *matrixSize, PetscMPIInt rank)
{
   PetscInt nz;
   PetscInt globalSize,ioffset,joffset;
   int location,transpose,sign;
   int iTt_mur,iTt_eps_re,iTt_eps_im,iG,iGT,iSz,iTz_re,iTz_im,iSt;
   char *resultsDir;
   Mat A,B;
   PetscErrorCode ierr=0;
   EPS eps;
   ST st;
   //EPSType type;
   PetscInt nev,maxit,i,j,its;
   //SVD svd;
   PetscReal tol;
   double pi,ko;
   long int TtHeight,TtWidth,TtSparseWidth;   // PetscInt
   long int GHeight,GWidth,GSparseWidth;
   long int GTHeight,GTWidth,GTSparseWidth;   // transpose of G
   long int TzHeight,TzWidth,TzSparseWidth;
   long int SzHeight,SzWidth,SzSparseWidth;
   double *Et_re_dof=NULL,*Et_im_dof=NULL,*Ez_re_dof=NULL,*Ez_im_dof=NULL;
   size_t Et_re_count,Et_im_count,Ez_re_count,Ez_im_count;
   PetscScalar *dof;
   PetscScalar gamma,target;
   Vec initial_guess;
   PetscInt initial_guess_count;
   PetscInt *TtTz_locations;
   int fail=0;

   pi=4.*atan(1.);

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"      Solving with order %d ...\n",order);

   //print_project(&projData);

   // directory to hold results
   resultsDir=(char *) malloc ((5+strlen(projData->project_name)+1+1)*sizeof(char));
   sprintf(resultsDir,"temp_%s/",projData->project_name);  // align this with OpenParEM2D.cpp, Hsolve.c, and Fields::saveFields

   // run through some files to get counts for allocating matrices
   if (rank == 0) {
      if (loadDataFileStats("Tt_mur_mat",resultsDir,projData->project_name,&TtHeight,&TtWidth,&TtSparseWidth) != 0) {
         prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2118: Failed to scan \"Tt_mur_mat\".\n");
         fail=1;
      }
      if (loadDataFileStats("G_mat",resultsDir,projData->project_name,&GHeight,&GWidth,&GSparseWidth) != 0) {
         prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2119: Failed to scan \"G_mat\".\n");
         fail=1;
      }
      if (loadDataFileStats("GT_mat",resultsDir,projData->project_name,&GTHeight,&GTWidth,&GTSparseWidth) != 0) {
         prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2120: Failed to scan \"GT_mat\".\n");
         fail=1;
      }
      if (loadDataFileStats("Tz_eps_re_mat",resultsDir,projData->project_name,&TzHeight,&TzWidth,&TzSparseWidth) != 0) {
         prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2121: Failed to scan \"Tz_eps_re_mat\".\n");
         fail=1;
      }
      if (loadDataFileStats("Sz_mat",resultsDir,projData->project_name,&SzHeight,&SzWidth,&SzSparseWidth) != 0) {
         prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2122: Failed to scan \"Sz_mat\".\n");
         fail=1;
      }
      if (fail) goto EXIT;
   }

   if (MPI_Bcast(&TtHeight,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;
   if (MPI_Bcast(&TtWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;
   if (MPI_Bcast(&TtSparseWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;

   if (MPI_Bcast(&GHeight,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;
   if (MPI_Bcast(&GWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;
   if (MPI_Bcast(&GSparseWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;

   if (MPI_Bcast(&GTHeight,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;
   if (MPI_Bcast(&GTWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;
   if (MPI_Bcast(&GTSparseWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;

   if (MPI_Bcast(&TzHeight,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;
   if (MPI_Bcast(&TzWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;
   if (MPI_Bcast(&TzSparseWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;

   if (MPI_Bcast(&SzHeight,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;
   if (MPI_Bcast(&SzWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;
   if (MPI_Bcast(&SzSparseWidth,1,MPI_LONG,0,PETSC_COMM_WORLD)) fail=1;

   if (MPI_Barrier(PETSC_COMM_WORLD)) fail=1;

   if (fail) {
      prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2123: Failed to broadcast matrix sizes.\n");
      goto EXIT;
   }

   //printf ("rank %d: TtHeight=%zu  TtWidth=%zu  TtSparseWidth=%zu\n",rank,TtHeight,TtWidth,TtSparseWidth);
   //printf ("rank %d: GHeight=%zu  GWidth=%zu  GSparseWidth=%zu\n",rank,GHeight,GWidth,GSparseWidth);
   //printf ("rank %d: GTHeight=%zu  GTWidth=%zu  GTSparseWidth=%zu\n",rank,GTHeight,GTWidth,GTSparseWidth);
   //printf ("rank %d: TzHeight=%zu  TzWidth=%zu  TzSparseWidth=%zu\n",rank,TzHeight,TzWidth,TzSparseWidth);
   //printf ("rank %d: SzHeight=%zu  SzWidth=%zu  SzSparseWidth=%zu\n",rank,SzHeight,SzWidth,SzSparseWidth);
   prefix(); PetscPrintf (PETSC_COMM_WORLD,"         matrix size: %zu\n",TtHeight+TzHeight);
   *matrixSize=TtHeight+TzHeight;

   // build a location array for later operations [just a vector indexed 0,1,2,...]
   ierr=PetscMalloc((TtHeight+TzHeight)*sizeof(PetscInt),&TtTz_locations); CHKERRQ(ierr);
   i=0;
   while (i < TtHeight+TzHeight) {
      TtTz_locations[i]=i;
      i++;
   }

   // Build the matrices A and B for the generalized eigenvalue problem Ax=kBx.
   //
   // A=[Att Atz]   Att=1/mur*St - ko^2*ecr*Tt, Atz=0, Azt=0, Azz=0
   //   [Azt Azz]  
   //
   // B=[Btt Btz]   Btt=1/mur*Tt, Btz=1/mur*G, Bzt=1/mur*G(transpose), Bzz=1/mur*Sz-ko^2*ecr*Tz
   //   [Bzt Bzz]
   //
   // As the integrated results [Tt, G, Sz, Tz, St] are pulled in from the files, plug these directly into the A and B submatrices
   // Note that the material constances are pulled into the matrix elements, so they do not appear below.
   //
   // There may be performance improvements available from using block matrices. - ToDo

   globalSize=TtHeight+TzHeight;

   // A 
   // matrix type is "mpiaij" - A matrix type to be used for parallel sparse matrices. 
   ierr=MatCreate(PETSC_COMM_WORLD,&A); CHKERRQ(ierr);
   ierr=MatSetType(A,MATAIJ); CHKERRQ(ierr);
   ierr=MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,globalSize,globalSize); CHKERRQ(ierr);
   nz=TtSparseWidth;
   ierr=MatSeqAIJSetPreallocation(A,nz,NULL); CHKERRQ(ierr);
   ierr=MatMPIAIJSetPreallocation(A,nz,NULL,nz,NULL); CHKERRQ(ierr);
   ierr=MatZeroEntries(A); CHKERRQ(ierr);

   // B
   ierr=MatCreate(PETSC_COMM_WORLD,&B); CHKERRQ(ierr);
   ierr=MatSetType(B,MATAIJ); CHKERRQ(ierr);
   ierr=MatSetSizes(B,PETSC_DECIDE,PETSC_DECIDE,globalSize,globalSize); CHKERRQ(ierr);
   nz=GTSparseWidth+TzSparseWidth;
   ierr=MatSeqAIJSetPreallocation(B,nz,NULL); CHKERRQ(ierr);
   ierr=MatMPIAIJSetPreallocation(B,nz,NULL,nz,NULL); CHKERRQ(ierr);
   ierr=MatZeroEntries(B); CHKERRQ(ierr);

   // populate the matrices

   // Tt_mur
   ioffset=0; joffset=0; location=0; transpose=0; sign=0;
   iTt_mur=loadDataFile ("Tt_mur_mat",resultsDir,projData->project_name,&B,ioffset,joffset,location,transpose,sign,rank);
   if (iTt_mur) {prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2124: Failed to load \"Tt_mur_mat\" data file.\n"); fail=1;}

   // Tt_eps_re
   sign=1;
   iTt_eps_re=loadDataFile ("Tt_eps_re_mat",resultsDir,projData->project_name,&A,ioffset,joffset,location,transpose,sign,rank);
   if (iTt_eps_re) {prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2125: Failed to load \"Tt_eps_re_mat\" data file.\n"); fail=1;}

   // Tt_eps_im
   location=1;
   iTt_eps_im=loadDataFile ("Tt_eps_im_mat",resultsDir,projData->project_name,&A,ioffset,joffset,location,transpose,sign,rank);
   if (iTt_eps_im) {prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2126: Failed to load \"Tt_eps_im_mat\" data file.\n"); fail=1;}

   // G
   ioffset=0; joffset=TtHeight; location=0; transpose=0; sign=0;
   iG=loadDataFile ("G_mat",resultsDir,projData->project_name,&B,ioffset,joffset,location,transpose,sign,rank);  
   if (iG) {prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2127: Failed to load \"G_mat\" data file.\n"); fail=1;}

   // GT
   ioffset=TtHeight; joffset=0; location=0; transpose=0; sign=0;
   iGT=loadDataFile ("GT_mat",resultsDir,projData->project_name,&B,ioffset,joffset,location,transpose,sign,rank);        
   if (iGT) {prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2128: Failed to load \"GT_mat\" data file.\n"); fail=1;}

   // Sz
   ioffset=TtHeight; joffset=TtHeight; location=0; transpose=0; sign=0;
   iSz=loadDataFile ("Sz_mat",resultsDir,projData->project_name,&B,ioffset,joffset,location,transpose,sign,rank);
   if (iSz) {prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2129: Failed to load \"Sz_mat\" data file.\n"); fail=1;}

   // Tz
   ioffset=TtHeight; joffset=TtHeight; location=0; transpose=0; sign=1;
   iTz_re=loadDataFile ("Tz_eps_re_mat",resultsDir,projData->project_name,&B,ioffset,joffset,location,transpose,sign,rank);
   if (iTz_re) {prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2130: Failed to load \"Tz_re_mat\" data file.\n"); fail=1;}

   location=1;
   iTz_im=loadDataFile ("Tz_eps_im_mat",resultsDir,projData->project_name,&B,ioffset,joffset,location,transpose,sign,rank);
   if (iTz_im) {prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2131: Failed to load \"Tz_im_mat\" data file.\n"); fail=1;}

   // St
   ioffset=0; joffset=0; location=0; transpose=0; sign=0;
   iSt=loadDataFile ("St_mat",resultsDir,projData->project_name,&A,ioffset,joffset,location,transpose,sign,rank);
   if (iSt) {prefix(); PetscPrintf (PETSC_COMM_WORLD,"ERROR2132: Failed to load \"St_mat\" data file.\n"); fail=1;}

   if (fail) goto EXIT;

   // finalize the matrices by assembling

   ierr=MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);
   ierr=MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);

   ierr=MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);
   ierr=MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);

   ko=2*pi*frequency*sqrt(4e-7*pi*8.8541878176e-12);

   // apply the boundary conditions

   // offset the Ez indices to align with the Ez block in the constructed matrix
   i=0;
   while (i < ess_tdof_size_Ez) {
      Ez_ess_tdof[i]+=TtHeight;
      i++;
   }

   // A - Et only - The matrix is not populated for Ez, so just need to do the part for Et.
   ierr=MatZeroRowsColumns(A,ess_tdof_size_Et,Et_ess_tdof,0.0,NULL,NULL); CHKERRQ(ierr);

   // B - Et and Ez - The matrix is fully populated, so let the subroutine call place the constant on the diagonal.
   ierr=MatZeroRowsColumns(B,ess_tdof_size_Et,Et_ess_tdof,1/(ko*ko),NULL,NULL); CHKERRQ(ierr);
   ierr=MatZeroRowsColumns(B,ess_tdof_size_Ez,Ez_ess_tdof,1/(ko*ko),NULL,NULL); CHKERRQ(ierr);

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"         solving the eigenvalue problem ...\n");

   ierr=EPSCreate(PETSC_COMM_WORLD,&eps); CHKERRQ(ierr);
   ierr=EPSSetOperators(eps,A,B); CHKERRQ(ierr);

   ierr=EPSSetWhichEigenpairs(eps,EPS_SMALLEST_REAL); CHKERRQ(ierr);
   if (projData->solution_accurate_residual) {
      ierr=EPSSetTrueResidual(eps,PETSC_TRUE); CHKERRQ(ierr);  // slower per iteration, but fewer iterations
                                                               // can prevent convergence in some problems
   }
   //EPSSetProblemType(eps,EPS_GNHEP);   // the default, so this setting does not change anything

   tol=projData->solution_tolerance;
   maxit=projData->solution_iteration_limit;
   ierr=EPSSetTolerances(eps,tol,maxit); CHKERRQ(ierr);

   nev=projData->solution_modes+projData->solution_modes_buffer; // number of eigenvalues to calculate
   ierr=EPSSetDimensions(eps,nev,PETSC_DEFAULT,PETSC_DEFAULT); CHKERRQ(ierr);
   //if (nnull > 0) EPSSetDeflationSpace(eps,nnull,V);

   if (projData->solution_shift_invert && use_initial_guess) {

      // define the target
      if (projData->solution_initial_alpha > 0 || projData->solution_initial_beta > 0) {
         target=projData->solution_initial_alpha+PETSC_i*projData->solution_initial_beta;
         projData->solution_initial_alpha=0;
         projData->solution_initial_beta=0;
      } else {
         target=(*alphaList)[0]+PETSC_i*(*betaList)[0];
      }

      // for the monitor
      use_shift_invert=projData->solution_shift_invert;   
      monitorTarget=sqrt(projData->solution_shift_factor)*target;  

      // set a target and use shift-and-invert to reduce the number of iterations by a huge factor
      ierr=EPSGetST(eps,&st); CHKERRQ(ierr);
      ierr=EPSSetWhichEigenpairs(eps,EPS_TARGET_REAL); CHKERRQ(ierr);  // overrides previous setting
      ierr=EPSSetTarget(eps,target*target); CHKERRQ(ierr);
      ierr=STSetType(st,STSINVERT); CHKERRQ(ierr);
      ierr=STSetShift(st,target*target*projData->solution_shift_factor); CHKERRQ(ierr);
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"            using initial guess for the dominant eigenvalue\n");
   }

   // set initial guess based on prior solution
   if (! projData->solution_shift_invert && use_initial_guess) {
      initial_guess_count=0;

      VecCreate(PETSC_COMM_WORLD,&initial_guess);
      VecSetType(initial_guess,VECSTANDARD);
      VecSetSizes(initial_guess,PETSC_DECIDE,TtHeight+TzHeight);
      VecSet(initial_guess,0);

      // loop through the modes
      i=0;
      while (i < projData->solution_active_mode_count) {

         // set an intial guess
         Et_re_dof=Readdof (resultsDir,"Et_re",i+1,&Et_re_count);
         Et_im_dof=Readdof (resultsDir,"Et_im",i+1,&Et_im_count);
         Ez_re_dof=Readdof (resultsDir,"Ez_re",i+1,&Ez_re_count);
         Ez_im_dof=Readdof (resultsDir,"Ez_im",i+1,&Ez_im_count);

         if (Et_re_dof != NULL && Et_im_dof != NULL && Ez_re_dof != NULL && Ez_im_dof != NULL) {

            // combine the dof data into PetscScalar (complex)
            ierr=PetscMalloc((TtHeight+TzHeight)*sizeof(PetscScalar),&dof); CHKERRQ(ierr);

            // reverse the scaling to get back to the scaled Et from the math
            gamma=(*alphaList)[i]+PETSC_i*(*betaList)[i];
            j=0;
            while (j < TtHeight) {
               dof[j]=(Et_re_dof[j]+PETSC_i*Et_im_dof[j])*gamma;
               j++;
            }

            j=0;
            while (j < TzHeight) {
               dof[j+TtHeight]=Ez_re_dof[j]+PETSC_i*Ez_im_dof[j];
               j++;
            }

            // put it into a Vec
            // sum across the initial guesses from all modes - tip from the SLEPc manual
            // This does not work with with shift-and-invert: Can produce failed runs, run-to-run variability, and sensitivity to solution.tolerance.
            if (i == 0) {ierr=VecSetValues(initial_guess,TtHeight+TzHeight,TtTz_locations,dof,INSERT_VALUES); CHKERRQ(ierr);}
            else {ierr=VecSetValues(initial_guess,TtHeight+TzHeight,TtTz_locations,dof,ADD_VALUES); CHKERRQ(ierr);}

            PetscFree(dof);
            initial_guess_count++;
         }

         if (Et_re_dof) {free(Et_re_dof); Et_re_dof=NULL;}
         if (Et_im_dof) {free(Et_im_dof); Et_im_dof=NULL;}
         if (Ez_re_dof) {free(Ez_re_dof); Ez_re_dof=NULL;}
         if (Ez_im_dof) {free(Ez_im_dof); Ez_im_dof=NULL;}

         i++;
      }

      VecAssemblyBegin(initial_guess);
      VecAssemblyEnd(initial_guess);

      // set the initial guess
      if (initial_guess_count > 0) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"            using initial guess for eigenvectors\n");
         ierr=EPSSetInitialSpace(eps,1,&initial_guess); CHKERRQ(ierr);
      }

      // clean up
      VecDestroy(&initial_guess);
   }

   if (projData->output_show_iterations) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"            ----------------------------------------------------------------------------\n");
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"            iteration   # converged      alpha, dB/m           beta/ko             error\n");
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"            ----------------------------------------------------------------------------\n");

      ierr=EPSMonitorSet(eps,monitorEM2D,NULL,NULL);
      koMonitor=ko;
      modesMonitor=nev;
      errorMonitor=1e300;
      errorLimitMonitor=projData->solution_tolerance*(1+1e-14);
      workingModeMonitor=0;
   }

   //MatView(A,PETSC_VIEWER_STDOUT_WORLD);
   //MatView(B,PETSC_VIEWER_STDOUT_WORLD);

   //EPSSetType(eps,EPSKRYLOVSCHUR);   // This is the default for SLEPc, so setting this has no effect
   ierr=EPSSolve(eps); CHKERRQ(ierr);

   if (projData->output_show_iterations) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"            ----------------------------------------------------------------------------\n");}

   // show results stats

   EPSGetIterationNumber(eps,&its);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"            number of iterations: %ld\n",its);

   //EPSGetType(eps,&type);
   //prefix(); PetscPrintf(PETSC_COMM_WORLD,"         solution method: %s\n",type);
   //EPSGetTolerances(eps,&tol,&maxit);
   //prefix(); PetscPrintf(PETSC_COMM_WORLD,"         stopping conditions: tolerance=%.4g, iterations=%D\n",(double)tol,maxit);

   if (its == projData->solution_iteration_limit) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"         ERROR2133: Stopped on iteration limit.\n");
      if (projData->solution_accurate_residual) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"                   Try setting solution.accurate.residual to false.\n");
      }
      fail=1;
      goto EXIT;
   }

   MatDestroy(&A);
   MatDestroy(&B);

   if (postProcess(projData,resultsDir,&eps,ko,alphaList,betaList,TtHeight,TzHeight,rank)) fail=1;

   EXIT:

   ierr=PetscFree(TtTz_locations); CHKERRQ(ierr);

   //STDestroy(&st);   // destroyed by EPSDestroy
   //EPSDestroy(&eps); // destroyed in postProcess

   free(resultsDir);

   return fail;
}


