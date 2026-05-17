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

#ifndef OPEMG_H
#define OPEMG_H

#include <Standard_Handle.hxx>

#include <QMainWindow>
#include <QStyledItemDelegate>
#include <QTimer>
#include <qplaintextedit.h>
#include <qscrollbar.h>
#include <qtextobject.h>

#include "LengthInputForm.h"
#include "LineEditForm.h"
#include "RotateInputForm.h"
#include "RectangleEditForm.h"
#include "PolycircleEditForm.h"
#include "Relay.h"
#include "CustomLineEdit.h"
#include "VectorInputForm.h"
#include "project.h"
#include "OpenParEMmaterials.hpp"
#include "port.hpp"
#include "CustomTreeWidgetItem.h"
#include "gmsh.h"
#include "ObjectCounts.h"


extern "C" void init_project (struct projectData *);
extern "C" void free_project (struct projectData *);
extern "C" PetscErrorCode load_project_file (const char *, struct projectData *, const char *);
extern "C" void set_project_name (struct projectData *, const char *);
extern "C" int save_project (const char *, struct projectData *, struct projectData *, const char *);
extern "C" void clear_physicalGroupMaterials (struct projectData *);
extern "C" void add_physicalGroupMaterial (struct projectData *, int, int, int, char *);

class LogViewerFilter : public QObject
{
    Q_OBJECT

public:
    bool followTail=true;
    bool skipLoad=false;

    LogViewerFilter(QObject *parent = nullptr)
        : QObject(parent) {}

    void setTextEdit (QPlainTextEdit *textEdit_) {textEdit=textEdit_;}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress ||
            event->type() == QEvent::MouseButtonPress)
        {
            skipLoad=true;
        }

        if (event->type() == QEvent::Wheel ||
            event->type() == QEvent::KeyRelease ||
            event->type() == QEvent::MouseButtonRelease)
        {
            skipLoad=false;

            QScrollBar *scrollBar=textEdit->verticalScrollBar();
            bool atBottom=(scrollBar->value() >= scrollBar->maximum() - 2);

            int topBlock = scrollBar->value();
            QTextBlock block = textEdit->document()->findBlockByLineNumber(topBlock);

            QTextCursor cursor(block);
            textEdit->setTextCursor(cursor);

            followTail=false;
            if (atBottom) followTail=true;
        }

        return QObject::eventFilter(obj, event);
    }

private:
    QPlainTextEdit *textEdit;
};

QT_BEGIN_NAMESPACE
namespace Ui {
class OpenParEMg;
}
QT_END_NAMESPACE

// list of items for a change in a single operation, such as a single edit or a multi-select move
class ItemChanges
{
public:
    long unsigned int getChangeListSize () {return changeList.size();}
    CustomTreeWidgetItem* getItem (long unsigned int i) {return changeList[i];}
    void push_back (CustomTreeWidgetItem *item) {changeList.push_back(item);}
    void clear () {changeList.clear();}
    void setPrior (ItemChanges *prior_) {prior=prior_;}
    void setNext (ItemChanges *next_) {next=next_;}
    ItemChanges *getPrior () {return prior;}
    ItemChanges *getNext () {return next;}
    void print ()
    {
        long unsigned int i=0;
        while (i < changeList.size()) {
            std::cout << "         item=" << changeList[i]
                      //<< "  type=" << changeList[i]->getShapeData()->getType()
                      << "  prior=" << prior
                      << "  next=" << next
                      << std::endl;
            //changeList[i]->print();
            i++;
        }
        std::cout.flush();
    }
private:
    std::vector<CustomTreeWidgetItem *> changeList;  // change list for a single operation
    ItemChanges *prior;                              // prior item in ItemChangesStack
    ItemChanges *next;                               // next item in ItemChangesStack
};

// list of ItemChanges, where each change is a different operation
class ItemChangesStack
{
public:
    ItemChangesStack ()
    {
        current=nullptr;
    }

    void startNew ()
    {
        ItemChanges *itemChanges=new ItemChanges();
        itemChangesList.push_back(itemChanges);
        if (current) {
            itemChanges->setPrior(current);
            current->setNext(itemChanges);
        }
        current=itemChanges;
    }

    void readNew () {readIndex=0;}

