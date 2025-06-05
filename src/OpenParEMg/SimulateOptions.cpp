#include "SimulateOptions.h"
#include "ui_SimulateOptions.h"


SimOptions::SimOptions(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SimOptions)
{
    ui->setupUi(this);
}

SimOptions::~SimOptions()
{
    delete ui;
}


void SimOptions::set_projData (struct projectData *a)
{
    projData=a;

    // Save projData to local variables to enable a cancel operation with no changes
    referenceImpedance=projData->reference_impedance;
    frequencyUnit=projData->touchstone_frequency_unit;
    cout << "frequencyUnit=" << frequencyUnit.toLatin1().data() << endl;

    // fill the panel with data
    ui->referenceImpedance->setValue(referenceImpedance);

    ui->frequencyUnit->addItem("Hz");
    ui->frequencyUnit->addItem("kHz");
    ui->frequencyUnit->addItem("MHz");
    ui->frequencyUnit->addItem("GHz");
    //int index=ui->frequencyUnit->findText(frequencyUnit.toLatin1().data(),Qt::MatchExactly);
    int index;
    if (strcmp(projData->touchstone_frequency_unit,"Hz") == 0) index=0;
    if (strcmp(projData->touchstone_frequency_unit,"kHz") == 0) index=1;
    if (strcmp(projData->touchstone_frequency_unit,"MHz") == 0) index=2;
    if (strcmp(projData->touchstone_frequency_unit,"GHz") == 0) index=3;
    cout << "index=" << index << endl;
    ui->frequencyUnit->setCurrentIndex(index);

}

void SimOptions::on_frequencyUnit_currentIndexChanged(int index)
{
    frequencyUnit=ui->frequencyUnit->currentText();
}

void SimOptions::on_referenceImpedance_textChanged(const QString &arg1)
{
    referenceImpedance=arg1.toDouble();
}

void SimOptions::on_simulateOptionOk_clicked()
{
    projData->reference_impedance=referenceImpedance;

    if (projData->touchstone_frequency_unit) free(projData->touchstone_frequency_unit);
    projData->touchstone_frequency_unit=(char *)malloc((frequencyUnit.length()+1)*sizeof(char));
    int i=0;
    while (i < frequencyUnit.length()) {
        projData->touchstone_frequency_unit[i]=frequencyUnit.data()[i].toLatin1();
        i++;
    }
    projData->touchstone_frequency_unit[i]='\0';


    close();
}

void SimOptions::on_simulateOptionCancel_clicked()
{
    close();
}




