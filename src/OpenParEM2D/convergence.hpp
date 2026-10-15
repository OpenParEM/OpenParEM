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

#ifndef CONVERGENCE_H
#define CONVERGENCE_H

#include "mfem.hpp"
#include <slepceps.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "project.h"

using namespace std;
using namespace mfem;

extern "C" int get_refinement_variable (char *, int);
extern "C" double get_refinement_tolerance (char *, int);

class Convergence {
   private:
      vector<double> results;
      long unsigned int number_of_passes;
      int type;
      double tolerance;
      bool converged;
      double last_error;
   public:
      Convergence (int, double, int);
      void push (double);
      int get_type () {return type;}
      bool is_converged () {return converged;}
      double get_last_error() {return last_error;}
      void set_not_converged () {converged=false;}
      bool has_progress ();
      void show_progress ();
};

class ConvergenceDatabase {
   private:
      vector<Convergence*> convergenceList;
   public:
      ~ConvergenceDatabase();
      void initialize (int, int, char *, char*);
      void push(int , complex<double>, double, double, double, double);
      bool is_converged();
      bool is_converged(int mode);
      int is_converged_count();
      void set_not_converged();
      double get_last_error();
      void show_progress();
};

#endif

