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
#include <thread>

//#include "petscsys.h"
#include "MeshOptions.h"
#include "SimulateOptions.h"
#include "about.h"
#include "license.h"
#include "FrequencyPlanG.h"
#include "Refinement.h"
#include "Materials.h"
#include "CustomOpenGLWidget.h"
#include "SelectMaterialsDatabase.h"
#include "CustomTreeWidgetItem.h"
#include "MaterialSelection.h"
#include "mpi.h"
#include "RectangleSelector.h"

OpenParEMg::OpenParEMg (QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OpenParEMg)
{
    ui->setupUi(this);

    MPI_PORT_COMM=nullptr;
    request=nullptr;

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

    ui->actionWireframe->setChecked(true);

    /////////////////////////////////////////////////////////////////////////////
    // drawing window
    /////////////////////////////////////////////////////////////////////////////

    ui->drawingWindow->set_drawingItemTree(&drawing);
    ui->drawingWindow->set_portItemTree(&port);
    ui->drawingWindow->set_boundaryItemTree(&boundary);
    ui->drawingWindow->set_meshItemTree(&mesh);

    showAction=nullptr;
    hideAction=nullptr;
    unselectAction=nullptr;
    deleteAction=nullptr;

    /////////////////////////////////////////////////////////////////////////////
    // item selection tree
    /////////////////////////////////////////////////////////////////////////////

    drawing.set_root(true);
    port.set_root(true);
    boundary.set_root(true);
    mesh.set_root(true);

    drawing.set_type(0);
    port.set_type(1);
    boundary.set_type(2);
    mesh.set_type(3);

    ui->drawingItemTree->setHeaderHidden(true);

    // four base list items: drawing, port, boundary, and mesh
    drawing.setText(0,"Drawing");
    ui->drawingItemTree->addTopLevelItem(&drawing);
    port.setText(0,"Port");
    ui->drawingItemTree->addTopLevelItem(&port);
    boundary.setText(0,"Boundary");
    ui->drawingItemTree->addTopLevelItem(&boundary);
    mesh.setText(0,"Mesh");
    mesh.setForeground(0,Qt::black);
    ui->drawingItemTree->addTopLevelItem(&mesh);

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

    ui->menuRun->setToolTipsVisible(true);

    drawing.setForeground(0,Qt::gray);
    setRootForeground(&port);
    setRootForeground(&boundary);
    setRootForeground(&mesh);

    /////////////////////////////////////////////////////////////////////////////
    // context menu for drawingWindow
    /////////////////////////////////////////////////////////////////////////////

    if (showAction) delete showAction;
    showAction=new QAction("Show",this);
    if (hideAction) delete hideAction;
    hideAction=new QAction("Hide",this);
    //QAction *selectAction=new QAction("Select",this);
    if (unselectAction) delete unselectAction;
    QAction *unselectAction=new QAction("Unselect",this);
    if (deleteAction) delete deleteAction;
    QAction *deleteAction=new QAction("Delete",this);

    connect(showAction, &QAction::triggered, this, &OpenParEMg::showDrawingItems);
    connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideDrawingItems);
    //connect(selectAction, &QAction::triggered, this, &OpenParEMg::selectItems);
    connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectDrawingItems);
    connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deleteDrawingItems);

    drawingContextMenu=new QMenu(this);
    drawingContextMenu->addAction(showAction);
    drawingContextMenu->addAction(hideAction);
    //drawingContextMenu->addAction(selectAction);
    drawingContextMenu->addAction(unselectAction);
    drawingContextMenu->addAction(deleteAction);
    ui->drawingWindow->set_contextMenu(drawingContextMenu);

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
    if (timer) delete timer;
    if (MPI_PORT_COMM) MPI_Comm_free(MPI_PORT_COMM);
    if (request) MPI_Request_free(request);
    gmsh::finalize();
    PetscFinalize();
    delete ui;
}

