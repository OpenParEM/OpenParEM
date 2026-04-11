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

void VectorInputForm::set_startPoint (gp_Pnt *startPoint)
{
    transferStartPoint=startPoint;
    localStartPoint=*transferStartPoint;

    ui->startX->setText(QString::number(localStartPoint.X()));
    ui->startY->setText(QString::number(localStartPoint.Y()));
    ui->startZ->setText(QString::number(localStartPoint.Z()));
}

void VectorInputForm::set_endPoint (gp_Pnt *endPoint)
{
    transferEndPoint=endPoint;
    localEndPoint=*transferEndPoint;

    ui->endX->setText(QString::number(localEndPoint.X()));
    ui->endY->setText(QString::number(localEndPoint.Y()));
    ui->endZ->setText(QString::number(localEndPoint.Z()));
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
    QDialog::close();
}

void VectorInputForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(true,32);
    QDialog::close();
}

void VectorInputForm::pickVertexFinished (gp_Pnt point)
{
    this->setEnabled(true);

    if (pickStartPoint) {
        hasStartPoint=true;
        localStartPoint=point;
        ui->startX->setText(QString::number(localStartPoint.X()));
        ui->startY->setText(QString::number(localStartPoint.Y()));
        ui->startZ->setText(QString::number(localStartPoint.Z()));
        activateWindow();
        raise();
        ui->pickOrigin->setChecked(false);
        ui->pickTip->setFocus();
    }

    if (pickEndPoint) {
        hasEndPoint=true;
        localEndPoint=point;
        ui->endX->setText(QString::number(localEndPoint.X()));
        ui->endY->setText(QString::number(localEndPoint.Y()));
        ui->endZ->setText(QString::number(localEndPoint.Z()));
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
    emit relay->finishOperation(true,33);

    QDialog::reject();
}

