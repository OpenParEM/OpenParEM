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

#include "Materials.h"
#include "ui_Materials.h"
#include <iostream>
#include "CustomLineEdit.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LineEditDelegate
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QWidget* LineEditDelegate::createEditor (QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CustomLineEdit *editor=new CustomLineEdit(parent);
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
    if (lineEdit) {
        model->setData(index,lineEdit->text());
        MaterialsModel *materialsModel=qobject_cast<MaterialsModel *>(model);
        materialsModel->signalSelection();
    }
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

KeywordValueItem* KeywordValueItem::child (int row)
{
    if (row < 0 || row >= m_childItems.size())
        return nullptr;
    return m_childItems.at(row);
}

int KeywordValueItem::childCount () const
{
    return m_childItems.count();
}

void KeywordValueItem::insertChild(QModelIndex index, int row, MaterialsModel *materialsModel)
{
    if (materialsModel->columnCount(index) == 0) {
        if (!materialsModel->insertColumn(0,index)) return;
    }

    if (!materialsModel->insertRow(row,index)) return;

    // insert the keywordValue pair
    QModelIndex child;
    int column=0;
    while (column < materialsModel->columnCount(index)) {
        child=materialsModel->index(row,column,index);
        materialsModel->setData(child,data(column),Qt::EditRole);
        column++;
    }

    child=materialsModel->index(row,0,index);

    // set markers
    KeywordValueItem *item=materialsModel->getItem(child);
    item->level=level;
    item->copyAllowed=copyAllowed;

    // recursively insert children
    int i=0;
    while (i < childCount()) {
        m_childItems[i]->insertChild(child,i,materialsModel);
        i++;
    }
}

bool KeywordValueItem::insertChildren(int position, int count, int columns)
{
    if (position < 0 || position > qsizetype(m_childItems.size()))
        return false;

    for (int row = 0; row < count; ++row) {
        QVector<QVariant> data(columns);
        KeywordValueItem *item = new KeywordValueItem(data, this);
        item->level=0;
        item->copyAllowed=false;
        m_childItems.insert(position, item);
    }

    return true;
}

bool KeywordValueItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > qsizetype(m_childItems.size()))
        return false;

    for (int row = 0; row < count; ++row)
        m_childItems.erase(m_childItems.cbegin() + position);

    return true;
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

KeywordValueItem* KeywordValueItem::copy()
{
    QList<QVariant> copy_m_itemData;

    // data
    int i=0;
    while (i < m_itemData.size()) {
        copy_m_itemData.append(m_itemData[i]);
        i++;
    }

    // create
    KeywordValueItem *item=new KeywordValueItem(copy_m_itemData,parentItem());
    item->level=level;
    item->copyAllowed=copyAllowed;

    // children
    i=0;
    while (i < m_childItems.size()) {
        KeywordValueItem *child=m_childItems[i]->copy();
        item->appendChild(child);
        i++;
    }

    return item;
}

void KeywordValueItem::print ()
{
    int i=0;
    while (i < columnCount()) {
        std::cout  << data(i).toString().toStdString() << " ";
        i++;
    }
    std::cout  << std::endl;
    std::cout  << "   level=" << level << std::endl;
    std::cout  << "   copyAllowed=" << copyAllowed << std::endl;

    i=0;
    while (i < childCount()) {
        m_childItems[i]->print();
        i++;
    }
}

void KeywordValueItem::print (QTextStream *fileOut, KeywordValueItem *rootItem)
{
    if (data(0) == "Temperature") {
        *fileOut << "   Temperature" << "\n";
        *fileOut << "      temperature=" << data(1).toString() << "\n";
    } else if (data(0) == "Frequency") {
        *fileOut << "      Frequency" << "\n";
        *fileOut << "         frequency=" << data(1).toString() << "\n";
    } else if (data(0) == "Source") {
        *fileOut << "   Source" << "\n";
    } else if (data(0) == "RootItem") {
        // root item
    } else {
        if (m_parentItem != rootItem) {
            if (data(0) != "Debye Model") {
                *fileOut << "         " << data(0).toString();
                if (data(1) != "") *fileOut << "=" << data(1).toString();
                *fileOut << "\n";
            }
        }
    }

    int i=0;
    while (i < m_childItems.size()) {
        KeywordValueItem *child=m_childItems[i];

        if (child->m_parentItem == rootItem) {
            *fileOut << "Material" << "\n";
            *fileOut << "   name=" << child->data(0).toString() << "\n";
        }

        child->print(fileOut,rootItem);

        if (child->m_parentItem == rootItem) {
            *fileOut << "EndMaterial" << "\n";
            *fileOut << "\n";
        }

        i++;
    }

    if (data(0) == "Temperature") {
        *fileOut << "   EndTemperature" << "\n";
    } else if (data(0) == "Frequency") {
        *fileOut << "      EndFrequency" << "\n";
    } else if (data(0) == "Source") {
        *fileOut << "   EndSource" << "\n";
    }
}

