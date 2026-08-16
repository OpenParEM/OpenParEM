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
//#include "Polywire.h"

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

    void audit () {drawingTracker->audit();}

    void set_wireframe (bool state)
    {
        if (state) viewerContext->SetDisplayMode(AIS_WireFrame, Standard_False);
        else viewerContext->SetDisplayMode(AIS_Shaded, Standard_False);
    }

    void set_drawingItemTree (RootDrawingItem *rootDrawingItem_) {rootDrawingItem=rootDrawingItem_;}
    void set_pathItemTree (RootPathItem *rootPathItem_) {rootPathItem=rootPathItem_;}
    void set_portItemTree (RootPortItem *rootPortItem_) {rootPortItem=rootPortItem_;}
    void set_boundaryItemTree (RootBoundaryItem *rootBoundaryItem_) {rootBoundaryItem=rootBoundaryItem_;}
    void set_meshItemTree (RootMeshItem *rootMeshItem_) {rootMeshItem=rootMeshItem_;}
    void set_relay (Relay *relay_) {relay=relay_;}

    void updateViewer ();
    void clearDrawing ();

    void fitAll () {view->FitAll(); view->Redraw();}
    void fitSelected () {viewerContext->FitSelected(view); view->Redraw();}
    //void setScale (double scale) {view->SetScale(scale); view->Redraw();}

    void wheelEvent (QWheelEvent*) override;
    void keyPressEvent (QKeyEvent*) override;
    bool pixelToSnappedGrid (int, int, Standard_Real, gp_Pnt& snappedPoint);
    void mousePressEvent (QMouseEvent*) override;
    void mouseReleaseEvent (QMouseEvent*) override;
    void mouseMoveEvent (QMouseEvent*) override;

    void showGrid (Standard_Real, Standard_Real, Standard_Real, Standard_Real,
                  Standard_Real, Standard_Real, Standard_Real, Standard_Real);
    void hideGrid ();
    bool PixelToPointOnPlane (const Standard_Integer, const Standard_Integer, gp_Pnt& thePoint3D);
    void set_snapToGrid (bool state) {
        snapToGrid=state;
        viewer->SetGridEcho(state);
    }
    void set_gridSpacing (Standard_Real gridSpacing_) {gridSpacing=gridSpacing_;}
    void set_gridPlane (TopoDS_Face &face);
    void set_gridPlane (gp_Pnt &origin, gp_Dir &direction);
    void set_gridPlane (gp_Pln &plane);
    gp_Ax3 get_gridPlane () {return viewer->PrivilegedPlane();}

    void set_pickFirstVertex (bool pickFirstVertex_) {
        pickFirstVertex=pickFirstVertex_;
        pickSecondVertex=false;
    }

    void set_pickSecondVertex (bool pickSecondVertex_) {
        pickSecondVertex=pickSecondVertex_;
        pickFirstVertex=false;
    }

    void showItem (BaseItem *item)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::showItem" << std::endl; std::cout.flush();
        drawingTracker->showItem(item);
        activateItem(item);
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

    bool isValidHide ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::isValidHide" << std::endl; std::cout.flush();
        return drawingTracker->isValidHide();
    }

    bool isNetValidHide ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::isNetValidHide" << std::endl; std::cout.flush();
        return drawingTracker->isNetValidHide();
    }

    void hideItem (BaseItem *item)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::hideItem" << std::endl; std::cout.flush();
        drawingTracker->hideItem(item);
    }

    void refreshSelectedItems ()
    {
        drawingTracker->refreshSelectedItems();
    }

    void simpleSelectDrawingItem (DrawingItem *drawingItem)
    {
        drawingTracker->simpleSelectDrawingItem(drawingItem);
    }

    void selectItem (BaseItem *item)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::selectItem" << std::endl; std::cout.flush();
        drawingTracker->selectItem(item);
    }

    void activateSelectItem (BaseItem *item)
    {
        drawingTracker->activateSelectItem(item);
    }

    void activateItem (BaseItem *item)
    {
        if (!item) return;
        if (item->getShape().IsNull()) return;
        viewerContext->Display(item->getShape(),Standard_False);
        viewerContext->Load(item->getShape());
        viewerContext->Activate(item->getShape(),0,Standard_False);
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

    int get_boundarySelectedCount ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::get_portSelectedCount" << std::endl; std::cout.flush();
        return drawingTracker->get_boundarySelectedCount();
    }

    void unselectItem (BaseItem *item)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::unselectItem" << std::endl; std::cout.flush();
        drawingTracker->unselectItem(item);
    }

    void unselectItem (BaseItem *item, long unsigned int index)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::unselectItem" << std::endl; std::cout.flush();
        drawingTracker->unselectItem(item,index);
    }

    void unselectAllItems ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::unselectAllItems" << std::endl; std::cout.flush();
        drawingTracker->unselectAllItems();   // items from the tree
        clearSelected(Standard_False);        // anything else that might be selected, such as an un-tracked edge or face
    }

    // for bare shapes not in the item tracker
    void clearSelected ()
    {
        viewerContext->ClearSelected(Standard_False);
    }

    void insertItemToMap (Handle(AIS_Shape) shape, BaseItem *item)
    {
        if (shape.IsNull()) {
            if (item) {std::cout << "   CustomOpenGLWidget::insertItemToMap: ASSERT: shape item is null for item=" << item->text(0).toStdString() << std::endl; std::cout.flush(); return;}
            else {std::cout << "   CustomOpenGLWidget::insertItemToMap: ASSERT: shape item is null for item=null" << std::endl; std::cout.flush(); return;}
        }
        drawingTracker->insertItemToMap(shape,item);
    }

    void removeItemFromMap (BaseItem *item)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::removeItemFromMap" << std::endl; std::cout.flush();
        drawingTracker->removeItemFromMap(item);
    }

    void displayShape (Handle(AIS_Shape) shape)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::displayShape   type" << TopAbs::ShapeTypeToString(shape->Shape().ShapeType()) << std::endl; std::cout.flush();
        viewerContext->Display(shape,Standard_False);
    }

    void removeShape (Handle(AIS_Shape) shape)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::removeShape   type" << TopAbs::ShapeTypeToString(shape->Shape().ShapeType()) << std::endl; std::cout.flush();
        viewerContext->Remove(shape,Standard_False);
    }

    bool isValidDelete ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::isValidDelete" << std::endl; std::cout.flush();
        return drawingTracker->isValidDelete();
    }

    void deleteShape (Handle(AIS_Shape) shape)
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::deleteShape" << std::endl; std::cout.flush();
        viewerContext->Remove(shape,Standard_False);  //xxx Standard_True
        shape.Nullify();
    }

    void Deactivate () {viewerContext->Deactivate();}
    void Activate (int mode, Standard_Boolean theIsForce) {viewerContext->Activate(mode,theIsForce);}
    Standard_Integer get_NbSelected () {return viewerContext->NbSelected();}

    //void getSelected (std::vector<Handle(AIS_InteractiveObject)> *);
    Handle(AIS_InteractiveObject) getLastSelected ();

    ItemTracker* get_itemTracker () {return drawingTracker;}

    void selectRectangle ();
    void endSelectRectangle ();

    //bool hasOneFaceSelected () {return drawingTracker->hasOneFaceSelected();}

    bool isDisplayed (Handle(AIS_Shape) shape)
    {
        if (viewerContext->IsDisplayed(shape)) return true;
        return false;
    }

    Standard_Integer NbSelected () {return viewerContext->NbSelected();}

    int numberDrawingFaceSelected ()
    {
        if (viewerContext.IsNull()) {
            return 0;
        }

        int count=0;
        for (viewerContext->InitSelected(); viewerContext->MoreSelected(); viewerContext->NextSelected()) {
            TopoDS_Shape pickedShape=viewerContext->SelectedShape();
            if (!pickedShape.IsNull()) {
                if (pickedShape.ShapeType() == TopAbs_FACE) count++;
            }
        }
        return count;
    }

    // get the selected subshape by index; max available verified elsewhere
    TopoDS_Shape get_selectedSubshape (int index)
    {
        int count=0;
        TopoDS_Shape pickedShape;
        for (viewerContext->InitSelected(); viewerContext->MoreSelected(); viewerContext->NextSelected()) {
            pickedShape=viewerContext->SelectedShape();
            if (count == index) break;
            count++;
        }
        return pickedShape;
    }

    void reset ()
    {
        if (showTracking) std::cout << "CustomOpenGLWidget::reset" << std::endl; std::cout.flush();
        drawingTracker->reset();
    }

    void removeSelectOnVertex ()
    {
        if (!vertexFilter.IsNull()) {
            viewerContext->RemoveFilter(vertexFilter);
            vertexFilter.Nullify();
        }
    }

    void clearSelected (const Standard_Boolean theToUpdateViewer) {viewerContext->ClearSelected(theToUpdateViewer);}

    void finishPickVertex (bool);

    Handle(AIS_InteractiveContext) get_viewerContext () {return viewerContext;}

    gp_Dir get_normal () {return view->Viewer()->PrivilegedPlane().Direction();}

    long unsigned int get_selectedItems_size () {return drawingTracker->getSelectedItemsSize();}
    BaseItem* get_selectedItem (long unsigned int i) {return drawingTracker->getSelectedItem(i);}
    long unsigned int get_selectedItems_count () {return drawingTracker->getSelectedItemsCount();}

    void setSubshapeSelection (bool isSubshapeSelection_) {isSubshapeSelection=isSubshapeSelection_;}
    void setSetToPlane (bool isSetToPlane_) {isSetToPlane=isSetToPlane_;}

    void setShaded (Handle(AIS_Shape) shape)
    {
        viewerContext->SetDisplayMode(shape,AIS_Shaded,Standard_True);
    }

    // void PrintAllActiveModes () {
    //     std::cout << "CustomOpenGLWidget:: PrintAllActiveModes" << std::endl; std::cout.flush();

    //     AIS_ListOfInteractive aDisplayedObjects;
    //     viewerContext->DisplayedObjects(aDisplayedObjects);

    //     for (AIS_ListIteratorOfListOfInteractive anObjIt(aDisplayedObjects); anObjIt.More(); anObjIt.Next()) {
    //         Handle(AIS_InteractiveObject) anObj = anObjIt.Value();
    //         TColStd_ListOfInteger aModes;

    //         viewerContext->ActivatedModes(anObj, aModes);

    //         for (TColStd_ListIteratorOfListOfInteger aModeIt(aModes); aModeIt.More(); aModeIt.Next()) {
    //             Standard_Integer aMode = aModeIt.Value();
    //             std::cout << "   aMode=" << aMode << std::endl; std::cout.flush();
    //         }
    //     }
    // }

    V3d_TypeOfOrientation getProjection () {return view->Viewer()->DefaultViewProj();}
    void setProjection (V3d_TypeOfOrientation projection) {view->SetProj(projection); updateViewer();}

    void compactSelectedItems () {drawingTracker->compactSelectedItems();}
    //void compactVisibleItems () {drawingTracker->compactVisibleItems();}
    void printTrackerStats () {drawingTracker->printStats();}
    void printDrawingSelectedCount () {
        std::cout << "      drawing selected count = " << viewerContext->NbSelected() << std::endl; std::cout.flush();
    }

    void shutdown()
    {
        makeCurrent();

        if (!viewerContext.IsNull())
            viewerContext->RemoveAll(Standard_False);

        viewCube.Nullify();
        focusView.Nullify();
        view.Nullify();
        viewerContext.Nullify();
        viewer.Nullify();
        graphicDriver.Nullify();

        doneCurrent();
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

    RootDrawingItem *rootDrawingItem;
    RootPathItem *rootPathItem;
    RootPortItem *rootPortItem;
    RootBoundaryItem *rootBoundaryItem;
    RootMeshItem *rootMeshItem;


    ItemTracker *drawingTracker;

    gp_Pln drawingPlane;
    gp_Pnt drawingPlaneOrigin;
    gp_Dir drawingPlaneDirection;
    gp_Ax1 drawingPlaneAxis;
    bool snapToGrid;
    Standard_Real gridSpacing;

    RectangleSelector *rectSelect;

    // all drawing
    Relay *relay;
    bool ignoreMouseRelease;
    bool isSubshapeSelection;
    bool isSingleSelection;
    bool isSetToPlane;
    gp_Pnt clickPoint;
    bool clickPointValid;

    // vertex
    bool pickFirstVertex, pickSecondVertex;
    gp_Pnt vertexPoint;

    // filter
    Handle(VertexFilter) vertexFilter;
    Handle(AIS_Shape) temporaryVertex;  // for mid-point selection on edges

    // for debug
    bool showTracking;

};

#endif // CUSTOMOPENGLWIDGET_H
