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


#include <AIS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <TopoDS_Shape.hxx>



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
    QActionList.push_back(removeAction);
    QActionList.push_back(assignAction);
    QActionList.push_back(insertAction);
    QActionList.push_back(renameAction);
    QActionList.push_back(expandAllAction);
    QActionList.push_back(collapseAllAction);
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
    QActionList.push_back(closeAction);
    QActionList.push_back(extrudeAction);
    QActionList.push_back(mergeAction);
    QActionList.push_back(subtractAction);
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
    pointCount=0; curveCount=0; surfaceCount=0; volumeCount=0;

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

    uLocalAxis.SetCoord(1,0,0);

    restrictToDrawingPlane=false;

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
    std::cout << "OpenParEMg::setMenusI  place=" << placeIndex << std::endl; std::cout.flush();

    bool boundaryDatabaseChanged=boundaryDatabase->is_modified();
    ui->drawingWindow->compactSelectedItems();
    ui->drawingWindow->compactVisibleItems();

    //printLockouts();
    debugPrintStats(0);
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
        if (projectChanged || boundaryDatabaseChanged || brepChanged || meshChanged) {
            if (strcmp(projData.project_name,"") == 0) ui->actionSave->setEnabled(false);
            else ui->actionSave->setEnabled(true);
        }
        ui->actionSaveAs->setEnabled(true);
        ui->actionClose->setEnabled(true);
        ui->actionExit->setEnabled(true);
        ui->actionSelectMaterialsDatabase->setEnabled(true);
        ui->actionMaterialsOptions->setEnabled(true);
        ui->actionUnselectAll->setEnabled(true);
        ui->actionShowAll->setEnabled(true);
        ui->actionHideAll->setEnabled(true);
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

        if (brepFileLoaded) {
            ui->actionImportBrep->setEnabled(false);
            ui->actionImportStep->setEnabled(false);
            ui->actionExportStep->setEnabled(true);

            ui->actionFitSelected->setEnabled(true);
            ui->actionFitAll->setEnabled(true);
            ui->actionMenuSelection->setEnabled(true);
            ui->actionSelectWithBox->setEnabled(true);
            ui->actionWireframe->setEnabled(true);

            ui->actionMeshGenerate->setEnabled(true);
        } else {
            ui->actionImportBrep->setEnabled(true);
            ui->actionImportStep->setEnabled(true);
            ui->actionExportStep->setEnabled(false);

            ui->actionFitSelected->setEnabled(false);
            ui->actionFitAll->setEnabled(false);
            ui->actionMenuSelection->setEnabled(false);
            ui->actionSelectWithBox->setEnabled(false);
            ui->actionUnselectAll->setEnabled(false);
            ui->actionHideAll->setEnabled(false);
            ui->actionWireframe->setEnabled(false);

            ui->actionMeshGenerate->setEnabled(false);
        }

        if (meshFileLoaded) {
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

        ui->actionDrawingPlaneSetToFace->setEnabled(false);
        if (brepFileLoaded) {
            ui->actionDrawingPlaneSetToFace->setEnabled(true);
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

        showAction->setEnabled(false);
        hideAction->setEnabled(false);
        selectAllAction->setEnabled(false);
        if (clickedItem->childCount() > 0) {
            showAction->setEnabled(isRootDrawingValidShow());
            hideAction->setEnabled(isRootDrawingValidHide());
            selectAllAction->setEnabled(isRootDrawingValidSelectAll());
        }

        menu.addAction(showAction);
        menu.addAction(hideAction);
        menu.addAction(selectAllAction);
    }

    if (clickedItem->is_drawing()) {
        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        //QAction *selectAction=new QAction("Select",this);
        unselectAction=new QAction("Unselect",this);
        deleteAction=new QAction("Delete",this);
        assignAction=new QAction("Assign Material");
        createPortAction=new QAction("Create Port");
        createPortAction->setToolTip("Copy the selected face and create a port.");
        createPathAction=new QAction("Create Path");
        createPathAction->setToolTip("Copy the selected face and create a path.");

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showDrawingItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideDrawingItems);
        //connect(selectAction, &QAction::triggered, this, &OpenParEMg::selectItems);
        connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectDrawingItems);
        connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deleteDrawingItems);
        connect(assignAction, &QAction::triggered, this, &OpenParEMg::assignMaterial);
        connect(createPortAction, &QAction::triggered, this, &OpenParEMg::createPort);
        connect(createPathAction, &QAction::triggered, this, &OpenParEMg::createPath);

        showAction->setEnabled(false);
        hideAction->setEnabled(false);
        unselectAction->setEnabled(false);
        deleteAction->setEnabled(false);
        assignAction->setEnabled(false);
        createPortAction->setEnabled(false);
        createPathAction->setEnabled(false);
        if (!clickedItem->is_root()) {
            showAction->setEnabled(isDrawingValidShow());
            hideAction->setEnabled(ui->drawingWindow->isValidHide());
            unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
            deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

            assignAction->setEnabled(false);
            if (clickedItem->is_solid()) assignAction->setEnabled(true);

            createPortAction->setEnabled(false);
            if (ui->drawingWindow->numberDrawingFaceSelected() == 1) {createPortAction->setEnabled(true);}

            createPathAction->setEnabled(false);
            if (ui->drawingWindow->numberDrawingFaceSelected() > 0) {createPathAction->setEnabled(true);}
        }

        menu.addAction(showAction);
        menu.addAction(hideAction);
        //menu.addAction(selectAction);
        menu.addAction(unselectAction);
        menu.addAction(deleteAction);
        menu.addAction(assignAction);
        menu.addAction(createPortAction);
        menu.addAction(createPathAction);
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

        showAction->setEnabled(false);
        hideAction->setEnabled(false);
        expandAllAction->setEnabled(false);
        collapseAllAction->setEnabled(false);
        if (clickedItem->childCount() > 0) {
            showAction->setEnabled(rootPathValidShow());
            hideAction->setEnabled(rootPathValidHide());
            expandAllAction->setEnabled(true);
            collapseAllAction->setEnabled(true);
        }

        menu.addAction(showAction);
        menu.addAction(hideAction);
        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_path()) {

        renameAction=new QAction("Rename",this);
        deleteAction=new QAction("Delete",this);
        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);

        connect(renameAction, &QAction::triggered, this, &OpenParEMg::renamePathItems);
        connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deletePathItems);
        connect(showAction, &QAction::triggered, this, &OpenParEMg::showPathItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hidePathItems);

        renameAction->setEnabled(false);
        deleteAction->setEnabled(false);
        showAction->setEnabled(false);
        hideAction->setEnabled(false);
        if (clickedItem->childCount() > 0) {
            renameAction->setEnabled(false);
            if (ui->drawingWindow->get_pathSelectedCount() == 1) renameAction->setEnabled(true);

            deleteAction->setEnabled(isPathValidDelete());
            showAction->setEnabled(ui->drawingWindow->isValidShow());
            hideAction->setEnabled(ui->drawingWindow->isValidHide());
        }

        menu.addAction(renameAction);
        menu.addAction(deleteAction);
        menu.addAction(showAction);
        menu.addAction(hideAction);
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

        showAction->setEnabled(false);
        hideAction->setEnabled(false);
        expandAllAction->setEnabled(false);
        collapseAllAction->setEnabled(false);
        if (clickedItem->childCount() > 0) {
            showAction->setEnabled(ui->drawingWindow->isValidShow());
            hideAction->setEnabled(ui->drawingWindow->isValidHide());
            expandAllAction->setEnabled(true);
            collapseAllAction->setEnabled(true);
        }

        menu.addAction(showAction);
        menu.addAction(hideAction);
        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
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

        showAction->setEnabled(false);
        hideAction->setEnabled(false);
        unselectAction->setEnabled(false);
        renameAction->setEnabled(false);
        insertAction->setEnabled(false);
        deleteAction->setEnabled(false);
        expandAllAction->setEnabled(false);
        collapseAllAction->setEnabled(false);
        if (clickedItem->childCount() > 0) {
            showAction->setEnabled(ui->drawingWindow->isValidShow());
            hideAction->setEnabled(ui->drawingWindow->isValidHide());
            unselectAction->setEnabled(ui->drawingWindow->hasPortSelectedItems());

            renameAction->setEnabled(false);
            if (ui->drawingWindow->get_portSelectedCount() == 1) renameAction->setEnabled(true);

            insertAction->setEnabled(true);
            deleteAction->setEnabled(true);
            expandAllAction->setEnabled(true);
            collapseAllAction->setEnabled(true);
        }

        menu.addAction(showAction);
        menu.addAction(hideAction);
        menu.addAction(unselectAction);
        menu.addAction(renameAction);
        menu.addAction(insertAction);
        menu.addAction(deleteAction);
        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
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

        showAction->setEnabled(false);
        hideAction->setEnabled(false);
        expandAllAction->setEnabled(false);
        collapseAllAction->setEnabled(false);
        if (clickedItem->childCount() > 0) {
            showAction->setEnabled(rootMeshValidShow());
            hideAction->setEnabled(rootMeshValidHide());
            expandAllAction->setEnabled(true);
            collapseAllAction->setEnabled(true);
        }

        menu.addAction(showAction);
        menu.addAction(hideAction);
        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_mesh()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showMeshItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideMeshItems);

        showAction->setEnabled(ui->drawingWindow->isValidShow());
        hideAction->setEnabled(ui->drawingWindow->isValidHide());

        menu.addAction(showAction);
        menu.addAction(hideAction);
    }

    if (clickedItem->is_sportLabel()) {
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        expandAllAction->setEnabled(false);
        collapseAllAction->setEnabled(false);
        if (clickedItem->childCount() > 0) {
            expandAllAction->setEnabled(true);
            collapseAllAction->setEnabled(true);
        }

        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
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

        showAction->setEnabled(false);
        hideAction->setEnabled(false);
        renameAction->setEnabled(false);
        deleteAction->setEnabled(false);
        expandAllAction->setEnabled(false);
        collapseAllAction->setEnabled(false);
        if (clickedItem->childCount() > 0) {
            showAction->setEnabled(ui->drawingWindow->isNetValidShow());
            hideAction->setEnabled(ui->drawingWindow->isNetValidHide());

            renameAction->setEnabled(false);
            if (ui->drawingWindow->get_selectedItems_count() == 1) renameAction->setEnabled(true);

            deleteAction->setEnabled(deleteSportValid());
            expandAllAction->setEnabled(true);
            collapseAllAction->setEnabled(true);
        }

        menu.addAction(showAction);
        menu.addAction(hideAction);
        menu.addAction(renameAction);
        menu.addAction(deleteAction);
        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
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

        showAction->setEnabled(false);
        hideAction->setEnabled(false);
        drawPathAction->setEnabled(true);
        drawPolylineAction->setEnabled(true);
        insertAction->setEnabled(true);
        expandAllAction->setEnabled(false);
        collapseAllAction->setEnabled(false);
        if (clickedItem->childCount() > 0) {
            showAction->setEnabled(ui->drawingWindow->isValidShow());
            hideAction->setEnabled(ui->drawingWindow->isValidHide());

            drawPathAction->setEnabled(false);
            if (ui->drawingWindow->get_selectedItems_count() == 1 && clickedItem->foreground(0) == Qt::black) drawPathAction->setEnabled(true);

            drawPolylineAction->setEnabled(false);
            if (ui->drawingWindow->get_selectedItems_count() == 1 && clickedItem->foreground(0) == Qt::black) drawPolylineAction->setEnabled(true);

            insertAction->setEnabled(insertActionValid());
            expandAllAction->setEnabled(true);
            collapseAllAction->setEnabled(true);
        }

        menu.addAction(showAction);
        menu.addAction(hideAction);
        menu.addAction(drawPathAction);
        menu.addAction(drawPolylineAction);
        menu.addAction(insertAction);
        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_integrationPathSegment()) {

        removeAction=new QAction("Remove",this);
        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);

        connect(removeAction, &QAction::triggered, this, &OpenParEMg::removeIntegrationPathItems);
        connect(showAction, &QAction::triggered, this, &OpenParEMg::showIntegrationPathItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideIntegrationPathItems);

        removeAction->setEnabled(true);
        showAction->setEnabled(ui->drawingWindow->isValidShow());
        hideAction->setEnabled(ui->drawingWindow->isValidHide());

        menu.addAction(removeAction);
        menu.addAction(showAction);
        menu.addAction(hideAction);
    }

    if (clickedItem->is_scale()) {
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        expandAllAction->setEnabled(false);
        collapseAllAction->setEnabled(false);
        if (clickedItem->childCount() > 0) {
            expandAllAction->setEnabled(true);
            collapseAllAction->setEnabled(true);
        }

        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
    }

    menu.exec(ui->drawingItemTree->mapToGlobal(pnt));

    freeQActionList();
}