bool KeywordValueItem::hasAny(KeywordValueItem *testChild)
{
    int i=0;
    while (i < childCount()) {
        KeywordValueItem *child=m_childItems[i];
        if (child->level == testChild->level && child->isAny()) return true;
        i++;
    }
    return false;
}

bool KeywordValueItem::hasLevel (int level)
{
    int i=0;
    while (i < childCount()) {
        KeywordValueItem *child=m_childItems[i];
        if (child->level == level) return true;
        i++;
    }
    return false;
}

bool KeywordValueItem::hasItem (KeywordValueItem *testChild)
{
    int i=0;
    while (i < childCount()) {
        KeywordValueItem *child=m_childItems[i];
        if (child->level == testChild->level) return true;
        i++;
    }
    return false;
}

int KeywordValueItem::lastRow (KeywordValueItem *item)
{
    int last=0;
    int i=0;
    while (i < childCount()) {
        KeywordValueItem *child=m_childItems[i];
        if (child->level == item->level) last=i;
        i++;
    }
    return last;
}

bool KeywordValueItem::hasOne (KeywordValueItem *item)
{
    int count=0;
    int i=0;
    while (i < childCount()) {
        KeywordValueItem *child=m_childItems[i];
        if (child->level == item->level) count++;
        i++;
    }
    if (count == 1) return true;
    return false;
}

QVariant KeywordValueItem::hasDuplicateKeyword ()
{
    int i=0;
    while (i < childCount()-1) {
        int j=i+1;
        while (j < childCount()) {
            if (child(i)->data(0) == child(j)->data(0)) return child(i)->data(0);
            j++;
        }
        i++;
    }
    return QVariant();
}

QVariant KeywordValueItem::hasDuplicateValue (int testLevel)
{
    int i=0;
    while (i < childCount()-1) {
        int j=i+1;
        while (j < childCount()) {
            if (child(i)->get_level() == testLevel && child(j)->get_level() == testLevel) {
                if (child(i)->data(1) == "any") {
                    if (child(j)->data(1) == "any" ) return child(i)->data(1);
                } else {
                    if (child(j)->data(1) == "any" ) {
                        return child(j)->data(1);
                    } else {
                        if (double_compare(child(i)->data(1).toDouble(),
                                           child(j)->data(1).toDouble(),1e-12)) return child(i)->data(1);
                    }
                }
            }
            j++;
        }
        i++;
    }
    return QVariant();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MaterialsModel
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MaterialsModel::MaterialsModel(const QString &data, QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem=new KeywordValueItem({tr("RootItem"), tr(""), tr("")});
    rootItem->set_level(0);
    rootItem->set_copyAllowed(false);
    dataHasChanged=false;
}

MaterialsModel::MaterialsModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem=new KeywordValueItem({tr("RootItem"), tr(""), tr("")});
    rootItem->set_level(0);
    rootItem->set_copyAllowed(false);
    dataHasChanged=false;
}

MaterialsModel::~MaterialsModel()
{
    if (rootItem) {delete rootItem; rootItem=nullptr;}
}

void MaterialsModel::signalSelection()
{
    materials->signalSelection();
}

QModelIndex MaterialsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();

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
    if (!index.isValid()) return QModelIndex();

    KeywordValueItem *childItem = static_cast<KeywordValueItem*>(index.internalPointer());
    KeywordValueItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem) return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int MaterialsModel::rowCount(const QModelIndex &parent) const
{
    KeywordValueItem *parentItem;
    if (parent.column() > 0) return 0;

    if (!parent.isValid()) parentItem = rootItem;
    else parentItem = static_cast<KeywordValueItem*>(parent.internalPointer());

    return parentItem->childCount();
}

bool MaterialsModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    KeywordValueItem *parentItem = getItem(parent);
    if (!parentItem) return false;

    beginInsertRows(parent, position, position + rows - 1);
        const bool success = parentItem->insertChildren(position,rows,rootItem->columnCount());
    endInsertRows();

    return success;
}

bool MaterialsModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    KeywordValueItem *parentItem = getItem(parent);
    if (!parentItem) return false;

    beginRemoveRows(parent, position, position + rows - 1);
    const bool success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
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

    // make an exception for Source text
    if (index.isValid()) {
        if (getItem(index)->get_level() == 7) {
            return defaultFlags | Qt::ItemIsEditable;
        }
    }

    return defaultFlags;
}

