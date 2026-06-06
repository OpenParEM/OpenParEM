#ifndef CUSTOMSPINBOX_H
#define CUSTOMSPINBOX_H

#include "ItemTracking.h"
#include <QSpinBox>
#include <QWheelEvent>

void spinValueChanged (int value, SportItem *);

class CustomSpinBox : public QSpinBox
{
    Q_OBJECT

public:

    CustomSpinBox(QWidget *parent = nullptr) : QSpinBox(parent)
    {
        connect(this,&QSpinBox::valueChanged,this,&CustomSpinBox::handleCustomValueChanged);
        setFocusPolicy(Qt::ClickFocus);
        drawingTracker=nullptr;
        sportItem=nullptr;
    }

    void set_itemTracker (ItemTracker *drawingTracker_) {drawingTracker=drawingTracker_;}
    void set_sportItem (SportItem *sportItem_) {sportItem=sportItem_;}

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
    void CustomValueChanged (int, SportItem *);

private slots:
    void handleCustomValueChanged (int index) {
        emit CustomValueChanged(index,sportItem);
    }

private:
    ItemTracker *drawingTracker;
    SportItem *sportItem;
};

#endif // CUSTOMSPINBOX_H
