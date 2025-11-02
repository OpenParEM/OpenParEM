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

#ifndef JOBRELATED_H
#define JOBRELATED_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <cfloat>
#include <chrono>
#include "petscsys.h"
#include "prefix.h"

using namespace std;

extern "C" void prefix ();

void exit_job_on_error (chrono::steady_clock::time_point, const char *, bool, int);
char* create_lock_file (const char *);
extern "C" void remove_lock_file (const char *);
void delete_file (const char *, string, string);
double elapsed_time (chrono::steady_clock::time_point, chrono::steady_clock::time_point);

#endif

