////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEMg - A GUI for OpenParEM3D                                      //
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


#include "SportNumbers.h"
#include <iostream>

SportNumbers::SportNumbers() {}


// sports go as 1,2,3,...
// so store them at sport-1 to be zero-based
void SportNumbers::add (long unsigned int sport)
{
    // create space, if needed
    if (sportList.size() < sport) {
        long unsigned int i=sportList.size();
        while (i < sport) {
            sportList.push_back(0);
            i++;
        }
    }

    // save
    sportList[sport-1]++;
}

void SportNumbers::remove (long unsigned int sport)
{
    if (sport-1 < sportList.size()) {
        sportList[sport-1]--;
    }
}

bool SportNumbers::isAssigned (long unsigned int sport)
{
    if (sport-1 < sportList.size()) {
        if (sportList[sport-1] > 0) return true;
    }
    return false;
}

bool SportNumbers::isStartWith1 ()
{
    if (sportList.size() == 0) return false;
    if (sportList[0] > 0) return true;
    return false;
}

bool SportNumbers::isContiguous ()
{
    if (sportList.size() == 0) return false;

    long unsigned int last=sportList.size()-1;
    while (last >= 0) {
        if (sportList[last] > 0) break;
        if (last == 0) break;
        last--;
    }

    if (last == 0) return true;  // assumes isStartWith1() is true

    long unsigned int i=0;
    while (i <= last) {
        if (sportList[i] == 0) return false;
        i++;
    }

    return true;
}

bool SportNumbers::hasDuplicates ()
{
    long unsigned int i=0;
    while (i < sportList.size()) {
        if (sportList[i] > 1) return true;
        i++;
    }
    return false;
}

void SportNumbers::clear ()
{
    long unsigned int i=0;
    while (i < sportList.size()) {
        sportList[i]=0;
        i++;
    }
    sportList.clear();
}

long unsigned int SportNumbers::next ()
{
    if (sportList.size() == 0) return 1;

    long unsigned int last=sportList.size()-1;
    while (last >= 0) {
        if (sportList[last] > 0) break;
        if (last == 0) break;
        last--;
    }

    return last+2;
}

void SportNumbers::print ()
{
    std::cout << "SportNumbers:" << std::endl;
    long unsigned int i=0;
    while (i < sportList.size()) {
        std::cout << "   " << sportList[i] << std::endl;
        i++;
    }
}
