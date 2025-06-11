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

#ifndef FREQUENCYVIEW_H
#define FREQUENCYVIEW_H

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidgetItem>
#include <quadmath.h>
#include "frequencyPlan.hpp"
#include "project.h"
#include "prefix.h"

namespace Ui {
class FrequencyView;
}

class FrequencyView : public QDialog
{
    Q_OBJECT

public:
    explicit FrequencyView(QWidget *parent = nullptr);
    ~FrequencyView();
    void populate (struct projectData *);

private slots:
    void on_frequencyViewClose_clicked();

private:
    Ui::FrequencyView *ui;
    FrequencyPlan frequencyPlan;
    int scrollBarWidth;
    int verticalHeaderWidth;
    int scrollBarOffset;
    int viewBoxWidth;
    int frequencyColWidth;
    int priorityColWidth;
    int restartColWidth;
};

#endif // FREQUENCYVIEW_H
