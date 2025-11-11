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

void MaterialSelection::populate ()
{
    if (!materialDatabase) return;

    materialDatabase->print("populate: ");
    std::cout << "place a" << std::endl; std::cout.flush();
    long unsigned int i=0;
    while (i < materialDatabase->get_size()) {
        std::cout << "place b" << std::endl; std::cout.flush();
        Material *material=materialDatabase->get_material(i);
        std::cout << "place c" << std::endl; std::cout.flush();
        QListWidgetItem *item=new QListWidgetItem();
        std::cout << "place d" << std::endl; std::cout.flush();
        std::string name=material->get_name()->get_value();
        std::cout << "place e" << std::endl; std::cout.flush();
        item->setText(QString::fromStdString(name));
        std::cout << "place f" << std::endl; std::cout.flush();
        ui->materialList->addItem(item);
        std::cout << "place g" << std::endl; std::cout.flush();
        i++;
    }
    std::cout << "place h" << std::endl; std::cout.flush();
    ui->materialList->setCurrentRow(0);
    std::cout << "place i" << std::endl; std::cout.flush();
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

