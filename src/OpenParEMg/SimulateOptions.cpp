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

#include "SimulateOptions.h"
#include "ui_SimulateOptions.h"


SimOptions::SimOptions(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SimOptions)
{
    ui->setupUi(this);
}

SimOptions::~SimOptions()
{
    delete ui;
}


void SimOptions::set_projData (struct projectData *a)
{
    projData=a;

    // Save projData to local variables to enable a cancel operation with no changes
    referenceImpedance=projData->reference_impedance;
    frequencyUnit=projData->touchstone_frequency_unit;
    touchstoneFormat=projData->touchstone_format;
    temperature=projData->solution_temperature;
    tolerance2D=projData->solution_2D_tolerance;
    tolerance3D=projData->solution_3D_tolerance;
    iterationLimit=projData->solution_iteration_limit;
    useInitialGuess=projData->solution_use_initial_guess;
    checkHomogeneous=projData->solution_check_homogeneous;
    modesBuffer=projData->solution_modes_buffer;
    checkClosedLoop=projData->solution_check_closed_loop;
    accurateResidual=projData->solution_accurate_residual;
    shiftInvert=projData->solution_shift_invert;
    shiftFactor=projData->solution_shift_factor;

    // fill the panel with data

    ui->referenceImpedance->setValue(referenceImpedance);
    if (referenceImpedance == 0) {
        ui->referenceImpedance->setEnabled(false);
        ui->referenceImpedanceLabel->setEnabled(false);
        ui->frequencyUnit->setEnabled(false);
        ui->frequencyUnitLabel->setEnabled(false);
        ui->touchstoneFormat->setEnabled(false);
        ui->touchstoneFormatLabel->setEnabled(false);
        ui->normalizeSparam->setCheckState(Qt::Unchecked);
    } else {
        ui->referenceImpedance->setEnabled(true);
        ui->referenceImpedanceLabel->setEnabled(true);
        ui->frequencyUnit->setEnabled(true);
        ui->frequencyUnitLabel->setEnabled(true);
        ui->touchstoneFormat->setEnabled(true);
        ui->touchstoneFormatLabel->setEnabled(true);
        ui->normalizeSparam->setCheckState(Qt::Checked);
    }

    ui->frequencyUnit->addItem("Hz");
    ui->frequencyUnit->addItem("kHz");
    ui->frequencyUnit->addItem("MHz");
    ui->frequencyUnit->addItem("GHz");
    int index=0;
    if (strcmp(projData->touchstone_frequency_unit,"Hz") == 0) index=0;
    if (strcmp(projData->touchstone_frequency_unit,"kHz") == 0) index=1;
    if (strcmp(projData->touchstone_frequency_unit,"MHz") == 0) index=2;
    if (strcmp(projData->touchstone_frequency_unit,"GHz") == 0) index=3;
    ui->frequencyUnit->setCurrentIndex(index);

    ui->touchstoneFormat->addItem("DB");
    ui->touchstoneFormat->addItem("MA");
    ui->touchstoneFormat->addItem("RI");
    index=0;
    if (strcmp(projData->touchstone_format,"DB") == 0) index=0;
    if (strcmp(projData->touchstone_format,"MA") == 0) index=1;
    if (strcmp(projData->touchstone_format,"RI") == 0) index=2;
    ui->touchstoneFormat->setCurrentIndex(index);

    ui->temperature->setValue(temperature);

    ui->tolerance2D->setText(QString::number(projData->solution_2D_tolerance,'g'));
    toleranceValidator.setNotation(QDoubleValidator::ScientificNotation);
    ui->tolerance2D->setValidator(&toleranceValidator);

    ui->tolerance3D->setText(QString::number(projData->solution_3D_tolerance,'g'));
    ui->tolerance3D->setValidator(&toleranceValidator);

    ui->iterationLimit->setValue(iterationLimit);

    if (useInitialGuess) ui->useInitialGuess->setCheckState(Qt::Checked);
    else ui->useInitialGuess->setCheckState(Qt::Unchecked);

    if (checkHomogeneous) ui->checkHomogeneous->setCheckState(Qt::Checked);
    else ui->checkHomogeneous->setCheckState(Qt::Unchecked);

    if (checkClosedLoop) ui->checkClosedLoop->setCheckState(Qt::Checked);
    else ui->checkClosedLoop->setCheckState(Qt::Unchecked);

    if (accurateResidual) ui->accurateResidual->setCheckState(Qt::Checked);
    else ui->accurateResidual->setCheckState(Qt::Unchecked);

    if (shiftInvert) ui->shiftInvert->setCheckState(Qt::Checked);
    else ui->shiftInvert->setCheckState(Qt::Unchecked);

    ui->shiftFactor->setValue(shiftFactor);
    if (shiftInvert) {
        ui->shiftFactorLabel->setEnabled(true);
        ui->shiftFactor->setEnabled(true);
    } else {
        ui->shiftFactorLabel->setEnabled(false);
        ui->shiftFactor->setEnabled(false);
    }

    ui->modesBuffer->setValue(modesBuffer);

    // Disable the Ok button until changes are made
    ui->simulateOptionOk->setEnabled(false);
}

void SimOptions::on_frequencyUnit_currentIndexChanged(int index)
{
    frequencyUnit=ui->frequencyUnit->currentText();
    ui->simulateOptionOk->setEnabled(true);
}

void SimOptions::on_referenceImpedance_textChanged(const QString &arg1)
{
    referenceImpedance=arg1.toDouble();
    ui->simulateOptionOk->setEnabled(true);
}

