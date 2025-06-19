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
#include "project.h"
#include "OpenParEMmaterials.hpp"

using namespace std;



class MaterialItem {
public:
    explicit MaterialItem(const QList<QVariant>& data, MaterialItem* parent = nullptr);
    ~MaterialItem();

    /*
    void appendChild(MaterialItem* child);
    MaterialItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    MaterialItem* parentItem();
    */

private:
    QList<QVariant> m_itemData;
    QVector<MaterialItem*> m_childItems;
    MaterialItem* m_parentItem;
};

class MaterialsModel : public QAbstractItemModel {
    Q_OBJECT

public:
    MaterialsModel(const QStringList& headers, QObject* parent = nullptr) {;}
    ~MaterialsModel();

    /*
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    */

private:
    //void setupModelData(const QStringList& lines, MaterialItem* parent);
    //MaterialItem* getItem(const QModelIndex& index) const;

    //MaterialItem* m_rootItem;
};







namespace Ui {
class Materials;
}

class Materials : public QDialog
{
    Q_OBJECT

public:
    explicit Materials(QWidget *parent = nullptr);
    void load ();
    void populate ();
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
    struct projectData *projData;
    QString materialsFile;
    MaterialDatabase materialDatabase;


    MaterialsModel materialsModel;
};

#endif // MATERIALSg_H
