////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM3g - A GUI for OpenParEM3D                                     //
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


#ifndef SPORTNUMBERS_H
#define SPORTNUMBERS_H

#include <vector>

class SportNumbers
{
public:
    SportNumbers();
    void add (long unsigned int);
    void remove (long unsigned int);
    bool isAssigned (long unsigned int);
    bool isStartWith1 ();
    bool isContiguous ();
    bool hasDuplicates ();
    void clear ();
    long unsigned int next ();
    void print ();

private:
    std::vector<int> sportList;
};

#endif // SPORTNUMBERS_H
