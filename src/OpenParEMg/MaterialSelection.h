#ifndef MATERIALSELECTION_H
#define MATERIALSELECTION_H

#include <QDialog>
#include "OpenParEMmaterials.hpp"

namespace Ui {
class MaterialSelection;
}

class MaterialSelection : public QDialog
{
    Q_OBJECT

public:
    explicit MaterialSelection (QWidget *parent = nullptr);
    ~MaterialSelection ();

    void set_materialDatabase (MaterialDatabase *materialDatabase_) {materialDatabase=materialDatabase_;}
    void set_selectedMaterial (QString *selectedMaterial_) {selectedMaterial=selectedMaterial_;}

    void populate ();
private slots:
    void on_materialSelectOk_clicked ();
    void on_materialSelectCancel_clicked ();

private:
    Ui::MaterialSelection *ui;
    MaterialDatabase *materialDatabase;
    QString *selectedMaterial;
};

#endif // MATERIALSELECTION_H
