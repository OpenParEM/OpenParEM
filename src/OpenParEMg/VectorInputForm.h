#ifndef VECTORINPUTFORM_H
#define VECTORINPUTFORM_H

#include "CustomOpenGLWidget.h"
#include <QDialog>
#include <gp_Pnt.hxx>

namespace Ui {
class VectorInputForm;
}

class VectorInputForm : public QDialog
{
    Q_OBJECT

public:
    explicit VectorInputForm(QWidget *parent = nullptr);
    ~VectorInputForm();

    void set_startPoint (gp_Pnt *);
    void set_endPoint (gp_Pnt *);
    void set_drawingWindow (CustomOpenGLWidget *drawingWindow_) {drawingWindow=drawingWindow_;}
    void set_relay (Relay *relay_) {relay=relay_;}

    void set_conversionFactor (double conversionFactor_) {conversionFactor=conversionFactor_;}

    void reject () override;

private slots:
    void on_pickOrigin_clicked ();
    void on_pickTip_clicked ();
    void on_OkButton_clicked ();

public slots:
    void pickVertexFinished (gp_Pnt);
    void on_CancelButton_clicked ();

private:
    Ui::VectorInputForm *ui;

    bool pickStartPoint;
    bool pickEndPoint;
    bool hasStartPoint;
    bool hasEndPoint;
    gp_Pnt *transferStartPoint, *transferEndPoint, localStartPoint, localEndPoint;

    CustomOpenGLWidget *drawingWindow;
    Relay *relay;
    double conversionFactor;  // converts from m to some other unit coming in, then back to m going out
};

#endif // VECTORINPUTFORM_H
