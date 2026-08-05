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
#include "AntennaForm.h"
#include "CustomComboBox.h"
#include "DrawingPreferences.h"
#include "Process.h"
#include "ui_OPEMg.h"

#include <GC_MakeSegment.hxx>
#include <Geom_Plane.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <csignal>
//#include <quadmath.h>
#include <iostream>
#include <filesystem>
#include <thread>

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
#include <BOPAlgo_Builder.hxx>
#include <BOPAlgo_CellsBuilder.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <ShapeFix_Shape.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <BRepAlgoAPI_Common.hxx>

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
#include <QScrollArea>
#include <QTextBlock>
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QThread>

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
#include "CustomTreeWidgetItem.h"
#include "MaterialSelection.h"
#include "../OpenParEM3D/fileCleanup.hpp"
#include "mpi.h"


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

// top of file

OpenParEMg::OpenParEMg (QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OpenParEMg)
{
    // set depth and stencil buffer sizes
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    QSurfaceFormat::setDefaultFormat(format);

    // start
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
    connect(relay,&Relay::updateViewer,this,&OpenParEMg::updateViewer);
    connect(relay,&Relay::convertPathToFace,this,&OpenParEMg::convertPathToFace);
    connect(relay,&Relay::setShaded,this,&OpenParEMg::setShaded);
    connect(relay,&Relay::clearTreeSelection,this,&OpenParEMg::clearTreeSelection);
    connect(relay,&Relay::startPlaneSetToFace,this,&OpenParEMg::startPlaneSetToFace);

    /////////////////////////////////////////////////////////////////////////////
    // drawing window
    /////////////////////////////////////////////////////////////////////////////

    ui->drawingWindow->set_drawingItemTree(drawing);
    ui->drawingWindow->set_portItemTree(port);
    ui->drawingWindow->set_boundaryItemTree(boundary);
    ui->drawingWindow->set_meshItemTree(mesh);
    ui->drawingWindow->set_pathItemTree(path);
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
    QActionList.push_back(convertToPortAction);
    QActionList.push_back(convertToBoundaryAction);
    QActionList.push_back(removeAction);
    QActionList.push_back(assignAction);
    QActionList.push_back(insertAction);
    QActionList.push_back(renameAction);
    QActionList.push_back(expandAllAction);
    QActionList.push_back(collapseAllAction);
    QActionList.push_back(setPlaneAction);
    QActionList.push_back(setPlaneAxisAction);
    QActionList.push_back(createPathAction);
    QActionList.push_back(createPortAction);
    QActionList.push_back(createBoundaryAction);
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
    QActionList.push_back(createDiffpairAction);
    initQActionList();

    // signals to show the drawing tab when drawing actions are clicked
    connect(ui->menuView, &QMenu::aboutToShow, this, &OpenParEMg::onMenuAboutToShow);
    connect(ui->menuSelect, &QMenu::aboutToShow, this, &OpenParEMg::onMenuAboutToShow);
    connect(ui->menuDraw, &QMenu::aboutToShow, this, &OpenParEMg::onMenuAboutToShow);
    connect(ui->menuMesh, &QMenu::aboutToShow, this, &OpenParEMg::onMenuAboutToShow);

    /////////////////////////////////////////////////////////////////////////////
    // item selection tree
    /////////////////////////////////////////////////////////////////////////////

    drawing=new RootDrawingItem(this);
    path=new RootPathItem(this);
    port=new RootPortItem(this);
    boundary=new RootBoundaryItem(this);
    mesh=new RootMeshItem(this);

    resetLockouts();

    ui->drawingItemTree->setHeaderHidden(true);
    ui->drawingItemTree->setColumnCount(2);
    ui->drawingItemTree->header()->setStretchLastSection(false);
    ui->drawingItemTree->header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    ui->drawingItemTree->header()->setSectionResizeMode(1,QHeaderView::ResizeToContents);

    // five base list items: drawing, path, port, boundary, and mesh

    drawing->setText(0,"Drawing");
    ui->drawingItemTree->addTopLevelItem(drawing);

    path->setText(0,"Path");
    ui->drawingItemTree->addTopLevelItem(path);

    port->setText(0,"Port");
    ui->drawingItemTree->addTopLevelItem(port);

    boundary->setText(0,"Boundary");
    ui->drawingItemTree->addTopLevelItem(boundary);

    mesh->setText(0,"Initial Mesh");
    ui->drawingItemTree->addTopLevelItem(mesh);

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

    // drawing is always a COMPOUND
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    Handle(AIS_Shape) newShape=new AIS_Shape(compound);
    ShapeData *newShapeData=drawing->getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->setShape(newShape);
    drawing->addShapeData(newShapeData);

    ui->drawingItemTree->setCurrentItem(nullptr);

    /////////////////////////////////////////////////////////////////////////////
    // logging tabs
    /////////////////////////////////////////////////////////////////////////////

    ui->tabs->setTabToolTip(0,"Drawing space for creating 3D structures for simulation.");
    ui->tabs->setTabToolTip(1,"Output from OpenParEM3D.");
    ui->tabs->setTabToolTip(2,"Summary statistics on the simulation run.");
    ui->tabs->setTabToolTip(3,"S-parameter results summary.  Check the project directory for a Touchstone(TM) file, if generated.");
    ui->tabs->setTabToolTip(4,"Antenna parameter results summary.  Check the project directory for reports, if generated.");

    ui->logText->setReadOnly(true);
    ui->iterationsText->setReadOnly(true);
    ui->dataText->setReadOnly(true);
    ui->antennaText->setReadOnly(true);

    QFont monoFont=QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPointSize(10);
    ui->logText->setFont(monoFont);
    ui->iterationsText->setFont(monoFont);
    ui->dataText->setFont(monoFont);
    ui->antennaText->setFont(monoFont);

    logFilter=new LogViewerFilter(this);
    logFilter->setTextEdit(ui->logText);
    ui->logText->viewport()->installEventFilter(logFilter);
    ui->logText->verticalScrollBar()->installEventFilter(logFilter);

    iterationsFilter=new LogViewerFilter(this);
    iterationsFilter->setTextEdit(ui->iterationsText);
    ui->iterationsText->viewport()->installEventFilter(iterationsFilter);
    ui->iterationsText->verticalScrollBar()->installEventFilter(iterationsFilter);

    dataFilter=new LogViewerFilter(this);
    dataFilter->setTextEdit(ui->dataText);
    ui->dataText->viewport()->installEventFilter(dataFilter);
    ui->dataText->verticalScrollBar()->installEventFilter(dataFilter);

    antennaFilter=new LogViewerFilter(this);
    antennaFilter->setTextEdit(ui->antennaText);
    ui->antennaText->viewport()->installEventFilter(antennaFilter);
    ui->antennaText->verticalScrollBar()->installEventFilter(antennaFilter);

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
    // timer or checking when OpenParEM3D finishes and updating tabs with run data
    /////////////////////////////////////////////////////////////////////////////

    bool defaultForce=false;
    timer=new QTimer(this);
    connect(timer,&QTimer::timeout,this,&OpenParEMg::checkFinish);
    connect(timer,&QTimer::timeout,this,[this, defaultForce]() {updateLogTab(defaultForce);});
    connect(timer,&QTimer::timeout,this,[this, defaultForce]() {updateIterationsTab(defaultForce);});
    connect(timer,&QTimer::timeout,this,[this, defaultForce]() {updateDataTab(defaultForce);});
    connect(timer,&QTimer::timeout,this,[this, defaultForce]() {updateAntennaTab(defaultForce);});

    /////////////////////////////////////////////////////////////////////////////
    // misc
    /////////////////////////////////////////////////////////////////////////////

    renameItem=nullptr;
    isDisabledSelection=false;

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

    // form defaults
    uLocalAxis.SetCoord(1,0,0);  // rectangles
    length=1;                    // extrusion
    angle=90;                    // rotation
    startPoint.SetCoord(0,0,0);  // rotation and vector input
    endPoint.SetCoord(0,0,1);    // rotation and vector input

    restrictToDrawingPlane=false;
    activeAction=false;

    lineEdit=nullptr;
    rectangleEdit=nullptr;
    polycircleEdit=nullptr;

    currentDrawingItem=nullptr;
    renameEdit=nullptr;
    renameItem=nullptr;

    /////////////////////////////////////////////////////////////////////////////
    // change flags
    /////////////////////////////////////////////////////////////////////////////

    projectChanged=false;
    meshObsolete=false;

    /////////////////////////////////////////////////////////////////////////////

    ui->drawingItemTree->show();
    ui->drawingWindow->show();

    // set the starting window size

    int windowWidth=1510;
    int windowHeight=900;

    QScreen *screen=QGuiApplication::primaryScreen();
    QSize maxAvailableSize=screen->availableSize();
    int maxWidth=maxAvailableSize.width();
    int maxHeight=maxAvailableSize.height();

    if (windowWidth > maxWidth) windowWidth=maxWidth;
    if (windowHeight > maxHeight) windowHeight=maxHeight;

    int locationX=(maxWidth-windowWidth)/2;
    int locationY=(maxHeight-windowHeight)/2;

    this->setGeometry(locationX,locationY,windowWidth,windowHeight);

    setMenus();

    close_event=nullptr;

    PetscInitializeNoArguments();
}

OpenParEMg::~OpenParEMg ()
{
    //std::cout << "OpenParEMg::~OpenParEMg" << std::endl; std::cout.flush();

    ui->drawingWindow->shutdown();

    freeQActionList();
    free_project(&defaultData);
    free_project(&projData);
    if (materialDatabase) {delete materialDatabase; materialDatabase=nullptr;}
    if (relay) {delete relay; relay=nullptr;}
    if (timer) delete timer;
    if (MPI_PORT_COMM) MPI_Comm_free(MPI_PORT_COMM);
    if (request) MPI_Request_free(request);
    gmsh::finalize();
    PetscFinalize();

    delete ui;
}

void OpenParEMg::updateViewer ()
{
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::setShaded (Handle(AIS_Shape) shape)
{
    if (shape.IsNull()) return;
    ui->drawingWindow->setShaded(shape);
}

void OpenParEMg::convertPathToFace (BaseItem *baseItem)
{
    if (!baseItem) return;
    Handle(AIS_Shape) shape=baseItem->getShape();
    if (shape.IsNull()) return;

    ui->drawingWindow->hideItem(baseItem);
    ui->drawingWindow->removeItemFromMap(baseItem);
    ui->drawingWindow->deleteShape(shape);

    PathItem *convertPathItem=nullptr;

    PathItem *pathItem=dynamic_cast<PathItem *>(baseItem);
    if (pathItem && pathItem->is_path()) convertPathItem=pathItem;

    BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(baseItem);
    if (boundaryItem && boundaryItem->is_boundary()) convertPathItem=boundaryItem->getPathItem();

    PortItem *portItem=dynamic_cast<PortItem *>(baseItem);
    if (portItem && portItem->is_port()) convertPathItem=portItem->getPathItem();

    if (!convertPathItem) return;

    Path *path=static_cast<Path *>(convertPathItem->getPath());
    if (path) {
        TopoDS_Wire wire=path->create_TopoDS_Wire();
        if (!wire.IsNull()) {
            BRepBuilderAPI_MakeFace faceMaker(wire);
            if (faceMaker.IsDone()) {
                TopoDS_Face face=faceMaker.Face();
                Handle(AIS_Shape) newShape=new AIS_Shape(face);
                ShapeData *shapeData=convertPathItem->getShapeData();
                shapeData->setShape(newShape);

                ui->drawingWindow->insertItemToMap(newShape,baseItem);
                ui->drawingWindow->showItem(baseItem);
            }
        }
    }
}

bool OpenParEMg::isModified ()
{
    // std::cout << "OpenParEMg::isModified" << std::endl; std::cout.flush();
    // std::cout << "   projectChanged=" << projectChanged << std::endl; std::cout.flush();
    // std::cout << "   drawing->isModified()=" << drawing->isModified() << std::endl; std::cout.flush();
    // std::cout << "   path->isModified()=" << path->isModified() << std::endl; std::cout.flush();
    // std::cout << "   port->isModified()=" << port->isModified() << std::endl; std::cout.flush();
    // std::cout << "   boundary->isModified()=" << boundary->isModified() << std::endl; std::cout.flush();
    // std::cout << "   mesh->isModified()=" << mesh->isModified() << std::endl; std::cout.flush();
    // std::cout << "   meshObsolete=" << meshObsolete << std::endl; std::cout.flush();

    if (projectChanged) return true;
    if (drawing->isModified()) return true;
    if (path->isModified()) return true;
    if (port->isModified()) return true;
    if (boundary->isModified()) return true;
    if (mesh->isModified()) return true;
    if (meshObsolete) return true;
    return false;
}

void OpenParEMg::setUnmodified ()
{
    projectChanged=false;
    drawing->setModified(false);
    path->setModified(false);
    port->setModified(false);
    boundary->setModified(false);
    mesh->setModified(false);
}

bool OpenParEMg::hasResults ()
{
    std::cout << "OpenParEMg::hasResults" << std::endl; std::cout.flush();
    std::cout << "   baseName=" << projData.project_name << std::endl; std::cout.flush();
    std::cout << "   SportCount=" << port->get_SportCount() << std::endl; std::cout.flush();

    if (has_results_files(projData.project_name,port->get_SportCount())) return true;
    return false;
}

int OpenParEMg::check_changed ()
{
    int retVal=0;
    if (isModified()) {
        QMessageBox msgBox(this);
        msgBox.setText("The project has been modified.");
        if (hasResults()) {
            msgBox.setInformativeText("Do you want to save your changes? \n\nWarning: Previous computed results will be permanently deleted.");
        } else {
            msgBox.setInformativeText("Do you want to save your changes?");
        }
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        retVal=msgBox.exec();
    }

    return retVal;
}

void OpenParEMg::closeWindow_triggered ()
{
    //std::cout << "OpenParEMg::closeWindow_triggered" << std::endl; std::cout.flush();

    if (lengthInputForm) {delete lengthInputForm; lengthInputForm=nullptr;}
    if (vectorInputForm) {delete vectorInputForm; vectorInputForm=nullptr;}
    if (lengthEditForm) {delete lengthEditForm; lengthEditForm=nullptr;}
    if (lineEditForm) {delete lineEditForm; lineEditForm=nullptr;}
    if (rectangleEditForm) {delete rectangleEditForm; rectangleEditForm=nullptr;}
    if (polycircleEditForm) {delete polycircleEditForm; polycircleEditForm=nullptr;}
    if (rotateInputForm) {delete rotateInputForm; rotateInputForm=nullptr;}
    finishOperation(true,6000);

    int retVal=check_changed();
    if (retVal) {
        if (retVal == QMessageBox::Save) {
            on_actionSave_triggered();
        } else if (retVal == QMessageBox::Discard) {
            // do nothing
        } else if (retVal == QMessageBox::Cancel) {
            if (close_event) close_event->ignore();
            return;
        }
    }

    if (close_event) close_event->accept();
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

// void OpenParEMg::restoreSelection ()
// {
//     //std::cout << "OpenParEMg::restoreSelection  previousSelectionIndex=" << previousSelectionIndex << std::endl; std::cout.flush();

//     if (previousSelectionIndex == 0) on_actionShape_triggered();
//     else if (previousSelectionIndex == 1) on_actionVertex_triggered();
//     else if (previousSelectionIndex == 2) on_actionEdge_triggered();
//     else if (previousSelectionIndex == 3) on_actionWire_triggered();
//     else if (previousSelectionIndex == 4) on_actionFace_triggered();
//     else if (previousSelectionIndex == 5) on_actionShell_triggered();
//     else if (previousSelectionIndex == 6) on_actionSolid_triggered();
// }

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

    ui->drawingWindow->compactSelectedItems();
    //ui->drawingWindow->compactVisibleItems();

    // debug options
    //itemChangesStack.print();
    //printLockouts();
    //debugPrintStats(0);
    //ui->drawingWindow->PrintAllActiveModes();
    //ui->drawingWindow->audit();
    //sportNumbers.print();


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
        if (isModified()) {
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
        ui->actionSelectFace->setEnabled(false);
        ui->actionSelectWithBox2->setEnabled(false);
        ui->actionDrawLine->setEnabled(true);
        ui->actionDrawPolyline->setEnabled(true);
        ui->actionDrawPolycircle->setEnabled(true);
        ui->actionDrawRectangle->setEnabled(true);
        ui->actionPreferences->setEnabled(true);
        ui->actionMeshOptions->setEnabled(true);
        ui->actionMeshLoad->setEnabled(true);
        ui->actionMeshSave->setEnabled(false);
        ui->actionMeshSaveAs->setEnabled(false);
        ui->actionMeshDelete->setEnabled(false);
        ui->actionMaterials->setEnabled(true);
        ui->actionFrequencyPlan->setEnabled(true);
        ui->actionAntennaPatterns->setEnabled(hasRadiationBoundary());
        //ui->actionRefinement->setEnabled(true);
        if (strcmp(projData.refinement_frequency,"none") == 0) ui->actionRefinement->setEnabled(false);
        else ui->actionRefinement->setEnabled(true);
        ui->actionSimulateOptions->setEnabled(true);
        ui->actionRun->setEnabled(false);
        ui->actionStop->setEnabled(false);
        ui->actionAbort->setEnabled(false);
        ui->actionAbortAndExit->setEnabled(false);
        ui->actionMaterialsEditor->setEnabled(true);
        ui->actionAbout->setEnabled(true);
        ui->actionLicense->setEnabled(true);
        ui->actionImportBrep->setEnabled(true);
        ui->actionImportStep->setEnabled(true);
        ui->actionExportBrep->setEnabled(isValidSaveBrepFile());
        ui->actionExportStep->setEnabled(isValidSaveStepFile());
        ui->actionFitSelected->setEnabled(false);
        ui->actionFitAll->setEnabled(false);
        ui->actionMenuSelection->setEnabled(false);
        ui->actionSelectFace->setEnabled(false);
        ui->actionSelectWithBox2->setEnabled(false);
        ui->actionUnselectAll->setEnabled(false);
        ui->actionHideAll->setEnabled(false);
        ui->actionWireframe->setEnabled(false);
        ui->actionDrawingPlaneSetToFace->setEnabled(false);
        ui->actionDrawingPlaneSetToFaceAxis->setEnabled(false);
        ui->actionMeshGenerate->setEnabled(false);
        if (drawing->childCount() > 0) {
            ui->actionExportBrep->setEnabled(isValidSaveBrepFile());
            ui->actionExportStep->setEnabled(isValidSaveStepFile());
            ui->actionFitSelected->setEnabled(true);
            ui->actionFitAll->setEnabled(true);
            ui->actionMenuSelection->setEnabled(true);
            ui->actionSelectFace->setEnabled(true);
            ui->actionSelectWithBox2->setEnabled(true);
            ui->actionUnselectAll->setEnabled(true);
            ui->actionHideAll->setEnabled(true);
            ui->actionWireframe->setEnabled(true);
            ui->actionDrawingPlaneSetToFace->setEnabled(true);
            ui->actionDrawingPlaneSetToFaceAxis->setEnabled(true);
            ui->actionMeshGenerate->setEnabled(true);
        }

        if (path->childCount() > 0) {
            ui->actionFitSelected->setEnabled(true);
            ui->actionFitAll->setEnabled(true);
            ui->actionSelectWithBox2->setEnabled(true);
            ui->actionUnselectAll->setEnabled(true);
            ui->actionHideAll->setEnabled(true);
        }

        bool okToSimulate=true;
        QString issues="Setup issues preventing simulation:";

        if (!validDrawing()) {
            issues.append("\n- Drawing is not valid for simulation.");
            okToSimulate=false;
        }

        if (drawing->isModified()) {
            issues.append("\n- Drawing is modified.");
            okToSimulate=false;
        }

        if (path->isModified()) {
            issues.append("\n- Paths are modified.");
            okToSimulate=false;
        }

        if (port->childCount() == 0) {
            issues.append("\n- At least one port must be defined.");
            okToSimulate=false;
        }

        if (!validPorts()) {
            issues.append("\n- At least one port does not have a valid impedance calculation.");
            okToSimulate=false;
        }

        if (!validMultimodeLinePorts()) {
            issues.append("\n- At least one port does not have both voltage and current specified.");
            okToSimulate=false;
        }

        if (port->isModified()) {
            issues.append("\n- Ports are modified.");
            okToSimulate=false;
        }

        if (!sportNumbers.isStartWith1()) {
            issues.append("- Sport numbering must start with 1.");
            okToSimulate=false;
        }

        if (!sportNumbers.isContiguous()) {
            issues.append("\n- Sport numbering must be contiguous.");
            okToSimulate=false;
        }

        if (sportNumbers.hasDuplicates()) {
            issues.append("\n- Sport numbering has duplicate assignments.");
            okToSimulate=false;
        }

        if (!materialsAssigned()) {
            issues.append("\n- At least one drawing 3D element does not have a material assigned.");
            okToSimulate=false;
        }

        if (projData.inputFrequencyPlansCount == 0) {
            issues.append("\n- At least one frequency must be assigned for simulation.");
            okToSimulate=false;
        }

        if (mesh->childCount() == 0) {
            issues.append("\n- Mesh generation is required.");
            okToSimulate=false;
        }

        if (mesh->childCount() > 0) {

            ui->actionFitAll->setEnabled(true);
            ui->actionMenuSelection->setEnabled(true);
            ui->actionWireframe->setEnabled(true);
            ui->actionMeshGenerate->setEnabled(false);
            ui->actionMeshLoad->setEnabled(false);
            ui->actionMeshSave->setEnabled(false);
            if (mesh->isModified()) ui->actionMeshSave->setEnabled(true);
            ui->actionMeshSaveAs->setEnabled(true);
            ui->actionMeshDelete->setEnabled(true);

            if (okToSimulate) {
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
                if (mesh->isModified() || meshObsolete) {
                    ui->actionRun->setEnabled(false);
                    ui->actionRun->setToolTip("Mesh regeneration is required.");
                    ui->actionStop->setEnabled(false);
                    ui->actionAbort->setEnabled(false);
                    ui->actionAbortAndExit->setEnabled(false);
                }
            }

            if (!meshAssigned()) {
                issues.append("\n- Mesh has at least one region missing a material assignment.");
                okToSimulate=false;
            }
        }

        if (projectChanged) {
            ui->actionRun->setEnabled(false);
            issues.append("\n- Project save is required.");
            ui->actionStop->setEnabled(false);
            ui->actionAbort->setEnabled(false);
            ui->actionAbortAndExit->setEnabled(false);
            okToSimulate=false;
        }

        // show the cummulative setup issues in the tool tip
        if (!okToSimulate) ui->actionRun->setToolTip(issues);

        if (drawingPlaneShown) {
            ui->actionDrawingPlaneShow->setEnabled(false);
            ui->actionDrawingPlaneHide->setEnabled(true);
            ui->actionDrawingPlaneSnapToGrid->setEnabled(true);
            ui->actionDrawingSetPlaneToXY->setEnabled(true);
            ui->actionDrawingSetPlaneToXZ->setEnabled(true);
            ui->actionDrawingSetPlaneToYZ->setEnabled(true);
        } else {
            ui->actionDrawingPlaneShow->setEnabled(true);
            ui->actionDrawingPlaneHide->setEnabled(false);
            ui->actionDrawingPlaneSnapToGrid->setEnabled(false);
            ui->actionDrawingSetPlaneToXY->setEnabled(false);
            ui->actionDrawingSetPlaneToXZ->setEnabled(false);
            ui->actionDrawingSetPlaneToYZ->setEnabled(false);
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
        ui->actionExportBrep->setEnabled(false);
        ui->actionExportStep->setEnabled(false);

        ui->actionFitSelected->setEnabled(false);
        ui->actionFitAll->setEnabled(false);
        ui->actionMenuSelection->setEnabled(false);
        ui->actionSelectFace->setEnabled(false);
        ui->actionSelectWithBox2->setEnabled(false);
        ui->actionUnselectAll->setEnabled(false);
        ui->actionShowAll->setEnabled(false);
        ui->actionHideAll->setEnabled(false);
        ui->actionWireframe->setEnabled(false);

        ui->actionDrawingPlaneShow->setEnabled(false);
        ui->actionDrawingSetPlaneToXY->setEnabled(false);
        ui->actionDrawingSetPlaneToXZ->setEnabled(false);
        ui->actionDrawingSetPlaneToYZ->setEnabled(false);
        ui->actionDrawingPlaneHide->setEnabled(false);
        ui->actionDrawingPlaneSnapToGrid->setEnabled(false);
        ui->actionDrawingPlaneSetToFace->setEnabled(false);
        ui->actionDrawingPlaneSetToFaceAxis->setEnabled(false);
        ui->actionDrawLine->setEnabled(false);
        ui->actionDrawPolyline->setEnabled(false);
        ui->actionDrawPolycircle->setEnabled(false);
        ui->actionDrawRectangle->setEnabled(false);
        ui->actionPreferences->setEnabled(false);

        ui->actionSelectMaterialsDatabase->setEnabled(false);
        ui->actionMaterialsOptions->setEnabled(false);

        ui->actionMeshOptions->setEnabled(false);
        ui->actionMeshGenerate->setEnabled(false);
        ui->actionMeshLoad->setEnabled(false);
        ui->actionMeshSave->setEnabled(false);
        ui->actionMeshSaveAs->setEnabled(false);
        ui->actionMeshDelete->setEnabled(false);

        ui->actionMaterials->setEnabled(false);
        ui->actionFrequencyPlan->setEnabled(false);
        ui->actionAntennaPatterns->setEnabled(false);
        ui->actionRefinement->setEnabled(false);
        ui->actionSimulateOptions->setEnabled(false);

        ui->actionRun->setEnabled(false);
        ui->actionRun->setToolTip("Project load is required.");
        ui->actionStop->setEnabled(false);
        ui->actionAbort->setEnabled(false);
        ui->actionAbortAndExit->setEnabled(false);

        ui->actionMaterialsEditor->setEnabled(true);
        ui->actionAbout->setEnabled(true);
        ui->actionLicense->setEnabled(true);
    }

    if (simulationRunning) {
        ui->actionRun->setToolTip("Simulation is running.");
        ui->actionNew->setEnabled(false);
        ui->actionOpen->setEnabled(false);
        ui->actionSave->setEnabled(false);
        ui->actionSaveAs->setEnabled(false);
        ui->actionClose->setEnabled(false);
        ui->actionExit->setEnabled(false);
        ui->actionImportBrep->setEnabled(false);
        ui->actionImportStep->setEnabled(false);
        ui->actionExportBrep->setEnabled(false);
        ui->actionExportStep->setEnabled(false);
        ui->actionDrawLine->setEnabled(false);
        ui->actionDrawPolyline->setEnabled(false);
        ui->actionDrawPolycircle->setEnabled(false);
        ui->actionDrawRectangle->setEnabled(false);
        ui->actionPreferences->setEnabled(false);
        ui->actionDrawingPlaneSnapToGrid->setEnabled(false);
        ui->actionDrawingPlaneSetToFace->setEnabled(false);
        ui->actionDrawingPlaneSetToFaceAxis->setEnabled(false);
        ui->actionDrawingSetPlaneToXY->setEnabled(false);
        ui->actionDrawingSetPlaneToXZ->setEnabled(false);
        ui->actionDrawingSetPlaneToYZ->setEnabled(false);
        ui->actionSelectMaterialsDatabase->setEnabled(true);
        ui->actionMaterialsOptions->setEnabled(true);
        ui->actionMeshOptions->setEnabled(true);
        ui->actionMeshLoad->setEnabled(false);
        ui->actionMeshSave->setEnabled(false);
        ui->actionMeshSaveAs->setEnabled(false);
        ui->actionMeshDelete->setEnabled(false);
        ui->actionSimulateOptions->setEnabled(true);
        ui->actionFrequencyPlan->setEnabled(true);
        ui->actionAntennaPatterns->setEnabled(true);
    }

    ui->actionUndo->setEnabled(itemChangesStack.hasUndo());
    ui->actionRedo->setEnabled(itemChangesStack.hasRedo());

    if (simulationRunning) {
        if (!isDisabledSelection) {
            enablePortBoundarySelections(port,false);
            enablePortBoundarySelections(boundary,false);
            isDisabledSelection=true;
        }
    } else {
        if (isDisabledSelection) {
            enablePortBoundarySelections(port,true);
            enablePortBoundarySelections(boundary,true);
            isDisabledSelection=false;
        }
    }
}

//xxx
void OpenParEMg::enablePortBoundarySelections (BaseItem *baseItem, bool enable)
{
    std::cout << "OpenParEMg::enablePortBoundarySelections  item=" << baseItem->text(0).toStdString() << std::endl; std::cout.flush();

    if (!baseItem) return;

    bool found=false;
    if (baseItem->is_impedanceDefinition()) found=true;
    else if (baseItem->is_impedanceCalculation()) found=true;
    else if (baseItem->is_boundaryWaveImpedance()) found=true;
    else if (baseItem->is_boundaryMaterial()) found=true;
    else if (baseItem->is_sportNumber()) found=true;
    else if (baseItem->is_scaleValue()) found=true;

    if (found) {
        std::cout << "   found" << std::endl; std::cout.flush();
        QWidget *widget=ui->drawingItemTree->itemWidget(baseItem,0);
        if (widget) {
            std::cout << "   widget found, setting enable=" << enable << std::endl; std::cout.flush();
            widget->setEnabled(enable);
        }
    }

    int i=0;
    while (i < baseItem->childCount()) {
        BaseItem *childItem=dynamic_cast<BaseItem *>(baseItem->child(i));
        enablePortBoundarySelections(childItem,enable);
        i++;
    }
}

void OpenParEMg::expand (BaseItem *baseItem)
{
    if (!baseItem) return;
    baseItem->setExpanded(Standard_True);
    int i=0;
    while (i < baseItem->childCount()) {
        BaseItem *child=dynamic_cast<BaseItem *>(baseItem->child(i));
        expand(child);
        i++;
    }
}

void OpenParEMg::collapse (BaseItem *baseItem)
{
    if (!baseItem) return;
    baseItem->setExpanded(Standard_False);
    int i=0;
    while (i < baseItem->childCount()) {
        BaseItem *child=dynamic_cast<BaseItem *>(baseItem->child(i));
        collapse(child);
        i++;
    }
}

void OpenParEMg::expandAllItems ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=ui->drawingWindow->get_selectedItem(i);
        expand(baseItem);
        i++;
    }
}

