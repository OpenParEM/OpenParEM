#ifndef POLYCIRCLEEDITFORM_H
#define POLYCIRCLEEDITFORM_H

#include "CustomOpenGLWidget.h"
#include "Polywire.h"
#include <QDialog>
#include <gp_Dir.hxx>
#include <qvalidator.h>

namespace Ui {
class PolycircleEditForm;
}

class PolycircleEditForm : public QDialog
{
    Q_OBJECT

public:
    explicit PolycircleEditForm(QWidget *parent = nullptr);
    ~PolycircleEditForm();

    void set_Polycircle (Polycircle *);
    void set_drawingWindow (CustomOpenGLWidget *drawingWindow_) {drawingWindow=drawingWindow_;}
    void set_relay (Relay *relay_) {relay=relay_;}
    void pickVertexFinished (gp_Pnt);

    void reject () override;

public slots:
    void on_CancelButton_clicked ();

private slots:
    void on_centerPositionX_returnPressed ();
    void on_centerPositionY_returnPressed ();
    void on_centerPositionZ_returnPressed ();
    void on_pickCenter_clicked ();
    void on_radius_returnPressed ();
    void on_firstPositionX_returnPressed ();
    void on_firstPositionY_returnPressed ();
    void on_firstPositionZ_returnPressed ();
    void on_pickFirst_clicked ();
    void on_vertexCount_returnPressed ();
    void on_OkButton_clicked ();

private:
    Ui::PolycircleEditForm *ui;

    Polycircle *polycircle;

    bool pickCenterPoint;
    bool pickFirstPoint;
    QDoubleValidator doubleValidator;
    QIntValidator intValidator;
    CustomOpenGLWidget *drawingWindow;
    Relay *relay;
};

#endif // POLYCIRCLEEDITFORM_H
