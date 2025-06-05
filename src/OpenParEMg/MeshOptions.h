#ifndef MESHOPTIONS_H
#define MESHOPTIONS_H

#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <iostream>
#include "project.h"

using namespace std;

namespace Ui {
class MeshDialog;
}

class MeshDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MeshDialog(QWidget *parent = nullptr);
    void set_projData (struct projectData *);
    ~MeshDialog();

private slots:
    void on_meshOrderSpinBox_valueChanged(int arg1);

    void on_meshFileLineEdit_returnPressed();

    void on_meshFilePushButton_clicked();

    void on_meshSaveRefined_checkStateChanged(const Qt::CheckState &arg1);

    void on_meshRefinementFraction_textChanged(const QString &arg1);

    void on_meshQualityLimit_textChanged(const QString &arg1);

    void on_meshOptionOk_clicked();

    void on_meshOptionCancel_clicked();

private:
    Ui::MeshDialog *ui;
    struct projectData *projData;
    int meshOrder;
    QString meshFile;
    int meshSaveRefined;
    double meshRefinementFraction;
    double meshQualityLimit;
};

#endif // MESHOPTIONS_H
