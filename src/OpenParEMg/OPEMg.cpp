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

//#include "petscsys.h"
#include "MeshOptions.h"
#include "SimulateOptions.h"
#include "about.h"
#include "license.h"
#include "FrequencyPlanG.h"
#include "Refinement.h"
#include "Materials.h"
#include "CustomOpenGLWidget.h"
#include "CustomLineEdit.h"
#include "SelectMaterialsDatabase.h"
#include "CustomTreeWidgetItem.h"
#include "MaterialSelection.h"
#include "mpi.h"
//#include "RectangleSelector.h"

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
    // lockouts
    /////////////////////////////////////////////////////////////////////////////

    resetLockouts();

    /////////////////////////////////////////////////////////////////////////////
    // main window setup
    /////////////////////////////////////////////////////////////////////////////

    absolutePath=QDir::currentPath();
    materialDatabase=new MaterialDatabase();
    boundaryDatabase=new BoundaryDatabase();

    projectFile="";
    init_project (&defaultData);
    init_project (&projData);

    ui->actionShape->setCheckable(true);
    ui->actionShape->setChecked(true);
    currentSelectionAction=ui->actionShape;
    selectionIndex=0;
    previousSelectionIndex=0;

    ui->actionWireframe->setChecked(true);

    /////////////////////////////////////////////////////////////////////////////
    // relay for receiving signals from controls
    /////////////////////////////////////////////////////////////////////////////

    relay=new Relay();
    connect(relay,&Relay::setMenus,this,&OpenParEMg::setMenus);
    connect(relay,&Relay::drawLineFinished,this,&OpenParEMg::drawLineFinished);
    connect(relay,&Relay::cancelDraw,this,&OpenParEMg::cancelDraw);

    /////////////////////////////////////////////////////////////////////////////
    // drawing window
    /////////////////////////////////////////////////////////////////////////////

    ui->drawingWindow->set_drawingItemTree(&drawing);
    ui->drawingWindow->set_portItemTree(&port);
    ui->drawingWindow->set_boundaryItemTree(&boundary);
    ui->drawingWindow->set_meshItemTree(&mesh);
    ui->drawingWindow->set_pathItemTree(&path);
    ui->drawingWindow->set_relay(relay);

    showAction=nullptr;
    hideAction=nullptr;
    unselectAction=nullptr;
    deleteAction=nullptr;
    assignAction=nullptr;
    insertAction=nullptr;
    renameAction=nullptr;
    createPortAction=nullptr;
    createPathAction=nullptr;
    drawAction=nullptr;

    /////////////////////////////////////////////////////////////////////////////
    // item selection tree
    /////////////////////////////////////////////////////////////////////////////

    drawing.set_type(100);
    port.set_type(101);
    boundary.set_type(102);
    mesh.set_type(103);
    path.set_type(104);

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

    drawing.setForeground(0,Qt::gray);
    //setRootForeground(&port);
    //setRootForeground(&boundary);
    //setRootForeground(&mesh);

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
    boundaryDatabaseChanged=false;

    /////////////////////////////////////////////////////////////////////////////
    // drawing
    /////////////////////////////////////////////////////////////////////////////

    isActiveDrawing=false;

    /////////////////////////////////////////////////////////////////////////////

    ui->drawingItemTree->show();
    ui->drawingWindow->show();
    setMenus();

    PetscInitializeNoArguments();
}

OpenParEMg::~OpenParEMg ()
{
    if (showAction) delete showAction;
    if (hideAction) delete hideAction;
    if (unselectAction) delete unselectAction;
    if (deleteAction) delete deleteAction;
    if (assignAction) delete assignAction;
    if (insertAction) delete insertAction;
    if (renameAction) delete renameAction;
    if (expandAllAction) delete expandAllAction;
    if (collapseAllAction) delete collapseAllAction;
    if (createPortAction) delete createPortAction;
    if (createPathAction) delete createPathAction;
    if (drawAction) delete drawAction;

    if (timer) delete timer;
    if (MPI_PORT_COMM) MPI_Comm_free(MPI_PORT_COMM);
    if (request) MPI_Request_free(request);
    gmsh::finalize();
    PetscFinalize();
    delete ui;
}

void OpenParEMg::setMenus ()
{
    std::cout << "OpenParEMg::setMenus" << std::endl; std::cout.flush();

    // boundaryDatabaseChanged is modified on the fly and in BoundaryDatabase methods, so update for both
    if (boundaryDatabaseLoaded && !boundaryDatabaseChanged) boundaryDatabaseChanged=boundaryDatabase->is_modified();
    if (boundaryDatabaseLoaded) {std::cout << "boundaryDatabase->is_modified()=" << boundaryDatabase->is_modified() << std::endl; std::cout.flush();}

    //printLockouts();

    // disable all menus while actively drawing
    std::cout << "OpenParEMg::setMenus: isActiveDrawing=" << isActiveDrawing << std::endl; std::cout.flush();
    if (isActiveDrawing) {
        ui->menubar->setEnabled(false);
        return;
    }

    ui->menubar->setEnabled(true);

    if (projectFileLoaded) {
        ui->actionNew->setEnabled(false);
        ui->actionOpen->setEnabled(false);
        ui->actionSave->setEnabled(false);
        if (projectFileChanged) ui->actionSave->setEnabled(true);
        if (boundaryDatabaseChanged) ui->actionSave->setEnabled(true);
        ui->actionSaveAs->setEnabled(true);
        ui->actionClose->setEnabled(true);
        ui->actionExit->setEnabled(true);
        ui->actionSelectMaterialsDatabase->setEnabled(true);
        ui->actionUnselectAll->setEnabled(true);
        ui->actionShowAll->setEnabled(true);
        ui->actionHideAll->setEnabled(true);
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

        if (brepFileLoaded || stepFileLoaded) {
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
            if (meshFileChanged) ui->actionMeshSave->setEnabled(true);
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
            if (meshFileChanged) {
                ui->actionRun->setEnabled(false);
                ui->actionRun->setToolTip("Run OpenParEM3D.");
                ui->actionStop->setEnabled(false);
                ui->actionAbort->setEnabled(false);
                ui->actionAbortAndExit->setEnabled(false);
            }
            // end run block
        }

        if (projectFileChanged || meshFileChanged || boundaryDatabaseChanged) {
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

            if (brepFileLoaded || stepFileLoaded) {
                if (ui->drawingWindow->hasOneFaceSelected()) {
                    ui->actionDrawingPlaneSetToFace->setEnabled(true);
                }
            } else {
                ui->actionDrawingPlaneSetToFace->setEnabled(false);
            }

        } else {
            ui->actionDrawingPlaneShow->setEnabled(true);
            ui->actionDrawingPlaneHide->setEnabled(false);
            ui->actionDrawingPlaneSnapToGrid->setEnabled(false);
            ui->actionDrawingPlaneSetToFace->setEnabled(false);
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

        ui->actionSelectMaterialsDatabase->setEnabled(false);

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
        ui->actionSelectMaterialsDatabase->setEnabled(true);
        ui->actionMeshOptions->setEnabled(true);
        ui->actionMeshLoad->setEnabled(false);
        ui->actionMeshSave->setEnabled(false);
        ui->actionMeshSaveAs->setEnabled(false);
        ui->actionMeshDelete->setEnabled(false);
        ui->actionSimulateOptions->setEnabled(true);
        ui->actionFrequencyPlan->setEnabled(true);
    }
}


void OpenParEMg::expand (CustomTreeWidgetItem *item)
{
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
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        expand(item);
        i++;
    }
}

void OpenParEMg::collapseAllItems ()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        collapse(item);
        i++;
    }
}

