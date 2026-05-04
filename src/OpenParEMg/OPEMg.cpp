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
#include "Process.h"
#include "ui_OPEMg.h"

#include <Geom_Plane.hxx>
#include <csignal>
#include <quadmath.h>
#include <iostream>
#include <filesystem>

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <BRepBuilderAPI_Copy.hxx>

#include <QIcon>
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
//#include <thread>

#include "MeshOptions.h"
#include "SimulateOptions.h"
#include "LengthInputForm.h"
#include "RotateInputForm.h"
#include "about.h"
#include "license.h"
#include "FrequencyPlanG.h"
#include "Refinement.h"
#include "Materials.h"
#include "CustomOpenGLWidget.h"
#include "CustomLineEdit.h"
#include "SelectMaterialsDatabase.h"
#include "MaterialsOptions.h"
#include "CustomTreeWidgetItem.h"
#include "MaterialSelection.h"
#include "mpi.h"
//#include "RectangleSelector.h"
//#include "Polywire.h"


#include <AIS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <TopoDS_Shape.hxx>

std::string trim(const std::string& str);
bool extractText(const std::string& input, std::string& keyword, std::string& value);

bool cstrFromQString (char **aCstr, QString& aQString)
{
    if (*aCstr) free(*aCstr);
    *aCstr=(char *)malloc((aQString.length()+1)*sizeof(char));
    int i=0;
    while (i < aQString.length()) {
        (*aCstr)[i]=aQString.data()[i].toLatin1();  // ToDo: Generalize this for better character support?
        i++;
    }
    (*aCstr)[i]='\0';
    return false;
}

// trim and readFileToVector courtesy of ChatGPT
// Prompt: In c++, I would like a subroutine that reads a text file and puts each line of text into a std::vector<std::string>.
//         All text at and after the comment characters "//" are omitted. Leading and trailing white space are removed.
//         Empty lines are ignored. Use while loops instead of for loops.

// Helper function to trim leading and trailing whitespace
// std::string trim(const std::string& str) {
//     size_t start = 0;
//     while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
//         start++;
//     }

//     if (start == str.size()) return "";

//     size_t end = str.size() - 1;
//     while (end > start && std::isspace(static_cast<unsigned char>(str[end]))) {
//         end--;
//     }

//     return str.substr(start, end - start + 1);
// }

std::vector<std::string> readFileToVector(const std::string& filename) {
    std::vector<std::string> result;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file\n";
        return result;
    }

    std::string line;

    while (std::getline(file, line)) {
        // Remove comments starting with //
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Trim whitespace
        line = trim(line);

        // Ignore empty lines
        if (!line.empty()) {
            result.push_back(line);
        }
    }

    return result;
}

// courtesy of ChatGPT
std::vector<std::string> splitWhitespace(const std::string& input) {
    std::vector<std::string> result;

    size_t i = 0;
    size_t n = input.size();

    while (i < n) {
        // Skip leading whitespace
        while (i < n && std::isspace(static_cast<unsigned char>(input[i]))) {
            i++;
        }

        if (i >= n) break;

        // Start of a token
        size_t start = i;

        // Consume non-whitespace
        while (i < n && !std::isspace(static_cast<unsigned char>(input[i]))) {
            i++;
        }

        // Extract token
        result.push_back(input.substr(start, i - start));
    }

    return result;
}

OpenParEMg::OpenParEMg (QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OpenParEMg)
{
    ui->setupUi(this);

    MPI_PORT_COMM=nullptr;
    request=nullptr;

    // load an icon for the window
    setWindowIcon(QIcon(":/images/images/logo2.svg"));

    /////////////////////////////////////////////////////////////////////////////
    // main window setup
    /////////////////////////////////////////////////////////////////////////////

    absolutePath=QDir::currentPath();
    materialDatabase=new MaterialDatabase();
    boundaryDatabase=new BoundaryDatabase();
    resetLockouts();

    projectFile="";
    init_project (&defaultData);
    init_project (&projData);

    ui->actionShape->setCheckable(true);
    ui->actionShape->setChecked(true);

    on_actionShape_triggered();

    ui->actionWireframe->setChecked(true);

    /////////////////////////////////////////////////////////////////////////////
    // relay for receiving signals from controls
    /////////////////////////////////////////////////////////////////////////////

    relay=new Relay();
    connect(relay,&Relay::finishOperation,this,&OpenParEMg::finishOperation);
    connect(relay,&Relay::getCurrentMousePosition,this,&OpenParEMg::getCurrentMousePosition);
    connect(relay,&Relay::getPickedVertex,this,&OpenParEMg::getPickedVertex);
    connect(relay,&Relay::setMenus,this,&OpenParEMg::setMenus);
    connect(relay,&Relay::clearTreeSelection,this,&OpenParEMg::clearTreeSelection);
    connect(relay,&Relay::startPlaneSetToFace,this,&OpenParEMg::startPlaneSetToFace);

    /////////////////////////////////////////////////////////////////////////////
    // drawing window
    /////////////////////////////////////////////////////////////////////////////

    ui->drawingWindow->set_drawingItemTree(&drawing);
    ui->drawingWindow->set_portItemTree(&port);
    ui->drawingWindow->set_boundaryItemTree(&boundary);
    ui->drawingWindow->set_meshItemTree(&mesh);
    ui->drawingWindow->set_pathItemTree(&path);
    ui->drawingWindow->set_relay(relay);

    QActionList.push_back(showAction);
    QActionList.push_back(hideAction);
    QActionList.push_back(selectAllAction);
    QActionList.push_back(unselectAction);
    QActionList.push_back(copyAction);
    QActionList.push_back(deleteAction);
    QActionList.push_back(deletePointAction);
    QActionList.push_back(insertPointAction);
    QActionList.push_back(closePolylineAction);
    QActionList.push_back(openPolylineAction);
    QActionList.push_back(convertToPolylineAction);
    QActionList.push_back(convertToPathAction);
    QActionList.push_back(removeAction);
    QActionList.push_back(assignAction);
    QActionList.push_back(insertAction);
    QActionList.push_back(renameAction);
    QActionList.push_back(expandAllAction);
    QActionList.push_back(collapseAllAction);
    QActionList.push_back(setPlaneAction);
    QActionList.push_back(setPlaneAxisAction);
    QActionList.push_back(createPortAction);
    QActionList.push_back(createPathAction);
    QActionList.push_back(drawPathAction);
    QActionList.push_back(drawPolylineAction);
    QActionList.push_back(editAction);
    QActionList.push_back(moveAction);
    QActionList.push_back(stretchAction);
    QActionList.push_back(rotateAction);
    QActionList.push_back(doneAction);
    QActionList.push_back(cancelAction);
    QActionList.push_back(deleteLastPointAction);
    QActionList.push_back(extrudeAction);
    QActionList.push_back(mergeAction);
    QActionList.push_back(subtractAction);
    QActionList.push_back(assignMaterialAction);
    initQActionList();


    /////////////////////////////////////////////////////////////////////////////
    // item selection tree
    /////////////////////////////////////////////////////////////////////////////

    drawing.set_itemType(100);
    port.set_itemType(101);
    boundary.set_itemType(102);
    mesh.set_itemType(103);
    path.set_itemType(104);

    ui->drawingItemTree->setHeaderHidden(true);
    ui->drawingItemTree->setColumnCount(2);
    ui->drawingItemTree->header()->setStretchLastSection(false);
    ui->drawingItemTree->header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    ui->drawingItemTree->header()->setSectionResizeMode(1,QHeaderView::ResizeToContents);

    // five base list items: drawing, path, port, boundary, and mesh

    drawing.setText(0,"Drawing");
    ui->drawingItemTree->addTopLevelItem(&drawing);

    path.setText(0,"Path");
    ui->drawingItemTree->addTopLevelItem(&path);
    path.setForeground(0,Qt::black);

    port.setText(0,"Port");
    ui->drawingItemTree->addTopLevelItem(&port);
    port.setForeground(0,Qt::black);

    boundary.setText(0,"Boundary");
    ui->drawingItemTree->addTopLevelItem(&boundary);
    boundary.setForeground(0,Qt::black);

    mesh.setText(0,"Mesh");
    ui->drawingItemTree->addTopLevelItem(&mesh);
    mesh.setForeground(0,Qt::black);

    // context menu
    ui->drawingItemTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->drawingItemTree,&QTreeView::customContextMenuRequested,this,&OpenParEMg::itemTreeContextMenu_triggered);

    ui->drawingItemTree->viewport()->installEventFilter(this);

    // enable multi-selection with CTRL and SHIFT modifiers
    ui->drawingItemTree->setSelectionMode(QAbstractItemView::MultiSelection);
    CTRLpressed=false;
    SHIFTpressed=false;
    clickedItem=nullptr;
    previousClickedItem=nullptr;
    workingItem=nullptr;

    ui->menuRun->setToolTipsVisible(true);

    drawing.setForeground(0,Qt::black);
    path.setForeground(0,Qt::black);
    port.setForeground(0,Qt::black);
    boundary.setForeground(0,Qt::black);
    mesh.setForeground(0,Qt::black);

    // drawing is always a COMPOUND
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    Handle(AIS_Shape) newShape=new AIS_Shape(compound);
    ShapeData *newShapeData=new ShapeData(1,nullptr,nullptr,newShape);
    drawing.addShapeData(newShapeData);

    /////////////////////////////////////////////////////////////////////////////
    // context menu for drawingWindow
    /////////////////////////////////////////////////////////////////////////////

    ui->drawingWindow->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->drawingWindow,&QOpenGLWidget::customContextMenuRequested,this,&OpenParEMg::drawingWindowContextMenu_triggered);

    /////////////////////////////////////////////////////////////////////////////
    // gmsh
    /////////////////////////////////////////////////////////////////////////////

    gmsh::initialize();
    gmsh::option::setNumber("Mesh.MshFileVersion",2.2);

    /////////////////////////////////////////////////////////////////////////////
    // timer or checking when OpenParEM3D finishes
    /////////////////////////////////////////////////////////////////////////////

    timer=new QTimer(this);
    connect(timer,&QTimer::timeout,this,&OpenParEMg::checkFinish);

    /////////////////////////////////////////////////////////////////////////////
    // misc
    /////////////////////////////////////////////////////////////////////////////

    renameItem=nullptr;

    /////////////////////////////////////////////////////////////////////////////
    // drawing
    /////////////////////////////////////////////////////////////////////////////

    isIntegrationPath=false;
    activePolywire=nullptr;

    lengthInputForm=nullptr;
    vectorInputForm=nullptr;
    lengthEditForm=nullptr;
    lineEditForm=nullptr;
    rectangleEditForm=nullptr;
    polycircleEditForm=nullptr;
    rotateInputForm=nullptr;

    uLocalAxis.SetCoord(1,0,0);  // rectangles
    length=0;                    // extrusion
    angle=90;                    // rotation
    startPoint.SetCoord(0,0,0);  // rotating and vector input
    endPoint.SetCoord(0,0,1);    // rotation and vector input

    restrictToDrawingPlane=false;
    activeAction=false;

    /////////////////////////////////////////////////////////////////////////////

    ui->drawingItemTree->show();
    ui->drawingWindow->show();
    setMenus();

    PetscInitializeNoArguments();
}

OpenParEMg::~OpenParEMg ()
{
    freeQActionList();

    if (timer) delete timer;
    if (MPI_PORT_COMM) MPI_Comm_free(MPI_PORT_COMM);
    if (request) MPI_Request_free(request);
    gmsh::finalize();
    PetscFinalize();
    delete ui;
}

void OpenParEMg::initQActionList ()
{
    long unsigned int i=0;
    while (i < QActionList.size()) {
        QActionList[i]=nullptr;
        i++;
    }
}

void OpenParEMg::freeQActionList ()
{
    long unsigned int i=0;
    while (i < QActionList.size()) {
        if (QActionList[i]) {
            delete QActionList[i];
            QActionList[i]=nullptr;
        }
        i++;
    }
}

void OpenParEMg::clearSelection ()
{
    ui->actionShape->setChecked(false);
    ui->actionShape->setCheckable(false);

    ui->actionVertex->setChecked(false);
    ui->actionVertex->setCheckable(false);

    ui->actionEdge->setChecked(false);
    ui->actionEdge->setCheckable(false);

    ui->actionWire->setChecked(false);
    ui->actionWire->setCheckable(false);

    ui->actionFace->setChecked(false);
    ui->actionFace->setCheckable(false);

    ui->actionShell->setChecked(false);
    ui->actionShell->setCheckable(false);

    ui->actionSolid->setChecked(false);
    ui->actionSolid->setCheckable(false);
}

void OpenParEMg::restoreSelection ()
{
    //std::cout << "OpenParEMg::restoreSelection  previousSelectionIndex=" << previousSelectionIndex << std::endl; std::cout.flush();

    if (previousSelectionIndex == 0) on_actionShape_triggered();
    else if (previousSelectionIndex == 1) on_actionVertex_triggered();
    else if (previousSelectionIndex == 2) on_actionEdge_triggered();
    else if (previousSelectionIndex == 3) on_actionWire_triggered();
    else if (previousSelectionIndex == 4) on_actionFace_triggered();
    else if (previousSelectionIndex == 5) on_actionShell_triggered();
    else if (previousSelectionIndex == 6) on_actionSolid_triggered();
}

void OpenParEMg::debugPrintStats (int i)
{
    std::cout << "Debug place " << i << std::endl; std::cout.flush();
    ui->drawingWindow->printTrackerStats();
    ui->drawingWindow->printDrawingSelectedCount();
}

void OpenParEMg::setMenus ()
{
    setMenusI(-1);
}

void OpenParEMg::setMenusI (int placeIndex)
{
    //std::cout << "OpenParEMg::setMenusI  place=" << placeIndex << std::endl; std::cout.flush();

    bool boundaryDatabaseChanged=boundaryDatabase->is_modified();
    ui->drawingWindow->compactSelectedItems();
    ui->drawingWindow->compactVisibleItems();

    // debug options
    //itemChangesStack.print();
    //printLockouts();
    //debugPrintStats(0);
    //ui->drawingWindow->PrintAllActiveModes();

    // disable all menus on command
    if (disableMenus) {
        ui->menubar->setEnabled(false);
        return;
    }

    ui->menubar->setEnabled(true);

    if (projectFileLoaded) {
        ui->actionNew->setEnabled(false);
        ui->actionOpen->setEnabled(false);
        ui->actionSave->setEnabled(false);
        if (projectChanged || boundaryDatabaseChanged || drawingChanged || meshChanged) {
            if (strcmp(projData.project_name,"") != 0) ui->actionSave->setEnabled(true);
        }
        ui->actionSaveAs->setEnabled(true);
        ui->actionClose->setEnabled(true);
        ui->actionExit->setEnabled(true);
        ui->actionSelectMaterialsDatabase->setEnabled(true);
        ui->actionMaterialsOptions->setEnabled(true);
        ui->actionUnselectAll->setEnabled(true);
        ui->actionShowAll->setEnabled(true);
        ui->actionHideAll->setEnabled(true);
        ui->actionSelectEdge->setEnabled(false);
        ui->actionSelectWire->setEnabled(false);
        ui->actionSelectFace->setEnabled(false);
        ui->actionSelectWithBox->setEnabled(false);
        ui->actionDrawLine->setEnabled(true);
        ui->actionDrawPolyline->setEnabled(true);
        ui->actionDrawPolycircle->setEnabled(true);
        ui->actionDrawRectangle->setEnabled(true);
        ui->actionMeshOptions->setEnabled(true);
        ui->actionMeshLoad->setEnabled(true);
        ui->actionMeshSave->setEnabled(false);
        ui->actionMeshSaveAs->setEnabled(false);
        ui->actionMeshDelete->setEnabled(false);
        ui->actionFrequencyPlan->setEnabled(true);
        //ui->actionRefinement->setEnabled(true);
        if (strcmp(projData.refinement_frequency,"none") == 0) ui->actionRefinement->setEnabled(false);
        else ui->actionRefinement->setEnabled(true);
        ui->actionSimulateOptions->setEnabled(true);
        ui->actionRun->setEnabled(false);
        ui->actionRun->setToolTip("Project does not have a mesh.");
        ui->actionStop->setEnabled(false);
        ui->actionAbort->setEnabled(false);
        ui->actionAbortAndExit->setEnabled(false);
        ui->actionMaterialsEditor->setEnabled(true);
        ui->actionAbout->setEnabled(true);
        ui->actionLicense->setEnabled(true);

        // if (brepFileLoaded) {
        //     ui->actionImportBrep->setEnabled(false);
        //     ui->actionImportStep->setEnabled(false);
        //     ui->actionExportStep->setEnabled(true);

        //     ui->actionFitSelected->setEnabled(true);
        //     ui->actionFitAll->setEnabled(true);
        //     ui->actionMenuSelection->setEnabled(true);
        //     ui->actionSelectEdge->setEnabled(true);
        //     ui->actionSelectWire->setEnabled(true);
        //     ui->actionSelectFace->setEnabled(true);
        //     ui->actionSelectWithBox->setEnabled(true);
        //     ui->actionWireframe->setEnabled(true);

        //     ui->actionMeshGenerate->setEnabled(true);
        // } else {
        //     ui->actionImportBrep->setEnabled(true);
        //     ui->actionImportStep->setEnabled(true);
        //     ui->actionExportStep->setEnabled(false);

        //     ui->actionFitSelected->setEnabled(false);
        //     ui->actionFitAll->setEnabled(false);
        //     ui->actionMenuSelection->setEnabled(false);
        //     ui->actionSelectEdge->setEnabled(false);
        //     ui->actionSelectWire->setEnabled(false);
        //     ui->actionSelectFace->setEnabled(false);
        //     ui->actionSelectWithBox->setEnabled(false);
        //     ui->actionUnselectAll->setEnabled(false);
        //     ui->actionHideAll->setEnabled(false);
        //     ui->actionWireframe->setEnabled(false);

        //     ui->actionMeshGenerate->setEnabled(false);
        // }

        ui->actionImportBrep->setEnabled(true);
        ui->actionImportBrep->setEnabled(true);
        ui->actionExportStep->setEnabled(false);
        ui->actionExportStep->setEnabled(false);
        ui->actionFitSelected->setEnabled(false);
        ui->actionFitAll->setEnabled(false);
        ui->actionMenuSelection->setEnabled(false);
        ui->actionSelectEdge->setEnabled(false);
        ui->actionSelectWire->setEnabled(false);
        ui->actionSelectFace->setEnabled(false);
        ui->actionSelectWithBox->setEnabled(false);
        ui->actionUnselectAll->setEnabled(false);
        ui->actionHideAll->setEnabled(false);
        ui->actionWireframe->setEnabled(false);
        ui->actionDrawingPlaneSetToFace->setEnabled(false);
        ui->actionMeshGenerate->setEnabled(false);
        if (drawing.childCount() > 0) {
            ui->actionExportStep->setEnabled(true);
            ui->actionExportStep->setEnabled(true);
            ui->actionFitSelected->setEnabled(true);
            ui->actionFitAll->setEnabled(true);
            ui->actionMenuSelection->setEnabled(true);
            ui->actionSelectEdge->setEnabled(true);
            ui->actionSelectWire->setEnabled(true);
            ui->actionSelectFace->setEnabled(true);
            ui->actionSelectWithBox->setEnabled(true);
            ui->actionUnselectAll->setEnabled(true);
            ui->actionHideAll->setEnabled(true);
            ui->actionWireframe->setEnabled(true);
            ui->actionDrawingPlaneSetToFace->setEnabled(true);
            ui->actionMeshGenerate->setEnabled(true);
        }

        if (mesh.childCount() > 0) {
            ui->actionFitAll->setEnabled(true);
            ui->actionMenuSelection->setEnabled(true);
            ui->actionWireframe->setEnabled(true);
            ui->actionMeshGenerate->setEnabled(false);
            ui->actionMeshLoad->setEnabled(false);
            ui->actionMeshSave->setEnabled(false);
            if (meshChanged) ui->actionMeshSave->setEnabled(true);
            ui->actionMeshSaveAs->setEnabled(true);
            ui->actionMeshDelete->setEnabled(true);

            // start run block
            ui->actionRun->setEnabled(true);
            ui->actionRun->setToolTip("Run OpenParEM3D.");
            if (simulationRunning) {
                ui->actionRun->setEnabled(false);
                ui->actionRun->setToolTip("OpenParEM3D is running.");
                ui->actionStop->setEnabled(true);
                ui->actionAbort->setEnabled(true);
                ui->actionAbortAndExit->setEnabled(true);
            }
            if (simulationStopping) {
                ui->actionRun->setEnabled(false);
                ui->actionRun->setToolTip("OpenParEM3D is stopping.");
                ui->actionStop->setEnabled(false);
                ui->actionAbort->setEnabled(true);
                ui->actionAbortAndExit->setEnabled(true);
            }
            if (simulationAborting) {
                ui->actionRun->setEnabled(false);
                ui->actionRun->setToolTip("OpenParEM3D is aborting.");
                ui->actionStop->setEnabled(false);
                ui->actionAbort->setEnabled(false);
                ui->actionAbortAndExit->setEnabled(true);
            }
            if (meshChanged) {
                ui->actionRun->setEnabled(false);
                ui->actionRun->setToolTip("Run OpenParEM3D.");
                ui->actionStop->setEnabled(false);
                ui->actionAbort->setEnabled(false);
                ui->actionAbortAndExit->setEnabled(false);
            }
            // end run block
        }

        if (projectChanged || meshChanged || boundaryDatabaseChanged) {
            ui->actionRun->setEnabled(false);
            ui->actionRun->setToolTip("OpenParEM3D is running.");
            ui->actionStop->setEnabled(false);
            ui->actionAbort->setEnabled(false);
            ui->actionAbortAndExit->setEnabled(false);
        }

        if (drawingPlaneShown) {
            ui->actionDrawingPlaneShow->setEnabled(false);
            ui->actionDrawingPlaneHide->setEnabled(true);
            ui->actionDrawingPlaneSnapToGrid->setEnabled(true);
        } else {
            ui->actionDrawingPlaneShow->setEnabled(true);
            ui->actionDrawingPlaneHide->setEnabled(false);
            ui->actionDrawingPlaneSnapToGrid->setEnabled(false);
        }

    } else {
        ui->actionNew->setEnabled(true);
        ui->actionOpen->setEnabled(true);
        ui->actionSave->setEnabled(false);
        ui->actionSaveAs->setEnabled(false);
        ui->actionClose->setEnabled(false);
        ui->actionExit->setEnabled(true);
        ui->actionImportBrep->setEnabled(false);
        ui->actionImportStep->setEnabled(false);
        ui->actionExportStep->setEnabled(false);

        ui->actionFitSelected->setEnabled(false);
        ui->actionFitAll->setEnabled(false);
        ui->actionMenuSelection->setEnabled(false);
        ui->actionSelectEdge->setEnabled(false);
        ui->actionSelectWire->setEnabled(false);
        ui->actionSelectFace->setEnabled(false);
        ui->actionSelectWithBox->setEnabled(false);
        ui->actionUnselectAll->setEnabled(false);
        ui->actionShowAll->setEnabled(false);
        ui->actionHideAll->setEnabled(false);
        ui->actionWireframe->setEnabled(false);

        ui->actionDrawingPlaneShow->setEnabled(false);
        ui->actionDrawingPlaneHide->setEnabled(false);
        ui->actionDrawingPlaneSnapToGrid->setEnabled(false);
        ui->actionDrawingPlaneSetToFace->setEnabled(false);
        ui->actionDrawLine->setEnabled(false);
        ui->actionDrawPolyline->setEnabled(false);
        ui->actionDrawPolycircle->setEnabled(false);
        ui->actionDrawRectangle->setEnabled(false);

        ui->actionSelectMaterialsDatabase->setEnabled(false);
        ui->actionMaterialsOptions->setEnabled(false);

        ui->actionMeshOptions->setEnabled(false);
        ui->actionMeshGenerate->setEnabled(false);
        ui->actionMeshLoad->setEnabled(false);
        ui->actionMeshSave->setEnabled(false);
        ui->actionMeshSaveAs->setEnabled(false);
        ui->actionMeshDelete->setEnabled(false);

        ui->actionFrequencyPlan->setEnabled(false);
        ui->actionRefinement->setEnabled(false);
        ui->actionSimulateOptions->setEnabled(false);

        ui->actionRun->setEnabled(false);
        ui->actionRun->setToolTip("Project not loaded.");
        ui->actionStop->setEnabled(false);
        ui->actionAbort->setEnabled(false);
        ui->actionAbortAndExit->setEnabled(false);

        ui->actionMaterialsEditor->setEnabled(true);
        ui->actionAbout->setEnabled(true);
        ui->actionLicense->setEnabled(true);
    }

    if (simulationRunning) {
        ui->actionNew->setEnabled(false);
        ui->actionOpen->setEnabled(false);
        ui->actionSave->setEnabled(false);
        ui->actionSaveAs->setEnabled(false);
        ui->actionClose->setEnabled(false);
        ui->actionExit->setEnabled(false);
        ui->actionImportBrep->setEnabled(false);
        ui->actionImportStep->setEnabled(false);
        ui->actionExportStep->setEnabled(false);
        ui->actionDrawLine->setEnabled(false);
        ui->actionDrawPolyline->setEnabled(false);
        ui->actionDrawPolycircle->setEnabled(false);
        ui->actionDrawRectangle->setEnabled(false);
        ui->actionDrawingPlaneSnapToGrid->setEnabled(false);
        ui->actionDrawingPlaneSetToFace->setEnabled(false);
        ui->actionSelectMaterialsDatabase->setEnabled(true);
        ui->actionMaterialsOptions->setEnabled(true);
        ui->actionMeshOptions->setEnabled(true);
        ui->actionMeshLoad->setEnabled(false);
        ui->actionMeshSave->setEnabled(false);
        ui->actionMeshSaveAs->setEnabled(false);
        ui->actionMeshDelete->setEnabled(false);
        ui->actionSimulateOptions->setEnabled(true);
        ui->actionFrequencyPlan->setEnabled(true);
    }

    ui->actionUndo->setEnabled(itemChangesStack.hasUndo());
    ui->actionRedo->setEnabled(itemChangesStack.hasRedo());

    boundaryDatabase->set_comboZdef();
}

void OpenParEMg::expand (CustomTreeWidgetItem *item)
{
    if (!item) return;
    item->setExpanded(Standard_True);
    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
        expand(child);
        i++;
    }
}

void OpenParEMg::collapse (CustomTreeWidgetItem *item)
{
    if (!item) return;
    item->setExpanded(Standard_False);
    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
        collapse(child);
        i++;
    }
}

void OpenParEMg::expandAllItems ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        expand(item);
        i++;
    }
}

void OpenParEMg::collapseAllItems ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        collapse(item);
        i++;
    }
}

void OpenParEMg::buildDrawingMenu (QMenu &menu)
{
    assignMaterialAction=new QAction("Assign Material");
    showAction=new QAction("Show");
    hideAction=new QAction("Hide");
    editAction=new QAction("Edit");
    moveAction=new QAction("Move");
    stretchAction=new QAction("Stretch");
    deletePointAction=new QAction("Delete Point");
    insertPointAction=new QAction("Insert Point");
    closePolylineAction=new QAction("Close Polyline");
    openPolylineAction=new QAction("Open Polyline");
    convertToPolylineAction=new QAction("Convert to Polyline");
    convertToPathAction=new QAction("Convert to Path");
    rotateAction=new QAction("Rotate");
    unselectAction=new QAction("Unselect");
    copyAction=new QAction("Copy");
    renameAction=new QAction("Rename",this);
    deleteAction=new QAction("Delete");
    setPlaneAction=new QAction("Set Drawing Plane");
    setPlaneAxisAction=new QAction("Set Drawing Plane with Axis");
    createPortAction=new QAction("Create Port");
    createPortAction->setToolTip("Copy the selected face and create a port.");
    createPathAction=new QAction("Create Path");
    createPathAction->setToolTip("Copy the selected face and create a path.");
    extrudeAction=new QAction("Extrude");
    extrudeAction->setToolTip("Extrude the selected polywires along each normal.");
    mergeAction=new QAction("Merge");
    mergeAction->setToolTip("Merge two solid objects.");
    subtractAction=new QAction("Subtract");
    subtractAction->setToolTip("Subtract the second selected solid object from the first selected solid object.");
    cancelAction=new QAction("Cancel");

    connect(assignMaterialAction, &QAction::triggered, this, &OpenParEMg::assignMaterial);
    connect(showAction, &QAction::triggered, this, &OpenParEMg::showDrawingItems);
    connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideDrawingItems);
    connect(editAction, &QAction::triggered, this, &OpenParEMg::editObject);
    connect(moveAction, &QAction::triggered, this, &OpenParEMg::moveObject);
    connect(stretchAction, &QAction::triggered, this, &OpenParEMg::stretchObject);
    connect(deletePointAction, &QAction::triggered, this, &OpenParEMg::deletePoint);
    connect(insertPointAction, &QAction::triggered, this, &OpenParEMg::insertPoint);
    connect(closePolylineAction, &QAction::triggered, this, &OpenParEMg::closeExistingPolyline);
    connect(openPolylineAction, &QAction::triggered, this, &OpenParEMg::openExistingPolyline);
    connect(convertToPolylineAction, &QAction::triggered, this, &OpenParEMg::convertToPolyline);
    connect(convertToPathAction, &QAction::triggered, this, &OpenParEMg::convertToPath);
    connect(rotateAction, &QAction::triggered, this, &OpenParEMg::rotateObject);
    connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectDrawingItems);
    connect(renameAction, &QAction::triggered, this, &OpenParEMg::renameDrawingItems);
    connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deleteDrawingItems);
    connect(copyAction, &QAction::triggered, this, &OpenParEMg::copyDrawingItems);
    connect(setPlaneAction, &QAction::triggered, this, &OpenParEMg::setPlaneToFace);
    connect(setPlaneAxisAction, &QAction::triggered, this, &OpenParEMg::setPlaneToFaceAxis);
    connect(createPortAction, &QAction::triggered, this, &OpenParEMg::createPort);
    connect(createPathAction, &QAction::triggered, this, &OpenParEMg::createPath);
    connect(extrudeAction, &QAction::triggered, this, &OpenParEMg::extrudePolywire);
    connect(mergeAction, &QAction::triggered, this, &OpenParEMg::mergeSolids);
    connect(subtractAction, &QAction::triggered, this, &OpenParEMg::subtractSolids);
    connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelDrawingMenu);

    if (isValidAssignMaterial()) menu.addAction(assignMaterialAction);
    if (isValidObjectShow()) menu.addAction(showAction);
    if (isValidObjectHide()) menu.addAction(hideAction);
    //menu.addAction(unselectAction);
    if (isValidCopy()) menu.addAction(copyAction);
    if (isValidRenameDrawingItems()) menu.addAction(renameAction);
    if (isValidObjectDelete()) menu.addAction(deleteAction);
    if (isValidSetPlane()) menu.addAction(setPlaneAction);
    if (isValidSetPlane()) menu.addAction(setPlaneAxisAction);
    if (isValidCreatePort()) menu.addAction(createPortAction);
    if (isValidCreatePath()) menu.addAction(createPathAction);
    if (isValidObjectEdit()) menu.addAction(editAction);
    if (isValidObjectMove()) menu.addAction(moveAction);
    if (isValidObjectStretch()) menu.addAction(stretchAction);
    if (isValidInsertPoint()) menu.addAction(insertPointAction);
    if (isValidDeletePoint()) menu.addAction(deletePointAction);
    if (isValidCloseExistingPolyline()) menu.addAction(closePolylineAction);
    if (isValidOpenExistingPolyline()) menu.addAction(openPolylineAction);
    if (isValidConvertToPolyline()) menu.addAction(convertToPolylineAction);
    if (isValidConvertToPath()) menu.addAction(convertToPathAction);
    if (isValidRotateObject()) menu.addAction(rotateAction);
    if (isValidExtrudePolywire()) menu.addAction(extrudeAction);
    if (isValidMergeSolids()) menu.addAction(mergeAction);
    if (isValidSubtractSolids()) menu.addAction(subtractAction);
    menu.addAction(cancelAction);
}

