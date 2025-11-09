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

#include "string.h"


SimOptions::SimOptions(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SimOptions)
{
    ui->setupUi(this);
    this->setFixedSize(518,503);
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
    slotCount=projData->gui_slot_count;
    temperature=projData->solution_temperature;
    tolerance2D=projData->solution_2D_tolerance;
    tolerance3D=projData->solution_3D_tolerance;
    iterationLimit=projData->solution_iteration_limit;
    initialGuessLevel=projData->solution_initial_guess_level;
    checkHomogeneous=projData->solution_check_homogeneous;
    modesBuffer=projData->solution_modes_buffer;
    checkClosedLoop=projData->solution_check_closed_loop;
    accurateResidual=projData->solution_accurate_residual;
    shiftInvert=projData->solution_shift_invert;
    shiftFactor=projData->solution_shift_factor;
    saveFields=projData->project_save_fields;
    calculatePoynting=projData->project_calculate_poynting;
    showProjectFile=projData->debug_show_project;
    showFrequencyPlan=projData->debug_show_frequency_plan;
    showImpedanceDetails=projData->debug_show_impedance_details;
    showPortDefinitions=projData->debug_show_port_definitions;
    showMaterials=projData->debug_show_materials;
    showMemoryUsage=projData->debug_show_memory;
    savePortFields=projData->debug_save_port_fields;
    keepTempFiles=projData->debug_tempfiles_keep;
    skipMixedModeConversion=projData->debug_skip_mixed_conversion;
    skipForcedReciprocity=projData->debug_skip_forced_reciprocity;
    preconditioner=projData->debug_refine_preconditioner;
    createTestCases=projData->test_create_cases;
    showDetailedCases=projData->test_show_detailed_cases;

    // fill the panel with data

    ui->normalizeLabel->setEnabled(true);
    ui->referenceImpedance->setValue(referenceImpedance);
    if (referenceImpedance == 0) {
        ui->referenceImpedance->setEnabled(false);
        ui->referenceImpedanceLabel->setEnabled(false);
        ui->frequencyUnit->setEnabled(false);
        ui->frequencyUnitLabel->setEnabled(false);
        ui->touchstoneFormat->setEnabled(false);
        ui->touchstoneFormatLabel->setEnabled(false);
        ui->normalizeSparam->setCheckState(Qt::Unchecked);
        if (simulationRunning) {
            ui->normalizeSparam->setEnabled(false);
        }
    } else {
        bool enabled=true;
        if (simulationRunning) enabled=false;

        ui->referenceImpedance->setEnabled(enabled);
        ui->referenceImpedanceLabel->setEnabled(true);
        ui->frequencyUnit->setEnabled(enabled);
        ui->frequencyUnitLabel->setEnabled(true);
        ui->touchstoneFormat->setEnabled(enabled);
        ui->touchstoneFormatLabel->setEnabled(true);
        ui->normalizeSparam->setCheckState(Qt::Checked);
        ui->normalizeSparam->setEnabled(enabled);
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

    ui->slotCount->setValue(slotCount);
    ui->temperature->setValue(temperature);

    ui->tolerance2D->setText(QString::number(projData->solution_2D_tolerance,'g'));
    toleranceValidator.setNotation(QDoubleValidator::ScientificNotation);
    ui->tolerance2D->setValidator(&toleranceValidator);

    ui->tolerance3D->setText(QString::number(projData->solution_3D_tolerance,'g'));
    ui->tolerance3D->setValidator(&toleranceValidator);

    ui->iterationLimit->setValue(iterationLimit);

    ui->initialGuess->addItem("none");
    ui->initialGuess->addItem("prior solution");
    ui->initialGuess->addItem("prior solution with order ramping");
    ui->initialGuess->addItem("order ramping");
    ui->initialGuess->setCurrentIndex(projData->solution_initial_guess_level);
    ui->initialGuess->setFixedWidth(150);

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

    if (saveFields) ui->saveFields->setCheckState(Qt::Checked);
    else ui->saveFields->setCheckState(Qt::Unchecked);

    if (calculatePoynting) ui->calculatePoynting->setCheckState(Qt::Checked);
    else ui->calculatePoynting->setCheckState(Qt::Unchecked);

    if (saveFields) {
        ui->calculatePoynting->setEnabled(true);
    } else {
        ui->calculatePoynting->setCheckState(Qt::Unchecked);
        ui->calculatePoynting->setEnabled(false);
    }

    if (showProjectFile) ui->showProjectFile->setCheckState(Qt::Checked);
    else ui->showProjectFile->setCheckState(Qt::Unchecked);

    if (showFrequencyPlan) ui->showFrequencyPlan->setCheckState(Qt::Checked);
    else ui->showFrequencyPlan->setCheckState(Qt::Unchecked);

    if (showImpedanceDetails) ui->showImpedanceDetails->setCheckState(Qt::Checked);
    else ui->showImpedanceDetails->setCheckState(Qt::Unchecked);

    if (showPortDefinitions) ui->showPortDefinitions->setCheckState(Qt::Checked);
    else ui->showPortDefinitions->setCheckState(Qt::Unchecked);

    if (showMaterials) ui->showMaterials->setCheckState(Qt::Checked);
    else ui->showMaterials->setCheckState(Qt::Unchecked);

    if (showMemoryUsage) ui->showMemoryUsage->setCheckState(Qt::Checked);
    else ui->showMemoryUsage->setCheckState(Qt::Unchecked);

    if (savePortFields) ui->savePortFields->setCheckState(Qt::Checked);
    else ui->savePortFields->setCheckState(Qt::Unchecked);

    if (keepTempFiles) ui->keepTempFiles->setCheckState(Qt::Checked);
    else ui->keepTempFiles->setCheckState(Qt::Unchecked);

    if (skipMixedModeConversion) ui->skipMixedModeConversion->setCheckState(Qt::Checked);
    else ui->skipMixedModeConversion->setCheckState(Qt::Unchecked);

    if (skipForcedReciprocity) ui->skipForcedReciprocity->setCheckState(Qt::Checked);
    else ui->skipForcedReciprocity->setCheckState(Qt::Unchecked);

    ui->preconditioner->addItem("Diagonal");
    ui->preconditioner->addItem("BoomerAMG");
    index=0;
    if (preconditioner == 0) index=0;
    if (preconditioner == 1) index=1;
    ui->preconditioner->setCurrentIndex(index);

    if (createTestCases) ui->createTestCases->setCheckState(Qt::Checked);
    else ui->createTestCases->setCheckState(Qt::Unchecked);

    if (showDetailedCases) ui->showDetailedCases->setCheckState(Qt::Checked);
    else ui->showDetailedCases->setCheckState(Qt::Unchecked);

    if (simulationRunning) {
        ui->slotCount->setEnabled(false);
        ui->temperature->setEnabled(false);
        ui->tolerance2D->setEnabled(false);
        ui->tolerance3D->setEnabled(false);
        ui->iterationLimit->setEnabled(false);
        ui->initialGuess->setEnabled(false);
        ui->checkHomogeneous->setEnabled(false);
        ui->modesBuffer->setEnabled(false);
        ui->checkClosedLoop->setEnabled(false);
        ui->accurateResidual->setEnabled(false);
        ui->shiftInvert->setEnabled(false);
        ui->shiftFactor->setEnabled(false);
        ui->saveFields->setEnabled(false);
        ui->calculatePoynting->setEnabled(false);
        ui->solverIterations->setEnabled(false);
        ui->meshRefinement->setEnabled(false);
        ui->showLicense->setEnabled(false);
        ui->postProcessing->setEnabled(false);
        ui->createTestCases->setEnabled(false);
        ui->showDetailedCases->setEnabled(false);
        ui->showProjectFile->setEnabled(false);
        ui->showFrequencyPlan->setEnabled(false);
        ui->showImpedanceDetails->setEnabled(false);
        ui->showPortDefinitions->setEnabled(false);
        ui->showMaterials->setEnabled(false);
        ui->showMemoryUsage->setEnabled(false);
        ui->savePortFields->setEnabled(false);
        ui->keepTempFiles->setEnabled(false);
        ui->skipMixedModeConversion->setEnabled(false);
        ui->skipForcedReciprocity->setEnabled(false);
        ui->preconditioner->setEnabled(false);
    }

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
    // reference_impedance
    if (projData->reference_impedance != referenceImpedance) {
        projData->reference_impedance=referenceImpedance;
        projData->modified=1;
    }

    // touchstone_frequency_unit
    if (frequencyUnit.compare(projData->touchstone_frequency_unit,Qt::CaseSensitive) != 0) {
        if (projData->touchstone_frequency_unit) free(projData->touchstone_frequency_unit);
        projData->touchstone_frequency_unit=(char *)malloc((frequencyUnit.length()+1)*sizeof(char));
        int i=0;
        while (i < frequencyUnit.length()) {
            projData->touchstone_frequency_unit[i]=frequencyUnit.data()[i].toLatin1();
            i++;
        }
        projData->touchstone_frequency_unit[i]='\0';
        projData->modified=1;
    }

    // touchstone_format
    if (touchstoneFormat.compare(projData->touchstone_format,Qt::CaseSensitive) != 0) {
        if (projData->touchstone_format) free(projData->touchstone_format);
        projData->touchstone_format=(char *)malloc((touchstoneFormat.length()+1)*sizeof(char));
        int i=0;
        while (i < touchstoneFormat.length()) {
            projData->touchstone_format[i]=touchstoneFormat.data()[i].toLatin1();
            i++;
        }
        projData->touchstone_format[i]='\0';
        projData->modified=1;
    }

    // slot count
    if (projData->gui_slot_count != slotCount) {
        projData->gui_slot_count=slotCount;
        projData->modified=1;
    }

    // temperature
    if (projData->solution_temperature != temperature) {
        projData->solution_temperature=temperature;
        projData->modified=1;
    }

    // solution_2D_tolerance
    if (projData->solution_2D_tolerance != tolerance2D) {
        projData->solution_2D_tolerance=tolerance2D;
        projData->modified=1;
    }

    // solution_3D_tolerance
    if (projData->solution_3D_tolerance != tolerance3D) {
        projData->solution_3D_tolerance=tolerance3D;
        projData->modified=1;
    }

    //solution_iteration_limit
    if (projData->solution_iteration_limit != iterationLimit) {
        projData->solution_iteration_limit=iterationLimit;
        projData->modified=1;
    }

    // solution_initial_guess
    if (projData->solution_initial_guess_level != initialGuessLevel) {
        projData->solution_initial_guess_level=initialGuessLevel;
        projData->modified=1;
    }

    // solution_check_homogeneous
    if (projData->solution_check_homogeneous != checkHomogeneous) {
        projData->solution_check_homogeneous=checkHomogeneous;
        projData->modified=1;
    }

    // solution_modes_buffer
    if (projData->solution_modes_buffer != modesBuffer) {
        projData->solution_modes_buffer=modesBuffer;
        projData->modified=1;
    }

    // solution_check_closed_loop
    if (projData->solution_check_closed_loop != checkClosedLoop) {
        projData->solution_check_closed_loop=checkClosedLoop;
        projData->modified=1;
    }

    // solution_accurate_residual
    if (projData->solution_accurate_residual != accurateResidual) {
        projData->solution_accurate_residual=accurateResidual;
        projData->modified=1;
    }

    // sollution_shift_invert
    if (projData->solution_shift_invert != shiftInvert) {
        projData->solution_shift_invert=shiftInvert;
        projData->modified=1;
    }

    // solution_shift_factor
    if (projData->solution_shift_factor != shiftFactor) {
        projData->solution_shift_factor=shiftFactor;
        projData->modified=1;
    }

    if (projData->solution_shift_invert) {
        ui->shiftFactorLabel->setEnabled(true);
        ui->shiftFactor->setEnabled(true);
    } else {
        ui->shiftFactorLabel->setEnabled(false);
        ui->shiftFactor->setEnabled(false);
    }

    // project_save_fields
    if (projData->project_save_fields != saveFields) {
        projData->project_save_fields=saveFields;
        projData->modified=1;
    }

    // project_calculate_poynting
    if (projData->project_calculate_poynting != calculatePoynting) {
        projData->project_calculate_poynting=calculatePoynting;
        projData->modified=1;
    }

    // debug_show_project
    if (projData->debug_show_project != showProjectFile) {
        projData->debug_show_project=showProjectFile;
        projData->modified=1;
    }

    // debug_show_frequency_plan
    if (projData->debug_show_frequency_plan != showFrequencyPlan) {
        projData->debug_show_frequency_plan=showFrequencyPlan;
        projData->modified=1;
    }

    // debug_show_impedance_details
    if (projData->debug_show_impedance_details != showImpedanceDetails) {
        projData->debug_show_impedance_details=showImpedanceDetails;
        projData->modified=1;
    }

    // debug_show_port_definitions
    if (projData->debug_show_port_definitions != showPortDefinitions) {
        projData->debug_show_port_definitions=showPortDefinitions;
        projData->modified=1;
    }

    // debug_show_materials
    if (projData->debug_show_materials != showMaterials) {
        projData->debug_show_materials=showMaterials;
        projData->modified=1;
    }

    //debug_show_memory
    if (projData->debug_show_memory != showMemoryUsage) {
        projData->debug_show_memory=showMemoryUsage;
        projData->modified=1;
    }

    // debug_save_port
    if (projData->debug_save_port_fields != savePortFields) {
        projData->debug_save_port_fields=savePortFields;
        projData->modified=1;
    }

    // debug_tempfiles_keep
    if (projData->debug_tempfiles_keep != keepTempFiles) {
        projData->debug_tempfiles_keep=keepTempFiles;
        projData->modified=1;
    }

    // debug_skip_mixed_conversion
    if (projData->debug_skip_mixed_conversion != skipMixedModeConversion) {
        projData->debug_skip_mixed_conversion=skipMixedModeConversion;
        projData->modified=1;
    }

    // debug_skip_forced_reciprocity
    if (projData->debug_skip_forced_reciprocity != skipForcedReciprocity) {
        projData->debug_skip_mixed_conversion=skipMixedModeConversion;
        projData->modified=1;
    }

    // debug_refine_preconditioner
    if (projData->debug_refine_preconditioner != preconditioner) {
        projData->debug_refine_preconditioner=preconditioner;
        projData->modified=1;
    }

    // test_create_cases
    if (projData->test_create_cases != createTestCases) {
        projData->test_create_cases=createTestCases;
        projData->modified=1;
    }

    // test_show_detailed_cases
    if (projData->test_show_detailed_cases != showDetailedCases) {
        projData->test_show_detailed_cases=showDetailedCases;
        projData->modified=1;
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

void SimOptions::on_slotCount_valueChanged(int arg1)
{
    slotCount=arg1;
    ui->simulateOptionOk->setEnabled(true);
}

void SimOptions::on_touchstoneFormat_activated (int index)
{
    touchstoneFormat=ui->touchstoneFormat->currentText();
    ui->simulateOptionOk->setEnabled(true);
}

void SimOptions::on_temperature_valueChanged (double arg1)
{
    temperature=arg1;
    ui->simulateOptionOk->setEnabled(true);
}

void SimOptions::on_tolerance2D_returnPressed ()
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


void SimOptions::on_saveFields_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) {
        saveFields=1;
        ui->calculatePoynting->setEnabled(true);
    } else {
        saveFields=0;
        ui->calculatePoynting->setEnabled(false);
        ui->calculatePoynting->setCheckState(Qt::Unchecked);
    }
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_calculatePoynting_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) calculatePoynting=1;
    else calculatePoynting=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_showProjectFile_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) showProjectFile=1;
    else showProjectFile=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_showFrequencyPlan_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) showFrequencyPlan=1;
    else showFrequencyPlan=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_showImpedanceDetails_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) showImpedanceDetails=1;
    else showImpedanceDetails=0;
    ui->simulateOptionOk->setEnabled(true);
}

void SimOptions::on_showPortDefinitions_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) showPortDefinitions=1;
    else showPortDefinitions=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_showMaterials_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) showMaterials=1;
    else showMaterials=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_showMemoryUsage_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) showMemoryUsage=1;
    else showMemoryUsage=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_savePortFields_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) savePortFields=1;
    else savePortFields=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_keepTempFiles_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) keepTempFiles=1;
    else keepTempFiles=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_skipMixedModeConversion_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) skipMixedModeConversion=1;
    else skipMixedModeConversion=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_skipForcedReciprocity_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) skipForcedReciprocity=1;
    else skipForcedReciprocity=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_preconditioner_activated(int index)
{
    if (index == 0) preconditioner=0;
    if (index == 1) preconditioner=1;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_createTestCases_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) createTestCases=1;
    else createTestCases=0;
    ui->simulateOptionOk->setEnabled(true);
}


void SimOptions::on_showDetailedCases_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) showDetailedCases=1;
    else showDetailedCases=0;
    ui->simulateOptionOk->setEnabled(true);
}

void SimOptions::on_initialGuess_currentIndexChanged(int index)
{
    initialGuessLevel=index;
}