void OpenParEMg::itemTreeContextMenu_triggered (const QPoint& pnt)
{
    std::cout << "OpenParEMg::itemTreeContextMenu_triggered" << std::endl; std::cout.flush();

    clickedItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemAt(pnt);
    if (!clickedItem) return;
    if (!clickedItem->isSelected()) return;

    QMenu menu(this);

    if (clickedItem->is_rootDrawing()) {
        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        //QAction *selectAction=new QAction("Select",this);
        unselectAction=new QAction("Unselect",this);
        deleteAction=new QAction("Delete",this);
        assignAction=new QAction("Assign Material");

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showRootDrawingItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideRootDrawingItems);
        connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectRootDrawingItems);
        connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deleteRootDrawingItems);
        connect(assignAction, &QAction::triggered, this, &OpenParEMg::assignMaterial);

        showAction->setEnabled(ui->drawingWindow->isValidShow());
        hideAction->setEnabled(ui->drawingWindow->isValidHide());
        unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
        deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

        assignAction->setEnabled(false);
        if (clickedItem->is_solid()) assignAction->setEnabled(true);

        menu.addAction(showAction);
        menu.addAction(hideAction);
        menu.addAction(unselectAction);
        menu.addAction(deleteAction);
        menu.addAction(assignAction);
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

        showAction->setEnabled(ui->drawingWindow->isValidShow());
        hideAction->setEnabled(ui->drawingWindow->isValidHide());
        unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
        deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

        assignAction->setEnabled(false);
        if (clickedItem->is_solid()) assignAction->setEnabled(true);

        createPortAction->setEnabled(false);
        if (ui->drawingWindow->hasOneFaceSelected()) {createPortAction->setEnabled(true);}

        createPathAction->setEnabled(false);
        if (ui->drawingWindow->hasOneFaceSelected()) {createPathAction->setEnabled(true);}

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

        showAction->setEnabled(rootPathValidShow());
        hideAction->setEnabled(rootPathValidHide());

        menu.addAction(showAction);
        menu.addAction(hideAction);
        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_path()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showPathItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hidePathItems);

        showAction->setEnabled(ui->drawingWindow->isValidShow());
        hideAction->setEnabled(ui->drawingWindow->isValidHide());

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

        showAction->setEnabled(ui->drawingWindow->isValidShow());
        hideAction->setEnabled(ui->drawingWindow->isValidHide());

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

        showAction->setEnabled(ui->drawingWindow->isValidShow());
        hideAction->setEnabled(ui->drawingWindow->isValidHide());
        unselectAction->setEnabled(ui->drawingWindow->hasPortSelectedItems());

        renameAction->setEnabled(false);
        if (treeSelectionCount() == 1 || treeSelectionCount() == 2) renameAction->setEnabled(true);

        deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

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

        showAction->setEnabled(rootMeshValidShow());
        hideAction->setEnabled(rootMeshValidHide());

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

        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_sport()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        renameAction=new QAction("Rename",this);
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showNetItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideNetItems);
        connect(renameAction, &QAction::triggered, this, &OpenParEMg::renameSportNet);
        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        renameAction->setEnabled(false);
        if (treeSelectionCount() == 1) renameAction->setEnabled(true);

        showAction->setEnabled(ui->drawingWindow->isNetValidShow());
        hideAction->setEnabled(ui->drawingWindow->isNetValidHide());

        menu.addAction(showAction);
        menu.addAction(hideAction);
        menu.addAction(renameAction);
        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_voltage() || clickedItem->is_current()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        drawAction=new QAction("Draw Path");
        insertAction=new QAction("Add Path");
        //unselectAction=new QAction("Unselect",this);
        //deleteAction=new QAction("Delete",this);
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showVIItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideVIItems);
        connect(drawAction, &QAction::triggered, this, &OpenParEMg::drawPath);
        connect(insertAction, &QAction::triggered, this, &OpenParEMg::insertSelectedPath);
        //connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectVIItems);
        //connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deleteVIItems);
        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        showAction->setEnabled(ui->drawingWindow->isValidShow());
        hideAction->setEnabled(ui->drawingWindow->isValidHide());
        //unselectAction->setEnabled(ui->drawingWindow->hasVISelectedItems());
        //deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

        drawAction->setEnabled(false);
        if (treeSelectionCount() == 1 && clickedItem->foreground(0) == Qt::black) drawAction->setEnabled(true);

        insertAction->setEnabled(insertActionValid());


        menu.addAction(showAction);
        menu.addAction(hideAction);
        menu.addAction(drawAction);
        menu.addAction(insertAction);
        //menu.addAction(unselectAction);
        //menu.addAction(deleteAction);
        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
    }

    if (clickedItem->is_integrationPathSegment()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        //unselectAction=new QAction("Unselect",this);
        //deleteAction=new QAction("Delete",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showIntegrationPathItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideIntegrationPathItems);
        //connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectVIItems);
        //connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deleteVIItems);

        showAction->setEnabled(ui->drawingWindow->isValidShow());
        hideAction->setEnabled(ui->drawingWindow->isValidHide());
        //unselectAction->setEnabled(ui->drawingWindow->hasVISelectedItems());
        //deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

        menu.addAction(showAction);
        menu.addAction(hideAction);
        //menu.addAction(unselectAction);
        //menu.addAction(deleteAction);
    }

    if (clickedItem->is_scale()) {
        expandAllAction=new QAction("Expand All",this);
        collapseAllAction=new QAction("Collapse All",this);

        connect(expandAllAction, &QAction::triggered, this, &OpenParEMg::expandAllItems);
        connect(collapseAllAction, &QAction::triggered, this, &OpenParEMg::collapseAllItems);

        menu.addAction(expandAllAction);
        menu.addAction(collapseAllAction);
    }

    menu.exec(ui->drawingItemTree->mapToGlobal(pnt));

    if (showAction) {delete showAction; showAction=nullptr;}
    if (hideAction) {delete hideAction; hideAction=nullptr;}
    if (unselectAction) {delete unselectAction; unselectAction=nullptr;}
    if (deleteAction) {delete deleteAction; deleteAction=nullptr;}
    if (assignAction) {delete assignAction; assignAction=nullptr;}
    if (insertAction) {delete insertAction; insertAction=nullptr;}
    if (createPortAction) {delete createPortAction; createPortAction=nullptr;}
    if (createPathAction) {delete createPathAction; createPathAction=nullptr;}
}

void OpenParEMg::drawingWindowContextMenu_triggered(const QPoint& pnt)
{
    std::cout << "OpenParEMg::drawingWindowContextMenu_triggered" << std::endl; std::cout.flush();

    if (!ui->drawingWindow->hasAnySelectedItems()) return;

    showAction=new QAction("Show",this);
    hideAction=new QAction("Hide",this);
    unselectAction=new QAction("Unselect",this);
    deleteAction=new QAction("Delete",this);
    createPortAction=new QAction("Create Port");
    createPortAction->setToolTip("Copy the selected face and create a port.");
    createPathAction=new QAction("Create Path");
    createPathAction->setToolTip("Copy the selected face and create a path.");

    connect(showAction, &QAction::triggered, this, &OpenParEMg::showDrawingItems);
    connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideDrawingItems);
    connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectDrawingItems);
    connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deleteDrawingItems);
    connect(createPortAction, &QAction::triggered, this, &OpenParEMg::createPort);
    connect(createPathAction, &QAction::triggered, this, &OpenParEMg::createPath);

    QMenu menu(this);
    menu.addAction(showAction);
    menu.addAction(hideAction);
    menu.addAction(unselectAction);
    menu.addAction(deleteAction);
    menu.addAction(createPortAction);
    menu.addAction(createPathAction);

    createPortAction->setEnabled(false);
    if (ui->drawingWindow->hasOneFaceSelected()) {createPortAction->setEnabled(true);}

    createPathAction->setEnabled(false);
    if (ui->drawingWindow->hasOneFaceSelected()) {createPathAction->setEnabled(true);}

    menu.exec(ui->drawingWindow->mapToGlobal(pnt));

    if (showAction) {delete showAction; showAction=nullptr;}
    if (hideAction) {delete hideAction; hideAction=nullptr;}
    if (unselectAction) {delete unselectAction; unselectAction=nullptr;}
    if (deleteAction) {delete deleteAction; deleteAction=nullptr;}
    if (createPortAction) {delete createPortAction; createPortAction=nullptr;}
    if (createPathAction) {delete createPathAction; createPathAction=nullptr;}
}

void OpenParEMg::showRootDrawingItems ()
{
    std::cout << "OpenParEMg::showRootDrawingItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        //if (item->is_rootDrawing()) {
            ui->drawingWindow->showItem(item);
        //}
        i++;
    }

    //setRootForeground(&drawing);
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::showDrawingItems ()
{
    std::cout << "OpenParEMg::showDrawingItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        //if (item->is_drawing()) {
            ui->drawingWindow->showItem(item);
        //}
        //if (item->is_port()) {
            ui->drawingWindow->showItem(item);
        //}
        i++;
    }

    //setRootForeground(&drawing);
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::hideRootDrawingItems ()
{
    std::cout << "OpenParEMg::hideRootDrawingItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        //if (item->is_rootDrawing()) {
            ui->drawingWindow->hideItem(item);
        //}
        i++;
    }

    //setRootForeground(&drawing);
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::hideDrawingItems ()
{
    std::cout << "OpenParEMg::hideDrawingItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        //if (item->is_drawing()) {
            ui->drawingWindow->hideItem(item);
        //}
        //if (item->is_path()) {
        //    ui->drawingWindow->hideItem(item);
        //}
        i++;
    }

    //setRootForeground(&drawing);
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::showRootPathItems ()
{
    std::cout << "OpenParEMg::showRootPathItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_rootPath()) {
            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                ui->drawingWindow->showItem(child);
                child->setForeground(0,Qt::black);
                j++;
            }
        } else {
            ui->drawingWindow->showItem(item);
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

bool OpenParEMg::rootPathValidShow ()
{
    int i=0;
    while (i < path.childCount()) {
        std::cout << "OpenParEMg::rootPathValidShow" << std::endl; std::cout.flush();
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) path.child(i);
        if (child->foreground(0) == Qt::gray) return true;
        i++;
    }
    return false;
}