KeywordValueItem* MaterialsModel::getItem (const QModelIndex &index) const
{
    if (index.isValid()) {
        if (auto *item = static_cast<KeywordValueItem*>(index.internalPointer())) return item;
    }
    return rootItem;
}

QVariant MaterialsModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        QVariant header;
        if (section == 0) header="Keyword";
        if (section == 1) header="Value";
        if (section == 2) header="Unit";
        return header;
    }

    return QVariant();
}

bool MaterialsModel::setData (const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) return false;

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
        materialItem->set_level(1);
        materialItem->set_copyAllowed(true);

        parent->appendChild(materialItem);

        long unsigned int j=0;
        while (j < material->get_temperatureList_size()) {
            Temperature *temperature=material->get_temperature(j);

            QList<QVariant> data;
            QVariant name="Temperature";

            QVariant value;
            QVariant unit="";
            if (temperature->get_temperature()->get_value().compare("any") == 0) {
                value="any";
            } else {
                value=temperature->get_temperature()->get_dbl_value();
                unit="C";
            }
            data.append(name);
            data.append(value);
            data.append(unit);

            KeywordValueItem *temperatureItem=new KeywordValueItem(data,materialItem);
            temperatureItem->set_level(2);
            temperatureItem->set_copyAllowed(true);
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
                debyeItem->set_level(5);
                debyeItem->set_copyAllowed(false);
                temperatureItem->appendChild(debyeItem);

                // er_infinity

                data.clear();

                QVariant keyword=QString::fromStdString(temperature->get_er_infinity().get_keyword());
                value=temperature->get_er_infinity().get_dbl_value();
                unit="";
                data.append(keyword);
                data.append(value);
                data.append(unit);

                KeywordValueItem *pairItem=new KeywordValueItem(data,debyeItem);
                pairItem->set_noCopy();
                debyeItem->appendChild(pairItem);

                // delta_er

                data.clear();

                keyword=QString::fromStdString(temperature->get_delta_er().get_keyword());
                value=temperature->get_delta_er().get_dbl_value();
                unit="";
                data.append(keyword);
                data.append(value);
                data.append(unit);

                pairItem=new KeywordValueItem(data,debyeItem);
                pairItem->set_noCopy();
                debyeItem->appendChild(pairItem);

                // m1

                data.clear();

                keyword=QString::fromStdString(temperature->get_m1().get_keyword());
                value=temperature->get_m1().get_dbl_value();
                unit="";
                data.append(keyword);
                data.append(value);
                data.append(unit);

                pairItem=new KeywordValueItem(data,debyeItem);
                pairItem->set_noCopy();
                debyeItem->appendChild(pairItem);

                // m2

                data.clear();

                keyword=QString::fromStdString(temperature->get_m2().get_keyword());
                value=temperature->get_m2().get_dbl_value();
                unit="";
                data.append(keyword);
                data.append(value);
                data.append(unit);

                pairItem=new KeywordValueItem(data,debyeItem);
                pairItem->set_noCopy();
                debyeItem->appendChild(pairItem);

                // relative_permeability

                data.clear();

                keyword=QString::fromStdString(temperature->get_relative_permeability().get_keyword());
                value=temperature->get_relative_permeability().get_dbl_value();
                unit="";
                data.append(keyword);
                data.append(value);
                data.append(unit);

                pairItem=new KeywordValueItem(data,debyeItem);
                pairItem->set_noCopy();
                debyeItem->appendChild(pairItem);

                // loss

                data.clear();

                keyword=QString::fromStdString(temperature->get_loss().get_keyword());
                value=temperature->get_loss().get_dbl_value();
                unit="S/m";
                data.append(keyword);
                data.append(value);
                data.append(unit);

                pairItem=new KeywordValueItem(data,debyeItem);
                pairItem->set_noCopy();
                debyeItem->appendChild(pairItem);

            } else {
                long unsigned int k=0;
                while (k < temperature->get_frequencyList_size()) {
                    Frequency *frequency=temperature->get_frequency(k);

                    QList<QVariant> data;
                    QVariant name="Frequency";

                    QVariant value;
                    QVariant unit="";
                    if (frequency->get_frequency()->get_value().compare("any") == 0) {
                        value="any";
                    } else {
                        value=frequency->get_frequency()->get_dbl_value();
                        unit="Hz";
                    }
                    data.append(name);
                    data.append(value);
                    data.append(unit);

                    KeywordValueItem *frequencyItem=new KeywordValueItem(data,temperatureItem);
                    frequencyItem->set_level(4);
                    frequencyItem->set_copyAllowed(true);
                    temperatureItem->appendChild(frequencyItem);

                    // relative_permittivity

                    data.clear();

                    QVariant keyword=QString::fromStdString(frequency->get_relative_permittivity()->get_keyword());
                    value=frequency->get_relative_permittivity()->get_dbl_value();
                    unit="";
                    data.append(keyword);
                    data.append(value);
                    data.append(unit);

                    KeywordValueItem *pairItem=new KeywordValueItem(data,frequencyItem);
                    pairItem->set_noCopy();
                    frequencyItem->appendChild(pairItem);

                    // relative_permeability

                    data.clear();

                    keyword=QString::fromStdString(frequency->get_relative_permeability()->get_keyword());
                    value=frequency->get_relative_permeability()->get_dbl_value();
                    unit="";
                    data.append(keyword);
                    data.append(value);
                    data.append(unit);

                    pairItem=new KeywordValueItem(data,frequencyItem);
                    pairItem->set_noCopy();
                    frequencyItem->appendChild(pairItem);

                    // loss

                    data.clear();

                    keyword=QString::fromStdString(frequency->get_loss()->get_keyword());
                    value=frequency->get_loss()->get_dbl_value();
                    unit="S/m";
                    data.append(keyword);
                    data.append(value);
                    data.append(unit);

                    pairItem=new KeywordValueItem(data,frequencyItem);
                    pairItem->set_noCopy();
                    frequencyItem->appendChild(pairItem);

                    // Rz

                    data.clear();

                    keyword=QString::fromStdString(frequency->get_Rz()->get_keyword());
                    value=frequency->get_Rz()->get_dbl_value();
                    unit="m";
                    data.append(keyword);
                    data.append(value);
                    data.append(unit);

                    pairItem=new KeywordValueItem(data,frequencyItem);
                    pairItem->set_noCopy();
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
        sourceItem->set_level(3);
        sourceItem->set_copyAllowed(true);
        materialItem->appendChild(sourceItem);

        j=0;
        while (j < material->get_sourceList_size()) {

            std::vector<std::string> lineList=material->get_source_lineList(j);

            long unsigned int k=0;
            while (k < lineList.size()) {

                QList<QVariant> data;
                QVariant text=QString::fromStdString(lineList[k]);
                data.append(text);
                text="";
                data.append(text);
                data.append(text);

                KeywordValueItem *pairItem=new KeywordValueItem(data,sourceItem);
                pairItem->set_level(7);
                pairItem->set_copyAllowed(true);
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
    dataHasChanged=true;
    materials->materials_edited();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Materials
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Materials::Materials (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Materials)
{
    ui->setupUi(this);
    itemCopy=nullptr;
    materialsModel=nullptr;

    // cell editing delegate
    LineEditDelegate *delegate = new LineEditDelegate(ui->materialsTreeView);
    ui->materialsTreeView->setItemDelegate(delegate);

    // selection
    ui->materialsTreeView->setSelectionMode(QAbstractItemView::SingleSelection);

    // window options
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);  // in QtCreator, right click the main window -> Layout -> Layout Vertically
    ui->materialsTreeView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->materialsTreeView->header()->setStretchLastSection(true);
    ui->materialsTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->materialsTreeView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    ui->materialsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->materialsTreeView, &QTreeView::customContextMenuRequested, this, &Materials::contextMenu_triggered);

    // menu bar

    QMenuBar *menuBar = new QMenuBar(nullptr);

    // File

    QMenu *fileMenu = new QMenu("File", menuBar);

    fileNew=new QAction("New",fileMenu);
    fileNew->setShortcut(QKeySequence::New);
    fileMenu->addAction(fileNew);
    connect(fileNew, &QAction::triggered, this, &Materials::newAction_triggered);

    fileOpen=new QAction("Open ...",fileMenu);
    fileOpen->setShortcut(QKeySequence::Open);
    fileMenu->addAction(fileOpen);
    connect(fileOpen, &QAction::triggered, this, &Materials::openAction_triggered);

    fileSave=new QAction("Save",fileMenu);
    fileSave->setShortcut(QKeySequence::Save);
    fileSave->setEnabled(false);
    fileMenu->addAction(fileSave);
    connect(fileSave, &QAction::triggered, this, &Materials::saveAction_triggered);

    fileSaveAs=new QAction("Save As ...",fileMenu);
    fileSaveAs->setShortcut(QKeySequence::SaveAs);
    fileSaveAs->setEnabled(false);
    fileMenu->addAction(fileSaveAs);
    connect(fileSaveAs, &QAction::triggered, this, &Materials::saveAsAction_triggered);

    fileClose=new QAction("Close",fileMenu);
    fileClose->setShortcut(QKeySequence::Close);
    fileMenu->addAction(fileClose);
    connect(fileClose, &QAction::triggered, this, &Materials::closeAction_triggered);

    menuBar->addMenu(fileMenu);

    // Edit

    QMenu *editMenu = new QMenu("Edit", menuBar);

    editCopy=new QAction("Copy",fileMenu);
    editCopy->setShortcut(QKeySequence::Copy);
    editCopy->setEnabled(false);
    editMenu->addAction(editCopy);
    connect(editCopy, &QAction::triggered, this, &Materials::copyData);

    editPaste=new QAction("Paste",fileMenu);
    editPaste->setShortcut(QKeySequence::Paste);
    editPaste->setEnabled(false);
    editMenu->addAction(editPaste);
    connect(editPaste, &QAction::triggered, this, &Materials::pasteData);

    QKeySequence Append(Qt::CTRL | Qt::Key_A);
    editAppend=new QAction("Append",fileMenu);
    editAppend->setShortcut(Append);
    editAppend->setEnabled(false);
    editMenu->addAction(editAppend);
    connect(editAppend, &QAction::triggered, this, &Materials::appendData);

    QKeySequence Convert(Qt::CTRL | Qt::Key_R);
    editConvert=new QAction("Set to \"any\"",fileMenu);
    editConvert->setShortcut(Convert);
    editConvert->setEnabled(false);
    editMenu->addAction(editConvert);
    connect(editConvert, &QAction::triggered, this, &Materials::convertData);

    QKeySequence InsertNew(Qt::CTRL | Qt::Key_M);
    editNew=new QAction("Insert New",fileMenu);
    editNew->setShortcut(InsertNew);
    editNew->setEnabled(false);
    editMenu->addAction(editNew);
    connect(editNew, &QAction::triggered, this, &Materials::insertNewData);

    editDelete=new QAction("Delete",fileMenu);
    editDelete->setShortcut(QKeySequence::Delete);
    editDelete->setEnabled(false);
    editMenu->addAction(editDelete);
    connect(editDelete, &QAction::triggered, this, &Materials::deleteData);

    menuBar->addMenu(editMenu);
    ui->materialsMenuBar->addWidget(menuBar);
}

Materials::~Materials ()
{
    if (materialsModel) {delete materialsModel; materialsModel=nullptr;}
    if (itemCopy) {delete itemCopy; itemCopy=nullptr;}
    delete ui;
}

void Materials::keyPressEvent (QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        event->ignore();
    } else {
        QDialog::keyPressEvent(event);
    }
}

void Materials::reset (bool clearMaterials)
{
    materialsFile="";
    if (clearMaterials) materialDatabase.clear();
    if (materialsModel) {delete materialsModel; materialsModel=nullptr;}
    if (itemCopy) {delete itemCopy; itemCopy=nullptr;}

    editCopy->setEnabled(false);
    editPaste->setEnabled(false);
    editAppend->setEnabled(false);
    editConvert->setEnabled(false);
    editNew->setEnabled(false);
    editDelete->setEnabled(false);

    fileSave->setEnabled(false);
    fileSaveAs->setEnabled(false);
}

void Materials::signalSelection ()
{
    QItemSelectionModel* selectionModel=ui->materialsTreeView->selectionModel();
    QItemSelection from=selectionModel->selection();
    emit selectionModel->selectionChanged(from,from);
}

void Materials::copyData()
{
    QModelIndexList selectedIndices=ui->materialsTreeView->selectionModel()->selectedIndexes();
    if (selectedIndices.size() == 0) return;

    // just do for the first column
    QModelIndex index=selectedIndices[0];
    if (!index.isValid()) return;
    if (!materialsModel->getItem(index)->canCopy()) return;

    KeywordValueItem *item=materialsModel->getItem(index)->copy();

    if (itemCopy) delete itemCopy;
    itemCopy=item;

    signalSelection();
}

void Materials::pasteData ()
{
    if (!itemCopy) return;

    const QModelIndex index = ui->materialsTreeView->selectionModel()->currentIndex();
    QModelIndex parent=index.parent();
    itemCopy->insertChild(parent,index.row()+1,materialsModel);

    signalSelection();
}

void Materials::appendData ()
{
    if (!itemCopy) return;

    const QModelIndex rawIndex=ui->materialsTreeView->selectionModel()->currentIndex();
    QModelIndex index=rawIndex.siblingAtColumn(0);
    itemCopy->insertChild(index,materialsModel->getItem(index)->lastRow(itemCopy)+1,materialsModel);

    signalSelection();
}

void Materials::insertNewData ()
{
    // location to put the new material
    QModelIndexList selectedIndices=ui->materialsTreeView->selectionModel()->selectedIndexes();
    if (selectedIndices.size() == 0) return;
    QModelIndex index=selectedIndices[0];
    if (!index.isValid()) return;

    // three new materials: dielectric frequency list, dielectric Debye, and metal

    MaterialDatabase tempMaterialDatabase;

    Material *material=new Material(0,0);
    material->set_freespace();
    tempMaterialDatabase.push(material);

    material=new Material(0,0);
    material->set_FR4();
    tempMaterialDatabase.push(material);

    material=new Material(0,0);
    material->set_copper();
    tempMaterialDatabase.push(material);

    // convert to MaterialsModel
    QModelIndex parentIndex=QModelIndex();
    MaterialsModel tempMaterialsModel;
    tempMaterialsModel.setMaterials(this);
    tempMaterialsModel.populate(&tempMaterialDatabase,tempMaterialsModel.get_rootItem());

    // get the KeywordValueItems and save to the model

    QModelIndex newIndex=tempMaterialsModel.index(0,0,parentIndex);
    KeywordValueItem *item=tempMaterialsModel.getItem(newIndex);
    QModelIndex parent=index.parent();
    item->insertChild(parent,index.row()+1,materialsModel);

    newIndex=tempMaterialsModel.index(1,0,parentIndex);
    item=tempMaterialsModel.getItem(newIndex);
    parent=index.parent();
    item->insertChild(parent,index.row()+2,materialsModel);

    newIndex=tempMaterialsModel.index(2,0,parentIndex);
    item=tempMaterialsModel.getItem(newIndex);
    parent=index.parent();
    item->insertChild(parent,index.row()+3,materialsModel);

    // file options
    fileSave->setEnabled(true);
    fileSaveAs->setEnabled(true);

    signalSelection();
}

void Materials::convertData ()
{
    QModelIndexList selectedIndices=ui->materialsTreeView->selectionModel()->selectedIndexes();
    if (selectedIndices.size() == 0) return;
    QModelIndex index=selectedIndices[0];
    if (!index.isValid()) return;

    KeywordValueItem *item=materialsModel->getItem(index);
    if (item->get_level() == 2 || item->get_level() == 4) {
        QVariant data="any";
        item->setData(1,data);
    }

    signalSelection();
}

void Materials::deleteData ()
{
    QModelIndexList selectedIndices=ui->materialsTreeView->selectionModel()->selectedIndexes();

    if (selectedIndices.size() == 0) return;

    // just do for the first column
    QModelIndex index=selectedIndices[0];
    materialsModel->removeRows(index.row(),1,index.parent());

    // clear copy
    if (itemCopy) {delete itemCopy; itemCopy=nullptr;}

    signalSelection();
}

int Materials::check_changed ()
{
    int retVal=0;
    if (materialsModel) {
        if (materialsModel->hasChanged()) {
            QMessageBox msgBox;
            msgBox.setText("The materials have been modified.");
            msgBox.setInformativeText("Do you want to save your changes?");
            msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Save);
            retVal = msgBox.exec();
        }
    }
    return retVal;
}

