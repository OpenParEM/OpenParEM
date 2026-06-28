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

#include "FrequencyPlanG.h"
#include "ui_FrequencyPlanG.h"
#include "CustomLineEdit.h"
#include "FrequencyView.h"

FrequencyPlanG::FrequencyPlanG (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FrequencyPlanG)
{
    this->setWindowIcon(QApplication::windowIcon());

    ui->setupUi(this);
    this->setFixedWidth(695);

    doubleValidator.setBottom(0);
    intValidator.setBottom(0);

    // column setup

    ui->frequencyTable->insertColumn(0);  // type
    ui->frequencyTable->insertColumn(1);  // frequency or start
    ui->frequencyTable->insertColumn(2);  // stop
    ui->frequencyTable->insertColumn(3);  // step
    ui->frequencyTable->insertColumn(4);  // points per decade
    ui->frequencyTable->insertColumn(5);  // refine
    ui->frequencyTable->insertColumn(6);  // type before change (hidden)
    ui->frequencyTable->insertColumn(7);  // adaptive frequencies before change (hidden)

    ui->frequencyTable->setColumnHidden(6,true);
    ui->frequencyTable->setColumnHidden(7,true);

    frequencyBoxWidth=581; // from FrequencyPlanG.ui
    //frequencyBoxWidth=ui->frequencyTable->geometry().width(); // gives the wrong width
    verticalHeaderWidth=ui->frequencyTable->verticalHeader()->width();

    typeColWidth=96; // 106
    frequencyColWidth=87; // 90
    ppdColWidth=169; // 150
    refineColWidth=frequencyBoxWidth-typeColWidth-3*frequencyColWidth-ppdColWidth;
    scrollBarWidth=qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    scrollBarOffset=0;
    elasticColWidth=11;
    elasticColAdj=2;

    ui->frequencyTable->setColumnWidth(0,typeColWidth);
    ui->frequencyTable->setColumnWidth(1,frequencyColWidth);
    ui->frequencyTable->setColumnWidth(2,frequencyColWidth);
    ui->frequencyTable->setColumnWidth(3,frequencyColWidth);
    ui->frequencyTable->setColumnWidth(4,ppdColWidth-verticalHeaderWidth-scrollBarOffset);
    ui->frequencyTable->setColumnWidth(5,refineColWidth);

    //QStringList headers;
    //headers << "Type" << "Start" << "Stop" << "Step" << "Points Per Decade" << "Refine" << "current type";
    //ui->frequencyTable->setHorizontalHeaderLabels(headers);

    // number cell background colors
    enabledBackground="background: rgb(255,255,255);";
    disabledBackground="background: rgb(240,240,240);";

    ui->frequencyTable->setEnabled(true);
    ui->frequencyPlanGOk->setEnabled(false);
    ui->frequencyDelete->setEnabled(false);
    ui->AMR->setEnabled(true);
    ui->adaptiveFrequenciesLabel->setEnabled(false);
    ui->adaptiveFrequencies->setEnabled(false);

    conversionFactor=1;
}

FrequencyPlanG::~FrequencyPlanG ()
{
    delete ui;
}

