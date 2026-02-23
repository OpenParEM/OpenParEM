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

    ui->positionX->setValidator(&validator);
    ui->positionY->setValidator(&validator);
    ui->positionZ->setValidator(&validator);
    ui->width->setValidator(&validator);
    ui->height->setValidator(&validator);

    ui->pick->setCheckable(true);
    ui->OkButton->setCheckable(true);
    ui->CancelButton->setCheckable(true);

    ui->OkButton->setEnabled(false);
}

RectangleEditForm::~RectangleEditForm ()
{
    delete ui;
}

void RectangleEditForm::set_polywire (Rectangle *polywire_)
{
    polywire=polywire_;

    ui->positionX->setText(QString::number(polywire->getPosition().X()));
    ui->positionY->setText(QString::number(polywire->getPosition().Y()));
    ui->positionZ->setText(QString::number(polywire->getPosition().Z()));

    ui->width->setText(QString::number(polywire->getWidth()));
    ui->height->setText(QString::number(polywire->getHeight()));
}

void RectangleEditForm::on_positionX_returnPressed ()
{
    position.SetX(ui->positionX->text().toDouble());
    ui->OkButton->setEnabled(true);
}

void RectangleEditForm::on_positionY_returnPressed ()
{
    position.SetY(ui->positionX->text().toDouble());
    ui->OkButton->setEnabled(true);
}

void RectangleEditForm::on_positionZ_returnPressed ()
{
    position.SetZ(ui->positionX->text().toDouble());
    ui->OkButton->setEnabled(true);
}

void RectangleEditForm::on_pick_clicked ()
{
    ui->pick->setChecked(true);

    drawingWindow->unselectAllItems();
    drawingWindow->set_pickVertex(true);
    drawingWindow->updateViewer();

    ui->OkButton->setEnabled(true);
}

void RectangleEditForm::on_width_returnPressed ()
{
    width=ui->positionX->text().toDouble();
    ui->OkButton->setEnabled(true);
}

void RectangleEditForm::on_height_returnPressed ()
{
    height=ui->positionX->text().toDouble();
    ui->OkButton->setEnabled(true);
}

void RectangleEditForm::on_OkButton_clicked ()
{
    ui->OkButton->setChecked(true);

    gp_Pnt position(ui->positionX->text().toDouble(),ui->positionY->text().toDouble(),ui->positionZ->text().toDouble());
    if (!position.IsEqual(polywire->getPosition(),Precision::Confusion())) {
        polywire->moveTo(position);
    }

    bool recalculate=false;

    if (abs(ui->width->text().toDouble()-polywire->getWidth()) > Precision::Confusion()) {
        polywire->setWidth(ui->width->text().toDouble());
        recalculate=true;
    }

    if (abs(ui->height->text().toDouble()-polywire->getHeight()) > Precision::Confusion()) {
        polywire->setHeight(ui->height->text().toDouble());
        recalculate=true;
    }

    if (recalculate) polywire->recalculate();

    emit relay->finishEditObject(false);

    QDialog::close();
}

void RectangleEditForm::on_CancelButton_clicked()
{
    ui->CancelButton->setChecked(true);
    QDialog::close();
}

void RectangleEditForm::pickVertexFinished (gp_Pnt point)
{
    ui->positionX->setText(QString::number(point.X()));
    ui->positionY->setText(QString::number(point.Y()));
    ui->positionZ->setText(QString::number(point.Z()));
    ui->pick->setChecked(false);

    ui->OkButton->setEnabled(true);
}

void RectangleEditForm::reject ()
{
    ui->CancelButton->setChecked(true);
    emit relay->finishEditObject(true);

    QDialog::reject();
}

