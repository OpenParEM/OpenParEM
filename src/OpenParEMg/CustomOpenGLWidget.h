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
///
#ifndef CUSTOMOPENGLWIDGET_H
#define CUSTOMOPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QMessageBox>
#include <QMouseEvent>

#include "CustomTreeWidgetItem.h"
#include "RectangleSelector.h"
#include "ItemTracking.h"

#include "OpenGl_GraphicDriver.hxx"
#include <Aspect_DisplayConnection.hxx>
#include <AIS_InteractiveContext.hxx>
#include "AIS_ViewController.hxx"
#include "AIS_Shape.hxx"
#include "AIS_ViewCube.hxx"

#include <Standard_WarningsDisable.hxx>
#include <Standard_WarningsRestore.hxx>
#include <V3d_View.hxx>

class CustomOpenGLWidget : public QOpenGLWidget, public AIS_ViewController
{
    Q_OBJECT
public:
    CustomOpenGLWidget (QWidget *parent = nullptr);
    virtual ~CustomOpenGLWidget ();

    void set_drawingItemTree (CustomTreeWidgetItem *drawingItemTree_) {drawingItemTree=drawingItemTree_;}
    void set_portItemTree (CustomTreeWidgetItem *portItemTree_) {portItemTree=portItemTree_;}
    void set_boundaryItemTree (CustomTreeWidgetItem *boundaryItemTree_) {boundaryItemTree=boundaryItemTree_;}
    void set_meshItemTree (CustomTreeWidgetItem *meshItemTree_) {meshItemTree=meshItemTree_;}
//    void set_drawingToItemMap (std::unordered_map<Handle(AIS_Shape), CustomTreeWidgetItem*> *drawingToItemMap_) {drawingToItemMap=drawingToItemMap_;}
    void set_contextMenu (QMenu *contextMenu_) {contextMenu=contextMenu_;}

    void updateViewer ();
    void clearDrawing ();

    void fitAll () {view->FitAll(); view->Redraw();}
    void fitSelected () {viewerContext->FitSelected(view); view->Redraw();}

    void wheelEvent (QWheelEvent*) override;
    void keyPressEvent (QKeyEvent*) override;
    void mousePressEvent (QMouseEvent*) override;
    void mouseReleaseEvent (QMouseEvent*) override;
    void mouseMoveEvent (QMouseEvent*) override;

    void showGrid ();
    void hideGrid ();
    bool PixelToPointOnPlane (const Standard_Integer, const Standard_Integer, gp_Pnt& thePoint3D);
    void set_snapToGrid (bool state) {
        snapToGrid=state;
        viewer->SetGridEcho(state);
    }
    bool set_gridPlane ();

    void reshowItems () {
        drawingTracker->reshowVisibleItems();
    }

    void showItem (CustomTreeWidgetItem *item)
    {
        drawingTracker->showItem(item);
    }

    bool isDrawingValidShow ()
    {
        return drawingTracker->isValidShow();
    }

    bool isDrawingValidHide ()
    {
        return drawingTracker->isValidHide();
    }

    void hideItem (CustomTreeWidgetItem *item)
    {
        drawingTracker->hideItem(item);
    }

    void hideAllItems () {
        drawingTracker->hideAllItems();
    }

    void hideItems () {
        drawingTracker->hideItems();
    }

    void selectItem (CustomTreeWidgetItem *item)
    {
        drawingTracker->selectItem(item);
    }

    bool hasDrawingSelectedItems ()
    {
        return drawingTracker->hasSelectedItems();
    }

    void unselectItem (CustomTreeWidgetItem *item)
    {
        drawingTracker->unselectItem(item);
    }

    void unselectAllItems () {
        drawingTracker->unselectAllItems();
    }

    void deleteItem (CustomTreeWidgetItem *item) {
        drawingTracker->deleteItem(item);
    }


    void insertItemToMap (Handle(AIS_Shape) shape, CustomTreeWidgetItem *item)
    {
        drawingTracker->insertItemToMap(shape,item);
    }

    void displayShape (Handle(AIS_Shape) shape, int displayMode, int selectionMode)
    {
        viewerContext->Display(shape,displayMode,selectionMode,Standard_False);
    }

    bool isDrawingValidDelete ()
    {
        return drawingTracker->isValidDelete();
    }

