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

#ifndef ROTATEINPUTFORM_H
#define ROTATEINPUTFORM_H

#include "CustomOpenGLWidget.h"
#include <QDialog>
#include <gp_Pnt.hxx>
#include <qvalidator.h>

namespace Ui {
class RotateInputForm;
}

class RotateInputForm : public QDialog
{
    Q_OBJECT

public:
    explicit RotateInputForm(QWidget *parent = nullptr);
    ~RotateInputForm();

    void set_angle (double *);
    void set_startPoint (gp_Pnt *);
    void set_endPoint (gp_Pnt *);
    void set_drawingWindow (CustomOpenGLWidget *drawingWindow_) {drawingWindow=drawingWindow_;}
    void set_relay (Relay *relay_) {relay=relay_;}

    void set_conversionFactor (double conversionFactor_) {conversionFactor=conversionFactor_;}

    void reject () override;

private slots:
    void on_Xaxis_clicked ();
    void on_Yaxis_clicked ();
    void on_Zaxis_clicked ();
    void on_CustomAxis_clicked ();
    void on_pickStart_clicked ();
    void on_pickEnd_clicked ();
    void on_OkButton_clicked ();

public slots:
    void pickVertexFinished (gp_Pnt);
    void on_CancelButton_clicked ();

private:
    Ui::RotateInputForm *ui;

    bool pickStartPoint;
    bool pickEndPoint;
    bool hasStartPoint;
    bool hasEndPoint;
    double *transferAngle, localAngle;
    gp_Pnt *transferStartPoint, *transferEndPoint, localStartPoint, localEndPoint;

    CustomOpenGLWidget *drawingWindow;
    Relay *relay;
    double conversionFactor;  // converts from m to some other unit coming in, then back to m going out
    bool isXclose;            // user clicked the "X" to close
};

#endif // ROTATEINPUTFORM_H