void Materials::newAction_triggered ()
{
    int retVal=check_changed();
    if (retVal) {
        if (retVal == QMessageBox::Save) {
            saveAction_triggered();
        } else if (retVal == QMessageBox::Discard) {
            // do nothing
        } else if (retVal == QMessageBox::Cancel) {
            return;
        }
    }

    reset(true);

    // two materials - one frequency list and one Debye to have each style to work with

    Material *material=new Material(0,0);
    material->set_freespace();
    materialDatabase.push(material);

    material=new Material(0,0);
    material->set_FR4();
    materialDatabase.push(material);

    material=new Material(0,0);
    material->set_copper();
    materialDatabase.push(material);

    // put them in a MaterialsModel
    QModelIndex parentIndex=QModelIndex();
    materialsModel=new MaterialsModel();
    materialsModel->setMaterials(this);
    materialsModel->populate(&materialDatabase,materialsModel->get_rootItem());
    connect(materialsModel, &QAbstractItemModel::dataChanged, materialsModel, &MaterialsModel::materialsModel_dataChanged);

    // set up QTreeView
    ui->materialsTreeView->setModel(materialsModel);
    connect(ui->materialsTreeView->selectionModel(),&QItemSelectionModel::selectionChanged,this,&Materials::selection_changed);
    ui->materialsTreeView->resizeColumnToContents(0);
    ui->materialsTreeView->show();

    fileSave->setEnabled(false);
    fileSaveAs->setEnabled(true);

    materialsModel->setChanged();
}

