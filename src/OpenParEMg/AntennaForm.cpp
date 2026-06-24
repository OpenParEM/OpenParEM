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

#include "AntennaForm.h"
#include "misc.hpp"
#include "project.h"
#include "ui_AntennaForm.h"
#include <qtimer.h>

AntennaForm::AntennaForm(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AntennaForm)
{
    ui->setupUi(this);

    // 3D patterns
    patternG3D=false;
    patternD3D=false;
    patternEtheta3D=false;
    patternEphi3D=false;
    patternHtheta3D=false;
    patternHphi3D=false;

    // currentResolution
    ui->currentResolution->setMinimum(0.01);
    ui->currentResolution->setMaximum(0.15);
    ui->currentResolution->setDecimals(2);
    ui->currentResolution->setSingleStep(0.01);

    // plotRange2D
    ui->plotRange2D->setMinimum(0.1);
    ui->plotRange2D->setMaximum(200);
    ui->plotRange2D->setDecimals(1);
    ui->plotRange2D->setSingleStep(5);

    // axisInterval2D
    ui->axisInterval2D->setMinimum(0.1);
    ui->axisInterval2D->setMaximum(200);
    ui->axisInterval2D->setDecimals(1);
    ui->axisInterval2D->setSingleStep(5);

    // plotResolution2D
    ui->plotResolution2D->setMinimum(0.1);
    ui->plotResolution2D->setMaximum(16);
    ui->plotResolution2D->setDecimals(1);
    ui->plotResolution2D->setSingleStep(0.1);

    // plotResolution3D
    ui->plotResolution3D->addItem("very coarse (15.9 deg)");
    ui->plotResolution3D->addItem("coarse (7.83 deg)");
    ui->plotResolution3D->addItem("medium (3.96 deg)");
    ui->plotResolution3D->addItem("fine (1.98 deg)");
    ui->plotResolution3D->addItem("very fine (0.991 deg)");



}

AntennaForm::~AntennaForm()
{
    delete ui;



}


void AntennaForm::check3Dpatterns ()
{
    if (patternG3D) return;
    if (patternD3D) return;
    if (patternEtheta3D) return;
    if (patternEphi3D) return;
    if (patternHtheta3D) return;
    if (patternHphi3D) return;

    // must have one pattern
    patternG3D=true;
    ui->patternG3D->setChecked(true);
}

void AntennaForm::set_projData (struct projectData *a)
{
    projData=a;

    // currentResolution
    currentResolution=projData->antenna_plot_current_resolution;
    ui->currentResolution->setValue(currentResolution);

    // plotRange2D
    plotRange2D=projData->antenna_plot_2D_range;
    ui->plotRange2D->setValue(plotRange2D);

    // axisInterval2D
    axisInterval2D=projData->antenna_plot_2D_interval;
    ui->axisInterval2D->setValue(axisInterval2D);

    // plotResolution2D
    plotResolution2D=projData->antenna_plot_2D_resolution;
    ui->plotResolution2D->setValue(plotResolution2D);

    // dataSummary2D
    dataSummary2D=projData->antenna_plot_2D_annotations;
    ui->dataSummary2D->setChecked(dataSummary2D);

    // savePlots2D
    savePlots2D=projData->antenna_plot_2D_save;
    ui->savePlots2D->setChecked(savePlots2D);

    // plotResolution3D
    plotResolution3D=projData->antenna_plot_3D_refinement-2;
    ui->plotResolution3D->setCurrentIndex(plotResolution3D);

    // generateSphere
    generateSphere=projData->antenna_plot_3D_sphere;
    ui->generateSphere->setChecked(generateSphere);

    // savePlots3D
    savePlots3D=projData->antenna_plot_3D_save;
    ui->savePlots3D->setChecked(savePlots3D);

    // saveRawData
    saveRawData=projData->antenna_plot_raw_save;
    ui->saveRawData->setChecked(saveRawData);

    // make sure at least one 3D pattern is enabled
    check3Dpatterns();

    // disable while a simulation is running
    if (simulationRunning) {

        ui->patternG3D->setEnabled(false);
        ui->patternD3D->setEnabled(false);
        ui->patternEtheta3D->setEnabled(false);
        ui->patternEphi3D->setEnabled(false);
        ui->patternHtheta3D->setEnabled(false);
        ui->patternHphi3D->setEnabled(false);

        ui->currentResolution->setEnabled(false);
        ui->plotRange2D->setEnabled(false);
        ui->axisInterval2D->setEnabled(false);
        ui->plotResolution2D->setEnabled(false);
        ui->dataSummary2D->setEnabled(false);
        ui->savePlots2D->setEnabled(false);
        ui->plotResolution3D->setEnabled(false);
        ui->generateSphere->setEnabled(false);
        ui->savePlots3D->setEnabled(false);
        ui->saveRawData->setEnabled(false);

        ui->patternTable->setEnabled(false);
        ui->add2Dslice->setEnabled(false);
        ui->delete2Dslice->setEnabled(false);
    }

    ui->OkButton->setEnabled(false);
}

