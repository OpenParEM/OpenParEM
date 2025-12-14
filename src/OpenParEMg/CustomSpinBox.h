#ifndef CUSTOMSPINBOX_H
#define CUSTOMSPINBOX_H

#include "ItemTracking.h"
#include <QSpinBox>
#include <QWheelEvent>

class CustomSpinBox : public QSpinBox
{
public:

    CustomSpinBox(QWidget *parent = nullptr) : QSpinBox(parent)
    {
        setFocusPolicy(Qt::ClickFocus);
        drawingTracker=nullptr;
    }

    void set_itemTracker (ItemTracker *drawingTracker_) {drawingTracker=drawingTracker_;}

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

private:
    ItemTracker *drawingTracker;
};

#endif // CUSTOMSPINBOX_H
