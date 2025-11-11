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

#ifndef SELECTMATERIALSDATABASE_H
#define SELECTMATERIALSDATABASE_H

#include <QDialog>
#include <unistd.h>
#include "project.h"
#include "OpenParEMmaterials.hpp"

namespace Ui {
class SelectMaterialsDatabase;
}

class SelectMaterialsDatabase : public QDialog{
    Q_OBJECT

public:
    explicit SelectMaterialsDatabase (QWidget *parent = nullptr);
    void set_materialDatabase (MaterialDatabase *materialDatabase_) {materialDatabase=materialDatabase_;}
    void set_absolutePath (QString *absolutePath_) {absolutePath=absolutePath_;}
    void set_projData (struct projectData *);
    void set_simulationRunning (bool simulationRunning_) {simulationRunning=simulationRunning_;}
    ~SelectMaterialsDatabase();

private slots:
    void on_selectLocal_clicked ();
    void on_selectGlobal_clicked ();
    void on_OkButton_clicked ();
    void on_cancelButton_clicked ();
    void on_globalFile_stateChanged (int);
    void on_localFile_stateChanged (int);
    void on_globalMaterialFile_returnPressed ();
    void on_localMaterialFile_returnPressed ();
    void on_checkLimits_stateChanged (int);

private:
    Ui::SelectMaterialsDatabase *ui;
    struct projectData *projData;
    MaterialDatabase *materialDatabase;
    QString *absolutePath;
    char *globalPath, *globalFilename;
    char *localPath, *localFilename;
    bool globalIsValid;
    bool localIsValid;
    int checkLimits;
    bool simulationRunning;
};

#endif // SELECTMATERIALSDATABASE_H
