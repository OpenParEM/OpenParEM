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

#include "CustomOpenGLWidget.h"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <Geom_Plane.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <glx.h>
#include "OcctQtTools.h"

#include <QWidget>
#include <QResizeEvent>
#include <QColorSpace>

#include <V3d_View.hxx>
#include <TopoDS_Shape.hxx>
#include <AIS_Shape.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <TopoDS_Face.hxx>
#include "Aspect_NeutralWindow.hxx"
#include <OpenGl_FrameBuffer.hxx>
#include <V3d_RectangularGrid.hxx>

#include <V3d_View.hxx>
#include <gp_Pln.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <IntAna_IntConicQuad.hxx>
#include <Standard_Boolean.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <TopExp.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Graphic3d_AspectMarker3d.hxx>

CustomOpenGLWidget::CustomOpenGLWidget (QWidget* theParent) : QOpenGLWidget (theParent)
{
    // debug
    showTracking=false;

    // settings
    setMouseTracking(true);
    setBackgroundRole(QPalette::NoRole);
    setFocusPolicy(Qt::StrongFocus);
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

    // displacy connection
    displayConnection=new Aspect_DisplayConnection();

    // graphics driver
    graphicDriver=new OpenGl_GraphicDriver(displayConnection);
    graphicDriver->ChangeOptions().buffersNoSwap = true;
    graphicDriver->ChangeOptions().useSystemBuffer = false;

    // 3D viewer
    viewer=new V3d_Viewer(graphicDriver);
    viewer->SetDefaultBackgroundColor(Quantity_NOC_BLACK);
    viewer->SetDefaultLights();
    viewer->SetLightOn();
    //viewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);  // shows a 2D grid; ToDo: hook this up when drawing is enabled

    // AIS context
    viewerContext=new AIS_InteractiveContext(viewer);

    // item tracking
    drawingTracker=new ItemTracker(viewerContext);

    // mesh
    //hasMesh=false;
    //meshVisibility=false;

    // create an orientation cube for the display
    viewCube=new AIS_ViewCube();
    viewCube->SetViewAnimation(myViewAnimation);
    viewCube->SetFixedAnimationLoop(false);
    viewCube->SetAutoStartAnimation(true);
    viewCube->TransformPersistence()->SetOffset2d(Graphic3d_Vec2i(100, 150));
    viewCube->SetDuration(0.0);  // snaps to face views when clicking the cube

    // viewer
    view=viewer->CreateView();
    view->SetImmediateUpdate(false);
    view->ChangeRenderingParams().NbMsaaSamples=4;
    view->ChangeRenderingParams().ToShowStats=false;

    // drawing plane
    drawingPlaneOrigin.SetCoord(0,0,0);
    drawingPlane.SetLocation(drawingPlaneOrigin);

    drawingPlaneDirection.SetCoord(0,0,1);
    drawingPlaneAxis.SetDirection(drawingPlaneDirection);
    drawingPlane.SetAxis(drawingPlaneAxis);

    snapToGrid=false;
    viewer->SetGridEcho(Standard_False);

    rectSelect=nullptr;
    pickFirstVertex=false;
    pickSecondVertex=false;

    // set a default so that all vertices highlight with a circle
    Handle(Prs3d_Drawer) drawer=viewerContext->DefaultDrawer();

    viewerContext->SetAutoActivateSelection(Standard_False);
}

CustomOpenGLWidget::~CustomOpenGLWidget ()
{
    viewerContext->RemoveAll(false);
    viewerContext.Nullify();
    view->Remove();
    view.Nullify();
    viewer.Nullify();

    if (drawingTracker) delete drawingTracker;

    makeCurrent();
    displayConnection.Nullify();
}

