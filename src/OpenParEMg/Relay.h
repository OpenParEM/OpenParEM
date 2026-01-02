#ifndef RELAY_H
#define RELAY_H

#pragma once
#include <QObject>
#include <Standard_Handle.hxx>
#include <AIS_Shape.hxx>

class Relay : public QObject
{
    Q_OBJECT
public:
    explicit Relay (QObject *parent = nullptr);

signals:
    void setMenus ();
    void drawLineFinished (Handle(AIS_Shape));
    void cancelDraw ();
};

#endif // RELAY_H
