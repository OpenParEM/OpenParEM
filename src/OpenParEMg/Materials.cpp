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
#include <qtimer.h>
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
{
    isModified=false;
}

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
    if (m_itemData[column] != value) {
        m_itemData[column]=value;
        set_isModified(true);
    }
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
    item->type=type;
    item->set_isModified(true);  // copied item is new data

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
    std::cout << std::endl;
    std::cout << "   level=" << level << std::endl;
    std::cout << "   copyAllowed=" << copyAllowed << std::endl;
    std::cout << "   isModified=" << isModified << std::endl;

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
        if (data(0) != "local" && data(0) != "global") {
            if (m_parentItem != rootItem) {
                if (data(0) != "Debye Model") {
                    *fileOut << "         " << data(0).toString();
                    if (data(1) != "") *fileOut << "=" << data(1).toString();
                    *fileOut << "\n";
                }
            }
        }
    }

    int i=0;
    while (i < m_childItems.size()) {
        KeywordValueItem *child=m_childItems[i];

        if (child->m_parentItem == rootItem) {
            *fileOut << "Material" << "\n";
            *fileOut << "   name=" << child->data(0).toString() << "\n";
            *fileOut << "   type=" << child->getType() << "\n";
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
    // must have local and global
    if (childCount() != 2) return QVariant();

    // local vs. local
    int i=0;
    while (i < child(0)->childCount()-1) {
        int j=i+1;
        while (j < child(0)->childCount()) {
            if (child(0)->child(i)->data(0) == child(0)->child(j)->data(0)) return child(0)->child(i)->data(0);
            j++;
        }
        i++;
    }

    // local vs. global
    i=0;
    while (i < child(0)->childCount()) {
        int j=0;
        while (j < child(1)->childCount()) {
            if (child(0)->child(i)->data(0) == child(1)->child(j)->data(0)) return child(0)->child(i)->data(0);
            j++;
        }
        i++;
    }

    // global vs. global
    i=0;
    while (i < child(1)->childCount()-1) {
        int j=i+1;
        while (j < child(1)->childCount()) {
            if (child(1)->child(i)->data(0) == child(1)->child(j)->data(0)) return child(1)->child(i)->data(0);
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

void KeywordValueItem::set_isModified (bool isModified_)
{
    isModified=isModified_;
    if (m_parentItem) m_parentItem->set_isModified(isModified_);
}

bool KeywordValueItem::get_isModified () {return isModified;}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MaterialsModel
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MaterialsModel::MaterialsModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    std::cout << "MaterialsModel::MaterialsModel" << std::endl; std::cout.flush();

    rootItem=new KeywordValueItem({tr("RootItem"), tr(""), tr("")});
    rootItem->set_level(0);
    rootItem->set_copyAllowed(false);

    localItem=new KeywordValueItem({tr("local"), tr(""), tr("")},rootItem);
    localItem->set_level(1);
    localItem->set_copyAllowed(false);
    rootItem->appendChild(localItem);

    globalItem=new KeywordValueItem({tr("global"), tr(""), tr("")},rootItem);
    globalItem->set_level(1);
    globalItem->set_copyAllowed(false);
    rootItem->appendChild(globalItem);

}

MaterialsModel::~MaterialsModel()
{
    if (rootItem) {delete rootItem; rootItem=nullptr;}
}

void MaterialsModel::signalSelection()
{
    materials->signalSelection();
}

QModelIndex MaterialsModel::index (int row, int column, const QModelIndex &parent) const
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

QModelIndex MaterialsModel::parent (const QModelIndex &index) const
{
    if (!index.isValid()) return QModelIndex();

    KeywordValueItem *childItem = static_cast<KeywordValueItem*>(index.internalPointer());
    KeywordValueItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem) return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int MaterialsModel::rowCount (const QModelIndex &parent) const
{
    KeywordValueItem *parentItem;
    if (parent.column() > 0) return 0;

    if (!parent.isValid()) parentItem = rootItem;
    else parentItem = static_cast<KeywordValueItem*>(parent.internalPointer());

    return parentItem->childCount();
}

bool MaterialsModel::insertRows (int position, int rows, const QModelIndex &parent)
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
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    if (index.isValid()) {
        if (index.column() == 0) {
            // material names are editable
            if (getItem(index)->get_level() == 2) return defaultFlags | Qt::ItemIsEditable;
        } else if (index.column() == 1) {
            // property values are editable
            return defaultFlags | Qt::ItemIsEditable;
        }

        // source text is editable
        if (getItem(index)->get_level() == 8) {
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
    //std::cout << "MaterialsModel::setData" << std::endl; std::cout.flush();

    if (role != Qt::EditRole) return false;

    KeywordValueItem *item=getItem(index);
    bool result=item->setData(index.column(),value);

    if (result)
        emit dataChanged(index,index,{Qt::DisplayRole, Qt::EditRole});

    return result;
}

void MaterialsModel::populate (MaterialDatabase *materialDatabase)
{
    std::cout << "MaterialsModel::populate" << std::endl; std::cout.flush();

    if (!materialDatabase) return;

    long unsigned int i=0;
    while (i < materialDatabase->get_size()) {
        Material *material=materialDatabase->get_material(i);

        QList<QVariant> data;
        QVariant name=QString::fromStdString(material->get_name()->get_value());
        data.append(name);

        name="";
        data.append(name);  // to force a second column
        data.append(name);  // to force a third column

        KeywordValueItem *materialItem=nullptr;
        if (material->get_isLocal()) materialItem=new KeywordValueItem(data,localItem);
        else materialItem=new KeywordValueItem(data,globalItem);
        materialItem->set_level(2);
        materialItem->setType(QString::fromStdString(material->get_type()));
        materialItem->set_copyAllowed(true);

        if (material->get_isLocal()) localItem->appendChild(materialItem);
        else globalItem->appendChild(materialItem);

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
            temperatureItem->set_level(3);
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
                debyeItem->set_level(6);
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
                    frequencyItem->set_level(5);
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
        sourceItem->set_level(5);
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
                pairItem->set_level(8);
                pairItem->set_copyAllowed(true);
                sourceItem->appendChild(pairItem);

                k++;
            }
            j++;
        }

        // set to unmodified
        materialItem->set_isModified(false);

        i++;
    }
}

void MaterialsModel::materialsModel_dataChanged (const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
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

    setFixedSize(QDialog::width(),QDialog::height());

    copyItem=nullptr;
    contextMenuItem=nullptr;
    materialDatabase=nullptr;
    localMaterialDatabase=nullptr;
    materials_global_path=nullptr;
    materials_global_name=nullptr;
    materials_local_path=nullptr;
    materials_local_name=nullptr;
    materials_default_boundary=nullptr;
    materials_check_limits=1;

    // MaterialsModel

    materialsModel=new MaterialsModel();
    materialsModel->setMaterials(this);

    connect(materialsModel, &QAbstractItemModel::dataChanged, materialsModel, &MaterialsModel::materialsModel_dataChanged);
    connect(materialsModel, &QAbstractItemModel::modelReset, this, &Materials::expandFirstLevel);

    ui->materialsTreeView->setModel(materialsModel);
    connect(ui->materialsTreeView->selectionModel(),&QItemSelectionModel::selectionChanged,this,&Materials::selection_changed);


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
    connect(ui->materialsTreeView, &QTreeView::expanded, this, &Materials::onExpanded);

    // menu bar

    QMenuBar *menuBar = new QMenuBar(nullptr);

    // File

    QMenu *fileMenu = new QMenu("File", menuBar);

    defaultMaterial=new QAction("Assign Default",this);
    connect(defaultMaterial, &QAction::triggered, this, &Materials::defaultMaterial_triggered);

    filePrototypes=new QAction("Insert Prototypes",this);
    filePrototypes->setShortcut(QKeySequence::New);
    //fileMenu->addAction(filePrototypes);
    connect(filePrototypes, &QAction::triggered, this, &Materials::newAction_triggered);

    fileOpen=new QAction("Open ...",this);
    fileOpen->setShortcut(QKeySequence::Open);
    // fileMenu->addAction(fileOpen);
    connect(fileOpen, &QAction::triggered, this, &Materials::openAction_triggered);

    fileSave=new QAction("Save",this);
    fileSave->setShortcut(QKeySequence::Save);
    // fileSave->setEnabled(false);
    // fileMenu->addAction(fileSave);
    connect(fileSave, &QAction::triggered, this, &Materials::saveAction_triggered);

    fileSaveAs=new QAction("Save As ...",this);
    fileSaveAs->setShortcut(QKeySequence::SaveAs);
    // fileSaveAs->setEnabled(false);
    // fileMenu->addAction(fileSaveAs);
    connect(fileSaveAs, &QAction::triggered, this, &Materials::saveAsAction_triggered);

    fileClose=new QAction("Close",this);
    fileClose->setShortcut(QKeySequence::Close);
    // fileMenu->addAction(fileClose);
    connect(fileClose, &QAction::triggered, this, &Materials::closeAction_triggered);

    fileExit=new QAction("Exit",fileMenu);
    fileMenu->addAction(fileExit);
    connect(fileExit, &QAction::triggered, this, &Materials::exitAction_triggered);


    menuBar->addMenu(fileMenu);

    // // Edit

    // QMenu *editMenu = new QMenu("Edit", menuBar);

    editCopy=new QAction("Copy",this);
    editCopy->setShortcut(QKeySequence::Copy);
    // editCopy->setEnabled(false);
    // editMenu->addAction(editCopy);
    connect(editCopy, &QAction::triggered, this, &Materials::copyData);

    editPaste=new QAction("Paste",this);
    editPaste->setShortcut(QKeySequence::Paste);
    // editPaste->setEnabled(false);
    // editMenu->addAction(editPaste);
    connect(editPaste, &QAction::triggered, this, &Materials::pasteData);

    QKeySequence Append(Qt::CTRL | Qt::Key_A);
    editAppend=new QAction("Append",this);
    editAppend->setShortcut(Append);
    // editAppend->setEnabled(false);
    // editMenu->addAction(editAppend);
    connect(editAppend, &QAction::triggered, this, &Materials::appendData);

    QKeySequence Convert(Qt::CTRL | Qt::Key_R);
    editConvert=new QAction("Set to \"any\"",this);
    editConvert->setShortcut(Convert);
    // editConvert->setEnabled(false);
    // editMenu->addAction(editConvert);
    connect(editConvert, &QAction::triggered, this, &Materials::convertData);

    QKeySequence InsertNew(Qt::CTRL | Qt::Key_M);
    editNew=new QAction("Insert New",this);
    editNew->setShortcut(InsertNew);
    // editNew->setEnabled(false);
    // editMenu->addAction(editNew);
    connect(editNew, &QAction::triggered, this, &Materials::insertNewData);

    editDelete=new QAction("Delete",this);
    editDelete->setShortcut(QKeySequence::Delete);
    editDelete->setEnabled(false);
    // editMenu->addAction(editDelete);
    connect(editDelete, &QAction::triggered, this, &Materials::deleteData);

   //menuBar->addMenu(editMenu);
    //ui->materialsMenuBar->addWidget(menuBar);

    ui->materialsTreeView->show();

    ui->defaultBoundaryMaterial->setAlignment(Qt::AlignVCenter);
    ui->OkButton->setCheckable(true);
    ui->CancelButton->setCheckable(true);

    ui->OkButton->setEnabled(false);

    bool isXclose;            // user clicked the "X" to close

}

Materials::~Materials ()
{
    if (materialsModel) {delete materialsModel; materialsModel=nullptr;}
    if (copyItem) {delete copyItem; copyItem=nullptr;}
    if (localMaterialDatabase) {delete localMaterialDatabase; localMaterialDatabase=nullptr;}
    if (materials_global_path) {free(materials_global_path); materials_global_path=nullptr;}
    if (materials_global_name) {free(materials_global_name); materials_global_name=nullptr;}
    if (materials_local_path) {free(materials_local_path); materials_local_path=nullptr;}
    if (materials_local_name) {free(materials_local_name); materials_local_name=nullptr;}
    delete ui;
}

// courtesy of ChatGPT
static int treeDepth(QModelIndex index)
{
    int depth = 0;

    while (index.parent().isValid())
    {
        ++depth;
        index = index.parent();
    }

    return depth;
}

void Materials::set_projData (struct projectData *a)
{
    projData=a;

    // Save projData to local variables to enable a cancel operation with no changes
    materials_global_path=allocCopyString(projData->materials_global_path);
    materials_global_name=allocCopyString(projData->materials_global_name);
    materials_local_path=allocCopyString(projData->materials_local_path);
    materials_local_name=allocCopyString(projData->materials_local_name);
    materials_default_boundary=allocCopyString(projData->materials_default_boundary);
    materials_check_limits=projData->materials_check_limits;

    // load the materials into a local database
    if (localMaterialDatabase) {delete localMaterialDatabase; localMaterialDatabase=nullptr;}
    localMaterialDatabase=new MaterialDatabase();
    localMaterialDatabase->load_materials(materials_global_path,materials_global_name,
                                          materials_local_path,materials_local_name,
                                          materials_check_limits);

    // populate the data
    if (localMaterialDatabase) {
        populate();
        ui->checkLimits->setChecked(materials_check_limits);
        ui->defaultBoundaryMaterial->setText(materials_default_boundary);

        QString globalFilename=materials_global_path;
        globalFilename.append(materials_global_name);
        materialsModel->set_globalItem_filename(globalFilename);

        QString localFilename=materials_local_path;
        localFilename.append(materials_local_name);
        materialsModel->set_localItem_filename(localFilename);
    }

    ui->OkButton->setEnabled(false);
}

// courtesy of ChatGPT with the depth == 0 break added and required variable name edits
void Materials::onExpanded(const QModelIndex& index)
{
    int depth = treeDepth(index);

    QModelIndex last = index;

    // Walk down the last child in each level
    while (ui->materialsTreeView->model()->rowCount(last) > 0)
    {
        int rows = ui->materialsTreeView->model()->rowCount(last);
        last = ui->materialsTreeView->model()->index(rows - 1, 0, last);

        if (depth == 0) break;
    }

    ui->materialsTreeView->scrollTo(
        last,
        QAbstractItemView::PositionAtBottom);
}

void Materials::keyPressEvent (QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        event->ignore();
    } else {
        QDialog::keyPressEvent(event);
    }
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

    if (copyItem) delete copyItem;
    copyItem=item;

    signalSelection();
}

void Materials::pasteData ()
{
    if (!copyItem) return;

    const QModelIndex index = ui->materialsTreeView->selectionModel()->currentIndex();
    QModelIndex parent=index.parent();
    copyItem->insertChild(parent,index.row()+1,materialsModel);

    signalSelection();
}

void Materials::appendData ()
{
    if (!copyItem) return;

    const QModelIndex rawIndex=ui->materialsTreeView->selectionModel()->currentIndex();
    QModelIndex index=rawIndex.siblingAtColumn(0);
    copyItem->insertChild(index,materialsModel->getItem(index)->lastRow(copyItem)+1,materialsModel);

    signalSelection();
}

void Materials::insertNewData ()
{
    // location to put the new material
    // QModelIndexList selectedIndices=ui->materialsTreeView->selectionModel()->selectedIndexes();
    // if (selectedIndices.size() == 0) return;
    // QModelIndex index=selectedIndices[0];
    // if (!index.isValid()) return;

    // three new materials: dielectric frequency list, dielectric Debye, and metal

    localMaterialDatabase->clear();

    Material *material=new Material(0,0);
    material->set_freespace();
    material->set_isLocal(true);
    localMaterialDatabase->push(material);

    material=new Material(0,0);
    material->set_FR4();
    material->set_isLocal(true);
    localMaterialDatabase->push(material);

    material=new Material(0,0);
    material->set_copper();
    material->set_isLocal(true);
    localMaterialDatabase->push(material);

    populate();

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
    if (copyItem) {delete copyItem; copyItem=nullptr;}

    signalSelection();
}

int Materials::check_changed ()
{
    std::cout << "Materials::check_changed" << std::endl; std::cout.flush();

    if (!materialsModel) return QMessageBox::Discard;
    if (!contextMenuItem) return QMessageBox::Discard;

    int retVal=0;
    bool isLocal=false;
    if (contextMenuItem->data(0) == "local") isLocal=true;
    else if (contextMenuItem->data(0) == "global") isLocal=false;

    bool isChanged=false;
    if (isLocal) {
        if (materialsModel->get_isLocalModified()) isChanged=true;
    } else {
        if (materialsModel->get_isGlobalModified()) isChanged=true;
    }

    std::cout << "materialsModel->get_isLocalModified()=" << materialsModel->get_isLocalModified() << std::endl; std::cout.flush();
    std::cout << "materialsModel->get_isGlobalModified()=" << materialsModel->get_isGlobalModified() << std::endl; std::cout.flush();

    if (isChanged) {
        QMessageBox msgBox;
        msgBox.setText("The materials have been modified.");
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        retVal = msgBox.exec();
    }
    return retVal;
}

void Materials::defaultMaterial_triggered ()
{
    if (materials_default_boundary) {free(materials_default_boundary); materials_default_boundary=nullptr;}
    materials_default_boundary=allocCopyConstString(contextMenuItem->data(0).toString().toUtf8().constData());
    ui->defaultBoundaryMaterial->setText(contextMenuItem->data(0).toString());
    ui->OkButton->setEnabled(true);
}

void Materials::newAction_triggered ()
{
    std::cout << "Materials::newAction_triggered" << std::endl; std::cout.flush();

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

    // three starter materials - simple dielectric, one frequency list, and one Debye to have each style to work with

    localMaterialDatabase->clear();

    bool isLocal;
    if (contextMenuItem->data(0) == "local") isLocal=true;
    else if (contextMenuItem->data(0) == "global") isLocal=false;

    Material *material=new Material(0,0);
    material->set_freespace();
    material->set_isLocal(isLocal);
    localMaterialDatabase->push(material);

    material=new Material(0,0);
    material->set_FR4();
    material->set_isLocal(isLocal);
    localMaterialDatabase->push(material);

    material=new Material(0,0);
    material->set_copper();
    material->set_isLocal(isLocal);
    localMaterialDatabase->push(material);

    populate();

    if (isLocal) materialsModel->set_isLocalModified(true);
    else materialsModel->set_isGlobalModified(true);

    ui->OkButton->setEnabled(true);
}

void Materials::expandFirstLevel ()
{
    //std::cout << "Materials::expandFirstLevel" << std::endl; std::cout.flush();

    QTimer::singleShot(0, this, [this]() {
        ui->materialsTreeView->blockSignals(true);
        ui->materialsTreeView->expandToDepth(0);
        ui->materialsTreeView->blockSignals(false);
    });
}

void Materials::populate ()
{
    std::cout << "Materials::populate" << std::endl; std::cout.flush();

    materialsModel->populate(localMaterialDatabase);
    ui->materialsTreeView->resizeColumnToContents(0);
    expandFirstLevel();
    //ui->materialsTreeView->show();
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

    QString testMaterialsFile=QFileDialog::getOpenFileName(this,tr("Open Materials File"), absolutePath,
                                tr("Data Files (*.txt);;All Files (*)"),nullptr,QFileDialog::DontUseNativeDialog);

    // return if user cancels
    if (testMaterialsFile.isNull()) return;

    char nullstring[1]; nullstring[0]='\0';
    if (QFile::exists(testMaterialsFile)) {

        localMaterialDatabase->clear();

        // load as local or global
        if (contextMenuItem->data(0) == "local") {

            materialsModel->clear_localItem();

            // break up the returned filename

            QFileInfo fileInfo(testMaterialsFile);

            if (materials_local_path) {free(materials_local_path); materials_local_path=nullptr;}
            materials_local_path=allocCopyConstString(fileInfo.absolutePath().toUtf8().constData());

            if (materials_local_name) {free(materials_local_name); materials_local_name=nullptr;}
            materials_local_name=allocCopyConstString(fileInfo.fileName().toUtf8().constData());

            // load
            if (localMaterialDatabase) {delete localMaterialDatabase; localMaterialDatabase=nullptr;}
            localMaterialDatabase=new MaterialDatabase();
            if (localMaterialDatabase->load_materials(nullstring,nullstring,materials_local_path,materials_local_path,materials_check_limits)) {
                QMessageBox mb;
                mb.critical(nullptr, "Error", "Failed to load materials file.");
                mb.setFixedSize(500, 200);
                return;
            } else {
                materialsModel->set_localItem_filename(testMaterialsFile);
            }
        } else if (contextMenuItem->data(0) == "global") {

            materialsModel->clear_globalItem();

            // break up the returned filename

            QFileInfo fileInfo(testMaterialsFile);

            if (materials_global_path) {free(materials_global_path); materials_global_path=nullptr;}
            materials_global_path=allocCopyConstString(fileInfo.absolutePath().toUtf8().constData());

            if (materials_global_name) {free(materials_global_name); materials_global_name=nullptr;}
            materials_global_name=allocCopyConstString(fileInfo.fileName().toUtf8().constData());

            // load
            if (localMaterialDatabase) {delete localMaterialDatabase; localMaterialDatabase=nullptr;}
            localMaterialDatabase=new MaterialDatabase();
            if (localMaterialDatabase->load_materials(nullstring,nullstring,materials_global_path,materials_global_path,materials_check_limits)) {
                QMessageBox mb;
                mb.critical(nullptr, "Error", "Failed to load materials file.");
                mb.setFixedSize(500, 200);
                return;
            } else {
                materialsModel->set_globalItem_filename(testMaterialsFile);
            }
        }

        populate();

    } else {
        QMessageBox mb;
        mb.critical(nullptr,"Error","File not found.");
        mb.setFixedSize(500,200);

        // fileOpen->setEnabled(false);
        return;
    }
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

    KeywordValueItem *rootItem=materialsModel->get_rootItem();
    if (rootItem->childCount() != 2) return true;

    // local/global
    int m=0;
    while (m < 2) {
        int i=0;
        while (i < rootItem->child(m)->childCount()) {

            // temperature
            KeywordValueItem *child_i=rootItem->child(m)->child(i);
            QVariant duplicate=child_i->hasDuplicateValue(3);
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
                QVariant duplicate=child_j->hasDuplicateValue(5);
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
        m++;
    }
    return false;
}

void Materials::saveAction_triggered ()
{
    if (check_duplicates()) return;

    if (!contextMenuItem) return;

    // file name for the database type
    QString materialsFile;
    if (contextMenuItem->data(0) == "local") {
        materialsFile=materials_local_path;
        materialsFile.append(materials_local_name);
    } else if (contextMenuItem->data(0) == "global") {
        materialsFile=materials_global_path;
        materialsFile.append(materials_global_name);
    }

    // save
    QFile file(materialsFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream *fileOut=new QTextStream(&file);

        QString version=QString::fromStdString(localMaterialDatabase->get_version_name()+" "+localMaterialDatabase->get_version_value());

        *fileOut << version << "\n";
        *fileOut << "\n";

        contextMenuItem->print(fileOut,contextMenuItem);
    } else {
        saveAsAction_triggered();
    }
}

void Materials::saveAsAction_triggered ()
{
    if (check_duplicates()) return;

    QString testMaterialsFile=QFileDialog::getSaveFileName(this,tr("Save Materials File"), absolutePath,
                                tr("Data Files (*.txt);;All Files (*)"),nullptr,QFileDialog::DontUseNativeDialog);

    // return if user cancels
    if (testMaterialsFile.isNull()) return;

    // break up the file name

    if (contextMenuItem->data(0) == "local") {
        QFileInfo fileInfo(testMaterialsFile);

        if (materials_local_path) {free(materials_local_path); materials_local_path=nullptr;}
        materials_local_path=allocCopyConstString(fileInfo.absolutePath().toUtf8().constData());

        if (materials_local_name) {free(materials_local_name); materials_local_name=nullptr;}
        materials_local_name=allocCopyConstString(fileInfo.fileName().toUtf8().constData());
    } else if (contextMenuItem->data(0) == "global") {
        QFileInfo fileInfo(testMaterialsFile);

        if (materials_global_path) {free(materials_global_path); materials_global_path=nullptr;}
        materials_global_path=allocCopyConstString(fileInfo.absolutePath().toUtf8().constData());

        if (materials_global_name) {free(materials_global_name); materials_global_name=nullptr;}
        materials_global_name=allocCopyConstString(fileInfo.fileName().toUtf8().constData());
    }


    QFile file(testMaterialsFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream *fileOut=new QTextStream(&file);

        QString version=QString::fromStdString(localMaterialDatabase->get_version_name()+" "+localMaterialDatabase->get_version_value());

        *fileOut << version << "\n";
        *fileOut << "\n";

        contextMenuItem->print(fileOut,contextMenuItem);
    }
}

void Materials::closeAction_triggered ()
{
    bool closeItem=true;
    if (contextMenuItem->data(0) == "local" && materialsModel->get_isLocalModified()) closeItem=false;
    else if (contextMenuItem->data(0) == "global" && materialsModel->get_isGlobalModified()) closeItem=false;

    int retVal=0;
    if (!closeItem) {
        QMessageBox msgBox;
        msgBox.setText("The materials have been modified.");
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        retVal = msgBox.exec();
    }

    if (retVal == QMessageBox::Save) {
        saveAction_triggered();
    } else if (retVal == QMessageBox::Cancel) return;

    contextMenuItem->removeChildren(0,contextMenuItem->childCount());
    expandFirstLevel();

    contextMenuItem->set_isModified(false);
}

void Materials::closeWindow_triggered ()
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

void Materials::exitAction_triggered ()
{
    close_event=nullptr;
    close();
}

void Materials::contextMenu_triggered(const QPoint& pnt)
{
    QMenu menu(this);
    QModelIndex index = ui->materialsTreeView->indexAt(pnt);
    if (!index.isValid()) return;

    contextMenuItem=static_cast<KeywordValueItem*>(index.internalPointer());
    if (contextMenuItem) {

        if (contextMenuItem->data(0) == "local") {
            menu.addAction(filePrototypes);
            if (contextMenuItem->childCount() == 0) menu.addAction(fileOpen);
            if (contextMenuItem->childCount() > 0 && materialsModel->get_isLocalModified()) menu.addAction(fileSave);
            if (contextMenuItem->childCount() > 0) menu.addAction(fileSaveAs);
            menu.addAction(fileClose);
        } else if (contextMenuItem->data(0) == "global") {
            menu.addAction(filePrototypes);
            if (contextMenuItem->childCount() == 0) menu.addAction(fileOpen);
            if (contextMenuItem->childCount() > 0 && materialsModel->get_isGlobalModified()) menu.addAction(fileSave);
            if (contextMenuItem->childCount() > 0) menu.addAction(fileSaveAs);
            menu.addAction(fileClose);
        } else {
            menu.addAction(editNew);
            menu.addAction(editCopy);
            menu.addAction(editPaste);
            menu.addAction(editAppend);
            menu.addAction(editConvert);
            menu.addAction(editDelete);
            if (contextMenuItem->getType().compare("conductor") == 0) menu.addAction(defaultMaterial);
        }

        menu.exec(ui->materialsTreeView->mapToGlobal(pnt));
    }
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
    if (copyItem && copyItem->is_sameLevel(item)) {
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
    if (copyItem) {

        // material appending temperature
        if (item->get_level() == 1 && copyItem->get_level() == 2) {
            if (!item->hasAny(copyItem)) {
                if (item->hasItem(copyItem)) { // match level;
                    if (!copyItem->isAny()) {
                        editAppend->setEnabled(true);
                    }
                } else {
                    editAppend->setEnabled(true);
                }
            }
        }

        // temperature appending frequency
        if (item->get_level() == 2 && copyItem->get_level() == 4) {
            if (!item->hasLevel(5) && !item->hasAny(copyItem)) {
                if (item->hasItem(copyItem)) { // match level
                    if (!copyItem->isAny()) {
                        editAppend->setEnabled(true);
                    }
                } else {
                    editAppend->setEnabled(true);
                }
            }
        }

        // temperature appending Debye
        if (item->get_level() == 2 && copyItem->get_level() == 5) {
            if (!item->hasLevel(4) && !item->hasAny(copyItem)) {
                if (item->hasItem(copyItem)) { // match level
                    if (!copyItem->isAny()) {
                        editAppend->setEnabled(true);
                    }
                } else {
                    editAppend->setEnabled(true);
                }
            }
        }

        // material appending source
        if (item->get_level() == 1 && copyItem->get_level() == 3) {
            editAppend->setEnabled(true);
        }

        // source appending source text
        if (item->get_level() == 3 && copyItem->get_level() == 7) {
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
    // if (globalMaterialsFile != "") fileSave->setEnabled(true);
    // fileSaveAs->setEnabled(true);
    ui->OkButton->setEnabled(true);
}

void Materials::on_OkButton_clicked ()
{
    // save data back to the project

    if (projData->materials_global_path) {free(projData->materials_global_path); projData->materials_global_path=nullptr;}
    projData->materials_global_path=allocCopyString(materials_global_path);

    if (projData->materials_global_name) {free(projData->materials_global_name); projData->materials_global_name=nullptr;}
    projData->materials_global_name=allocCopyString(materials_global_name);

    if (projData->materials_local_path) {free(projData->materials_local_path); projData->materials_local_path=nullptr;}
    projData->materials_local_path=allocCopyString(materials_local_path);

    if (projData->materials_local_name) {free(projData->materials_local_name); projData->materials_local_name=nullptr;}
    projData->materials_local_name=allocCopyString(materials_local_name);

    if (projData->materials_default_boundary) {free(projData->materials_default_boundary); projData->materials_default_boundary=nullptr;}
    projData->materials_default_boundary=allocCopyString(materials_default_boundary);

    if (projData->materials_default_boundary) {free(projData->materials_default_boundary); projData->materials_default_boundary=nullptr;}
    projData->materials_default_boundary=allocCopyString(materials_default_boundary);

    projData->materials_check_limits=materials_check_limits;

    if (materialsModel->get_isLocalModified() || materialsModel->get_isGlobalModified()) {
        if (materialDatabase) {delete materialDatabase; materialDatabase=nullptr;}
        materialDatabase=new MaterialDatabase();
        materialDatabase->load_materials(materials_global_path,materials_global_name,
                                         materials_local_path,materials_local_name,
                                         materials_check_limits);
    }

    isXclose=false;
    QDialog::close();
}

void Materials::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    isXclose=false;
    QDialog::close();
}

void Materials::reject ()
{
    ui->CancelButton->setChecked(true);
    QDialog::reject();
}

void Materials::on_checkLimits_stateChanged (int arg1)
{
    materials_check_limits=arg1;
    ui->OkButton->setEnabled(true);
}

