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

#ifndef MATERIALSg_H
#define MATERIALSg_H

#include <QDialog>
#include <QVariant>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QFile>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QMenuBar>
#include <QFileDialog>
#include <QStyledItemDelegate>
#include "OpenParEMmaterials.hpp"

extern "C" char* allocCopyString (char *);
extern "C" char* allocCopyConstString (const char *);

class Materials;
class MaterialsModel;

class LineEditDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit LineEditDelegate (QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    QWidget* createEditor (QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData (QWidget *editor, const QModelIndex &index) const override;
    void setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private slots:
    void commitModelData ();
};

class KeywordValueItem
{
public:
    explicit KeywordValueItem (const QList<QVariant>& data, KeywordValueItem* parent = nullptr);
    ~KeywordValueItem ();

    void appendChild (KeywordValueItem *);
    KeywordValueItem* child (int);
    int childCount () const;
    void insertChild (QModelIndex, int, MaterialsModel *);
    bool insertChildren (int , int , int);
    bool removeChildren (int , int);
    int columnCount () const;
    QVariant data (int) const;
    int row () const;
    KeywordValueItem* parentItem ();

    QVector<KeywordValueItem*>* get_m_childItems () {return &m_childItems;}
    bool setData (int, const QVariant &value);
    void setType (QString type_) {type=type_;}
    QString getType () {return type;}
    KeywordValueItem* copy ();
    void print ();
    void print (QTextStream *, KeywordValueItem *);

    void set_level (int level_) {level=level_;}
    void set_copyAllowed (bool copyAllowed_) {copyAllowed=copyAllowed_;}
    int get_level () {return level;}
    bool isAny () {
        if (data(1) == "any") return true;
        else return false;
    }
    void set_noCopy () {level=6; copyAllowed=false;}

    bool canCopy () {return copyAllowed;}
    bool is_parentLevelMatch (KeywordValueItem *item) {
        if (m_parentItem->level == item->level) return true;
        return false;
    }
    bool is_sameLevel (KeywordValueItem *item) {
        if (level == item->level) return true;
        return false;
    }
    bool hasAny (KeywordValueItem *);
    bool hasLevel (int);
    bool hasItem (KeywordValueItem *);
    int lastRow (KeywordValueItem *);
    bool hasOne (KeywordValueItem *);
    QVariant hasDuplicateKeyword ();
    QVariant hasDuplicateValue (int);
    void set_isModified (bool);
    bool get_isModified ();

    QString getToolTip () const {return m_toolTip;}
    void setToolTip (const QString& text) {m_toolTip=text;}

private:
    QList<QVariant> m_itemData;
    QVector<KeywordValueItem*> m_childItems;
    KeywordValueItem *m_parentItem;
    QString m_toolTip;
    QString type;     // dielectric or conductor

    int level;        // 0=>root item, 1=>local/global, 2=>Material, 3=>Temperature,
                      // 4=>Source, 5=>Frequency, 6=>Debye, 7=>data keyword pairs, 8=>source text
    bool copyAllowed;
    bool isModified;  // either changed or new
};

class MaterialsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    //explicit MaterialsModel (const QString &data, QObject *parent = nullptr);
    explicit MaterialsModel (QObject *parent = nullptr);
    ~MaterialsModel();


    void beginInsertRows (const QModelIndex &parent, int first, int last)
    {
        return QAbstractItemModel::beginInsertRows(parent,first,last);
    }

    void endInsertRows ()
    {
        return QAbstractItemModel::endInsertRows();
    }


    QVariant data (const QModelIndex &index, int) const override;
    Qt::ItemFlags flags (const QModelIndex &index) const override;
    KeywordValueItem* getItem (const QModelIndex &index) const;
    QVariant headerData (int, Qt::Orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index (int, int,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent (const QModelIndex &index) const override;
    int rowCount (const QModelIndex &parent = QModelIndex()) const override;
    bool insertRows (int, int, const QModelIndex &parent = {}) override;
    bool removeRows (int, int, const QModelIndex &parent) override;
    int columnCount (const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex indexFromItem(KeywordValueItem* item) const;

    KeywordValueItem* get_rootItem () {return rootItem;}
    bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    void populate (MaterialDatabase *);
    void setMaterials (Materials *materials_) {materials=materials_;}
    void print () {rootItem->print();}
    void signalSelection ();

    KeywordValueItem* get_localItem () {return localItem;}
    KeywordValueItem* get_globalItem () {return globalItem;}

    bool is_localItem (KeywordValueItem *item) {if (item == localItem) return true; return false;}
    bool is_globalItem (KeywordValueItem *item) {if (item == globalItem) return true; return false;}

    bool get_isLocalModified () {return localItem->get_isModified();}
    bool get_isGlobalModified () {return globalItem->get_isModified();}

    void set_isLocalModified (bool isModified_) {localItem->set_isModified(isModified_);}
    void set_isGlobalModified (bool isModified_) {globalItem->set_isModified(isModified_);}

    void clear_localItem () {localItem->removeChildren(0,localItem->childCount());}
    void clear_globalItem () {globalItem->removeChildren(0,globalItem->childCount());}

    void set_globalItem_tip (QString text)
    {
        globalItem->setToolTip(text);
        QModelIndex idx=indexFromItem(globalItem);
        emit dataChanged(idx,idx,{Qt::ToolTipRole});
    }

    void set_localItem_tip (QString text)
    {
        localItem->setToolTip(text);
        QModelIndex idx=indexFromItem(localItem);
        emit dataChanged(idx,idx,{Qt::ToolTipRole});
    }

public slots:

    void materialsModel_dataChanged (const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());


private:
    //void setupModelData(const QStringList &lines, KeywordValueItem *parent);
    KeywordValueItem *rootItem;
    KeywordValueItem *globalItem;
    KeywordValueItem *localItem;
    Materials *materials;
};

namespace Ui {
class Materials;
}

class Materials : public QDialog
{
    Q_OBJECT

public:
    explicit Materials (QWidget *parent = nullptr);
    ~Materials ();
    void set_projData (struct projectData *);
    void setMaterialDatabase (MaterialDatabase **materialDatabase_) {materialDatabase=materialDatabase_;}
    void populate ();
    void keyPressEvent (QKeyEvent *) override;
    int check_changed ();
    void materials_edited ();
    void signalSelection ();
    void setAbsolutePath (QString absolutePath_) {absolutePath=absolutePath_;}

    void closeEvent (QCloseEvent *event) override {
        close_event=event;
        closeWindow_triggered();
    }

    bool check_duplicates ();

public slots:
    void expandFirstLevel ();

private slots:

    void copyData ();
    void pasteData ();
    void appendData ();
    //void insertNewData ();
    void convertData ();
    void deleteData ();

    void defaultMaterial_triggered ();
    void newAction_triggered ();
    void openAction_triggered ();
    void saveAction_triggered ();
    void saveAsAction_triggered ();
    void clearAction_triggered ();
    void exitAction_triggered ();
    void closeWindow_triggered ();
    void contextMenu_triggered (const QPoint& pnt);
    void selection_changed (const QItemSelection &selected, const QItemSelection &deselected);
    void onExpanded (const QModelIndex& index);
    void on_OkButton_clicked ();
    void on_CancelButton_clicked ();
    void reject () override;

    void on_checkLimits_stateChanged(int arg1);

private:
    Ui::Materials *ui;

    struct projectData *projData;
    QString absolutePath;  // to the project
    MaterialDatabase **materialDatabase;
    MaterialsModel *materialsModel;
    KeywordValueItem *copyItem;
    KeywordValueItem *contextMenuItem;

    QAction *defaultMaterial;
    QAction *filePrototypes;
    QAction *fileOpen;
    QAction *fileSave;
    QAction *fileSaveAs;
    QAction *fileClose;
    QAction *fileExit;

    QAction *editCopy;
    QAction *editPaste;
    QAction *editAppend;
    //QAction *editNew;
    QAction *editConvert;
    QAction *editDelete;

    QCloseEvent *close_event;

    // local variables for cancel
    MaterialDatabase *localMaterialDatabase;
    char *materials_global_path;
    char *materials_global_name;
    char *materials_local_path;
    char *materials_local_name;
    char *materials_default_boundary;
    int materials_check_limits;
    bool projectDataModified;

    bool isXclose;            // user clicked the "X" to close
};

#endif // MATERIALSg_H
