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

    ui->width->setValidator(&validator);
    ui->height->setValidator(&validator);

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

RectangleEditForm::~RectangleEditForm ()
{
    delete ui;
}

bool RectangleEditForm::isValid ()
{
    if (abs(width) < Precision::Confusion()) return false;
    if (abs(height) < Precision::Confusion()) return false;
    return true;
}

void RectangleEditForm::set_polywire (Rectangle *polywire_)
{
    polywire=polywire_;
    populate(polywire);
}

void RectangleEditForm::populate (Rectangle *polywire_)
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

    // local variables
    p0=polywire_->getPosition();
    p1=polywire_->getOppositeCorner();
    width=polywire_->getWidth();
    height=polywire_->getHeight();

    // fill form

    ui->positionX->setText(QString::number(p0.X()*conversionFactor,'g',15));
    ui->positionY->setText(QString::number(p0.Y()*conversionFactor,'g',15));
    ui->positionZ->setText(QString::number(p0.Z()*conversionFactor,'g',15));

    ui->position2X->setText(QString::number(p1.X()*conversionFactor,'g',15));
    ui->position2Y->setText(QString::number(p1.Y()*conversionFactor,'g',15));
    ui->position2Z->setText(QString::number(p1.Z()*conversionFactor,'g',15));

    ui->width->setText(QString::number(width*conversionFactor,'g',15));
    ui->height->setText(QString::number(height*conversionFactor,'g',15));
}

void RectangleEditForm::repopulate ()
{
    Rectangle temp(polywire);
    temp.recalculate(p0,p1);
    populate(&temp);
}

void RectangleEditForm::repopulateOffSize ()
{
    Rectangle temp(polywire);
    temp.recalculate(p0,width,height);
    populate(&temp);
}

void RectangleEditForm::on_positionX_editingFinished ()
{
    p0.SetX(ui->positionX->text().toDouble()/conversionFactor);
    repopulateOffSize();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_positionY_editingFinished ()
{
    p0.SetY(ui->positionY->text().toDouble()/conversionFactor);
    repopulateOffSize();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_positionZ_editingFinished ()
{
    p0.SetZ(ui->positionZ->text().toDouble()/conversionFactor);
    repopulateOffSize();
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

void RectangleEditForm::on_position2X_editingFinished ()
{
    p1.SetX(ui->position2X->text().toDouble()/conversionFactor);
    repopulate();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_position2Y_editingFinished ()
{
    p1.SetY(ui->position2Y->text().toDouble()/conversionFactor);
    repopulate();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_position2Z_editingFinished ()
{
    p1.SetZ(ui->position2Z->text().toDouble()/conversionFactor);
    repopulate();
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

void RectangleEditForm::on_width_editingFinished ()
{
    width=ui->width->text().toDouble()/conversionFactor;
    repopulateOffSize();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_height_editingFinished ()
{
    height=ui->height->text().toDouble()/conversionFactor;
    repopulateOffSize();
    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::on_OkButton_clicked ()
{
    ui->OkButton->setChecked(true);

    polywire->recalculate(p0,p1);

    if (!tempShape.IsNull()) {
        drawingWindow->removeShape(tempShape);
        tempShape.Nullify();
    }

    emit relay->finishOperation(false,41);

    isXclose=false;
    QDialog::close();
}

void RectangleEditForm::on_CancelButton_clicked()
{
    ui->CancelButton->setChecked(true);

    if (!tempShape.IsNull()) {
        drawingWindow->removeShape(tempShape);
        tempShape.Nullify();
    }

    emit relay->finishOperation(true,42);

    isXclose=false;
    QDialog::close();
}

void RectangleEditForm::pickVertexFinished (gp_Pnt point)
{
    setEnabled(true);

    if (pickPoint) {
        pickPoint=false;
        p0=point;
        ui->pick->setChecked(false);
    }

    if (pickPoint2) {
        pickPoint2=false;
        p1=point;
        ui->pick2->setChecked(false);
    }

    repopulate();

    ui->OkButton->setEnabled(isValid());
}

void RectangleEditForm::reject ()
{
    isClosing=true;

    if (!tempShape.IsNull()) {
        drawingWindow->removeShape(tempShape);
        tempShape.Nullify();
    }

    ui->CancelButton->setChecked(true);
    if(isXclose) emit relay->finishOperation(true,43);
    QDialog::reject();
}




