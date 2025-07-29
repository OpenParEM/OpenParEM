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
#include "SelectMaterialsDatabase.h"
#include "CustomTreeWidgetItem.h"

OpenParEMg::OpenParEMg (QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OpenParEMg)
{
    ui->setupUi(this);

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

    ui->drawingWindow->show();

    ui->drawingItemTree->setHeaderHidden(true);

    drawing.setText(0,"Drawing");
    ui->drawingItemTree->addTopLevelItem(&drawing);
    port.setText(0,"Port");
    ui->drawingItemTree->addTopLevelItem(&port);
    boundary.setText(0,"Boundary");
    ui->drawingItemTree->addTopLevelItem(&boundary);


    ui->drawingItemTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->drawingItemTree,&QTreeView::customContextMenuRequested,this,&OpenParEMg::contextMenu_triggered);
    ui->drawingItemTree->show();
    ui->drawingItemTree->viewport()->installEventFilter(this);

    CTRLpressed=false;

    PetscInitializeNoArguments();
}

OpenParEMg::~OpenParEMg ()
{
    PetscFinalize();
    delete ui;
}


void OpenParEMg::contextMenu_triggered(const QPoint& pnt)
{
    clickedItem=(CustomTreeWidgetItem *)ui->drawingItemTree->itemAt(pnt);
    if (!clickedItem) return;
    if (!clickedItem->get_AIS_Shape()) return;

    QAction *showAction=new QAction("Show",this);
    QAction *hideAction=new QAction("Hide",this);
    QAction *selectAction=new QAction("Select",this);
    QAction *unselectAction=new QAction("Unselect",this);
    QAction *deleteAction=new QAction("Delete",this);

    connect(showAction, &QAction::triggered, this, &OpenParEMg::showShape);
    connect(hideAction, &QAction::triggered, this, &OpenParEMg::hideShape);
    connect(selectAction, &QAction::triggered, this, &OpenParEMg::selectShape);
    connect(unselectAction, &QAction::triggered, this, &OpenParEMg::unselectShape);

    if (ui->drawingWindow->isDisplayed(clickedItem->get_AIS_Shape())) {
        showAction->setEnabled(false);
    } else {
        hideAction->setEnabled(false);
        selectAction->setEnabled(false);
        unselectAction->setEnabled(false);
        deleteAction->setEnabled(false);
    }

    QMenu menu(this);
    menu.addAction(showAction);
    menu.addAction(hideAction);
    menu.addAction(selectAction);
    menu.addAction(unselectAction);
    menu.addAction(deleteAction);

    menu.exec(ui->drawingItemTree->mapToGlobal(pnt));
}

void OpenParEMg::showShape ()
{
    clickedItem->setForeground(0,Qt::black);
    ui->drawingWindow->showShape(clickedItem->get_AIS_Shape());
    ui->drawingWindow->repaint();
}

void OpenParEMg::hideShape ()
{
    clickedItem->setForeground(0,Qt::gray);
    ui->drawingWindow->unselectShape(clickedItem->get_AIS_Shape());
    ui->drawingWindow->hideShape(clickedItem->get_AIS_Shape());
    ui->drawingWindow->repaint();
}

void OpenParEMg::selectShape ()
{
    ui->drawingWindow->selectShape(clickedItem->get_AIS_Shape());
    ui->drawingWindow->repaint();
}

void OpenParEMg::unselectItemShape (CustomTreeWidgetItem *item)
{
    ui->drawingWindow->unselectShape(item->get_AIS_Shape());

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        unselectItemShape(child);
        i++;
    }
}

void OpenParEMg::unselectShape ()
{
    ui->drawingWindow->unselectShape(clickedItem->get_AIS_Shape());
    ui->drawingWindow->repaint();

    /*
    int i=0;
    while (i < clickedItem->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)clickedItem->child(i);
        ui->drawingWindow->unselectShape(child->get_AIS_Shape());
        i++;
    }
    ui->drawingWindow->updateView();
*/
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

        // load boundaries, if any, and draw
        if (boundaryDatabase->load(projData.port_definition_file,projData.solution_check_closed_loop)) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "Error in loading port and boundary definitions.");
            mb.setFixedSize(500, 200);
        } else {
            boundaryDatabase->draw(&projData,ui->drawingWindow,ui->drawingItemTree,&port,&boundary,materialDatabase);
        }

        ui->drawingWindow->fitAll();
        ui->drawingWindow->updateView();
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
    ui->drawingWindow->updateView();

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
        QString base=fileInfo.baseName();

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
    ui->drawingWindow->updateView();
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


void OpenParEMg::on_drawingItemTree_itemClicked(QTreeWidgetItem *item, int column)
{
    clickedItem=(CustomTreeWidgetItem *)item;
    if (!CTRLpressed) ui->drawingWindow->unselectAll();
    selectShape();
}

void OpenParEMg::on_actionFit_Selected_triggered ()
{
    ui->drawingWindow->fitSelected();
    ui->drawingWindow->updateView();
}

void OpenParEMg::on_actionFit_All_triggered ()
{
    ui->drawingWindow->fitAll();
    ui->drawingWindow->updateView();
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
            }
            return false;
        }
    }
    return QObject::eventFilter(obj, event);
}

void OpenParEMg::keyPressEvent(QKeyEvent *event)
{
    // CTRL key
    if (event->key() & Qt::Key_Control) {
        CTRLpressed=true;
        std::cout << "CTRLpressed=" << CTRLpressed << std::endl; std::cout.flush();
    }

    QWidget::keyPressEvent(event);
}

void OpenParEMg::keyReleaseEvent(QKeyEvent *event)
{
    // CTRL key
    if (event->key() & Qt::Key_Control) {
        CTRLpressed=false;
        std::cout << "CTRLpressed=" << CTRLpressed << std::endl; std::cout.flush();
    }

    QWidget::keyReleaseEvent(event);
}

void OpenParEMg::grayOutTreeItems (CustomTreeWidgetItem *item)
{
    item->setForeground(0,Qt::gray);

    int i=0;
    while (i < item->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
        grayOutTreeItems(child);
        i++;
    }
}

void OpenParEMg::on_actionHide_All_triggered ()
{
    ui->drawingWindow->hideAll();
    ui->drawingWindow->repaint();

    grayOutTreeItems(&drawing);
    grayOutTreeItems(&port);
    grayOutTreeItems(&boundary);
}

void OpenParEMg::on_actionShow_All_triggered ()
{
    //ui->drawingWindow->showAll();
    ui->drawingWindow->hideAll();
    clickedItem=&drawing;
    showShape();
}

