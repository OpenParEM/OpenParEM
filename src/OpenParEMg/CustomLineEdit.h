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
#include <QLineEdit>
#include <QFocusEvent>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

void textValueChanged (QString, IntegrationPath *, BoundaryDatabase *);

class CustomLineEdit : public QLineEdit {
    Q_OBJECT

public:
    CustomLineEdit(QWidget *parent = nullptr) : QLineEdit(parent) {
        connect(this,&QLineEdit::textChanged,this,&CustomLineEdit::handleCustomTextChanged);
        rx.setPattern("^[A-Za-z0-9_\\[\\]]*$");  // alphanumeric plus _,[, and ]
        rxValidator.setRegularExpression(rx);
        drawingTracker=nullptr;
        integrationPath=nullptr;
        boundaryDatabase=nullptr;
    }

    void set_rxValidator() {setValidator(&rxValidator);}
    void set_itemTracker (ItemTracker *drawingTracker_) {drawingTracker=drawingTracker_;}
    void set_integrationPath (IntegrationPath *integrationPath_) {integrationPath=integrationPath_;}
    void set_boundaryDatabase (BoundaryDatabase *boundaryDatabase_) {boundaryDatabase=boundaryDatabase_;}

    IntegrationPath* get_integrationPath () {return integrationPath;}

protected:
    void focusInEvent(QFocusEvent *event) override
    {
        if (drawingTracker) drawingTracker->unselectAllItems();
        QLineEdit::focusInEvent(event);
    }

    void focusOutEvent(QFocusEvent *event) override
    {
        if (QLineEdit::isModified()) QLineEdit::returnPressed();
        QLineEdit::focusOutEvent(event);
    }

signals:
    void CustomTextChanged (QString text, IntegrationPath *, BoundaryDatabase *);

private slots:
    void handleCustomTextChanged (QString text) {
        emit CustomTextChanged(text,integrationPath,boundaryDatabase);
    }

private:
    QRegularExpression rx;
    QRegularExpressionValidator rxValidator;
    ItemTracker *drawingTracker;
    IntegrationPath *integrationPath;
    BoundaryDatabase *boundaryDatabase;
};

#endif // CUSTOMLINEEDIT_H
