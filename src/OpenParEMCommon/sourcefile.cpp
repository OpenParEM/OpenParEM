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

#include "sourcefile.hpp"

///////////////////////////////////////////////////////////////////////////////////////////
// SourceFile
///////////////////////////////////////////////////////////////////////////////////////////

SourceFile::SourceFile (int startLine_, int endLine_)
{
   startLine=startLine_;
   endLine=endLine_;

   // name
   name.push_alias("name");
   name.set_loaded(false);
   name.set_positive_required(false);
   name.set_non_negative_required(false);
   name.set_lowerLimit(0);
   name.set_upperLimit(0);
   name.set_checkLimits(false);
}

bool SourceFile::load (std::string *indent, inputFile *inputs)
{
   bool fail=false;

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {

      std::string token,value,line;
      line=inputs->get_line(lineNumber);
      get_token_pair(&line,&token,&value,&lineNumber,*indent);

      int recognized=0;

      if (name.match_alias(&token)) {
         if (name.is_loaded()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1106: Duplicate entry at line %d for previous entry at line %d.\n",
                                                   indent->c_str(),indent->c_str(),lineNumber,name.get_lineNumber());
            fail=true;
         } else {
            name.set_keyword(token);
            name.set_value(value);
            name.set_lineNumber(lineNumber);
            name.set_loaded(true);
         }
         recognized++;
      }

      // should recognize one keyword
      if (recognized != 1) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1107: Unrecognized keyword at line %d.\n",indent->c_str(),indent->c_str(),lineNumber);
         fail=true;
      }
      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }

   return fail;
}

bool SourceFile::inBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

bool SourceFile::check (std::string *indent)
{
   bool fail=false;

   if (!name.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1108: File block at line %d must specify a name.\n",indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   return fail;
}

SourceFile* SourceFile::clone ()
{
   SourceFile *sc=new SourceFile(startLine,endLine);
   if (!sc) return nullptr;

   sc->name=name;

   return sc;
}

void SourceFile::print () {
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"File\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   name=%s\n",name.get_value().c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"EndFile\n");
   return;
}

