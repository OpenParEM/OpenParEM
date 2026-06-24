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

#ifndef ANTENNAFORM_H
#define ANTENNAFORM_H

#include <QDialog>
#include <qvalidator.h>

namespace Ui {
class AntennaForm;
}

class AntennaForm : public QDialog
{
    Q_OBJECT

public:
    explicit AntennaForm(QWidget *parent = nullptr);
    ~AntennaForm();

    void set_projData (struct projectData *);
    void set_simulationRunning (bool simulationRunning_) {simulationRunning=simulationRunning_;}

private:
    void check3Dpatterns ();
    void reject () override;

private slots:
    void on_patternG3D_stateChanged (int arg1);
    void on_patternD3D_stateChanged (int arg1);
    void on_patternEtheta3D_stateChanged (int arg1);
    void on_patternEphi3D_stateChanged (int arg1);
    void on_patternHtheta3D_stateChanged (int arg1);
    void on_patternHphi3D_stateChanged (int arg1);
    void on_plotResolution3D_currentIndexChanged (int index);
    void on_generateSphere_stateChanged (int arg1);
    void on_savePlots3D_stateChanged (int arg1);
    void on_add2Dslice_clicked ();
    void on_delete2Dslice_clicked ();
    void on_plotRange2D_valueChanged (double arg1);
    void on_axisInterval2D_valueChanged (double arg1);
    void on_plotResolution2D_valueChanged (double arg1);
    void on_dataSummary2D_stateChanged (int arg1);
    void on_savePlots2D_stateChanged (int arg1);
    void on_currentResolution_valueChanged (double arg1);
    void on_saveRawData_stateChanged (int arg1);
    void on_OkButton_clicked ();
    void on_CancelButton_clicked ();

private:
    Ui::AntennaForm *ui;

    struct projectData *projData;

    bool patternG3D;
    bool patternD3D;
    bool patternEtheta3D;
    bool patternEphi3D;
    bool patternHtheta3D;
    bool patternHphi3D;
    int plotResolution3D;
    bool generateSphere;
    bool savePlots3D;
    double plotRange2D;
    double axisInterval2D;
    double plotResolution2D;
    bool dataSummary2D;
    bool savePlots2D;
    double currentResolution;
    bool saveRawData;

    bool simulationRunning;
};

#endif // ANTENNAFORM_H