void OpenParEMg::buildPathMenu (QMenu &menu)
{
    renameAction=new QAction("Rename",this);
    deleteAction=new QAction("Delete",this);
    showAction=new QAction("Show",this);
    hideAction=new QAction("Hide",this);
    cancelAction=new QAction("Cancel");

    connect(renameAction, &QAction::triggered, this, &OpenParEMg::renamePathItems);
    connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deletePathItems);
    connect(showAction, &QAction::triggered, this, &OpenParEMg::showPathItems);
    connect(hideAction, &QAction::triggered, this, &OpenParEMg::hidePathItems);
    connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelPathMenu);

    if (isValidShowPath()) menu.addAction(showAction);
    if (isValidHidePath()) menu.addAction(hideAction);
    if (ui->drawingWindow->get_pathSelectedCount() == 1) menu.addAction(renameAction);
    if (isPathValidDelete()) menu.addAction(deleteAction);
    menu.addAction(cancelAction);
}

void OpenParEMg::cancelDrawingMenu ()
{
    on_actionShape_triggered();
    ui->drawingWindow->setSubshapeSelection(false);
    ui->drawingWindow->setSetToPlane(false);
}

void OpenParEMg::cancelPathMenu ()
{
    // nothing to do
}

void OpenParEMg::itemTreeContextMenu_triggered (const QPoint& pnt)
{
    //std::cout << "OpenParEMg::itemTreeContextMenu_triggered" << std::endl; std::cout.flush();

    clickedItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemAt(pnt);
    if (!clickedItem) return;
    if (!clickedItem->isSelected()) return;

    QMenu menu(this);

    if (clickedItem->is_rootDrawing()) {
        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        selectAllAction=new QAction("Select All");

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showRootDrawingItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideRootDrawingItems);
        connect(selectAllAction, &QAction::triggered, this, &OpenParEMg::selectAllRootDrawingItems);

        if (isRootDrawingValidShow()) menu.addAction(showAction);
        if (isRootDrawingValidHide()) menu.addAction(hideAction);
        if (isRootDrawingValidSelectAll()) menu.addAction(selectAllAction);
    }

    if (clickedItem->is_drawing()) {
        if (ui->drawingWindow->get_NbSelected()) {
            buildDrawingMenu(menu);
        }
    }

    if (clickedItem->is_rootPath()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showRootPathItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideRootPathItems);
        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        if (rootPathValidShow()) menu.addAction(showAction);
        if (rootPathValidHide()) menu.addAction(hideAction);
        if (!clickedItem->isExpanded()) menu.addAction(expandAllAction);
        if (clickedItem->isExpanded()) menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_path()) {
        buildPathMenu(menu);
    }

    if (clickedItem->is_rootPort()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showRootPortItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideRootPortItems);
        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        if (isValidObjectShow()) menu.addAction(showAction);
        if (isValidObjectHide()) menu.addAction(hideAction);
        if (!clickedItem->isExpanded()) menu.addAction(expandAllAction);
        if (clickedItem->isExpanded()) menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_port()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        unselectAction=new QAction("Unselect",this);
        renameAction=new QAction("Rename",this);
        insertAction=new QAction("Insert Mode",this);
        deleteAction=new QAction("Delete",this);
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showPortItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hidePortItems);
        connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectPortItems);
        connect(insertAction, &QAction::triggered, this, &OpenParEMg::insertModeItems);
        connect(renameAction, &QAction::triggered, this, &OpenParEMg::renamePortItems);
        connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deletePortItems);
        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        if (isValidShowPath()) menu.addAction(showAction);
        if (isValidHidePath()) menu.addAction(hideAction);
        if (ui->drawingWindow->hasPortSelectedItems()) menu.addAction(unselectAction);
        if (ui->drawingWindow->get_portSelectedCount() == 1) menu.addAction(renameAction);
        menu.addAction(insertAction);
        menu.addAction(deleteAction);
        if (!clickedItem->isExpanded()) menu.addAction(expandAllAction);
        if (clickedItem->isExpanded()) menu.addAction(collapseAllAction);
    }

    // ToDo: boundary

    if (clickedItem->is_rootMesh()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showRootMeshItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideRootMeshItems);
        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        if (rootMeshValidShow()) menu.addAction(showAction);
        if (rootMeshValidHide()) menu.addAction(hideAction);
        if (!clickedItem->isExpanded()) menu.addAction(expandAllAction);
        if (clickedItem->isExpanded()) menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_mesh()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showMeshItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideMeshItems);

        if (isValidObjectShow()) menu.addAction(showAction);
        if (isValidObjectHide()) menu.addAction(hideAction);
    }

    if (clickedItem->is_sportLabel()) {
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        if (!clickedItem->isExpanded()) menu.addAction(expandAllAction);
        if (clickedItem->isExpanded()) menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_sport()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        renameAction=new QAction("Rename",this);
        deleteAction=new QAction("Delete",this);
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showNetItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideNetItems);
        connect(renameAction, &QAction::triggered, this, &OpenParEMg::renameSportNet);
        connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deleteSportItems);
        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        if (ui->drawingWindow->isNetValidShow()) menu.addAction(showAction);
        if (ui->drawingWindow->isNetValidHide()) menu.addAction(hideAction);
        if (ui->drawingWindow->get_selectedItems_count() == 1) menu.addAction(renameAction);
        if (deleteSportValid()) menu.addAction(deleteAction);
        if (!clickedItem->isExpanded()) menu.addAction(expandAllAction);
        if (clickedItem->isExpanded()) menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_voltage() || clickedItem->is_current()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        drawPathAction=new QAction("Draw Line Path");
        drawPolylineAction=new QAction("Draw Polyline Path");
        insertAction=new QAction("Add Path");
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showVIItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideVIItems);
        connect(drawPathAction, &QAction::triggered, this, &OpenParEMg::drawLinePath);
        connect(drawPolylineAction, &QAction::triggered, this, &OpenParEMg::drawPolylinePath);
        connect(insertAction, &QAction::triggered, this, &OpenParEMg::insertSelectedPath);
        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        if (isValidObjectShow()) menu.addAction(showAction);
        if (isValidObjectHide()) menu.addAction(hideAction);
        if (ui->drawingWindow->get_selectedItems_count() == 1 && clickedItem->foreground(0) == Qt::black) menu.addAction(drawPathAction);
        if (ui->drawingWindow->get_selectedItems_count() == 1 && clickedItem->foreground(0) == Qt::black) menu.addAction(drawPolylineAction);
        if (insertActionValid()) menu.addAction(insertAction);
        if (!clickedItem->isExpanded()) menu.addAction(expandAllAction);
        if (clickedItem->isExpanded()) menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_integrationPathSegment()) {

        removeAction=new QAction("Remove",this);
        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);

        connect(removeAction, &QAction::triggered, this, &OpenParEMg::removeIntegrationPathItems);
        connect(showAction, &QAction::triggered, this, &OpenParEMg::showIntegrationPathItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideIntegrationPathItems);

        if (isValidShowPath()) menu.addAction(showAction);
        if (isValidHidePath()) menu.addAction(hideAction);
        menu.addAction(removeAction);
    }

    if (clickedItem->is_scale()) {
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        if (!clickedItem->isExpanded()) menu.addAction(expandAllAction);
        if (clickedItem->isExpanded()) menu.addAction(collapseAllAction);
    }

    menu.exec(ui->drawingItemTree->mapToGlobal(pnt));

    freeQActionList();
}

void OpenParEMg::drawingWindowContextMenu_triggered(const QPoint& pnt)
{
    //std::cout << "OpenParEMg::drawingWindowContextMenu_triggered" << std::endl; std::cout.flush();

    QMenu menu(this);

    if (activePolywire) {

        if (dynamic_cast<Line *>(activePolywire)) {
            cancelAction=new QAction("Cancel");
            connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelDraw);
            menu.addAction(cancelAction);
        } else if (dynamic_cast<Polyline *>(activePolywire)) {
            deleteLastPointAction=new QAction("Delete Point");
            doneAction=new QAction("Finished");
            closeAction=new QAction("Close Polyline");
            cancelAction=new QAction("Cancel");

            connect(deleteLastPointAction, &QAction::triggered, this, &OpenParEMg::deleteLastPoint);
            connect(doneAction, &QAction::triggered, this, &OpenParEMg::finishPolyline);
            connect(closeAction, &QAction::triggered, this, &OpenParEMg::closePolyline);
            connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelDraw);

            deleteLastPointAction->setEnabled(false);
            if (activePolywire->canDeleteLastPoint()) deleteLastPointAction->setEnabled(true);

            doneAction->setEnabled(false);
            if (activePolywire->canFinish()) doneAction->setEnabled(true);

            closeAction->setEnabled(false);
            if (activePolywire->canClose()) closeAction->setEnabled(true);

            menu.addAction(deleteLastPointAction);
            menu.addAction(doneAction);
            menu.addAction(closeAction);
            menu.addAction(cancelAction);
        } else if (dynamic_cast<Rectangle *>(activePolywire)){
            cancelAction=new QAction("Cancel");
            connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelDraw);
            menu.addAction(cancelAction);
        } else if (dynamic_cast<Polycircle *>(activePolywire)) {
            cancelAction=new QAction("Cancel");
            connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelDraw);
            menu.addAction(cancelAction);
        }
    } else {
        // check for selected subshape
        if (ui->drawingWindow->get_selectedItems_size() == 0 && ui->drawingWindow->get_NbSelected()) {
            buildDrawingMenu(menu);
        }

        // check for selected items
        long unsigned int i=0;
        while (i < ui->drawingWindow->get_selectedItems_size()) {
            CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
            if (item) {
                if (item->is_drawing()) {
                    if (item->getEnableDeletePoint()) {
                        cancelAction=new QAction("Cancel");
                        connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelDeletePoint);
                        menu.addAction(cancelAction);
                    } else {
                        buildDrawingMenu(menu);
                    }
                    break;
                }
                if (item->is_path() || item->is_port() || item->is_integrationPathSegment()) {
                    buildPathMenu(menu);
                    break;
                }
            }
            i++;
        }
    }

    menu.exec(ui->drawingWindow->mapToGlobal(pnt));
    freeQActionList();
}

bool OpenParEMg::isRootDrawingValidShow ()
{
    int i=0;
    while (i < drawing.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) drawing.child(i);
        if (child->isValidShow()) return true;
        i++;
    }
    return false;
}

bool OpenParEMg::isRootDrawingValidHide ()
{
    int i=0;
    while (i < drawing.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) drawing.child(i);
        if (child->isValidHide()) return true;
        i++;
    }
    return false;
}

bool OpenParEMg::isRootDrawingValidSelectAll ()
{
    int i=0;
    while (i < drawing.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) drawing.child(i);
        if (!child->isSelected()) return true;
        i++;
    }
    return true;
}

void OpenParEMg::selectAllRootDrawingItems ()
{
    ui->drawingWindow->hideItem(&drawing);
    ui->drawingWindow->unselectItem(&drawing);
    ui->drawingItemTree->setCurrentItem(nullptr);

    int i=0;
    while (i < drawing.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) drawing.child(i);
        ui->drawingWindow->showItem(child);
        ui->drawingWindow->selectItem(child);
        i++;
    }
}

// all must be the same type
bool OpenParEMg::isDrawingValidShow ()
{
    //std::cout << "OpenParEMg::isDrawingValidShow" << std::endl; std::cout.flush();

    // bool foundType=false;
    // TopAbs_ShapeEnum type;
    // QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    // int i=0;
    // while (i < selectedItems.count()) {
    //     CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
    //     if (foundType) {
    //         if (item->get_AIS_Shape()->Shape().ShapeType() != type) return false;
    //     } else {
    //         foundType=true;
    //         type=item->get_AIS_Shape()->Shape().ShapeType();
    //     }
    //     i++;
    // }
    return true;
}

void OpenParEMg::showRootDrawingItems ()
{
    //std::cout << "OpenParEMg::showRootDrawingItems" << std::endl; std::cout.flush();

    ui->drawingWindow->hideItem(&drawing);
    drawing.setForeground(0,Qt::black);

    int i=0;
    while (i < drawing.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) drawing.child(i);
        child->setForeground(0,Qt::gray);
        ui->drawingWindow->showItem(child);
        i++;
    }

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::showDrawingItems ()
{
    //std::cout << "OpenParEMg::showDrawingItems" << std::endl; std::cout.flush();

    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            ui->drawingWindow->showItem(item);
        }
        i++;
    }

    ui->drawingItemTree->setCurrentItem(nullptr);
    ui->drawingWindow->updateViewer();
    setMenusI(1);
}

void OpenParEMg::hideRootDrawingItems ()
{
    std::cout << "OpenParEMg::hideRootDrawingItems" << std::endl; std::cout.flush();

    ui->drawingWindow->hideItem(&drawing);
    drawing.setForeground(0,Qt::black);

    ui->drawingWindow->updateViewer();
    setMenusI(2);
}

void OpenParEMg::hideDrawingItems ()
{
    std::cout << "OpenParEMg::hideDrawingItems" << std::endl; std::cout.flush();

    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            ui->drawingWindow->hideItem(item);
            ui->drawingWindow->unselectItem(item,i);
        }
        i++;
    }

    ui->drawingItemTree->setCurrentItem(nullptr);
    ui->drawingWindow->updateViewer();
    setMenusI(3);
}

void OpenParEMg::renamePathItems ()
{
    std::cout << "OpenParEMg::renamePathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_path()) {
            CustomLineEdit *name=new CustomLineEdit();
            name->setText(item->text(0));
            originalText=item->text(0);
            name->set_rxValidator();
            ui->drawingItemTree->setItemWidget(item,0,name);

            renameItem=item;
            renameEdit=name;
            connect(name,&CustomLineEdit::returnPressed,this,&OpenParEMg::rename_returnPressed);
        }
        i++;
    }
}

void OpenParEMg::deletePathItems ()
{
    std::cout << "OpenParEMg::deletePathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_path()) {
            boundaryDatabase->deletePath((Path *)item->get_OPEMobject());
            ui->drawingWindow->deleteItem(item);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(4);
}

void OpenParEMg::showRootPathItems ()
{
    std::cout << "OpenParEMg::showRootPathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        // if (item->is_rootPath()) {
        //     int j=0;
        //     while (j < item->childCount()) {
        //         CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
        //         ui->drawingWindow->showItem(child);
        //         child->setForeground(0,Qt::black);
        //         j++;
        //     }
        // } else {
        //     ui->drawingWindow->showItem(item);
        // }
        if (item && item->is_rootPath()) {
            ui->drawingWindow->showItem(item);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(5);
}

bool OpenParEMg::rootPathValidShow ()
{
    int i=0;
    while (i < path.childCount()) {
        std::cout << "OpenParEMg::rootPathValidShow" << std::endl; std::cout.flush();
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) path.child(i);
        if (child && child->foreground(0) == Qt::gray) return true;
        i++;
    }
    return false;
}

void OpenParEMg::hideRootPathItems ()
{
    std::cout << "OpenParEMg::hideRootPathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        // if (item->is_rootPath()) {
        //     int j=0;
        //     while (j < item->childCount()) {
        //         CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
        //         ui->drawingWindow->hideItem(child);
        //         child->setForeground(0,Qt::gray);
        //         j++;
        //     }
        // } else {
        //     ui->drawingWindow->hideItem(item);
        // }
        if (item && item->is_rootPath()) {
            ui->drawingWindow->hideItem(item);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(6);
}

bool OpenParEMg::isPathValidDelete ()
{
    std::cout << "OpenParEMg::isPathValidDelete" << std::endl; std::cout.flush();

    // see if any have linked items
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item) {
            if (item->is_path() || item->is_port() || item->is_integrationPathSegment()) {
                if (item->linkedItems_size() > 0) return false;
            }
        }
        i++;
    }
    return true;
}

bool OpenParEMg::isValidShowPath ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->isValidShow()) {
            if (item->is_path() || item->is_port() || item->is_integrationPathSegment()) return true;
        }
        i++;
    }
    return false;
}

void OpenParEMg::showPathItems ()
{
    std::cout << "OpenParEMg::showPathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_path()) {
            ui->drawingWindow->showItem(item);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(7);
}

bool OpenParEMg::rootPathValidHide ()
{
    int i=0;
    while (i < path.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) path.child(i);
        if (child && child->foreground(0) == Qt::black) return true;
        i++;
    }
    return false;
}

bool OpenParEMg::isValidHidePath ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->isValidHide()) {
            if (item->is_path() || item->is_port() || item->is_integrationPathSegment()) return true;
        }
        i++;
    }
    return false;
}

void OpenParEMg::hidePathItems ()
{
    std::cout << "OpenParEMg::hidePathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_path()) {
            ui->drawingWindow->hideItem(item);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(8);
}

void OpenParEMg::showRootPortItems ()
{
    std::cout << "OpenParEMg::showRootPortItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_rootPort()) {
        //     int j=0;
        //     while (j < item->childCount()) {
        //         CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
        //         ui->drawingWindow->showItem(child);
        //         child->setForeground(0,Qt::black);
        //         j++;
        //     }
        // } else {
            ui->drawingWindow->showItem(item);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(9);
}

void OpenParEMg::showPortItems ()
{
    std::cout << "OpenParEMg::showPortItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_port()) {
            ui->drawingWindow->showItem(item);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(10);
}

void OpenParEMg::hideRootPortItems ()
{
    std::cout << "OpenParEMg::hideRootPortItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_rootPort()) {
        //     int j=0;
        //     while (j < item->childCount()) {
        //         CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
        //         ui->drawingWindow->hideItem(child);
        //         j++;
        //     }
        // } else {
            ui->drawingWindow->hideItem(item);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(11);
}

void OpenParEMg::hidePortItems ()
{
    std::cout << "OpenParEMg::hidePortItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_port()) {
            ui->drawingWindow->hideItem(item);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(12);
}

void OpenParEMg::showRootMeshItems ()
{
    std::cout << "OpenParEMg::showRootMeshItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_rootMesh()) {
            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                ui->drawingWindow->showItem(child);
                child->setForeground(0,Qt::black);
                j++;
            }
        }
        // } else {
        //     ui->drawingWindow->showItem(item);
        // }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(13);
}

bool OpenParEMg::rootMeshValidShow ()
{
    int i=0;
    while (i < mesh.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) mesh.child(i);
        if (child->foreground(0) == Qt::gray) return true;
        i++;
    }
    return false;
}

void OpenParEMg::hideRootMeshItems ()
{
    std::cout << "OpenParEMg::hideRootMeshItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_rootMesh()) {
            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                ui->drawingWindow->hideItem(child);
                child->setForeground(0,Qt::gray);
                j++;
            }
        }
        // } else {
        //   ui->drawingWindow->hideItem(item);
        // }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(14);
}

void OpenParEMg::showMeshItems ()
{
    std::cout << "OpenParEMg::showMeshItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_mesh()) {
            ui->drawingWindow->showItem(item);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(15);
}

bool OpenParEMg::rootMeshValidHide ()
{
    int i=0;
    while (i < mesh.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) mesh.child(i);
        if (child->foreground(0) == Qt::black) return true;
        i++;
    }
    return false;
}

void OpenParEMg::hideMeshItems ()
{
    std::cout << "OpenParEMg::hideMeshItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_mesh()) {
            ui->drawingWindow->hideItem(item);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(16);
}

void OpenParEMg::renameSportNet ()
{
    std::cout << "OpenParEMg::renameSportNet" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_sport()) {
            CustomLineEdit *net=new CustomLineEdit();
            net->setText(item->text(0));
            originalText=item->text(0);
            net->set_rxValidator();
            ui->drawingItemTree->setItemWidget(item,0,net);

            renameItem=item;
            renameEdit=net;
            connect(net,&CustomLineEdit::returnPressed,this,&OpenParEMg::rename_returnPressed);
        }
        i++;
    }
}

bool is_uniqueItem (std::vector<CustomTreeWidgetItem *> *portItemList, QTreeWidgetItem *item)
{
    long unsigned int i=0;
    while (i < portItemList->size()) {
        if ((*portItemList)[i] == item) return false;
        i++;
    }
    return true;
}

bool OpenParEMg::deleteSportValid ()
{
    // get a list of unique port items
    std::vector<CustomTreeWidgetItem *> portItemList;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_sport()) {
            CustomTreeWidgetItem *portParentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            if (is_uniqueItem(&portItemList,portParentItem)) portItemList.push_back(portParentItem);
        }
        i++;
    }

    // cycle through the ports
    long unsigned int j=0;
    while (j < portItemList.size()) {

        // number of modes on this port
        Port *port=(Port *)portItemList[j]->get_OPEMobject();
        int modeCount=port->get_modeCount();

        // count the number of selected modes on this port
        int selectedCount=0;
        long unsigned int k=0;
        while (i < ui->drawingWindow->get_selectedItems_size()) {
            CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(k);
            if (item && item->is_sport()) {
                CustomTreeWidgetItem *portParentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
                if (portParentItem == portItemList[j]) selectedCount++;
            }
            k++;
        }

        if (modeCount == selectedCount) return false;

        j++;
    }

    return true;
}

bool OpenParEMg::hasOneSelectedSport ()
{
    bool found=false;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_sport()) {
            if (found) return false;
            found=true;
        }
        i++;
    }
    return true;
}

bool OpenParEMg::hasVoltage ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_sport()) {
            Mode *mode=(Mode *)item->get_OPEMobject();
            if (mode && mode->has_voltage()) return true;
        }
        i++;
    }
    return false;
}

bool OpenParEMg::hasCurrent ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_sport()) {
            Mode *mode=(Mode *)item->get_OPEMobject();
            if (mode) {
                if (mode->has_current()) return true;
            }
        }
        i++;
    }
    return false;
}

void OpenParEMg::insertPath (CustomTreeWidgetItem *item)
{
    std::cout << "OpenParEMg::insertPath" << std::endl; std::cout.flush();

    QDoubleValidator doubleValidator;
    doubleValidator.setBottom(0);

    // 1. collect the selected paths into a list
    // 2. find the mode to attach the paths to - there is only one due to pre-selection criteria

    std::vector<Path *> pathsToAdd;
    std::vector<CustomTreeWidgetItem *> pathItemList;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *selectedItem=ui->drawingWindow->get_selectedItem(i);
        if (selectedItem && selectedItem->is_path()) {
            Path *path=(Path *)selectedItem->get_OPEMobject();
            if (path) {
                pathsToAdd.push_back(path);
                pathItemList.push_back(selectedItem);
            }
        }
        i++;
    }

    // check that the selected paths can be used on the port

    CustomTreeWidgetItem *modeParentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    CustomTreeWidgetItem *portParentItem=(CustomTreeWidgetItem *)modeParentItem->QTreeWidgetItem::parent();

    // port outline
    if (portParentItem->linkedItems_size() != 1) return;
    CustomTreeWidgetItem *linkedItem=portParentItem->get_linkedItem(0);
    Path *portPath=static_cast<Path *>(linkedItem->get_OPEMobject());
    if (!portPath) return;

    long unsigned int j=0;
    while (j < pathsToAdd.size()) {
        if (!portPath->is_path_inside(pathsToAdd[j])) {
            QString message="Path \"";
            message.append(pathsToAdd[j]->get_name());
            message.append("\" cannot be assigned to the selected port.");
            QMessageBox mb;
            mb.critical(nullptr, "Error", message);
            mb.setFixedSize(500, 200);
            return;
        }
        j++;
    }

    // add an integration path
    std::vector<Path *> *pathList=boundaryDatabase->get_pathList_ptr();
    Mode *mode=(Mode *)modeParentItem->get_OPEMobject();
    IntegrationPath *integrationPath=nullptr;

    // add paths to the mode
    bool addScaleToTree=false;
    if (item->childCount() == 0) {
        // new integration path
        integrationPath=mode->addIntegrationPath(pathList,&pathsToAdd,item->text(0).toStdString());
        addScaleToTree=true;
    } else {
        // existing integration path
        int i=0;
        while (i < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
            std::cout << "child->get_itemType()=" << child->get_itemType() << std::endl; std::cout.flush();
            if (child->is_scale()) {
                int j=0;
                while (j < child->childCount()) {
                    CustomTreeWidgetItem *grandChild=(CustomTreeWidgetItem *)child->child(j);
                    std::cout << "   grandChild->get_itemType()=" << grandChild->get_itemType() << std::endl; std::cout.flush();
                    if (grandChild->is_scaleValue()) {
                        CustomLineEdit *scaleEdit=(CustomLineEdit *)ui->drawingItemTree->itemWidget(grandChild,0);
                        integrationPath=scaleEdit->get_integrationPath();

                        integrationPath->addPaths(pathList,&pathsToAdd);
                    }
                    j++;
                }
            }
            i++;
        }
    }

    if (!integrationPath) {
        std::cout << "ASSERT: OpenParEMg::insertPath failed to find an integration path" << std::endl; std::cout.flush();
        return;
    }

    if (addScaleToTree) {
        CustomTreeWidgetItem *itemScale=new CustomTreeWidgetItem(0);
        itemScale->setText(0,"scale");
        itemScale->set_itemType(12);
        itemScale->setFlags(item->flags() & ~Qt::ItemIsEditable);
        itemScale->setToolTip(0,"Scale factor for the integration path.");
        item->addChild(itemScale);

        CustomTreeWidgetItem *itemScaleValue=new CustomTreeWidgetItem(0);
        itemScaleValue->set_itemType(13);
        itemScaleValue->setFlags(itemScale->flags() & ~Qt::ItemIsSelectable);
        itemScale->addChild(itemScaleValue);

        CustomLineEdit *scaleEdit=new CustomLineEdit();
        scaleEdit->setText(QString::number(1));   // default value
        scaleEdit->set_itemTracker(ui->drawingWindow->get_itemTracker());
        scaleEdit->set_integrationPath(integrationPath);
        scaleEdit->set_boundaryDatabase(boundaryDatabase);
        scaleEdit->setValidator(&doubleValidator);
        ui->drawingItemTree->setItemWidget(itemScaleValue,0,scaleEdit);

        QObject::connect(scaleEdit,&CustomLineEdit::CustomTextChanged,&textValueChanged);
        QObject::connect(scaleEdit,&CustomLineEdit::CustomTextChanged,relay,&Relay::setMenus);
    }

    // add the path items to the tree

    i=0;
    while (i < pathsToAdd.size()) {
        QString pathText="+";
        pathText.append(pathsToAdd[i]->get_name());

        CustomTreeWidgetItem *pathItem=new CustomTreeWidgetItem(0);
        pathItem->setText(0,pathText);
        pathItem->setFlags(item->flags());
        pathItem->setToolTip(0,"Path segment for integration.");
        pathItem->set_itemType(14);
        pathItem->setForeground(0,Qt::gray);
        pathItem->set_OPEMobject(pathsToAdd[i]);
        item->addChild(pathItem);
        pathItemList[i]->push_linkedItem(pathItem);
        pathItem->push_linkedItem(pathItemList[i]);
        ui->drawingWindow->showItem(pathItem);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(17);
}

void OpenParEMg::rename_returnPressed ()
{
    std::cout << "OpenParEMg::rename_returnPressed" << std::endl; std::cout.flush();

    // new text
    QString newText=renameEdit->text();
    if (originalText.compare(newText) != 0) {
        if (renameItem->is_drawing()) {
            // nothing to do
        }

        if (renameItem->is_path()) {

            // item itself is now changed

            // change the names of the linked items
            long unsigned int i=0;
            while (i < renameItem->linkedItems_size()) {
                CustomTreeWidgetItem *item=renameItem->get_linkedItem(i);
                if (item->is_integrationPathSegment()) item->setText(0,newText);
                i++;
            }

            // change the database
            boundaryDatabase->renamePath(originalText.toStdString(),newText.toStdString());
        }

        if (renameItem->is_port()) {
            Port *port=(Port *)renameItem->get_OPEMobject();
            if (port) port->set_name(newText.toStdString());
        }

        if (renameItem->is_sport()) {
            Mode *mode=(Mode *)renameItem->get_OPEMobject();
            if (mode) mode->set_net(newText.toStdString());
        }
    }

    // replace
    ui->drawingItemTree->removeItemWidget(renameItem,0);
    renameItem->setText(0,newText);

    // update
    bool isExpanded=renameItem->isExpanded();
    renameItem->setExpanded(false);
    renameItem->setExpanded(true);
    if (!isExpanded) renameItem->setExpanded(false);
    renameItem=nullptr;

    setMenusI(18);
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::unselectRootDrawingItems()
{
    std::cout << "OpenParEMg::unselectRootDrawingItems" << std::endl; std::cout.flush();

    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_rootDrawing()) {
            ui->drawingWindow->unselectItem(item,i);
        }
        i++;
    }

    //setRootForeground(&drawing);
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    ui->drawingWindow->updateViewer();
    setMenusI(19);
}

void OpenParEMg::unselectDrawingItems()
{
    std::cout << "OpenParEMg::unselectDrawingItems" << std::endl; std::cout.flush();

    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            ui->drawingWindow->unselectItem(item,i);
        }
        i++;
    }

    ui->drawingItemTree->setCurrentItem(nullptr);
    ui->drawingWindow->updateViewer();
    setMenusI(20);
}

bool OpenParEMg::isValidRenameDrawingItems ()
{
    int count=0;
    QList<QTreeWidgetItem*> items=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < items.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)items[i];
        if (item && item->is_drawing()) count++;
        i++;
    }
    if (count == 1 && items.count() == count) return true;
    return false;
}

