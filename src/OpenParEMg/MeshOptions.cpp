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

#include "MeshOptions.h"
#include "ui_MeshOptions.h"
#include "misc.hpp"

MeshDialog::MeshDialog (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MeshDialog)
{
    this->setWindowIcon(QApplication::windowIcon());
    ui->setupUi(this);
    setFixedSize(width(),height());
    ui->meshFileLineEdit->setReadOnly(true);
    localMeshObsolete=false;
}

MeshDialog::~MeshDialog ()
{
    delete ui;
}

void MeshDialog::set_projData (struct projectData *a)
{
    projData=a;

    // Save projData to local variables to enable a cancel operation with no changes
    meshFile=projData->mesh_file;
    meshSaveRefined=projData->mesh_save_refined;
    meshRefinementFraction=projData->mesh_3D_refinement_fraction;
    meshQualityLimit=projData->mesh_quality_limit;
    meshSizeMultiplier=projData->gui_mesh_scale;
    meshMinElementSize=projData->gui_mesh_minSize;
    meshMaxElementSize=projData->gui_mesh_maxSize;

    // fill the panel with data

    ui->meshFileLineEdit->blockSignals(true);
    ui->meshFileLineEdit->setText(meshFile);
    ui->meshFileLineEdit->blockSignals(false);

    ui->meshSaveRefined->blockSignals(true);
    if (meshSaveRefined) ui->meshSaveRefined->setCheckState(Qt::Checked);
    else ui->meshSaveRefined->setCheckState(Qt::Unchecked);
    ui->meshSaveRefined->blockSignals(false);

    ui->meshRefinementFraction->blockSignals(true);
    ui->meshRefinementFraction->setValue(meshRefinementFraction);
    ui->meshRefinementFraction->blockSignals(false);

    ui->meshRefinementFraction->blockSignals(true);
    if (strcmp(projData->refinement_frequency,"none") == 0) {
        ui->meshRefinementFraction->setEnabled(false);
    }
    ui->meshRefinementFraction->blockSignals(false);

    ui->meshQualityLimit->blockSignals(true);
    ui->meshQualityLimit->setValue(meshQualityLimit);
    ui->meshQualityLimit->blockSignals(false);

    ui->meshSizeMultiplier->blockSignals(true);
    ui->meshSizeMultiplier->setValue(meshSizeMultiplier);
    ui->meshSizeMultiplier->blockSignals(false);

    ui->meshMinElementSize->blockSignals(true);
    ui->meshMinElementSize->setValue(meshMinElementSize);
    ui->meshMinElementSize->blockSignals(false);

    ui->meshMaxElementSize->blockSignals(true);
    ui->meshMaxElementSize->setValue(meshMaxElementSize);
    ui->meshMaxElementSize->blockSignals(false);

    ui->meshSizeMultiplier->setDecimals(2);
    ui->meshSizeMultiplier->setMinimum(0.01);
    ui->meshSizeMultiplier->setMaximum(100);

    ui->meshMinElementSize->setDecimals(2);
    ui->meshMinElementSize->setMinimum(0);

    ui->meshMaxElementSize->setDecimals(2);
    ui->meshMaxElementSize->setMinimum(0);

    if (simulationRunning) {
        ui->meshFileLineEdit->setEnabled(false);
        ui->meshSaveRefined->setEnabled(false);
        ui->meshRefinementFraction->setEnabled(false);
        ui->meshQualityLimit->setEnabled(false);
        ui->meshSizeMultiplier->setEnabled(false);
        ui->meshMinElementSize->setEnabled(false);
        ui->meshMaxElementSize->setEnabled(false);
    }

    ui->meshOptionOk->setEnabled(false);
}

void MeshDialog::on_meshSaveRefined_checkStateChanged (const Qt::CheckState &arg1)
{
    if (arg1 == 0) meshSaveRefined=0;
    else meshSaveRefined=1;
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshRefinementFraction_valueChanged (double arg1)
{
    meshRefinementFraction=arg1;
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshQualityLimit_valueChanged (double arg1)
{
    meshQualityLimit=arg1;
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshSizeMultiplier_valueChanged (double arg1)
{
    meshSizeMultiplier=arg1;
    localMeshObsolete=true;
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshMinElementSize_valueChanged (double arg1)
{
    meshMinElementSize=arg1;
    localMeshObsolete=true;
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshMaxElementSize_valueChanged (double arg1)
{
    meshMaxElementSize=arg1;
    localMeshObsolete=true;
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshOptionOk_clicked ()
{
    if (projData->mesh_save_refined != meshSaveRefined) {
        projData->mesh_save_refined=meshSaveRefined;
        projData->modified=1;
    }

    if (projData->mesh_3D_refinement_fraction != meshRefinementFraction) {
        projData->mesh_3D_refinement_fraction=meshRefinementFraction;
        projData->modified=1;
    }

    if (projData->mesh_quality_limit != meshQualityLimit) {
        projData->mesh_quality_limit=meshQualityLimit;
        projData->modified=1;
    }

    if (!double_compare(projData->gui_mesh_scale,meshSizeMultiplier,1e-12)) {
        projData->gui_mesh_scale=meshSizeMultiplier;
        projData->modified=1;
    }

    if (!double_compare(projData->gui_mesh_minSize,meshMinElementSize,1e-12)) {
        projData->gui_mesh_minSize=meshMinElementSize;
        projData->modified=1;
    }

    if (!double_compare(projData->gui_mesh_maxSize,meshMaxElementSize,1e-12)) {
        projData->gui_mesh_maxSize=meshMaxElementSize;
        projData->modified=1;
    }

    if (!(*meshObsolete) && localMeshObsolete) *meshObsolete=localMeshObsolete;

    close();
}

void MeshDialog::on_meshOptionCancel_clicked ()
{
    close();
}