    void add (CustomTreeWidgetItem *item)
    {
        current->push_back(item);
    }

    bool hasUndo ()
    {
        if (current) return true;
        return false;
    }

    bool hasRedo ()
    {
        if (current) {
            if (current->getNext()) return true;
        } else {
            if (itemChangesList.size() > 0) return true;
        }

        return false;
    }

    void undo ()
    {
        if (current && hasUndo()) current=current->getPrior();
        readIndex=0;
    }

    void redo ()
    {
        if (current) {
            if (current->getNext()) {
                current=current->getNext();
            }
        } else {
            if (itemChangesList.size() > 0) {
                current=itemChangesList[0];
            }
        }
        readIndex=0;
    }

    CustomTreeWidgetItem* getItem ()
    {
        if (!current) return nullptr;

        if (readIndex < current->getChangeListSize()) {
            CustomTreeWidgetItem *item=current->getItem(readIndex);
            readIndex++;
            return item;
        }
        return nullptr;
    }

    void clear ()
    {
        long unsigned int i=0;
        while (i < itemChangesList.size()) {
            if (itemChangesList[i]) {
                itemChangesList[i]->clear();
                delete itemChangesList[i];
            }
            i++;
        }
        itemChangesList.clear();
        current=nullptr;
        readIndex=0;
    }

    ItemChanges* getCurrentNext ()
    {
        if (current) return current->getNext();
        return nullptr;
    }

    void setCurrentNext (ItemChanges *next)
    {
        if (current) current->setNext(next);
    }

    void pop_back ()
    {
        if (itemChangesList.size() == 0) return;

        // item to delete
        ItemChanges *toDelete=itemChangesList[itemChangesList.size()-1];

        // reset current
        current=toDelete->getPrior();

        // reset next
        long unsigned int i=0;
        while (i < itemChangesList.size()) {
            if (itemChangesList[i]->getNext() == toDelete) {
                itemChangesList[i]->setNext(nullptr);
            }
            i++;
        }

        // remove
        if (itemChangesList.size() > 0) itemChangesList.pop_back();
    }

    void print ()
    {
        std::cout << "ItemChangesStack:" << std::endl;
        long unsigned int i=0;
        while (i < itemChangesList.size()) {
            if (itemChangesList[i]) {
                std::cout << "   ItemChanges: " << itemChangesList[i] << std::endl;
                itemChangesList[i]->print();
            }
            i++;
        }
        std::cout << "   current=" << current << std::endl;
    }

private:
    std::vector<ItemChanges *> itemChangesList;   // list of operations, where each one might be single- or multi- select operations
    ItemChanges *current;                         // current location
    long unsigned int readIndex;                  // index for reading out of itemChangesList
};

class CustomStyledItemDelegate : public QStyledItemDelegate
{
public:
    CustomStyledItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);

        if (option->state & QStyle::State_Selected) {
            //QVariant backgroundData = index.data(Qt::BackgroundRole);
            //if (backgroundData.canConvert<QBrush>()) {
            //    option->palette.setBrush(QPalette::Highlight, qvariant_cast<QBrush>(backgroundData));
            //} else {
            //    option->palette.setBrush(QPalette::Highlight, option->palette.brush(QPalette::Base));
            //}

            // keep the original text color
            QVariant foregroundData=index.data(Qt::ForegroundRole);
            if (foregroundData.canConvert<QBrush>()) {
                option->palette.setBrush(QPalette::HighlightedText, qvariant_cast<QBrush>(foregroundData));
            } else {
                option->palette.setBrush(QPalette::HighlightedText, option->palette.brush(QPalette::Text));
            }
        }
    }
};

class OpenParEMg : public QMainWindow
{
    Q_OBJECT

    friend class RootDrawingItem;
    friend class DrawingItem;

public:
    OpenParEMg (QWidget *parent = nullptr);
    ~OpenParEMg ();

    void closeEvent (QCloseEvent *event) override {
        close_event=event;
        closeWindow_triggered();
    }

    int check_changed ();
    void closeWindow_triggered ();

