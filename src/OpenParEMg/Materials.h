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
#include <iostream>
#include "OpenParEMmaterials.hpp"

using namespace std;

class KeywordValueItem {
public:
    explicit KeywordValueItem(const QList<QVariant>& data, KeywordValueItem* parent = nullptr);
    ~KeywordValueItem();

    void appendChild(KeywordValueItem* child);
    KeywordValueItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    KeywordValueItem* parentItem();

    QVector<KeywordValueItem*>* get_m_childItems() {return &m_childItems;}
    void print();

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

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    KeywordValueItem* get_rootItem () {return rootItem;}
    void populate(MaterialDatabase *, KeywordValueItem *);
    void print () {rootItem->print();}

private:
    void setupModelData(const QStringList &lines, KeywordValueItem *parent);
    KeywordValueItem *rootItem;
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

    void on_addMaterial_clicked();

    void materialItemClicked(QTreeWidgetItem *, int);

    void on_deleteMaterial_clicked();

    void on_duplicateMaterial_clicked();

    void newAction_triggered();

    void openAction_triggered();

    void closeAction_triggered();

private:
    Ui::Materials *ui;
    //struct projectData *projData;
    QString materialsFile;
    MaterialDatabase materialDatabase;
    MaterialsModel *materialsModel;
};

#endif // MATERIALSg_H
