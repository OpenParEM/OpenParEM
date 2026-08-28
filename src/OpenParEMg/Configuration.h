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


#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <qcontainerfwd.h>
#include <qdir.h>

class Configuration
{
public:
    Configuration();

    void setDefaultFontSize (double);
    double getDefaultFontSize ();

    void setLogFontSize (double);
    double getLogFontSize ();

    void setMainWindowWidth (int);
    int getMainWindowWidth ();

    void setMainWindowHeight (int);
    int getMainWindowHeight ();

    void setMainWindowOriginX (int);
    int getMainWindowOriginX ();

    void setMainWindowOriginY (int);
    int getMainWindowOriginY ();

    void setDefaultCoreCount (int);
    int getDefaultCoreCount ();

    bool exists ();
    bool create ();
    bool load ();
    void print ();

private:
    double defaultFontSize;  // font size for everything except for other called-out font sizes
    double logFontSize;      // font size for the log windows
    int mainWindowWidth;
    int mainWindowHeight;
    int mainWindowOriginX;
    int mainWindowOriginY;
    int defaultCoreCount;

    QString filePath;
};

#endif // CONFIGURATION_H
