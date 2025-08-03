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

#ifndef CUSTOMTREEWIDGETITEM_H
#define CUSTOMTREEWIDGETITEM_H

#include <QTreeWidgetItem>
#include <QMenu>
#include "AIS_Shape.hxx"

class CustomTreeWidgetItem : public QObject, public QTreeWidgetItem {
    Q_OBJECT

public:
    CustomTreeWidgetItem(QTreeWidgetItem *parent = nullptr,int type=Type) : QTreeWidgetItem(parent,type) {
        shape=nullptr;
        meshEntities=nullptr;
    }

    void set_AIS_Shape (Handle(AIS_Shape) shape_) {shape=shape_;}
    Handle(AIS_Shape) get_AIS_Shape () {return shape;}

    void set_meshEntities (std::vector<Handle(AIS_Shape)> *meshEntities_) {meshEntities=meshEntities_;}
    std::vector<Handle(AIS_Shape)>* get_meshEntities () {return meshEntities;}
    long unsigned int get_meshEntitiesSize () {return meshEntities->size();}
    Handle(AIS_Shape) get_meshEntity (long unsigned int i) {return (*meshEntities)[i];}

    void deleteChildren (QTreeWidgetItem *item)
    {
        QList<QTreeWidgetItem*> children=item->takeChildren();
        for (QTreeWidgetItem* child : children) {
            deleteChildren(child);
            delete child;
        }
    }

    void reset () {
        deleteChildren(this);
        set_AIS_Shape(nullptr);
        setForeground(0,Qt::black);
        setExpanded(Standard_False);
        set_meshEntities(nullptr);
    }

private slots:

private:
    Handle(AIS_Shape) shape;                       // for drawing
    std::vector<Handle(AIS_Shape)> *meshEntities;  // for mesh
};

#endif // CUSTOMTREEWIDGETITEM_H
