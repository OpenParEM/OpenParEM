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


#ifndef DRAWINGPREFERENCES_H
#define DRAWINGPREFERENCES_H

#include <QDialog>
#include "project.h"

namespace Ui {
class DrawingPreferences;
}

class DrawingPreferences : public QDialog
{
    Q_OBJECT

public:
    explicit DrawingPreferences(QWidget *parent = nullptr);
    ~DrawingPreferences();

    void set_projData (struct projectData *);
    void set_simulationRunning (bool simulationRunning_) {simulationRunning=simulationRunning_;}

private slots:
    void on_units_currentTextChanged (const QString &arg1);
    void on_gridSize_editingFinished ();
    void on_OkButton_clicked ();
    void on_CancelButton_clicked ();

private:
    Ui::DrawingPreferences *ui;

    struct projectData *projData;
    QString units;
    double gridSize;
    bool simulationRunning;
};

#endif // DRAWINGPREFERENCES_H