void CustomOpenGLWidget::initializeGL ()
{
    const QRect viewGeometry=rect();
    const Graphic3d_Vec2i viewSize(viewGeometry.right()-viewGeometry.left(),viewGeometry.bottom()-viewGeometry.top());

    Handle(Aspect_NeutralWindow) aspectNeutralWindow=new Aspect_NeutralWindow();
    Aspect_Drawable nativeWindow=(Aspect_Drawable)winId();
    aspectNeutralWindow->SetNativeHandle(nativeWindow);
    aspectNeutralWindow->SetSize (viewSize.x(),viewSize.y());
    view->SetWindow (aspectNeutralWindow,(Aspect_RenderingContext) glXGetCurrentContext());
}

void CustomOpenGLWidget::paintGL ()
{
    if (view->Window().IsNull()) return;

    // wrap frame buffer
    const Handle(OpenGl_Context)& sharedContext=graphicDriver->GetSharedContext();
    Handle(OpenGl_FrameBuffer) defaultFrameBuffer=sharedContext->DefaultFrameBuffer();
    if (defaultFrameBuffer.IsNull()) {
        defaultFrameBuffer=new OpenGl_FrameBuffer();
        sharedContext->SetDefaultFrameBuffer(defaultFrameBuffer);
    }
    defaultFrameBuffer->InitWrapper(sharedContext);

    // handle re-sizing of the window

    Handle(Aspect_NeutralWindow) aspectNeutralWindow=Handle(Aspect_NeutralWindow)::DownCast(view->Window());
    Graphic3d_Vec2i viewSizeOld;
    aspectNeutralWindow->Size(viewSizeOld.x(),viewSizeOld.y());

    Graphic3d_Vec2i viewSizeNew(defaultFrameBuffer->GetVPSizeX(),defaultFrameBuffer->GetVPSizeY());

    if (viewSizeNew != viewSizeOld) {
        aspectNeutralWindow->SetSize(viewSizeNew.x(),viewSizeNew.y());
        view->MustBeResized();
        view->Invalidate();
    }

    // display the orientation cube
    viewerContext->Display(viewCube,0,0,false);

    // flush and redraw
    view->InvalidateImmediate();
    FlushViewEvents (viewerContext,view,true);
}

void CustomOpenGLWidget::updateViewer ()
{
    //std::cout << "CustomOpenGLWidget::updateViewer" << std::endl; std::cout.flush();
    viewerContext->UpdateCurrentViewer();
    repaint();
}

void CustomOpenGLWidget::clearDrawing ()
{
    viewerContext->RemoveAll(false);
    viewerContext->UpdateCurrentViewer();
}

void CustomOpenGLWidget::cancelDraw ()
{
    //std::cout << "CustomOpenGLWidget::cancelDraw" << std::endl; std::cout.flush();

    // invalidate flags
    pickFirstVertex=false;
    pickSecondVertex=false;

    // remove temporaryVertex, if needed
    if (!temporaryVertex.IsNull()) {
        viewerContext->Remove(temporaryVertex,Standard_True);
        temporaryVertex.Nullify();
    }

    // reset point selection symbol
    viewerContext->DefaultDrawer()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_PLUS,Quantity_NOC_YELLOW1,2));

    // clear detection
    viewerContext->ClearDetected(Standard_True);
}

void CustomOpenGLWidget::wheelEvent (QWheelEvent* event)
{
    QOpenGLWidget::wheelEvent(event);
    if (view.IsNull()) return;

    const Graphic3d_Vec2i position(Graphic3d_Vec2d(event->position().x(),event->position().y()));
    if (UpdateZoom(Aspect_ScrollDelta(position,double(event->angleDelta().y())/8.0))) updateViewer();
}

void CustomOpenGLWidget::keyPressEvent (QKeyEvent* event)
{
    if (view.IsNull()) return;

    // define hot keys for specific functionality
    const Aspect_VKey aKey=OcctQtTools::qtKey2VKey(event->key());
    switch (aKey)
    {
        case Aspect_VKey_Escape: {
            finishPickVertex(true);
        }

        // case Aspect_VKey_F: {
        //     view->FitAll(0.01, false);
        //     update();
        //     return;
        // }
    }

    QOpenGLWidget::keyPressEvent(event);
}

