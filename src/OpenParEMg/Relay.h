#ifndef RELAY_H
#define RELAY_H

#pragma once
#include <QObject>

class Relay : public QObject
{
    Q_OBJECT
public:
    explicit Relay (QObject *parent = nullptr);

signals:
    void setMenus ();
    void drawLineFinished ();
};

#endif // RELAY_H
