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

#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <cstring>
#include <vector>

class CommandLine {
   private:
      std::vector<std::string> arguments;
      bool has_help;
      std::string full_path;
      bool has_OpenParEM2D_project;
      bool has_OpenParEM3D_project;
      bool has_macro;
      bool has_logfile;
      std::string filename;
      std::string directory;
      std::string logfile;

      bool is_help (std::string);
      bool test_for_help ();
      bool test_for_type ();

   public:
      CommandLine ();
      void set_arguments (std::vector<std::string> *arguments_) {arguments=*arguments_;}
      bool parse ();
      bool is_OpenParEM2D_project () {return has_OpenParEM2D_project;}
      bool is_OpenParEM3D_project () {return has_OpenParEM3D_project;}
      bool is_macro () {return has_macro;}
      bool has_logFilename () {return has_logfile;}
      bool help () {return has_help;}
      std::string get_filename () {return filename;}
      std::string get_directory () {return directory;}
      std::string get_logFilename () {return logfile;}
      void print ();
};

#endif
