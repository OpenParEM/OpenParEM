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
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>

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
#include "SelectMaterialsDatabase.h"
#include "CustomTreeWidgetItem.h"

OpenParEMg::OpenParEMg (QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OpenParEMg)
{
    ui->setupUi(this);

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

    drawing.set_type(0);
    port.set_type(1);
    boundary.set_type(2);
    mesh.set_type(3);

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

    /////////////////////////////////////////////////////////////////////////////
    // context menu for drawingWindow
    /////////////////////////////////////////////////////////////////////////////

    QAction *hideAction=new QAction("Hide",this);
    QAction *selectAction=new QAction("Select",this);
    QAction *unselectAction=new QAction("Unselect",this);

    connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideItems);
    connect(selectAction, &QAction::triggered, this, &OpenParEMg::selectItems);
    connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectItems);

    drawingContextMenu=new QMenu(this);
    drawingContextMenu->addAction(hideAction);
    drawingContextMenu->addAction(selectAction);
    drawingContextMenu->addAction(unselectAction);
    ui->drawingWindow->set_contextMenu(drawingContextMenu);

    /////////////////////////////////////////////////////////////////////////////
    // gmsh
    /////////////////////////////////////////////////////////////////////////////

    gmsh::initialize();

    /////////////////////////////////////////////////////////////////////////////


    ui->drawingItemTree->show();
    ui->drawingWindow->show();

    PetscInitializeNoArguments();
}

OpenParEMg::~OpenParEMg ()
{
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
    //QAction *deleteAction=new QAction("Delete",this);

    connect(showAction, &QAction::triggered, this, &OpenParEMg::showItems);
    connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideItems);
    connect(selectAction, &QAction::triggered, this, &OpenParEMg::selectItems);
    connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectItems);

    if (clickedItem->foreground(0) == Qt::black) {  // visible
        showAction->setEnabled(false);
        hideAction->setEnabled(true);
        selectAction->setEnabled(true);
        unselectAction->setEnabled(false);
        //deleteAction->setEnabled(false);
    } else if (clickedItem->foreground(0) == Qt::gray) {  // invisible
        showAction->setEnabled(true);
        hideAction->setEnabled(false);
        selectAction->setEnabled(false);
        unselectAction->setEnabled(false);
        //deleteAction->setEnabled(false);
    } else if (clickedItem->foreground(0) == Qt::red) {  // selected
        showAction->setEnabled(false);
        hideAction->setEnabled(true);
        selectAction->setEnabled(false);
        unselectAction->setEnabled(true);
        //deleteAction->setEnabled(false);
    }

    // special cases
    if (clickedItem->is_port() && clickedItem->is_root()) selectAction->setEnabled(false);
    if (clickedItem->is_boundary() && clickedItem->is_root()) selectAction->setEnabled(false);
    if (clickedItem->is_mesh()) selectAction->setEnabled(false);

    menu.addAction(showAction);
    menu.addAction(hideAction);
    menu.addAction(selectAction);
    menu.addAction(unselectAction);
    //menu.addAction(deleteAction);


    menu.exec(ui->drawingItemTree->mapToGlobal(pnt));
}

void OpenParEMg::showItems ()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];

        if (item->is_drawing()) {
            unselectDisplayShape(item);
            hideDisplayShape(item);
            showDisplayShape(item);
        }

        if (item->is_port()) showPortShape(item);
        if (item->is_boundary()) showPortShape(item);
        if (item->is_mesh()) showMeshEntitiesItem(item);
        i++;
    }
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::hideItems ()
{
    QList<QTreeWidgetItem*> selectedItems=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < selectedItems.count()) {
        CustomTreeWidgetItem *item=(CustomTreeWidgetItem *)selectedItems[i];
        if (item->is_drawing()) hideDisplayShape(item);
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
        if (item->is_drawing()) {
            selectDisplayShape(item);
            item->setForeground(0,Qt::red);
        }

        if (item->is_port() && !item->is_root()) {
            selectDisplayShape(item);
            item->setForeground(0,Qt::red);
        }

        if (item->is_boundary() && !item->is_root()) {
            selectDisplayShape(item);
            item->setForeground(0,Qt::red);
        }

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
        if (item->is_drawing()) {
            unselectDisplayShape(item);
            item->setForeground(0,Qt::black);
        }

        if (item->is_port()) {
            unselectDisplayShape(item);
            item->setForeground(0,Qt::black);
        }

        if (item->is_boundary()) {
            unselectDisplayShape(item);
            item->setForeground(0,Qt::black);
        }
        i++;
    }
    ui->drawingWindow->updateViewer();
}

// non-recursive
void OpenParEMg::showDisplayShape (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::black);
    ui->drawingWindow->showShape(item->get_AIS_Shape());

    if (menuAllHidden(&drawing)) drawing.setForeground(0,Qt::gray);
    else drawing.setForeground(0,Qt::black);

    if (item->is_root()) item->setForeground(0,Qt::black);
}

// recurseive
void OpenParEMg::hideDisplayShape (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::gray);
    ui->drawingWindow->unselectShape(item->get_AIS_Shape());
    ui->drawingWindow->hideShape(item->get_AIS_Shape());

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        hideDisplayShape(child);
        i++;
    }

    if (menuAllHidden(&drawing)) drawing.setForeground(0,Qt::gray);
    else drawing.setForeground(0,Qt::black);
}