void OpenParEMg::collapseAllItems ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=ui->drawingWindow->get_selectedItem(i);
        collapse(baseItem);
        i++;
    }
}

void OpenParEMg::buildFaceMenu (QMenu &menu)
{
    //std::cout  << "OpenParEMg::buildFaceMenu" << std::endl; std::cout.flush();

    createPathAction=new QAction("Create Path");
    createPathAction->setToolTip("Create a path from the face.");
    createPortAction=new QAction("Create Port");
    createPortAction->setToolTip("Create a port from the face.");
    createBoundaryAction=new QAction("Create Boundary");
    createBoundaryAction->setToolTip("Create a bounary from the face.");
    cancelAction=new QAction("Cancel");

    connect(createPathAction, &QAction::triggered, this, &OpenParEMg::createPathFromFace);
    connect(createPortAction, &QAction::triggered, this, &OpenParEMg::createPortFromFace);
    connect(createBoundaryAction, &QAction::triggered, this, &OpenParEMg::createBoundaryFromFace);
    connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelMenu);

    if (isValidCreatePathFromFace()) menu.addAction(createPathAction);
    if (isValidCreatePortFromFace()) menu.addAction(createPortAction);
    if (isValidCreateBoundaryFromFace()) menu.addAction(createBoundaryAction);
    menu.addAction(cancelAction);
}

void OpenParEMg::cancelMenu ()
{
    finishOperation(false,1);
}

void OpenParEMg::itemTreeContextMenu_triggered (const QPoint& pnt)
{
    //std::cout << "OpenParEMg::itemTreeContextMenu_triggered" << std::endl; std::cout.flush();

    clickedItem=dynamic_cast<BaseItem *>(ui->drawingItemTree->itemAt(pnt));
    if (!clickedItem) return;
    if (!clickedItem->isSelected()) return;

    QMenu menu(this);
    clickedItem->showMenu(&menu);

    // ToDo: move this menu when the scale item is fully implemented
    if (clickedItem->is_scaleLabel()) {
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
            buildFaceMenu(menu);
        }

        // check for selected items
        long unsigned int i=0;
        while (i < ui->drawingWindow->get_selectedItems_size()) {
            BaseItem *baseItem=ui->drawingWindow->get_selectedItem(i);
            if (baseItem) {
                DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(baseItem);
                if (drawingItem && drawingItem->is_drawing()) {
                    if (drawingItem->getEnableDeletePoint()) {
                        cancelAction=new QAction("Cancel");
                        connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelDeletePoint);
                        menu.addAction(cancelAction);
                    } else if (drawingItem->getEnableInsertPoint()) {
                        cancelAction=new QAction("Cancel");
                        connect(cancelAction, &QAction::triggered, this, &OpenParEMg::cancelInsertPoint);
                        menu.addAction(cancelAction);
                    } else {
                        drawingItem->showMenu(&menu);
                    }
                    break;
                }

                PathItem *pathItem=dynamic_cast<PathItem *>(baseItem);
                if (pathItem && pathItem->is_path()) pathItem->showMenu(&menu);

                // ToDo: convert the remaining items
                if (/*item->is_path() ||*/ baseItem->is_port() || baseItem->is_boundary() || baseItem->is_integrationPathSegment()) {
                    baseItem->showMenu(&menu);
                    break;
                }
            }
            i++;
        }
    }

    menu.exec(ui->drawingWindow->mapToGlobal(pnt));
    freeQActionList();
}

void OpenParEMg::renamePathItems ()
{
    //std::cout << "OpenParEMg::renamePathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (pathItem && pathItem->is_path()) {
            CustomLineEdit *name=new CustomLineEdit();
            name->setText(pathItem->text(0));
            originalText=pathItem->text(0);
            name->set_rxValidator();
            ui->drawingItemTree->setItemWidget(pathItem,0,name);

            renameItem=pathItem;
            renameEdit=name;
            connect(name,&CustomLineEdit::editingFinished,this,&OpenParEMg::rename_editingFinished);
        }
        i++;
    }
}

void OpenParEMg::deletePathItems ()
{
    //std::cout << "OpenParEMg::deletePathItems" << std::endl; std::cout.flush();

    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (pathItem && pathItem->is_path()) {
            pathItem->del();
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(4);
}

bool OpenParEMg::isValidDeletePath ()
{
    // see if any have linked items
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (pathItem && pathItem->linkedItems_size() > 0) return false;
        i++;
    }
    return true;
}

