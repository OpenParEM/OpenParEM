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

    ui->length->setValidator(&validator);

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
        p0=polywire_->getP0();
        p1=polywire_->getP1();

        ui->positionX->setText(QString::number(p0.X()));
        ui->positionY->setText(QString::number(p0.Y()));
        ui->positionZ->setText(QString::number(p0.Z()));

        ui->position2X->setText(QString::number(p1.X()));
        ui->position2Y->setText(QString::number(p1.Y()));
        ui->position2Z->setText(QString::number(p1.Z()));

        ui->length->setText(QString::number(p0.Distance(p1)));
    }
}

void LineEditForm::repopulate ()
{
    polywire->setP0(p0);
    polywire->setP1(p1);
}

void LineEditForm::on_length_returnPressed ()
{
    if (ui->length->text().toDouble() == 0) return;

    gp_Vec dir(p0,p1);
    dir.Normalize();
    dir*=ui->length->text().toDouble();
    p1=p0.Translated(dir);

    ui->position2X->setText(QString::number(p1.X()));
    ui->position2Y->setText(QString::number(p1.Y()));
    ui->position2Z->setText(QString::number(p1.Z()));

    ui->OkButton->setEnabled(isValid());
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
        p0=point;
        ui->positionX->setText(QString::number(p0.X()));
        ui->positionY->setText(QString::number(p0.Y()));
        ui->positionZ->setText(QString::number(p0.Z()));
    }

    if (pickPoint2) {
        pickPoint2=false;
        p1=point;
        ui->position2X->setText(QString::number(p1.X()));
        ui->position2Y->setText(QString::number(p1.Y()));
        ui->position2Z->setText(QString::number(p1.Z()));
    }

    ui->length->setText(QString::number(p0.Distance(p1)));

    ui->pick->setChecked(false);

    ui->OkButton->setEnabled(isValid());
}

void LineEditForm::reject ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true,63);
    QDialog::reject();
}