void Materials::openAction_triggered ()
{
    int retVal=check_changed();
    if (retVal) {
        if (retVal == QMessageBox::Save) {
            saveAction_triggered();
        } else if (retVal == QMessageBox::Discard) {
            // do nothing
        } else if (retVal == QMessageBox::Cancel) {
            return;
        }
    }

    QString testMaterialsFile=QFileDialog::getOpenFileName(this,tr("Open Materials File"), "/home/briany/OpenParEM", tr("Data Files (*.txt);;All Files (*)"));

    // return if user cancels
    if (testMaterialsFile.isNull()) return;

    // ToDo: tie this into projFile
    bool checkLimits=true;

    char nullstring[1]; nullstring[0]='\0';
    if (QFile::exists(testMaterialsFile)) {

        char *filename;
        filename=(char *) malloc((testMaterialsFile.length()+1)*sizeof(char));
        int i=0;
        while (i < testMaterialsFile.length()) {
            filename[i]=testMaterialsFile.data()[i].toLatin1();
            i++;
        }
        filename[i]='\0';

        materialDatabase.clear();
        if (materialDatabase.load_materials(nullstring,filename,nullstring,nullstring,checkLimits)) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "Failed to load materials file.");
            mb.setFixedSize(500, 200);
            if (filename) free(filename);
            return;
        }
        if (filename) free(filename);

        reset(false);
        materialsFile=testMaterialsFile;

        QModelIndex parentIndex=QModelIndex();
        materialsModel=new MaterialsModel();
        materialsModel->setMaterials(this);
        materialsModel->populate(&materialDatabase,materialsModel->get_rootItem());
        connect(materialsModel, &QAbstractItemModel::dataChanged, materialsModel, &MaterialsModel::materialsModel_dataChanged);

        ui->materialsTreeView->setModel(materialsModel);
        connect(ui->materialsTreeView->selectionModel(),&QItemSelectionModel::selectionChanged,this,&Materials::selection_changed);
        ui->materialsTreeView->resizeColumnToContents(0);
        ui->materialsTreeView->show();

        fileSave->setEnabled(true);
        fileSaveAs->setEnabled(true);

    } else {
        QMessageBox mb;
        mb.critical(nullptr,"Error","File not found.");
        mb.setFixedSize(500,200);

        fileOpen->setEnabled(false);
        return;
    }

    //ui->materialsFrame->show();
    materialsModel->setUnchanged();
}

