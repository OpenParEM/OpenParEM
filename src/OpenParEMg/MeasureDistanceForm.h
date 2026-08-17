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

#ifndef MEASUREDISTANCEFORM_H
#define MEASUREDISTANCEFORM_H

#include "CustomOpenGLWidget.h"
#include <QDialog>
#include <gp_Pnt.hxx>

namespace Ui {
class MeasureDistanceForm;
}

class MeasureDistanceForm : public QDialog
{
    Q_OBJECT

public:
    explicit MeasureDistanceForm(QWidget *parent = nullptr);
    ~MeasureDistanceForm();

    void pickVertexFinished (gp_Pnt);
    void set_drawingWindow (CustomOpenGLWidget *drawingWindow_) {drawingWindow=drawingWindow_;}
    void set_conversionFactor (double conversionFactor_) {conversionFactor=conversionFactor_;}
    void set_relay (Relay *relay_) {relay=relay_;}

    void reject () override;

public slots:
    void on_CloseButton_clicked ();

private slots:
    void on_pickOrigin_clicked ();
    void on_pickTip_clicked ();

private:
    Ui::MeasureDistanceForm *ui;

    bool pickStartPoint;
    bool pickEndPoint;
    gp_Pnt startPnt,endPnt;

    Handle(AIS_Shape) line;

    Relay *relay;
    CustomOpenGLWidget *drawingWindow;
    double conversionFactor;
    bool isXclose;            // user clicked the "X" to close
};

#endif // MEASUREDISTANCEFORM_H


