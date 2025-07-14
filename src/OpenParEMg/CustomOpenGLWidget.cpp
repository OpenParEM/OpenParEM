#include "CustomOpenGLWidget.h"
#include <V3d_View.hxx>
#include <iostream>

#include <QWidget>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <OpenGl_GraphicDriver.hxx>

//#include "BreptViewerWidget.h"
#include <Aspect_DisplayConnection.hxx>
#include <Xw_Window.hxx>
#include <TopoDS_Shape.hxx>
#include <AIS_Shape.hxx>
#include <QResizeEvent>

#include <Standard_Stream.hxx>


#include <BRepTools.hxx>
#include <TopoDS_Shape.hxx>

#include <glx.h>


CustomOpenGLWidget::CustomOpenGLWidget(QWidget* theParent)
    : QOpenGLWidget(theParent)
{
  aDisp   = new Aspect_DisplayConnection();

  aDriver = new OpenGl_GraphicDriver(aDisp);
  //aDriver->ChangeOptions().buffersNoSwap = true;
  //aDriver->ChangeOptions().buffersOpaqueAlpha = true;
  aDriver->ChangeOptions().useSystemBuffer = false;

  // create viewer
  viewer = new V3d_Viewer(aDriver);
  viewer->SetDefaultBackgroundColor(Quantity_NOC_BLACK);
  viewer->SetDefaultLights();
  viewer->SetLightOn();
  //viewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);

  // create AIS context
  viewerContext = new AIS_InteractiveContext(viewer);

  myViewCube = new AIS_ViewCube();
  myViewCube->SetViewAnimation(myViewAnimation);
  myViewCube->SetFixedAnimationLoop(false);
  myViewCube->SetAutoStartAnimation(true);
  myViewCube->TransformPersistence()->SetOffset2d(Graphic3d_Vec2i(100, 150));

  view = viewer->CreateView();
  view->SetImmediateUpdate(false);
  view->ChangeRenderingParams().NbMsaaSamples=4;
  view->ChangeRenderingParams().ToShowStats=false;
  //view->ChangeRenderingParams().CollectedStats = (Graphic3d_RenderingParams::PerfCounters)(
  //  Graphic3d_RenderingParams::PerfCounters_FrameRate | Graphic3d_RenderingParams::PerfCounters_Triangles);

  // Qt widget setup
  setMouseTracking(true);
  setBackgroundRole(QPalette::NoRole);
  setFocusPolicy(Qt::StrongFocus);
  setUpdatesEnabled(true);
  setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

  // OpenGL setup managed by Qt
  QSurfaceFormat aGlFormat;
  aGlFormat.setDepthBufferSize(24);
  aGlFormat.setStencilBufferSize(8);
  aGlFormat.setColorSpace (QSurfaceFormat::sRGBColorSpace);
  setTextureFormat (GL_SRGB8_ALPHA8);

  setFormat(aGlFormat);
}


CustomOpenGLWidget::~CustomOpenGLWidget()
{
    cout << "CustomOpenGLWidget::~CustomOpenGLWidget" << endl;

    Handle(Aspect_DisplayConnection) aDisp = viewer->Driver()->GetDisplayConnection();

    // release OCCT viewer
    viewerContext->RemoveAll(false);
    viewerContext.Nullify();
    view->Remove();
    view.Nullify();
    viewer.Nullify();

    // make active OpenGL context created by Qt
    makeCurrent();
    aDisp.Nullify();
}

