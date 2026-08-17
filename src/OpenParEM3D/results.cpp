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

#include "results.hpp"
#include "fem3D.hpp"

// ToDo: move this to a more sensible place
int commaCount (string a)
{
   int count=0;
   long unsigned int i=0;
   while (i < a.length()) {
      if (a[i] == ',') count ++;
      i++;
   }
   return count;
}

//---------------------------------------------------------------------------------------------------------------------------------
// Result
//---------------------------------------------------------------------------------------------------------------------------------
Result::Result ()
{
   maxAbsoluteError=-1;
   maxAbsoluteError=-1;
   S=new Mat;
}

PetscScalar Result::get_Sij (int i, int j)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   PetscInt low,high;
   MatGetOwnershipRange(*S,&low,&high);

   PetscScalar value=0;
   bool hasValue=false;

   if (i >= low && i < high) {
      MatGetValue(*S,i,j,&value);
      hasValue=true;
   }

   // collect at 0
   if (rank == 0) {
      int k=1;
      while (k < size) {
         int validData;
         double realData;
         double imagData;
         MPI_Recv(&validData,1,MPI_INT,k,10,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&realData,1,MPI_DOUBLE,k,11,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&imagData,1,MPI_DOUBLE,k,12,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         if (validData) value=realData+PETSC_i*imagData;
         k++;
      }
   } else {
      int validData=0;
      if (hasValue) validData=1;
      double realData=PetscRealPart(value);
      double imagData=PetscImaginaryPart(value);
      MPI_Send(&validData,1,MPI_INT,0,10,PETSC_COMM_WORLD);
      MPI_Send(&realData,1,MPI_DOUBLE,0,11,PETSC_COMM_WORLD);
      MPI_Send(&imagData,1,MPI_DOUBLE,0,12,PETSC_COMM_WORLD);
   }

   // send to all
   if (rank == 0) {
      double realData=PetscRealPart(value);
      double imagData=PetscImaginaryPart(value);
      int k=1;
      while (k < size) {
         MPI_Send(&realData,1,MPI_DOUBLE,k,21,PETSC_COMM_WORLD);
         MPI_Send(&imagData,1,MPI_DOUBLE,k,22,PETSC_COMM_WORLD);
         k++;
      }
   } else {
      double realData;
      double imagData;
      MPI_Recv(&realData,1,MPI_DOUBLE,0,21,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&imagData,1,MPI_DOUBLE,0,22,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      value=realData+PETSC_i*imagData;
   }

   return value;
}

void Result::save (ostream *out, struct projectData *projData, int SportCount)
{
   int rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank == 0) {
      *out << "[Result]" << endl;
      *out << "   type=" << type << endl;
      *out << "   active=" << active << endl;
      *out << "   iteration=" << iteration << endl;
      *out << setprecision(15) << "   frequency=" << frequency << endl;
      *out << "   shortestPerWavelength=" << shortestPerWavelength << endl;
      *out << "   longestPerWavelength=" << longestPerWavelength << endl;
      if (strcmp(projData->touchstone_format,"RI") == 0) *out << "   " << type << "(R,I):" << endl;
      if (strcmp(projData->touchstone_format,"MA") == 0) *out << "   " << type << "(mag,deg):" << endl;
      if (strcmp(projData->touchstone_format,"DB") == 0) *out << "   " << type << "(dB,deg):" << endl;

      *out << setprecision(15);
   }

   int i=0;
   while (i < SportCount) {
      int j=0;
      while (j < SportCount) {
         PetscScalar Spar=get_Sij(i,j);
         complex<double> Sparam=complex<double>(PetscRealPart(Spar),PetscImaginaryPart(Spar));
         if (rank == 0) *out << "      " << type << "(" << i+1 << "," << j+1 << ")=";
         if (rank == 0 && strcmp(projData->touchstone_format,"RI") == 0) *out << "(" << real(Sparam) << "," << imag(Sparam) << ")" << endl;
         if (rank == 0 && strcmp(projData->touchstone_format,"MA") == 0) *out << "(" << abs(Sparam) << "," << arg(Sparam)*180/M_PI << ")" << endl;
         if (rank == 0 && strcmp(projData->touchstone_format,"DB") == 0) *out << "(" << 20*log10(abs(Sparam)) << "," << arg(Sparam)*180/M_PI << ")" << endl;
         j++;
      }
      i++;
   }

   if (type.compare("S") == 0) {
      if (rank == 0) *out << "   S-port Zo:" << endl;
      long unsigned int i=0;
      while (i < Zo.size()) {
         if (rank == 0) *out << "      Zo(" << i << ")=(" << real(Zo[i]) << "," << imag(Zo[i]) << ")" << endl;
         i++;
      }
   }

   if (rank == 0) {
      *out << setprecision(6);
      *out << "   mesh_size=" << meshSize << endl;
      *out << "   matrix_size(DOF_count)=" << matrixSize << endl;
      *out << "   sparse_width=" << sparseWidth << endl;
      if (isRefined && iteration > 1) *out << "   maxRelativeError=" << maxRelativeError << endl;
      if (isRefined && maxAbsoluteError >= 0) *out << "   maxAbsoluteError=" << maxAbsoluteError << endl;
      *out << "   isRefined=" << isRefined << endl;
      if (isRefined) *out << "   isConverged=" << isConverged << endl;
      *out << "   fem_setup_time=" << get_fem_setup_time() << endl;
      *out << "   solve_time=" << get_solve_time() << endl;
      *out << "   mesh_refine_time=" << get_mesh_error_time() << endl;
      *out << "   refine_time=" << get_refine_time() << endl;
      if (hasRadiation) *out << "   radiation_time=" << get_radiation_time() << endl;

      *out << "[EndResult]" << endl;
   }
}

void Result::saveFormatted (ostream *out, double solveElapsedTime, double meshErrorTime, double femSetupTime,
                            double refineTime, double radiationTime, double *priorFrequency, double frequencyScale)
{
   int rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank == 0) {
      if (double_compare(frequency,*priorFrequency,1e-12)) *out << setw(15) << "";
      else *out << setw(15) << setprecision(15) << frequency*frequencyScale;

      *out << setw(12) << iteration;
      *out << setw(12) << meshSize;
      *out << setw(12) << matrixSize;   // DOF count
      *out << setw(12) << setprecision(4) << femSetupTime;
      *out << setw(12) << setprecision(4) << solveElapsedTime;

      if (meshErrorTime > 0) *out << setw(12) << setprecision(4) << meshErrorTime;
      else *out << setw(12) << "";

      if (refineTime > 0) *out << setw(12) << setprecision(4) << refineTime;
      else *out << setw(12) << "";

      if (hasRadiation) *out << setw(13) << setprecision(4) << radiationTime;

      *out << setw(12) << setprecision(4) << solveElapsedTime+femSetupTime+meshErrorTime+refineTime+radiationTime;

      if (double_compare(frequency,*priorFrequency,1e-12)) *out << setw(17) << maxRelativeError;
      else  *out << setw(17) << "";

      if (maxAbsoluteError >= 0) *out << setw(17) << maxAbsoluteError;
      else  *out << setw(17) << "";

      *priorFrequency=frequency;

      *out << endl;
   }
}