void OpenParEMg::showDrawingItems ()
{
    //std::cout << "OpenParEMg::showDrawingItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem) drawingItem->show(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::hideDrawingItems ()
{
    //std::cout << "OpenParEMg::hideDrawingItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem) drawingItem->hide(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::showPathItems ()
{
    //std::cout << "OpenParEMg::showPathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (pathItem) pathItem->show(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::hidePathItems ()
{
    //std::cout << "OpenParEMg::hidePathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (pathItem) pathItem->hide(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::showPortItems ()
{
    //std::cout << "OpenParEMg::showPortItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PortItem *portItem=dynamic_cast<PortItem *>(ui->drawingWindow->get_selectedItem(i));
        if (portItem) portItem->show(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::hidePortItems ()
{
    //std::cout << "OpenParEMg::hidePortItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PortItem *portItem=dynamic_cast<PortItem *>(ui->drawingWindow->get_selectedItem(i));
        if (portItem) portItem->hide(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::showModeItems ()
{
    //std::cout << "OpenParEMg::showModeItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        ModeItem *modeItem=dynamic_cast<ModeItem *>(ui->drawingWindow->get_selectedItem(i));
        if (modeItem) modeItem->show(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::hideModeItems ()
{
    //std::cout << "OpenParEMg::hideModeItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        ModeItem *modeItem=dynamic_cast<ModeItem *>(ui->drawingWindow->get_selectedItem(i));
        if (modeItem) modeItem->hide(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::showDiffPairItems ()
{
    //std::cout << "OpenParEMg::showDiffPairItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DiffPairItem *diffPairItem=dynamic_cast<DiffPairItem *>(ui->drawingWindow->get_selectedItem(i));
        if (diffPairItem) diffPairItem->show(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::hideDiffPairItems ()
{
    //std::cout << "OpenParEMg::hideDiffPairItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DiffPairItem *diffPairItem=dynamic_cast<DiffPairItem *>(ui->drawingWindow->get_selectedItem(i));
        if (diffPairItem) diffPairItem->hide(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::showVIItems ()
{
    //std::cout << "OpenParEMg::showVIItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        VIItem *viItem=dynamic_cast<VIItem *>(ui->drawingWindow->get_selectedItem(i));
        if (viItem) viItem->show(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::hideVIItems ()
{
    //std::cout << "OpenParEMg::hideVIItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        VIItem *viItem=dynamic_cast<VIItem *>(ui->drawingWindow->get_selectedItem(i));
        if (viItem) viItem->hide(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::showBoundaryItems ()
{
    //std::cout << "OpenParEMg::showBoundaryItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(ui->drawingWindow->get_selectedItem(i));
        if (boundaryItem) boundaryItem->show(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::hideBoundaryItems ()
{
    //std::cout << "OpenParEMg::hideBoundaryItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(ui->drawingWindow->get_selectedItem(i));
        if (boundaryItem) boundaryItem->hide(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::showIntegrationPathItems ()
{
    //std::cout << "OpenParEMg::showIntegrationPathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (integrationPathItem) integrationPathItem->show(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::hideIntegrationPathItems ()
{
    //std::cout << "OpenParEMg::hideIntegrationPathItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (integrationPathItem) integrationPathItem->hide(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::showMeshItems ()
{
    //std::cout << "OpenParEMg::showMeshItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        MeshItem *meshItem=dynamic_cast<MeshItem *>(ui->drawingWindow->get_selectedItem(i));
        if (meshItem) meshItem->show(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::hideMeshItems ()
{
    //std::cout << "OpenParEMg::hideMeshItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        MeshItem *meshItem=dynamic_cast<MeshItem *>(ui->drawingWindow->get_selectedItem(i));
        if (meshItem) meshItem->hide(false);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(30);
}

void OpenParEMg::insertSelectedPaths ()
{
    std::cout << "OpenParEMg::insertSelectedPaths" << std::endl; std::cout.flush();

    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        VIItem *viItem=dynamic_cast<VIItem *>(ui->drawingWindow->get_selectedItem(i));
        if (viItem) insertIntegrationPath(viItem);
        i++;
    }
}

void OpenParEMg::unselectBoundaryItems()
{
    //std::cout << "OpenParEMg::unselectBoundaryItems" << std::endl; std::cout.flush();

    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(ui->drawingWindow->get_selectedItem(i));
        if (boundaryItem) {
            ui->drawingWindow->unselectItem(boundaryItem,i);
        }
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(23);
}

void OpenParEMg::renameBoundaryItems ()
{
    //std::cout << "OpenParEMg::renameBoundaryItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(ui->drawingWindow->get_selectedItem(i));
        if (boundaryItem) {
            CustomLineEdit *name=new CustomLineEdit();
            name->setText(boundaryItem->text(0));
            originalText=boundaryItem->text(0);
            name->set_rxValidator();
            ui->drawingItemTree->setItemWidget(boundaryItem,0,name);

            renameItem=boundaryItem;
            renameEdit=name;
            connect(name,&CustomLineEdit::editingFinished,this,&OpenParEMg::rename_editingFinished);
        }
        i++;
    }
}

void OpenParEMg::deleteBoundaryItems ()
{
    //std::cout << "OpenParEMg::deleteBoundaryItems" << std::endl; std::cout.flush();

    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(ui->drawingWindow->get_selectedItem(i));
        if (boundaryItem) boundaryItem->del();
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    finishOperation(false,1);
}

void OpenParEMg::renameSportNet ()
{
    //std::cout << "OpenParEMg::renameSportNet" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        ModeItem *modeItem=dynamic_cast<ModeItem *>(ui->drawingWindow->get_selectedItem(i));
        if (modeItem) {
            CustomLineEdit *net=new CustomLineEdit();
            net->setText(modeItem->text(0));
            originalText=modeItem->text(0);
            net->set_rxValidator();
            ui->drawingItemTree->setItemWidget(modeItem,0,net);

            renameItem=modeItem;
            renameEdit=net;
            connect(net,&CustomLineEdit::editingFinished,this,&OpenParEMg::rename_editingFinished);
        }
        i++;
    }
}

// bool is_uniqueItem (std::vector<PortItem *> *portItemList, BaseItem *baseItem)
// {
//     long unsigned int i=0;
//     while (i < portItemList->size()) {
//         if ((*portItemList)[i] == baseItem) return false;
//         i++;
//     }
//     return true;
// }

bool OpenParEMg::hasOneSelectedSport ()
{
    bool found=false;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        ModeItem *modeItem=dynamic_cast<ModeItem *>(ui->drawingWindow->get_selectedItem(i));
        if (modeItem) {
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
        ModeItem *modeItem=dynamic_cast<ModeItem *>(ui->drawingWindow->get_selectedItem(i));
        if (modeItem) {
            int j=0;
            while (j < modeItem->childCount()) {
                VIItem *viItem=dynamic_cast<VIItem *>(modeItem->child(j));
                if (viItem && viItem->is_voltage()) return true;
                j++;
            }
        }
        i++;
    }
    return false;
}

bool OpenParEMg::hasCurrent ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        ModeItem *modeItem=dynamic_cast<ModeItem *>(ui->drawingWindow->get_selectedItem(i));
        if (modeItem && modeItem->is_sport()) {
            int j=0;
            while (j < modeItem->childCount()) {
                VIItem *viItem=dynamic_cast<VIItem *>(modeItem->child(j));
                if (viItem && viItem->is_current()) return true;
                j++;
            }
        }
        i++;
    }
    return false;
}

void OpenParEMg::insertIntegrationPath (VIItem *viItem)
{
    std::cout << "OpenParEMg::insertIntegrationPath" << std::endl; std::cout.flush();

    if (!viItem) return;

    QDoubleValidator doubleValidator;
    doubleValidator.setBottom(0);

    // list of paths to add
    std::vector<PathItem *> pathItemList;
    std::vector<Path *> pathsToAdd;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (pathItem) {
            std::cout << "  pathItem to add = " << pathItem->text(0).toStdString() << "   address=" << pathItem << std::endl; std::cout.flush();
            pathItemList.push_back(pathItem);
            pathsToAdd.push_back(pathItem->getPath());
        }
        i++;
    }

    // check that the selected paths can be used on the port

    ModeItem *modeItem=dynamic_cast<ModeItem *>(viItem->getParentItem());
    if (!modeItem) return;
    PortItem *portItem=dynamic_cast<PortItem *>(modeItem->getParentItem());
    if (!portItem) {
        DiffPairItem *diffPairItem=dynamic_cast<DiffPairItem *>(modeItem->getParentItem());
        if (diffPairItem) {
            portItem=dynamic_cast<PortItem *>(diffPairItem->getParentItem());
            if (!portItem) return;
        } else return;
    }

    // port outline
    PathItem *portPathItem=portItem->getPathItem();
    if (!portPathItem) return;
    Path *portPath=portPathItem->getPath();
    if (!portPath) return;

    i=0;
    while (i < pathItemList.size()) {
        Path *itemPath=pathItemList[i]->getPath();
        if (itemPath) {
            if (portPath->is_path_inside(itemPath)) {
                viItem->createIntegrationPathItemFromPath(pathItemList[i]);
            } else {
                QString message="Path \"";
                message.append(pathItemList[i]->text(0));
                message.append("\" cannot be assigned to the selected port.");
                QMessageBox mb;
                mb.critical(nullptr, "Error", message);
                mb.setFixedSize(500, 200);
                return;
            }
        }
        i++;
    }

    viItem->addRemoveScale();
    portItem->setImpedanceDefinitionOptions();

    ui->drawingWindow->updateViewer();
    setMenusI(17);
}

void OpenParEMg::rename_editingFinished ()
{
    //std::cout << "OpenParEMg::rename_editingFinished" << std::endl; std::cout.flush();

    bool unique=true;

    // paths
    PathItem *pathItem=dynamic_cast<PathItem *>(renameItem);
    if (pathItem) {
        if (!path->isUniquePathName(renameEdit->text())) unique=false;
    }

    // ports
    PortItem *portItem=dynamic_cast<PortItem *>(renameItem);
    if (portItem) {
        if (!port->isUniquePortName(renameEdit->text())) unique=false;
    }

    // nets
    ModeItem *modeItem=dynamic_cast<ModeItem *>(renameItem);
    if (modeItem) {
        if (!port->isUniqueNetName(renameEdit->text())) unique=false;
    }

    // boundaries
    BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(renameItem);
    if (boundaryItem) {
        if (!boundary->isUniqueBoundaryName(renameEdit->text())) unique=false;
    }

    if (!unique) {
        QMessageBox mb;
        mb.critical(nullptr, "Error", "Name must be unique.");
        mb.setFixedSize(500, 200);

        finishOperation(true,1);
        return;
    }

    // new text
    QString newText=renameEdit->text();
    if (originalText.compare(newText) != 0) {
        renameItem->rename(newText);
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

    projectChanged=true;
    setMenusI(18);
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::unselectRootDrawingItems()
{
    //std::cout << "OpenParEMg::unselectRootDrawingItems" << std::endl; std::cout.flush();

    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (rootDrawingItem) ui->drawingWindow->unselectItem(rootDrawingItem,i);
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
    //std::cout << "OpenParEMg::unselectDrawingItems" << std::endl; std::cout.flush();

    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem) ui->drawingWindow->unselectItem(drawingItem,i);
        i++;
    }

    ui->drawingItemTree->setCurrentItem(nullptr);
    ui->drawingWindow->updateViewer();
    setMenusI(20);
}

bool OpenParEMg::isValidRenameDrawingItems ()
{
    int count=0;
    QList<QTreeWidgetItem *> items=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < items.count()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(items[i]);
        if (drawingItem) count++;
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
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(items[i]);
        if (drawingItem) {
            //drawingItem->expandToItem();
            CustomLineEdit *name=new CustomLineEdit();
            name->setText(drawingItem->text(0));
            originalText=drawingItem->text(0);
            name->set_rxValidator();
            ui->drawingItemTree->setItemWidget(drawingItem,0,name);

            renameItem=drawingItem;
            renameEdit=name;
            connect(name,&CustomLineEdit::editingFinished,this,&OpenParEMg::rename_editingFinished);
        }
        i++;
    }
}

void OpenParEMg::deleteDrawingItems ()
{
    //std::cout << "OpenParEMg::deleteDrawingItems" << std::endl; std::cout.flush();

    //activeAction=true;  // no need since there is not a cancel option
    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem) drawingItem->del();
        i++;
    }

    finishOperation(false,100);
}

void OpenParEMg::insertModeItems ()
{
    //std::cout << "OpenParEMg::insertModeItems" << std::endl; std::cout.flush();

    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PortItem *portItem=dynamic_cast<PortItem *>(ui->drawingWindow->get_selectedItem(i));
        if (portItem) {
            ModeItem *newModeItem=new ModeItem(this,portItem,true);
            portItem->addChild(newModeItem);
            itemChangesStack.add(newModeItem);
        }
        i++;
    }

    setMenusI(22);
}

void OpenParEMg::unselectPortItems()
{
    //std::cout << "OpenParEMg::unselectPortItems" << std::endl; std::cout.flush();

    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PortItem *portItem=dynamic_cast<PortItem *>(ui->drawingWindow->get_selectedItem(i));
        if (portItem) ui->drawingWindow->unselectItem(portItem,i);
        i++;
    }

    ui->drawingWindow->updateViewer();
    setMenusI(23);
}

bool OpenParEMg::portNameExists (QString name)
{
    int i=0;
    while (i < port->childCount()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(port->child(i));
        if (pathItem) {
            if (pathItem->text(0).compare(name) == 0) return true;
        }
        i++;
    }
    return false;
}

void OpenParEMg::renamePortItems ()
{
    //std::cout << "OpenParEMg::renamePortItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PortItem *portItem=dynamic_cast<PortItem *>(ui->drawingWindow->get_selectedItem(i));
        if (portItem) {
            CustomLineEdit *name=new CustomLineEdit();
            name->setText(portItem->text(0));
            originalText=portItem->text(0);
            name->set_rxValidator();
            ui->drawingItemTree->setItemWidget(portItem,0,name);

            renameItem=portItem;
            renameEdit=name;
            connect(name,&CustomLineEdit::editingFinished,this,&OpenParEMg::rename_editingFinished);
        }
        i++;
    }
}

void OpenParEMg::deleteRootPortItems ()
{
    //std::cout << "OpenParEMg::deleteRootPortItems" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        RootPortItem *rootPortItem=dynamic_cast<RootPortItem *>(ui->drawingWindow->get_selectedItem(i));
        if (rootPortItem) {
            int j=0;
            while (j < rootPortItem->childCount()) {
                PortItem *portItem=dynamic_cast<PortItem *>(rootPortItem->child(j));
                if (portItem) portItem->del();
            }
        }
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    finishOperation(false,1);
}

void OpenParEMg::deletePortItems ()
{
    //std::cout << "OpenParEMg::deletePortItems" << std::endl; std::cout.flush();

    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PortItem *portItem=dynamic_cast<PortItem *>(ui->drawingWindow->get_selectedItem(i));
        if (portItem) portItem->del();
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    finishOperation(false,1);
}

void OpenParEMg::deleteDiffPairItems ()
{
    //std::cout << "OpenParEMg::deleteDiffPairItems" << std::endl; std::cout.flush();

    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DiffPairItem *diffPairItem=dynamic_cast<DiffPairItem *>(ui->drawingWindow->get_selectedItem(i));
        if (diffPairItem) diffPairItem->del();
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    finishOperation(false,1);
}

void OpenParEMg::deleteModeItems ()
{
    //std::cout << "OpenParEMg::deleteModeItems" << std::endl; std::cout.flush();

    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        ModeItem *modeItem=dynamic_cast<ModeItem *>(ui->drawingWindow->get_selectedItem(i));
        if (modeItem) modeItem->del();
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    finishOperation(false,1);
}

void OpenParEMg::flipSignIntegrationPathItems ()
{
    //std::cout << "OpenParEMg::flipSignIntegrationPathItems" << std::endl; std::cout.flush();

    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (integrationPathItem) integrationPathItem->flipSign();
        i++;
    }
    // IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(clickedItem);
    // if (integrationPathItem) integrationPathItem->flipSign();

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    finishOperation(false,1);
}

void OpenParEMg::deleteIntegrationPathItems ()
{
    //std::cout << "OpenParEMg::deleteIntegrationPathItems" << std::endl; std::cout.flush();

    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (integrationPathItem) integrationPathItem->del();
        i++;
    }

    clickedItem=nullptr;
    previousClickedItem=nullptr;

    finishOperation(false,1);
}

bool OpenParEMg::isValidCreatePath ()
{
    if (ui->drawingWindow->get_selectedItems_size() > 0) return false;
    if (ui->drawingWindow->numberDrawingFaceSelected() > 0) return true;
    return false;
}

PortItem* OpenParEMg::get_matchingPortItem (Path *testPath)
{
    if (!testPath) return nullptr;

    int i=0;
    while (i < port->childCount()) {
        PortItem *portItem=dynamic_cast<PortItem *>(port->child(i));
        if (portItem) {
            PathItem *pathItem=portItem->getPathItem();
            if (pathItem) {
                Path *portPath=pathItem->getPath();
                if (portPath) {
                    if (portPath->is_path_inside(testPath)) return portItem;
                }
            }
        }
        i++;
    }
    return nullptr;
}

void OpenParEMg::createPath ()
{
    //std::cout << "OpenParEMg::createPath  NbSelected=" << ui->drawingWindow->NbSelected() << std::endl; std::cout.flush();

    int i=0;
    while (i < ui->drawingWindow->NbSelected()) {
        TopoDS_Shape selectedShape=ui->drawingWindow->get_selectedSubshape(i);
        if (selectedShape.ShapeType() == TopAbs_FACE) {

            // default path name
            QString pathName=path->getUniquePathName();

            // path

            Path *newPath=new Path(0,0);
            newPath->set_name(pathName.toStdString());
            newPath->is_modified();
            newPath->addFacePoints(TopoDS::Face(selectedShape));
            newPath->create_wire_item(this,ui->drawingWindow,path);  // create item and add as child to path; creates AIS_Shape

            // save the items for later
            PathItem *pathItem=newPath->get_item();
            if (pathItem) {
                ui->drawingWindow->insertItemToMap(pathItem->getShape(),pathItem);
                ui->drawingWindow->showItem(pathItem);
                ui->drawingWindow->selectItem(pathItem);
            }

            // see if the path is within an existing port
            PortItem *portItem=get_matchingPortItem(newPath);
            if (portItem) newPath->set_portItem(portItem);
        }
        i++;
    }

    ui->drawingWindow->setSubshapeSelection(false);
    on_actionShape_triggered();

    ui->drawingWindow->updateViewer();
    setMenusI(36);
}

bool OpenParEMg::isValidExtrudePolywire ()
{
    int polywireCount=0;
    QList<QTreeWidgetItem*> items=ui->drawingItemTree->selectedItems();
    int i=0;
    while (i < items.count()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(items[i]);
        if (drawingItem && drawingItem->is_drawing()) {
            RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(drawingItem->getParentItem());
            if (rootDrawingItem) {
                Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
                if (polywire && polywire->isClosed()) polywireCount++;
            }
        }
        i++;
    }
    if (polywireCount > 0 && items.count() == polywireCount) return true;
    return false;
}

// from m to the given unit
double OpenParEMg::getConversionFactor ()
{
    if (!projData.gui_units) return 1;
    if (strcmp(projData.gui_units,"m") == 0) return 1;
    if (strcmp(projData.gui_units,"cm") == 0) return 100;
    if (strcmp(projData.gui_units,"mm") == 0) return 1000;
    if (strcmp(projData.gui_units,"um") == 0) return 1e6;
    if (strcmp(projData.gui_units,"ft") == 0) return 100/2.54/12;
    if (strcmp(projData.gui_units,"in") == 0) return 100/2.54;
    if (strcmp(projData.gui_units,"mil") == 0) return 100/2.54*1000;
    return 1;
}

void OpenParEMg::extrudePolywire ()
{
    std::cout << "OpenParEMg::extrudePolywire" << std::endl; std::cout.flush();

    startOperation(true);
    activeAction=false;

    // user input form
    if (lengthInputForm) delete lengthInputForm;
    lengthInputForm=new LengthInputForm(this);
    lengthInputForm->set_conversionFactor(getConversionFactor());
    lengthInputForm->set_drawingWindow(ui->drawingWindow);
    extrusionDirection.SetCoord(0,0,0);
    lengthInputForm->set_extrusionDirection(&extrusionDirection);
    lengthInputForm->set_length(&length);
    lengthInputForm->set_relay(relay);
    lengthInputForm->setModal(false);
    connect(this,&OpenParEMg::sendPnt,lengthInputForm,&LengthInputForm::pickVertexFinished);
    lengthInputForm->show();
}

void OpenParEMg::finishExtrudePolywire ()
{
    std::cout << "OpenParEMg::finishExtrudePolywire" << std::endl; std::cout.flush();

    if (abs(length) > 1e-12) {

        std::vector<DrawingItem *> selectedItems;
        int i=0;
        while (i < ui->drawingWindow->get_selectedItems_size()) {
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
            if (drawingItem && drawingItem->is_drawing()) selectedItems.push_back(drawingItem);
            i++;
        }

        if (selectedItems.size() > 0) itemChangesStack.startNew();

        i=0;
        while (i < selectedItems.size()) {
            selectedItems[i]->extrude();
            i++;
        }
    }

    if (lengthInputForm) {lengthInputForm=nullptr;}

    // Do not call finishOperation: lengthInputForm calls finishOperation
    //finishOperation(false,1);
}

// stop on incomplete structures - happens while loading a drawing
// eventually, everything loads and the reprocess completes
void OpenParEMg::reprocess (BaseItem *baseItem)
{
    //std::cout << "OpenParEMg::reprocess  item=" << item << std::endl; std::cout.flush();

    // QString message="OpenparEMg::reprocess  ";
    // message.append(baseItem->text(0));
    // {QMessageBox mb; mb.critical(nullptr, "Debug", message);}

    bool stop=false;

    if (!baseItem) return;

    //bool isDisplayed=ui->drawingWindow->isDisplayed(baseItem->getShape());
    bool isDisplayed=false;
    if (baseItem->foreground(0) == Qt::black) isDisplayed=true;

    RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(baseItem);
    if (rootDrawingItem) {

        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);

        // cycle through the top-level children and add to the compound
        int i=0;
        while (i < drawing->childCount()) {
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(drawing->child(i));
            if (drawingItem && !drawingItem->getShape().IsNull()) {
                builder.Add(compound,drawingItem->getShape()->Shape());
            }
            i++;
        }

        Handle(AIS_Shape) newAISshape=new AIS_Shape(compound);

        ShapeData *shapeData=baseItem->getShapeData();
        shapeData->setShape(newAISshape);

        return;
    }

    DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(baseItem);
    if (drawingItem && drawingItem->is_drawing()) {

        Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
        if (polywire) {

            ShapeData *shapeData=drawingItem->getShapeData();
            if (!shapeData->getShape().IsNull()) {
                ui->drawingWindow->hideItem(drawingItem);
                ui->drawingWindow->removeItemFromMap(drawingItem);
                ui->drawingWindow->deleteShape(drawingItem->getShape());
            }
            shapeData->setShape(polywire->get_AIS_Shape());

            ui->drawingWindow->insertItemToMap(baseItem->getShape(),drawingItem);
            ui->drawingWindow->activateItem(drawingItem);
        }

        Process *process=static_cast<Process *>(drawingItem->getProcess());
        if (process) {

            Extrude *extrude=dynamic_cast<Extrude *>(process);
            if (extrude) {
                if (drawingItem->childCount() > 0) {
                    int i=0;
                    while (i < drawingItem->childCount()) {
                        DrawingItem *child=(DrawingItem *)drawingItem->child(i);
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

                                    if (!drawingItem->getShape().IsNull()) {
                                        ui->drawingWindow->hideItem(drawingItem);
                                        ui->drawingWindow->removeItemFromMap(drawingItem);
                                        ui->drawingWindow->deleteShape(drawingItem->getShape());
                                    }

                                    ShapeData *shapeData=drawingItem->getShapeData();
                                    shapeData->setShape(newAISshape);

                                    ui->drawingWindow->insertItemToMap(drawingItem->getShape(),drawingItem);
                                    ui->drawingWindow->activateItem(drawingItem);

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
                if (drawingItem->childCount() == 2) {

                    DrawingItem *child1=(DrawingItem *)drawingItem->child(0);
                    DrawingItem *child2=(DrawingItem *)drawingItem->child(1);

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

                            if (!drawingItem->getShape().IsNull()) {
                                ui->drawingWindow->hideItem(drawingItem);
                                ui->drawingWindow->removeItemFromMap(drawingItem);
                                ui->drawingWindow->deleteShape(drawingItem->getShape());
                            }

                            ShapeData *shapeData=drawingItem->getShapeData();
                            shapeData->setShape(newAISshape);

                            ui->drawingWindow->insertItemToMap(drawingItem->getShape(),drawingItem);
                            ui->drawingWindow->activateItem(drawingItem);

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
                if (drawingItem->childCount() == 2) {

                    DrawingItem *child1=(DrawingItem *)drawingItem->child(0);
                    DrawingItem *child2=(DrawingItem *)drawingItem->child(1);

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

                            if (!drawingItem->getShape().IsNull()) {
                                ui->drawingWindow->hideItem(drawingItem);
                                ui->drawingWindow->removeItemFromMap(drawingItem);
                                ui->drawingWindow->deleteShape(drawingItem->getShape());
                            }

                            ShapeData *shapeData=drawingItem->getShapeData();
                            shapeData->setShape(newAISshape);

                            ui->drawingWindow->insertItemToMap(drawingItem->getShape(),drawingItem);
                            ui->drawingWindow->activateItem(drawingItem);

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

        if (!polywire && !process) {
            ui->drawingWindow->activateItem(drawingItem);
        }

        if (isDisplayed) ui->drawingWindow->showItem(baseItem);
        else ui->drawingWindow->hideItem(baseItem);

        // necessary?
        drawingItem->reset_transformation();
    }

    // recursively work to the top of the tree
    if (!stop) {
        reprocess(baseItem->getParentItem());
    }
}

bool OpenParEMg::isValidSetPlane ()
{
    if (ui->drawingWindow->get_selectedItems_size() > 0) return false;
    if (ui->drawingWindow->numberDrawingFaceSelected() == 1) return true;
    return false;
}

bool OpenParEMg::isValidAssignMaterial ()
{
    if (ui->drawingWindow->get_selectedItems_size() != 1) return false;
    DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(0));
    if (drawingItem && drawingItem->is_drawing()) {
        RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(drawingItem->getParentItem());
        if (rootDrawingItem) {

            // SOLID
            if (drawingItem->getShape()->Shape().ShapeType() == TopAbs_SOLID) {
                clickedItem=drawingItem;
                return true;
            }

            // COMPOUND
            if (drawingItem->getShape()->Shape().ShapeType() == TopAbs_COMPOUND) {
                // make sure it is not a polywire (a polycircle is a COMPOUND with a center point added)
                if (!drawingItem->getPolywire()) {
                    clickedItem=drawingItem;
                    return true;
                }
            }
        }
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
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
            if (polywire && polywire->canEdit()) count++;

            Process *process=static_cast<Process *>(drawingItem->getProcess());
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

    startOperation(true);
    activeAction=true;
    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->startEdit();
        i++;
    }
}

void OpenParEMg::finishEditObject (bool cancel)
{
    //std::cout << "OpenParEMg::finishEditObject  length=" << length << "  cancel=" << cancel << std::endl; std::cout.flush();

    if (cancel) {
        if (lineEdit) {delete lineEdit; lineEdit=nullptr;}
        if (rectangleEdit) {delete rectangleEdit; rectangleEdit=nullptr;}
        if (polycircleEdit) {delete polycircleEdit; polycircleEdit=nullptr;}
    } else {
        long unsigned int i=0;
        while (i < ui->drawingWindow->get_selectedItems_size()) {
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
            if (drawingItem && drawingItem->is_drawing()) drawingItem->finishEdit();
            i++;
        }
    }

    if (lengthEditForm) {lengthEditForm=nullptr;}
    if (lineEditForm) {lineEditForm=nullptr;}
    if (rectangleEditForm) {rectangleEditForm=nullptr;}
    if (polycircleEditForm) {polycircleEditForm=nullptr;}

    // Do not call finishOperation: the edit forms call finishOperation
    //finishOperation(false,2);
}

bool OpenParEMg::isValidMergeSolids ()
{
    //std::cout << "OpenParEMg::isValidMergeSolids" << std::endl; std::cout.flush();

    int solidCount=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=ui->drawingWindow->get_selectedItem(i);
        if (baseItem) {
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
            if (drawingItem && drawingItem->is_drawing()) {
                BaseItem *parent=baseItem->getParentItem();
                if (parent) {
                    RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(parent);
                    if (rootDrawingItem) {
                        Handle(AIS_Shape) shape=drawingItem->getShape();
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
    DrawingItem *item0=nullptr;
    DrawingItem *item1=nullptr;
    long unsigned int index0;
    long unsigned int index1;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=ui->drawingWindow->get_selectedItem(i);
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(baseItem);
        if (drawingItem && drawingItem->is_drawing()) {
            if (!item0) {
                item0=drawingItem;
                index0=i;
            } else {
                item1=drawingItem;
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
    DrawingItem *newItem=new DrawingItem(this,drawing);
    ShapeData *shapeData=newItem->getShapeData();
    shapeData->setProcess(merge);
    shapeData->setShape(newAISshape);
    newItem->setText(0,merge->getName(&objectCounts));
    shapeData->set_name(newItem->text(0));
    drawing->addChild(newItem);
    ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);
    ui->drawingWindow->showItem(newItem);
    ui->drawingWindow->selectItem(newItem);
    itemChangesStack.add(newItem);

    // save the objects for undo/redo
    newItem->push_child(item0);
    newItem->push_child(item1);
    newItem->demoteChildren();

    // reset materials
    if (!item0->text(1).isNull()) {
        newItem->setText(1,item0->text(1));
        item0->setText(1,QString());
    }
    if (!item1->text(1).isNull()) {
        newItem->setText(1,item1->text(1));
        item1->setText(1,QString());
    }

    // reset dimTags
    item0->set_dimTag(-1,-1);
    item1->set_dimTag(-1,-1);

    // adjust depth
    increase_depth(item0);
    increase_depth(item1);

    // rebuild top level
    reprocess(drawing);

    ui->drawingWindow->hideItem(item0);
    ui->drawingWindow->unselectItem(item0,index0);
    item0->resetOperation();

    ui->drawingWindow->hideItem(item1);
    ui->drawingWindow->unselectItem(item1,index1);
    item1->resetOperation();

    ui->drawingWindow->showItem(newItem);
    ui->drawingWindow->selectItem(newItem);

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
    DrawingItem *item0=nullptr;
    DrawingItem *item1=nullptr;
    long unsigned int index0;
    long unsigned int index1;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=ui->drawingWindow->get_selectedItem(i);
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(baseItem);
        if (drawingItem && drawingItem->is_drawing()) {
            if (!item0) {
                item0=drawingItem;
                index0=i;
            } else {
                item1=drawingItem;
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
    DrawingItem *newItem=new DrawingItem(this,drawing);
    ShapeData *shapeData=newItem->getShapeData();
    shapeData->setProcess(subtract);
    shapeData->setShape(newAISshape);
    newItem->setText(0,subtract->getName(&objectCounts));
    shapeData->set_name(newItem->text(0));
    drawing->addChild(newItem);
    ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);
    ui->drawingWindow->showItem(newItem);
    ui->drawingWindow->selectItem(newItem);
    itemChangesStack.add(newItem);

    // save the objects for undo/redo
    newItem->push_child(item0);
    newItem->push_child(item1);
    newItem->demoteChildren();

    // reset materials
    if (!item0->text(1).isNull()) {
        newItem->setText(1,item0->text(1));
        item0->setText(1,QString());
    }
    if (!item1->text(1).isNull()) {
        newItem->setText(1,item1->text(1));
        item1->setText(1,QString());
    }

    // reset dimTags
    item0->set_dimTag(-1,-1);
    item1->set_dimTag(-1,-1);

    // adjust depth
    increase_depth(item0);
    increase_depth(item1);

    // rebuild top level
    reprocess(drawing);

    ui->drawingWindow->hideItem(item0);
    ui->drawingWindow->unselectItem(item0,index0);
    item0->resetOperation();

    ui->drawingWindow->hideItem(item1);
    ui->drawingWindow->unselectItem(item1,index1);
    item1->resetOperation();

    ui->drawingWindow->showItem(newItem);
    ui->drawingWindow->selectItem(newItem);

    finishOperation(false,4);
}

bool OpenParEMg::isValidObjectMove ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) count++;
        i++;
    }
    if (count > 0 && count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::moveObject ()
{
    startOperation(true);
    activeAction=true;
    itemChangesStack.startNew();
    ui->drawingWindow->set_pickSecondVertex(true);

    // mark original selection
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->setOriginalSelection(true);
        i++;
    }

    // select children
    i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            int j=0;
            while (j < drawingItem->childCount()) {
                DrawingItem *childItem=dynamic_cast<DrawingItem *>(drawingItem->child(j));
                if (childItem && childItem->is_drawing()) {
                    ui->drawingWindow->simpleSelectDrawingItem(childItem);
                    childItem->setOriginalSelection(false);
                }
                j++;
            }
        }
        i++;
    }

    // set for undo/redo
    i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->startMove(drawingItem->isOriginalSelection());
        i++;
    }
}

// void printPnt (std::string &name, const gp_Pnt &p)
// {
//     std::cout << name << "=(" << p.X() << "," << p.Y() << "," << p.Z() << ")" << std::endl; std::cout.flush();
// }

void OpenParEMg::finishMoveObject (gp_Pnt p0, gp_Pnt p1)
{
    // std::cout << "OpenParEMg::finishMoveObject" << std::endl; std::cout.flush();

    // move the objects
    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->finishMove(p0,p1);
        i++;
    }

    // update the top level
    // i=0;
    // while (i < ui->drawingWindow->get_selectedItems_size()) {
    //     BaseItem *baseItem=ui->drawingWindow->get_selectedItem(i);
    //     if (baseItem) {
    //         DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(baseItem);
    //         if (drawingItem && drawingItem->is_drawing()) drawingItem->findTopLevelItem(drawingItem);
    //     }
    //     i++;
    // }

    finishOperation(false,250);
}

bool OpenParEMg::isValidCopy ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) count++;
        i++;
    }
    if (count > 0 && count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::copyDrawingItems ()
{
    //std::cout << "OpenParEMg::copyDrawingItems" << std::endl; std::cout.flush();

    startOperation(true);
    //activeAction=true;  // no need since there is not a cancel option
    itemChangesStack.startNew();

    std::vector<DrawingItem *> selectedList;

    // copy the selected items
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            DrawingItem *newDrawingItem=drawingItem->copy(drawing);
            selectedList.push_back(newDrawingItem);
        }
        i++;
    }

    // show and select the new items
    ui->drawingWindow->unselectAllItems();
    i=0;
    while (i < selectedList.size()) {
        //ui->drawingWindow->hideItem(selectedList[i]);
        ui->drawingWindow->showItem(selectedList[i]);
        ui->drawingWindow->selectItem(selectedList[i]);
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
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
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
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->startStretch();
        i++;
    }
}

void OpenParEMg::finishStretchObject ()
{
    //std::cout << "OpenParEMg::finishStretchObject" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->finishStretch();
        i++;
    }
}

bool OpenParEMg::isValidDeletePoint ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
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
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->startDeletePoint();
        i++;
    }
}

void OpenParEMg::finishDeletePoint (BaseItem *baseItem)
{
    if (!baseItem) return;
    baseItem->finishDeletePoint();
    finishOperation(false,8);
}

void OpenParEMg::cancelDeletePoint ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->cancelDeletePoint();
        i++;
    }
    finishOperation(true,4005);
}

bool OpenParEMg::isValidInsertPoint ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
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
    ui->drawingWindow->set_pickSecondVertex(true);

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->startInsertPoint();
        i++;
    }
}

void OpenParEMg::finishInsertPoint (BaseItem *baseItem)
{
    if (!baseItem) return;
    baseItem->finishInsertPoint();
}

// void OpenParEMg::finishStretchPoint (BaseItem *baseItem)
// {
//     //std::cout << "OpenParEMg::finishStretchPoint" << std::endl; std::cout.flush();

//     if (!baseItem) return;
//     baseItem->finishStretchPoint();
//     baseItem->findTopLevelItem(baseItem);

//     finishOperation(false,7);
// }

void OpenParEMg::cancelInsertPoint ()
{
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->cancelInsertPoint();
        i++;
    }
    finishOperation(true,4005);
}

bool OpenParEMg::isValidCloseExistingPolyline ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
            if (polywire && polywire->canClose()) count++;
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
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
            if (polywire) {

                // for undo/redo

                // activeAction=true;  // no need since there is no cancel option
                itemChangesStack.startNew();

                // remove the old version from display and tracking
                ui->drawingWindow->hideItem(drawingItem);
                ui->drawingWindow->removeItemFromMap(drawingItem);
                ui->drawingWindow->deleteShape(drawingItem->getShape());  // lose selection

                // clone the item onto itself for undo/redo
                ShapeData *newShapeData=drawingItem->getShapeData()->copyCreate();
                newShapeData->setEdit();
                drawingItem->addShapeData(newShapeData);
                itemChangesStack.add(drawingItem);

                // add the new item back to the display and tracking
                ui->drawingWindow->insertItemToMap(drawingItem->getShape(),drawingItem);

                // modify the clone

                polywire=static_cast<Polywire *>(drawingItem->getPolywire());
                polywire->close();
                reprocess(drawingItem);
                drawingItem->setText(0,polywire->getName(&objectCounts));
                newShapeData=drawingItem->getShapeData()->copyCreate();
                newShapeData->setChangeName();
                newShapeData->set_name(drawingItem->text(0));
                drawingItem->addShapeData(newShapeData);
                itemChangesStack.add(drawingItem);
                drawingItem->getShape()->SetZLayer(Graphic3d_ZLayerId_Top);
                ui->drawingWindow->showItem(drawingItem);
                ui->drawingWindow->selectItem(drawingItem);
                ui->drawingWindow->updateViewer();
                drawingItem->setModified(true);

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
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
            if (polywire && polywire->canOpen()) count++;
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
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
            if (polywire) {
                // for undo/redo

                //activeAction=true;  // no need since there is no cancel option
                itemChangesStack.startNew();

                // remove the old version from display and tracking
                ui->drawingWindow->hideItem(drawingItem);
                ui->drawingWindow->removeItemFromMap(drawingItem);
                ui->drawingWindow->deleteShape(drawingItem->getShape());  // lose selection

                // clone the item onto itself for undo/redo
                ShapeData *newShapeData=drawingItem->getShapeData()->copyCreate();
                newShapeData->setEdit();
                drawingItem->addShapeData(newShapeData);
                itemChangesStack.add(drawingItem);

                // add the new item back to the display and tracking
                ui->drawingWindow->insertItemToMap(drawingItem->getShape(),drawingItem);

                // modify the clone

                polywire=static_cast<Polywire *>(drawingItem->getPolywire());
                polywire->open();
                reprocess(drawingItem);
                drawingItem->setText(0,polywire->getName(&objectCounts));
                newShapeData=drawingItem->getShapeData()->copyCreate();
                newShapeData->setChangeName();
                newShapeData->set_name(drawingItem->text(0));
                drawingItem->addShapeData(newShapeData);
                itemChangesStack.add(drawingItem);
                drawingItem->getShape()->SetZLayer(Graphic3d_ZLayerId_Top);
                ui->drawingWindow->showItem(drawingItem);
                ui->drawingWindow->selectItem(drawingItem);
                ui->drawingWindow->updateViewer();
                drawingItem->setModified(true);

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
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
            if (polywire && polywire->canConvert()) count++;
        }
        i++;
    }
    if (count > 0 && count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::convertToPolyline ()
{
    startOperation(true);
    // activeAction=true;  // not needed since the action cannot be canceled
    itemChangesStack.startNew();

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->convertToPolyline();
        i++;
    }

    finishOperation(false,6000);
}

bool OpenParEMg::isValidConvertToPath ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
            if (polywire) {
                RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(drawingItem->getParentItem());
                if (rootDrawingItem) count++;
            }
        }
        i++;
    }
    if (count > 0 && count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::convertDrawingToPath ()
{
    std::cout << "OpenParEMg::convertDrawingToPath" << std::endl; std::cout.flush();
    convertDrawingToPathN(true);
}

void OpenParEMg::convertDrawingToPathN (bool startNew)
{
    std::cout << "OpenParEMg::convertDrawingToPathN" << std::endl; std::cout.flush();

    if (startNew) itemChangesStack.startNew();

    std::vector<DrawingItem *> selectedList;

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
            if (polywire) selectedList.push_back(drawingItem);
        }
        i++;
    }

    bool useArrows=startNew;
    i=0;
    while (i < selectedList.size()) {
        selectedList[i]->createPath(useArrows);
        selectedList[i]->del();
        i++;
    }

    finishOperation(false,1);
}

bool OpenParEMg::isValidRotateObject ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) count++;
        i++;
    }
    if (count > 0 && count == ui->drawingWindow->get_selectedItems_count()) return true;
    return false;
}

void OpenParEMg::rotateObject ()
{
    //std::cout << "OpenParEMg::rotateObject" << std::endl; std::cout.flush();

    startOperation(true);
    activeAction=true;
    itemChangesStack.startNew();

    // mark original selection
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->setOriginalSelection(true);
        i++;
    }

    // select children
    i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            int j=0;
            while (j < drawingItem->childCount()) {
                DrawingItem *childItem=dynamic_cast<DrawingItem *>(drawingItem->child(j));
                if (childItem && childItem->is_drawing()) {
                    ui->drawingWindow->simpleSelectDrawingItem(childItem);
                    childItem->setOriginalSelection(false);
                }
                j++;
            }
        }
        i++;
    }

    // set up for undo/redo
    i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) drawingItem->startRotate();
        i++;
    }

    ui->drawingWindow->updateViewer();

    if (rotateInputForm) delete rotateInputForm;
    rotateInputForm=new RotateInputForm(this);
    rotateInputForm->set_conversionFactor(getConversionFactor());
    rotateInputForm->set_drawingWindow(ui->drawingWindow);
    rotateInputForm->set_angle(&angle);
    rotateInputForm->set_startPoint(&startPoint);
    rotateInputForm->set_endPoint(&endPoint);
    rotateInputForm->set_relay(relay);
    rotateInputForm->setModal(false);
    connect(this,&OpenParEMg::sendPnt,rotateInputForm,&RotateInputForm::pickVertexFinished);
    rotateInputForm->show();
}

