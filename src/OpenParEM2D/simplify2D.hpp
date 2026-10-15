////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    simplify - simplifies project files for OpenParEM2D input decks         //
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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <limits>
#include <unistd.h>
#include <complex>
#include <filesystem>
#include "project.h"

extern "C" void init_project (struct projectData *);
extern "C" int load_project_file(const char*, projectData*, const char*);
extern "C" void print_project (struct projectData *, struct projectData *, const char *indent);
extern "C" void free_project(projectData*);
//extern "C" void prefix ();
//extern "C" char* get_prefix_text ();
//extern "C" void set_prefix_text (char *);

