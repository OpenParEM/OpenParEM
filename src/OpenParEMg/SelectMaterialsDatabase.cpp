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

#include "SelectMaterialsDatabase.h"
#include "ui_SelectMaterialsDatabase.h"
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include "OpenParEMmaterials.hpp"

SelectMaterialsDatabase::SelectMaterialsDatabase (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SelectMaterialsDatabase)
{
    ui->setupUi(this);

    globalPath=nullptr;
    globalFilename=nullptr;
    localPath=nullptr;
    localFilename=nullptr;

    globalIsValid=false;
    localIsValid=false;

    ui->OkButton->setEnabled(false);
}

SelectMaterialsDatabase::~SelectMaterialsDatabase ()
{
    delete ui;
}

void SelectMaterialsDatabase::set_projData (struct projectData *a)
{
    projData=a;

    ui->globalFile->setChecked(false);
    ui->globalMaterialFile->setEnabled(false);
    ui->selectGlobal->setEnabled(false);
    if (strcmp(projData->materials_global_name,"") != 0) {
        QString prefill=projData->materials_global_path;
        prefill+=projData->materials_global_name;

        ui->globalMaterialFile->setText(prefill);
        ui->globalFile->setChecked(true);
        ui->globalMaterialFile->setEnabled(true);
        ui->selectGlobal->setEnabled(true);

        emit ui->globalMaterialFile->returnPressed();
    }

    ui->localFile->setChecked(false);
    ui->localMaterialFile->setEnabled(false);
    ui->selectLocal->setEnabled(false);
    if (strcmp(projData->materials_local_name,"") != 0) {
        QString prefill=projData->materials_local_path;
        prefill+=projData->materials_local_name;

        ui->localMaterialFile->setText(prefill);
        ui->localFile->setChecked(true);
        ui->localMaterialFile->setEnabled(true);
        ui->selectLocal->setEnabled(true);

        emit ui->localMaterialFile->returnPressed();
    }

    if (simulationRunning) {
        ui->globalFile->setEnabled(false);
        ui->globalMaterialFile->setEnabled(false);
        ui->selectGlobal->setEnabled(false);
        ui->localFile->setEnabled(false);
        ui->localMaterialFile->setEnabled(false);
        ui->selectLocal->setEnabled(false);
    }
}

void SelectMaterialsDatabase::on_selectLocal_clicked ()
{
    QString localMaterialsFile=QFileDialog::getOpenFileName(this,tr("Open Materials File"), *absolutePath, tr("Data Files (*.txt);;All Files (*)"));

    // return if user cancels
    if (localMaterialsFile.isNull()) {
        emit ui->localMaterialFile->returnPressed();
        return;
    }

    ui->localMaterialFile->setText(localMaterialsFile);
    emit ui->localMaterialFile->returnPressed();
}

void SelectMaterialsDatabase::on_selectGlobal_clicked ()
{
    QString globalMaterialsFile=QFileDialog::getOpenFileName(this,tr("Open Materials File"), *absolutePath, tr("Data Files (*.txt);;All Files (*)"));

    // return if user cancels
    if (globalMaterialsFile.isNull()) {
        emit ui->globalMaterialFile->returnPressed();
        return;
    }

    ui->globalMaterialFile->setText(globalMaterialsFile);
    emit ui->globalMaterialFile->returnPressed();
}

void SelectMaterialsDatabase::on_OkButton_clicked ()
{
    if (! globalPath) {globalPath=(char *)malloc(sizeof(char)); globalPath[0]='\0';}
    if (! globalFilename) {globalFilename=(char *)malloc(sizeof(char)); globalFilename[0]='\0';}
    if (! localPath) {localPath=(char *)malloc(sizeof(char)); localPath[0]='\0';}
    if (! localFilename) {localFilename=(char *)malloc(sizeof(char)); localFilename[0]='\0';}

    if (projData->materials_global_path) free(projData->materials_global_path);
    projData->materials_global_path=globalPath;

    if (projData->materials_global_name) free(projData->materials_global_name);
    projData->materials_global_name=globalFilename;

    if (projData->materials_local_path) free(projData->materials_local_path);
    projData->materials_local_path=localPath;

    if (projData->materials_local_name) free(projData->materials_local_name);
    projData->materials_local_name=localFilename;

    close();
}

void SelectMaterialsDatabase::on_cancelButton_clicked ()
{
    close();
}

