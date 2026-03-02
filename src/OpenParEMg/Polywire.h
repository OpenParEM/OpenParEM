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

    virtual void drawRubberband () = 0;
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

    TopoDS_Wire buildWire();
    void moveTo (gp_Pnt &pnt);
    void shift (gp_Pnt &pnt1, gp_Pnt &pnt2);

    bool isModified () {return modified;}
    void setModified (bool modified_) {modified=modified_;}

signals:

protected:
    bool modified;
    std::vector<gp_Pnt> shapePoints;
    gp_Vec normal;
    gp_Pnt currentMousePosition;
    Handle(AIS_Shape) rubberband;
    Handle(AIS_InteractiveContext) viewerContext;
};

class Line : public Polywire
{
public:
    Line () {}
    void drawRubberband () override;
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

};

class Rectangle : public Polywire
{
public:
    Rectangle () {}
    void drawRubberband () override;
    bool isValidPoint (gp_Pnt &pnt) override;
    bool canDeleteLastPoint () override {return false;}
    bool canFinish () override {return false;}
    bool canClose () override {return false;}
    void addPoint (gp_Pnt &pnt) override;
    bool isFinished () override {if (shapePoints.size() == 5) return true; return false;}

    double getWidth () {return width;}
    double getHeight () {return height;}

    void setWidth (double width_) {width=width_;}
    void setHeight (double height_) {height=height_;}

    void recalculate ();

private:
    gp_Vec u,v;
    double width,height;
};



#endif // POLYWIRE_H
