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

//#include "petscsys.h"
#include "MeshOptions.h"
#include "SimulateOptions.h"
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

    hasProjData=false;
    ui->meshOptions->setEnabled(false);
    ui->simulateOptions->setEnabled(false);
    ui->actionFrequency_Plan->setEnabled(false);
    ui->actionRefinement->setEnabled(false);

    ui->allWireframe->setChecked(true);

    /////////////////////////////////////////////////////////////////////////////
    // drawing window
    /////////////////////////////////////////////////////////////////////////////

    ui->drawingWindow->set_drawingItemTree(&drawing);
    ui->drawingWindow->set_portItemTree(&port);
    ui->drawingWindow->set_boundaryItemTree(&boundary);
    ui->drawingWindow->set_meshItemTree(&mesh);

    /////////////////////////////////////////////////////////////////////////////
    // item selection tree
    /////////////////////////////////////////////////////////////////////////////

    drawing.set_root(true);
    port.set_root(true);
    boundary.set_root(true);
    mesh.set_root(true);

    drawing.set_rootType(0);
    port.set_rootType(1);
    boundary.set_rootType(2);
    mesh.set_rootType(3);

    ui->drawingItemTree->setHeaderHidden(true);
    //ToDo: uncomment and work with the colors
    //ui->drawingItemTree->setItemDelegate(new CustomStyledItemDelegate(ui->drawingItemTree));

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

    // hash between the drawing and item tree
    std::cout << "OpenParEMg: drawingToItemMap=" << &drawingToItemMap << std::endl; std::cout.flush();
    ui->drawingWindow->set_drawingToItemMap(&drawingToItemMap);

    // menu initial settings
    ui->actionGenerate->setEnabled(false);
    ui->actionMeshSave->setEnabled(false);
    ui->actionDeleteMesh->setEnabled(false);
    ui->actionStop->setEnabled(false);
    ui->actionAbort->setEnabled(false);

    /////////////////////////////////////////////////////////////////////////////
    // context menu for drawingWindow
    /////////////////////////////////////////////////////////////////////////////

    QAction *hideAction=new QAction("Hide",this);
    QAction *selectAction=new QAction("Select",this);
    QAction *unselectAction=new QAction("Unselect",this);
    QAction *deleteAction=new QAction("Delete",this);

    connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideItems);
    connect(selectAction, &QAction::triggered, this, &OpenParEMg::selectItems);
    connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectItems);
    connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deleteItems);

    drawingContextMenu=new QMenu(this);
    drawingContextMenu->addAction(hideAction);
    drawingContextMenu->addAction(selectAction);
    drawingContextMenu->addAction(unselectAction);
    drawingContextMenu->addAction(deleteAction);
    ui->drawingWindow->set_contextMenu(drawingContextMenu);

    /////////////////////////////////////////////////////////////////////////////
    // gmsh
    /////////////////////////////////////////////////////////////////////////////

    gmsh::initialize();
    pointCount=0; curveCount=0; surfaceCount=0; volumeCount=0;

    /////////////////////////////////////////////////////////////////////////////
    // timer or checking when OpenParEM3D finishes
    /////////////////////////////////////////////////////////////////////////////

    timer=new QTimer(this);
    connect(timer,&QTimer::timeout,this,&OpenParEMg::checkFinish);

    /////////////////////////////////////////////////////////////////////////////

    ui->drawingItemTree->show();
    ui->drawingWindow->show();

    PetscInitializeNoArguments();
}

OpenParEMg::~OpenParEMg ()
{
    if (timer) delete timer;
    if (MPI_PORT_COMM) delete MPI_PORT_COMM;
    gmsh::finalize();
    PetscFinalize();
    delete ui;
}

bool OpenParEMg::menuAllHidden (CustomTreeWidgetItem *item)
{
    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        if (child->foreground(0) == Qt::black) return false;
        if (!menuAllHidden(child)) return false;
        i++;
    }
    return true;
}

bool OpenParEMg::menuAllShown(CustomTreeWidgetItem *item)
{
    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        if (child->foreground(0) == Qt::gray) return false;
        if (!menuAllShown(child)) return false;
        i++;
    }
    return true;
}