void SelectMaterialsDatabase::on_globalFile_stateChanged (int arg1)
{
    if (arg1 == 0) {
        ui->globalFile->setChecked(false);
        ui->globalMaterialFile->setEnabled(false);
        ui->selectGlobal->setEnabled(false);

        ui->globalMaterialFile->setText("");
        emit ui->globalMaterialFile->returnPressed();
    } else {
        ui->globalFile->setChecked(true);
        ui->globalMaterialFile->setEnabled(true);
        ui->selectGlobal->setEnabled(true);
    }

    ui->OkButton->setEnabled(false);
    if (localPath && localFilename || globalPath && globalFilename) ui->OkButton->setEnabled(true);
}

void SelectMaterialsDatabase::on_localFile_stateChanged (int arg1)
{
    if (arg1 == 0) {      
        ui->localFile->setChecked(false);
        ui->localMaterialFile->setEnabled(false);
        ui->selectLocal->setEnabled(false);

        ui->localMaterialFile->setText("");
        emit ui->localMaterialFile->returnPressed();
    } else {
        ui->localFile->setChecked(true);
        ui->localMaterialFile->setEnabled(true);
        ui->selectLocal->setEnabled(true);
    }

    ui->OkButton->setEnabled(false);
    if (localPath && localFilename || globalPath && globalFilename) ui->OkButton->setEnabled(true);
}

void SelectMaterialsDatabase::on_globalMaterialFile_returnPressed ()
{
    MaterialDatabase materialDatabase;

    if (globalPath) {free(globalPath); globalPath=nullptr;}
    if (globalFilename) {free(globalFilename); globalFilename=nullptr;}

    globalIsValid=true;
    if (ui->globalMaterialFile->text().compare("") != 0) {
        char nullstring[1]; nullstring[0]='\0';

        QFileInfo fileInfo(ui->globalMaterialFile->text());
        QString Path=fileInfo.absolutePath()+"/";
        QString Filename=fileInfo.fileName();

        globalPath=(char *) malloc((Path.length()+1)*sizeof(char));
        int i=0;
        while (i < Path.length()) {
            globalPath[i]=Path.data()[i].toLatin1();
            i++;
        }
        globalPath[i]='\0';

        globalFilename=(char *) malloc((Filename.length()+1)*sizeof(char));
        i=0;
        while (i < Filename.length()) {
            globalFilename[i]=Filename.data()[i].toLatin1();
            i++;
        }
        globalFilename[i]='\0';

        // test load to ensure that the file is valid
        if (materialDatabase.load_materials(nullstring,nullstring,globalPath,globalFilename,false)) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "Failed to load global materials file.");
            mb.setFixedSize(500, 200);
            free(globalPath); globalPath=nullptr;
            free(globalFilename); globalFilename=nullptr;
            globalIsValid=false;
        }
    }

    ui->OkButton->setEnabled(false);
    if (globalIsValid && localIsValid) ui->OkButton->setEnabled(true);
}

void SelectMaterialsDatabase::on_localMaterialFile_returnPressed ()
{
    MaterialDatabase materialDatabase;

    if (localPath) {free(localPath); localPath=nullptr;}
    if (localFilename) {free(localFilename); localFilename=nullptr;}

    localIsValid=true;
    if (ui->localMaterialFile->text().compare("") != 0) {
        char nullstring[1]; nullstring[0]='\0';

        QFileInfo fileInfo(ui->localMaterialFile->text());
        QString Path=fileInfo.absolutePath()+"/";
        QString Filename=fileInfo.fileName();


        localPath=(char *) malloc((Path.length()+1)*sizeof(char));
        int i=0;
        while (i < Path.length()) {
            localPath[i]=Path.data()[i].toLatin1();
            i++;
        }
        localPath[i]='\0';

        localFilename=(char *) malloc((Filename.length()+1)*sizeof(char));
        i=0;
        while (i < Filename.length()) {
            localFilename[i]=Filename.data()[i].toLatin1();
            i++;
        }
        localFilename[i]='\0';

        // test load to ensure that the file is valid
        if (materialDatabase.load_materials(nullstring,nullstring,localPath,localFilename,false)) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "Failed to load local materials file.");
            mb.setFixedSize(500, 200);
            free(localPath); localPath=nullptr;
            free(localFilename); localFilename=nullptr;
            localIsValid=false;
        }
    }

    ui->OkButton->setEnabled(false);
    if (globalIsValid && localIsValid) ui->OkButton->setEnabled(true);
}
