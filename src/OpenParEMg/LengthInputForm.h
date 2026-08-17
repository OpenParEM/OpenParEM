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

#ifndef LENGTHINPUTFORM_H
#define LENGTHINPUTFORM_H

#include "CustomOpenGLWidget.h"
#include "Process.h"
#include "Relay.h"
#include <QDialog>
#include <TopoDS_Vertex.hxx>
#include <qvalidator.h>

namespace Ui {
class LengthInputForm;
}

class LengthInputForm : public QDialog
{
    Q_OBJECT

public:
    explicit LengthInputForm(QWidget *parent = nullptr);
    ~LengthInputForm();

    void set_length (double *);
    void set_extrusionDirection (gp_Vec *extrusionDirection_) {extrusionDirection=extrusionDirection_;}
    void set_drawingWindow (CustomOpenGLWidget *drawingWindow_) {drawingWindow=drawingWindow_;}
    void set_relay (Relay *relay_) {relay=relay_;}
    //void set_Extrude (Extrude *extrude_) {extrude=extrude_;}

    void set_conversionFactor (double conversionFactor_) {conversionFactor=conversionFactor_;}

    void reject () override;

    void print_point (gp_Pnt);

private slots:
    void on_lineEdit_editingFinished ();
    void on_pickStart_clicked ();
    void on_pickEnd_clicked ();
    void on_OkButton_clicked ();

public slots:
    void pickVertexFinished (gp_Pnt);
    void on_CancelButton_clicked ();

private:
    Ui::LengthInputForm *ui;

    gp_Vec *extrusionDirection;
    double *transferLength, localLength;
    bool pickStartPoint;
    bool pickEndPoint;
    bool hasStartPoint;
    bool hasEndPoint;
    gp_Pnt startPoint, endPoint;
    QDoubleValidator validator;
    //Extrude *extrude;

    CustomOpenGLWidget *drawingWindow;
    Relay *relay;

    double conversionFactor;   // converts from m to some other unit coming in, then back to m going out
    bool isXclose;             // user clicked the "X" to close
};

#endif // LENGTHINPUTFORM_H