bool CustomOpenGLWidget::PixelToPointOnPlane (const Standard_Integer xPix, const Standard_Integer yPix, gp_Pnt& thePoint3D)
{
    Standard_Real Xv,Yv,Zv;
    Standard_Real Vx,Vy,Vz;

    view->ConvertWithProj(xPix,yPix,Xv,Yv,Zv,Vx,Vy,Vz);

    gp_Pnt eyePnt(Xv,Yv,Zv);
    gp_Dir viewDir(Vx,Vy,Vz);
    gp_Lin viewRay(eyePnt,viewDir);

    IntAna_IntConicQuad intersector(viewRay, drawingPlane, Precision::Confusion());

    if (intersector.IsDone() && !intersector.IsParallel() && intersector.NbPoints() > 0) {
        thePoint3D=intersector.Point(1);
        return false;
    }

    //std::cout << "CustomOpenGLWidget::PixelToPointOnPlane  return true" << std::endl; std::cout.flush();
    return true;
}

// vertex draw or pick
void CustomOpenGLWidget::finishPickVertex (bool cancel)
{
    std::cout << "CustomOpenGLWidget::finishPickVertex  cancel=" << cancel << std::endl; std::cout.flush();

    // remove temporaryVertex
    if (!temporaryVertex.IsNull()) {
       viewerContext->Remove(temporaryVertex,Standard_True);
       temporaryVertex.Nullify();
    }

    emit relay->getPickedVertex(vertexPoint,cancel);
}

void CustomOpenGLWidget::mousePressEvent (QMouseEvent* event)
{
    std::cout << "CustomOpenGLWidget::mousePressEvent   pickFirstVertex=" << pickFirstVertex << std::endl; std::cout.flush();

    QOpenGLWidget::mousePressEvent(event);
    if (view.IsNull()) return;

    // point click position
    QPointF pos=event->position();

    // get a gp_Pnt
    clickPointValid=false;
    if (viewer->IsGridActive() && snapToGrid) {
        Standard_Real X,Y,Z;
        view->ConvertToGrid(pos.x(),pos.y(),X,Y,Z);
        clickPoint.SetCoord(X,Y,Z);
    } else {
        if (!PixelToPointOnPlane (pos.x(),pos.y(),clickPoint)) clickPointValid=true;
    }

    Handle(SelectMgr_EntityOwner) owner=viewerContext->DetectedOwner();

    if (event->button() == Qt::LeftButton) {

        if (pickFirstVertex /*&& clickPointValid*/) {
            ignoreMouseRelease=true;
            if (owner.IsNull()) {
                vertexPoint=clickPoint;
                finishPickVertex(false);
            } else {
                Handle(StdSelect_BRepOwner) brepOwner=Handle(StdSelect_BRepOwner)::DownCast(owner);
                if (!brepOwner.IsNull()) {
                    TopoDS_Shape shape = brepOwner->Shape();
                    if (!shape.IsNull()) {
                        if (shape.ShapeType() == TopAbs_VERTEX) {
                            TopoDS_Vertex vertex=TopoDS::Vertex(shape);
                            gp_Pnt pnt=BRep_Tool::Pnt(vertex);
                            if (!vertex.IsNull()) {
                                vertexPoint=pnt;
                                finishPickVertex(false);
                            }
                        }
                    }
                }
            }
        }
    }

    // pass the mouse press from Qt to OCCT
    bool passClick=true;
    if (event->button() == Qt::RightButton && viewerContext->NbSelected() > 0) passClick=false;  // a popup menu will appear
    if (event->button() == Qt::RightButton && pickFirstVertex) passClick=false;   // prevent right-click from zooming
    if (passClick) {
        const Graphic3d_Vec2i  point(event->pos().x(),event->pos().y());
        const Aspect_VKeyFlags flags=OcctQtTools::qtMouseModifiers2VKeys(event->modifiers());
        if (UpdateMouseButtons(point,OcctQtTools::qtMouseButtons2VKeys(event->buttons()),flags,false)) updateViewer();
    }
}