void OpenParEMg::drawingWindowContextMenu_triggered(const QPoint& pnt)
{
    //std::cout << "OpenParEMg::drawingWindowContextMenu_triggered" << std::endl; std::cout.flush();

    if (activePolywire) {

        if (dynamic_cast<Line *>(activePolywire)) {
            cancelAction=new QAction("Cancel");
            connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelDraw);

            QMenu menu(this);
            menu.addAction(cancelAction);

            menu.exec(ui->drawingWindow->mapToGlobal(pnt));
            freeQActionList();
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

            QMenu menu(this);
            menu.addAction(deleteLastPointAction);
            menu.addAction(doneAction);
            menu.addAction(closeAction);
            menu.addAction(cancelAction);

            menu.exec(ui->drawingWindow->mapToGlobal(pnt));

            freeQActionList();
        } else if (dynamic_cast<Rectangle *>(activePolywire)){
            cancelAction=new QAction("Cancel");
            connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelDraw);

            QMenu menu(this);
            menu.addAction(cancelAction);

            menu.exec(ui->drawingWindow->mapToGlobal(pnt));
            freeQActionList();
        } else if (dynamic_cast<Polycircle *>(activePolywire)) {
            cancelAction=new QAction("Cancel");
            connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelDraw);

            QMenu menu(this);
            menu.addAction(cancelAction);

            menu.exec(ui->drawingWindow->mapToGlobal(pnt));
            freeQActionList();
        }
    } else {
        if (ui->drawingWindow->get_NbSelected()) {

            showAction=new QAction("Show");
            hideAction=new QAction("Hide");
            editAction=new QAction("Edit");
            moveAction=new QAction("Move");
            stretchAction=new QAction("Stretch");
            deletePointAction=new QAction("Delete Point");
            insertPointAction=new QAction("InsertPoint");
            rotateAction=new QAction("Rotate");
            unselectAction=new QAction("Unselect");
            copyAction=new QAction("Copy");
            deleteAction=new QAction("Delete");
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

            connect(showAction, &QAction::triggered, this, &OpenParEMg::showDrawingItems);
            connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideDrawingItems);
            connect(editAction, &QAction::triggered, this, &OpenParEMg::editObject);
            connect(moveAction, &QAction::triggered, this, &OpenParEMg::moveObject);
            connect(stretchAction, &QAction::triggered, this, &OpenParEMg::stretchObject);
            connect(deletePointAction, &QAction::triggered, this, &OpenParEMg::deletePoint);
            connect(insertPointAction, &QAction::triggered, this, &OpenParEMg::insertPoint);
            connect(rotateAction, &QAction::triggered, this, &OpenParEMg::rotateObject);
            connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectDrawingItems);
            connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deleteDrawingItems);
            connect(copyAction, &QAction::triggered, this, &OpenParEMg::copyDrawingItems);
            connect(createPortAction, &QAction::triggered, this, &OpenParEMg::createPort);
            connect(createPathAction, &QAction::triggered, this, &OpenParEMg::createPath);
            connect(extrudeAction, &QAction::triggered, this, &OpenParEMg::extrudePolywire);
            connect(mergeAction, &QAction::triggered, this, &OpenParEMg::mergeSolids);
            connect(subtractAction, &QAction::triggered, this, &OpenParEMg::subtractSolids);


            QMenu menu(this);
            menu.addAction(showAction);
            menu.addAction(hideAction);
            menu.addAction(unselectAction);
            menu.addAction(copyAction);
            menu.addAction(deleteAction);
            //menu.addAction(createPortAction);
            //menu.addAction(createPathAction);

            QMenu setup("Setup");
            setup.addAction(createPortAction);
            setup.addAction(createPathAction);
            menu.addMenu(&setup);

            QMenu modify("Modify");
            modify.addAction(editAction);
            modify.addAction(moveAction);
            modify.addAction(stretchAction);
            modify.addAction(insertPointAction);
            modify.addAction(deletePointAction);
            modify.addAction(rotateAction);
            modify.addAction(extrudeAction);
            modify.addAction(mergeAction);
            modify.addAction(subtractAction);
            menu.addMenu(&modify);

            createPortAction->setEnabled(false);
            if (ui->drawingWindow->numberDrawingFaceSelected() == 1) {
                createPortAction->setEnabled(true);
            }

            createPathAction->setEnabled(false);
            if (ui->drawingWindow->numberDrawingFaceSelected() > 0) {createPathAction->setEnabled(true);}

            editAction->setEnabled(isValidObjectEdit());     // single object
            moveAction->setEnabled(true);
            stretchAction->setEnabled(isValidObjectStretch());   // single object
            deletePointAction->setEnabled(isValidDeletePoint()); // single object
            insertPointAction->setEnabled(isValidInsertPoint());    // single object
            rotateAction->setEnabled(true);
            extrudeAction->setEnabled(isValidExtrudePolywire());
            mergeAction->setEnabled(isValidMergeSolids());
            subtractAction->setEnabled(isValidSubtractSolids());
            copyAction->setEnabled(isValidCopy());
            deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

            menu.exec(ui->drawingWindow->mapToGlobal(pnt));

            freeQActionList();
        }
    }
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
            ui->drawingWindow->unselectItem(item,i);
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
            //path.removeChild(item);  // ToDo:: fix since now item=nullptr
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
        if (item && item->is_path()) {
            if (item->linkedItems_size() > 0) return false;
        }
        i++;
    }
    return true;
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


