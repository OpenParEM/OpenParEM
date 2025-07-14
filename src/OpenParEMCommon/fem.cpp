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

#include "fem.hpp"

// allocates memory that must be freed later
bool loadData (ifstream *eVecFile, double **eVecRe, double **eVecIm, size_t *vectorSize, char *filename)
{
   string line;
   int lineNumber=0;
   vector<string> tokens;
   bool foundVec=false,foundEndVec=false;
   size_t allocated,length,blockSize=256;

   allocated=blockSize;
   *eVecRe=(double *)malloc(allocated*sizeof(double));
   *eVecIm=(double *)malloc(allocated*sizeof(double));
   *vectorSize=0;

   if (*eVecRe == nullptr || *eVecIm == nullptr) {
      if (eVecRe != nullptr) free(eVecRe);
      if (eVecIm != nullptr) free(eVecIm);
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1010: Failed to allocate memory.\n");
      return true;
   }

   while (getline(*eVecFile,line)) {
      lineNumber++;
      split_on_space (&tokens,line);

      if (foundVec) {
         if (tokens.size() > 0) if (tokens[0].compare("];") == 0) foundEndVec=true;

         if (! foundEndVec) {
            bool savedValue=false;

            if (tokens.size() == 1) { // real part only
               try {
                  (*eVecRe)[*vectorSize]=stod(tokens[0]);
               } catch(const std::out_of_range&) {
                  // assume that the number is less than DBL_MIN and set to 0
                  (*eVecRe)[*vectorSize]=0;
               }
               (*eVecIm)[*vectorSize]=0;

               savedValue=true;
               (*vectorSize)++;
            } else if (tokens.size() == 2) {  // ?
              prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1011: Unsupported formatting in file \"%s\" at line %d\n",filename,lineNumber);
              return true;
            } else if (tokens.size() == 3) {  // complex

               try {
                  (*eVecRe)[*vectorSize]=stod(tokens[0]);
               } catch(const std::out_of_range&) {
                  // assume that the number is less than DBL_MIN and set to 0
                  (*eVecRe)[*vectorSize]=0;
               }
               length=tokens[2].length();

               try {
                  (*eVecIm)[*vectorSize]=stod(tokens[2].substr(0,length-1));
               } catch(const std::out_of_range&) {
                  // assume that the number is less than DBL_MIN and set to 0
                  (*eVecIm)[*vectorSize]=0;
               }
               if (tokens[1].compare("-") == 0) (*eVecIm)[*vectorSize]=-(*eVecIm)[*vectorSize];

               savedValue=true;
               (*vectorSize)++;
            } else {
              prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1012: Unsupported formatting in file \"%s\" at line %d\n",filename,lineNumber);
            }

            if (savedValue) {
               if (*vectorSize == allocated) {
                  allocated+=blockSize;
                  *eVecRe=(double *)realloc(*eVecRe,allocated*sizeof(double));
                  *eVecIm=(double *)realloc(*eVecIm,allocated*sizeof(double));

                  if (*eVecRe == nullptr || *eVecIm == nullptr) {
                     if (eVecRe != nullptr) free(eVecRe);
                     if (eVecIm != nullptr) free(eVecIm);
                     prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1013: Failed to allocate memory.\n");
                     return true;
                  }
               }
            }

         }

      } else {
         if (tokens.size() > 0) {if (tokens[0].substr(0,3).compare("Vec") == 0) foundVec=true;}
      }
   }

   return false;
}
