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
#include "ObjectCounts.h"

class OpenParEMg;

class Polyline;

class Polywire : public QObject
{
    Q_OBJECT

public:
    explicit Polywire(QObject *parent = nullptr);
    virtual ~Polywire() {}

    virtual void drawRubberband () = 0;         // while drawing - currentMousePosition is the next point
    virtual void drawStretchRubberband () = 0;  // while stretching - currentMousePosition takes the editIndex place
    void deleteRubberband ();
    virtual bool isValidPoint (gp_Pnt &pnt, bool);
    virtual bool isValidInsertPoint (gp_Pnt &pnt) {return true;}
    virtual bool canDeleteLastPoint () = 0;
    virtual bool canFinish () = 0;
    virtual bool canClose () = 0;
    virtual void close () = 0;
    virtual bool canOpen () = 0;
    virtual void open () = 0;
    virtual bool canConvert () = 0;
    virtual Polyline* convert () = 0;
    virtual bool canEdit () = 0;
    virtual void addPoint (gp_Pnt &pnt);
    void setCurrentMousePosition (gp_Pnt &currentMousePosition_) {currentMousePosition=currentMousePosition_;}
    void deleteLastPoint ();
    virtual bool isFinished () = 0;
    virtual bool canDeletePoint () = 0;
    virtual void deletePoint (gp_Pnt &pnt) = 0;
    virtual bool canInsertPoint () = 0;
    virtual void insertPoint (gp_Pnt &pnt) = 0;

    virtual bool isPointOnPlane (gp_Pnt &pnt);

    void setNormal (gp_Vec normal_) {normal=normal_;}
    void setNormal (struct point normal_) {normal.SetCoord(normal_.x,normal_.y,normal_.z);}
    void setNormal (double x, double y, double z) {normal.SetCoord(x,y,z);}
    void setNormal (gp_Pnt pnt) {normal.SetCoord(pnt.X(),pnt.Y(),pnt.Z());}
    gp_Vec getNormal () {return normal;}

    virtual void setU (gp_Vec u_) {return;}

    gp_Pnt getPosition ();
    virtual gp_Pln getPlane ();

    void set_viewerContext (Handle(AIS_InteractiveContext) viewerContext_) {viewerContext=viewerContext_;}

    void setEditIndex (gp_Pnt &pnt);
    virtual void setEditPoint (gp_Pnt &pnt);

    TopoDS_Wire buildWire();
    virtual TopoDS_Face buildFace (TopoDS_Wire &wire) {TopoDS_Face face; return face;}
    virtual void shift (gp_Pnt &pnt1, gp_Pnt &pnt2) = 0;
    virtual void rotate (double &angleDegrees, gp_Pnt &p1, gp_Pnt &p2);

    bool isModified () {return modified;}
    void setModified (bool modified_) {modified=modified_;}

    bool isClosed () {return closed;}

    void setDrawEnable (bool drawEnable_) {drawEnable=drawEnable_;}
    bool getDrawEnable () {return drawEnable;}

    virtual Polywire* copyCreate () = 0;
    virtual Handle(AIS_Shape) get_AIS_Shape () = 0;
    virtual QString getName (ObjectCounts *objectCounts) = 0;

    void setReverseExtrusionDirection (bool reverseExtrusionDirection_) {reverseExtrusionDirection=reverseExtrusionDirection_;}
    bool getReverseExtrusionDirection () {return reverseExtrusionDirection;}

    virtual void save (std::ofstream *, QString, int) = 0;
    virtual bool load (std::vector<std::string> &inputData, long unsigned int,
                      long unsigned int, std::string& name, ObjectCounts *objectCounts) = 0;

    virtual void print () = 0;

signals:

protected:
    bool modified;
    bool closed;
    std::vector<gp_Pnt> shapePoints;                // shape outline
    gp_Vec normal;                                  // normal to the shape
    gp_Pnt currentMousePosition;                    // current mouse position while drawing
    Handle(AIS_Shape) rubberband;                   // rubberband for drawing - currentMousePosition is the next point
    Handle(AIS_InteractiveContext) viewerContext;   // drawing context