void OpenParEMg::renameDrawingItems ()
{
    QList<QTreeWidgetItem*> items=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < items.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)items[i];
        if (item && item->is_drawing()) {
            CustomLineEdit *name=new CustomLineEdit();
            name->setText(item->text(0));
            originalText=item->text(0);
            name->set_rxValidator();
            ui->drawingItemTree->setItemWidget(item,0,name);

            renameItem=item;
            renameEdit=name;
            connect(name,&CustomLineEdit::returnPressed,this,&OpenParEMg::rename_returnPressed);
        }
        i++;
    }
}

void OpenParEMg::deleteDrawingItems ()
{
    std::cout << "OpenParEMg::deleteDrawingItems" << std::endl; std::cout.flush();

    //activeAction=true;  // no need since there is not a cancel option
    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {

            // parentItem
            CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();

            if (parentItem && parentItem->is_rootDrawing()) {
                int insertIndex=parentItem->indexOfChild(item);

                // move children to parent
                while (item->childCount() > 0) {
                    CustomTreeWidgetItem* child=(CustomTreeWidgetItem *)item->takeChild(0);
                    parentItem->insertChild(insertIndex++,child);
                    ui->drawingWindow->showItem(child);

                    // set the materials
                    if (!item->text(1).isNull()) {
                        if (!child->getPolywire()) child->setText(1,item->text(1));
                    }

                    // adjust the depth
                    decrease_depth(child);
                }

                parentItem->removeChild(item);
            }

            // remove the item
            //ui->drawingWindow->deleteItem(item);
            itemChangesStack.add(item);

            // remove the old version from display and tracking
            ui->drawingWindow->hideItem(item);
            ui->drawingWindow->removeItemFromMap(item);
            ui->drawingWindow->deleteShape(item->getShape());  // lose selection

            // clone the item onto itself for undo/redo
            ShapeData *newShapeData=item->getShapeData()->copyCreate();
            newShapeData->setDelete();
            item->addShapeData(newShapeData);
            ui->drawingWindow->unselectItem(item);

            std::cout << "   item=" << item << "  item->getChildrenSize()=" << item->getChildrenSize() << std::endl; std::cout.flush();

            // reset the top-level compound
            reprocess(&drawing);

            drawingChanged=true;
        }
        i++;
    }

    // see if everything has been deleted
    //if (drawing.childCount() == 0) resetDrawing();

    // restoreSelection();
    // clickedItem=nullptr;
    // previousClickedItem=nullptr;

    // ui->drawingWindow->updateViewer();
    // setMenusI(21);

    finishOperation(false,100);
}

void OpenParEMg::insertModeItems ()
{
    std::cout << "OpenParEMg::insertModeItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_port()) {
            Port *port=boundaryDatabase->get_port(item->text(0).toStdString());

            Mode *newMode=new Mode(0,0,port->get_impedance_calculation());
            std::string net="net";
            net.append(std::to_string(boundaryDatabase->get_SportCount()+1));
            newMode->set_net(net);
            newMode->set_Sport(boundaryDatabase->get_SportCount()+1);
            port->push_mode(newMode);

            newMode->draw(relay,boundaryDatabase,ui->drawingWindow,ui->drawingItemTree,&path,item);
        }
        i++;
    }

    setMenusI(22);
}

void OpenParEMg::unselectPortItems()
{
    std::cout << "OpenParEMg::unselectPortItems" << std::endl; std::cout.flush();

    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_port()) {
            ui->drawingWindow->unselectItem(item,i);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(23);
}

void OpenParEMg::renamePortItems ()
{
    std::cout << "OpenParEMg::renamePortItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_port()) {
            CustomLineEdit *name=new CustomLineEdit();
            name->setText(item->text(0));
            originalText=item->text(0);
            name->set_rxValidator();
            ui->drawingItemTree->setItemWidget(item,0,name);

            renameItem=item;
            renameEdit=name;
            connect(name,&CustomLineEdit::returnPressed,this,&OpenParEMg::rename_returnPressed);
        }
        i++;
    }
}

void OpenParEMg::deletePortItem (CustomTreeWidgetItem * item)
{
    std::cout << "OpenParEMg::deletePortItem" << std::endl; std::cout.flush();

    // unlink the outline
    Port *port=(Port *)item->get_OPEMobject();
    Path *outline=port->get_outline();
    CustomTreeWidgetItem *outlineItem=outline->get_item();
    outlineItem->removeLinkedItem(item);

    // remove from the boundary database
    boundaryDatabase->deletePort(item->text(0).toStdString());

    // remove the integration paths
    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
        if (child->is_sport()) {
            int j=0;
            while (j < child->childCount()) {
                CustomTreeWidgetItem *grandChild=(CustomTreeWidgetItem *) child->child(j);
                if (grandChild->is_voltage() || grandChild->is_current()) {
                    int k=0;
                    while (k < grandChild->childCount()) {
                        CustomTreeWidgetItem *greatGrandChild=(CustomTreeWidgetItem *) grandChild->child(k);
                        if (greatGrandChild->is_integrationPathSegment()) {

                            // unlink
                            Path *path=(Path *)greatGrandChild->get_OPEMobject();
                            CustomTreeWidgetItem *pathItem=path->get_item();
                            pathItem->removeLinkedItem(greatGrandChild);

                            // delete
                            ui->drawingWindow->deleteItem(greatGrandChild);
                        }
                        k++;
                    }
                }
                j++;
            }
        }
        i++;
    }

    ui->drawingWindow->deleteItem(item);
    ui->drawingWindow->updateViewer();
    setMenusI(24);
}

void OpenParEMg::deleteRootPortItems ()
{
    std::cout << "OpenParEMg::deleteRootPortItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_rootPort()) {
            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                deletePortItem(child);
            }
        }
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    setMenusI(25);
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::deletePortItems ()
{
    std::cout << "OpenParEMg::deletePortItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_port()) {
            deletePortItem(item);
        }
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    ui->drawingWindow->updateViewer();
    setMenusI(26);
}

void OpenParEMg::deleteSportItem (CustomTreeWidgetItem *item)
{
    std::cout << "OpenParEMg::deleteSportItem" << std::endl; std::cout.flush();

    if (!item) return;

    // port
    CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    Port *port=boundaryDatabase->get_port(parentItem->text(0).toStdString());

    // remove from the port
    port->deleteMode(item->text(0).toStdString());

    // remove the integration paths
    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        if (child->is_voltage() || child->is_current()) {
            int j=0;
            while (j < child->childCount()) {
                CustomTreeWidgetItem *grandChild=(CustomTreeWidgetItem *)child->child(j);
                if (grandChild->is_integrationPathSegment()) {

                    // unlink
                    Path *path=(Path *)grandChild->get_OPEMobject();
                    CustomTreeWidgetItem *pathItem=path->get_item();
                    pathItem->removeLinkedItem(grandChild);

                    ui->drawingWindow->deleteItem(grandChild);
                }
                j++;
            }
        }
        i++;
    }

    ui->drawingWindow->deleteItem(item);
    setMenusI(27);
}

void OpenParEMg::deleteSportItems ()
{
    std::cout << "OpenParEMg::deleteSportItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_sport()) {
            deleteSportItem(item);
        }
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    ui->drawingWindow->updateViewer();
    setMenusI(28);
}

void OpenParEMg::showNetItems ()
{
    std::cout << "OpenParEMg::showNetItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_sport()) {
            ui->drawingWindow->showItem(item);

            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                if (child->is_voltage() || child->is_current()) {
                    //child->setForeground(0,Qt::black);
                    int k=0;
                    while (k < child->childCount()) {
                        CustomTreeWidgetItem *grandChild=(CustomTreeWidgetItem *) child->child(k);
                        if (grandChild->is_integrationPathSegment()) {
                            ui->drawingWindow->showItem(grandChild);
                            grandChild->setForeground(0,Qt::black);
                        }
                        k++;
                    }
                }
                j++;
            }
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(29);
}

void OpenParEMg::showVIItems ()
{
    std::cout << "OpenParEMg::showVIItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item) {
            if (item->is_voltage() || item->is_current()) {
                ui->drawingWindow->showItem(item);
            }
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::removeIntegrationPathItems ()
{
    std::cout << "OpenParEMg::removeIntegrationPathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_integrationPathSegment()) {

            CustomTreeWidgetItem *VIParentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            CustomTreeWidgetItem *modeParentItem=(CustomTreeWidgetItem *)VIParentItem->QTreeWidgetItem::parent();

            Mode *mode=(Mode *)modeParentItem->get_OPEMobject();
            if (mode) {
                // remove path
                std::string type=VIParentItem->text(0).toStdString();
                mode->removeIntegrationPath(VIParentItem->text(0).toStdString(),(Path *)item->get_OPEMobject());

                // remove from linked items
                int j=0;
                while (j < path.childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) path.child(j);
                    child->removeLinkedItem(item);
                    j++;
                }

                // remove item
                ui->drawingWindow->hideItem(item);
                ui->drawingWindow->deleteItem(item);

                // remove scale if there are no paths
                bool hasPaths=false;
                j=0;
                while (j < VIParentItem->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) VIParentItem->child(j);
                    if (child->is_integrationPathSegment()) {hasPaths=true; break;}
                    j++;
                }
                if (!hasPaths) {
                    int j=0;
                    while (j < VIParentItem->childCount()) {
                        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) VIParentItem->child(j);
                        if (child->is_scale()) {
                            ui->drawingWindow->deleteItem(child);
                            break;
                        }
                        j++;
                    }
                }
            }
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(31);
}

void OpenParEMg::showIntegrationPathItems ()
{
    std::cout << "OpenParEMg::showIntegrationPathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_integrationPathSegment()) {
            ui->drawingWindow->showItem(item);
            item->setForeground(0,Qt::black);
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
    setMenusI(32);
}

void OpenParEMg::hideNetItems ()
{
    std::cout << "OpenParEMg::hideNetItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_sport()) {
            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                if (child->is_voltage() || child->is_current()) {
                    //child->setForeground(0,Qt::gray);
                    int k=0;
                    while (k < child->childCount()) {
                        CustomTreeWidgetItem *grandChild=(CustomTreeWidgetItem *) child->child(k);
                        if (grandChild->is_integrationPathSegment()) {
                            ui->drawingWindow->hideItem(grandChild);
                            grandChild->setForeground(0,Qt::gray);
                        }
                        k++;
                    }
                }
                j++;
            }
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(33);
}

void OpenParEMg::hideVIItems ()
{
    std::cout << "OpenParEMg::hideVIItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item) {
            if (item->is_voltage() || item->is_current()) {
                ui->drawingWindow->hideItem(item);
            }
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(34);
}

void OpenParEMg::hideIntegrationPathItems ()
{
    std::cout << "OpenParEMg::hideIntegrationPathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_integrationPathSegment()) {
            ui->drawingWindow->hideItem(item);
            item->setForeground(0,Qt::gray);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(35);
}

void OpenParEMg::createPath ()
{
    std::cout << "OpenParEMg::createPath  NbSelected=" << ui->drawingWindow->NbSelected() << std::endl; std::cout.flush();

    int count=0;
    while (count < ui->drawingWindow->NbSelected()) {

        // default path name

        std::string pathName="p";

        int i=1;
        while (boundaryDatabase->pathNameExists(pathName)) {
            std::string testName=pathName;
            testName.append("_").append(std::to_string(i));
            if (boundaryDatabase->pathNameExists(testName)) {i++;}
            else {pathName=testName; break;}
        }

        // path name placed in a keywordPair
        keywordPair *kwPathName=new keywordPair();
        kwPathName->set_keyword("path");
        kwPathName->set_value(pathName);
        kwPathName->set_lineNumber(0);
        kwPathName->set_loaded(true);

        // path

        Path *newPath=new Path(0,0);
        newPath->set_name(pathName);
        newPath->is_modified();
        TopoDS_Shape subshape=ui->drawingWindow->get_selectedSubshape(count);
        if (subshape.ShapeType() == TopAbs_FACE) newPath->addFacePoints(TopoDS::Face(subshape));
        else if (subshape.ShapeType() == TopAbs_WIRE) newPath->addWirePoints(TopoDS::Wire(subshape));
        else if (subshape.ShapeType () == TopAbs_EDGE) newPath->addEdgePoints(TopoDS::Edge(subshape));
        newPath->create_item(ui->drawingWindow,&path);  // create item and add as child to path; creates AIS_Shape

        boundaryDatabase->push_path(newPath);

        // add new path to the drawing
        CustomTreeWidgetItem *item=newPath->get_item();
        if (item) insertToMapActivateItem(item);

        // see if the path is within an existing port
        Port *port=boundaryDatabase->get_matchingPort(newPath);
        if (port) newPath->set_portItem(port->get_item());

        count++;
    }

    ui->drawingWindow->setSubshapeSelection(false);
    on_actionShape_triggered();
    ui->drawingWindow->updateViewer();
    setMenusI(36);
}

// void OpenParEMg::replaceItemShape (CustomTreeWidgetItem *item, TopoDS_Shape &shape, int i)
// {
//     //std:: cout << "OpenParEMg::replaceItemShape  item=" << item << "  place=" << i << std::endl; std::cout.flush();

//     if (!item) return;

//     // save for later restoration since the operations will modify the visible item list
//     std::vector<CustomTreeWidgetItem *> displayedItems=ui->drawingWindow->getVisibleDrawingItems();

//     // remove old shape
//     if (!item->getShape().IsNull()) {
//         ui->drawingWindow->hideItem(item);
//         ui->drawingWindow->removeItemFromMap(item);
//         ui->drawingWindow->deleteShape(item->getShape());  // lose selection
//     }

//     // refresh selection: Processing on rootDrawing deselects all but 1 item, so reselect here.
//     if (item->is_rootDrawing()) {
//         long unsigned int i=0;
//         while (i < ui->drawingWindow->get_selectedItems_size()) {
//             CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
//             if (item) ui->drawingWindow->refreshSelectedItem(item);
//             i++;
//         }
//     }

//     // install new shape
//     Handle(AIS_Shape) AISshape=new AIS_Shape(shape);
//     // must activate for selection since SetAutoActivateSelection is set to false in CustomOpenGLWidget.cpp
//     if (!item->is_rootDrawing()) ui->drawingWindow->activateSelectShape(AISshape);
//     item->setShape(AISshape);
//     ui->drawingWindow->insertItemToMap(AISshape,item);

//     // redisplay
//     if (item->is_rootDrawing()) {
//         long unsigned int i=0;
//         while (i < displayedItems.size()) {
//             ui->drawingWindow->showItem(displayedItems[i]);
//             i++;
//         }
//     }
// }

// void OpenParEMg::replaceItemShape (CustomTreeWidgetItem *item, Polywire *polywire, int i)
// {
//     //std:: cout << "OpenParEMg::replaceItemShape  item=" << item << "  polywire=" << polywire << "  place=" << i << std::endl; std::cout.flush();

//     if (!item) return;
//     if (!polywire) return;

//     // save for later restoration since the operations will modify the visible item list
//     std::vector<CustomTreeWidgetItem *> displayedItems=ui->drawingWindow->getVisibleDrawingItems();

//     // remove old shape
//     if (!item->getShape().IsNull()) {
//         ui->drawingWindow->hideItem(item);
//         ui->drawingWindow->removeItemFromMap(item);
//         ui->drawingWindow->deleteShape(item->getShape());  // lose selection
//     }

//     // refresh selection: Processing on rootDrawing deselects all but 1 item, so reselect here.
//     if (item->is_rootDrawing()) {
//         long unsigned int i=0;
//         while (i < ui->drawingWindow->get_selectedItems_size()) {
//             CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
//             if (item) ui->drawingWindow->refreshSelectedItem(item);
//             i++;
//         }
//     }

//     // get a new shape
//     Handle(AIS_Shape) AISshape=polywire->get_AIS_Shape();
//     if (AISshape.IsNull()) return;

//     // install new shape
//     // must activate for selection since SetAutoActivateSelection is set to false in CustomOpenGLWidget.cpp
//     if (!item->is_rootDrawing()) ui->drawingWindow->activateSelectShape(AISshape);
//     if (item->getShapeData()) {
//         item->getShapeData()->setShape(AISshape);
//     } else {
//         ShapeData *newShapeData=new ShapeData (0,nullptr,nullptr,AISshape);
//         item->addShapeData(newShapeData);
//     }
//     ui->drawingWindow->insertItemToMap(AISshape,item);

//     // redisplay
//     if (item->is_rootDrawing()) {
//         long unsigned int i=0;
//         while (i < displayedItems.size()) {
//             ui->drawingWindow->showItem(displayedItems[i]);
//             i++;
//         }
//     }
// }

bool OpenParEMg::isValidExtrudePolywire ()
{
    int polywireCount=0;
    QList<QTreeWidgetItem*> items=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < items.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)items[i];
        if (item && item->is_drawing()) {
            CustomTreeWidgetItem *parent=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            if (parent && parent->is_rootDrawing()) {
                Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
                if (polywire && polywire->isClosed()) polywireCount++;
            }
        }
        i++;
    }
    if (polywireCount > 0 && items.count() == polywireCount) return true;
    return false;
}

void OpenParEMg::extrudePolywire ()
{
    //std::cout << "OpenParEMg::extrudePolywire" << std::endl; std::cout.flush();

    startOperation(true);
    activeAction=true;
    itemChangesStack.startNew();

    // long unsigned int i=0;
    // while (i < ui->drawingWindow->get_selectedItems_size()) {
    //     CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
    //     if (item && item->is_drawing()) {
    //         itemChangesStack.add(item);
    //     }
    //     i++;
    // }

    // user input form
    if (lengthInputForm) delete lengthInputForm;
    lengthInputForm=new LengthInputForm();
    lengthInputForm->set_drawingWindow(ui->drawingWindow);
    extrusionDirection.SetCoord(0,0,0);
    lengthInputForm->set_extrusionDirection(&extrusionDirection);
    lengthInputForm->set_length(&length);
    lengthInputForm->set_relay(relay);
    lengthInputForm->setModal(false);
    connect(this,&OpenParEMg::sendPnt,lengthInputForm,&LengthInputForm::pickVertexFinished);
    lengthInputForm->show();
}

void OpenParEMg::finishExtrudePolywire (bool cancel)
{
    //std::cout << "OpenParEMg::finishExtrudePolywire" << std::endl; std::cout.flush();

    if (!cancel && abs(length) > 1e-12) {

        int i=0;
        while (i < ui->drawingWindow->get_selectedItems_size()) {
            CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
            if (item && !item->getShape().IsNull()) {

                // don't clone; a new item will be created

                TopoDS_Shape extrudeShape=item->getShape()->Shape();

                // pick off the face to exclude any extra vertices added for selection convenience
                if (extrudeShape.ShapeType() == TopAbs_COMPOUND) {
                    TopoDS_Iterator it(extrudeShape);
                    for (; it.More(); it.Next()) {
                        TopoDS_Shape subShape=it.Value();
                        if (subShape.ShapeType() == TopAbs_FACE) {
                            extrudeShape=subShape;
                            break;
                        }
                    }
                }

                // extrude
                Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
                if (polywire) {

                    // set direction
                    polywire->setReverseExtrusionDirection(false);
                    if (extrusionDirection.Magnitude() > 1e-12) {
                        if (polywire->getNormal().IsOpposite(extrusionDirection,1.5)) {
                            polywire->setReverseExtrusionDirection(true);
                        }
                    }

                    // scale it
                    gp_Vec scaledVec=gp_Vec(polywire->getNormal())*length;
                    if (polywire->getReverseExtrusionDirection()) scaledVec=-scaledVec;

                    // extrude it
                    BRepPrimAPI_MakePrism aPrism(extrudeShape,scaledVec);
                    if (aPrism.IsDone()) {

                        Handle(AIS_Shape) newShape=new AIS_Shape(aPrism);

                        // define the process
                        Extrude *extrude=new Extrude();
                        extrude->set_length(length);

                        // add it
                        CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem(0);
                        newItem->setText(0,extrude->getName(&objectCounts));
                        ShapeData *newShapeData=new ShapeData(1,nullptr,extrude,newShape);
                        newItem->addShapeData(newShapeData);
                        itemChangesStack.add(newItem);

                        drawing.addChild(newItem);
                        extrude=nullptr;

                        //ui->drawingWindow->displayShape(newItem->getShape());
                        ui->drawingWindow->activateItem(newItem);
                        ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);
                        ui->drawingWindow->showItem(newItem);
                        ui->drawingWindow->selectItem(newItem);

                        // move the object in the selection tree
                        int index=drawing.indexOfChild(item);
                        drawing.takeChild(index);
                        newItem->addChild(item);

                        // add the object to the child list for undo/redo
                        newItem->push_child(item);

                        // hide/show
                        ui->drawingWindow->hideItem(item);
                        ui->drawingWindow->unselectItem(item,i);  // changes list

                        ui->drawingWindow->showItem(newItem);
                        ui->drawingWindow->selectItem(newItem);

                        increase_depth(item);
                        previousClickedItem=clickedItem;
                        clickedItem=newItem;

                        drawingChanged=true;
                    }
                }
                item->resetOperation();
                activeAction=false;
            }
            i++;
        }
    }

    if (lengthInputForm) {lengthInputForm=nullptr;}
    finishOperation(false,1);
}

// void OpenParEMg::reextrudePolywire (CustomTreeWidgetItem *item, CustomTreeWidgetItem *child)
// {
//     std::cout << "OpenParEMg::reextrudePolywire  item=" << item << "  child=" << child << std::endl; std::cout.flush();

//     if (!item) return;

//     Process *process=item->getProcess();
//     if (process) {
//         Extrude *extrude=dynamic_cast<Extrude *>(process);
//         if (extrude) {
//             Polywire *polywire=child->getPolywire();
//             if (polywire) {
//                 gp_Vec scaledVec=gp_Vec(polywire->getNormal())*extrude->get_length();
//                 if (polywire->getReverseExtrusionDirection()) scaledVec=-scaledVec;

//                 // see finishExtrudePolywire for comments
//                 TopoDS_Shape extrudeShape=child->getShape()->Shape();
//                 if (extrudeShape.ShapeType() == TopAbs_COMPOUND) {
//                     TopoDS_Iterator it(extrudeShape);
//                     for (; it.More(); it.Next()) {
//                         TopoDS_Shape subShape=it.Value();
//                         if (subShape.ShapeType () == TopAbs_FACE) {
//                             extrudeShape=subShape;
//                             break;
//                         }
//                     }
//                 }

//                 BRepPrimAPI_MakePrism aPrism(extrudeShape,scaledVec);
//                 TopoDS_Shape newShape=aPrism;
//                 replaceItemShape(item,newShape,1);  // inserts to item map
//                 drawingChanged=true;
//             }
//         }
//     }
// }

// stop on incomplete structures - happens while loading a drawing
// eventually, everything loads and the reprocess completes
void OpenParEMg::reprocess (CustomTreeWidgetItem *item)
{
    //std::cout << "OpenParEMg::reprocess  item=" << item << std::endl; std::cout.flush();

    bool stop=false;

    if (!item) return;

    if (item == &drawing) {

        // if (!item->getShape().IsNull()) {
        //     ui->drawingWindow->hideItem(item);
        //     ui->drawingWindow->removeItemFromMap(item);
        //     ui->drawingWindow->deleteShape(item->getShape());
        // }

        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);

        // cycle through the top-level children and add to the compound
        int i=0;
        while (i < drawing.childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)drawing.child(i);
            if (child && !child->getShape().IsNull()) {
                builder.Add(compound,child->getShape()->Shape());
            }
            i++;
        }

        Handle(AIS_Shape) newAISshape=new AIS_Shape(compound);

        ShapeData *shapeData=item->getShapeData();
        shapeData->setShape(newAISshape);

        // ui->drawingWindow->insertItemToMap(item->getShape(),item);
        // ui->drawingWindow->displayShape(item->getShape());
        // ui->drawingWindow->selectItem(item);
        // ui->drawingWindow->showItem(item);

        return;
    }

    Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
    if (polywire) {
        // if (!item->getShape().IsNull()) {
        //     ui->drawingWindow->hideItem(item);
        //     ui->drawingWindow->removeItemFromMap(item);
        //     ui->drawingWindow->deleteShape(item->getShape());
        // }

        ShapeData *shapeData=item->getShapeData();
        shapeData->setShape(polywire->get_AIS_Shape());

        ui->drawingWindow->insertItemToMap(item->getShape(),item);
        //ui->drawingWindow->displayShape(item->getShape());
        ui->drawingWindow->activateItem(item);
        //ui->drawingWindow->selectItem(item);
        //ui->drawingWindow->showItem(item);

        //replaceItemShape(item,polywire,2);
    }

    Process *process=static_cast<Process *>(item->getProcess());
    if (process) {

        Extrude *extrude=dynamic_cast<Extrude *>(process);
        if (extrude) {
            if (item->childCount() > 0) {
                int i=0;
                while (i < item->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
                    if (child) {

                        Polywire *polywire=static_cast<Polywire *>(child->getPolywire());
                        if (polywire) {
                            gp_Vec scaledVec=gp_Vec(polywire->getNormal())*extrude->get_length();
                            if (polywire->getReverseExtrusionDirection()) scaledVec=-scaledVec;

                            Handle(AIS_Shape) AISextrudeShape=child->getShape();
                            if (!AISextrudeShape.IsNull()) {

                                // see finishExtrudePolywire for comments
                                TopoDS_Shape extrudeShape=AISextrudeShape->Shape();
                                if (extrudeShape.ShapeType() == TopAbs_COMPOUND) {
                                    TopoDS_Iterator it(extrudeShape);
                                    for (; it.More(); it.Next()) {
                                        TopoDS_Shape subShape=it.Value();
                                        if (subShape.ShapeType () == TopAbs_FACE) {
                                            extrudeShape=subShape;
                                            break;
                                        }
                                    }
                                }

                                BRepPrimAPI_MakePrism aPrism(extrudeShape,scaledVec);
                                TopoDS_Shape newShape=aPrism;
                                Handle(AIS_Shape) newAISshape=new AIS_Shape(newShape);

                                if (!item->getShape().IsNull()) {
                                    ui->drawingWindow->hideItem(item);
                                    ui->drawingWindow->removeItemFromMap(item);
                                    ui->drawingWindow->deleteShape(item->getShape());
                                }

                                ShapeData *shapeData=item->getShapeData();
                                shapeData->setShape(newAISshape);

                                ui->drawingWindow->insertItemToMap(item->getShape(),item);
                                //ui->drawingWindow->displayShape(item->getShape());
                                ui->drawingWindow->activateItem(item);
                                //ui->drawingWindow->selectItem(item);
                                //ui->drawingWindow->showItem(item);

                                drawingChanged=true;
                            } else {
                                stop=true;
                            }
                        }
                    } else {
                        stop=true;
                    }
                    i++;
                }
            } else {
                stop=true;
            }
        }

        Merge *merge=dynamic_cast<Merge *>(process);
        if (merge) {
            if (item->childCount() == 2) {

                CustomTreeWidgetItem *child1=(CustomTreeWidgetItem *)item->child(0);
                CustomTreeWidgetItem *child2=(CustomTreeWidgetItem *)item->child(1);

                if (child1 && child2) {

                    ui->drawingWindow->hideItem(child1);
                    ui->drawingWindow->hideItem(child2);

                    // get shapes

                    Handle(AIS_Shape) AISshape1=child1->getShape();
                    Handle(AIS_Shape) AISshape2=child2->getShape();

                    if (!AISshape1.IsNull() && !AISshape2.IsNull()) {
                        TopoDS_Shape shape1=AISshape1->Shape();
                        TopoDS_Shape shape2=AISshape2->Shape();

                        // build merged shape
                        BRepAlgoAPI_Fuse fuse(shape1,shape2);
                        fuse.Build();
                        if (!fuse.IsDone()) return;

                        TopoDS_Shape mergedShape=fuse.Shape();

                        ShapeUpgrade_UnifySameDomain unify(mergedShape);
                        unify.Build();
                        mergedShape=unify.Shape();
                        Handle(AIS_Shape) newAISshape=new AIS_Shape(mergedShape);

                        if (!item->getShape().IsNull()) {
                            ui->drawingWindow->hideItem(item);
                            ui->drawingWindow->removeItemFromMap(item);
                            ui->drawingWindow->deleteShape(item->getShape());
                        }

                        ShapeData *shapeData=item->getShapeData();
                        shapeData->setShape(newAISshape);

                        ui->drawingWindow->insertItemToMap(item->getShape(),item);
                        //ui->drawingWindow->displayShape(item->getShape());
                        ui->drawingWindow->activateItem(item);
                        //ui->drawingWindow->selectItem(item);
                        //ui->drawingWindow->showItem(item);

                        //replaceItemShape(item,mergedShape,4);  // inserts to item map
                        drawingChanged=true;
                    } else {
                        stop=true;
                    }
                } else {
                    stop=true;
                }
            } else {
                stop=true;
            }
        }

        Subtract *subtract=dynamic_cast<Subtract *>(process);
        if (subtract) {
            if (item->childCount() == 2) {

                CustomTreeWidgetItem *child1=(CustomTreeWidgetItem *)item->child(0);
                CustomTreeWidgetItem *child2=(CustomTreeWidgetItem *)item->child(1);

                if (child1 && child2) {

                    ui->drawingWindow->hideItem(child1);
                    ui->drawingWindow->hideItem(child2);

                    Handle(AIS_Shape) AISshape1=child1->getShape();
                    Handle(AIS_Shape) AISshape2=child2->getShape();

                    if (!AISshape1.IsNull() && !AISshape2.IsNull()) {

                        // get shapes
                        TopoDS_Shape shape1=AISshape1->Shape();
                        TopoDS_Shape shape2=AISshape2->Shape();

                        // build subtracted shape
                        BRepAlgoAPI_Cut cut(shape1,shape2);
                        cut.Build();
                        if (!cut.IsDone()) return;

                        TopoDS_Shape subtractedShape=cut.Shape();

                        ShapeUpgrade_UnifySameDomain unify(subtractedShape);
                        unify.Build();
                        subtractedShape=unify.Shape();
                        Handle(AIS_Shape) newAISshape=new AIS_Shape(subtractedShape);

                        if (!item->getShape().IsNull()) {
                            ui->drawingWindow->hideItem(item);
                            ui->drawingWindow->removeItemFromMap(item);
                            ui->drawingWindow->deleteShape(item->getShape());
                        }

                        ShapeData *shapeData=item->getShapeData();
                        shapeData->setShape(newAISshape);

                        ui->drawingWindow->insertItemToMap(item->getShape(),item);
                        //ui->drawingWindow->displayShape(item->getShape());
                        ui->drawingWindow->activateItem(item);
                        //ui->drawingWindow->selectItem(item);
                        //ui->drawingWindow->showItem(item);

                        //replaceItemShape(item,subtractedShape,5);  // inserts to item map
                        drawingChanged=true;
                    } else {
                        stop=true;
                    }
                } else {
                    stop=true;
                }
            } else {
                stop=true;
            }
        }
    }

    // necessary?
    item->reset_transformation();

    // recursively work to the top of the tree
    if (!stop) {
        CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
        reprocess(parentItem);
    }
}