void Result::saveCSV (ostream *out, struct projectData *projData, double frequencyScale)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   PetscInt m,n;
   MatGetSize(*S,&m,&n);
   int numSport=m;

   if (rank == 0) *out << setprecision(15);
   if (rank == 0) *out << frequency*frequencyScale;

   int i=0;
   while (i < numSport) {
      int j=0;
      while (j < numSport) {
         PetscScalar Spar=get_Sij(i,j);
         complex<double> Sparam=complex<double>(PetscRealPart(Spar),PetscImaginaryPart(Spar));
         if (rank == 0 && strcmp(projData->touchstone_format,"RI") == 0) *out << "," << real(Sparam) << "," << imag(Sparam);
         if (rank == 0 && strcmp(projData->touchstone_format,"MA") == 0) *out << "," << abs(Sparam) << "," << arg(Sparam)*180/M_PI;
         if (rank == 0 && strcmp(projData->touchstone_format,"DB") == 0) *out << "," << 20*log10(abs(Sparam)) << "," << arg(Sparam)*180/M_PI;
         j++;
      }
      i++;
   }
   if (rank == 0) *out << setprecision(6) << endl;
}

void Result::set (string type_, double frequency_, double shortestPerWavelength_, double longestPerWavelength_, int iteration_, ParMesh *pmesh)
{
   type=type_;
   frequency=frequency_;
   shortestPerWavelength=shortestPerWavelength_;
   longestPerWavelength=longestPerWavelength_;
   iteration=iteration_+1;
   active=true;
   meshSize=getGlobalNE(pmesh);
}

void Result::print ()
{
   cout << "Result: " << this << endl;
   cout << "   type=" << type << endl;
   cout << "   iteration=" << iteration << endl;
   cout << "   frequency=" << frequency << endl;
   cout << "   shortestPerWavelength=" << shortestPerWavelength << endl;
   cout << "   longestPerWavelength=" << longestPerWavelength << endl;
   cout << "   active=" << active << endl;
   cout << "   magLimitdB=" << magLimitdB << endl;
   cout << "   equalMagLimit=" << equalMagLimit << endl;
   cout << "   argLimitdeg=" << argLimitdeg << endl;
   cout << "   equalArgLimit=" << equalArgLimit << endl;
   cout << "   meshSize=" << meshSize << endl;
   cout << "   matrixSize(DOF_count)=" << matrixSize << endl;
   cout << "   sparseWidth=" << sparseWidth << endl;
   cout << "   maxRelativeError=" << maxRelativeError << endl;
   cout << "   maxAbsoluteError=" << maxAbsoluteError << endl;
   cout << "   isRefined=" << isRefined << endl;
   cout << "   isConverged=" << isConverged << endl;
   cout << "   hasRadiation=" << hasRadiation << endl;
   cout << "   solve_time=" << solve_time << endl;
   cout << "   fem_setup_time=" << fem_setup_time << endl;
   cout << "   refine_time=" << refine_time << endl;
   cout << "   radiation_time=" << radiation_time << endl;
   cout << "   " << type << ":" << endl;
   MatView(*S,PETSC_VIEWER_STDOUT_WORLD);
}

PetscErrorCode Result::save_as_test (ofstream *out, const char *casename, int frequency_index, int *casenumber)
{
   PetscErrorCode ierr=0;
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   PetscInt m,n;
   MatGetSize(*S,&m,&n);
   int numSport=m;

   int i=0;
   while (i < numSport) {
      int j=0;
      while (j < numSport) {

         PetscScalar value=get_Sij(i,j);

         if (rank == 0) {

            // mag

            *out << casename << "_" << frequency_index+1 << "_" << (*casenumber)++ << "_result" << ",";
            *out << setprecision(15) << frequency << ",";
            *out << "magS(dB),";
            *out << j+1 << ",";
            *out << i+1 << ",";

            if (20*log10(abs(value)) > magLimitdB) {
               *out << "equal" << ",";
               *out << setprecision(15) << 20*log10(abs(value)) << ",";
               *out << equalMagLimit << endl;
            } else {
               *out << "lessthan" << ",";
               *out << magLimitdB << endl;
            }

            // phase

            if (20*log10(abs(value)) > magLimitdB) {

               *out << casename << "_" << frequency_index+1 << "_" << (*casenumber)++ << "_result" << ",";
               *out << setprecision(15) << frequency << ",";
               *out << "argS(deg),";
               *out << j+1 << ",";
               *out << i+1 << ",";

               if (abs(arg(value)*180/M_PI) > argLimitdeg) {
                  *out << "equal" << ",";
                  *out << setprecision(15) << arg(value)*180/M_PI << ",";
                  *out << equalArgLimit << endl;
               } else {
                  *out << "lessthan" << ",";
                  *out << argLimitdeg << endl;
               }

            }

         }

         j++;
      }
      i++;
   }

   return ierr;
}

