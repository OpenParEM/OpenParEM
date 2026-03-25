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

#include "RotateInputForm.h"
#include "ui_RotateInputForm.h"

RotateInputForm::RotateInputForm(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RotateInputForm)
{
    ui->setupUi(this);

    setFixedSize(width(),height());

    // default to custom axis

    ui->pickStart->setCheckable(true);
    ui->pickStart->setEnabled(true);
    ui->pickEnd->setCheckable(true);
    ui->pickEnd->setEnabled(true);
    ui->OkButton->setCheckable(true);
    ui->CancelButton->setCheckable(true);

    pickStartPoint=false;
    pickEndPoint=false;
    hasStartPoint=false;
    hasEndPoint=false;

    ui->Xaxis->setChecked(false);
    ui->Yaxis->setChecked(false);
    ui->Zaxis->setChecked(false);
    ui->CustomAxis->setChecked(true);

    ui->startX->setText("0");
    ui->startY->setText("0");
    ui->startZ->setText("0");
    startPoint.SetCoord(0,0,0);

    ui->endX->setText("1");
    ui->endY->setText("0");
    ui->endZ->setText("0");
    endPoint.SetCoord(1,0,0);

    // default angle is set to 90 degrees in the forms editor
}

RotateInputForm::~RotateInputForm ()
{
    delete ui;
}

void RotateInputForm::on_Xaxis_clicked ()
{
    ui->Xaxis->setChecked(true);
    ui->Yaxis->setChecked(false);
    ui->Zaxis->setChecked(false);
    ui->CustomAxis->setChecked(false);

    ui->startX->setText("0");
    ui->startY->setText("0");
    ui->startZ->setText("0");
    startPoint.SetCoord(0,0,0);

    ui->endX->setText("1");
    ui->endY->setText("0");
    ui->endZ->setText("0");
    endPoint.SetCoord(1,0,0);

    ui->pickStart->setEnabled(false);
    ui->pickEnd->setEnabled(false);

    ui->OkButton->setEnabled(true);
}

void RotateInputForm::on_Yaxis_clicked ()
{
    ui->Xaxis->setChecked(false);
    ui->Yaxis->setChecked(true);
    ui->Zaxis->setChecked(false);
    ui->CustomAxis->setChecked(false);

    ui->startX->setText("0");
    ui->startY->setText("0");
    ui->startZ->setText("0");
    startPoint.SetCoord(0,0,0);

    ui->endX->setText("0");
    ui->endY->setText("1");
    ui->endZ->setText("0");
    endPoint.SetCoord(0,1,0);

    ui->pickStart->setEnabled(false);
    ui->pickEnd->setEnabled(false);

    ui->OkButton->setEnabled(true);
}

void RotateInputForm::on_Zaxis_clicked ()
{
    ui->Xaxis->setChecked(false);
    ui->Yaxis->setChecked(false);
    ui->Zaxis->setChecked(true);
    ui->CustomAxis->setChecked(false);

    ui->startX->setText("0");
    ui->startY->setText("0");
    ui->startZ->setText("0");
    startPoint.SetCoord(0,0,0);

    ui->endX->setText("0");
    ui->endY->setText("0");
    ui->endZ->setText("1");
    endPoint.SetCoord(0,0,1);

    ui->pickStart->setEnabled(false);
    ui->pickEnd->setEnabled(false);

    ui->OkButton->setEnabled(true);
}

void RotateInputForm::on_CustomAxis_clicked ()
{
    ui->OkButton->setEnabled(false);

    ui->Xaxis->setChecked(false);
    ui->Yaxis->setChecked(false);
    ui->Zaxis->setChecked(false);
    ui->CustomAxis->setChecked(true);

    ui->startX->setText("");
    ui->startY->setText("");
    ui->startZ->setText("");

    ui->endX->setText("");
    ui->endY->setText("");
    ui->endZ->setText("");

    ui->pickStart->setEnabled(true);
    ui->pickEnd->setEnabled(true);
}

void RotateInputForm::on_pickStart_clicked ()
{
    std::cout << "RotateInputForm::on_pickStart_clicked" << std::endl; std::cout.flush();
    pickStartPoint=true;
    pickEndPoint=false;
    this->setEnabled(false);

    ui->pickStart->setChecked(true);

    drawingWindow->set_pickVertex(true);
    drawingWindow->updateViewer();
}

void RotateInputForm::on_pickEnd_clicked ()
{
    std::cout << "RotateInputForm::on_pickEnd_clicked" << std::endl; std::cout.flush();
    pickStartPoint=false;
    pickEndPoint=true;
    this->setEnabled(false);

    ui->pickEnd->setChecked(true);

    drawingWindow->set_pickVertex(true);
    drawingWindow->updateViewer();
}

void RotateInputForm::on_OkButton_clicked ()
{
    ui->OkButton->setChecked(true);
    double angle=ui->angleDegrees->value();
    emit relay->finishOperation(gp_Pnt(0,0,0),0,angle,startPoint,endPoint,false);
    QDialog::close();
}

void RotateInputForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true);
    QDialog::close();
}

void RotateInputForm::pickVertexFinished (gp_Pnt point)
{
    std::cout << "RotateInputForm::pickVertexFinished" << std::endl; std::cout.flush();

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
        if (distance > 1e-12) {
            ui->OkButton->setEnabled(true);
            ui->OkButton->setFocus();
        } else {
            ui->OkButton->setEnabled(false);
        }
    }
}

void RotateInputForm::reject ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true);
    QDialog::reject();
}

