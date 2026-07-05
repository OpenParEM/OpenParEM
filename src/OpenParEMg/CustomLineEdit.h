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

#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H

#include "ItemTracking.h"
#include "port.hpp"
#include "CustomTreeWidgetItem.h"
#include <QLineEdit>
#include <QFocusEvent>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

void textValueChanged (QString, BaseItem *, BoundaryDatabase *);

class CustomLineEdit : public QLineEdit {
    Q_OBJECT

public:
    CustomLineEdit(QWidget *parent = nullptr) : QLineEdit(parent) {
        connect(this,&QLineEdit::editingFinished,this,&CustomLineEdit::handleFinishEdit);
        rx.setPattern("^[A-Za-z0-9_\\[\\]]*$");  // alphanumeric plus _,[, and ]
        rxValidator.setRegularExpression(rx);
        doubleValidator.setNotation(QDoubleValidator::ScientificNotation);
        doubleValidator.setBottom(0);
        drawingTracker=nullptr;
        baseItem=nullptr;
        boundaryDatabase=nullptr;
    }

    void set_rxValidator () {setValidator(&rxValidator);}
    void set_doubleValidator () {setValidator(&doubleValidator);}
    void set_itemTracker (ItemTracker *drawingTracker_) {drawingTracker=drawingTracker_;}
    void set_baseItem (BaseItem *baseItem_) {baseItem=baseItem_;}
    void set_boundaryDatabase (BoundaryDatabase *boundaryDatabase_) {boundaryDatabase=boundaryDatabase_;}

    //BaseItem* get_baseItem () {return baseItem;}

protected:
    void focusInEvent(QFocusEvent *event) override
    {
        if (drawingTracker) drawingTracker->unselectAllItems();
        QLineEdit::focusInEvent(event);
    }

    // void focusOutEvent(QFocusEvent *event) override
    // {
    //     //if (QLineEdit::isModified()) emit QLineEdit::returnPressed();
    //     //qDebug() << "--- Focus Lost! ---" << event->reason();
    //     QLineEdit::focusOutEvent(event);
    // }

    void mouseDoubleClickEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            selectAll();
            event->accept();
            return;
        }
        QLineEdit::mouseDoubleClickEvent(event);
    }

signals:
    void CustomEditFinished (QString text, BaseItem *, BoundaryDatabase *);

private slots:
    void handleFinishEdit () {
        emit CustomEditFinished(this->text(),baseItem,boundaryDatabase);
    }

private:
    QRegularExpression rx;
    QRegularExpressionValidator rxValidator;
    QDoubleValidator doubleValidator;
    ItemTracker *drawingTracker;
    BaseItem *baseItem;
    BoundaryDatabase *boundaryDatabase;
};

#endif // CUSTOMLINEEDIT_H
