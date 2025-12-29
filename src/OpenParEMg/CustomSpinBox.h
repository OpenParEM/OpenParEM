#ifndef CUSTOMSPINBOX_H
#define CUSTOMSPINBOX_H

#include "port.hpp"
#include "ItemTracking.h"
#include <QSpinBox>
#include <QWheelEvent>

void spinValueChanged (int value, Mode *, BoundaryDatabase *);

class CustomSpinBox : public QSpinBox
{
    Q_OBJECT

public:

    CustomSpinBox(QWidget *parent = nullptr) : QSpinBox(parent)
    {
        connect(this,&QSpinBox::valueChanged,this,&CustomSpinBox::handleCustomValueChanged);
        setFocusPolicy(Qt::ClickFocus);
        drawingTracker=nullptr;
        mode=nullptr;
        boundaryDatabase=nullptr;
    }

    void set_itemTracker (ItemTracker *drawingTracker_) {drawingTracker=drawingTracker_;}
    void set_mode (Mode *mode_) {mode=mode_;}
    void set_boundaryDatabase (BoundaryDatabase *boundaryDatabase_) {boundaryDatabase=boundaryDatabase_;}

protected:
    void focusInEvent(QFocusEvent *event) override
    {
        if (drawingTracker) drawingTracker->unselectAllItems();
        QSpinBox::focusInEvent(event);
    }

    void wheelEvent(QWheelEvent *event) override
    {
        // ignore wheel events
        event->ignore();
    }

signals:
    void CustomValueChanged (int, Mode *, BoundaryDatabase *);

private slots:
    void handleCustomValueChanged (int index) {
        emit CustomValueChanged(index,mode,boundaryDatabase);
    }

private:
    ItemTracker *drawingTracker;
    Mode *mode;
    BoundaryDatabase *boundaryDatabase;
};

#endif // CUSTOMSPINBOX_H