void OpenParEMg::setMenus ()
{
    //printLockouts();

    if (projectFileLoaded) {
        ui->actionNew->setEnabled(false);
        ui->actionOpen->setEnabled(false);
        ui->actionSave->setEnabled(false);
        if (projectFileChanged) ui->actionSave->setEnabled(true);
        ui->actionSaveAs->setEnabled(true);
        ui->actionClose->setEnabled(true);
        ui->actionExit->setEnabled(true);
        ui->actionSelectMaterialsDatabase->setEnabled(true);
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
            ui->actionUnselectAll->setEnabled(true);
            ui->actionHideAll->setEnabled(true);
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

        if (projectFileChanged) {
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

void OpenParEMg::itemTreeContextMenu_triggered(const QPoint& pnt)
{
    std::cout << "OpenParEMg::itemTreeContextMenu_triggered" << std::endl; std::cout.flush();

    clickedItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemAt(pnt);
    if (!clickedItem) return;
    if (!clickedItem->isSelected()) return;

    //xxx
    clickedItem->print();

    QMenu menu(this);

    if (clickedItem->is_drawing()) {
        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);
        //QAction *selectAction=new QAction("Select",this);
        unselectAction=new QAction("Unselect",this);
        deleteAction=new QAction("Delete",this);
        assignMaterialAction=new QAction("Assign Material");


        connect(showAction, &QAction::triggered, this, &OpenParEMg::showDrawingItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideDrawingItems);
        //connect(selectAction, &QAction::triggered, this, &OpenParEMg::selectItems);
        connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectDrawingItems);
        connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deleteDrawingItems);
        connect(assignMaterialAction, &QAction::triggered, this, &OpenParEMg::assignMaterial);

        showAction->setEnabled(ui->drawingWindow->isValidShow());
        hideAction->setEnabled(ui->drawingWindow->isValidHide());
        unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
        deleteAction->setEnabled(ui->drawingWindow->isDrawingValidDelete());

        assignMaterialAction->setEnabled(false);
        if (clickedItem->is_solid()) assignMaterialAction->setEnabled(true);

        menu.addAction(showAction);
        menu.addAction(hideAction);
        //menu.addAction(selectAction);
        menu.addAction(unselectAction);
        menu.addAction(deleteAction);
        menu.addAction(assignMaterialAction);
    }

    if (clickedItem->is_port()) {

        showAction=new QAction("Show",this);
        hideAction=new QAction("Hide",this);

        connect(showAction, &QAction::triggered, this, &OpenParEMg::showPortItems);
        connect(hideAction, &QAction::triggered, this, &OpenParEMg::hidePortItems);

        showAction->setEnabled(ui->drawingWindow->isValidShow());
        hideAction->setEnabled(ui->drawingWindow->isValidHide());

        menu.addAction(showAction);
        menu.addAction(hideAction);
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

    menu.exec(ui->drawingItemTree->mapToGlobal(pnt));
}

void OpenParEMg::showDrawingItems ()
{
    std::cout << "OpenParEMg::showDrawingItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        ui->drawingWindow->showItem(item);
        i++;
    }
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isDrawingValidDelete());
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::hideDrawingItems ()
{
    std::cout << "OpenParEMg::hideDrawingItems" << std::endl; std::cout.flush();

    ui->drawingWindow->hideItems();
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isDrawingValidDelete());
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::showPortItems ()
{
    std::cout << "OpenParEMg::showPortItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_port()) {
            if (item->is_root()) {
                int i=0;
                while (i < item->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                    ui->drawingWindow->showItem(child);
                    child->setForeground(0,Qt::black);
                    i++;
                }
            } else {
                ui->drawingWindow->showItem(item);
            }
            item->setForeground(0,Qt::black);
        }
        i++;
    }

    setRootForeground(&port);
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
        if (item->is_port()) {
            if (item->is_root()) {
                int i=0;
                while (i < item->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                    ui->drawingWindow->hideItem(child);
                    child->setForeground(0,Qt::gray);
                    i++;
                }
            } else {
                ui->drawingWindow->hideItem(item);
            }
            item->setForeground(0,Qt::gray);
        }
        i++;
    }

    setRootForeground(&port);
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
        ui->drawingWindow->showItem(item);
        i++;
    }

    setRootForeground(&mesh);
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::hideMeshItems ()
{
    std::cout << "OpenParEMg::hideMeshItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        ui->drawingWindow->hideItem(item);
        i++;
    }

    setRootForeground(&mesh);
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::selectItems()
{
    std::cout << "OpenParEMg::selectItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        ui->drawingWindow->selectItem(item);
        i++;
    }
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isDrawingValidDelete());
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::unselectDrawingItems()
{
    std::cout << "OpenParEMg::unselectItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        ui->drawingWindow->unselectItem(item);
        i++;
    }
    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isDrawingValidDelete());
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::deleteDrawingItems()
{
    std::cout << "OpenParEMg::deleteDrawingItems" << std::endl; std::cout.flush();

    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        ui->drawingWindow->deleteItem(item);
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    showAction->setEnabled(ui->drawingWindow->isValidShow());
    hideAction->setEnabled(ui->drawingWindow->isValidHide());
    unselectAction->setEnabled(ui->drawingWindow->hasDrawingSelectedItems());
    deleteAction->setEnabled(ui->drawingWindow->isDrawingValidDelete());

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::setRootForeground (CustomTreeWidgetItem * rootItem)
{
    int blackCount=0;
    int i=0;
    while (i < rootItem->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) rootItem->child(i);
        if (child->foreground(0) == Qt::black) blackCount++;
        i++;
    }

    rootItem->setForeground(0,Qt::gray);
    if (blackCount > 0) rootItem->setForeground(0,Qt::black);
}

