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
#include "port.hpp"
#include "CustomTreeWidgetItem.h"

void comboIndexChanged (int, Port *, Boundary *, int, CustomTreeWidgetItem *, CustomTreeWidgetItem *);
void comboTextChanged (QString value, Boundary *);

class CustomComboBox : public QComboBox {
    Q_OBJECT

public:
    CustomComboBox(QWidget *parent = nullptr) : QComboBox(parent) {
        connect(this, &QComboBox::currentIndexChanged,this, &CustomComboBox::handleCurrentIndexChanged);
        connect(this, &QComboBox::currentTextChanged,this, &CustomComboBox::handleCurrentTextChanged);
    }
    void set_port (Port *port_) {port=port_;}
    void set_boundary (Boundary *boundary_) {boundary=boundary_;}
    void set_type (int type_) {type=type_;}
    void set_itemMaterial (CustomTreeWidgetItem *itemMaterial_) {itemMaterial=itemMaterial_;}
    void set_itemWaveImpedance (CustomTreeWidgetItem *itemWaveImpedance_) {itemWaveImpedance=itemWaveImpedance_;}

signals:
    void CustomCurrentIndexChanged (int index, Port *port, Boundary *boundary, int type, CustomTreeWidgetItem *itemMaterial, CustomTreeWidgetItem *itemWaveImpedance);
    void CustomCurrentTextChanged (QString text, Boundary *boundary);

private slots:
    void handleCurrentIndexChanged (int index) {
        emit CustomCurrentIndexChanged(index,port,boundary,type,itemMaterial,itemWaveImpedance);
    }
    void handleCurrentTextChanged (QString text) {
        emit CustomCurrentTextChanged(text,boundary);
    }

private:
    Port *port;
    Boundary *boundary;
    int type;  // 0 - Port: impedance definition
               // 1 - Port: impedance calculation
               // 2 - Boundary: boundary type
    CustomTreeWidgetItem *itemMaterial;
    CustomTreeWidgetItem *itemWaveImpedance;
};


#endif // CUSTOMCOMBOBOX_H