void OpenParEMg::finishRotateObject (double angle_, gp_Pnt startPoint_, gp_Pnt endPoint_)
{
    //std::cout << "OpenParEMg::finishRotateObject" << std::endl; std::cout.flush();

    std::vector<DrawingItem *> selectedList;
    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) selectedList.push_back(drawingItem);
        i++;
    }

    i=0;
    while (i < selectedList.size()) {
        selectedList[i]->finishRotate(angle_,startPoint_,endPoint_);
        i++;
    }

    // Do not call finishOperation: rotateInputForm calls finishOperation
    //finishOperation(false,1);
}

bool OpenParEMg::isValidCreatePortFromFace ()
{
    if (ui->drawingWindow->get_selectedItems_size() > 0) return false;
    if (ui->drawingWindow->numberDrawingFaceSelected() > 0) return true;
    return false;
}

void OpenParEMg::createPortFromFace ()
{
    //std::cout << "OpenParEMg::createPortFromFace" << std::endl; std::cout.flush();
    itemChangesStack.startNew();
    createPathFromFaceN(false);
    createPortFromPathN(false);
}

bool OpenParEMg::isValidCreatePortFromPath ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (pathItem) {
            // cannot have any linked items, showing that it is already in use
            if (pathItem->linkedItems_size() == 0) {
                // must be closed
                Path *aPath=pathItem->getPath();
                if (aPath && aPath->is_closed()) count ++;
            }
        }
        i++;
    }

    if (count > 0 && ui->drawingWindow->get_selectedItems_count() == count) return true;
    return false;
}

void OpenParEMg::createPortFromPath ()
{
    createPortFromPathN(true);
}


void OpenParEMg::createPortFromPathN (bool startNew)
{
    //std::cout << "OpenParEMg::createPortFromPath" << std::endl; std::cout.flush();

    if (startNew) itemChangesStack.startNew();

    // create list of selected items

    std::vector<PathItem *> selectedList;

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (pathItem) {
            if (pathItem->linkedItems_size() == 0) {
                Path *aPath=static_cast<Path *>(pathItem->getPath());
                if (aPath && aPath->is_closed()) {
                    selectedList.push_back(pathItem);
                }
            }
        }
        i++;
    }

    // create the ports

    std::vector<PortItem *> convertedItems;

    i=0;
    while (i < selectedList.size()) {

        Path *aPath=static_cast<Path *>(selectedList[i]->getPath());
        if (aPath) {
            // remove the arrows from the path
            selectedList[i]->showArrows(false);

            // next available s-port number
            int sport=sportNumbers.next();

            QString impedance_calculation="line";
            QString impedance_definition="invalid";
            PortItem *newPortItem=new PortItem(this,selectedList[i],impedance_calculation,impedance_definition);
            if (newPortItem) {

                // add one default mode since at least one mode is required
                ModeItem *newModeItem=new ModeItem(this,newPortItem,true);
                newPortItem->addChild(newModeItem);

                convertedItems.push_back(newPortItem);
                itemChangesStack.add(newPortItem);

                port->addChild(newPortItem);
                port->setExpanded(true);
                newPortItem->setExpanded(true);

                newPortItem->setImpedanceDefinitionOptions();
            }
        }
        i++;
    }

    // clear all selections
    clearTreeSelection();

    // select the new items
    i=0;
    while (i < convertedItems.size()) {
        ui->drawingWindow->selectItem(convertedItems[i]);
        i++;
    }

    finishOperation(false,1);
}

void OpenParEMg::createPathFromFace ()
{
    createPathFromFaceN(true);
}

void OpenParEMg::createPathFromFaceN (bool startNew)
{
    //std::cout << "OpenParEMg::createPathFromFace" << std::endl; std::cout.flush();

    if (startNew) itemChangesStack.startNew();

    std::vector<BaseItem *> createdItemsList;

    int i=0;
    while (i < ui->drawingWindow->NbSelected()) {
        TopoDS_Shape selectedShape=ui->drawingWindow->get_selectedSubshape(i);
        if (!selectedShape.IsNull()) {
            if (selectedShape.ShapeType() == TopAbs_FACE) {

                // default path name
                QString pathName=path->getUniquePathName();

                // path
                Path *newPath=new Path(0,0);
                newPath->set_name(pathName.toStdString());
                newPath->is_modified();
                newPath->addFacePoints(TopoDS::Face(selectedShape));
                newPath->create_face_item(this,ui->drawingWindow,path);  // create item and add as child to path; creates AIS_Shape

                // add new path to the drawing
                PathItem *pathItem=newPath->get_item();
                if (pathItem) {
                    itemChangesStack.add(pathItem);
                    createdItemsList.push_back(pathItem);
                }
            }
        }
        i++;
    }

    // clear all selections
    clearTreeSelection();

    // add the new items
    i=0;
    while (i < createdItemsList.size()) {
        ui->drawingWindow->insertItemToMap(createdItemsList[i]->getShape(),createdItemsList[i]);
        ui->drawingWindow->showItem(createdItemsList[i]);
        ui->drawingWindow->selectItem(createdItemsList[i]);
        i++;
    }

    finishOperation(false,1);
}

bool OpenParEMg::isValidCreatePathFromFace ()
{
    return isValidCreatePortFromFace();
}

bool OpenParEMg::isValidCreateBoundaryFromFace ()
{
    return isValidCreatePortFromFace();
}

void OpenParEMg::createBoundaryFromFace ()
{
    //std::cout << "OpenParEMg::createBoundaryFromFace" << std::endl; std::cout.flush();
    itemChangesStack.startNew();
    createPathFromFaceN(false);
    createBoundaryFromPathN(false);
}

bool OpenParEMg::isValidCreateBoundaryFromPath ()
{
    std::cout << "OpenParEMg::isValidCreateBoundaryFromPath" << std::endl; std::cout.flush();
    return isValidCreatePortFromPath();
}

void OpenParEMg::createBoundaryFromPath ()
{
    std::cout << "OpenParEMg::createBoundaryFromPath" << std::endl; std::cout.flush();
    createBoundaryFromPathN(true);
}

void OpenParEMg::createBoundaryFromPathN (bool startNew)
{
    std::cout << "OpenParEMg::createBoundaryFromPathN" << std::endl; std::cout.flush();

    if (startNew) itemChangesStack.startNew();

    // create list of selected items

    std::vector<PathItem *> selectedList;

    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (pathItem) {
            if (pathItem->linkedItems_size() == 0) {
                Path *aPath=static_cast<Path *>(pathItem->getPath());
                if (aPath->is_closed()) {
                    selectedList.push_back(pathItem);
                }
            }
        }
        i++;
    }

    // create the boundaries

    std::vector<BoundaryItem *> convertedItems;

    i=0;
    while (i < selectedList.size()) {

        // remove the arrows from the path
        selectedList[i]->showArrows(false);

        // default to radiation
        double default_wave_impedance=sqrt(M_PI*4e-7/8.8541878176e-12);
        QString boundary_material;
        BoundaryItem *newBoundaryItem=new BoundaryItem(this,selectedList[i],3,default_wave_impedance,boundary_material);

        QString boundaryName=boundary->getUniqueBoundaryName();
        newBoundaryItem->setText(0,boundaryName);

        boundary->addChild(newBoundaryItem);
        boundary->setExpanded(true);
        newBoundaryItem->setExpanded(true);

        // hack: cycle through the comboBox to get it to show properly
        // otherwise, the material item will not hide
        { // begin hack
        BaseItem *boundaryType=nullptr;
        BaseItem *boundaryWaveImpedance=nullptr;
        BaseItem *boundaryMaterial=nullptr;

        int j=0;
        while (j < newBoundaryItem->childCount()) {
            BaseItem *baseItem=dynamic_cast<BaseItem *>(newBoundaryItem->child(j));
            if (baseItem) {
                if (baseItem->is_boundaryType()) boundaryType=baseItem;
                else if (baseItem->is_boundaryWaveImpedance()) boundaryWaveImpedance=baseItem;
                else if (baseItem->is_boundaryMaterial()) boundaryMaterial=baseItem;
            }
            j++;
        }
        ShapeData *shapeData=newBoundaryItem->getShapeData();
        comboRefresh(shapeData->get_boundary_type(),nullptr,newBoundaryItem,2,boundaryMaterial,boundaryWaveImpedance);
        } // end hack

        convertedItems.push_back(newBoundaryItem);
        itemChangesStack.add(newBoundaryItem);

        i++;
    }

    // clear all selections
    clearTreeSelection();

    // select the new items
    i=0;
    while (i < convertedItems.size()) {
        ui->drawingWindow->selectItem(convertedItems[i]);
        i++;
    }

    finishOperation(false,1);
}

bool OpenParEMg::isValidConvertToPort ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            // must be a polywire at the top level
            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
            if (polywire) {
                RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(drawingItem->getParentItem());
                if (rootDrawingItem) {
                    if (polywire->isClosed()) count++;
                }
            }
        }
        i++;
    }

    if (count > 0 && ui->drawingWindow->get_selectedItems_count() == count) return true;
    return false;
}

void OpenParEMg::convertDrawingToPort ()
{
    itemChangesStack.startNew();
    convertDrawingToPathN(false);
    createPortFromPathN(false);
}

bool OpenParEMg::isValidConvertToBoundary ()
{
    return isValidConvertToPort();
}

void OpenParEMg::convertDrawingToBoundary ()
{
    itemChangesStack.startNew();
    convertDrawingToPathN(false);
    createBoundaryFromPathN(false);
}

bool OpenParEMg::isValidReversePath ()
{
    //std::cout << "OpenParEMg::isValidReversePath" << std::endl; std::cout.flush();

    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (pathItem) {
            count++;
            count+=pathItem->linkedItems_size();

            if (pathItem->linkedItems_size() > 0) {
                bool found=false;
                long unsigned int j=0;
                while (j < pathItem->linkedItems_size()) {
                    BaseItem *linkedItem=pathItem->get_linkedItem(j);
                    if (linkedItem && linkedItem->is_integrationPathSegment()) {found=true; break;}
                    j++;
                }
                if (!found) return false;
            }
        }
        i++;
    }

    if (count > 0 && ui->drawingWindow->get_selectedItems_count() == count) return true;
    return false;
}

void OpenParEMg::reversePathItems ()
{
    itemChangesStack.startNew();

    // make a list of items to reverse
    std::vector<PathItem *> changeList;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(ui->drawingWindow->get_selectedItem(i));
        if (pathItem) {
            if (pathItem) changeList.push_back(pathItem);
        }
        i++;
    }

    // make the changes
    i=0;
    while (i < changeList.size()) {
        changeList[i]->reverse();
        i++;
    }

    finishOperation(false,1);
}

void OpenParEMg::createDiffPairItem ()
{
    //std::cout << "OpenParEMg::createDiffPairIte" << std::endl; std::cout.flush();

    itemChangesStack.startNew();

    // make a list of mode items to process
    std::vector<ModeItem *> modeList;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        ModeItem *modeItem=dynamic_cast<ModeItem *>(ui->drawingWindow->get_selectedItem(i));
        if (modeItem) modeList.push_back(modeItem);
        i++;
    }

    // should only be 2
    if (modeList.size() != 2) return;

    PortItem *parentPortItem=dynamic_cast<PortItem *>(modeList[0]->getParentItem());

    // create the diffpair
    DiffPairItem *newDiffPairItem=new DiffPairItem(this,parentPortItem,modeList[0],modeList[1]);
    parentPortItem->addChild(newDiffPairItem);
    newDiffPairItem->demoteChildren();

    // disable the impedance calculation selection
    newDiffPairItem->enableZcalcControl(false);

    itemChangesStack.add(newDiffPairItem);

    finishOperation(false,1);
}

bool OpenParEMg::isValidCreateDiffPair ()
{
    QList<QTreeWidgetItem *> items=ui->drawingItemTree->selectedItems();

    // must only have 2 selected items
    if (items.count() != 2) return false;

    std::vector<ModeItem *> modeList;
    int i=0;
    while (i < items.count()) {
        ModeItem *modeItem=dynamic_cast<ModeItem *>(items[i]);
        if (modeItem) modeList.push_back(modeItem);
        i++;
    }

    // must have only two
    if (modeList.size() != 2) return false;

    // must have the same PortItem parent
    if (modeList[0]->getParentItem() != modeList[1]->getParentItem()) return false;

    // parent must be port
    PortItem *portItem=dynamic_cast<PortItem *>(modeList[0]->getParentItem());
    if (!portItem) return false;

    // port must use line calculation
    ShapeData *shapeData=portItem->getShapeData();
    if (shapeData->get_impedance_calculation().compare("line") != 0) return false;

    return true;
}

