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

#ifndef MESHOPTIONS_H
#define MESHOPTIONS_H

#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include "project.h"

namespace Ui {
class MeshDialog;
}

class MeshDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MeshDialog (QWidget *parent = nullptr);
    void set_projData (struct projectData *);
    void set_simulationRunning (bool simulationRunning_) {simulationRunning=simulationRunning_;}
    void set_meshObsolete (bool *meshObsolete_) {meshObsolete=meshObsolete_;}
    ~MeshDialog();

private slots:
    void on_meshSaveRefined_checkStateChanged (const Qt::CheckState &arg1);
    void on_meshRefinementFraction_valueChanged (double arg1);
    void on_meshQualityLimit_valueChanged (double arg1);
    void on_meshOptionOk_clicked ();
    void on_meshOptionCancel_clicked ();
    void on_meshSizeMultiplier_valueChanged (double arg1);
    void on_meshMinElementSize_valueChanged (double arg1);
    void on_meshMaxElementSize_valueChanged (double arg1);

private:
    Ui::MeshDialog *ui;
    struct projectData *projData;
    bool *meshObsolete;
    QString meshFile;
    int meshSaveRefined;
    double meshSizeMultiplier;
    double meshMinElementSize;
    double meshMaxElementSize;
    double meshRefinementFraction;
    double meshQualityLimit;
    bool simulationRunning;
    bool localMeshObsolete;
};

#endif // MESHOPTIONS_H
