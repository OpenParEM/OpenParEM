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


#include "MaterialsOptions.h"
#include "project.h"
#include "ui_MaterialsOptions.h"

// similar to cstrFromQString
bool cstrFromString (char **aCstr, std::string& aString)
{
    if (*aCstr) free(*aCstr);
    *aCstr=(char *)malloc((aString.length()+1)*sizeof(char));
    int i=0;
    while (i < aString.length()) {
        (*aCstr)[i]=aString[i];  // ToDo: Generalize this for better character support?
        i++;
    }
    (*aCstr)[i]='\0';
    return false;
}

MaterialsOptions::MaterialsOptions (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MaterialsOptions)
{
    ui->setupUi(this);

    checkLimits=1;
    materialDatabase=nullptr;
    ui->OkButton->setEnabled(false);
}

MaterialsOptions::~MaterialsOptions ()
{
    delete ui;
}

void MaterialsOptions::set_projData (struct projectData *a)
{
    projData=a;

    checkLimits=projData->materials_check_limits;
    ui->checkLimits->blockSignals(true);
    std::cout << "0 checkLimits=" << checkLimits << std::endl; std::cout.flush();
    ui->checkLimits->setChecked(checkLimits);
    std::cout << "1 checkLimits=" << checkLimits << std::endl; std::cout.flush();
    ui->checkLimits->blockSignals(false);

    defaultMaterial="PEC";
    if (strcmp(projData->materials_default_boundary,"") != 0) {
        defaultMaterial=projData->materials_default_boundary;
    }

    if (simulationRunning) {
        ui->checkLimits->setEnabled(false);
        ui->boundaryMaterialLabel->setEnabled(false);
        ui->defaultBoundaryMaterial->setEnabled(false);
    }
}

void MaterialsOptions::fillMaterialSelector ()
{
    ui->defaultBoundaryMaterial->blockSignals(true);

    // reset

    ui->defaultBoundaryMaterial->clear();
    ui->defaultBoundaryMaterial->addItem("PEC");
    ui->defaultBoundaryMaterial->setCurrentIndex(0);

    if (!materialDatabase) {
        defaultMaterial=ui->defaultBoundaryMaterial->currentText().toStdString();
        return;
    }

    // fill
    int i=0;
    while (i < materialDatabase->get_size()) {
        ui->defaultBoundaryMaterial->addItem(QString::fromStdString(materialDatabase->get_material(i)->get_name()->get_value()));
        i++;
    }

    // set current
    int index=ui->defaultBoundaryMaterial->findText(QString::fromStdString(defaultMaterial));

    if (index >= 0) ui->defaultBoundaryMaterial->setCurrentIndex(index);
    else ui->defaultBoundaryMaterial->setCurrentIndex(0);
    defaultMaterial=ui->defaultBoundaryMaterial->currentText().toStdString();

    ui->defaultBoundaryMaterial->blockSignals(false);
}

void MaterialsOptions::on_checkLimits_stateChanged (int arg1)
{
    std::cout << "2 checkLimits=" << checkLimits << std::endl; std::cout.flush();
    if (arg1 == 0) checkLimits=0;
    else checkLimits=1;
    std::cout << "3 checkLimits=" << checkLimits << std::endl; std::cout.flush();
    ui->OkButton->setEnabled(true);
}

void MaterialsOptions::on_defaultBoundaryMaterial_currentTextChanged (const QString &arg1)
{
    defaultMaterial=arg1.toStdString();
    ui->OkButton->setEnabled(true);
}

void MaterialsOptions::on_OkButton_clicked ()
{
    // check limits
    std::cout << "4 checkLimits=" << checkLimits << std::endl; std::cout.flush();
    if (projData->materials_check_limits != checkLimits) {
        std::cout << "5 checkLimits=" << checkLimits << std::endl; std::cout.flush();
        projData->materials_check_limits=checkLimits;
        projData->modified=1;
    }

    // default boundary condition material
    if (strcmp(projData->materials_default_boundary,defaultMaterial.c_str()) != 0) {
        cstrFromString (&(projData->materials_default_boundary),defaultMaterial);
        projData->modified=1;
    }

    close();
}

void MaterialsOptions::on_CancelButton_clicked ()
{
    close();
}