    bool drawEnable;                                // flag for enabling drawing objects
    long unsigned int editIndex;                    // index into a completed shape outline for stretching
    bool reverseExtrusionDirection;                 // to line up with the direction selected using the LengthInputForm
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
    void close () override {return;}
    bool canOpen () override {return false;}
    void open () override {return;}
    bool canConvert () override {return false;}
    Polyline* convert () override {return nullptr;}
    bool canEdit () override {return true;}
    bool isFinished () override {if (shapePoints.size() == 2) return true; return false;}
    bool canDeletePoint () override {return false;}
    void deletePoint (gp_Pnt &gp_Pnt) override {return;}
    bool canInsertPoint () override {return false;}
    void insertPoint (gp_Pnt &pnt) override {return;}
    gp_Pnt getP0 ();
    gp_Pnt getP1 ();
    void setP0 (gp_Pnt &P0);
    void setP1 (gp_Pnt &P1);
    Line* copyCreate () override;
    Handle(AIS_Shape) get_AIS_Shape () override;
    void shift (gp_Pnt &pnt1, gp_Pnt &pnt2) override;
    QString getName (ObjectCounts *objectCounts) override;
    void save (std::ofstream *, QString, int) override;
    bool load (std::vector<std::string> &inputData, long unsigned int,
              long unsigned int, std::string& name, ObjectCounts *objectCounts) override;
    void print () override;
};

class Polyline : public Polywire
{
public:
    Polyline ()
    {
        checkIntersection=true;
    }
    void drawRubberband () override;
    void drawStretchRubberband () override;
    bool isValidInsertPoint (gp_Pnt &pnt) override;
    bool canDeleteLastPoint () override {if (shapePoints.size() > 1) return true; return false;}
    bool canFinish () override {if (shapePoints.size() > 1) return true; return false;}
    bool canClose () override;
    void close () override;
    bool canOpen () override;
    void open () override;
    bool canConvert () override {return false;}
    bool canEdit () override {return false;}
    Polyline* convert () override {return nullptr;}
    bool isFinished () override {
        if (shapePoints.size() > 1) {
            if (shapePoints[shapePoints.size()-1].IsEqual(shapePoints[0],Precision::Confusion())) {
                return true;
            }
        }
        return false;
    }
    void buildFromFace (TopoDS_Face &face);
    TopoDS_Face buildFace (TopoDS_Wire &wire) override;
    void setEditPoint (gp_Pnt &pnt) override;
    bool canDeletePoint () override;
    void deletePoint (gp_Pnt &pnt) override;
    bool canInsertPoint () override;
    void insertPoint (gp_Pnt &pnt) override;
    Polyline* copyCreate () override;
    Handle(AIS_Shape) get_AIS_Shape () override;
    void shift (gp_Pnt &pnt1, gp_Pnt &pnt2) override;
    QString getName (ObjectCounts *objectCounts) override;
    void save (std::ofstream *, QString, int) override;
    bool load (std::vector<std::string> &inputData, long unsigned int,
              long unsigned int, std::string& name, ObjectCounts *objectCounts) override;
    void print () override;
private:
    bool checkIntersection;

    friend class Rectangle;
    friend class Polycircle;
};

class Rectangle : public Polywire
{
public:
    Rectangle ()
    {
        closed=true;
        isSquare=false;
    }

    Rectangle (Rectangle *);
    void drawRubberband () override;
    void drawStretchRubberband () override;
    bool isValidPoint (gp_Pnt &pnt, bool) override;
    bool canDeleteLastPoint () override {return false;}
    bool canFinish () override {return false;}
    bool canClose () override {return false;}
    void close () override {return;}
    bool canOpen () override {return false;}
    void open () override {return;}
    bool canConvert () override {return true;}
    Polyline* convert () override;
    bool canEdit () override {return true;}
    void addPoint (gp_Pnt &pnt) override;
    bool isFinished () override {if (shapePoints.size() == 5) return true; return false;}
    bool canDeletePoint () override {return false;}
    void deletePoint (gp_Pnt &gp_Pnt) override {return;}
    bool canInsertPoint () override {return false;}
    void insertPoint (gp_Pnt &pnt) override {return;}

