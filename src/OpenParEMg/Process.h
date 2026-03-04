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

#ifndef PROCESS_H
#define PROCESS_H

//#include "Polywire.h"
#include <QWidget>

class Process : public QWidget
{
    Q_OBJECT
public:
    explicit Process (QWidget *parent = nullptr);
    bool isModified () {return modified;}
    //void setCurrentMousePosition (gp_Pnt &currentMousePosition_) {currentMousePosition=currentMousePosition_;}
    //void setRubberband (TopoDS_Shape &shape) {rubberband=new AIS_Shape(shape);}
    //void moveRubberband (gp_Pnt &pnt);
    //virtual void drawRubberband (gp_Pnt &pnt) = 0;
    //void deleteRubberband ();
    //void set_viewerContext (Handle(AIS_InteractiveContext) viewerContext_) {viewerContext=viewerContext_;}

signals:

protected:
    bool modified;
    //gp_Pnt startingPosition;              // starting position of the rubberband
    //gp_Pnt currentMousePosition;          // mouse position for moving the rubberband
    //Handle(AIS_Shape) rubberband;
    //Handle(AIS_InteractiveContext) viewerContext;
};

class Extrude : public Process
{
public:
    void set_length (double length_) {length=length_;}
    double get_length () {return length;}
    //void set_Polywire (Polywire *polywire_) {polywire=polywire_;}
    //void drawRubberband (gp_Pnt &pnt) override;
private:
    double length;       // length of the extrusion
    //Polywire *polywire;  // base Polywire for extrusion
};

class Merge : public Process
{

};

class Subtract : public Process
{

};

#endif // PROCESS_H