void SimOptions::on_simulateOptionOk_clicked()
{
    projData->reference_impedance=referenceImpedance;

    if (projData->touchstone_frequency_unit) free(projData->touchstone_frequency_unit);
    projData->touchstone_frequency_unit=(char *)malloc((frequencyUnit.length()+1)*sizeof(char));
    int i=0;
    while (i < frequencyUnit.length()) {
        projData->touchstone_frequency_unit[i]=frequencyUnit.data()[i].toLatin1();
        i++;
    }
    projData->touchstone_frequency_unit[i]='\0';

    if (projData->touchstone_format) free(projData->touchstone_format);
    projData->touchstone_format=(char *)malloc((touchstoneFormat.length()+1)*sizeof(char));
    i=0;
    while (i < touchstoneFormat.length()) {
        projData->touchstone_format[i]=touchstoneFormat.data()[i].toLatin1();
        i++;
    }
    projData->touchstone_format[i]='\0';

    projData->solution_temperature=temperature;
    projData->solution_2D_tolerance=tolerance2D;
    projData->solution_3D_tolerance=tolerance3D;
    projData->solution_iteration_limit=iterationLimit;
    projData->solution_use_initial_guess=useInitialGuess;
    projData->solution_check_homogeneous=checkHomogeneous;
    projData->solution_modes_buffer=modesBuffer;
    projData->solution_check_closed_loop=checkClosedLoop;
    projData->solution_accurate_residual=accurateResidual;

    projData->solution_shift_invert=shiftInvert;
    projData->solution_shift_factor=shiftFactor;
    if (projData->solution_shift_invert) {
        ui->shiftFactorLabel->setEnabled(true);
        ui->shiftFactor->setEnabled(true);
    } else {
        ui->shiftFactorLabel->setEnabled(false);
        ui->shiftFactor->setEnabled(false);
    }

    close();
}

void SimOptions::on_simulateOptionCancel_clicked()
{
    close();
}

void SimOptions::on_normalizeSparam_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) {
        ui->referenceImpedance->setEnabled(true);
        ui->referenceImpedanceLabel->setEnabled(true);
        ui->frequencyUnit->setEnabled(true);
        ui->frequencyUnitLabel->setEnabled(true);
        ui->touchstoneFormat->setEnabled(true);
        ui->touchstoneFormatLabel->setEnabled(true);
        if (referenceImpedance == 0) {
            referenceImpedance=50;
        }
    } else {
        referenceImpedance=0;
        ui->referenceImpedance->setEnabled(false);
        ui->referenceImpedanceLabel->setEnabled(false);
        ui->frequencyUnit->setEnabled(false);
        ui->frequencyUnitLabel->setEnabled(false);
        ui->touchstoneFormat->setEnabled(false);
        ui->touchstoneFormatLabel->setEnabled(false);
    }

    ui->referenceImpedance->setValue(referenceImpedance);
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_touchstoneFormat_activated(int index)
{
    touchstoneFormat=ui->touchstoneFormat->currentText();
    ui->simulateOptionOk->setEnabled(true);
}

void SimOptions::on_temperature_valueChanged(double arg1)
{
    temperature=arg1;
    ui->simulateOptionOk->setEnabled(true);
}

void SimOptions::on_tolerance2D_returnPressed()
{
    tolerance2D=ui->tolerance2D->text().toDouble();
    if (tolerance2D < 1e-15*(1-1e-14)) {
        QMessageBox mb;
        mb.information(nullptr, "Out of Range", "The minimum value for the 2D tolerance is 1e-15.");
        mb.setFixedSize(500, 200);
        tolerance2D=1e-15;
    }
    if (tolerance2D > 1e-06*(1+1e-14)) {
        QMessageBox mb;
        mb.information(nullptr, "Out of Range", "The maximum value for the 2D tolerance is 1e-06.");
        mb.setFixedSize(500, 200);
        tolerance2D=1e-06;
    }
    ui->tolerance2D->setText(QString::number(tolerance2D,'g'));
    ui->simulateOptionOk->setEnabled(true);
}

void SimOptions::on_tolerance3D_returnPressed()
{
    tolerance3D=ui->tolerance3D->text().toDouble();
    if (tolerance3D < 1e-15*(1-1e-14)) {
        QMessageBox mb;
        mb.information(nullptr, "Out of Range", "The minimum value for the 3D tolerance is 1e-15.");
        mb.setFixedSize(500, 200);
        tolerance3D=1e-15;
    }
    if (tolerance3D > 1e-06*(1+1e-14)) {
        QMessageBox mb;
        mb.information(nullptr, "Out of Range", "The maximum value for the 3D tolerance is 1e-06.");
        mb.setFixedSize(500, 200);
        tolerance3D=1e-06;
    }
    ui->tolerance3D->setText(QString::number(tolerance3D,'g'));
    ui->simulateOptionOk->setEnabled(true);
}

void SimOptions::on_iterationLimit_valueChanged(int arg1)
{
    iterationLimit=arg1;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_useInitialGuess_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) useInitialGuess=1;
    else useInitialGuess=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_checkHomogeneous_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) checkHomogeneous=1;
    else checkHomogeneous=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_modesBuffer_valueChanged(int arg1)
{
    modesBuffer=arg1;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_checkClosedLoop_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) checkClosedLoop=1;
    else checkClosedLoop=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_accurateResidual_stateChanged(int arg1)
{
    accurateResidual=arg1;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_shiftInvert_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) {
        shiftInvert=1;
        ui->shiftFactorLabel->setEnabled(true);
        ui->shiftFactor->setEnabled(true);
    } else {
        shiftInvert=0;
        ui->shiftFactorLabel->setEnabled(false);
        ui->shiftFactor->setEnabled(false);
    }
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_shiftFactor_valueChanged(double arg1)
{
    shiftFactor=arg1;
    ui->simulateOptionOk->setEnabled(true);
}

