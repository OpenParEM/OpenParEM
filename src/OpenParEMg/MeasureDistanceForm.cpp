////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEMg - A GUI for OpenParEM3D                                      //
//    Copyright (C) 2026 Brian Young                                          //
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

#include "MeasureDistanceForm.h"
#include "ui_MeasureDistanceForm.h"

#include "BRepBuilderAPI_MakeEdge.hxx"

MeasureDistanceForm::MeasureDistanceForm(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MeasureDistanceForm)
{
    ui->setupUi(this);

    startPnt.SetCoord(0,0,0);
    endPnt.SetCoord(0,0,0);

    ui->startX->setText("0");
    ui->startY->setText("0");
    ui->startZ->setText("0");

    ui->endX->setText("0");
    ui->endY->setText("0");
    ui->endZ->setText("0");

    ui->lengthLabel->setText("Length=0");

    isXclose=true;

    // position
    QRect parentRect=parent->geometry();
    int x=parentRect.right()-width()-20;
    int y=parentRect.top()+20;
    move(mapToGlobal(QPoint(x,y)));
}

MeasureDistanceForm::~MeasureDistanceForm ()
{
    delete ui;
}

void MeasureDistanceForm::on_pickOrigin_clicked ()
{
    pickStartPoint=true;
    pickEndPoint=false;
    this->setEnabled(false);

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void MeasureDistanceForm::on_pickTip_clicked ()
{
    pickStartPoint=false;
    pickEndPoint=true;
    this->setEnabled(false);

    drawingWindow->set_pickFirstVertex(true);
    drawingWindow->updateViewer();
}

void MeasureDistanceForm::on_CloseButton_clicked ()
{
    ui->CloseButton->setChecked(true);
    if (!line.IsNull()) {
        drawingWindow->removeShape(line);
        line.Nullify();
    }
    emit relay->finishOperation(false,71);
    isXclose=false;
    QDialog::close();
}

void MeasureDistanceForm::pickVertexFinished (gp_Pnt point)
{
    this->setEnabled(true);

    if (pickStartPoint) {
        startPnt=point;
        ui->startX->setText(QString::number(startPnt.X()*conversionFactor,'g',15));
        ui->startY->setText(QString::number(startPnt.Y()*conversionFactor,'g',15));
        ui->startZ->setText(QString::number(startPnt.Z()*conversionFactor,'g',15));
        activateWindow();
        raise();
        ui->pickTip->setFocus();

        pickStartPoint=false;
        drawingWindow->set_pickFirstVertex(false);
    }

    if (pickEndPoint) {
        endPnt=point;
        ui->endX->setText(QString::number(endPnt.X()*conversionFactor,'g',15));
        ui->endY->setText(QString::number(endPnt.Y()*conversionFactor,'g',15));
        ui->endZ->setText(QString::number(endPnt.Z()*conversionFactor,'g',15));
        activateWindow();
        raise();
        ui->pickOrigin->setFocus();

        pickEndPoint=false;
        drawingWindow->set_pickFirstVertex(false);
    }

    if (!line.IsNull()) {
        drawingWindow->removeShape(line);
        line.Nullify();
    }
    TopoDS_Edge edge=BRepBuilderAPI_MakeEdge(startPnt,endPnt);
    if (!edge.IsNull()) {
        line=new AIS_Shape(edge);
        if (!line.IsNull()) {
            drawingWindow->displayShape(line);
        }
    }

    QString lengthText="length=";
    lengthText.append(QString::number(startPnt.Distance(endPnt),'g',15));
    ui->lengthLabel->setText(lengthText);
}

void MeasureDistanceForm::reject ()
{
    ui->CloseButton->setChecked(true);
    if (isXclose) {
        if (!line.IsNull()) {
            drawingWindow->removeShape(line);
            line.Nullify();
        }

        emit relay->finishOperation(true,33);
    }
    QDialog::reject();
}

