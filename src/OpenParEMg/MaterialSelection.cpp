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

#include "MaterialSelection.h"
#include "ui_MaterialSelection.h"

MaterialSelection::MaterialSelection (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MaterialSelection)
{
    ui->setupUi(this);
}

MaterialSelection::~MaterialSelection ()
{
    delete ui;
}

// materialType is conductor, dielectric, or any
void MaterialSelection::populate (std::string materialType)
{
    //std::cout << "MaterialSelection::populate  materialDatabase=" << materialDatabase << std::endl;  std::cout.flush();

    if (!materialDatabase) return;

    long unsigned int i=0;
    while (i < materialDatabase->get_size()) {
        Material *material=materialDatabase->get_material(i);

        if ((material->is_conductor() && materialType.compare("conductor") == 0) ||
            (material->is_dielectric() && materialType.compare("dielectric") == 0) ||
            (materialType.compare("any") == 0)) {

            QListWidgetItem *item=new QListWidgetItem();
            std::string name=material->get_name()->get_value();
            item->setText(QString::fromStdString(name));
            ui->materialList->addItem(item);
        }
        i++;
    }
    ui->materialList->setCurrentRow(0);
}

void MaterialSelection::on_materialSelectOk_clicked ()
{
    QListWidgetItem *material=ui->materialList->currentItem();
    *selectedMaterial=material->text();
    close();
}


void MaterialSelection::on_materialSelectCancel_clicked ()
{
    *selectedMaterial="";
    close();
}