void OpenParEMg::itemTreeContextMenu_triggered(const QPoint& pnt)
{
    clickedItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemAt(pnt);
    if (!clickedItem) return;
    if (!clickedItem->isSelected()) return;

    QMenu menu(this);

    QAction *showAction=new QAction("Show",this);
    QAction *hideAction=new QAction("Hide",this);
    QAction *selectAction=new QAction("Select",this);
    QAction *unselectAction=new QAction("Unselect",this);
    QAction *deleteAction=new QAction("Delete",this);
    QAction *assignMaterialAction=new QAction("Assign Material");

    connect(showAction, &QAction::triggered, this, &OpenParEMg::showItems);
    connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideItems);
    connect(selectAction, &QAction::triggered, this, &OpenParEMg::selectItems);
    connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectItems);
    connect(deleteAction, &QAction::triggered, this, &OpenParEMg::deleteItems);
    connect(assignMaterialAction, &QAction::triggered, this, &OpenParEMg::assignMaterial);

    if (clickedItem->foreground(0) == Qt::black) {  // visible
        showAction->setEnabled(false);
        hideAction->setEnabled(true);
        selectAction->setEnabled(true);
        unselectAction->setEnabled(false);
        deleteAction->setEnabled(true);
        assignMaterialAction->setEnabled(false);
        //deleteAction->setEnabled(false);
    } else if (clickedItem->foreground(0) == Qt::gray) {  // invisible
        showAction->setEnabled(true);
        hideAction->setEnabled(false);
        selectAction->setEnabled(false);
        unselectAction->setEnabled(false);
        deleteAction->setEnabled(true);
        assignMaterialAction->setEnabled(false);
        //deleteAction->setEnabled(false);
    } else if (clickedItem->foreground(0) == Qt::red) {  // selected
        showAction->setEnabled(false);
        hideAction->setEnabled(true);
        selectAction->setEnabled(false);
        unselectAction->setEnabled(true);
        deleteAction->setEnabled(true);
        assignMaterialAction->setEnabled(false);
        //deleteAction->setEnabled(false);
    }

    // special cases
    if (clickedItem->is_port() && clickedItem->is_root()) selectAction->setEnabled(false);
    if (clickedItem->is_boundary() && clickedItem->is_root()) selectAction->setEnabled(false);
    if (clickedItem->is_mesh()) selectAction->setEnabled(false);
    if (clickedItem->is_solid()) assignMaterialAction->setEnabled(true);

    menu.addAction(showAction);
    menu.addAction(hideAction);
    menu.addAction(selectAction);
    menu.addAction(unselectAction);
    menu.addAction(deleteAction);
    menu.addAction(assignMaterialAction);

    menu.exec(ui->drawingItemTree->mapToGlobal(pnt));
}

void OpenParEMg::showItems ()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_drawing()) showDisplayShape(item);
        if (item->is_port()) showPortShape(item);
        if (item->is_boundary()) showPortShape(item);
        if (item->is_mesh()) showMeshEntitiesItem(item);
        i++;
    }
    ui->drawingWindow->updateViewer();
}

// for after changing the selection mode
// recursive
void OpenParEMg::reshowItem (CustomTreeWidgetItem *item)
{
    if (item->foreground(0) == Qt::black || item->foreground(0) == Qt::red) {
        ui->drawingWindow->redisplayShape(item->get_AIS_Shape(),item->get_displayMode(),item->get_selectionMode());
        if (item->foreground(0) == Qt::red) ui->drawingWindow->selectShape(item->get_AIS_Shape());
    }

    int i=0;
    while (i < item->childCount()) {
        reshowItem((CustomTreeWidgetItem *)item->child(i));
        i++;
    }
}

void OpenParEMg::hideItems ()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_drawing()) {hideDisplayShape(item);}
        if (item->is_port()) hidePortShape(item);
        if (item->is_boundary()) hidePortShape(item);
        if (item->is_mesh()) hideMeshEntitiesItem(item);
        i++;
    }
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::selectItems()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_drawing()) selectDisplayShape(item);
        if (item->is_port() && !item->is_root()) selectDisplayShape(item);
        if (item->is_boundary() && !item->is_root()) selectDisplayShape(item);

        i++;
    }
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::unselectItems()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_drawing()) unselectDisplayShape(item);
        if (item->is_port()) unselectDisplayShape(item);
        if (item->is_boundary()) unselectDisplayShape(item);
        i++;
    }
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::deleteItems()
{
    std::cout << "place 1" << std::endl; std::cout.flush();
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    std::cout << "place 2" << std::endl; std::cout.flush();
    int i=0;
    while (i < selectedItems.count()) {
        std::cout << "place 3" << std::endl; std::cout.flush();
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        std::cout << "place 4" << std::endl; std::cout.flush();
        if (item->is_drawing()) {deleteDisplayShape(item); delete item;}
        if (item->is_port()) {deleteDisplayShape(item); delete item;}
        if (item->is_boundary()) {deleteDisplayShape(item); delete item;}
        i++;
    }
    std::cout << "place 4" << std::endl; std::cout.flush();
    ui->drawingWindow->updateViewer();
    std::cout << "place 6" << std::endl; std::cout.flush();
}

// non-recursive
void OpenParEMg::showDisplayShape (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::black);
    ui->drawingWindow->displayShape(item->get_AIS_Shape(),item->get_displayMode(),item->get_selectionMode());
}

// recurseive
void OpenParEMg::hideDisplayShape (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::gray);
    ui->drawingWindow->hideShape(item->get_AIS_Shape());

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        hideDisplayShape(child);
        i++;
    }
}

// non-recursive
void OpenParEMg::selectDisplayShape (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::red);
    ui->drawingWindow->displayShape(item->get_AIS_Shape(),item->get_displayMode(),item->get_selectionMode());
    ui->drawingWindow->selectShape(item->get_AIS_Shape());
}

// non-recursive
void OpenParEMg::unselectDisplayShape (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::black);
    ui->drawingWindow->unselectShape(item->get_AIS_Shape());
}

// recursive
void OpenParEMg::deleteDisplayShape (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::gray);
    ui->drawingWindow->deleteShape(item->get_AIS_Shape());
    item->get_AIS_Shape().Nullify();

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        deleteDisplayShape(child);
        i++;
    }
}

