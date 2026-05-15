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


#ifndef LINEEDITFORM_H
#define LINEEDITFORM_H

#include "CustomOpenGLWidget.h"
#include "Polywire.h"
#include <QDialog>

namespace Ui {
class LineEditForm;
}

class LineEditForm : public QDialog
{
    Q_OBJECT

public:
    explicit LineEditForm(QWidget *parent = nullptr);
    ~LineEditForm();

    bool isValid ();
    void set_polywire (Line *);
    void populate (Line *);
    void repopulate ();
    void set_drawingWindow (CustomOpenGLWidget *drawingWindow_) {drawingWindow=drawingWindow_;}
    void set_relay (Relay *relay_) {relay=relay_;}
    void pickVertexFinished (gp_Pnt);

    void set_conversionFactor (double conversionFactor_) {conversionFactor=conversionFactor_;}

    void reject () override;

private slots:
    void on_length_returnPressed ();
    void on_positionX_returnPressed ();
    void on_positionY_returnPressed ();
    void on_positionZ_returnPressed ();
    void on_pick_clicked ();
    void on_position2X_returnPressed ();
    void on_position2Y_returnPressed ();
    void on_position2Z_returnPressed ();
    void on_pick2_clicked ();
    void on_OkButton_clicked ();

public slots:
    void on_CancelButton_clicked ();

private:
    Ui::LineEditForm *ui;

    gp_Pnt p0,p1;   // store in variable to avoid losing precision in the form
    double length;  // store in variable to avoid losing precision in the form
    bool pickPoint;
    bool pickPoint2;
    Line *polywire;
    QDoubleValidator validator;

    CustomOpenGLWidget *drawingWindow;
    Relay *relay;
    Handle(AIS_Shape) tempShape;
    double conversionFactor;   // converts from m to some other unit coming in, then back to m going out
    bool isXclose;             // user clicked the "X" to close
};

#endif // LINEEDITFORM_H