    void saveProject ();
    void insertToMapActivateItem (CustomTreeWidgetItem *);
    QString getAISshapeName (Handle(AIS_Shape));
    bool loadBrepFile (QString, bool);
    bool loadStepFile (QString, bool);
    bool isValidSaveBrepFile ();
    bool saveBrepFile (QString);
    bool isValidSaveStepFile ();
    bool saveStepFile (QString);
    bool saveBoundaryDatabase ();
    void increase_depth (CustomTreeWidgetItem *);
    void decrease_depth (CustomTreeWidgetItem *);
    void saveItem (std::ofstream *, CustomTreeWidgetItem *);

    int isStartBlock (std::vector<std::string> &inputData, long unsigned int);
    int isEndBlock (std::vector<std::string> &inputData, long unsigned int);
    bool getBlockKeywordValue (std::vector<std::string> &inputData, int,
                              long unsigned int &startBlockIndex, long unsigned int &endBlockIndex,
                              std::string &keyword, std::string &value);
    int findStartNextBlock (std::vector<std::string> &inputData, long unsigned int &startBlockIndex);
    int findEndNextBlock (std::vector<std::string> &inputData, int, long unsigned int &endBlockIndex);
    bool loadItem (std::vector<std::string> &inputData, long unsigned int &startBlockIndex,
                  long unsigned int &endBlockIndex, CustomTreeWidgetItem *, bool);
    bool saveDrawingFile (QString);
    bool loadDrawingFile ();
    void drawMesh ();
    void deleteMesh (bool);

    bool isValidRootDrawingShow ();
    bool isValidRootDrawingHide ();


    bool isValidRootDrawingSelectAll ();

    bool hasSelectedPaths ();
    void clearTreeSelection ();

    void dumpDrawingEntities ();
    void shapeCount (TopoDS_Shape, int *);
    void renumberDimTag ();
    void setPhysicalGroups ();
    void setMaterials ();

    void clearSelection ();
    void restoreSelection ();

    void reprocess (CustomTreeWidgetItem *);

    void startOperation (bool);

    bool isValidAssignMaterial ();
    bool isValidObjectShow ();
    bool isValidObjectHide ();
    bool isValidObjectDelete ();

    bool isValidSetPlane ();
    void setPlaneToFace ();
    void setPlaneToFaceAxis ();

    bool isValidCreatePath ();
    bool isValidCreatePortFromFace ();
    bool isValidCreatePortFromPath ();
    bool isValidReversePath ();

    bool isValidCreateBoundaryFromFace ();
    bool isValidCreateBoundaryFromPath ();

    bool isValidConvertToPort ();
    bool isValidConvertToBoundary ();

    bool isValidObjectMove ();
    bool isValidRotateObject ();

    bool isValidObjectEdit ();

    bool isValidObjectStretch ();

    bool isValidCopy ();
    CustomTreeWidgetItem * copyItem (CustomTreeWidgetItem *, CustomTreeWidgetItem *);
    void copyDrawingItems ();

    bool isValidDeletePoint ();
    void finishDeletePoint (DrawingItem *);
    void cancelDeletePoint ();

    bool isValidInsertPoint ();
    void finishInsertPoint (DrawingItem *);
    void finishStretchPoint (DrawingItem *);
    void cancelInsertPoint ();

    bool isValidCloseExistingPolyline ();
    void closeExistingPolyline ();

    bool isValidOpenExistingPolyline ();
    void openExistingPolyline ();

    bool isValidConvertToPolyline ();
    void convertToPolyline ();

    bool isValidConvertToPath ();
    void convertItemToPath (CustomTreeWidgetItem *, bool);
    void convertToPath ();

    bool isValidExtrudePolywire ();
    void finishExtrudePolywire (bool);

    void finishMoveObject (DrawingItem *, gp_Pnt p0, gp_Pnt p1, bool);
    void finishMoveObject (DrawingItem *, gp_Pnt p0, gp_Pnt p1);

    void finishStretchObject (DrawingItem *);

    void finishRotateObject (DrawingItem *);
    void finishRotateObject ();

    bool isValidMergeSolids ();
    void finishMergeSolids ();

    bool isValidSubtractSolids ();
    void finishSubtractSolids ();

    void finishPlaneSetToFace ();

    void debugPrintStats (int);

    void undoItem (CustomTreeWidgetItem *);
    void redoItem (CustomTreeWidgetItem *);