//xxx ToDo: item is shadowed
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
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_path()) {
            Path *path=(Path *)item->get_OPEMobject();
            if (path) {
                pathsToAdd.push_back(path);
                pathItemList.push_back(item);
            }
        }
        i++;
    }

    // check that the selected paths can be used on the port

    CustomTreeWidgetItem *modeParentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    CustomTreeWidgetItem *portParentItem=(CustomTreeWidgetItem *)modeParentItem->QTreeWidgetItem::parent();

    if (!modeParentItem->get_OPEMobject()) {
        std::cout << "ASSERT: OpenParEMg::insertPath found mode item without attached object." << std::endl; std::cout.flush();
        return;
    }

    long unsigned int j=0;
    while (j < pathsToAdd.size()) {
        if (pathsToAdd[j]->get_portItem() != portParentItem) {
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
    QString net=renameEdit->text();
    if (originalText.compare(net) != 0) {
        if (renameItem->is_path()) {

            // item itself is now changed

            // change the names of the linked items
            long unsigned int i=0;
            while (i < renameItem->linkedItems_size()) {
                CustomTreeWidgetItem *item=renameItem->get_linkedItem(i);
                if (item->is_integrationPathSegment()) item->setText(0,net);
                i++;
            }

            // change the database
            boundaryDatabase->renamePath(originalText.toStdString(),net.toStdString());
        }

        if (renameItem->is_port()) {
            Port *port=(Port *)renameItem->get_OPEMobject();
            if (port) port->set_name(net.toStdString());
        }

        if (renameItem->is_sport()) {
            Mode *mode=(Mode *)renameItem->get_OPEMobject();
            if (mode) mode->set_net(net.toStdString());
        }
    }

    // replace
    ui->drawingItemTree->removeItemWidget(renameItem,0);
    renameItem->setText(0,net);

    // update
    bool isExpanded=renameItem->isExpanded();
    renameItem->setExpanded(false);
    renameItem->setExpanded(true);
    if (!isExpanded) renameItem->setExpanded(false);

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

void OpenParEMg::deleteDrawingItems()
{
    //std::cout << "OpenParEMg::deleteDrawingItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {

            // parentItem
            CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();

            if (parentItem) {
                int insertIndex=parentItem->indexOfChild(item);

                // move children to parent
                while (item->childCount() > 0) {
                    CustomTreeWidgetItem* child=(CustomTreeWidgetItem *)item->takeChild(0);
                    parentItem->insertChild(insertIndex++,child);
                    ui->drawingWindow->showItem(child);
                    //ui->drawingItemTree->setCurrentItem(nullptr);
                }

                parentItem->removeChild(item);
            }

            // remove the item
            ui->drawingWindow->deleteItem(item);

            // reset the top-level compound
            reprocess(&drawing);

            brepChanged=true;
        }
        i++;
    }

    // see if everything has been deleted
    if (drawing.childCount() == 0) resetDrawing();

    restoreSelection();
    clickedItem=nullptr;
    previousClickedItem=nullptr;

    ui->drawingWindow->updateViewer();
    setMenusI(21);
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
        if (item && item->is_voltage() || item->is_current()) {
            ui->drawingWindow->showItem(item);
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
        if (item && item->is_voltage() || item->is_current()) {
            ui->drawingWindow->hideItem(item);
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
    std::cout << "OpenParEMg::createPath" << std::endl; std::cout.flush();

    int faceCount=0;
    while (faceCount < ui->drawingWindow->numberDrawingFaceSelected()) {

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
        newPath->addFacePoints(ui->drawingWindow->get_selectedFace(faceCount),true,true);
        newPath->create_item(ui->drawingWindow,&path);  // create item and add as child to path; creates AIS_Shape

        boundaryDatabase->push_path(newPath);

        // add new path to the drawing
        CustomTreeWidgetItem *item=newPath->get_item();
        if (item) {
            addItemWithShape(item);

            long unsigned int j=0;
            while (j < item->get_arrowHeads_size()) {
                //ui->drawingWindow->displayShape(item->get_arrowHead(j),item->get_displayMode(),item->get_selectionMode());
                ui->drawingWindow->displayShape(item->get_arrowHead(j));
                ui->drawingWindow->insertItemToMap(item->get_arrowHead(j),item);
                j++;
            }
        }

        // see if the path is within an existing port
        Port *port=boundaryDatabase->get_matchingPort(newPath);
        if (port) newPath->set_portItem(port->get_item());

        faceCount++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(36);
}

void OpenParEMg::replaceItemShape (CustomTreeWidgetItem *item, TopoDS_Shape &shape, int i)
{
    //std:: cout << "OpenParEMg::replaceItemShape  item=" << item << "  place=" << i << std::endl; std::cout.flush();

    if (!item) return;

    // save for later restoration since the operations will modify the visible item list
    std::vector<CustomTreeWidgetItem *> displayedItems=ui->drawingWindow->getVisibleDrawingItems();

    // remove old shape
    if (!item->get_AIS_Shape().IsNull()) {
        ui->drawingWindow->hideItem(item);
        ui->drawingWindow->removeItemFromMap(item);
        ui->drawingWindow->deleteShape(item->get_AIS_Shape());  // lose selection
    }

    // refresh selection: Processing on rootDrawing deselects all but 1 item, so reselect here.
    if (item->is_rootDrawing()) {
        long unsigned int i=0;
        while (i < ui->drawingWindow->get_selectedItems_size()) {
            CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
            if (item) ui->drawingWindow->refreshSelectedItem(item);
            i++;
        }
    }

    // install new shape
    Handle(AIS_Shape) AISshape=new AIS_Shape(shape);
    // must activate for selection since SetAutoActivateSelection is set to false in CustomOpenGLWidget.cpp
    if (!item->is_rootDrawing()) ui->drawingWindow->activateSelectShape(AISshape);
    item->set_AIS_Shape(AISshape);
    ui->drawingWindow->insertItemToMap(AISshape,item);

    // redisplay
    if (item->is_rootDrawing()) {
        long unsigned int i=0;
        while (i < displayedItems.size()) {
            ui->drawingWindow->showItem(displayedItems[i]);
            i++;
        }
    }
}

bool OpenParEMg::isValidExtrudePolywire ()
{
    //std::cout << "OpenParEMg::isValidExtrudePolywire" << std::endl; std::cout.flush();

    int polywireCount=0;
    QList<QTreeWidgetItem*> items=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < items.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)items[i];
        if (item && item->is_drawing()) {
            CustomTreeWidgetItem *parent=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            if (parent && parent->is_rootDrawing()) {
                Polywire *polywire=item->get_Polywire();
                if (polywire) polywireCount++;
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

    if (lengthInputForm) delete lengthInputForm;
    lengthInputForm=new LengthInputForm();
    lengthInputForm->set_drawingWindow(ui->drawingWindow);
    lengthInputForm->set_relay(relay);
    lengthInputForm->setModal(false);
    connect(this,&OpenParEMg::sendPnt,lengthInputForm,&LengthInputForm::pickVertexFinished);
    lengthInputForm->show();
}

void OpenParEMg::finishExtrudePolywire (double length, bool cancel)
{
    //std::cout << "OpenParEMg::finishExtrudePolywire" << std::endl; std::cout.flush();

    if (!cancel && abs(length) > 1e-12) {

        int i=0;
        while (i < ui->drawingWindow->get_selectedItems_size()) {
            CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);

            if (item && !item->get_AIS_Shape().IsNull()) {
                Polywire *polywire=item->get_Polywire();
                if (polywire) {

                    // scale it
                    gp_Vec scaledVec=gp_Vec(polywire->getNormal())*length;

                    // extrude it
                    BRepPrimAPI_MakePrism aPrism(item->get_AIS_Shape()->Shape(),scaledVec);
                    if (aPrism.IsDone()) {

                        // add it
                        CustomTreeWidgetItem *newItem=addItemShape(aPrism,&drawing);  // inserts to item map

                        // define the process
                        Extrude *extrude=new Extrude();
                        extrude->set_length(length);
                        newItem->set_Process(extrude);

                        // save the process
                        extrude=nullptr;

                        // move the object
                        drawing.removeChild(item);
                        newItem->addChild(item);

                        // hide/show
                        ui->drawingWindow->hideItem(item);
                        ui->drawingWindow->unselectItem(item,i);  // changes list

                        ui->drawingWindow->showItem(newItem);
                        ui->drawingWindow->selectItem(newItem);

                        previousClickedItem=clickedItem;
                        clickedItem=newItem;

                        brepChanged=true;
                    }
                }
                item->resetOperation();
            }
            i++;
        }
    }

    if (lengthInputForm) {lengthInputForm=nullptr;}
    finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,1);
}

void OpenParEMg::reextrudePolywire (CustomTreeWidgetItem *item, CustomTreeWidgetItem *child)
{
    //std::cout << "OpenParEMg::reextrudePolywire" << std::endl; std::cout.flush();

    if (!item) return;

    Process *process=item->get_Process();
    if (process) {
        Extrude *extrude=dynamic_cast<Extrude *>(process);
        if (extrude) {
            Polywire *polywire=child->get_Polywire();
            if (polywire) {
                gp_Vec scaledVec=gp_Vec(polywire->getNormal())*extrude->get_length();
                BRepPrimAPI_MakePrism aPrism(child->get_AIS_Shape()->Shape(),scaledVec);
                TopoDS_Shape newShape=aPrism;
                replaceItemShape(item,newShape,1);  // inserts to item map
                brepChanged=true;
            }
        }
    }
}

void OpenParEMg::reprocess (CustomTreeWidgetItem *item)
{
    //std::cout << "OpenParEMg::reprocess" << std::endl; std::cout.flush();

    if (!item) return;

    if (item == &drawing) {
        rebuildTopLevelShape();
        return;
    }

    Polywire *polywire=item->get_Polywire();
    if (polywire) {
        TopoDS_Wire wire=polywire->buildWire();
        if (!wire.IsNull()) {
            TopoDS_Face face=polywire->buildFace(wire);
            if (face.IsNull()) {
                replaceItemShape(item,wire,2);  // inserts to item map
            } else {
                replaceItemShape(item,face,3);  // inserts to item map
            }
        }
    }

    Process *process=item->get_Process();
    if (process) {

        Extrude *extrude=dynamic_cast<Extrude *>(process);
        if (extrude) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
                reextrudePolywire(item,child);
                i++;
            }
        }

        Merge *merge=dynamic_cast<Merge *>(process);
        if (merge) {
            if (item->childCount() == 2) {

                CustomTreeWidgetItem *child1=(CustomTreeWidgetItem *)item->child(0);
                CustomTreeWidgetItem *child2=(CustomTreeWidgetItem *)item->child(1);

                // get shapes
                TopoDS_Shape shape1=child1->get_AIS_Shape()->Shape();
                TopoDS_Shape shape2=child2->get_AIS_Shape()->Shape();

                // build merged shape
                BRepAlgoAPI_Fuse fuse(shape1,shape2);
                fuse.Build();
                if (!fuse.IsDone()) return;

                TopoDS_Shape mergedShape=fuse.Shape();

                ShapeUpgrade_UnifySameDomain unify(mergedShape);
                unify.Build();
                mergedShape=unify.Shape();

                replaceItemShape(item,mergedShape,4);  // inserts to item map
                brepChanged=true;
            }
        }

        Subtract *subtract=dynamic_cast<Subtract *>(process);
        if (subtract) {
            if (item->childCount() == 2) {

                CustomTreeWidgetItem *child1=(CustomTreeWidgetItem *)item->child(0);
                CustomTreeWidgetItem *child2=(CustomTreeWidgetItem *)item->child(1);

                // get shapes
                TopoDS_Shape shape1=child1->get_AIS_Shape()->Shape();
                TopoDS_Shape shape2=child2->get_AIS_Shape()->Shape();

                // build subtracted shape
                BRepAlgoAPI_Cut cut(shape1,shape2);
                cut.Build();
                if (!cut.IsDone()) return;

                TopoDS_Shape subtractedShape=cut.Shape();

                ShapeUpgrade_UnifySameDomain unify(subtractedShape);
                unify.Build();
                subtractedShape=unify.Shape();

                replaceItemShape(item,subtractedShape,5);  // inserts to item map
                brepChanged=true;
            }
        }
    }

    // necessary?
    item->reset_transformation();

    // recursively work to the top of the tree
    CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    reprocess(parentItem);
}