void OpenParEMg::renumberDimTag ()
{
    //std::cout << "OpenParEMg::renumberDimTag" << std::endl; std::cout.flush();

    int count=1;
    int i=0;
    while (i < drawing->childCount()) {
        DrawingItem *child=dynamic_cast<DrawingItem *>(drawing->child(i));

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

void OpenParEMg::setMaterials ()
{
    //std::cout << "OpenParEMg::setMaterials" << std::endl; std::cout.flush();

    int i=0;
    while (i < drawing->childCount()) {
        DrawingItem *child=dynamic_cast<DrawingItem *>(drawing->child(i));

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
    std::cout << "OpenParEMg::assignMaterial" << std::endl; std::cout.flush();

    // Cannot assign material to existing mesh
    if (mesh->childCount() > 0) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,"OpenParEMg","Materials cannot be assigned to an existing mesh.  Do you want to delete the mesh?",QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
        deleteMesh(true);
    }

    MaterialSelection *materialSelection=new MaterialSelection();
    materialSelection->set_materialDatabase(materialDatabase);
    materialSelection->set_selectedMaterial(&selectedMaterial);
    materialSelection->populate("dielectric");
    materialSelection->exec();
    delete materialSelection;

    if (selectedMaterial != "") {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(clickedItem);
        if (drawingItem && drawingItem->is_drawing()) {
            drawingItem->set_Material(selectedMaterial);
            drawingItem->setText(1,selectedMaterial);
            drawingItem->setModified(true);
        }
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

void OpenParEMg::clonePathData ()
{
    int i=0;
    while (i < path->childCount()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(path->child(i));
        if (pathItem) {
            Path *oldPath=pathItem->getPath();
            if (oldPath) {
                Path *newPath=oldPath->clone();
                if (newPath) {
                    pathItem->setPath(newPath);
                }
            }
        }
        i++;
    }
}

void OpenParEMg::on_actionOpen_triggered ()
{
    QString testProjectFile=QFileDialog::getOpenFileName(this,tr("Open Project"),absolutePath,tr("Project Files (*.proj);;All Files (*)"),
                                                         nullptr,QFileDialog::DontUseNativeDialog);

    // return if user cancels
    if (testProjectFile.isNull()) return;

    // reset as new
    on_actionNew_triggered ();

    // set the window title bar
    QString title="OpenParEMg: ";
    title.append(testProjectFile);
    setWindowTitle(title);

    // break up the full path
    QFileInfo fileInfo(testProjectFile);
    absolutePath=fileInfo.absolutePath();
    projectFile=fileInfo.fileName();
    projectName=fileInfo.completeBaseName();

    QDir::setCurrent(absolutePath);

    QString currentPath;
    currentPath=QDir::currentPath();

    BoundaryDatabase boundaryDatabase;

    // load the file
    if (QFile::exists(projectFile)) {

        int retVal=0;

        // ignore errors so that users can save incomplete project for safety and later work
        int tempRetVal=load_project_file (projectFile.toStdString().c_str(),&projData,"   ");
        if (tempRetVal) retVal=1;

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

                retVal=1;
            } else materialsLoaded=true;
        }

        // load boundaries, if any, and draw
        if (strcmp(projData.port_definition_file,"") != 0) {
            if (boundaryDatabase.load(projData.port_definition_file,projData.solution_check_closed_loop)) {
                QMessageBox mb;
                mb.critical(nullptr, "Error", "Paths, ports, and/or boundaries are not complete and require correction.");
                mb.setFixedSize(500, 200);

                retVal=1;
            }

            // continue despite any errors

            boundaryDatabase.draw(relay,&projData,
                                  this,ui->drawingWindow,ui->drawingItemTree,
                                  path,port,boundary,materialDatabase);

            // add paths to the selection map and activate
            int i=0;
            while (i < path->childCount()) {
                PathItem *pathItem=dynamic_cast<PathItem *>(path->child(i));
                if (pathItem) {
                    ui->drawingWindow->insertItemToMap(pathItem->getShape(),pathItem);
                    ui->drawingWindow->activateItem(pathItem);
                }
                i++;
            }
        }

        // load drawing
        bool drawingLoaded=false;
        if (loadDrawingFile()) {
            retVal=1;
        } else {
            drawingLoaded=true;
        }

        // set data for meshing
        if (drawingLoaded && materialsLoaded) {
            renumberDimTag();
            setMaterials();
        }

        // load mesh, if any, and draw
        if (strcmp(projData.mesh_file,"") != 0) {
            loadMeshFile(QString::fromStdString(projData.mesh_file));
        }

        // load last results to the tabs, if any
        if (retVal) {
            ui->logText->clear();
            ui->iterationsText->clear();
            ui->dataText->clear();
        } else {
            updateLogTab(true);
            updateIterationsTab(true);
            updateDataTab(true);
            updateAntennaTab(true);
        }

        ui->drawingWindow->fitAll();
        ui->drawingWindow->updateViewer();

        if (retVal) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "There were errors on loading the project.");
            mb.setFixedSize(500, 200);
        }

        scaleFormDefaults();

        projData.modified=0;
        projectFileLoaded=true;
        setUnmodified();
    } else {
        // should not occur
        QMessageBox mb;
        mb.critical(nullptr, "Error", "The requested project file does not exist.");
        mb.setFixedSize(500, 200);
    }

    // ensure the ports and boundaries are only defined by single paths.  This is a restriction for a safer GUI.
    if (boundaryDatabase.has_complex_path()) {
        QMessageBox mb;
        mb.critical(nullptr, "Warning", "One or more ports or boundaries have a definition using more than one path.");
        mb.setFixedSize(500, 200);
    }

    // boundaryDatabase is local and will be deleted on exit from this function.
    // The Path data continues to be used, so the Path data must be made permanent.
    clonePathData();

    on_actionShape_triggered();  // ToDo: see if this is still required
    clearTreeSelection();
    setMenusI(39);
}

void OpenParEMg::resetLockouts ()
{
    disableMenus=false;
    projectFileLoaded=false;
    setUnmodified();
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
              << "   drawingChanged=" << drawing->isModified() << std::endl
              << "   drawingChangedSinceMeshRegen=" << drawing->isModifiedSinceMeshRegen() << std::endl
              << "   pathChanged=" << path->isModified() << std::endl
              << "   portChanged=" << port->isModified() << std::endl
              << "   boundaryChanged=" << boundary->isModified() << std::endl
              << "   meshChanged=" << mesh->isModified() << std::endl
              << "   drawingPlaneShown=" << drawingPlaneShown << std::endl
              << "   simulationRunning=" << simulationRunning << std::endl
              << "   simulationStopping=" << simulationStopping << std::endl
              << "   simulationAborting=" << simulationAborting << std::endl;
}

void OpenParEMg::resetDrawing ()
{
    //std::cout << "OpenParEMg::resetDrawing  drawing=" << &drawing << std::endl; std::cout.flush();

    // mesh
    deleteMesh(false);

    // reset drawing window
    ui->drawingWindow->clearDrawing();
    ui->drawingWindow->updateViewer();

    // selection tree
    drawing->reset();

    // drawing is always a COMPOUND
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    Handle(AIS_Shape) newShape=new AIS_Shape(compound);

    ShapeData *newShapeData=drawing->getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->setShape(newShape);
    drawing->addShapeData(newShapeData);
    drawing->setModified(false);

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

    // form defaults
    uLocalAxis.SetCoord(1,0,0);  // rectangles
    length=1;                    // extrusion
    angle=90;                    // rotation
    startPoint.SetCoord(0,0,0);  // rotation and vector input
    endPoint.SetCoord(0,0,1);    // rotation and vector input

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
    projectFileLoaded=false;

    resetDrawing();

    // reset material database
    if (materialDatabase) delete materialDatabase;
    materialDatabase=new MaterialDatabase();

    // reset selection tree
    //drawing.reset();  // reset in resetDrawing() above
    path->reset();
    port->reset();
    boundary->reset();
    mesh->reset();

    sportNumbers.clear();

    // clear tabs

    ui->logText->clear();
    ui->iterationsText->clear();
    ui->dataText->clear();
    ui->antennaText->clear();

    logLastPos=0;
    iterationLastPos=0;
    dataLastPos=0;
    antennaLastPos=0;

    logLastChar='\0';
    iterationsLastChar='\0';
    dataLastChar='\0';
    antennaLastChar='\0';

    // bring drawing to the front
    if (ui->tabs->currentWidget() != ui->drawingTab) {
        ui->tabs->setCurrentWidget(ui->drawingTab);
    }

    resetLockouts();

    setMenusI(41);
}

void OpenParEMg::scaleFormDefaults ()
{
    //uLocalAxis.SetCoord(1,0,0);                        // rectangles
    length=1/getConversionFactor();                    // extrusion
    angle=90;                                          // rotation
    startPoint.SetCoord(0,0,0);                        // rotation and vector input
    endPoint.SetCoord(0,0,1/getConversionFactor());    // rotation and vector input
}

// set the scale in the drawing window to the size of the drawing plane
void OpenParEMg::setScale ()
{
    // Note that using setScale method of QOpenGLWidget causes jitter until a fitAll command is issued,
    // so don't do it this way.
    //ui->drawingWindow->setScale(getConversionFactor()*100);  // <<-- jittery

    // draw a line, call fitAll, then delete the line

    double size=1/getConversionFactor()*projData.gui_grid_size/3;

    gp_Pnt p1(-size,-size,0);
    gp_Pnt p2(size,size,0);
    Handle(Geom_TrimmedCurve) lineGeom=GC_MakeSegment(p1,p2);
    TopoDS_Edge lineEdge=BRepBuilderAPI_MakeEdge(lineGeom);
    Handle(AIS_Shape) line=new AIS_Shape(lineEdge);
    ui->drawingWindow->displayShape(line);
    ui->drawingWindow->fitAll();
    ui->drawingWindow->removeShape(line);
    line.Nullify();
    ui->drawingWindow->updateViewer();
}

void OpenParEMg::onMenuAboutToShow ()
{
    // bring drawing to the front
    if (ui->tabs->currentWidget() != ui->drawingTab) {
        ui->tabs->setCurrentWidget(ui->drawingTab);
    }
}

void OpenParEMg::on_actionNew_triggered ()
{
    resetProject();
    init_project (&defaultData);
    init_project (&projData);

    setScale();
    scaleFormDefaults();

    projData.modified=0;
    projectFileLoaded=true;
    projectChanged=false;
    setMenusI(42);
}

void OpenParEMg::on_actionClose_triggered()
{
    //std::cout << "OpenParEMg::on_actionClose_triggered" << std::endl; std::cout.flush();

    int retVal=check_changed();
    if (retVal) {
        if (retVal == QMessageBox::Save) {
            on_actionSave_triggered();
        } else if (retVal == QMessageBox::Discard) {
            // do nothing
        } else if (retVal == QMessageBox::Cancel) {
            return;
        }
    }

    // set the window title bar
    setWindowTitle("OpenParEMg");

    resetProject();
}

void OpenParEMg::on_actionMeshOptions_triggered ()
{
    MeshDialog *meshDialog=new MeshDialog();
    meshDialog->set_conversionFactor(getConversionFactor());
    meshDialog->set_simulationRunning(simulationRunning);
    meshDialog->set_projData(&projData);
    meshDialog->set_meshObsolete(&meshObsolete);
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

void OpenParEMg::on_actionAntennaPatterns_triggered ()
{
    AntennaForm *antennaForm=new AntennaForm();
    antennaForm->set_simulationRunning(simulationRunning);
    antennaForm->set_projData(&projData);
    antennaForm->exec();
    delete antennaForm;

    if (projData.modified) {
        projectChanged=true;
    }
    setMenusI(46);
}

void OpenParEMg::saveProject ()
{
    // adjust the project file for antennas
    if (hasRadiationBoundary()) {
        // must have at least 1 pattern with gain
        // assumes that if there are patterns, that one is gain, which is enforced by the antenna pattern form
        if (projData.inputAntennaPatternsCount == 0) {
            char *quantity1=(char *)malloc(2*sizeof(char));
            quantity1[0]='G';
            quantity1[1]='\0';
            add_antennaPattern(&projData,0,3,quantity1,nullptr,nullptr,0,0,0,0);
            if (quantity1) free(quantity1);
        }
    } else {
        // cannot have patterns
        if (projData.inputAntennaPatterns) {
           long unsigned int i=0;
           while (i < projData.inputAntennaPatternsAllocated) {
               if (projData.inputAntennaPatterns[i].quantity1) {free(projData.inputAntennaPatterns[i].quantity1); projData.inputAntennaPatterns[i].quantity1=NULL;}
               if (projData.inputAntennaPatterns[i].quantity2) {free(projData.inputAntennaPatterns[i].quantity2); projData.inputAntennaPatterns[i].quantity2=NULL;}
               if (projData.inputAntennaPatterns[i].plane) {free(projData.inputAntennaPatterns[i].plane); projData.inputAntennaPatterns[i].plane=NULL;}
              i++;
           }
           free(projData.inputAntennaPatterns); projData.inputAntennaPatterns=NULL;
        }
        projData.inputAntennaPatternsCount=0;
        projData.inputAntennaPatternsAllocated=0;
    }

    // update included file names

    // port_definition_file
    QString portDefinitionFile=projectName;
    portDefinitionFile.append("_ports.txt");
    if (portDefinitionFile.compare(projData.port_definition_file) != 0) projectChanged=true;
    cstrFromQString (&(projData.port_definition_file),portDefinitionFile);
    std::cout << "projData.port_definition_file=" << projData.port_definition_file << std::endl; std::cout.flush();

    // mesh_file
    if (mesh->childCount() > 0) {
        if (strcmp(projData.mesh_file,"") == 0) {
            QString meshFile=projectName;
            meshFile.append(".msh");
            if (meshFile.compare(projData.mesh_file) != 0) projectChanged=true;
            cstrFromQString (&(projData.mesh_file),meshFile);
        }
        std::cout << "projData.mesh_file=" << projData.mesh_file << std::endl; std::cout.flush();
    }

    // save files

    // project
    if (save_project(projectFile.toStdString().c_str(),&projData,&defaultData,"")) {
        QString message="Error in saving the project file.";
        QMessageBox mb;
        mb.critical(nullptr,"Error",message);
        mb.setFixedSize(500, 200);
    } else {
        std::cout << "Saved project file" << std::endl; std::cout.flush();
        projData.modified=0;
        projectChanged=false;
    }

    // drawing
    QString drawingFile=projectName;
    drawingFile.append(".opd");
    if (saveDrawingFile(drawingFile)) {
        std::cout << "Saved drawing file" << std::endl; std::cout.flush();
    }

    // ports and boundaries
    if (saveBoundaryDatabase()) {
        QString message="Error in saving the boundary database.";
        QMessageBox mb;
        mb.critical(nullptr, "Error",message);
        mb.setFixedSize(500, 200);
    } else {
        std::cout << "Saved boundary database file" << std::endl; std::cout.flush();
    }

    // mesh
    if (mesh->childCount() > 0) {
        std::cout << "Saved mesh file" << std::endl; std::cout.flush();
        on_actionMeshSave_triggered();
    }

    setMenusI(47);
}

void OpenParEMg::on_actionSave_triggered ()
{
    std::cout << "projectFile=" << projectFile.toStdString() << std::endl; std::cout.flush();
    if (QFile::exists(projectFile)) {
        std::cout << "file exists" << std::endl; std::cout.flush();

        // check for existing data
        if (hasResults()) {
            int retVal=0;
            QMessageBox msgBox(this);
            msgBox.setText("The project has existing computed results.");
            msgBox.setInformativeText("Do you want to permanently delete the existing results?");
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            retVal=msgBox.exec();

            if (retVal == QMessageBox::Cancel) return;

            // bring drawing to the front
            if (ui->tabs->currentWidget() != ui->drawingTab) {
                ui->tabs->setCurrentWidget(ui->drawingTab);
            }

            // clear stale data
            ui->logText->clear();
            ui->iterationsText->clear();
            ui->dataText->clear();
            delete_stale_files(projData.project_name,port->get_SportCount());
        }

        // check for obsolete mesh
        if (meshObsolete && mesh->childCount() > 0) {
            int retVal=0;
            QMessageBox msgBox(this);
            msgBox.setText("The mesh is obsolete due to changes to the meshing criteria.");
            msgBox.setInformativeText("Do you want to permanently delete the existing mesh?");
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            retVal=msgBox.exec();

            if (retVal == QMessageBox::Cancel) return;

            deleteMesh(true);
        }

        // check for drawing changes
        if (drawing->isModifiedSinceMeshRegen() && mesh->childCount() > 0) {
            int retVal=0;
            QMessageBox msgBox(this);
            msgBox.setText("The mesh is obsolete due to changes to the drawing.");
            msgBox.setInformativeText("Do you want to permanently delete the existing mesh?");
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            retVal=msgBox.exec();

            if (retVal == QMessageBox::Cancel) return;

            deleteMesh(true);
        }

        saveProject();
    } else {
        std::cout << "file does not exist" << std::endl; std::cout.flush();
        on_actionSaveAs_triggered();
    }
}

void OpenParEMg::on_actionSaveAs_triggered ()
{
    QString filePath=QFileDialog::getSaveFileName(this,tr("Save Project"),absolutePath,tr("Project Files (*.proj)","All Files (*)"),
                                                  nullptr,QFileDialog::DontUseNativeDialog);
    if (filePath.isEmpty()) return;

    // set the window title bar
    QString title="OpenParEMg: ";
    title.append(filePath);
    setWindowTitle(title);

    QFileInfo fileInfo(filePath);

    // see if this is being saved to a new project
    bool isNewProject=false;
    if (absolutePath.compare(fileInfo.absolutePath()) != 0) isNewProject=true;
    if (projectName.compare(fileInfo.completeBaseName()) != 0) isNewProject=true;

    // assign data for this (possibly) new project
    absolutePath=fileInfo.absolutePath();
    projectFile=fileInfo.fileName();
    projectName=fileInfo.completeBaseName();
    set_project_name(&projData,projectName.toStdString().c_str());
    QDir::setCurrent(absolutePath);

    // delete stale results if this is the existing project
    // otherwise, the old data is not copied to the new project
    if (!isNewProject && hasResults()) {
        int retVal=0;
        QMessageBox msgBox(this);
        msgBox.setText("The project has existing computed results.");
        msgBox.setInformativeText("Do you want to permanently delete the existing results?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        retVal=msgBox.exec();

        if (retVal == QMessageBox::Cancel) return;
    }

    // bring drawing to the front
    if (ui->tabs->currentWidget() != ui->drawingTab) {
        ui->tabs->setCurrentWidget(ui->drawingTab);
    }

    // clear stale data
    ui->logText->clear();
    ui->iterationsText->clear();
    ui->dataText->clear();
    delete_stale_files(projData.project_name,port->get_SportCount());

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

void OpenParEMg::on_actionMaterials_triggered  ()
{
    //std::cout << "OpenParEMg::on_actionMaterialsEditor_triggered" << std::endl; std::cout.flush();

    Materials *localMaterials=new Materials();
    localMaterials->set_projData(&projData);
    localMaterials->setMaterialDatabase(&materialDatabase);
    localMaterials->setAbsolutePath(absolutePath);
    localMaterials->exec();
    delete localMaterials;

    boundary->fillMaterialWidgets();

    if (projData.modified) {
        projectChanged=true;
    }
    setMenusI(49);
}


// int CountSubShapes(const TopoDS_Shape& shape, TopAbs_ShapeEnum type)
// {
//     int count = 0;

//     for (TopExp_Explorer exp(shape, type); exp.More(); exp.Next()) {
//         ++count;
//     }

//     return count;
// }

// void ListChildren (const TopoDS_Shape& theShape)
// {
//     std::cout << TopAbs::ShapeTypeToString(theShape.ShapeType()) << std::endl;

//     // 3 levels of children
//     TopoDS_Iterator anIterator(theShape);
//     for (; anIterator.More(); anIterator.Next()) {
//         const TopoDS_Shape& aChildShape = anIterator.Value();
//         std::cout << "   " << TopAbs::ShapeTypeToString(aChildShape.ShapeType()) << std::endl;

//         TopoDS_Iterator anIterator2(aChildShape);
//         for (; anIterator2.More(); anIterator2.Next()) {
//             const TopoDS_Shape& aChildShape2 = anIterator2.Value();
//             std::cout << "      " << TopAbs::ShapeTypeToString(aChildShape2.ShapeType()) << std::endl;

//             TopoDS_Iterator anIterator3(aChildShape2);
//             for (; anIterator3.More(); anIterator3.Next()) {
//                 const TopoDS_Shape& aChildShape3 = anIterator3.Value();
//                 std::cout << "         " << TopAbs::ShapeTypeToString(aChildShape3.ShapeType()) << std::endl;
//             }
//         }
//     }
//     // std::cout << std::endl;

//     // std::cout << "Faces using TopExp_Explorer:" << std::endl;
//     // for (TopExp_Explorer anExplorer(theShape, TopAbs_FACE); anExplorer.More(); anExplorer.Next()) {
//     //     const TopoDS_Face& aFace = TopoDS::Face(anExplorer.Current());
//     //     std::cout << "  Found a Face" << std::endl;
//     // }

//     // std::cout << "Solid using TopExp_Explorer:" << std::endl;
//     // for (TopExp_Explorer anExplorer(theShape, TopAbs_SOLID); anExplorer.More(); anExplorer.Next()) {
//     //     const TopoDS_Solid& aSolid = TopoDS::Solid(anExplorer.Current());
//     //     std::cout << "  Found a Solid" << std::endl;
//     // }

//     // std::cout << "Wire using TopExp_Explorer:" << std::endl;
//     // for (TopExp_Explorer anExplorer(theShape, TopAbs_WIRE); anExplorer.More(); anExplorer.Next()) {
//     //     const TopoDS_Wire& aWire = TopoDS::Wire(anExplorer.Current());
//     //     std::cout << "  Found a Wire" << std::endl;
//     // }

//     // std::cout << "CompSolid using TopExp_Explorer:" << std::endl;
//     // for (TopExp_Explorer anExplorer(theShape, TopAbs_COMPSOLID); anExplorer.More(); anExplorer.Next()) {
//     //     const TopoDS_CompSolid& aCompSolid = TopoDS::CompSolid(anExplorer.Current());
//     //     std::cout << "  Found a CompSolid" << std::endl;
//     // }

//     // std::cout << "Compound using TopExp_Explorer:" << std::endl;
//     // for (TopExp_Explorer anExplorer(theShape, TopAbs_COMPOUND); anExplorer.More(); anExplorer.Next()) {
//     //     const TopoDS_Compound& aCompound = TopoDS::Compound(anExplorer.Current());
//     //     std::cout << "  Found a Compound" << std::endl;
//     // }

//     // std::cout << "Edge using TopExp_Explorer:" << std::endl;
//     // for (TopExp_Explorer anExplorer(theShape, TopAbs_EDGE); anExplorer.More(); anExplorer.Next()) {
//     //     const TopoDS_Edge& aEdge = TopoDS::Edge(anExplorer.Current());
//     //     std::cout << "  Found a Edge" << std::endl;
//     // }

//     // std::cout << "Shell using TopExp_Explorer:" << std::endl;
//     // for (TopExp_Explorer anExplorer(theShape, TopAbs_SHELL); anExplorer.More(); anExplorer.Next()) {
//     //     const TopoDS_Shell& aShell = TopoDS::Shell(anExplorer.Current());
//     //     std::cout << "  Found a Shell" << std::endl;
//     // }

//     // std::cout << "Vertex using TopExp_Explorer:" << std::endl;
//     // for (TopExp_Explorer anExplorer(theShape, TopAbs_VERTEX); anExplorer.More(); anExplorer.Next()) {
//     //     const TopoDS_Vertex& aVertex = TopoDS::Vertex(anExplorer.Current());
//     //     std::cout << "  Found a Vertex" << std::endl;
//     // }
// }

// void OpenParEMg::shapeCount (TopoDS_Shape shape, int *count)
// {
//     TopoDS_Iterator topoIterator(shape);

//     while (topoIterator.More()) {
//         const TopoDS_Shape& child=topoIterator.Value();
//         (*count)++;
//         shapeCount(child,count);
//         topoIterator.Next();
//     }
// }

// void OpenParEMg::insertToMapActivateItem (BaseItem *baseItem)
// {
//     //std::cout << "OpenParEMg::addItemWithShape" << std::endl; std::cout.flush();

//     if (!baseItem) return;

//     Handle(AIS_Shape) drawingShape=baseItem->getShape();
//     ui->drawingWindow->insertItemToMap(drawingShape,baseItem);

//     baseItem->setForeground(0,Qt::gray);
//     ui->drawingWindow->showItem(baseItem);
//     ui->drawingWindow->selectItem(baseItem);
// }

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

QString OpenParEMg::getAISshapeName (Handle(AIS_Shape) shape)
{
    QString name;
    int count=0;
    if (shape.IsNull()) return name;

    if (shape->Shape().ShapeType() == TopAbs_COMPOUND) {name="COMPOUND"; objectCounts.compound++; count=objectCounts.compound;}
    else if (shape->Shape().ShapeType() == TopAbs_COMPSOLID) {name="COMPSOLID"; objectCounts.compsolid++; count=objectCounts.compsolid;}
    else if (shape->Shape().ShapeType() == TopAbs_SHELL) {name="SHELL"; objectCounts.shell++; count=objectCounts.shell;}
    else if (shape->Shape().ShapeType() == TopAbs_SOLID) {name="SOLID"; objectCounts.solid++; count=objectCounts.solid;}
    else if (shape->Shape().ShapeType() == TopAbs_FACE) {name="FACE"; objectCounts.face++; count=objectCounts.face;}
    else if (shape->Shape().ShapeType() == TopAbs_WIRE) {name="WIRE"; objectCounts.wire++; count=objectCounts.wire;}
    else if (shape->Shape().ShapeType() == TopAbs_EDGE) {name="EDGE"; objectCounts.edge++; count=objectCounts.edge;}
    else if (shape->Shape().ShapeType() == TopAbs_VERTEX) {name="VERTEX"; objectCounts.vertex++; count=objectCounts.vertex;}
    else {name="UNKNOWN"; objectCounts.unknown++; count=objectCounts.unknown;}

    name.append(QString::number(count));
    return name;
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
            if (!normalized.IsNull()) {
                Handle(AIS_Shape) shape=new AIS_Shape(normalized);
                if (!shape.IsNull()) {
                    DrawingItem *newItem=new DrawingItem(this,drawing);
                    ShapeData *shapeData=newItem->getShapeData();
                    shapeData->setShape(shape);
                    newItem->setText(0,getAISshapeName(shape));
                    shapeData->set_name(newItem->text(0));
                    drawing->addChild(newItem);
                    ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);
                    ui->drawingWindow->showItem(newItem);
                    ui->drawingWindow->selectItem(newItem);
                    itemChangesStack.startNew();
                    itemChangesStack.add(newItem);

                    // put it on the Z-layer to get it higher selection priority
                    newItem->getShape()->SetZLayer(Graphic3d_ZLayerId_Top);
                }
            }

            //  ??? needed
            //showRootDrawingItems();

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
            if (!normalized.IsNull()) {
                Handle(AIS_Shape) shape=new AIS_Shape(normalized);
                if (!shape.IsNull()) {
                    DrawingItem *newItem=new DrawingItem(this,drawing);
                    ShapeData *shapeData=newItem->getShapeData();
                    shapeData->setShape(shape);
                    newItem->setText(0,getAISshapeName(shape));
                    shapeData->set_name(newItem->text(0));
                    drawing->addChild(newItem);
                    ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);
                    ui->drawingWindow->showItem(newItem);
                    ui->drawingWindow->selectItem(newItem);
                    itemChangesStack.startNew();
                    itemChangesStack.add(newItem);

                    // put it on the Z-layer to get it higher selection priority
                    newItem->getShape()->SetZLayer(Graphic3d_ZLayerId_Top);
                }
            }

            // ??? needed
            //showRootDrawingItems();

        } else retval=true;
    }
    setMenusI(51);
    return retval;
}

