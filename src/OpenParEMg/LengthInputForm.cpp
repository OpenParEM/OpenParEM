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

#include "LengthInputForm.h"
#include "ui_LengthInputForm.h"
#include <qtimer.h>

LengthInputForm::LengthInputForm (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LengthInputForm)
{
    this->setWindowIcon(QApplication::windowIcon());

    ui->setupUi(this);

    setFixedSize(width(),height());

    validator.setNotation(QDoubleValidator::ScientificNotation);
    ui->lineEdit->setValidator(&validator);
    ui->lineEdit->setFocus();

    ui->pickStart->setCheckable(true);
    ui->pickEnd->setCheckable(true);
    ui->OkButton->setCheckable(true);
    ui->CancelButton->setCheckable(true);

    ui->OkButton->setEnabled(false);

    localLength=0;
    transferLength=0;
    pickStartPoint=false;
    pickEndPoint=false;
    hasStartPoint=false;
    hasEndPoint=false;

    conversionFactor=1;
    isXclose=true;

    // position
    QRect parentRect=parent->geometry();
    int x=parentRect.right()-width()-20;
    int y=parentRect.top()+20;
    move(mapToGlobal(QPoint(x,y)));
}

LengthInputForm::~LengthInputForm ()
{
    delete ui;
}

void LengthInputForm::set_length (double *length_)
{
    transferLength=length_;
    localLength=*transferLength;
    localLength*=conversionFactor;
    ui->lineEdit->setText(QString::number(localLength,'g',15));

    if (abs(localLength) > 1e-14) {
        activateWindow();
        raise();
        ui->lineEdit->clearFocus();
        ui->OkButton->setEnabled(true);
    }
}

void LengthInputForm::on_lineEdit_editingFinished ()
{
    localLength=ui->lineEdit->text().toDouble();
    ui->OkButton->setEnabled(true);
    if (abs(localLength) > 1e-14) {
        activateWindow();
        raise();
        ui->lineEdit->clearFocus();
        ui->OkButton->setEnabled(true);
    }
}

void LengthInputForm::on_pickStart_clicked ()
{
    pickStartPoint=true;
    pickEndPoint=false;
    this->setEnabled(false);

    ui->pickStart->setChecked(true);

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void LengthInputForm::on_pickEnd_clicked ()
{
    pickStartPoint=false;
    pickEndPoint=true;
    this->setEnabled(false);

    ui->pickEnd->setChecked(true);

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void LengthInputForm::on_OkButton_clicked ()
{
    ui->OkButton->setChecked(true);
    *transferLength=localLength/conversionFactor;
    emit relay->finishOperation(false,71);
    isXclose=false;
    QDialog::close();
}

void LengthInputForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(true,72);
    isXclose=false;
    QDialog::close();
}

void LengthInputForm::pickVertexFinished (gp_Pnt point)
{
    this->setEnabled(true);

    if (pickStartPoint) {
        hasStartPoint=true;
        startPoint=point;
        ui->startX->setText(QString::number(startPoint.X()*conversionFactor,'g',15));
        ui->startY->setText(QString::number(startPoint.Y()*conversionFactor,'g',15));
        ui->startZ->setText(QString::number(startPoint.Z()*conversionFactor,'g',15));
        activateWindow();
        raise();
        ui->pickStart->setChecked(false);
        ui->pickEnd->setFocus();
    }

    if (pickEndPoint) {
        hasEndPoint=true;
        endPoint=point;
        ui->endX->setText(QString::number(endPoint.X()*conversionFactor,'g',15));
        ui->endY->setText(QString::number(endPoint.Y()*conversionFactor,'g',15));
        ui->endZ->setText(QString::number(endPoint.Z()*conversionFactor,'g',15));
        activateWindow();
        raise();
        ui->pickEnd->setChecked(false);
        ui->pickStart->setFocus();
    }

    if (hasStartPoint && hasEndPoint) {
        Standard_Real distance=startPoint.Distance(endPoint);
        localLength=distance*conversionFactor;
        if (localLength > 1e-13) {
            extrusionDirection->SetCoord(endPoint.X()-startPoint.X(),endPoint.Y()-startPoint.Y(),endPoint.Z()-startPoint.Z());
            ui->OkButton->setEnabled(true);
            ui->OkButton->setFocus();
        } else {
            ui->OkButton->setEnabled(false);
        }

        ui->lineEdit->setText(QString::number(localLength,'g',15));
    }
}

void LengthInputForm::reject ()
{
    ui->CancelButton->setChecked(true);
    if (isXclose) emit relay->finishOperation(true,73);
    QDialog::reject();
}

void LengthInputForm::print_point (gp_Pnt point)
{
    std::cout << "picked point: (" << point.X() << "," << point.Y() << "," << point.Z() << ")" << std::endl; std::cout.flush();
}

