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
        std::cout << "CustomOpenGLWidget::reshowItems" << std::endl; std::cout.flush();
        drawingTracker->reshowVisibleItems();
    }

    void showItem (CustomTreeWidgetItem *item)
    {
        std::cout << "CustomOpenGLWidget::showItem" << std::endl; std::cout.flush();
        drawingTracker->showItem(item);
    }

    bool isValidShow ()
    {
        std::cout << "CustomOpenGLWidget::isValidShow" << std::endl; std::cout.flush();
        return drawingTracker->isValidShow();
    }

    bool isNetValidShow ()
    {
        std::cout << "CustomOpenGLWidget::isNetValidShow" << std::endl; std::cout.flush();
        return drawingTracker->isNetValidShow();
    }

    bool isVIValidShow ()
    {
        std::cout << "CustomOpenGLWidget::isVIValidShow" << std::endl; std::cout.flush();
        return drawingTracker->isVIValidShow();
    }

    bool isValidHide ()
    {
        std::cout << "CustomOpenGLWidget::isValidHide" << std::endl; std::cout.flush();
        return drawingTracker->isValidHide();
    }

    bool isVIValidHide ()
    {
        std::cout << "CustomOpenGLWidget::isVIValidHide" << std::endl; std::cout.flush();
        return drawingTracker->isVIValidHide();
    }

    bool isNetValidHide ()
    {
        std::cout << "CustomOpenGLWidget::isNetValidHide" << std::endl; std::cout.flush();
        return drawingTracker->isNetValidHide();
    }

    void hideItem (CustomTreeWidgetItem *item)
    {
        std::cout << "CustomOpenGLWidget::hideItem" << std::endl; std::cout.flush();
        drawingTracker->hideItem(item);
    }

    void hideAllItems () {
        std::cout << "CustomOpenGLWidget::hideAllItems" << std::endl; std::cout.flush();
        drawingTracker->hideAllItems();
    }

    // void hideItems () {
    //     std::cout << "CustomOpenGLWidget::hideItems" << std::endl; std::cout.flush();
    //     drawingTracker->hideItems();
    // }

    void selectItem (CustomTreeWidgetItem *item)
    {
        std::cout << "CustomOpenGLWidget::selectItem" << std::endl; std::cout.flush();
        drawingTracker->selectItem(item);
    }

    bool hasSelectedItems (int type)
    {
        std::cout << "CustomOpenGLWidget::hasSelectedItems" << std::endl; std::cout.flush();
        return drawingTracker->hasSelectedItems(type);
    }

    bool hasDrawingSelectedItems ()
    {
        std::cout << "CustomOpenGLWidget::hasDrawingSelectedItems" << std::endl; std::cout.flush();
        return drawingTracker->hasSelectedItems(0);
    }

    bool hasPortSelectedItems ()
    {
        std::cout << "CustomOpenGLWidget::hasPortSelectedItems" << std::endl; std::cout.flush();
        return drawingTracker->hasSelectedItems(1);
    }

    bool hasBoundarySelectedItems ()
    {
        std::cout << "CustomOpenGLWidget::hasBoundarySelectedItems" << std::endl; std::cout.flush();
        return drawingTracker->hasSelectedItems(2);
    }

    bool hasOneSelectedItem ()
    {
        std::cout << "CustomOpenGLWidget::hasOneSelectedItem" << std::endl; std::cout.flush();
        return drawingTracker->hasOneSelectedItem();
    }

    bool hasAnySelectedItems ()
    {
        std::cout << "CustomOpenGLWidget::hasAnySelectedItems" << std::endl; std::cout.flush();
        return drawingTracker->hasAnySelectedItems();
    }

    void unselectItem (CustomTreeWidgetItem *item)
    {
        std::cout << "CustomOpenGLWidget::unselectItem" << std::endl; std::cout.flush();
        drawingTracker->unselectItem(item);
    }

    void unselectAllItems () {
        std::cout << "CustomOpenGLWidget::unselectAllItems" << std::endl; std::cout.flush();
        drawingTracker->unselectAllItems();
    }

    void deleteItem (CustomTreeWidgetItem *item) {
        std::cout << "CustomOpenGLWidget::deleteItem" << std::endl; std::cout.flush();
        drawingTracker->deleteItem(item);
    }

    void insertItemToMap (Handle(AIS_Shape) shape, CustomTreeWidgetItem *item)
    {
        std::cout << "CustomOpenGLWidget::insertItemToMap" << std::endl; std::cout.flush();
        drawingTracker->insertItemToMap(shape,item);
    }

    void displayShape (Handle(AIS_Shape) shape, int displayMode, int selectionMode)
    {
        std::cout << "CustomOpenGLWidget::displayShape" << std::endl; std::cout.flush();
        viewerContext->Display(shape,displayMode,selectionMode,Standard_False);
    }

    void hideShape (Handle(AIS_Shape) shape)
    {
        std::cout << "CustomOpenGLWidget::hideShape" << std::endl; std::cout.flush();
        viewerContext->Erase(shape,Standard_False);
    }

    bool isValidDelete ()
    {
        std::cout << "CustomOpenGLWidget::isValidDelete" << std::endl; std::cout.flush();
        return drawingTracker->isValidDelete();
    }

    // void hideShape (Handle(AIS_Shape) shape) {
    //     std::cout << "CustomOpenGLWidget::hideShape" << std::endl; std::cout.flush();
    //     if (!viewerContext->IsDisplayed(shape)) return;
    //     if (viewerContext->IsSelected(shape)) viewerContext->AddOrRemoveSelected(shape,Standard_True);
    //     viewerContext->Erase(shape,Standard_False);
    // }

    void deleteShape (Handle(AIS_Shape) shape) {
        std::cout << "CustomOpenGLWidget::deleteShape" << std::endl; std::cout.flush();
        viewerContext->Remove(shape,Standard_True);
        shape.Nullify();
    }

    void setSelectionShape () {viewerContext->Activate(0);}
    void setSelectionVertex () {viewerContext->Activate(1);}
    void setSelectionEdge () {viewerContext->Activate(2);}
    void setSelectionWire () {viewerContext->Activate(3);}
    void setSelectionFace () {viewerContext->Activate(4);}
    void setSelectionShell () {viewerContext->Activate(5);}
    void setSelectionSolid () {viewerContext->Activate(6);}

    void getSelected (std::vector<Handle(AIS_InteractiveObject)> *);
    Handle(AIS_InteractiveObject) getLastSelected ();

    ItemTracker* get_itemTracker () {return drawingTracker;}

    void selectRectangle ();
    void endSelectRectangle ();

    bool hasOneFaceSelected () {return drawingTracker->hasOneFaceSelected();}

    void reset () {
        std::cout << "CustomOpenGLWidget::reset" << std::endl; std::cout.flush();
        drawingTracker->reset();
    }

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

    QMenu *contextMenu;

    gp_Pln drawingPlane;
    gp_Pnt drawingPlaneOrigin;
    gp_Dir drawingPlaneDirection;
    gp_Ax1 drawingPlaneAxis;
    bool snapToGrid;

    RectangleSelector *rectSelect;
};

#endif // CUSTOMOPENGLWIDGET_H
