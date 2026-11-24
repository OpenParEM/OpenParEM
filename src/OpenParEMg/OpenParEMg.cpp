////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEMg - A GUI for OpenParEM3D                                      //
//    Copyright (C) 2026 Brian Young                                          //
//                                                                            //
//    This program is free software: you can redistribute it and/or modify    //
//    it under the terms of the GNU General Public License as published by    //
//    the Free Software Foundation, either version 3 of the License, or       //
//    (at your option) any later version.                                     //
//                                                                            //
//    This program is distributed in the hope that it will be useful,         //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of          //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           //
//    GNU General Public License for more details.                            //
//                                                                            //
//    You should have received a copy of the GNU General Public License       //
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.   //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include "OPEMg.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QtGui/qguiapplication_platform.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM","xcb");
    qputenv("QT_QPA_PLATFORMTHEME","generic");

    QApplication a(argc, argv);

    OpenParEMg w;
    w.setWindowIcon(QIcon(":/images/images/logo2.svg"));
    a.setDesktopFileName("OpenParEMg");

    // use X11 functions to change the color from black to white
    WId id=w.winId();
    Display *display=qApp->nativeInterface<QNativeInterface::QX11Application>()->display();
    XSetWindowBackground(display,id,WhitePixel(display,DefaultScreen(display)));
    XFlush(display);

    w.show();

    // finish initialization
    QTimer::singleShot(0, &w, &OpenParEMg::initialize);
    return a.exec();
}

