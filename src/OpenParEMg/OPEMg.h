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

#ifndef OPEMG_H
#define OPEMG_H

#include <quadmath.h>
#include <iostream>
#include "project.h"
#include <QMainWindow>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QWindow>
#include "MeshOptions.h"
#include "SimulateOptions.h"
#include "license.h"
#include "FrequencyPlanG.h"
#include "Refinement.h"
#include "Materials.h"
#include "CustomOpenGLWidget.h"
//#include "TestOpenGLWidget.h"
//#include "BreptViewerWidget.h"
#include <TopoDS_Shape.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <AIS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <AIS_InteractiveContext.hxx>
#include <Xw_Window.hxx>
#include <OpenGl_GraphicDriver.hxx>


#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <Standard_WarningsRestore.hxx>

#include <Standard_Version.hxx>




using namespace std;

extern "C" void init_project (struct projectData *);
extern "C" void free_project (struct projectData *);
extern "C" PetscErrorCode load_project_file (const char *, struct projectData *, const char *);
extern "C" void print_project (struct projectData *, struct projectData *, const char *);

QT_BEGIN_NAMESPACE
namespace Ui {
class OpenParEMg;
}
QT_END_NAMESPACE

class OpenParEMg : public QMainWindow
{
    Q_OBJECT

public:
    OpenParEMg(QWidget *parent = nullptr);
    ~OpenParEMg();

private slots:
    void on_fileOpen_triggered();

    void on_fileNew_triggered();

    void on_meshOptions_triggered();

    void on_simulateOptions_triggered();

    void on_actionLicense_triggered();

    void on_actionFrequency_Plan_triggered();

    void on_actionSave_triggered();

    void on_actionRefinement_triggered();

    void on_actionMaterials_Editor_triggered();

    void on_actionBrep_triggered();

private:
    Ui::OpenParEMg *ui;
    bool hasProjData;
    QString projectFile;
    struct projectData projData,defaultData;
};
#endif // OPEMG_H