    void setScale ();

private slots:
    // File
    void on_actionNew_triggered ();
    void on_actionOpen_triggered ();
    void on_actionSave_triggered ();
    void on_actionSaveAs_triggered();
    void on_actionClose_triggered();
    void on_actionImportBrep_triggered ();
    void on_actionImportStep_triggered();
    void on_actionExportBrep_triggered ();
    void on_actionExportStep_triggered();
    void on_actionExit_triggered ();

    // View
    void on_actionFitSelected_triggered ();
    void on_actionFitAll_triggered ();
    void on_actionUnselectAll_triggered ();   // note similar functionality in CustomOpenGLWidget
    void on_actionShowAll_triggered ();
    void on_actionHideAll_triggered ();
    void on_actionWireframe_triggered();

    // Select
    void on_actionSelectFace_triggered ();
    void on_actionSelectWithBox2_triggered ();

    // Drawing
    void on_actionDrawingPlaneShow_triggered ();
    void on_actionDrawingPlaneHide_triggered ();
    void on_actionDrawingPlaneSnapToGrid_triggered ();

    // Drawing plane
    void on_actionDrawingPlaneSetToFace_triggered ();
    void on_actionDrawingPlaneSetToFaceAxis_triggered ();
    void on_actionDrawingSetPlaneToXY_triggered ();
    void on_actionDrawingSetPlaneToXZ_triggered ();
    void on_actionDrawingSetPlaneToYZ_triggered ();

    // Drawing preferences
    void on_actionPreferences_triggered ();

    // Materials
    void on_actionSelectMaterialsDatabase_triggered ();

    // Mesh
    void on_actionMeshOptions_triggered ();
    void on_actionMeshGenerate_triggered ();
    void on_actionMeshLoad_triggered ();
    void on_actionMeshSave_triggered ();
    void on_actionMeshSaveAs_triggered ();
    void on_actionMeshDelete_triggered ();

    // Simulation Setup
    void on_actionFrequencyPlan_triggered ();
    void on_actionRefinement_triggered ();
    void on_actionSimulateOptions_triggered ();

    // Run
    void on_actionRun_triggered ();
    void on_actionStop_triggered();
    void on_actionAbort_triggered();
    void on_actionAbortAndExit_triggered();

    // Tools
    void on_actionMaterialsEditor_triggered ();

    // Help
    void on_actionAbout_triggered ();
    void on_actionLicense_triggered ();

    // Functionality

    void rootDrawingSelectAll ();
    void on_drawingItemTree_itemClicked(QTreeWidgetItem *item, int column);
    void expand (CustomTreeWidgetItem *);
    void collapse (CustomTreeWidgetItem *);
    void expandAllItems ();
    void collapseAllItems ();
    bool isValidDrawingShow ();
    void rootDrawingShow ();
    void showDrawingItems ();
    void rootDrawingHide ();
    void hideDrawingItems ();
    void renamePathItems ();
    void deletePathItems ();
    void showRootPathItems ();
    bool isValidDeletePath ();
    bool isValidShowPath ();
    void showPathItems ();
    bool isValidRootPathShow ();
    void hideRootPathItems ();
    bool isValidHidePath ();
    void hidePathItems ();
    bool isValidRootPathHide ();
    void rootPortHide ();
    bool isValidRootPortShow ();
    void rootPortShow ();
    void showPortItems ();
    bool isValidRootPortHide ();
    void hidePortItems ();

    //xxx
    bool isValidRootBoundaryShow ();
    void rootBoundaryShow ();
    bool isValidRootBoundaryHide ();
    void rootBoundaryHide ();
    void showBoundaryItems ();
    void hideBoundaryItems ();
    void unselectBoundaryItems ();
    void renameBoundaryItems ();
    void deleteBoundaryItems ();
    void deleteBoundaryItem (CustomTreeWidgetItem *);