void OpenParEMg::hideRootPathItems ()
{
    std::cout << "OpenParEMg::hideRootPathItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_rootPath()) {
            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                ui->drawingWindow->hideItem(child);
                child->setForeground(0,Qt::gray);
                j++;
            }
        } else {
            ui->drawingWindow->hideItem(item);
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::showPathItems ()
{
    std::cout << "OpenParEMg::showPathItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        //if (item->is_path()) {
            ui->drawingWindow->showItem(item);
        //}
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

bool OpenParEMg::rootPathValidHide ()
{
    int i=0;
    while (i < path.childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) path.child(i);
        if (child->foreground(0) == Qt::black) return true;
        i++;
    }
    return false;
}

void OpenParEMg::hidePathItems ()
{
    std::cout << "OpenParEMg::hidePathItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        //if (item->is_path()) {
            ui->drawingWindow->hideItem(item);
        //}
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::showRootPortItems ()
{
    std::cout << "OpenParEMg::showRootPortItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_rootPort()) {
            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                ui->drawingWindow->showItem(child);
                child->setForeground(0,Qt::black);
                j++;
            }
        } else {
            ui->drawingWindow->showItem(item);
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::showPortItems ()
{
    std::cout << "OpenParEMg::showPortItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        //if (item->is_port()) {
            ui->drawingWindow->showItem(item);
        //}
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::hideRootPortItems ()
{
    std::cout << "OpenParEMg::hideRootPortItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_rootPort()) {
            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                ui->drawingWindow->hideItem(child);
                j++;
            }
        } else {
            ui->drawingWindow->hideItem(item);
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::hidePortItems ()
{
    std::cout << "OpenParEMg::hidePortItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        //if (item->is_port()) {
            ui->drawingWindow->hideItem(item);
        //}
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::showRootMeshItems ()
{
    std::cout << "OpenParEMg::showRootMeshItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_rootMesh()) {
            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                ui->drawingWindow->showItem(child);
                child->setForeground(0,Qt::black);
                j++;
            }
        } else {
            ui->drawingWindow->showItem(item);
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
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

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_rootMesh()) {
            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                ui->drawingWindow->hideItem(child);
                child->setForeground(0,Qt::gray);
                j++;
            }
        } else {
          ui->drawingWindow->hideItem(item);
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::showMeshItems ()
{
    std::cout << "OpenParEMg::showMeshItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        //if (item->is_mesh()) {
            ui->drawingWindow->showItem(item);
        //}
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
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

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        //if (item->is_mesh()) {
            ui->drawingWindow->hideItem(item);
        //}
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::renameSportNet ()
{
    std::cout << "OpenParEMg::renameSportNet" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_sport()) {
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

void OpenParEMg::deleteSportNet ()
{
    std::cout << "OpenParEMg::deleteSportNet" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_sport()) {
            ui->drawingWindow->deleteItem(item);
            boundaryDatabaseChanged=true;
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
}

bool OpenParEMg::hasOneSelectedSport ()
{
    bool found=false;
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_sport()) {
            if (found) return false;
            found=true;
        }
        i++;
    }
    return true;
}

bool OpenParEMg::hasVoltage ()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_sport()) {
            Mode *mode=(Mode *)item->get_OPEMojbect();
            if (mode && mode->has_voltage()) return true;
        }
        i++;
    }
    return false;
}