void CustomOpenGLWidget::initializeGL ()
{
    cout << "CustomOpenGLWidget::initializeGL" << endl;

    initializeOpenGLFunctions(); // Required for using OpenGL functions
    //glClearColor(0.5f, 0.0f, 0.0f, 1.0f); // Set background color


    const QRect           aRect = rect();
    const Graphic3d_Vec2i aViewSize(aRect.right() - aRect.left(), aRect.bottom() - aRect.top());

    Aspect_Drawable aNativeWin = (Aspect_Drawable)winId();



    Handle(OpenGl_Context) aGlCtx = new OpenGl_Context();
    if (!aGlCtx->Init())
    {
        Message::SendFail() << "Error: OpenGl_Context is unable to wrap OpenGL context";
        QMessageBox::critical(0, "Failure", "OpenGl_Context is unable to wrap OpenGL context");
        //QApplication::exit(1);
        return;
    }

    Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast(view->Window());
    if (!aWindow.IsNull())
    {
        //aWindow->SetNativeHandle(aNativeWin);
        aWindow->SetSize(aViewSize.x(), aViewSize.y());
        view->SetWindow(aWindow, aGlCtx->RenderingContext());
        //dumpGlInfo(true, true);
    }
    else
    {
        aWindow = new Aspect_NeutralWindow();

        Aspect_Drawable aNativeWin = (Aspect_Drawable )winId();
        aWindow->SetNativeHandle (aNativeWin);
        aWindow->SetSize (aViewSize.x(), aViewSize.y());
        view->SetWindow (aWindow, (Aspect_RenderingContext) glXGetCurrentContext());
        //dumpGlInfo (true);

        //viewerContext->Display (myViewCube, 0, 0, false);
    }

    {
        // dummy shape for testing
        //TopoDS_Shape      aBox   = BRepPrimAPI_MakeBox(100.0, 50.0, 90.0).Shape();
        //Handle(AIS_Shape) aShape = new AIS_Shape(aBox);
        //viewerContext->Display(aShape, 0, 0, false);
    }


}

void CustomOpenGLWidget::paintGL ()
{
    cout << "CustomOpenGLWidget::paintGL" << endl;

    if (view->Window().IsNull())
    {
        return;
    }

    glBegin(GL_LINES);
    // Define the start point of the line
    glVertex2f(-0.5f, -0.5f);
    // Define the end point of the line
    glVertex2f(0.5f, 0.5f);
    glEnd();

    // wrap FBO created by QOpenGLWidget
    Handle(OpenGl_GraphicDriver) aDriver = Handle(OpenGl_GraphicDriver)::DownCast (viewerContext->CurrentViewer()->Driver());
    const Handle(OpenGl_Context)& aGlCtx = aDriver->GetSharedContext();
    Handle(OpenGl_FrameBuffer) aDefaultFbo = aGlCtx->DefaultFrameBuffer();
    if (aDefaultFbo.IsNull())
    {
        aDefaultFbo = new OpenGl_FrameBuffer();
        aGlCtx->SetDefaultFrameBuffer (aDefaultFbo);
    }
    if (!aDefaultFbo->InitWrapper (aGlCtx))
    {
        aDefaultFbo.Nullify();
        Message::DefaultMessenger()->Send ("Default FBO wrapper creation failed", Message_Fail);
        QMessageBox::critical (0, "Failure", "Default FBO wrapper creation failed");
        //QApplication::exit (1);
        return;
    }

    Graphic3d_Vec2i aViewSizeOld;
    //const QRect aRect = rect(); Graphic3d_Vec2i aViewSizeNew(aRect.right() - aRect.left(), aRect.bottom() - aRect.top());
    Graphic3d_Vec2i aViewSizeNew(aDefaultFbo->GetVPSizeX(),aDefaultFbo->GetVPSizeY());
    Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast (view->Window());
    aWindow->Size (aViewSizeOld.x(), aViewSizeOld.y());
    if (aViewSizeNew != aViewSizeOld)
    {
        aWindow->SetSize (aViewSizeNew.x(), aViewSizeNew.y());
        view->MustBeResized();
        view->Invalidate();
    }

    // test shape
    TopoDS_Shape      aBox   = BRepPrimAPI_MakeBox(100.0, 50.0, 90.0).Shape();
    Handle(AIS_Shape) aShape = new AIS_Shape(aBox);
    cout << "aShape.IsNull()=" << aShape.IsNull() << endl;
    //viewerContext->Display(aShape, 0, 0, false);


    TopoDS_Shape      aBox2   = BRepPrimAPI_MakeBox(200.0, 100.0, 180.0).Shape();
    Handle(AIS_Shape) aShape2 = new AIS_Shape(aBox2);
    cout << "aShape2.IsNull()=" << aShape2.IsNull() << endl;
    //viewerContext->Display(aShape2, 0, 0, false);

    // show drawing
    //cout << "drawing.IsNull()=" << drawing.IsNull() << endl;
    //viewerContext->Display(drawing, 0, 0, true);

    viewerContext->Display (myViewCube, 0, 0, false);

    // flush pending input events and redraw the viewer
    view->InvalidateImmediate();
    FlushViewEvents (viewerContext, view, true);
}

void CustomOpenGLWidget::updateView()
{
    cout << "CustomOpenGLWidget::updateView" << endl;
    update();
    // if (window() != NULL) { window()->update(); }
}