bool OpenParEMg::isValidObjectEdit ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=item->get_Polywire();
            if (polywire) count++;

            Process *process=item->get_Process();
            if (process) count++;
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

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=item->get_Polywire();

            Line *line=dynamic_cast<Line *>(polywire);
            if (line) {
                if (lineEditForm) delete lineEditForm;
                lineEditForm=new LineEditForm();
                lineEditForm->set_polywire(line);
                lineEditForm->set_drawingWindow(ui->drawingWindow);
                lineEditForm->set_relay(relay);
                lineEditForm->setModal(false);
                connect(this,&OpenParEMg::sendPnt,lineEditForm,&LineEditForm::pickVertexFinished);
                lineEditForm->show();
            }

            Rectangle *rectangle=dynamic_cast<Rectangle *>(polywire);
            if (rectangle) {
                if (rectangleEditForm) delete rectangleEditForm;
                rectangleEditForm=new RectangleEditForm();
                rectangleEditForm->set_polywire(rectangle);
                rectangleEditForm->set_drawingWindow(ui->drawingWindow);
                rectangleEditForm->set_relay(relay);
                rectangleEditForm->setModal(false);
                connect(this,&OpenParEMg::sendPnt,rectangleEditForm,&RectangleEditForm::pickVertexFinished);
                rectangleEditForm->show();
            }

            Polycircle *polycircle=dynamic_cast<Polycircle *>(polywire);
            if (polycircle) {
                if (polycircleEditForm) delete polycircleEditForm;
                polycircleEditForm=new PolycircleEditForm();
                polycircleEditForm->set_Polycircle(polycircle);
                polycircleEditForm->set_drawingWindow(ui->drawingWindow);
                polycircleEditForm->set_relay(relay);
                polycircleEditForm->setModal(false);
                connect(this,&OpenParEMg::sendPnt,polycircleEditForm,&PolycircleEditForm::pickVertexFinished);
                polycircleEditForm->show();
            }

            Process *process=item->get_Process();
            if (process) {
                Extrude *extrude=dynamic_cast<Extrude *>(process);
                if (extrude) {
                    Polywire *polywire=nullptr;
                    int i=0;
                    while (i < item->childCount()) {
                        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
                        polywire=child->get_Polywire();
                        if (polywire) break;
                        i++;
                    }

                    if (polywire) {
                        if (lengthEditForm) delete lengthEditForm;
                        lengthEditForm=new LengthInputForm();
                        lengthEditForm->set_Extrude(extrude);
                        lengthEditForm->set_length(extrude->get_length());
                        lengthEditForm->set_normal(polywire->getNormal());
                        lengthEditForm->set_drawingWindow(ui->drawingWindow);
                        lengthEditForm->set_relay(relay);
                        lengthEditForm->setModal(false);
                        lengthEditForm->show();
                    }
                }
            }
        }
        i++;
    }
}

void OpenParEMg::rebuildTopLevelShape ()
{
    //std::cout << "OpenParEMg::rebuildTopLevelShape" << std::endl; std::cout.flush();

    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);

    // cycle through the top-level children and add to the compound
    int i=0;
    while (i < drawing.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)drawing.child(i);
        if (child && !child->get_AIS_Shape().IsNull()) {
            builder.Add(compound,child->get_AIS_Shape()->Shape());
        }
        i++;
    }
    replaceItemShape(&drawing,compound,6);  // inserts to item map
}

void OpenParEMg::finishEditObject (double length, bool cancel)
{
    //std::cout << "OpenParEMg::finishEditObject  length=" << length << "  cancel=" << cancel << std::endl; std::cout.flush();

    if (!cancel) {
        long unsigned int i=0;
        while (i < ui->drawingWindow->get_selectedItems_size()) {
            CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
            if (item && item->is_drawing()) {
                Polywire *polywire=item->get_Polywire();
                if (polywire) reprocess(item);

                Process *process=item->get_Process();
                if (process) {
                    Extrude *extrude=dynamic_cast<Extrude *>(process);
                    if (extrude) {
                        extrude->set_length(length);
                        reprocess(item);
                    }
                }

                // find and show the top-level item
                CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
                while (!parentItem->is_rootDrawing()) {
                    item=parentItem;
                    parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
                }
                ui->drawingWindow->showItem(item);
            }
            i++;
        }

        brepChanged=true;
    }

    if (lengthEditForm) {lengthEditForm=nullptr;}
    if (lineEditForm) {lineEditForm=nullptr;}
    if (rectangleEditForm) {rectangleEditForm=nullptr;}
    if (polycircleEditForm) {polycircleEditForm=nullptr;}

    finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,2);
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
                Handle(AIS_Shape) shape=item->get_AIS_Shape();
                if (!shape.IsNull()) {
                    if (shape->Shape().ShapeType() == TopAbs_SOLID || shape->Shape().ShapeType() == TopAbs_COMPOUND) solidCount++;
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
    TopoDS_Shape shape1=item0->get_AIS_Shape()->Shape();
    TopoDS_Shape shape2=item1->get_AIS_Shape()->Shape();

    // build merged shape
    BRepAlgoAPI_Fuse fuse(shape1,shape2);
    fuse.Build();
    if (!fuse.IsDone()) return;

    TopoDS_Shape mergedShape=fuse.Shape();

    ShapeUpgrade_UnifySameDomain unify(mergedShape);
    unify.Build();
    mergedShape=unify.Shape();

    // add it
    CustomTreeWidgetItem *newItem=addItemShape(mergedShape,&drawing);  // inserts to item map

    // define the process
    Merge *merge=new Merge();
    newItem->set_Process(merge);

    // save the process
    merge=nullptr;

    // move the items in the tree

    drawing.removeChild(item0);
    newItem->addChild(item0);

    drawing.removeChild(item1);
    newItem->addChild(item1);

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

    brepChanged=true;

    finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,3);
}

bool OpenParEMg::isValidSubtractSolids ()
{
    //std::cout << "OpenParEMg::isValidSubtractSolids" << std::endl; std::cout.flush();
    return isValidMergeSolids();
}

void OpenParEMg::subtractSolids ()
{
    startOperation(false);
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
    TopoDS_Shape shape1=item0->get_AIS_Shape()->Shape();
    TopoDS_Shape shape2=item1->get_AIS_Shape()->Shape();

    // build subtacted shape
    BRepAlgoAPI_Cut cut(shape1,shape2);
    cut.Build();
    if (!cut.IsDone()) return;

    TopoDS_Shape subtractedShape=cut.Shape();

    ShapeUpgrade_UnifySameDomain unify(subtractedShape);
    unify.Build();
    subtractedShape=unify.Shape();

    // add it
    CustomTreeWidgetItem *newItem=addItemShape(subtractedShape,&drawing);  // inserts to item map

    // define the process
    Subtract *subtract=new Subtract();
    newItem->set_Process(subtract);

    // save the process
    subtract=nullptr;

    // move the items in the tree

    drawing.removeChild(item0);
    newItem->addChild(item0);

    drawing.removeChild(item1);
    newItem->addChild(item1);

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

    brepChanged=true;

    finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,4);
}

void OpenParEMg::moveObject ()
{
    std::cout << "OpenParEMg::moveObject" << std::endl; std::cout.flush();

    startOperation(true);
    ui->drawingWindow->set_pickSecondVertex(true);

    // set up for animation
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            item->setAnimate(ui->drawingWindow->get_viewerContext());
            item->resetP0P1();
            item->reset_transformation();
            item->setEnableMove(true);
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
    std::cout << "OpenParEMg::finishMoveObject  isChild=" << isChild << std::endl; std::cout.flush();

    if (!item) return;

    item->setEnableMove(false);
    item->unsetAnimate(ui->drawingWindow->get_viewerContext());
    item->reset_transformation();

    Polywire *polywire=item->get_Polywire();
    if (polywire) {
        polywire->shift(p1,p0);
        reprocess(item);
        brepChanged=true;
    }

    Process *process=item->get_Process();
    if (process) {
        int i=0;
        while (i < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
            finishMoveObject(child,p0,p1,false);
            reprocess(item);
            brepChanged=true;
            i++;
        }
    }

    if (!polywire && !process) {
        TopoDS_Shape newShape=item->moveShape(p0,p1,ui->drawingWindow->get_viewerContext());
        replaceItemShape(item,newShape,7);
        reprocess(item);
        brepChanged=true;
    }
}