bool OpenParEMg::hasCurrent ()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_sport()) {
            Mode *mode=(Mode *)item->get_OPEMojbect();
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
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *selectedItem=(CustomTreeWidgetItem *)selectedItems[i];
        if (selectedItem->is_path()) {
            Path *path=(Path *)selectedItem->get_OPEMojbect();
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

    if (!modeParentItem->get_OPEMojbect()) {
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
    Mode *mode=(Mode *)modeParentItem->get_OPEMojbect();
    IntegrationPath *integrationPath=nullptr;

    // add paths to the mode
    bool addScaleToTree=false;
    if (item->childCount() == 0) {
        // new integration path
        integrationPath=mode->addIntegrationPath(pathList,&pathsToAdd,item->text(0).toStdString());
        addScaleToTree=true;
    } else {
        // existing integration path
        //xxx
        int i=0;
        while (i < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
            std::cout << "child->get_type()=" << child->get_type() << std::endl; std::cout.flush();
            if (child->is_scale()) {
                int j=0;
                while (j < child->childCount()) {
                    CustomTreeWidgetItem *grandChild=(CustomTreeWidgetItem *)child->child(j);
                    std::cout << "   grandChild->get_type()=" << grandChild->get_type() << std::endl; std::cout.flush();
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
        itemScale->set_type(12);
        itemScale->setFlags(item->flags() & ~Qt::ItemIsEditable);
        itemScale->setToolTip(0,"Scale factor for the integration path.");
        item->addChild(itemScale);

        CustomTreeWidgetItem *itemScaleValue=new CustomTreeWidgetItem(0);
        itemScaleValue->set_type(13);
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
        pathItem->set_type(14);
        pathItem->setForeground(0,Qt::gray);
        pathItem->set_OPEMobject(pathsToAdd[i]);
        item->addChild(pathItem);
        pathItemList[i]->push_linkedItem(pathItem);
        pathItem->push_linkedItem(pathItemList[i]);
        ui->drawingWindow->showItem(pathItem);
        i++;
    }

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::rename_returnPressed ()
{
    std::cout << "OpenParEMg::rename_returnPressed" << std::endl; std::cout.flush();

    // new text
    QString net=renameEdit->text();
    if (originalText.compare(net) != 0) {
        if (renameItem->is_port()) {
            Port *port=(Port *)renameItem->get_OPEMojbect();
            if (port) {
                port->set_name(net.toStdString());
                port->set_modified();
                boundaryDatabase->set_modified();
                boundaryDatabaseChanged=true;
            }
        }

        if (renameItem->is_sport()) {
            Mode *mode=(Mode *)renameItem->get_OPEMojbect();
            if (mode) {
                mode->set_net(net.toStdString());
                mode->set_modified();
                boundaryDatabase->set_modified();
                boundaryDatabaseChanged=true;
            }
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

    setMenus();
    ui->drawingWindow->updateViewer();
}

// void OpenParEMg::selectItems()
// {
//     std::cout << "OpenParEMg::selectItems" << std::endl; std::cout.flush();

//     QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
//     int i=0;
//     while (i < selectedItems.count()) {
//         CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
//         ui->drawingWindow->selectItem(item);
//         i++;
//     }
//     showAction->setEnabled(ui->drawingWindow->isValidShow());
//     hideAction->setEnabled(ui->drawingWindow->isValidHide());
//     unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
//     deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

//     ui->drawingWindow->updateViewer();
// }

void OpenParEMg::unselectRootDrawingItems()
{
    std::cout << "OpenParEMg::unselectRootDrawingItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_rootDrawing()) {
            ui->drawingWindow->unselectItem(item);
        }
        i++;
    }

    //setRootForeground(&drawing);
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::unselectDrawingItems()
{
    std::cout << "OpenParEMg::unselectDrawingItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_drawing()) {
            ui->drawingWindow->unselectItem(item);
        }
        i++;
    }

    //setRootForeground(&drawing);
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::deleteRootDrawingItems()
{
    std::cout << "OpenParEMg::deleteRootDrawingItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_rootDrawing()) {
            ui->drawingWindow->deleteItem(item);
        }
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    //setRootForeground(&drawing);
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::deleteDrawingItems()
{
    std::cout << "OpenParEMg::deleteDrawingItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_drawing()) {
            ui->drawingWindow->deleteItem(item);
        }
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    //setRootForeground(&drawing);
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::unselectRootPortItems()
{
    std::cout << "OpenParEMg::unselectRootPortItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_rootPort()) {
            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(j);
                ui->drawingWindow->unselectItem(child);
                j++;
            }
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasPortSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::insertModeItems ()
{
    std::cout << "OpenParEMg::insertModeItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_port()) {
            Port *port=boundaryDatabase->get_port(item->text(0).toStdString());

            Mode *newMode=new Mode(0,0,port->get_impedance_calculation());
            std::string net="net";
            net.append(std::to_string(boundaryDatabase->get_SportCount()+1));
            newMode->set_net(net);
            newMode->set_Sport(boundaryDatabase->get_SportCount()+1);
            port->push_mode(newMode);

            newMode->draw(relay,boundaryDatabase,ui->drawingWindow,ui->drawingItemTree,&path,item);

            port->set_modified();
            newMode->set_modified();
            boundaryDatabase->set_modified();
            boundaryDatabaseChanged=true;
        }
        i++;
    }
    setMenus();
}

void OpenParEMg::unselectPortItems()
{
    std::cout << "OpenParEMg::unselectPortItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_port()) {
            ui->drawingWindow->unselectItem(item);
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasPortSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::renamePortItems ()
{
    std::cout << "OpenParEMg::renamePortItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_port()) {
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

    setMenus();
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::deletePortItem (CustomTreeWidgetItem * item)
{
    std::cout << "OpenParEMg::deletePortItem" << std::endl; std::cout.flush();

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

    // remove the port outline
    ui->drawingWindow->deleteItem(item);

    boundaryDatabaseChanged=true;
    setMenus();
}

void OpenParEMg::deleteRootPortItems ()
{
    std::cout << "OpenParEMg::deleteRootPortItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_rootPort()) {
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

    // showAction->setEnabled(ui->drawingWindow->isValidShow());
    // hideAction->setEnabled(ui->drawingWindow->isValidHide());
    // unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    // deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    // setMenus();
    // ui->drawingWindow->updateViewer();
}

void OpenParEMg::deletePortItems ()
{
    std::cout << "OpenParEMg::deletePortItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_port()) {
            deletePortItem(item);
        }
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    // showAction->setEnabled(ui->drawingWindow->isValidShow());
    // hideAction->setEnabled(ui->drawingWindow->isValidHide());
    // unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    // deleteAction->setEnabled(ui->drawingWindow->isValidDelete());

    // boundaryDatabaseChanged=true;
    // setMenus();
    // ui->drawingWindow->updateViewer();
}

void OpenParEMg::deleteSportItem (CustomTreeWidgetItem *sportItem)
{
    std::cout << "OpenParEMg::deleteSportItem" << std::endl; std::cout.flush();

    // port
    CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)sportItem->QTreeWidgetItem::parent();
    Port *port=boundaryDatabase->get_port(parentItem->text(0).toStdString());

    // remove from the port
    port->deleteMode(sportItem->text(0).toStdString());

    // remove the integration paths
    int i=0;
    while (i < sportItem->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) sportItem->child(i);
        if (child->is_sport()) {
            int j=0;
            while (j < child->childCount()) {
                CustomTreeWidgetItem *grandChild=(CustomTreeWidgetItem *) child->child(j);
                if (grandChild->is_voltage() || grandChild->is_current()) {
                    int k=0;
                    while (k < grandChild->childCount()) {
                        CustomTreeWidgetItem *greatGrandChild=(CustomTreeWidgetItem *) grandChild->child(k);
                        if (greatGrandChild->is_integrationPathSegment()) {
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

    boundaryDatabaseChanged=true;
    setMenus();
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::deleteSportItems ()
{
    std::cout << "OpenParEMg::deleteSportItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_sportLabel()) {
            deleteSportItem(item);
        }
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    // ui->drawingWindow->updateViewer();
}

void OpenParEMg::showNetItems ()
{
    std::cout << "OpenParEMg::showNetItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_sport()) {
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

    showAction->setEnabled(ui->drawingWindow->isNetValidShow());
    hideAction->setEnabled(ui->drawingWindow->isNetValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::showVIItems ()
{
    std::cout << "OpenParEMg::showVIItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_voltage() || item->is_current()) {
            ui->drawingWindow->showItem(item);
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isVIValidShow());
    hideAction->setEnabled(ui->drawingWindow->isVIValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::showIntegrationPathItems ()
{
    std::cout << "OpenParEMg::showIntegrationPathItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_integrationPathSegment()) {
            ui->drawingWindow->showItem(item);
            item->setForeground(0,Qt::black);
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::hideNetItems ()
{
    std::cout << "OpenParEMg::hideNetItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_sport()) {
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

    //setRootForeground(&port);
    showAction->setEnabled(ui->drawingWindow->isNetValidShow());
    hideAction->setEnabled(ui->drawingWindow->isNetValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::hideVIItems ()
{
    std::cout << "OpenParEMg::hideVIItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_voltage() || item->is_current()) {
            ui->drawingWindow->hideItem(item);
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isVIValidShow());
    hideAction->setEnabled(ui->drawingWindow->isVIValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::hideIntegrationPathItems ()
{
    std::cout << "OpenParEMg::hideIntegrationPathItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_integrationPathSegment()) {
            ui->drawingWindow->hideItem(item);
            item->setForeground(0,Qt::gray);
        }
        i++;
    }

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::createPath ()
{
    std::cout << "OpenParEMg::createPath" << std::endl; std::cout.flush();

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

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        newPath->addFacePoints(item->get_AIS_Shape(),true,true);
        i++;
    }

    boundaryDatabase->push_path(newPath);
    newPath->create_item(ui->drawingWindow,&path);

    // add new path to the drawing
    CustomTreeWidgetItem *item=newPath->get_item();
    if (item) {
        addShape(item->get_AIS_Shape()->Shape(),item,false);
        item->setForeground(0,Qt::gray);
        ui->drawingWindow->showItem(item);

        long unsigned int j=0;
        while (j < item->get_arrowHeads_size()) {
            ui->drawingWindow->displayShape(item->get_arrowHead(j),item->get_displayMode(),item->get_selectionMode());
            ui->drawingWindow->insertItemToMap(item->get_arrowHead(j),item);
            j++;
        }
    }

    // see if the path is within an existing port
    Port *port=boundaryDatabase->get_matchingPort(newPath);
    if (port) newPath->set_portItem(port->get_item());

    setMenus();
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::createPort ()
{
    std::cout << "OpenParEMg::createPort" << std::endl; std::cout.flush();

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

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        newPath->addFacePoints(item->get_AIS_Shape(),true,true);
        i++;
    }

    boundaryDatabase->push_path(newPath);
    newPath->create_item(ui->drawingWindow,&path);

    // add new path to the drawing
    CustomTreeWidgetItem *item=newPath->get_item();
    if (item) {
        addShape(item->get_AIS_Shape()->Shape(),item,false);
        item->setForeground(0,Qt::gray);
        ui->drawingWindow->showItem(item);

        long unsigned int j=0;
        while (j < item->get_arrowHeads_size()) {
            ui->drawingWindow->displayShape(item->get_arrowHead(j),item->get_displayMode(),item->get_selectionMode());
            ui->drawingWindow->insertItemToMap(item->get_arrowHead(j),item);
            j++;
        }
    }

    // port

    Port *newPort=new Port(0,0);
    newPort->set_name(portName);
    newPort->set_outline(newPath);
    newPort->set_modified();

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
    newMode->set_modified();

    // add to boundary database
    boundaryDatabase->push_port(newPort);
    boundaryDatabase->set_modified();
    boundaryDatabaseChanged=true;

    // draw it
    boundaryDatabase->draw_port(relay,newPort,&projData,ui->drawingWindow,ui->drawingItemTree,&path,&port,&boundary,materialDatabase);

    setMenus();
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
void OpenParEMg::set_selectionMode (CustomTreeWidgetItem *item, int selectionMode)
{
    item->set_selectionMode(selectionMode);

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        set_selectionMode(child,selectionMode);
        i++;
    }
}

// recursive
void OpenParEMg::set_displayMode (CustomTreeWidgetItem *item, int displayMode)
{
    item->set_displayMode(displayMode);

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        set_displayMode(child,displayMode);
        i++;
    }
}

void OpenParEMg::setPhysicalGroups ()
{
    int i=0;
    while (i < projData.physicalGroupMaterialCount) {
        std::vector<int> physicalGroupList;
        physicalGroupList.push_back(0);
        physicalGroupList[0]=projData.physicalGroupMaterials[i].tag;
        std::cout << "OpenParEMg::setPhysicalGroups:  dim=" << projData.physicalGroupMaterials[i].dim
                  << "  tag=" << projData.physicalGroupMaterials[i].tag
                  << "  materialName=" << projData.physicalGroupMaterials[i].materialName << std::endl; std::cout.flush();
        gmsh::model::addPhysicalGroup(projData.physicalGroupMaterials[i].dim,physicalGroupList,-1,projData.physicalGroupMaterials[i].materialName);
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
        deleteMesh();
        meshFileChanged=false;
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

        QByteArray byteArray=selectedMaterial.toUtf8();
        char *material=byteArray.data();
        add_physicalGroupMaterial(&projData,-1,clickedItem->get_dimTag().first,clickedItem->get_dimTag().second,material);
        projectFileChanged=true;
        setMenus();
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
    projectFile=testProjectFile;

    // break up the full path
    QFileInfo fileInfo(projectFile);
    absolutePath=fileInfo.absolutePath();
    QString projectName=fileInfo.fileName();

    QDir::setCurrent(absolutePath);
    projectFile=projectName;

    QString currentPath;
    currentPath=QDir::currentPath();

    // load the file
    if (QFile::exists(projectFile)) {

        //QDir::setCurrent(absolutePath);
        //projectFile=projectName;

        if (load_project_file (projectName.toLatin1().toStdString().c_str(),&projData,"   ")) {
            projectFile="";
            QDir::setCurrent(currentPath);

            QMessageBox mb;
            mb.critical(nullptr, "Error", "Unable to load the requested project file.");
            mb.setFixedSize(500, 200);

            on_actionClose_triggered();

            return;
        }

        /* stress test to look for leaks in loading/freeing projData; run while monitoring memory consumption with top
        std::cout << "starting memory test" << std::endl; std::cout.flush();
        int i=0;
        while (i < 1000000) {
            free_project(&projData);
            init_project (&projData);
            load_project_file (projectName.toLatin1().toStdString().c_str(),&projData,"   ");
            std::cout << "i=" << i << std::endl; std::cout.flush();
            i++;
        }
        exit(1); */

        // load materials
        if (materialDatabase->load_materials(projData.materials_global_path,projData.materials_global_name,
                                             projData.materials_local_path,projData.materials_local_name,
                                             projData.materials_check_limits)) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "Unable to load the specified materials files.");
            mb.setFixedSize(500, 200);
        }

        // load brep file, if defined
        if (strcmp(projData.gui_brep_file,"") != 0) {

            QString filePath=projData.gui_brep_file;

            if (loadBrepFile(filePath)) {
                QString message="Unable to load Brep file \"";
                message.append(filePath);
                message.append("\".");
                QMessageBox mb;
                mb.critical(nullptr, "Error",message);
                mb.setFixedSize(500, 200);
            }
        }

        // load boundaries, if any, and draw
        if (boundaryDatabase->load(projData.port_definition_file,projData.solution_check_closed_loop)) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "Error in loading port and boundary definitions.");
            mb.setFixedSize(500, 200);
        } else {
            boundaryDatabaseLoaded=true;
            boundaryDatabase->assignPathNormals();  // to correctly orient arrow heads
            // ToDo: rename draw since this does not actually draw
            boundaryDatabase->draw(relay,&projData,ui->drawingWindow,ui->drawingItemTree,&path,&port,&boundary,materialDatabase);

            // add paths to the drawing
            long unsigned int i=0;
            while (i < boundaryDatabase->get_pathList_size()) {
                Path *path=boundaryDatabase->get_path(i);
                CustomTreeWidgetItem *item=path->get_item();
                if (item) {
                    addShape(item->get_AIS_Shape()->Shape(),item,false);
                    item->setForeground(0,Qt::gray);
                    ui->drawingWindow->showItem(item);

                    long unsigned int j=0;
                    while (j < item->get_arrowHeads_size()) {
                        ui->drawingWindow->displayShape(item->get_arrowHead(j),item->get_displayMode(),item->get_selectionMode());
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
        loadMeshFile(QString::fromStdString(projData.mesh_file));

        ui->drawingWindow->fitAll();
        ui->drawingWindow->updateViewer();

        projData.modified=0;
        projectFileLoaded=true;
        projectFileChanged=false;
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

    //setRootForeground(&port);
    //setRootForeground(&boundary);
    //setRootForeground(&mesh);
    setMenus();
}

void OpenParEMg::resetLockouts ()
{
    projectFileLoaded=false;
    projectFileChanged=false;
    boundaryDatabaseLoaded=false;
    boundaryDatabaseChanged=false;
    meshFileLoaded=false;
    meshFileChanged=false;
    brepFileLoaded=false;
    stepFileLoaded=false;
    drawingPlaneShown=false;
    simulationRunning=false;
    simulationStopping=false;
    simulationAborting=false;
    setMenus();
}

void OpenParEMg::printLockouts ()
{
    std::cout << "Lockouts:" << std::endl
              << "   projectFileLoaded=" << projectFileLoaded << std::endl
              << "   projectFileChanged=" << projectFileChanged << std::endl
              << "   boundaryDatabaseLoaded=" << boundaryDatabaseLoaded << std::endl
              << "   boundaryDatabaseChanged=" << boundaryDatabaseChanged << std::endl
              << "   meshFileLoaded=" << meshFileLoaded << std::endl
              << "   meshFileChanged=" << meshFileChanged << std::endl
              << "   brepFileLoaded=" << brepFileLoaded << std::endl
              << "   stepFileLoaded=" << stepFileLoaded << std::endl
              << "   drawingPlaneShown=" << drawingPlaneShown << std::endl
              << "   simulationRunning=" << simulationRunning << std::endl
              << "   simulationStopping=" << simulationStopping << std::endl
              << "   simulationAborting=" << simulationAborting << std::endl;
}

void OpenParEMg::resetProject ()
{
    if (!projectFileLoaded) return;

    // project file
    projectFile="";
    free_project(&defaultData);
    free_project(&projData);

    // mesh
    deleteMesh();

    // reset material database
    if (materialDatabase) delete materialDatabase;
    materialDatabase=new MaterialDatabase();

    // reset boundary database
    if (boundaryDatabase) delete boundaryDatabase;
    boundaryDatabase=new BoundaryDatabase();

    // reset drawing window

    ui->drawingWindow->clearDrawing();
    ui->drawingWindow->updateViewer();

    // reset selection tree
    drawing.reset();
    path.reset();
    port.reset();
    boundary.reset();
    mesh.reset();

    //reset the tracking
    ui->drawingWindow->reset();

    clickedItem=nullptr;
    previousClickedItem=nullptr;
    workingItem=nullptr;
    currentSelectionAction=ui->actionShape;
    selectionIndex=0;
    previousSelectionIndex=0;

    resetLockouts();

    setMenus();
}

void OpenParEMg::on_actionNew_triggered ()
{
    resetProject();
    init_project (&defaultData);
    init_project (&projData);
    projData.modified=0;
    projectFileLoaded=true;
    setMenus();
}

void OpenParEMg::on_actionClose_triggered()
{
    if (projectFileChanged || meshFileChanged || boundaryDatabaseChanged) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,"OpenParEMg","There are unsaved changes.  Do you want to close anyway?",QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No) return;
    }
    resetProject();
    setMenus();
}

void OpenParEMg::on_actionMeshOptions_triggered ()
{
    MeshDialog *meshDialog=new MeshDialog();
    meshDialog->set_simulationRunning(simulationRunning);
    meshDialog->set_projData(&projData);
    meshDialog->exec();
    delete meshDialog;

    if (projData.modified) {
        projectFileChanged=true;
    }
    setMenus();
}

void OpenParEMg::on_actionSimulateOptions_triggered ()
{
    SimOptions *simOptions=new SimOptions();
    simOptions->set_simulationRunning(simulationRunning);
    simOptions->set_projData(&projData);
    simOptions->exec();
    delete simOptions;

    if (projData.modified) {
        projectFileChanged=true;
    }
    setMenus();
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
        projectFileChanged=true;
    }
    setMenus();
}

void OpenParEMg::on_actionSave_triggered ()
{
    // project
    if (projectFileLoaded) {
        if (QFile::exists(projectFile)) {
            if (save_project (projectFile.toStdString().c_str(),&projData,&defaultData,"")) {
                QString message="Error in saving the project file.";
                QMessageBox mb;
                mb.critical(nullptr, "Error",message);
                mb.setFixedSize(500, 200);
            }
            projData.modified=0;
            projectFileChanged=false;
        } else {
            on_actionSaveAs_triggered();
        }
    }

    // ports and boundaries
    if (boundaryDatabaseLoaded) {
        if (saveBoundaryDatabase()) {
            QString message="Error in saving the boundary database.";
            QMessageBox mb;
            mb.critical(nullptr, "Error",message);
            mb.setFixedSize(500, 200);
        }
    }

    // mesh
    if (meshFileLoaded) {
        on_actionMeshSave_triggered();
        meshFileChanged=false;
    } else {
        if (QFile::exists(projData.mesh_file)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this,"OpenParEMg","A mesh file exists.  Do you want to delete it?",QMessageBox::Yes|QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                if (!QFile::remove(projData.mesh_file)) {
                    QString message="Error in deleting the mesh file.";
                    QMessageBox mb;
                    mb.critical(nullptr, "Error",message);
                    mb.setFixedSize(500, 200);
                }
            }
        }
    }

    setMenus();
}

void OpenParEMg::on_actionSaveAs_triggered ()
{
    QString filePath=QFileDialog::getSaveFileName(this, tr("Open Project File"), absolutePath, tr("Project Files (*.proj)","All Files (*)"),
                                                  nullptr,QFileDialog::DontUseNativeDialog);
    if (filePath.isEmpty()) return;

    if (save_project (filePath.toStdString().c_str(),&projData,&defaultData,"")) {
        QString message="Error in saving the project file.";
        QMessageBox mb;
        mb.critical(nullptr, "Error",message);
        mb.setFixedSize(500, 200);
    } else {
        // replace the project file name with the new one
        QFileInfo fileInfo(filePath);
        projectFile=fileInfo.fileName();

        projData.modified=0;
        projectFileChanged=false;
    }

    // mesh
    if (meshFileLoaded) {
        on_actionMeshSave_triggered();
        meshFileChanged=false;
    } else {
        if (QFile::exists(projData.mesh_file)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this,"OpenParEMg","A mesh file exists.  Do you want to delete it?",QMessageBox::Yes|QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                if (!QFile::remove(projData.mesh_file)) {
                    QString message="Error in deleting the mesh file.";
                    QMessageBox mb;
                    mb.critical(nullptr, "Error",message);
                    mb.setFixedSize(500, 200);
                }
            }
        }
    }

    setMenus();
}

void OpenParEMg::on_actionRefinement_triggered ()
{
    OPEMg_Refinement *refinement=new OPEMg_Refinement();
    refinement->set_simulationRunning(simulationRunning);
    refinement->set_projData(&projData);
    refinement->exec();
    delete refinement;

    if (projData.modified) {
        projectFileChanged=true;
    }
    setMenus();
}

void OpenParEMg::on_actionMaterialsEditor_triggered ()
{
    Materials *localMaterials=new Materials();
    localMaterials->exec();
    delete localMaterials;

    if (projData.modified) {
        projectFileChanged=true;
    }
    setMenus();
}

void ListChildren (const TopoDS_Shape& theShape)
{
    // Using TopoDS_Iterator (iterates immediate sub-shapes)
    TopoDS_Iterator anIterator(theShape);
    std::cout << "Children using TopoDS_Iterator:" << std::endl;
    for (; anIterator.More(); anIterator.Next()) {
        const TopoDS_Shape& aChildShape = anIterator.Value();
        // You can then get the type of the child shape using aChildShape.ShapeType()
        std::cout << "  Child Shape Type: " << aChildShape.ShapeType() << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Faces using TopExp_Explorer:" << std::endl;
    for (TopExp_Explorer anExplorer(theShape, TopAbs_FACE); anExplorer.More(); anExplorer.Next()) {
        const TopoDS_Face& aFace = TopoDS::Face(anExplorer.Current());
        std::cout << "  Found a Face" << std::endl;
    }

    std::cout << "Solid using TopExp_Explorer:" << std::endl;
    for (TopExp_Explorer anExplorer(theShape, TopAbs_SOLID); anExplorer.More(); anExplorer.Next()) {
        const TopoDS_Solid& aSolid = TopoDS::Solid(anExplorer.Current());
        std::cout << "  Found a Solid" << std::endl;
    }

    std::cout << "Wire using TopExp_Explorer:" << std::endl;
    for (TopExp_Explorer anExplorer(theShape, TopAbs_WIRE); anExplorer.More(); anExplorer.Next()) {
        const TopoDS_Wire& aWire = TopoDS::Wire(anExplorer.Current());
        std::cout << "  Found a Wire" << std::endl;
    }

    std::cout << "CompSolid using TopExp_Explorer:" << std::endl;
    for (TopExp_Explorer anExplorer(theShape, TopAbs_COMPSOLID); anExplorer.More(); anExplorer.Next()) {
        const TopoDS_CompSolid& aCompSolid = TopoDS::CompSolid(anExplorer.Current());
        std::cout << "  Found a CompSolid" << std::endl;
    }

    std::cout << "Compound using TopExp_Explorer:" << std::endl;
    for (TopExp_Explorer anExplorer(theShape, TopAbs_COMPOUND); anExplorer.More(); anExplorer.Next()) {
        const TopoDS_Compound& aCompound = TopoDS::Compound(anExplorer.Current());
        std::cout << "  Found a Compound" << std::endl;
    }

    std::cout << "Edge using TopExp_Explorer:" << std::endl;
    for (TopExp_Explorer anExplorer(theShape, TopAbs_EDGE); anExplorer.More(); anExplorer.Next()) {
        const TopoDS_Edge& aEdge = TopoDS::Edge(anExplorer.Current());
        std::cout << "  Found a Edge" << std::endl;
    }

    std::cout << "Shell using TopExp_Explorer:" << std::endl;
    for (TopExp_Explorer anExplorer(theShape, TopAbs_SHELL); anExplorer.More(); anExplorer.Next()) {
        const TopoDS_Shell& aShell = TopoDS::Shell(anExplorer.Current());
        std::cout << "  Found a Shell" << std::endl;
    }

    std::cout << "Vertex using TopExp_Explorer:" << std::endl;
    for (TopExp_Explorer anExplorer(theShape, TopAbs_VERTEX); anExplorer.More(); anExplorer.Next()) {
        const TopoDS_Vertex& aVertex = TopoDS::Vertex(anExplorer.Current());
        std::cout << "  Found a Vertex" << std::endl;
    }
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

void OpenParEMg::addShape (TopoDS_Shape shape, CustomTreeWidgetItem *item, bool isRoot)
{
    if (shape.IsNull()) return;

    // tree item name

    QString name="";

    std::pair<int,int> dimTag;
    dimTag.first=-1; dimTag.second=-1;

    TopAbs_ShapeEnum shapeType=shape.ShapeType();
    switch (shapeType) {
    case TopAbs_COMPOUND:
        name="COMPOUND";
        break;
    case TopAbs_COMPSOLID:
        name="COMPSOLID";
        volumeCount++;   // a guess
        dimTag.first=3; dimTag.second=volumeCount++;
        break;
    case TopAbs_SOLID:
        name="SOLID";
        volumeCount++;
        dimTag.first=3; dimTag.second=volumeCount++;
        break;
    case TopAbs_SHELL:
        name="SHELL";
        surfaceCount++;  // a guess
        dimTag.first=2; dimTag.second=surfaceCount++;
        break;
    case TopAbs_FACE:
        name="FACE";
        surfaceCount++;  // aguess
        dimTag.first=2; dimTag.second=surfaceCount++;
        break;
    case TopAbs_WIRE:
        name="WIRE";
        curveCount++;    // a guess
        dimTag.first=2; dimTag.second=surfaceCount++;
        break;
    case TopAbs_EDGE:
        name="EDGE";
        curveCount++;
        dimTag.first=2; dimTag.second=surfaceCount++;
        break;
    case TopAbs_VERTEX:
        name="VERTEX";
        pointCount++;
        dimTag.first=1; dimTag.second=pointCount++;
        break;
    case TopAbs_SHAPE:
        name="SHAPE";
        break;
    default:
        std::cout << "ASSERT:: Unknown shape type." << std::endl;  std::cout.flush();
        break;
    }

    // item entry

    Handle(AIS_Shape) drawingShape=new AIS_Shape(shape);
    CustomTreeWidgetItem *newItem;

    if (isRoot) {
        isRoot=false;
        ui->drawingWindow->insertItemToMap(drawingShape,&drawing);
        drawing.set_AIS_Shape(drawingShape);
        ui->drawingWindow->showItem(&drawing);
        ui->drawingWindow->unselectItem(&drawing);
        newItem=&drawing;
    } else {
        newItem=new CustomTreeWidgetItem(0);
        newItem->set_AIS_Shape(drawingShape);
        newItem->setText(0,name);
        newItem->set_type(0);  // default value
        newItem->setForeground(0,Qt::gray);
        item->addChild(newItem);
        ui->drawingWindow->insertItemToMap(drawingShape,newItem);
    }

    // properties for the CustomTreeWidgetItem
    //std::cout << "checking to add physical group name to item list" << std::endl; std::cout.flush();
    newItem->set_dimTag(dimTag);
    if (name.compare("SOLID") == 0) {
        int i=0;
        while (i < projData.physicalGroupMaterialCount) {
            if (projData.physicalGroupMaterials[i].tag == dimTag.second) {
                newItem->setText(0,projData.physicalGroupMaterials[i].materialName);
                break;
            }
            i++;
        }
    }

    // children
    TopoDS_Iterator topoIterator(shape);
    while (topoIterator.More()) {
        const TopoDS_Shape& child=topoIterator.Value();
        addShape(child,newItem,isRoot);
        topoIterator.Next();
    }
}


bool OpenParEMg::loadBrepFile (QString filePath)
{
    bool retval=false;
    if (filePath.isEmpty()) {
        retval=true;
    } else {
        QFileInfo fileInfo(filePath);
        TopoDS_Shape s;
        BRep_Builder b;
        if (BRepTools::Read(s,filePath.toStdString().c_str(),b)) {
            addShape(s,nullptr,true);
            brepFileLoaded=true;
            projectFileChanged=true;
        } else retval=true;
    }
    setMenus();
    return retval;
}

bool OpenParEMg::loadStepFile (QString filePath)
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
            addShape(s,nullptr,true);
            stepFileLoaded=true;
            projectFileChanged=true;
        } else retval=true;
    }
    return retval;
}

bool OpenParEMg::saveStepFile (QString filePath, std::vector<Handle(AIS_InteractiveObject)> *selectedList)
{
    if (!filePath.isEmpty()) {

        // create a compund object from the selected objects
        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);
        Handle(AIS_Shape) shape;

        long unsigned int i=0;
        while (i < selectedList->size()) {
            shape=Handle(AIS_Shape)::DownCast((*selectedList)[i]);
            builder.Add(compound,shape->Shape());
            i++;
        }

        // write the STEP file

        STEPControl_Writer writer;
        writer.Transfer(compound,STEPControl_ManifoldSolidBrep,Standard_True);

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
    QString filename=absolutePath.append("/").append(projData.port_definition_file);
    std::cout << "OpenParEMg::saveBoundaryDatabase: filename=" << filename.toStdString() << std::endl; std::cout.flush();
    std::ofstream outputFile(filename.toStdString());
    if (outputFile.is_open()) {
        std::cout << "OpenParEMg::saveBoundaryDatabase:saving" << std::endl; std::cout.flush();
        boundaryDatabase->save(&outputFile);
        boundaryDatabaseChanged=false;
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
    if (loadBrepFile(filePath)) {
        QString message="Unable to load Brep file \"";
        message.append(filePath);
        message.append("\".");
        QMessageBox mb;
        mb.critical(nullptr, "Error",message);
        mb.setFixedSize(500, 200);
    } else {
        QFileInfo fileInfo(filePath);
        QString brepName=fileInfo.fileName();

        if (projData.gui_brep_file) free(projData.gui_brep_file);
        projData.gui_brep_file=(char *)malloc((brepName.length()+1)*sizeof(char));
        int i=0;
        while (i < brepName.length()) {
            projData.gui_brep_file[i]=brepName.data()[i].toLatin1();
            i++;
        }
        projData.gui_brep_file[i]='\0';
        projectFileChanged=true;
    }

    ui->drawingWindow->fitAll();
    ui->drawingWindow->updateViewer();
    setMenus();
}

void OpenParEMg::on_actionImportStep_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open STEP File"), "", tr("STEP Files (*.step *.stp)"),
                                                    nullptr,QFileDialog::DontUseNativeDialog);
    if (filePath.isEmpty()) return;
    if (loadStepFile(filePath)) {
        QString message="Unable to load STEP file \"";
        message.append(filePath);
        message.append("\".");
        QMessageBox mb;
        mb.critical(nullptr, "Error",message);
        mb.setFixedSize(500, 200);
    }

    ui->drawingWindow->fitAll();
    ui->drawingWindow->updateViewer();

    projectFileChanged=true;
    setMenus();
}

void OpenParEMg::on_actionExportStep_triggered()
{
    std::vector<Handle(AIS_InteractiveObject)> selectedList;
    ui->drawingWindow->getSelected (&selectedList);
    if (selectedList.size() == 0) {
        QMessageBox mb;
        mb.critical(nullptr,"Error","Select solid shapes to export.");
        mb.setFixedSize(500, 200);
        return;
    }

    QString filePath=QFileDialog::getSaveFileName(this,tr("Save STEP File"), "/home/briany/OpenParEM", tr("STEP Files (*.step *.stp)"),
                                                  nullptr,QFileDialog::DontUseNativeDialog);
    if (filePath.isNull()) return;

    if (saveStepFile(filePath,&selectedList)) {
        QString message="Unable to save STEP file \"";
        message.append(filePath);
        message.append("\".");
        QMessageBox mb;
        mb.critical(nullptr, "Error",message);
        mb.setFixedSize(500, 200);
    }
    setMenus();
}

void OpenParEMg::on_actionExit_triggered ()
{
    if (projectFileChanged || meshFileChanged || boundaryDatabaseChanged) {
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
        projectFileChanged=true;
    }
    setMenus();
}

void OpenParEMg::on_drawingItemTree_itemClicked (QTreeWidgetItem *item, int column)
{
    std::cout << "OpenParEMg::on_drawingItemTree_itemClicked" << std::endl; std::cout.flush();

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
        ui->drawingWindow->unselectAllItems();

        CustomTreeWidgetItem *clickedItemKeep=clickedItem;
        clearTreeSelection();
        clickedItem=clickedItemKeep;

        ui->drawingWindow->selectItem(clickedItem);
        previousClickedItem=clickedItem;
    }
    ui->drawingWindow->updateViewer();
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

void OpenParEMg::on_actionShape_triggered()
{
    std::cout << "OpenParEMg::on_actionShape_triggered" << std::endl; std::cout.flush();

    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionShape;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    previousSelectionIndex=selectionIndex;
    selectionIndex=0;
    set_selectionMode(&drawing,selectionIndex);
    set_selectionMode(&path,selectionIndex);
    set_selectionMode(&port,selectionIndex);
    set_selectionMode(&boundary,selectionIndex);
    ui->drawingWindow->reshowItems();
}

void OpenParEMg::on_actionVertex_triggered()
{
    std::cout << "OpenParEMg::on_actionVertex_triggered" << std::endl; std::cout.flush();

    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionVertex;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    previousSelectionIndex=selectionIndex;
    selectionIndex=1;
    set_selectionMode(&drawing,selectionIndex);
    set_selectionMode(&path,selectionIndex);
    set_selectionMode(&port,selectionIndex);
    set_selectionMode(&boundary,selectionIndex);
    ui->drawingWindow->reshowItems();
}

void OpenParEMg::on_actionEdge_triggered()
{
    std::cout << "OpenParEMg::on_actionEdge_triggered" << std::endl; std::cout.flush();

    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionEdge;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    previousSelectionIndex=selectionIndex;
    selectionIndex=2;
    set_selectionMode(&drawing,selectionIndex);
    set_selectionMode(&path,selectionIndex);
    set_selectionMode(&port,selectionIndex);
    set_selectionMode(&boundary,selectionIndex);
    ui->drawingWindow->reshowItems();
}

void OpenParEMg::on_actionWire_triggered()
{
    std::cout << "OpenParEMg::on_actionWire_triggered" << std::endl; std::cout.flush();

    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionWire;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    previousSelectionIndex=selectionIndex;
    selectionIndex=3;
    set_selectionMode(&drawing,selectionIndex);
    set_selectionMode(&path,selectionIndex);
    set_selectionMode(&port,selectionIndex);
    set_selectionMode(&boundary,selectionIndex);
    ui->drawingWindow->reshowItems();
}

void OpenParEMg::on_actionFace_triggered()
{
    std::cout << "OpenParEMg::on_actionFace_triggered" << std::endl; std::cout.flush();

    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionFace;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    previousSelectionIndex=selectionIndex;
    selectionIndex=4;
    set_selectionMode(&drawing,selectionIndex);
    set_selectionMode(&path,selectionIndex);
    set_selectionMode(&port,selectionIndex);
    set_selectionMode(&boundary,selectionIndex);
    ui->drawingWindow->reshowItems();
}

void OpenParEMg::on_actionShell_triggered()
{
    std::cout << "OpenParEMg::on_actionShell_triggered" << std::endl; std::cout.flush();

    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionShell;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    previousSelectionIndex=selectionIndex;
    selectionIndex=5;
    set_selectionMode(&drawing,selectionIndex);
    set_selectionMode(&path,selectionIndex);
    set_selectionMode(&port,selectionIndex);
    set_selectionMode(&boundary,selectionIndex);
    ui->drawingWindow->reshowItems();
}

void OpenParEMg::on_actionSolid_triggered()
{
    std::cout << "OpenParEMg::on_actionSolid_triggered" << std::endl; std::cout.flush();

    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionSolid;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    previousSelectionIndex=selectionIndex;
    selectionIndex=6;
    set_selectionMode(&drawing,selectionIndex);
    set_selectionMode(&path,selectionIndex);
    set_selectionMode(&port,selectionIndex);
    set_selectionMode(&boundary,selectionIndex);
    ui->drawingWindow->reshowItems();
}

int OpenParEMg::treeSelectionCount ()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    return selectedItems.count();
}

bool OpenParEMg::hasSelectedPaths ()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_path()) return true;
        i++;
    }
    return false;
}

void OpenParEMg::clearTreeSelection ()
{
    std::cout << "OpenParEMg::clearTreeSelection" << std::endl; std::cout.flush();
    ui->drawingItemTree->clearSelection();
    ui->drawingItemTree->setCurrentItem(nullptr);
    ui->drawingWindow->unselectAllItems();
    clickedItem=nullptr;
    previousClickedItem=nullptr;
    ui->drawingWindow->updateViewer();
}

// click on background in the item tree to clear the selection
bool OpenParEMg::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->drawingItemTree->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (ui->drawingItemTree->indexAt(mouseEvent->pos()).isValid() == false) {
                clearTreeSelection();
            }
            return false;
        }
    }
    return QObject::eventFilter(obj, event);
}

void OpenParEMg::keyPressEvent (QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control) {
        CTRLpressed=true;
    } else if (event->key() == Qt::Key_Shift) {
        SHIFTpressed=true;
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
    ui->drawingWindow->showItem(&drawing);
    ui->drawingWindow->showItem(&path);
    ui->drawingWindow->showItem(&port);
    ui->drawingWindow->showItem(&boundary);
    ui->drawingWindow->showItem(&mesh);
    setMenus();
    clickedItem=nullptr;
    previousClickedItem=nullptr;
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_actionHideAll_triggered ()
{
    ui->drawingWindow->unselectAllItems();
    ui->drawingWindow->hideAllItems();
    clearTreeSelection();
    //setRootForeground(&port);
    //setRootForeground(&boundary);
    //setRootForeground(&mesh);
    setMenus();
}

void OpenParEMg::on_actionUnselectAll_triggered ()
{
    ui->drawingWindow->unselectAllItems();
    ui->drawingWindow->updateViewer();
    clearTreeSelection();
    setMenus();
}

void OpenParEMg::drawMesh()
{
    std::cout << "OpenParEMg::drawMesh" << std::endl; std::cout.flush();

    // clear the mesh tree items
    //mesh.deleteChildren(&mesh);

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
            verticesItem->set_type(3);
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
            verticesItem->setSelected(false);
            ui->drawingWindow->showItem(verticesItem);
            ui->drawingWindow->unselectItem(verticesItem);
        }

        // edges
        if (elementTypes[e] == 1) {
            CustomTreeWidgetItem *edgesItem=new CustomTreeWidgetItem(0);
            edgesItem->setText(0,"Edges");
            edgesItem->set_type(3);
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
            edgesItem->setSelected(false);
            ui->drawingWindow->showItem(edgesItem);
            ui->drawingWindow->unselectItem(edgesItem);
        }

        // triangles
        if (elementTypes[e] == 2) {
            CustomTreeWidgetItem *wiresItem=new CustomTreeWidgetItem(0);
            wiresItem->setText(0,"Wires");
            wiresItem->set_type(3);
            wiresItem->setForeground(0,Qt::gray);
            mesh.addChild(wiresItem);

            CustomTreeWidgetItem *trianglesItem=new CustomTreeWidgetItem(0);
            trianglesItem->setText(0,"Triangles");
            trianglesItem->set_type(3);
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
            wiresItem->setSelected(false);
            ui->drawingWindow->showItem(wiresItem);
            ui->drawingWindow->unselectItem(wiresItem);
            trianglesItem->setSelected(false);
            ui->drawingWindow->showItem(trianglesItem);
            ui->drawingWindow->unselectItem(trianglesItem);
        }

        // tetrahedron
        if (elementTypes[e] == 4) {
            CustomTreeWidgetItem *tetrahedronsItem=new CustomTreeWidgetItem(0);
            tetrahedronsItem->setText(0,"Tetrahedrons");
            tetrahedronsItem->set_type(3);
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
                builder.Add(tetrahedron, face123);
                builder.Add(tetrahedron, face134);
                builder.Add(tetrahedron, face124);
                builder.Add(tetrahedron, face234);

                Handle(AIS_Shape) shape=new AIS_Shape(tetrahedron);
                tetrahedronsItem->get_meshEntities()->push_back(shape);

                i+=4;
                count++;
            }
            tetrahedronsItem->setSelected(false);
            ui->drawingWindow->showItem(tetrahedronsItem);
            ui->drawingWindow->unselectItem(tetrahedronsItem);
        }

        e++;
    }

    // update menus
    //ui->drawingWindow->set_hasMesh(true);
    ui->drawingWindow->fitAll();
    ui->drawingWindow->updateViewer();
    setMenus();
}

