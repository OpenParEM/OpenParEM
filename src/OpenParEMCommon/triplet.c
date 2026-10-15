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

#include "triplet.h"
#include <stdlib.h>

void print_dataTriplet (struct dataTriplet *a) {
   if (a) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%zu %zu %20.14e\n",a->i,a->j,a->value);
   }
}

void transpose_dataTriplet (struct dataTriplet *a) {
   size_t temp;
   if (a != NULL) {
     temp=a->i;
     a->i=a->j;
     a->j=temp;
   }
   return;
}

struct vectorTriplet* alloc_vectorTriplet (size_t block_size) {
   struct vectorTriplet *a=NULL;

   if (block_size <= 0) return a;

   a=(struct vectorTriplet*) malloc(sizeof(struct vectorTriplet));
   if (a != NULL) {
      a->vector=NULL;
      a->vector=malloc(block_size*sizeof(struct dataTriplet));
      if (a->vector) {
         a->block_size=block_size;
         a->max_size=block_size;
         a->size=0;
      } else {
         free(a);
         a=NULL;
      }
   }

   return a;
}

int push_vectorTriplet (struct vectorTriplet *a, struct dataTriplet *b) {
   if (a == NULL || b == NULL) return 1;

   if (a->size == a->max_size) {
      a->vector=(struct dataTriplet*) realloc(a->vector,(a->max_size+a->block_size)*sizeof(struct dataTriplet));
      if (a->vector == NULL) {
         free(a);
         return 1;
      }
      a->max_size+=a->block_size;
   }

   a->vector[a->size].i=b->i;
   a->vector[a->size].j=b->j;
   a->vector[a->size].value=b->value;
   a->size++;

   return 0;
}

void reset_vectorTriplet (struct vectorTriplet *a) {
   if (a != NULL) a->size=0;
   return;
}


void print_vectorTriplet (struct vectorTriplet *a) {
   int n;

   if (a == NULL) return;

   n=0;
   while (n < a->size) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%zu %zu %20.14e\n",a->vector[n].i,a->vector[n].j,a->vector[n].value);
      n++;
   }
   return;
}

void delete_vectorTriplet (struct vectorTriplet *a) {
   if (a == NULL) return;
   if (a->vector) free(a->vector);
   free(a);
   return;
}


