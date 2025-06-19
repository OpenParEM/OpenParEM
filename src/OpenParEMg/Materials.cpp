#include "Materials.h"
#include "ui_Materials.h"

Materials::Materials(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Materials)
{
    ui->setupUi(this);
    //ui->materialsTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    //ui->materialsTree->setTextElideMode(Qt::ElideNone);

    ui->materialsTree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->materialsTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->materialsTree->header()->setStretchLastSection(false);

    QStringList headers;
    headers << "Keyword" << "Value" << "Unit";
    ui->materialsTree->setHeaderLabels(headers);

    // signals
    connect(ui->materialsTree, &QTreeWidget::itemClicked, this, &Materials::materialItemClicked);

    // menu bar

    QMenuBar *menuBar = new QMenuBar(nullptr);
    QMenu *fileMenu = new QMenu("File", menuBar);

    QAction *newAction=new QAction("New",fileMenu);
    fileMenu->addAction(newAction);
    connect(newAction, &QAction::triggered, this, &Materials::newAction_triggered);

    QAction *openAction=new QAction("Open ...",fileMenu);
    fileMenu->addAction(openAction);
    connect(openAction, &QAction::triggered, this, &Materials::openAction_triggered);

    QAction *closeAction=new QAction("Close",fileMenu);
    fileMenu->addAction(closeAction);
    connect(closeAction, &QAction::triggered, this, &Materials::closeAction_triggered);

    menuBar->addMenu(fileMenu);
    ui->materialsMenuBar->addWidget(menuBar);

    ui->materialsFrame->hide();





    ui->materialsTreeView->setModel(&materialsModel);


}

Materials::~Materials()
{
    delete ui;
}

void Materials::load()
{
    /*
    char nullstring[1]; nullstring[0]='\n';
    if (QFile::exists(materialsFile)) {
        if (type == 0) {          // global
            if (materialDatabase.load_materials(projData->materials_global_path,projData->materials_global_name,
                                            nullstring,nullstring,
                                                projData->materials_check_limits)) {
                QMessageBox mb;
                mb.critical(nullptr, "Error", "Failed to load global materials file.");
                mb.setFixedSize(500, 200);
            }

        } else if (type == 1) {   // local
            if (materialDatabase.load_materials(nullstring,nullstring,
                                            projData->materials_local_path,projData->materials_local_name,
                                                projData->materials_check_limits)) {
                QMessageBox mb;
                mb.critical(nullptr, "Error", "Failed to load global materials file.");
                mb.setFixedSize(500, 200);
            }
        }
    } else {
        QMessageBox mb;
        mb.critical(nullptr, "Error", "File not found.");
        mb.setFixedSize(500, 200);
        return;
    }
*/
}

