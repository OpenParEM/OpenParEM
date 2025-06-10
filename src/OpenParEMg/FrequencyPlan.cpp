#include "FrequencyPlan.h"
#include "ui_FrequencyPlan.h"

FrequencyPlan::FrequencyPlan(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FrequencyPlan)
{
    ui->setupUi(this);
    this->setFixedSize(695,212);

    // column setup

    ui->frequencyTable->insertColumn(0);  // type
    ui->frequencyTable->insertColumn(1);  // frequency or start
    ui->frequencyTable->insertColumn(2);  // stop
    ui->frequencyTable->insertColumn(3);  // step
    ui->frequencyTable->insertColumn(4);  // points per decade
    ui->frequencyTable->insertColumn(5);  // refine
    ui->frequencyTable->insertColumn(6);  // type before change (hidden)

    ui->frequencyTable->setColumnHidden(6,true);

    ui->frequencyTable->setColumnWidth(0,106);
    ui->frequencyTable->setColumnWidth(1,90);
    ui->frequencyTable->setColumnWidth(2,90);
    ui->frequencyTable->setColumnWidth(3,90);
    ui->frequencyTable->setColumnWidth(4,150);
    ui->frequencyTable->setColumnWidth(5,50);

    QStringList headers;
    headers << "Type" << "Start" << "Stop" << "Step" << "Points Per Decade" << "Refine" << "current type";
    ui->frequencyTable->setHorizontalHeaderLabels(headers);

    // number cell background colors
    enabledBackground="background: rgb(255,255,255);";
    disabledBackground="background: rgb(240,240,240);";

    ui->frequencyTable->setEnabled(true);
}

FrequencyPlan::~FrequencyPlan()
{
    delete ui;
}

void FrequencyPlan::set_projData (struct projectData *a)
{
    projData=a;

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
        connect(type,&QComboBox::currentIndexChanged,this,&FrequencyPlan::typeComboBox_changed);

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

        // frequencies - linear
        if (projData->inputFrequencyPlans[i].type == 0) {
            CustomLineEdit *start=new CustomLineEdit();
            start->setText(QString::number(projData->inputFrequencyPlans[i].start,'g'));
            start->setAlignment(Qt::AlignHCenter);
            start->setStyleSheet(enabledBackground);
            ui->frequencyTable->setCellWidget(i,1,start);

            CustomLineEdit *stop=new CustomLineEdit();
            stop->setText(QString::number(projData->inputFrequencyPlans[i].stop,'g'));
            stop->setAlignment(Qt::AlignHCenter);
            stop->setStyleSheet(enabledBackground);
            ui->frequencyTable->setCellWidget(i,2,stop);

            CustomLineEdit *step=new CustomLineEdit();
            step->setText(QString::number(projData->inputFrequencyPlans[i].step,'g'));
            step->setAlignment(Qt::AlignHCenter);
            step->setStyleSheet(enabledBackground);
            ui->frequencyTable->setCellWidget(i,3,step);

            QLineEdit *pointsPerDecade=new QLineEdit();
            pointsPerDecade->setText("");
            pointsPerDecade->setEnabled(false);
            pointsPerDecade->setAlignment(Qt::AlignHCenter);
            pointsPerDecade->setStyleSheet(disabledBackground);
            ui->frequencyTable->setCellWidget(i,4,pointsPerDecade);
        }

        // frequencies - log
        if (projData->inputFrequencyPlans[i].type == 1) {
            CustomLineEdit *start=new CustomLineEdit();
            start->setText(QString::number(projData->inputFrequencyPlans[i].start,'g'));
            start->setAlignment(Qt::AlignHCenter);
            start->setStyleSheet(enabledBackground);
            ui->frequencyTable->setCellWidget(i,1,start);

            CustomLineEdit *stop=new CustomLineEdit();
            stop->setText(QString::number(projData->inputFrequencyPlans[i].stop,'g'));
            stop->setAlignment(Qt::AlignHCenter);
            stop->setStyleSheet(enabledBackground);
            ui->frequencyTable->setCellWidget(i,2,stop);

            CustomLineEdit *step=new CustomLineEdit();
            step->setText("");
            step->setEnabled(false);
            step->setAlignment(Qt::AlignHCenter);
            step->setStyleSheet(disabledBackground);
            ui->frequencyTable->setCellWidget(i,3,step);

            QLineEdit *pointsPerDecade=new QLineEdit();
            pointsPerDecade->setText(QString::number(projData->inputFrequencyPlans[i].pointsPerDecade));
            pointsPerDecade->setAlignment(Qt::AlignHCenter);
            pointsPerDecade->setStyleSheet(enabledBackground);
            ui->frequencyTable->setCellWidget(i,4,pointsPerDecade);
        }

        // frequencies - frequency
        if (projData->inputFrequencyPlans[i].type == 2) {
            CustomLineEdit *start=new CustomLineEdit();
            start->setText(QString::number(projData->inputFrequencyPlans[i].frequency,'g'));
            start->setAlignment(Qt::AlignHCenter);
            start->setStyleSheet(enabledBackground);
            ui->frequencyTable->setCellWidget(i,1,start);

            CustomLineEdit *stop=new CustomLineEdit();
            stop->setText("");
            stop->setEnabled(false);
            stop->setAlignment(Qt::AlignHCenter);
            stop->setStyleSheet(disabledBackground);
            ui->frequencyTable->setCellWidget(i,2,stop);

            CustomLineEdit *step=new CustomLineEdit();
            step->setText("");
            step->setEnabled(false);
            step->setAlignment(Qt::AlignHCenter);
            step->setStyleSheet(disabledBackground);
            ui->frequencyTable->setCellWidget(i,3,step);

            QLineEdit *pointsPerDecade=new QLineEdit();
            pointsPerDecade->setText("");
            pointsPerDecade->setEnabled(false);
            pointsPerDecade->setAlignment(Qt::AlignHCenter);
            pointsPerDecade->setStyleSheet(disabledBackground);
            ui->frequencyTable->setCellWidget(i,4,pointsPerDecade);
        }

        i++;
    }

    int scrollBarWidth = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    if (ui->frequencyTable->rowCount() > 4) {
        ui->frequencyTable->setColumnWidth(4,150-scrollBarWidth);
    }
}

