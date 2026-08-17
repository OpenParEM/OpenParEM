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

#ifndef POLYCIRCLEEDITFORM_H
#define POLYCIRCLEEDITFORM_H

#include "CustomOpenGLWidget.h"
#include "Polywire.h"
#include <QDialog>
#include <gp_Dir.hxx>
#include <qvalidator.h>

namespace Ui {
class PolycircleEditForm;
}

class PolycircleEditForm : public QDialog
{
    Q_OBJECT

public:
    explicit PolycircleEditForm(QWidget *parent = nullptr);
    ~PolycircleEditForm();

    void set_Polycircle (Polycircle *);
    void set_drawingWindow (CustomOpenGLWidget *drawingWindow_) {drawingWindow=drawingWindow_;}
    void set_relay (Relay *relay_) {relay=relay_;}
    void pickVertexFinished (gp_Pnt);
    bool isValid ();

    void populate (Polycircle *);
    void repopulate ();
    // void repopulateOffFirstPoint ();
    void reject () override;

    void set_conversionFactor (double conversionFactor_) {conversionFactor=conversionFactor_;}

public slots:
    void on_CancelButton_clicked ();

private slots:
    void on_centerPositionX_editingFinished ();
    void on_centerPositionY_editingFinished ();
    void on_centerPositionZ_editingFinished ();
    void on_pickCenter_clicked ();
    void on_radius_editingFinished ();
    void on_firstPositionX_editingFinished ();
    void on_firstPositionY_editingFinished ();
    void on_firstPositionZ_editingFinished ();
    void on_pickFirst_clicked ();
    void on_vertexCount_editingFinished ();
    void on_OkButton_clicked ();

private:
    Ui::PolycircleEditForm *ui;

    Polycircle *polycircle;

    gp_Pnt centerPoint, firstPoint;  // store in variables to avoid precision loss in the form
    double radius;                   // store in variables to avoid precision loss in the form
    int vertexCount;
    bool pickCenterPoint;
    bool pickFirstPoint;
    QDoubleValidator doubleValidator;
    QIntValidator intValidator;
    CustomOpenGLWidget *drawingWindow;
    Relay *relay;
    Handle(AIS_Shape) tempShape;
    double conversionFactor;   // converts from m to some other unit coming in, then back to m going out
    bool isXclose;             // user clicked the "X" to close
    bool isClosing;            // flag to block re-populations during closing operations
};

#endif // POLYCIRCLEEDITFORM_H
