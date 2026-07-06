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


#include "LineEditForm.h"
#include "ui_LineEditForm.h"

LineEditForm::LineEditForm (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LineEditForm)
{
    this->setWindowIcon(QApplication::windowIcon());

    ui->setupUi(this);

    setFixedSize(QDialog::width(),QDialog::height());

    validator.setNotation(QDoubleValidator::ScientificNotation);
    polywire=nullptr;

    pickPoint=false;
    pickPoint2=false;

    ui->positionX->setValidator(&validator);
    ui->positionY->setValidator(&validator);
    ui->positionZ->setValidator(&validator);
    ui->pick->setCheckable(true);

    ui->length->setValidator(&validator);

    ui->position2X->setValidator(&validator);
    ui->position2Y->setValidator(&validator);
    ui->position2Z->setValidator(&validator);
    ui->pick2->setCheckable(true);

    ui->OkButton->setCheckable(true);
    ui->CancelButton->setCheckable(true);

    ui->OkButton->setEnabled(false);

    conversionFactor=1;
    isXclose=true;
    isClosing=false;
}

LineEditForm::~LineEditForm ()
{
    delete ui;
}

bool LineEditForm::isValid ()
{
    if (p0.IsEqual(p1,Precision::Confusion())) return false;
    return true;
}

void LineEditForm::set_polywire (Line *polywire_)
{
    polywire=polywire_;
    populate(polywire);
}

void LineEditForm::populate (Line *polywire_)
{
    if (isClosing) return;

    // temporarily draw the shape
    if (!tempShape.IsNull()) {
        drawingWindow->removeShape(tempShape);
        tempShape.Nullify();
    }
    tempShape=polywire_->get_AIS_Shape();
    if (!tempShape.IsNull()) {
        drawingWindow->displayShape(tempShape);
        drawingWindow->updateViewer();
    }

    p0=polywire_->getP0();
    p1=polywire_->getP1();
    length=p0.Distance(p1);

    ui->positionX->setText(QString::number(p0.X()*conversionFactor,'g',15));
    ui->positionY->setText(QString::number(p0.Y()*conversionFactor,'g',15));
    ui->positionZ->setText(QString::number(p0.Z()*conversionFactor,'g',15));

    ui->position2X->setText(QString::number(p1.X()*conversionFactor,'g',15));
    ui->position2Y->setText(QString::number(p1.Y()*conversionFactor,'g',15));
    ui->position2Z->setText(QString::number(p1.Z()*conversionFactor,'g',15));

    ui->length->setText(QString::number(length*conversionFactor,'g',15));
}

void LineEditForm::repopulate ()
{
    Line temp=Line(polywire);
    temp.setP0(p0);
    temp.setP1(p1);
    populate(&temp);
}

void LineEditForm::on_length_editingFinished ()
{
    if (ui->length->text().toDouble() == 0) return;
    length=ui->length->text().toDouble();

    gp_Vec dir(p0,p1);
    dir.Normalize();
    dir*=length/conversionFactor;
    p1=p0.Translated(dir); 
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void LineEditForm::on_positionX_editingFinished ()
{
    p0.SetX(ui->positionX->text().toDouble()/conversionFactor);
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void LineEditForm::on_positionY_editingFinished ()
{
    p0.SetY(ui->positionY->text().toDouble()/conversionFactor);
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void LineEditForm::on_positionZ_editingFinished ()
{
    p0.SetZ(ui->positionZ->text().toDouble()/conversionFactor);
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void LineEditForm::on_pick_clicked ()
{
    ui->pick->setChecked(true);
    this->setEnabled(false);

    pickPoint=true;

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void LineEditForm::on_position2X_editingFinished ()
{
    p1.SetX(ui->position2X->text().toDouble()/conversionFactor);
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void LineEditForm::on_position2Y_editingFinished ()
{
    p1.SetY(ui->position2Y->text().toDouble()/conversionFactor);
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void LineEditForm::on_position2Z_editingFinished ()
{
    p1.SetZ(ui->position2Z->text().toDouble()/conversionFactor);
    repopulate();
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void LineEditForm::on_pick2_clicked ()
{
    ui->pick2->setChecked(true);
    this->setEnabled(false);

    pickPoint2=true;

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void LineEditForm::on_OkButton_clicked ()
{
    polywire->setP0(p0);
    polywire->setP1(p1);

    if (!tempShape.IsNull()) {
        drawingWindow->removeShape(tempShape);
        tempShape.Nullify();
    }

    emit relay->finishOperation(false,61);

    isXclose=false;
    QDialog::close();
}

void LineEditForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);

    if (!tempShape.IsNull()) {
        drawingWindow->removeShape(tempShape);
        tempShape.Nullify();
    }

    emit relay->finishOperation(true,62);

    isXclose=false;
    QDialog::close();
}

void LineEditForm::pickVertexFinished (gp_Pnt point)
{
    this->setEnabled(true);

    if (pickPoint) {
        pickPoint=false;
        p0=point;
    }

    if (pickPoint2) {
        pickPoint2=false;
        p1=point;
    }

    repopulate();
    ui->pick->setChecked(false);
    if (isValid()) {
        ui->OkButton->setEnabled(true);
    }
}

void LineEditForm::reject ()
{
    isClosing=true;

    if (!tempShape.IsNull()) {
        drawingWindow->removeShape(tempShape);
        tempShape.Nullify();
    }

    ui->CancelButton->setChecked(true);
    if (isXclose) emit relay->finishOperation(true,63);
    QDialog::reject();
}