    double getWidth () {return width;}
    double getHeight () {return height;}

    void setU (gp_Vec u_) override {u=u_;}
    void setWidth (double width_) {width=width_;}
    void setHeight (double height_) {height=height_;}

    void setIsSquare (bool isSquare_) {isSquare=isSquare_;}

    void recalculate ();
    void recalculate (gp_Pnt);
    void recalculate (gp_Pnt, gp_Pnt);
    void recalculate (double, double);

    gp_Pnt getOppositeCorner ();

    TopoDS_Face buildFace (TopoDS_Wire &wire) override;
    void setEditPoint (gp_Pnt &pnt) override;

    void shift (gp_Pnt &pnt1, gp_Pnt &pnt2) override;
    void rotate (double &angleDegrees, gp_Pnt &p1, gp_Pnt &p2) override;
    Rectangle* copyCreate () override;
    Handle(AIS_Shape) get_AIS_Shape () override;
    QString getName (ObjectCounts *objectCounts) override;
    void save (std::ofstream *, QString, int) override;
    bool load (std::vector<std::string> &inputData, long unsigned int,
              long unsigned int, std::string& name, ObjectCounts *objectCounts) override;
    void print () override;
private:
    gp_Vec u,v;
    double width,height;
    bool isSquare;
    double tempWidth,tempHeight;  // for stretching squares
};

class Polycircle : public Polywire
{
public:
    Polycircle ()
    {
        centerPointSet=false;
        firstPointSet=false;
        vertexCount=12;
        closed=true;
    }
    Polycircle (Polycircle *);

    void drawRubberband () override;
    void drawStretchRubberband () override;
    bool isValidPoint (gp_Pnt &pnt, bool) override;
    bool canDeleteLastPoint () override {return false;}
    bool canFinish () override {return false;}
    bool canClose () override {return false;}
    void close () override {return;}
    bool canOpen () override {return false;}
    void open () override {return;}
    bool canConvert () override {return true;}
    Polyline* convert () override;
    void addPoint (gp_Pnt &pnt) override;
    bool canEdit () override {return true;}
    bool isFinished () override {if (firstPointSet) return true; return false;}
    bool canDeletePoint () override {return false;}
    void deletePoint (gp_Pnt &gp_Pnt) override {return;}
    bool canInsertPoint () override {return false;}
    void insertPoint (gp_Pnt &pnt) override {return;}

    bool isPointOnPlane (gp_Pnt &pnt) override;

    gp_Pnt getCenterPoint () {return centerPoint;}
    gp_Pnt getFirstPoint () {return firstPoint;}

    void setCenterPoint (gp_Pnt centerPoint_) {centerPoint=centerPoint_;}
    void setFirstPoint (gp_Pnt firstPoint_) {firstPoint=firstPoint_;}

    int getVertexCount () {return vertexCount;}
    void setVertexCount (int vertexCount_) {vertexCount=vertexCount_;}

    void recalculate ();

    TopoDS_Face buildFace (TopoDS_Wire &wire) override;
    void setEditPoint (gp_Pnt &pnt) override;

    void shift (gp_Pnt &pnt1, gp_Pnt &pnt2) override;

    gp_Pln getPlane () override;

    void rotate (double &angleDegrees, gp_Pnt &p1, gp_Pnt &p2) override;
    Polycircle* copyCreate () override;
    Handle(AIS_Shape) get_AIS_Shape () override;
    QString getName (ObjectCounts *objectCounts) override;
    void save (std::ofstream *, QString, int) override;
    bool load (std::vector<std::string> &inputData, long unsigned int,
              long unsigned int, std::string& name, ObjectCounts *objectCounts) override;
    void print () override;
private:
    bool centerPointSet;
    gp_Pnt centerPoint;
    bool firstPointSet;
    gp_Pnt firstPoint;
    int vertexCount;
};



#endif // POLYWIRE_H
