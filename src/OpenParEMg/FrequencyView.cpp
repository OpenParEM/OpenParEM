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

#include "FrequencyView.h"
#include "ui_FrequencyView.h"
#include <qlineedit.h>

FrequencyView::FrequencyView(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FrequencyView)
{
    ui->setupUi(this);

    ui->frequencyView->insertColumn(0);  // frequency
    ui->frequencyView->insertColumn(1);  // refinement priority
    ui->frequencyView->insertColumn(2);  // restart

    viewBoxWidth=421;     // from FrequencyView.ui
    frequencyColWidth=120;
    restartColWidth=90;
    priorityColWidth=viewBoxWidth-frequencyColWidth-restartColWidth;

    ui->frequencyView->setColumnWidth(0,frequencyColWidth);
    ui->frequencyView->setColumnWidth(1,priorityColWidth);
    ui->frequencyView->setColumnWidth(2,restartColWidth);

    scrollBarWidth=qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    scrollBarOffset=0;

    QStringList headers;
    headers << "Frequency" << "Refinement Priority" << "Restart";
    ui->frequencyView->setHorizontalHeaderLabels(headers);

    QTableWidgetItem *headerItem=ui->frequencyView->horizontalHeaderItem(0);
    if (headerItem) headerItem->setToolTip("Simulation frequencies.");

    headerItem=ui->frequencyView->horizontalHeaderItem(1);
    if (headerItem) headerItem->setToolTip("Adaptive mesh refinement is applied at frequencies marked with integers in numerical order starting with 1.");

    headerItem=ui->frequencyView->horizontalHeaderItem(2);
    if (headerItem) headerItem->setToolTip("Adaptive mesh refinement restarts with the initial mesh if the frequency is marked with \"restart\".");
}

FrequencyView::~FrequencyView()
{
    delete ui;
}

void FrequencyView::populate(struct projectData *projData)
{
    if (projData->inputFrequencyPlansCount > 0 &&
        !frequencyPlan.assemble(projData->refinement_frequency,projData->inputFrequencyPlansCount,projData->inputFrequencyPlans)) {
        int currentRow=0;
        long unsigned int i=0;
        while (i < frequencyPlan.get_plan_size()) {
            FrequencyPlanPoint *frequency=frequencyPlan.get_frequency(i);

            if (frequency && frequency->get_active()) {

                ui->frequencyView->insertRow(currentRow);

                QLineEdit *entry=new QLineEdit();
                entry->setText(QString::number(frequency->get_frequency()));
                entry->setAlignment(Qt::AlignHCenter);
                entry->setReadOnly(true);
                ui->frequencyView->setCellWidget(currentRow,0,entry);

                if (frequency->get_refinementPriority() > 0) {

                    QLineEdit *entry=new QLineEdit();
                    entry->setText(QString::number(frequency->get_refinementPriority()));
                    entry->setAlignment(Qt::AlignHCenter);
                    entry->setReadOnly(true);
                    ui->frequencyView->setCellWidget(currentRow,1,entry);

                    if (frequency->get_restart()) {
                        entry=new QLineEdit();
                        entry->setText("restart");
                        entry->setAlignment(Qt::AlignHCenter);
                        entry->setReadOnly(true);
                        ui->frequencyView->setCellWidget(currentRow,2,entry);
                    }
                }
                currentRow++;
            }
            i++;
        }

        if (ui->frequencyView->rowCount() > 8) {
            scrollBarOffset=scrollBarWidth;
        }

        ui->frequencyView->scrollToBottom();  // trick to refresh the vertical header so that
        ui->frequencyView->scrollToTop();     // verticalHeader()->width() does not return 0
        verticalHeaderWidth=ui->frequencyView->verticalHeader()->width();

        ui->frequencyView->setColumnWidth(0,frequencyColWidth);
        ui->frequencyView->setColumnWidth(1,priorityColWidth-scrollBarOffset-verticalHeaderWidth-2);
        ui->frequencyView->setColumnWidth(2,restartColWidth);
    }
}

void FrequencyView::on_frequencyViewClose_clicked()
{
    close();
}