void FrequencyPlanG::set_projData (struct projectData *a)
{
    projData=a;

    // frequency conversions
    if (strcmp(projData->touchstone_frequency_unit,"Hz") == 0) conversionFactor=1;
    else if (strcmp(projData->touchstone_frequency_unit,"kHz") == 0) conversionFactor=1e3;
    else if (strcmp(projData->touchstone_frequency_unit,"MHz") == 0) conversionFactor=1e6;
    else if (strcmp(projData->touchstone_frequency_unit,"GHz") == 0) conversionFactor=1e9;

    // headers
    QStringList headers;
    if (strcmp(projData->touchstone_frequency_unit,"Hz") == 0) {
        headers << "Type" << "Start, Hz" << "Stop, Hz" << "Step, Hz" << "Points Per Decade" << "Refine" << "current type";
    } else if (strcmp(projData->touchstone_frequency_unit,"kHz") == 0) {
        headers << "Type" << "Start, kHz" << "Stop, kHz" << "Step, kHz" << "Points Per Decade" << "Refine" << "current type";
    } else if (strcmp(projData->touchstone_frequency_unit,"MHz") == 0) {
        headers << "Type" << "Start, MHz" << "Stop, MHz" << "Step, MHz" << "Points Per Decade" << "Refine" << "current type";
    } else if (strcmp(projData->touchstone_frequency_unit,"GHz") == 0) {
        headers << "Type" << "Start, GHz" << "Stop, GHz" << "Step, GHz" << "Points Per Decade" << "Refine" << "current type";
    }
    ui->frequencyTable->setHorizontalHeaderLabels(headers);

    // AMR

    ui->adaptiveFrequencies->addItem("marked refinement frequencies");
    ui->adaptiveFrequencies->addItem("highest frequency");
    ui->adaptiveFrequencies->addItem("lowest frequency");
    ui->adaptiveFrequencies->addItem("highest then lowest frequency");
    ui->adaptiveFrequencies->addItem("lowest then highest frequency");
    ui->adaptiveFrequencies->addItem("all frequencies");

    int adaptiveIndex=-1;
    if (strcmp(projData->refinement_frequency,"none") == 0) {
        ui->AMR->setCheckState(Qt::Unchecked);
        ui->adaptiveFrequenciesLabel->setEnabled(false);
        ui->adaptiveFrequencies->setEnabled(false);
        enableRefineColumn=false;
    } else {
        ui->AMR->setCheckState(Qt::Checked);
        ui->adaptiveFrequenciesLabel->setEnabled(true);
        ui->adaptiveFrequencies->setEnabled(true);

        if (strcmp(projData->refinement_frequency,"plan") == 0) {
            ui->adaptiveFrequencies->setCurrentIndex(0);
            adaptiveIndex=0;
            enableRefineColumn=true;
        } else if (strcmp(projData->refinement_frequency,"high") == 0) {
            ui->adaptiveFrequencies->setCurrentIndex(1);
            adaptiveIndex=1;
            enableRefineColumn=false;
        } else if (strcmp(projData->refinement_frequency,"low") == 0) {
            ui->adaptiveFrequencies->setCurrentIndex(2);
            adaptiveIndex=2;
            enableRefineColumn=false;
        } else if (strcmp(projData->refinement_frequency,"highlow") == 0) {
            ui->adaptiveFrequencies->setCurrentIndex(3);
            adaptiveIndex=3;
            enableRefineColumn=false;
        } else if (strcmp(projData->refinement_frequency,"lowhigh") == 0) {
            ui->adaptiveFrequencies->setCurrentIndex(4);
            adaptiveIndex=4;
            enableRefineColumn=false;
        } else if (strcmp(projData->refinement_frequency,"all") == 0) {
            ui->adaptiveFrequencies->setCurrentIndex(5);
            adaptiveIndex=5;
            enableRefineColumn=false;
        }
    }

    // frequencies
    int i=0;
    while (i < projData->inputFrequencyPlansCount) {

        ui->frequencyTable->insertRow(i);

        // type
        QComboBox *type=new QComboBox();
        type->addItem("linear");
        type->addItem("log");
        type->addItem("frequency");

        int index=0;
        if (projData->inputFrequencyPlans[i].type == 0) index=0;
        if (projData->inputFrequencyPlans[i].type == 1) index=1;
        if (projData->inputFrequencyPlans[i].type == 2) index=2;
        type->setCurrentIndex(index);

        ui->frequencyTable->setCellWidget(i,0,type);
        connect(type,&QComboBox::currentIndexChanged,this,&FrequencyPlanG::typeComboBox_changed);

        QLineEdit *currentType=new QLineEdit();
        currentType->setText(QString::number(index));
        currentType->setAlignment(Qt::AlignHCenter);
        ui->frequencyTable->setCellWidget(i,6,currentType);

        QWidget *checkBoxWidget = new QWidget();
        QCheckBox *checkBox = new QCheckBox();
        QHBoxLayout *layoutCheckBox = new QHBoxLayout(checkBoxWidget);
        layoutCheckBox->addWidget(checkBox);
        layoutCheckBox->setAlignment(Qt::AlignCenter);
        layoutCheckBox->setContentsMargins(0,0,0,0);
        if (projData->inputFrequencyPlans[i].refine) checkBox->setChecked(Qt::Checked);
        else checkBox->setChecked(Qt::Unchecked);
        checkBoxWidget->setStyleSheet(enabledBackground);
        ui->frequencyTable->setCellWidget(i,5,checkBoxWidget);
        connect(checkBox,&QCheckBox::checkStateChanged,this,&FrequencyPlanG::refine_checkStateChanged);

        QLineEdit *currentAdaptiveFrequencies=new QLineEdit();
        currentAdaptiveFrequencies->setText(QString::number(adaptiveIndex));
        currentAdaptiveFrequencies->setAlignment(Qt::AlignHCenter);
        ui->frequencyTable->setCellWidget(i,7,currentAdaptiveFrequencies);

        // frequencies - linear
        if (projData->inputFrequencyPlans[i].type == 0) {
            CustomLineEdit *start=new CustomLineEdit();
            start->setText(QString::number(projData->inputFrequencyPlans[i].start/conversionFactor,'g'));
            start->setAlignment(Qt::AlignHCenter);
            start->setStyleSheet(enabledBackground);
            start->setValidator(&doubleValidator);
            ui->frequencyTable->setCellWidget(i,1,start);
            connect(start,&CustomLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

            CustomLineEdit *stop=new CustomLineEdit();
            stop->setText(QString::number(projData->inputFrequencyPlans[i].stop/conversionFactor,'g'));
            stop->setAlignment(Qt::AlignHCenter);
            stop->setStyleSheet(enabledBackground);
            stop->setValidator(&doubleValidator);
            ui->frequencyTable->setCellWidget(i,2,stop);
            connect(stop,&CustomLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

            CustomLineEdit *step=new CustomLineEdit();
            step->setText(QString::number(projData->inputFrequencyPlans[i].step/conversionFactor,'g'));
            step->setAlignment(Qt::AlignHCenter);
            step->setStyleSheet(enabledBackground);
            step->setValidator(&doubleValidator);
            ui->frequencyTable->setCellWidget(i,3,step);
            connect(step,&CustomLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

            QLineEdit *pointsPerDecade=new QLineEdit();
            pointsPerDecade->setText("");
            pointsPerDecade->setEnabled(false);
            pointsPerDecade->setAlignment(Qt::AlignHCenter);
            pointsPerDecade->setStyleSheet(disabledBackground);
            pointsPerDecade->setValidator(&intValidator);
            ui->frequencyTable->setCellWidget(i,4,pointsPerDecade);
            connect(pointsPerDecade,&QLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);
        }

        // frequencies - log
        if (projData->inputFrequencyPlans[i].type == 1) {
            CustomLineEdit *start=new CustomLineEdit();
            start->setText(QString::number(projData->inputFrequencyPlans[i].start/conversionFactor,'g'));
            start->setAlignment(Qt::AlignHCenter);
            start->setStyleSheet(enabledBackground);
            start->setValidator(&doubleValidator);
            ui->frequencyTable->setCellWidget(i,1,start);
            connect(start,&CustomLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

            CustomLineEdit *stop=new CustomLineEdit();
            stop->setText(QString::number(projData->inputFrequencyPlans[i].stop/conversionFactor,'g'));
            stop->setAlignment(Qt::AlignHCenter);
            stop->setStyleSheet(enabledBackground);
            stop->setValidator(&doubleValidator);
            ui->frequencyTable->setCellWidget(i,2,stop);
            connect(stop,&CustomLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

            CustomLineEdit *step=new CustomLineEdit();
            step->setText("");
            step->setEnabled(false);
            step->setAlignment(Qt::AlignHCenter);
            step->setStyleSheet(disabledBackground);
            step->setValidator(&doubleValidator);
            ui->frequencyTable->setCellWidget(i,3,step);
            connect(step,&CustomLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

            QLineEdit *pointsPerDecade=new QLineEdit();
            pointsPerDecade->setText(QString::number(projData->inputFrequencyPlans[i].pointsPerDecade));
            pointsPerDecade->setAlignment(Qt::AlignHCenter);
            pointsPerDecade->setStyleSheet(enabledBackground);
            pointsPerDecade->setValidator(&intValidator);
            ui->frequencyTable->setCellWidget(i,4,pointsPerDecade);
            connect(pointsPerDecade,&QLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);
        }

        // frequencies - frequency
        if (projData->inputFrequencyPlans[i].type == 2) {
            CustomLineEdit *start=new CustomLineEdit();
            start->setText(QString::number(projData->inputFrequencyPlans[i].frequency/conversionFactor,'g'));
            start->setAlignment(Qt::AlignHCenter);
            start->setStyleSheet(enabledBackground);
            start->setValidator(&doubleValidator);
            ui->frequencyTable->setCellWidget(i,1,start);
            connect(start,&CustomLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

            CustomLineEdit *stop=new CustomLineEdit();
            stop->setText("");
            stop->setEnabled(false);
            stop->setAlignment(Qt::AlignHCenter);
            stop->setStyleSheet(disabledBackground);
            stop->setValidator(&doubleValidator);
            ui->frequencyTable->setCellWidget(i,2,stop);
            connect(stop,&CustomLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

            CustomLineEdit *step=new CustomLineEdit();
            step->setText("");
            step->setEnabled(false);
            step->setAlignment(Qt::AlignHCenter);
            step->setStyleSheet(disabledBackground);
            step->setValidator(&doubleValidator);
            ui->frequencyTable->setCellWidget(i,3,step);
            connect(step,&CustomLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

            QLineEdit *pointsPerDecade=new QLineEdit();
            pointsPerDecade->setText("");
            pointsPerDecade->setEnabled(false);
            pointsPerDecade->setAlignment(Qt::AlignHCenter);
            pointsPerDecade->setStyleSheet(disabledBackground);
            pointsPerDecade->setValidator(&intValidator);
            ui->frequencyTable->setCellWidget(i,4,pointsPerDecade);
            connect(pointsPerDecade,&QLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);
        }

        ui->frequencyDelete->setEnabled(true);

        i++;
    }

    ui->frequencyTable->scrollToBottom();  // trick to refresh the vertical header so that
    ui->frequencyTable->scrollToTop();     // verticalHeader()->width() does not return 0
    verticalHeaderWidth=ui->frequencyTable->verticalHeader()->width();

    if (ui->frequencyTable->rowCount() > 4) {
        scrollBarOffset=scrollBarWidth;
    }

    if (enableRefineColumn) {
        ui->frequencyTable->setColumnWidth(0,typeColWidth);
        ui->frequencyTable->setColumnWidth(1,frequencyColWidth);
        ui->frequencyTable->setColumnWidth(2,frequencyColWidth);
        ui->frequencyTable->setColumnWidth(3,frequencyColWidth);
        ui->frequencyTable->setColumnWidth(4,ppdColWidth-verticalHeaderWidth-scrollBarOffset);
        ui->frequencyTable->setColumnWidth(5,refineColWidth);

        ui->frequencyTable->setColumnHidden(5,false);
    } else {
        ui->frequencyTable->setColumnWidth(0,typeColWidth+elasticColWidth+elasticColAdj);
        ui->frequencyTable->setColumnWidth(1,frequencyColWidth+elasticColWidth);
        ui->frequencyTable->setColumnWidth(2,frequencyColWidth+elasticColWidth);
        ui->frequencyTable->setColumnWidth(3,frequencyColWidth+elasticColWidth);
        ui->frequencyTable->setColumnWidth(4,ppdColWidth-verticalHeaderWidth-scrollBarOffset+elasticColWidth);

        ui->frequencyTable->setColumnHidden(5,true);
    }

    if (simulationRunning) {
        ui->AMR->setEnabled(false);
        ui->adaptiveFrequencies->setEnabled(false);
        ui->frequencyAdd->setEnabled(false);
        ui->frequencyDelete->setEnabled(false);
        ui->frequencyTable->setEnabled(false);
    }

    ui->frequencyPlanGOk->setEnabled(false);
}

void FrequencyPlanG::on_frequencyAdd_clicked ()
{
    int currentRow=ui->frequencyTable->currentRow()+1;
    ui->frequencyTable->insertRow(currentRow);

    QComboBox *type=new QComboBox();
    type->addItem("linear");
    type->addItem("log");
    type->addItem("frequency");
    type->setCurrentIndex(2);
    ui->frequencyTable->setCellWidget(currentRow,0,type);
    connect(type,&QComboBox::currentIndexChanged,this,&FrequencyPlanG::typeComboBox_changed);

    CustomLineEdit *start=new CustomLineEdit();
    start->setText(QString::number(1e9/conversionFactor,'g'));
    start->setAlignment(Qt::AlignHCenter);
    start->setStyleSheet(enabledBackground);
    start->setValidator(&doubleValidator);
    ui->frequencyTable->setCellWidget(currentRow,1,start);
    connect(start,&CustomLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

    CustomLineEdit *stop=new CustomLineEdit();
    stop->setText("");
    stop->setEnabled(false);
    stop->setAlignment(Qt::AlignHCenter);
    stop->setStyleSheet(disabledBackground);
    stop->setValidator(&doubleValidator);
    ui->frequencyTable->setCellWidget(currentRow,2,stop);
    connect(stop,&CustomLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

    CustomLineEdit *step=new CustomLineEdit();
    step->setText("");
    step->setEnabled(false);
    step->setAlignment(Qt::AlignHCenter);
    step->setStyleSheet(disabledBackground);
    step->setValidator(&doubleValidator);
    ui->frequencyTable->setCellWidget(currentRow,3,step);
    connect(step,&CustomLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

    QLineEdit *pointsPerDecade=new QLineEdit();
    pointsPerDecade->setText("");
    pointsPerDecade->setEnabled(false);
    pointsPerDecade->setAlignment(Qt::AlignHCenter);
    pointsPerDecade->setStyleSheet(disabledBackground);
    pointsPerDecade->setValidator(&intValidator);
    ui->frequencyTable->setCellWidget(currentRow,4,pointsPerDecade);
    connect(pointsPerDecade,&QLineEdit::textChanged,this,&FrequencyPlanG::frequency_textChanged);

    QWidget *checkBoxWidget = new QWidget();
    QCheckBox *checkBox = new QCheckBox();
    QHBoxLayout *layoutCheckBox = new QHBoxLayout(checkBoxWidget);
    layoutCheckBox->addWidget(checkBox);
    layoutCheckBox->setAlignment(Qt::AlignCenter);
    layoutCheckBox->setContentsMargins(0,0,0,0);
    checkBox->setChecked(Qt::Unchecked);
    checkBoxWidget->setStyleSheet(enabledBackground);
    ui->frequencyTable->setCellWidget(currentRow,5,checkBoxWidget);
    connect(checkBox,&QCheckBox::checkStateChanged,this,&FrequencyPlanG::refine_checkStateChanged);

    QLineEdit *currentRowWidget=new QLineEdit();
    currentRowWidget->setText(QString::number(2));
    currentRowWidget->setAlignment(Qt::AlignHCenter);
    ui->frequencyTable->setCellWidget(currentRow,6,currentRowWidget);

    QLineEdit *currentAdaptiveFrequencies=new QLineEdit();
    currentAdaptiveFrequencies->setText(QString::number(1));
    currentAdaptiveFrequencies->setAlignment(Qt::AlignHCenter);
    ui->frequencyTable->setCellWidget(currentRow,7,currentAdaptiveFrequencies);

    ui->frequencyTable->scrollToBottom();  // trick to refresh the vertical header so that
    ui->frequencyTable->scrollToTop();     // verticalHeader()->width() does not return 0
    verticalHeaderWidth=ui->frequencyTable->verticalHeader()->width();

    if (ui->frequencyTable->rowCount() > 4) {
        scrollBarOffset=scrollBarWidth;
    }

    if (enableRefineColumn) {
        ui->frequencyTable->setColumnWidth(0,typeColWidth);
        ui->frequencyTable->setColumnWidth(1,frequencyColWidth);
        ui->frequencyTable->setColumnWidth(2,frequencyColWidth);
        ui->frequencyTable->setColumnWidth(3,frequencyColWidth);
        ui->frequencyTable->setColumnWidth(4,ppdColWidth-verticalHeaderWidth-scrollBarOffset);
        ui->frequencyTable->setColumnWidth(5,refineColWidth);

        ui->frequencyTable->setColumnHidden(5,false);
    } else {
        ui->frequencyTable->setColumnWidth(0,typeColWidth+elasticColWidth+elasticColAdj);
        ui->frequencyTable->setColumnWidth(1,frequencyColWidth+elasticColWidth);
        ui->frequencyTable->setColumnWidth(2,frequencyColWidth+elasticColWidth);
        ui->frequencyTable->setColumnWidth(3,frequencyColWidth+elasticColWidth);
        ui->frequencyTable->setColumnWidth(4,ppdColWidth-verticalHeaderWidth-scrollBarOffset+elasticColWidth);

        ui->frequencyTable->setColumnHidden(5,true);
    }

    ui->frequencyTable->selectRow(currentRow);
    ui->frequencyDelete->setEnabled(true);
    projData->modified=1;
    ui->frequencyPlanGOk->setEnabled(true);
    ui->AMR->setEnabled(true);
    ui->adaptiveFrequenciesLabel->setEnabled(true);
    ui->adaptiveFrequencies->setEnabled(true);
}

void FrequencyPlanG::on_frequencyDelete_clicked ()
{
    int currentRow=ui->frequencyTable->currentRow();
    ui->frequencyTable->removeRow(currentRow);

    ui->frequencyTable->scrollToBottom();  // trick to refresh the vertical header so that
    ui->frequencyTable->scrollToTop();     // verticalHeader()->width() does not return 0
    verticalHeaderWidth=ui->frequencyTable->verticalHeader()->width();

    if (ui->frequencyTable->rowCount() < 5) {
        scrollBarOffset=0;
    }

    if (enableRefineColumn) {
        ui->frequencyTable->setColumnWidth(0,typeColWidth);
        ui->frequencyTable->setColumnWidth(1,frequencyColWidth);
        ui->frequencyTable->setColumnWidth(2,frequencyColWidth);
        ui->frequencyTable->setColumnWidth(3,frequencyColWidth);
        ui->frequencyTable->setColumnWidth(4,ppdColWidth-verticalHeaderWidth-scrollBarOffset);
        ui->frequencyTable->setColumnWidth(5,refineColWidth);

        ui->frequencyTable->setColumnHidden(5,false);
    } else {
        ui->frequencyTable->setColumnWidth(0,typeColWidth+elasticColWidth+elasticColAdj);
        ui->frequencyTable->setColumnWidth(1,frequencyColWidth+elasticColWidth);
        ui->frequencyTable->setColumnWidth(2,frequencyColWidth+elasticColWidth);
        ui->frequencyTable->setColumnWidth(3,frequencyColWidth+elasticColWidth);
        ui->frequencyTable->setColumnWidth(4,ppdColWidth-verticalHeaderWidth-scrollBarOffset+elasticColWidth);

        ui->frequencyTable->setColumnHidden(5,true);
    }

    if (ui->frequencyTable->rowCount() == 0) {
        ui->frequencyDelete->setEnabled(false);
        ui->AMR->setEnabled(false);
        ui->adaptiveFrequenciesLabel->setEnabled(false);
        ui->adaptiveFrequencies->setEnabled(false);
    }

    projData->modified=1;
    ui->frequencyPlanGOk->setEnabled(true);
}

void FrequencyPlanG::on_frequencyPlanGOk_clicked ()
{
    // checks
    if (check_inputs()) return;

    get_projData();

    close();
}

void FrequencyPlanG::get_projData ()
{
    // AMR
    if (projData->refinement_frequency) free(projData->refinement_frequency);
    if (ui->AMR->isChecked()) {
        if (ui->adaptiveFrequencies->currentIndex() == 0) {
            projData->refinement_frequency=(char *) malloc (5*sizeof(char));
            sprintf(projData->refinement_frequency,"plan");
        } else if (ui->adaptiveFrequencies->currentIndex() == 1) {
            projData->refinement_frequency=(char *) malloc (5*sizeof(char));
            sprintf(projData->refinement_frequency,"high");
        } else if (ui->adaptiveFrequencies->currentIndex() == 2) {
            projData->refinement_frequency=(char *) malloc (4*sizeof(char));
            sprintf(projData->refinement_frequency,"low");
        } else if (ui->adaptiveFrequencies->currentIndex() == 3) {
            projData->refinement_frequency=(char *) malloc (8*sizeof(char));
            sprintf(projData->refinement_frequency,"highlow");
        } else if (ui->adaptiveFrequencies->currentIndex() == 4) {
            projData->refinement_frequency=(char *) malloc (8*sizeof(char));
            sprintf(projData->refinement_frequency,"lowhigh");
        } else if (ui->adaptiveFrequencies->currentIndex() == 5) {
            projData->refinement_frequency=(char *) malloc (4*sizeof(char));
            sprintf(projData->refinement_frequency,"all");
        }
    } else {
        projData->refinement_frequency=(char *) malloc (5*sizeof(char));
        sprintf(projData->refinement_frequency,"none");
    }

    // frequencies

    if (projData->inputFrequencyPlans) free(projData->inputFrequencyPlans);

    projData->inputFrequencyPlansAllocated=ui->frequencyTable->rowCount();
    projData->inputFrequencyPlansCount=ui->frequencyTable->rowCount();

    projData->inputFrequencyPlans=(struct inputFrequencyPlan *) malloc(projData->inputFrequencyPlansAllocated*sizeof(struct inputFrequencyPlan));

    int i=0;
    while (i < ui->frequencyTable->rowCount()) {
        QWidget *widget=ui->frequencyTable->cellWidget(i,0);
        QComboBox *comboBox=qobject_cast<QComboBox *>(widget);

        widget=ui->frequencyTable->cellWidget(i,5);
        QCheckBox *refine=widget->findChild<QCheckBox *>();

        int index=comboBox->currentIndex();

        // linear
        if (index == 0) {
            projData->inputFrequencyPlans[i].type=0;

            QWidget *widget=ui->frequencyTable->cellWidget(i,1);
            CustomLineEdit *start=qobject_cast<CustomLineEdit *>(widget);
            projData->inputFrequencyPlans[i].start=start->text().toDouble()*conversionFactor;

            widget=ui->frequencyTable->cellWidget(i,2);
            CustomLineEdit *stop=qobject_cast<CustomLineEdit *>(widget);
            projData->inputFrequencyPlans[i].stop=stop->text().toDouble()*conversionFactor;

            widget=ui->frequencyTable->cellWidget(i,3);
            CustomLineEdit *step=qobject_cast<CustomLineEdit *>(widget);
            projData->inputFrequencyPlans[i].step=step->text().toDouble()*conversionFactor;

            if (refine->isChecked()) projData->inputFrequencyPlans[i].refine=1;
            else projData->inputFrequencyPlans[i].refine=0;

            projData->inputFrequencyPlans[i].frequency=0;
            projData->inputFrequencyPlans[i].pointsPerDecade=0;
            projData->inputFrequencyPlans[i].lineNumber=0;
        }

        // log
        if (index == 1) {
            projData->inputFrequencyPlans[i].type=1;

            QWidget *widget=ui->frequencyTable->cellWidget(i,1);
            CustomLineEdit *start=qobject_cast<CustomLineEdit *>(widget);
            projData->inputFrequencyPlans[i].start=start->text().toDouble()*conversionFactor;

            widget=ui->frequencyTable->cellWidget(i,2);
            CustomLineEdit *stop=qobject_cast<CustomLineEdit *>(widget);
            projData->inputFrequencyPlans[i].stop=stop->text().toDouble()*conversionFactor;

            widget=ui->frequencyTable->cellWidget(i,4);
            QLineEdit *pointsPerDecade=qobject_cast<QLineEdit *>(widget);
            projData->inputFrequencyPlans[i].pointsPerDecade=pointsPerDecade->text().toInt();

            if (refine->isChecked()) projData->inputFrequencyPlans[i].refine=1;
            else projData->inputFrequencyPlans[i].refine=0;

            projData->inputFrequencyPlans[i].frequency=0;
            projData->inputFrequencyPlans[i].step=0;
            projData->inputFrequencyPlans[i].lineNumber=0;
        }

        // point
        if (index == 2) {
            projData->inputFrequencyPlans[i].type=2;

            QWidget *widget=ui->frequencyTable->cellWidget(i,1);
            CustomLineEdit *frequency=qobject_cast<CustomLineEdit *>(widget);
            projData->inputFrequencyPlans[i].frequency=frequency->text().toDouble()*conversionFactor;

            if (refine->isChecked()) projData->inputFrequencyPlans[i].refine=1;
            else projData->inputFrequencyPlans[i].refine=0;

            projData->inputFrequencyPlans[i].start=0;
            projData->inputFrequencyPlans[i].stop=0;
            projData->inputFrequencyPlans[i].step=0;
            projData->inputFrequencyPlans[i].pointsPerDecade=0;
            projData->inputFrequencyPlans[i].lineNumber=0;
        }

        i++;
    }
}

void FrequencyPlanG::on_frequencyPlanGCancel_clicked ()
{
    close();
}

void FrequencyPlanG::typeComboBox_changed (int newIndex)
{
    int currentRow=ui->frequencyTable->currentRow();
    QLineEdit *currentRowWidget=(QLineEdit *) ui->frequencyTable->cellWidget(currentRow,6);
    int currentIndex=currentRowWidget->text().toInt();

    // linear
    if (newIndex == 0) {

        // linear to linear
        if (currentIndex == 0) {
            // nothing to do
        }

        // log to linear
        if (currentIndex == 1) {
            currentRowWidget->setText(QString::number(newIndex));

            CustomLineEdit *start=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,1);
            double startValue=start->text().toDouble();

            CustomLineEdit *stop=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,2);
            double stopValue=stop->text().toDouble();

            CustomLineEdit *step=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,3);
            step->setText(QString::number((stopValue-startValue)/10,'g'));
            step->setStyleSheet(enabledBackground);
            step->setEnabled(true);

            QLineEdit *pointsPerDecade=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,4);
            pointsPerDecade->setText("");
            pointsPerDecade->setStyleSheet(disabledBackground);
            pointsPerDecade->setEnabled(false);

            projData->modified=1;
            ui->frequencyPlanGOk->setEnabled(true);
        }

        // frequency to linear
        if (currentIndex == 2) {
            currentRowWidget->setText(QString::number(newIndex));

            CustomLineEdit *start=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,1);
            double startValue=start->text().toDouble();

            CustomLineEdit *stop=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,2);
            double stopValue=startValue*2;
            stop->setText(QString::number(stopValue,'g'));
            stop->setStyleSheet("enabledBackground");
            stop->setEnabled(true);

            CustomLineEdit *step=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,3);
            step->setText(QString::number((stopValue-startValue)/10,'g'));
            step->setStyleSheet("enabledBackground");
            step->setEnabled(true);

            QLineEdit *pointsPerDecade=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,4);
            pointsPerDecade->setText("");
            pointsPerDecade->setStyleSheet(disabledBackground);
            pointsPerDecade->setEnabled(false);

            projData->modified=1;
            ui->frequencyPlanGOk->setEnabled(true);
        }
    }

    // log
    if (newIndex == 1) {

        // linear to log
        if (currentIndex == 0) {
            currentRowWidget->setText(QString::number(newIndex));

            CustomLineEdit *step=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,3);
            step->setText("");
            step->setStyleSheet(disabledBackground);
            step->setEnabled(false);

            QLineEdit *pointsPerDecade=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,4);
            pointsPerDecade->setText(QString::number(10));
            pointsPerDecade->setStyleSheet(enabledBackground);
            pointsPerDecade->setEnabled(true);

            projData->modified=1;
            ui->frequencyPlanGOk->setEnabled(true);
        }

        // log to log
        if (currentIndex == 1) {
            // nothing to do
        }

        // frequency to log
        if (currentIndex == 2) {
            currentRowWidget->setText(QString::number(newIndex));

            CustomLineEdit *start=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,1);
            double startValue=start->text().toDouble();

            CustomLineEdit *stop=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,2);
            double stopValue=startValue*2;
            stop->setText(QString::number(stopValue,'g'));
            stop->setStyleSheet(enabledBackground);
            stop->setEnabled(true);

            CustomLineEdit *step=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,3);
            step->setText("");
            step->setStyleSheet(disabledBackground);
            step->setEnabled(false);

            QLineEdit *pointsPerDecade=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,4);
            pointsPerDecade->setText(QString::number(10));
            pointsPerDecade->setStyleSheet(enabledBackground);
            pointsPerDecade->setEnabled(true);

            projData->modified=1;
            ui->frequencyPlanGOk->setEnabled(true);
        }
    }

    // frequency
    if (newIndex == 2) {

        // linear to frequency
        if (currentIndex == 0) {
            currentRowWidget->setText(QString::number(newIndex));

            CustomLineEdit *stop=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,2);
            stop->setText("");
            stop->setStyleSheet(disabledBackground);
            stop->setEnabled(false);

            CustomLineEdit *step=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,3);
            step->setText("");
            step->setStyleSheet(disabledBackground);
            step->setEnabled(false);

            QLineEdit *pointsPerDecade=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,4);
            pointsPerDecade->setText("");
            pointsPerDecade->setStyleSheet(disabledBackground);
            pointsPerDecade->setEnabled(false);

            projData->modified=1;
            ui->frequencyPlanGOk->setEnabled(true);
        }

        // log to frequency
        if (currentIndex == 1) {
            currentRowWidget->setText(QString::number(newIndex));

            CustomLineEdit *stop=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,2);
            stop->setText("");
            stop->setStyleSheet(disabledBackground);
            stop->setEnabled(false);

            CustomLineEdit *step=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,3);
            step->setText("");
            step->setStyleSheet(disabledBackground);
            step->setEnabled(false);

            QLineEdit *pointsPerDecade=(CustomLineEdit *) ui->frequencyTable->cellWidget(currentRow,4);
            pointsPerDecade->setText("");
            pointsPerDecade->setStyleSheet(disabledBackground);
            pointsPerDecade->setEnabled(false);

            projData->modified=1;
            ui->frequencyPlanGOk->setEnabled(true);
        }

        // frequency to frequency
        if (currentIndex == 2) {
            // nothing to do
        }
    }
}