void OpenParEMg::showDisplayShape (CustomTreeWidgetItem *item)
{
    std::cout << "OpenParEMg::showDisplayShape" << std::endl; std::cout.flush();
    ui->drawingWindow->showItem(item);
}

void OpenParEMg::hideDisplayShape (CustomTreeWidgetItem *item)
{
    std::cout << "OpenParEMg::hideDisplayShape" << std::endl; std::cout.flush();
    ui->drawingWindow->hideItem(item);
}

void OpenParEMg::selectDisplayShape (CustomTreeWidgetItem *item)
{
    std::cout << "OpenParEMg::selectDisplayShape" << std::endl; std::cout.flush();
    ui->drawingWindow->hideItem(item);
}

void OpenParEMg::unselectDisplayShape (CustomTreeWidgetItem *item)
{
    std::cout << "OpenParEMg::unselectDisplayShape" << std::endl; std::cout.flush();
    ui->drawingWindow->unselectItem(item);
}

void OpenParEMg::showPortShape (CustomTreeWidgetItem *item)
{
    std::cout << "OpenParEMg::showPortShape" << std::endl; std::cout.flush();
    ui->drawingWindow->showItem(item);
}

void OpenParEMg::hidePortShape (CustomTreeWidgetItem *item)
{
    std::cout << "OpenParEMg::hidePortShape" << std::endl; std::cout.flush();
    ui->drawingWindow->hideItem(item);
}

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
            //boundaryDatabase->set_drawingToItemMap(&drawingToItemMap);
            boundaryDatabase->draw(&projData,ui->drawingWindow,ui->drawingItemTree,&port,&boundary,materialDatabase);
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

    setRootForeground(&port);
    setRootForeground(&boundary);
    setRootForeground(&mesh);
    setMenus();
}

