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

#include <QLineEdit>
#include <QFocusEvent>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

class CustomLineEdit : public QLineEdit {
    Q_OBJECT

public:
    CustomLineEdit(QWidget *parent = nullptr) : QLineEdit(parent) {
        //rx.setPattern("[A-Za-z0-9]*");         // alphanumeric
        rx.setPattern("^[A-Za-z0-9_\\[\\]]*$");  // alphanumeric plus _,[, and ]
        rxValidator.setRegularExpression(rx);
    }

    void set_rxValidator() {setValidator(&rxValidator);}

protected:
    void focusOutEvent(QFocusEvent *event) override
    {
        if (QLineEdit::isModified()) QLineEdit::returnPressed();
        QLineEdit::focusOutEvent(event);
    }

private:
    QRegularExpression rx;
    QRegularExpressionValidator rxValidator;
};

#endif // CUSTOMLINEEDIT_H
