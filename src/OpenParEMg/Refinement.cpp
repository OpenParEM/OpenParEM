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

#include "Refinement.h"
#include "ui_Refinement.h"

Refinement::Refinement(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Refinement)
{
    ui->setupUi(this);
    this->setFixedSize(450,317);

    toleranceValidator.setBottom(0);

    ui->refinementVariable->addItem("S");
    ui->refinementVariable->addItem("S and H");
    ui->refinementVariable->addItem("S or H");

    ui->refineOk->setEnabled(false);
}

Refinement::~Refinement()
{
    delete ui;
}

void Refinement::set_projData(struct projectData *a)
{
    projData=a;

    ui->refineMin->blockSignals(true);
    ui->refineMin->setValue(projData->refinement_iteration_min);
    ui->refineMin->blockSignals(false);

    ui->refineMax->blockSignals(true);
    ui->refineMax->setValue(projData->refinement_iteration_max);
    ui->refineMax->blockSignals(false);

    ui->requiredPasses->setValue(projData->refinement_required_passes);

    ui->relativeTol->setText(QString::number(projData->refinement_relative_tolerance,'g'));
    toleranceValidator.setNotation(QDoubleValidator::ScientificNotation);
    ui->relativeTol->setValidator(&toleranceValidator);

    ui->absoluteTol->setText(QString::number(projData->refinement_absolute_tolerance,'g'));
    ui->absoluteTol->setValidator(&toleranceValidator);

    if (strcmp(projData->refinement_variable,"S") == 0) {
        ui->refinementVariable->setCurrentIndex(0);
        ui->absoluteTolLabel->setEnabled(false);
        ui->absoluteTol->setEnabled(false);
    } else if (strcmp(projData->refinement_variable,"SandH") == 0) {
        ui->refinementVariable->setCurrentIndex(1);
        ui->absoluteTolLabel->setEnabled(true);
        ui->absoluteTol->setEnabled(true);
    } else if (strcmp(projData->refinement_variable,"SorH") == 0) {
        ui->refinementVariable->setCurrentIndex(2);
        ui->absoluteTolLabel->setEnabled(true);
        ui->absoluteTol->setEnabled(true);
    }

    ui->refineOk->setEnabled(false);
}


void Refinement::on_requiredPasses_textChanged(const QString &arg1)
{
    ui->refineOk->setEnabled(true);
}


void Refinement::on_refinementVariable_activated(int index)
{
    if (index == 0) {
        ui->absoluteTolLabel->setEnabled(false);
        ui->absoluteTol->setEnabled(false);
    } else {
        ui->absoluteTolLabel->setEnabled(true);
        ui->absoluteTol->setEnabled(true);
    }

    ui->refineOk->setEnabled(true);
}


void Refinement::on_refineOk_clicked()
{
    projData->refinement_iteration_min=ui->refineMin->value();
    projData->refinement_iteration_max=ui->refineMax->value();
    projData->refinement_required_passes=ui->requiredPasses->value();
    projData->refinement_relative_tolerance=ui->relativeTol->text().toDouble();
    projData->refinement_absolute_tolerance=ui->absoluteTol->text().toDouble();

    int index=ui->refinementVariable->currentIndex();
    if (index == 0) {
        if (projData->refinement_variable) free(projData->refinement_variable);
        projData->refinement_variable=(char *) malloc(2*sizeof(char));
        sprintf(projData->refinement_variable,"%s","S");
    } else if (index == 1) {
        if (projData->refinement_variable) free(projData->refinement_variable);
        projData->refinement_variable=(char *) malloc(6*sizeof(char));
        sprintf(projData->refinement_variable,"%s","SandH");
    } else if (index == 2) {
        if (projData->refinement_variable) free(projData->refinement_variable);
        projData->refinement_variable=(char *) malloc(5*sizeof(char));
        sprintf(projData->refinement_variable,"%s","SorH");
    }

    close();
}


void Refinement::on_refineCancel_clicked()
{
    close();
}

void Refinement::on_relativeTol_returnPressed()
{
    double relativeTol=ui->relativeTol->text().toDouble();
    if (relativeTol < 1e-15*(1-1e-14)) {
        QMessageBox mb;
        mb.information(nullptr, "Out of Range", "The minimum value for the relative tolerance is 1e-15.");
        mb.setFixedSize(500, 200);
        relativeTol=1e-15;
    }
    if (relativeTol > 1) {
        QMessageBox mb;
        mb.information(nullptr, "Out of Range", "The maximum value for the relative tolerance is 1.");
        mb.setFixedSize(500, 200);
        relativeTol=1;
    }
    ui->relativeTol->setText(QString::number(relativeTol,'g'));
    ui->refineOk->setEnabled(true);
}


void Refinement::on_absoluteTol_returnPressed()
{
    double absoluteTol=ui->absoluteTol->text().toDouble();
    if (absoluteTol < 1e-15*(1-1e-14)) {
        QMessageBox mb;
        mb.information(nullptr, "Out of Range", "The minimum value for the absoute tolerance is 1e-15.");
        mb.setFixedSize(500, 200);
        absoluteTol=1e-15;
    }
    if (absoluteTol > 1) {
        QMessageBox mb;
        mb.information(nullptr, "Out of Range", "The maximum value for the absolute tolerance is 1.");
        mb.setFixedSize(500, 200);
        absoluteTol=1;
    }
    ui->absoluteTol->setText(QString::number(absoluteTol,'g'));
    ui->refineOk->setEnabled(true);
}


void Refinement::on_refineMin_valueChanged(int arg1)
{
    if (arg1 > ui->refineMax->value()) ui->refineMin->setValue(ui->refineMax->value());
    ui->refineOk->setEnabled(true);
}


void Refinement::on_refineMax_valueChanged(int arg1)
{
    if (arg1 < ui->refineMin->value()) ui->refineMax->setValue(ui->refineMin->value());
    ui->refineOk->setEnabled(true);
}

