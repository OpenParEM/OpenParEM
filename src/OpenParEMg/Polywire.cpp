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

#include "Polywire.h"
#include <AIS_InteractiveContext.hxx>
#include "Precision.hxx"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <TopoDS_Wire.hxx>

Handle(AIS_Shape) CreateAISLineFromVertices (const gp_Pnt& p1, const gp_Pnt& p2)
{
    TopoDS_Vertex v1=BRepBuilderAPI_MakeVertex(p1);
    TopoDS_Vertex v2=BRepBuilderAPI_MakeVertex(p2);

    BRepBuilderAPI_MakeEdge makeEdge(v1, v2);
    TopoDS_Edge edge=makeEdge.Edge();

    if (!makeEdge.IsDone()) return nullptr;
    Handle(AIS_Shape) shape=new AIS_Shape(edge);

    return shape;
}

Polywire::Polywire(QObject *parent)
    : QObject{parent}
{}

void Polywire::deleteRubberband ()
{
    if (!rubberband.IsNull()) {viewerContext->Remove(rubberband,Standard_True); rubberband.Nullify();}
}

bool Polywire::isValidPoint (gp_Pnt &pnt)
{
    if (shapePoints.size() == 0) return true;
    if (shapePoints[shapePoints.size()-1].IsEqual(pnt,Precision::Confusion())) return false;
    return true;
}

void Polywire::addPoint (gp_Pnt &pnt)
{
    shapePoints.push_back(pnt);
}

void Polywire::deleteLastPoint ()
{
    shapePoints.pop_back();
}

void Polywire::close ()
{
    if (shapePoints.size() > 2) {
        shapePoints.push_back(shapePoints[0]);
    }
}

TopoDS_Wire Polywire::buildWire ()
{
    BRepBuilderAPI_MakeWire wireBuilder;
    long unsigned int i=0;
    while (i < shapePoints.size()-1) {
        TopoDS_Edge edge=BRepBuilderAPI_MakeEdge(shapePoints[i],shapePoints[i+1]);
        wireBuilder.Add(edge);
        i++;
    }

    return wireBuilder.Wire();
}

void Line::drawRubberband ()
{
    //std::cout << "Line::drawRubberband  shapePoints.size()=" << shapePoints.size() << std::endl; std::cout.flush();

    if (shapePoints.size() == 0) return;

    if (!rubberband.IsNull()) {viewerContext->Remove(rubberband,Standard_True); rubberband.Nullify();}
    rubberband=CreateAISLineFromVertices(shapePoints[shapePoints.size()-1],currentMousePosition);

    if (!rubberband.IsNull()) {
        viewerContext->Display(rubberband,0,-1,Standard_True);
    }
}

void Polyline::drawRubberband ()
{
    //std::cout << "Polyline::drawRubberband  shapePoints.size()=" << shapePoints.size() << std::endl; std::cout.flush();

    if (shapePoints.size() == 0) return;

    if (!rubberband.IsNull()) {viewerContext->Remove(rubberband,Standard_True); rubberband.Nullify();}

    BRepBuilderAPI_MakePolygon polyMaker;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        polyMaker.Add(shapePoints[i]);
        i++;
    }
    polyMaker.Add(currentMousePosition);

    TopoDS_Wire wire=polyMaker.Wire();
    rubberband=new AIS_Shape(wire);

    if (!rubberband.IsNull()) {
        viewerContext->Display(rubberband,0,-1,Standard_True);
    }
}
