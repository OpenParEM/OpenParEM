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
#include <TopoDS_Wire.hxx>
#include <gp_Pnt.hxx>
#include "keywordPair.hpp"



class Polywire : public QObject
{
    Q_OBJECT

public:
    explicit Polywire(QObject *parent = nullptr);
    void drawRubberband ();
    void deleteRubberband ();
    bool isValidPoint (gp_Pnt &pnt);
    bool canDeleteLastPoint ();
    bool canFinish ();
    bool canClose ();
    void addPoint (gp_Pnt &pnt);
    void setCurrentMousePosition (gp_Pnt &currentMousePosition_) {currentMousePosition=currentMousePosition_;}
    void deleteLastPoint ();
    void close ();
    bool isFinished ();

    void set_line () {type=0;}
    void set_polyline () {type=1;}

    bool is_line () {if (type == 0) return true; return false;}
    bool is_polyline () {if (type == 1) return true; return false;}

    void setNormal (struct point normal_) {normal=normal_;}
    struct point getNormal () {return normal;}

    void set_viewerContext (Handle(AIS_InteractiveContext) viewerContext_) {viewerContext=viewerContext_;}

    TopoDS_Wire buildWire();

signals:

private:
    bool type;     // 0 - line; 1 - polyline
    //bool isPath;
    std::vector<gp_Pnt> shapePoints;
    struct point normal;
    gp_Pnt currentMousePosition;
    Handle(AIS_Shape) rubberband;

    Handle(AIS_InteractiveContext) viewerContext;
};


#endif // POLYWIRE_H
