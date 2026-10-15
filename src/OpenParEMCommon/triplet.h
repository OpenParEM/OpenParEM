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

#ifndef TRIPLET_H
#define TRIPLET_H

#include <stddef.h>
#include <stdio.h>
#include "petscsys.h"
#include "prefix.h"

void prefix ();

// hold one line of data from the transfer file
struct dataTriplet {
   size_t i;
   size_t j;
   double value;
};

void transpose_dataTriplet (struct dataTriplet *);

// hold N lines of data from the transfer file
struct vectorTriplet {
   size_t block_size; // increment for allocating memory
   size_t max_size;   // allocated max number of elements
   size_t size;       // populated number of elements
   struct dataTriplet *vector;
};

void print_dataTriplet (struct dataTriplet *);
struct vectorTriplet* alloc_vectorTriplet (size_t);
int push_vectorTriplet (struct vectorTriplet *, struct dataTriplet *);
void reset_vectorTriplet (struct vectorTriplet *);
void print_vectorTriplet (struct vectorTriplet *);
void delete_vectorTriplet (struct vectorTriplet *);

#endif

