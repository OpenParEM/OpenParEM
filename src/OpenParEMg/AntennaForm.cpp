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

#include "AntennaForm.h"
#include "misc.hpp"
#include "project.h"
#include "ui_AntennaForm.h"
#include <qtimer.h>

AntennaForm::AntennaForm(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AntennaForm)
{
    this->setWindowIcon(QApplication::windowIcon());

    ui->setupUi(this);

    this->setFixedWidth(757);
    this->setFixedHeight(743);

    // 3D patterns
    patternG3D=false;
    patternD3D=false;
    patternEtheta3D=false;
    patternEphi3D=false;
    patternHtheta3D=false;
    patternHphi3D=false;

    // currentResolution
    ui->currentResolution->setMinimum(0.01);
    ui->currentResolution->setMaximum(0.15);
    ui->currentResolution->setDecimals(2);
    ui->currentResolution->setSingleStep(0.01);

    // plotRange2D
    ui->plotRange2D->setMinimum(0.1);
    ui->plotRange2D->setMaximum(200);
    ui->plotRange2D->setDecimals(1);
    ui->plotRange2D->setSingleStep(5);

    // axisInterval2D
    ui->axisInterval2D->setMinimum(0.1);
    ui->axisInterval2D->setMaximum(200);
    ui->axisInterval2D->setDecimals(1);
    ui->axisInterval2D->setSingleStep(5);

    // plotResolution2D
    ui->plotResolution2D->setMinimum(0.1);
    ui->plotResolution2D->setMaximum(16);
    ui->plotResolution2D->setDecimals(1);
    ui->plotResolution2D->setSingleStep(0.1);

    // plotResolution3D
    ui->plotResolution3D->addItem("very coarse (15.9 deg)");
    ui->plotResolution3D->addItem("coarse (7.83 deg)");
    ui->plotResolution3D->addItem("medium (3.96 deg)");
    ui->plotResolution3D->addItem("fine (1.98 deg)");
    ui->plotResolution3D->addItem("very fine (0.991 deg)");


    // 2D pattern table

    ui->patternTable->insertColumn(0);       // pattern
    ui->patternTable->insertColumn(1);       // plane
    ui->patternTable->insertColumn(2);       // theta
    ui->patternTable->insertColumn(3);       // phi
    ui->patternTable->insertColumn(4);       // latitude
    ui->patternTable->insertColumn(5);       // rotation

    QStringList headers;
    headers << "Quantity" << "Plane" << "Theta\ndeg" << "Phi\ndeg" << "Latitude\ndeg" << "Rotation\ndeg";
    ui->patternTable->setHorizontalHeaderLabels(headers);


    patternBoxWidth=621;
    scrollBarWidth=qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    scrollBarOffset=0;
    verticalHeaderWidth=ui->patternTable->verticalHeader()->width();

    quantityWidth=125;
    thetaWidth=90;
    phiWidth=90;
    latitudeWidth=90;
    rotationWidth=90;

    planeWidth=patternBoxWidth-scrollBarOffset-verticalHeaderWidth-
               quantityWidth-thetaWidth-phiWidth-latitudeWidth-rotationWidth;

    ui->patternTable->setColumnWidth(0,quantityWidth);
    ui->patternTable->setColumnWidth(1,planeWidth);
    ui->patternTable->setColumnWidth(2,thetaWidth);
    ui->patternTable->setColumnWidth(3,phiWidth);
    ui->patternTable->setColumnWidth(4,latitudeWidth);
    ui->patternTable->setColumnWidth(5,rotationWidth);
}

AntennaForm::~AntennaForm()
{
    delete ui;
}


void AntennaForm::check3Dpatterns ()
{
    if (patternG3D) return;
    if (patternD3D) return;
    if (patternEtheta3D) return;
    if (patternEphi3D) return;
    if (patternHtheta3D) return;
    if (patternHphi3D) return;

    // must have one pattern
    patternG3D=true;
    ui->patternG3D->setChecked(true);
}