void FrequencyPlan::on_frequencyAdd_clicked()
{
    int currentRow=ui->frequencyTable->currentRow()+1;
    ui->frequencyTable->insertRow(currentRow);

    QComboBox *type=new QComboBox();
    type->addItem("linear");
    type->addItem("log");
    type->addItem("frequency");
    type->setCurrentIndex(2);
    ui->frequencyTable->setCellWidget(currentRow,0,type);
    connect(type,&QComboBox::currentIndexChanged,this,&FrequencyPlan::typeComboBox_changed);

    CustomLineEdit *start=new CustomLineEdit();
    start->setText(QString::number(1e9,'g'));
    start->setAlignment(Qt::AlignHCenter);
    start->setStyleSheet(enabledBackground);
    ui->frequencyTable->setCellWidget(currentRow,1,start);

    CustomLineEdit *stop=new CustomLineEdit();
    stop->setText("");
    stop->setEnabled(false);
    stop->setAlignment(Qt::AlignHCenter);
    stop->setStyleSheet(disabledBackground);
    ui->frequencyTable->setCellWidget(currentRow,2,stop);

    CustomLineEdit *step=new CustomLineEdit();
    step->setText("");
    step->setEnabled(false);
    step->setAlignment(Qt::AlignHCenter);
    step->setStyleSheet(disabledBackground);
    ui->frequencyTable->setCellWidget(currentRow,3,step);

    QLineEdit *pointsPerDecade=new QLineEdit();
    pointsPerDecade->setText("");
    pointsPerDecade->setEnabled(false);
    pointsPerDecade->setAlignment(Qt::AlignHCenter);
    pointsPerDecade->setStyleSheet(disabledBackground);
    ui->frequencyTable->setCellWidget(currentRow,4,pointsPerDecade);

    QWidget *checkBoxWidget = new QWidget();
    QCheckBox *checkBox = new QCheckBox();
    QHBoxLayout *layoutCheckBox = new QHBoxLayout(checkBoxWidget);
    layoutCheckBox->addWidget(checkBox);
    layoutCheckBox->setAlignment(Qt::AlignCenter);
    layoutCheckBox->setContentsMargins(0,0,0,0);
    checkBox->setChecked(Qt::Unchecked);
    checkBoxWidget->setStyleSheet(enabledBackground);
    ui->frequencyTable->setCellWidget(currentRow,5,checkBoxWidget);

    QLineEdit *currentRowWidget=new QLineEdit();
    currentRowWidget->setText(QString::number(2));
    currentRowWidget->setAlignment(Qt::AlignHCenter);
    currentRowWidget->setAlignment(Qt::AlignHCenter);
    ui->frequencyTable->setCellWidget(currentRow,6,currentRowWidget);

    int scrollBarWidth = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    if (ui->frequencyTable->rowCount() > 4) {
        ui->frequencyTable->setColumnWidth(4,150-scrollBarWidth);
    }
}


void FrequencyPlan::on_frequencyDelete_clicked()
{
    int currentRow=ui->frequencyTable->currentRow();
    ui->frequencyTable->removeRow(currentRow);

    if (ui->frequencyTable->rowCount() < 5) {
        ui->frequencyTable->setColumnWidth(4,150);
    }
}


void FrequencyPlan::on_frequencyPlanOk_clicked()
{
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
            projData->inputFrequencyPlans[i].start=start->text().toDouble();

            widget=ui->frequencyTable->cellWidget(i,2);
            CustomLineEdit *stop=qobject_cast<CustomLineEdit *>(widget);
            projData->inputFrequencyPlans[i].stop=stop->text().toDouble();

            widget=ui->frequencyTable->cellWidget(i,3);
            CustomLineEdit *step=qobject_cast<CustomLineEdit *>(widget);
            projData->inputFrequencyPlans[i].step=step->text().toDouble();

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
            projData->inputFrequencyPlans[i].start=start->text().toDouble();

            widget=ui->frequencyTable->cellWidget(i,2);
            CustomLineEdit *stop=qobject_cast<CustomLineEdit *>(widget);
            projData->inputFrequencyPlans[i].stop=stop->text().toDouble();

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
            projData->inputFrequencyPlans[i].frequency=frequency->text().toDouble();

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

    close();
}


void FrequencyPlan::on_frequencyPlanCancel_clicked()
{
    close();
}

void FrequencyPlan::typeComboBox_changed(int newIndex)
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
        }

        // frequency to frequency
        if (currentIndex == 2) {
            // nothing to do
        }
    }
}