bool OpenParEMg::isValidSetPlane ()
{
    if (ui->drawingWindow->get_selectedItems_size() > 0) return false;
    if (ui->drawingWindow->numberDrawingFaceSelected() == 1) return true;
    return false;
}

bool OpenParEMg::isValidCreatePort ()
{
    if (ui->drawingWindow->get_selectedItems_size() > 0) return false;
    if (ui->drawingWindow->numberDrawingFaceSelected() > 0) return true;
    return false;
}

bool OpenParEMg::isValidCreatePath ()
{
    if (ui->drawingWindow->get_selectedItems_size() > 0) return false;
    if (ui->drawingWindow->numberDrawingFaceSelected() > 0) return true;
    return false;
}

bool OpenParEMg::isValidAssignMaterial ()
{
    if (ui->drawingWindow->get_selectedItems_size() != 1) return false;
    CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(0);
    if (item && item->is_drawing()) {
        CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
        if (parentItem && parentItem->is_rootDrawing()) {

            // SOLID
            if (item->getShape()->Shape().ShapeType() == TopAbs_SOLID) {
                clickedItem=item;
                return true;
            }

            // COMPOUND
            if (item->getShape()->Shape().ShapeType() == TopAbs_COMPOUND) {
                // make sure it is not a polywire (a polycircle is a COMPOUND with a center point added)
                if (!item->getPolywire()) {
                    clickedItem=item;
                    return true;
                }
            }
        }
    }
    return false;
}

bool OpenParEMg::isValidObjectShow ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing() && item->isValidShow()) return true;
        i++;
    }
    return false;
}

bool OpenParEMg::isValidObjectHide ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing() && item->isValidHide()) return true;
        i++;
    }
    return false;
}

bool OpenParEMg::isValidObjectDelete ()
{
    return ui->drawingWindow->isValidDelete();
}

bool OpenParEMg::isValidObjectEdit ()
{
    //std::cout << "OpenParEMg::isValidObjectEdit" << std::endl; std::cout.flush();

    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
            if (polywire && polywire->canEdit()) count++;

            Process *process=static_cast<Process *>(item->getProcess());
            if (process && process->canEdit()) count++;
        }
        i++;
    }
    if (count == 1 && count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::editObject ()
{
    //std::cout << "OpenParEMg::editObject" << std::endl; std::cout.flush();

    startOperation(false);
    activeAction=true;
    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {

            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());

            lineEdit=nullptr;
            rectangleEdit=nullptr;
            polycircleEdit=nullptr;

            Line *line=dynamic_cast<Line *>(polywire);
            if (line) {
                lineEdit=line->copyCreate();
                if (lineEditForm) delete lineEditForm;
                lineEditForm=new LineEditForm();
                lineEditForm->set_polywire(lineEdit);
                lineEditForm->set_drawingWindow(ui->drawingWindow);
                lineEditForm->set_relay(relay);
                lineEditForm->setModal(false);
                connect(this,&OpenParEMg::sendPnt,lineEditForm,&LineEditForm::pickVertexFinished);
                lineEditForm->show();
            }

            Rectangle *rectangle=dynamic_cast<Rectangle *>(polywire);
            if (rectangle) {
                rectangleEdit=rectangle->copyCreate();
                if (rectangleEditForm) delete rectangleEditForm;
                rectangleEditForm=new RectangleEditForm();
                rectangleEditForm->set_polywire(rectangleEdit);
                rectangleEditForm->set_drawingWindow(ui->drawingWindow);
                rectangleEditForm->set_relay(relay);
                rectangleEditForm->setModal(false);
                connect(this,&OpenParEMg::sendPnt,rectangleEditForm,&RectangleEditForm::pickVertexFinished);
                rectangleEditForm->show();
            }

            Polycircle *polycircle=dynamic_cast<Polycircle *>(polywire);
            if (polycircle) {
                polycircleEdit=polycircle->copyCreate();
                if (polycircleEditForm) delete polycircleEditForm;
                polycircleEditForm=new PolycircleEditForm();
                polycircleEditForm->set_Polycircle(polycircleEdit);
                polycircleEditForm->set_drawingWindow(ui->drawingWindow);
                polycircleEditForm->set_relay(relay);
                polycircleEditForm->setModal(false);
                connect(this,&OpenParEMg::sendPnt,polycircleEditForm,&PolycircleEditForm::pickVertexFinished);
                polycircleEditForm->show();
            }

            Process *process=static_cast<Process *>(item->getProcess());
            if (process) {
                Extrude *extrude=dynamic_cast<Extrude *>(process);
                if (extrude) {
                    Polywire *polywire=nullptr;
                    int i=0;
                    while (i < item->childCount()) {
                        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
                        polywire=static_cast<Polywire *>(child->getPolywire());
                        if (polywire) break;
                        i++;
                    }

                    if (polywire) {
                        if (lengthEditForm) delete lengthEditForm;
                        lengthEditForm=new LengthInputForm();
                        length=extrude->get_length();
                        lengthEditForm->set_length(&length);
                        lengthEditForm->set_drawingWindow(ui->drawingWindow);
                        lengthEditForm->set_relay(relay);
                        lengthEditForm->setModal(false);
                        lengthEditForm->show();
                    }
                }
            }

            itemChangesStack.add(item);
        }
        i++;
    }
}

// void OpenParEMg::rebuildTopLevelShape ()
// {
//     //std::cout << "OpenParEMg::rebuildTopLevelShape" << std::endl; std::cout.flush();

//     TopoDS_Compound compound;
//     BRep_Builder builder;
//     builder.MakeCompound(compound);

//     // cycle through the top-level children and add to the compound
//     int i=0;
//     while (i < drawing.childCount()) {
//         CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)drawing.child(i);
//         if (child && !child->getShape().IsNull()) {
//             builder.Add(compound,child->getShape()->Shape());
//         }
//         i++;
//     }

//     //yyy
//     replaceItemShape(&drawing,compound,6);  // inserts to item map
// }

void OpenParEMg::finishEditObject (bool cancel)
{
    //std::cout << "OpenParEMg::finishEditObject  length=" << length << "  cancel=" << cancel << std::endl; std::cout.flush();

    if (!cancel) {
        long unsigned int i=0;
        while (i < ui->drawingWindow->get_selectedItems_size()) {
            CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
            if (item && item->is_drawing()) {

                // remove the old version from display and tracking
                ui->drawingWindow->hideItem(item);
                ui->drawingWindow->removeItemFromMap(item);
                ui->drawingWindow->deleteShape(item->getShape());  // lose selection

                // clone the item onto itself for undo/redo
                ShapeData *newShapeData=item->getShapeData()->copyCreate();
                newShapeData->setEdit();
                item->addShapeData(newShapeData);

                // modify the clone

                Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
                if (polywire) {
                    Line *line=dynamic_cast<Line *>(polywire);
                    Rectangle *rectangle=dynamic_cast<Rectangle *>(polywire);
                    Polycircle *polycircle=dynamic_cast<Polycircle *>(polywire);
                    if (line) item->setPolywire(lineEdit);
                    else if (rectangle) item->setPolywire(rectangleEdit);
                    else if (polycircle) item->setPolywire(polycircleEdit);

                    reprocess(item);
                }

                Process *process=static_cast<Process *>(item->getProcess());
                if (process) {
                    Extrude *extrude=dynamic_cast<Extrude *>(process);
                    if (extrude) {

                        // clone the child so that undo/redo works properly
                        int i=0;
                        while (i < item->childCount()) {
                            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
                            Polywire *polywire=static_cast<Polywire *>(child->getPolywire());
                            if (polywire) {
                                ShapeData *newShapeData=child->getShapeData()->copyCreate();
                                newShapeData->setEdit();
                                child->addShapeData(newShapeData);
                            }
                            i++;
                        }

                        extrude->set_length(length);
                        reprocess(item);
                    }
                }

                activeAction=false;

                // find and show the top-level item
                ui->drawingWindow->hideItem(item);
                CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
                if (parentItem) {
                    while (!parentItem->is_rootDrawing()) {
                        item=parentItem;
                        parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
                        if (!parentItem) break;
                    }
                }
                ui->drawingWindow->showItem(item);
            }
            i++;
        }

        drawingChanged=true;
    }

    if (lengthEditForm) {lengthEditForm=nullptr;}
    if (lineEditForm) {lineEditForm=nullptr;}
    if (rectangleEditForm) {rectangleEditForm=nullptr;}
    if (polycircleEditForm) {polycircleEditForm=nullptr;}

    finishOperation(false,2);
}

bool OpenParEMg::isValidMergeSolids ()
{
    //std::cout << "OpenParEMg::isValidMergeSolids" << std::endl; std::cout.flush();

    int solidCount=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            CustomTreeWidgetItem *parent=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            if (parent && parent == &drawing) {
                Handle(AIS_Shape) shape=item->getShape();
                if (!shape.IsNull()) {

                    // check for SOLID
                    if (shape->Shape().ShapeType() == TopAbs_SOLID) solidCount++;

                    // check for COMPOUND, where it might be a polycircle (due to the center vertex), which is invalid for merging
                    if (shape->Shape().ShapeType() == TopAbs_COMPOUND) {
                        bool foundPolycircle=false;
                        TopoDS_Shape testShape=shape->Shape();
                        if (testShape.ShapeType() == TopAbs_COMPOUND) {
                            TopoDS_Iterator it(testShape);
                            for (; it.More(); it.Next()) {
                                TopoDS_Shape subShape=it.Value();
                                if (subShape.ShapeType () == TopAbs_FACE) {
                                    foundPolycircle=true;
                                    break;
                                }
                            }
                        }
                        if (!foundPolycircle) solidCount++;
                    }
                }
            }
        }
        i++;
    }
    if (solidCount == 2 && ui->drawingWindow->get_selectedItems_count() == solidCount) return true;
    return false;
}

void OpenParEMg::mergeSolids ()
{
    startOperation(false);
    //activeAction=true;  // no need since there is no cancel option
    itemChangesStack.startNew();
    finishMergeSolids();
}

void OpenParEMg::finishMergeSolids ()
{
    if (ui->drawingWindow->get_selectedItems_count() != 2) return;

    // items
    CustomTreeWidgetItem *item0=nullptr;
    CustomTreeWidgetItem *item1=nullptr;
    long unsigned int index0;
    long unsigned int index1;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item) {
            if (!item0) {
                item0=item;
                index0=i;
            } else {
                item1=item;
                index1=i;
                break;
            }
        }
        i++;
    }
    if (!item0) return;
    if (!item1) return;

    // shapes
    TopoDS_Shape shape1=item0->getShape()->Shape();
    TopoDS_Shape shape2=item1->getShape()->Shape();

    // build merged shape
    BRepAlgoAPI_Fuse fuse(shape1,shape2);
    fuse.Build();
    if (!fuse.IsDone()) return;

    TopoDS_Shape mergedShape=fuse.Shape();

    ShapeUpgrade_UnifySameDomain unify(mergedShape);
    unify.Build();
    mergedShape=unify.Shape();
    Handle(AIS_Shape) newAISshape=new AIS_Shape(mergedShape);

    // define the process
    Merge *merge=new Merge();

    // add it
    CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem(0);
    ShapeData *newShapeData=new ShapeData(1,nullptr,merge,newAISshape);
    newItem->addShapeData(newShapeData);
    newItem->setText(0,merge->getName(&objectCounts));
    drawing.addChild(newItem);
    ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);
    //ui->drawingWindow->displayShape(newItem->getShape());
    ui->drawingWindow->activateItem(newItem);
    ui->drawingWindow->selectItem(newItem);
    ui->drawingWindow->showItem(newItem);
    itemChangesStack.add(newItem);

    // ToDo: put back?
    //merge=nullptr;

    // save the objects for undo/redo
    newItem->push_child(item0);
    newItem->push_child(item1);

    // move the objects in the selection tree

    int index=drawing.indexOfChild(item0);
    drawing.takeChild(index);
    newItem->addChild(item0);

    index=drawing.indexOfChild(item1);
    drawing.takeChild(index);
    newItem->addChild(item1);

    // reset materials
    QString nullMaterial;
    if (!item0->text(1).isNull()) {
        newItem->setText(1,item0->text(1));
        item0->setText(1,nullMaterial);
    }
    if (!item1->text(1).isNull()) item1->setText(1,nullMaterial);

    // reset dimTags
    item0->set_dimTag(-1,-1);
    item1->set_dimTag(-1,-1);

    // adjust depth
    increase_depth(item0);
    increase_depth(item1);

    // rebuild top level
    reprocess(&drawing);

    ui->drawingWindow->hideItem(item0);
    ui->drawingWindow->unselectItem(item0,index0);
    item0->resetOperation();

    ui->drawingWindow->hideItem(item1);
    ui->drawingWindow->unselectItem(item1,index1);
    item1->resetOperation();

    ui->drawingWindow->showItem(newItem);
    ui->drawingWindow->selectItem(newItem);

    drawingChanged=true;

    finishOperation(false,3);
}

bool OpenParEMg::isValidSubtractSolids ()
{
    //std::cout << "OpenParEMg::isValidSubtractSolids" << std::endl; std::cout.flush();
    return isValidMergeSolids();
}

void OpenParEMg::subtractSolids ()
{
    startOperation(false);
    //activeAction=true;  // no need since there is no cancel option
    itemChangesStack.startNew();
    finishSubtractSolids();
}

void OpenParEMg::finishSubtractSolids ()
{
    if (ui->drawingWindow->get_selectedItems_count() != 2) return;

    // items
    CustomTreeWidgetItem *item0=nullptr;
    CustomTreeWidgetItem *item1=nullptr;
    long unsigned int index0;
    long unsigned int index1;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item) {
            if (!item0) {
                item0=item;
                index0=i;
            } else {
                item1=item;
                index1=i;
                break;
            }
        }
        i++;
    }
    if (!item0) return;
    if (!item1) return;

    // shapes
    TopoDS_Shape shape1=item0->getShape()->Shape();
    TopoDS_Shape shape2=item1->getShape()->Shape();

    // build subtacted shape
    BRepAlgoAPI_Cut cut(shape1,shape2);
    cut.Build();
    if (!cut.IsDone()) return;

    TopoDS_Shape subtractedShape=cut.Shape();

    ShapeUpgrade_UnifySameDomain unify(subtractedShape);
    unify.Build();
    subtractedShape=unify.Shape();
    Handle(AIS_Shape) newAISshape=new AIS_Shape(subtractedShape);

    // define the process
    Subtract *subtract=new Subtract();

    // add it
    CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem(0);
    ShapeData *newShapeData=new ShapeData(1,nullptr,subtract,newAISshape);
    newItem->addShapeData(newShapeData);
    newItem->setText(0,subtract->getName(&objectCounts));
    drawing.addChild(newItem);
    ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);
    //ui->drawingWindow->displayShape(newItem->getShape());
    ui->drawingWindow->activateItem(newItem);
    ui->drawingWindow->selectItem(newItem);
    ui->drawingWindow->showItem(newItem);
    itemChangesStack.add(newItem);

    subtract=nullptr;

    // save the objects for undo/redo
    newItem->push_child(item0);
    newItem->push_child(item1);

    // move the object in the selection tree

    int index=drawing.indexOfChild(item0);
    drawing.takeChild(index);
    newItem->addChild(item0);

    index=drawing.indexOfChild(item1);
    drawing.takeChild(index);
    newItem->addChild(item1);

    // reset materials
    QString nullMaterial;
    if (!item0->text(1).isNull()) {
        newItem->setText(1,item0->text(1));
        item0->setText(1,nullMaterial);
    }
    if (!item1->text(1).isNull()) item1->setText(1,nullMaterial);

    // reset dimTags
    item0->set_dimTag(-1,-1);
    item1->set_dimTag(-1,-1);

    // adjust depth
    increase_depth(item0);
    increase_depth(item1);

    // rebuild top level
    reprocess(&drawing);

    ui->drawingWindow->hideItem(item0);
    ui->drawingWindow->unselectItem(item0,index0);
    item0->resetOperation();

    ui->drawingWindow->hideItem(item1);
    ui->drawingWindow->unselectItem(item1,index1);
    item1->resetOperation();

    ui->drawingWindow->showItem(newItem);
    ui->drawingWindow->selectItem(newItem);

    drawingChanged=true;

    finishOperation(false,4);
}

bool OpenParEMg::isValidObjectMove ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) count++;
        i++;
    }
    if (count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::moveObject ()
{
    //std::cout << "OpenParEMg::moveObject" << std::endl; std::cout.flush();

    startOperation(true);
    activeAction=true;
    itemChangesStack.startNew();
    ui->drawingWindow->set_pickSecondVertex(true);

    // set up for animation and undo/redo
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            item->setAnimate(ui->drawingWindow->get_viewerContext());
            item->resetP0P1();
            item->reset_transformation();
            item->setEnableMove(true);
            itemChangesStack.add(item);
        }
        i++;
    }
}

// void printPnt (std::string &name, const gp_Pnt &p)
// {
//     std::cout << name << "=(" << p.X() << "," << p.Y() << "," << p.Z() << ")" << std::endl; std::cout.flush();
// }

void OpenParEMg::finishMoveObject (CustomTreeWidgetItem *item, gp_Pnt p0, gp_Pnt p1, bool isChild)
{
    //std::cout << "OpenParEMg::finishMoveObject  isChild=" << isChild << std::endl; std::cout.flush();

    if (!item) return;

    item->unsetAnimate(ui->drawingWindow->get_viewerContext());

    // remove the old version from display and tracking
    ui->drawingWindow->hideItem(item);
    ui->drawingWindow->removeItemFromMap(item);
    ui->drawingWindow->deleteShape(item->getShape());

    // clone the item onto itself for undo/redo
    ShapeData *newShapeData=item->getShapeData()->copyCreate();
    newShapeData->setEdit();
    item->addShapeData(newShapeData);

    // modify the clone

    Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
    if (polywire) {
        polywire->shift(p1,p0);
        reprocess(item);
        drawingChanged=true;
    }

    Process *process=static_cast<Process *>(item->getProcess());
    if (process) {
        int i=0;
        while (i < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
            finishMoveObject(child,p0,p1,false);
            drawingChanged=true;
            i++;
        }

        reprocess(item);
        ui->drawingWindow->activateItem(item);
    }

    if (!polywire && !process) {
        TopoDS_Shape newShape=item->moveShape(p0,p1,ui->drawingWindow->get_viewerContext());
        Handle(AIS_Shape) newAISshape=new AIS_Shape(newShape);

        ShapeData *newShapeData=item->getShapeData()->copyCreate();
        newShapeData->setShape(newAISshape);
        item->addShapeData(newShapeData);

        // add the new item back to the display and tracking
        ui->drawingWindow->insertItemToMap(item->getShape(),item);
        //ui->drawingWindow->showItem(item);

        reprocess(item);
        drawingChanged=true;
    }

    activeAction=false;
}

void OpenParEMg::finishMoveObject (CustomTreeWidgetItem *item, gp_Pnt p0, gp_Pnt p1)
{
    //std::cout << "OpenParEMg::finishMoveObject" << std::endl; std::cout.flush();

    if (!item) return;

    finishMoveObject(item,p0,p1,false);
    item->resetOperation();

    // find and show the top-level item
    ui->drawingWindow->hideItem(item);
    CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    if (parentItem) {
        while (!parentItem->is_rootDrawing()) {
            item=parentItem;
            parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            if (!parentItem) break;
        }
    }
    ui->drawingWindow->showItem(item);
}

bool OpenParEMg::isValidCopy ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) count++;
        i++;
    }
    if (count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

CustomTreeWidgetItem* OpenParEMg::copyItem (CustomTreeWidgetItem *item, CustomTreeWidgetItem *newItemParent)
{
    //std::cout << "OpenParEMg::copyItem" << std::endl; std::cout.flush();

    if (!item) return nullptr;

    CustomTreeWidgetItem *newItem=item->copyCreate();
    ShapeData *shapeData=newItem->getShapeData();
    shapeData->setCreate();
    newItem->setForeground(0,Qt::black);
    ui->drawingWindow->activateItem(newItem);
    ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);
    newItemParent->addChild(newItem);

    // children for processes
    Process *process=static_cast<Process *>(newItem->getProcess());
    if (process) {
        int i=0;
        while (i < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
            if (child) {
                copyItem(child,newItem);
                ShapeData *shapeData=child->getShapeData();
                shapeData->setCreate();
            }
            i++;
        }
    }

    return newItem;
}

void OpenParEMg::copyDrawingItems ()
{
    //std::cout << "OpenParEMg::copyDrawingItems" << std::endl; std::cout.flush();

    startOperation(true);
    //activeAction=true;  // no need since there is not a cancel option
    itemChangesStack.startNew();
    ui->drawingWindow->set_pickSecondVertex(true);

    // list of items to copy
    std::vector<CustomTreeWidgetItem *> copyList;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            copyList.push_back(item);
        }
        i++;
    }

    // copy
    i=0;
    while (i < copyList.size()) {
        CustomTreeWidgetItem *item=copyList[i];
        if (item && item->is_drawing()) {

            CustomTreeWidgetItem *newItem=copyItem(item,&drawing);

            int i=0;
            while (i < newItem->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)newItem->child(i);
                if (child) itemChangesStack.add(newItem);
                i++;
            }

            itemChangesStack.add(newItem);

            ui->drawingWindow->unselectItem(item);
            ui->drawingWindow->hideItem(newItem);
            ui->drawingWindow->showItem(newItem);
            ui->drawingWindow->selectItem(newItem);
        }
        i++;
    }

    finishOperation(false,6);
}

void PrintActiveSelectionModes(const Handle(AIS_InteractiveContext)& theContext,
                               const Handle(AIS_InteractiveObject)& theObject)
{
    if (theContext.IsNull() || theObject.IsNull()) return;

    TColStd_ListOfInteger activeModes;
    theContext->ActivatedModes(theObject, activeModes);

    std::cout << "Active selection modes for object: ";
    for (TColStd_ListOfInteger::Iterator anIt(activeModes); anIt.More(); anIt.Next())
    {
        std::cout << anIt.Value() << " ";
    }
    std::cout << std::endl;
}

bool OpenParEMg::isValidObjectStretch ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
            if (polywire) count++;
        }
        i++;
    }
    if (count == 1 && count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::stretchObject ()
{
    //std::cout << "OpenParEMg::stretchObject" << std::endl; std::cout.flush();

    startOperation(false);
    activeAction=true;
    itemChangesStack.startNew();
    ui->drawingWindow->set_pickSecondVertex(true);

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Handle(AIS_Shape) shape=item->getShape();
            if (!shape.IsNull()) {

                // set the drawing plane
                currentPrivilegedPlane=ui->drawingWindow->get_gridPlane();
                //restrictToDrawingPlane=true;

                Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
                if (polywire) {
                    item->setEnableStretch(true);
                    item->resetP0P1();
                    gp_Pln plane=polywire->getPlane();
                    ui->drawingWindow->set_gridPlane(plane);
                    itemChangesStack.add(item);
                }
            }
        }
        i++;
    }
}

void OpenParEMg::finishStretchObject (CustomTreeWidgetItem *item)
{
    //std::cout << "OpenParEMg::finishStretchObject" << std::endl; std::cout.flush();

    if (!item) return;

    Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
    if (!polywire) return;
    polywire->deleteRubberband();

    // clone the item onto itself for undo/redo
    ShapeData *newShapeData=item->getShapeData()->copyCreate();
    newShapeData->setEdit();
    item->addShapeData(newShapeData);

    // modify the clone

    polywire=static_cast<Polywire *>(item->getPolywire());
    if (!polywire) return;

    Rectangle *rectangle=dynamic_cast<Rectangle *>(polywire);
    if (rectangle) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
            rectangle->setIsSquare(true);
        } else {
            rectangle->setIsSquare(false);
        }
    }

    item->setEnableStretch(false);
    gp_Pnt pnt=item->getP1();
    polywire->setEditPoint(pnt);
    reprocess(item);
    activeAction=false;

    ui->drawingWindow->set_gridPlane(currentPrivilegedPlane);
    item->resetOperation();
    drawingChanged=true;

    // find and show the top-level item
    ui->drawingWindow->hideItem(item);
    CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    if (parentItem) {
        while (!parentItem->is_rootDrawing()) {
            item=parentItem;
            parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            if (!parentItem) break;
        }
    }
    ui->drawingWindow->showItem(item);

    finishOperation(false,7);
}

bool OpenParEMg::isValidDeletePoint ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
            if (polywire && polywire->canDeletePoint()) count++;
        }
        i++;
    }
    if (count == 1 && count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::deletePoint ()
{
    startOperation(false);
    activeAction=true;
    itemChangesStack.startNew();
    ui->drawingWindow->set_pickSecondVertex(true);

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Handle(AIS_Shape) shape=item->getShape();
            if (!shape.IsNull()) {
                // set the selected shape to be the only selectable shape
                // includes selecting just on vertices of the shape and not midpoints
                //ui->drawingWindow->set_activeShape(shape);

                // set the drawing plane
                currentPrivilegedPlane=ui->drawingWindow->get_gridPlane();
                //restrictToDrawingPlane=true;

                Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
                if (polywire) {
                    item->setEnableDeletePoint(true);
                    item->resetP0P1();
                    gp_Pln plane=polywire->getPlane();
                    ui->drawingWindow->set_gridPlane(plane);
                    itemChangesStack.add(item);
                }
            }
        }
        i++;
    }
}

void OpenParEMg::finishDeletePoint (CustomTreeWidgetItem *item)
{
    if (!item) return;

    // remove the old version from display and tracking
    ui->drawingWindow->hideItem(item);
    ui->drawingWindow->removeItemFromMap(item);
    ui->drawingWindow->deleteShape(item->getShape());  // lose selection

    // clone the item onto itself for undo/redo
    ShapeData *newShapeData=item->getShapeData()->copyCreate();
    newShapeData->setEdit();
    item->addShapeData(newShapeData);

    // add the new item back to the display and tracking
    ui->drawingWindow->insertItemToMap(item->getShape(),item);
    // ui->drawingWindow->showItem(item);

    Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
    if (!polywire) return;

    gp_Pnt p0=item->getP0();
    polywire->deletePoint(p0);
    reprocess(item);
    activeAction=false;
    item->resetOperation();

    ui->drawingWindow->set_gridPlane(currentPrivilegedPlane);

    drawingChanged=true;

    // find and show the top-level item
    ui->drawingWindow->hideItem(item);
    CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    if (parentItem) {
        while (!parentItem->is_rootDrawing()) {
            item=parentItem;
            parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            if (!parentItem) break;
        }
    }
    ui->drawingWindow->showItem(item);

    finishOperation(false,8);
}

void OpenParEMg::cancelDeletePoint ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            item->setEnableDeletePoint(false);
        }
        i++;
    }
    finishOperation(true,4005);
}

bool OpenParEMg::isValidInsertPoint ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
            if (polywire && polywire->canInsertPoint()) count++;
        }
        i++;
    }
    if (count == 1 && count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::insertPoint ()
{
    startOperation(false);
    activeAction=true;
    itemChangesStack.startNew();
    ui->drawingWindow->set_pickFirstVertex(true);

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Handle(AIS_Shape) shape=item->getShape();
            if (!shape.IsNull()) {
                // set the selected shape to be the only selectable shape
                // includes selecting just on vertices of the shape and not midpoints
                //ui->drawingWindow->set_activeShape(shape);

                // set the drawing plane
                currentPrivilegedPlane=ui->drawingWindow->get_gridPlane();
                //restrictToDrawingPlane=true;

                Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
                if (polywire) {

                    // remove the old version from display and tracking
                    ui->drawingWindow->hideItem(item);
                    ui->drawingWindow->removeItemFromMap(item);
                    ui->drawingWindow->deleteShape(item->getShape());  // lose selection

                    // clone the item onto itself for undo/redo
                    ShapeData *newShapeData=item->getShapeData()->copyCreate();
                    newShapeData->setEdit();
                    item->addShapeData(newShapeData);

                    // add the new item back to the display and tracking
                    ui->drawingWindow->insertItemToMap(item->getShape(),item);
                    ui->drawingWindow->showItem(item);

                    // modify the clone

                    polywire=static_cast<Polywire *>(item->getPolywire());
                    item->setEnableInsertPoint(true);
                    item->resetP0P1();
                    gp_Pln plane=polywire->getPlane();
                    ui->drawingWindow->set_gridPlane(plane);
                    itemChangesStack.add(item);
                }
            }
        }
        i++;
    }
}

void OpenParEMg::finishInsertPoint (CustomTreeWidgetItem *item)
{
    if (!item) return;

    Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
    if (polywire) {
        gp_Pnt p0=item->getP0();
        polywire->insertPoint(p0);
        //reprocess(item);
        item->resetOperation();

        ui->drawingWindow->set_gridPlane(currentPrivilegedPlane);

        drawingChanged=true;

        // switch over to stretch behavior
        // set up the first point pick, then let the stretchObject code do the rest for the second pick
        startOperation(false);

        item->setEnableStretch(true);
        ui->drawingWindow->set_pickSecondVertex(true);

        clearTreeSelection();
        ui->drawingWindow->showItem(item);
        ui->drawingWindow->selectItem(item);
        item->setP0(p0);
        polywire->setEditIndex(p0);
        polywire->setCurrentMousePosition(p0);
        polywire->drawStretchRubberband();

        ui->drawingWindow->hideItem(item);
        ui->drawingWindow->updateViewer();

        activeAction=false;
    }
}

bool OpenParEMg::isValidCloseExistingPolyline ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            if (parentItem && parentItem->is_rootDrawing()) {
                Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
                if (polywire && polywire->canClose()) count++;
            }
        }
        i++;
    }
    if (count == 1 && count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::closeExistingPolyline ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
            if (polywire) {

                // for undo/redo

                // activeAction=true;  // no need since there is no cancel option
                itemChangesStack.startNew();

                // remove the old version from display and tracking
                ui->drawingWindow->hideItem(item);
                ui->drawingWindow->removeItemFromMap(item);
                ui->drawingWindow->deleteShape(item->getShape());  // lose selection

                // clone the item onto itself for undo/redo
                ShapeData *newShapeData=item->getShapeData()->copyCreate();
                newShapeData->setEdit();
                item->addShapeData(newShapeData);
                itemChangesStack.add(item);

                // add the new item back to the display and tracking
                ui->drawingWindow->insertItemToMap(item->getShape(),item);
                ui->drawingWindow->showItem(item);

                // modify the clone

                polywire=static_cast<Polywire *>(item->getPolywire());
                polywire->close();
                reprocess(item);
                //item->setText(0,"FACE");
                item->setText(0,polywire->getName(&objectCounts));
                item->getShape()->SetZLayer(Graphic3d_ZLayerId_Top);
                ui->drawingWindow->showItem(item);
                ui->drawingWindow->activateSelectItem(item);
                ui->drawingWindow->updateViewer();
                drawingChanged=true;

                finishOperation(false,10);
            }
        }
        i++;
    }
}

