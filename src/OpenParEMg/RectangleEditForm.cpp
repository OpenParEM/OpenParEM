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

#include "RectangleEditForm.h"
#include "ui_RectangleEditForm.h"

RectangleEditForm::RectangleEditForm (QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RectangleEditForm)
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

    ui->width->setValidator(&validator);
    ui->height->setValidator(&validator);

    ui->position2X->setValidator(&validator);
    ui->position2Y->setValidator(&validator);
    ui->position2Z->setValidator(&validator);
    ui->pick2->setCheckable(true);

    ui->OkButton->setCheckable(true);
    ui->CancelButton->setCheckable(true);

    ui->OkButton->setEnabled(false);
}

RectangleEditForm::~RectangleEditForm ()
{
    delete ui;
}

bool RectangleEditForm::isValid ()
{
    if (abs(ui->width->text().toDouble()) < Precision::Confusion()) return false;
    if (abs(ui->height->text().toDouble()) < Precision::Confusion()) return false;
    return true;
}

void RectangleEditForm::set_polywire (Rectangle *polywire_)
{
    polywire=polywire_;
    populate(polywire);
}

void RectangleEditForm::populate (Rectangle *polywire_)
{
    ui->positionX->setText(QString::number(polywire_->getPosition().X()));
    ui->positionY->setText(QString::number(polywire_->getPosition().Y()));
    ui->positionZ->setText(QString::number(polywire_->getPosition().Z()));

    ui->position2X->setText(QString::number(polywire_->getOppositeCorner().X()));
    ui->position2Y->setText(QString::number(polywire_->getOppositeCorner().Y()));
    ui->position2Z->setText(QString::number(polywire_->getOppositeCorner().Z()));

    ui->width->setText(QString::number(polywire_->getWidth()));
    ui->height->setText(QString::number(polywire_->getHeight()));
}

void RectangleEditForm::repopulateOffOrigin ()
{
    Rectangle temp(polywire);
    gp_Pnt p0(ui->positionX->text().toDouble(),ui->positionY->text().toDouble(),ui->positionZ->text().toDouble());
    temp.recalculate(p0);
    populate(&temp);
}

void RectangleEditForm::repopulateOffPositions ()
{
    Rectangle temp(polywire);
    gp_Pnt p0(ui->positionX->text().toDouble(),ui->positionY->text().toDouble(),ui->positionZ->text().toDouble());
    gp_Pnt p1(ui->position2X->text().toDouble(),ui->position2Y->text().toDouble(),ui->position2Z->text().toDouble());
    temp.recalculate(p0,p1);
    populate(&temp);
}

void RectangleEditForm::repopulateOffSize ()
{
    Rectangle temp(polywire);
    temp.setWidth(ui->width->text().toDouble());
    temp.setHeight(ui->height->text().toDouble());
    temp.recalculate();
    populate(&temp);
}

void RectangleEditForm::on_positionX_returnPressed ()
{
    repopulateOffOrigin();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_positionY_returnPressed ()
{
    repopulateOffOrigin();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_positionZ_returnPressed ()
{
    repopulateOffOrigin();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_pick_clicked ()
{
    ui->pick->setChecked(true);
    this->setEnabled(false);

    pickPoint=true;

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void RectangleEditForm::on_position2X_returnPressed ()
{
    repopulateOffPositions();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_position2Y_returnPressed ()
{
    repopulateOffPositions();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_position2Z_returnPressed ()
{
    repopulateOffPositions();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_pick2_clicked ()
{
    ui->pick2->setChecked(true);
    this->setEnabled(false);

    pickPoint2=true;

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void RectangleEditForm::on_width_returnPressed ()
{
    repopulateOffSize();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_height_returnPressed ()
{
    repopulateOffSize();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_OkButton_clicked ()
{
    ui->OkButton->setChecked(true);

    gp_Pnt p0(ui->positionX->text().toDouble(),ui->positionY->text().toDouble(),ui->positionZ->text().toDouble());
    gp_Pnt p1(ui->position2X->text().toDouble(),ui->position2Y->text().toDouble(),ui->position2Z->text().toDouble());
    polywire->recalculate(p0,p1);

    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,41);

    QDialog::close();
}

void RectangleEditForm::on_CancelButton_clicked()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true,42);
    QDialog::close();
}

void RectangleEditForm::pickVertexFinished (gp_Pnt point)
{
    this->setEnabled(true);

    if (pickPoint) {
        pickPoint=false;
        ui->positionX->setText(QString::number(point.X()));
        ui->positionY->setText(QString::number(point.Y()));
        ui->positionZ->setText(QString::number(point.Z()));
        repopulateOffOrigin();
    }

    if (pickPoint2) {
        pickPoint2=false;
        ui->position2X->setText(QString::number(point.X()));
        ui->position2Y->setText(QString::number(point.Y()));
        ui->position2Z->setText(QString::number(point.Z()));
        repopulateOffPositions();
    }

    ui->pick->setChecked(false);

    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::reject ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true,43);
    QDialog::reject();
}




