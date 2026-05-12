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
    void clearTreeSelection ();
    void finishOperation (bool, int);
    void getCurrentMousePosition (gp_Pnt);
    void getPickedVertex (gp_Pnt, bool);
    void startPlaneSetToFace ();
    void updateViewer ();
};

#endif // RELAY_H
