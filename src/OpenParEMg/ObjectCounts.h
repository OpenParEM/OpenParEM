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


#ifndef OBJECTCOUNTS_H
#define OBJECTCOUNTS_H

// class to keep track of the counts of the various types of objects in a drawing
// Uniqueness is not required, but unique numbering may help when discussing drawings.
class ObjectCounts
{
public:
    ObjectCounts ()
    {
        line=0;
        polyline=0;
        rectangle=0;
        polycircle=0;
        extrude=0;
        merge=0;
        subtract=0;
        solid=0;
        compsolid=0;
        compound=0;
    }

    void reset ()
    {
        line=0;
        polyline=0;
        rectangle=0;
        polycircle=0;
        extrude=0;
        merge=0;
        subtract=0;
        solid=0;
        compsolid=0;
        compound=0;
    }

    long unsigned int line,polyline,rectangle,polycircle;  // polywire objects
    long unsigned int extrude,merge,subtract;              // operation objects
    long unsigned int solid,compsolid,compound;            // general AIS shapes
};
#endif // OBJECTCOUNTS_H