void OpenParEMg::finishMoveObject (CustomTreeWidgetItem *item, gp_Pnt p0, gp_Pnt p1)
{
    std::cout << "OpenParEMg::finishMoveObject" << std::endl; std::cout.flush();

    if (!item) return;

    finishMoveObject(item,p0,p1,false);
    item->resetOperation();

    // find and show the top-level item
    CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    while (!parentItem->is_rootDrawing()) {
        item=parentItem;
        parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    }
    ui->drawingWindow->showItem(item);
}

bool OpenParEMg::isValidCopy ()
{
    //std::cout << "OpenParEMg::isValidCopy" << std::endl; std::cout.flush();

    if (ui->drawingWindow->NbSelected() == 0) return false;
    return true;
}

CustomTreeWidgetItem* OpenParEMg::copyItem (CustomTreeWidgetItem *item, CustomTreeWidgetItem *newItemParent)
{
    //std::cout << "OpenParEMg::copyItem" << std::endl; std::cout.flush();

    if (!item) return nullptr;

    CustomTreeWidgetItem *newItem=item->copyCreate();
    newItem->setForeground(0,Qt::black);
    ui->drawingWindow->activateItem(newItem);
    ui->drawingWindow->insertItemToMap(newItem->get_AIS_Shape(),newItem);
    newItemParent->addChild(newItem);

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        if (child) copyItem(child,newItem);
        i++;
    }

    return newItem;
}

void OpenParEMg::copyDrawingItems ()
{
    //std::cout << "OpenParEMg::copyDrawingItems" << std::endl; std::cout.flush();

    startOperation(true);
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
            ui->drawingWindow->unselectItem(item);
            ui->drawingWindow->hideItem(newItem);
            ui->drawingWindow->showItem(newItem);
            ui->drawingWindow->selectItem(newItem);
        }
        i++;
    }

    finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,100);
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
            Polywire *polywire=item->get_Polywire();
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
    ui->drawingWindow->set_pickSecondVertex(true);

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Handle(AIS_Shape) shape=item->get_AIS_Shape();
            if (!shape.IsNull()) {

                // set the drawing plane
                currentPrivilegedPlane=ui->drawingWindow->get_gridPlane();
                //restrictToDrawingPlane=true;

                Polywire *polywire=item->get_Polywire();
                if (polywire) {
                    item->setEnableStretch(true);
                    item->resetP0P1();
                    gp_Pln plane=polywire->getPlane();
                    ui->drawingWindow->set_gridPlane(plane);
                }
            }
        }
        i++;
    }
}

void OpenParEMg::finishStretchObject (CustomTreeWidgetItem *item)
{
    std::cout << "OpenParEMg::finishStretchObject" << std::endl; std::cout.flush();

    if (!item) return;

    Polywire *polywire=item->get_Polywire();
    if (!polywire) return;

    item->setEnableStretch(false);
    gp_Pnt pnt=item->getP1();
    polywire->setEditPoint(pnt);
    reprocess(item);

    polywire->deleteRubberband();
    ui->drawingWindow->set_gridPlane(currentPrivilegedPlane);

    item->resetOperation();
    brepChanged=true;

    // find and show the top-level item
    CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    while (!parentItem->is_rootDrawing()) {
        item=parentItem;
        parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    }
    ui->drawingWindow->showItem(item);

    finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,5);
}

bool OpenParEMg::isValidDeletePoint ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=item->get_Polywire();
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
    ui->drawingWindow->set_pickSecondVertex(true);

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Handle(AIS_Shape) shape=item->get_AIS_Shape();
            if (!shape.IsNull()) {
                // set the selected shape to be the only selectable shape
                // includes selecting just on vertices of the shape and not midpoints
                //ui->drawingWindow->set_activeShape(shape);

                // set the drawing plane
                currentPrivilegedPlane=ui->drawingWindow->get_gridPlane();
                //restrictToDrawingPlane=true;

                Polywire *polywire=item->get_Polywire();
                if (polywire) {
                    item->setEnableDeletePoint(true);
                    item->resetP0P1();
                    gp_Pln plane=polywire->getPlane();
                    ui->drawingWindow->set_gridPlane(plane);
                }
            }
        }
        i++;
    }
}

void OpenParEMg::finishDeletePoint (CustomTreeWidgetItem *item)
{
    if (!item) return;

    Polywire *polywire=item->get_Polywire();
    if (!polywire) return;

    gp_Pnt p0=item->getP0();
    polywire->deletePoint(p0);
    reprocess(item);
    item->resetOperation();

    ui->drawingWindow->set_gridPlane(currentPrivilegedPlane);

    brepChanged=true;

    // find and show the top-level item
    CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    while (!parentItem->is_rootDrawing()) {
        item=parentItem;
        parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
    }
    ui->drawingWindow->showItem(item);

    finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,6);
}

bool OpenParEMg::isValidInsertPoint ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Polywire *polywire=item->get_Polywire();
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
    ui->drawingWindow->set_pickFirstVertex(true);

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_drawing()) {
            Handle(AIS_Shape) shape=item->get_AIS_Shape();
            if (!shape.IsNull()) {
                // set the selected shape to be the only selectable shape
                // includes selecting just on vertices of the shape and not midpoints
                //ui->drawingWindow->set_activeShape(shape);

                // set the drawing plane
                currentPrivilegedPlane=ui->drawingWindow->get_gridPlane();
                //restrictToDrawingPlane=true;

                Polywire *polywire=item->get_Polywire();
                if (polywire) {
                    item->setEnableInsertPoint(true);
                    item->resetP0P1();
                    gp_Pln plane=polywire->getPlane();
                    ui->drawingWindow->set_gridPlane(plane);
                }
            }
        }
        i++;
    }
}

void OpenParEMg::finishInsertPoint (CustomTreeWidgetItem *item)
{
    if (!item) return;

    Polywire *polywire=item->get_Polywire();
    if (polywire) {
        gp_Pnt p0=item->getP0();
        polywire->insertPoint(p0);
        //reprocess(item);
        item->resetOperation();

        ui->drawingWindow->set_gridPlane(currentPrivilegedPlane);

        brepChanged=true;

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
    }
}

void OpenParEMg::rotateObject ()
{
    std::cout << "OpenParEMg::rotateObject" << std::endl; std::cout.flush();

    startOperation(true);

    if (rotateInputForm) delete rotateInputForm;
    rotateInputForm=new RotateInputForm();
    rotateInputForm->set_drawingWindow(ui->drawingWindow);
    rotateInputForm->set_relay(relay);
    rotateInputForm->setModal(false);
    connect(this,&OpenParEMg::sendPnt,rotateInputForm,&RotateInputForm::pickVertexFinished);
    rotateInputForm->show();
}

void OpenParEMg::finishRotateObject (CustomTreeWidgetItem *item, double &angleDegrees, gp_Pnt &p1, gp_Pnt &p2)
{
    //std::cout << "OpenParEMg::finishRotateObject" << std::endl; std::cout.flush();

    if (!item) return;

    Polywire *polywire=item->get_Polywire();
    if (polywire) {
        polywire->rotate(angleDegrees,p1,p2);
        reprocess(item);
        brepChanged=true;
    }

    Process *process=item->get_Process();
    if (process) {
        int i=0;
        while (i < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
            finishRotateObject(child,angleDegrees,p1,p2);
            i++;
        }
    }

    if (!polywire && !process) {
        TopoDS_Shape newShape=item->rotateShape(angleDegrees,p1,p2,ui->drawingWindow->get_viewerContext());
        replaceItemShape(item,newShape,10);
        //ui->drawingWindow->showItem(item);
        reprocess(item);
        brepChanged=true;
    }
}

void OpenParEMg::finishRotateObject (double &angleDegrees, gp_Pnt &p1, gp_Pnt &p2)
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item) {
            finishRotateObject(item,angleDegrees,p1,p2);

            item->resetOperation();

            // find and show the top-level item
            CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            while (!parentItem->is_rootDrawing()) {
                item=parentItem;
                parentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
            }
            ui->drawingWindow->showItem(item);
        }
        i++;
    }
    if (rotateInputForm) {rotateInputForm=nullptr;}
    finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,7);
}

