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
#include <gp_Pnt.hxx>

class Process : public QWidget
{
    Q_OBJECT
public:
    explicit Process (QWidget *parent = nullptr);
    bool isModified () {return modified;}
    virtual Process* copyCreate () = 0;
    virtual bool canEdit () = 0;

signals:

protected:
    bool modified;
};

class Extrude : public Process
{
public:
    void set_length (double length_) {length=length_;}
    double get_length () {return length;}
    Extrude* copyCreate () override;
    bool canEdit () override {return true;}
private:
    double length;       // length of the extrusion
};

class Merge : public Process
{
public:
    Merge* copyCreate () override;
    bool canEdit () override {return false;}
};

class Subtract : public Process
{
public:
    Subtract* copyCreate () override;
    bool canEdit () override {return false;}
};

#endif // PROCESS_H
