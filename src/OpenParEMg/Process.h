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
#include "ObjectCounts.h"

class CustomTreeWidgetItem;

class Process : public QWidget
{
    Q_OBJECT
public:
    explicit Process (QWidget *parent = nullptr);
    virtual QString getTip (double) = 0;
    virtual Process* copyCreate () = 0;
    virtual bool canEdit () = 0;
    virtual QString getName (ObjectCounts *objectCounts) = 0;
    virtual void startSave (std::ofstream *, QString, QString, int) = 0;
    virtual void endSave (std::ofstream *, int) = 0;

signals:

protected:
    bool modified;
    //std::vector<CustomTreeWidgetItem *> childList;
};

class Extrude : public Process
{
public:
    QString getTip (double) override;
    void set_length (double length_) {length=length_;}
    double get_length () {return length;}
    Extrude* copyCreate () override;
    bool canEdit () override {return true;}
    QString getName (ObjectCounts *objectCounts) override;
    void startSave (std::ofstream *, QString, QString, int) override;
    void endSave (std::ofstream *, int) override;
private:
    double length;       // length of the extrusion
};

class Merge : public Process
{
public:
    QString getTip (double) override;
    Merge* copyCreate () override;
    bool canEdit () override {return false;}
    QString getName (ObjectCounts *objectCounts) override;
    void startSave (std::ofstream *, QString, QString, int) override;
    void endSave (std::ofstream *, int) override;
};

class Subtract : public Process
{
public:
    QString getTip (double) override;
    Subtract* copyCreate () override;
    bool canEdit () override {return false;}
    QString getName (ObjectCounts *objectCounts) override;
    void startSave (std::ofstream *, QString, QString, int) override;
    void endSave (std::ofstream *, int) override;
};

#endif // PROCESS_H
