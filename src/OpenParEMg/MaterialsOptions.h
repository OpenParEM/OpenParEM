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


#ifndef MATERIALSOPTIONS_H
#define MATERIALSOPTIONS_H

#include <QDialog>
#include "OpenParEMmaterials.hpp"

namespace Ui {
class MaterialsOptions;
}

class MaterialsOptions : public QDialog
{
    Q_OBJECT

public:
    explicit MaterialsOptions (QWidget *parent = nullptr);
    ~MaterialsOptions ();

    void set_projData (struct projectData *);
    void set_materialDatabase (MaterialDatabase *materialDatabase_) {materialDatabase=materialDatabase_;}
    void fillMaterialSelector ();
    void set_simulationRunning (bool simulationRunning_) {simulationRunning=simulationRunning_;}

private slots:
    void on_checkLimits_stateChanged (int arg1);
    void on_defaultBoundaryMaterial_currentTextChanged (const QString &arg1);
    void on_OkButton_clicked ();
    void on_CancelButton_clicked ();

private:
    Ui::MaterialsOptions *ui;

    struct projectData *projData;
    MaterialDatabase *materialDatabase;
    std::string defaultMaterial;
    int checkLimits;
    bool simulationRunning;
};

#endif // MATERIALSOPTIONS_H