void OpenParEMg::resetLockouts ()
{
    projectFileLoaded=false;
    projectFileChanged=false;
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
    port.reset();
    boundary.reset();
    mesh.reset();

    //reset the tracking
    ui->drawingWindow->reset();

    clickedItem=nullptr;
    previousClickedItem=nullptr;
    currentSelectionAction=nullptr;

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
    if (projectFileChanged || meshFileChanged) {
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

    // std::cout << "Faces using TopExp_Explorer:" << std::endl;
    // for (TopExp_Explorer anExplorer(theShape, TopAbs_FACE); anExplorer.More(); anExplorer.Next()) {
    //     const TopoDS_Face& aFace = TopoDS::Face(anExplorer.Current());
    //     std::cout << "  Found a Face" << std::endl;
    // }

    std::cout << "Solid using TopExp_Explorer:" << std::endl;
    for (TopExp_Explorer anExplorer(theShape, TopAbs_SOLID); anExplorer.More(); anExplorer.Next()) {
        const TopoDS_Solid& aSolid = TopoDS::Solid(anExplorer.Current());
        std::cout << "  Found a Solid" << std::endl;
    }

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
    if (projectFileChanged || meshFileChanged) {
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

    std::cout << "on_actionSelectMaterialsDatabase_triggered: projData->materials_check_limits=" << projData.materials_check_limits << std::endl; std::cout.flush();

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
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionShape;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    set_selectionMode(&drawing,0);
    set_selectionMode(&port,0);
    set_selectionMode(&boundary,0);
    ui->drawingWindow->reshowItems();
}

void OpenParEMg::on_actionVertex_triggered()
{
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionVertex;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    set_selectionMode(&drawing,1);
    set_selectionMode(&port,1);
    set_selectionMode(&boundary,1);
    ui->drawingWindow->reshowItems();
}

void OpenParEMg::on_actionEdge_triggered()
{
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionEdge;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    set_selectionMode(&drawing,2);
    set_selectionMode(&port,2);
    set_selectionMode(&boundary,2);
    ui->drawingWindow->reshowItems();
}

void OpenParEMg::on_actionWire_triggered()
{
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionWire;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    set_selectionMode(&drawing,3);
    set_selectionMode(&port,3);
    set_selectionMode(&boundary,3);
    ui->drawingWindow->reshowItems();
}

void OpenParEMg::on_actionFace_triggered()
{
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionFace;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    set_selectionMode(&drawing,4);
    set_selectionMode(&port,4);
    set_selectionMode(&boundary,4);
    ui->drawingWindow->reshowItems();
}

void OpenParEMg::on_actionShell_triggered()
{
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionShell;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    set_selectionMode(&drawing,5);
    set_selectionMode(&port,5);
    set_selectionMode(&boundary,5);
    ui->drawingWindow->reshowItems();
}

void OpenParEMg::on_actionSolid_triggered()
{
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionSolid;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    set_selectionMode(&drawing,6);
    set_selectionMode(&port,6);
    set_selectionMode(&boundary,6);
    ui->drawingWindow->reshowItems();
}

// click on background in the item tree to clear the selection
bool OpenParEMg::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->drawingItemTree->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (ui->drawingItemTree->indexAt(mouseEvent->pos()).isValid() == false) {
                ui->drawingItemTree->clearSelection();
                ui->drawingItemTree->setCurrentItem(nullptr);
                ui->drawingWindow->unselectAllItems();
                clickedItem=nullptr;
                previousClickedItem=nullptr;
                ui->drawingWindow->updateViewer();
                std::cout << "clear tree click" << std::endl; std::cout.flush();
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

void OpenParEMg::on_actionHideAll_triggered ()
{
    ui->drawingWindow->unselectAllItems();
    ui->drawingWindow->hideAllItems();
    clickedItem=nullptr;
    previousClickedItem=nullptr;
    ui->drawingWindow->updateViewer();
    setRootForeground(&port);
    setRootForeground(&boundary);
    setRootForeground(&mesh);
    setMenus();
}

void OpenParEMg::on_actionUnselectAll_triggered ()
{
    ui->drawingWindow->unselectAllItems();
    ui->drawingWindow->updateViewer();
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
        std::cout << "elementType=" << elementTypes[e] << std::endl; std::cout.flush();

        // vertices
        if (elementTypes[e] == 15) {
            CustomTreeWidgetItem *verticesItem=new CustomTreeWidgetItem(0);
            verticesItem->setText(0,"Vertices");
            verticesItem->set_type(3);
            verticesItem->setForeground(0,Qt::black);
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
            std::cout << "   element type 15: vertices: n=" << count << std::endl; std::cout.flush();
        }

        // edges
        if (elementTypes[e] == 1) {
            CustomTreeWidgetItem *edgesItem=new CustomTreeWidgetItem(0);
            edgesItem->setText(0,"Edges");
            edgesItem->set_type(3);
            edgesItem->setForeground(0,Qt::black);
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
            std::cout << "   element type 1: edges: n=" << count << std::endl; std::cout.flush();
        }

        // triangles
        if (elementTypes[e] == 2) {
            CustomTreeWidgetItem *wiresItem=new CustomTreeWidgetItem(0);
            wiresItem->setText(0,"Wires");
            wiresItem->set_type(3);
            wiresItem->setForeground(0,Qt::black);
            mesh.addChild(wiresItem);

            CustomTreeWidgetItem *trianglesItem=new CustomTreeWidgetItem(0);
            trianglesItem->setText(0,"Triangles");
            trianglesItem->set_type(3);
            trianglesItem->setForeground(0,Qt::black);
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
            std::cout << "   element type 2: triangles: n=" << count << std::endl; std::cout.flush();
        }

        // tetrahedron
        if (elementTypes[e] == 4) {
            CustomTreeWidgetItem *tetrahedronsItem=new CustomTreeWidgetItem(0);
            tetrahedronsItem->setText(0,"Tetrahedrons");
            tetrahedronsItem->set_type(3);
            tetrahedronsItem->setForeground(0,Qt::black);
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

            std::cout << "   element type 4: tetrahedra: n=" << count << std::endl; std::cout.flush();
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

        //xxx
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
    setRootForeground(&mesh);
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

    setRootForeground(&mesh);
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