bool Result::extractS (string line, string frequency_unit, int SportCount)
{
   PetscErrorCode ierr=0;

   if (line.length() == 0) return true;
   if (line[0] == '#') return true;

   ierr=MatCreate(PETSC_COMM_WORLD,S); if (ierr) return ierr;
   ierr=MatSetType(*S,MATDENSE); if (ierr) return ierr;
   ierr=MatSetSizes(*S,PETSC_DECIDE,PETSC_DECIDE,SportCount,SportCount); if (ierr) return ierr;
   ierr=MatZeroEntries(*S); if (ierr) return ierr;

   PetscInt low,high;
   MatGetOwnershipRange(*S,&low,&high);

   bool foundFrequency=false;
   int portIn=1;
   int portOut=1;
   double data1=0;
   double data2=0;
   bool loaded_data1=false;

   stringstream ssLine(line);
   string value;

   while (std::getline(ssLine,value,',')) {
      if (foundFrequency) {
         if (loaded_data1) {
            data2=stod(value);
            if (portIn-1 >= low && portIn-1 < high) {
               PetscScalar dataValue=data1+PETSC_i*data2;
               ierr=MatSetValue(*S,portIn-1,portOut-1,dataValue,INSERT_VALUES); if (ierr) return ierr;
            }

            portOut++;
            if (portOut > SportCount) {portIn++; portOut=1;}

            loaded_data1=false;
         } else {
            data1=stod(value);
            loaded_data1=true;
         }
      } else {
         double frequency=stod(value);
         if (frequency_unit.compare("Hz") == 0) frequency*=1;
         if (frequency_unit.compare("kHz") == 0) frequency*=1e3;
         if (frequency_unit.compare("MHz") == 0) frequency*=1e6;
         if (frequency_unit.compare("GHz") == 0) frequency*=1e9;
         set_frequency(frequency);
         foundFrequency=true;
      }
   }

   ierr=MatAssemblyBegin(*S,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   ierr=MatAssemblyEnd(*S,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   return false;
}

PetscErrorCode Result::forceReciprocal ()
{
   PetscErrorCode ierr=0;

   // transpose of S
   Mat ST;
   ierr=MatTranspose(*S,MAT_INITIAL_MATRIX,&ST); if (ierr) return ierr;

   // S+ST
   ierr=MatAXPY(*S,1.0,ST,SAME_NONZERO_PATTERN); if (ierr) return ierr;

   // 1/2(S+ST)
   ierr=MatScale(*S,0.5); if (ierr) return ierr;

   ierr=MatDestroy(&ST); if (ierr) return ierr;

   return ierr;
}

// k must be destroyed elsewhere
PetscErrorCode  Result::get_k (bool normalize, complex<double> normalizeZo, Mat *k)
{
   PetscErrorCode ierr=0;
   complex<double> Zo;
   PetscScalar value;
   PetscInt low,high,m,n;

   MatGetSize(*S,&m,&n);
   int numSport=m;

   ierr=MatCreate(PETSC_COMM_WORLD,k); if (ierr) return ierr;
   ierr=MatSetType(*k,MATAIJ); if (ierr) return ierr;
   ierr=MatSetSizes(*k,PETSC_DECIDE,PETSC_DECIDE,numSport,numSport); if (ierr) return ierr;
   ierr=MatSeqAIJSetPreallocation(*k,1,NULL); if (ierr) return ierr;
   ierr=MatMPIAIJSetPreallocation(*k,1,NULL,1,NULL); if (ierr) return ierr;
   ierr=MatZeroEntries(*k); if (ierr) return ierr;

   MatGetOwnershipRange(*k,&low,&high);

   int i=low;
   while (i < high) {
      if (normalize) Zo=normalizeZo;
      else Zo=get_Zo(i);
      value=real(Zo)+PETSC_i*imag(Zo);
      ierr=MatSetValue(*k,i,i,sqrt(value),INSERT_VALUES); if (ierr) return ierr;
      i++;
   }

   ierr=MatAssemblyBegin(*k,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   ierr=MatAssemblyEnd(*k,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;

   return ierr;
}

PetscErrorCode Result::S2Z ()
{
   PetscErrorCode ierr=0;
   bool renormalize=false;
   PetscInt low,high;

   if (!is_type_S()) return 2;

   // to become I+S
   Mat IpS;
   ierr=MatConvert(*S,MATSAME,MAT_INITIAL_MATRIX,&IpS); if (ierr) return ierr;

   // I+S
   MatGetOwnershipRange(IpS,&low,&high);
   int i=low;
   while (i < high) {
      ierr=MatSetValue(IpS,i,i,1.0,ADD_VALUES); if (ierr) return ierr;
      i++;
   }

   ierr=MatAssemblyBegin(IpS,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   ierr=MatAssemblyEnd(IpS,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;

   // to become I-S
   Mat ImS;
   ierr=MatConvert(*S,MATSAME,MAT_INITIAL_MATRIX,&ImS); if (ierr) return ierr;
   ierr=MatScale(ImS,-1); if (ierr) return ierr;

   // I-S
   MatGetOwnershipRange(ImS,&low,&high);
   i=low;
   while (i < high) {
      ierr=MatSetValue(ImS,i,i,1.0,ADD_VALUES); if (ierr) return ierr;
      i++;
   }

   ierr=MatAssemblyBegin(ImS,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   ierr=MatAssemblyEnd(ImS,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;

   // k
   Mat k;
   ierr=get_k(renormalize,0,&k); if (ierr) return ierr;

   // (I-S)^-1
   ierr=MatInvert(&ImS,0); if (ierr) return ierr;

   // calculate Z
   Mat C,D;
   ierr=MatMatMult(k,IpS,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&C); if (ierr) return ierr;
   ierr=MatMatMult(C,ImS,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&D); if (ierr) return ierr;
   ierr=MatDestroy(&C); if (ierr) return ierr;
   ierr=MatMatMult(D,k,MAT_INITIAL_MATRIX,PETSC_DEFAULT,S); if (ierr) return ierr;

   // change type
   set_type_Z();

   // cleanup
   ierr=MatDestroy(&IpS); if (ierr) return ierr;
   ierr=MatDestroy(&ImS); if (ierr) return ierr;
   ierr=MatDestroy(&D); if (ierr) return ierr;
   ierr=MatDestroy(&k); if (ierr) return ierr;

   return ierr;
}

PetscErrorCode Result::Z2S (bool renormalize, complex<double> renormalizeZo)
{
   PetscErrorCode ierr=0;

   if (!is_type_Z()) return 2;

   // k
   Mat k;
   ierr=get_k(renormalize,renormalizeZo,&k); if (ierr) return ierr;

   // k^(-1)
   Mat ki;
   ierr=get_k(renormalize,renormalizeZo,&ki); if (ierr) return ierr;
   ierr=MatInvert(&ki,1); if (ierr) return ierr;

   // Z
   Mat Z;
   ierr=MatConvert(*S,MATSAME,MAT_INITIAL_MATRIX,&Z); if (ierr) return ierr;

   Mat Zkipk;
   ierr=MatMatMult(Z,ki,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&Zkipk); if (ierr) return ierr;
   ierr=MatAXPY(Zkipk,1,k,DIFFERENT_NONZERO_PATTERN); if (ierr) return ierr;
   ierr=MatInvert(&Zkipk,0); if (ierr) return ierr;

   Mat Zkimk;
   ierr=MatMatMult(Z,ki,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&Zkimk); if (ierr) return ierr;
   ierr=MatAXPY(Zkimk,-1,k,DIFFERENT_NONZERO_PATTERN); if (ierr) return ierr;

   // calculate S
   Mat C;
   ierr=MatMatMult(Zkipk,Zkimk,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&C); if (ierr) return ierr;
   ierr=MatCopy(C,*S,DIFFERENT_NONZERO_PATTERN);

   // change type
   set_type_S();

   // cleanup
   ierr=MatDestroy(&Z); if (ierr) return ierr;
   ierr=MatDestroy(&Zkipk); if (ierr) return ierr;
   ierr=MatDestroy(&Zkimk); if (ierr) return ierr;
   ierr=MatDestroy(&C); if (ierr) return ierr;
   ierr=MatDestroy(&k); if (ierr) return ierr;
   ierr=MatDestroy(&ki); if (ierr) return ierr;

   return ierr;
}

PetscErrorCode Result::renormalize (complex<double> renormalizeZo)
{
   PetscErrorCode ierr=0;

   // renormalize
   ierr=S2Z (); if (ierr) return ierr;
   ierr=Z2S (true,renormalizeZo); if (ierr) return ierr;

   // reset the impedances
   long unsigned int i=0;
   while (i < Zo.size()) {
      Zo[i]=renormalizeZo;
      i++;
   }

   return ierr;
}

// Petrie Meyer and David S. Prinsloo, "Generalized Multimode Scattering Parameter and Antenna Far-Field Conversions,"
//     IEEE Trans. Antennas and Propagation, vol. 63, no. 11, Nov. 2015, pp. 4818-4826.
// eq. 15: Sb=(Mc+MsSa)(Ms+McSa)^(-1)
PetscErrorCode Result::SparameterConversion (BoundaryDatabase *boundaryDatabase, Mat *Mc, Mat *Ms, vector<complex<double>> *SportZoList)
{
   PetscErrorCode ierr=0;

   // (Ms+McSa)^(-1)

   Mat McSa;
   ierr=MatMatMult(*Mc,*S,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&McSa); if (ierr) return ierr;

   Mat MspMcSa;
   ierr=MatConvert(*Ms,MATSAME,MAT_INITIAL_MATRIX,&MspMcSa); if (ierr) return ierr;
   ierr=MatAXPY(MspMcSa,1,McSa,SAME_NONZERO_PATTERN); if (ierr) return ierr;
   ierr=MatInvert(&MspMcSa,0); if (ierr) return ierr;

   ierr=MatDestroy(&McSa); if (ierr) return ierr;

   // (Mc+MsSa)
   Mat MsSa;
   ierr=MatMatMult(*Ms,*S,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&MsSa); if (ierr) return ierr;

   Mat McpMsSa;
   ierr=MatConvert(*Mc,MATSAME,MAT_INITIAL_MATRIX,&McpMsSa); if (ierr) return ierr;
   ierr=MatAXPY(McpMsSa,1,MsSa,SAME_NONZERO_PATTERN); if (ierr) return ierr;

   ierr=MatDestroy(&MsSa); if (ierr) return ierr;

   // Sb=(Mc+MsSa)(Ms+McSa)^(-1)
   ierr=MatMatMult(McpMsSa,MspMcSa,MAT_REUSE_MATRIX,PETSC_DEFAULT,S); if (ierr) return ierr;

   ierr=MatDestroy(&MspMcSa); if (ierr) return ierr;
   ierr=MatDestroy(&McpMsSa); if (ierr) return ierr;

   // reset the impedances
   long unsigned int i=0;
   while (i < Zo.size()) {
      Zo[i]=(*SportZoList)[i];
      i++;
   }

   return false;
}

Result::~Result ()
{
   //Cannot perform MPI in a destructor if PETSCFinalize has been called.
   //if (S) {MatDestroy(S); S=nullptr;}
}

//---------------------------------------------------------------------------------------------------------------------------------
// ResultDatabase
//---------------------------------------------------------------------------------------------------------------------------------

void ResultDatabase::set_frequencyScale (struct projectData *projData)
{
      if (strcmp(projData->touchstone_frequency_unit,"Hz") == 0) frequencyScale=1;
      if (strcmp(projData->touchstone_frequency_unit,"kHz") == 0) frequencyScale=1e-3;
      if (strcmp(projData->touchstone_frequency_unit,"MHz") == 0) frequencyScale=1e-6;
      if (strcmp(projData->touchstone_frequency_unit,"GHz") == 0) frequencyScale=1e-9;
}

// result given iteration
Result* ResultDatabase::get_Result (double frequency, int iteration)
{
   long unsigned int i=0;
   while (i < results.size()) {
      if (double_compare(results[i]->get_frequency(),frequency,1e-12) && 
          results[i]->get_iteration() == iteration) return results[i];
      i++;
   }
   return nullptr;
}

// active result
Result* ResultDatabase::get_Result (double frequency)
{
   long unsigned int i=0;
   while (i < results.size()) {
      if (results[i]->is_active() &&
          double_compare(results[i]->get_frequency(),frequency,1e-12)) return results[i];
      i++;
   }
   return nullptr;
}

void ResultDatabase::push (Result *result)
{
   Result *test=get_Result(result->get_frequency(),result->get_iteration()-1);
   if (test) {
      test->set_inactive();
   }

   result->set_active();
   results.push_back(result);

   // keep a sorted list of unique frequencies

   // unique?
   bool found=false;
   long unsigned int i=0;
   while (i < unique_frequencies.size()) {
      if (fabs(unique_frequencies[i]-result->get_frequency())/unique_frequencies[i] <= tol) {
         found=true;
         break;
      }
      i++;
   }

   if (! found) {

      // save
      unique_frequencies.push_back(result->get_frequency());

      // ascending sort
      found=true;
      while (found) {
         found=false;
         i=0;
         while (i < unique_frequencies.size()-1) {
            long unsigned int j=i+1;
            while (j < unique_frequencies.size()) {
               if (unique_frequencies[j] < unique_frequencies[i]) {
                  found=true;
                  double temp=unique_frequencies[i];
                  unique_frequencies[i]=unique_frequencies[j];
                  unique_frequencies[j]=temp;
               }
               j++;
            }
            i++;
         }
      }
   }
}

// Error = ||Sn-Sn-1||/||Sn|| using the Frobenius norm.
// More standard calculation
double ResultDatabase::calculate_maxRelativeError (struct projectData *projData, double frequency, int iteration)
{
   PetscErrorCode ierr=0;
   PetscReal normN,normNmNm1;
   double maxRelativeError=-1;

   if (iteration > 1) {

      if (calculate_relative_error_on_S(projData->refinement_variable)) {

         // current iteration

         Result *result=get_Result(frequency,iteration);
         Mat *S;
         S=result->get_S();

         Mat SN;
         ierr=MatConvert(*S,MATSAME,MAT_INITIAL_MATRIX,&SN); if (ierr) return ierr;

         ierr=MatNorm(SN,NORM_FROBENIUS,&normN); if (ierr) return ierr;

         // prior iteration
 
         result=get_Result(frequency,iteration-1); 
         S=result->get_S();

         // difference, saved in difference

         Mat difference;
         ierr=MatConvert(*S,MATSAME,MAT_INITIAL_MATRIX,&difference); if (ierr) return ierr;

         ierr=MatAXPY(difference,-1,SN,SAME_NONZERO_PATTERN); if (ierr) return ierr;
         ierr=MatNorm(difference,NORM_FROBENIUS,&normNmNm1); if (ierr) return ierr;

         maxRelativeError=normNmNm1/normN;

         MatDestroy(&SN);
         MatDestroy(&difference);

      }
   }

   return maxRelativeError;
}


double ResultDatabase::calculate_maxAbsoluteError (struct projectData *projData, double frequency, int iteration)
{
   double maxAbsoluteError=-1;
   long unsigned int i=0;
   while (i < results.size()) {
      if (double_compare(frequency,results[i]->get_frequency(),1e-12) && iteration == results[i]->get_iteration()) {
         if (results[i]->get_maxAbsoluteError() > maxAbsoluteError) maxAbsoluteError=results[i]->get_maxAbsoluteError();
      }
      i++;
   }
   return maxAbsoluteError;
}

void ResultDatabase::set_refine_time (double elapsed, double frequency, int iteration)
{
   Result *result=get_Result(frequency,iteration);
   if (result) result->set_refine_time(elapsed);
}

void ResultDatabase::set_radiation_time (double elapsed, double frequency, int iteration)
{
   Result *result=get_Result(frequency,iteration);
   if (result) result->set_radiation_time(elapsed);
}

bool ResultDatabase::hasRefinement ()
{
   long unsigned int i=0;
   while (i < results.size()) {
      if (results[i]->get_isRefined()) {
          return true;
      }
      i++;
   }
   return false;
}

bool ResultDatabase::isAllConverged ()
{
   long unsigned int i=0;
   while (i < results.size()) {
      if (results[i]->is_active() && 
          results[i]->get_isRefined() && 
         !results[i]->get_isConverged()) {
         return false;
      }
      i++;
   }
   return true;
}

bool ResultDatabase::isSequentialConverged (struct projectData *projData, double frequency)
{
   Result *result=get_Result(frequency);
   int iteration=result->get_iteration();

   int i=0;
   while (i < projData->refinement_required_passes) {
      Result *iteration_result=get_Result(frequency,iteration-i);
      if (iteration_result) {
         if (!iteration_result->get_isConverged()) return false;
      } else return false;
      i++;
   }

   return true;
}

bool ResultDatabase::get_hasRadiation ()
{
   long unsigned int i=0;
   while (i < results.size()) {
      if (results[i]->get_hasRadiation()) return true;
      i++;
   }
   return false;
}

//ToDo: add error checking to ensure that the file wrote out completely
bool ResultDatabase::save (struct projectData *projData)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   int isFail=0;
   ofstream out;

   stringstream ss;
   ss << projData->project_name << "_results.txt";

   if (rank == 0) {
      out.open(ss.str().c_str(),ofstream::out);
      if (!out.is_open()) isFail=1;

      int k=1;
      while (k < size) {
         MPI_Send(&isFail,1,MPI_INT,k,300,PETSC_COMM_WORLD);
         k++;
      }
   } else {
      MPI_Recv(&isFail,1,MPI_INT,0,300,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   }

   if (isFail) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3127: Failed to open file \"%s\" for writing.\n",ss.str().c_str());
      return true;
   }

   if (rank == 0) {
      out << "[Simulator]" << endl;
      out << "   OpenParEM3D " << projData->version_major << "." << projData->version_minor << "." << projData->version_patch << endl;
      out << "[EndSimulator]" << endl;
   }

   long unsigned int i=0;
   while (i < results.size()) {
      results[i]->save(&out,projData,SportCount);
      i++;
   }

   if (rank == 0) {
      if (solve_time > 0) {
         out << "[Time]" << endl;
         out << "   job_time=" << solve_time << endl;
         out << "[EndTime]" << endl;
      }
      out.close();
   }

   MPI_Barrier(PETSC_COMM_WORLD);

   return false;
}

//ToDo: add error checking to ensure that the file wrote out completely
bool ResultDatabase::saveFormatted (struct projectData *projData)
{
   bool fail=false;
   int rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank == 0) {
      stringstream ss;
      ss << projData->project_name << "_iterations.txt";

      ofstream out;
      out.open(ss.str().c_str(),ofstream::out);

      if (out.is_open()) {
         saveFormatted(&out);
         out.close();
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3128: Failed to open file \"%s\" for writing.\n",ss.str().c_str());
         fail=true;
      }
   }

   MPI_Barrier(PETSC_COMM_WORLD);

   return fail;
}

void ResultDatabase::saveCSV (ostream *out, struct projectData *projData,
                              BoundaryDatabase *boundaryDatabase, vector<DifferentialPair *> *aggregateList,
                              bool allIterations)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   int portCount=boundaryDatabase->get_SportCount();

   // Sport information

   vector<string> infoList;
   int j=0;
   while (j < portCount) {
      stringstream ss;
      ss << "#S-port " << j+1 << ",";
      Mode *mode=boundaryDatabase->getDrivingMode(j+1);
      if (mode && mode->net_is_loaded()) ss << mode->get_net() << ",";
      else ss << "net" << j+1 << ",";

      if (projData->reference_impedance == 0) ss << "not renormalized";
      else ss << projData->reference_impedance;
      infoList.push_back(ss.str());
      j++;
   }

   if (!projData->debug_skip_mixed_conversion) {
      long unsigned k=0;
      while (k < aggregateList->size()) {

         int p=(*aggregateList)[k]->get_Sport_P()-1;
         Mode *modeP=boundaryDatabase->getDrivingMode(p+1);
         stringstream ssP;
         if (modeP && modeP->net_is_loaded()) ssP << modeP->get_net();
         else ssP << "net" << j+1;

         int n=(*aggregateList)[k]->get_Sport_N()-1;
         Mode *modeN=boundaryDatabase->getDrivingMode(n+1);
         stringstream ssN;
         if (modeN && modeN->net_is_loaded()) ssN << modeN->get_net();
         else ssN << "net" << j+1;

         // align with fem3D::build_Mc_Ms

         stringstream ssC;
         ssC << "#S-port comm_" << ssP.str() << "_" << ssN.str() << "," << projData->reference_impedance/2;
         infoList[p]=ssC.str();

         stringstream ssD;
         ssD << "#S-port diff_" << ssP.str() << "_" << ssN.str() << "," << projData->reference_impedance*2;
         infoList[n]=ssD.str();

         k++;
      }
   }

   // header

   if (rank == 0) {
      *out << "#OpenParEM3D " << projData->version_major << "." << projData->version_minor << "." << projData->version_patch << endl;
      *out << "#Touchstone format," << projData->touchstone_format << endl;
      *out << "#frequency unit," << projData->touchstone_frequency_unit << endl;
      //*out << "#number of frequencies," << unique_frequencies.size() << endl;
      *out << "#number of ports," << portCount << endl;

      j=0;
      while (j < portCount) {
         *out << infoList[j] << endl;
         j++;
      }

      if (allIterations) *out << "#prior iterations pre-pended by #" << endl;

      if (strcmp(projData->touchstone_frequency_unit,"Hz") == 0) {*out << "#Frequency(Hz)";}
      if (strcmp(projData->touchstone_frequency_unit,"kHz") == 0) {*out << "#Frequency(kHz)";}
      if (strcmp(projData->touchstone_frequency_unit,"MHz") == 0) {*out << "#Frequency(MHz)";}
      if (strcmp(projData->touchstone_frequency_unit,"GHz") == 0) {*out << "#Frequency(GHz)";}

      int col=0;
      while (col < portCount) {
         int row=0;
         while (row < portCount) {
            if (strcmp(projData->touchstone_format,"RI") == 0) *out << ",Re(S(" << row+1 << ";" << col+1 << ")),Im(S(" << row+1 << ";" << col+1 << "))";
            if (strcmp(projData->touchstone_format,"MA") == 0) *out << ",mag(S(" << row+1 << ";" << col+1 << ")),deg(S(" << row+1 << ";" << col+1 << "))";
            if (strcmp(projData->touchstone_format,"DB") == 0) *out << ",dB(S(" << row+1 << ";" << col+1 << ")),deg(S(" << row+1 << ";" << col+1 << "))";
            row++;
         }
         col++;
      }
      *out << endl;
   }

   // data

   long unsigned int i=0;
   while (i < unique_frequencies.size()) {
      int lastIteration=get_lastIteration(unique_frequencies[i]);

      int k=lastIteration;
      if (allIterations) k=1;

      while (k <= lastIteration) {
         if (k != lastIteration) *out << "#";
         Result *result=get_Result(unique_frequencies[i],k);
         if (result) result->saveCSV(out,projData,frequencyScale);
         k++;
      }

      i++;
   }
}

// ToDo: Add checks to make sure that the mixed-mode result is properly symmetric and exit without writing a Touchstone
// file if the fields are actually hybrid.
bool ResultDatabase::saveTouchstone (struct projectData *projData, BoundaryDatabase *boundaryDatabase, vector<DifferentialPair *> *aggregateList)
{
   bool fail=false;
   int rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   // Do not output a Touchstone file if the S-parameters have not been renormalized.  Without renormalization, the reference
   // impedance is in general frequency dependent, and the TouchStone file formats do not support frequency-dependent reference
   // impedances.
   if (projData->reference_impedance == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"            INFO: Skipping Touchstone file output for non-renormalized data.\n");
      return false;
   }

   // Do not output a Touchstone file for modal setups because the modes may or may not be mixed mode for Touchstone 2.0
   // and the even/odd mode ordering cannot be automatically determined to get the correct port ordering.
   if (boundaryDatabase->is_modal()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"            INFO: Skipping Touchstone file output due to modal setup.\n");
      return false;
   }

   // select the Touchstone version to use

   bool TouchstoneVersion1p1=true;
   bool TouchstoneVersion2p0=false;

   bool isMixedMode=boundaryDatabase->is_mixed_mode();
   if (projData->debug_skip_mixed_conversion) isMixedMode=false;

   if (isMixedMode) {
      TouchstoneVersion1p1=false;
      TouchstoneVersion2p0=true;

      if (aggregateList->size() == 0) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: ResultDatabase::saveTouchstone found inconsistent mixed-mode data.\n");
      }
   }

   // common operations between formats

   int portCount=boundaryDatabase->get_SportCount();

   stringstream ss;
   ss << projData->project_name << ".s" << portCount << "p";

   ofstream out;
   if (rank == 0) {
      out.open(ss.str().c_str(),ofstream::out);

      if (!out.is_open()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3175: Failed to open file \"%s\" for writing.\n",ss.str().c_str());
         fail=true;
      }
   }
   if (fail) return fail;

   // output Touchstone

   if (TouchstoneVersion1p1) {

      // header

      if (rank == 0) {
         out << "! OpenParEM3D " << projData->version_major << "." << projData->version_minor << "." << projData->version_patch << endl;
         out << "! " << portCount << "-port S-parameter data" << endl;

         int i=0;
         while (i < portCount) {
            Mode *mode=boundaryDatabase->getDrivingMode(i+1);
            if (mode && mode->net_is_loaded()) out << "! S-port " << i+1 << " " <<  mode->get_net() << endl;
            else out << "! S-port " << i+1 << " net" << i+1 << endl;
            i++;
         }

         out << "# " << projData->touchstone_frequency_unit << " S " << projData->touchstone_format << " R " << projData->reference_impedance << endl;

         // comment line for 1 and 2 ports

         if (portCount < 3) {
            out << "!freq";

            int col=0;
            while (col < portCount) {
               int row=0;
               while (row < portCount) {
                  if (strcmp(projData->touchstone_format,"RI") == 0) out << " ReS"  << row+1 << col+1 << " ImS"  << row+1 << col+1;
                  if (strcmp(projData->touchstone_format,"MA") == 0) out << " magS" << row+1 << col+1 << " angS" << row+1 << col+1;
                  if (strcmp(projData->touchstone_format,"DB") == 0) out << " dBS"  << row+1 << col+1 << " angS" << row+1 << col+1;
                  row++;
               }
               col++;
            }
            out << endl;
         }
      }

      // data

      long unsigned int k=0;
      while (k < unique_frequencies.size()) {
         int lastIteration=get_lastIteration(unique_frequencies[k]);

         out << setprecision(15);
         out << unique_frequencies[k]*frequencyScale;

         Result *result=get_Result(unique_frequencies[k],lastIteration);

         int i=1;
         while (i <= portCount) {
            int count=0;
            bool row_printed=false;

            int j=1;
            while (j <= portCount) {
               PetscScalar Sparam;
               if (portCount < 3) {
                  Sparam=result->get_Sij(j-1,i-1);
               } else {
                  Sparam=result->get_Sij(i-1,j-1);
               }

               if (rank == 0 && strcmp(projData->touchstone_format,"RI") == 0) out << " " << real(Sparam) << " " << imag(Sparam);
               if (rank == 0 && strcmp(projData->touchstone_format,"MA") == 0) out << " " << abs(Sparam) << " " << arg(Sparam)*180/M_PI;
               if (rank == 0 && strcmp(projData->touchstone_format,"DB") == 0) out << " " << 20*log10(abs(Sparam)) << " " << arg(Sparam)*180/M_PI;

               count++;
               if (portCount > 2 && (portCount == count || count == 4)) {
                  if (!row_printed) {
                     if (rank == 0) out << " !row " << i;
                     row_printed=true;
                  }
                  if (rank == 0) out << endl;
                  count=0;
               }

               j++;
            }

            if (rank == 0 && portCount > 2 && !row_printed) out << " !row " << i;
            if (rank == 0 && portCount > 2 && count > 0) out << endl;

            i++;
         }

         if (rank == 0 && portCount <= 2) out << endl;

         k++;
      }
   }

   if (TouchstoneVersion2p0) {

      if (rank == 0) {
         out << "! OpenParEM3D " << projData->version_major << "." << projData->version_minor << "." << projData->version_patch << endl;
         out << "! " << portCount << "-port S-parameter data" << endl;
      }

      vector<string> infoList;
      int j=0;
      while (j < portCount) {
         stringstream ss;
         ss << "! S-port S" << j+1 << " ";
         Mode *mode=boundaryDatabase->getDrivingMode(j+1);
         if (mode && mode->net_is_loaded()) ss << mode->get_net();
         else ss << "net" << j+1;

         infoList.push_back(ss.str());
         j++;
      }

      if (!projData->debug_skip_mixed_conversion) {
         long unsigned k=0;
         while (k < aggregateList->size()) {

            int p=(*aggregateList)[k]->get_Sport_P()-1;
            Mode *modeP=boundaryDatabase->getDrivingMode(p+1);
            stringstream ssP;
            if (modeP && modeP->net_is_loaded()) ssP << modeP->get_net();
            else ssP << "net" << j+1;

            int n=(*aggregateList)[k]->get_Sport_N()-1;
            Mode *modeN=boundaryDatabase->getDrivingMode(n+1);
            stringstream ssN;
            if (modeN && modeN->net_is_loaded()) ssN << modeN->get_net();
            else ssN << "net" << j+1;

            // align with fem3D::build_Mc_Ms

            stringstream ssC;
            ssC << "! S-port comm_" << ssP.str() << "_" << ssN.str();
            infoList[p]=ssC.str();

            stringstream ssD;
            ssD << "! S-port diff_" << ssP.str() << "_" << ssN.str();
            infoList[n]=ssD.str();

            k++;
         }
      }

      long unsigned int k=0;
      while (k < infoList.size()) {
         if (rank == 0) out << infoList[k] << endl;
         k++;
      }

      if (rank == 0) out << "[Version] 2.0" << endl;

      if (rank == 0) out << "# " << projData->touchstone_frequency_unit << " S " << projData->touchstone_format << " R " << projData->reference_impedance << endl;

      if (rank == 0) out << "[Number of Ports] " << portCount << endl;

      if (portCount == 2) {
         if (rank == 0) out << "[Two-Port Data Order] 21_12" << endl;
      }

      if (rank == 0) out << "[Number of Frequencies] " << unique_frequencies.size() << endl;

      if (rank == 0) out << "[Matrix Format] Full" << endl;

      if (isMixedMode) {
         if (rank == 0) out << "[Mixed-Mode Order]";

         vector<string> nameList;
         int i=0;
         while (i < portCount) {
            stringstream ssS;
            ssS << "S" << i+1;
            nameList.push_back(ssS.str());
            i++;
         }

         long unsigned k=0;
         while (k < aggregateList->size()) {
            int p=(*aggregateList)[k]->get_Sport_P()-1;
            int n=(*aggregateList)[k]->get_Sport_N()-1;

            // align with fem3D::build_Mc_Ms

            stringstream ssC;
            ssC << "C" << p+1 << "," << n+1;
            nameList[p]=ssC.str();

            stringstream ssD;
            ssD << "D" << p+1 << "," << n+1;
            nameList[n]=ssD.str();

            k++;
         }

         i=0;
         while (i < portCount) {
            if (rank == 0) out << " " << nameList[i];
            i++;
         }

         if (rank == 0) out << endl;
      }

      if (rank == 0) out << "[Network Data]" << endl;

      k=0;
      while (k < unique_frequencies.size()) {
         int lastIteration=get_lastIteration(unique_frequencies[k]);

         if (rank == 0) out << setprecision(15);
         if (rank == 0) out << unique_frequencies[k]*frequencyScale;

         Result *result=get_Result(unique_frequencies[k],lastIteration);

         int i=1;
         while (i <= portCount) {
            int count=0;
            bool row_printed=false;

            int j=1;
            while (j <= portCount) {
               PetscScalar Sparam;
               if (portCount < 3) {
                  Sparam=result->get_Sij(j-1,i-1);
               } else {
                  Sparam=result->get_Sij(i-1,j-1);
               }

               if (rank == 0 && strcmp(projData->touchstone_format,"RI") == 0) out << " " << real(Sparam) << " " << imag(Sparam);
               if (rank == 0 && strcmp(projData->touchstone_format,"MA") == 0) out << " " << abs(Sparam) << " " << arg(Sparam)*180/M_PI;
               if (rank == 0 && strcmp(projData->touchstone_format,"DB") == 0) out << " " << 20*log10(abs(Sparam)) << " " << arg(Sparam)*180/M_PI;

               count++;
               if (portCount > 2 && (portCount == count || count == 4)) {
                  if (!row_printed) {
                     if (rank == 0) out << " !row " << i;
                     row_printed=true;
                  }
                  if (rank == 0) out << endl;
                  count=0;
               }

               j++;
            }

            if (rank == 0 && portCount > 2 && !row_printed) out << " !row " << i;
            if (rank == 0 && portCount > 2 && count > 0) out << endl;

            i++;
         }

         if (rank == 0 && portCount <= 2) out << endl;

         k++;
      }

      if (rank == 0) out << "[End]" << endl;
   }

   if (rank == 0) out.close();

   MPI_Barrier(PETSC_COMM_WORLD);

   return fail;
}