void CustomOpenGLWidget::mouseReleaseEvent (QMouseEvent* event)
{
    std::cout << "CustomOpenGLWidget::mouseReleaseEvent   pickSecondVertex=" << pickSecondVertex << "  ignoreMouseRelease=" << ignoreMouseRelease << std::endl; std::cout.flush();

    QOpenGLWidget::mouseReleaseEvent(event);
    if (view.IsNull()) return;

    // pass the mouse release from Qt to OCCT
    bool passClick=true;
    if (event->button() == Qt::RightButton && viewerContext->NbSelected() > 0) passClick=false;  // a popup menu will appear
    if (passClick) {
        const Graphic3d_Vec2i  point(event->pos().x(),event->pos().y());
        const Aspect_VKeyFlags flags=OcctQtTools::qtMouseModifiers2VKeys(event->modifiers());
        if (UpdateMouseButtons(point,OcctQtTools::qtMouseButtons2VKeys(event->buttons()),flags,false)) updateViewer();
    }

    Handle(SelectMgr_EntityOwner) owner=viewerContext->DetectedOwner();

    // process for vertex click
    if (event->button() == Qt::LeftButton) {
        if (pickSecondVertex /*&& clickPointValid*/) {
            ignoreMouseRelease=true;
            if (owner.IsNull()) {
                vertexPoint=clickPoint;
                finishPickVertex(false);
            } else {
                Handle(StdSelect_BRepOwner) brepOwner=Handle(StdSelect_BRepOwner)::DownCast(owner);
                if (!brepOwner.IsNull()) {
                    TopoDS_Shape shape = brepOwner->Shape();
                    if (!shape.IsNull()) {
                        if (shape.ShapeType() == TopAbs_VERTEX) {
                            TopoDS_Vertex vertex=TopoDS::Vertex(shape);
                            gp_Pnt pnt=BRep_Tool::Pnt(vertex);
                            if (!vertex.IsNull()) {
                                vertexPoint=pnt;
                                finishPickVertex(false);
                            }
                        }
                    }
                }
            }
        }
    }

    // process for object selection
    if (event->button() == Qt::LeftButton) {
        if (!ignoreMouseRelease /*&& !pickVertex*/) {

            // check for CTRL and SHIFT then select
            bool hasModifier=false;
            if (event->button() == Qt::LeftButton) {
                AIS_SelectionScheme scheme;

                if (event->modifiers() & Qt::ControlModifier) {
                    hasModifier=true;
                    scheme=AIS_SelectionScheme_Add;
                } else if (event->modifiers() & Qt::ShiftModifier) {
                    hasModifier=true;
                    scheme=AIS_SelectionScheme_Add;
                } else {
                    scheme=AIS_SelectionScheme_Replace;
                }

                viewerContext->SelectDetected(scheme);
            }

            // clear or add to item tracking
            Handle(AIS_InteractiveObject) anIO=getLastSelected();
            if (anIO.IsNull()) {
                emit relay->clearTreeSelection();
            } else {

                // *** important GUI functionality ***
                // cross select into the tree menu from selected item in the drawing window
                if (hasModifier) {

                    // make a list of the selected shapes to enable clearing the tree of stale selects
                    std::vector<Handle(AIS_Shape)> shapeList;
                    for (viewerContext->InitSelected(); viewerContext->MoreSelected(); viewerContext->NextSelected()) {
                        Handle(AIS_InteractiveObject) object=viewerContext->SelectedInteractive();
                        Handle(AIS_Shape) shape=Handle(AIS_Shape)::DownCast(object);
                        if (!shape.IsNull()) {shapeList.push_back(shape);}
                    }

                    // clear the tree
                    emit relay->clearTreeSelection();

                    // add to the selection database
                    long unsigned int i=0;
                    while (i < shapeList.size()) {
                        drawingTracker->selectItemShape(shapeList[i]);
                        i++;
                    }
                } else {
                    Handle(AIS_Shape) shape=Handle(AIS_Shape)::DownCast(anIO);
                    if (!shape.IsNull()) {
                        emit relay->clearTreeSelection();
                        drawingTracker->selectItemShape(shape);
                    }
                }
            }
        }
        updateViewer();

    } else if (event->button() == Qt::RightButton) {
        if (viewerContext->NbSelected() > 0) {
            contextMenu->exec(QCursor::pos());
        }
    }

    ignoreMouseRelease=false;
}

