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

#ifndef CUSTOMCOMBOBOX_H
#define CUSTOMCOMBOBOX_H

#include <QComboBox>
#include "OPEMg.h"
#include "port.hpp"
#include "CustomTreeWidgetItem.h"

void comboIndexChanged (int, Port *, Boundary *, BoundaryDatabase *, int, CustomTreeWidgetItem *, CustomTreeWidgetItem *);
void comboTextChanged (QString value, Boundary *, BoundaryDatabase *);

class CustomComboBox : public QComboBox {
    Q_OBJECT

public:
    CustomComboBox(QWidget *parent = nullptr) : QComboBox(parent) {
        connect(this, &QComboBox::currentIndexChanged, this, &CustomComboBox::handleCurrentIndexChanged);
        connect(this, &QComboBox::currentTextChanged, this, &CustomComboBox::handleCurrentTextChanged);
        //connect(this, &QComboBox::currentIndexChanged, parent, &OpenParEMg::setMenus);
        setFocusPolicy(Qt::ClickFocus);
        drawingTracker=nullptr;
    }

    void set_port (Port *port_) {port=port_;}
    void set_boundary (Boundary *boundary_) {boundary=boundary_;}
    void set_boundaryDatabase (BoundaryDatabase *boundaryDatabase_) {boundaryDatabase=boundaryDatabase_;}
    void set_type (int type_) {type=type_;}
    void set_itemMaterial (CustomTreeWidgetItem *itemMaterial_) {itemMaterial=itemMaterial_;}
    void set_itemWaveImpedance (CustomTreeWidgetItem *itemWaveImpedance_) {itemWaveImpedance=itemWaveImpedance_;}
    void set_itemTracker (ItemTracker *drawingTracker_) {drawingTracker=drawingTracker_;}

protected:
    void focusInEvent(QFocusEvent *event) override
    {
        if (drawingTracker) drawingTracker->unselectAllItems();
        QComboBox::focusInEvent(event);
    }

    void wheelEvent(QWheelEvent *event) override
    {
        // ignore wheel events
        event->ignore();
    }

signals:
    //void CustomCurrentIndexChanged (int index, Port *port, Boundary *boundary, BoundaryDatabase *, int type, CustomTreeWidgetItem *itemMaterial, CustomTreeWidgetItem *itemWaveImpedance);
    //void CustomCurrentTextChanged (QString text, Boundary *boundary, BoundaryDatabase *);
    void CustomCurrentIndexChanged (int, Port *, Boundary *, BoundaryDatabase *, int, CustomTreeWidgetItem *, CustomTreeWidgetItem *);
    void CustomCurrentTextChanged (QString, Boundary *, BoundaryDatabase *);

private slots:
    void handleCurrentIndexChanged (int index) {
        emit CustomCurrentIndexChanged(index,port,boundary,boundaryDatabase,type,itemMaterial,itemWaveImpedance);
    }
    void handleCurrentTextChanged (QString text) {
        emit CustomCurrentTextChanged(text,boundary,boundaryDatabase);
    }

private:
    Port *port;
    Boundary *boundary;
    int type;  // 0 - Port: impedance definition
               // 1 - Port: impedance calculation
               // 2 - Boundary: boundary type
    CustomTreeWidgetItem *itemMaterial;
    CustomTreeWidgetItem *itemWaveImpedance;
    ItemTracker *drawingTracker;
    BoundaryDatabase *boundaryDatabase;  // for setting the modified flag
};


#endif // CUSTOMCOMBOBOX_H
