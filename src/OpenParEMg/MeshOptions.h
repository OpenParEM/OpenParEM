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
    bool set_simulationRunning (bool simulationRunning_) {simulationRunning=simulationRunning_;}
    ~MeshDialog();

private slots:
    void on_meshOrderSpinBox_valueChanged (int);
    void on_meshSaveRefined_checkStateChanged (const Qt::CheckState &arg1);
    void on_meshRefinementFraction_textChanged (const QString &arg1);
    void on_meshQualityLimit_textChanged (const QString &arg1);
    void on_meshOptionOk_clicked ();
    void on_meshOptionCancel_clicked ();
private:
    Ui::MeshDialog *ui;
    struct projectData *projData;
    int meshOrder;
    QString meshFile;
    int meshSaveRefined;
    double meshRefinementFraction;
    double meshQualityLimit;
    bool simulationRunning;
};

#endif // MESHOPTIONS_H