void OpenParEMg::deleteMesh ()
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

    //ui->drawingWindow->set_hasMesh(false);
    ui->drawingWindow->updateViewer();
    mesh.deleteChildren(&mesh);
    drawingEntities.clear();
    gmsh::clear();
    pointCount=0; curveCount=0; surfaceCount=0; volumeCount=0;
    //setRootForeground(&mesh);
}

void OpenParEMg::on_actionMeshGenerate_triggered ()
{
    if (meshFileLoaded) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,"OpenParEMg","Delete the existing mesh?",QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        deleteMesh();
    }

    // generate mesh
    TopoDS_Shape shape=drawing.get_AIS_Shape()->Shape();
    gmsh::model::occ::importShapesNativePointer((void *) &shape,drawingEntities,false);
    gmsh::model::occ::synchronize();
    gmsh::model::mesh::generate();

    meshFileLoaded=true;
    meshFileChanged=true;

    drawMesh();

    // set the physical groups with material names
    setPhysicalGroups();

    // gmsh::merge("phys_groups.msh");
    // gmsh::model::mesh::removeDuplicateNodes();     // ineffective
    // gmsh::model::mesh::removeDuplicateElements();  // ineffective
    // gmsh::model::mesh::optimize();                 // ineffective
    // gmsh::model::mesh::recombine();                // small improvement

    //setRootForeground(&mesh);
    setMenus();
}