Handle(AIS_InteractiveObject) CustomOpenGLWidget::getLastSelected ()
{
    Handle(AIS_InteractiveObject) io;
    viewerContext->InitSelected();
    while (viewerContext->MoreSelected()) {
        io=viewerContext->SelectedInteractive();
        viewerContext->NextSelected();
    }
    return io;
}

void CustomOpenGLWidget::mouseMoveEvent (QMouseEvent* event)
{
    //std::cout << "CustomOpenGLWidget::mouseMoveEvent  polywire=" << polywire << std::endl; std::cout.flush();

    QOpenGLWidget::mouseMoveEvent(event);

    if (view.IsNull()) return;

    if (viewerContext->HasDetected()) {
        Handle(SelectMgr_EntityOwner) anOwner = viewerContext->DetectedOwner();
        Handle(StdSelect_BRepOwner) aBRepOwner = Handle(StdSelect_BRepOwner)::DownCast(anOwner);
        if (!aBRepOwner.IsNull()) {
            TopoDS_Shape detectedShape = aBRepOwner->Shape();

            // create a temporary vertex at the mid point
            if (detectedShape.ShapeType() == TopAbs_EDGE) {

                // clean up
                if (!temporaryVertex.IsNull()) {
                    viewerContext->Remove(temporaryVertex,Standard_True);
                    temporaryVertex.Nullify();
                }

                // get the mid point
                const TopoDS_Edge& edge=TopoDS::Edge(detectedShape);
                TopoDS_Vertex v1=TopExp::FirstVertex(edge);
                const gp_Pnt pnt1=BRep_Tool::Pnt(v1);
                TopoDS_Vertex v2=TopExp::LastVertex(edge);
                const gp_Pnt pnt2=BRep_Tool::Pnt(v2);
                gp_Pnt pntmid((pnt1.X()+pnt2.X())/2.0,(pnt1.Y()+pnt2.Y())/2.0,(pnt1.Z()+pnt2.Z())/2.0);

                // create vertex
                TopoDS_Vertex newVertex=BRepBuilderAPI_MakeVertex(pntmid);
                temporaryVertex=new AIS_Shape(newVertex);
                viewerContext->Display(temporaryVertex,Standard_False);
                viewerContext->Load(temporaryVertex);
                viewerContext->Activate(temporaryVertex,0,Standard_True);
                viewerContext->UpdateCurrentViewer();
            }
        }

    } else {
        // remove the temporary vertex
        if (!temporaryVertex.IsNull()) {
            viewerContext->Remove(temporaryVertex,Standard_True);
            temporaryVertex.Nullify();
        }
    }

    // for rubberband

    Standard_Real x,y,z;
    view->Convert(event->pos().x(),event->pos().y(),x,y,z);
    gp_Pnt mousePosition(x,y,z);
    if (!PixelToPointOnPlane (event->pos().x(),event->pos().y(),mousePosition)) {
        emit relay->getCurrentMousePosition(mousePosition);
    }

    // pass the mouse position from Qt to OCCT
    const Graphic3d_Vec2i position(event->pos().x(),event->pos().y());
    if (UpdateMousePosition(position,OcctQtTools::qtMouseButtons2VKeys(event->buttons()),
                                     OcctQtTools::qtMouseModifiers2VKeys(event->modifiers()),false)) updateViewer();

    //std::cout << "exit CustomOpenGLWidget::mouseMoveEvent  polywire=" << polywire << std::endl; std::cout.flush();
}