void OpenParEMg::createPort ()
{
    std::cout << "OpenParEMg::createPortFromDrawing" << std::endl; std::cout.flush();

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

    TopoDS_Shape selectedShape=ui->drawingWindow->get_selectedFace();
    newPath->addFacePoints(selectedShape,true,true);
    newPath->create_item(ui->drawingWindow,&path);  // create item and add as child to path; creates AIS_Shape

    boundaryDatabase->push_path(newPath);

    // add new path to the drawing
    CustomTreeWidgetItem *item=newPath->get_item();
    if (item) {
        addItemWithShape(item);

        long unsigned int j=0;
        while (j < item->get_arrowHeads_size()) {
            //ui->drawingWindow->displayShape(item->get_arrowHead(j),item->get_displayMode(),item->get_selectionMode());
            ui->drawingWindow->displayShape(item->get_arrowHead(j));
            ui->drawingWindow->insertItemToMap(item->get_arrowHead(j),item);
            j++;
        }
    }

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

void OpenParEMg::setPhysicalGroups ()
{
    //std::cout << "OpenParEMg::setPhysicalGroups" << std::endl; std::cout.flush();

    // re-build the physical groups list

    clear_physicalGroupMaterials (&projData);

    // check the first-level children for SOLID
    int i=0;
    while (i < drawing.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)drawing.child(i);
        if (child->get_AIS_Shape()->Shape().ShapeType() == TopAbs_SOLID) {
            QString itemMaterial=child->text(0);
            char *material=nullptr;
            cstrFromQString (&material,itemMaterial);
            add_physicalGroupMaterial(&projData,-1,child->get_dimTag().first,child->get_dimTag().second,material);
            if (material) {free(material);}
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

void OpenParEMg::assignMaterial ()
{
    // Cannot assign material to existing mesh
    if (meshFileLoaded) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,"OpenParEMg","Materials cannot be assigned to an existing mesh.  Do you want to delete the mesh?",QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
        deleteMesh(true);
        meshChanged=false;
        meshFileLoaded=false;
    }

    MaterialSelection *materialSelection=new MaterialSelection();
    materialSelection->set_materialDatabase(materialDatabase);
    materialSelection->set_selectedMaterial(&selectedMaterial);
    materialSelection->populate();
    materialSelection->exec();
    delete materialSelection;

    if (selectedMaterial != "") {
        clickedItem->setText(0,selectedMaterial);
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
        if (strcmp(projData.materials_global_name,"") != 0 || strcmp(projData.materials_local_name,"") != 0) {
            if (materialDatabase->load_materials(projData.materials_global_path,projData.materials_global_name,
                                                 projData.materials_local_path,projData.materials_local_name,
                                                 projData.materials_check_limits)) {
                QMessageBox mb;
                mb.critical(nullptr, "Error", "Unable to load the specified materials files.");
                mb.setFixedSize(500, 200);
            }
        }

        // load brep file, if defined
        if (strcmp(projData.gui_brep_file,"") != 0) {

            QString filePath=projData.gui_brep_file;

            if (loadBrepFile(filePath,false)) {
                QString message="Unable to load Brep file \"";
                message.append(filePath);
                message.append("\".");
                QMessageBox mb;
                mb.critical(nullptr, "Error",message);
                mb.setFixedSize(500, 200);
            }
        }

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
                if (item) {
                    addItemWithShape(item);

                    long unsigned int j=0;
                    while (j < item->get_arrowHeads_size()) {
                        //ui->drawingWindow->displayShape(item->get_arrowHead(j),item->get_displayMode(),item->get_selectionMode());
                        ui->drawingWindow->displayShape(item->get_arrowHead(j));
                        ui->drawingWindow->insertItemToMap(item->get_arrowHead(j),item);
                        j++;
                    }
                }
                i++;
            }

            // cross link paths to ports
            boundaryDatabase->crossLink();
        }

        // load mesh, if any, and draw
        if (strcmp(projData.mesh_file,"") != 0) {
            loadMeshFile(QString::fromStdString(projData.mesh_file));
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
    setMenusI(39);
}

void OpenParEMg::resetLockouts ()
{
    disableMenus=false;
    projectFileLoaded=false;
    projectChanged=false;
    meshFileLoaded=false;
    meshChanged=false;
    brepFileLoaded=false;
    brepChanged=false;
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
              << "   meshFileLoaded=" << meshFileLoaded << std::endl
              << "   meshChanged=" << meshChanged << std::endl
              << "   brepFileLoaded=" << brepFileLoaded << std::endl
              << "   brepChanged=" << brepChanged << std::endl
              << "   drawingPlaneShown=" << drawingPlaneShown << std::endl
              << "   simulationRunning=" << simulationRunning << std::endl
              << "   simulationStopping=" << simulationStopping << std::endl
              << "   simulationAborting=" << simulationAborting << std::endl;
}

void OpenParEMg::resetDrawing ()
{
    //std::cout << "OpenParEMg::resetDrawing" << std::endl; std::cout.flush();

    // counts
    pointCount=0;
    curveCount=0;
    surfaceCount=0;
    volumeCount=0;

    // mesh
    deleteMesh(false);

    // reset drawing window
    ui->drawingWindow->clearDrawing();
    ui->drawingWindow->updateViewer();

    // selection tree
    drawing.reset();

    //reset the tracking
    ui->drawingWindow->reset();

    clickedItem=nullptr;
    previousClickedItem=nullptr;
    workingItem=nullptr;

    brepFileLoaded=false;

    on_actionShape_triggered();
}

void OpenParEMg::resetProject ()
{
    //std::cout << "OpenParEMg::resetProject" << std::endl; std::cout.flush();

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
    // update included file names, if needed

    // port_definition_file
    if (boundaryDatabase->is_modified() && strcmp(projData.port_definition_file,"") == 0) {
        QString portDefinitionFile=projectName;
        portDefinitionFile.append("_ports.txt");
        cstrFromQString (&(projData.port_definition_file),portDefinitionFile);
        projectChanged=true;
    }

    // mesh_file
    if (meshFileLoaded && strcmp(projData.mesh_file,"") == 0 ) {
        QString meshFile=projectName;
        meshFile.append(".msh");
        cstrFromQString (&(projData.mesh_file),meshFile);
        projectChanged=true;
    }

    // gui_brep_file
    if (brepFileLoaded && strcmp(projData.gui_brep_file,"") == 0) {
        QString brepFile=projectName;
        brepFile.append(".brep");
        cstrFromQString (&(projData.gui_brep_file),brepFile);
        projectChanged=true;
    }

    // save

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

    // Brep
    if (brepChanged) {
        std::cout << "Saved Brep file" << std::endl; std::cout.flush();
        if (saveBrepFile(projData.gui_brep_file)) {
            QString message="Error in saving the drawing file.";
            QMessageBox mb;
            mb.critical(nullptr, "Error",message);
            mb.setFixedSize(500, 200);
        } else brepChanged=0;
    }

    // mesh
    if (meshFileLoaded && meshChanged) {
        std::cout << "Saved mesh file" << std::endl; std::cout.flush();
        on_actionMeshSave_triggered();
        meshChanged=false;
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

// recursively fill out the whole tree
// void OpenParEMg::addShape (TopoDS_Shape shape, CustomTreeWidgetItem *item, bool isRoot, bool itemHasAISShape)
// {
//     if (shape.IsNull()) return;

//     // ensure the root item is a compound
//     if (isRoot && shape.ShapeType() != TopAbs_COMPOUND) {
//         BRep_Builder builder;
//         TopoDS_Compound compound;
//         builder.MakeCompound(compound);
//         builder.Add(compound,shape);
//         shape=compound;
//     }

//     // tree item name

//     QString name="";

//     std::pair<int,int> dimTag;
//     dimTag.first=-1; dimTag.second=-1;

//     TopAbs_ShapeEnum shapeType=shape.ShapeType();
//     switch (shapeType) {
//     case TopAbs_COMPOUND:
//         name="COMPOUND";
//         break;
//     case TopAbs_COMPSOLID:
//         name="COMPSOLID";
//         volumeCount++;   // a guess
//         dimTag.first=3; dimTag.second=volumeCount;
//         break;
//     case TopAbs_SOLID:
//         name="SOLID";
//         volumeCount++;
//         dimTag.first=3; dimTag.second=volumeCount;
//         break;
//     case TopAbs_SHELL:
//         name="SHELL";
//         surfaceCount++;  // a guess
//         dimTag.first=2; dimTag.second=surfaceCount;
//         break;
//     case TopAbs_FACE:
//         name="FACE";
//         surfaceCount++;  // aguess
//         dimTag.first=2; dimTag.second=surfaceCount;
//         break;
//     case TopAbs_WIRE:
//         name="WIRE";
//         curveCount++;    // a guess
//         dimTag.first=2; dimTag.second=surfaceCount;
//         break;
//     case TopAbs_EDGE:
//         name="EDGE";
//         curveCount++;
//         dimTag.first=2; dimTag.second=surfaceCount;
//         break;
//     case TopAbs_VERTEX:
//         name="VERTEX";
//         pointCount++;
//         dimTag.first=1; dimTag.second=pointCount;
//         break;
//     case TopAbs_SHAPE:
//         name="SHAPE";
//         break;
//     default:
//         std::cout << "ASSERT:: Unknown shape type." << std::endl;  std::cout.flush();
//         break;
//     }

//     // item entry

//     Handle(AIS_Shape) drawingShape;
//     if (itemHasAISShape) drawingShape=item->get_AIS_Shape();  // reuse AIS_Shape
//     else drawingShape=new AIS_Shape(shape);                   // create new AIS_Shape
//     CustomTreeWidgetItem *newItem;

//     // only drawing can have isRoot=true
//     if (isRoot) {
//         ui->drawingWindow->insertItemToMap(drawingShape,&drawing);
//         drawing.set_AIS_Shape(drawingShape);
//         //ui->drawingWindow->showItem(&drawing);
//         showRootDrawingItems();
//         ui->drawingWindow->unselectItem(&drawing);
//         newItem=&drawing;
//     } else {
//         newItem=new CustomTreeWidgetItem(0);
//         if (!itemHasAISShape) newItem->set_AIS_Shape(drawingShape);
//         newItem->setText(0,name);
//         newItem->set_type(0);  // default value
//         newItem->setForeground(0,Qt::gray);
//         item->addChild(newItem);
//         ui->drawingWindow->insertItemToMap(drawingShape,newItem);
//     }

//     // properties for the CustomTreeWidgetItem
//     newItem->set_Polywire(polywire);
//     newItem->set_dimTag(dimTag);
//     if (name.compare("SOLID") == 0) {
//         int i=0;
//         while (i < projData.physicalGroupMaterialCount) {
//             if (projData.physicalGroupMaterials[i].tag == dimTag.second) {
//                 newItem->setText(0,projData.physicalGroupMaterials[i].materialName);
//                 break;
//             }
//             i++;
//         }
//     }

//     // children
//     TopoDS_Iterator topoIterator(shape);
//     while (topoIterator.More()) {
//         const TopoDS_Shape& child=topoIterator.Value();
//         addShape(child,newItem,false,false);
//         topoIterator.Next();
//     }
// }


void OpenParEMg::addChildDisplayShape (CustomTreeWidgetItem *item, std::pair<int,int> &dimTag)
{
    if (!item) return;

    // item must be display for now
    if (item != &drawing) return;

    Handle(AIS_Shape) shape=item->get_AIS_Shape();
    if (shape.IsNull()) return;

    // add the shape
    //TopoDS_Compound compound=TopoDS::Compound(shape->Shape());
    //BRep_Builder builder;

    TopoDS_Iterator topoIterator(shape->Shape());
    while (topoIterator.More()) {
        const TopoDS_Shape& child=topoIterator.Value();
        TopAbs_ShapeEnum shapeType=child.ShapeType();
        // TopAbs_COMPSOLID may not be needed
        if (shapeType == TopAbs_COMPSOLID || shapeType == TopAbs_SOLID || shapeType == TopAbs_COMPOUND) {
            volumeCount++;
            dimTag.first=3; dimTag.second=volumeCount;

            CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem(0);
            Handle(AIS_Shape) drawingShape=new AIS_Shape(child);
            newItem->set_AIS_Shape(drawingShape);
            if (shapeType == TopAbs_COMPSOLID) newItem->setText(0,"COMPSOLID");
            else if (shapeType == TopAbs_SOLID) newItem->setText(0,"SOLID");
            else if (shapeType == TopAbs_COMPOUND) newItem->setText(0,"COMPOUND");
            newItem->set_itemType(0);  // a drawing item
            newItem->setForeground(0,Qt::gray);
            newItem->set_dimTag(dimTag);
            item->addChild(newItem);
            addItemWithShape(newItem);

            //builder.Add(compound,child);
        }
        topoIterator.Next();
    }

    //shape->Redisplay(true);
}

void OpenParEMg::addRootDisplayShape (TopoDS_Shape shape)
{
    if (shape.IsNull()) return;

    // ensure the root item is a compound
    if (shape.ShapeType() != TopAbs_COMPOUND) {
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        builder.Add(compound,shape);
        shape=compound;
    }

    // variable to support meshing
    std::pair<int,int> dimTag;
    dimTag.first=-1; dimTag.second=-1;

    // drawing item
    Handle(AIS_Shape) drawingShape=new AIS_Shape(shape);
    ui->drawingWindow->insertItemToMap(drawingShape,&drawing);
    drawing.set_AIS_Shape(drawingShape);

    // pick off solids that are included in the compound and add below drawing
    TopoDS_Iterator topoIterator(shape);
    while (topoIterator.More()) {
        const TopoDS_Shape& child=topoIterator.Value();
        TopAbs_ShapeEnum shapeType=child.ShapeType();
        // TopAbs_COMPSOLID may not be needed
        if (shapeType == TopAbs_COMPSOLID || shapeType == TopAbs_SOLID) {
            volumeCount++;
            dimTag.first=3; dimTag.second=volumeCount;

            CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem(0);
            Handle(AIS_Shape) drawingShape=new AIS_Shape(child);
            ui->drawingWindow->insertItemToMap(drawingShape,newItem);
            newItem->set_AIS_Shape(drawingShape);
            if (shapeType == TopAbs_COMPSOLID) newItem->setText(0,"COMPSOLID");
            else if (shapeType == TopAbs_SOLID) newItem->setText(0,"SOLID");
            newItem->set_itemType(0);  // a drawing item
            newItem->setForeground(0,Qt::gray);
            newItem->set_Polywire(nullptr);
            newItem->set_dimTag(dimTag);
            drawing.addChild(newItem);

            // set materials through the name
            if (shapeType == TopAbs_SOLID) {
                int i=0;
                while (i < projData.physicalGroupMaterialCount) {
                    if (projData.physicalGroupMaterials[i].tag == dimTag.second) {
                        newItem->setText(0,projData.physicalGroupMaterials[i].materialName);
                        break;
                    }
                    i++;
                }
            }
        }
        topoIterator.Next();
    }

    drawing.setForeground(0,Qt::gray);
    ui->drawingWindow->showItem(&drawing);
    ui->drawingWindow->unselectItem(&drawing);
}

/*
// for root drawing item: fill out solids as part of a top-level compound
void OpenParEMg::addRootDisplayShape (TopoDS_Shape shape)
{
    if (shape.IsNull()) return;

    // ensure the root item is a compound
    if (shape.ShapeType() != TopAbs_COMPOUND) {
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        builder.Add(compound,shape);
        shape=compound;
    }

    // variable to support meshing
    std::pair<int,int> dimTag;
    dimTag.first=-1; dimTag.second=-1;

    // drawing item
    Handle(AIS_Shape) drawingShape=new AIS_Shape(shape);
    ui->drawingWindow->insertItemToMap(drawingShape,&drawing);
    drawing.set_AIS_Shape(drawingShape);
    addChildDisplayShape(&drawing,dimTag);

    // apply materials
    int i=0;
    while (i < drawing.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) drawing.child(i);
        if (child) {
            Handle(AIS_Shape) shape=child->get_AIS_Shape();
            if (!shape.IsNull()) {
                if (shape->Shape().ShapeType() == TopAbs_SOLID) {
                    int j=0;
                    while (j < projData.physicalGroupMaterialCount) {
                        if (projData.physicalGroupMaterials[j].tag == dimTag.second) {
                            child->setText(0,projData.physicalGroupMaterials[j].materialName);
                            break;
                        }
                        j++;
                    }
                }
            }
        }
        i++;
    }

    //drawing.setForeground(0,Qt::gray);
    //ui->drawingWindow->showItem(&drawing);
    showRootDrawingItems();
    //ui->drawingWindow->unselectItem(&drawing);
}
*/

// for non-root drawing items where the item and shape have already been created
void OpenParEMg::addItemWithShape (CustomTreeWidgetItem *item)
{
    if (!item) return;

    Handle(AIS_Shape) drawingShape=item->get_AIS_Shape();
    ui->drawingWindow->insertItemToMap(drawingShape,item);
    item->setForeground(0,Qt::gray);
    ui->drawingWindow->showItem(item);
    ui->drawingWindow->unselectItem(item);
}

// for non-root drawing items where item does not exist
CustomTreeWidgetItem* OpenParEMg::addItemShape (TopoDS_Shape shape, CustomTreeWidgetItem *parentItem)
{
    if (shape.IsNull()) return nullptr;
    if (!parentItem) return nullptr;

    // variable to support meshing
    std::pair<int,int> dimTag;
    dimTag.first=-1; dimTag.second=-1;

    // new item
    CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem(0);
    replaceItemShape(newItem,shape,11);  // inserts to item map

    // add to top-level COMPOUND if parent is drawing
    if (parentItem == &drawing) {
        TopoDS_Compound compound=TopoDS::Compound(parentItem->get_AIS_Shape()->Shape());
        BRep_Builder builder;
        builder.Add(compound,shape);
        parentItem->get_AIS_Shape()->Redisplay(true);
    }

    // process for SOLID
    TopAbs_ShapeEnum shapeType=shape.ShapeType();
    if (shapeType == TopAbs_COMPSOLID || shapeType == TopAbs_SOLID) {
        volumeCount++;
        dimTag.first=3; dimTag.second=volumeCount;

        // set materials through the name
        if (shapeType == TopAbs_SOLID) {
            int i=0;
            while (i < projData.physicalGroupMaterialCount) {
                if (projData.physicalGroupMaterials[i].tag == dimTag.second) {
                    newItem->setText(0,projData.physicalGroupMaterials[i].materialName);
                    break;
                }
                i++;
            }
        }
    }

    // set variables
    newItem->setText(0,TopAbs::ShapeTypeToString(shape.ShapeType()));
    newItem->set_itemType(0);  // default to drawing, but generally need to change elsewhere
    newItem->setForeground(0,Qt::gray);
    newItem->set_Polywire(activePolywire);
    newItem->set_dimTag(dimTag);
    parentItem->addChild(newItem);

    return newItem;
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
            addRootDisplayShape(normalized);
            brepFileLoaded=true;
            brepChanged=false;

            if (createName) {
                QFileInfo fileInfo(filePath);
                QString brepName=fileInfo.fileName();
                cstrFromQString (&(projData.gui_brep_file),brepName);
                projectChanged=true;
                brepChanged=true;
            }

            //drawing.setForeground(0,Qt::gray);
            //ui->drawingWindow->showItem(&drawing);
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
            addRootDisplayShape(normalized);
            brepFileLoaded=true;
            brepChanged=true;

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

    Handle(AIS_Shape) shape=item->get_AIS_Shape();
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

    if (drawing.get_AIS_Shape()->Shape().IsNull()) return true;
    if (BRepTools::Write(drawing.get_AIS_Shape()->Shape(),filePath)) brepChanged=false;
    else return true;

    return false;
}

bool OpenParEMg::saveStepFile (QString filePath)
{
    if (!filePath.isEmpty()) {
        STEPControl_Writer writer;
        writer.Transfer(drawing.get_AIS_Shape()->Shape(),STEPControl_ManifoldSolidBrep,Standard_True);

        IFSelect_ReturnStatus status=writer.Write(filePath.toStdString().c_str());
        if (status == IFSelect_RetDone) {
            return false;
        }
    }
    return true;
}

bool OpenParEMg::saveBoundaryDatabase ()
{
    if (!boundaryDatabase) return true;

    QString filename=absolutePath;
    filename.append("/").append(projData.port_definition_file);
    std::cout << "Saving the boundary database to " << filename.toStdString() << std::endl; std::cout.flush();

    std::ofstream outputFile(filename.toStdString());
    if (outputFile.is_open()) {
        boundaryDatabase->save(&outputFile);
        outputFile.close();
        return false;
    }
    return true;
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
    if (projectChanged || brepChanged || meshChanged || boundaryDatabase->is_modified()) {
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
    ui->drawingItemTree->setCurrentItem(clickedItem);

    if (CTRLpressed) {
        if (SHIFTpressed) {
        } else {
            ui->drawingWindow->selectItem(clickedItem);
            previousClickedItem=clickedItem;
        }
    } else if (SHIFTpressed) {
        if (CTRLpressed) {
        } else {
            if (previousClickedItem) {
                if (clickedItem->QTreeWidgetItem::parent() == previousClickedItem->QTreeWidgetItem::parent()) {

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
                }
            }
        }
    } else {
        CustomTreeWidgetItem *clickedItemKeep=clickedItem;
        clearTreeSelection();
        clickedItem=clickedItemKeep;

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
                clearTreeSelection();
            }
        }
    }

    if (event->type() == QEvent::Paint) {

    }

    return QObject::eventFilter(obj, event);
}

void OpenParEMg::keyPressEvent (QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control) {
        CTRLpressed=true;
    } else if (event->key() == Qt::Key_Shift) {
        SHIFTpressed=true;
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
    }
    QWidget::keyPressEvent(event);
}

void OpenParEMg::keyReleaseEvent (QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control) {
        CTRLpressed=false;
    } else if (event->key() == Qt::Key_Shift) {
        SHIFTpressed=false;
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

    if (!meshFileLoaded) return;

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
    pointCount=0; curveCount=0; surfaceCount=0; volumeCount=0;
}

void OpenParEMg::on_actionMeshGenerate_triggered ()
{
    if (meshFileLoaded) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,"OpenParEMg","Delete the existing mesh?",QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        deleteMesh(true);
    }

    // generate mesh
    TopoDS_Shape shape=drawing.get_AIS_Shape()->Shape();
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

    meshFileLoaded=true;
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

        if (meshFileLoaded) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this,"OpenParEMg","Delete the existing mesh?",QMessageBox::Yes|QMessageBox::No);
            if (reply != QMessageBox::Yes) return;

            deleteMesh(false);
        }

        // load and display
        //gmsh::model::remove();
        gmsh::open(meshfile.toStdString());

        meshFileLoaded=true;
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
    meshFileLoaded=false;
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