bool ResultDatabase::saveCSV (struct projectData *projData, BoundaryDatabase *boundaryDatabase,
                              vector<DifferentialPair *> *aggregateList, bool allIterations)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   int isFail=0;

   stringstream ss;
   ss << projData->project_name << "_results.csv";

   ofstream out;

   if (rank == 0) {
      out.open(ss.str().c_str(),ofstream::out);
      if (!out.is_open()) isFail=1;

      int k=1;
      while (k < size) {
         MPI_Send(&isFail,1,MPI_INT,k,300,PETSC_COMM_WORLD);
         k++;
      }
   } else {
      MPI_Recv(&isFail,1,MPI_INT,0,300,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   }

   if (isFail) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3129: Failed to open file \"%s\" for writing.\n",ss.str().c_str());
      return true;
   }

   saveCSV(&out,projData,boundaryDatabase,aggregateList,allIterations);

   if (rank == 0) out.close();


   return false;
}

void ResultDatabase::saveFormatted (ostream *out)
{
   bool hasRadiation=get_hasRadiation();

   *out << setw(15) << "---------------"
        << setw(12) << "------------"
        << setw(12) << "------------"
        << setw(12) << "------------"
        << setw(12) << "------------"
        << setw(12) << "------------"
        << setw(12) << "------------"
        << setw(12) << "------------";
   if (hasRadiation) *out << setw(13) << "-------------";
   *out << setw(12) << "------------"
        << setw(17) << "-----------------"
        << setw(17) << "-----------------"
        << endl;

   if (double_compare(frequencyScale,1,1e-12)) *out << setw(15) << "frequency, Hz";
   if (double_compare(frequencyScale,1e-3,1e-12)) *out << setw(15) << "frequency, kHz";
   if (double_compare(frequencyScale,1e-6,1e-12)) *out << setw(15) << "frequency, MHz";
   if (double_compare(frequencyScale,1e-9,1e-12)) *out << setw(15) << "frequency, GHz";
  
   *out << setw(12) << "iteration"
        << setw(12) << "mesh size"
        << setw(12) << "DOF count"
        << setw(12) << "setup,s"
        << setw(12) << "solve,s"
        << setw(12) << "mesh err,s"
        << setw(12) << "refine,s";
   if (hasRadiation) *out << setw(13) << "radiation,s";
   *out << setw(12) << "total,s"
        << setw(17) << "relative S error"
        << setw(17) << "H error metric"
        << endl;

   *out << setw(15) << "---------------"
        << setw(12) << "------------"
        << setw(12) << "------------"
        << setw(12) << "------------"
        << setw(12) << "------------"
        << setw(12) << "------------"
        << setw(12) << "------------"
        << setw(12) << "------------";
   if (hasRadiation) *out << setw(13) << "-------------";
   *out << setw(12) << "------------"
        << setw(17) << "-----------------"
        << setw(17) << "-----------------"
        << endl;

   double priorFrequency=-DBL_MAX;
   double solveElapsed,meshError,femSetup,refine,radiation;
   long unsigned int i=0;
   while (i < results.size()) {
      solveElapsed=results[i]->get_solve_time();
      femSetup=results[i]->get_fem_setup_time();
      refine=results[i]->get_refine_time();
      meshError=results[i]->get_mesh_error_time();
      radiation=results[i]->get_radiation_time();
      results[i]->saveFormatted(out,solveElapsed,meshError,femSetup,refine,radiation,&priorFrequency,frequencyScale);
      i++;
   }
}