void Materials::populate()
{
    long unsigned int i=0;
    while (i < materialDatabase.get_size()) {

        // material
        Material *material=materialDatabase.get_material(i);

        // top level item
        QTreeWidgetItem *materialItem=new QTreeWidgetItem();
        materialItem->setText(0,QString::fromStdString(material->get_name()->get_value()));
        materialItem->setFlags(materialItem->flags() | Qt::ItemIsEditable);
        ui->materialsTree->insertTopLevelItem(i,materialItem);

        // data

        long unsigned int j=0;
        while (j < material->get_temperatureList_size()) {
            Temperature *temperature=material->get_temperature(j);

            // temperature

            QTreeWidgetItem *temperatureItem=new QTreeWidgetItem();
            temperatureItem->setText(0,QString::fromStdString("Temperature"));
            if (temperature->get_temperature()->get_value().compare("any") == 0) {
                temperatureItem->setText(1,QString::fromStdString(temperature->get_temperature()->get_value()));
            } else {
                temperatureItem->setText(1,QString::number(temperature->get_temperature()->get_dbl_value(),'g'));
                temperatureItem->setText(2,QString::fromStdString("C"));
            }
            materialItem->addChild(temperatureItem);

            if (temperature->get_frequencyList_size() == 0) {  // Debye model
                QTreeWidgetItem *DebyeItem=new QTreeWidgetItem();
                DebyeItem->setText(0,QString::fromStdString("Debye Model"));
                materialItem->addChild(DebyeItem);

                QTreeWidgetItem *item=new QTreeWidgetItem();
                item->setText(0,QString::fromStdString(temperature->get_er_infinity().get_keyword()));
                item->setText(1,QString::number(temperature->get_er_infinity().get_dbl_value(),'g'));
                DebyeItem->addChild(item);

                item=new QTreeWidgetItem();
                item->setText(0,QString::fromStdString(temperature->get_delta_er().get_keyword()));
                item->setText(1,QString::number(temperature->get_delta_er().get_dbl_value(),'g'));
                DebyeItem->addChild(item);

                item=new QTreeWidgetItem();
                item->setText(0,QString::fromStdString(temperature->get_m1().get_keyword()));
                item->setText(1,QString::number(temperature->get_m1().get_dbl_value(),'g'));
                DebyeItem->addChild(item);

                item=new QTreeWidgetItem();
                item->setText(0,QString::fromStdString(temperature->get_m2().get_keyword()));
                item->setText(1,QString::number(temperature->get_m2().get_dbl_value(),'g'));
                DebyeItem->addChild(item);

                item=new QTreeWidgetItem();
                item->setText(0,QString::fromStdString(temperature->get_relative_permeability().get_keyword()));
                item->setText(1,QString::number(temperature->get_relative_permeability().get_dbl_value(),'g'));
                DebyeItem->addChild(item);


                item=new QTreeWidgetItem();
                item->setText(0,QString::fromStdString(temperature->get_loss().get_keyword()));
                item->setText(1,QString::number(temperature->get_loss().get_dbl_value(),'g'));
                QString unit="";
                if (temperature->get_loss().get_keyword().compare("conductivity") == 0) unit="S/m";
                else if (temperature->get_loss().get_keyword().compare("sigma") == 0) unit="S.m";
                item->setText(3,unit);
                DebyeItem->addChild(item);
            } else {

                // Frequency

                long unsigned int k=0;
                while (k < temperature->get_frequencyList_size()) {

                    Frequency *frequency=temperature->get_frequency(k);

                    QTreeWidgetItem *frequencyItem=new QTreeWidgetItem();
                    frequencyItem->setText(0,QString::fromStdString("Frequency"));
                    if (frequency->get_frequency()->get_value().compare("any") == 0) {
                        frequencyItem->setText(1,QString::fromStdString(frequency->get_frequency()->get_value()));
                    } else {
                        frequencyItem->setText(1,QString::number(frequency->get_frequency()->get_dbl_value(),'g'));
                        frequencyItem->setText(2,QString::fromStdString("Hz"));
                    }
                    materialItem->addChild(frequencyItem);

                    QTreeWidgetItem *item=new QTreeWidgetItem();
                    item->setText(0,QString::fromStdString(frequency->get_relative_permittivity()->get_keyword()));
                    item->setText(1,QString::number(frequency->get_relative_permittivity()->get_dbl_value(),'g'));
                    frequencyItem->addChild(item);

                    item=new QTreeWidgetItem();
                    item->setText(0,QString::fromStdString(frequency->get_relative_permeability()->get_keyword()));
                    item->setText(1,QString::number(frequency->get_relative_permeability()->get_dbl_value(),'g'));
                    frequencyItem->addChild(item);

                    item=new QTreeWidgetItem();
                    item->setText(0,QString::fromStdString(frequency->get_loss()->get_keyword()));
                    item->setText(1,QString::number(frequency->get_loss()->get_dbl_value(),'g'));
                    QString unit="";
                    if (frequency->get_loss()->get_keyword().compare("conductivity") == 0) unit="S/m";
                    else if (frequency->get_loss()->get_keyword().compare("sigma") == 0) unit="S.m";
                    item->setText(2,unit);
                    frequencyItem->addChild(item);

                    item=new QTreeWidgetItem();
                    item->setText(0,QString::fromStdString(frequency->get_Rz()->get_keyword()));
                    item->setText(1,QString::number(frequency->get_Rz()->get_dbl_value(),'g'));
                    item->setText(2,QString::fromStdString("m"));
                    frequencyItem->addChild(item);

                    k++;
                }
            }
            j++;
        }


        // sources

        QTreeWidgetItem *sourceItem=new QTreeWidgetItem();
        sourceItem->setText(0,QString::fromStdString("Source"));
        materialItem->addChild(sourceItem);

        j=0;
        while (j < material->get_sourceList_size()) {
            vector<string> lineList=material->get_source_lineList(j);

            long unsigned int k=0;
            while (k < lineList.size()) {
                QTreeWidgetItem *lineItem=new QTreeWidgetItem();
                lineItem->setText(0,QString::fromStdString(lineList[k]));
                sourceItem->addChild(lineItem);
                k++;
            }
            j++;
        }

        i++;
    }

}


