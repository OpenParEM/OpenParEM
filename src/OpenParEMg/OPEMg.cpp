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

#include "OPEMg.h"
#include "ui_OPEMg.h"
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <TopTools.hxx>


OpenParEMg::OpenParEMg(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OpenParEMg)
{
    cout << "OpenParEMg::OpenParEMg" << endl;
    ui->setupUi(this);

    projectFile="";
    init_project (&defaultData);
    init_project (&projData);

    hasProjData=false;
    ui->meshOptions->setEnabled(false);
    ui->simulateOptions->setEnabled(false);
    ui->actionFrequency_Plan->setEnabled(false);
    ui->actionRefinement->setEnabled(false);

    ui->drawingWindow->show();

    PetscInitializeNoArguments();

    cout << "exit OpenParEMg::OpenParEMg" << endl;
}

OpenParEMg::~OpenParEMg()
{
    PetscFinalize();
    delete ui;
}

void OpenParEMg::on_fileOpen_triggered()
{
    projectFile=QFileDialog::getOpenFileName(this,tr("Open Project"), "/home/briany/OpenParEM", tr("Project Files (*.proj);;All Files (*)"));

    // return if user cancels
    if (projectFile.isNull()) return;

    // break up the full path
    QFileInfo fileInfo(projectFile);
    QString absolutePath=fileInfo.absolutePath();
    QString projectName=fileInfo.fileName();

    // load the file
    if (QFile::exists(projectFile)) {

        free_project(&defaultData);
        free_project(&projData);

        init_project (&defaultData);
        init_project (&projData);

        if (load_project_file (projectFile.toLatin1().toStdString().c_str(),&projData,"   ")) {
            projectFile="";

            QMessageBox mb;
            mb.critical(nullptr, "Error", "Unable to load the requested project file.");
            mb.setFixedSize(500, 200);

            return;
        }

        // successfully loaded

        QDir::setCurrent(absolutePath);
        projectFile=projectName;

        ui->meshOptions->setEnabled(true);
        ui->simulateOptions->setEnabled(true);
        ui->actionFrequency_Plan->setEnabled(true);

        if (strcmp(projData.refinement_frequency,"none") == 0) ui->actionRefinement->setEnabled(false);
        else ui->actionRefinement->setEnabled(true);

    } else {
        // should not occur
        QMessageBox mb;
        mb.critical(nullptr, "Error", "The requested project file does not exist.");
        mb.setFixedSize(500, 200);
    }
}


void OpenParEMg::on_fileNew_triggered()
{
    cout << "creating new project file" << endl;

    projectFile="";

    free_project(&defaultData);
    free_project(&projData);

    init_project (&defaultData);
    init_project (&projData);

    ui->meshOptions->setEnabled(true);
    ui->simulateOptions->setEnabled(true);
    ui->actionFrequency_Plan->setEnabled(true);

    if (strcmp(projData.refinement_frequency,"none") == 0) ui->actionRefinement->setEnabled(false);
    else ui->actionRefinement->setEnabled(true);

    cout.flush();
}

void OpenParEMg::on_meshOptions_triggered()
{
    MeshDialog *meshDialog=new MeshDialog();
    meshDialog->set_projData(&projData);
    meshDialog->exec();
    delete meshDialog;
}


void OpenParEMg::on_simulateOptions_triggered()
{
    SimOptions *simOptions=new SimOptions();
    simOptions->set_projData(&projData);
    simOptions->exec();
    delete simOptions;
}


void OpenParEMg::on_actionLicense_triggered()
{
    License *license=new License();
    license->exec();
    delete license;
}


void OpenParEMg::on_actionFrequency_Plan_triggered()
{
    FrequencyPlanG *frequencyPlan=new FrequencyPlanG();
    frequencyPlan->set_projData(&projData);
    frequencyPlan->exec();
    delete frequencyPlan;

    if (strcmp(projData.refinement_frequency,"none") == 0) ui->actionRefinement->setEnabled(false);
    else ui->actionRefinement->setEnabled(true);
}

void OpenParEMg::on_actionSave_triggered()
{
    print_project(&projData,&defaultData,"");
    cout.flush();
}


void OpenParEMg::on_actionRefinement_triggered()
{
    OPEMg_Refinement *refinement=new OPEMg_Refinement();
    refinement->set_projData(&projData);
    refinement->exec();
    delete refinement;
}


void OpenParEMg::on_actionMaterials_Editor_triggered ()
{
    Materials *localMaterials=new Materials();
    localMaterials->exec();
    delete localMaterials;
}


void OpenParEMg::on_actionBrep_triggered ()
{
    cout << "enter OpenParEMg::on_actionBrep_triggered" << endl;

    QString filePath = QFileDialog::getOpenFileName(this, tr("Open BREP File"), "", tr("BREP Files (*.brep)"));
    if (!filePath.isEmpty()) {
        //std::ifstream brepFile(filePath.toStdString().c_str(), std::ios_base::in | std::ios_base::binary);
        std::ifstream brepFile(filePath.toStdString().c_str(), std::ios_base::in);
        if (brepFile.is_open()) {
            TopoDS_Shape s;
            BRep_Builder b;
            BRepTools::Read(s,brepFile,b);
            brepFile.close();
            //TopTools::Dump(s, std::cout);

            Handle(AIS_Shape) drawingShape=new AIS_Shape (s);
            ui->drawingWindow->displayDrawing(drawingShape);
        }
    }

    ui->drawingWindow->updateView();

    cout << "exit OpenParEMg::on_actionBrep_triggered" << endl;
}

