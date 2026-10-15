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

#ifndef CUSTOMSPINBOX_H
#define CUSTOMSPINBOX_H

#include "ItemTracking.h"
#include <QSpinBox>
#include <QWheelEvent>

void spinValueChanged (int value, SportNumberItem *);

class CustomSpinBox : public QSpinBox
{
    Q_OBJECT

public:

    CustomSpinBox(QWidget *parent = nullptr) : QSpinBox(parent)
    {
        connect(this,&QSpinBox::valueChanged,this,&CustomSpinBox::handleCustomValueChanged);
        setFocusPolicy(Qt::ClickFocus);
        drawingTracker=nullptr;
        sportNumberItem=nullptr;
    }

    void set_itemTracker (ItemTracker *drawingTracker_) {drawingTracker=drawingTracker_;}
    void set_sportNumberItem (SportNumberItem *sportNumberItem_) {sportNumberItem=sportNumberItem_;}

protected:
    void focusInEvent(QFocusEvent *event) override
    {
        if (drawingTracker) drawingTracker->unselectAllItems();
        QSpinBox::focusInEvent(event);
    }

    void wheelEvent(QWheelEvent *event) override
    {
        // ignore wheel events
        event->ignore();
    }

signals:
    void CustomValueChanged (int, SportNumberItem *);

private slots:
    void handleCustomValueChanged (int index) {
        emit CustomValueChanged(index,sportNumberItem);
    }

private:
    ItemTracker *drawingTracker;
    SportNumberItem *sportNumberItem;
};

#endif // CUSTOMSPINBOX_H