bool OpenParEMg::isValidOpenExistingPolyline ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            if (parentItem && parentItem->is_rootDrawing()) {
                Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
                if (polywire && polywire->canOpen()) count++;
            }
        }
        i++;
    }
    if (count == 1 && count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::openExistingPolyline ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
            if (polywire) {
                // for undo/redo

                //activeAction=true;  // no need since there is no cancel option
                itemChangesStack.startNew();

                // remove the old version from display and tracking
                ui->drawingWindow->hideItem(item);
                ui->drawingWindow->removeItemFromMap(item);
                ui->drawingWindow->deleteShape(item->getShape());  // lose selection

                // clone the item onto itself for undo/redo
                ShapeData *newShapeData=item->getShapeData()->copyCreate();
                newShapeData->setEdit();
                item->addShapeData(newShapeData);
                itemChangesStack.add(item);

                // add the new item back to the display and tracking
                ui->drawingWindow->insertItemToMap(item->getShape(),item);
                ui->drawingWindow->showItem(item);

                // modify the clone

                polywire=static_cast<Polywire *>(item->getPolywire());
                polywire->open();
                reprocess(item);
                //item->setText(0,"WIRE");
                item->setText(0,polywire->getName(&objectCounts));
                item->getShape()->SetZLayer(Graphic3d_ZLayerId_Top);
                ui->drawingWindow->showItem(item);
                ui->drawingWindow->activateSelectItem(item);
                ui->drawingWindow->updateViewer();
                drawingChanged=true;

                finishOperation(false,11);
            }
        }
        i++;
    }
}

bool OpenParEMg::isValidConvertToPolyline ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
            if (polywire && polywire->canConvert()) count++;
        }
        i++;
    }
    if (count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::convertToPolyline ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
            if (polywire) {
                Polyline *newPolyline=polywire->convert();
                delete polywire; polywire=nullptr;
                item->setPolywire(newPolyline);
                reprocess(item);
                item->getShape()->SetZLayer(Graphic3d_ZLayerId_Top);
                ui->drawingWindow->activateSelectItem(item);
                drawingChanged=true;

                // find and show the top-level item
                CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
                while (!parentItem->is_rootDrawing()) {
                    item=parentItem;
                    parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
                }
                ui->drawingWindow->showItem(item);
                ui->drawingWindow->updateViewer();
            }
        }
        i++;
    }
}

bool OpenParEMg::isValidConvertToPath ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
            if (polywire) {
                CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
                if (parentItem && parentItem->is_rootDrawing()) count++;
            }
        }
        i++;
    }
    if (count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::convertToPath ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
            if (polywire) {

                // default path name

                std::string pathName="p";

                int i=1;
                while (boundaryDatabase->pathNameExists(pathName)) {
                    std::string testName=pathName;
                    testName.append("_").append(std::to_string(i));
                    if (boundaryDatabase->pathNameExists(testName)) {i++;}
                    else {pathName=testName; break;}
                }

                // path name placed in a keywordPair
                keywordPair *kwPathName=new keywordPair();
                kwPathName->set_keyword("path");
                kwPathName->set_value(pathName);
                kwPathName->set_lineNumber(0);
                kwPathName->set_loaded(true);

                // path

                Path *newPath=new Path(0,0);
                newPath->set_name(pathName);
                newPath->is_modified();
                newPath->set_normal(polywire->getNormal());
                newPath->addWirePoints(polywire->buildWire());
                newPath->create_item(ui->drawingWindow,&path);  // create item and add as child to path; creates AIS_Shape

                boundaryDatabase->push_path(newPath);

                // add new path to the drawing
                CustomTreeWidgetItem *pathItem=newPath->get_item();
                if (pathItem) {
                    insertToMapActivateItem(pathItem);
                    pathItem->set_itemType(4);
                }

                // see if the path is within an existing port
                Port *port=boundaryDatabase->get_matchingPort(newPath);
                if (port) newPath->set_portItem(port->get_item());

                // delete the old item - assumes that item is a child of drawing with no children
                drawing.removeChild(item);
                ui->drawingWindow->deleteItem(item);
            }
        }
        i++;
    }

    setMenusI(2020);
    ui->drawingWindow->updateViewer();
}

bool OpenParEMg::isValidRotateObject ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) count++;
        i++;
    }
    if (count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::rotateObject ()
{
    std::cout << "OpenParEMg::rotateObject" << std::endl; std::cout.flush();

    startOperation(true);
    activeAction=true;
    itemChangesStack.startNew();

    // set up for undo/redo
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            itemChangesStack.add(item);
        }
        i++;
    }

    if (rotateInputForm) delete rotateInputForm;
    rotateInputForm=new RotateInputForm();
    rotateInputForm->set_drawingWindow(ui->drawingWindow);
    rotateInputForm->set_angle(&angle);
    rotateInputForm->set_startPoint(&startPoint);
    rotateInputForm->set_endPoint(&endPoint);
    rotateInputForm->set_relay(relay);
    rotateInputForm->setModal(false);
    connect(this,&OpenParEMg::sendPnt,rotateInputForm,&RotateInputForm::pickVertexFinished);
    rotateInputForm->show();
}

void OpenParEMg::finishRotateObject (CustomTreeWidgetItem *item)
{
    //std::cout << "OpenParEMg::finishRotateObject" << std::endl; std::cout.flush();

    if (!item) return;

    // remove the old version from display and tracking
    ui->drawingWindow->hideItem(item);
    ui->drawingWindow->removeItemFromMap(item);
    ui->drawingWindow->deleteShape(item->getShape());  // lose selection

    // clone the item onto itself for undo/redo
    ShapeData *newShapeData=item->getShapeData()->copyCreate();
    newShapeData->setEdit();
    item->addShapeData(newShapeData);
    activeAction=false;

    // add the new item back to the display and tracking
    // ui->drawingWindow->insertItemToMap(item->getShape(),item);
    // ui->drawingWindow->showItem(item);

    // modify the clone

    Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
    if (polywire) {
        polywire->rotate(angle,startPoint,endPoint);
        reprocess(item);
        drawingChanged=true;
    }

    Process *process=static_cast<Process *>(item->getProcess());
    if (process) {

        // ui->drawingWindow->hideItem(item);
        // ui->drawingWindow->removeItemFromMap(item);
        // ui->drawingWindow->deleteShape(item->getShape());

        int i=0;
        while (i < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
            finishRotateObject(child);
            i++;
        }
    }

    if (!polywire && !process) {
        TopoDS_Shape newShape=item->rotateShape(angle,startPoint,endPoint,ui->drawingWindow->get_viewerContext());
        Handle(AIS_Shape) newAISshape=new AIS_Shape(newShape);

        // ui->drawingWindow->hideItem(item);
        // ui->drawingWindow->removeItemFromMap(item);
        // ui->drawingWindow->deleteShape(item->getShape());

        ShapeData *newShapeData=item->getShapeData()->copyCreate();
        newShapeData->setShape(newAISshape);
        item->addShapeData(newShapeData);

        // add the new item back to the display and tracking
        ui->drawingWindow->insertItemToMap(item->getShape(),item);
        ui->drawingWindow->showItem(item);

        reprocess(item);
        drawingChanged=true;
    }
}

void OpenParEMg::finishRotateObject ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item) {
            finishRotateObject(item);

            item->resetOperation();

            // find and show the top-level item
            ui->drawingWindow->hideItem(item);
            CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            if (parentItem) {
                while (!parentItem->is_rootDrawing()) {
                    item=parentItem;
                    parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
                    if (!parentItem) break;
                }
            }
            ui->drawingWindow->showItem(item);
        }
        i++;
    }
    if (rotateInputForm) {rotateInputForm=nullptr;}
    finishOperation(false,9);
}

void OpenParEMg::createPort ()
{
    //std::cout << "OpenParEMg::createPort" << std::endl; std::cout.flush();

    int count=0;
    while (count < ui->drawingWindow->NbSelected()) {
        TopoDS_Shape selectedShape=ui->drawingWindow->get_selectedSubshape(count);
        if (!selectedShape.IsNull()) {
            if (selectedShape.ShapeType() == TopAbs_FACE) {

                // next available s-port number
                int sport=boundaryDatabase->get_SportCount()+1;

                // default port name

                std::string portName="port";
                portName.append(std::to_string(sport));

                int i=1;
                while (boundaryDatabase->portNameExists(portName)) {
                    std::string testName=portName;
                    testName.append("_").append(std::to_string(i));
                    if (boundaryDatabase->portNameExists(testName)) {i++;}
                    else {portName=testName; break;}
                }

                // default net name

                std::string netName="net";
                netName.append(std::to_string(sport));

                i=1;
                while (boundaryDatabase->netNameExists(netName)) {
                    std::string testName=netName;
                    testName.append("_").append(std::to_string(i));
                    if (boundaryDatabase->netNameExists(testName)) {i++;}
                    else {netName=testName; break;}
                }

                // default path name

                std::string pathName="port";
                pathName.append(std::to_string(sport));

                i=1;
                while (boundaryDatabase->pathNameExists(pathName)) {
                    std::string testName=pathName;
                    testName.append("_").append(std::to_string(i));
                    if (boundaryDatabase->pathNameExists(testName)) {i++;}
                    else {pathName=testName; break;}
                }

                // path name placed in a keywordPair
                keywordPair *kwPathName=new keywordPair();
                kwPathName->set_keyword("path");
                kwPathName->set_value(pathName);
                kwPathName->set_lineNumber(0);
                kwPathName->set_loaded(true);

                // path

                Path *newPath=new Path(0,0);
                newPath->set_name(pathName);
                newPath->is_modified();
                newPath->addFacePoints(TopoDS::Face(selectedShape));
                newPath->create_item(ui->drawingWindow,&path);  // create item and add as child to path; creates AIS_Shape

                boundaryDatabase->push_path(newPath);

                // add new path to the drawing
                CustomTreeWidgetItem *item=newPath->get_item();
                if (item) insertToMapActivateItem(item);

                // port

                Port *newPort=new Port(0,0);
                newPort->set_name(portName);
                newPort->set_outline(newPath);

                // path info
                newPort->push_path(kwPathName,boundaryDatabase->get_pathList_size()-1,false);

                // impedance
                if (boundaryDatabase->get_portList_size() == 0) {
                    newPort->set_impedance_definition("PV");
                    newPort->set_impedance_calculation("line");
                } else {
                    newPort->set_impedance_definition(boundaryDatabase->get_port(boundaryDatabase->get_portList_size()-1)->get_impedance_definition());
                    newPort->set_impedance_calculation(boundaryDatabase->get_port(boundaryDatabase->get_portList_size()-1)->get_impedance_calculation());
                }

                // must have at least one mode per port - default to sensible assumptions
                Mode *newMode=new Mode(0,0,newPort->get_impedance_calculation());
                newMode->set_net(netName);
                newMode->set_Sport(sport);
                newPort->push_mode(newMode);

                // add to boundary database
                boundaryDatabase->push_port(newPort);

                // draw it
                boundaryDatabase->draw_port(relay,newPort,&projData,ui->drawingWindow,ui->drawingItemTree,&path,&port,&boundary,materialDatabase);
            }
        }
        count++;
    }

    ui->drawingWindow->setSubshapeSelection(false);
    on_actionShape_triggered();
    setMenusI(37);
    ui->drawingWindow->updateViewer();
}

// void OpenParEMg::showDisplayShape (CustomTreeWidgetItem *item)
// {
//     std::cout << "OpenParEMg::showDisplayShape" << std::endl; std::cout.flush();
//     ui->drawingWindow->showItem(item);
// }

// void OpenParEMg::hideDisplayShape (CustomTreeWidgetItem *item)
// {
//     std::cout << "OpenParEMg::hideDisplayShape" << std::endl; std::cout.flush();
//     ui->drawingWindow->hideItem(item);
// }

// void OpenParEMg::selectDisplayShape (CustomTreeWidgetItem *item)
// {
//     std::cout << "OpenParEMg::selectDisplayShape" << std::endl; std::cout.flush();
//     ui->drawingWindow->hideItem(item);
// }

// void OpenParEMg::unselectDisplayShape (CustomTreeWidgetItem *item)
// {
//     std::cout << "OpenParEMg::unselectDisplayShape" << std::endl; std::cout.flush();
//     ui->drawingWindow->unselectItem(item);
// }

// void OpenParEMg::showPortShape (CustomTreeWidgetItem *item)
// {
//     std::cout << "OpenParEMg::showPortShape" << std::endl; std::cout.flush();
//     ui->drawingWindow->showItem(item);
// }

// void OpenParEMg::hidePortShape (CustomTreeWidgetItem *item)
// {
//     std::cout << "OpenParEMg::hidePortShape" << std::endl; std::cout.flush();
//     ui->drawingWindow->hideItem(item);
// }

// recursive
// void OpenParEMg::set_selectionMode (CustomTreeWidgetItem *item, int selectionMode)
// {
//     //item->set_selectionMode(selectionMode);

//     int i=0;
//     while (i < item->childCount()) {
//         CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
//         set_selectionMode(child,selectionMode);
//         i++;
//     }
// }

// recursive
// void OpenParEMg::set_displayMode (CustomTreeWidgetItem *item, int displayMode)
// {
//     //item->set_displayMode(displayMode);

//     int i=0;
//     while (i < item->childCount()) {
//         CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
//         set_displayMode(child,displayMode);
//         i++;
//     }
// }

void OpenParEMg::resetDimTag (CustomTreeWidgetItem *item)
{
    item->set_dimTag(-1,-1);

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        resetDimTag(child);
        i++;
    }
}

void OpenParEMg::renumberDimTag ()
{
    double count=1;
    int i=0;
    while (i < drawing.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)drawing.child(i);

        // SOLID
        if (child->getShape()->Shape().ShapeType() == TopAbs_SOLID) {
            child->set_dimTag(3,count);
            count++;
        }

        // COMPOUND
        if (child->getShape()->Shape().ShapeType() == TopAbs_COMPOUND) {
            // make sure it is not a polywire (a polycircle is a COMPOUND with a center point added)
            if (!child->getPolywire()) {
                child->set_dimTag(3,count);
                count++;
            }
        }

        i++;
    }
}

void OpenParEMg::setPhysicalGroups ()
{
    //std::cout << "OpenParEMg::setPhysicalGroups" << std::endl; std::cout.flush();

    // re-build the physical groups list

    clear_physicalGroupMaterials (&projData);

    // check the first-level children for SOLID or COMPOUND
    int i=0;
    while (i < drawing.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)drawing.child(i);

        // SOLID
        if (child->getShape()->Shape().ShapeType() == TopAbs_SOLID) {
            QString itemMaterial=child->text(1);
            char *material=nullptr;
            cstrFromQString (&material,itemMaterial);
            add_physicalGroupMaterial(&projData,-1,child->get_dimTag().first,child->get_dimTag().second,material);
            if (material) {free(material);}
        }

        // COMPOUND
        if (child->getShape()->Shape().ShapeType() == TopAbs_COMPOUND) {
            // make sure it is not a polywire (a polycircle is a COMPOUND with a center point added)
            if (!child->getPolywire()) {
                QString itemMaterial=child->text(1);
                char *material=nullptr;
                cstrFromQString (&material,itemMaterial);
                add_physicalGroupMaterial(&projData,-1,child->get_dimTag().first,child->get_dimTag().second,material);
                if (material) {free(material);}
            }
        }

        i++;
    }

    // assign to mesh
    i=0;
    while (i < projData.physicalGroupMaterialCount) {
        std::vector<int> physicalGroupList;
        physicalGroupList.push_back(0);
        physicalGroupList[0]=projData.physicalGroupMaterials[i].tag;

        // std::cout << "OpenParEMg::setPhysicalGroups:  dim=" << projData.physicalGroupMaterials[i].dim
        //           << "  tag=" << projData.physicalGroupMaterials[i].tag
        //           << "  materialName=" << projData.physicalGroupMaterials[i].materialName << std::endl; std::cout.flush();

        // uniquify the physical group name with the group tag to avoid gmsh eliminating
        // physical groups with duplicated names from the $PhysicalNames/$EndPhysicalNames block in the msh file
        std::string groupName=projData.physicalGroupMaterials[i].materialName;
        groupName.append("_OPEM_RESERVED_");
        groupName.append(std::to_string(projData.physicalGroupMaterials[i].tag));

        gmsh::model::addPhysicalGroup(projData.physicalGroupMaterials[i].dim,physicalGroupList,-1,groupName.c_str());
        i++;
    }
}

void OpenParEMg::setMaterials ()
{
    int i=0;
    while (i < drawing.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) drawing.child(i);

        bool processMaterial=false;
        // SOLID
        if (child->getShape()->Shape().ShapeType() == TopAbs_SOLID) processMaterial=true;

        // COMPOUND
        if (child->getShape()->Shape().ShapeType() == TopAbs_COMPOUND) {
            // make sure it is not a polywire (a polycircle is a COMPOUND with a center point added)
            if (!child->getPolywire()) processMaterial=true;
        }

        // set materials
        if (processMaterial) {
            int j=0;
            while (j < projData.physicalGroupMaterialCount) {
                if (projData.physicalGroupMaterials[j].tag == child->get_dimTag().second) {
                    child->setText(1,projData.physicalGroupMaterials[j].materialName);
                    break;
                }
                j++;
            }
        }

        i++;
    }
}

void OpenParEMg::assignMaterial ()
{
    // Cannot assign material to existing mesh
    if (mesh.childCount() > 0) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,"OpenParEMg","Materials cannot be assigned to an existing mesh.  Do you want to delete the mesh?",QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
        deleteMesh(true);
        meshChanged=false;
    }

    MaterialSelection *materialSelection=new MaterialSelection();
    materialSelection->set_materialDatabase(materialDatabase);
    materialSelection->set_selectedMaterial(&selectedMaterial);
    materialSelection->populate();
    materialSelection->exec();
    delete materialSelection;

    if (selectedMaterial != "") {
        clickedItem->set_Material(selectedMaterial);
        clickedItem->setText(1,selectedMaterial);
        projectChanged=true;
        setMenusI(38);
    }
}

void OpenParEMg::dumpDrawingEntities ()
{
    long unsigned int i=0;
    while (i < drawingEntities.size()) {
        std::pair<int,int> pair=drawingEntities[i];
        std::string type,name;
        gmsh::model::getEntityType(pair.first,pair.second,type);
        gmsh::model::getEntityName(pair.first,pair.second,name);
        //std::cout << "pari=" << "<" << pair.first << "," << pair.second << ">" << "  type=" << type << "  name=" << name << std::endl;

        if (pair.first == 3) {
            std::cout << "pari=" << "<" << pair.first << "," << pair.second << ">" << "  type=" << type << "  name=" << name << std::endl;
            std::vector<int> integers;
            std::vector<double> reals;
            gmsh::model::getEntityProperties(pair.first,pair.second,integers,reals);

            std::cout << "   integer.size()=" << integers.size() << std::endl;
            long unsigned int i=0;
            while (i < integers.size()) {
                std::cout << "   integer=" << integers[i] << std::endl;
                i++;
            }

            std::cout << "   reals.size()=" << reals.size() << std::endl;
            i=0;
            while (i < reals.size()) {
                std::cout << "   real=" << reals[i] << std::endl;
                i++;
            }
        }
        i++;
    }
}

void OpenParEMg::on_actionOpen_triggered ()
{
    QString testProjectFile=QFileDialog::getOpenFileName(this,tr("Open Project"), "", tr("Project Files (*.proj);;All Files (*)"),
                                                         nullptr,QFileDialog::DontUseNativeDialog);

    // return if user cancels
    if (testProjectFile.isNull()) return;

    // reset as new
    on_actionNew_triggered ();

    // break up the full path
    QFileInfo fileInfo(testProjectFile);
    absolutePath=fileInfo.absolutePath();
    projectFile=fileInfo.fileName();
    projectName=fileInfo.completeBaseName();

    QDir::setCurrent(absolutePath);

    QString currentPath;
    currentPath=QDir::currentPath();

    // load the file
    if (QFile::exists(projectFile)) {

        // ignore errors so that users can save incomplete project for safety and later work
        load_project_file (projectFile.toStdString().c_str(),&projData,"   ");

        // stress test to look for leaks in loading/freeing projData; run while monitoring memory consumption with top
        // std::cout << "starting memory test" << std::endl; std::cout.flush();
        // int i=0;
        // while (i < 1000000) {
        //     free_project(&projData);
        //     init_project (&projData);
        //     load_project_file (projectName.toStdString().c_str(),&projData,"   ");
        //     std::cout << "i=" << i << std::endl; std::cout.flush();
        //     i++;
        // }
        // exit(1);

        // load materials
        bool materialsLoaded=false;
        if (strcmp(projData.materials_global_name,"") != 0 || strcmp(projData.materials_local_name,"") != 0) {
            if (materialDatabase->load_materials(projData.materials_global_path,projData.materials_global_name,
                                                 projData.materials_local_path,projData.materials_local_name,
                                                 projData.materials_check_limits)) {
                QMessageBox mb;
                mb.critical(nullptr, "Error", "Unable to load the specified materials files.");
                mb.setFixedSize(500, 200);
            } else materialsLoaded=true;
        }

        // // load brep file, if defined
        // bool brepLoaded=false;
        // if (strcmp(projData.gui_brep_file,"") != 0) {

        //     QString filePath=projData.gui_brep_file;

        //     if (loadBrepFile(filePath,false)) {
        //         QString message="Unable to load Brep file \"";
        //         message.append(filePath);
        //         message.append("\".");
        //         QMessageBox mb;
        //         mb.critical(nullptr, "Error",message);
        //         mb.setFixedSize(500, 200);
        //     } else brepLoaded=true;
        // }

        // load boundaries, if any, and draw
        if (strcmp(projData.port_definition_file,"") != 0) {
            if (boundaryDatabase->load(projData.port_definition_file,projData.solution_check_closed_loop)) {
                QMessageBox mb;
                mb.critical(nullptr, "Info", "Boundaries require additional setup.");
                mb.setFixedSize(500, 200);
            }

            // continue despite the errors

            boundaryDatabase->set_unmodified();
            boundaryDatabase->assignPathNormals();  // to correctly orient arrow heads
            // ToDo: rename draw since this does not actually draw
            boundaryDatabase->draw(relay,&projData,ui->drawingWindow,ui->drawingItemTree,&path,&port,&boundary,materialDatabase);

            // add paths to the tree
            long unsigned int i=0;
            while (i < boundaryDatabase->get_pathList_size()) {
                Path *path=boundaryDatabase->get_path(i);
                CustomTreeWidgetItem *item=path->get_item();
                if (item) insertToMapActivateItem(item);
                i++;
            }
        }

        // load mesh, if any, and draw
        if (strcmp(projData.mesh_file,"") != 0) {
            loadMeshFile(QString::fromStdString(projData.mesh_file));
        }

        // load file
        bool drawingLoaded=false;
        if (loadDrawingFile()) {
        } else {
            drawingLoaded=true;
        }

        // set dimTag and material
        if (drawingLoaded && materialsLoaded) {
            //resetDimTag(&drawing);
            renumberDimTag();
            setMaterials();
        }


        ui->drawingWindow->fitAll();
        ui->drawingWindow->updateViewer();

        projData.modified=0;
        projectFileLoaded=true;
        projectChanged=false;
    } else {
        // should not occur
        QMessageBox mb;
        mb.critical(nullptr, "Error", "The requested project file does not exist.");
        mb.setFixedSize(500, 200);
    }

    // ensure the ports and boundaries are only defined by single paths.  This is a restriction for a safer GUI.
    if (boundaryDatabase->has_complex_path()) {
        QMessageBox mb;
        mb.critical(nullptr, "Warning", "One or more ports or boundaries have a definition using more than one path.");
        mb.setFixedSize(500, 200);
    }

    on_actionShape_triggered();  // ToDo: see if this is still required
    clearTreeSelection();
    setMenusI(39);
}

void OpenParEMg::resetLockouts ()
{
    disableMenus=false;
    projectFileLoaded=false;
    projectChanged=false;
    meshChanged=false;
    drawingChanged=false;
    // brepFileLoaded=false;
    // brepChanged=false;
    drawingPlaneShown=false;
    simulationRunning=false;
    simulationStopping=false;
    simulationAborting=false;
    setMenusI(40);
}

void OpenParEMg::printLockouts ()
{
    std::cout << "Lockouts:" << std::endl
              << "   disableMenus=" << disableMenus << std::endl
              << "   projectFileLoaded=" << projectFileLoaded << std::endl
              << "   projectChanged=" << projectChanged << std::endl
              << "   boundaryDatabaseChanged=" << boundaryDatabase->is_modified() << std::endl
              << "   meshChanged=" << meshChanged << std::endl
              << "   drawingChanged=" << drawingChanged << std::endl
              << "   drawingPlaneShown=" << drawingPlaneShown << std::endl
              << "   simulationRunning=" << simulationRunning << std::endl
              << "   simulationStopping=" << simulationStopping << std::endl
              << "   simulationAborting=" << simulationAborting << std::endl;
}

void OpenParEMg::resetDrawing ()
{
    std::cout << "OpenParEMg::resetDrawing" << std::endl; std::cout.flush();

    // mesh
    deleteMesh(false);

    // reset drawing window
    ui->drawingWindow->clearDrawing();
    ui->drawingWindow->updateViewer();

    // selection tree
    drawing.reset();

    // drawing is always a COMPOUND
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    Handle(AIS_Shape) newShape=new AIS_Shape(compound);
    ShapeData *newShapeData=new ShapeData(1,nullptr,nullptr,newShape);
    drawing.addShapeData(newShapeData);

    //reset the tracking
    ui->drawingWindow->reset();

    // undo/redo
    itemChangesStack.clear();

    // for uniquifying names
    objectCounts.reset();

    // selection
    clickedItem=nullptr;
    previousClickedItem=nullptr;
    workingItem=nullptr;

    drawingChanged=true;

    on_actionShape_triggered();
    std::cout << "exit OpenParEMg::resetDrawing" << std::endl; std::cout.flush();
}

void OpenParEMg::resetProject ()
{
    std::cout << "OpenParEMg::resetProject" << std::endl; std::cout.flush();

    if (!projectFileLoaded) return;

    // drawing plane
    if (drawingPlaneShown) on_actionDrawingPlaneHide_triggered();
    on_actionDrawingSetPlaneToXY_triggered();

    // project file
    projectFile="";
    free_project(&defaultData);
    free_project(&projData);

    resetDrawing();

    // reset material database
    if (materialDatabase) delete materialDatabase;
    materialDatabase=new MaterialDatabase();

    // reset boundary database
    if (boundaryDatabase) delete boundaryDatabase;
    boundaryDatabase=new BoundaryDatabase();

    // reset selection tree
    //drawing.reset();
    path.reset();
    port.reset();
    boundary.reset();
    mesh.reset();

    resetLockouts();

    setMenusI(41);
    std::cout << "exit OpenParEMg::resetProject" << std::endl; std::cout.flush();
}

void OpenParEMg::on_actionNew_triggered ()
{
    resetProject();
    init_project (&defaultData);
    init_project (&projData);
    projData.modified=0;
    projectFileLoaded=true;
    projectChanged=true;
    setMenusI(42);
}

void OpenParEMg::on_actionClose_triggered()
{
    //std::cout << "OpenParEMg::on_actionClose_triggered" << std::endl; std::cout.flush();

    if (projectChanged || meshChanged || boundaryDatabase->is_modified()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,"OpenParEMg","There are unsaved changes.  Do you want to close anyway?",QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No) return;
    }

    resetProject();
    setMenusI(43);
}

void OpenParEMg::on_actionMeshOptions_triggered ()
{
    MeshDialog *meshDialog=new MeshDialog();
    meshDialog->set_simulationRunning(simulationRunning);
    meshDialog->set_projData(&projData);
    meshDialog->exec();
    delete meshDialog;

    if (projData.modified) {
        projectChanged=true;
    }
    setMenusI(44);
}

void OpenParEMg::on_actionSimulateOptions_triggered ()
{
    SimOptions *simOptions=new SimOptions();
    simOptions->set_simulationRunning(simulationRunning);
    simOptions->set_projData(&projData);
    simOptions->exec();
    delete simOptions;

    if (projData.modified) {
        projectChanged=true;
    }
    setMenusI(45);
}

void OpenParEMg::on_actionAbout_triggered ()
{
    About *about=new About();
    about->exec();
    delete about;
}

void OpenParEMg::on_actionLicense_triggered ()
{
    License *license=new License();
    license->exec();
    delete license;
}

void OpenParEMg::on_actionFrequencyPlan_triggered ()
{
    FrequencyPlanG *frequencyPlan=new FrequencyPlanG();
    frequencyPlan->set_simulationRunning(simulationRunning);
    frequencyPlan->set_projData(&projData);
    frequencyPlan->exec();
    delete frequencyPlan;

    if (strcmp(projData.refinement_frequency,"none") == 0) ui->actionRefinement->setEnabled(false);
    else ui->actionRefinement->setEnabled(true);

    if (projData.modified) {
        projectChanged=true;
    }
    setMenusI(46);
}

void OpenParEMg::saveProject ()
{
    // update included file names

    // port_definition_file
    if (boundaryDatabase->is_modified() && strcmp(projData.port_definition_file,"") == 0) {
        QString portDefinitionFile=projectName;
        portDefinitionFile.append("_ports.txt");
        cstrFromQString (&(projData.port_definition_file),portDefinitionFile);
        projectChanged=true;
    }

    // mesh_file
    if (mesh.childCount() > 0 && strcmp(projData.mesh_file,"") == 0 ) {
        QString meshFile=projectName;
        meshFile.append(".msh");
        cstrFromQString (&(projData.mesh_file),meshFile);
        projectChanged=true;
    }

    // ToDo: remove gui brep file from projData

    // save files

    // project
    if (projectChanged) {
        std::cout << "Saved project file" << std::endl; std::cout.flush();
        if (save_project (projectFile.toStdString().c_str(),&projData,&defaultData,"")) {
            QString message="Error in saving the project file.";
            QMessageBox mb;
            mb.critical(nullptr,"Error",message);
            mb.setFixedSize(500, 200);
        }
        projData.modified=0;
        projectChanged=false;
    }

    // ports and boundaries
    if (boundaryDatabase->is_modified()) {
        std::cout << "Saved boundary database file" << std::endl; std::cout.flush();
        if (saveBoundaryDatabase()) {
            QString message="Error in saving the boundary database.";
            QMessageBox mb;
            mb.critical(nullptr, "Error",message);
            mb.setFixedSize(500, 200);
        }
    }

    // mesh
    if (mesh.childCount() > 0 && meshChanged) {
        std::cout << "Saved mesh file" << std::endl; std::cout.flush();
        on_actionMeshSave_triggered();
        meshChanged=false;
    }

    // drawing
    if (drawingChanged) {
        std::cout << "Saved drawing file" << std::endl; std::cout.flush();
        QString drawingFile=projectName;
        drawingFile.append(".opd");
        saveDrawingFile(drawingFile);
    }

    setMenusI(47);
}

