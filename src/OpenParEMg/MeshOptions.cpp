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

MeshDialog::MeshDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MeshDialog)
{
    ui->setupUi(this);
    this->setFixedSize(608,234);
}

MeshDialog::~MeshDialog()
{
    delete ui;
}

void MeshDialog::set_projData (struct projectData *a)
{
    projData=a;

    // Save projData to local variables to enable a cancel operation with no changes
    meshOrder=projData->mesh_order;
    meshFile=projData->mesh_file;
    meshSaveRefined=projData->mesh_save_refined;
    meshRefinementFraction=projData->mesh_3D_refinement_fraction;
    meshQualityLimit=projData->mesh_quality_limit;

    // fill the panel with data
    ui->meshOrderSpinBox->setValue(meshOrder);
    ui->meshFileLineEdit->setText(meshFile);
    if (meshSaveRefined) ui->meshSaveRefined->setCheckState(Qt::Checked);
    else ui->meshSaveRefined->setCheckState(Qt::Unchecked);
    ui->meshRefinementFraction->setValue(meshRefinementFraction);
    ui->meshQualityLimit->setValue(meshQualityLimit);

    ui->meshOptionOk->setEnabled(false);
}

void MeshDialog::on_meshOrderSpinBox_valueChanged(int arg1)
{
    meshOrder=arg1;
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshFileLineEdit_returnPressed()
{
    QString trialMeshFile=ui->meshFileLineEdit->text();
    if (QFile::exists(trialMeshFile)) {
        meshFile=trialMeshFile;
    } else {
        std::cout << "requested mesh file " << trialMeshFile.toLatin1().toStdString() << " does not exist" << std::endl;
        std::cout.flush();

        QMessageBox mb;
        mb.critical(nullptr, "Error", "The requested mesh file does not exist.");
        mb.setFixedSize(500, 200);

        ui->meshFileLineEdit->setText(meshFile);
    }
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshFilePushButton_clicked()
{
    QString trialMeshFile=QFileDialog::getOpenFileName(this,tr("Select Mesh"), "", tr("Mesh Files (*.msh);;All Files (*)"));

    if (QFile::exists(trialMeshFile)) {
        meshFile=trialMeshFile;
        ui->meshFileLineEdit->setText(meshFile);
    } else {
        std::cout << "requested mesh file " << trialMeshFile.toLatin1().toStdString() << " does not exist" << std::endl;
        std::cout.flush();

        QMessageBox mb;
        mb.critical(nullptr, "Error", "The requested mesh file does not exist.");
        mb.setFixedSize(500, 200);

        ui->meshFileLineEdit->setText(meshFile);
    }
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshSaveRefined_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == 0) meshSaveRefined=0;
    else meshSaveRefined=1;
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshRefinementFraction_textChanged(const QString &arg1)
{
    meshRefinementFraction=arg1.toDouble();
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshQualityLimit_textChanged(const QString &arg1)
{
    meshQualityLimit=arg1.toDouble();
    ui->meshOptionOk->setEnabled(true);
}

void MeshDialog::on_meshOptionOk_clicked()
{
    projData->mesh_order=meshOrder;
    projData->mesh_save_refined=meshSaveRefined;
    projData->mesh_3D_refinement_fraction=meshRefinementFraction;
    projData->mesh_quality_limit=meshQualityLimit;

    if (projData->mesh_file) free(projData->mesh_file);
    projData->mesh_file=(char *)malloc((meshFile.length()+1)*sizeof(char));
    int i=0;
    while (i < meshFile.length()) {
        projData->mesh_file[i]=meshFile.data()[i].toLatin1();
        i++;
    }
    projData->mesh_file[i]='\0';

    close();
}


void MeshDialog::on_meshOptionCancel_clicked()
{
    close();
}

