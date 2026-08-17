////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEMg - A GUI for OpenParEM3D                                      //
//    Copyright (C) 2026 Brian Young                                          //
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

#ifndef MATERIALSELECTION_H
#define MATERIALSELECTION_H

#include <QDialog>
#include "OpenParEMmaterials.hpp"

namespace Ui {
class MaterialSelection;
}

class MaterialSelection : public QDialog
{
    Q_OBJECT

public:
    explicit MaterialSelection (QWidget *parent = nullptr);
    ~MaterialSelection ();

    void set_materialDatabase (MaterialDatabase *materialDatabase_) {materialDatabase=materialDatabase_;}
    void set_selectedMaterial (QString *selectedMaterial_) {selectedMaterial=selectedMaterial_;}

    void populate (std::string);
private slots:
    void on_materialSelectOk_clicked ();
    void on_materialSelectCancel_clicked ();

private:
    Ui::MaterialSelection *ui;
    MaterialDatabase *materialDatabase;
    QString *selectedMaterial;
};

#endif // MATERIALSELECTION_H