void FrequencyPlanG::on_AMR_checkStateChanged (const Qt::CheckState &arg1)
{
    // nothing to do if there is no data
    if (ui->frequencyTable->rowCount() == 0) return;

    int currentRow=ui->frequencyTable->currentRow();
    if (currentRow < 0) currentRow=0;
    QLineEdit *currentRowWidget=(QLineEdit *) ui->frequencyTable->cellWidget(currentRow,7);

    ui->frequencyTable->scrollToBottom();  // trick to refresh the vertical header so that
    ui->frequencyTable->scrollToTop();     // verticalHeader()->width() does not return 0
    verticalHeaderWidth=ui->frequencyTable->verticalHeader()->width();

    if (arg1 == Qt::Checked) {
        ui->adaptiveFrequenciesLabel->setEnabled(true);
        ui->adaptiveFrequencies->setEnabled(true);
    } else {
        ui->adaptiveFrequenciesLabel->setEnabled(false);
        ui->adaptiveFrequencies->setEnabled(false);
    }

    ui->adaptiveFrequencies->setCurrentIndex(1); // high
    currentRowWidget->setText(QString::number(1));

    ui->frequencyTable->setColumnWidth(0,typeColWidth+elasticColWidth+elasticColAdj);
    ui->frequencyTable->setColumnWidth(1,frequencyColWidth+elasticColWidth);
    ui->frequencyTable->setColumnWidth(2,frequencyColWidth+elasticColWidth);
    ui->frequencyTable->setColumnWidth(3,frequencyColWidth+elasticColWidth);
    ui->frequencyTable->setColumnWidth(4,ppdColWidth-verticalHeaderWidth-scrollBarOffset+elasticColWidth);

    ui->frequencyTable->setColumnHidden(5,true);
    enableRefineColumn=false;

    int i=0;
    while (i < ui->frequencyTable->rowCount()) {
        QWidget *widget=ui->frequencyTable->cellWidget(i,5);
        QCheckBox *refine=widget->findChild<QCheckBox *>();
        refine->setChecked(false);
        i++;
    }

    projData->modified=1;
    ui->frequencyPlanGOk->setEnabled(true);
}

