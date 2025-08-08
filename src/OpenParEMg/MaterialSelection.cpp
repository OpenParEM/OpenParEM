#include "MaterialSelection.h"
#include "ui_MaterialSelection.h"

MaterialSelection::MaterialSelection(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MaterialSelection)
{
    ui->setupUi(this);
}

MaterialSelection::~MaterialSelection()
{
    delete ui;
}

void MaterialSelection::populate ()
{
    long unsigned int i=0;
    while (i < materialDatabase->get_size()) {
        Material *material=materialDatabase->get_material(i);
        QListWidgetItem *item=new QListWidgetItem();
        std::string name=material->get_name()->get_value();
        item->setText(QString::fromStdString(name));
        ui->materialList->addItem(item);
        i++;
    }
    ui->materialList->setCurrentRow(0);
}

void MaterialSelection::on_materialSelectOk_clicked()
{
    QListWidgetItem *material=ui->materialList->currentItem();
    *selectedMaterial=material->text();
    close();
}


void MaterialSelection::on_materialSelectCancel_clicked()
{
    *selectedMaterial="";
    close();
}

