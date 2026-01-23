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
#include "PolygonSelection.h"
#include "RectangleSelector.h"
#include "ItemTracking.h"
#include "Relay.h"

#include "OpenGl_GraphicDriver.hxx"
#include <Aspect_DisplayConnection.hxx>
#include <AIS_InteractiveContext.hxx>
#include "AIS_ViewController.hxx"
#include "AIS_Shape.hxx"
#include "AIS_ViewCube.hxx"

#include <Standard_WarningsDisable.hxx>
#include <Standard_WarningsRestore.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <TopoDS_Face.hxx>
#include <V3d_View.hxx>
#include <Prs3d_PointAspect.hxx>

class CustomOpenGLWidget : public QOpenGLWidget, public AIS_ViewController
{
    Q_OBJECT
public:
    CustomOpenGLWidget (QWidget *parent = nullptr);
    virtual ~CustomOpenGLWidget ();

    void set_wireframe (bool state)
    {
        if (state) viewerContext->SetDisplayMode(AIS_WireFrame, Standard_False);
        else viewerContext->SetDisplayMode(AIS_Shaded, Standard_False);
    }

    void set_drawingItemTree (CustomTreeWidgetItem *drawingItemTree_) {drawingItemTree=drawingItemTree_;}
    void set_portItemTree (CustomTreeWidgetItem *portItemTree_) {portItemTree=portItemTree_;}
    void set_boundaryItemTree (CustomTreeWidgetItem *boundaryItemTree_) {boundaryItemTree=boundaryItemTree_;}
    void set_meshItemTree (CustomTreeWidgetItem *meshItemTree_) {meshItemTree=meshItemTree_;}
    void set_pathItemTree (CustomTreeWidgetItem *pathItemTree_) {pathItemTree=pathItemTree_;}
    void set_contextMenu (QMenu *contextMenu_) {contextMenu=contextMenu_;}
    void set_relay (Relay *relay_) {relay=relay_;}

    void updateViewer ();
    void clearDrawing ();

    void fitAll () {view->FitAll(); view->Redraw();}
    void fitSelected () {viewerContext->FitSelected(view); view->Redraw();}

    void cancelDraw ();
    void drawRubberBand (gp_Pnt);

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
    void set_gridPlane ();

    void set_isPath (bool isPath_) {isPath=isPath_;}
    bool get_isPath () {return isPath;}

