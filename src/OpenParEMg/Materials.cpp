#include "Materials.h"
#include "ui_Materials.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LineEditDelegate
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QWidget* LineEditDelegate::createEditor (QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CustomLineEdit *editor=new CustomLineEdit(parent);

    // add a validator for numbers in column 1
    if (index.column() == 1) editor->setValidator(new QDoubleValidator(editor));

    return editor;
}

void LineEditDelegate::setEditorData (QWidget *editor, const QModelIndex &index) const
{
    QString value=index.model()->data(index,Qt::EditRole).toString();
    CustomLineEdit *lineEdit=qobject_cast<CustomLineEdit *>(editor);
    if (lineEdit) lineEdit->setText(value);
}

void LineEditDelegate::setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    CustomLineEdit *lineEdit=qobject_cast<CustomLineEdit *>(editor);
    if (lineEdit) model->setData(index,lineEdit->text());
}

void LineEditDelegate::commitModelData ()
{
    CustomLineEdit *editor=qobject_cast<CustomLineEdit*>(sender());
    if (editor) emit commitData(editor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// KeywordValueItem
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

KeywordValueItem::KeywordValueItem (const QVector<QVariant> &data, KeywordValueItem *parent)
    : m_itemData(data), m_parentItem(parent)
{}

KeywordValueItem::~KeywordValueItem()
{
    qDeleteAll(m_childItems);
}

void KeywordValueItem::appendChild (KeywordValueItem *item)
{
    m_childItems.append(item);
}

KeywordValueItem *KeywordValueItem::child (int row)
{
    if (row < 0 || row >= m_childItems.size())
        return nullptr;
    return m_childItems.at(row);
}

int KeywordValueItem::childCount () const
{
    return m_childItems.count();
}

int KeywordValueItem::row () const
{
    if (m_parentItem)
        return m_parentItem->get_m_childItems()->indexOf(const_cast<KeywordValueItem*>(this));

    return 0;
}

int KeywordValueItem::columnCount () const
{
    return m_itemData.count();
}

QVariant KeywordValueItem::data (int column) const
{
    if (column < 0 || column >= m_itemData.size())
        return QVariant();
    return m_itemData.at(column);
}

KeywordValueItem* KeywordValueItem::parentItem ()
{
    return m_parentItem;
}

bool KeywordValueItem::setData (int column, const QVariant &value)
{
    if (column < 0 || column >= m_itemData.size()) return false;
    m_itemData[column]=value;
    return true;
}

void KeywordValueItem::print ()
{
    int i=0;
    while (i < columnCount()) {
        cout << data(i).toString().toStdString() << " -> ";
        i++;
    }
    cout << endl;

    i=0;
    while (i < childCount()) {
        m_childItems[i]->print();
        i++;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MaterialsModel
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MaterialsModel::MaterialsModel(const QString &data, QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new KeywordValueItem({tr("Title"), tr("Summary")});
}

MaterialsModel::MaterialsModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new KeywordValueItem({tr("Keyword"), tr("Value"), tr("Unit")});
}

MaterialsModel::~MaterialsModel()
{
    delete rootItem;
}

QModelIndex MaterialsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    KeywordValueItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<KeywordValueItem*>(parent.internalPointer());

    KeywordValueItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex MaterialsModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    KeywordValueItem *childItem = static_cast<KeywordValueItem*>(index.internalPointer());
    KeywordValueItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int MaterialsModel::rowCount(const QModelIndex &parent) const
{
    KeywordValueItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<KeywordValueItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int MaterialsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<KeywordValueItem*>(parent.internalPointer())->columnCount();
    return rootItem->columnCount();
}

QVariant MaterialsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        KeywordValueItem *item = static_cast<KeywordValueItem*>(index.internalPointer());
        return item->data(index.column());
    }

    return QVariant();
}

Qt::ItemFlags MaterialsModel::flags(const QModelIndex &index) const
{
    QModelIndex parentIndex=parent(index);
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    if (parentIndex.isValid()) {
        if (index.isValid() && index.column() == 1) {
           return defaultFlags | Qt::ItemIsEditable;
        }
    } else {
        if (index.isValid() && index.column() == 0) {
            return defaultFlags | Qt::ItemIsEditable;
        }
    }

    return defaultFlags;
}

KeywordValueItem* MaterialsModel::getItem (const QModelIndex &index) const
{
    if (index.isValid()) {
        if (auto *item = static_cast<KeywordValueItem*>(index.internalPointer()))
            return item;
    }
    return rootItem;
}

QVariant MaterialsModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

bool MaterialsModel::setData (const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    KeywordValueItem *item=getItem(index);
    bool result=item->setData(index.column(),value);

    if (result)
        emit dataChanged(index,index,{Qt::DisplayRole, Qt::EditRole});

    return result;
}

void MaterialsModel::populate (MaterialDatabase *materialDatabase, KeywordValueItem *parent)
{
    long unsigned int i=0;
    while (i < materialDatabase->get_size()) {
        Material *material=materialDatabase->get_material(i);

        QList<QVariant> data;
        QVariant name=QString::fromStdString(material->get_name()->get_value());
        data.append(name);

        name="";
        data.append(name);  // to force a second column
        data.append(name);  // to force a third column

        KeywordValueItem *materialItem=new KeywordValueItem(data,parent);
        parent->appendChild(materialItem);

        long unsigned int j=0;
        while (j < material->get_temperatureList_size()) {
            Temperature *temperature=material->get_temperature(j);

            QList<QVariant> data;
            QVariant name="Temperature";
            data.append(name);

            QVariant value;
            QVariant unit="";
            if (temperature->get_temperature()->get_value().compare("any") == 0) {
                value="any";
            } else {
                value=temperature->get_temperature()->get_dbl_value();
                unit="C";
            }
            data.append(value);
            data.append(unit);

            KeywordValueItem *temperatureItem=new KeywordValueItem(data,materialItem);
            materialItem->appendChild(temperatureItem);


            if (temperature->get_frequencyList_size() == 0) {

                // Debye model

                QList<QVariant> data;
                QVariant name="Debye Model";
                data.append(name);

                name="";
                data.append(name);  // to force a second column
                data.append(name);  // to force a third column

                KeywordValueItem *debyeItem=new KeywordValueItem(data,temperatureItem);
                temperatureItem->appendChild(debyeItem);

                // er_infinity

                data.clear();

                QVariant keyword=QString::fromStdString(temperature->get_er_infinity().get_keyword());
                data.append(keyword);

                QVariant value=temperature->get_er_infinity().get_dbl_value();
                data.append(value);

                KeywordValueItem *pairItem=new KeywordValueItem(data,debyeItem);
                debyeItem->appendChild(pairItem);

                // delta_er

                data.clear();

                keyword=QString::fromStdString(temperature->get_delta_er().get_keyword());
                data.append(keyword);

                value=temperature->get_delta_er().get_dbl_value();
                data.append(value);

                pairItem=new KeywordValueItem(data,debyeItem);
                debyeItem->appendChild(pairItem);

                // m1

                data.clear();

                keyword=QString::fromStdString(temperature->get_m1().get_keyword());
                data.append(keyword);

                value=temperature->get_m1().get_dbl_value();
                data.append(value);

                pairItem=new KeywordValueItem(data,debyeItem);
                debyeItem->appendChild(pairItem);

                // m2

                data.clear();

                keyword=QString::fromStdString(temperature->get_m2().get_keyword());
                data.append(keyword);

                value=temperature->get_m2().get_dbl_value();
                data.append(value);

                pairItem=new KeywordValueItem(data,debyeItem);
                debyeItem->appendChild(pairItem);

                // relative_permeability

                data.clear();

                keyword=QString::fromStdString(temperature->get_relative_permeability().get_keyword());
                data.append(keyword);

                value=temperature->get_relative_permeability().get_dbl_value();
                data.append(value);

                pairItem=new KeywordValueItem(data,debyeItem);
                debyeItem->appendChild(pairItem);

                // loss

                data.clear();

                keyword=QString::fromStdString(temperature->get_loss().get_keyword());
                data.append(keyword);

                value=temperature->get_loss().get_dbl_value();
                data.append(value);

                unit="S/m";
                data.append(unit);

                pairItem=new KeywordValueItem(data,debyeItem);
                debyeItem->appendChild(pairItem);

            } else {
                long unsigned int k=0;
                while (k < temperature->get_frequencyList_size()) {
                    Frequency *frequency=temperature->get_frequency(k);

                    QList<QVariant> data;
                    QVariant name="Frequency";
                    data.append(name);

                    QVariant value;
                    QVariant unit="";
                    if (frequency->get_frequency()->get_value().compare("any") == 0) {
                        value="any";
                    } else {
                        value=frequency->get_frequency()->get_dbl_value();
                        unit="Hz";
                    }
                    data.append(value);
                    data.append(unit);

                    KeywordValueItem *frequencyItem=new KeywordValueItem(data,temperatureItem);
                    temperatureItem->appendChild(frequencyItem);

                    // relative_permittivity

                    data.clear();

                    QVariant keyword=QString::fromStdString(frequency->get_relative_permittivity()->get_keyword());
                    data.append(keyword);

                    value=frequency->get_relative_permittivity()->get_dbl_value();
                    data.append(value);

                    KeywordValueItem *pairItem=new KeywordValueItem(data,frequencyItem);
                    frequencyItem->appendChild(pairItem);

                    // relative_permeability

                    data.clear();

                    keyword=QString::fromStdString(frequency->get_relative_permeability()->get_keyword());
                    data.append(keyword);

                    value=frequency->get_relative_permeability()->get_dbl_value();
                    data.append(value);

                    pairItem=new KeywordValueItem(data,frequencyItem);
                    frequencyItem->appendChild(pairItem);

                    // loss

                    data.clear();

                    keyword=QString::fromStdString(frequency->get_loss()->get_keyword());
                    data.append(keyword);

                    value=frequency->get_loss()->get_dbl_value();
                    data.append(value);

                    unit="S/m";
                    data.append(unit);

                    pairItem=new KeywordValueItem(data,frequencyItem);
                    frequencyItem->appendChild(pairItem);

                    // Rz

                    data.clear();

                    keyword=QString::fromStdString(frequency->get_Rz()->get_keyword());
                    data.append(keyword);

                    value=frequency->get_Rz()->get_dbl_value();
                    data.append(value);

                    unit="m";
                    data.append(unit);

                    pairItem=new KeywordValueItem(data,frequencyItem);
                    frequencyItem->appendChild(pairItem);

                    k++;
                }

            }

            j++;
        }

        // sources

        data.clear();
        name="Source";
        data.append(name);

        KeywordValueItem *sourceItem=new KeywordValueItem(data,materialItem);
        materialItem->appendChild(sourceItem);

        j=0;
        while (j < material->get_sourceList_size()) {

            vector<string> lineList=material->get_source_lineList(j);

            long unsigned int k=0;
            while (k < lineList.size()) {

                QList<QVariant> data;
                QVariant text=QString::fromStdString(lineList[k]);
                data.append(text);

                KeywordValueItem *pairItem=new KeywordValueItem(data,sourceItem);
                sourceItem->appendChild(pairItem);

                k++;
            }

            j++;
        }

        i++;
    }
}

void MaterialsModel::materialsModel_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    cout << "data changed" << endl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Materials
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Materials::Materials(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Materials)
{
    ui->setupUi(this);

    //ui->materialsTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    //ui->materialsTreeView->setTextElideMode(Qt::ElideNone);

    LineEditDelegate *delegate = new LineEditDelegate(ui->materialsTreeView);
    ui->materialsTreeView->setItemDelegate(delegate);

    ui->materialsTreeView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->materialsTreeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->materialsTreeView->header()->setStretchLastSection(false);

    // signals
    //connect(ui->materialsTree, &QTreeWidget::itemClicked, this, &Materials::materialItemClicked);

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

}

Materials::~Materials()
{
    delete materialsModel;
    delete ui;
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

        if (materialDatabase.load_materials(nullstring,filename,nullstring,nullstring,checkLimits)) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "Failed to load materials file.");
            mb.setFixedSize(500, 200);
            return;
        }
        if (filename) free(filename);

        QModelIndex parentIndex=QModelIndex();
        materialsModel=new MaterialsModel();
        materialsModel->populate(&materialDatabase,materialsModel->get_rootItem());
        connect(materialsModel, &QAbstractItemModel::dataChanged, materialsModel, &MaterialsModel::materialsModel_dataChanged);
        //materialsModel->print();

        ui->materialsTreeView->setModel(materialsModel);
        ui->materialsTreeView->resizeColumnToContents(0);
        ui->materialsTreeView->show();

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