void OpenParEMg::loadMeshFile (QString meshfile)
{
    if (QFile::exists(meshfile)) {

        if (meshFileLoaded) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this,"OpenParEMg","Delete the existing mesh?",QMessageBox::Yes|QMessageBox::No);
            if (reply != QMessageBox::Yes) return;

            deleteMesh();
        }

        // load and display
        //gmsh::model::remove();
        gmsh::open(meshfile.toStdString());

        meshFileLoaded=true;
        meshFileChanged=false;

        drawMesh();

        // save the file name if different
        if (meshfile.compare(projData.mesh_file) != 0) {
            if (projData.mesh_file) free(projData.mesh_file);
            projData.mesh_file=(char *)malloc((meshfile.length()+1)*sizeof(char));
            int i=0;
            while (i < meshfile.length()) {
                projData.mesh_file[i]=meshfile.data()[i].toLatin1();
                i++;
            }
            projData.mesh_file[i]='\0';
            projectFileChanged=true;
        }

    } else {
        // if (meshfile.compare("") != 0) {
        //     QMessageBox mb;
        //     QString message="Error in loading the specified mesh file \"";
        //     message.append(meshfile);
        //     message.append("\".");
        //     mb.critical(nullptr, "Error",message);
        //     mb.setFixedSize(500, 200);
        // }
    }
    setMenus();
}