bool Materials::check_duplicates ()
{
    // check for duplicate material names
    QVariant duplicate=materialsModel->get_rootItem()->hasDuplicateKeyword();
    if (duplicate.isValid()) {
        QMessageBox msgBox;
        QString message="The material \""+duplicate.toString()+"\" is duplicated.";
        msgBox.critical(nullptr,"Error",message);
        msgBox.setFixedSize(500,200);
        return true;
    }

    // check for duplicate temperatures and frequencies
    int i=0;
    while (i < materialsModel->get_rootItem()->childCount()) {

        // temperature
        KeywordValueItem *child_i=materialsModel->get_rootItem()->child(i);
        QVariant duplicate=child_i->hasDuplicateValue(2);
        if (duplicate.isValid()) {
            QMessageBox msgBox;
            QString message="The material \""+child_i->data(0).toString()+"\" has duplicated temperature \""+duplicate.toString()+"\".";
            msgBox.critical(nullptr,"Error",message);
            msgBox.setFixedSize(500,200);
            return true;
        }

        int j=0;
        while (j < child_i->childCount()-1) {

            // frequencies
            KeywordValueItem *child_j=child_i->child(j);
            QVariant duplicate=child_j->hasDuplicateValue(4);
            if (duplicate.isValid()) {
                QMessageBox msgBox;
                QString message="The material \""+child_i->data(0).toString()+
                                "\" has duplicated frequency \""+duplicate.toString()+
                                "\" for temperature \""+child_j->data(1).toString()+
                                "\".";
                msgBox.critical(nullptr,"Error",message);
                msgBox.setFixedSize(500,200);
                return true;
            }

            j++;
        }
        i++;
    }
    return false;
}

