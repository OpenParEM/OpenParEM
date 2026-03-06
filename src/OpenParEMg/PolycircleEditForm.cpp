#include "PolycircleEditForm.h"
#include "ui_PolycircleEditForm.h"

PolycircleEditForm::PolycircleEditForm (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PolycircleEditForm)
{
    ui->setupUi(this);

    polycircle=nullptr;

    doubleValidator.setNotation(QDoubleValidator::ScientificNotation);
    intValidator.setBottom(3);

    ui->centerPositionX->setValidator(&doubleValidator);
    ui->centerPositionY->setValidator(&doubleValidator);
    ui->centerPositionZ->setValidator(&doubleValidator);

    ui->firstPositionX->setValidator(&doubleValidator);
    ui->firstPositionY->setValidator(&doubleValidator);
    ui->firstPositionZ->setValidator(&doubleValidator);

    ui->radius->setValidator(&doubleValidator);
    ui->vertexCount->setValidator(&intValidator);

    pickCenterPoint=false;
    pickFirstPoint=false;

    ui->pickCenter->setCheckable(true);
    ui->pickFirst->setCheckable(true);
    ui->OkButton->setCheckable(true);
    ui->CancelButton->setCheckable(true);

    ui->OkButton->setEnabled(false);
}

PolycircleEditForm::~PolycircleEditForm ()
{
    delete ui;
}

void PolycircleEditForm::set_Polycircle (Polycircle *polycircle_)
{
    polycircle=polycircle_;

    ui->centerPositionX->setText(QString::number(polycircle->getCenterPoint().X()));
    ui->centerPositionY->setText(QString::number(polycircle->getCenterPoint().Y()));
    ui->centerPositionZ->setText(QString::number(polycircle->getCenterPoint().Z()));

    ui->firstPositionX->setText(QString::number(polycircle->getFirstPoint().X()));
    ui->firstPositionY->setText(QString::number(polycircle->getFirstPoint().Y()));
    ui->firstPositionZ->setText(QString::number(polycircle->getFirstPoint().Z()));

    double radius=polycircle->getCenterPoint().Distance(polycircle->getFirstPoint());
    ui->radius->setText(QString::number(radius));

    ui->vertexCount->setText(QString::number(polycircle->getVertexCount()));
}

void PolycircleEditForm::on_centerPositionX_returnPressed ()
{
    ui->OkButton->setEnabled(true);
}

void PolycircleEditForm::on_centerPositionY_returnPressed ()
{
    ui->OkButton->setEnabled(true);
}

void PolycircleEditForm::on_centerPositionZ_returnPressed ()
{
    ui->OkButton->setEnabled(true);
}

void PolycircleEditForm::on_pickCenter_clicked ()
{
    ui->pickCenter->setChecked(true);

    pickCenterPoint=true;

    drawingWindow->set_pickVertex(true);
    drawingWindow->updateViewer();

    ui->OkButton->setEnabled(true);
}

void PolycircleEditForm::on_radius_returnPressed ()
{
    gp_Pnt centerPoint(ui->centerPositionX->text().toDouble(),ui->centerPositionY->text().toDouble(),ui->centerPositionZ->text().toDouble());
    gp_Pnt firstPoint(ui->firstPositionX->text().toDouble(),ui->firstPositionY->text().toDouble(),ui->firstPositionZ->text().toDouble());

    gp_Vec dir(centerPoint,firstPoint);
    dir.Normalize();

    gp_Pnt newFirstPoint=centerPoint.Translated(dir*ui->radius->text().toDouble());
    ui->firstPositionX->setText(QString::number(newFirstPoint.X()));
    ui->firstPositionY->setText(QString::number(newFirstPoint.Y()));
    ui->firstPositionZ->setText(QString::number(newFirstPoint.Z()));

    ui->OkButton->setEnabled(true);
}

void PolycircleEditForm::on_firstPositionX_returnPressed ()
{
    ui->OkButton->setEnabled(true);
}

void PolycircleEditForm::on_firstPositionY_returnPressed ()
{
    ui->OkButton->setEnabled(true);
}

void PolycircleEditForm::on_firstPositionZ_returnPressed ()
{
    ui->OkButton->setEnabled(true);
}

void PolycircleEditForm::on_pickFirst_clicked ()
{
    ui->pickFirst->setChecked(true);

    pickFirstPoint=true;

    drawingWindow->set_pickVertex(true);
    drawingWindow->updateViewer();

    ui->OkButton->setEnabled(true);
}

void PolycircleEditForm::on_vertexCount_returnPressed ()
{
    ui->OkButton->setEnabled(true);
}

void PolycircleEditForm::on_OkButton_clicked ()
{
    gp_Pnt centerPoint(ui->centerPositionX->text().toDouble(),ui->centerPositionY->text().toDouble(),ui->centerPositionZ->text().toDouble());
    gp_Pnt firstPoint(ui->firstPositionX->text().toDouble(),ui->firstPositionY->text().toDouble(),ui->firstPositionZ->text().toDouble());

    polycircle->setCenterPoint(centerPoint);
    polycircle->setFirstPoint(firstPoint);

    int vertexCount=ui->vertexCount->text().toInt();
    polycircle->setVertexCount(vertexCount);

    polycircle->recalculate();

    emit relay->finishOperation(gp_Pnt(0,0,0),0,false);

    QDialog::close();
}

void PolycircleEditForm::pickVertexFinished (gp_Pnt point)
{
    if (pickCenterPoint) {
        pickCenterPoint=false;
        ui->centerPositionX->setText(QString::number(point.X()));
        ui->centerPositionY->setText(QString::number(point.Y()));
        ui->centerPositionZ->setText(QString::number(point.Z()));
        ui->pickCenter->setChecked(false);
    }

    if (pickFirstPoint) {
        pickFirstPoint=false;
        ui->firstPositionX->setText(QString::number(point.X()));
        ui->firstPositionY->setText(QString::number(point.Y()));
        ui->firstPositionZ->setText(QString::number(point.Z()));
        ui->pickFirst->setChecked(false);
    }

    gp_Pnt centerPoint(ui->centerPositionX->text().toDouble(),ui->centerPositionY->text().toDouble(),ui->centerPositionZ->text().toDouble());
    gp_Pnt firstPoint(ui->firstPositionX->text().toDouble(),ui->firstPositionY->text().toDouble(),ui->firstPositionZ->text().toDouble());
    double radius=centerPoint.Distance(firstPoint);
    ui->radius->setText(QString::number(radius));

    ui->OkButton->setEnabled(true);
}

void PolycircleEditForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    QDialog::close();
}

void PolycircleEditForm::reject ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,true);
    QDialog::reject();
}