int ResultDatabase::get_lastIteration (double frequency)
{
   int iteration=0;

   long unsigned int i=0;
   while (i < results.size()) {
      if (double_compare(results[i]->get_frequency(),frequency,1e-12) &&
          results[i]->get_iteration() > iteration) {
         iteration=results[i]->get_iteration();
      }
      i++;
   }
   return iteration;
}

void ResultDatabase::print ()
{
   long unsigned int i=0;
   while (i < results.size()) {
      results[i]->print();
      i++;
   }
}

bool ResultDatabase::save_as_test (string testFilename,struct projectData *projData, int *casenumber)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   int isFail=0;
   ofstream out;

   if (rank == 0) {
      out.open(testFilename.c_str(),ofstream::app);
      if (!out.is_open()) isFail=1;

      int k=1;
      while (k < size) {
         MPI_Send(&isFail,1,MPI_INT,k,300,PETSC_COMM_WORLD);
         k++;
      }
   } else {
      MPI_Recv(&isFail,1,MPI_INT,0,300,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   }

   if (isFail) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3101: Failed to open file \"%s\" for writing.\n",testFilename.c_str());
      return true;
   }

   if (rank == 0) out << "# ResultDatabase::save_as_test" << endl;

   long unsigned int i=0;
   while (i < unique_frequencies.size()) {
      Result *result=this->get_Result(unique_frequencies[i]);  // gets the active result
      if (result) result->save_as_test (&out, projData->project_name,i,casenumber);
      else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Failed to find a result at frequency %g.\n",unique_frequencies[i]);}
      i++;
   }

   if (rank == 0) out.close();

   return false;
}