    void showRootMeshItems ();
    void showMeshItems ();
    bool isValidRootMeshShow ();
    void hideRootMeshItems ();
    void hideMeshItems ();
    bool isValidRootMeshHide ();
    //void addSportNet ();
    void renameSportNet ();
    bool isValidDeleteValid ();
    void deleteSportItem (CustomTreeWidgetItem *);
    void deleteSportItems ();
    void rename_returnPressed ();
    bool hasOneSelectedSport ();
    bool hasVoltage ();
    bool hasCurrent ();
    void insertPath (CustomTreeWidgetItem *);
    //void selectItems ();
    bool isValidRenameDrawingItems ();
    void renameDrawingItems ();
    void unselectRootDrawingItems ();
    void unselectDrawingItems ();
    void deleteDrawingItems ();
    void insertModeItems ();
    void unselectPortItems ();
    void renamePortItems ();
    void deletePortItem (CustomTreeWidgetItem *);
    void deleteRootPortItems ();
    void deletePortItems ();
    void showNetItems ();
    void hideNetItems ();
    void showVIItems ();
    void hideVIItems ();
    void removeIntegrationPathItems ();
    void showIntegrationPathItems ();
    void hideIntegrationPathItems ();

    void assignMaterial ();

    void cancelDrawingMenu ();
    void cancelPathMenu ();
    void buildPathMenu (QMenu &menu);
    void itemTreeContextMenu_triggered (const QPoint& pnt);
    void drawingWindowContextMenu_triggered (const QPoint& pnt);

    void on_actionShape_triggered ();
    void on_actionVertex_triggered ();
    void on_actionEdge_triggered ();
    void on_actionWire_triggered ();
    void on_actionFace_triggered ();
    void on_actionShell_triggered ();
    void on_actionSolid_triggered ();

    bool eventFilter (QObject *, QEvent *) override;
    void keyPressEvent (QKeyEvent *) override;
    void keyReleaseEvent (QKeyEvent *) override;

    void findShowTopLevelItem (CustomTreeWidgetItem *, bool);

    void editObject ();
    void moveObject ();
    void stretchObject ();
    void deletePoint ();
    void insertPoint ();
    void rotateObject ();
    void createPath ();
    void createPortFromFace ();
    void createPortFromPath ();
    void createBoundaryFromFace ();
    void createBoundaryFromPath ();
    void convertItemToPort (CustomTreeWidgetItem *, bool);
    void convertToPort ();
    void convertItemToBoundary (CustomTreeWidgetItem *, bool);
    void convertToBoundary ();
    void reversePathItem (CustomTreeWidgetItem *, bool);
    void reversePathItems ();
    double getConversionFactor ();
    void extrudePolywire ();
    void mergeSolids ();
    void subtractSolids ();

    void loadMeshFile (QString);
    void updateLogTab (bool);
    void updateIterationsTab (bool);
    void updateDataTab (bool);
    void checkFinish ();

    void cancelDraw ();
    void on_actionDrawLine_triggered ();
    void on_actionDrawPolyline_triggered ();
    void on_actionDrawPolycircle_triggered ();
    void on_actionDrawRectangle_triggered ();
    void finishDraw ();
    void drawPath ();
    void drawLinePath ();
    void drawPolylinePath ();
    bool isValidInsertAction ();
    void insertSelectedPath ();
    void finishPolyline ();
    void deleteLastPoint ();
    void closePolyline ();

    void initQActionList();
    void freeQActionList();

    void on_actionMaterialsOptions_triggered();

    void getCurrentMousePosition (gp_Pnt);
    void finishOperation (bool, int);
    void getPickedVertex (gp_Pnt, bool);

    void on_actionUndo_triggered ();
    void on_actionRedo_triggered ();

public slots:
    void setMenus ();
    void setMenusI (int);
    void finishEditObject (bool);
    void startPlaneSetToFace ();
    void updateViewer ();
    void setShaded (Handle(AIS_Shape));
    void convertPathToFace (CustomTreeWidgetItem *);

signals:
    void sendPnt (gp_Pnt);

private:
    //void setMenus ();
    void resetLockouts ();
    void printLockouts ();
    void resetDrawing ();
    void resetProject ();
    //void getActionRunSetup (bool *, QString *);

    Ui::OpenParEMg *ui;
    QString absolutePath;  // to the project
    QString projectFile;   // projectFile without path but with extension
    QString projectName;   // projectFile without path and without extension
    struct projectData projData,defaultData;

    MaterialDatabase *materialDatabase;
    QString selectedMaterial;
    BoundaryDatabase *boundaryDatabase;

    RootDrawingItem drawing;
    CustomTreeWidgetItem path;
    CustomTreeWidgetItem port;
    CustomTreeWidgetItem boundary;
    CustomTreeWidgetItem mesh;

