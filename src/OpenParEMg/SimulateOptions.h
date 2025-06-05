#ifndef SIMULATEOPTIONS_H
#define SIMULATEOPTIONS_H

#include <QDialog>
#include <iostream>
#include "project.h"
#include "string.h"

using namespace std;

namespace Ui {
class SimOptions;
}

class SimOptions : public QDialog
{
    Q_OBJECT

public:
    explicit SimOptions(QWidget *parent = nullptr);
    void set_projData (struct projectData *);
    ~SimOptions();

private slots:
    void on_referenceImpedance_textChanged(const QString &arg1);

    void on_simulateOptionCancel_clicked();

    void on_simulateOptionOk_clicked();

    void on_frequencyUnit_currentIndexChanged(int index);

private:
    Ui::SimOptions *ui;
    struct projectData *projData;
    double referenceImpedance;
    QString frequencyUnit;
};

#endif // SIMULATEOPTIONS_H