    void set_drawLine (bool drawLine_)
    {
        drawLine=drawLine_;
        viewerContext->DefaultDrawer()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O,Quantity_NOC_CYAN1,2));
    }

    void set_drawPolyline (bool drawPolyline_)
    {
        drawPolyline=drawPolyline_;
        viewerContext->DefaultDrawer()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O,Quantity_NOC_CYAN1,2));
    }

    void clearDrawPoints () {shapePoints.clear();}

    void reshowItems ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::reshowItems" << std::endl; std::cout.flush();
        drawingTracker->reshowVisibleItems();
    }

    void showItem (CustomTreeWidgetItem *item)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::showItem" << std::endl; std::cout.flush();
        drawingTracker->showItem(item);
    }

    bool isValidShow ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::isValidShow" << std::endl; std::cout.flush();
        return drawingTracker->isValidShow();
    }

    bool isNetValidShow ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::isNetValidShow" << std::endl; std::cout.flush();
        return drawingTracker->isNetValidShow();
    }

    bool isVIValidShow ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::isVIValidShow" << std::endl; std::cout.flush();
        return drawingTracker->isVIValidShow();
    }

    bool isValidHide ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::isValidHide" << std::endl; std::cout.flush();
        return drawingTracker->isValidHide();
    }

    bool isVIValidHide ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::isVIValidHide" << std::endl; std::cout.flush();
        return drawingTracker->isVIValidHide();
    }

    bool isNetValidHide ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::isNetValidHide" << std::endl; std::cout.flush();
        return drawingTracker->isNetValidHide();
    }

    void hideItem (CustomTreeWidgetItem *item)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::hideItem" << std::endl; std::cout.flush();
        drawingTracker->hideItem(item);
    }

    void hideAllItems ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::hideAllItems" << std::endl; std::cout.flush();
        drawingTracker->hideAllItems();
    }

    // void hideItems () {
    //     if (showTracking) std::cout << "CustomOpenGLWidget::hideItems" << std::endl; std::cout.flush();
    //     drawingTracker->hideItems();
    // }

    void selectItem (CustomTreeWidgetItem *item)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::selectItem" << std::endl; std::cout.flush();
        drawingTracker->selectItem(item);
    }

    bool hasSelectedItems (int type)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::hasSelectedItems" << std::endl; std::cout.flush();
        return drawingTracker->hasSelectedItems(type);
    }

    bool hasDrawingSelectedItems ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::hasDrawingSelectedItems" << std::endl; std::cout.flush();
        return drawingTracker->hasSelectedItems(0);
    }

    bool hasPortSelectedItems ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::hasPortSelectedItems" << std::endl; std::cout.flush();
        return drawingTracker->hasSelectedItems(1);
    }

    bool hasBoundarySelectedItems ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::hasBoundarySelectedItems" << std::endl; std::cout.flush();
        return drawingTracker->hasSelectedItems(2);
    }

    bool hasOneSelectedItem ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::hasOneSelectedItem" << std::endl; std::cout.flush();
        return drawingTracker->hasOneSelectedItem();
    }

    TopoDS_Face getSelectedFace ()
    {
        TopoDS_Face face;

        // 1 - the bare face
        // 2 - the face as part of an object
        if (viewerContext->NbSelected() == 1 || viewerContext->NbSelected() == 2) {
            for (viewerContext->InitSelected(); viewerContext->MoreSelected(); viewerContext->NextSelected()) {
                Handle(SelectMgr_EntityOwner) owner=viewerContext->SelectedOwner();
                Handle(StdSelect_BRepOwner) brepOwner=Handle(StdSelect_BRepOwner)::DownCast(owner);
                if (!brepOwner.IsNull()) {
                    TopoDS_Shape shape=brepOwner->Shape();
                    if (shape.ShapeType() == TopAbs_FACE) {
                        face=TopoDS::Face(shape);
                        break;
                    }
                }
            }
        }

        return face;
    }

    bool hasAnySelectedItems ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::hasAnySelectedItems" << std::endl; std::cout.flush();
        return drawingTracker->hasAnySelectedItems();
    }

    int get_pathSelectedCount ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::get_pathSelectedCount" << std::endl; std::cout.flush();
        return drawingTracker->get_pathSelectedCount();
    }

    int get_portSelectedCount ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::get_portSelectedCount" << std::endl; std::cout.flush();
        return drawingTracker->get_portSelectedCount();
    }

    void unselectItem (CustomTreeWidgetItem *item)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::unselectItem" << std::endl; std::cout.flush();
        drawingTracker->unselectItem(item);
    }

    void unselectAllItems () {
        if (showTracking) std::cout << "CustomOpenGLWidget::unselectAllItems" << std::endl; std::cout.flush();
        drawingTracker->unselectAllItems();
    }

    void deleteItem (CustomTreeWidgetItem *item) {
        if (showTracking) std::cout << "CustomOpenGLWidget::deleteItem" << std::endl; std::cout.flush();
        drawingTracker->deleteItem(item);
    }

    void insertItemToMap (Handle(AIS_Shape) shape, CustomTreeWidgetItem *item)
    {
        if (shape.IsNull()) {std::cout << "   CustomOpenGLWidget::insertItemToMap: ASSERT: shape item is null" << std::endl; std::cout.flush(); return;}
        if (showTracking) std::cout << "CustomOpenGLWidget::insertItemToMap" << std::endl; std::cout.flush();
        drawingTracker->insertItemToMap(shape,item);
    }

    void removeItemFromMap (CustomTreeWidgetItem *item)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::removeItemFromMap" << std::endl; std::cout.flush();
        drawingTracker->removeItemFromMap(item);
    }

    void displayShape (Handle(AIS_Shape) shape, int displayMode, int selectionMode)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::displayShape   type=" << TopAbs::ShapeTypeToString(shape->Shape().ShapeType()) << std::endl; std::cout.flush();
        viewerContext->Display(shape,displayMode,selectionMode,Standard_False);
    }

    void displayShape (Handle(AIS_Shape) shape)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::displayShape   type" << TopAbs::ShapeTypeToString(shape->Shape().ShapeType()) << std::endl; std::cout.flush();
        viewerContext->Display(shape,Standard_False);
    }

    void hideShape (Handle(AIS_Shape) shape)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::hideShape" << std::endl; std::cout.flush();
        viewerContext->Erase(shape,Standard_False);
    }

    bool isValidDelete ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::isValidDelete" << std::endl; std::cout.flush();
        return drawingTracker->isValidDelete();
    }

    // void hideShape (Handle(AIS_Shape) shape) {
    //     if (showTracking) std::cout << "CustomOpenGLWidget::hideShape" << std::endl; std::cout.flush();
    //     if (!viewerContext->IsDisplayed(shape)) return;
    //     if (viewerContext->IsSelected(shape)) viewerContext->AddOrRemoveSelected(shape,Standard_True);
    //     viewerContext->Erase(shape,Standard_False);
    // }

    void deleteShape (Handle(AIS_Shape) shape)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::deleteShape" << std::endl; std::cout.flush();
        viewerContext->Remove(shape,Standard_True);
        shape.Nullify();
    }

    void Activate (Standard_Integer mode, Standard_Boolean theIsForce) {viewerContext->Activate(mode,theIsForce);}
    void Activate (Handle(AIS_Shape) shape, Standard_Integer mode, Standard_Boolean theIsForce) {viewerContext->Activate(shape,mode,theIsForce);}
    void Deactivate () {viewerContext->Deactivate();}


    void getSelected (std::vector<Handle(AIS_InteractiveObject)> *);
    Handle(AIS_InteractiveObject) getLastSelected ();

    ItemTracker* get_itemTracker () {return drawingTracker;}

    void selectRectangle ();
    void endSelectRectangle ();

    bool hasOneFaceSelected () {return drawingTracker->hasOneFaceSelected();}

    Standard_Integer NbSelected () {return viewerContext->NbSelected();}

    int numberDrawingFaceSelected ()
    {
        int count=0;
        for (viewerContext->InitSelected(); viewerContext->MoreSelected(); viewerContext->NextSelected()) {
            TopoDS_Shape pickedShape = viewerContext->SelectedShape();
            if (pickedShape.ShapeType() == TopAbs_FACE) count++;
        }
        return count;
    }

    // assumes one selected shape that has been verified elsewhere
    TopoDS_Shape get_selectedFace ()
    {
        TopoDS_Shape pickedShape;
        for (viewerContext->InitSelected(); viewerContext->MoreSelected(); viewerContext->NextSelected()) {
            pickedShape=viewerContext->SelectedShape();
            if (pickedShape.ShapeType() == TopAbs_FACE) {
                break;
            }
        }
        return pickedShape;
    }

    // get the selected face by index; max available verified elsewhere
    TopoDS_Shape get_selectedFace (int index)
    {
        int count=0;
        TopoDS_Shape pickedShape;
        for (viewerContext->InitSelected(); viewerContext->MoreSelected(); viewerContext->NextSelected()) {
            pickedShape=viewerContext->SelectedShape();
            if (pickedShape.ShapeType() == TopAbs_FACE) {
                if (count == index) break;
                count++;
            }
        }
        return pickedShape;
    }

    void reset ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::reset" << std::endl; std::cout.flush();
        drawingTracker->reset();
    }

    void selectOnVertex (Path *outline)
    {
        if (!outline) return;

        if (vertexFilter.IsNull()) {
            viewerContext->RemoveFilter(vertexFilter);
            vertexFilter.Nullify();
        }

        vertexFilter=new VertexFilter();
        vertexFilter->set_outline(outline);
        viewerContext->AddFilter(vertexFilter);
    }

    void removeSelectOnVertex ()
    {
        if (!vertexFilter.IsNull()) {
            viewerContext->RemoveFilter(vertexFilter);
            vertexFilter.Nullify();
        }
    }

    void deleteLastPoint ()
    {
        shapePoints.pop_back();

        QPoint localPos=mapFromGlobal(QCursor::pos());

        Standard_Real x,y,z;
        view->Convert(localPos.x(),localPos.y(),x,y,z);
        gp_Pnt movePoint(x,y,z);

        drawRubberBand(movePoint);
    }

    void closePolyline ()
    {
        if (shapePoints.size() > 2) {
            shapePoints.push_back(shapePoints[0]);
            finishDrawLine();
        }
    }

    long unsigned int get_shapePoints_size () {return shapePoints.size();}

    void clearSelected (const Standard_Boolean theToUpdateViewer) {viewerContext->ClearSelected(theToUpdateViewer);}

    void finishDrawLine ();

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
    CustomTreeWidgetItem *pathItemTree;

    ItemTracker *drawingTracker;

    QMenu *contextMenu;

    gp_Pln drawingPlane;
    gp_Pnt drawingPlaneOrigin;
    gp_Dir drawingPlaneDirection;
    gp_Ax1 drawingPlaneAxis;
    bool snapToGrid;

    RectangleSelector *rectSelect;

    // all drawing
    bool ignoreLeftMouseRelease;
    bool isPath;
    Relay *relay;

    // line
    Handle(AIS_Shape) lineRubberBand;
    bool drawLine, drawPolyline;
    std::vector<gp_Pnt> shapePoints;

    // filter
    Handle(VertexFilter) vertexFilter;
    Handle(AIS_Shape) temporaryVertex;  // for mid-point selection on edges

    // for debug
    bool showTracking;

};

#endif // CUSTOMOPENGLWIDGET_H
