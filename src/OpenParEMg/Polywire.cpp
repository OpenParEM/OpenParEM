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
#include <AIS_ViewController.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRep_Builder.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
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
#include "path.hpp"


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

void printPnt (std::string &name, const gp_Pnt &p)
{
    std::cout << "   " << name << "=(" << p.X() << "," << p.Y() << "," << p.Z() << ")" << std::endl;  std::cout.flush();
}

void printVec (std::string &name, const gp_Vec &p)
{
    std::cout << "   " << name << "=(" << p.X() << "," << p.Y() << "," << p.Z() << ")" << std::endl;  std::cout.flush();
}

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

// courtesy of ChatGPT
std::string trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        start++;
    }

    if (start == str.size()) return "";

    size_t end = str.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(str[end]))) {
        end--;
    }

    return str.substr(start, end - start + 1);
}

// based on routine provided by ChatGPT
// parse keyword=text
// return true on success
bool extractText(const std::string& input, std::string& keyword, std::string& value) {
    size_t pos = input.find('=');

    // Must contain '=' and not at beginning
    if (pos == std::string::npos || pos == 0) return false;

    std::string name=input.substr(0,pos);
    std::string text=input.substr(pos+1);

    name=trim(name);
    text=trim(text);

    if (name.empty()) return false;
    if (name.compare(keyword) != 0) return false;

    value=text;
    return true;
}