void FrequencyPlanG::on_adaptiveFrequencies_activated (int newIndex)
{
    // nothing to do if there is no data
    if (ui->frequencyTable->rowCount() == 0) return;

    int currentRow=ui->frequencyTable->currentRow();
    if (currentRow < 0) currentRow=0;
    QLineEdit *currentRowWidget=(QLineEdit *) ui->frequencyTable->cellWidget(currentRow,7);
    int currentIndex=currentRowWidget->text().toInt();

    // nothing to do
    if (newIndex == currentIndex) return;

    // update

    ui->frequencyTable->scrollToBottom();  // trick to refresh the vertical header so that
    ui->frequencyTable->scrollToTop();     // verticalHeader()->width() does not return 0
    verticalHeaderWidth=ui->frequencyTable->verticalHeader()->width();

    if (newIndex == 0) { // plan
        ui->frequencyTable->setColumnWidth(0,typeColWidth);
        ui->frequencyTable->setColumnWidth(1,frequencyColWidth);
        ui->frequencyTable->setColumnWidth(2,frequencyColWidth);
        ui->frequencyTable->setColumnWidth(3,frequencyColWidth);
        ui->frequencyTable->setColumnWidth(4,ppdColWidth-verticalHeaderWidth-scrollBarOffset);
        ui->frequencyTable->setColumnWidth(5,refineColWidth);

        ui->frequencyTable->setColumnHidden(5,false);
        enableRefineColumn=true;
    } else {
        int i=0;
        while (i < ui->frequencyTable->rowCount()) {
            QWidget *widget=ui->frequencyTable->cellWidget(i,5);
            QCheckBox *refine=widget->findChild<QCheckBox *>();
            refine->setChecked(false);
            i++;
        }

        ui->frequencyTable->setColumnWidth(0,typeColWidth+elasticColWidth+elasticColAdj);
        ui->frequencyTable->setColumnWidth(1,frequencyColWidth+elasticColWidth);
        ui->frequencyTable->setColumnWidth(2,frequencyColWidth+elasticColWidth);
        ui->frequencyTable->setColumnWidth(3,frequencyColWidth+elasticColWidth);
        ui->frequencyTable->setColumnWidth(4,ppdColWidth-verticalHeaderWidth-scrollBarOffset+elasticColWidth);

        ui->frequencyTable->setColumnHidden(5,true);
        enableRefineColumn=false;
    }

    currentRowWidget->setText(QString::number(newIndex));
    projData->modified=1;
    ui->frequencyPlanGOk->setEnabled(true);
}

