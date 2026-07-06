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
    this->setWindowIcon(QApplication::windowIcon());

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

    conversionFactor=1;
    isXclose=true;
    isClosing=false;
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
    if (isClosing) return;

    // temporarily draw the shape
    if (!tempShape.IsNull()) {
        drawingWindow->removeShape(tempShape);
        tempShape.Nullify();
    }
    tempShape=polycircle_->get_AIS_Shape();
    if (!tempShape.IsNull()) {
        drawingWindow->displayShape(tempShape);
        drawingWindow->updateViewer();
    }

    centerPoint=polycircle_->getCenterPoint();
    firstPoint=polycircle_->getFirstPoint();
    vertexCount=polycircle_->getVertexCount();

    ui->centerPositionX->setText(QString::number(centerPoint.X()*conversionFactor,'g',15));
    ui->centerPositionY->setText(QString::number(centerPoint.Y()*conversionFactor,'g',15));
    ui->centerPositionZ->setText(QString::number(centerPoint.Z()*conversionFactor,'g',15));

    ui->firstPositionX->setText(QString::number(firstPoint.X()*conversionFactor,'g',15));
    ui->firstPositionY->setText(QString::number(firstPoint.Y()*conversionFactor,'g',15));
    ui->firstPositionZ->setText(QString::number(firstPoint.Z()*conversionFactor,'g',15));

    radius=centerPoint.Distance(firstPoint);
    ui->radius->setText(QString::number(radius*conversionFactor,'g',15));

    ui->vertexCount->setText(QString::number(vertexCount));
}

void PolycircleEditForm::set_Polycircle (Polycircle *polycircle_)
{
    polycircle=polycircle_;
    populate(polycircle);
}

void PolycircleEditForm::repopulate ()
{
    Polycircle temp(polycircle);
    temp.setCenterPoint(centerPoint);
    temp.setFirstPoint(firstPoint);
    temp.setVertexCount(vertexCount);
    temp.recalculate();
    populate(&temp);
}

void PolycircleEditForm::on_centerPositionX_editingFinished ()
{
    firstPoint.SetX(firstPoint.X()-centerPoint.X()+ui->centerPositionX->text().toDouble()/conversionFactor);
    centerPoint.SetX(ui->centerPositionX->text().toDouble()/conversionFactor);
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void PolycircleEditForm::on_centerPositionY_editingFinished ()
{
    firstPoint.SetY(firstPoint.Y()-centerPoint.Y()+ui->centerPositionY->text().toDouble()/conversionFactor);
    centerPoint.SetY(ui->centerPositionY->text().toDouble()/conversionFactor);
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void PolycircleEditForm::on_centerPositionZ_editingFinished ()
{
    firstPoint.SetZ(firstPoint.Z()-centerPoint.Z()+ui->centerPositionZ->text().toDouble()/conversionFactor);
    centerPoint.SetZ(ui->centerPositionZ->text().toDouble()/conversionFactor);
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void PolycircleEditForm::on_pickCenter_clicked ()
{
    ui->pickCenter->setChecked(true);
    this->setEnabled(false);

    pickCenterPoint=true;

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

// void PolycircleEditForm::repopulateOffFirstPoint ()
// {
//     Polycircle temp(polycircle);
//     temp.setFirstPoint(firstPoint);
//     temp.recalculate();
//     populate(&temp);
// }

void PolycircleEditForm::on_radius_editingFinished ()
{
    if (ui->radius->text().toDouble() < Precision::Confusion()) return;
    radius=ui->radius->text().toDouble()/conversionFactor;

    gp_Vec dir(centerPoint,firstPoint);
    dir.Normalize();
    gp_Pnt newFirstPoint=centerPoint.Translated(dir*radius);

    Polycircle temp(polycircle);
    temp.setCenterPoint(centerPoint);
    temp.setFirstPoint(newFirstPoint);
    temp.recalculate();
    populate(&temp);

    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void PolycircleEditForm::on_firstPositionX_editingFinished ()
{
    firstPoint.SetX(ui->firstPositionX->text().toDouble()/conversionFactor);
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void PolycircleEditForm::on_firstPositionY_editingFinished ()
{
    firstPoint.SetY(ui->firstPositionY->text().toDouble()/conversionFactor);
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void PolycircleEditForm::on_firstPositionZ_editingFinished ()
{
    firstPoint.SetZ(ui->firstPositionZ->text().toDouble()/conversionFactor);
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void PolycircleEditForm::on_pickFirst_clicked ()
{
    ui->pickFirst->setChecked(true);
    this->setEnabled(false);

    pickFirstPoint=true;

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void PolycircleEditForm::on_vertexCount_editingFinished ()
{
    vertexCount=ui->vertexCount->text().toInt();
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void PolycircleEditForm::on_OkButton_clicked ()
{
    polycircle->setCenterPoint(centerPoint);
    polycircle->setFirstPoint(firstPoint);
    polycircle->setVertexCount(vertexCount);

    polycircle->recalculate();

    if (!tempShape.IsNull()) {
        drawingWindow->removeShape(tempShape);
        tempShape.Nullify();
    }

    emit relay->finishOperation(false,51);

    isXclose=false;
    QDialog::close();
}

void PolycircleEditForm::pickVertexFinished (gp_Pnt point)
{
    this->setEnabled(true);

    if (pickCenterPoint) {
        pickCenterPoint=false;
        centerPoint=point;
        ui->pickCenter->setChecked(false);
    }

    if (pickFirstPoint) {
        pickFirstPoint=false;
        firstPoint=point;
        ui->pickFirst->setChecked(false);
    }

    repopulate();

    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void PolycircleEditForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);

    if (!tempShape.IsNull()) {
        drawingWindow->removeShape(tempShape);
        tempShape.Nullify();
    }

    emit relay->finishOperation(true,52);

    isXclose=false;
    QDialog::close();
}

void PolycircleEditForm::reject ()
{
    isClosing=true;

    if (!tempShape.IsNull()) {
        drawingWindow->removeShape(tempShape);
        tempShape.Nullify();
    }

    ui->CancelButton->setChecked(true);
    if (isXclose) emit relay->finishOperation(true,53);
    QDialog::reject();
}
