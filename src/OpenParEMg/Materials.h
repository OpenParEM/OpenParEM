#ifndef MATERIALSg_H
#define MATERIALSg_H

#include <QDialog>
#include <QFile>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QMenuBar>
#include <QFileDialog>
#include <iostream>
#include "project.h"
#include "OpenParEMmaterials.hpp"

using namespace std;

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
};

#endif // MATERIALSg_H