void Materials::saveAction_triggered ()
{
    if (check_duplicates()) return;

    if (materialsFile == "") {saveAsAction_triggered(); return;}

    QFile file(materialsFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream *fileOut=new QTextStream(&file);

        QString version=QString::fromStdString(materialDatabase.get_version_name()+" "+materialDatabase.get_version_value());

        *fileOut << version << "\n";
        *fileOut << "\n";

        materialsModel->get_rootItem()->print(fileOut,materialsModel->get_rootItem());

        fileSave->setEnabled(false);
        fileSaveAs->setEnabled(true);
    } else {
        saveAsAction_triggered();
    }

    materialsModel->setUnchanged ();
}

void Materials::saveAsAction_triggered ()
{
    if (check_duplicates()) return;

    QString testMaterialsFile=QFileDialog::getSaveFileName(this,tr("Open Materials File"), "/home/briany/OpenParEM", tr("Data Files (*.txt);;All Files (*)"));

    // return if user cancels
    if (testMaterialsFile.isNull()) return;

    materialsFile=testMaterialsFile;

    QFile file(materialsFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream *fileOut=new QTextStream(&file);

        QString version=QString::fromStdString(materialDatabase.get_version_name()+" "+materialDatabase.get_version_value());

        *fileOut << version << "\n";
        *fileOut << "\n";

        materialsModel->get_rootItem()->print(fileOut,materialsModel->get_rootItem());

        fileSave->setEnabled(false);
    }

    materialsModel->setUnchanged();
}

