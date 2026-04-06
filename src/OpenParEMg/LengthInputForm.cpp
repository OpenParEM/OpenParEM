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

    length=0;
    pickStartPoint=false;
    pickEndPoint=false;
    hasStartPoint=false;
    hasEndPoint=false;

    extrude=nullptr;
}

LengthInputForm::~LengthInputForm ()
{
    delete ui;
}

void LengthInputForm::set_length (double length_)
{
    length=length_;
    ui->lineEdit->setText(QString::number(length));
}

void LengthInputForm::on_lineEdit_returnPressed ()
{
    length=ui->lineEdit->text().toDouble();
    ui->OkButton->setEnabled(true);
    if (abs(length) < 1e-12) {
        activateWindow();
        raise();
        ui->lineEdit->clearFocus();
        ui->OkButton->setEnabled(true);
        ui->OkButton->setFocus();
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
    emit relay->finishOperation(gp_Pnt(0,0,0),length,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,71);
    QDialog::close();
}

void LengthInputForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true,72);
    QDialog::close();
}

void LengthInputForm::pickVertexFinished (gp_Pnt point)
{
    this->setEnabled(true);

    if (pickStartPoint) {
        hasStartPoint=true;
        startPoint=point;
        ui->startX->setText(QString::number(startPoint.X()));
        ui->startY->setText(QString::number(startPoint.Y()));
        ui->startZ->setText(QString::number(startPoint.Z()));
        activateWindow();
        raise();
        ui->pickStart->setChecked(false);
        ui->pickEnd->setFocus();
    }

    if (pickEndPoint) {
        hasEndPoint=true;
        endPoint=point;
        ui->endX->setText(QString::number(endPoint.X()));
        ui->endY->setText(QString::number(endPoint.Y()));
        ui->endZ->setText(QString::number(endPoint.Z()));
        activateWindow();
        raise();
        ui->pickEnd->setChecked(false);
        ui->pickStart->setFocus();
    }

    if (hasStartPoint && hasEndPoint) {
        Standard_Real distance=startPoint.Distance(endPoint);
        length=distance;
        if (length > 1e-13) {
            extrusionDirection->SetCoord(endPoint.X()-startPoint.X(),endPoint.Y()-startPoint.Y(),endPoint.Z()-startPoint.Z());
            ui->OkButton->setEnabled(true);
            ui->OkButton->setFocus();
        } else {
            ui->OkButton->setEnabled(false);
        }

        ui->lineEdit->setText(QString::number(length));
    }
}

void LengthInputForm::reject ()
{
    //std::cout << "LengthInputForm::reject" << std::endl; std::cout.flush();
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true,73);

    QDialog::reject();
}

void LengthInputForm::print_point (gp_Pnt point)
{
    std::cout << "picked point: (" << point.X() << "," << point.Y() << "," << point.Z() << ")" << std::endl; std::cout.flush();
}

