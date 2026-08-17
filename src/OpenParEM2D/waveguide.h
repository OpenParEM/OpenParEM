////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    waveguide - A calculator for several waveguide types.                   //
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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>

struct rectWaveguide {
   double width, height;
   double epsr,mur;
};

struct partiallyFilledRectWaveguide {
   double width, height, thickness;
   double epsr1, epsr2;    // 0->thickness: epsr1; thickness->height: epsr2
   double mur1, mur2;      // 0->thickness: mur1;  thickness->height: mur2
};