void Materials::closeAction_triggered ()
{
    close_event=nullptr;
    close();
}

void Materials::closeWindow_triggered()
{
    int retVal=check_changed();
    if (retVal) {
        if (retVal == QMessageBox::Save) {
            saveAction_triggered();
        } else if (retVal == QMessageBox::Discard) {
            // do nothing
        } else if (retVal == QMessageBox::Cancel) {
            if (close_event) close_event->ignore();
            return;
        }
    }
    if (close_event) close_event->accept();
}

void Materials::contextMenu_triggered(const QPoint& pnt)
{
    QMenu menu(this);
    QModelIndex index = ui->materialsTreeView->indexAt(pnt);
    if (!index.isValid()) return;

    menu.addAction(editCopy);
    menu.addAction(editPaste);
    menu.addAction(editAppend);
    menu.addAction(editConvert);
    menu.addAction(editNew);
    menu.addAction(editDelete);;

    menu.exec(ui->materialsTreeView->mapToGlobal(pnt));
}

void Materials::selection_changed (const QItemSelection &selected, const QItemSelection &deselected)
{
    QModelIndexList indexes = selected.indexes();
    if (indexes.size() == 0) return;

    QModelIndex index=indexes[0];
    if (!index.isValid()) return;
    KeywordValueItem *item=materialsModel->getItem(index);

    editDelete->setEnabled(false);
    if (item->canCopy()) editDelete->setEnabled(true);
    if (item->parentItem()->hasOne(item)) editDelete->setEnabled(false);

    editCopy->setEnabled(false);
    if (item->canCopy()) editCopy->setEnabled(true);

    editPaste->setEnabled(false);
    if (itemCopy && itemCopy->is_sameLevel(item)) {
        KeywordValueItem *parent=item->parentItem();

        // check for disallowed "any" combinations
        if (!parent->hasAny(item)) {     // looking for "any"
            if (parent->hasItem(item)) { // match level
                if (!item->isAny()) {
                    editPaste->setEnabled(true);
                }
            } else {
                editPaste->setEnabled(true);
            }
        }
    }

    editAppend->setEnabled(false);
    if (itemCopy) {

        // material appending temperature
        if (item->get_level() == 1 && itemCopy->get_level() == 2) {
            if (!item->hasAny(itemCopy)) {
                if (item->hasItem(itemCopy)) { // match level;
                    if (!itemCopy->isAny()) {
                        editAppend->setEnabled(true);
                    }
                } else {
                    editAppend->setEnabled(true);
                }
            }
        }

        // temperature appending frequency
        if (item->get_level() == 2 && itemCopy->get_level() == 4) {
            if (!item->hasLevel(5) && !item->hasAny(itemCopy)) {
                if (item->hasItem(itemCopy)) { // match level
                    if (!itemCopy->isAny()) {
                        editAppend->setEnabled(true);
                    }
                } else {
                    editAppend->setEnabled(true);
                }
            }
        }

        // temperature appending Debye
        if (item->get_level() == 2 && itemCopy->get_level() == 5) {
            if (!item->hasLevel(4) && !item->hasAny(itemCopy)) {
                if (item->hasItem(itemCopy)) { // match level
                    if (!itemCopy->isAny()) {
                        editAppend->setEnabled(true);
                    }
                } else {
                    editAppend->setEnabled(true);
                }
            }
        }

        // material appending source
        if (item->get_level() == 1 && itemCopy->get_level() == 3) {
            editAppend->setEnabled(true);
        }

        // source appending source text
        if (item->get_level() == 3 && itemCopy->get_level() == 7) {
            editAppend->setEnabled(true);
        }
    }

    editConvert->setEnabled(false);
    if (item->get_level() == 2 || item->get_level() == 4) {
        if (!item->parentItem()->hasAny(item) && item->parentItem()->hasOne(item)) {
            editConvert->setEnabled(true);
        }
    }

    editNew->setEnabled(false);
    if (item->get_level() == 1) editNew->setEnabled(true);
}

void Materials::materials_edited ()
{
    if (materialsFile != "") fileSave->setEnabled(true);
    fileSaveAs->setEnabled(true);
}

