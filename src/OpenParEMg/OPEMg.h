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
#include <unordered_map>

#include <QMainWindow>
#include <QStyledItemDelegate>
#include <QTimer>

#include "AIS_Shape.hxx"

#include "CustomLineEdit.h"
#include "project.h"
#include "OpenParEMmaterials.hpp"
#include "port.hpp"
#include "CustomTreeWidgetItem.h"
#include "gmsh.h"


extern "C" void init_project (struct projectData *);
extern "C" void free_project (struct projectData *);
extern "C" PetscErrorCode load_project_file (const char *, struct projectData *, const char *);
extern "C" int save_project (const char *, struct projectData *, struct projectData *, const char *);
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

    void addShape (TopoDS_Shape, CustomTreeWidgetItem *, bool);
    bool loadBrepFile (QString);
    bool loadStepFile (QString);
    bool saveStepFile (QString, std::vector<Handle(AIS_InteractiveObject)> *);
    //bool menuAllHidden (CustomTreeWidgetItem *);
    //bool menuAllShown (CustomTreeWidgetItem *);
    //void meshShowEntities ();
    //void meshHideEntities ();
    void drawMesh ();
    void deleteMesh ();

    void clearTreeSelection ();

    void dumpDrawingEntities ();
    void shapeCount (TopoDS_Shape, int *);
    void setPhysicalGroups ();

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
    void on_actionSelectWithBox_triggered();
    void on_actionUnselectAll_triggered ();   // note similar functionality in CustomOpenGLWidget
    void on_actionHideAll_triggered ();
    void on_actionWireframe_triggered();

    // Drawing
    void on_actionDrawingPlaneShow_triggered();
    void on_actionDrawingPlaneHide_triggered();
    void on_actionDrawingPlaneSnapToGrid_triggered();
    void on_actionDrawingPlaneSetToFace_triggered();

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

    void on_drawingItemTree_itemClicked(QTreeWidgetItem *item, int column);
    void expand (CustomTreeWidgetItem *);
    void collapse (CustomTreeWidgetItem *);
    void expandAllItems ();
    void collapseAllItems ();
    void showDrawingItems ();
    void hideDrawingItems ();
    void showPortItems ();
    void hidePortItems ();
    void showMeshItems ();
    void hideMeshItems ();
    //void addSportNet ();
    void renameSportNet ();
    void deleteSportNet ();
    void rename_returnPressed ();
    void selectItems ();
    void unselectDrawingItems ();
    void deleteDrawingItems ();
    void unselectPortItems ();
    void renamePortItems ();
    void deletePortItems ();
    void showNetItems ();
    void hideNetItems ();
    void showVIItems ();
    void hideVIItems ();
    void showIntegrationPathItems ();
    void hideIntegrationPathItems ();
    void setRootForeground (CustomTreeWidgetItem *);
    //void showDisplayShape (CustomTreeWidgetItem *);
    //void showPortShape (CustomTreeWidgetItem *);
    //void hideDisplayShape (CustomTreeWidgetItem *);
    //void hidePortShape (CustomTreeWidgetItem *);
    //void selectDisplayShape (CustomTreeWidgetItem *);
    //void unselectDisplayShape (CustomTreeWidgetItem *);

    void assignMaterial ();

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

    void set_displayMode (CustomTreeWidgetItem *, int);
    void set_selectionMode (CustomTreeWidgetItem *, int);

    void loadMeshFile (QString);
    void checkFinish ();

private:
    void setMenus ();
    void resetLockouts ();
    void printLockouts ();
    void resetProject ();
    //void getActionRunSetup (bool *, QString *);

    Ui::OpenParEMg *ui;
    QString projectFile;
    struct projectData projData,defaultData;

    MaterialDatabase *materialDatabase;
    QString selectedMaterial;
    BoundaryDatabase *boundaryDatabase;

    QString absolutePath;

    CustomTreeWidgetItem drawing;
    CustomTreeWidgetItem port;
    CustomTreeWidgetItem boundary;
    CustomTreeWidgetItem mesh;

    CustomTreeWidgetItem *clickedItem,*previousClickedItem;
    QAction *currentSelectionAction;
    bool CTRLpressed;
    bool SHIFTpressed;

    QMenu *drawingContextMenu;

    MPI_Comm *MPI_PORT_COMM;
    MPI_Request *request;
    int signal;
    QTimer *timer;

    QAction *showAction;
    QAction *hideAction;
    QAction *unselectAction;
    QAction *deleteAction;
    QAction *assignMaterialAction;
    QAction *addNetAction;
    QAction *renameAction;
    QAction *expandAllAction;
    QAction *collapseAllAction;

    // gmsh
    gmsh::vectorpair drawingEntities;
    int pointCount;
    int curveCount;
    int surfaceCount;
    int volumeCount;

    // lockouts
    bool projectFileLoaded;
    bool projectFileChanged;
    bool boundaryDatabaseLoaded;
    bool boundaryDatabaseChanged;
    bool meshFileLoaded;
    bool meshFileChanged;
    bool brepFileLoaded;
    bool stepFileLoaded;
    bool drawingPlaneShown;
    bool simulationRunning;
    bool simulationStopping;
    bool simulationAborting;

    // rename
    QString originalText;
    CustomLineEdit *renameEdit;
    CustomTreeWidgetItem *renameItem;

};

#endif // OPEMG_H
