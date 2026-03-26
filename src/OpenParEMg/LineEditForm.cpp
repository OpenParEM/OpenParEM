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

    ui->position2X->setValidator(&validator);
    ui->position2Y->setValidator(&validator);
    ui->position2Z->setValidator(&validator);
    ui->pick2->setCheckable(true);

    ui->OkButton->setCheckable(true);
    ui->CancelButton->setCheckable(true);

    ui->OkButton->setEnabled(false);
}

LineEditForm::~LineEditForm ()
{
    delete ui;
}

bool LineEditForm::isValid ()
{
    gp_Pnt p0(ui->positionX->text().toDouble(),ui->positionY->text().toDouble(),ui->positionZ->text().toDouble());
    gp_Pnt p1(ui->position2X->text().toDouble(),ui->position2Y->text().toDouble(),ui->position2Z->text().toDouble());

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
    if (polywire_) {
        ui->positionX->setText(QString::number(polywire_->getP0().X()));
        ui->positionY->setText(QString::number(polywire_->getP0().Y()));
        ui->positionZ->setText(QString::number(polywire_->getP0().Z()));

        ui->position2X->setText(QString::number(polywire_->getP1().X()));
        ui->position2Y->setText(QString::number(polywire_->getP1().Y()));
        ui->position2Z->setText(QString::number(polywire_->getP1().Z()));
    }
}

void LineEditForm::repopulate ()
{
    gp_Pnt p0(ui->positionX->text().toDouble(),ui->positionY->text().toDouble(),ui->positionZ->text().toDouble());
    gp_Pnt p1(ui->position2X->text().toDouble(),ui->position2Y->text().toDouble(),ui->position2Z->text().toDouble());

    polywire->setP0(p0);
    polywire->setP1(p1);
}

void LineEditForm::on_positionX_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
}

void LineEditForm::on_positionY_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
}

void LineEditForm::on_positionZ_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
}

void LineEditForm::on_pick_clicked ()
{
    ui->pick->setChecked(true);
    this->setEnabled(false);

    pickPoint=true;

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void LineEditForm::on_position2X_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
}

void LineEditForm::on_position2Y_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
}

void LineEditForm::on_position2Z_returnPressed ()
{
    ui->OkButton->setEnabled(isValid());
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
    repopulate();
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,61);
    QDialog::close();
}

void LineEditForm::on_CancelButton_clicked ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true,62);
    QDialog::close();
}

void LineEditForm::pickVertexFinished (gp_Pnt point)
{
    this->setEnabled(true);

    if (pickPoint) {
        pickPoint=false;
        ui->positionX->setText(QString::number(point.X()));
        ui->positionY->setText(QString::number(point.Y()));
        ui->positionZ->setText(QString::number(point.Z()));
    }

    if (pickPoint2) {
        pickPoint2=false;
        ui->position2X->setText(QString::number(point.X()));
        ui->position2Y->setText(QString::number(point.Y()));
        ui->position2Z->setText(QString::number(point.Z()));
    }

    ui->pick->setChecked(false);

    ui->OkButton->setEnabled(isValid());
}

void LineEditForm::reject ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true,63);
    QDialog::reject();
}