void OpenParEMg::on_actionSave_triggered ()
{
    std::cout << "projectFile=" << projectFile.toStdString() << std::endl; std::cout.flush();
    if (QFile::exists(projectFile)) {
        std::cout << "file exists" << std::endl; std::cout.flush();
        saveProject();
    } else {
        std::cout << "file does not exist" << std::endl; std::cout.flush();
        on_actionSaveAs_triggered();
    }
}

void OpenParEMg::on_actionSaveAs_triggered ()
{
    QString filePath=QFileDialog::getSaveFileName(this, tr("Save Project"), absolutePath, tr("Project Files (*.proj)","All Files (*)"),
                                                  nullptr,QFileDialog::DontUseNativeDialog);
    if (filePath.isEmpty()) return;

    QFileInfo fileInfo(filePath);
    absolutePath=fileInfo.absolutePath();
    projectFile=fileInfo.fileName();
    projectName=fileInfo.completeBaseName();
    set_project_name(&projData,projectName.toStdString().c_str());

    QDir::setCurrent(absolutePath);

    saveProject();
}

void OpenParEMg::on_actionRefinement_triggered ()
{
    OPEMg_Refinement *refinement=new OPEMg_Refinement();
    refinement->set_simulationRunning(simulationRunning);
    refinement->set_projData(&projData);
    refinement->exec();
    delete refinement;

    if (projData.modified) {
        projectChanged=true;
    }
    setMenusI(48);
}

void OpenParEMg::on_actionMaterialsEditor_triggered ()
{
    Materials *localMaterials=new Materials();
    localMaterials->exec();
    delete localMaterials;

    if (projData.modified) {
        projectChanged=true;
    }
    setMenusI(49);
}


int CountSubShapes(const TopoDS_Shape& shape, TopAbs_ShapeEnum type)
{
    int count = 0;

    for (TopExp_Explorer exp(shape, type); exp.More(); exp.Next()) {
        ++count;
    }

    return count;
}

void ListChildren (const TopoDS_Shape& theShape)
{
    std::cout << TopAbs::ShapeTypeToString(theShape.ShapeType()) << std::endl;

    // 3 levels of children
    TopoDS_Iterator anIterator(theShape);
    for (; anIterator.More(); anIterator.Next()) {
        const TopoDS_Shape& aChildShape = anIterator.Value();
        std::cout << "   " << TopAbs::ShapeTypeToString(aChildShape.ShapeType()) << std::endl;

        TopoDS_Iterator anIterator2(aChildShape);
        for (; anIterator2.More(); anIterator2.Next()) {
            const TopoDS_Shape& aChildShape2 = anIterator2.Value();
            std::cout << "      " << TopAbs::ShapeTypeToString(aChildShape2.ShapeType()) << std::endl;

            TopoDS_Iterator anIterator3(aChildShape2);
            for (; anIterator3.More(); anIterator3.Next()) {
                const TopoDS_Shape& aChildShape3 = anIterator3.Value();
                std::cout << "         " << TopAbs::ShapeTypeToString(aChildShape3.ShapeType()) << std::endl;
            }
        }
    }
    // std::cout << std::endl;

    // std::cout << "Faces using TopExp_Explorer:" << std::endl;
    // for (TopExp_Explorer anExplorer(theShape, TopAbs_FACE); anExplorer.More(); anExplorer.Next()) {
    //     const TopoDS_Face& aFace = TopoDS::Face(anExplorer.Current());
    //     std::cout << "  Found a Face" << std::endl;
    // }

    // std::cout << "Solid using TopExp_Explorer:" << std::endl;
    // for (TopExp_Explorer anExplorer(theShape, TopAbs_SOLID); anExplorer.More(); anExplorer.Next()) {
    //     const TopoDS_Solid& aSolid = TopoDS::Solid(anExplorer.Current());
    //     std::cout << "  Found a Solid" << std::endl;
    // }

    // std::cout << "Wire using TopExp_Explorer:" << std::endl;
    // for (TopExp_Explorer anExplorer(theShape, TopAbs_WIRE); anExplorer.More(); anExplorer.Next()) {
    //     const TopoDS_Wire& aWire = TopoDS::Wire(anExplorer.Current());
    //     std::cout << "  Found a Wire" << std::endl;
    // }

    // std::cout << "CompSolid using TopExp_Explorer:" << std::endl;
    // for (TopExp_Explorer anExplorer(theShape, TopAbs_COMPSOLID); anExplorer.More(); anExplorer.Next()) {
    //     const TopoDS_CompSolid& aCompSolid = TopoDS::CompSolid(anExplorer.Current());
    //     std::cout << "  Found a CompSolid" << std::endl;
    // }

    // std::cout << "Compound using TopExp_Explorer:" << std::endl;
    // for (TopExp_Explorer anExplorer(theShape, TopAbs_COMPOUND); anExplorer.More(); anExplorer.Next()) {
    //     const TopoDS_Compound& aCompound = TopoDS::Compound(anExplorer.Current());
    //     std::cout << "  Found a Compound" << std::endl;
    // }

    // std::cout << "Edge using TopExp_Explorer:" << std::endl;
    // for (TopExp_Explorer anExplorer(theShape, TopAbs_EDGE); anExplorer.More(); anExplorer.Next()) {
    //     const TopoDS_Edge& aEdge = TopoDS::Edge(anExplorer.Current());
    //     std::cout << "  Found a Edge" << std::endl;
    // }

    // std::cout << "Shell using TopExp_Explorer:" << std::endl;
    // for (TopExp_Explorer anExplorer(theShape, TopAbs_SHELL); anExplorer.More(); anExplorer.Next()) {
    //     const TopoDS_Shell& aShell = TopoDS::Shell(anExplorer.Current());
    //     std::cout << "  Found a Shell" << std::endl;
    // }

    // std::cout << "Vertex using TopExp_Explorer:" << std::endl;
    // for (TopExp_Explorer anExplorer(theShape, TopAbs_VERTEX); anExplorer.More(); anExplorer.Next()) {
    //     const TopoDS_Vertex& aVertex = TopoDS::Vertex(anExplorer.Current());
    //     std::cout << "  Found a Vertex" << std::endl;
    // }
}

void OpenParEMg::shapeCount (TopoDS_Shape shape, int *count)
{
    TopoDS_Iterator topoIterator(shape);

    while (topoIterator.More()) {
        const TopoDS_Shape& child=topoIterator.Value();
        (*count)++;
        shapeCount(child,count);
        topoIterator.Next();
    }
}

void OpenParEMg::addRootDisplayShapeCreate (TopoDS_Shape shape)
{
    if (shape.IsNull()) return;

    // ToDo: delete this?
    // ensure the root item is a compound
    if (shape.ShapeType() != TopAbs_COMPOUND) {
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        builder.Add(compound,shape);
        shape=compound;
    }

    // drawing item
    Handle(AIS_Shape) drawingShape=new AIS_Shape(shape);
    ui->drawingWindow->insertItemToMap(drawingShape,&drawing);
    drawing.setShape(drawingShape);

    // pick off solids that are included in the compound and add below drawing
    TopoDS_Iterator topoIterator(shape);
    while (topoIterator.More()) {
        const TopoDS_Shape& child=topoIterator.Value();
        TopAbs_ShapeEnum shapeType=child.ShapeType();
        // TopAbs_COMPSOLID may not be needed
        if (shapeType == TopAbs_COMPSOLID || shapeType == TopAbs_SOLID) {
            CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem(0);
            Handle(AIS_Shape) drawingShape=new AIS_Shape(child);
            ui->drawingWindow->insertItemToMap(drawingShape,newItem);
            newItem->setShape(drawingShape);
            if (shapeType == TopAbs_COMPSOLID) {
                objectCounts.compsolid++;
                QString name="COMPSOLID";
                name.append(QString::number(objectCounts.compsolid));
                newItem->setText(0,name);
            } else if (shapeType == TopAbs_SOLID) {
                objectCounts.solid++;
                QString name="SOLID";
                name.append(QString::number(objectCounts.solid));
                newItem->setText(0,name);
            }
            newItem->set_itemType(0);  // a drawing item
            newItem->setForeground(0,Qt::gray);
            newItem->setPolywire(nullptr);
            //newItem->set_dimTag(dimTag);
            drawing.addChild(newItem);
        }
        topoIterator.Next();
    }

    drawing.setForeground(0,Qt::gray);
    ui->drawingWindow->showItem(&drawing);
    ui->drawingWindow->unselectItem(&drawing);
}

void OpenParEMg::insertToMapActivateItem (CustomTreeWidgetItem *item)
{
    //std::cout << "OpenParEMg::addItemWithShape" << std::endl; std::cout.flush();

    if (!item) return;

    Handle(AIS_Shape) drawingShape=item->getShape();
    ui->drawingWindow->insertItemToMap(drawingShape,item);

    long unsigned int i=0;
    while (i < item->getArrowHeadsSize()) {
        ui->drawingWindow->insertItemToMap(item->getArrowHead(i),item);
        i++;
    }

    item->setForeground(0,Qt::gray);
    ui->drawingWindow->showItem(item);
    ui->drawingWindow->activateSelectItem(item);
}

TopoDS_Shape NormalizeCompound (const TopoDS_Shape& shape, BRep_Builder& builder)
{
    if (shape.IsNull()) return shape;

    if (shape.ShapeType() != TopAbs_COMPOUND) return shape;

    // Count children
    TopoDS_Iterator it(shape);
    if (!it.More()) return shape; // empty compound, keep it

    TopoDS_Shape first=it.Value();
    it.Next();

    // Single child → unwrap
    if (!it.More()) return NormalizeCompound(first,builder);

    // Multiple children → rebuild
    TopoDS_Compound result;
    builder.MakeCompound(result);

    for (TopoDS_Iterator it2(shape); it2.More(); it2.Next()) {
        TopoDS_Shape normalizedChild=NormalizeCompound(it2.Value(), builder);
        builder.Add(result, normalizedChild);
    }

    return result;
}

bool OpenParEMg::loadBrepFile (QString filePath, bool createName)
{
    bool retval=false;
    if (filePath.isEmpty()) {
        retval=true;
    } else {
        TopoDS_Shape s;
        BRep_Builder b;
        if (BRepTools::Read(s,filePath.toStdString().c_str(),b)) {

            BRep_Builder builder;
            TopoDS_Shape normalized=NormalizeCompound(s,builder);
            addRootDisplayShapeCreate(normalized);
            drawingChanged=false;

            showRootDrawingItems();

        } else retval=true;
    }
    setMenusI(50);
    return retval;
}

bool OpenParEMg::loadStepFile (QString filePath, bool createName)
{
    bool retval=false;
    if (filePath.isEmpty()) {
        retval=true;
    } else {
        STEPControl_Reader reader;
        IFSelect_ReturnStatus status=reader.ReadFile(filePath.toStdString().c_str());
        if (status == IFSelect_RetDone) {
            reader.TransferRoots();
            TopoDS_Shape s=reader.OneShape();

            BRep_Builder builder;
            TopoDS_Shape normalized=NormalizeCompound(s,builder);
            addRootDisplayShapeCreate(normalized);
            drawingChanged=true;

            if (createName) {
                QFileInfo fileInfo(filePath);
                QString brepName=fileInfo.fileName();
                cstrFromQString (&(projData.gui_brep_file),brepName);
                projectChanged=true;
            }

            //drawing.setForeground(0,Qt::gray);
            //ui->drawingWindow->showItem(&drawing);
            showRootDrawingItems();

        } else retval=true;
    }
    setMenusI(51);
    return retval;
}

CustomTreeWidgetItem* get_vertexItem (CustomTreeWidgetItem *item)
{
    if (!item) return nullptr;;

    Handle(AIS_Shape) shape=item->getShape();
    if (shape.IsNull()) return nullptr;
    if (shape->Shape().ShapeType() == TopAbs_VERTEX) return item;

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
        if (get_vertexItem(child)) return child;
        i++;
    }

    return nullptr;
}

bool OpenParEMg::saveBrepFile (char *filePath)
{
    std::cout << "OpenParEMg::saveBrepFile" << std::endl; std::cout.flush();

    if (drawing.getShape()->Shape().IsNull()) return true;
    if (!BRepTools::Write(drawing.getShape()->Shape(),filePath)) return true;
    return false;
}

bool OpenParEMg::saveStepFile (QString filePath)
{
    if (!filePath.isEmpty()) {
        STEPControl_Writer writer;
        writer.Transfer(drawing.getShape()->Shape(),STEPControl_ManifoldSolidBrep,Standard_True);

        IFSelect_ReturnStatus status=writer.Write(filePath.toStdString().c_str());
        if (status == IFSelect_RetDone) {
            return false;
        }
    }
    return true;
}

bool OpenParEMg::saveBoundaryDatabase ()
{
    QString filename=absolutePath;
    filename.append("/").append(projData.port_definition_file);

    std::ofstream outputFile(filename.toStdString());
    if (outputFile.is_open()) {
        boundaryDatabase->save(&outputFile);
        outputFile.close();
        return false;
    }
    return true;
}

void OpenParEMg::increase_depth (CustomTreeWidgetItem *item)
{
    if (!item) return;

    item->increase_depth();

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
        increase_depth(child);
        i++;
    }
}

void OpenParEMg::decrease_depth (CustomTreeWidgetItem *item)
{
    if (!item) return;

    item->decrease_depth();

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
        decrease_depth(child);
        i++;
    }
}

void OpenParEMg::saveItem (std::ofstream *out, CustomTreeWidgetItem *item)
{
    if (!item) return;

    if (item->is_rootDrawing()) {
        int i=0;
        while (i < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
            saveItem(out,child);
            i++;
        }
    } else if (item->is_drawing()) {

        Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
        if (polywire) {
            polywire->save(out,item->get_name(),item->get_depth());
        }

        Process *process=static_cast<Process *>(item->getProcess());
        if (process) {
            process->startSave(out,item->get_name(),item->get_material(),item->get_depth());

            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                saveItem(out,child);
                i++;
            }

            process->endSave(out,item->get_depth());
        }

        // Brep
        std::cout << "  polywire=" << polywire << std::endl; std::cout.flush();
        std::cout << "  process=" << process << std::endl; std::cout.flush();
        if (!polywire && !process) {
            std::string space;
            long unsigned int i=0;
            while (i < item->get_depth()) {
                space.append("   ");
                i++;
            }

            *out << space << "BRep" << std::endl;
            if (!item->get_name().isEmpty()) {
                *out << space << "   name=" << item->get_name().toStdString() << std::endl;
            }
            if (!item->get_material().isEmpty()) {
                *out << space << "   material=" << item->get_material().toStdString() << std::endl;
            }

            // uses TopTools_FormatVersion_VERSION_1
            BRepTools::Write(item->getShape()->Shape(),*out);

            *out << std::endl;
            *out << space << "EndBRep" << std::endl;
        }
    }
}

int OpenParEMg::isStartBlock (std::vector<std::string> &inputData, long unsigned int index)
{
    if (inputData[index].compare("Line") == 0) return 1;
    if (inputData[index].compare("Polyline") == 0) return 2;
    if (inputData[index].compare("Polycircle") == 0) return 3;
    if (inputData[index].compare("Rectangle") == 0) return 4;
    if (inputData[index].compare("Extrude") == 0) return 5;
    if (inputData[index].compare("Merge") == 0) return 6;
    if (inputData[index].compare("Subtract") == 0) return 7;
    if (inputData[index].compare("BRep") == 0) return 8;
    return 0;
}

int OpenParEMg::isEndBlock (std::vector<std::string> &inputData, long unsigned int index)
{
    if (inputData[index].compare("EndLine") == 0) return 1;
    if (inputData[index].compare("EndPolyline") == 0) return 2;
    if (inputData[index].compare("EndPolycircle") == 0) return 3;
    if (inputData[index].compare("EndRectangle") == 0) return 4;
    if (inputData[index].compare("EndExtrude") == 0) return 5;
    if (inputData[index].compare("EndMerge") == 0) return 6;
    if (inputData[index].compare("EndSubtract") == 0) return 7;
    if (inputData[index].compare("EndBRep") == 0) return 8;
    return 0;
}

bool OpenParEMg::getBlockKeywordValue (std::vector<std::string> &inputData, int type,
                                       long unsigned int &startBlockIndex, long unsigned int &endBlockIndex,
                                       std::string &keyword, std::string &value)
{
    //std::cout << "OpenParEMg::getBlockKeywordValue" << std::endl; std::cout.flush();

    int level=0;  // keeping track of levels in the hierarchy
    long unsigned int index=startBlockIndex;
    while (index <= endBlockIndex) {
        int startType=isStartBlock(inputData,index);
        if (startType == type) level++;

        int endType=isEndBlock(inputData,index);
        if (endType == type) level--;

        if (level == 0 && extractText(inputData[index],keyword,value)) {
            return true;
        }

        index++;
    }

    return false;  // failed to find the keyword
}

int OpenParEMg::findStartNextBlock (std::vector<std::string> &inputData, long unsigned int &startBlockIndex)
{
    while (startBlockIndex < inputData.size()) {
        int type=isStartBlock(inputData,startBlockIndex);
        if (type) return type;
        startBlockIndex++;
    }
    return 0;  // failed to find a block
}

int OpenParEMg::findEndNextBlock (std::vector<std::string> &inputData, int type, long unsigned int &endBlockIndex)
{
    int level=0;
    while (endBlockIndex < inputData.size()) {

        // keep track of nested blocks of the same type
        int startType=isStartBlock(inputData,endBlockIndex);
        if (startType == type) level++;

        int endType=isEndBlock(inputData,endBlockIndex);
        if (level == 0 && endType == type) return type;

        if (endType == type) level--;

        endBlockIndex++;
    }
    return 0;  // failed to find a block terminator
}


bool OpenParEMg::loadItem (std::vector<std::string> &inputData, long unsigned int &startBlockIndex,
                           long unsigned int &endBlockIndex, CustomTreeWidgetItem *parent, bool increaseDepth)
{
    //std::cout << "OpenParEMg::loadItem" << "  startBlockIndex=" << startBlockIndex << "  endBlockIndex=" << endBlockIndex << std::endl; std::cout.flush();

    int typeStart=findStartNextBlock (inputData,startBlockIndex);

    // check to see if done
    if (typeStart == 0) {return false;}

    endBlockIndex=startBlockIndex+1;
    int typeEnd=findEndNextBlock (inputData,typeStart,endBlockIndex);

    // missing block terminator
    if (typeEnd == 0) {
        return false;
    }

    // incorrect block formatting
    if (typeStart != typeEnd) {
        return false;
    }

    Polywire *polywire=nullptr;
    if (typeStart == 1) polywire=new Line();
    else if (typeStart == 2) polywire=new Polyline();
    else if (typeStart == 3) polywire=new Polycircle();
    else if (typeStart == 4) polywire=new Rectangle();

    std::string name;
    if (polywire) {
        polywire->load(inputData,startBlockIndex,endBlockIndex,name,&objectCounts);
        polywire->set_viewerContext(ui->drawingWindow->get_viewerContext());
        //CustomTreeWidgetItem *newItem=addItemShapeCreate(polywire,parent);
        CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem(0);
        Handle(AIS_Shape) dummy;
        ShapeData *newShapeData=new ShapeData(1,polywire,nullptr,dummy);
        newItem->addShapeData(newShapeData);
        newItem->setText(0,QString::fromStdString(name));
        if (!parent->is_rootDrawing()) newItem->copy_depth(parent);
        if (increaseDepth) newItem->increase_depth();
        parent->addChild(newItem);
        reprocess(newItem);
        drawingChanged=true;
        ui->drawingWindow->showItem(newItem);
        startBlockIndex=endBlockIndex;
    }

    bool loadBrep=false;
    Process *process=nullptr;
    if (typeStart == 5) process=new Extrude();
    if (typeStart == 6) process=new Merge();
    if (typeStart == 7) process=new Subtract();
    if (typeStart == 8) loadBrep=true;

    if (process) {
        CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem();
        Handle(AIS_Shape) dummy;
        ShapeData *newShapeData=new ShapeData(1,nullptr,process,dummy);
        newItem->addShapeData(newShapeData);
        parent->addChild(newItem);

        // extrude
        if (typeStart == 5) {

            // name
            long unsigned int localStartBlockIndex=startBlockIndex+1;
            long unsigned int localEndBlockIndex=endBlockIndex-1;
            std::string keyword="name";
            std::string name;
            if (getBlockKeywordValue(inputData,typeStart,localStartBlockIndex,localEndBlockIndex,keyword,name)) {
                newItem->setText(0,QString::fromStdString(name));
                if (!parent->is_rootDrawing()) newItem->copy_depth(parent);
                if (increaseDepth) newItem->increase_depth();
                objectCounts.extrude++;
            }

            // length
            localStartBlockIndex=startBlockIndex+1;
            localEndBlockIndex=endBlockIndex-1;
            keyword="length";
            std::string length;
            if (getBlockKeywordValue(inputData,typeStart,localStartBlockIndex,localEndBlockIndex,keyword,length)) {
                Extrude *extrude=dynamic_cast<Extrude *>(process);
                if (extrude) {
                    extrude->set_length(stod(length));
                }
            }

            // get one child
            localStartBlockIndex=startBlockIndex+1;
            localEndBlockIndex=startBlockIndex+1;
            loadItem(inputData,localStartBlockIndex,localEndBlockIndex,newItem,true);

            reprocess(newItem);
            drawingChanged=true;
            ui->drawingWindow->showItem(newItem);
        }

        // merge and subtract
        if (typeStart == 6 || typeStart == 7) {

            // name
            long unsigned int localStartBlockIndex=startBlockIndex+1;
            long unsigned int localEndBlockIndex=endBlockIndex-1;
            std::string keyword="name";
            std::string name;
            if (getBlockKeywordValue(inputData,typeStart,localStartBlockIndex,localEndBlockIndex,keyword,name)) {
                newItem->setText(0,QString::fromStdString(name));
                if (!parent->is_rootDrawing()) newItem->copy_depth(parent);
                if (increaseDepth) newItem->increase_depth();
                if (typeStart == 6) objectCounts.merge++;
                if (typeStart == 7) objectCounts.subtract++;
            }

            // get two children

            localStartBlockIndex=startBlockIndex+1;
            localEndBlockIndex=startBlockIndex+1;
            loadItem(inputData,localStartBlockIndex,localEndBlockIndex,newItem,true);

            localStartBlockIndex=localEndBlockIndex+1;
            loadItem(inputData,localStartBlockIndex,localEndBlockIndex,newItem,true);

            reprocess(newItem);
            drawingChanged=true;
            ui->drawingWindow->showItem(newItem);
        }

        startBlockIndex=endBlockIndex;
    }

    if (loadBrep) {
        std::stringstream ss;

        long unsigned int i=startBlockIndex+1;
        while (i < endBlockIndex) {
            ss << inputData[i] << std::endl;
            i++;
        }

        TopoDS_Shape shape;
        BRep_Builder builder;
        BRepTools::Read(shape,ss,builder);

        if (!shape.IsNull()) {
            //CustomTreeWidgetItem *newItem=addItemShapeCreate(shape,parent);
            CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem(0);
            Handle(AIS_Shape) dummy;
            ShapeData *newShapeData=new ShapeData(1,polywire,nullptr,dummy);
            newItem->addShapeData(newShapeData);

            // name
            long unsigned int localStartBlockIndex=startBlockIndex+1;
            long unsigned int localEndBlockIndex=endBlockIndex-1;
            std::string keyword="name";
            std::string name;
            if (getBlockKeywordValue(inputData,typeStart,localStartBlockIndex,localEndBlockIndex,keyword,name)) {
                newItem->setText(0,QString::fromStdString(name));
                if (!parent->is_rootDrawing()) newItem->copy_depth(parent);
                if (increaseDepth) newItem->increase_depth();
                objectCounts.solid++;
            }

            parent->addChild(newItem);
            reprocess(newItem);
            drawingChanged=true;
            ui->drawingWindow->showItem(newItem);
        }

        startBlockIndex=endBlockIndex;
    }

    return true;
}

bool OpenParEMg::saveDrawingFile (QString filename)
{
    std::ofstream outputFile(filename.toStdString());
    if (outputFile.is_open()) {
        outputFile << std::setprecision(15);
        outputFile << "#OpenParEMgDrawing 1.0" << std::endl;
        outputFile << std::endl;
        saveItem(&outputFile,&drawing);
        outputFile.close();
        drawingChanged=false;
        return false;
    }
    return true;
}

bool OpenParEMg::loadDrawingFile ()
{
    QString filename=absolutePath;
    filename.append("/").append(projData.project_name);
    filename.append(".opd");

    // put the data in the file into a string vector
    std::vector<std::string> inputData=readFileToVector(filename.toStdString());
    if (inputData.size() == 0) return true;

    // check the format
    bool found=false;
    long unsigned int i=0;
    while (i < inputData.size()) {
        std::vector<std::string> tokens=splitWhitespace(inputData[i]);
        if (tokens.size() == 2) {
            if (tokens[0].compare("#OpenParEMgDrawing") == 0) {
                if (tokens[1].compare("1.0") == 0) {found=true; break;}
            }
        }
        i++;
    }
    if (!found) {
        QMessageBox mb;
        mb.critical(nullptr, "Error","Drawing file format not recognized.");
        mb.setFixedSize(500, 200);
        return true;
    }

    // load
    long unsigned int startBlockIndex=0;
    long unsigned int endBlockIndex=0;
    while (loadItem(inputData,startBlockIndex,endBlockIndex,&drawing,false)) {
        //startBlockIndex=endBlockIndex;
    }

    return false;
}

void OpenParEMg::on_actionImportBrep_triggered ()
{
    QString filePath=QFileDialog::getOpenFileName(this, tr("Open BREP File"), "", tr("BREP Files (*.brep)"),
                                                  nullptr,QFileDialog::DontUseNativeDialog);
    if (filePath.isEmpty()) return;

    if (loadBrepFile(filePath,true)) {
        QString message="Unable to load Brep file \"";
        message.append(filePath);
        message.append("\".");
        QMessageBox mb;
        mb.critical(nullptr, "Error",message);
        mb.setFixedSize(500, 200);
    }

    ui->drawingWindow->fitAll();
    ui->drawingWindow->updateViewer();
    setMenusI(52);
}

void OpenParEMg::on_actionImportStep_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open STEP File"), "", tr("STEP Files (*.step *.stp)"),
                                                    nullptr,QFileDialog::DontUseNativeDialog);
    if (filePath.isEmpty()) return;
    if (loadStepFile(filePath,true)) {
        QString message="Unable to load STEP file \"";
        message.append(filePath);
        message.append("\".");
        QMessageBox mb;
        mb.critical(nullptr, "Error",message);
        mb.setFixedSize(500, 200);
    }

    ui->drawingWindow->fitAll();
    ui->drawingWindow->updateViewer();

    setMenusI(53);
}

void OpenParEMg::on_actionExportStep_triggered()
{
    // std::vector<Handle(AIS_InteractiveObject)> selectedList;
    // ui->drawingWindow->getSelected (&selectedList);
    // if (selectedList.size() == 0) {
    //     QMessageBox mb;
    //     mb.critical(nullptr,"Error","Select solid shapes to export.");
    //     mb.setFixedSize(500, 200);
    //     return;
    // }

    QString filePath=QFileDialog::getSaveFileName(this,tr("Save STEP File"), absolutePath, tr("STEP Files (*.step *.stp)"),
                                                  nullptr,QFileDialog::DontUseNativeDialog);
    if (filePath.isNull()) return;

    //if (saveStepFile(filePath,&selectedList)) {
    if (saveStepFile(filePath)) {
        QString message="Unable to save STEP file \"";
        message.append(filePath);
        message.append("\".");
        QMessageBox mb;
        mb.critical(nullptr, "Error",message);
        mb.setFixedSize(500, 200);
    }
    setMenusI(54);
}

void OpenParEMg::on_actionExit_triggered ()
{
    if (projectChanged || drawingChanged || meshChanged || boundaryDatabase->is_modified()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,"OpenParEMg","There are unsaved changes.  Do you want to exit anyway?",QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No) return;
    }
    QApplication::quit();
}

void OpenParEMg::on_actionSelectMaterialsDatabase_triggered ()
{
    SelectMaterialsDatabase *selectMaterialsDatabase=new SelectMaterialsDatabase();
    selectMaterialsDatabase->set_simulationRunning(simulationRunning);
    selectMaterialsDatabase->set_projData(&projData);
    selectMaterialsDatabase->set_absolutePath(&absolutePath);
    selectMaterialsDatabase->set_materialDatabase(materialDatabase);
    selectMaterialsDatabase->exec();
    delete selectMaterialsDatabase;

    if (projData.modified) {
        projectChanged=true;
    }
    setMenusI(55);
}

void OpenParEMg::on_actionMaterialsOptions_triggered()
{
    MaterialsOptions *materialsOptions=new MaterialsOptions();
    materialsOptions->set_simulationRunning(simulationRunning);
    materialsOptions->set_projData(&projData);
    materialsOptions->set_materialDatabase(materialDatabase);
    materialsOptions->fillMaterialSelector();
    materialsOptions->exec();
    delete materialsOptions;

    if (projData.modified) {
        projectChanged=true;
    }
    setMenusI(56);
}