    long unsigned int trackerDrawingCount ()
    {
        return drawingTracker->get_drawingCount();
    }

    /*
    void redisplayShape(Handle(AIS_Shape) shape, int displayMode, int selectionMode)
    {
        viewerContext->Erase(shape,Standard_False);
        viewerContext->Display(shape,displayMode,selectionMode,Standard_False);
    }

    void selectShape (Handle(AIS_Shape) shape) {
        if (!viewerContext->IsDisplayed(shape)) return;
        if (viewerContext->IsSelected(shape)) return;
        viewerContext->AddOrRemoveSelected(shape,Standard_False);
    }

    void unselectShape (Handle(AIS_Shape) shape) {
        if (!viewerContext->IsDisplayed(shape)) return;
        if (!viewerContext->IsSelected(shape)) return;
        viewerContext->AddOrRemoveSelected(shape,Standard_False);
    }
*/
    void hideShape (Handle(AIS_Shape) shape) {
        if (!viewerContext->IsDisplayed(shape)) return;
        if (viewerContext->IsSelected(shape)) viewerContext->AddOrRemoveSelected(shape,Standard_True);
        viewerContext->Erase(shape,Standard_False);
    }

    void deleteShape (Handle(AIS_Shape) shape) {
        viewerContext->Remove(shape,Standard_True);
        shape.Nullify();
    }

/*
    void hideAll () {
        viewerContext->EraseAll(Standard_True);
        viewerContext->UpdateCurrentViewer();
    }

    void showAll () {
        viewerContext->DisplayAll(Standard_True);
        viewerContext->UpdateCurrentViewer();
    }

    void unselectAll () {
        viewerContext->ClearSelected(Standard_True);
        viewerContext->UpdateCurrentViewer();
    }
*/

    void setSelectionShape () {viewerContext->Activate(0);}
    void setSelectionVertex () {viewerContext->Activate(1);}
    void setSelectionEdge () {viewerContext->Activate(2);}
    void setSelectionWire () {viewerContext->Activate(3);}
    void setSelectionFace () {viewerContext->Activate(4);}
    void setSelectionShell () {viewerContext->Activate(5);}
    void setSelectionSolid () {viewerContext->Activate(6);}

//    bool isDisplayed (Handle(AIS_Shape) shape) {return viewerContext->IsDisplayed(shape);}

//    void unselectTreeItems (CustomTreeWidgetItem *);

    void getSelected (std::vector<Handle(AIS_InteractiveObject)> *);
    Handle(AIS_InteractiveObject) getLastSelected ();
//    void showItemsSelected ();

    void selectRectangle ();
    void endSelectRectangle ();

//    long unsigned int get_shownItemListSize() {return shownItemList.size();}
//    long unsigned int get_selectedItemListSize() {return selectedItemList.size();}

    void reset () {
        drawingTracker->reset();
        //portTracker->reset();
        //boundaryTracker->reset();
        //meshTracker->reset();
    }

    void set_meshVisibility (bool value) {meshVisibility=value;}
    bool get_meshVisibility () {return meshVisibility;}

    void set_hasMesh (bool value) {hasMesh=value;}
    bool get_hasMesh () {return hasMesh;}

protected:
    void initializeGL () override;
    void paintGL () override;

private:
    Handle(Aspect_DisplayConnection) displayConnection;
    Handle(OpenGl_GraphicDriver) graphicDriver;
    Handle(V3d_Viewer) viewer;
    Handle(V3d_View) view;
    Handle(AIS_InteractiveContext) viewerContext;
    Handle(V3d_View) focusView;
    Handle(AIS_ViewCube) viewCube;

    CustomTreeWidgetItem *drawingItemTree;
    CustomTreeWidgetItem *portItemTree;
    CustomTreeWidgetItem *boundaryItemTree;
    CustomTreeWidgetItem *meshItemTree;

    ItemTracker *drawingTracker;
    bool hasMesh;
    bool meshVisibility;

    QMenu *contextMenu;

    gp_Pln drawingPlane;
    gp_Pnt drawingPlaneOrigin;
    gp_Dir drawingPlaneDirection;
    gp_Ax1 drawingPlaneAxis;
    bool snapToGrid;

    RectangleSelector *rectSelect;
};

#endif // CUSTOMOPENGLWIDGET_H
