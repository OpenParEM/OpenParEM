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

#include "DrawingPreferences.h"
#include "ui_DrawingPreferences.h"

DrawingPreferences::DrawingPreferences (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DrawingPreferences)
{
    this->setWindowIcon(QApplication::windowIcon());
    ui->setupUi(this);
    setFixedSize(width(),height());
}

DrawingPreferences::~DrawingPreferences ()
{
    delete ui;
}

void DrawingPreferences::set_projData (struct projectData *a)
{
    projData=a;

    // Save projData to local variables to enable a cancel operation with no changes
    units=projData->gui_units;
    gridSize=projData->gui_grid_size;

    // fill the panel with data

    ui->units->setCurrentText(units);
    ui->gridSize->setValue(gridSize);


    if (simulationRunning) {
        ui->units->setEnabled(false);
        ui->gridSize->setEnabled(false);
    }

    ui->OkButton->setEnabled(false);
}

void DrawingPreferences::on_units_currentTextChanged (const QString &arg1)
{
    units=arg1;
    ui->OkButton->setEnabled(true);
    ui->OkButton->setFocus();
}

void DrawingPreferences::on_gridSize_valueChanged (double arg1)
{
    gridSize=arg1;
    ui->OkButton->setEnabled(true);
    ui->OkButton->setFocus();
}

void DrawingPreferences::on_OkButton_clicked ()
{
    if (projData->gui_grid_size != gridSize) {
        projData->gui_grid_size=gridSize;
        projData->modified=1;
    }

    if (units.compare(projData->gui_units) != 0) {
        if (projData->gui_units) delete projData->gui_units;
        projData->gui_units=(char *)malloc((units.length()+1)*sizeof(char));
        std::sprintf(projData->gui_units,"%s",units.toStdString().c_str());
        projData->modified=1;
    }

    close();
}

void DrawingPreferences::on_CancelButton_clicked ()
{
    close();
}

