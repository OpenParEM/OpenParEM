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
    void set_drawingToItemMap (std::unordered_map<Handle(AIS_Shape), CustomTreeWidgetItem*> *drawingToItemMap_) {drawingToItemMap=drawingToItemMap_;}
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

    void displayShape (Handle(AIS_Shape) shape, int displayMode, int selectionMode)
    {
        viewerContext->Display(shape,displayMode,selectionMode,Standard_False);
    }

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

    void hideShape (Handle(AIS_Shape) shape) {
        if (!viewerContext->IsDisplayed(shape)) return;
        if (viewerContext->IsSelected(shape)) viewerContext->AddOrRemoveSelected(shape,Standard_True);
        viewerContext->Erase(shape,Standard_False);
    }

    void deleteShape (Handle(AIS_Shape) shape) {
        viewerContext->Remove(shape,Standard_True);
    }

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

    void setSelectionShape () {viewerContext->Activate(0);}
    void setSelectionVertex () {viewerContext->Activate(1);}
    void setSelectionEdge () {viewerContext->Activate(2);}
    void setSelectionWire () {viewerContext->Activate(3);}
    void setSelectionFace () {viewerContext->Activate(4);}
    void setSelectionShell () {viewerContext->Activate(5);}
    void setSelectionSolid () {viewerContext->Activate(6);}

    bool isDisplayed (Handle(AIS_Shape) shape) {return viewerContext->IsDisplayed(shape);}

    void unselectTreeItems (CustomTreeWidgetItem *);

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
    std::unordered_map<Handle(AIS_Shape), CustomTreeWidgetItem*> *drawingToItemMap;

    QMenu *contextMenu;
};

#endif // CUSTOMOPENGLWIDGET_H
