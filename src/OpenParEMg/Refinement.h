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

#ifndef REFINEMENT_H
#define REFINEMENT_H

#include <QDialog>
#include <QMessageBox>
#include <QDoubleValidator>
#include "project.h"

namespace Ui {
class OPEMg_Refinement;
}

class OPEMg_Refinement : public QDialog
{
    Q_OBJECT

public:
    explicit OPEMg_Refinement(QWidget *parent = nullptr);
    void set_projData (struct projectData *);
    void set_simulationRunning (bool simulationRunning_) {simulationRunning=simulationRunning_;}
    ~OPEMg_Refinement();

private slots:
    void on_requiredPasses_editingFinished (const QString &arg1);
    void on_refinementVariable_activated (int);
    void on_refineOk_clicked ();
    void on_refineCancel_clicked ();
    void on_relativeTol_editingFinished ();
    void on_absoluteTol_editingFinished ();
    void on_refineMin_editingFinished ();
    void on_refineMax_editingFinished ();

private:
    Ui::OPEMg_Refinement *ui;
    struct projectData *projData;
    int refinementVariableIndex;
    QDoubleValidator toleranceValidator;
    bool simulationRunning;
};

#endif // REFINEMENT_H