bool ResultDatabase::loadCSV (const char *filename)
{
   bool fail=false;
   string touchstone_format="";
   string frequency_unit="";
   //int number_of_frequencies=-1;

   bool load_touchstone_format=false;
   bool load_frequency_unit=false;
   //bool load_number_of_frequencies=false;
   bool load_number_of_ports=false;

   // load header information

   ifstream CSV;
   CSV.open(filename,ifstream::in);
   if (CSV.is_open()) {
      string line;
      while (getline(CSV,line)) {
         stringstream ssLine(line);
         string value;
         while (std::getline(ssLine,value,',')) {
            if (load_touchstone_format) {touchstone_format=value; load_touchstone_format=false;}
            if (load_frequency_unit) {frequency_unit=value; load_frequency_unit=false;}
            //if (load_number_of_frequencies) {number_of_frequencies=stoi(value); load_number_of_frequencies=false;}
            if (load_number_of_ports) {SportCount=stoi(value); load_number_of_ports=false;}

            if (value.compare("#Touchstone format") == 0) load_touchstone_format=true;
            if (value.compare("#frequency unit") == 0) load_frequency_unit=true;
            //if (value.compare("#number of frequencies") == 0) load_number_of_frequencies=true;
            if (value.compare("#number of ports") == 0) load_number_of_ports=true;
         }

         ssLine.str("");
         ssLine.clear();
      }
      CSV.close();
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3131: Unable to open file \"%s\" for reading.\n",filename);
      fail=true;
   }

   // load S-parameters

   CSV.open(filename,ifstream::in);
   if (CSV.is_open()) {
      string line;
      while (getline(CSV,line)) {
         if (line.length() != 0 && line[0] != '#') {
            Result *newResult=new Result();
            newResult->set_type_S();
            newResult->set_active();
            if (newResult->extractS (line,frequency_unit,SportCount)) {
               delete newResult;
            } else {
               results.push_back(newResult);
            }
         }
      }
      CSV.close();
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3132: Unable to open file \"%s\" for reading.\n",filename);
      fail=true;
   }

   return fail;
}

ResultDatabase::~ResultDatabase ()
{
   long unsigned int i=0;
   while (i < results.size()) {
      delete results[i];
      i++;
   }
}



