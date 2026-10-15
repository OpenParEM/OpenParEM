////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEMg - A GUI for OpenParEM3D                                      //
//    Copyright (C) 2026 Brian Young                                          //
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
#include "port.hpp"
#include "CustomTreeWidgetItem.h"

void comboRefresh (int, PortItem *, BoundaryItem *, int, BaseItem *, BaseItem *);
void comboIndexChanged (int, PortItem *, BoundaryItem *, int, BaseItem *, BaseItem *);
void comboTextChanged (QString value, BoundaryItem *);

class CustomComboBox : public QComboBox {
    Q_OBJECT

public:
    CustomComboBox(QWidget *parent = nullptr) : QComboBox(parent) {
        connect(this,&QComboBox::currentIndexChanged,this,&CustomComboBox::handleCurrentIndexChanged);
        connect(this,&QComboBox::currentTextChanged,this,&CustomComboBox::handleCurrentTextChanged);
        setFocusPolicy(Qt::ClickFocus);
        portItem=nullptr;
        boundaryItem=nullptr;
        itemMaterial=nullptr;
        itemWaveImpedance=nullptr;
        drawingTracker=nullptr;
    }

    void set_portItem (PortItem *portItem_) {portItem=portItem_;}
    void set_boundaryItem (BoundaryItem *boundaryItem_) {boundaryItem=boundaryItem_;}
    void set_type (int type_) {type=type_;}
    void set_itemMaterial (BaseItem *itemMaterial_) {itemMaterial=itemMaterial_;}
    void set_itemWaveImpedance (BaseItem *itemWaveImpedance_) {itemWaveImpedance=itemWaveImpedance_;}
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
    void CustomCurrentIndexChanged (int, PortItem *, BoundaryItem *, int, BaseItem *, BaseItem *);
    void CustomCurrentTextChanged (QString, BoundaryItem *);

private slots:
    void handleCurrentIndexChanged (int index) {
        emit CustomCurrentIndexChanged(index,portItem,boundaryItem,type,itemMaterial,itemWaveImpedance);
    }
    void handleCurrentTextChanged (QString text) {
        emit CustomCurrentTextChanged(text,boundaryItem);
    }

private:
    PortItem *portItem;
    BoundaryItem *boundaryItem;
    int type;  // 0 - Port: impedance definition
               // 1 - Port: impedance calculation
               // 2 - Boundary: boundary type
    BaseItem *itemMaterial;
    BaseItem *itemWaveImpedance;
    ItemTracker *drawingTracker;
};


#endif // CUSTOMCOMBOBOX_H
