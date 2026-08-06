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

#include "VectorInputForm.h"
#include "ui_VectorInputForm.h"

VectorInputForm::VectorInputForm (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VectorInputForm)
{
    this->setWindowIcon(QApplication::windowIcon());

    ui->setupUi(this);

    setFixedSize(width(),height());


    ui->pickOrigin->setCheckable(true);
    ui->pickTip->setCheckable(true);
    ui->OkButton->setCheckable(true);
    ui->CancelButton->setCheckable(true);

    ui->OkButton->setEnabled(false);

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

VectorInputForm::~VectorInputForm ()
{
    delete ui;
}

void VectorInputForm::set_startPoint (gp_Pnt *startPoint)
{
    transferStartPoint=startPoint;
    localStartPoint=*transferStartPoint;

    ui->startX->setText(QString::number(localStartPoint.X()*conversionFactor,'g',15));
    ui->startY->setText(QString::number(localStartPoint.Y()*conversionFactor,'g',15));
    ui->startZ->setText(QString::number(localStartPoint.Z()*conversionFactor,'g',15));
}

void VectorInputForm::set_endPoint (gp_Pnt *endPoint)
{
    transferEndPoint=endPoint;
    localEndPoint=*transferEndPoint;

    ui->endX->setText(QString::number(localEndPoint.X()*conversionFactor,'g',15));
    ui->endY->setText(QString::number(localEndPoint.Y()*conversionFactor,'g',15));
    ui->endZ->setText(QString::number(localEndPoint.Z()*conversionFactor,'g',15));
}

void VectorInputForm::on_pickOrigin_clicked ()
{
    pickStartPoint=true;
    pickEndPoint=false;
    this->setEnabled(false);

    ui->pickOrigin->setChecked(true);

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void VectorInputForm::on_pickTip_clicked ()
{
    pickStartPoint=false;
    pickEndPoint=true;
    this->setEnabled(false);

    ui->pickTip->setChecked(true);

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void VectorInputForm::on_OkButton_clicked ()
{
    ui->OkButton->setChecked(true);
    *transferStartPoint=localStartPoint;
    *transferEndPoint=localEndPoint;
    emit relay->finishOperation(false,31);
    isXclose=false;
    QDialog::close();
}

void VectorInputForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(true,32);
    isXclose=false;
    QDialog::close();
}

void VectorInputForm::pickVertexFinished (gp_Pnt point)
{
    this->setEnabled(true);

    if (pickStartPoint) {
        hasStartPoint=true;
        localStartPoint=point;
        ui->startX->setText(QString::number(localStartPoint.X()*conversionFactor,'g',15));
        ui->startY->setText(QString::number(localStartPoint.Y()*conversionFactor,'g',15));
        ui->startZ->setText(QString::number(localStartPoint.Z()*conversionFactor,'g',15));
        activateWindow();
        raise();
        ui->pickOrigin->setChecked(false);
        ui->pickTip->setFocus();
    }

    if (pickEndPoint) {
        hasEndPoint=true;
        localEndPoint=point;
        ui->endX->setText(QString::number(localEndPoint.X()*conversionFactor,'g',15));
        ui->endY->setText(QString::number(localEndPoint.Y()*conversionFactor,'g',15));
        ui->endZ->setText(QString::number(localEndPoint.Z()*conversionFactor,'g',15));
        activateWindow();
        raise();
        ui->pickTip->setChecked(false);
        ui->pickOrigin->setFocus();
    }

    if (hasStartPoint && hasEndPoint) {
        ui->OkButton->setEnabled(true);
        ui->OkButton->setFocus();
    }
}

void VectorInputForm::reject ()
{
    ui->CancelButton->setChecked(true);
    if (isXclose) emit relay->finishOperation(true,33);
    QDialog::reject();
}