void CustomOpenGLWidget::set_gridPlane (TopoDS_Face &face)
{
    //std::cout << "CustomOpenGLWidget::set_gridPlane" << std::endl; std::cout.flush();

    if (!face.IsNull()) {

        // plane
        Handle(Geom_Surface) surface=BRep_Tool::Surface(face);
        Handle(Geom_Plane) plane=Handle(Geom_Plane)::DownCast(surface);

        // set
        //drawingPlane=plane->Pln();
        //viewer->SetPrivilegedPlane(drawingPlane.Position());

        gp_Pnt origin=plane->Location();
        gp_Dir axis=plane->Axis().Direction();

        set_gridPlane(origin,axis);
    }
}

void CustomOpenGLWidget::set_gridPlane (gp_Pnt &origin, gp_Dir &normal)
{
    gp_Ax3 system(origin,normal);
    Handle(Geom_Plane) plane=new Geom_Plane(system);
    //drawingPlane=plane->Pln();
    //viewer->SetPrivilegedPlane(drawingPlane.Position());
    gp_Pln barePlane=plane->Pln();
    set_gridPlane(barePlane);
}

void CustomOpenGLWidget::set_gridPlane (gp_Pln &plane)
{
    drawingPlane=plane;
    viewer->SetPrivilegedPlane(drawingPlane.Position());
}

void CustomOpenGLWidget::showGrid ()
{
    //Handle(V3d_RectangularGrid) grid=new V3d_RectangularGrid(&(*viewer),Quantity_NOC_GRAY80,Quantity_NOC_GRAY50);
    viewer->ActivateGrid(Aspect_GT_Rectangular,Aspect_GDM_Lines);

    Standard_Real xOrigin,yOrigin,xStep,yStep,rotationAngle,xSize,ySize,offset;
    viewer->RectangularGridValues(xOrigin,yOrigin,xStep,yStep,rotationAngle);
    viewer->RectangularGridGraphicValues(xSize,ySize,offset);
    std::cout << "Origin: (" << xOrigin << "," << yOrigin << "), step: (" << xStep << "," << yStep << ")" << ", size: (" << xSize << "," << ySize << "), offset=" << offset << std::endl; std::cout.flush();

    xStep=0.01; yStep=0.01;
    viewer->SetRectangularGridValues(xOrigin,yOrigin,xStep,yStep,rotationAngle);

    xSize=10*xStep; ySize=10*yStep; offset=0;
    viewer->SetRectangularGridGraphicValues(xSize,ySize,offset);
}

void CustomOpenGLWidget::hideGrid ()
{
    //std::cout << "CustomOpenGLWidget::hideGrid" << std::endl; std::cout.flush();

    viewer->DeactivateGrid();
}

void CustomOpenGLWidget::selectRectangle ()
{
    if (rectSelect) delete rectSelect;
    rectSelect=new RectangleSelector(viewerContext,view,this);

    connect(rectSelect,&RectangleSelector::selectionFinished,this,&CustomOpenGLWidget::endSelectRectangle);
}

void CustomOpenGLWidget::endSelectRectangle ()
{
    viewerContext->InitSelected();
    while (viewerContext->MoreSelected()) {
        Handle(AIS_InteractiveObject) anIO = viewerContext->SelectedInteractive();
        Handle(AIS_Shape) shape = Handle(AIS_Shape)::DownCast(anIO);
        drawingTracker->selectItemShape(shape);
        viewerContext->NextSelected();
    }

    if (rectSelect) delete rectSelect;
    rectSelect=nullptr;
    updateViewer();
}

