#ifndef RELAY_H
#define RELAY_H

#pragma once
#include <QObject>
#include <Standard_Handle.hxx>
#include <TopoDS.hxx>

class Relay : public QObject
{
    Q_OBJECT
public:
    explicit Relay (QObject *parent = nullptr);

signals:
    void setMenus ();
    void pickVertexFinished (gp_Pnt);
    void finishExtrudeFace (double, bool);
    void drawLineFinished (TopoDS_Wire);
    void cancelDraw ();
};

#endif // RELAY_H
