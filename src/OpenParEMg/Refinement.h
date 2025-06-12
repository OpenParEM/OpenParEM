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
class Refinement;
}

class Refinement : public QDialog
{
    Q_OBJECT

public:
    explicit Refinement(QWidget *parent = nullptr);
    void set_projData (struct projectData *);
    ~Refinement();

private slots:

    void on_requiredPasses_textChanged(const QString &arg1);

    void on_refinementVariable_activated(int index);

    void on_refineOk_clicked();

    void on_refineCancel_clicked();

    void on_relativeTol_returnPressed();

    void on_absoluteTol_returnPressed();

    void on_refineMin_valueChanged(int arg1);

    void on_refineMax_valueChanged(int arg1);

private:
    Ui::Refinement *ui;
    struct projectData *projData;
    QDoubleValidator toleranceValidator;
};

#endif // REFINEMENT_H