void OpenParEMg::showPortShape (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::black);

    if (item->is_root()) {
        int j=0;
        while (j < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(j);
            ui->drawingWindow->displayShape(child->get_AIS_Shape(),item->get_displayMode(),item->get_selectionMode());
            child->setForeground(0,Qt::black);
            j++;
        }
    } else {
        ui->drawingWindow->displayShape(item->get_AIS_Shape(),item->get_displayMode(),item->get_selectionMode());
    }

    // do both port and boundary menu since this method is used for both

    if (menuAllHidden(&port)) port.setForeground(0,Qt::gray);
    else port.setForeground(0,Qt::black);

    if (menuAllHidden(&boundary)) boundary.setForeground(0,Qt::gray);
    else boundary.setForeground(0,Qt::black);
}

void OpenParEMg::hidePortShape (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::gray);

    if (item->is_root()) {
        int j=0;
        while (j < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(j);
            ui->drawingWindow->hideShape(child->get_AIS_Shape());
            child->setForeground(0,Qt::gray);
            j++;
        }
    } else {
        ui->drawingWindow->hideShape(item->get_AIS_Shape());
    }

    // do both port and boundary menu since this method is used for both

    if (menuAllHidden(&port)) port.setForeground(0,Qt::gray);
    else port.setForeground(0,Qt::black);

    if (menuAllHidden(&boundary)) boundary.setForeground(0,Qt::gray);
    else boundary.setForeground(0,Qt::black);
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

void OpenParEMg::assignMaterial ()
{
    MaterialSelection *materialSelection=new MaterialSelection();
    materialSelection->set_materialDatabase(materialDatabase);
    materialSelection->set_selectedMaterial(&selectedMaterial);
    materialSelection->populate();
    materialSelection->exec();
    delete materialSelection;

    if (selectedMaterial != "") {
        clickedItem->setText(0,selectedMaterial);
        std::vector<int> physicalGroups;
        physicalGroups.push_back(0);
        physicalGroups[0]=clickedItem->get_dimTag().second;
        gmsh::model::addPhysicalGroup(3,physicalGroups,-1,selectedMaterial.toStdString());
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

void OpenParEMg::on_fileOpen_triggered ()
{
    QString testProjectFile=QFileDialog::getOpenFileName(this,tr("Open Project"), "", tr("Project Files (*.proj);;All Files (*)"),
                                                         nullptr,QFileDialog::DontUseNativeDialog);

    // return if user cancels
    if (testProjectFile.isNull()) return;

    // reset as new
    on_fileNew_triggered ();
    projectFile=testProjectFile;

    // break up the full path
    QFileInfo fileInfo(projectFile);
    absolutePath=fileInfo.absolutePath();
    QString projectName=fileInfo.fileName();

    // load the file
    if (QFile::exists(projectFile)) {

        QString currentPath;
        currentPath=QDir::currentPath();

        QDir::setCurrent(absolutePath);
        projectFile=projectName;

        if (load_project_file (projectName.toLatin1().toStdString().c_str(),&projData,"   ")) {
            projectFile="";
            QDir::setCurrent(currentPath);

            QMessageBox mb;
            mb.critical(nullptr, "Error", "Unable to load the requested project file.");
            mb.setFixedSize(500, 200);

            return;
        }

        // set GUI options

        ui->meshOptions->setEnabled(true);
        ui->simulateOptions->setEnabled(true);
        ui->actionFrequency_Plan->setEnabled(true);

        if (strcmp(projData.refinement_frequency,"none") == 0) ui->actionRefinement->setEnabled(false);
        else ui->actionRefinement->setEnabled(true);

        // load materials
        if (materialDatabase->load_materials(projData.materials_global_path,projData.materials_global_name,
                                             projData.materials_local_path,projData.materials_local_name,
                                             projData.materials_check_limits)) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "Unable to load the specified materials files.  Use Tools->Materials Editor to select a materials database.");
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
            } else {
                ui->actionGenerate->setEnabled(true);
            }
        }

        // load boundaries, if any, and draw
        if (boundaryDatabase->load(projData.port_definition_file,projData.solution_check_closed_loop)) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "Error in loading port and boundary definitions.");
            mb.setFixedSize(500, 200);
        } else {
            boundaryDatabase->set_drawingToItemMap(&drawingToItemMap);
            boundaryDatabase->draw(&projData,ui->drawingWindow,ui->drawingItemTree,&port,&boundary,materialDatabase);
        }

        // load mesh, if any, and draw
        loadMeshFile(QString::fromStdString(projData.mesh_file));

        ui->drawingWindow->fitAll();
        ui->drawingWindow->updateViewer();

        // menu options
        ui->importBrep->setEnabled(false);
        ui->importSTEP->setEnabled(false);

    } else {
        // should not occur
        QMessageBox mb;
        mb.critical(nullptr, "Error", "The requested project file does not exist.");
        mb.setFixedSize(500, 200);
    }
}

