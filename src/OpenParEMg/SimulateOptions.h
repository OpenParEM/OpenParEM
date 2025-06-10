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

#ifndef SIMULATEOPTIONS_H
#define SIMULATEOPTIONS_H

#include <QDialog>
#include <QMessageBox>
#include <iostream>
#include <qspinbox.h>
#include "project.h"
#include "string.h"

using namespace std;

namespace Ui {
class SimOptions;
}

class SimOptions : public QDialog
{
    Q_OBJECT

public:
    explicit SimOptions(QWidget *parent = nullptr);
    void set_projData (struct projectData *);
    ~SimOptions();

private slots:
    void on_referenceImpedance_textChanged(const QString &arg1);

    void on_simulateOptionCancel_clicked();

    void on_simulateOptionOk_clicked();

    void on_frequencyUnit_currentIndexChanged(int index);

    void on_normalizeSparam_checkStateChanged(const Qt::CheckState &arg1);

    void on_touchstoneFormat_activated(int index);

    void on_tolerance2D_returnPressed();
    void on_tolerance3D_returnPressed();

    void on_temperature_valueChanged(double arg1);

    void on_iterationLimit_valueChanged(int arg1);

    void on_useInitialGuess_checkStateChanged(const Qt::CheckState &arg1);

    void on_checkHomogeneous_checkStateChanged(const Qt::CheckState &arg1);

    void on_modesBuffer_valueChanged(int arg1);

    void on_checkClosedLoop_checkStateChanged(const Qt::CheckState &arg1);

    void on_accurateResidual_stateChanged(int arg1);

    void on_shiftInvert_checkStateChanged(const Qt::CheckState &arg1);

    void on_shiftFactor_valueChanged(double arg1);

    void on_saveFields_checkStateChanged(const Qt::CheckState &arg1);

    void on_calculatePoynting_checkStateChanged(const Qt::CheckState &arg1);

    void on_showProjectFile_checkStateChanged(const Qt::CheckState &arg1);

    void on_showFrequencyPlan_checkStateChanged(const Qt::CheckState &arg1);

    void on_showImpedanceDetails_checkStateChanged(const Qt::CheckState &arg1);

    void on_showPortDefinitions_checkStateChanged(const Qt::CheckState &arg1);

    void on_showMaterials_checkStateChanged(const Qt::CheckState &arg1);

    void on_showMemoryUsage_checkStateChanged(const Qt::CheckState &arg1);

    void on_savePortFields_checkStateChanged(const Qt::CheckState &arg1);

    void on_keepTempFiles_checkStateChanged(const Qt::CheckState &arg1);

    void on_skipMixedModeConversion_checkStateChanged(const Qt::CheckState &arg1);

    void on_skipForcedReciprocity_checkStateChanged(const Qt::CheckState &arg1);

    void on_preconditioner_activated(int index);

    void on_createTestCases_checkStateChanged(const Qt::CheckState &arg1);

    void on_showDetailedCases_checkStateChanged(const Qt::CheckState &arg1);

private:
    Ui::SimOptions *ui;
    bool cancelClose;
    struct projectData *projData;
    double referenceImpedance;
    QString frequencyUnit;
    QString touchstoneFormat;
    double temperature;
    double tolerance2D;
    double tolerance3D;
    QDoubleValidator toleranceValidator;
    int iterationLimit;
    int useInitialGuess;
    int checkHomogeneous;
    int modesBuffer;
    int checkClosedLoop;
    int accurateResidual;
    int shiftInvert;
    double shiftFactor;
    int saveFields;
    int calculatePoynting;
    int showProjectFile;
    int showFrequencyPlan;
    int showImpedanceDetails;
    int showPortDefinitions;
    int showMaterials;
    int showMemoryUsage;
    int savePortFields;
    int keepTempFiles;
    int skipMixedModeConversion;
    int skipForcedReciprocity;
    int preconditioner;
    int createTestCases;
    int showDetailedCases;
};

#endif // SIMULATEOPTIONS_H
