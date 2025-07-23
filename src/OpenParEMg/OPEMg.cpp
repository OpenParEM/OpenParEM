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

#include <quadmath.h>
#include <iostream>

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>

#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QWindow>
#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QList>
#include <QTreeWidgetItem>

//#include "petscsys.h"
#include "MeshOptions.h"
#include "SimulateOptions.h"
#include "license.h"
#include "FrequencyPlanG.h"
#include "Refinement.h"
#include "Materials.h"
#include "CustomOpenGLWidget.h"
#include "CustomAIS_Shape.h"

void deleteChildren (QTreeWidgetItem *item)
{
    if (!item) return;

    QList<QTreeWidgetItem*> children=item->takeChildren();
    for (QTreeWidgetItem* child : children) {
        deleteChildren(child);
        delete child;
    }
}

OpenParEMg::OpenParEMg(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OpenParEMg)
{
    ui->setupUi(this);

    absolutePath=QDir::currentPath();
    boundaryDatabase=new BoundaryDatabase();

    projectFile="";
    init_project (&defaultData);
    init_project (&projData);

    hasProjData=false;
    ui->meshOptions->setEnabled(false);
    ui->simulateOptions->setEnabled(false);
    ui->actionFrequency_Plan->setEnabled(false);
    ui->actionRefinement->setEnabled(false);

    ui->drawingWindow->show();

    ui->drawingItemTree->setHeaderHidden(true);

    drawing.setText(0,"Drawing");
    ui->drawingItemTree->addTopLevelItem(&drawing);
    port.setText(0,"Port");
    ui->drawingItemTree->addTopLevelItem(&port);
    boundary.setText(0,"Boundary");
    ui->drawingItemTree->addTopLevelItem(&boundary);


    ui->drawingItemTree->show();

    PetscInitializeNoArguments();
}

OpenParEMg::~OpenParEMg()
{
    PetscFinalize();
    delete ui;
}

void OpenParEMg::on_fileOpen_triggered()
{
    projectFile=QFileDialog::getOpenFileName(this,tr("Open Project"), "", tr("Project Files (*.proj);;All Files (*)"));

    // return if user cancels
    if (projectFile.isNull()) return;

    // break up the full path
    QFileInfo fileInfo(projectFile);
    absolutePath=fileInfo.absolutePath();
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

        print_project(&projData,&defaultData,">>>>  ");

        std::cout << "OpenParEMg::on_fileOpen_triggered()  projData.solution_impedance_definition=" << projData.solution_impedance_definition << std::endl;
        std::cout << "OpenParEMg::on_fileOpen_triggered()  projData.solution_impedance_calculation=" << projData.solution_impedance_calculation << std::endl;

        // successfully loaded

        QDir::setCurrent(absolutePath);
        projectFile=projectName;

        ui->meshOptions->setEnabled(true);
        ui->simulateOptions->setEnabled(true);
        ui->actionFrequency_Plan->setEnabled(true);

        if (strcmp(projData.refinement_frequency,"none") == 0) ui->actionRefinement->setEnabled(false);
        else ui->actionRefinement->setEnabled(true);

        // load boundaries, if any, and draw
        if (!boundaryDatabase->load(projData.port_definition_file,projData.solution_check_closed_loop)) {
            boundaryDatabase->draw(&projData,ui->drawingWindow,ui->drawingItemTree,&port,&boundary);
        }

    } else {
        // should not occur
        QMessageBox mb;
        mb.critical(nullptr, "Error", "The requested project file does not exist.");
        mb.setFixedSize(500, 200);
    }
}

void OpenParEMg::on_fileNew_triggered()
{
    projectFile="";

    free_project(&defaultData);
    free_project(&projData);

    init_project (&defaultData);
    init_project (&projData);

    if (boundaryDatabase) delete boundaryDatabase;
    boundaryDatabase=new BoundaryDatabase();

    ui->meshOptions->setEnabled(true);
    ui->simulateOptions->setEnabled(true);
    ui->actionFrequency_Plan->setEnabled(true);

    if (strcmp(projData.refinement_frequency,"none") == 0) ui->actionRefinement->setEnabled(false);
    else ui->actionRefinement->setEnabled(true);

    ui->drawingWindow->clearDrawing();
    ui->drawingWindow->updateView();
    deleteChildren(&drawing);
    deleteChildren(&port);
    deleteChildren(&boundary);
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
    std::cout.flush();
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

void ListChildren(const TopoDS_Shape& theShape) {
    // Using TopoDS_Iterator (iterates immediate sub-shapes)
    TopoDS_Iterator anIterator(theShape);
    std::cout << "Children using TopoDS_Iterator:" << std::endl;
    for (; anIterator.More(); anIterator.Next()) {
        const TopoDS_Shape& aChildShape = anIterator.Value();
        // You can then get the type of the child shape using aChildShape.ShapeType()
        std::cout << "  Child Shape Type: " << aChildShape.ShapeType() << std::endl;
    }
    std::cout << std::endl;

    // Using TopExp_Explorer (iterates all sub-shapes of a given type)
    std::cout << "Faces using TopExp_Explorer:" << std::endl;
    for (TopExp_Explorer anExplorer(theShape, TopAbs_FACE); anExplorer.More(); anExplorer.Next()) {
        const TopoDS_Face& aFace = TopoDS::Face(anExplorer.Current());
        // You can now work with the face
        std::cout << "  Found a Face" << std::endl;
    }
}

void OpenParEMg::on_actionBrep_triggered ()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open BREP File"), "", tr("BREP Files (*.brep)"));
    if (!filePath.isEmpty()) {

        // break up the full path
        QFileInfo fileInfo(filePath);
        absolutePath=fileInfo.absolutePath();
        QString base=fileInfo.baseName();

        //std::ifstream brepFile(filePath.toStdString().c_str(), std::ios_base::in | std::ios_base::binary);
        std::ifstream brepFile(filePath.toStdString().c_str(), std::ios_base::in);
        if (brepFile.is_open()) {
            TopoDS_Shape s;
            BRep_Builder b;
            BRepTools::Read(s,brepFile,b);
            brepFile.close();
            //TopTools::Dump(s, std::cout);
            //ListChildren(s);

            ui->drawingWindow->clearDrawing();
            Handle(AIS_Shape) drawingShape=new AIS_Shape (s);
            ui->drawingWindow->displayDrawing(drawingShape);

            QTreeWidgetItem *item=new QTreeWidgetItem(0);
            item->setText(0,base);
            drawing.addChild(item);
            ui->drawingItemTree->show();
        }
    }

    ui->drawingWindow->updateView();
}

void OpenParEMg::on_actionExit_triggered()
{
    exit(0);
}

