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

#ifndef FREQUENCYPLANG_H
#define FREQUENCYPLANG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QScrollBar>
#include <QMessageBox>
#include "project.h"

extern "C" void init_project (struct projectData *);
extern "C" void free_project (struct projectData *);

namespace Ui {
class FrequencyPlanG;
}

class FrequencyPlanG : public QDialog
{
    Q_OBJECT

public:
    explicit FrequencyPlanG (QWidget *parent = nullptr);
    void set_projData (struct projectData *);
    void get_projData ();
    void set_simulationRunning (bool simulationRunning_) {simulationRunning=simulationRunning_;}
    ~FrequencyPlanG ();

private slots:
    void on_frequencyAdd_clicked ();
    void on_frequencyDelete_clicked ();
    void on_frequencyPlanGOk_clicked ();
    void on_frequencyPlanGCancel_clicked ();
    void typeComboBox_changed (int);
    void refine_checkStateChanged ();
    void frequency_textChanged ();
    void on_AMR_checkStateChanged (const Qt::CheckState &arg1);
    void on_adaptiveFrequencies_activated (int);
    bool check_inputs ();
    void on_planView_clicked ();

private:
    Ui::FrequencyPlanG *ui;
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
    bool simulationRunning;

    double conversionFactor;    // for converting frequencies between Hz and kHz, MHz, or GHz
};

#endif // FREQUENCYPLANG_H