void AntennaForm::set_projData (struct projectData *a)
{
    projData=a;

    // currentResolution
    currentResolution=projData->antenna_plot_current_resolution;
    ui->currentResolution->setValue(currentResolution);

    // plotRange2D
    plotRange2D=projData->antenna_plot_2D_range;
    ui->plotRange2D->setValue(plotRange2D);

    // axisInterval2D
    axisInterval2D=projData->antenna_plot_2D_interval;
    ui->axisInterval2D->setValue(axisInterval2D);

    // plotResolution2D
    plotResolution2D=projData->antenna_plot_2D_resolution;
    ui->plotResolution2D->setValue(plotResolution2D);

    // dataSummary2D
    dataSummary2D=projData->antenna_plot_2D_annotations;
    ui->dataSummary2D->setChecked(dataSummary2D);

    // savePlots2D
    savePlots2D=projData->antenna_plot_2D_save;
    ui->savePlots2D->setChecked(savePlots2D);

    // plotResolution3D
    plotResolution3D=projData->antenna_plot_3D_refinement-2;
    ui->plotResolution3D->setCurrentIndex(plotResolution3D);

    // generateSphere
    generateSphere=projData->antenna_plot_3D_sphere;
    ui->generateSphere->setChecked(generateSphere);

    // savePlots3D
    savePlots3D=projData->antenna_plot_3D_save;
    ui->savePlots3D->setChecked(savePlots3D);

    // saveRawData
    saveRawData=projData->antenna_plot_raw_save;
    ui->saveRawData->setChecked(saveRawData);

    // disable while a simulation is running
    if (simulationRunning) {

        ui->patternG3D->setEnabled(false);
        ui->patternD3D->setEnabled(false);
        ui->patternEtheta3D->setEnabled(false);
        ui->patternEphi3D->setEnabled(false);
        ui->patternHtheta3D->setEnabled(false);
        ui->patternHphi3D->setEnabled(false);

        ui->currentResolution->setEnabled(false);
        ui->plotRange2D->setEnabled(false);
        ui->axisInterval2D->setEnabled(false);
        ui->plotResolution2D->setEnabled(false);
        ui->dataSummary2D->setEnabled(false);
        ui->savePlots2D->setEnabled(false);
        ui->plotResolution3D->setEnabled(false);
        ui->generateSphere->setEnabled(false);
        ui->savePlots3D->setEnabled(false);
        ui->saveRawData->setEnabled(false);

        ui->patternTable->setEnabled(false);
        ui->add2Dslice->setEnabled(false);
        ui->delete2Dslice->setEnabled(false);
    }

    // set here - appendPattern may set to true
    ui->OkButton->setEnabled(false);

    // 2D

    ui->delete2Dslice->setEnabled(false);
    long unsigned int i=0;
    while (i < projData->inputAntennaPatternsCount) {
        if (projData->inputAntennaPatterns[i].dim == 2) {
            appendPattern(&(projData->inputAntennaPatterns[i]));
            ui->delete2Dslice->setEnabled(true);
        }
        i++;
    }

    ui->patternTable->scrollToBottom();  // trick to refresh the vertical header so that
    ui->patternTable->scrollToTop();     // verticalHeader()->width() does not return 0
    verticalHeaderWidth=ui->patternTable->verticalHeader()->width();
    scrollBarOffset=0;
    if (ui->patternTable->rowCount() > 4) scrollBarOffset=scrollBarWidth;
    planeWidth=patternBoxWidth-scrollBarOffset-verticalHeaderWidth-
                 quantityWidth-thetaWidth-phiWidth-latitudeWidth-rotationWidth;
    ui->patternTable->setColumnWidth(1,planeWidth);

    // 3D
    i=0;
    while (i < projData->inputAntennaPatternsCount) {
        if (projData->inputAntennaPatterns[i].dim == 3) {
            if (projData->inputAntennaPatterns[i].quantity1) {
                if (strcmp(projData->inputAntennaPatterns[i].quantity1,"G") == 0) ui->patternG3D->setChecked(true);
                else if (strcmp(projData->inputAntennaPatterns[i].quantity1,"D") == 0) ui->patternD3D->setChecked(true);
                else if (strcmp(projData->inputAntennaPatterns[i].quantity1,"Etheta") == 0) ui->patternEtheta3D->setChecked(true);
                else if (strcmp(projData->inputAntennaPatterns[i].quantity1,"Ephi") == 0) ui->patternEphi3D->setChecked(true);
                else if (strcmp(projData->inputAntennaPatterns[i].quantity1,"Htheta") == 0) ui->patternHtheta3D->setChecked(true);
                else if (strcmp(projData->inputAntennaPatterns[i].quantity1,"Hphi") == 0) ui->patternHphi3D->setChecked(true);
            }
        }
        i++;
    }

    // make sure at least one 3D pattern is enabled
    check3Dpatterns();
}

