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

MeshDialog::MeshDialog (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MeshDialog)
{
    ui->setupUi(this);
    setFixedSize(608,234);
    ui->meshFileLineEdit->setReadOnly(true);
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

    // fill the panel with data

    ui->meshFileLineEdit->setText(meshFile);

    if (meshSaveRefined) ui->meshSaveRefined->setCheckState(Qt::Checked);
    else ui->meshSaveRefined->setCheckState(Qt::Unchecked);

    ui->meshRefinementFraction->setValue(meshRefinementFraction);
    if (strcmp(projData->refinement_frequency,"none") == 0) {
        ui->meshRefinementFraction->setEnabled(false);
    }
    ui->meshQualityLimit->setValue(meshQualityLimit);

    if (simulationRunning) {
        ui->meshFileLineEdit->setEnabled(false);
        ui->meshSaveRefined->setEnabled(false);
        ui->meshRefinementFraction->setEnabled(false);
        ui->meshQualityLimit->setEnabled(false);
    }

    ui->meshOptionOk->setEnabled(false);
}

void MeshDialog::on_meshSaveRefined_checkStateChanged (const Qt::CheckState &arg1)
{
    if (arg1 == 0) meshSaveRefined=0;
    else meshSaveRefined=1;
    projData->modified=1;
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshRefinementFraction_textChanged (const QString &arg1)
{
    meshRefinementFraction=arg1.toDouble();
    projData->modified=1;
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshQualityLimit_textChanged (const QString &arg1)
{
    meshQualityLimit=arg1.toDouble();
    projData->modified=1;
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshOptionOk_clicked ()
{
    projData->mesh_save_refined=meshSaveRefined;
    projData->mesh_3D_refinement_fraction=meshRefinementFraction;
    projData->mesh_quality_limit=meshQualityLimit;

    close();
}

void MeshDialog::on_meshOptionCancel_clicked ()
{
    close();
}