// BaseItem* get_vertexItem (BaseItem *baseItem)
// {
//     if (!baseItem) return nullptr;;

//     Handle(AIS_Shape) shape=baseItem->getShape();
//     if (shape.IsNull()) return nullptr;
//     if (shape->Shape().ShapeType() == TopAbs_VERTEX) return baseItem;

//     int i=0;
//     while (i < baseItem->childCount()) {
//         BaseItem *child=dynamic_cast<BaseItem *>(baseItem->child(i));
//         if (get_vertexItem(child)) return child;
//         i++;
//     }

//     return nullptr;
// }

bool OpenParEMg::isValidSaveBrepFile ()
{
    int count=0;
    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (rootDrawingItem && rootDrawingItem->is_rootDrawing()) {
            Handle(AIS_Shape) shape=rootDrawingItem->getShape();
            if (!shape.IsNull()) {
                count++;
            }
        }

        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Handle(AIS_Shape) shape=drawingItem->getShape();
            if (!shape.IsNull()) {
                count++;
            }
        }
        i++;
    }
    if (count == 1 && ui->drawingWindow->get_selectedItems_count() == count) return true;
    return false;
}

// return true on error
bool OpenParEMg::saveBrepFile (QString filePath)
{
    std::cout << "OpenParEMg::saveBrepFile" << std::endl; std::cout.flush();

    if (filePath.isEmpty()) return true;

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (rootDrawingItem) {
            Handle(AIS_Shape) shape=rootDrawingItem->getShape();
            if (!shape.IsNull()) {
                if (BRepTools::Write(shape->Shape(),filePath.toStdString().c_str())) return false;
            }
        }

        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Handle(AIS_Shape) shape=drawingItem->getShape();
            if (!shape.IsNull()) {
                if (BRepTools::Write(shape->Shape(),filePath.toStdString().c_str())) return false;
            }
        }
        i++;
    }

    return true;
}

bool OpenParEMg::isValidSaveStepFile ()
{
    return isValidSaveBrepFile();
}

bool OpenParEMg::saveStepFile (QString filePath)
{
    //std::cout << "OpenParEMg::saveStepFile" << std::endl; std::cout.flush();

    if (filePath.isEmpty()) return true;

    long unsigned int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (rootDrawingItem) {
            Handle(AIS_Shape) shape=rootDrawingItem->getShape();
            if (!shape.IsNull()) {
                STEPControl_Writer writer;
                writer.Transfer(shape->Shape(),STEPControl_ManifoldSolidBrep,Standard_True);

                IFSelect_ReturnStatus status=writer.Write(filePath.toStdString().c_str());
                if (status == IFSelect_RetDone) {
                    return false;
                }
            }
        }

        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {
            Handle(AIS_Shape) shape=drawingItem->getShape();
            if (!shape.IsNull()) {
                STEPControl_Writer writer;
                writer.Transfer(shape->Shape(),STEPControl_ManifoldSolidBrep,Standard_True);

                IFSelect_ReturnStatus status=writer.Write(filePath.toStdString().c_str());
                if (status == IFSelect_RetDone) {
                    return false;
                }
            }
        }
        i++;
    }

    return true;
}

bool OpenParEMg::saveBoundaryDatabase ()
{
    QString filename=absolutePath;
    filename.append("/").append(projData.port_definition_file);

    std::ofstream outputFile(filename.toStdString());
    if (outputFile.is_open()) {

        // header

        outputFile << "#OpenParEMports 1.0" << std::endl;
        outputFile << std::endl;

        outputFile << "File" << std::endl;
        outputFile << "   name=generated_by_OpenParEMg" << std::endl;
        outputFile << "EndFile" << std::endl;
        outputFile << std::endl;

        // paths
        int i=0;
        while (i < path->childCount()) {
            PathItem *pathItem=dynamic_cast<PathItem *>(path->child(i));
            if (pathItem) pathItem->save(&outputFile);
            i++;
        }
        path->setModified(false);

        // boundaries
        i=0;
        while (i < boundary->childCount()) {
            BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(boundary->child(i));
            if (boundaryItem) boundaryItem->save(&outputFile);
            i++;
        }
        boundary->setModified(false);

        // ports
        i=0;
        while (i < port->childCount()) {
            PortItem *portItem=dynamic_cast<PortItem *>(port->child(i));
            if (portItem) portItem->save(&outputFile);
            i++;
        }
        port->setModified(false);

        outputFile.close();

        return false;
    }
    return true;
}

bool OpenParEMg::hasRadiationBoundary ()
{
    int i=0;
    while (i < boundary->childCount()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(boundary->child(i));
        if (boundaryItem) {
            ShapeData *shapeData=boundaryItem->getShapeData();
            if (shapeData->get_boundary_type() == 3) return true;
        }
        i++;
    }
    return false;
}

void OpenParEMg::increase_depth (DrawingItem *item)
{
    if (!item) return;

    item->increase_depth();

    int i=0;
    while (i < item->childCount()) {
        DrawingItem *child=dynamic_cast<DrawingItem *>(item->child(i));
        if (child) increase_depth(child);
        i++;
    }
}

// void OpenParEMg::decrease_depth (DrawingItem *item)
// {
//     if (!item) return;

//     item->decrease_depth();

//     int i=0;
//     while (i < item->childCount()) {
//         DrawingItem *child=dynamic_cast<DrawingItem *>(item->child(i));
//         if (child) decrease_depth(child);
//         i++;
//     }
// }

void OpenParEMg::saveItem (std::ofstream *out, BaseItem *baseItem)
{
    if (!baseItem) return;

    std::cout << "OpenParEMg::saveItem  item=" << baseItem->text(0).toStdString() << std::endl; std::cout.flush();

    RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(baseItem);
    if (rootDrawingItem) {
        std::cout << "   rootDrawingItem" << std::endl; std::cout.flush();
        int i=0;
        while (i < rootDrawingItem->childCount()) {
            DrawingItem *child=(DrawingItem *) rootDrawingItem->child(i);
            saveItem(out,child);
            i++;
        }
        rootDrawingItem->setModified(false);
    }

    DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(baseItem);
    if (drawingItem && drawingItem->is_drawing()) {

        Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());
        if (polywire) {
            polywire->save(out,drawingItem->text(0),drawingItem->get_depth());
        }

        Process *process=static_cast<Process *>(drawingItem->getProcess());
        if (process) {
            process->startSave(out,drawingItem->text(0),drawingItem->get_material(),drawingItem->get_depth());

            int i=0;
            while (i < drawingItem->childCount()) {
                DrawingItem *child=(DrawingItem *) drawingItem->child(i);
                saveItem(out,child);
                i++;
            }

            process->endSave(out,drawingItem->get_depth());
        }

        // Brep
        if (!polywire && !process) {
            std::string space;
            long unsigned int i=0;
            while (i < drawingItem->get_depth()) {
                space.append("   ");
                i++;
            }

            *out << space << "BRep" << std::endl;
            if (!drawingItem->text(0).isEmpty()) {
                *out << space << "   name=" << drawingItem->text(0).toStdString() << std::endl;
            }
            if (!drawingItem->get_material().isEmpty()) {
                *out << space << "   material=" << drawingItem->get_material().toStdString() << std::endl;
            }

            // uses TopTools_FormatVersion_VERSION_1
            BRepTools::Write(drawingItem->getShape()->Shape(),*out);

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
                           long unsigned int &endBlockIndex, BaseItem *baseParent, bool increaseDepth)
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
        DrawingItem *newDrawingItem=new DrawingItem(this,baseParent);
        ShapeData *shapeData=newDrawingItem->getShapeData();
        shapeData->setPolywire(polywire);
        newDrawingItem->setText(0,QString::fromStdString(name));
        shapeData->set_name(newDrawingItem->text(0));

        DrawingItem *parentItem=dynamic_cast<DrawingItem *>(baseParent);
        if (parentItem && parentItem->is_drawing()) newDrawingItem->copy_depth(parentItem);

        if (increaseDepth) newDrawingItem->increase_depth();
        baseParent->addChild(newDrawingItem);
        baseParent->push_child(newDrawingItem);
        reprocess(newDrawingItem);
        newDrawingItem->alignForegroundColor();
        ui->drawingWindow->hideItem(newDrawingItem);
        if (baseParent->is_rootDrawing()) ui->drawingWindow->showItem(newDrawingItem);
        startBlockIndex=endBlockIndex;
    }

    bool loadBrep=false;
    Process *process=nullptr;
    if (typeStart == 5) process=new Extrude();
    if (typeStart == 6) process=new Merge();
    if (typeStart == 7) process=new Subtract();
    if (typeStart == 8) loadBrep=true;

    if (process) {
        DrawingItem *newDrawingItem=new DrawingItem(this,baseParent);
        ShapeData *shapeData=newDrawingItem->getShapeData();
        shapeData->setProcess(process);
        baseParent->addChild(newDrawingItem);
        baseParent->push_child(newDrawingItem);

        // extrude
        if (typeStart == 5) {

            // name
            long unsigned int localStartBlockIndex=startBlockIndex+1;
            long unsigned int localEndBlockIndex=endBlockIndex-1;
            std::string keyword="name";
            std::string name;
            if (getBlockKeywordValue(inputData,typeStart,localStartBlockIndex,localEndBlockIndex,keyword,name)) {
                newDrawingItem->setText(0,QString::fromStdString(name));
                shapeData->set_name(newDrawingItem->text(0));
                DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(baseParent);
                if (drawingItem && drawingItem->is_drawing()) newDrawingItem->copy_depth(drawingItem);
                if (increaseDepth) newDrawingItem->increase_depth();
                objectCounts.extrude++;
            }

            // material
            localStartBlockIndex=startBlockIndex+1;
            localEndBlockIndex=endBlockIndex-1;
            keyword="material";
            std::string material;
            if (getBlockKeywordValue(inputData,typeStart,localStartBlockIndex,localEndBlockIndex,keyword,material)) {
                newDrawingItem->set_Material(QString::fromStdString(material));
                newDrawingItem->setText(1,QString::fromStdString(material));
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
            loadItem(inputData,localStartBlockIndex,localEndBlockIndex,newDrawingItem,true);

            reprocess(newDrawingItem);
            newDrawingItem->alignForegroundColor();
            ui->drawingWindow->hideItem(newDrawingItem);
            if (baseParent->is_rootDrawing()) ui->drawingWindow->showItem(newDrawingItem);
        }

        // merge and subtract
        if (typeStart == 6 || typeStart == 7) {

            // name
            long unsigned int localStartBlockIndex=startBlockIndex+1;
            long unsigned int localEndBlockIndex=endBlockIndex-1;
            std::string keyword="name";
            std::string name;
            if (getBlockKeywordValue(inputData,typeStart,localStartBlockIndex,localEndBlockIndex,keyword,name)) {
                newDrawingItem->setText(0,QString::fromStdString(name));
                DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(baseParent);
                if (drawingItem && drawingItem->is_drawing()) newDrawingItem->copy_depth(drawingItem);
                if (increaseDepth) newDrawingItem->increase_depth();
                if (typeStart == 6) objectCounts.merge++;
                if (typeStart == 7) objectCounts.subtract++;
            }

            // material
            localStartBlockIndex=startBlockIndex+1;
            localEndBlockIndex=endBlockIndex-1;
            keyword="material";
            std::string material;
            if (getBlockKeywordValue(inputData,typeStart,localStartBlockIndex,localEndBlockIndex,keyword,material)) {
                newDrawingItem->set_Material(QString::fromStdString(material));
                newDrawingItem->setText(1,QString::fromStdString(material));
            }

            // get two children

            localStartBlockIndex=startBlockIndex+1;
            localEndBlockIndex=startBlockIndex+1;
            loadItem(inputData,localStartBlockIndex,localEndBlockIndex,newDrawingItem,true);

            localStartBlockIndex=localEndBlockIndex+1;
            loadItem(inputData,localStartBlockIndex,localEndBlockIndex,newDrawingItem,true);

            reprocess(newDrawingItem);
            newDrawingItem->alignForegroundColor();
            ui->drawingWindow->hideItem(newDrawingItem);
            if (baseParent->is_rootDrawing()) ui->drawingWindow->showItem(newDrawingItem);
        }

        startBlockIndex=endBlockIndex;
    }

    if (loadBrep) {
        std::stringstream ss;

        long unsigned int i=startBlockIndex+2;
        while (i < endBlockIndex) {
            ss << inputData[i] << std::endl;
            i++;
        }

        TopoDS_Shape shape;
        BRep_Builder builder;
        BRepTools::Read(shape,ss,builder);

        if (!shape.IsNull()) {
            DrawingItem *newItem=new DrawingItem(this,baseParent);
            Handle(AIS_Shape) aisShape=new AIS_Shape(shape);
            ShapeData *shapeData=newItem->getShapeData();
            shapeData->setPolywire(polywire);
            shapeData->setShape(aisShape);

            // name
            long unsigned int localStartBlockIndex=startBlockIndex+1;
            long unsigned int localEndBlockIndex=endBlockIndex-1;
            std::string keyword="name";
            std::string name;
            if (getBlockKeywordValue(inputData,typeStart,localStartBlockIndex,localEndBlockIndex,keyword,name)) {
                newItem->setText(0,QString::fromStdString(name));
                shapeData->set_name(newItem->text(0));
                DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(baseParent);
                if (drawingItem && drawingItem->is_drawing()) newItem->copy_depth(drawingItem);
                if (increaseDepth) newItem->increase_depth();
                objectCounts.solid++;
            }

            // material
            localStartBlockIndex=startBlockIndex+1;
            localEndBlockIndex=endBlockIndex-1;
            keyword="material";
            std::string material;
            if (getBlockKeywordValue(inputData,typeStart,localStartBlockIndex,localEndBlockIndex,keyword,material)) {
                newItem->set_Material(QString::fromStdString(material));
                newItem->setText(1,QString::fromStdString(material));
            }

            baseParent->addChild(newItem);
            baseParent->push_child(newItem);

            ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);
            ui->drawingWindow->activateItem(newItem);

            reprocess(newItem);
            newItem->alignForegroundColor();
            ui->drawingWindow->hideItem(newItem);
            if (baseParent->is_rootDrawing()) ui->drawingWindow->showItem(newItem);
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
        saveItem(&outputFile,drawing);
        outputFile.close();
        itemChangesStack.clear();
        return false;
    }
    return true;
}

bool OpenParEMg::loadDrawingFile ()
{
    //std::cout << "OpenParEMg::loadDrawingFile" << std::endl; std::cout.flush();

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
    while (loadItem(inputData,startBlockIndex,endBlockIndex,drawing,false)) {
        //startBlockIndex=endBlockIndex;
    }

    return false;
}

