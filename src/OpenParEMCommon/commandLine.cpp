////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM - A fullwave electromagnetic simulator suite.                 //
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

#include "commandLine.hpp"

CommandLine::CommandLine ()
{
   has_help=false;
   has_OpenParEM2D_project=false;
   has_OpenParEM3D_project=false;
   has_macro=false;
   has_logfile=false;
}

bool CommandLine::is_help (std::string value)
{
   if (value.compare("-h") == 0) return true;
   if (value.compare("--h") == 0) return true;
   if (value.compare("-help") == 0) return true;
   if (value.compare("--help") == 0) return true;
   return false;
}

bool CommandLine::test_for_help ()
{
   long unsigned int i=1;
   while (i < arguments.size()) {
      if (is_help(arguments[i])) return true;
      i++;
   }
   return false;
}

bool CommandLine::test_for_type ()
{
   std::ifstream ifs;
   ifs.open(full_path,std::ifstream::in);
   if (ifs.is_open()) {
      std::string line;
      while (getline(ifs,line)) {

         if (line.compare("#OpenParEM2Dproject 1.0") == 0) {
            has_OpenParEM2D_project=true;
            break;
         }

         if (line.compare("#OpenParEM3Dproject 1.0") == 0) {
            has_OpenParEM3D_project=true;
            break;
         }

         if (line.compare("#OpenParEMgMacro 1.0") == 0) {
            has_macro=true;
            break;
         }
      }
      ifs.close();
   } else {
      return true; // cannot open file
   }

   return false;  // is a project or macro
}

bool CommandLine::parse ()
{
   if (arguments.size() == 0) {
      has_help=true;
      return false;
   }

   // check to see if help is requested
   has_help=test_for_help();

   // check for valid input count
   if (has_help) {
      // program name + help
      if (arguments.size() == 2) return false;
      return true;
   } else {

      // program name + project|macro 
      if (arguments.size() == 2) {
         full_path=arguments[1];
         directory=std::filesystem::path(full_path).parent_path().string();
         filename=std::filesystem::path(full_path).filename().string();
      }

      // program name + project|macro + log file
      else if (arguments.size() == 3) {
         full_path=arguments[1];
         directory=std::filesystem::path(full_path).parent_path().string();
         filename=std::filesystem::path(full_path).filename().string();

         has_logfile=true;
         logfile=arguments[2];
      }

      else {
         return true;
      }
   }

   // get the type and return on fail
   if (test_for_type()) return true;

   return false;
}

void CommandLine::print ()
{
   std::cout << "CommandLine:" << std::endl;
   std::cout << "   argument size = " << arguments.size() << std::endl;
   long unsigned int i=0;
   while (i < arguments.size()) {
      std::cout << "      argument[" << i << "] = " << arguments[i] << std::endl;
      i++;
   }
   std::cout << "   full_path=" << full_path << std::endl;
   std::cout << "   filename=" << filename << std::endl;
   std::cout << "   directory=" << directory << std::endl;
   std::cout << "   logfile=" << logfile << std::endl;
   std::cout << "   has_help = " << has_help << std::endl;
   std::cout << "   has_OpenParEM2D_project = " << has_OpenParEM2D_project << std::endl;
   std::cout << "   has_OpenParEM3D_project = " << has_OpenParEM3D_project << std::endl;
   std::cout << "   has_macro = " << has_macro << std::endl;
   std::cout << "   has_logfile = " << has_logfile << std::endl;
}