void CustomOpenGLWidget::handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                               const Handle(V3d_View)&               theView)
{
    cout << "CustomOpenGLWidget::handleViewRedraw" << endl;
    AIS_ViewController::handleViewRedraw(theCtx, theView);
    if (myToAskNextFrame)
        updateView(); // ask more frames for animation
}

void CustomOpenGLWidget::dumpGlInfo(bool theIsBasic, bool theToPrint)
{
    cout << "CustomOpenGLWidget::dumpGlInfo" << endl;

    TColStd_IndexedDataMapOfStringString aGlCapsDict;
    view->DiagnosticInformation(aGlCapsDict,
                                  theIsBasic ? Graphic3d_DiagnosticInfo_Basic : Graphic3d_DiagnosticInfo_Complete);
    TCollection_AsciiString anInfo;
    for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aGlCapsDict); aValueIter.More(); aValueIter.Next())
    {
        if (!aValueIter.Value().IsEmpty())
        {
            if (!anInfo.IsEmpty())
                anInfo += "\n";

            anInfo += aValueIter.Key() + ": " + aValueIter.Value();
        }
    }

    if (theToPrint)
        Message::SendInfo(anInfo);

    myGlInfo=QString::fromUtf8(anInfo.ToCString());
}

void CustomOpenGLWidget::displayDrawing (Handle(AIS_Shape) drawing)
{
    // clear prior objects
    viewerContext->RemoveAll(false);

    // show the new object
    viewerContext->Display(drawing, 0, 0, false);
    viewerContext->UpdateCurrentViewer();

    // scale to fit the new object
    viewerContext->SetSelected(drawing,false);
    viewerContext->FitSelected(view,0,false);
}

void CustomOpenGLWidget::wheelEvent(QWheelEvent* theEvent)
{
    QOpenGLWidget::wheelEvent(theEvent);
    if (view.IsNull()) return;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    const Graphic3d_Vec2i aPos(Graphic3d_Vec2d(theEvent->position().x(), theEvent->position().y()));
#else
    const Graphic3d_Vec2i aPos(theEvent->pos().x(), theEvent->pos().y());
#endif
    if (!view->Subviews().IsEmpty())
    {
        Handle(V3d_View) aPickedView = view->PickSubview(aPos);
        if (!aPickedView.IsNull() && aPickedView != focusView)
        {
            // switch input focus to another subview
            OnSubviewChanged(viewerContext, focusView, aPickedView);
            updateView();
            return;
        }
    }

    if (UpdateZoom(Aspect_ScrollDelta(aPos, double(theEvent->angleDelta().y()) / 8.0)))
        updateView();
}

void CustomOpenGLWidget::keyPressEvent(QKeyEvent* event)
{
    if (view.IsNull()) return;

    const Aspect_VKey aKey = OcctQtTools::qtKey2VKey(event->key());
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
    QOpenGLWidget::keyPressEvent(event);
}

void CustomOpenGLWidget::mousePressEvent(QMouseEvent* event)
{
    QOpenGLWidget::mousePressEvent(event);
    if (view.IsNull()) return;

    const Graphic3d_Vec2i  aPnt(event->pos().x(), event->pos().y());
    const Aspect_VKeyFlags aFlags = OcctQtTools::qtMouseModifiers2VKeys(event->modifiers());
    if (UpdateMouseButtons(aPnt, OcctQtTools::qtMouseButtons2VKeys(event->buttons()), aFlags, false))
        updateView();
}

void CustomOpenGLWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QOpenGLWidget::mouseReleaseEvent(event);
    if (view.IsNull()) return;

    const Graphic3d_Vec2i  aPnt(event->pos().x(), event->pos().y());
    const Aspect_VKeyFlags aFlags = OcctQtTools::qtMouseModifiers2VKeys(event->modifiers());
    if (UpdateMouseButtons(aPnt, OcctQtTools::qtMouseButtons2VKeys(event->buttons()), aFlags, false))
        updateView();
}

void CustomOpenGLWidget::mouseMoveEvent(QMouseEvent* event)
{
    QOpenGLWidget::mouseMoveEvent(event);
    if (view.IsNull()) return;

    const Graphic3d_Vec2i aNewPos(event->pos().x(), event->pos().y());
    if (UpdateMousePosition(aNewPos,
                            OcctQtTools::qtMouseButtons2VKeys(event->buttons()),
                            OcctQtTools::qtMouseModifiers2VKeys(event->modifiers()),
                            false))
    {
        updateView();
    }
}
