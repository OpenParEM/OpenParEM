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
#include <glx.h>
#include "OcctQtTools.h"

#include <QWidget>
#include <QResizeEvent>
#include <QColorSpace>

#include <V3d_View.hxx>
#include <TopoDS_Shape.hxx>
#include <AIS_Shape.hxx>
#include <BRepTools.hxx>
#include "Aspect_NeutralWindow.hxx"
#include <OpenGl_FrameBuffer.hxx>

CustomOpenGLWidget::CustomOpenGLWidget (QWidget* theParent) : QOpenGLWidget (theParent)
{
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

    // create an orientation cube for the display
    viewCube=new AIS_ViewCube();
    viewCube->SetViewAnimation(myViewAnimation);
    viewCube->SetFixedAnimationLoop(false);
    viewCube->SetAutoStartAnimation(true);
    viewCube->TransformPersistence()->SetOffset2d(Graphic3d_Vec2i(100, 150));

    // viewer
    view=viewer->CreateView();
    view->SetImmediateUpdate(false);
    view->ChangeRenderingParams().NbMsaaSamples=4;
    view->ChangeRenderingParams().ToShowStats=false;
}

CustomOpenGLWidget::~CustomOpenGLWidget ()
{
    viewerContext->RemoveAll(false);
    viewerContext.Nullify();
    view->Remove();
    view.Nullify();
    viewer.Nullify();

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

void CustomOpenGLWidget::updateView ()
{
    update();
}

void CustomOpenGLWidget::clearDrawing ()
{
    viewerContext->RemoveAll(false);
    viewerContext->UpdateCurrentViewer();
}

void CustomOpenGLWidget::displayDrawing (Handle(AIS_Shape) shape)
{
    viewerContext->Display(shape,0,0,false);
    viewerContext->UpdateCurrentViewer();

    // scale to fit the new object
    //viewerContext->SetSelected(shape,false);
    //viewerContext->FitSelected(view,0,false);
}

void CustomOpenGLWidget::wheelEvent (QWheelEvent* event)
{
    QOpenGLWidget::wheelEvent(event);
    if (view.IsNull()) return;

    const Graphic3d_Vec2i position(Graphic3d_Vec2d(event->position().x(),event->position().y()));
    if (UpdateZoom(Aspect_ScrollDelta(position,double(event->angleDelta().y())/8.0))) updateView();
}

void CustomOpenGLWidget::keyPressEvent (QKeyEvent* event)
{
    if (view.IsNull()) return;

    // define hot keys for specific functionality
    /*
    const Aspect_VKey aKey=OcctQtTools::qtKey2VKey(event->key());
    switch (aKey)
    {
        case Aspect_VKey_Escape: {
            //QApplication::exit();
            return;
        }
        case Aspect_VKey_F: {
            view->FitAll(0.01, false);
            update();
            return;
        }
    }
*/
    QOpenGLWidget::keyPressEvent(event);
}

void CustomOpenGLWidget::mousePressEvent (QMouseEvent* event)
{
    QOpenGLWidget::mousePressEvent(event);
    if (view.IsNull()) return;

    // pass the mouse press from Qt to OCCT
    const Graphic3d_Vec2i  point(event->pos().x(),event->pos().y());
    const Aspect_VKeyFlags flags=OcctQtTools::qtMouseModifiers2VKeys(event->modifiers());
    if (UpdateMouseButtons(point,OcctQtTools::qtMouseButtons2VKeys(event->buttons()),flags,false)) updateView();
}

void CustomOpenGLWidget::mouseReleaseEvent (QMouseEvent* event)
{
    QOpenGLWidget::mouseReleaseEvent(event);
    if (view.IsNull()) return;

    // pass the mouse release from Qt to OCCT
    const Graphic3d_Vec2i  point(event->pos().x(),event->pos().y());
    const Aspect_VKeyFlags flags=OcctQtTools::qtMouseModifiers2VKeys(event->modifiers());
    if (UpdateMouseButtons(point,OcctQtTools::qtMouseButtons2VKeys(event->buttons()),flags,false)) updateView();
}

void CustomOpenGLWidget::mouseMoveEvent(QMouseEvent* event)
{
    QOpenGLWidget::mouseMoveEvent(event);
    if (view.IsNull()) return;

    // pass the mouse position from Qt to OCCT
    const Graphic3d_Vec2i position(event->pos().x(),event->pos().y());
    if (UpdateMousePosition(position,OcctQtTools::qtMouseButtons2VKeys(event->buttons()),
                                     OcctQtTools::qtMouseModifiers2VKeys(event->modifiers()),false)) updateView();
}