void OpenParEMg::on_drawingItemTree_itemClicked (QTreeWidgetItem *item, int column)
{
    std::cout << "OpenParEMg::on_drawingItemTree_itemClicked" << std::endl; std::cout.flush();
    std::cout << "   clickedItem=" << clickedItem << std::endl;
    std::cout << "   previousClickedItem=" << previousClickedItem << std::endl;
    std::cout << "   CTRLpressed=" << CTRLpressed << std::endl;
    std::cout << "   SHIFTpressed=" << SHIFTpressed << std::endl;

    clickedItem=(CustomTreeWidgetItem *)item;

    // allow multiple selection on matched types only
    bool matchedType=false;
    if (previousClickedItem) {
        if (clickedItem->get_itemType() == previousClickedItem->get_itemType()) {
            matchedType=true;
        }
    }

    // allow multiple selection only at the same level
    bool matchedLevel=false;
    if (previousClickedItem) {
        if (clickedItem->QTreeWidgetItem::parent() == previousClickedItem->QTreeWidgetItem::parent()) {
            matchedLevel=true;
        }
    }

    // exception: allow path + (voltage or current) for assigning paths
    if (previousClickedItem) {
        if ((clickedItem->is_path() && previousClickedItem->is_voltage() || previousClickedItem->is_current()) ||
            (previousClickedItem->is_path() && clickedItem->is_voltage() || clickedItem->is_current())) {
            matchedType=true;  // not really
            matchedLevel=true; // not really
        }
    }

    if (CTRLpressed) {
        if (SHIFTpressed) {
        } else {
            if (matchedType && matchedLevel) {
                ui->drawingItemTree->setCurrentItem(clickedItem);
                ui->drawingWindow->selectItem(clickedItem);
                previousClickedItem=clickedItem;
            } else {
                clickedItem->setSelected(false);
                ui->drawingItemTree->setCurrentItem(nullptr);
            }
        }
    } else if (SHIFTpressed) {
        if (CTRLpressed) {
        } else {
            if (matchedType && matchedLevel) {

                ui->drawingItemTree->setCurrentItem(clickedItem);

                // select the end item

                //ui->drawingWindow->selectItem(previousClickedItem);
                ui->drawingWindow->selectItem(clickedItem);

                // select the middle items

                int count=ui->drawingItemTree->indexFromItem(clickedItem,0).row()-
                        ui->drawingItemTree->indexFromItem(previousClickedItem,0).row();

                if (count > 1) {  // forward
                    CustomTreeWidgetItem *nextItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemBelow(previousClickedItem);
                    while (nextItem != clickedItem) {
                        if (nextItem->QTreeWidgetItem::parent() == clickedItem->QTreeWidgetItem::parent()) {
                            ui->drawingWindow->selectItem(nextItem);
                        }
                        nextItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemBelow(nextItem);
                    }
                } else if (-count > 1) {  // reversed
                    CustomTreeWidgetItem *nextItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemBelow(clickedItem);
                    while (nextItem != previousClickedItem) {
                        if (nextItem->QTreeWidgetItem::parent() == previousClickedItem->QTreeWidgetItem::parent()) {
                            ui->drawingWindow->selectItem(nextItem);
                        }
                        nextItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemBelow(nextItem);
                    }
                }

                previousClickedItem=clickedItem;
            } else {
                clickedItem->setSelected(false);
                ui->drawingItemTree->setCurrentItem(nullptr);
            }
        }
    } else {
        CustomTreeWidgetItem *clickedItemKeep=clickedItem;
        clearTreeSelection();
        clickedItem=clickedItemKeep;

        ui->drawingItemTree->setCurrentItem(clickedItem);
        ui->drawingWindow->selectItem(clickedItem);
        previousClickedItem=clickedItem;
    }
    ui->drawingWindow->updateViewer();
    setMenusI(57);
}

void OpenParEMg::on_actionFitSelected_triggered ()
{
    ui->drawingWindow->fitSelected();
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_actionFitAll_triggered ()
{
    ui->drawingWindow->fitAll();
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_actionShape_triggered ()
{
    //std::cout << "OpenParEMg::on_actionShape_triggered" << std::endl; std::cout.flush();

    clearSelection();
    ui->actionShape->setCheckable(true);
    ui->actionShape->setChecked(true);
    previousSelectionIndex=0;

    ui->drawingWindow->Deactivate();
    ui->drawingWindow->Activate(0,Standard_False);
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_actionVertex_triggered ()
{
    //std::cout << "OpenParEMg::on_actionVertex_triggered" << std::endl; std::cout.flush();

    clearSelection();
    ui->actionVertex->setCheckable(true);
    ui->actionVertex->setChecked(true);
    previousSelectionIndex=1;

    ui->drawingWindow->Deactivate();
    ui->drawingWindow->Activate(1,Standard_False);
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_actionEdge_triggered ()
{
    //std::cout << "OpenParEMg::on_actionEdge_triggered" << std::endl; std::cout.flush();

    clearSelection();
    ui->actionEdge->setCheckable(true);
    ui->actionEdge->setChecked(true);
    previousSelectionIndex=2;

    ui->drawingWindow->Deactivate();
    ui->drawingWindow->Activate(2,Standard_False);
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_actionWire_triggered ()
{
    //std::cout << "OpenParEMg::on_actionWire_triggered" << std::endl; std::cout.flush();

    clearSelection();
    ui->actionWire->setCheckable(true);
    ui->actionWire->setChecked(true);
    previousSelectionIndex=3;

    ui->drawingWindow->Deactivate();
    ui->drawingWindow->Activate(3,Standard_False);
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_actionFace_triggered ()
{
    //std::cout << "OpenParEMg::on_actionFace_triggered" << std::endl; std::cout.flush();

    clearSelection();
    ui->actionFace->setCheckable(true);
    ui->actionFace->setChecked(true);
    previousSelectionIndex=4;

    ui->drawingWindow->Deactivate();
    ui->drawingWindow->Activate(4,Standard_False);
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_actionShell_triggered ()
{
    //std::cout << "OpenParEMg::on_actionShell_triggered" << std::endl; std::cout.flush();

    clearSelection();
    ui->actionShell->setCheckable(true);
    ui->actionShell->setChecked(true);
    previousSelectionIndex=5;

    ui->drawingWindow->Deactivate();
    ui->drawingWindow->Activate(5,Standard_False);
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_actionSolid_triggered ()
{
    //std::cout << "OpenParEMg::on_actionSolid_triggered" << std::endl; std::cout.flush();

    clearSelection();
    ui->actionSolid->setCheckable(true);
    ui->actionSolid->setChecked(true);
    previousSelectionIndex=6;

    ui->drawingWindow->Deactivate();
    ui->drawingWindow->Activate(6,Standard_False);
    ui->drawingWindow->updateViewer();
}

bool OpenParEMg::hasSelectedPaths ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_path()) return true;
        i++;
    }
    return false;
}

void OpenParEMg::clearTreeSelection ()
{
    //std::cout << "OpenParEMg::clearTreeSelection" << std::endl; std::cout.flush();

    ui->drawingWindow->unselectAllItems();
    ui->drawingItemTree->clearSelection();
    ui->drawingItemTree->setCurrentItem(nullptr);
    clickedItem=nullptr;
    previousClickedItem=nullptr;
    ui->drawingWindow->updateViewer();
    setMenusI(58);
}

bool OpenParEMg::eventFilter (QObject *obj, QEvent *event)
{
    //std::cout << "OpenParEMg::eventFilter  event->type()=" << event->type() << std::endl; std::cout.flush();

    if (event->type() == QEvent::MouseButtonPress) {

        // click on background in the item tree to clear the selection
        if (obj == ui->drawingItemTree->viewport()) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (ui->drawingItemTree->indexAt(mouseEvent->pos()).isValid() == false) {
                on_actionShape_triggered();
                clearTreeSelection();
            }
        }
    }

    // if (event->type() == QEvent::Paint) {
    // }

    return QObject::eventFilter(obj, event);
}

void OpenParEMg::keyPressEvent (QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control) {
        CTRLpressed=true;
    } else if (event->key() == Qt::Key_Shift) {
        SHIFTpressed=true;

        if (activePolywire) {
            Rectangle *rectangle=dynamic_cast<Rectangle *>(activePolywire);
            if (rectangle) rectangle->setIsSquare(true);
        }
    } else if (event->key() == Qt::Key_Escape) {
        //std::cout << "OpenParEMg::keyPressEvent   Qt::Key_Escape" << std::endl; std::cout.flush();

        if (lengthInputForm) {
            lengthInputForm->on_CancelButton_clicked();
            lengthInputForm=nullptr;
        }

        if (vectorInputForm) {
            vectorInputForm->on_CancelButton_clicked();
            vectorInputForm=nullptr;
        }

        if (lengthEditForm) {
            lengthEditForm->on_CancelButton_clicked();
            lengthEditForm=nullptr;
        }

        if (lineEditForm) {
            lineEditForm->on_CancelButton_clicked();
            lineEditForm=nullptr;
        }

        if (rectangleEditForm) {
            rectangleEditForm->on_CancelButton_clicked();
            rectangleEditForm=nullptr;
        }

        if (polycircleEditForm) {
            polycircleEditForm->on_CancelButton_clicked();
            polycircleEditForm=nullptr;
        }

        if (rotateInputForm) {
            rotateInputForm->on_CancelButton_clicked();
            rotateInputForm=nullptr;
        }

        on_actionShape_triggered();
        ui->drawingWindow->setSubshapeSelection(false);
        ui->drawingWindow->setSetToPlane(false);

        if (renameItem) {
            ui->drawingItemTree->removeItemWidget(renameItem,0);
            renameItem=nullptr;
        }
    }
    QWidget::keyPressEvent(event);
}

void OpenParEMg::keyReleaseEvent (QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control) {
        CTRLpressed=false;
    } else if (event->key() == Qt::Key_Shift) {
        SHIFTpressed=false;

        if (activePolywire) {
            Rectangle *rectangle=dynamic_cast<Rectangle *>(activePolywire);
            if (rectangle) rectangle->setIsSquare(false);
        }
    }
    QWidget::keyReleaseEvent(event);
}

void OpenParEMg::on_actionShowAll_triggered()
{
    ui->drawingWindow->unselectAllItems();
    showRootDrawingItems();
    ui->drawingWindow->showItem(&path);
    ui->drawingWindow->showItem(&port);
    ui->drawingWindow->showItem(&boundary);
    ui->drawingWindow->showItem(&mesh);
    clickedItem=nullptr;
    previousClickedItem=nullptr;
    ui->drawingWindow->updateViewer();
    setMenusI(59);
}

void OpenParEMg::on_actionHideAll_triggered ()
{
    ui->drawingWindow->hideAllItems();
    clearTreeSelection();
    setMenusI(60);
}

void OpenParEMg::on_actionUnselectAll_triggered ()
{
    ui->drawingWindow->updateViewer();
    clearTreeSelection();
    setMenusI(61);
}

void OpenParEMg::drawMesh()
{
    //std::cout << "OpenParEMg::drawMesh" << std::endl; std::cout.flush();

    // get all nodes
    std::vector<std::size_t> nodeTags;
    std::vector<double> nodeCoords, nodeParams;
    gmsh::model::mesh::getNodes(nodeTags, nodeCoords, nodeParams);

    // map node tag to coordinates
    std::map<std::size_t,gp_Pnt> nodeMap;
    long unsigned int i=0;
    while (i < nodeTags.size()) {
        double x=nodeCoords[3*i];
        double y=nodeCoords[3*i+1];
        double z=nodeCoords[3*i+2];
        nodeMap[nodeTags[i]]=gp_Pnt(x, y, z);
        i++;
    }

    // get all elements
    std::vector<int> elementTypes;
    std::vector<std::vector<std::size_t>> elementTags, nodeTagsPerElem;
    gmsh::model::mesh::getElements(elementTypes, elementTags, nodeTagsPerElem);

    size_t e=0;
    while (e < elementTypes.size()) {

        // vertices
        if (elementTypes[e] == 15) {
            CustomTreeWidgetItem *verticesItem=new CustomTreeWidgetItem(0);
            verticesItem->setText(0,"Vertices");
            verticesItem->set_itemType(3);
            verticesItem->setForeground(0,Qt::gray);
            mesh.insertChild(0,verticesItem);

            int count=0;
            std::vector<std::size_t> conn=nodeTagsPerElem[e];
            long unsigned int i=0;
            while (i < conn.size()) {
                gp_Pnt p=nodeMap[conn[i]];
                TopoDS_Vertex vertex=BRepBuilderAPI_MakeVertex(p);
                Handle(AIS_Shape) shape=new AIS_Shape(vertex);
                verticesItem->get_meshEntities()->push_back(shape);
                i+=1;
                count++;
            }
            // verticesItem->setSelected(false);
            // ui->drawingWindow->showItem(verticesItem);
            // ui->drawingWindow->unselectItem(verticesItem);
        }

        // edges
        if (elementTypes[e] == 1) {
            CustomTreeWidgetItem *edgesItem=new CustomTreeWidgetItem(0);
            edgesItem->setText(0,"Edges");
            edgesItem->set_itemType(3);
            edgesItem->setForeground(0,Qt::gray);
            mesh.addChild(edgesItem);

            int count=0;
            std::vector<std::size_t> conn=nodeTagsPerElem[e];
            long unsigned int i=0;
            while (i < conn.size()) {
                gp_Pnt p1=nodeMap[conn[i]];
                gp_Pnt p2=nodeMap[conn[i+1]];
                TopoDS_Edge edge=BRepBuilderAPI_MakeEdge(p1,p2);
                Handle(AIS_Shape) shape=new AIS_Shape(edge);
                edgesItem->get_meshEntities()->push_back(shape);
                i+=2;
                count++;
            }
            // edgesItem->setSelected(false);
            // ui->drawingWindow->showItem(edgesItem);
            // ui->drawingWindow->unselectItem(edgesItem);
        }

        // triangles
        if (elementTypes[e] == 2) {
            CustomTreeWidgetItem *wiresItem=new CustomTreeWidgetItem(0);
            wiresItem->setText(0,"Wires");
            wiresItem->set_itemType(3);
            wiresItem->setForeground(0,Qt::gray);
            mesh.addChild(wiresItem);

            CustomTreeWidgetItem *trianglesItem=new CustomTreeWidgetItem(0);
            trianglesItem->setText(0,"Triangles");
            trianglesItem->set_itemType(3);
            trianglesItem->setForeground(0,Qt::gray);
            mesh.addChild(trianglesItem);

            int count=0;
            std::vector<std::size_t> conn=nodeTagsPerElem[e];
            long unsigned int i=0;
            while (i < conn.size()) {
                gp_Pnt p1=nodeMap[conn[i]];
                gp_Pnt p2=nodeMap[conn[i+1]];
                gp_Pnt p3=nodeMap[conn[i+2]];

                TopoDS_Edge edge12=BRepBuilderAPI_MakeEdge(p1,p2);
                TopoDS_Edge edge23=BRepBuilderAPI_MakeEdge(p2,p3);
                TopoDS_Edge edge31=BRepBuilderAPI_MakeEdge(p3,p1);

                TopoDS_Wire wire=BRepBuilderAPI_MakeWire(edge12,edge23,edge31);
                Handle(AIS_Shape) shape=new AIS_Shape(wire);
                wiresItem->get_meshEntities()->push_back(shape);

                TopoDS_Face face=BRepBuilderAPI_MakeFace(wire);
                shape=new AIS_Shape(face);
                trianglesItem->get_meshEntities()->push_back(shape);

                i+=3;
                count++;
            }
            // wiresItem->setSelected(false);
            // ui->drawingWindow->showItem(wiresItem);
            // ui->drawingWindow->unselectItem(wiresItem);
            // trianglesItem->setSelected(false);
            // ui->drawingWindow->showItem(trianglesItem);
            // ui->drawingWindow->unselectItem(trianglesItem);
        }

        // tetrahedron
        if (elementTypes[e] == 4) {
            CustomTreeWidgetItem *tetrahedronsItem=new CustomTreeWidgetItem(0);
            tetrahedronsItem->setText(0,"Tetrahedrons");
            tetrahedronsItem->set_itemType(3);
            tetrahedronsItem->setForeground(0,Qt::gray);
            mesh.addChild(tetrahedronsItem);

            int count=0;
            std::vector<std::size_t> conn=nodeTagsPerElem[e];
            long unsigned int i=0;
            while (i < conn.size()) {
                gp_Pnt p1=nodeMap[conn[i]];
                gp_Pnt p2=nodeMap[conn[i+1]];
                gp_Pnt p3=nodeMap[conn[i+2]];
                gp_Pnt p4=nodeMap[conn[i+3]];

                TopoDS_Edge edge12=BRepBuilderAPI_MakeEdge(p1,p2);
                TopoDS_Edge edge13=BRepBuilderAPI_MakeEdge(p1,p3);
                TopoDS_Edge edge14=BRepBuilderAPI_MakeEdge(p1,p4);
                TopoDS_Edge edge23=BRepBuilderAPI_MakeEdge(p2,p3);
                TopoDS_Edge edge34=BRepBuilderAPI_MakeEdge(p3,p4);
                TopoDS_Edge edge42=BRepBuilderAPI_MakeEdge(p4,p2);

                TopoDS_Wire wire123=BRepBuilderAPI_MakeWire(edge12,edge23,edge13);
                TopoDS_Wire wire134=BRepBuilderAPI_MakeWire(edge13,edge34,edge14);
                TopoDS_Wire wire124=BRepBuilderAPI_MakeWire(edge12,edge42,edge14);
                TopoDS_Wire wire234=BRepBuilderAPI_MakeWire(edge23,edge34,edge42);

                TopoDS_Face face123=BRepBuilderAPI_MakeFace(wire123);
                TopoDS_Face face134=BRepBuilderAPI_MakeFace(wire134);
                TopoDS_Face face124=BRepBuilderAPI_MakeFace(wire124);
                TopoDS_Face face234=BRepBuilderAPI_MakeFace(wire234);

                TopoDS_Compound tetrahedron;
                BRep_Builder builder;
                builder.MakeCompound(tetrahedron);
                builder.Add(tetrahedron,face123);
                builder.Add(tetrahedron,face134);
                builder.Add(tetrahedron,face124);
                builder.Add(tetrahedron,face234);

                Handle(AIS_Shape) shape=new AIS_Shape(tetrahedron);
                tetrahedronsItem->get_meshEntities()->push_back(shape);

                i+=4;
                count++;
            }
            // tetrahedronsItem->setSelected(false);
            // ui->drawingWindow->showItem(tetrahedronsItem);
            // ui->drawingWindow->unselectItem(tetrahedronsItem);
        }

        e++;
    }

    setMenusI(62);
}

void OpenParEMg::deleteMesh (bool deleteMeshFile)
{
    std::cout << "OpenParEMg::deleteMesh" << std::endl; std::cout.flush();

    int i=0;
    while (i < mesh.childCount()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *) mesh.child(i);

        // remove from tracker
        ui->drawingWindow->hideItem(item);
        ui->drawingWindow->unselectItem(item);

        // remove drawing shapes
        std::vector<Handle(AIS_Shape)> *meshEntities=item->get_meshEntities();
        long unsigned int j=0;
        while (j < meshEntities->size()) {
            ui->drawingWindow->deleteShape((*meshEntities)[j]);
            j++;
        }
        i++;
    }

    // remove the mesh file
    if (deleteMeshFile && strcmp(projData.mesh_file,"") != 0) {
        QFile meshFile=QFile(projData.mesh_file);
        if (meshFile.exists()) {
            meshFile.remove();
            QString blank="";
            cstrFromQString(&(projData.mesh_file),blank);
            projectChanged=true;
        }
    }

    ui->drawingWindow->updateViewer();
    mesh.deleteChildren(&mesh);
    drawingEntities.clear();
    gmsh::clear();
}

void OpenParEMg::on_actionMeshGenerate_triggered ()
{
    if (mesh.childCount() > 0) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,"OpenParEMg","Delete the existing mesh?",QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        deleteMesh(true);
    }

    // reset dimTags
    //resetDimTag(&drawing);
    renumberDimTag();

    // generate mesh
    TopoDS_Shape shape=drawing.getShape()->Shape();
    gmsh::model::occ::importShapesNativePointer((void *) &shape,drawingEntities,false);
    gmsh::model::occ::synchronize();
    gmsh::model::mesh::generate();

    // default name for the mesh
    if (strcmp(projData.mesh_file,"") == 0) {
        QString meshFile=projectName;
        meshFile.append(".msh");
        cstrFromQString(&(projData.mesh_file),meshFile);
        projectChanged=true;
    }

    meshChanged=true;

    drawMesh();
    setPhysicalGroups();
    ui->drawingWindow->showItem(&mesh);

    ui->drawingWindow->updateViewer();
    setMenusI(63);
}

void OpenParEMg::loadMeshFile (QString meshfile)
{
    if (QFile::exists(meshfile)) {

        if (mesh.childCount() > 0) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this,"OpenParEMg","Delete the existing mesh?",QMessageBox::Yes|QMessageBox::No);
            if (reply != QMessageBox::Yes) return;

            deleteMesh(false);
        }

        // load and display
        //gmsh::model::remove();
        gmsh::open(meshfile.toStdString());

        meshChanged=false;

        drawMesh();

        // save the file name if different
        if (meshfile.compare(projData.mesh_file) != 0) {
            cstrFromQString (&(projData.mesh_file),meshfile);
            projectChanged=true;
        }

    }

    setMenusI(64);
}

void OpenParEMg::on_actionMeshLoad_triggered ()
{
    QString meshfile=QFileDialog::getOpenFileName(this,tr("Open Mesh"), "", tr("Mesh Files (*.msh);;All Files (*)"),
                                                  nullptr,QFileDialog::DontUseNativeDialog);

    // return if user cancels
    if (meshfile.isNull()) return;

    loadMeshFile(meshfile);
    ui->drawingWindow->showItem(&mesh);
    setMenusI(65);
}

void OpenParEMg::on_actionMeshSave_triggered ()
{
    if (strcmp(projData.mesh_file,"") != 0) {
        gmsh::write(projData.mesh_file);
        meshChanged=false;
    } else {
        on_actionMeshSaveAs_triggered();
    }
    setMenusI(66);
}

void OpenParEMg::on_actionMeshSaveAs_triggered ()
{
    QString testAbsolutePath="";
    QString meshfile;

    while (testAbsolutePath != absolutePath) {

        // get file name
        QString testMeshFile=QFileDialog::getSaveFileName(this,tr("Save Mesh File"), absolutePath, tr("Data Files (*.msh);;All Files (*)"),
                                                          nullptr,QFileDialog::DontUseNativeDialog);
        if (testMeshFile.isNull()) return;

        // break up the full path
        QFileInfo fileInfo(testMeshFile);
        testAbsolutePath=fileInfo.absolutePath();
        meshfile=fileInfo.fileName();

        // test if done
        if (testAbsolutePath == absolutePath) break;

        // enforce working directory restriction
        QMessageBox mb;
        mb.critical(nullptr, "Error","Cannot save meshes outside of the project directory.");
        mb.setFixedSize(500, 200);
    }

    gmsh::write(meshfile.toStdString());
    meshChanged=false;

    // save the filename in projData
    if (meshfile.compare(projData.mesh_file) != 0) {
        cstrFromQString (&(projData.mesh_file),meshfile);
        projectChanged=true;
    }

    setMenusI(67);
}

void OpenParEMg::on_actionMeshDelete_triggered ()
{
    deleteMesh(true);

    meshChanged=false;
    projectChanged=true;
    setMenusI(68);
}

void OpenParEMg::on_actionWireframe_triggered ()
{
    // if (ui->actionWireframe->isChecked() == true) {
    //     set_displayMode(&drawing,0);
    //     set_displayMode(&port,0);
    //     set_displayMode(&boundary,0);
    // } else {
    //     set_displayMode(&drawing,1);
    //     set_displayMode(&port,1);
    //     set_displayMode(&boundary,1);
    // }

    // ui->drawingWindow->reshowItems();
    // ui->drawingWindow->updateViewer();

    if (ui->actionWireframe->isChecked() == true) {
        ui->drawingWindow->set_wireframe(true);
    } else {
        ui->drawingWindow->set_wireframe(false);
    }
    ui->drawingWindow->updateViewer();
}

void eh3D(MPI_Comm *comm, int *err, ...)
{
    QMessageBox mb;
    mb.critical(nullptr, "Error","eh3D: Failed to launch OpenParEM3D.");
}

void OpenParEMg::on_actionRun_triggered ()
{
    // check for an existing lock file

    std::cout << "absolutePath=" << absolutePath.toStdString() << std::endl; std::cout.flush();
    QString currentPath;
    currentPath=QDir::currentPath();
    std::cout << "currentPath=" << currentPath.toStdString() << std::endl; std::cout.flush();

    std::string lockfile=".";
    lockfile.append(projData.project_name);
    lockfile.append(".lock");

    if (std::filesystem::exists(lockfile)) {
        QMessageBox mb;
        mb.setText("A lock file is present preventing execution, indicating a running job or a killed job.");
        mb.setInformativeText("Click OK to remove the lock and continue or Cancel to abort.");
        mb.setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
        mb.setDefaultButton(QMessageBox::Cancel);
        if (mb.exec() == QMessageBox::Cancel) return;

        if (remove(lockfile.c_str())) {
            QMessageBox mb;
            QString message="Error removing the lock file \"";
            message.append(lockfile);
            message.append("\".");
            mb.critical(nullptr, "Error",message);
            return;
        }
    }

    // OpenParEM2D

    if (system("OpenParEM2D -h > /dev/null")) {
        QMessageBox mb;
        mb.critical(nullptr, "Error","Cannot execute OpenParEM2D.");
        mb.setFixedSize(500, 200);
        return;
    }

    // OpenParEM3D

    if (system("which OpenParEM3D > /dev/null")) {
        QMessageBox mb;
        mb.critical(nullptr, "Error","Cannot find OpenParEM3D.");
        mb.setFixedSize(500, 200);
        return;
    }

    if (system("OpenParEM3D -h > /dev/null")) {
        QMessageBox mb;
        mb.critical(nullptr, "Error","Cannot execute OpenParEM3D.");
        mb.setFixedSize(500, 200);
        return;
    }

    // run OpenParEM3D

    char *project=nullptr;
    cstrFromQString (&project,projectFile);

    char** argv=(char **)malloc(2*sizeof(char *));
    argv[0]=project;
    argv[1]=nullptr;

    int *error_codes=(int *)malloc(projData.gui_slot_count*sizeof(int));

    MPI_Errhandler errorHandler;
    MPI_Comm_create_errhandler(eh3D,&errorHandler);
    MPI_Comm_set_errhandler(PETSC_COMM_WORLD,errorHandler);

    if (MPI_PORT_COMM) MPI_Comm_free(MPI_PORT_COMM);
    MPI_PORT_COMM=new MPI_Comm();

    MPI_Comm_spawn ("OpenParEM3D",argv,projData.gui_slot_count,MPI_INFO_NULL,0,PETSC_COMM_WORLD,MPI_PORT_COMM,error_codes);

    // check that all processes spawned
    bool fail=false;
    int i=0;
    while (i < projData.gui_slot_count) {
        if (error_codes[i] == MPI_ERR_SPAWN) {
            fail=true;
            break;
        }
        i++;
    }
    if (fail) {
        QMessageBox mb;
        mb.critical(nullptr, "Error","Failed to launch OpenParEM3D.");
    } else {

        simulationRunning=true;
        setMenusI(69);

        timer->start(500);

        // send current path
        std::filesystem::path currentPath=std::filesystem::current_path();
        std::string projectPath=currentPath.string();
        std::cout << "sending projectPath=" << projectPath << std::endl; std::cout.flush();
        int length=projectPath.length();
        int i=0;
        while (i < projData.gui_slot_count) {
            MPI_Send(&length,1,MPI_INT,i,10,*MPI_PORT_COMM);
            MPI_Send(projectPath.c_str(),length,MPI_CHAR,i,11,*MPI_PORT_COMM);
            i++;
        }

        // set up for OpenParEM3D to signal completion
        if (request) MPI_Request_free(request);
        request=new MPI_Request();
        MPI_Irecv(&signal,1,MPI_INT,0,310000,*MPI_PORT_COMM,request);
    }

    // clean up

    MPI_Comm_set_errhandler(PETSC_COMM_WORLD,MPI_ERRORS_RETURN);
    MPI_Errhandler_free(&errorHandler);

    if (project) free(project);
    if (argv) free(argv);
    if (error_codes) free(error_codes);
}

void OpenParEMg::on_actionStop_triggered ()
{
    MPI_Send(&signal,1,MPI_INT,0,300000,*MPI_PORT_COMM);
    simulationStopping=true;
    setMenusI(70);
}

void OpenParEMg::checkFinish ()
{
    //std::cout << "OpenParEMg::checkFinish" << std::endl; std::cout.flush();

    // test for the signal that OpenParEM3D is finished
    int finished;
    MPI_Test(request,&finished,MPI_STATUS_IGNORE);

    if (finished) {
        bool isAbort=simulationAborting;
        simulationRunning=false;
        simulationStopping=false;
        simulationAborting=false;
        setMenusI(71);

        timer->stop();

        // get the status of the 3D simulations
        int fail3D=0;
        MPI_Recv(&fail3D,1,MPI_INT,0,320000,*MPI_PORT_COMM,MPI_STATUS_IGNORE);

        if (fail3D & !isAbort) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "OpenParEM3D failed to properly run. Check the logged output for messages.");
            mb.setFixedSize(500, 200);
        }

        // unblock OpenParEM3D
        MPI_Send(&signal,1,MPI_INT,0,300000,*MPI_PORT_COMM);  // stop
        if (!isAbort) MPI_Send(&signal,1,MPI_INT,0,300001,*MPI_PORT_COMM);  // abort

        if (!isAbort) MPI_Comm_disconnect(MPI_PORT_COMM);

        MPI_Comm_free(MPI_PORT_COMM);
        MPI_PORT_COMM=nullptr;

        MPI_Request_free(request);
        request=nullptr;

        if (isAbort) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"OpenParEM3D Job Aborted.\n");
        }
    }
}

void OpenParEMg::on_actionAbort_triggered ()
{
    //std::cout << "OpenParEMg::on_actionAbort_triggered:" << std::endl;  std::cout.flush();

    simulationStopping=false;
    simulationAborting=true;
    setMenusI(72);

    //timer->stop();

    int signal=1;
    MPI_Send(&signal,1,MPI_INT,0,300001,*MPI_PORT_COMM);

    /*
    // get the status of the 3D simulations
    int fail3D=0;
    MPI_Recv(&fail3D,1,MPI_INT,0,320000,*MPI_PORT_COMM,MPI_STATUS_IGNORE);

    // unblock OpenParEM3D
    signal;
    MPI_Send(&signal,1,MPI_INT,0,300000,*MPI_PORT_COMM);

    //MPI_Comm_disconnect(MPI_PORT_COMM);

    MPI_Comm_free(MPI_PORT_COMM);
    MPI_PORT_COMM=nullptr;

    MPI_Request_free(request);
    request=nullptr;

    prefix(); PetscPrintf(PETSC_COMM_WORLD,"OpenParEM3D Job Aborted.\n");
*/
}

void OpenParEMg::on_actionAbortAndExit_triggered ()
{
    if (projectChanged) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,"OpenParEMg","Are you sure you want to exit?",QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
    }
    prefix(); PetscPrintf(PETSC_COMM_WORLD,"OpenParEM3D Job Aborted.\n");

    std::string lockfile=".";
    lockfile.append(projData.project_name);
    lockfile.append(".lock");

    if (std::filesystem::exists(lockfile)) {
        remove(lockfile.c_str());
    }

    QApplication::quit();
}

void OpenParEMg::on_actionSelectEdge_triggered ()
{
    on_actionEdge_triggered();
    ui->drawingWindow->setSubshapeSelection(true);
}

void OpenParEMg::on_actionSelectWire_triggered()
{
    on_actionWire_triggered();
    ui->drawingWindow->setSubshapeSelection(true);
}

void OpenParEMg::on_actionSelectFace_triggered ()
{
    clearTreeSelection();
    on_actionFace_triggered();
    ui->drawingWindow->setSubshapeSelection(true);
}

void OpenParEMg::on_actionSelectWithBox2_triggered ()
{
    ui->drawingWindow->selectRectangle();
    setMenusI(79);
}

void OpenParEMg::on_actionDrawingPlaneShow_triggered ()
{
    drawingPlaneShown=true;
    ui->drawingWindow->showGrid();
    ui->drawingWindow->updateViewer();
    setMenusI(73);
}

