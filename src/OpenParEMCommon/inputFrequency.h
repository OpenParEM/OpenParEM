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

#ifndef INPUTFREQUENCY_H
#define INPUTFREQUENCY_H

struct inputFrequencyPlan {
   int type;                 // 0 => linear, 1 => log, 2 => point
   double frequency;         // for point
   double start;             // for linear and log
   double stop;              // for linear and log
   double step;              // for linear
   int pointsPerDecade;      // for log
   int refine;               // 0 => do not refine the mesh at the frequency point(s), 2 => refine the mesh
   int lineNumber;
};

#endif