void Materials::on_addMaterial_clicked()
{
    QTreeWidgetItem *materialItem=new QTreeWidgetItem();
    materialItem->setText(0,QString::fromStdString("newMaterial"));
    materialItem->setFlags(materialItem->flags() | Qt::ItemIsEditable);
    ui->materialsTree->insertTopLevelItem(ui->materialsTree->topLevelItemCount(),materialItem);

    // Source
    QTreeWidgetItem *sourceItem=new QTreeWidgetItem();
    sourceItem->setText(0,QString::fromStdString("Source"));
    materialItem->addChild(sourceItem);

    QTreeWidgetItem *lineItem=new QTreeWidgetItem();
    lineItem->setText(0,QString::fromStdString("TBD"));
    lineItem->setFlags(lineItem->flags() | Qt::ItemIsEditable);
    sourceItem->addChild(lineItem);

}

void Materials::materialItemClicked(QTreeWidgetItem *item, int column)
{
    cout << "materialItemClicked" << endl;
    cout << "   text=" << item->text(column).toLatin1().toStdString() << endl;
}


void Materials::on_deleteMaterial_clicked()
{
    QTreeWidgetItem *currentItem=ui->materialsTree->currentItem();
    if (! currentItem->parent()) {
        int index=ui->materialsTree->indexOfTopLevelItem(currentItem);
        QTreeWidgetItem *removedItem=ui->materialsTree->takeTopLevelItem(index);
        delete removedItem;
    }
}


void Materials::on_duplicateMaterial_clicked()
{
    QTreeWidgetItem *currentItem=ui->materialsTree->currentItem();
    if (! currentItem->parent()) {
        QTreeWidgetItem* clonedItem = currentItem->clone();
        int index=ui->materialsTree->indexOfTopLevelItem(currentItem);
        ui->materialsTree->insertTopLevelItem(index+1,clonedItem);
    }
}

void Materials::newAction_triggered()
{
    ui->materialsFrame->show();
}

void Materials::openAction_triggered()
{

    materialsFile=QFileDialog::getOpenFileName(this,tr("Open Materials File"), "/home/briany/OpenParEM", tr("Data Files (*.txt);;All Files (*)"));
    cout << "materialsFile=" << materialsFile.toStdString() << endl;

    // return if user cancels
    if (materialsFile.isNull()) return;

    // ToDo: tie this into projFile
    bool checkLimits=true;

    char nullstring[1]; nullstring[0]='\0';
    if (QFile::exists(materialsFile)) {

        char *filename;
        filename=(char *) malloc((materialsFile.length()+1)*sizeof(char));
        int i=0;
        while (i < materialsFile.length()) {
            filename[i]=materialsFile.data()[i].toLatin1();
            i++;
        }
        filename[i]='\0';

        cout << "filename=[" << filename << "]" << endl;

        if (materialDatabase.load_materials(nullstring,filename,nullstring,nullstring,checkLimits)) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "Failed to load materials file.");
            mb.setFixedSize(500, 200);
            return;
        }
        if (filename) free(filename);

        populate();
    } else {
        QMessageBox mb;
        mb.critical(nullptr, "Error", "File not found.");
        mb.setFixedSize(500, 200);
        return;
    }

    ui->materialsFrame->show();
}

void Materials::closeAction_triggered()
{
    close();
}

