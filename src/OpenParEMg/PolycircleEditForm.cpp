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

bool PolycircleEditForm::isValid ()
{
    double radius=centerPoint.Distance(firstPoint);
    if (radius < Precision::Confusion()) return false;
    return true;
}

void PolycircleEditForm::populate (Polycircle *polycircle_)
{
    centerPoint=polycircle_->getCenterPoint();
    firstPoint=polycircle_->getFirstPoint();
    vertexCount=polycircle_->getVertexCount();

    ui->centerPositionX->setText(QString::number(centerPoint.X()));
    ui->centerPositionY->setText(QString::number(centerPoint.Y()));
    ui->centerPositionZ->setText(QString::number(centerPoint.Z()));

    ui->firstPositionX->setText(QString::number(firstPoint.X()));
    ui->firstPositionY->setText(QString::number(firstPoint.Y()));
    ui->firstPositionZ->setText(QString::number(firstPoint.Z()));

    double radius=centerPoint.Distance(firstPoint);
    ui->radius->setText(QString::number(radius));

    ui->vertexCount->setText(QString::number(vertexCount));
}

void PolycircleEditForm::set_Polycircle (Polycircle *polycircle_)
{
    polycircle=polycircle_;
    populate(polycircle);
}

void PolycircleEditForm::on_centerPositionX_returnPressed ()
{
    centerPoint.SetX(ui->centerPositionX->text().toDouble());
    repopulateOffCenter();
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_centerPositionY_returnPressed ()
{
    centerPoint.SetY(ui->centerPositionY->text().toDouble());
    repopulateOffCenter();
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_centerPositionZ_returnPressed ()
{
    centerPoint.SetZ(ui->centerPositionZ->text().toDouble());
    repopulateOffCenter();
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_pickCenter_clicked ()
{
    ui->pickCenter->setChecked(true);
    this->setEnabled(false);

    pickCenterPoint=true;

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void PolycircleEditForm::repopulateOffCenter ()
{
    Polycircle temp(polycircle);
    gp_Pnt oldCenter=temp.getCenterPoint();
    temp.shift(centerPoint,oldCenter);
    populate(&temp);
}

void PolycircleEditForm::repopulateOffFirstPoint ()
{
    Polycircle temp(polycircle);
    temp.setFirstPoint(firstPoint);
    temp.recalculate();
    populate(&temp);
}

void PolycircleEditForm::on_radius_returnPressed ()
{
    if (ui->radius->text().toDouble() < Precision::Confusion()) return;

    Polycircle temp(polycircle);

    if (centerPoint.Distance(firstPoint) < Precision::Confusion()) return;

    gp_Vec dir(centerPoint,firstPoint);
    dir.Normalize();

    gp_Pnt newFirstPoint=centerPoint.Translated(dir*ui->radius->text().toDouble());
    temp.setFirstPoint(newFirstPoint);
    temp.recalculate();
    populate(&temp);

    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_firstPositionX_returnPressed ()
{
    firstPoint.SetX(ui->firstPositionX->text().toDouble());
    repopulateOffFirstPoint();
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_firstPositionY_returnPressed ()
{
    firstPoint.SetY(ui->firstPositionY->text().toDouble());
    repopulateOffFirstPoint();
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_firstPositionZ_returnPressed ()
{
    firstPoint.SetZ(ui->firstPositionZ->text().toDouble());
    repopulateOffFirstPoint();
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_pickFirst_clicked ()
{
    ui->pickFirst->setChecked(true);
    this->setEnabled(false);

    pickFirstPoint=true;

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void PolycircleEditForm::on_vertexCount_returnPressed ()
{
    vertexCount=ui->vertexCount->text().toInt();
    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_OkButton_clicked ()
{
    polycircle->setCenterPoint(centerPoint);
    polycircle->setFirstPoint(firstPoint);
    polycircle->setVertexCount(vertexCount);

    polycircle->recalculate();

    emit relay->finishOperation(false,51);

    QDialog::close();
}

void PolycircleEditForm::pickVertexFinished (gp_Pnt point)
{
    this->setEnabled(true);

    if (pickCenterPoint) {
        pickCenterPoint=false;
        centerPoint=point;
        ui->centerPositionX->setText(QString::number(centerPoint.X()));
        ui->centerPositionY->setText(QString::number(centerPoint.Y()));
        ui->centerPositionZ->setText(QString::number(centerPoint.Z()));
        ui->pickCenter->setChecked(false);
        repopulateOffCenter();
    }

    if (pickFirstPoint) {
        pickFirstPoint=false;
        firstPoint=point;
        ui->firstPositionX->setText(QString::number(firstPoint.X()));
        ui->firstPositionY->setText(QString::number(firstPoint.Y()));
        ui->firstPositionZ->setText(QString::number(firstPoint.Z()));
        ui->pickFirst->setChecked(false);
        repopulateOffFirstPoint();
    }

    ui->OkButton->setEnabled(isValid());
}

void PolycircleEditForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(true,52);
    QDialog::close();
}

void PolycircleEditForm::reject ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(true,53);
    QDialog::reject();
}