void OpenParEMg::on_actionMeshLoad_triggered ()
{
    QString meshfile=QFileDialog::getOpenFileName(this,tr("Open Mesh"), "", tr("Mesh Files (*.msh);;All Files (*)"),
                                                  nullptr,QFileDialog::DontUseNativeDialog);

    // return if user cancels
    if (meshfile.isNull()) return;

    loadMeshFile(meshfile);
    setMenus();
}

void OpenParEMg::on_actionMeshSave_triggered ()
{
    if (strcmp(projData.mesh_file,"") != 0) {
        gmsh::write(projData.mesh_file);
        meshFileChanged=false;
    } else {
        on_actionMeshSaveAs_triggered();
    }
    setMenus();
}

void OpenParEMg::on_actionMeshSaveAs_triggered ()
{
    // get file name
    QString testMeshFile=QFileDialog::getSaveFileName(this,tr("Save Mesh File"), absolutePath, tr("Data Files (*.msh);;All Files (*)"),
                                                        nullptr,QFileDialog::DontUseNativeDialog);
    if (testMeshFile.isNull()) return;

    gmsh::write(testMeshFile.toStdString());
    meshFileChanged=false;

    // save the filename in projData
    if (testMeshFile.compare(projData.mesh_file) != 0) {
        if (projData.mesh_file) free(projData.mesh_file);
        projData.mesh_file=(char *) malloc((testMeshFile.size()+1)*sizeof(char));
        int i=0;
        while (i < testMeshFile.size()) {
            projData.mesh_file[i]=testMeshFile.data()[i].toLatin1();
            i++;
        }
        projData.mesh_file[i]='\0';
        projectFileChanged=true;
    }

    setMenus();
}