void OpenParEMg::on_fileNew_triggered ()
{
    // reset project data

    projectFile="";

    free_project(&defaultData);
    free_project(&projData);

    init_project (&defaultData);
    init_project (&projData);

    // reset material database
    if (materialDatabase) delete materialDatabase;
    materialDatabase=new MaterialDatabase();

    // reset boundary database
    if (boundaryDatabase) delete boundaryDatabase;
    boundaryDatabase=new BoundaryDatabase();

    // reset GUI options

    ui->meshOptions->setEnabled(true);
    ui->actionGenerate->setEnabled(false);
    ui->actionMeshSave->setEnabled(false);
    ui->actionDeleteMesh->setEnabled(false);
    ui->simulateOptions->setEnabled(true);
    ui->actionFrequency_Plan->setEnabled(true);

    if (strcmp(projData.refinement_frequency,"none") == 0) ui->actionRefinement->setEnabled(false);
    else ui->actionRefinement->setEnabled(true);

    // reset drawing window

    ui->drawingWindow->clearDrawing();
    ui->drawingWindow->updateViewer();

    // reset selection tree
    drawing.reset();
    port.reset();
    boundary.reset();
    mesh.reset();

    // menu options
    ui->importBrep->setEnabled(true);
    ui->importSTEP->setEnabled(true);
}

void OpenParEMg::on_meshOptions_triggered ()
{
    MeshDialog *meshDialog=new MeshDialog();
    meshDialog->set_projData(&projData);
    meshDialog->exec();
    delete meshDialog;
}

void OpenParEMg::on_simulateOptions_triggered ()
{
    SimOptions *simOptions=new SimOptions();
    simOptions->set_projData(&projData);
    simOptions->exec();
    delete simOptions;
}

void OpenParEMg::on_actionLicense_triggered ()
{
    License *license=new License();
    license->exec();
    delete license;
}

void OpenParEMg::on_actionFrequency_Plan_triggered ()
{
    FrequencyPlanG *frequencyPlan=new FrequencyPlanG();
    frequencyPlan->set_projData(&projData);
    frequencyPlan->exec();
    delete frequencyPlan;

    if (strcmp(projData.refinement_frequency,"none") == 0) ui->actionRefinement->setEnabled(false);
    else ui->actionRefinement->setEnabled(true);
}

void OpenParEMg::on_actionSave_triggered ()
{
    print_project(&projData,&defaultData,"");
    std::cout.flush();
}

void OpenParEMg::on_actionRefinement_triggered ()
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
        drawing.set_AIS_Shape(drawingShape);
        drawing.setForeground(0,Qt::black);
        ui->drawingWindow->displayShape(drawingShape,drawing.get_displayMode(),drawing.get_selectionMode());
        newItem=&drawing;
    } else {
        newItem=new CustomTreeWidgetItem(0);
        newItem->set_AIS_Shape(drawingShape);
        newItem->setText(0,name);
        newItem->setForeground(0,Qt::gray);
        item->addChild(newItem);
    }
    drawingToItemMap.insert({drawingShape,newItem});

    newItem->set_dimTag(dimTag);

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
    if (filePath.isEmpty()) {
        return true;
    } else {

        QFileInfo fileInfo(filePath);
        //QString base=fileInfo.baseName();

        TopoDS_Shape s;
        BRep_Builder b;
        if (!BRepTools::Read(s,filePath.toStdString().c_str(),b)) return true;

        addShape(s,nullptr,true);
    }
    return false;
}

bool OpenParEMg::loadStepFile (QString filePath)
{
    if (!filePath.isEmpty()) {
        STEPControl_Reader reader;
        IFSelect_ReturnStatus status=reader.ReadFile(filePath.toStdString().c_str());
        if (status == IFSelect_RetDone) {
            reader.TransferRoots();
//xxx
            TopoDS_Shape s=reader.OneShape();
            addShape(s,nullptr,true);

            // seems equivalent
            // int shapeCount=reader.NbShapes();
            // int i=0;
            // while (i < reader.NbShapes()) {
            //     TopoDS_Shape s=reader.Shape(i+1);
            //     addShape(s,nullptr,true);
            //     i++;
            // }


            return false;
        }
    }
    return true;
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
        //xxx
        std::cout << "   IFSelect_ReturnStatus=" << status << std::endl; std::cout.flush();
        if (status == IFSelect_RetDone) {
            return false;
        }
    }
    return true;
}

void OpenParEMg::on_importBrep_triggered ()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open BREP File"), "", tr("BREP Files (*.brep)"),
                                                    nullptr,QFileDialog::DontUseNativeDialog);
    if (filePath.isEmpty()) return;
    if (loadBrepFile(filePath)) {
        QString message="Unable to load Brep file \"";
        message.append(filePath);
        message.append("\".");
        QMessageBox mb;
        mb.critical(nullptr, "Error",message);
        mb.setFixedSize(500, 200);
    }
    ui->drawingWindow->fitAll();
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_importSTEP_triggered()
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
}

//xxx
void OpenParEMg::on_exportSTEP_triggered()
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
}

void OpenParEMg::on_actionExit_triggered ()
{
    exit(0);
}

void OpenParEMg::on_actionSelect_Database_triggered ()
{
    SelectMaterialsDatabase *selectMaterialsDatabase=new SelectMaterialsDatabase(&projData,&absolutePath);
    selectMaterialsDatabase->exec();
    delete selectMaterialsDatabase;
}