void AntennaForm::on_patternG3D_stateChanged (int arg1)
{
    patternG3D=false;
    if (arg1 == 2) patternG3D=true;

    QTimer::singleShot(0, this, [this]() {
        check3Dpatterns();
    });

    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_patternD3D_stateChanged (int arg1)
{
    patternD3D=false;
    if (arg1 == 2) patternD3D=true;
    check3Dpatterns();
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_patternEtheta3D_stateChanged (int arg1)
{
    patternEtheta3D=false;
    if (arg1 == 2) patternEtheta3D=true;
    check3Dpatterns();
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_patternEphi3D_stateChanged (int arg1)
{
    patternEphi3D=false;
    if (arg1 == 2) patternEphi3D=true;
    check3Dpatterns();
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_patternHtheta3D_stateChanged (int arg1)
{
    patternHtheta3D=false;
    if (arg1 == 2) patternHtheta3D=true;
    check3Dpatterns();
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_patternHphi3D_stateChanged (int arg1)
{
    patternHphi3D=false;
    if (arg1 == 2) patternHphi3D=true;
    check3Dpatterns();
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_plotResolution3D_currentIndexChanged (int index)
{
    plotResolution3D=index;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_generateSphere_stateChanged (int arg1)
{
    generateSphere=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_savePlots3D_stateChanged (int arg1)
{
    savePlots3D=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_add2Dslice_clicked ()
{

    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_delete2Dslice_clicked ()
{

    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_plotRange2D_valueChanged (double arg1)
{
    plotRange2D=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_axisInterval2D_valueChanged (double arg1)
{
    axisInterval2D=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_plotResolution2D_valueChanged (double arg1)
{
    plotResolution2D=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_dataSummary2D_stateChanged (int arg1)
{
    dataSummary2D=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_savePlots2D_stateChanged (int arg1)
{
    savePlots2D=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_currentResolution_valueChanged (double arg1)
{
    currentResolution=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_saveRawData_stateChanged (int arg1)
{
    saveRawData=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_OkButton_clicked ()
{

    if (!double_compare(projData->antenna_plot_current_resolution,currentResolution,1e-12)){
        projData->antenna_plot_current_resolution=currentResolution;
        projData->modified=1;
    }

    if (!double_compare(projData->antenna_plot_2D_range,plotRange2D,1e-12)) {
        projData->antenna_plot_2D_range=plotRange2D;
        projData->modified=1;
    }

    if (!double_compare(projData->antenna_plot_2D_interval,axisInterval2D,1e-12)) {
        projData->antenna_plot_2D_interval=axisInterval2D;
        projData->modified=1;
    }

    if (!double_compare(projData->antenna_plot_2D_resolution,plotResolution2D,1e-12)) {
        projData->antenna_plot_2D_resolution=plotResolution2D;
        projData->modified=1;
    }

    if (projData->antenna_plot_2D_annotations != dataSummary2D) {
        projData->antenna_plot_2D_annotations=dataSummary2D;
        projData->modified=1;
    }

    if (projData->antenna_plot_2D_save != savePlots2D) {
        projData->antenna_plot_2D_save=savePlots2D;
        projData->modified=1;
    }

    if (projData->antenna_plot_3D_refinement != plotResolution3D+2) {
        projData->antenna_plot_3D_refinement=plotResolution3D+2;
        projData->modified=1;
    }

    if (projData->antenna_plot_3D_sphere != generateSphere) {
        projData->antenna_plot_3D_sphere=generateSphere;
        projData->modified=1;
    }

    if (projData->antenna_plot_3D_save != savePlots3D) {
        projData->antenna_plot_3D_save=savePlots3D;
        projData->modified=1;
    }

    if (projData->antenna_plot_raw_save != saveRawData) {
        projData->antenna_plot_raw_save=saveRawData;
        projData->modified=1;
    }

    QDialog::close();
}

void AntennaForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    QDialog::close();
}

void AntennaForm::reject ()
{
    ui->CancelButton->setChecked(true);

    QDialog::reject();
}