void OpenParEMg::on_actionMeshDelete_triggered ()
{
    deleteMesh();
    meshFileChanged=false;
    meshFileLoaded=false;
    setMenus();
}

void OpenParEMg::on_actionWireframe_triggered ()
{
    if (ui->actionWireframe->isChecked() == true) {
        set_displayMode(&drawing,0);
        set_displayMode(&port,0);
        set_displayMode(&boundary,0);
    } else {
        set_displayMode(&drawing,1);
        set_displayMode(&port,1);
        set_displayMode(&boundary,1);
    }

    ui->drawingWindow->reshowItems();
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

    char *project=(char *)malloc((projectFile.toLatin1().toStdString().length()+1)*sizeof(char));
    int i=0;
    while (i < projectFile.toLatin1().toStdString().length()) {
        project[i]=projectFile.data()[i].toLatin1();
        i++;
    }
    project[i]='\0';

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
    i=0;
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
        setMenus();

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
    setMenus();
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
        setMenus();

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
    setMenus();

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

void OpenParEMg::on_actionAbortAndExit_triggered()
{
    if (projectFileChanged) {
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

void OpenParEMg::on_actionDrawingPlaneShow_triggered()
{
    drawingPlaneShown=true;
    ui->drawingWindow->showGrid();
    ui->drawingWindow->updateViewer();
    setMenus();
}

void OpenParEMg::on_actionDrawingPlaneHide_triggered()
{
    drawingPlaneShown=false;
    ui->drawingWindow->hideGrid();
    ui->drawingWindow->updateViewer();
    setMenus();
}

void OpenParEMg::on_actionDrawingPlaneSnapToGrid_triggered()
{
    if (ui->actionDrawingPlaneSnapToGrid->isChecked()) {
        ui->drawingWindow->set_snapToGrid(true);
    } else {
        ui->drawingWindow->set_snapToGrid(false);
    }
    setMenus();
}

void OpenParEMg::on_actionDrawingPlaneSetToFace_triggered ()
{
    ui->drawingWindow->set_gridPlane();
    ui->drawingWindow->updateViewer();
    setMenus();
}

void OpenParEMg::on_actionSelectWithBox_triggered ()
{
    ui->drawingWindow->selectRectangle();
    setMenus();
}

void OpenParEMg::cancelDraw ()
{
    isActiveDrawing=false;
    ui->drawingWindow->set_drawLine(false);

    // restore the prior selection index
    if (previousSelectionIndex == 0) on_actionShape_triggered();
    else if (previousSelectionIndex == 1) on_actionVertex_triggered();
    else if (previousSelectionIndex == 2) on_actionEdge_triggered();
    else if (previousSelectionIndex == 3) on_actionWire_triggered();
    else if (previousSelectionIndex == 4) on_actionFace_triggered();
    else if (previousSelectionIndex == 5) on_actionShell_triggered();
    else if (previousSelectionIndex == 6) on_actionSolid_triggered();

    workingItem=nullptr;
    ui->drawingWindow->removeSelectOnVertex();
    ui->drawingWindow->updateViewer();
    setMenus();
}

void OpenParEMg::on_actionDrawLine_triggered ()
{
    isActiveDrawing=true;
    ui->drawingWindow->unselectAllItems();
    ui->drawingWindow->set_drawLine(true);
    ui->drawingWindow->updateViewer();
    setMenus();
}

void OpenParEMg::drawLineFinished (Handle(AIS_Shape) lineShape)
{
    std::cout << "OpenParEMg::drawLineFinished" << std::endl; std::cout.flush();

    isActiveDrawing=false;

    // add to tree
    if (ui->drawingWindow->get_isPath()) {

        // unique new path name
        std::string pathName="p";
        int i=1;
        while (boundaryDatabase->pathNameExists(pathName)) {
            std::string testName=pathName;
            testName.append("_").append(std::to_string(i));
            if (boundaryDatabase->pathNameExists(testName)) {i++;}
            else {pathName=testName; break;}
        }

        // path
        Path *newPath=new Path(0,0);
        newPath->set_name(pathName);
        newPath->set_closed(false);
        newPath->addWirePoints(lineShape);
        newPath->set_normal(normal);
        boundaryDatabase->push_path(newPath);

        // item
        newPath->create_item(ui->drawingWindow,&path,lineShape,true);

        // add new path to the drawing
        CustomTreeWidgetItem *item=newPath->get_item();
        if (item) {
            addShape(item->get_AIS_Shape()->Shape(),item,false);
            item->setForeground(0,Qt::gray);
            ui->drawingWindow->showItem(item);

            long unsigned int j=0;
            while (j < item->get_arrowHeads_size()) {
                ui->drawingWindow->displayShape(item->get_arrowHead(j),item->get_displayMode(),item->get_selectionMode());
                ui->drawingWindow->insertItemToMap(item->get_arrowHead(j),item);
                j++;
            }
        }

        // set portItem
        CustomTreeWidgetItem *modeParentItem=(CustomTreeWidgetItem *)workingItem->QTreeWidgetItem::parent();
        CustomTreeWidgetItem *portParentItem=(CustomTreeWidgetItem *)modeParentItem->QTreeWidgetItem::parent();
        newPath->set_portItem(portParentItem);

        // select
        ui->drawingWindow->selectItem(item);


        ui->drawingWindow->set_isPath(false);

        // add the path to the working item
        insertPath(workingItem);


    } else {
        //ToDo
    }

    // restore the prior selection index
    if (previousSelectionIndex == 0) on_actionShape_triggered();
    else if (previousSelectionIndex == 1) on_actionVertex_triggered();
    else if (previousSelectionIndex == 2) on_actionEdge_triggered();
    else if (previousSelectionIndex == 3) on_actionWire_triggered();
    else if (previousSelectionIndex == 4) on_actionFace_triggered();
    else if (previousSelectionIndex == 5) on_actionShell_triggered();
    else if (previousSelectionIndex == 6) on_actionSolid_triggered();

    workingItem=nullptr;
    ui->drawingWindow->removeSelectOnVertex();
    ui->drawingWindow->updateViewer();
    setMenus();
}

void OpenParEMg::drawPath ()
{
    std::cout << "OpenParEMg::drawPath" << std::endl; std::cout.flush();

    // to avoid stray clicks in the selection tree
    workingItem=clickedItem;

    // set to select on vertices
    on_actionVertex_triggered();

    // enable selection on just the port
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_voltage() || item->is_current()) {

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
                Path *portPath=(Path *)portItem->get_OPEMojbect();

                // select on vertices within the port path
                ui->drawingWindow->selectOnVertex(portPath);

                // get the normal to apply to the drawn Path
                // Since the drawing is confined to the drawn Path, the normals will be the same.
                normal=portPath->get_normal();
            }
        }
        i++;
    }

    ui->drawingWindow->set_isPath(true);
    on_actionDrawLine_triggered();
}

bool OpenParEMg::insertActionValid ()
{
    int VIcount=0;
    CustomTreeWidgetItem *VIitem;
    int pathCount=0;

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_voltage() || item->is_current()) {VIitem=item; VIcount++;}
        if (item->is_path()) pathCount++;
        i++;
    }

    if (VIcount != 1) return false;
    if (pathCount == 0) return false;

    // check that the paths are within the port

    CustomTreeWidgetItem *modeParentItem=(CustomTreeWidgetItem *)VIitem->QTreeWidgetItem::parent();
    CustomTreeWidgetItem *portParentItem=(CustomTreeWidgetItem *)modeParentItem->QTreeWidgetItem::parent();

    i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_path()) {
            Path *path=(Path *)item->get_OPEMojbect();
            if (path->get_portItem() != portParentItem) return false;
        }
        i++;
    }

    return true;
}

void OpenParEMg::insertSelectedPath ()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_voltage() || item->is_current()) {insertPath(item);}
        i++;
    }
}

