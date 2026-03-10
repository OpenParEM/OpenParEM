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

#include "PolycircleEditForm.h"
#include "ui_PolycircleEditForm.h"

PolycircleEditForm::PolycircleEditForm (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PolycircleEditForm)
{
    ui->setupUi(this);

    setFixedSize(width(),height());

    polywire=nullptr;

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

bool PolycircleEditForm::isValid ()
{
    gp_Pnt centerPoint(ui->centerPositionX->text().toDouble(),ui->centerPositionY->text().toDouble(),ui->centerPositionZ->text().toDouble());
    gp_Pnt firstPoint(ui->firstPositionX->text().toDouble(),ui->firstPositionY->text().toDouble(),ui->firstPositionZ->text().toDouble());
    double radius=centerPoint.Distance(firstPoint);
    if (radius < Precision::Confusion()) return false;
    return true;
}

void PolycircleEditForm::set_Polycircle (Polycircle *polycircle_)
{
    polywire=polycircle_;

    ui->centerPositionX->setText(QString::number(polywire->getCenterPoint().X()));
    ui->centerPositionY->setText(QString::number(polywire->getCenterPoint().Y()));
    ui->centerPositionZ->setText(QString::number(polywire->getCenterPoint().Z()));

    ui->firstPositionX->setText(QString::number(polywire->getFirstPoint().X()));
    ui->firstPositionY->setText(QString::number(polywire->getFirstPoint().Y()));
    ui->firstPositionZ->setText(QString::number(polywire->getFirstPoint().Z()));

    double radius=polywire->getCenterPoint().Distance(polywire->getFirstPoint());
    ui->radius->setText(QString::number(radius));

    ui->vertexCount->setText(QString::number(polywire->getVertexCount()));
}

void PolycircleEditForm::on_centerPositionX_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_centerPositionY_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_centerPositionZ_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_pickCenter_clicked ()
{
    ui->pickCenter->setChecked(true);
    this->setEnabled(false);

    pickCenterPoint=true;

    drawingWindow->set_pickVertex(true);
    drawingWindow->updateViewer();
}

void PolycircleEditForm::on_radius_returnPressed ()
{
    if (ui->radius->text().toDouble() < Precision::Confusion()) return;

    gp_Pnt centerPoint(ui->centerPositionX->text().toDouble(),ui->centerPositionY->text().toDouble(),ui->centerPositionZ->text().toDouble());
    gp_Pnt firstPoint(ui->firstPositionX->text().toDouble(),ui->firstPositionY->text().toDouble(),ui->firstPositionZ->text().toDouble());

    if (centerPoint.Distance(firstPoint) < Precision::Confusion()) return;

    gp_Vec dir(centerPoint,firstPoint);
    dir.Normalize();

    gp_Pnt newFirstPoint=centerPoint.Translated(dir*ui->radius->text().toDouble());
    ui->firstPositionX->setText(QString::number(newFirstPoint.X()));
    ui->firstPositionY->setText(QString::number(newFirstPoint.Y()));
    ui->firstPositionZ->setText(QString::number(newFirstPoint.Z()));

    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_firstPositionX_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_firstPositionY_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_firstPositionZ_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_pickFirst_clicked ()
{
    ui->pickFirst->setChecked(true);
    this->setEnabled(false);

    pickFirstPoint=true;

    drawingWindow->set_pickVertex(true);
    drawingWindow->updateViewer();
}

void PolycircleEditForm::on_vertexCount_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_OkButton_clicked ()
{
    gp_Pnt centerPoint(ui->centerPositionX->text().toDouble(),ui->centerPositionY->text().toDouble(),ui->centerPositionZ->text().toDouble());
    gp_Pnt firstPoint(ui->firstPositionX->text().toDouble(),ui->firstPositionY->text().toDouble(),ui->firstPositionZ->text().toDouble());

    polywire->setCenterPoint(centerPoint);
    polywire->setFirstPoint(firstPoint);

    int vertexCount=ui->vertexCount->text().toInt();
    polywire->setVertexCount(vertexCount);

    polywire->recalculate();

    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false);

    QDialog::close();
}

void PolycircleEditForm::pickVertexFinished (gp_Pnt point)
{
    this->setEnabled(true);

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

    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true);
    QDialog::close();
}

void PolycircleEditForm::reject ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true);
    QDialog::reject();
}
