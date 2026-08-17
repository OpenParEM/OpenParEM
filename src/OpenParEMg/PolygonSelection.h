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

#ifndef POLYGONSELECTION_H
#define POLYGONSELECTION_H

#include <SelectMgr_Filter.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include "path.hpp"

class VertexFilter : public SelectMgr_Filter
{
public:
    DEFINE_STANDARD_RTTI_INLINE(VertexFilter, SelectMgr_Filter)

    //bool set_outline (Path *);
    //bool get_midPoint (TopoDS_Edge& edge, gp_Pnt *);
    virtual Standard_Boolean IsOk (const Handle(SelectMgr_EntityOwner)& theOwner) const override;

private:
    std::vector<gp_Pnt> outline;
    gp_Pln plane;
    std::vector<gp_Pnt2d> outline2D;
};

#endif // POLYGONSELECTION_H
