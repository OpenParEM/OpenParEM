////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    license_process - license file formatter for placement in c++ source    //
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

#include <iostream>
#include <fstream>

using namespace std;

int main ()
{
   int lineNumber=0;
   string line;

   ifstream inFile;
   inFile.open("LICENSE",ifstream::in);

   if (inFile.is_open()) {

      while (getline(inFile,line)) {
         lineNumber++;

          cout << "   cout << \"" << line << "\" << endl;" << endl;

      }

   } else {
      cout << "ERROR1116: Failed to open file \"LICENSE\" for reading." << endl;
      return 1;
   }

   return 0;
}