// based on routine provided by ChatGPT
// Parse keyword=(x,y,z)
// return true on success
bool extractPoint(const std::string& input, std::string& keyword, gp_Pnt& point) {
    size_t eqPos = input.find('=');
    if (eqPos == std::string::npos || eqPos == 0) {
        return false;
    }

    std::string name = trim(input.substr(0, eqPos));
    std::string rhs  = trim(input.substr(eqPos + 1));

    if (name.empty()) return false;
    if (name.compare(keyword) != 0) return false;

    // Must start with '(' and end with ')'
    if (rhs.size() < 5 || rhs.front() != '(' || rhs.back() != ')') {
        return false;
    }

    // Remove parentheses
    rhs = rhs.substr(1, rhs.size() - 2);

    // Parse three comma-separated values
    double values[3];
    size_t start = 0;
    int count = 0;

    while (start < rhs.size() && count < 3) {
        size_t comma = rhs.find(',', start);
        std::string token;

        if (comma == std::string::npos) {
            token = rhs.substr(start);
            start = rhs.size();
        } else {
            token = rhs.substr(start, comma - start);
            start = comma + 1;
        }

        token = trim(token);
        if (token.empty()) return false;

        try {
            values[count] = std::stod(token);
        } catch (...) {
            return false;
        }

        count++;
    }

    // Must have exactly 3 values and no extra data
    if (count != 3 || start < rhs.size()) {
        return false;
    }

    point = gp_Pnt(values[0], values[1], values[2]);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Polywire
////////////////////////////////////////////////////////////////////////////////

Polywire::Polywire(QObject *parent)
    : QObject{parent}
{
    modified=false;
    closed=false;
    reverseExtrusionDirection=false;
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

        // point to check
        long unsigned int index=shapePoints.size()-1;  // drawing
        if (!drawEnable) index=editIndex;              // stretching

        // check
        if (DoSegmentsIntersectInterior(shapePoints[i], shapePoints[i+1],
                                        shapePoints[index],pnt,
                                        1e-12)) {
            return false;
        }
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
    BRepBuilderAPI_MakeWire wireBuilder;
    int count=0;
    if (shapePoints.size() > 0) {
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

// re-use the basic code from Path::fill_wire_item [now removed]
void Polywire::addArrows (BRep_Builder& builder, TopoDS_Compound& compound)
{
    if (!hasArrows) return;

    // variable for use in the existing code

    keywordPair p1;
    keywordPair p2;

    struct point normal_sp;
    normal_sp.x=normal.X(); normal_sp.y=normal.Y(); normal_sp.z=normal.Z(); normal_sp.dim=3;

    // shortest segment
    double shortestLength=DBL_MAX;
    long unsigned int i=0;
    while (i < shapePoints.size()-1) {
        gp_Pnt from=shapePoints[i];
        gp_Pnt to=shapePoints[i+1];
        double length=from.Distance(to);
        if (length > 0 && length < shortestLength) shortestLength=length;
        i++;
    }

    // make arrows
    i=0;
    while (i < shapePoints.size()-1) {
        p1.set_point_value(shapePoints[i].X(),shapePoints[i].Y(),shapePoints[i].Z());
        p2.set_point_value(shapePoints[i+1].X(),shapePoints[i+1].Y(),shapePoints[i+1].Z());

        keywordPair *from=&p1;
        keywordPair *to=&p2;

        struct point center=point_midpoint(from->get_point_value(),to->get_point_value());
        struct point centerOffset=point_scale(shortestLength/20,point_normalize(point_subtraction(center,from->get_point_value())));
        struct point arrowOffset=point_scale(2,point_cross_product(normal_sp,centerOffset));

        keywordPair *tip=new keywordPair();
        tip->set_point_value(point_addition(center,centerOffset));

        keywordPair *p1=new keywordPair ();
        p1->set_point_value(point_subtraction(point_subtraction(center,centerOffset),arrowOffset));

        keywordPair *p2=new keywordPair ();
        p2->set_point_value(point_addition(point_subtraction(center,centerOffset),arrowOffset));

        Path arrowHead(0,0);
        arrowHead.set_closed(false);
        arrowHead.push_point(p1);
        arrowHead.push_point(tip);
        arrowHead.push_point(p2);

        builder.Add(compound,arrowHead.create_TopoDS_Wire());

        i++;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Line
////////////////////////////////////////////////////////////////////////////////

Line::Line (Line *line)
{
    normal=line->normal;
    modified=line->modified;
    drawEnable=line->drawEnable;
    editIndex=line->editIndex;
    closed=line->closed;
    reverseExtrusionDirection=line->reverseExtrusionDirection;

    long unsigned int i=0;
    while (i < line->shapePoints.size()) {
        shapePoints.push_back(line->shapePoints[i]);
        i++;
    }

    viewerContext=line->viewerContext;
}

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

Line* Line::copyCreate ()
{
    Line *newLine=new Line();
    newLine->modified=modified;
    newLine->closed=closed;
    newLine->hasArrows=hasArrows;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        newLine->shapePoints.push_back(shapePoints[i]);
        i++;
    }
    newLine->normal=normal;
    newLine->currentMousePosition=currentMousePosition;
    newLine->viewerContext=viewerContext;
    newLine->drawEnable=drawEnable;
    newLine->editIndex=editIndex;
    newLine->reverseExtrusionDirection=reverseExtrusionDirection;
    return newLine;
}

Handle(AIS_Shape) Line::get_AIS_Shape ()
{
    Handle(AIS_Shape) shape;
    TopoDS_Wire wire=buildWire();
    if (!wire.IsNull()) {
        if (hasArrows) {
            TopoDS_Compound compound;
            BRep_Builder builder;
            builder.MakeCompound(compound);
            builder.Add(compound,wire);
            addArrows(builder,compound);
            shape=new AIS_Shape(compound);
        } else {
            shape=new AIS_Shape(wire);
        }
    }
    return shape;
}

QString Line::getName (ObjectCounts *objectCounts) {
    objectCounts->line++;
    QString name="Line";
    name.append(QString::number(objectCounts->line));
    return name;
}

void Line::shift (gp_Pnt &pnt1, gp_Pnt &pnt2)
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

void Line::reverseOrder ()
{
    if (shapePoints.size() > 1) {
        gp_Pnt temp=shapePoints[0];
        shapePoints[0]=shapePoints[1];
        shapePoints[1]=temp;
    }
}

void Line::save (std::ofstream *out, QString name, int level)
{
    if (shapePoints.size() != 2) return;

    std::string space;
    long unsigned int i=0;
    while (i < level) {
        space.append("   ");
        i++;
    }

    *out << space << "Line" << std::endl;
    if (!name.isEmpty()) {
        *out << space << "   name=" << name.toStdString() << std::endl;
    }
    *out << space << "   point=("
         << shapePoints[0].X() << ","
         << shapePoints[0].Y() << ","
         << shapePoints[0].Z() << ")" << std::endl;

    *out << space << "   point=("
         << shapePoints[1].X() << ","
         << shapePoints[1].Y() << ","
         << shapePoints[1].Z() << ")" << std::endl;

    *out << space << "   normal=("
         << normal.X() << ","
         << normal.Y() << ","
         << normal.Z() << ")" << std::endl;

    *out << space << "   reverse=";
    if (reverseExtrusionDirection) *out << "true" << std::endl;
    else *out << "false" << std::endl;

    *out << space << "EndLine" << std::endl;
}

bool Line::load (std::vector<std::string> &inputData, long unsigned int start,
                long unsigned int end, std::string& name, ObjectCounts *objectCounts)
{
    bool foundName=false;
    bool foundNormal=false;
    bool foundReverse=false;

    long unsigned int i=start;
    while (i <= end) {

        gp_Pnt pnt;
        std::string keyword;

        // name
        keyword="name";
        std::string testName;
        if (extractText(inputData[i],keyword,testName)) {
            name=testName;
            foundName=true;
            i++;
            continue;
        }

        // point
        keyword="point";
        if (extractPoint(inputData[i],keyword,pnt)) {
            shapePoints.push_back(pnt);
            i++;
            continue;
        }

        // normal
        keyword="normal";
        if (extractPoint(inputData[i],keyword,pnt)) {
            setNormal(pnt);
            foundNormal=true;
            i++;
            continue;
        }

        // reverse
        keyword="reverse";
        std::string testReverse;
        if (extractText(inputData[i],keyword,testReverse)) {
            // expect true/false, otherwise no change from the default
            if (testReverse.compare("true") == 0) reverseExtrusionDirection=true;
            else if (testReverse.compare("false") == 0) reverseExtrusionDirection=false;
            foundReverse=true;
            i++;
            continue;
        }

        i++;
    }

    // check for completeness
    if (shapePoints.size() != 2) return true;
    if (!foundName) return true;
    if (!foundNormal) return true;
    if (!foundReverse) return true;

    objectCounts->line++;

    return false;
}

void Line::print ()
{
    std::cout << "Line:" << std::endl;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        std::string name="point";
        printPnt(name,shapePoints[i]);
        i++;
    }

    std::string name="normal";
    printVec(name,normal);
}

////////////////////////////////////////////////////////////////////////////////
// Polyline
////////////////////////////////////////////////////////////////////////////////

bool Polyline::canClose ()
{
    if (shapePoints.size() < 3) return false;
    if (closed) return false;

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

void Polyline::close ()
{
    if (shapePoints.size() > 2) {
        shapePoints.push_back(shapePoints[0]);
        closed=true;
    }
}

bool Polyline::canOpen ()
{
    if (closed) return true;
    return false;
}

void Polyline::open ()
{
    shapePoints.pop_back();
    closed=false;
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

bool Polyline::isValidInsertPoint (gp_Pnt &pnt)
{
    if (checkIntersection && shapePoints.size() > 2) {
        long unsigned int i=0;
        while (i < shapePoints.size()-1) {
            if (i != editIndex) {

                long unsigned int indexm1,indexp1;
                bool skipm=false;
                bool skipp=false;
                if (closed) {
                    if (editIndex == 0) {
                        indexm1=shapePoints.size()-2;
                        if (i == shapePoints.size()) {skipm=true;}
                    } else {
                        indexm1=editIndex-1;
                    }

                    if (editIndex == shapePoints.size()-1) {
                        skipm=true;
                        skipp=true;
                    } else {
                        indexp1=editIndex+1;
                    }
                } else {
                    if (editIndex == 0) {
                        skipm=true;
                    } else {
                        indexm1=editIndex-1;
                    }

                    if (editIndex == shapePoints.size()-1) {
                        skipp=true;
                    } else {
                        indexp1=editIndex+1;
                    }
                }

                if (!skipm && DoSegmentsIntersectInterior(shapePoints[i],shapePoints[i+1],
                                                          shapePoints[indexm1],currentMousePosition,1e-12)) {
                    return false;
                }
                if (!skipp && DoSegmentsIntersectInterior(shapePoints[i],shapePoints[i+1],
                                                          shapePoints[indexp1],currentMousePosition,1e-12)) {
                    return false;
                }
            }
            i++;
        }
    }

    return true;
}

// same simple algorithm as Path::reverseOrder
// An in-place reverse could be nice to have at some point.
void Polyline::reverseOrder ()
{
    std::vector<gp_Pnt> reversed_points;

    long unsigned int i=0;
    while (i < shapePoints.size()) {
        reversed_points.push_back(shapePoints[shapePoints.size()-1-i]);
        i++;
    }

    i=0;
    while (i < shapePoints.size()) {
        shapePoints[i]=reversed_points[i];
        i++;
    }
}

Polyline* Polyline::copyCreate ()
{
    Polyline *newPolyline=new Polyline();
    newPolyline->modified=modified;
    newPolyline->closed=closed;
    newPolyline->hasArrows=hasArrows;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        newPolyline->shapePoints.push_back(shapePoints[i]);
        i++;
    }
    newPolyline->normal=normal;
    newPolyline->currentMousePosition=currentMousePosition;
    newPolyline->viewerContext=viewerContext;
    newPolyline->drawEnable=drawEnable;
    newPolyline->editIndex=editIndex;
    newPolyline->checkIntersection=checkIntersection;
    newPolyline->reverseExtrusionDirection=reverseExtrusionDirection;
    return newPolyline;
}

Handle(AIS_Shape) Polyline::get_AIS_Shape ()
{
    Handle(AIS_Shape) shape;
    TopoDS_Wire wire=buildWire();
    if (!wire.IsNull()) {
        if (hasArrows) {
            TopoDS_Compound compound;
            BRep_Builder builder;
            builder.MakeCompound(compound);
            if (closed) {
                TopoDS_Face face=buildFace(wire);
                if (!face.IsNull()) {
                    builder.Add(compound,face);
                }
            } else {
                builder.Add(compound,wire);
            }
            addArrows(builder,compound);
            shape=new AIS_Shape(compound);
        } else {
            if (closed) {
                TopoDS_Face face=buildFace(wire);
                if (!face.IsNull()) {
                    shape=new AIS_Shape(face);
                }
            } else {
                shape=new AIS_Shape(wire);
            }
        }
    }
    return shape;
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
    if (!isValidInsertPoint(currentMousePosition)) return;

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

    if (polyMaker.IsDone()) {
        TopoDS_Wire wire=polyMaker.Wire();
        if (!wire.IsNull()) {
            rubberband=new AIS_Shape(wire);
            if (!rubberband.IsNull()) {
                viewerContext->Display(rubberband,0,-1,Standard_True);
            }
        }
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

bool Polyline::canInsertPoint ()
{
    return true;
}

void Polyline::insertPoint (gp_Pnt &pnt)
{
    long unsigned int closestIndex=0;
    double closest=DBL_MAX;

    // closest point
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        double distance=shapePoints[i].Distance(pnt);
        if (distance < closest) {
            closest=distance;
            closestIndex=i;
        }
        i++;
    }

    // next closest point on either side
    if (closed) {
        if (closestIndex == 0) {
            shapePoints.insert(shapePoints.begin(),pnt);
        } else if (closestIndex == shapePoints.size()-1) {
            shapePoints.insert(shapePoints.begin(),pnt);
        } else {
            shapePoints.insert(shapePoints.begin()+closestIndex,pnt);
        }
    } else {
        if (closestIndex == 0) {
            shapePoints.insert(shapePoints.begin(),pnt);
        } else if (closestIndex == shapePoints.size()-1) {
            shapePoints.insert(shapePoints.begin()+shapePoints.size()-1,pnt);
        } else {
            shapePoints.insert(shapePoints.begin()+closestIndex,pnt);
        }
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

QString Polyline::getName (ObjectCounts *objectCounts) {
    objectCounts->polyline++;
    QString name="Polyline";
    name.append(QString::number(objectCounts->polyline));
    return name;
}

void Polyline::shift (gp_Pnt &pnt1, gp_Pnt &pnt2)
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

void Polyline::save (std::ofstream *out, QString name, int level)
{
    if (shapePoints.size() < 2) return;

    std::string space;
    long unsigned int i=0;
    while (i < level) {
        space.append("   ");
        i++;
    }

    *out << space << "Polyline" << std::endl;
    if (!name.isEmpty()) {
        *out << space << "   name=" << name.toStdString() << std::endl;
    }

    i=0;
    while (i < shapePoints.size()) {
        *out << space << "   point=("
             << shapePoints[i].X() << ","
             << shapePoints[i].Y() << ","
             << shapePoints[i].Z() << ")" << std::endl;
        i++;
    }

    *out << space << "   normal=("
         << normal.X() << ","
         << normal.Y() << ","
         << normal.Z() << ")" << std::endl;

    *out << space << "   reverse=";
    if (reverseExtrusionDirection) *out << "true" << std::endl;
    else *out << "false" << std::endl;

    *out << space << "EndPolyline" << std::endl;
}

bool Polyline::load (std::vector<std::string> &inputData, long unsigned int start,
                    long unsigned int end, std::string& name, ObjectCounts *objectCounts)
{
    bool foundName=false;
    bool foundNormal=false;
    bool foundReverse=false;

    long unsigned int i=start;
    while (i <= end) {

        gp_Pnt pnt;
        std::string keyword;

        // name
        keyword="name";
        std::string testName;
        if (extractText(inputData[i],keyword,testName)) {
            name=testName;
            foundName=true;
            i++;
            continue;
        }

        // point
        keyword="point";
        if (extractPoint(inputData[i],keyword,pnt)) {
            shapePoints.push_back(pnt);
            i++;
            continue;
        }

        // normal
        keyword="normal";
        if (extractPoint(inputData[i],keyword,pnt)) {
            setNormal(pnt);
            foundNormal=true;
            i++;
            continue;
        }

        // reverse
        keyword="reverse";
        std::string testReverse;
        if (extractText(inputData[i],keyword,testReverse)) {
            // expect true/false, otherwise no change from the default
            if (testReverse.compare("true") == 0) reverseExtrusionDirection=true;
            else if (testReverse.compare("false") == 0) reverseExtrusionDirection=false;
            foundReverse=true;
            i++;
            continue;
        }

        i++;
    }

    // check for completeness
    if (shapePoints.size() < 2) return true;
    if (!foundName) return true;
    if (!foundNormal) return true;
    if (!foundReverse) return true;

    if (shapePoints[0].IsEqual(shapePoints[shapePoints.size()-1],Precision::Confusion())) closed=true;

    objectCounts->polyline++;

    return false;
}

void Polyline::print ()
{
    std::cout << "Polyline:" << std::endl;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        std::string name="point";
        printPnt(name,shapePoints[i]);
        i++;
    }

    std::string name="normal";
    printVec(name,normal);
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
    isSquare=rectangle->isSquare;
    tempWidth=rectangle->tempWidth;
    tempHeight=rectangle->tempHeight;

    normal=rectangle->normal;
    modified=rectangle->modified;
    drawEnable=rectangle->drawEnable;
    editIndex=rectangle->editIndex;
    closed=rectangle->closed;
    reverseExtrusionDirection=rectangle->reverseExtrusionDirection;

    long unsigned int i=0;
    while (i < rectangle->shapePoints.size()) {
        shapePoints.push_back(rectangle->shapePoints[i]);
        i++;
    }

    viewerContext=rectangle->viewerContext;
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

    if (isSquare) {
        if (abs(width) > abs(height)) {
            height=abs(width);
            if (tempHeight < 0) height=-height;
        } else {
            width=abs(height);
            if (tempWidth < 0) width=-width;
        }

        shapePoints[2]=shapePoints[0].Translated(u*width).Translated(v*height);
    }

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

    tempWidth=width;
    tempHeight=height;
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

    double tw;
    double th;

    if (editIndex == 0 || editIndex == 4) {
        gp_Vec d(currentMousePosition,tempShapePoints[2]);
        tw=d.Dot(u);
        th=d.Dot(v);

        if (isSquare) {
            if (abs(tw) > abs(th)) {
                th=abs(tw);
                if (tempHeight < 0) th=-th;
            } else {
                tw=abs(th);
                if (tempWidth < 0) tw=-tw;
            }

            currentMousePosition=tempShapePoints[2].Translated(-u*tw).Translated(-v*th);
        }

        if (abs(tw) > Precision::Confusion() && abs(th) > Precision::Confusion()) {
            tempShapePoints[0]=currentMousePosition;
            tempShapePoints[1]=tempShapePoints[2].Translated(-v*th);
            tempShapePoints[3]=tempShapePoints[2].Translated(-u*tw);
            tempShapePoints[4]=tempShapePoints[0];
        }
    } else if (editIndex == 1) {
        gp_Vec d(tempShapePoints[3],currentMousePosition);
        tw=d.Dot(u);
        th=d.Dot(v);

        if (isSquare) {
            if (abs(tw) > abs(th)) {
                th=abs(tw);
                if (tempHeight < 0) th=-th;
            } else {
                tw=abs(th);
                if (tempWidth < 0) tw=-tw;
            }

            currentMousePosition=tempShapePoints[3].Translated(u*tw).Translated(v*th);
        }

        if (abs(tw) > Precision::Confusion() && abs(th) > Precision::Confusion()) {
            tempShapePoints[0]=tempShapePoints[3].Translated(v*th);
            tempShapePoints[1]=currentMousePosition;
            tempShapePoints[2]=tempShapePoints[3].Translated(u*tw);
            tempShapePoints[4]=tempShapePoints[0];
        }
    } else if (editIndex == 2) {
        gp_Vec d(tempShapePoints[0],currentMousePosition);
        tw=d.Dot(u);
        th=d.Dot(v);

        if (isSquare) {
            if (abs(tw) > abs(th)) {
                th=abs(tw);
                if (tempHeight < 0) th=-th;
            } else {
                tw=abs(th);
                if (tempWidth < 0) tw=-tw;
            }

            currentMousePosition=tempShapePoints[0].Translated(u*tw).Translated(v*th);
        }

        if (abs(tw) > Precision::Confusion() && abs(th) > Precision::Confusion()) {
            tempShapePoints[1]=tempShapePoints[0].Translated(u*tw);
            tempShapePoints[2]=currentMousePosition;
            tempShapePoints[3]=tempShapePoints[0].Translated(v*th);
            tempShapePoints[4]=tempShapePoints[0];
        }
    } else if (editIndex == 3) {
        gp_Vec d(currentMousePosition,tempShapePoints[1]);
        tw=d.Dot(u);
        th=d.Dot(v);

        if (isSquare) {
            if (abs(tw) > abs(th)) {
                th=abs(tw);
                if (tempHeight < 0) th=-th;
            } else {
                tw=abs(th);
                if (tempWidth < 0) tw=-tw;
            }

            currentMousePosition=tempShapePoints[1].Translated(-u*tw).Translated(-v*th);
        }

        if (abs(tw) > Precision::Confusion() && abs(th) > Precision::Confusion()) {
            tempShapePoints[0]=tempShapePoints[1].Translated(-u*tw);
            tempShapePoints[2]=tempShapePoints[1].Translated(-v*th);
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

    tempWidth=tw;
    tempHeight=th;
}

TopoDS_Face Rectangle::buildFace (TopoDS_Wire &wire)
{
    //std::cout << "Rectangle::buildFace" << std::endl; std::cout.flush();

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
    //std::cout << "Rectangle::setEditPoint  tempWidth=" << tempWidth << "  tempHeight=" << tempHeight << std::endl; std::cout.flush();

    if (editIndex == 0 || editIndex == 4) {
        gp_Vec d(pnt,shapePoints[2]);
        width=d.Dot(u);
        height=d.Dot(v);

        if (isSquare) {
            std::cout << "   width=" << width << "  height=" << height << std::endl; std::cout.flush();
            if (abs(width) > abs(height)) {
                height=abs(width);
                if (tempHeight < 0) height=-height;
            } else {
                width=abs(height);
                if (tempWidth < 0) width=-width;
            }

            pnt=shapePoints[2].Translated(-u*width).Translated(-v*height);
        }

        shapePoints[0]=pnt;
        shapePoints[1]=shapePoints[2].Translated(-v*height);
        shapePoints[3]=shapePoints[2].Translated(-u*width);
        shapePoints[4]=shapePoints[0];
    } else if (editIndex == 1) {
        gp_Vec d(shapePoints[3],pnt);
        width=d.Dot(u);
        height=-d.Dot(v);

        if (isSquare) {
            if (abs(width) > abs(height)) {
                height=abs(width);
                if (tempHeight < 0) height=-height;
            } else {
                width=abs(height);
                if (tempWidth < 0) width=-width;
            }

            pnt=shapePoints[3].Translated(u*width).Translated(-v*height);
        }

        shapePoints[0]=shapePoints[3].Translated(-v*height);
        shapePoints[1]=pnt;
        shapePoints[2]=shapePoints[3].Translated(u*width);
        shapePoints[4]=shapePoints[0];
    } else if (editIndex == 2) {
        gp_Vec d(shapePoints[0],pnt);
        width=d.Dot(u);
        height=d.Dot(v);

        if (isSquare) {
            if (abs(width) > abs(height)) {
                height=abs(width);
                if (tempHeight < 0) height=-height;
            } else {
                width=abs(height);
                if (tempWidth < 0) width=-width;
            }

            pnt=shapePoints[0].Translated(u*width).Translated(v*height);
        }

        shapePoints[1]=shapePoints[0].Translated(u*width);
        shapePoints[2]=pnt;
        shapePoints[3]=shapePoints[0].Translated(v*height);
        shapePoints[4]=shapePoints[0];
    } else if (editIndex == 3) {
        gp_Vec d(pnt,shapePoints[1]);
        width=d.Dot(u);
        height=-d.Dot(v);

        if (isSquare) {
            if (abs(width) > abs(height)) {
                height=abs(width);
                if (tempHeight < 0) height=-height;
            } else {
                width=abs(height);
                if (tempWidth < 0) width=-width;
            }

            pnt=shapePoints[1].Translated(-u*width).Translated(v*height);
        }

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

// for change in width and/or height
void Rectangle::recalculate (gp_Pnt p0, double width_, double height_)
{
    shapePoints[0]=p0;

    width=width_;
    height=height_;

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

void Rectangle::shift (gp_Pnt &pnt1, gp_Pnt &pnt2)
{
    if (shapePoints.size() == 0) return;
    modified=true;

    gp_Pnt offset;
    offset=pnt2.XYZ()-pnt1.XYZ();

    shapePoints[0]=shapePoints[0].XYZ()-offset.XYZ();
    recalculate();
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

Rectangle* Rectangle::copyCreate ()
{
    //std::cout << "Rectangle::copyCreate" << std::endl; std::cout.flush();
    Rectangle *newRectangle=new Rectangle();
    newRectangle->modified=modified;
    newRectangle->closed=closed;
    newRectangle->hasArrows=hasArrows;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        newRectangle->shapePoints.push_back(shapePoints[i]);
        i++;
    }
    newRectangle->normal=normal;
    newRectangle->currentMousePosition=currentMousePosition;
    newRectangle->viewerContext=viewerContext;
    newRectangle->drawEnable=drawEnable;
    newRectangle->editIndex=editIndex;
    newRectangle->u=u;
    newRectangle->v=v;
    newRectangle->width=width;
    newRectangle->height=height;
    newRectangle->isSquare=isSquare;
    newRectangle->tempWidth=tempWidth;
    newRectangle->tempHeight=tempHeight;
    newRectangle->reverseExtrusionDirection=reverseExtrusionDirection;
    return newRectangle;

}

Handle(AIS_Shape) Rectangle::get_AIS_Shape ()
{
    Handle(AIS_Shape) shape;
    TopoDS_Wire wire=buildWire();
    if (!wire.IsNull()) {
        if (hasArrows) {
            TopoDS_Compound compound;
            BRep_Builder builder;
            builder.MakeCompound(compound);
            if (closed) {
                TopoDS_Face face=buildFace(wire);
                if (!face.IsNull()) {
                    builder.Add(compound,face);
                }
            } else {
                builder.Add(compound,wire);
            }
            addArrows(builder,compound);
            shape=new AIS_Shape(compound);
        } else {
            if (closed) {
                TopoDS_Face face=buildFace(wire);
                if (!face.IsNull()) {
                    shape=new AIS_Shape(face);
                }
            } else {
                shape=new AIS_Shape(wire);
            }
        }
    }
    return shape;
}

Polyline* Rectangle::convert ()
{
    Polyline* polyline=new Polyline();
    polyline->modified=true;
    polyline->closed=true;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        polyline->shapePoints.push_back(shapePoints[i]);
        i++;
    }
    polyline->normal=normal;
    polyline->viewerContext=viewerContext;
    return polyline;
}

void Rectangle::reverseOrder ()
{
    if (shapePoints.size() > 3) {
        shapePoints[0]=shapePoints[1];
    }
    u=-u;
    //v=-v;
    recalculate();
}

QString Rectangle::getName (ObjectCounts *objectCounts) {
    objectCounts->rectangle++;
    QString name="Rectangle";
    name.append(QString::number(objectCounts->rectangle));
    return name;
}

void Rectangle::save (std::ofstream *out, QString name, int level)
{
    if (shapePoints.size() == 0) return;

    std::string space;
    long unsigned int i=0;
    while (i < level) {
        space.append("   ");
        i++;
    }

    *out << space << "Rectangle" << std::endl;
    if (!name.isEmpty()) {
        *out << space << "   name=" << name.toStdString() << std::endl;
    }

    *out << space << "   origin=("
         << shapePoints[0].X() << ","
         << shapePoints[0].Y() << ","
         << shapePoints[0].Z() << ")" << std::endl;

    *out << space << "   width=" << width << std::endl;
    *out << space << "   height=" << height << std::endl;
    *out << space << "   isSquare=" << isSquare << std::endl;
    *out << space << "   tempWidth=" << tempWidth << std::endl;
    *out << space << "   tempHeight=" << tempHeight << std::endl;

    *out << space << "   u=("
         << u.X() << ","
         << u.Y() << ","
         << u.Z() << ")" << std::endl;

    *out << space << "   v=("
         << v.X() << ","
         << v.Y() << ","
         << v.Z() << ")" << std::endl;

    *out << space << "   normal=("
         << normal.X() << ","
         << normal.Y() << ","
         << normal.Z() << ")" << std::endl;

    *out << space << "   reverse=";
    if (reverseExtrusionDirection) *out << "true" << std::endl;
    else *out << "false" << std::endl;

    *out << space << "EndRectangle" << std::endl;
}

bool Rectangle::load (std::vector<std::string> &inputData, long unsigned int start,
                     long unsigned int end, std::string& name, ObjectCounts *objectCounts)
{
    bool foundName=false;
    bool foundOrigin=false;
    bool foundWidth=false;
    bool foundHeight=false;
    bool foundu=false;
    bool foundv=false;
    bool foundNormal=false;
    bool foundReverse=false;

    long unsigned int i=start;
    while (i <= end) {

        gp_Pnt pnt;
        std::string keyword;

        // name
        keyword="name";
        std::string testName;
        if (extractText(inputData[i],keyword,testName)) {
            name=testName;
            foundName=true;
            i++;
            continue;
        }

        // origin
        keyword="origin";
        if (extractPoint(inputData[i],keyword,pnt)) {
            shapePoints.push_back(pnt);
            foundOrigin=true;
            i++;
            continue;
        }

        // width
        keyword="width";
        std::string testWidth;
        if (extractText(inputData[i],keyword,testWidth)) {
            width=std::stod(testWidth);
            foundWidth=true;
            i++;
            continue;
        }

        // height
        keyword="height";
        std::string testHeight;
        if (extractText(inputData[i],keyword,testHeight)) {
            height=std::stod(testHeight);
            foundHeight=true;
            i++;
            continue;
        }

        // u
        keyword="u";
        if (extractPoint(inputData[i],keyword,pnt)) {
            u.SetCoord(pnt.X(),pnt.Y(),pnt.Z());
            foundu=true;
            i++;
            continue;
        }

        // v
        keyword="v";
        if (extractPoint(inputData[i],keyword,pnt)) {
            v.SetCoord(pnt.X(),pnt.Y(),pnt.Z());
            foundv=true;
            i++;
            continue;
        }

        // normal
        keyword="normal";
        if (extractPoint(inputData[i],keyword,pnt)) {
            setNormal(pnt);
            foundNormal=true;
            i++;
            continue;
        }

        // reverse
        keyword="reverse";
        std::string testReverse;
        if (extractText(inputData[i],keyword,testReverse)) {
            // expect true/false, otherwise no change from the default
            if (testReverse.compare("true") == 0) reverseExtrusionDirection=true;
            else if (testReverse.compare("false") == 0) reverseExtrusionDirection=false;
            foundReverse=true;
            i++;
            continue;
        }

        i++;
    }

    // check for completeness
    if (!foundOrigin) return true;
    if (!foundName) return true;
    if (!foundWidth) return true;
    if (!foundHeight) return true;
    if (!foundu) return true;
    if (!foundv) return true;
    if (!foundNormal) return true;
    if (!foundReverse) return true;

    // create space for the other points
    i=0;
    while (i < 4) {
        shapePoints.push_back(shapePoints[0]);
        i++;
    }

    isSquare=false;
    tempWidth=0;
    tempHeight=0;
    recalculate();

    objectCounts->rectangle++;

    return false;
}

void Rectangle::print ()
{
    std::cout << "Rectangle:" << std::endl;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        std::string name="point";
        printPnt(name,shapePoints[i]);
        i++;
    }

    std::string name="normal";
    printVec(name,normal);
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

    if (currentMousePosition.IsEqual(centerPoint,Precision::Confusion())) return;

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

void Polycircle::reverseOrder ()
{
    normal=-normal;
    recalculate();
}

Polycircle* Polycircle::copyCreate ()
{
    Polycircle *newPolycircle=new Polycircle();
    newPolycircle->modified=modified;
    newPolycircle->closed=closed;
    newPolycircle->hasArrows=hasArrows;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        newPolycircle->shapePoints.push_back(shapePoints[i]);
        i++;
    }
    newPolycircle->normal=normal;
    newPolycircle->currentMousePosition=currentMousePosition;
    newPolycircle->viewerContext=viewerContext;
    newPolycircle->drawEnable=drawEnable;
    newPolycircle->editIndex=editIndex;
    newPolycircle->centerPointSet=centerPointSet;
    newPolycircle->centerPoint=centerPoint;
    newPolycircle->firstPointSet=firstPointSet;
    newPolycircle->firstPoint=firstPoint;
    newPolycircle->vertexCount=vertexCount;
    newPolycircle->reverseExtrusionDirection=reverseExtrusionDirection;
    return newPolycircle;
}

Handle(AIS_Shape) Polycircle::get_AIS_Shape ()
{
    Handle(AIS_Shape) shape;
    TopoDS_Wire wire=buildWire();
    if (!wire.IsNull()) {
        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);

        if (closed) {
            TopoDS_Face face=buildFace(wire);
            if (!face.IsNull()) {
                builder.Add(compound,face);
            }
        } else {
            builder.Add(compound,wire);
        }

        if (hasArrows) {
            addArrows(builder,compound);
        }

        // center point for selection
        TopoDS_Vertex vertex=BRepBuilderAPI_MakeVertex(centerPoint);
        builder.Add(compound,vertex);

        shape=new AIS_Shape(compound);
    }
    return shape;
}


Polyline* Polycircle::convert ()
{
    Polyline* polyline=new Polyline();
    polyline->modified=true;
    polyline->closed=true;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        polyline->shapePoints.push_back(shapePoints[i]);
        i++;
    }
    polyline->normal=normal;
    polyline->viewerContext=viewerContext;
    return polyline;
}

QString Polycircle::getName (ObjectCounts *objectCounts) {
    objectCounts->polycircle++;
    QString name="Polycircle";
    name.append(QString::number(objectCounts->polycircle));
    return name;
}

void Polycircle::save (std::ofstream *out, QString name, int level)
{
    if (shapePoints.size() < 2) return;

    std::string space;
    long unsigned int i=0;
    while (i < level) {
        space.append("   ");
        i++;
    }

    *out << space << "Polycircle" << std::endl;
    if (!name.isEmpty()) {
        *out << space << "   name=" << name.toStdString() << std::endl;
    }
    *out << space << "   N=" << vertexCount << std::endl;

    *out << space << "   center=("
         << centerPoint.X() << ","
         << centerPoint.Y() << ","
         << centerPoint.Z() << ")" << std::endl;

    *out << space << "   point=("
         << firstPoint.X() << ","
         << firstPoint.Y() << ","
         << firstPoint.Z() << ")" << std::endl;

    *out << space << "   normal=("
         << normal.X() << ","
         << normal.Y() << ","
         << normal.Z() << ")" << std::endl;

    *out << space << "   reverse=";
    if (reverseExtrusionDirection) *out << "true" << std::endl;
    else *out << "false" << std::endl;

    *out << space << "EndPolycircle" << std::endl;
}

bool Polycircle::load (std::vector<std::string> &inputData, long unsigned int start,
                      long unsigned int end, std::string& name, ObjectCounts *objectCounts)
{
    bool foundName=false;
    bool foundCenter=false;
    bool foundPoint=false;
    bool foundCount=false;
    bool foundNormal=false;
    bool foundReverse=false;

    long unsigned int i=start;
    while (i <= end) {

        gp_Pnt pnt;
        std::string keyword;

        // name
        keyword="name";
        std::string testName;
        if (extractText(inputData[i],keyword,testName)) {
            name=testName;
            foundName=true;
            i++;
            continue;
        }

        // count
        keyword="N";
        std::string testCount;
        if (extractText(inputData[i],keyword,testCount)) {
            vertexCount=stoi(testCount);
            foundCount=true;
            i++;
            continue;
        }

        // center
        keyword="center";
        if (extractPoint(inputData[i],keyword,pnt)) {
            centerPoint=pnt;
            centerPointSet=true;
            foundCenter=true;
            i++;
            continue;
        }

        // firstPoint
        keyword="point";
        if (extractPoint(inputData[i],keyword,pnt)) {
            firstPoint=pnt;
            firstPointSet=true;
            foundPoint=true;
            i++;
            continue;
        }

        // normal
        keyword="normal";
        if (extractPoint(inputData[i],keyword,pnt)) {
            setNormal(pnt);
            foundNormal=true;
            i++;
            continue;
        }

        // reverse
        keyword="reverse";
        std::string testReverse;
        if (extractText(inputData[i],keyword,testReverse)) {
            // expect true/false, otherwise no change from the default
            if (testReverse.compare("true") == 0) reverseExtrusionDirection=true;
            else if (testReverse.compare("false") == 0) reverseExtrusionDirection=false;
            foundReverse=true;
            i++;
            continue;
        }

        i++;
    }

    // check for completeness
    if (!foundName) return true;
    if (!foundCount) return true;
    if (!foundCenter) return true;
    if (!foundPoint) return true;
    if (!foundNormal) return true;
    if (!foundReverse) return true;

    recalculate();

    objectCounts->polycircle++;

    return false;
}

void Polycircle::print ()
{
    std::cout << "Polycircle:" << std::endl;
    long unsigned int i=0;
    while (i < shapePoints.size()) {
        std::string name="point";
        printPnt(name,shapePoints[i]);
        i++;
    }

    std::string name="normal";
    printVec(name,normal);
}
