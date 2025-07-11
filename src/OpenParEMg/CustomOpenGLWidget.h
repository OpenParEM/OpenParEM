#ifndef CUSTOMOPENGLWIDGET_H
#define CUSTOMOPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QMessageBox>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <V3d_View.hxx>
#include "Aspect_DisplayConnection.hxx"
#include "OpenGl_GraphicDriver.hxx"
#include "OpenGl_View.hxx"
#include "OpenGl_Context.hxx"
#include "OpenGl_Window.hxx"
#include "V3d_Viewer.hxx"
#include "AIS_InteractiveContext.hxx"
#include "AIS_Shape.hxx"
#include "Aspect_NeutralWindow.hxx"
#include "BRepPrimAPI_MakeBox.hxx"
#include "Aspect_NeutralWindow.hxx"
#include "AIS_ViewCube.hxx"
#include "AIS_ViewController.hxx"

using namespace std;

//! Auxiliary wrapper to avoid OpenGL macros collisions between Qt and OCCT headers.
class OcctGlTools
{
public:
    //! Return GL context.
    static Handle(OpenGl_Context) GetGlContext(const Handle(V3d_View)& theView);
};


//! OpenGL FBO subclass for wrapping FBO created by Qt using GL_RGBA8
//! texture format instead of GL_SRGB8_ALPHA8.
//! This FBO is set to OpenGl_Context::SetDefaultFrameBuffer() as a final target.
//! Subclass calls OpenGl_Context::SetFrameBufferSRGB() with sRGB=false flag,
//! which asks OCCT to disable GL_FRAMEBUFFER_SRGB and apply sRGB gamma correction manually.
class OcctQtFrameBuffer : public OpenGl_FrameBuffer
{
    DEFINE_STANDARD_RTTI_INLINE(OcctQtFrameBuffer, OpenGl_FrameBuffer)
public:
    //! Empty constructor.
    OcctQtFrameBuffer() {}

    //! Make this FBO active in context.
    void BindBuffer(const Handle(OpenGl_Context)& theGlCtx) override
    {
        cout << "OcctQtFrameBuffer:BindBuffer" << endl;
        OpenGl_FrameBuffer::BindBuffer(theGlCtx);
        //theGlCtx->SetFrameBufferSRGB(true, false);
    }

    //! Make this FBO as drawing target in context.
    void BindDrawBuffer(const Handle(OpenGl_Context)& theGlCtx) override
    {
        cout << "OcctQtFrameBuffer:BindDrawBuffer" << endl;
        OpenGl_FrameBuffer::BindDrawBuffer(theGlCtx);
        //theGlCtx->SetFrameBufferSRGB(true, false);
    }

    //! Make this FBO as reading source in context.
    void BindReadBuffer(const Handle(OpenGl_Context)& theGlCtx) override
    {
        cout << "OcctQtFrameBuffer:BindReadBuffer" << endl;
        OpenGl_FrameBuffer::BindReadBuffer(theGlCtx);
    }
};



class CustomOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions, public AIS_ViewController
{
    Q_OBJECT
public:
    CustomOpenGLWidget(QWidget *parent = nullptr);
    virtual ~CustomOpenGLWidget();
    const Handle(V3d_Viewer)& Viewer() const { return viewer; }
    const Handle(V3d_View)& View() const { return view; }
    const Handle(AIS_InteractiveContext)& Context() const { return viewerContext; }
    const QString& getGlInfo() const { return myGlInfo; }
    virtual QSize minimumSizeHint() const override { return QSize(200, 200); }
    virtual QSize sizeHint() const override { return QSize(720, 480); }
    virtual void handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx, const Handle(V3d_View)& theView) override;
    void setDrawing (Handle(AIS_Shape) drawing_) {drawing=drawing_;}
    void showDrawing (Handle(AIS_Shape) drawing_) {
        viewerContext->Display(drawing, 0, 0, false);
        view->InvalidateImmediate();
        FlushViewEvents (viewerContext, view, true);
    }
    void dumpGlInfo(bool, bool);
    void updateView();

    void displayDrawing (Handle(AIS_Shape));

    void wheelEvent(QWheelEvent* theEvent) override;

protected:
    void initializeGL() override;
    //void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    Handle(Aspect_DisplayConnection) aDisp;
    Handle(OpenGl_GraphicDriver) aDriver;
    Handle(V3d_Viewer) viewer;
    Handle(V3d_View) view;
    Handle(AIS_InteractiveContext) viewerContext;
    Handle(AIS_Shape) drawing;

    Handle(V3d_View) focusView;

    Handle(AIS_ViewCube) myViewCube;

    QString myGlInfo;

    bool myIsCoreProfile=false;
};

#endif // CUSTOMOPENGLWIDGET_H
