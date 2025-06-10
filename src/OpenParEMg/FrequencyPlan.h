#ifndef FREQUENCYPLAN_H
#define FREQUENCYPLAN_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QScrollBar>
#include "project.h"
#include <iostream>
#include "CustomLineEdit.h"

using namespace std;

namespace Ui {
class FrequencyPlan;
}

class FrequencyPlan : public QDialog
{
    Q_OBJECT

public:
    explicit FrequencyPlan(QWidget *parent = nullptr);
    void set_projData (struct projectData *);
    ~FrequencyPlan();

private slots:
    void on_frequencyAdd_clicked();

    void on_frequencyDelete_clicked();

    void on_frequencyPlanOk_clicked();

    void on_frequencyPlanCancel_clicked();

    void typeComboBox_changed(int);

private:
    Ui::FrequencyPlan *ui;
    struct projectData *projData;
    QString disabledBackground;
    QString enabledBackground;
};

#endif // FREQUENCYPLAN_H
