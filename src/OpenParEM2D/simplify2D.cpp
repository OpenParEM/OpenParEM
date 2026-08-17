////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    simplify2D - simplifies project files for OpenParEM2D input decks       //
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

// simplify project files
// read in a proj file then output the same file with defaults commented out

#include "simplify2D.hpp"

using namespace std;

int main (int argc, char *argv[])
{
   struct projectData defaultData;
   struct projectData projData;

   if (argc != 2) {
      cout << "usage: simplify *.proj" << endl;
      exit(1);
   }

   PetscInitializeNoArguments();

   // load the project file
   const char *projFile;
   projFile=argv[1];

   init_project (&defaultData);
   //print_project (&defaultData,nullptr,"");

   init_project (&projData);
   if (load_project_file (projFile, &projData, "")) {
      cout << "ERROR2274: Failed to load project file \"" << projFile << "\" for reading." << endl;
      exit(1);
   }
   print_project (&projData,&defaultData,"");

   PetscFinalize();
}