void OpenParEMg::on_drawingItemTree_itemClicked (QTreeWidgetItem *item, int column)
{
    clickedItem=(CustomTreeWidgetItem *)item;
    clickedItem->setSelected(false);
    ui->drawingItemTree->setCurrentItem(nullptr);

    if (CTRLpressed) {
        if (SHIFTpressed) {
            ui->drawingItemTree->setCurrentItem(previousClickedItem);
        } else {
            ui->drawingItemTree->setCurrentItem(clickedItem);
            clickedItem->setSelected(true);
            previousClickedItem=clickedItem;
        }
    } else if (SHIFTpressed) {
        if (CTRLpressed) {
            ui->drawingItemTree->setCurrentItem(previousClickedItem);
        } else {
            if (previousClickedItem) {
                if (clickedItem->QTreeWidgetItem::parent() == previousClickedItem->QTreeWidgetItem::parent()) {

                    ui->drawingItemTree->clearSelection();

                    // select the end items

                    previousClickedItem->setSelected(true);
                    clickedItem->setSelected(true);

                    // select the middle items

                    int count=ui->drawingItemTree->indexFromItem(clickedItem,0).row()-
                              ui->drawingItemTree->indexFromItem(previousClickedItem,0).row();

                    if (count > 1) {  // forward
                        CustomTreeWidgetItem *nextItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemBelow(previousClickedItem);
                        int i=0;
                        while (i < count-1) {
                            //std::cout << "forward: nextItem=" << nextItem->text(0).toStdString() << std::endl; std::cout.flush();
                            if (nextItem->QTreeWidgetItem::parent() == clickedItem->QTreeWidgetItem::parent()) {
                                nextItem->setSelected(true);
                                i++;
                            }
                            nextItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemBelow(nextItem);
                        }
                    } else if (-count > 1) {  // reversed
                        CustomTreeWidgetItem *nextItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemBelow(clickedItem);
                        int i=0;
                        while (i < -count-1) {
                            //std::cout << "reverse: nextItem=" << nextItem->text(0).toStdString() << std::endl; std::cout.flush();
                            if (nextItem->QTreeWidgetItem::parent() == clickedItem->QTreeWidgetItem::parent()) {
                                nextItem->setSelected(true);
                                i++;
                            }
                            nextItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemBelow(nextItem);
                        }
                    }

                    previousClickedItem=clickedItem;
                } else {
                    ui->drawingItemTree->setCurrentItem(previousClickedItem);
                    previousClickedItem->setSelected(true);
                }
            }
        }
    } else {
        ui->drawingItemTree->clearSelection();
        ui->drawingItemTree->setCurrentItem(clickedItem);
        clickedItem->setSelected(true);
        previousClickedItem=clickedItem;
    }
}

void OpenParEMg::on_actionFit_Selected_triggered ()
{
    ui->drawingWindow->fitSelected();
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_actionFit_All_triggered ()
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
    reshowItem(&drawing);
    reshowItem(&port);
    reshowItem(&boundary);
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
    reshowItem(&drawing);
    reshowItem(&port);
    reshowItem(&boundary);
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
    reshowItem(&drawing);
    reshowItem(&port);
    reshowItem(&boundary);
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
    reshowItem(&drawing);
    reshowItem(&port);
    reshowItem(&boundary);
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
    reshowItem(&drawing);
    reshowItem(&port);
    reshowItem(&boundary);
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
    reshowItem(&drawing);
    reshowItem(&port);
    reshowItem(&boundary);
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
    reshowItem(&drawing);
    reshowItem(&port);
    reshowItem(&boundary);
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
                clickedItem=nullptr;
                previousClickedItem=nullptr;
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

void OpenParEMg::on_actionHide_All_triggered ()
{
    // drawing
    ui->drawingItemTree->clearSelection();
    drawing.setSelected(true);
    clickedItem=&drawing;
    previousClickedItem=nullptr;
    hideDisplayShape(&drawing);

    // port
    ui->drawingItemTree->clearSelection();
    port.setSelected(true);
    clickedItem=&port;
    previousClickedItem=nullptr;
    hidePortShape(&port);

    // boundary
    ui->drawingItemTree->clearSelection();
    boundary.setSelected(true);
    clickedItem=&boundary;
    previousClickedItem=nullptr;
    hidePortShape(&boundary);

    // mesh
    ui->drawingItemTree->clearSelection();
    mesh.setSelected(true);
    clickedItem=&mesh;
    previousClickedItem=nullptr;
    hideMeshEntitiesItem(&mesh);

    ui->drawingItemTree->clearSelection();
    ui->drawingItemTree->setCurrentItem(nullptr);
    clickedItem=nullptr;
    previousClickedItem=nullptr;
}

void OpenParEMg::on_actionShow_All_triggered ()
{
    // drawing
    ui->drawingItemTree->clearSelection();
    drawing.setSelected(true);
    clickedItem=&drawing;
    previousClickedItem=nullptr;
    //showDisplayShape();
    showDisplayShape(&drawing);

    // port
    ui->drawingItemTree->clearSelection();
    port.setSelected(true);
    clickedItem=&port;
    previousClickedItem=nullptr;
    //showPortShape();
    showPortShape(&port);

    // boundary
    ui->drawingItemTree->clearSelection();
    boundary.setSelected(true);
    clickedItem=&boundary;
    previousClickedItem=nullptr;
    //showPortShape();
    showPortShape(&boundary);

    // mesh
    ui->drawingItemTree->clearSelection();
    mesh.setSelected(true);
    clickedItem=&mesh;
    previousClickedItem=nullptr;
    //meshShowEntities();
    showMeshEntitiesItem(&mesh);

    ui->drawingItemTree->clearSelection();
    ui->drawingItemTree->setCurrentItem(nullptr);
    clickedItem=nullptr;
    previousClickedItem=nullptr;
}

// recursive
void OpenParEMg::unselectTreeItems (CustomTreeWidgetItem *item)
{
    if (item->foreground(0) == Qt::red) item->setForeground(0,Qt::black);

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        unselectTreeItems(child);
        i++;
    }
}