void OpenParEMg::on_actionImportBrep_triggered ()
{
    QString filePath=QFileDialog::getOpenFileName(this,tr("Open BREP File"),absolutePath,tr("BREP Files (*.brep)"),
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
    QString filePath = QFileDialog::getOpenFileName(this,tr("Open STEP File"),absolutePath,tr("STEP Files (*.step *.stp)"),
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

void OpenParEMg::on_actionExportBrep_triggered ()
{
    QString filePath=QFileDialog::getSaveFileName(this,tr("Save BRep File"),absolutePath,tr("BRep Files (*.brep)"),
                                                  nullptr,QFileDialog::DontUseNativeDialog);
    if (filePath.isNull()) return;

    if (saveBrepFile(filePath)) {
        QString message="Unable to save BRep file \"";
        message.append(filePath);
        message.append("\".");
        QMessageBox mb;
        mb.critical(nullptr, "Error",message);
        mb.setFixedSize(500, 200);
    }
    setMenusI(54);
}

void OpenParEMg::on_actionExportStep_triggered()
{
    QString filePath=QFileDialog::getSaveFileName(this,tr("Save STEP File"),absolutePath,tr("STEP Files (*.step *.stp)"),
                                                  nullptr,QFileDialog::DontUseNativeDialog);
    if (filePath.isNull()) return;

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
    QApplication::quit();
}

void OpenParEMg::on_drawingItemTree_itemClicked (QTreeWidgetItem *item, int column)
{
    std::cout << "OpenParEMg::on_drawingItemTree_itemClicked" << std::endl; std::cout.flush();

    std::cout << "   clickedItem=" << clickedItem;
    if (clickedItem) std::cout << "  type=" << clickedItem->get_itemType();
    std::cout << std::endl;

    std::cout << "   previousClickedItem=" << previousClickedItem;
    if (previousClickedItem) std::cout << "  type=" << previousClickedItem->get_itemType();
    std::cout << std::endl;

    std::cout << "   CTRLpressed=" << CTRLpressed << std::endl;
    std::cout << "   SHIFTpressed=" << SHIFTpressed << std::endl;

    if (!item) return;

    // bring drawing to the front
    if (ui->tabs->currentWidget() != ui->drawingTab) {
        ui->tabs->setCurrentWidget(ui->drawingTab);
    }

    clickedItem=dynamic_cast<BaseItem *>(item);
    if (!clickedItem) return;

    // allow multiple selection on matched types only
    bool matchedType=false;
    if (previousClickedItem) {
        int clickedItemType=clickedItem->get_itemType();
        if (clickedItemType == 11) clickedItemType=10;  // voltage and current treated the same

        int previousClickedType=previousClickedItem->get_itemType();
        if (previousClickedType == 11) previousClickedType=10;  // voltage and current treated the same

        if (clickedItemType == previousClickedType) {
            matchedType=true;
        }
    }

    // allow multiple selection only within the same root group
    bool matchedGroup=false;
    if (previousClickedItem) {
        BaseItem *previousRootParent=previousClickedItem->getRootParent();
        BaseItem *currentRootParent=clickedItem->getRootParent();
        if (previousRootParent && currentRootParent) {
            if (previousRootParent == currentRootParent) {
                matchedGroup=true;
            }
        }
    }

    // exception: allow path + (voltage or current) for assigning paths
    if (previousClickedItem) {
        { // PathItem then VIItem
            PathItem *pathItem=dynamic_cast<PathItem *>(previousClickedItem);
            VIItem *viiItem=dynamic_cast<VIItem *>(clickedItem);
            if (pathItem && viiItem) {
                matchedType=true;  // not really
                matchedGroup=true; // not really
            }
        }
        { // VIItem then PathItem
            PathItem *pathItem=dynamic_cast<PathItem *>(clickedItem);
            VIItem *viiItem=dynamic_cast<VIItem *>(previousClickedItem);
            if (pathItem && viiItem) {
                matchedType=true;  // not really
                matchedGroup=true; // not really
            }
        }
    }

    if (CTRLpressed) {
        if (SHIFTpressed) {
        } else {
            if (matchedType && matchedGroup) {
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
            if (matchedType && matchedGroup) {

                ui->drawingItemTree->setCurrentItem(clickedItem);

                // select the end item

                //ui->drawingWindow->selectItem(previousClickedItem);
                ui->drawingWindow->selectItem(clickedItem);

                // select the middle items

                int count=ui->drawingItemTree->indexFromItem(clickedItem,0).row()-
                        ui->drawingItemTree->indexFromItem(previousClickedItem,0).row();

                if (count > 1) {  // forward
                    BaseItem *nextItem=dynamic_cast<BaseItem *>(ui->drawingItemTree->itemBelow(previousClickedItem));
                    while (nextItem != clickedItem) {
                        if (nextItem->QTreeWidgetItem::parent() == clickedItem->QTreeWidgetItem::parent()) {
                            ui->drawingWindow->selectItem(nextItem);
                        }
                        nextItem=dynamic_cast<BaseItem *>(ui->drawingItemTree->itemBelow(nextItem));
                    }
                } else if (-count > 1) {  // reversed
                    BaseItem *nextItem=dynamic_cast<BaseItem *>(ui->drawingItemTree->itemBelow(clickedItem));
                    while (nextItem != previousClickedItem) {
                        if (nextItem->QTreeWidgetItem::parent() == previousClickedItem->QTreeWidgetItem::parent()) {
                            ui->drawingWindow->selectItem(nextItem);
                        }
                        nextItem=dynamic_cast<BaseItem *>(ui->drawingItemTree->itemBelow(nextItem));
                    }
                }

                previousClickedItem=clickedItem;
            } else {
                clickedItem->setSelected(false);
                ui->drawingItemTree->setCurrentItem(nullptr);
            }
        }
    } else {
        BaseItem *clickedItemKeep=clickedItem;
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
    std::cout << "OpenParEMg::on_actionFace_triggered" << std::endl; std::cout.flush();

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

// bool OpenParEMg::hasSelectedPaths ()
// {
//     long unsigned int i=0;
//     while (i < ui->drawingWindow->get_selectedItems_size()) {
//         BaseItem *baseItem=ui->drawingWindow->get_selectedItem(i);
//         if (baseItem) {
//             PathItem *pathItem=dynamic_cast<PathItem *>(baseItem);
//             if (pathItem && pathItem->is_path()) return true;
//         }
//         i++;
//     }
//     return false;
// }

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

        finishOperation(true,30);
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
    drawing->show(true);
    ui->drawingWindow->showItem(path);
    ui->drawingWindow->showItem(port);
    ui->drawingWindow->showItem(boundary);
    ui->drawingWindow->showItem(mesh);
    clickedItem=nullptr;
    previousClickedItem=nullptr;
    ui->drawingWindow->updateViewer();
    setMenusI(59);
}

void OpenParEMg::on_actionHideAll_triggered ()
{
    //ui->drawingWindow->hideAllItems();

    drawing->hide(false);
    path->hide(false);
    port->hide(false);
    boundary->hide(false);
    mesh->hide(false);

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

    materialList.clear();
    shapeList.clear();

    // get all nodes
    std::vector<std::size_t> nodeTags;
    std::vector<double> nodeCoords, nodeParams;
    gmsh::model::mesh::getNodes(nodeTags,nodeCoords,nodeParams);

    // map node tag to coordinates
    std::map<std::size_t,gp_Pnt> nodeMap;
    long unsigned int i=0;
    while (i < nodeTags.size()) {
        double x=nodeCoords[3*i];
        double y=nodeCoords[3*i+1];
        double z=nodeCoords[3*i+2];
        nodeMap[nodeTags[i]]=gp_Pnt(x,y,z);
        i++;
    }

    // get the entities for 3D
    std::vector<std::pair<int,int>> physicalGroups;
    std::vector<gmsh::vectorpair> entities;
    gmsh::model::getPhysicalGroupsEntities(physicalGroups,entities,3);

    // loop through the physical groups
    size_t pg=0;
    while (pg < physicalGroups.size())
    {
        // save all physical group shapes into a compound
        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);

        // loop through the tetrahedons of the physical group
        size_t ent=0;
        while (ent < entities[pg].size()) {

            int entityDim=entities[pg][ent].first;
            int entityTag=entities[pg][ent].second;

            std::vector<int> elementTypes;
            std::vector<std::vector<std::size_t>> elementTags;
            std::vector<std::vector<std::size_t>> nodeTags;
            gmsh::model::mesh::getElements(elementTypes,elementTags,nodeTags,entityDim,entityTag);

            int count=0;
            std::vector<std::size_t> conn=nodeTags[ent];
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

                if (!edge12.IsNull() && !edge13.IsNull() && !edge14.IsNull() && !edge23.IsNull() && !edge34.IsNull() && !edge42.IsNull()) {

                    // for viewing meshes, only the edges are needed
                    builder.Add(compound,edge12);
                    builder.Add(compound,edge13);
                    builder.Add(compound,edge14);
                    builder.Add(compound,edge23);
                    builder.Add(compound,edge34);
                    builder.Add(compound,edge42);

                    // skip building the rest of the tetrahedrons
                    // This code no longer works with rest of this routine.

                    // TopoDS_Wire wire123=BRepBuilderAPI_MakeWire(edge12,edge23,edge13);
                    // TopoDS_Wire wire134=BRepBuilderAPI_MakeWire(edge13,edge34,edge14);
                    // TopoDS_Wire wire124=BRepBuilderAPI_MakeWire(edge12,edge42,edge14);
                    // TopoDS_Wire wire234=BRepBuilderAPI_MakeWire(edge23,edge34,edge42);

                    // if (!wire123.IsNull() && !wire134.IsNull() && !wire124.IsNull() && !wire234.IsNull()) {

                    //     TopoDS_Face face123=BRepBuilderAPI_MakeFace(wire123);
                    //     TopoDS_Face face134=BRepBuilderAPI_MakeFace(wire134);
                    //     TopoDS_Face face124=BRepBuilderAPI_MakeFace(wire124);
                    //     TopoDS_Face face234=BRepBuilderAPI_MakeFace(wire234);

                    //     if (!face123.IsNull() && !face134.IsNull() && !face124.IsNull() && !face234.IsNull()) {

                    //         TopoDS_Compound tetrahedron;
                    //         BRep_Builder builder;
                    //         builder.MakeCompound(tetrahedron);
                    //         builder.Add(tetrahedron,face123);
                    //         builder.Add(tetrahedron,face134);
                    //         builder.Add(tetrahedron,face124);
                    //         builder.Add(tetrahedron,face234);

                    //         Handle(AIS_Shape) shape=new AIS_Shape(tetrahedron);
                    //         tetrahedronsItem->get_meshEntities()->push_back(shape);

                    //         builder.MakeCompound(compound);
                    //         builder.Add(compound,face123);
                    //         builder.Add(compound,face134);
                    //         builder.Add(compound,face124);
                    //         builder.Add(compound,face234);
                    //     }
                    // }
                }
                i+=4;
                count++;
            }
            ++ent;
        }

        // pull the material name, if available
        QString name;
        if (pg < projData.physicalGroupMaterialCount) {
            name=projData.physicalGroupMaterials[pg].materialName;
        } else {
            name="physicalGroup";
            name.append(QString::number(pg+1));
        }

        materialList.push_back(name);
        shapeList.push_back(compound);

        // // make the item
        // MeshItem *groupItem=new MeshItem(this);
        // groupItem->setText(0,name);
        // groupItem->set_itemType(3);
        // groupItem->setForeground(0,Qt::gray);
        // mesh->addChild(groupItem);

        // Handle(AIS_Shape) newAISshape=new AIS_Shape(compound);
        // if (!newAISshape.IsNull()) {
        //     ShapeData *shapeData=groupItem->getShapeData();
        //     shapeData->setShape(newAISshape);
        // }

        ++pg;
    }
    setMenusI(62);
}

void OpenParEMg::finishDrawMesh ()
{
    long unsigned int i=0;
    while (i < materialList.size()) {

        // make the item
        MeshItem *groupItem=new MeshItem(this);
        groupItem->setText(0,materialList[i]);
        groupItem->set_itemType(3);
        groupItem->setForeground(0,Qt::gray);
        mesh->addChild(groupItem);

        Handle(AIS_Shape) newAISshape=new AIS_Shape(shapeList[i]);
        if (!newAISshape.IsNull()) {
            ShapeData *shapeData=groupItem->getShapeData();
            shapeData->setShape(newAISshape);
        }

        i++;
    }

    meshObsolete=false;
    drawing->reset_modifiedSinceMeshRegen();
    //ui->drawingWindow->fitAll();
    //mesh->show(true);
    setMenusI(63);
}

void OpenParEMg::finishDrawMeshShow ()
{
    finishDrawMesh();
    mesh->show(true);
}

void OpenParEMg::deleteMesh (bool deleteMeshFile)
{
    //std::cout << "OpenParEMg::deleteMesh" << std::endl; std::cout.flush();

    int i=0;
    while (i < mesh->childCount()) {
        MeshItem *meshItem=dynamic_cast<MeshItem *>(mesh->child(i));

        // remove from tracker
        ui->drawingWindow->hideItem(meshItem);
        ui->drawingWindow->unselectItem(meshItem);

        // remove drawing shape
        ShapeData *shapeData=meshItem->getShapeData();
        Handle(AIS_Shape) shape=shapeData->getShape();
        if (!shape.IsNull()) {
            ui->drawingWindow->deleteShape(shape);
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
    mesh->deleteChildren(mesh);
    mesh->setModified(false);
    meshObsolete=false;
    drawingEntities.clear();
    drawing->reset_modifiedSinceMeshRegen();
    gmsh::clear();

    if (projData.mesh_file) {
        free(projData.mesh_file);
        projData.mesh_file=(char *)malloc(sizeof(char));
        projData.mesh_file[0]='\0';
    }
}

bool OpenParEMg::validDrawing ()
{
    int i=0;
    while (i < drawing->childCount()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(drawing->child(i));
        if (drawingItem) {
            Handle(AIS_Shape) aisShape=drawingItem->getShape();
            if (aisShape) {
                TopoDS_Shape shape=aisShape->Shape();
                if (shape.ShapeType() == TopAbs_SOLID || shape.ShapeType() == TopAbs_COMPOUND) {
                    return true;
                }
            }
        }
        i++;
    }

    return false;
}

bool OpenParEMg::validPorts ()
{
    int i=0;
    while (i < port->childCount()) {
        PortItem *portItem=dynamic_cast<PortItem *>(port->child(i));
        if (!portItem->isValid()) return false;
        i++;
    }

    return true;
}

bool OpenParEMg::validMultimodeLinePorts ()
{
    int i=0;
    while (i < port->childCount()) {
        PortItem *portItem=dynamic_cast<PortItem *>(port->child(i));
        if (!portItem->isValidMultimodeLine()) return false;
        i++;
    }

    return true;
}

bool OpenParEMg::meshAssigned ()
{
    if (mesh->childCount() == 0) return false;

    int i=0;
    while (i < mesh->childCount()) {
        MeshItem *meshItem=dynamic_cast<MeshItem *>(mesh->child(i));
        if (meshItem) {
            if (meshItem->text(0).compare("unassigned") == 0) return false;
        }
        i++;
    }

    return true;
}

bool OpenParEMg::materialsAssigned ()
{
    if (drawing->childCount() == 0) return false;

    int i=0;
    while (i < drawing->childCount()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(drawing->child(i));
        if (drawingItem) {
            Handle(AIS_Shape) aisShape=drawingItem->getShape();
            if (aisShape) {
                TopoDS_Shape shape=aisShape->Shape();
                if (shape.ShapeType() == TopAbs_SOLID || shape.ShapeType() == TopAbs_COMPOUND) {
                    if (drawingItem->text(1).isEmpty()) return false;
                }
            }
        }
        i++;
    }

    return true;
}

Standard_Real calculateVolume (const TopoDS_Shape& shape)
{
    GProp_GProps properties;
    BRepGProp::VolumeProperties(shape,properties);
    return properties.Mass(); // Mass() returns volume for VolumeProperties
}

double calculateSolidComparison (const TopoDS_Shape& newSolid, const TopoDS_Shape& originalSolid)
{
    //std::cout << "calculateSolidComparison" << std::endl; std::cout.flush();

    double tol=1e-30;

    Standard_Real newSolidVolume=calculateVolume(newSolid);
    //std::cout << "   newSolidVoume=" << newSolidVolume << std::endl; std::cout.flush();
    if (newSolidVolume < tol) return 0;

    Standard_Real originalSolidVolume=calculateVolume(originalSolid);
    //std::cout << "   originalSolidVolume=" << originalSolidVolume << std::endl; std::cout.flush();
    if (originalSolidVolume < tol) return 0;

    if (newSolidVolume > originalSolidVolume*(1+1e-6)) return 0;

    // calculate intersection
    BRepAlgoAPI_Common intersection(newSolid,originalSolid);
    intersection.Build();

    // get the result
    const TopoDS_Shape& commonResult=intersection.Shape();

    Standard_Real commonResultVolume=calculateVolume(commonResult);
    //std::cout << "   commonResultVolume=" << commonResultVolume << std::endl; std::cout.flush();
    if (commonResultVolume < tol) return 0;

    //std::cout << "   metric=" << commonResultVolume/originalSolidVolume << std::endl; std::cout.flush();
    return commonResultVolume/originalSolidVolume;
}

void OpenParEMg::on_actionMeshGenerate_triggered ()
{
    //std::cout << "OpenParEMg::on_actionMeshGenerate_triggered" << std::endl; std::cout.flush();

    if (mesh->childCount() > 0) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this,"OpenParEMg","Delete the existing mesh?",QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        deleteMesh(true);
    }

    // reset dimTags
    renumberDimTag();
    reprocess(drawing);
    // build a boolean fragments shape
    // ToDo: remove the COMPOUND at the drawing level since that is obsolete (wrong approach)

    TopTools_ListOfShape arguments;

    // build boolean fragments shape
    std::cout << "build boolean fragments shape" << std::endl; std::cout.flush();
    int i=0;
    while (i < drawing->childCount()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(drawing->child(i));
        if (drawingItem) {
            ShapeData *shapeData=drawingItem->getShapeData();
            TopoDS_Shape shape=shapeData->getShape()->Shape();
            if (!shape.IsNull()) {
                std::cout << "   add drawingItem=" << drawingItem->text(0).toStdString();
                TopAbs_Orientation orient=shape.Orientation();
                if (orient == TopAbs_FORWARD) std::cout << "  TopAbs_FORWARD";
                if (orient == TopAbs_REVERSED) std::cout << "  TopAbs_REVERSED";
                if (orient == TopAbs_INTERNAL) std::cout << "  TopAbs_INTERNAL";
                if (orient == TopAbs_EXTERNAL) std::cout << "  TopAbs_EXTERNAL";
                std::cout << std::endl; std::cout.flush();
                arguments.Append(shape);
            }
        }
        i++;
    }

    // path items - imprint the paths onto the mesh for better accuracy
    i=0;
    while (i < path->childCount()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(path->child(i));
        if (pathItem) {
            ShapeData *shapeData=pathItem->getShapeData();
            Handle(AIS_Shape) aisShape=shapeData->getShape();
            if (!aisShape.IsNull()) {
                TopoDS_Shape shape=aisShape->Shape();
                if (!shape.IsNull()) {
                    if (shape.ShapeType() == TopAbs_COMPOUND) {
                        TopoDS_Iterator it(shape);
                        while (it.More()) {
                            // paths with arrows - take the first TopoDS_Shape with the rest being arrowheads
                            TopoDS_Shape childShape=it.Value();
                            if (childShape.ShapeType() != TopAbs_COMPOUND) {
                                std::cout << "   imprint pathItem without arrows=" << pathItem->text(0).toStdString() << std::endl; std::cout.flush();
                                arguments.Append(childShape);
                                break;
                            }
                            it.Next();
                        }
                    } else {
                        std::cout << "   imprint pathItem=" << pathItem->text(0).toStdString() << std::endl; std::cout.flush();
                        arguments.Append(shape);
                    }
                }
            }
        }
        i++;
    }

    // assemble - This can produce duplicates of enclosed solids.
    // BOPAlgo_Builder builder;
    // builder.SetArguments(arguments);
    // builder.Perform();
    // if (builder.HasErrors()) return;
    // TopoDS_Shape fragments=builder.Shape();

    // assemble
    BOPAlgo_CellsBuilder cellBuilder;
    cellBuilder.SetArguments(arguments);
    cellBuilder.Perform();
    if (cellBuilder.HasErrors()) return;
    TopoDS_Shape fragments=cellBuilder.GetAllParts();

    // build mesh
    gmsh::model::occ::importShapesNativePointer((void *) &fragments,drawingEntities,false);
    gmsh::option::setNumber("Mesh.MeshSizeFactor",projData.gui_mesh_scale);
    gmsh::option::setNumber("Mesh.MeshSizeMin",projData.gui_mesh_minSize);
    gmsh::option::setNumber("Mesh.MeshSizeMax",projData.gui_mesh_maxSize);
    //xxx
    std::cout << "OpenParEMg::on_actionMeshGenerate_triggered  projData.gui_mesh_maxSize=" << projData.gui_mesh_maxSize << std::endl; std::cout.flush();
    gmsh::model::occ::synchronize();
    gmsh::model::mesh::generate(3);

    // default name for the mesh
    if (strcmp(projData.mesh_file,"") == 0) {
        QString meshFile=projectName;
        meshFile.append(".msh");
        cstrFromQString(&(projData.mesh_file),meshFile);
        projectChanged=true;
    }

    clear_physicalGroupMaterials (&projData);

    //const TopoDS_Shape& finalResult=builder.Shape();
    //TopExp_Explorer exp(finalResult,TopAbs_SOLID);
    TopExp_Explorer exp(fragments,TopAbs_SOLID);

    // create physical groups
    int solidNumber=1;
    for (; exp.More(); exp.Next()) {
        const TopoDS_Shape& newSolid=exp.Current();

        // find the drawingItem with the best metric
        double maxMetric=0;
        DrawingItem *bestDrawingItem=nullptr;
        int i=0;
        while (i < drawing->childCount()) {
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(drawing->child(i));
            if (drawingItem) {
                Handle(AIS_Shape) aisShape=drawingItem->getShape();
                if (!aisShape.IsNull()) {
                    double metric=calculateSolidComparison(newSolid,aisShape->Shape());
                    if (metric > maxMetric) {
                        maxMetric=metric;
                        bestDrawingItem=drawingItem;
                    }
                }
            }
            i++;
        }

        // get a material name
        QString material="unassigned";
        if (bestDrawingItem) {
            if (!bestDrawingItem->text(1).isEmpty()) {
                material=bestDrawingItem->text(1);
            }
        }

        // create a physical group
        char *materialName=nullptr;
        cstrFromQString (&materialName,material);
        add_physicalGroupMaterial(&projData,-1,3,solidNumber,materialName);
        solidNumber++;
    }

    // assign to mesh
    i=0;
    while (i < projData.physicalGroupMaterialCount) {
        std::vector<int> physicalGroupList;
        physicalGroupList.push_back(0);
        physicalGroupList[0]=projData.physicalGroupMaterials[i].tag;

        // uniquify the physical group name with the group tag to avoid gmsh eliminating
        // physical groups with duplicated names from the $PhysicalNames/$EndPhysicalNames block in the msh file
        std::string groupName=projData.physicalGroupMaterials[i].materialName;
        groupName.append("_OPEM_RESERVED_");
        groupName.append(std::to_string(projData.physicalGroupMaterials[i].tag));

        gmsh::model::addPhysicalGroup(projData.physicalGroupMaterials[i].dim,physicalGroupList,-1,groupName.c_str());
        i++;
    }

    // draw the mesh - use a concurrent run so the GUI doesn't lock up on long draws
    // This may or may not be helping.
    // ToDo: keep experimenting
    QFutureWatcher<void> *watcher=new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished,this,&OpenParEMg::finishDrawMeshShow);
    QFuture<void> future=QtConcurrent::run(&OpenParEMg::drawMesh,this);
    watcher->setFuture(future);
}

void OpenParEMg::loadMeshFile (QString meshfile)
{
    //std::cout << "OpenParEMg::loadMeshFile" << std::endl; std::cout.flush();

    if (QFile::exists(meshfile)) {

        if (mesh->childCount() > 0) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this,"OpenParEMg","Delete the existing mesh?",QMessageBox::Yes|QMessageBox::No);
            if (reply != QMessageBox::Yes) return;

            deleteMesh(false);
        }

        // load and display
        gmsh::open(meshfile.toStdString());
        //gmsh::merge(meshfile.toStdString());
        //gmsh::plugin::run("CreatePhysicalGroupsByField");
        drawMesh();
        finishDrawMesh();

        // set the item names
        gmsh::vectorpair physicalGroups;
        gmsh::model::getPhysicalGroups(physicalGroups);
        for (const auto& group : physicalGroups) {
            int dim = group.first;
            int tag = group.second;
            std::string name;
            gmsh::model::getPhysicalName(dim,tag,name);

            size_t pos=name.rfind("_OPEM_RESERVED_");
            std::string strippedName=name.substr(0,pos);

            if (tag-1 < mesh->childCount()) {
                mesh->child(tag-1)->setText(0,QString::fromStdString(strippedName));
            }
        }

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
    QString meshfile=QFileDialog::getOpenFileName(this,tr("Open Mesh"),absolutePath,tr("Mesh Files (*.msh);;All Files (*)"),
                                                  nullptr,QFileDialog::DontUseNativeDialog);

    // return if user cancels
    if (meshfile.isNull()) return;

    loadMeshFile(meshfile);
    ui->drawingWindow->showItem(mesh);
    setMenusI(65);
}

void OpenParEMg::on_actionMeshSave_triggered ()
{
    if (strcmp(projData.mesh_file,"") != 0) {
        gmsh::write(projData.mesh_file);
        mesh->setModified(false);
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
        QString testMeshFile=QFileDialog::getSaveFileName(this,tr("Save Mesh File"),absolutePath,tr("Data Files (*.msh);;All Files (*)"),
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
    mesh->setModified(false);

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

    projectChanged=true;
    setMenusI(68);
}

void OpenParEMg::on_actionWireframe_triggered ()
{
    if (ui->actionWireframe->isChecked() == true) {
        ui->drawingWindow->set_wireframe(true);
    } else {
        ui->drawingWindow->set_wireframe(false);
    }
    ui->drawingWindow->updateViewer();
}

void eh3D (MPI_Comm *comm, int *err, ...)
{
    //QMessageBox mb;
    //mb.critical(nullptr, "Error","eh3D: Failed to launch OpenParEM3D.");
}

void OpenParEMg::on_actionRun_triggered ()
{
    // check for an existing lock file

    QString currentPath;
    currentPath=QDir::currentPath();

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

    // clear the results in the tabs

    ui->logText->clear();
    ui->iterationsText->clear();
    ui->dataText->clear();
    ui->antennaText->clear();

    logLastPos=0;
    iterationLastPos=0;
    dataLastPos=0;
    antennaLastPos=0;

    logLastChar='\0';
    iterationsLastChar='\0';
    dataLastChar='\0';
    antennaLastChar='\0';

    // bring log to the front
    if (ui->tabs->currentWidget() != ui->logTab) {
        ui->tabs->setCurrentWidget(ui->logTab);
    }

    // install event filters in the tabs

    ui->logText->viewport()->installEventFilter(logFilter);
    ui->logText->verticalScrollBar()->installEventFilter(logFilter);

    ui->iterationsText->viewport()->installEventFilter(iterationsFilter);
    ui->iterationsText->verticalScrollBar()->installEventFilter(iterationsFilter);

    ui->dataText->viewport()->installEventFilter(dataFilter);
    ui->dataText->verticalScrollBar()->installEventFilter(dataFilter);

    // assemble the command line argument

    char *project=nullptr;
    cstrFromQString (&project,projectFile);

    char *logfile=(char *)malloc((strlen(projData.project_name)+5)*sizeof(char));
    sprintf(logfile,"%s.log",projData.project_name);

    char *argv[3];
    argv[0]=project;
    argv[1]=logfile;
    argv[2]=nullptr;

    int *error_codes=(int *)malloc(projData.gui_slot_count*sizeof(int));

    // MPI_INFO object
    MPI_Info info;
    MPI_Info_create(&info);
    if (projData.gui_oversubscribe) {
        MPI_Info_set(info,"map_by", "node:OVERSUBSCRIBE");
    }

    // run the job

    MPI_Errhandler errorHandler;
    MPI_Comm_create_errhandler(eh3D,&errorHandler);
    MPI_Comm_set_errhandler(PETSC_COMM_WORLD,errorHandler);

    if (MPI_PORT_COMM) MPI_Comm_free(MPI_PORT_COMM);
    MPI_PORT_COMM=new MPI_Comm();

    MPI_Comm_spawn ("OpenParEM3D",argv,projData.gui_slot_count,info,0,PETSC_COMM_WORLD,MPI_PORT_COMM,error_codes);

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
    if (logfile) free(logfile);
    //if (argv) free(argv);
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

        // finish results loading, if they are not done already
        updateLogTab(true);
        updateIterationsTab(true);
        updateDataTab(true);
        updateAntennaTab(true);

        // uninstall event filters on the tabs

        ui->logText->verticalScrollBar()->removeEventFilter(logFilter);
        ui->logText->viewport()->removeEventFilter(logFilter);

        ui->iterationsText->verticalScrollBar()->removeEventFilter(iterationsFilter);
        ui->iterationsText->viewport()->removeEventFilter(iterationsFilter);

        ui->dataText->verticalScrollBar()->removeEventFilter(dataFilter);
        ui->dataText->viewport()->removeEventFilter(dataFilter);

        // get the status of the 3D simulations
        //std::cout << "OPEMg receive 320000" << std::endl; std::cout.flush();
        int fail3D=0;
        MPI_Recv(&fail3D,1,MPI_INT,0,320000,*MPI_PORT_COMM,MPI_STATUS_IGNORE);

        if (fail3D & !isAbort) {
            QMessageBox mb;
            mb.critical(nullptr, "Error", "OpenParEM3D failed to properly run. Check the logged output for messages.");
            mb.setFixedSize(500, 200);
        }

        // unblock OpenParEM3D
        //std::cout << "OPEMg send 300000" << std::endl; std::cout.flush();
        MPI_Send(&signal,1,MPI_INT,0,300000,*MPI_PORT_COMM);  // stop
        if (!isAbort) {
            //std::cout << "OPEMg send 300001" << std::endl; std::cout.flush();
            MPI_Send(&signal,1,MPI_INT,0,300001,*MPI_PORT_COMM);  // abort
        }

        //std::cout << "OPEMg disconnect MPI_PORT_COMM" << std::endl; std::cout.flush();
        if (!isAbort) MPI_Comm_disconnect(MPI_PORT_COMM);

        //std::cout << "OPEMg free MPI_PORT_COMM" << std::endl; std::cout.flush();
        MPI_Comm_free(MPI_PORT_COMM);
        MPI_PORT_COMM=nullptr;

        //std::cout << "OPEMg free request" << std::endl; std::cout.flush();
        MPI_Request_free(request);
        request=nullptr;

        if (isAbort) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"OpenParEM3D Job Aborted.\n");
        }
        //std::cout << "OPEMg return checkFinish" << std::endl; std::cout.flush();
    }
}

void OpenParEMg::on_actionAbort_triggered ()
{
    //std::cout << "OpenParEMg::on_actionAbort_triggered:" << std::endl;  std::cout.flush();

    simulationStopping=false;
    simulationAborting=true;
    setMenusI(72);

    int signal=1;
    MPI_Send(&signal,1,MPI_INT,0,300001,*MPI_PORT_COMM);
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

    Standard_Real xOrigin=0;
    Standard_Real yOrigin=0;
    Standard_Real xStep=1/getConversionFactor();
    Standard_Real yStep=1/getConversionFactor();
    Standard_Real rotationAngle=0;
    Standard_Real xSize=1/getConversionFactor()*projData.gui_grid_size/2;
    Standard_Real ySize=1/getConversionFactor()*projData.gui_grid_size/2;
    Standard_Real offset=0;

    ui->drawingWindow->showGrid(xOrigin,yOrigin,xStep,yStep,rotationAngle,xSize,ySize,offset);
    setScale();
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
    //std::cout << "OpenParEMg::setPlaneToFace" << std::endl; std::cout.flush();
    skipDrawingPlaneAxisForm=true;
    startPlaneSetToFace();
}

void OpenParEMg::setPlaneToFaceAxis ()
{
    //std::cout << "OpenParEMg::setPlaneToFaceAxis" << std::endl; std::cout.flush();
    skipDrawingPlaneAxisForm=false;
    startPlaneSetToFace();
}

void OpenParEMg::on_actionDrawingPlaneSetToFace_triggered ()
{
    //std::cout << "OpenParEMg::on_actionDrawingPlaneSetToFace_triggered" << std::endl; std::cout.flush();

    startOperation(false);

    skipDrawingPlaneAxisForm=true;
    on_actionFace_triggered();
    ui->drawingWindow->setSubshapeSelection(true);
    ui->drawingWindow->setSetToPlane(true);
}

void OpenParEMg::on_actionDrawingPlaneSetToFaceAxis_triggered ()
{
    //std::cout << "OpenParEMg::on_actionDrawingPlaneSetToFaceAxis_triggered" << std::endl; std::cout.flush();

    restrictToDrawingPlane=true;
    startOperation(true);

    skipDrawingPlaneAxisForm=false;
    on_actionFace_triggered();
    ui->drawingWindow->setSubshapeSelection(true);
    ui->drawingWindow->setSetToPlane(true);
}

void OpenParEMg::startPlaneSetToFace ()
{
    //std::cout << "OpenParEMg::startPlaneSetToFace" << std::endl; std::cout.flush();

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
    vectorInputForm=new VectorInputForm(this);
    vectorInputForm->set_conversionFactor(getConversionFactor());
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

    // Do not call finishOperation: vectorInputForm calls finishOperation
    //finishOperation(false,11);
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

void OpenParEMg::on_actionPreferences_triggered ()
{
    DrawingPreferences *drawingPreferences=new DrawingPreferences();
    drawingPreferences->set_simulationRunning(simulationRunning);
    drawingPreferences->set_projData(&projData);
    drawingPreferences->exec();
    delete drawingPreferences;

    if (projData.modified) {
        projectChanged=true;
    }

    if (drawingPlaneShown) {
        on_actionDrawingPlaneShow_triggered();
    }
    setMenusI(56);
}

void OpenParEMg::cancelDraw ()
{
    if (currentDrawingItem) {
        currentDrawingItem->cancelDraw();
        delete currentDrawingItem;
        currentDrawingItem=nullptr;
    }

    finishOperation(true,12);
}

void OpenParEMg::on_actionDrawLine_triggered ()
{
    currentDrawingItem=new DrawingItem(this,drawing);
    currentDrawingItem->startLine();
}

void OpenParEMg::on_actionDrawPolyline_triggered ()
{
    currentDrawingItem=new DrawingItem(this,drawing);
    currentDrawingItem->startPolyline();
}

void OpenParEMg::on_actionDrawPolycircle_triggered ()
{
    currentDrawingItem=new DrawingItem(this,drawing);
    currentDrawingItem->startPolycircle();
}

void OpenParEMg::on_actionDrawRectangle_triggered ()
{
    currentDrawingItem=new DrawingItem(this,drawing);
    currentDrawingItem->startRectangle();
}

void OpenParEMg::finishDraw ()
{
    if (!activePolywire) return;
    activePolywire->setDrawEnable(false);

    if (isIntegrationPath) {

        VIItem *viItem=dynamic_cast<VIItem *>(workingItem);
        if (viItem) {

            currentDrawingItem->finishDraw();

            // port
            PortItem *portItem=viItem->getPortItem();
            if (!portItem) return;

            // port outline
            PathItem *portPathItem=portItem->getPathItem();
            if (!portPathItem) return;
            Path *portPath=portPathItem->getPath();
            if (!portPath) return;

            // set to a name reasonable for an integration path
            int Sport=viItem->getModeItem()->get_Sport();
            QString name;
            if (viItem->is_voltage()) name="v";
            if (viItem->is_current()) name="i";
            name.append(QString::number(Sport));

            // create path
            PathItem *newPathItem=currentDrawingItem->createPath(true);
            ShapeData *shapeData=newPathItem->getShapeData();
            shapeData->set_name(name);
            newPathItem->setText(0,name);
            currentDrawingItem->del();

            // see if the path is within an existing port
            Path *path=newPathItem->getPath();
            if (portPath->is_path_inside(path)) {

                // create the integration path
                viItem->createIntegrationPathItemFromPath(newPathItem);

                // set the impedance definition to a reasonable value
                int i=0;
                while (i < portItem->childCount()) {
                    BaseItem *baseItem=dynamic_cast<BaseItem *>(portItem->child(i));
                    if (baseItem && baseItem->is_impedanceDefinition()) {
                        CustomComboBox *comboZdef=dynamic_cast<CustomComboBox *>(ui->drawingItemTree->itemWidget(baseItem,0));
                        if (comboZdef) {
                            if (comboZdef->currentIndex() == 3) {  // invalid
                                if (viItem->is_voltage()) comboZdef->setCurrentIndex(1);
                                if (viItem->is_current()) comboZdef->setCurrentIndex(2);
                            }
                        }
                    }
                    i++;
                }

                // add scale, if needed
                viItem->addScaleItem();
            } else {
                QMessageBox mb;
                mb.critical(nullptr, "Error", "The path is not within the port outline.");
                mb.setFixedSize(500, 200);

                newPathItem->del();
            }

            portItem->setImpedanceDefinitionOptions();

            isIntegrationPath=false;
            currentDrawingItem=nullptr;
        }
    } else {
        currentDrawingItem->finishDraw();
        currentDrawingItem=nullptr;
    }

    finishOperation(false,13);
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

    setMenusI(100);
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
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {

            // move
            if (drawingItem->getEnableMove() && drawingItem->hasP0()) {
                drawingItem->moveAnimateShape(lastMousePosition,pnt,ui->drawingWindow->get_viewerContext());
            }

            Polywire *polywire=static_cast<Polywire *>(drawingItem->getPolywire());

            // stretch
            if (polywire && drawingItem->getEnableStretch() && drawingItem->hasP0()) {
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

            // insert point
            if (polywire && drawingItem->getEnableInsertPoint() && drawingItem->hasP0()) {
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

    // actions for which several selected items are allowed
    bool finishMove=false;
    int i=0;
    while (i < ui->drawingWindow->get_selectedItems_size()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
        if (drawingItem && drawingItem->is_drawing()) {

            // move
            if (drawingItem->getEnableMove()) {
                if (!drawingItem->hasP0()) {
                    drawingItem->setP0(pnt);  // just to use the set flag
                    startPoint=pnt;
                    //ui->drawingWindow->hideItem(drawingItem);
                    ui->drawingWindow->removeShape(drawingItem->getShape());
                } else {
                    //drawingItem->setP1(pnt);
                    endPoint=pnt;
                    finishMove=true;
                }
            }
        }
        i++;
    }
    if (finishMove) finishMoveObject(startPoint,endPoint);

    // actions for which only one selected item is allowed or needed
    if (ui->drawingWindow->get_selectedItems_size() > 0) {
        BaseItem *baseItem=ui->drawingWindow->get_selectedItem(0);
        if (baseItem) {
            // BaseItem *workItem=nullptr;
            Polywire *polywire=nullptr;

            if (baseItem->is_drawing()) {
                DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(baseItem);
                polywire=static_cast<Polywire *>(drawingItem->getPolywire());
            }

            if (baseItem->is_path()) {
                PathItem *pathItem=dynamic_cast<PathItem *>(baseItem);
                polywire=static_cast<Polywire *>(pathItem->getPolywire());
            }


            if (polywire) {

                // stretch
                if (baseItem->getEnableStretch()) {

                    Rectangle *rectangle=dynamic_cast<Rectangle *>(polywire);
                    if (rectangle) {
                        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                            rectangle->setIsSquare(true);
                        } else {
                            rectangle->setIsSquare(false);
                        }
                    }

                    if (!baseItem->hasP0()) {
                        baseItem->setP0(pnt);
                        polywire->setEditIndex(pnt);
                        polywire->setCurrentMousePosition(pnt);
                        polywire->drawStretchRubberband();

                        // switch to allowing selection on midpoints
                        startOperation(true);

                        //ui->drawingWindow->hideItem(baseItem);
                        ui->drawingWindow->removeShape(baseItem->getShape());
                        ui->drawingWindow->updateViewer();
                    } else {
                        if (polywire->isPointOnPlane(pnt) && polywire->isValidInsertPoint(pnt)) {
                            baseItem->setP1(pnt);
                            polywire->setCurrentMousePosition(pnt);
                            polywire->drawStretchRubberband();
                            finishStretchObject();
                        }
                    }
                }

                // delete point
                if (baseItem->getEnableDeletePoint()) {
                    if (polywire->isPointOnPlane(pnt)) {
                        baseItem->setP0(pnt);
                        finishDeletePoint(baseItem);
                    }
                }

                // insert point
                if (baseItem->getEnableInsertPoint()) {
                    if (polywire->isPointOnPlane(pnt)) {
                        if (!baseItem->hasP0()) {
                            baseItem->setP0(pnt);
                            finishInsertPoint(baseItem);
                        } else {
                            baseItem->setP1(pnt);
                            polywire->setCurrentMousePosition(pnt);
                            polywire->drawStretchRubberband();
                            baseItem->finishStretchPoint();
                        }
                    }
                }
            }
        }
    }

    lastMousePosition=pnt;
}

void OpenParEMg::finishOperation (bool cancel, int source)
{
    //std::cout << "OpenParEMg::finishOperation  cancel=" << cancel << "  source=" << source << std::endl; std::cout.flush();

    if (cancel) {

        if (currentDrawingItem) {
            currentDrawingItem->cancelDraw();
            delete currentDrawingItem;
            currentDrawingItem=nullptr;
        }

        if (renameItem) {
            ui->drawingItemTree->removeItemWidget(renameItem,0);
            renameItem=nullptr;
        }

        if (activeAction) {
            long unsigned int i=0;
            while (i < ui->drawingWindow->get_selectedItems_size()) {
                DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(ui->drawingWindow->get_selectedItem(i));
                if (drawingItem && drawingItem->is_drawing()) drawingItem->cancelOperation();
                i++;
            }

            itemChangesStack.pop_back();
            activeAction=false;

            ui->drawingWindow->set_gridPlane(currentPrivilegedPlane);
        }

        if (vectorInputForm) {
            ui->drawingWindow->set_gridPlane(currentPrivilegedPlane);
        }

        restrictToDrawingPlane=false;

    } else {
        if (lengthInputForm) finishExtrudePolywire();
        if (vectorInputForm) finishPlaneSetToFace();
        if (rotateInputForm) finishRotateObject(angle,startPoint,endPoint);
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
    on_actionShape_triggered();
    ui->drawingWindow->setSubshapeSelection(false);
    ui->drawingWindow->setSetToPlane(false);
    isIntegrationPath=false;

    // refresh the selection to enable further operations on the selected items
    ui->drawingWindow->refreshSelectedItems();

    ui->drawingWindow->updateViewer();
    setMenusI(0);
}

void OpenParEMg::on_actionUndo_triggered ()
{
    std::cout << "OpenParEMg::on_actionUndo_triggered" << std::endl; std::cout.flush();

    itemChangesStack.readNew();
    BaseItem *baseItem=itemChangesStack.getItem();
    while (baseItem) {
        std::cout << "*** undo item:" << std::endl; std::cout.flush();
        baseItem->print_itemType();

        baseItem->undo();

        baseItem=itemChangesStack.getItem();
    }

    itemChangesStack.undo();
    ui->drawingWindow->updateViewer();
    setMenusI(3000);
}

void OpenParEMg::on_actionRedo_triggered ()
{
    std::cout << "OpenParEMg::on_actionRedo_triggered" << std::endl; std::cout.flush();

    itemChangesStack.redo();
    itemChangesStack.readNew();
    BaseItem *baseItem=itemChangesStack.getItem();
    while (baseItem) {
        std::cout << "**** redo item:" << std::endl; std::cout.flush();
        baseItem->print_itemType();

        baseItem->redo();

        baseItem=itemChangesStack.getItem();
    }

    ui->drawingWindow->updateViewer();
    setMenusI(3000);
}

void OpenParEMg::updateLogTab (bool force)
{
    // default log file name used throughout
    QString logFile=projData.project_name;
    logFile.append(".log");

    // open the file
    QFile file(logFile);
    if (!file.exists()) return;
    if (!QFileInfo(logFile).isFile()) return;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    // get new data that has been added to the file
    file.seek(logLastPos);
    QByteArray newData=file.readAll();
    logLastPos=file.pos();
    if (newData.isEmpty()) return;

    // test for a quirk of back-to-back line returns due to block boundaries when loading
    if (logLastChar == '\n' && newData[0] == '\n') newData.removeFirst();
    logLastChar=newData[newData.size()-1];

    // check for skip or force
    if (!force && logFilter->skipLoad) return;

    // load the new data
    // set the cursor so the user can scroll up to view prior data

    QTextCursor cursor=ui->logText->textCursor();
    QTextCursor oldCursor=cursor;

    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QString::fromUtf8(newData));

    if (logFilter->followTail) {
        ui->logText->setTextCursor(cursor);
    } else {
        ui->logText->setTextCursor(oldCursor);
    }
}

void OpenParEMg::updateIterationsTab (bool force)
{
    // default iterations file name used throughout
    QString iterationsFile=projData.project_name;
    iterationsFile.append("_iterations.txt");

    // open the file
    QFile file(iterationsFile);
    if (!file.exists()) return;
    if (!QFileInfo(iterationsFile).isFile()) return;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    // get new data that has been added to the file
    file.seek(iterationLastPos);
    QByteArray newData=file.readAll();
    iterationLastPos=file.pos();
    if (newData.isEmpty()) return;

    // test for a quirk of back-to-back line returns due to block boundaries when loading
    if (iterationsLastChar == '\n' && newData[0] == '\n') newData.removeFirst();
    iterationsLastChar=newData[newData.size()-1];

    // check for skip or force
    if (!force && iterationsFilter->skipLoad) return;

    // load the new data
    // set the cursor so the user can scroll up to view prior data

    QTextCursor cursor=ui->iterationsText->textCursor();
    QTextCursor oldCursor=cursor;

    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QString::fromUtf8(newData));

    if (iterationsFilter->followTail) {
        ui->iterationsText->setTextCursor(cursor);
    } else {
        ui->iterationsText->setTextCursor(oldCursor);
    }
}

// void OpenParEMg::updateDataTab (bool force)
// {
//     // default data csv file name used throughout
//     QString dataFile=projData.project_name;
//     dataFile.append("_results.csv");

//     // open the file
//     QFile file(dataFile);
//     if (!file.exists()) return;
//     if (!QFileInfo(dataFile).isFile()) return;
//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

//     // get new data that has been added to the file
//     file.seek(dataLastPos);
//     QByteArray newData=file.readAll();
//     dataLastPos=file.pos();
//     if (newData.isEmpty()) return;

//     // test for a quirk of back-to-back line returns due to block boundaries when loading
//     if (dataLastChar == '\n' && newData[0] == '\n') newData.removeFirst();
//     dataLastChar=newData[newData.size()-1];

//     // check for skip or force
//     if (!force && dataFilter->skipLoad) return;

//     // load the new data
//     // set the cursor so the user can scroll up to view prior data

//     QTextCursor cursor=ui->dataText->textCursor();
//     QTextCursor oldCursor=cursor;

//     cursor.movePosition(QTextCursor::End);
//     cursor.insertText(QString::fromUtf8(newData));

//     if (dataFilter->followTail) {
//         ui->dataText->setTextCursor(cursor);
//     } else {
//         ui->dataText->setTextCursor(oldCursor);
//     }
// }

// updateTextKeepScroll is courtesy of Google AI
void updateTextKeepScroll(QPlainTextEdit *editor, const QString &newText) {
    // 1. Save the current scroll position
    QScrollBar *scrollbar = editor->verticalScrollBar();
    int previousScrollValue = scrollbar->value();

    // 2. Update the content
    editor->setPlainText(newText);

    // 3. Force Qt to process the layout so the scrollbar max value updates
    editor->document()->documentLayout()->update();

    // 4. Restore the scroll position
    scrollbar->setValue(previousScrollValue);
}

// change from the other tabs since new data can appear in the middle of the text block
void OpenParEMg::updateDataTab (bool force)
{
    // default data csv file name used throughout
    QString dataFile=projData.project_name;
    dataFile.append("_results.csv");

    // open the file
    QFile file(dataFile);
    if (!file.exists()) return;
    if (!QFileInfo(dataFile).isFile()) return;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    // see if new data that has been added to the file
    file.seek(dataLastPos);
    QByteArray newData=file.readAll();
    dataLastPos=file.pos();
    if (newData.isEmpty()) return;

    // load all of the data because new data may appear in the middle
    file.seek(0);
    newData=file.readAll();
    dataLastPos=file.pos();

    // check for skip or force
    if (!force && dataFilter->skipLoad) return;

    // update the text
    updateTextKeepScroll(ui->dataText,QString::fromUtf8(newData));
}

void OpenParEMg::updateAntennaTab (bool force)
{
    // default data csv file name used throughout
    QString antennaFile=projData.project_name;
    antennaFile.append("_FarField_results.csv");

    // open the file
    QFile file(antennaFile);
    if (!file.exists()) return;
    if (!QFileInfo(antennaFile).isFile()) return;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    // see if new data that has been added to the file
    file.seek(antennaLastPos);
    QByteArray newData=file.readAll();
    antennaLastPos=file.pos();
    if (newData.isEmpty()) return;

    // load all of the data because new data may appear in the middle
    file.seek(0);
    newData=file.readAll();
    antennaLastPos=file.pos();

    // check for skip or force
    if (!force && antennaFilter->skipLoad) return;

    // update the text
    updateTextKeepScroll(ui->antennaText,QString::fromUtf8(newData));
}

// end of file
