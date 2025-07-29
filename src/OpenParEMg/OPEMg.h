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

#include <QMainWindow>

#include "AIS_Shape.hxx"

#include "project.h"
#include "OpenParEMmaterials.hpp"
#include "port.hpp"
#include "CustomTreeWidgetItem.h"


extern "C" void init_project (struct projectData *);
extern "C" void free_project (struct projectData *);
extern "C" PetscErrorCode load_project_file (const char *, struct projectData *, const char *);
extern "C" void print_project (struct projectData *, struct projectData *, const char *);

QT_BEGIN_NAMESPACE
namespace Ui {
class OpenParEMg;
}
QT_END_NAMESPACE

class OpenParEMg : public QMainWindow
{
    Q_OBJECT

public:
    OpenParEMg (QWidget *parent = nullptr);
    ~OpenParEMg ();

    void addShape (TopoDS_Shape, CustomTreeWidgetItem *, bool);
    bool loadBrepFile (QString);

private slots:
    void on_fileOpen_triggered ();
    void on_fileNew_triggered ();
    void on_meshOptions_triggered ();
    void on_simulateOptions_triggered ();
    void on_actionLicense_triggered ();
    void on_actionFrequency_Plan_triggered ();
    void on_actionSave_triggered ();
    void on_actionRefinement_triggered ();
    void on_actionMaterials_Editor_triggered ();
    void on_actionBrep_triggered ();
    void on_actionExit_triggered ();
    void on_actionSelect_Database_triggered ();

    void on_drawingItemTree_itemClicked(QTreeWidgetItem *item, int column);

    void showShape ();
    void grayOutTreeItems (CustomTreeWidgetItem *);
    void hideShape ();
    void selectShape ();
    void unselectItemShape (CustomTreeWidgetItem *);
    void unselectShape ();

    void contextMenu_triggered (const QPoint& pnt);

    void on_actionFit_Selected_triggered ();

    void on_actionFit_All_triggered ();

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

    void on_actionHide_All_triggered ();

    void on_actionShow_All_triggered ();

private:
    Ui::OpenParEMg *ui;
    bool hasProjData;
    QString projectFile;
    struct projectData projData,defaultData;

    MaterialDatabase *materialDatabase;
    BoundaryDatabase *boundaryDatabase;

    QString absolutePath;

    CustomTreeWidgetItem drawing;
    CustomTreeWidgetItem port;
    CustomTreeWidgetItem boundary;

    CustomTreeWidgetItem *clickedItem;

    QAction *currentSelectionAction;

    bool CTRLpressed;

};

#endif // OPEMG_H