void OpenParEMg::on_actionUnselect_All_triggered ()
{
    ui->drawingWindow->unselectAll();
    unselectTreeItems(&drawing);
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::showMeshEntitiesItem (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::black);

    if (item->is_root()) {
        int i=0;
        while (i < mesh.childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)mesh.child(i);
            child->setForeground(0,Qt::black);
            std::vector<Handle(AIS_Shape)> *entities=child->get_meshEntities();
            long unsigned int j=0;
            while (j < entities->size()) {
                //ui->drawingWindow->displayShape((*entities)[j],child->get_displayMode(),selectionMode);
                ui->drawingWindow->displayShape((*entities)[j],child->get_displayMode(),-1);  // mesh is non-selectable
                j++;
            }
            i++;
        }
    } else {
        std::vector<Handle(AIS_Shape)> *entities=item->get_meshEntities();
        long unsigned int j=0;
        while (j < entities->size()) {
            //ui->drawingWindow->displayShape((*entities)[j],item->get_displayMode(),selectionMode);
            ui->drawingWindow->displayShape((*entities)[j],item->get_displayMode(),-1);  // mesh is non-selectable
            j++;
        }
    }

    if (menuAllHidden(&mesh)) mesh.setForeground(0,Qt::gray);
    else mesh.setForeground(0,Qt::black);

    ui->drawingWindow->updateViewer();
}

void OpenParEMg::hideMeshEntitiesItem (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::gray);

    if (item->is_root()) {
        int i=0;
        while (i < mesh.childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)mesh.child(i);
            child->setForeground(0,Qt::gray);
            std::vector<Handle(AIS_Shape)> *entities=child->get_meshEntities();
            long unsigned int j=0;
            while (j < entities->size()) {
                ui->drawingWindow->hideShape((*entities)[j]);
                j++;
            }
            i++;
        }
    } else {
        std::vector<Handle(AIS_Shape)> *entities=item->get_meshEntities();
        long unsigned int j=0;
        while (j < entities->size()) {
            ui->drawingWindow->hideShape((*entities)[j]);
            j++;
        }
    }

    if (menuAllHidden(&mesh)) mesh.setForeground(0,Qt::gray);
    else mesh.setForeground(0,Qt::black);

    ui->drawingWindow->updateViewer();
}

/*
void OpenParEMg::meshShowEntities ()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int k=0;
    while (k < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[k];

        if (item->is_root()) {
            int i=0;
            while (i < mesh.childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)mesh.child(i);
                child->setForeground(0,Qt::black);
                std::vector<Handle(AIS_Shape)> *entities=child->get_meshEntities();
                long unsigned int j=0;
                while (j < entities->size()) {
                    ui->drawingWindow->displayShape((*entities)[j]);
                    j++;
                }
                i++;
            }
        } else {
            std::vector<Handle(AIS_Shape)> *entities=item->get_meshEntities();
            long unsigned int j=0;
            while (j < entities->size()) {
                ui->drawingWindow->displayShape((*entities)[j]);
                j++;
            }
        }

        item->setForeground(0,Qt::black);
        k++;
    }

    if (menuAllHidden(&mesh)) mesh.setForeground(0,Qt::gray);
    else mesh.setForeground(0,Qt::black);

    ui->drawingWindow->updateViewer();
}
*/

/*
void OpenParEMg::meshHideEntities ()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int k=0;
    while (k < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[k];

        if (item->is_root()) {
            int i=0;
            while (i < mesh.childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)mesh.child(i);
                child->setForeground(0,Qt::gray);
                std::vector<Handle(AIS_Shape)> *entities=child->get_meshEntities();
                long unsigned int j=0;
                while (j < entities->size()) {
                    ui->drawingWindow->hideShape((*entities)[j]);
                    j++;
                }
                i++;
            }
        } else {
            std::vector<Handle(AIS_Shape)> *entities=item->get_meshEntities();
            long unsigned int j=0;
            while (j < entities->size()) {
                ui->drawingWindow->hideShape((*entities)[j]);
                j++;
            }
        }
        item->setForeground(0,Qt::gray);
        k++;
    }

    if (menuAllHidden(&mesh)) mesh.setForeground(0,Qt::gray);
    else mesh.setForeground(0,Qt::black);

    ui->drawingWindow->updateViewer();
}
*/

