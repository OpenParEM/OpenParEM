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
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GC_MakeSegment.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <Geom_Curve.hxx>
#include <GC_MakeSegment.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <Precision.hxx>
#include <algorithm>


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

// void printPnt (std::string &name, const gp_Pnt &p)
// {
//     std::cout << name << "=(" << p.X() << "," << p.Y() << "," << p.Z() << ")" << std::endl; std::cout.flush();
// }

// DoSegmentsIntersectInterior courtesy of Google AI
bool DoSegmentsIntersectInterior (const gp_Pnt& P1, const gp_Pnt& P2,
                                  const gp_Pnt& P3, const gp_Pnt& P4,
                                  double tol = Precision::Confusion())
{
    gp_Vec v1(P1, P2);
    gp_Vec v2(P3, P4);

    // Guard against zero-length segments
    if (v1.SquareMagnitude() < tol * tol || v2.SquareMagnitude() < tol * tol)
        return false;

    // 1. Handle Parallel/Collinear case (Avoids the crash)
    if (v1.IsParallel(v2, Precision::Angular())) {
        // Check if they lie on the same line (cross product of P1P3 and v1)
        gp_Vec v13(P1, P3);
        if (v1.CrossSquareMagnitude(v13) > tol * tol)
            return false; // Parallel but separate

        // 1D overlap check: Project P3 and P4 onto segment P1-P2
        auto getT = [&](const gp_Pnt& P) {
            return gp_Vec(P1, P).Dot(v1) / v1.SquareMagnitude();
        };
        double t3 = getT(P3);
        double t4 = getT(P4);

        double min_t = std::min(t3, t4);
        double max_t = std::max(t3, t4);

        // Interior overlap: the intervals [0,1] and [min_t, max_t]
        // must overlap by more than just a point at 0 or 1.
        return (min_t < (1.0 - tol) && max_t > tol);
    }

    // 2. Non-parallel case: Safe to use Extrema
    Handle(Geom_TrimmedCurve) seg1 = GC_MakeSegment(P1, P2).Value();
    Handle(Geom_TrimmedCurve) seg2 = GC_MakeSegment(P3, P4).Value();

    GeomAPI_ExtremaCurveCurve extrema(seg1, seg2);

    // Instead of IsDone(), check NbExtrema()
    Standard_Integer nb = extrema.NbExtrema();
    if (nb == 0) return false;

    for (int i = 1; i <= nb; ++i) {
        if (extrema.Distance(i) <= tol) {
            Standard_Real u, v;
            extrema.Parameters(i, u, v);

            // Convert parameters to 0-1 range for boundary checks
            double u_norm = u / P1.Distance(P2);
            double v_norm = v / P3.Distance(P4);

            if (u_norm > tol && u_norm < (1.0 - tol) &&
                v_norm > tol && v_norm < (1.0 - tol)) {
                return true;
            }
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
// Polywire
////////////////////////////////////////////////////////////////////////////////

Polywire::Polywire(QObject *parent)
    : QObject{parent}
{
    modified=false;
    closed=false;
}

void Polywire::deleteRubberband ()
{
    if (!rubberband.IsNull()) {viewerContext->Remove(rubberband,Standard_True); rubberband.Nullify();}
}

bool Polywire::isValidPoint (gp_Pnt &pnt, bool zeroPntLogic)
{
    //std::cout << "Polywire::isValidPoint" << std::endl; std::cout.flush();

    if (zeroPntLogic) {if (shapePoints.size() == 0) return true;}
    else {if (shapePoints.size() == 0) return false;}

    if (shapePoints[shapePoints.size()-1].IsEqual(pnt,Precision::Confusion())) return false;

    // check for intersections
    long unsigned int i=0;
    while (i < shapePoints.size()-1) {
        if (DoSegmentsIntersectInterior(shapePoints[i], shapePoints[i+1],
                                        shapePoints[shapePoints.size()-1],pnt,
                                        1e-12)) return false;
        i++;
    }

    return true;
}

void Polywire::addPoint (gp_Pnt &pnt)
{
    if (shapePoints.size() > 0) {
        if (shapePoints[0].IsEqual(pnt,Precision::Confusion())) closed=true;
    }
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
        closed=true;
    }
}

gp_Pnt Polywire::getPosition ()
{
    gp_Pnt position(0,0,0);
    if (shapePoints.size() > 0) position=shapePoints[0];
    return position;
}

gp_Pln Polywire::getPlane ()
{
    gp_Pln plane;
    if (shapePoints.size() > 0) {
        gp_Pln tempPlane(shapePoints[0],normal);
        plane=tempPlane;
    }
    return plane;
}

void Polywire::setEditIndex (gp_Pnt &pnt)
{
    double closest=DBL_MAX;
    long unsigned i=0;
    while (i < shapePoints.size()) {
        double distance=shapePoints[i].Distance(pnt);
        if (distance < closest) {
            closest=distance;
            editIndex=i;
        }
        i++;
    }
}

void Polywire::setEditPoint (gp_Pnt &pnt) {
    shapePoints[editIndex]=pnt;
}

TopoDS_Wire Polywire::buildWire ()
{
    int count=0;
    BRepBuilderAPI_MakeWire wireBuilder;
    long unsigned int i=0;
    while (i < shapePoints.size()-1) {
        if (shapePoints[i].IsEqual(shapePoints[i+1],Precision::Confusion())) {
            //std::cout << "ASSERT: Polywire::buildWire found duplicate points" << std::endl; std::cout.flush();
        } else {
            count++;
            TopoDS_Edge edge=BRepBuilderAPI_MakeEdge(shapePoints[i],shapePoints[i+1]);
            wireBuilder.Add(edge);
        }
        i++;
    }

    TopoDS_Wire wire;
    if (count > 0) wire=wireBuilder.Wire();

    return wire;
}

bool Polywire::isPointOnPlane (gp_Pnt &pnt)
{
    if (shapePoints.size() == 0) return false;
    gp_Pln plane(shapePoints[0],normal);
    if (plane.Distance(pnt) < Precision::Confusion()) return true;
    return false;
}

void Polywire::shift (gp_Pnt &pnt1, gp_Pnt &pnt2)
{
    //std::cout << "Polywire::shift" << std::endl; std::cout.flush();

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

void Polywire::rotate (double &angleDegrees, gp_Pnt &p1, gp_Pnt &p2)
{
    if (shapePoints.size() == 0) return;
    modified=true;

    gp_Dir dir(gp_Vec(p1,p2));
    gp_Ax1 axis(p1,dir);

    double angleRadians=angleDegrees*M_PI/180;

    // points
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        shapePoints[i]=shapePoints[i].Rotated(axis,angleRadians);
        i++;
    }

    // normal
    normal=normal.Rotated(axis,angleRadians);
}

////////////////////////////////////////////////////////////////////////////////
// Line
////////////////////////////////////////////////////////////////////////////////

gp_Pnt Line::getP0 ()
{
    gp_Pnt P0;
    if (shapePoints.size() > 0) P0=shapePoints[0];
    return P0;
}

gp_Pnt Line::getP1 ()
{
    gp_Pnt P1;
    if (shapePoints.size() > 1) P1=shapePoints[1];
    return P1;
}

void Line::setP0 (gp_Pnt &P0)
{
    if (shapePoints.size() == 0) shapePoints.push_back(P0);
    else shapePoints[0]=P0;
}

void Line::setP1 (gp_Pnt &P1)
{
    if (shapePoints.size() == 0) shapePoints.push_back(gp_Pnt(0,0,0));
    if (shapePoints.size() == 1) shapePoints.push_back(P1);
    else shapePoints[1]=P1;
}

void Line::drawRubberband ()
{
    //std::cout << "Line::drawRubberband  shapePoints.size()=" << shapePoints.size() << std::endl; std::cout.flush();

    if (!isValidPoint(currentMousePosition,false)) return;

    if (!rubberband.IsNull()) {viewerContext->Remove(rubberband,Standard_True); rubberband.Nullify();}
    rubberband=CreateAISLineFromVertices(shapePoints[shapePoints.size()-1],currentMousePosition);

    if (!rubberband.IsNull()) {
        viewerContext->Display(rubberband,0,-1,Standard_True);
    }
}

void Line::drawStretchRubberband ()
{
    //std::cout << "Line::drawStretchRubberband  shapePoints.size()=" << shapePoints.size() << std::endl; std::cout.flush();

    if (!rubberband.IsNull()) {viewerContext->Remove(rubberband,Standard_True); rubberband.Nullify();}

    gp_Pnt p0=shapePoints[0];
    gp_Pnt p1=shapePoints[1];

    if (editIndex == 0) p0=currentMousePosition;
    if (editIndex == 1) p1=currentMousePosition;

    rubberband=CreateAISLineFromVertices(p0,p1);

    if (!rubberband.IsNull()) {
        viewerContext->Display(rubberband,0,-1,Standard_True);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Polyline
////////////////////////////////////////////////////////////////////////////////

bool Polyline::canClose ()
{
    if (shapePoints.size() < 3) return false;

    long unsigned int i=0;
    while (i < shapePoints.size()-2) {
        if (DoSegmentsIntersectInterior(shapePoints[i],shapePoints[i+1],
                                        shapePoints[0],shapePoints[shapePoints.size()-1],
                                        1e-12)) {
            return false;
        }
        i++;
    }

    return true;
}

TopoDS_Face Polyline::buildFace (TopoDS_Wire &wire)
{
    TopoDS_Face face;
    if (wire.IsNull()) return face;

    if (closed) {
        if (wire.Closed()) {
            BRepBuilderAPI_MakeFace faceBuilder(wire);
            if (faceBuilder.IsDone()) {
                face=faceBuilder.Face();
            }
        }
    }
    return face;
}

void Polyline::drawRubberband ()
{
    //std::cout << "Polyline::drawRubberband  shapePoints.size()=" << shapePoints.size() << std::endl; std::cout.flush();

    if (!isValidPoint(currentMousePosition,false)) return;

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

void Polyline::drawStretchRubberband ()
{
    //std::cout << "Polyline::drawStretchRubberband  shapePoints.size()=" << shapePoints.size() << std::endl; std::cout.flush();

    if (checkIntersection && shapePoints.size() > 2) {
        bool skipTest=false;

        gp_Pnt t1,t2;
        if (closed) {
            if (editIndex == 0) t1=shapePoints[shapePoints.size()-2];
            else t1=shapePoints[editIndex-1];

            if (editIndex == shapePoints.size()-1) t2=shapePoints[1];
            else t2=shapePoints[editIndex+1];
        } else {
            if (editIndex == 0) skipTest=true;
            else t1=shapePoints[editIndex-1];

            if (editIndex == shapePoints.size()-1) skipTest=true;
            else t2=shapePoints[editIndex+1];
        }

        if (editIndex > 0) {
            long unsigned int i=0;
            while (i < editIndex-1) {
                if (DoSegmentsIntersectInterior(shapePoints[i],shapePoints[i+1],t1,currentMousePosition,1e-12)) {
                    //std::cout << "Fail on test 1" << std::endl; std::cout.flush();
                    return;
                }
                if (!skipTest && DoSegmentsIntersectInterior(shapePoints[i],shapePoints[i+1],t2,currentMousePosition,1e-12)) {
                    //std::cout << "Fail on test 2" << std::endl; std::cout.flush();
                    return;
                }
                i++;
            }
        }

        long unsigned int limit=shapePoints.size()-1;
        if (closed) limit=shapePoints.size()-2;
        long unsigned int i=editIndex+1;
        while (i < limit) {
            if (!skipTest && DoSegmentsIntersectInterior(shapePoints[i],shapePoints[i+1],t1,currentMousePosition,1e-12)) {
                //std::cout << "Fail on test 3" << std::endl; std::cout.flush();
                return;
            }
            if (DoSegmentsIntersectInterior(shapePoints[i],shapePoints[i+1],t2,currentMousePosition,1e-12)) {
                //std::cout << "Fail on test 4" << std::endl; std::cout.flush();
                return;
            }
            i++;
        }
    }

    if (!rubberband.IsNull()) {viewerContext->Remove(rubberband,Standard_True); rubberband.Nullify();}

    BRepBuilderAPI_MakePolygon polyMaker;
    if (closed) {
        if (editIndex == 0) {
            long unsigned int i=0;
            while (i < shapePoints.size()-1) {
                if (i == editIndex) polyMaker.Add(currentMousePosition);
                else polyMaker.Add(shapePoints[i]);
                i++;
            }
            polyMaker.Add(currentMousePosition);
        } else if (editIndex == shapePoints.size()-1) {
            polyMaker.Add(currentMousePosition);
            long unsigned int i=1;
            while (i < shapePoints.size()) {
                if (i == editIndex) polyMaker.Add(currentMousePosition);
                else polyMaker.Add(shapePoints[i]);
                i++;
            }
        } else {
            long unsigned int i=0;
            while (i < shapePoints.size()) {
                if (i == editIndex) polyMaker.Add(currentMousePosition);
                else polyMaker.Add(shapePoints[i]);
                i++;
            }
        }
    } else {
        long unsigned int i=0;
        while (i < shapePoints.size()) {
            if (i == editIndex) polyMaker.Add(currentMousePosition);
            else polyMaker.Add(shapePoints[i]);
            i++;
        }
    }

    TopoDS_Wire wire=polyMaker.Wire();
    rubberband=new AIS_Shape(wire);

    if (!rubberband.IsNull()) {
        viewerContext->Display(rubberband,0,-1,Standard_True);
    }
}

void Polyline::setEditPoint (gp_Pnt &pnt)
{
    if (closed) {
        if (editIndex == 0 || editIndex == shapePoints.size()-1) {
            shapePoints[0]=pnt;
            shapePoints[shapePoints.size()-1]=pnt;
        } else {
            shapePoints[editIndex]=pnt;
        }
    } else {
        shapePoints[editIndex]=pnt;
    }
}

bool Polyline::canDeletePoint ()
{
    if (closed) {
        if (shapePoints.size() > 4) return true;
    } else {
        if (shapePoints.size() > 2) return true;
    }
    return false;
}

void Polyline::deletePoint (gp_Pnt &pnt)
{
    long unsigned int deleteIndex=0;
    double closest=DBL_MAX;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        double distance=shapePoints[i].Distance(pnt);
        if (distance < closest) {
            closest=distance;
            deleteIndex=i;
        }
        i++;
    }

    if (closed) {
        if (deleteIndex == 0 || deleteIndex == shapePoints.size()-1) {
            shapePoints.erase(shapePoints.begin());
            shapePoints[shapePoints.size()-1]=shapePoints[0];
        } else {
            shapePoints.erase(shapePoints.begin()+deleteIndex);
        }
    } else {
        shapePoints.erase(shapePoints.begin()+deleteIndex);
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

////////////////////////////////////////////////////////////////////////////////
// Rectangle
////////////////////////////////////////////////////////////////////////////////

Rectangle::Rectangle (Rectangle *rectangle)
{
    u=rectangle->u;
    v=rectangle->v;
    width=rectangle->width;
    height=rectangle->height;
    normal=rectangle->normal;

    long unsigned int i=0;
    while (i < rectangle->shapePoints.size()) {
        shapePoints.push_back(rectangle->shapePoints[i]);
        i++;
    }

    viewerContext=rectangle->viewerContext;
    modified=rectangle->modified;
}

bool Rectangle::isValidPoint (gp_Pnt &pnt, bool zeroPntLogic)
{
    if (zeroPntLogic) {if (shapePoints.size() == 0) return true;}
    else {if (shapePoints.size() == 0) return false;}

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

    if (!isValidPoint(currentMousePosition,false)) return;

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

    // v direction
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

void Rectangle::drawStretchRubberband ()
{
    //std::cout << "Rectangle::drawStretchedRubberband  editIndex=" << editIndex << std::endl; std::cout.flush();

    //if (!isValidPoint(currentMousePosition,false)) return;

    if (!rubberband.IsNull()) {viewerContext->Remove(rubberband,Standard_True); rubberband.Nullify();}

    std::vector<gp_Pnt> tempShapePoints;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        tempShapePoints.push_back(shapePoints[i]);
        i++;
    }

    if (editIndex == 0 || editIndex == 4) {
        gp_Vec d(currentMousePosition,tempShapePoints[2]);
        double tempWidth=d.Dot(u);
        double tempHeight=d.Dot(v);

        if (abs(tempWidth) > Precision::Confusion() && abs(tempHeight) > Precision::Confusion()) {
            tempShapePoints[0]=currentMousePosition;
            tempShapePoints[1]=tempShapePoints[2].Translated(-v*tempHeight);
            tempShapePoints[3]=tempShapePoints[2].Translated(-u*tempWidth);
            tempShapePoints[4]=tempShapePoints[0];
        }
    } else if (editIndex == 1) {
        gp_Vec d(tempShapePoints[3],currentMousePosition);
        double tempWidth=d.Dot(u);
        double tempHeight=-d.Dot(v);

        if (abs(tempWidth) > Precision::Confusion() && abs(tempHeight) > Precision::Confusion()) {
            tempShapePoints[0]=tempShapePoints[3].Translated(-v*tempHeight);
            tempShapePoints[1]=currentMousePosition;
            tempShapePoints[2]=tempShapePoints[3].Translated(u*tempWidth);
            tempShapePoints[4]=tempShapePoints[0];
        }
    } else if (editIndex == 2) {
        gp_Vec d(tempShapePoints[0],currentMousePosition);
        double tempWidth=d.Dot(u);
        double tempHeight=d.Dot(v);

        if (abs(tempWidth) > Precision::Confusion() && abs(tempHeight) > Precision::Confusion()) {
            tempShapePoints[1]=tempShapePoints[0].Translated(u*tempWidth);
            tempShapePoints[2]=currentMousePosition;
            tempShapePoints[3]=tempShapePoints[0].Translated(v*tempHeight);
            tempShapePoints[4]=tempShapePoints[0];
        }
    } else if (editIndex == 3) {
        gp_Vec d(currentMousePosition,tempShapePoints[1]);
        double tempWidth=d.Dot(u);
        double tempHeight=-d.Dot(v);

        if (abs(tempWidth) > Precision::Confusion() && abs(tempHeight) > Precision::Confusion()) {
            tempShapePoints[0]=tempShapePoints[1].Translated(-u*tempWidth);
            tempShapePoints[2]=tempShapePoints[1].Translated(v*tempHeight);
            tempShapePoints[3]=currentMousePosition;
            tempShapePoints[4]=tempShapePoints[0];
        }
    }

    // build wire

    BRepBuilderAPI_MakePolygon polyMaker;
    i=0;
    while (i < tempShapePoints.size()) {
        polyMaker.Add(tempShapePoints[i]);
        i++;
    }

    TopoDS_Wire wire=polyMaker.Wire();
    rubberband=new AIS_Shape(wire);

    if (!rubberband.IsNull()) {
        viewerContext->Display(rubberband,0,-1,Standard_True);
    }
}

TopoDS_Face Rectangle::buildFace (TopoDS_Wire &wire)
{
    TopoDS_Face face;
    if (wire.IsNull()) return face;

    if (wire.Closed()) {
        BRepBuilderAPI_MakeFace faceBuilder(wire);
        if (faceBuilder.IsDone()) {
            face=faceBuilder.Face();
        }
    }
    return face;
}

void Rectangle::setEditPoint (gp_Pnt &pnt)
{
    if (editIndex == 0 || editIndex == 4) {
        gp_Vec d(pnt,shapePoints[2]);
        width=d.Dot(u);
        height=d.Dot(v);

        shapePoints[0]=pnt;
        shapePoints[1]=shapePoints[2].Translated(-v*height);
        shapePoints[3]=shapePoints[2].Translated(-u*width);
        shapePoints[4]=shapePoints[0];
    } else if (editIndex == 1) {
        gp_Vec d(shapePoints[3],pnt);
        width=d.Dot(u);
        height=-d.Dot(v);

        shapePoints[0]=shapePoints[3].Translated(-v*height);
        shapePoints[1]=pnt;
        shapePoints[2]=shapePoints[3].Translated(u*width);
        shapePoints[4]=shapePoints[0];
    } else if (editIndex == 2) {
        gp_Vec d(shapePoints[0],pnt);
        width=d.Dot(u);
        height=d.Dot(v);

        shapePoints[1]=shapePoints[0].Translated(u*width);
        shapePoints[2]=pnt;
        shapePoints[3]=shapePoints[0].Translated(v*height);
        shapePoints[4]=shapePoints[0];
    } else if (editIndex == 3) {
        gp_Vec d(pnt,shapePoints[1]);
        width=d.Dot(u);
        height=-d.Dot(v);

        shapePoints[0]=shapePoints[1].Translated(-u*width);
        shapePoints[2]=shapePoints[1].Translated(v*height);
        shapePoints[3]=pnt;
        shapePoints[4]=shapePoints[0];
    }
}

void Rectangle::recalculate ()
{
    modified=true;
    shapePoints[1]=shapePoints[0].Translated(u*width);
    shapePoints[2]=shapePoints[0].Translated(u*width).Translated(v*height);
    shapePoints[3]=shapePoints[0].Translated(v*height);
    shapePoints[4]=shapePoints[0];
}

// for change in origin
void Rectangle::recalculate (gp_Pnt p0)
{
    shapePoints[0]=p0;

    shapePoints[1]=shapePoints[0].Translated(u*width);
    shapePoints[2]=shapePoints[0].Translated(u*width).Translated(v*height);
    shapePoints[3]=shapePoints[0].Translated(v*height);
    shapePoints[4]=shapePoints[0];
}

// for change in corner points
void Rectangle::recalculate (gp_Pnt p0, gp_Pnt p1)
{
    shapePoints[0]=p0;

    gp_Vec d(p0,p1);

    width=d.Dot(u);
    height=d.Dot(v);

    shapePoints[1]=shapePoints[0].Translated(u*width);
    shapePoints[2]=shapePoints[0].Translated(u*width).Translated(v*height);
    shapePoints[3]=shapePoints[0].Translated(v*height);
    shapePoints[4]=shapePoints[0];
}

gp_Pnt Rectangle::getOppositeCorner ()
{
    gp_Pnt position(0,0,0);
    if (shapePoints.size() > 3) position=shapePoints[2];
    return position;
}

void Rectangle::rotate (double &angleDegrees, gp_Pnt &p1, gp_Pnt &p2)
{
    modified=true;

    gp_Dir dir(gp_Vec(p1,p2));
    gp_Ax1 axis(p1,dir);

    double angleRadians=angleDegrees*M_PI/180;

    // u and v
    u=u.Rotated(axis,angleRadians);
    v=v.Rotated(axis,angleRadians);

    if (shapePoints.size() > 0) shapePoints[0]=shapePoints[0].Rotated(axis,angleRadians);

    // normal
    normal=normal.Rotated(axis,angleRadians);

    recalculate();
}

////////////////////////////////////////////////////////////////////////////////
// Polycircle
////////////////////////////////////////////////////////////////////////////////

Polycircle::Polycircle (Polycircle *polycircle)
{
    normal=polycircle->normal;

    centerPointSet=polycircle->centerPointSet;
    centerPoint=polycircle->centerPoint;
    firstPointSet=polycircle->firstPointSet;
    firstPoint=polycircle->firstPoint;
    vertexCount=polycircle->vertexCount;;

    long unsigned int i=0;
    while (i < polycircle->shapePoints.size()) {
        shapePoints.push_back(polycircle->shapePoints[i]);
        i++;
    }

    viewerContext=polycircle->viewerContext;
    modified=polycircle->modified;
}

void Polycircle::drawRubberband ()
{
    //std::cout << "Polycircle::drawRubberband" << std::endl; std::cout.flush();

    if (!isValidPoint(currentMousePosition,false)) return;

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

void Polycircle::drawStretchRubberband ()
{
    //std::cout << "Polycircle::drawRubberband" << std::endl; std::cout.flush();

    //if (!isValidPoint(currentMousePosition,false)) return;

    // reset
    if (!rubberband.IsNull()) {viewerContext->Remove(rubberband,Standard_True); rubberband.Nullify();}

    // shape

    gp_Ax1 axis(centerPoint,normal);
    firstPoint=currentMousePosition;

    double step=2.0*M_PI/vertexCount;

    std::vector<gp_Pnt> tempShapePoints;
    int i=0;
    while (i < vertexCount) {
        gp_Trsf rot;
        rot.SetRotation(axis,i*step);

        gp_Pnt p=firstPoint;;
        p.Transform(rot);

        tempShapePoints.push_back(p);

        i++;
    }
    tempShapePoints.push_back(tempShapePoints[0]);

    // build wire

    BRepBuilderAPI_MakePolygon polyMaker;
    long unsigned int j=0;
    while (j < tempShapePoints.size()) {
        polyMaker.Add(tempShapePoints[j]);
        j++;
    }

    TopoDS_Wire wire=polyMaker.Wire();
    rubberband=new AIS_Shape(wire);

    if (!rubberband.IsNull()) {
        viewerContext->Display(rubberband,0,-1,Standard_True);
    }
}

TopoDS_Face Polycircle::buildFace (TopoDS_Wire &wire)
{
    TopoDS_Face face;
    if (wire.IsNull()) return face;

    if (wire.Closed()) {
        BRepBuilderAPI_MakeFace faceBuilder(wire);
        if (faceBuilder.IsDone()) {
            face=faceBuilder.Face();
        }
    }
    return face;
}

void Polycircle::setEditPoint (gp_Pnt &pnt)
{
    // skip for invalid circle
    if (centerPoint.IsEqual(pnt,Precision::Confusion())) return;

    gp_Ax1 axis(centerPoint,normal);
    firstPoint=pnt;

    double step=2.0*M_PI/vertexCount;

    shapePoints.clear();
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
}

bool Polycircle::isValidPoint (gp_Pnt &pnt, bool zeroPntLogic)
{
    if (zeroPntLogic) {if (!centerPointSet) return true;}
    else {if (!centerPointSet) return false;}

    if (centerPoint.IsEqual(pnt,Precision::Confusion())) return false;
    return true;
}

void Polycircle::addPoint (gp_Pnt &pnt)
{
    if (!centerPointSet) {
        centerPoint=pnt;
        centerPointSet=true;
    } else {
        if (isValidPoint(pnt,false)) {
            firstPoint=pnt;
            firstPointSet=true;
            currentMousePosition=pnt;
            drawRubberband();
        }
    }
}

void Polycircle::recalculate ()
{
    //std::cout << "Polycircle::recalculate" << std::endl; std::cout.flush();

    shapePoints.clear();

    gp_Ax1 axis(centerPoint,normal);
    double step=2.0*M_PI/vertexCount;

    int i=0;
    while (i < vertexCount) {
        gp_Trsf rot;
        rot.SetRotation(axis,i*step);

        gp_Pnt p=firstPoint;
        p.Transform(rot);

        shapePoints.push_back(p);

        i++;
    }
    shapePoints.push_back(shapePoints[0]);
}

bool Polycircle::isPointOnPlane (gp_Pnt &pnt)
{
    if (centerPoint.Distance(pnt) < Precision::Confusion()) return true;

    gp_Pln plane(centerPoint,normal);
    if (plane.Distance(pnt) < Precision::Confusion()) return true;
    return false;
}

void Polycircle::shift (gp_Pnt &pnt1, gp_Pnt &pnt2)
{
    //std::cout << "Polycircle::shift" << std::endl; std::cout.flush();

    modified=true;
    if (!centerPointSet) return;
    if (!firstPointSet) return;

    gp_Pnt offset;
    offset=pnt2.XYZ()-pnt1.XYZ();

    centerPoint=centerPoint.XYZ()-offset.XYZ();
    firstPoint=firstPoint.XYZ()-offset.XYZ();

    recalculate();
}

gp_Pln Polycircle::getPlane ()
{
    gp_Pln plane;
    if (centerPointSet) {
        gp_Pln tempPlane(centerPoint,normal);
        plane=tempPlane;
    }
    return plane;
}

void Polycircle::rotate (double &angleDegrees, gp_Pnt &p1, gp_Pnt &p2)
{
    modified=true;

    gp_Dir dir(gp_Vec(p1,p2));
    gp_Ax1 axis(p1,dir);

    double angleRadians=angleDegrees*M_PI/180;

    // points
    centerPoint=centerPoint.Rotated(axis,angleRadians);
    firstPoint=firstPoint.Rotated(axis,angleRadians);

    // normal
    normal=normal.Rotated(axis,angleRadians);

    recalculate();
}
