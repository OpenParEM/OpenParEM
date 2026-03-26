#include "VectorInputForm.h"
#include "ui_VectorInputForm.h"

VectorInputForm::VectorInputForm (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VectorInputForm)
{
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
}

VectorInputForm::~VectorInputForm ()
{
    delete ui;
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
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,startPoint,endPoint,false,31);
    QDialog::close();
}

void VectorInputForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true,32);
    QDialog::close();
}

void VectorInputForm::pickVertexFinished (gp_Pnt point)
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
        ui->pickOrigin->setChecked(false);
        ui->pickTip->setFocus();
    }

    if (pickEndPoint) {
        hasEndPoint=true;
        endPoint=point;
        ui->endX->setText(QString::number(endPoint.X()));
        ui->endY->setText(QString::number(endPoint.Y()));
        ui->endZ->setText(QString::number(endPoint.Z()));
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
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true,33);

    QDialog::reject();
}

