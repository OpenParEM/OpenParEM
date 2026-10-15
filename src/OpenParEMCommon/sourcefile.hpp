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

#ifndef SOURCEFILE_H
#define SOURCEFILE_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <limits>
#include "misc.hpp"
#include "path.hpp"
#include "prefix.h"

class SourceFile {
   private:
      int startLine;
      int endLine;
      keywordPair name;
   public:
      SourceFile (int,int);
      SourceFile* clone ();
      bool load (std::string *, inputFile *);
      bool inBlock (int);
      std::string get_name () {return name.get_value();}
      bool check (std::string *);
      void print ();
      void save (std::ofstream *);
};

#endif