void AntennaForm::on_patternG3D_stateChanged (int arg1)
{
    patternG3D=false;
    if (arg1 == 2) patternG3D=true;

    QTimer::singleShot(0, this, [this]() {
        check3Dpatterns();
    });

    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_patternD3D_stateChanged (int arg1)
{
    patternD3D=false;
    if (arg1 == 2) patternD3D=true;
    check3Dpatterns();
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_patternEtheta3D_stateChanged (int arg1)
{
    patternEtheta3D=false;
    if (arg1 == 2) patternEtheta3D=true;
    check3Dpatterns();
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_patternEphi3D_stateChanged (int arg1)
{
    patternEphi3D=false;
    if (arg1 == 2) patternEphi3D=true;
    check3Dpatterns();
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_patternHtheta3D_stateChanged (int arg1)    // new pattern

{
    patternHtheta3D=false;
    if (arg1 == 2) patternHtheta3D=true;
    check3Dpatterns();
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_patternHphi3D_stateChanged (int arg1)
{
    patternHphi3D=false;
    if (arg1 == 2) patternHphi3D=true;
    check3Dpatterns();
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_plotResolution3D_currentIndexChanged (int index)
{
    plotResolution3D=index;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_generateSphere_stateChanged (int arg1)
{
    generateSphere=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_savePlots3D_stateChanged (int arg1)
{
    savePlots3D=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::appendPattern (struct inputAntennaPattern *pattern)
{
    int rowPosition=ui->patternTable->rowCount();
    ui->patternTable->insertRow(rowPosition);
    ui->patternTable->setCurrentCell(rowPosition,0);
    int currentRow=ui->patternTable->currentRow();

    QComboBox *quantityBox=new QComboBox();
    quantityBox->addItem("G");
    quantityBox->addItem("D");
    quantityBox->addItem("Etheta");
    quantityBox->addItem("Ephi");
    quantityBox->addItem("Htheta");
    quantityBox->addItem("Hphi");
    quantityBox->addItem("Etheta+Ephi");
    quantityBox->addItem("Ephi+Etheta");
    quantityBox->addItem("Htheta+Hphi");
    quantityBox->addItem("Hphi+Htheta");

    if (pattern->quantity1) {
        if (strcmp(pattern->quantity1,"G") == 0) quantityBox->setCurrentIndex(0);
        else if (strcmp(pattern->quantity1,"D") == 0) quantityBox->setCurrentIndex(1);
        else if (strcmp(pattern->quantity1,"Etheta") == 0) {
            quantityBox->setCurrentIndex(2);
            if (pattern->quantity2 && strcmp(pattern->quantity2,"Ephi") == 0) {
                quantityBox->setCurrentIndex(6);
            }
        } else if (strcmp(pattern->quantity1,"Ephi") == 0) {
            quantityBox->setCurrentIndex(3);
            if (pattern->quantity2 && strcmp(pattern->quantity2,"Etheta") == 0) {
                quantityBox->setCurrentIndex(7);
            }
        } else if (strcmp(pattern->quantity1,"Htheta") == 0) {
            quantityBox->setCurrentIndex(4);
            if (pattern->quantity2 && strcmp(pattern->quantity2,"Hphi") == 0) {
                quantityBox->setCurrentIndex(8);
            }
        } else if (strcmp(pattern->quantity1,"Hphi") == 0) {
            quantityBox->setCurrentIndex(5);
            if (pattern->quantity2 && strcmp(pattern->quantity2,"Htheta") == 0) {
                quantityBox->setCurrentIndex(9);
            }
        }
    }
    ui->patternTable->setCellWidget(currentRow,0,quantityBox);
    connect(quantityBox,&QComboBox::currentIndexChanged,this,&AntennaForm::quantityBox_changed);

    QComboBox *planeBox=new QComboBox();
    planeBox->addItem("xy");
    planeBox->addItem("xz");
    planeBox->addItem("yz");
    planeBox->addItem("specify");
    if (pattern->plane) {
        if (strcmp(pattern->plane,"xy") == 0) planeBox->setCurrentIndex(0);
        else if (strcmp(pattern->plane,"xz") == 0) planeBox->setCurrentIndex(1);
        else if (strcmp(pattern->plane,"yz") == 0) planeBox->setCurrentIndex(2);
    } else {
        planeBox->setCurrentIndex(3);
    }
    ui->patternTable->setCellWidget(currentRow,1,planeBox);
    connect(planeBox,&QComboBox::currentIndexChanged,this,&AntennaForm::planeBox_changed);

    QDoubleSpinBox *thetaBox = new QDoubleSpinBox();
    thetaBox->setMinimum(-180);
    thetaBox->setMaximum(180);
    thetaBox->setValue(pattern->theta);
    thetaBox->setDecimals(0);
    thetaBox->setSingleStep(5);

    // use a container to keep the background selection from the table from
    // bleeding around the edges of the widget and making it look like box highlighting
    // during row or column selection
    QWidget *container=new QWidget(ui->patternTable);
    QHBoxLayout *layout=new QHBoxLayout(container);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    thetaBox->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    container->setAutoFillBackground(true);
    container->setPalette(thetaBox->palette());
    layout->addWidget(thetaBox);
    // layout->setAlignment(thetaBox,Qt::AlignCenter);

    ui->patternTable->setCellWidget(currentRow,2,container);
    connect(thetaBox,&QDoubleSpinBox::valueChanged,this,&AntennaForm::thetaBox_changed);

    QDoubleSpinBox *phiBox = new QDoubleSpinBox();
    phiBox->setMinimum(-180);
    phiBox->setMaximum(180);
    phiBox->setValue(pattern->phi);
    phiBox->setDecimals(0);
    phiBox->setSingleStep(5);

    container=new QWidget(ui->patternTable);
    layout=new QHBoxLayout(container);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    phiBox->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    container->setAutoFillBackground(true);
    container->setPalette(phiBox->palette());
    layout->addWidget(phiBox);
    // layout->setAlignment(phiBox,Qt::AlignCenter);

    ui->patternTable->setCellWidget(currentRow,3,container);
    connect(phiBox,&QDoubleSpinBox::valueChanged,this,&AntennaForm::phiBox_changed);


    if (pattern->plane) {

        // disable theta and phi

        QWidget *container=ui->patternTable->cellWidget(currentRow,2);
        if (container) {
            QDoubleSpinBox *thetaBox=container->findChild<QDoubleSpinBox*>();
            if (thetaBox) thetaBox->setEnabled(false);
        }

        container=ui->patternTable->cellWidget(currentRow,3);
        if (container) {
            QDoubleSpinBox *phiBox=container->findChild<QDoubleSpinBox*>();
            if (phiBox) phiBox->setEnabled(false);
        }
    }

    QDoubleSpinBox *latitudeBox = new QDoubleSpinBox();
    latitudeBox->setMinimum(-90);
    latitudeBox->setMaximum(90);
    latitudeBox->setValue(pattern->latitude);
    latitudeBox->setDecimals(0);
    latitudeBox->setSingleStep(5);

    container=new QWidget(ui->patternTable);
    layout=new QHBoxLayout(container);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    latitudeBox->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    container->setAutoFillBackground(true);
    container->setPalette(latitudeBox->palette());
    layout->addWidget(latitudeBox);
    // layout->setAlignment(latitudeBox,Qt::AlignCenter);

    ui->patternTable->setCellWidget(currentRow,4,container);
    connect(latitudeBox,&QDoubleSpinBox::valueChanged,this,&AntennaForm::latitudeBox_changed);


    QDoubleSpinBox *rotationBox = new QDoubleSpinBox();
    rotationBox->setMinimum(-360);
    rotationBox->setMaximum(360);
    rotationBox->setValue(pattern->rotation);
    rotationBox->setDecimals(0);
    rotationBox->setSingleStep(5);

    container=new QWidget(ui->patternTable);
    layout=new QHBoxLayout(container);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    rotationBox->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    container->setAutoFillBackground(true);
    container->setPalette(rotationBox->palette());
    layout->addWidget(rotationBox);
    // layout->setAlignment(rotationBox,Qt::AlignCenter);

    ui->patternTable->setCellWidget(currentRow,5,container);
    connect(rotationBox,&QDoubleSpinBox::valueChanged,this,&AntennaForm::rotationBox_changed);
}

void AntennaForm::on_add2Dslice_clicked ()
{
    struct inputAntennaPattern pattern;

    pattern.lineNumber=0;
    pattern.dim=2;

    pattern.quantity1=(char *)malloc(2*sizeof(char));
    sprintf(pattern.quantity1,"%s","G");

    pattern.quantity2=nullptr;

    pattern.plane=(char *)malloc(3*sizeof(char));
    sprintf(pattern.plane,"%s","xy");

    pattern.theta=0;
    pattern.phi=0;
    pattern.latitude=0;
    pattern.rotation=0;

    appendPattern(&pattern);

    if (pattern.quantity1) {free(pattern.quantity1); pattern.quantity1=nullptr;}
    if (pattern.plane) {free(pattern.plane); pattern.plane=nullptr;}

    ui->patternTable->scrollToBottom();  // trick to refresh the vertical header so that
    ui->patternTable->scrollToTop();     // verticalHeader()->width() does not return 0
    verticalHeaderWidth=ui->patternTable->verticalHeader()->width();
    scrollBarOffset=0;
    if (ui->patternTable->rowCount() > 4) scrollBarOffset=scrollBarWidth;
    planeWidth=patternBoxWidth-scrollBarOffset-verticalHeaderWidth-
                 quantityWidth-thetaWidth-phiWidth-latitudeWidth-rotationWidth;
    ui->patternTable->setColumnWidth(1,planeWidth);

    ui->delete2Dslice->setEnabled(true);

    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_delete2Dslice_clicked ()
{
    ui->patternTable->removeRow(ui->patternTable->currentRow());

    ui->patternTable->scrollToBottom();  // trick to refresh the vertical header so that
    ui->patternTable->scrollToTop();     // verticalHeader()->width() does not return 0
    verticalHeaderWidth=ui->patternTable->verticalHeader()->width();
    scrollBarOffset=0;
    if (ui->patternTable->rowCount() > 4) scrollBarOffset=scrollBarWidth;
    planeWidth=patternBoxWidth-scrollBarOffset-verticalHeaderWidth-
                 quantityWidth-thetaWidth-phiWidth-latitudeWidth-rotationWidth;
    ui->patternTable->setColumnWidth(1,planeWidth);

    if (ui->patternTable->rowCount() == 0) ui->delete2Dslice->setEnabled(false);

    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_plotRange2D_valueChanged (double arg1)
{
    plotRange2D=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_axisInterval2D_valueChanged (double arg1)
{
    axisInterval2D=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_plotResolution2D_valueChanged (double arg1)
{
    plotResolution2D=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_dataSummary2D_stateChanged (int arg1)
{
    dataSummary2D=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_savePlots2D_stateChanged (int arg1)
{
    savePlots2D=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_currentResolution_valueChanged (double arg1)
{
    currentResolution=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::on_saveRawData_stateChanged (int arg1)
{
    saveRawData=arg1;
    ui->OkButton->setEnabled(true);
}

void AntennaForm::extractPatterns ()
{

    // 3D

    if (ui->patternG3D->isChecked()) {
        inputAntennaPattern *pattern=new inputAntennaPattern;
        pattern->lineNumber=0;
        pattern->dim=3;
        pattern->quantity1=nullptr;
        pattern->quantity2=nullptr;
        pattern->plane=nullptr;
        QString text="G"; cstrFromQString(&(pattern->quantity1),text);
        patterns.push_back(pattern);
    }

    if (ui->patternD3D->isChecked()) {
        inputAntennaPattern *pattern=new inputAntennaPattern;
        pattern->lineNumber=0;
        pattern->dim=3;
        pattern->quantity1=nullptr;
        pattern->quantity2=nullptr;
        pattern->plane=nullptr;
        QString text="D"; cstrFromQString(&(pattern->quantity1),text);
        patterns.push_back(pattern);
    }

    if (ui->patternEtheta3D->isChecked()) {
        inputAntennaPattern *pattern=new inputAntennaPattern;
        pattern->lineNumber=0;
        pattern->dim=3;
        pattern->quantity1=nullptr;
        pattern->quantity2=nullptr;
        pattern->plane=nullptr;
        QString text="Etheta"; cstrFromQString(&(pattern->quantity1),text);
        patterns.push_back(pattern);
    }

    if (ui->patternEphi3D->isChecked()) {
        inputAntennaPattern *pattern=new inputAntennaPattern;
        pattern->lineNumber=0;
        pattern->dim=3;
        pattern->quantity1=nullptr;
        pattern->quantity2=nullptr;
        pattern->plane=nullptr;
        QString text="Ephi"; cstrFromQString(&(pattern->quantity1),text);
        patterns.push_back(pattern);
    }

    if (ui->patternHtheta3D->isChecked()) {
        inputAntennaPattern *pattern=new inputAntennaPattern;
        pattern->lineNumber=0;
        pattern->dim=3;
        pattern->quantity1=nullptr;
        pattern->quantity2=nullptr;
        pattern->plane=nullptr;
        QString text="Htheta"; cstrFromQString(&(pattern->quantity1),text);
        patterns.push_back(pattern);
    }

    if (ui->patternHphi3D->isChecked()) {
        inputAntennaPattern *pattern=new inputAntennaPattern;
        pattern->lineNumber=0;
        pattern->dim=3;
        pattern->quantity1=nullptr;
        pattern->quantity2=nullptr;
        pattern->plane=nullptr;
        QString text="Hphi"; cstrFromQString(&(pattern->quantity1),text);
        patterns.push_back(pattern);
    }

    // 2D

    int i=0;
    while (i < ui->patternTable->rowCount()) {

        inputAntennaPattern *pattern=new inputAntennaPattern;

        pattern->lineNumber=0;
        pattern->dim=2;
        pattern->quantity1=nullptr;
        pattern->quantity2=nullptr;
        pattern->plane=nullptr;

        QComboBox* quantity1Box=qobject_cast<QComboBox*>(ui->patternTable->cellWidget(i,0));
        QString currentText=quantity1Box->currentText();
        if (currentText.compare("G") == 0) cstrFromQString (&(pattern->quantity1),currentText);
        else if (currentText.compare("D") == 0) cstrFromQString (&(pattern->quantity1),currentText);
        else if (currentText.compare("Etheta") == 0) cstrFromQString (&(pattern->quantity1),currentText);
        else if (currentText.compare("Ephi") == 0) cstrFromQString (&(pattern->quantity1),currentText);
        else if (currentText.compare("Htheta") == 0) cstrFromQString (&(pattern->quantity1),currentText);
        else if (currentText.compare("Hphi") == 0) cstrFromQString (&(pattern->quantity1),currentText);
        else if (currentText.compare("Etheta+Ephi") == 0) {
            QString text="Etheta";
            cstrFromQString (&(pattern->quantity1),text);
            text="Ephi";
            cstrFromQString (&(pattern->quantity2),text);
        } else if (currentText.compare("Ephi+Etheta") == 0) {
            QString text="Ephi";
            cstrFromQString (&(pattern->quantity1),text);
            text="Etheta";
            cstrFromQString (&(pattern->quantity2),text);
        } else if (currentText.compare("Htheta+Hphi") == 0) {
            QString text="Htheta";
            cstrFromQString (&(pattern->quantity1),text);
            text="Hphi";
            cstrFromQString (&(pattern->quantity2),text);
        } else if (currentText.compare("Hphi+Htheta") == 0) {
            QString text="Hphi";
            cstrFromQString (&(pattern->quantity1),text);
            text="Htheta";
            cstrFromQString (&(pattern->quantity2),text);
        }

        QComboBox* planeBox=qobject_cast<QComboBox*>(ui->patternTable->cellWidget(i,1));
        currentText=planeBox->currentText();
        if (currentText.compare("specify") == 0) pattern->plane=nullptr;
        else cstrFromQString (&(pattern->plane),currentText);

        QWidget *container=ui->patternTable->cellWidget(i,2);
        QDoubleSpinBox *thetaBox = container ? container->findChild<QDoubleSpinBox*>() : nullptr;
        if (thetaBox) {
            pattern->theta=thetaBox->value();
        }

        container=ui->patternTable->cellWidget(i,3);
        QDoubleSpinBox *phiBox = container ? container->findChild<QDoubleSpinBox*>() : nullptr;
        if (phiBox) {
            pattern->phi=phiBox->value();
        }

        container=ui->patternTable->cellWidget(i,4);
        QDoubleSpinBox *latitudeBox = container ? container->findChild<QDoubleSpinBox*>() : nullptr;
        if (latitudeBox) {
            pattern->latitude=latitudeBox->value();
        }

        container=ui->patternTable->cellWidget(i,5);
        QDoubleSpinBox *rotationBox = container ? container->findChild<QDoubleSpinBox*>() : nullptr;
        if (rotationBox) {
            pattern->rotation=rotationBox->value();
        }

        patterns.push_back(pattern);
        i++;
    }
}

bool AntennaForm::hasPatternChanges ()
{
    std::cout << "AntennaForm::hasPatternChanges" << std::endl; std::cout.flush();

    std::cout << "patterns.size()=" << patterns.size() << "  projData->inputAntennaPatternsCount=" << projData->inputAntennaPatternsCount << std::endl; std::cout.flush();
    if (patterns.size() != projData->inputAntennaPatternsCount) return true;

    // 3D
    long unsigned int i=0;
    while (i < patterns.size()) {
        if (patterns[i]->dim == 3) {
            bool foundMatch=false;
            int j=0;
            while (j < projData->inputAntennaPatternsCount) {
                if (projData->inputAntennaPatterns[j].dim == 3) {

                    if (patterns[i]->quantity1) {
                        if (projData->inputAntennaPatterns[j].quantity1) {
                            if (strcmp(patterns[i]->quantity1,projData->inputAntennaPatterns[j].quantity1) != 0) {j++; continue;}
                        } else {j++; continue;};
                    } else {
                        if (projData->inputAntennaPatterns[j].quantity1) {j++; continue;};
                    }

                    foundMatch=true;
                    break;
                }
                j++;
            }
            if (!foundMatch) return true;
        }
        i++;
    }

    // 2D
    i=0;
    while (i < patterns.size()) {
        if (patterns[i]->dim == 2) {
            bool foundMatch=false;
            int j=0;
            while (j < projData->inputAntennaPatternsCount) {
                if (projData->inputAntennaPatterns[j].dim == 2) {

                    if (patterns[i]->quantity1) {
                        if (projData->inputAntennaPatterns[j].quantity1) {
                            if (strcmp(patterns[i]->quantity1,projData->inputAntennaPatterns[j].quantity1) != 0) {j++; continue;}
                        } else {j++; continue;};
                    } else {
                        if (projData->inputAntennaPatterns[j].quantity1) {j++; continue;};
                    }

                    if (patterns[i]->quantity2) {
                        if (projData->inputAntennaPatterns[j].quantity2) {
                            if (strcmp(patterns[i]->quantity2,projData->inputAntennaPatterns[j].quantity2) != 0) {j++; continue;};
                        } else {j++; continue;};
                    } else {
                        if (projData->inputAntennaPatterns[j].quantity2) {j++; continue;};
                    }


                    if (patterns[i]->plane) {
                        if (projData->inputAntennaPatterns[j].plane) {
                            if (strcmp(patterns[i]->plane,projData->inputAntennaPatterns[j].plane) != 0) {j++; continue;};
                        } else {j++; continue;};
                    } else {
                        if (projData->inputAntennaPatterns[j].plane) {j++; continue;};
                    }

                    if (!double_compare(patterns[i]->theta,projData->inputAntennaPatterns[j].theta,1e-12)) {j++; continue;};
                    if (!double_compare(patterns[i]->phi,projData->inputAntennaPatterns[j].phi,1e-12)) {j++; continue;};
                    if (!double_compare(patterns[i]->latitude,projData->inputAntennaPatterns[j].latitude,1e-12)) {j++; continue;};
                    if (!double_compare(patterns[i]->rotation,projData->inputAntennaPatterns[j].rotation,1e-12)) {j++; continue;};

                    foundMatch=true;
                    break;
                }
                j++;
            }
            if (!foundMatch) return true;
        }
        i++;
    }

    return false;
}

void AntennaForm::on_OkButton_clicked ()
{
    if (!double_compare(projData->antenna_plot_current_resolution,currentResolution,1e-12)){
        projData->antenna_plot_current_resolution=currentResolution;
        projData->modified=1;
    }

    if (!double_compare(projData->antenna_plot_2D_range,plotRange2D,1e-12)) {
        projData->antenna_plot_2D_range=plotRange2D;
        projData->modified=1;
    }

    if (!double_compare(projData->antenna_plot_2D_interval,axisInterval2D,1e-12)) {
        projData->antenna_plot_2D_interval=axisInterval2D;
        projData->modified=1;
    }

    if (!double_compare(projData->antenna_plot_2D_resolution,plotResolution2D,1e-12)) {
        projData->antenna_plot_2D_resolution=plotResolution2D;
        projData->modified=1;
    }

    if (projData->antenna_plot_2D_annotations != dataSummary2D) {
        projData->antenna_plot_2D_annotations=dataSummary2D;
        projData->modified=1;
    }

    if (projData->antenna_plot_2D_save != savePlots2D) {
        projData->antenna_plot_2D_save=savePlots2D;
        projData->modified=1;
    }

    if (projData->antenna_plot_3D_refinement != plotResolution3D+2) {
        projData->antenna_plot_3D_refinement=plotResolution3D+2;
        projData->modified=1;
    }

    if (projData->antenna_plot_3D_sphere != generateSphere) {
        projData->antenna_plot_3D_sphere=generateSphere;
        projData->modified=1;
    }

    if (projData->antenna_plot_3D_save != savePlots3D) {
        projData->antenna_plot_3D_save=savePlots3D;
        projData->modified=1;
    }

    if (projData->antenna_plot_raw_save != saveRawData) {
        projData->antenna_plot_raw_save=saveRawData;
        projData->modified=1;
    }

    // extract the patterns from the table
    extractPatterns();

    // check for changes
    if (hasPatternChanges()) projData->modified=1;

    // clear existing patterns
    int i=0;
    while (i < projData->inputAntennaPatternsCount) {
        if (projData->inputAntennaPatterns[i].quantity1 != 0) {
            free(projData->inputAntennaPatterns[i].quantity1);
            projData->inputAntennaPatterns[i].quantity1=nullptr;
        }

        if (projData->inputAntennaPatterns[i].quantity2 != 0) {
            free(projData->inputAntennaPatterns[i].quantity2);
            projData->inputAntennaPatterns[i].quantity2=nullptr;
        }

        if (projData->inputAntennaPatterns[i].plane != 0) {
            free(projData->inputAntennaPatterns[i].plane);
            projData->inputAntennaPatterns[i].plane=nullptr;
        }
        i++;
    }
    projData->inputAntennaPatternsCount=0;

    // reserve space
    if (projData->inputAntennaPatternsAllocated < patterns.size()) {
        projData->inputAntennaPatterns=(struct inputAntennaPattern *)
            realloc(projData->inputAntennaPatterns,patterns.size()*sizeof(struct inputAntennaPattern));
        projData->inputAntennaPatternsAllocated=patterns.size();
    }

    int j=0;
    while (j < patterns.size()) {
        projData->inputAntennaPatterns[j].lineNumber=patterns[j]->lineNumber;
        projData->inputAntennaPatterns[j].dim=patterns[j]->dim;
        projData->inputAntennaPatterns[j].quantity1=allocCopyString(patterns[j]->quantity1);
        projData->inputAntennaPatterns[j].quantity2=allocCopyString(patterns[j]->quantity2);
        projData->inputAntennaPatterns[j].plane=allocCopyString(patterns[j]->plane);
        projData->inputAntennaPatterns[j].theta=patterns[j]->theta;
        projData->inputAntennaPatterns[j].phi=patterns[j]->phi;
        projData->inputAntennaPatterns[j].latitude=patterns[j]->latitude;
        projData->inputAntennaPatterns[j].rotation=patterns[j]->rotation;
        j++;
    }
    projData->inputAntennaPatternsCount=patterns.size();

    // free memory
    j=0;
    while (j < patterns.size()) {
        if (patterns[j]->quantity1) {free(patterns[j]->quantity1); patterns[j]->quantity1=nullptr;}
        if (patterns[j]->quantity2) {free(patterns[j]->quantity2); patterns[j]->quantity2=nullptr;}
        if (patterns[j]->plane) {free(patterns[j]->plane); patterns[j]->plane=nullptr;}
        delete patterns[j];
        j++;
    }

    QDialog::close();
}

void AntennaForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    QDialog::close();
}

void AntennaForm::reject ()
{
    ui->CancelButton->setChecked(true);

    QDialog::reject();
}

void AntennaForm::quantityBox_changed (int newIndex)
{
    ui->OkButton->setEnabled(true);
}

void AntennaForm::planeBox_changed (int newIndex)
{
    if (newIndex == 3) {
        QWidget *container=ui->patternTable->cellWidget(ui->patternTable->currentRow(),2);
        if (container) {
            QDoubleSpinBox *thetaBox=container->findChild<QDoubleSpinBox*>();
            if (thetaBox) thetaBox->setEnabled(true);
        }

        container=ui->patternTable->cellWidget(ui->patternTable->currentRow(),3);
        if (container) {
            QDoubleSpinBox *phiBox=container->findChild<QDoubleSpinBox*>();
            if (phiBox) phiBox->setEnabled(true);
        }
    } else {
        QWidget *container=ui->patternTable->cellWidget(ui->patternTable->currentRow(),2);
        if (container) {
            QDoubleSpinBox *thetaBox=container->findChild<QDoubleSpinBox*>();
            if (thetaBox) thetaBox->setEnabled(false);
        }

        container=ui->patternTable->cellWidget(ui->patternTable->currentRow(),3);
        if (container) {
            QDoubleSpinBox *phiBox=container->findChild<QDoubleSpinBox*>();
            if (phiBox) phiBox->setEnabled(false);
        }
    }

    ui->OkButton->setEnabled(true);
}

void AntennaForm::thetaBox_changed (int newValue)
{
    ui->OkButton->setEnabled(true);
}

void AntennaForm::phiBox_changed (double newValue)
{
    ui->OkButton->setEnabled(true);
}

void AntennaForm::latitudeBox_changed (double newValue)
{
    ui->OkButton->setEnabled(true);
}

void AntennaForm::rotationBox_changed (double newValue)
{
    ui->OkButton->setEnabled(true);
}





















