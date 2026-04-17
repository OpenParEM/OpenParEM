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

QT_BEGIN_NAMESPACE
namespace Ui {
class OpenParEMg;
}
QT_END_NAMESPACE

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

public:
    OpenParEMg (QWidget *parent = nullptr);
    ~OpenParEMg ();

    void saveProject ();
    //void addChildDisplayShape (CustomTreeWidgetItem *, std::pair<int,int> &dimTag);
    void addRootDisplayShape (TopoDS_Shape);
    void addItemWithShape (CustomTreeWidgetItem *);
    CustomTreeWidgetItem* addItemShape (TopoDS_Shape, CustomTreeWidgetItem *);
    CustomTreeWidgetItem* addItemShape (Polywire *, CustomTreeWidgetItem *);
    bool loadBrepFile (QString, bool);
    bool loadStepFile (QString, bool);
    bool saveBrepFile (char *);
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
                  long unsigned int &endBlockIndex, CustomTreeWidgetItem *);
    bool saveDrawingFile (QString);
    bool loadDrawingFile ();
    //bool menuAllHidden (CustomTreeWidgetItem *);
    //bool menuAllShown (CustomTreeWidgetItem *);
    //void meshShowEntities ();
    //void meshHideEntities ();
    void drawMesh ();
    void deleteMesh (bool);

    bool isRootDrawingValidShow ();
    bool isRootDrawingValidHide ();


    bool isRootDrawingValidSelectAll ();

    bool hasSelectedPaths ();
    void clearTreeSelection ();

    void dumpDrawingEntities ();
    void shapeCount (TopoDS_Shape, int *);
    void resetDimTag (CustomTreeWidgetItem *);
    void renumberDimTag ();
    void setPhysicalGroups ();
    void setMaterials ();

    void clearSelection ();
    void restoreSelection ();

    void rebuildTopLevelShape ();
    void reprocess (CustomTreeWidgetItem *);

    void startOperation (bool);

    bool isValidAssignMaterial ();
    bool isValidObjectShow ();
    bool isValidObjectHide ();
    bool isValidObjectDelete ();

    bool isValidSetPlane ();
    void setPlaneToFace ();
    void setPlaneToFaceAxis ();

    bool isValidCreatePort ();
    bool isValidCreatePath ();

    bool isValidObjectMove ();
    bool isValidRotateObject ();

    bool isValidObjectEdit ();

    bool isValidObjectStretch ();

    bool isValidCopy ();
    CustomTreeWidgetItem * copyItem (CustomTreeWidgetItem *, CustomTreeWidgetItem *);
    void copyDrawingItems ();

    bool isValidDeletePoint ();
    void finishDeletePoint (CustomTreeWidgetItem *);

    bool isValidInsertPoint ();
    void finishInsertPoint (CustomTreeWidgetItem *);

    bool isValidCloseExistingPolyline ();
    void closeExistingPolyline ();

    bool isValidOpenExistingPolyline ();
    void openExistingPolyline ();

    bool isValidConvertToPolyline ();
    void convertToPolyline ();

    bool isValidConvertToPath ();
    void convertToPath ();

    bool isValidExtrudePolywire ();
    void reextrudePolywire (CustomTreeWidgetItem *, CustomTreeWidgetItem *);
    void finishExtrudePolywire (bool);

    void finishMoveObject (CustomTreeWidgetItem *, gp_Pnt p0, gp_Pnt p1, bool);
    void finishMoveObject (CustomTreeWidgetItem *, gp_Pnt p0, gp_Pnt p1);

    void finishStretchObject (CustomTreeWidgetItem *);

    void finishRotateObject (CustomTreeWidgetItem *);
    void finishRotateObject ();

    bool isValidMergeSolids ();
    void finishMergeSolids ();

    bool isValidSubtractSolids ();
    void finishSubtractSolids ();

    void finishPlaneSetToFace ();

    void debugPrintStats (int);

private slots:
    // File
    void on_actionNew_triggered ();
    void on_actionOpen_triggered ();
    void on_actionSave_triggered ();
    void on_actionSaveAs_triggered();
    void on_actionClose_triggered();
    void on_actionImportBrep_triggered ();
    void on_actionImportStep_triggered();
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
    //void finishSelect ();
    void on_actionSelectEdge_triggered ();
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

    void selectAllRootDrawingItems ();
    void on_drawingItemTree_itemClicked(QTreeWidgetItem *item, int column);
    void expand (CustomTreeWidgetItem *);
    void collapse (CustomTreeWidgetItem *);
    void expandAllItems ();
    void collapseAllItems ();
    bool isDrawingValidShow ();
    void showRootDrawingItems ();
    void showDrawingItems ();
    void hideRootDrawingItems ();
    void hideDrawingItems ();
    void renamePathItems ();
    void deletePathItems ();
    void showRootPathItems ();
    bool isPathValidDelete ();
    bool isValidShowPath ();
    void showPathItems ();
    bool rootPathValidShow ();
    void hideRootPathItems ();
    bool isValidHidePath ();
    void hidePathItems ();
    bool rootPathValidHide ();
    void showRootPortItems ();
    void showPortItems ();
    void hideRootPortItems ();
    void hidePortItems ();
    void showRootMeshItems ();
    void showMeshItems ();
    bool rootMeshValidShow ();
    void hideRootMeshItems ();
    void hideMeshItems ();
    bool rootMeshValidHide ();
    //void addSportNet ();
    void renameSportNet ();
    bool deleteSportValid ();
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
    //void showDisplayShape (CustomTreeWidgetItem *);
    //void showPortShape (CustomTreeWidgetItem *);
    //void hideDisplayShape (CustomTreeWidgetItem *);
    //void hidePortShape (CustomTreeWidgetItem *);
    //void selectDisplayShape (CustomTreeWidgetItem *);
    //void unselectDisplayShape (CustomTreeWidgetItem *);

    void assignMaterial ();

    void cancelDrawingMenu ();
    void buildDrawingMenu (QMenu &menu);
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

    //void set_displayMode (CustomTreeWidgetItem *, int);
    //void set_selectionMode (CustomTreeWidgetItem *, int);

    void editObject ();
    void moveObject ();
    void stretchObject ();
    void deletePoint ();
    void insertPoint ();
    void rotateObject ();
    void createPort ();
    void createPath ();
    void replaceItemShape (CustomTreeWidgetItem *, TopoDS_Shape &shape, int);
    void replaceItemShape (CustomTreeWidgetItem *, Polywire *, int);
    void extrudePolywire ();
    void mergeSolids ();
    void subtractSolids ();

    void loadMeshFile (QString);
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
    bool insertActionValid ();
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

    void on_actionSelectWire_triggered();

public slots:
    void setMenus ();
    void setMenusI (int);
    void finishEditObject (bool);
    void startPlaneSetToFace ();

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

    CustomTreeWidgetItem drawing;
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
    QAction *createPortAction;
    QAction *createPathAction;
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
    bool disableMenus;
    bool isIntegrationPath;
    bool restrictToDrawingPlane;
    bool skipDrawingPlaneAxisForm;
    gp_Pnt lastMousePosition;
    gp_Vec extrusionDirection;   // use (0,0,0) as the "unset" state
    Polywire *activePolywire;  // when drawing
    gp_Vec uLocalAxis;  // local axis for transfer to rectangles
    gp_Pln currentPrivilegedPlane;
    ObjectCounts objectCounts;    // for uniquely numbering objects in their item names
    double length;                // extrusion
    double angle;                 // rotation
    gp_Pnt startPoint, endPoint;  // rotation and vector input
};

#endif // OPEMG_H