// non-recursive
void OpenParEMg::selectDisplayShape (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::red);
    unselectDisplayShape(item);
    ui->drawingWindow->showShape(item->get_AIS_Shape());
    ui->drawingWindow->selectShape(item->get_AIS_Shape());
}

// recursive
void OpenParEMg::unselectDisplayShape (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::gray);
    ui->drawingWindow->unselectShape(item->get_AIS_Shape());
    ui->drawingWindow->hideShape(item->get_AIS_Shape());

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        unselectDisplayShape(child);
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
            ui->drawingWindow->showShape(child->get_AIS_Shape());
            child->setForeground(0,Qt::black);
            j++;
        }
    } else {
        ui->drawingWindow->showShape(item->get_AIS_Shape());
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

    //ui->drawingWindow->updateViewer();
}

void OpenParEMg::on_fileOpen_triggered ()
{
    QString testProjectFile=QFileDialog::getOpenFileName(this,tr("Open Project"), "", tr("Project Files (*.proj);;All Files (*)"));

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

        if (load_project_file (projectFile.toLatin1().toStdString().c_str(),&projData,"   ")) {
            projectFile="";

            QMessageBox mb;
            mb.critical(nullptr, "Error", "Unable to load the requested project file.");
            mb.setFixedSize(500, 200);

            return;
        }

        // set project path
        QDir::setCurrent(absolutePath);
        projectFile=projectName;

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
            }
        }

        // extract drawing entities for gmsh
        TopoDS_Shape shape=drawing.get_AIS_Shape()->Shape();
        gmsh::model::occ::importShapesNativePointer((void *) &shape,drawingEntities,false);
        gmsh::model::occ::synchronize();
        gmsh::model::mesh::generate();
        gmsh::write("testfile.msh");
        drawMesh();


        // load boundaries, if any, and draw
        if (boundaryDatabase->load(projData.port_definition_file,projData.solution_check_closed_loop)) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "Error in loading port and boundary definitions.");
            mb.setFixedSize(500, 200);
        } else {
            boundaryDatabase->set_drawingToItemMap(&drawingToItemMap);
            boundaryDatabase->draw(&projData,ui->drawingWindow,ui->drawingItemTree,&port,&boundary,materialDatabase);
        }

        ui->drawingWindow->fitAll();
        ui->drawingWindow->updateViewer();
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

void OpenParEMg::addShape (TopoDS_Shape shape, CustomTreeWidgetItem *item, bool isRoot)
{
    if (shape.IsNull()) return;

    // tree item name

    QString name="";

    TopAbs_ShapeEnum shapeType=shape.ShapeType();
    switch (shapeType) {
    case TopAbs_COMPOUND:
        name="COMPOUND";
        break;
    case TopAbs_COMPSOLID:
        name="COMPSOLID";
        break;
    case TopAbs_SOLID:
        name="SOLID";
        break;
    case TopAbs_SHELL:
        name="SHELL";
        break;
    case TopAbs_FACE:
        name="FACE";
        break;
    case TopAbs_WIRE:
        name="WIRE";
        break;
    case TopAbs_EDGE:
        name="EDGE";
        break;
    case TopAbs_VERTEX:
        name="VERTEX";
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
        ui->drawingWindow->showShape(drawingShape);
        newItem=&drawing;
    } else {
        newItem=new CustomTreeWidgetItem(0);
        newItem->set_AIS_Shape(drawingShape);
        newItem->setText(0,name);
        newItem->setForeground(0,Qt::gray);
        item->addChild(newItem);
    }
    drawingToItemMap.insert({drawingShape,newItem});

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

        ui->drawingWindow->clearDrawing();
        addShape(s,nullptr,true);
    }
    return false;
}

void OpenParEMg::on_actionBrep_triggered ()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open BREP File"), "", tr("BREP Files (*.brep)"));
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
    ui->drawingWindow->setSelectionShape();
}

void OpenParEMg::on_actionVertex_triggered()
{
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionVertex;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    ui->drawingWindow->setSelectionShape();
    ui->drawingWindow->setSelectionVertex();
}

void OpenParEMg::on_actionEdge_triggered()
{
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionEdge;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    ui->drawingWindow->setSelectionShape();
    ui->drawingWindow->setSelectionEdge();
}

void OpenParEMg::on_actionWire_triggered()
{
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionWire;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    ui->drawingWindow->setSelectionShape();
    ui->drawingWindow->setSelectionWire();
}

void OpenParEMg::on_actionFace_triggered()
{
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionFace;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    ui->drawingWindow->setSelectionShape();
    ui->drawingWindow->setSelectionFace();
}

void OpenParEMg::on_actionShell_triggered()
{
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionShell;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    ui->drawingWindow->setSelectionShape();
    ui->drawingWindow->setSelectionShell();
}

void OpenParEMg::on_actionSolid_triggered()
{
    currentSelectionAction->setCheckable(false);
    currentSelectionAction=ui->actionSolid;
    currentSelectionAction->setCheckable(true);
    currentSelectionAction->setChecked(true);
    ui->drawingWindow->setSelectionShape();
    ui->drawingWindow->setSelectionSolid();
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
    QWidget::keyPressEvent(event);
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
            std::cout << "   element type 4: tetrahedra: n=" << count << std::endl; std::cout.flush();
        }

        e++;
    }

    // display the shapes
    mesh.setSelected(true);
    //meshShowEntities();
    showMeshEntitiesItem(&mesh);
    mesh.setSelected(false);
}

