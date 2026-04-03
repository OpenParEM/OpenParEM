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

#include "Process.h"
#include <AIS_InteractiveContext.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>

Process::Process (QWidget *parent)
    : QWidget{parent}
{}

Extrude* Extrude::copyCreate ()
{
    Extrude* newExtrude=new Extrude();
    newExtrude->modified=modified;
    newExtrude->length=length;
    return newExtrude;
}

Merge* Merge::copyCreate ()
{
    Merge* newMerge=new Merge();
    newMerge->modified=modified;
    return newMerge;
}

Subtract* Subtract::copyCreate ()
{
    Subtract* newSubtract=new Subtract();
    newSubtract->modified=modified;
    return newSubtract;
}
