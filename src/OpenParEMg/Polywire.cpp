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
#include <BRepTools_WireExplorer.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>

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
{
    modified=false;
}

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

gp_Pnt Polywire::getPosition ()
{
    gp_Pnt position(0,0,0);
    if (shapePoints.size() > 0) position=shapePoints[0];
    return position;
}

TopoDS_Wire Polywire::buildWire ()
{
    BRepBuilderAPI_MakeWire wireBuilder;
    long unsigned int i=0;
    while (i < shapePoints.size()-1) {
        if (shapePoints[i].IsEqual(shapePoints[i+1],Precision::Confusion())) {
            std::cout << "ASSERT: Polywire::buildWire found duplicate points" << std::endl; std::cout.flush();
        } else {
            TopoDS_Edge edge=BRepBuilderAPI_MakeEdge(shapePoints[i],shapePoints[i+1]);
            wireBuilder.Add(edge);
        }
        i++;
    }

    return wireBuilder.Wire();
}

void Polywire::moveTo (gp_Pnt &pnt)
{
    std::cout << "Polywire::moveTo  pnt=(" << pnt.X() << "," << pnt.Y() << "," << pnt.Z() << ")" << std::endl; std::cout.flush();

    if (shapePoints.size() == 0) return;
    modified=true;

    gp_Pnt offset;
    offset=shapePoints[0].XYZ()-pnt.XYZ();

    long unsigned int i=0;
    while (i < shapePoints.size()) {
        shapePoints[i]=shapePoints[i].XYZ()-offset.XYZ();
        i++;
    }
}

void Polywire::shift (gp_Pnt &pnt1, gp_Pnt &pnt2)
{
    if (shapePoints.size() == 0) return;
    modified=true;

    gp_Pnt offset;
    offset=pnt2.XYZ()-pnt1.XYZ();

    long unsigned int i=0;
    while (i < shapePoints.size()) {
        shapePoints[i]=shapePoints[i].XYZ()-offset.XYZ();
        i++;
    }
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

void Polyline::buildFromFace (TopoDS_Face &face)
{
    // use the outer wire
    TopoDS_Wire wire=BRepTools::OuterWire(face);

    // transfer vertices
    BRepTools_WireExplorer wireExp(wire,face);
    while (wireExp.More())
    {
        gp_Pnt point=BRep_Tool::Pnt(wireExp.CurrentVertex());
        shapePoints.push_back(point);
        wireExp.Next();
    }
}

bool Rectangle::isValidPoint (gp_Pnt &pnt)
{
    if (shapePoints.size() == 0) return true;
    if (shapePoints[0].IsEqual(pnt,Precision::Confusion())) return false;
    return true;
}

void Rectangle::addPoint (gp_Pnt &pnt)
{
    if (shapePoints.size() == 0) {
        shapePoints.push_back(pnt);
    } else {
        currentMousePosition=pnt;
        drawRubberband();
    }
}

void Rectangle::drawRubberband ()
{
    //std::cout << "Rectangle::drawRubberband  shapePoints.size()=" << shapePoints.size() << std::endl; std::cout.flush();

    if (shapePoints.size() == 0) return;

    if (!rubberband.IsNull()) {viewerContext->Remove(rubberband,Standard_True); rubberband.Nullify();}

    // ensure enough space
    if (shapePoints.size() < 5) {
        long unsigned int i=0;
        while (i < 5) {
            if (shapePoints.size() == 5) break;
            shapePoints.push_back(shapePoints[0]); // dummy data except that shapePoints[4]=shapePoints[0] closes the shape
            i++;
        }
    }

    // current location
    shapePoints[2]=currentMousePosition;

    // diagonal vector
    gp_Vec d(shapePoints[0],currentMousePosition);

    // u, v directions

    u.SetCoord(1,0,0);
    gp_Vec test=normal.Crossed(u);
    if (test.IsEqual(gp_Vec(0,0,0),Precision::Confusion(),Precision::Confusion())) {
        u.SetCoord(0,1,0);
    }

    v=normal.Crossed(u).Normalized();

    // the other two points

    width=d.Dot(u);
    height=d.Dot(v);

    shapePoints[1]=shapePoints[0].Translated(u*width);
    shapePoints[3]=shapePoints[0].Translated(v*height);

    // build wire

    BRepBuilderAPI_MakePolygon polyMaker;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        polyMaker.Add(shapePoints[i]);
        i++;
    }

    TopoDS_Wire wire=polyMaker.Wire();
    rubberband=new AIS_Shape(wire);

    if (!rubberband.IsNull()) {
        viewerContext->Display(rubberband,0,-1,Standard_True);
    }
}

// for change in width and/or height
void Rectangle::recalculate ()
{
    modified=true;
    shapePoints[1]=shapePoints[0].Translated(u*width);
    shapePoints[2]=shapePoints[0].Translated(u*width).Translated(v*height);
    shapePoints[3]=shapePoints[0].Translated(v*height);
    shapePoints[4]=shapePoints[0];
}

void Polycircle::drawRubberband ()
{
    //std::cout << "Polycircle::drawRubberband" << std::endl; std::cout.flush();

    if (!centerPointSet) return;

    // reset
    if (!rubberband.IsNull()) {viewerContext->Remove(rubberband,Standard_True); rubberband.Nullify();}
    shapePoints.clear();

    // shape

    gp_Ax1 axis(centerPoint,normal);
    firstPoint=currentMousePosition;

    double step=2.0*M_PI/vertexCount;

    int i=0;
    while (i < vertexCount) {
        gp_Trsf rot;
        rot.SetRotation(axis,i*step);

        gp_Pnt p=firstPoint;;
        p.Transform(rot);

        shapePoints.push_back(p);

        i++;
    }
    shapePoints.push_back(shapePoints[0]);

    // build wire

    BRepBuilderAPI_MakePolygon polyMaker;
    long unsigned int j=0;
    while (j < shapePoints.size()) {
        polyMaker.Add(shapePoints[j]);
        j++;
    }

    TopoDS_Wire wire=polyMaker.Wire();
    rubberband=new AIS_Shape(wire);

    if (!rubberband.IsNull()) {
        viewerContext->Display(rubberband,0,-1,Standard_True);
    }
}

bool Polycircle::isValidPoint (gp_Pnt &pnt)
{
    if (!centerPointSet) return true;
    if (centerPoint.IsEqual(pnt,Precision::Confusion())) return false;
    return true;
}

void Polycircle::addPoint (gp_Pnt &pnt)
{
    if (!centerPointSet) {
        centerPoint=pnt;
        centerPointSet=true;
    } else {
        firstPoint=pnt;
        firstPointSet=true;
        currentMousePosition=pnt;
        drawRubberband();
    }
}

void Polycircle::recalculate ()
{
    shapePoints.clear();

    gp_Ax1 axis(centerPoint,normal);
    double step=2.0*M_PI/vertexCount;

    int i=0;
    while (i < vertexCount) {
        gp_Trsf rot;
        rot.SetRotation(axis,i*step);

        gp_Pnt p=firstPoint;;
        p.Transform(rot);

        shapePoints.push_back(p);

        i++;
    }
}