    CustomTreeWidgetItem *clickedItem,*previousClickedItem,*workingItem;
    int previousSelectionIndex;
    bool CTRLpressed;
    bool SHIFTpressed;

    QMenu *drawingContextMenu;

    MPI_Comm *MPI_PORT_COMM;
    MPI_Request *request;
    int signal;
    QTimer *timer;

    QAction *showAction;
    QAction *hideAction;
    QAction *selectAllAction;
    QAction *unselectAction;
    QAction *copyAction;
    QAction *deleteAction;
    QAction *deletePointAction;
    QAction *insertPointAction;
    QAction *closePolylineAction;
    QAction *openPolylineAction;
    QAction *removeAction;
    QAction *assignAction;
    QAction *insertAction;
    QAction *renameAction;
    QAction *expandAllAction;
    QAction *collapseAllAction;
    QAction *createPathAction;
    QAction *createPortAction;
    QAction *reversePathAction;
    QAction *createBoundaryAction;
    QAction *drawPathAction;
    QAction *drawPolylineAction;
    QAction *drawPolycircleAction;
    QAction *editAction;
    QAction *moveAction;
    QAction *stretchAction;
    QAction *rotateAction;
    QAction *doneAction;
    QAction *cancelAction;
    QAction *deleteLastPointAction;
    QAction *closeAction;
    QAction *extrudeAction;
    QAction *mergeAction;
    QAction *subtractAction;
    QAction *convertToPolylineAction;
    QAction *convertToPathAction;
    QAction *convertToPortAction;
    QAction *convertToBoundaryAction;
    QAction *setPlaneAction;
    QAction *setPlaneAxisAction;
    QAction *assignMaterialAction;
    std::vector<QAction *> QActionList;

    // gmsh
    gmsh::vectorpair drawingEntities;  // only used for debug

    // lockouts
    bool projectFileLoaded;  // project setup variable including paths
    bool projectChanged;
    // bool brepFileLoaded;     // drawing (brep or step)
    // bool brepChanged;
    bool drawingChanged;
    //bool meshFileLoaded;     // mesh
    bool meshChanged;
    bool drawingPlaneShown;
    bool simulationRunning;
    bool simulationStopping;
    bool simulationAborting;

    // rename
    QString originalText;
    CustomLineEdit *renameEdit;
    CustomTreeWidgetItem *renameItem;

    // relay
    Relay *relay;

    // forms
    LengthInputForm *lengthInputForm;
    VectorInputForm *vectorInputForm;
    LengthInputForm *lengthEditForm;  // same form as for lengthInputForm
    LineEditForm *lineEditForm;
    RectangleEditForm *rectangleEditForm;
    PolycircleEditForm *polycircleEditForm;
    RotateInputForm *rotateInputForm;

    // drawing
    DrawingItem *currentDrawingItem;
    bool disableMenus;
    bool isIntegrationPath;
    QString integrationPathName;
    bool restrictToDrawingPlane;
    bool skipDrawingPlaneAxisForm;
    gp_Pnt lastMousePosition;
    gp_Vec extrusionDirection;   // use (0,0,0) as the "unset" state
    Polywire *activePolywire;  // when drawing
    gp_Vec uLocalAxis;  // local axis for transfer to rectangles
    gp_Pln currentPrivilegedPlane;
    ObjectCounts objectCounts;    // for uniquely numbering objects in their item names

    // for edit forms
    Line *lineEdit;               // for undo/redo
    Rectangle *rectangleEdit;     // for undo/redo
    Polycircle *polycircleEdit;   // for undo/redo
    double length;                // extrusion
    double angle;                 // rotation
    gp_Pnt startPoint, endPoint;  // rotation and vector input

    // for undo/redo
    bool activeAction;            // shows whether a current action is active, such as move, stretch, edit, etc.
    ItemChangesStack itemChangesStack;

    // for the logging tabs
    qint64 logLastPos;
    qint64 iterationLastPos;
    qint64 dataLastPos;
    LogViewerFilter *logFilter;
    LogViewerFilter *iterationsFilter;
    LogViewerFilter *dataFilter;
    QString partialLogLine;
    char logLastChar;
    char iterationsLastChar;
    char dataLastChar;

    // for closing
    QCloseEvent *close_event;
};

#endif // OPEMG_H
