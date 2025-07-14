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

#ifndef MISC_H
#define MISC_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <cfloat>
#include <quadmath.h>
#include "petscsys.h"
#include "prefix.h"


using namespace std;

extern "C" void prefix ();

bool is_comment (string);
bool is_hashComment (string);
void split_on_space (vector<string> *, string);
bool double_compare (double, double, double);
bool complex_compare (complex<double>, complex<double>, double);
double relative_error (double, double);
bool is_double (const char *);
bool is_double (string *);
bool is_complex (complex<double>);
bool is_int (string *);
bool is_point (string *, int);
bool point_get (string *, double *, double *, double *, int, string, int);
void get_token_pair (string *, string *, string *, int *, string);
string processOutputNumber (double);
bool processInputNumber (string, double *);

class inputFile
{
   private:
      vector<string> lineTextList;
      vector<int> lineNumberList;
      vector<int> crossReferenceList;   // for easy lookup
      string indent="   ";
   public:
      bool load (const char *);
      void createCrossReference ();
      bool checkVersion (string, string);
      void print ();
      bool findBlock (int, int, int *, int *, string, string, bool);
      unsigned long int get_size () {return lineNumberList.size();}
      int get_lineNumber (int i) {return lineNumberList[i];}
      int get_first_lineNumber ();
      int get_last_lineNumber();
      int get_previous_lineNumber (int);
      int get_next_lineNumber (int);
      string get_line (int);
      void clear ();
};

#endif