void OpenParEMg::on_actionDrawingPlaneSetToFace_triggered ()
{
    std::cout << "OpenParEMg::on_actionDrawingPlaneSetToFace_triggered" << std::endl; std::cout.flush();

    startOperation(false);

    skipDrawingPlaneAxisForm=true;
    on_actionFace_triggered();
    ui->drawingWindow->setFaceSelection();
}

void OpenParEMg::on_actionDrawingPlaneSetToFaceAxis_triggered ()
{
    std::cout << "OpenParEMg::on_actionDrawingPlaneSetToFaceAxis_triggered" << std::endl; std::cout.flush();

    restrictToDrawingPlane=true;
    startOperation(true);

    skipDrawingPlaneAxisForm=false;
    on_actionFace_triggered();
    ui->drawingWindow->setFaceSelection();
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
        finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,8);
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
    vectorInputForm->set_drawingWindow(ui->drawingWindow);
    vectorInputForm->set_relay(relay);
    vectorInputForm->setModal(false);
    connect(this,&OpenParEMg::sendPnt,vectorInputForm,&VectorInputForm::pickVertexFinished);
    vectorInputForm->show();
}

void OpenParEMg::finishPlaneSetToFace (gp_Pnt &p1, gp_Pnt &p2)
{
    std::cout << "OpenParEMg::finishPlaneSetToFace" << std::endl; std::cout.flush();

    // u vector
    uLocalAxis=p2.XYZ()-p1.XYZ();
    uLocalAxis.Normalize();

    // origin

    gp_Pln plane=ui->drawingWindow->get_gridPlane();
    plane.SetLocation(p1);

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
    finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,8);
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