void OpenParEMg::drawMesh()
{
    // clear the mesh tree items
    mesh.deleteChildren(&mesh);

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
            verticesItem->set_rootType(3);
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
            std::cout << "   element type 15: vertices: n=" << count << std::endl; std::cout.flush();
        }

        // edges
        if (elementTypes[e] == 1) {
            CustomTreeWidgetItem *edgesItem=new CustomTreeWidgetItem(0);
            edgesItem->setText(0,"Edges");
            edgesItem->set_rootType(3);
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
            std::cout << "   element type 1: edges: n=" << count << std::endl; std::cout.flush();
        }

        // triangles
        if (elementTypes[e] == 2) {
            CustomTreeWidgetItem *wiresItem=new CustomTreeWidgetItem(0);
            wiresItem->setText(0,"Wires");
            wiresItem->set_rootType(3);
            wiresItem->setForeground(0,Qt::black);
            mesh.addChild(wiresItem);

            CustomTreeWidgetItem *trianglesItem=new CustomTreeWidgetItem(0);
            trianglesItem->setText(0,"Triangles");
            trianglesItem->set_rootType(3);
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
            std::cout << "   element type 2: triangles: n=" << count << std::endl; std::cout.flush();
        }

        // tetrahedron
        if (elementTypes[e] == 4) {
            CustomTreeWidgetItem *tetrahedronsItem=new CustomTreeWidgetItem(0);
            tetrahedronsItem->setText(0,"Tetrahedrons");
            tetrahedronsItem->set_rootType(3);
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
            std::cout << "   element type 4: tetrahedra: n=" << count << std::endl; std::cout.flush();
        }

        e++;
    }

    // display the shapes
    mesh.setSelected(true);
    showMeshEntitiesItem(&mesh);
    mesh.setSelected(false);
    ui->drawingWindow->fitAll();
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::deleteMesh ()
{
    int i=0;
    while (i < mesh.childCount()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *) mesh.child(i);
        std::vector<Handle(AIS_Shape)> *meshEntities=item->get_meshEntities();
        long unsigned int j=0;
        while (j < meshEntities->size()) {
            ui->drawingWindow->deleteShape((*meshEntities)[j]);
            j++;
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    mesh.deleteChildren(&mesh);
    drawingEntities.clear();
    gmsh::clear();
    ui->actionMeshSave->setEnabled(false);
    ui->actionDeleteMesh->setEnabled(false);

}

void OpenParEMg::on_actionGenerate_triggered ()
{
    deleteMesh();

    TopoDS_Shape shape=drawing.get_AIS_Shape()->Shape();
    gmsh::model::occ::importShapesNativePointer((void *) &shape,drawingEntities,false);
    gmsh::model::occ::synchronize();
    gmsh::model::mesh::generate();

    drawMesh();

    ui->actionMeshSave->setEnabled(true);
    ui->actionDeleteMesh->setEnabled(true);
}

void OpenParEMg::loadMeshFile (QString meshfile)
{
    if (QFile::exists(meshfile)) {

        // load and display
        deleteMesh();
        gmsh::open(meshfile.toStdString());
        drawMesh();

        // save the file name
        if (projData.mesh_file) free(projData.mesh_file);
        projData.mesh_file=(char *)malloc((meshfile.length()+1)*sizeof(char));
        int i=0;
        while (i < meshfile.length()) {
            projData.mesh_file[i]=meshfile.data()[i].toLatin1();
            i++;
        }
        projData.mesh_file[i]='\0';

        ui->actionMeshSave->setEnabled(true);
        ui->actionDeleteMesh->setEnabled(true);
    } else {

        // alert the user
        QMessageBox mb;
        QString message="Error in loading the specified mesh file \"";
        message.append(meshfile);
        message.append("\".");
        mb.critical(nullptr, "Error",message);
        mb.setFixedSize(500, 200);

        // null out the file name
        if (strlen(projData.mesh_file) > 0) {
            projData.mesh_file[0]='\0';
        } else {
            if (projData.mesh_file) free(projData.mesh_file);
            projData.mesh_file=(char*)malloc(sizeof(char));
            projData.mesh_file[0]='\0';
        }
    }
}

void OpenParEMg::on_actionMeshLoad_triggered ()
{
    //QString meshfile=get_meshfileName();
    QString meshfile=QFileDialog::getOpenFileName(this,tr("Open Mesh"), "", tr("Mesh Files (*.msh);;All Files (*)"),
                                                  nullptr,QFileDialog::DontUseNativeDialog);

    // return if user cancels
    if (meshfile.isNull()) return;

    loadMeshFile(meshfile);
}

void OpenParEMg::on_actionMeshSave_triggered ()
{
    QString testMeshFile=QFileDialog::getSaveFileName(this,tr("Save Mesh File"), "/home/briany/OpenParEM", tr("Data Files (*.msh);;All Files (*)"),
                                                      nullptr,QFileDialog::DontUseNativeDialog);
    if (testMeshFile.isNull()) return;
    gmsh::write(testMeshFile.toStdString());
}

void OpenParEMg::on_actionDeleteMesh_triggered ()
{
    deleteMesh();
}

void OpenParEMg::on_allWireframe_triggered ()
{
    if (ui->allWireframe->isChecked() == true) {
        set_displayMode(&drawing,0);
        set_displayMode(&port,0);
        set_displayMode(&boundary,0);
    } else {
        set_displayMode(&drawing,1);
        set_displayMode(&port,1);
        set_displayMode(&boundary,1);
    }

    reshowItem(&drawing);
    reshowItem(&port);
    reshowItem(&boundary);

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

    std::string lockfile=".";
    lockfile.append(projData.project_name);
    lockfile.append(".lock");
    std::cout << "projData.project_name=\"" << projData.project_name << "\"" << std::endl; std::cout.flush();
    std::cout << "check for lock file \"" << lockfile << "\"" << std::endl; std::cout.flush();

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

    int size=10;  // ToDo: pull this from an option panel

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

    int *error_codes=(int *)malloc(size*sizeof(int));

    // launch the job
    MPI_Errhandler errorHandler;
    MPI_Comm_create_errhandler(eh3D,&errorHandler);
    MPI_Comm_set_errhandler(PETSC_COMM_WORLD,errorHandler);

    if (MPI_PORT_COMM) delete MPI_PORT_COMM;
    MPI_PORT_COMM=new MPI_Comm();
    MPI_Comm_spawn ("OpenParEM3D",argv,size,MPI_INFO_NULL,0,PETSC_COMM_WORLD,MPI_PORT_COMM,error_codes);

    // check that all processes spawned
    bool fail=false;
    i=0;
    while (i < size) {
        if (error_codes[i] == MPI_ERR_SPAWN) {
            fail=true;
            break;
        }
        i++;
    }
    if (fail) {
        QMessageBox mb;
        mb.critical(nullptr, "Error","Failed to launch OpenParEM3D.");
    }

    // get the pids
    pidList.clear();
    i=0;
    while (i < size) {
        int pid;
        MPI_Recv(&pid,1,MPI_INT,i,10000,*MPI_PORT_COMM,MPI_STATUS_IGNORE);
        pidList.push_back(pid);
        i++;
    }

    timer->start(500);
    MPI_Irecv(&signal,1,MPI_INT,0,100000,*MPI_PORT_COMM,&request);
    //MPI_Comm_disconnect(MPI_PORT_COMM);

    ui->actionOptions->setEnabled(false);
    ui->actionRun->setEnabled(false);
    ui->actionStop->setEnabled(true);
    ui->actionAbort->setEnabled(true);

    // clean up

    MPI_Comm_set_errhandler(PETSC_COMM_WORLD,MPI_ERRORS_RETURN);
    MPI_Errhandler_free(&errorHandler);

    if (project) free(project);
    if (argv) free(argv);
    if (error_codes) free(error_codes);
}

void OpenParEMg::on_actionStop_triggered ()
{
    ui->actionStop->setEnabled(false);
    ui->actionAbort->setEnabled(true);
    MPI_Send(&signal,1,MPI_INT,0,300000,*MPI_PORT_COMM);
}

void OpenParEMg::checkFinish ()
{
    //std::cout << "OpenParEMg::checkFinish" << std::endl; std::cout.flush();
    int test;
    MPI_Test(&request,&test,MPI_STATUS_IGNORE);

    if (test) {
        std::cout << "   finished" << std::endl; std::cout.flush();
        timer->stop();
        pidList.clear();
        //MPI_Comm_free(MPI_PORT_COMM);  //MPI_Comm_disconnect also doesn't clear the resources
        delete MPI_PORT_COMM;
        MPI_PORT_COMM=nullptr;

        ui->actionOptions->setEnabled(true);
        ui->actionRun->setEnabled(true);
        ui->actionStop->setEnabled(false);
        ui->actionAbort->setEnabled(false);
    }
}

void OpenParEMg::on_actionAbort_triggered ()
{
    int i=0;
    while (i < pidList.size()) {
        //std::string command="kill -9 ";
        std::string command="kill ";
        command.append(std::to_string(pidList[i]));
        system(command.c_str());
        i++;
    }
    pidList.clear();

    ui->actionOptions->setEnabled(true);
    ui->actionRun->setEnabled(true);
    ui->actionStop->setEnabled(false);
    ui->actionAbort->setEnabled(false);
}

void OpenParEMg::on_actionShow_triggered()
{
    ui->drawingWindow->showGrid();
    ui->drawingWindow->updateViewer();
}


void OpenParEMg::on_actionHide_triggered()
{
    ui->drawingWindow->hideGrid();
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_actionSnap_To_Grid_triggered()
{
    if (ui->actionSnap_To_Grid->isChecked()) {
        ui->drawingWindow->set_snapToGrid(true);
    } else {
        ui->drawingWindow->set_snapToGrid(false);
    }
}

void OpenParEMg::on_actionSet_To_Face_triggered()
{
    ui->drawingWindow->set_gridPlane();
    ui->drawingWindow->updateViewer();
}



void OpenParEMg::on_actionSelect_with_Box_triggered()
{
    ui->drawingWindow->selectRectangle();
}

