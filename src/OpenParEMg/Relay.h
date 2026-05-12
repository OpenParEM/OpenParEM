#ifndef RELAY_H
#define RELAY_H

#include "CustomTreeWidgetItem.h"
#include <AIS_Shape.hxx>
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
    void convertPathToFace (CustomTreeWidgetItem *);
    void setShaded (Handle(AIS_Shape));
};

#endif // RELAY_H