void OpenParEMg::on_actionDrawingPlaneHide_triggered ()
{
    //std::cout << "OpenParEMg::on_actionDrawingPlaneHide_triggered" << std::endl; std::cout.flush();

    drawingPlaneShown=false;
    ui->drawingWindow->hideGrid();
    ui->drawingWindow->updateViewer();
    setMenusI(74);
}

void OpenParEMg::on_actionDrawingPlaneSnapToGrid_triggered ()
{
    if (ui->actionDrawingPlaneSnapToGrid->isChecked()) {
        ui->drawingWindow->set_snapToGrid(true);
    } else {
        ui->drawingWindow->set_snapToGrid(false);
    }
    setMenusI(75);
}

void OpenParEMg::setPlaneToFace ()
{
    skipDrawingPlaneAxisForm=true;
    startPlaneSetToFace();
}

void OpenParEMg::setPlaneToFaceAxis ()
{
    skipDrawingPlaneAxisForm=false;
    startPlaneSetToFace();
}

void OpenParEMg::on_actionDrawingPlaneSetToFace_triggered ()
{
    std::cout << "OpenParEMg::on_actionDrawingPlaneSetToFace_triggered" << std::endl; std::cout.flush();

    startOperation(false);

    skipDrawingPlaneAxisForm=true;
    on_actionFace_triggered();
    ui->drawingWindow->setSubshapeSelection(true);
    ui->drawingWindow->setSetToPlane(true);
}

void OpenParEMg::on_actionDrawingPlaneSetToFaceAxis_triggered ()
{
    std::cout << "OpenParEMg::on_actionDrawingPlaneSetToFaceAxis_triggered" << std::endl; std::cout.flush();

    restrictToDrawingPlane=true;
    startOperation(true);

    skipDrawingPlaneAxisForm=false;
    on_actionFace_triggered();
    ui->drawingWindow->setSubshapeSelection(true);
}

void OpenParEMg::startPlaneSetToFace ()
{
    std::cout << "OpenParEMg::startPlaneSetToFace" << std::endl; std::cout.flush();

    // reset the selection
    on_actionShape_triggered();

    // set the plane
    TopoDS_Face selectedFace=ui->drawingWindow->getSelectedFace();
    if (selectedFace.IsNull()) return;
    ui->drawingWindow->set_gridPlane(selectedFace);
    ui->drawingWindow->clearSelected();

    // skip the form
    if (skipDrawingPlaneAxisForm) {
        gp_Pln plane=ui->drawingWindow->get_gridPlane();
        currentPrivilegedPlane=plane;
        uLocalAxis.SetXYZ(plane.XAxis().Direction().XYZ());
        finishOperation(false,10);
        return;
    }

    // restrict
    restrictToDrawingPlane=true;

    // set to select on vertices and edges
    ui->drawingWindow->Deactivate();
    ui->drawingWindow->Activate(1,Standard_False);  // vertices
    ui->drawingWindow->Activate(2,Standard_False);  // edges

    // use a form to get the local x-direction
    if (vectorInputForm) delete vectorInputForm;
    vectorInputForm=new VectorInputForm();
    vectorInputForm->set_startPoint(&startPoint);
    vectorInputForm->set_endPoint(&endPoint);
    vectorInputForm->set_drawingWindow(ui->drawingWindow);
    vectorInputForm->set_relay(relay);
    vectorInputForm->setModal(false);
    connect(this,&OpenParEMg::sendPnt,vectorInputForm,&VectorInputForm::pickVertexFinished);
    vectorInputForm->show();
}

void OpenParEMg::finishPlaneSetToFace ()
{
    std::cout << "OpenParEMg::finishPlaneSetToFace" << std::endl; std::cout.flush();

    // u vector
    uLocalAxis=endPoint.XYZ()-startPoint.XYZ();
    uLocalAxis.Normalize();

    // origin

    gp_Pln plane=ui->drawingWindow->get_gridPlane();
    plane.SetLocation(startPoint);

    // x-axis
    gp_Ax3 newAxis=plane.Position();
    newAxis.SetXDirection(uLocalAxis);
    plane.SetPosition(newAxis);

    // direction
    gp_Dir newDir=newAxis.Direction();

    // set it
    ui->drawingWindow->set_gridPlane(plane);
    currentPrivilegedPlane=plane;

    restrictToDrawingPlane=false;

    if (vectorInputForm) {vectorInputForm=nullptr;}
    finishOperation(false,11);
}

void OpenParEMg::on_actionDrawingSetPlaneToXY_triggered ()
{
    gp_Pnt origin(0,0,0);
    gp_Dir direction(0,0,1);
    uLocalAxis.SetCoord(1,0,0);
    ui->drawingWindow->set_gridPlane(origin,direction);
    ui->drawingWindow->updateViewer();
    setMenusI(76);
}

void OpenParEMg::on_actionDrawingSetPlaneToXZ_triggered ()
{
    gp_Pnt origin(0,0,0);
    gp_Dir direction(0,1,0);
    uLocalAxis.SetCoord(1,0,0);
    ui->drawingWindow->set_gridPlane(origin,direction);
    ui->drawingWindow->updateViewer();
    setMenusI(77);
}

void OpenParEMg::on_actionDrawingSetPlaneToYZ_triggered ()
{
    gp_Pnt origin(0,0,0);
    gp_Dir normal(1,0,0);
    uLocalAxis.SetCoord(0,1,0);
    ui->drawingWindow->set_gridPlane(origin,normal);
    ui->drawingWindow->updateViewer();
    setMenusI(78);
}

void OpenParEMg::cancelDraw ()
{
    std::cout << "OpenParEMg::cancelDraw" << std::endl; std::cout.flush();

    finishOperation(true,12);
}

void OpenParEMg::on_actionDrawLine_triggered ()
{
    //std::cout << "OpenParEMg::on_actionDrawLine_triggered" << std::endl; std::cout.flush();

    restrictToDrawingPlane=true;
    startOperation(true);
    activeAction=true;
    itemChangesStack.startNew();
    ui->drawingWindow->set_pickFirstVertex(true);
    clearTreeSelection();

    if (activePolywire) {
        std::cout << "ASSERT: OpenParEMg::on_actionDrawLine_triggered found activePolywire" << std::endl; std::cout.flush();
    } else {
        activePolywire=new Line();
        gp_Dir normal=ui->drawingWindow->get_normal();
        activePolywire->setNormal(normal.X(),normal.Y(),normal.Z());
        activePolywire->set_viewerContext(ui->drawingWindow->get_viewerContext());
        activePolywire->setDrawEnable(true);
    }
}

void OpenParEMg::on_actionDrawPolyline_triggered ()
{
    //std::cout << "OpenParEMg::on_actionDrawPolyline_triggered" << std::endl; std::cout.flush();

    restrictToDrawingPlane=true;
    startOperation(true);
    activeAction=true;
    itemChangesStack.startNew();
    ui->drawingWindow->set_pickFirstVertex(true);
    clearTreeSelection();

    if (activePolywire) {
        std::cout << "ASSERT: OpenParEMg::on_actionDrawPolyline_triggered found activePolywire" << std::endl; std::cout.flush();
    } else {
        activePolywire=new Polyline();
        gp_Dir normal=ui->drawingWindow->get_normal();
        activePolywire->setNormal(normal.X(),normal.Y(),normal.Z());
        activePolywire->set_viewerContext(ui->drawingWindow->get_viewerContext());
        activePolywire->setDrawEnable(true);
    }
}

void OpenParEMg::on_actionDrawPolycircle_triggered ()
{
    //std::cout << "OpenParEMg::on_actionDrawPolycircle_triggered" << std::endl; std::cout.flush();

    restrictToDrawingPlane=true;
    startOperation(true);
    activeAction=true;
    itemChangesStack.startNew();
    ui->drawingWindow->set_pickFirstVertex(true);
    clearTreeSelection();

    if (activePolywire) {
        std::cout << "ASSERT: OpenParEMg::on_actionDrawPolycircle_triggered found activePolywire" << std::endl; std::cout.flush();
    } else {
        activePolywire=new Polycircle();
        gp_Dir normal=ui->drawingWindow->get_normal();
        activePolywire->setNormal(normal.X(),normal.Y(),normal.Z());
        activePolywire->set_viewerContext(ui->drawingWindow->get_viewerContext());
        activePolywire->setDrawEnable(true);
    }
}

void OpenParEMg::on_actionDrawRectangle_triggered ()
{
    //std::cout << "OpenParEMg::on_actionDrawRectangle_triggered" << std::endl; std::cout.flush();

    restrictToDrawingPlane=true;
    startOperation(true);
    activeAction=true;
    itemChangesStack.startNew();
    ui->drawingWindow->set_pickFirstVertex(true);
    clearTreeSelection();

    if (activePolywire) {
        std::cout << "ASSERT: OpenParEMg::on_actionDrawRectangle_triggered found activePolywire" << std::endl; std::cout.flush();
    } else {
        activePolywire=new Rectangle();
        gp_Dir normal=ui->drawingWindow->get_normal();
        activePolywire->setNormal(normal.X(),normal.Y(),normal.Z());
        activePolywire->set_viewerContext(ui->drawingWindow->get_viewerContext());
        activePolywire->setDrawEnable(true);
        activePolywire->setU(uLocalAxis);
    }
}

void OpenParEMg::finishDraw ()
{
    //std::cout << "OpenParEMg::finishDraw" << std::endl; std::cout.flush();

    if (!activePolywire) return;
    activePolywire->setDrawEnable(false);

    if (isIntegrationPath) {
        std::cout << "   isIntegrationPath" << std::endl; std::cout.flush();

        // default path name

        std::string pathName="p";

        int i=1;
        while (boundaryDatabase->pathNameExists(pathName)) {
            std::string testName=pathName;
            testName.append("_").append(std::to_string(i));
            if (boundaryDatabase->pathNameExists(testName)) {i++;}
            else {pathName=testName; break;}
        }

        // path name placed in a keywordPair
        keywordPair *kwPathName=new keywordPair();
        kwPathName->set_keyword("path");
        kwPathName->set_value(pathName);
        kwPathName->set_lineNumber(0);
        kwPathName->set_loaded(true);

        // path

        Path *newPath=new Path(0,0);
        newPath->set_name(pathName);
        newPath->is_modified();
        newPath->set_normal(activePolywire->getNormal());
        newPath->addWirePoints(activePolywire->buildWire());
        newPath->create_item(ui->drawingWindow,&path);  // create item and add as child to path; creates AIS_Shape

        boundaryDatabase->push_path(newPath);

        // add new path to the drawing
        CustomTreeWidgetItem *pathItem=newPath->get_item();
        if (pathItem) {
            insertToMapActivateItem(pathItem);
            pathItem->set_itemType(4);
        }

        // see if the path is within an existing port
        Port *port=boundaryDatabase->get_matchingPort(newPath);
        if (port) newPath->set_portItem(port->get_item());

        ui->drawingWindow->selectItem(workingItem);  // select the original selected item
        insertSelectedPath();                        // relies on the newly created path already being selected

        isIntegrationPath=false;
    } else {
        CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem(0);
        ShapeData *newShapeData=new ShapeData(1,activePolywire,nullptr,activePolywire->get_AIS_Shape());
        newItem->addShapeData(newShapeData);
        newItem->setText(0,activePolywire->getName(&objectCounts));
        drawing.addChild(newItem);
        ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);
        ui->drawingWindow->activateItem(newItem);
        ui->drawingWindow->selectItem(newItem);
        ui->drawingWindow->showItem(newItem);
        itemChangesStack.add(newItem);
        activeAction=false;

        // put it on the Z-layer to get it higher selection priority
        newItem->getShape()->SetZLayer(Graphic3d_ZLayerId_Top);

        previousClickedItem=clickedItem;
        clickedItem=newItem;
    }

    restrictToDrawingPlane=false;

    activePolywire->deleteRubberband();

    Rectangle *rectangle=dynamic_cast<Rectangle *>(activePolywire);
    if (rectangle) rectangle->setIsSquare(false);

    activePolywire=nullptr;

    drawingChanged=true;
    workingItem=nullptr;
    ui->drawingWindow->removeSelectOnVertex();

    finishOperation(false,13);
}

void OpenParEMg::drawPath ()
{
    std::cout << "OpenParEMg::drawPath" << std::endl; std::cout.flush();

    // to avoid stray clicks in the selection tree
    workingItem=clickedItem;

    // enable selection on just the port
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item) {
            if (item->is_voltage() || item->is_current()) {

                // mode parent
                CustomTreeWidgetItem *modeParentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();

                // port parent
                CustomTreeWidgetItem *portParentItem=(CustomTreeWidgetItem *)modeParentItem->QTreeWidgetItem::parent();

                // port shape
                if (portParentItem->linkedItems_size() > 0) {

                    // port shape
                    CustomTreeWidgetItem *portItem=portParentItem->get_linkedItem(0);  // should just be one linked item to the port outline
                    Handle(AIS_Shape) portShape=portItem->getShape();

                    // port path
                    Path *portPath=(Path *)portItem->get_OPEMobject();

                    // select on vertices within the port path
                    std::cout << "selectOnVertex  portPath=" << portPath << std::endl; std::cout.flush();
                    ui->drawingWindow->selectOnVertex(portPath);

                    // get the normal to apply to the drawn Path
                    // Since the drawing is confined to the drawn Path, the normals will be the same.
                    activePolywire->setNormal(portPath->get_normal());
                }

                startOperation(true);
                ui->drawingWindow->set_pickFirstVertex(true);
                activePolywire->setDrawEnable(true);
            }
        }
        i++;
    }

    isIntegrationPath=true;
}

void OpenParEMg::drawLinePath ()
{
    std::cout << "OpenParEMg::drawLinePath" << std::endl; std::cout.flush();

    activePolywire=new Line();
    activePolywire->set_viewerContext(ui->drawingWindow->get_viewerContext());

    drawPath();  // executes startDrawingOperation
}

void OpenParEMg::drawPolylinePath ()
{
    std::cout << "OpenParEMg::drawPolylinePath" << std::endl; std::cout.flush();

    activePolywire=new Polyline();
    activePolywire->set_viewerContext(ui->drawingWindow->get_viewerContext());

    drawPath();  // executes startDrawingOperation
}

bool OpenParEMg::insertActionValid ()
{
    std::cout << "OpenParEMg::insertActionValid" << std::endl; std::cout.flush();

    int VIcount=0;
    CustomTreeWidgetItem *VIitem;
    int pathCount=0;

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item) {
            if (item->is_voltage() || item->is_current()) {VIitem=item; VIcount++;}
            if (item->is_path()) pathCount++;
        }
        i++;
    }

    if (VIcount != 1) return false;
    if (pathCount == 0) return false;

    // check that the paths are within the port

    CustomTreeWidgetItem *modeParentItem=(CustomTreeWidgetItem *)VIitem->QTreeWidgetItem::parent();
    CustomTreeWidgetItem *portParentItem=(CustomTreeWidgetItem *)modeParentItem->QTreeWidgetItem::parent();

    // port outline
    if (portParentItem->linkedItems_size() != 1) return false;
    CustomTreeWidgetItem *linkedItem=portParentItem->get_linkedItem(0);
    Path *portPath=static_cast<Path *>(linkedItem->get_OPEMobject());
    if (!portPath) return false;

    i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_path()) {
            Path *itemPath=static_cast<Path *>(item->get_OPEMobject());
            portPath->print("parent: ");
            itemPath->print("item: ");
            if (!portPath->is_path_inside(itemPath)) {
                return false;
            }
        }
        i++;
    }

    return true;
}

void OpenParEMg::insertSelectedPath ()
{
    std::cout << "OpenParEMg::insertSelectedPath" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item) {
            if (item->is_voltage() || item->is_current()) insertPath(item);
        }
        i++;
    }
}

void OpenParEMg::deleteLastPoint ()
{
    activePolywire->deleteLastPoint();
    activePolywire->drawRubberband();
}

void OpenParEMg::finishPolyline ()
{
    finishDraw();
}

void OpenParEMg::closePolyline ()
{
    activePolywire->close();
    finishDraw();
}

void OpenParEMg::startOperation (bool withMidPoints)
{
    //std::cout << "OpenParEMg::startOperation  operation=" << operation << std::endl; std::cout.flush();

    // disable tree
    ui->drawingItemTree->setEnabled(false);

    // disable menus
    disableMenus=true;

    // set to select on vertices and edges
    ui->drawingWindow->Deactivate();
    ui->drawingWindow->Activate(1,Standard_False);                     // vertices
    if (withMidPoints) ui->drawingWindow->Activate(2,Standard_False);  // edges, to get midpoints
}

void OpenParEMg::getCurrentMousePosition (gp_Pnt pnt)
{
    //std::cout << "OpenParEMg::getCurrentMousePosition  restrictToDrawingPlane=" << restrictToDrawingPlane << std::endl; std::cout.flush();

    // draw
    if (activePolywire && activePolywire->getDrawEnable()) {
        activePolywire->setCurrentMousePosition(pnt);
        activePolywire->drawRubberband();
    }

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item) {

            // move
            if (item->getEnableMove() && item->hasP0()) {
                item->moveAnimateShape(lastMousePosition,pnt,ui->drawingWindow->get_viewerContext());
            }

            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());

            // stretch
            if (polywire && item->getEnableStretch() && item->hasP0()) {
                Rectangle *rectangle=dynamic_cast<Rectangle *>(polywire);
                if (rectangle) {
                    if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                        rectangle->setIsSquare(true);
                    } else {
                        rectangle->setIsSquare(false);
                    }
                }
                polywire->setCurrentMousePosition(pnt);
                polywire->drawStretchRubberband();
            }
        }
        i++;
    }

    lastMousePosition=pnt;
}

void OpenParEMg::getPickedVertex (gp_Pnt pnt, bool cancel)
{
    std::cout << "OpenParEMg::getPickedVertex  cancel=" << cancel << "  restrictToDrawingPlane=" << restrictToDrawingPlane << std::endl; std::cout.flush();

    if (cancel) finishOperation(true,14);

    // restrict to the drawing plane
    if (restrictToDrawingPlane) {
        gp_Pln plane=ui->drawingWindow->get_gridPlane();
        if (plane.Distance(pnt) > 1e-12) {  // ToDo: set the tolerance to a preference setting?
            ui->drawingWindow->refreshSelectedItems();
            ui->drawingWindow->updateViewer();
            return;
        }
    }

    // draw
    if (activePolywire && activePolywire->getDrawEnable()) {
        if (activePolywire->isValidPoint(pnt,true)) {
            activePolywire->addPoint(pnt);

            Polywire *polyline=dynamic_cast<Polyline *>(activePolywire);
            if (!polyline) {
                 ui->drawingWindow->set_pickSecondVertex(true);
            }
            if (activePolywire->isFinished()) {
                ui->drawingWindow->set_pickSecondVertex(false);
                finishDraw();
            }
        }
    }

    if (vectorInputForm) vectorInputForm->pickVertexFinished(pnt);
    if (lengthInputForm) lengthInputForm->pickVertexFinished(pnt);
    if (rotateInputForm) rotateInputForm->pickVertexFinished(pnt);
    if (lengthEditForm) lengthEditForm->pickVertexFinished(pnt);
    if (lineEditForm) lineEditForm->pickVertexFinished(pnt);
    if (rectangleEditForm) rectangleEditForm->pickVertexFinished(pnt);
    if (polycircleEditForm) polycircleEditForm->pickVertexFinished(pnt);

    bool finishedMove=false;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {

        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item) {

            // move
            if (item->getEnableMove()) {
                if (!item->hasP0()) {
                    item->setP0(pnt);
                    ui->drawingWindow->hideItem(item);
                } else {
                    gp_Pnt p0=item->getP0();
                    finishMoveObject(item,p0,pnt);
                    finishedMove=true;
                }
            }

            Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
            if (polywire) {

                // stretch
                if (item->getEnableStretch()) {

                    Rectangle *rectangle=dynamic_cast<Rectangle *>(polywire);
                    if (rectangle) {
                        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                            rectangle->setIsSquare(true);
                        } else {
                            rectangle->setIsSquare(false);
                        }
                    }

                    if (!item->hasP0()) {
                        item->setP0(pnt);
                        polywire->setEditIndex(pnt);
                        polywire->setCurrentMousePosition(pnt);
                        polywire->drawStretchRubberband();

                        // switch to allowing selection on midpoints
                        startOperation(true);

                        ui->drawingWindow->hideItem(item);
                        ui->drawingWindow->updateViewer();
                    } else {
                        if (polywire->isPointOnPlane(pnt) && polywire->isValidInsertPoint(pnt)) {
                            item->setP1(pnt);
                            polywire->setCurrentMousePosition(pnt);
                            polywire->drawStretchRubberband();
                            finishStretchObject(item);
                        }
                    }
                }

                // delete point
                if (item->getEnableDeletePoint()) {
                    if (polywire->isPointOnPlane(pnt)) {
                        item->setP0(pnt);
                        finishDeletePoint(item);
                    }
                }

                // insert point
                if (item->getEnableInsertPoint()) {
                    if (polywire->isPointOnPlane(pnt)) {
                        item->setP0(pnt);
                        finishInsertPoint(item);
                    }
                }
            }
        }
        i++;
    }

    if (finishedMove) {
        finishOperation(false,15);
    }

    lastMousePosition=pnt;
}

void OpenParEMg::finishOperation (bool cancel, int source)
{
    //std::cout << "OpenParEMg::finishOperation  cancel=" << cancel << "  source=" << source << std::endl; std::cout.flush();

    if (cancel) {

        if (activeAction) {
            itemChangesStack.pop_back();
            activeAction=false;
        }

        if (activePolywire && activePolywire->getDrawEnable()) {
            activePolywire->setDrawEnable(false);
            activePolywire->deleteRubberband();
            // xxx
            ui->drawingWindow->finishPickVertex(true);
            ui->drawingWindow->updateViewer();
            delete activePolywire;
            activePolywire=nullptr;
        }

        long unsigned int i=0;
        while (i < ui->drawingWindow->get_selectedItems_size()) {
            CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
            if (item) {

                // move
                if (item->getEnableMove()) {
                    item->unsetAnimate(ui->drawingWindow->get_viewerContext());
                    item->setEnableMove(false);
                    ui->drawingWindow->showItem(item);
                }

                Polywire *polywire=static_cast<Polywire *>(item->getPolywire());
                if (polywire) {

                    // stretch
                    if (item->getEnableStretch()) {

                        Rectangle *rectangle=dynamic_cast<Rectangle *>(polywire);
                        if (rectangle) {
                            if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                                rectangle->setIsSquare(true);
                            } else {
                                rectangle->setIsSquare(false);
                            }
                        }

                        item->setEnableStretch(false);
                        polywire->deleteRubberband();
                        ui->drawingWindow->showItem(item);
                        ui->drawingWindow->set_gridPlane(currentPrivilegedPlane);
                    }

                    // delete point
                    if (item->getEnableDeletePoint()) {
                        item->setEnableDeletePoint(false);
                        //polywire->deleteRubberband();
                        ui->drawingWindow->showItem(item);
                        ui->drawingWindow->set_gridPlane(currentPrivilegedPlane);
                    }
                }
            }
            i++;
        }

        if (vectorInputForm) {
            ui->drawingWindow->set_gridPlane(currentPrivilegedPlane);
        }

        restrictToDrawingPlane=false;

    } else {
        if (lengthInputForm) finishExtrudePolywire(false);
        if (vectorInputForm) finishPlaneSetToFace();
        if (rotateInputForm) finishRotateObject();
        if (lineEditForm || rectangleEditForm || polycircleEditForm || lengthEditForm) finishEditObject(false);
    }

    ui->drawingWindow->set_pickFirstVertex(false);
    ui->drawingWindow->set_pickSecondVertex(false);

    if (lengthInputForm) {lengthInputForm=nullptr;}
    if (vectorInputForm) {vectorInputForm=nullptr;}
    if (lengthEditForm) {lengthEditForm=nullptr;}
    if (lineEditForm) {lineEditForm=nullptr;}
    if (rectangleEditForm) {rectangleEditForm=nullptr;}
    if (polycircleEditForm) {polycircleEditForm=nullptr;}
    if (rotateInputForm) {rotateInputForm=nullptr;}

    // enable tree
    ui->drawingItemTree->setEnabled(true);

    // enable menus
    disableMenus=false;

    // restore selection
    restoreSelection();
    ui->drawingWindow->setSubshapeSelection(false);
    ui->drawingWindow->setSetToPlane(false);
    isIntegrationPath=false;

    // reset the vertex symbol
    //ui->drawingWindow->set_pickFirstVertex(false);  // also sets the second vertex

    // refresh the selection to enable further operations on the selected items
    ui->drawingWindow->refreshSelectedItems();

    ui->drawingWindow->updateViewer();
    setMenusI(0);
}

void OpenParEMg::undoItem (CustomTreeWidgetItem *item)
{
    std::cout << "OpenParEMg::undoItem  item=" << item << std::endl; std::cout.flush();

    if (!item) return;

    // must have ShapeData
    ShapeData *shapeData=item->getShapeData();
    if (!shapeData) return;

    // save the children for redo
    //xxx
    // item->clearChildren();
    // int i=0;
    // while (i < item->childCount()) {
    //     CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
    //     item->push_child(child);
    //     i++;
    // }

    // go through the cases
    if (shapeData->isNoop()) {
        return;
    } else if (shapeData->isCreate()) {

        // remove

        CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
        if (!parentItem) {
            return;
        }

        // collect children
        std::vector<CustomTreeWidgetItem *> childList;
        int i=0;
        while (i < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
            childList.push_back(child);
            i++;
        }

        // promote children
        QList<QTreeWidgetItem*> children=item->takeChildren();
        int index=parentItem->indexOfChild(item);
        parentItem->insertChildren(index,children);

        // remove the item
        ui->drawingWindow->hideItem(item);
        ui->drawingWindow->removeItemFromMap(item);
        ui->drawingWindow->deleteShape(item->getShape());
        parentItem->removeChild(item);

        // process the children
        i=0;
        while (i < childList.size()) {
            ui->drawingWindow->showItem(childList[i]);
            childList[i]->decrease_depth();
            i++;
        }

        item->undo();
    } else if (shapeData->isEdit()) {

        ui->drawingWindow->hideItem(item);
        ui->drawingWindow->removeItemFromMap(item);
        ui->drawingWindow->deleteShape(item->getShape());

        item->undo();

        ShapeData *shapeData=item->getShapeData();
        if (shapeData) {
            Process *process=static_cast<Process *>(shapeData->getProcess());
            if (process) {
                int i=0;
                while (i < item->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                    undoItem(child);
                    ui->drawingWindow->hideItem(child);
                    i++;
                }
            } else {
                reprocess(item);
            }
        }

        //ui->drawingWindow->showItem(item);
    } else if (shapeData->isDelete()) {

        // recreate

        CustomTreeWidgetItem *parentItem=&drawing;
        parentItem->addChild(item);

        long unsigned int i=0;
        while (i < item->getChildrenSize()) {
            CustomTreeWidgetItem *child=item->getChild(i);
            if (child) {
                int index=parentItem->indexOfChild(child);
                parentItem->takeChild(index);
                item->addChild(child);
                child->increase_depth();
            }
            i++;
        }

        item->undo();

        reprocess(item);
        ui->drawingWindow->showItem(item);
    }

    // find and show the top-level item
    // ui->drawingWindow->hideItem(item);
    // CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    // if (parentItem) {
    //     while (!parentItem->is_rootDrawing()) {
    //         item=parentItem;
    //         parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    //         if (!parentItem) break;
    //     }
    // }
    // ui->drawingWindow->showItem(item);
    // ui->drawingWindow->activateItem(item);
}

void OpenParEMg::redoItem (CustomTreeWidgetItem *item)
{
    std::cout << "OpenParEMg::redoItem  item=" << item << std::endl; std::cout.flush();

    if (!item) return;

    // must have ShapeData
    ShapeData *shapeData=item->getShapeData();
    if (!shapeData) return;

    ShapeData *next=shapeData->getNext();
    if (!next) return;

    // go through the cases
    if (next->isNoop()) {
        return;
    } else if (next->isCreate()) {
        item->redo();

        // create

        CustomTreeWidgetItem *parentItem=&drawing;
        parentItem->addChild(item);

        long unsigned int i=0;
        while (i < item->getChildrenSize()) {
            CustomTreeWidgetItem *child=item->getChild(i);
            if (child) {
                int index=parentItem->indexOfChild(child);
                parentItem->takeChild(index);
                item->addChild(child);
                child->increase_depth();
            }
            i++;
        }

        reprocess(item);
        //ui->drawingWindow->showItem(item);
    } else if (next->isEdit()) {
        ui->drawingWindow->hideItem(item);
        ui->drawingWindow->removeItemFromMap(item);
        ui->drawingWindow->deleteShape(item->getShape());

        item->redo();

        ShapeData *shapeData=item->getShapeData();
        if (shapeData) {
            Process *process=static_cast<Process *>(shapeData->getProcess());
            if (process) {
                int i=0;
                while (i < item->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                    redoItem(child);
                    i++;
                }
            }
        }

        reprocess(item);
        insertToMapActivateItem(item);
    } else if (next->isDelete()) {

        // remove

        ui->drawingWindow->hideItem(item);
        ui->drawingWindow->removeItemFromMap(item);
        ui->drawingWindow->deleteShape(item->getShape());

        item->redo();

        CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
        if (!parentItem) {
            return;
        }

        // promote children
        QList<QTreeWidgetItem*> children=item->takeChildren();
        int index=parentItem->indexOfChild(item);
        parentItem->insertChildren(index,children);

        parentItem->removeChild(item);

        // process the children
        long unsigned int i=0;
        while (i < item->getChildrenSize()) {
            ui->drawingWindow->showItem(item->getChild(i));
            item->getChild(i)->decrease_depth();
            i++;
        }
    }

    // find and show the top-level item
    // ui->drawingWindow->hideItem(item);
    // CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    // if (parentItem) {
    //     while (!parentItem->is_rootDrawing()) {
    //         item=parentItem;
    //         parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    //         if (!parentItem) break;
    //     }
    // }
    // ui->drawingWindow->showItem(item);
    // ui->drawingWindow->activateItem(item);
}

void OpenParEMg::on_actionUndo_triggered ()
{
    //std::cout << "OpenParEMg::on_actionUndo_triggered" << std::endl; std::cout.flush();

    itemChangesStack.readNew();
    CustomTreeWidgetItem *item=itemChangesStack.getItem();
    while (item) {
        undoItem(item);
        item=itemChangesStack.getItem();
    }

    itemChangesStack.undo();
    ui->drawingWindow->updateViewer();
    setMenusI(3000);
}

void OpenParEMg::on_actionRedo_triggered ()
{
    //std::cout << "OpenParEMg::on_actionRedo_triggered" << std::endl; std::cout.flush();

    itemChangesStack.redo();
    itemChangesStack.readNew();
    CustomTreeWidgetItem *item=itemChangesStack.getItem();
    while (item) {
        redoItem(item);
        item=itemChangesStack.getItem();
    }

    ui->drawingWindow->updateViewer();
    setMenusI(3000);
}