void OpenParEMg::on_actionSelectWithBox_triggered ()
{
    ui->drawingWindow->selectRectangle();
    setMenusI(79);
}

void OpenParEMg::cancelDraw ()
{
    std::cout << "OpenParEMg::cancelDraw" << std::endl; std::cout.flush();

    finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true,9);
}

void OpenParEMg::on_actionDrawLine_triggered ()
{
    //std::cout << "OpenParEMg::on_actionDrawLine_triggered" << std::endl; std::cout.flush();

    restrictToDrawingPlane=true;
    startOperation(true);
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

    CustomTreeWidgetItem *newItem;
    TopoDS_Wire newWire=activePolywire->buildWire();
    if (newWire.IsNull()) return;
    TopoDS_Face newFace=activePolywire->buildFace(newWire);
    if (newFace.IsNull()) {
        newItem=addItemShape(newWire,&drawing);  // inserts to item map
    } else {
        newItem=addItemShape(newFace,&drawing);  // inserts to item map
    }

    // put it on the Z-layer to get it higher selection priority
    newItem->get_AIS_Shape()->SetZLayer(Graphic3d_ZLayerId_Top);

    //ui->drawingItemTree->setCurrentItem(newItem);
    ui->drawingWindow->showItem(newItem);
    ui->drawingWindow->activateSelectItem(newItem);
    previousClickedItem=clickedItem;
    clickedItem=newItem;

    restrictToDrawingPlane=false;

    activePolywire->deleteRubberband();
    activePolywire=nullptr;

    brepChanged=true;
    workingItem=nullptr;
    ui->drawingWindow->removeSelectOnVertex();

    finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,10);
}

void OpenParEMg::drawPath ()
{
    // to avoid stray clicks in the selection tree
    workingItem=clickedItem;

    // enable selection on just the port
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_voltage() || item->is_current()) {

            // mode parent
            CustomTreeWidgetItem *modeParentItem=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();

            // port parent
            CustomTreeWidgetItem *portParentItem=(CustomTreeWidgetItem *)modeParentItem->QTreeWidgetItem::parent();

            // port shape
            if (portParentItem->linkedItems_size() > 0) {

                // port shape
                CustomTreeWidgetItem *portItem=portParentItem->get_linkedItem(0);  // should just be one linked item to the port outline
                Handle(AIS_Shape) portShape=portItem->get_AIS_Shape();

                // port path
                Path *portPath=(Path *)portItem->get_OPEMobject();

                // select on vertices within the port path
                ui->drawingWindow->selectOnVertex(portPath);

                // get the normal to apply to the drawn Path
                // Since the drawing is confined to the drawn Path, the normals will be the same.
                activePolywire->setNormal(portPath->get_normal());
            }
        }
        i++;
    }

    isIntegrationPath=true;
}

void OpenParEMg::drawLinePath ()
{
    activePolywire=new Line();
    activePolywire->set_viewerContext(ui->drawingWindow->get_viewerContext());

    drawPath();
    on_actionDrawLine_triggered();
}

void OpenParEMg::drawPolylinePath ()
{
    activePolywire=new Polyline();
    activePolywire->set_viewerContext(ui->drawingWindow->get_viewerContext());

    drawPath();
    on_actionDrawPolyline_triggered();
}

bool OpenParEMg::insertActionValid ()
{
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

    i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_path()) {
            Path *path=(Path *)item->get_OPEMobject();
            if (path->get_portItem() != portParentItem) return false;
        }
        i++;
    }

    return true;
}

void OpenParEMg::insertSelectedPath ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        CustomTreeWidgetItem *item=ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_voltage() || item->is_current()) {insertPath(item);}
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
    //std::cout << "OpenParEMg::finishPolyline  operation=" << operation  << std::endl; std::cout.flush();
    //finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false);
    finishDraw();
}

void OpenParEMg::closePolyline ()
{
    activePolywire->close();
    //finishPolyline();
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

            Polywire *polywire=item->get_Polywire();

            // stretch
            if (polywire && item->getEnableStretch() && item->hasP0()) {
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

    if (cancel) finishOperation(pnt,0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),true,11);

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

            Polywire *polywire=item->get_Polywire();
            if (polywire) {

                // stretch
                if (item->getEnableStretch()) {
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
        finishOperation(gp_Pnt(0,0,0),0,0,gp_Pnt(0,0,0),gp_Pnt(0,0,0),false,12);
    }

    lastMousePosition=pnt;
}

// pass in variables needed by various operations; not all operations use all variables
void OpenParEMg::finishOperation (gp_Pnt pnt, double length, double angleDegrees, gp_Pnt p1, gp_Pnt p2, bool cancel, int source)
{
    std::cout << "OpenParEMg::finishOperation  cancel=" << cancel << "  source=" << source << std::endl; std::cout.flush();

    if (cancel) {

        if (activePolywire && activePolywire->getDrawEnable()) {
            activePolywire->setDrawEnable(false);
            activePolywire->deleteRubberband();
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

                Polywire *polywire=item->get_Polywire();
                if (polywire) {

                    // stretch
                    if (item->getEnableStretch()) {
                        item->setEnableStretch(false);
                        polywire->deleteRubberband();
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
        if (lengthInputForm) finishExtrudePolywire(length,false);
        if (vectorInputForm) finishPlaneSetToFace(p1,p2);
        if (rotateInputForm) finishRotateObject(angleDegrees,p1,p2);
        if (lineEditForm || rectangleEditForm || polycircleEditForm || lengthEditForm) finishEditObject(length,false);
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

    // refresh the selection to enable further operations on the selected items
    ui->drawingWindow->refreshSelectedItems();

    ui->drawingWindow->updateViewer();
    setMenusI(0);
}


