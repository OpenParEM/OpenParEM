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
#include <iostream>
#include "OpenParEMmaterials.hpp"
#include "CustomLineEdit.h"

using namespace std;

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

class KeywordValueItem {
public:
    explicit KeywordValueItem (const QList<QVariant>& data, KeywordValueItem* parent = nullptr);
    ~KeywordValueItem ();

    void appendChild (KeywordValueItem* child);
    KeywordValueItem* child (int row);
    int childCount () const;
    void insertChild(QModelIndex index, int row, MaterialsModel *materialsModel);
    bool insertChildren(int position, int count, int columns);
    bool removeChildren(int position, int count);
    int columnCount () const;
    QVariant data (int column) const;
    int row () const;
    KeywordValueItem* parentItem ();

    QVector<KeywordValueItem*>* get_m_childItems () {return &m_childItems;}
    bool setData (int column, const QVariant &value);
    KeywordValueItem* copy();
    void print ();
    void print (QTextStream *textOut, KeywordValueItem *rootItem);

private:
    QList<QVariant> m_itemData;
    QVector<KeywordValueItem*> m_childItems;
    KeywordValueItem *m_parentItem;
};

class MaterialsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit MaterialsModel(const QString &data, QObject *parent = nullptr);
    explicit MaterialsModel(QObject *parent = nullptr);
    ~MaterialsModel();

    void beginInsertRows(const QModelIndex &parent, int first, int last)
    {
        return QAbstractItemModel::beginInsertRows(parent,first,last);
    }

    void endInsertRows()
    {
        return QAbstractItemModel::endInsertRows();
    }

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    KeywordValueItem* getItem(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool insertRows(int position, int rows, const QModelIndex &parent = {}) override;
    bool removeRows(int position, int rows, const QModelIndex &parent) override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    KeywordValueItem* get_rootItem () {return rootItem;}
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    void populate(MaterialDatabase *, KeywordValueItem *);
    bool hasChanged () {return dataHasChanged;}
    void setUnchanged () {dataHasChanged=false;}
    void print () {rootItem->print();}

public slots:

    void materialsModel_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());


private:
    void setupModelData(const QStringList &lines, KeywordValueItem *parent);
    KeywordValueItem *rootItem;
    bool dataHasChanged;
};

namespace Ui {
class Materials;
}

class Materials : public QDialog
{
    Q_OBJECT

public:
    explicit Materials(QWidget *parent = nullptr);
    ~Materials();

private slots:

    void copyData();
    void pasteData();
    void deleteData();

    void newAction_triggered();
    void openAction_triggered();
    void saveAction_triggered();
    void closeAction_triggered();
    void contextMenu_triggered(const QPoint& pnt);

private:
    Ui::Materials *ui;
    //struct projectData *projData;
    QString materialsFile;
    MaterialDatabase materialDatabase;
    MaterialsModel *materialsModel;
    KeywordValueItem *itemCopy;
};

#endif // MATERIALSg_H
