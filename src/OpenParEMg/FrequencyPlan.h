#ifndef FREQUENCYPLAN_H
#define FREQUENCYPLAN_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QScrollBar>
#include <QMessageBox>
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

    void refine_checkStateChanged();

    void frequency_textChanged();

    void on_AMR_checkStateChanged(const Qt::CheckState &arg1);

    void on_adaptiveFrequencies_activated(int index);

    bool check_inputs();

private:
    Ui::FrequencyPlan *ui;
    struct projectData *projData;
    QString disabledBackground;
    QString enabledBackground;
    QDoubleValidator doubleValidator;
    QIntValidator intValidator;
    bool enableRefineColumn;
    int scrollBarWidth;
    int verticalHeaderWidth;
    int frequencyBoxWidth;
    int typeColWidth;
    int frequencyColWidth;
    int ppdColWidth;
    int refineColWidth;
    int scrollBarOffset;
    int elasticColWidth;
    int elasticColAdj;
};

#endif // FREQUENCYPLAN_H
