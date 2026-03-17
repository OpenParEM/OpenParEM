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

#ifndef POLYWIRE_H
#define POLYWIRE_H

#include <AIS_Shape.hxx>
#include <QObject>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Pnt.hxx>
#include "keywordPair.hpp"

class OpenParEMg;

class Polywire : public QObject
{
    Q_OBJECT

public:
    explicit Polywire(QObject *parent = nullptr);
    virtual ~Polywire() {}

    virtual void drawRubberband () = 0;         // while drawing - currentMousePosition is the next point
    virtual void drawStretchRubberband () {}  // while stretching - currentMousePosition takes the editIndex place
    void deleteRubberband ();
    virtual bool isValidPoint (gp_Pnt &pnt);
    virtual bool canDeleteLastPoint () = 0;
    virtual bool canFinish () = 0;
    virtual bool canClose () = 0;
    virtual void addPoint (gp_Pnt &pnt);
    void setCurrentMousePosition (gp_Pnt &currentMousePosition_) {currentMousePosition=currentMousePosition_;}
    void deleteLastPoint ();
    void close ();
    virtual bool isFinished () = 0;

    void setNormal (gp_Vec normal_) {normal=normal_;}
    void setNormal (struct point normal_) {normal.SetCoord(normal_.x,normal_.y,normal_.z);}
    void setNormal (double x, double y, double z) {normal.SetCoord(x,y,z);}
    gp_Vec getNormal () {return normal;}

    gp_Pnt getPosition ();

    void set_viewerContext (Handle(AIS_InteractiveContext) viewerContext_) {viewerContext=viewerContext_;}

    void setEditIndex (gp_Pnt &pnt);
    virtual void setEditPoint (gp_Pnt &pnt);

    TopoDS_Wire buildWire();
    void moveTo (gp_Pnt &pnt);
    void shift (gp_Pnt &pnt1, gp_Pnt &pnt2);
    void rotate (double &angleDegrees, gp_Pnt &p1, gp_Pnt &p2);

    bool isModified () {return modified;}
    void setModified (bool modified_) {modified=modified_;}

signals:

protected:
    bool modified;
    std::vector<gp_Pnt> shapePoints;                // shape outline
    gp_Vec normal;                                  // normal to the shape
    gp_Pnt currentMousePosition;                    // current mouse position while drawing
    Handle(AIS_Shape) rubberband;                   // rubberband for drawing - currentMousePosition is the next point
    Handle(AIS_InteractiveContext) viewerContext;   // drawing context

    long unsigned int editIndex;                    // index into a completed shape outline for stretching
};

class Line : public Polywire
{
public:
    Line () {}
    void drawRubberband () override;
    void drawStretchRubberband () override;
    bool canDeleteLastPoint () override {return false;}
    bool canFinish () override {return false;}
    bool canClose () override {return false;}
    bool isFinished () override {if (shapePoints.size() == 2) return true; return false;}
};

class Polyline : public Polywire
{
public:
    Polyline () {}
    void drawRubberband () override;
    void drawStretchRubberband () override;
    bool canDeleteLastPoint () override {if (shapePoints.size() > 1) return true; return false;}
    bool canFinish () override {if (shapePoints.size() > 1) return true; return false;}
    bool canClose () override {if (shapePoints.size() > 2) return true; return false;}
    bool isFinished () override {
        if (shapePoints.size() > 1) {
            if (shapePoints[shapePoints.size()-1].IsEqual(shapePoints[0],Precision::Confusion())) {
                return true;
            }
        }
        return false;
    }
    void buildFromFace (TopoDS_Face &face);
    void setEditPoint (gp_Pnt &pnt) override;

};

class Rectangle : public Polywire
{
public:
    Rectangle () {}
    Rectangle (Rectangle *);
    void drawRubberband () override;
    void drawStretchRubberband () override;
    bool isValidPoint (gp_Pnt &pnt) override;
    bool canDeleteLastPoint () override {return false;}
    bool canFinish () override {return false;}
    bool canClose () override {return false;}
    void addPoint (gp_Pnt &pnt) override;
    bool isFinished () override {if (shapePoints.size() == 5) return true; return false;}

    double getWidth () {return width;}
    double getHeight () {return height;}

    void setU (gp_Vec u_) {
        u=u_;
        std::cout << "Rectangle::setU  u=(" << u.X() << "," << u.Y() << "," << u.Z() << ")" << std::endl; std::cout.flush();
    }
    void setWidth (double width_) {width=width_;}
    void setHeight (double height_) {height=height_;}

    void recalculate ();
    void recalculate (gp_Pnt, gp_Pnt);

    gp_Pnt getOppositeCorner ();

    void setEditPoint (gp_Pnt &pnt) override;

private:
    gp_Vec u,v;
    double width,height;
};

class Polycircle : public Polywire
{
public:
    Polycircle ()
    {
        centerPointSet=false;
        firstPointSet=false;
        vertexCount=12;
    }

    void drawRubberband () override;
    void drawStretchRubberband () override;
    bool isValidPoint (gp_Pnt &pnt) override;
    bool canDeleteLastPoint () override {return false;}
    bool canFinish () override {return false;}
    bool canClose () override {return false;}
    void addPoint (gp_Pnt &pnt) override;
    bool isFinished () override {if (firstPointSet) return true; return false;}

    gp_Pnt getCenterPoint () {return centerPoint;}
    gp_Pnt getFirstPoint () {return firstPoint;}

    void setCenterPoint (gp_Pnt centerPoint_) {centerPoint=centerPoint_;}
    void setFirstPoint (gp_Pnt firstPoint_) {firstPoint=firstPoint_;}

    int getVertexCount () {return vertexCount;}
    void setVertexCount (int vertexCount_) {vertexCount=vertexCount_;}

    void recalculate ();

    void setEditPoint (gp_Pnt &pnt) override;

private:
    bool centerPointSet;
    gp_Pnt centerPoint;
    bool firstPointSet;
    gp_Pnt firstPoint;
    int vertexCount;
};



#endif // POLYWIRE_H