void FrequencyPlanG::refine_checkStateChanged ()
{
    projData->modified=1;
    ui->frequencyDelete->setEnabled(true);
    ui->frequencyPlanGOk->setEnabled(true);
}

void FrequencyPlanG::frequency_textChanged ()
{
    projData->modified=1;
    ui->frequencyDelete->setEnabled(true);
    ui->frequencyPlanGOk->setEnabled(true);
}

bool FrequencyPlanG::check_inputs ()
{
    // must have at least one frequency row
    if (ui->frequencyTable->rowCount() == 0) {
        QMessageBox mb;
        mb.critical(nullptr, "Error", "Must have at least one frequency row.");
        mb.setFixedSize(500, 200);
        return true;
    }

    // must have a refine box checked for plan refinement_frequency
    if (ui->AMR->isChecked() && ui->adaptiveFrequencies->currentIndex() == 0) {
        bool found=false;
        int i=0;
        while (i < ui->frequencyTable->rowCount()) {
            QWidget *widget=ui->frequencyTable->cellWidget(i,5);
            QCheckBox *refine=widget->findChild<QCheckBox *>();
            if (refine->isChecked()) {found=true; break;}
            i++;
        }

        if (!found) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "At least one \"Refine\" box must be checked.");
            mb.setFixedSize(500, 200);
            return true;
        }
    }

    // must have start frequency != 0
    int i=0;
    while (i < ui->frequencyTable->rowCount()) {
        QWidget *widget=ui->frequencyTable->cellWidget(i,0);
        QComboBox *type=qobject_cast<QComboBox *>(widget);
        int currentType=type->currentIndex();

        if (currentType == 0 || currentType == 1 || currentType == 2) { // linear, log, frequency
            QWidget *widget=ui->frequencyTable->cellWidget(i,1);
            CustomLineEdit *start=qobject_cast<CustomLineEdit *>(widget);
            double startFrequency=start->text().toDouble();

            if (startFrequency == 0) {
                QMessageBox mb;
                QString message;
                message="The start frequency cannot be 0 at row "+QString::number(i+1)+".";
                mb.critical(nullptr, "Error",message);
                return true;
            }
        }
        i++;
    }

    // must have stop frequency != 0
    i=0;
    while (i < ui->frequencyTable->rowCount()) {
        QWidget *widget=ui->frequencyTable->cellWidget(i,0);
        QComboBox *type=qobject_cast<QComboBox *>(widget);
        int currentType=type->currentIndex();

        if (currentType == 0 || currentType == 1) { // linear and log
            QWidget *widget=ui->frequencyTable->cellWidget(i,2);
            CustomLineEdit *stop=qobject_cast<CustomLineEdit *>(widget);
            double stopFrequency=stop->text().toDouble();

            if (stopFrequency == 0) {
                QMessageBox mb;
                QString message;
                message="The stop frequency cannot be 0 at row "+QString::number(i+1)+".";
                mb.critical(nullptr, "Error",message);
                return true;
            }
        }
        i++;
    }

    // must have step frequency != 0
    i=0;
    while (i < ui->frequencyTable->rowCount()) {
        QWidget *widget=ui->frequencyTable->cellWidget(i,0);
        QComboBox *type=qobject_cast<QComboBox *>(widget);
        int currentType=type->currentIndex();

        if (currentType == 0) { // linear
            QWidget *widget=ui->frequencyTable->cellWidget(i,3);
            CustomLineEdit *step=qobject_cast<CustomLineEdit *>(widget);
            double stepFrequency=step->text().toDouble();

            if (stepFrequency == 0) {
                QMessageBox mb;
                QString message;
                message="The step frequency cannot be 0 at row "+QString::number(i+1)+".";
                mb.critical(nullptr, "Error",message);
                return true;
            }
        }
        i++;
    }

    // must have stop frequency greater than start frequency
    i=0;
    while (i < ui->frequencyTable->rowCount()) {
        QWidget *widget=ui->frequencyTable->cellWidget(i,0);
        QComboBox *type=qobject_cast<QComboBox *>(widget);
        int currentType=type->currentIndex();

        if (currentType == 0) { // linear
            QWidget *widget=ui->frequencyTable->cellWidget(i,1);
            CustomLineEdit *start=qobject_cast<CustomLineEdit *>(widget);
            double startFrequency=start->text().toDouble();

            widget=ui->frequencyTable->cellWidget(i,2);
            CustomLineEdit *stop=qobject_cast<CustomLineEdit *>(widget);
            double stopFrequency=stop->text().toDouble();

            if (startFrequency >= stopFrequency) {
                QMessageBox mb;
                QString message;
                message="The stop frequency must be greater than the start frequency at row "+QString::number(i+1)+".";
                mb.critical(nullptr, "Error",message);
                return true;
            }
        }
        i++;
    }

    // must have points-per-decade != 0
    i=0;
    while (i < ui->frequencyTable->rowCount()) {
        QWidget *widget=ui->frequencyTable->cellWidget(i,0);
        QComboBox *type=qobject_cast<QComboBox *>(widget);
        int currentType=type->currentIndex();

        if (currentType == 1) { // log
            QWidget *widget=ui->frequencyTable->cellWidget(i,4);
            QLineEdit *pointsPerDecade=qobject_cast<QLineEdit *>(widget);
            int ppd=pointsPerDecade->text().toInt();

            if (ppd == 0) {
                QMessageBox mb;
                QString message;
                message="The points-per-decade cannot be 0 at row "+QString::number(i+1)+".";
                mb.critical(nullptr, "Error",message);
                return true;
            }
        }
        i++;
    }

    return false;
}

void FrequencyPlanG::on_planView_clicked ()
{
    if (check_inputs()) return;

    // frequency unit
    std::string frequency_unit=projData->touchstone_frequency_unit;

    // save the existing pointer
    struct projectData *saveProjData=projData;

    // temp data holder
    struct projectData tempProjData;
    init_project (&tempProjData);
    projData=&tempProjData;

    // init_project sets GHz as default, so align the frequency unit to the current data
    if (projData->touchstone_frequency_unit) free(projData->touchstone_frequency_unit);
    projData->touchstone_frequency_unit=(char *)malloc((frequency_unit.length()+1)*sizeof(char));
    sprintf(projData->touchstone_frequency_unit,"%s",frequency_unit.c_str());

    // populate the temporary project data with data from the form
    get_projData();

    // view it
    FrequencyView *view=new FrequencyView();
    view->populate(projData);
    view->exec();
    delete view;

    // clean up and restore the original project data pointer
    free_project(&tempProjData);
    projData=saveProjData;
}
