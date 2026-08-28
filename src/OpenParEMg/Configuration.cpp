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

#include "Configuration.h"
#include <iostream>
#include <qcontainerfwd.h>
#include <QDir>

Configuration::Configuration()
{
    // configuration file
    QString homePath=QDir::homePath();
    filePath=homePath+"/.OpenParEMg";

    // set defaults
    defaultFontSize=10.5;
    logFontSize=9.5;
    mainWindowWidth=0;    // application chooses
    mainWindowHeight=0;   // application chooses
    mainWindowOriginX=0;  // application chooses
    mainWindowOriginY=0;  // application chooses
    defaultCoreCount=5;

}

void Configuration::setDefaultFontSize (double defaultFontSize_) {defaultFontSize=defaultFontSize_;}
double Configuration::getDefaultFontSize () {return defaultFontSize;}

void Configuration::setLogFontSize (double logFontSize_) {logFontSize=logFontSize_;}
double Configuration::getLogFontSize () {return logFontSize;}

void Configuration::setMainWindowWidth (int mainWindowWidth_) {mainWindowWidth=mainWindowWidth_;}
int Configuration::getMainWindowWidth () {return mainWindowWidth;}

void Configuration::setMainWindowHeight (int mainWindowHeight_) {mainWindowHeight=mainWindowHeight_;}
int Configuration::getMainWindowHeight () {return mainWindowHeight;}

void Configuration::setMainWindowOriginX (int mainWindowOriginX_) {mainWindowOriginX=mainWindowOriginX_;}
int Configuration::getMainWindowOriginX () {return mainWindowOriginX;}

void Configuration::setMainWindowOriginY (int mainWindowOriginY_) {mainWindowOriginY=mainWindowOriginY_;}
int Configuration::getMainWindowOriginY () {return mainWindowOriginY;}

void Configuration::setDefaultCoreCount (int defaultCoreCount_) {defaultCoreCount=defaultCoreCount_;}
int Configuration::getDefaultCoreCount () {return defaultCoreCount;}

bool Configuration::exists ()
{
    if (QFile::exists(filePath)) return true;
    return false;
}

bool Configuration::create ()
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return true;

    QTextStream out(&file);
    out << "defaultFontSize=" << QString::number(defaultFontSize) << "\n";
    out << "logFontSize=" << QString::number(logFontSize) << "\n";
    out << "mainWindowWidth=" << QString::number(mainWindowWidth) << "\n";
    out << "mainWindowHeight=" << QString::number(mainWindowHeight) << "\n";
    out << "mainWindowOriginX=" << QString::number(mainWindowOriginX) << "\n";
    out << "mainWindowOriginY=" << QString::number(mainWindowOriginY) << "\n";
    out << "defaultCoreCount=" << QString::number(defaultCoreCount) << "\n";

    file.close();

    return false;
}

bool Configuration::load ()
{
    // return if there is not a configuration file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return true;

    // load the variables

    QTextStream in(&file);

    while (!in.atEnd()) {

        QString line=in.readLine().trimmed();

        // skip blank lines
        if (line.isEmpty()) continue;

        // skip commented lines
        if (line.startsWith('#')) continue;

        // split into tokens
        QStringList tokens=line.split('=',Qt::SkipEmptyParts);
        if (tokens.size() >= 2) {
            QString keyword=tokens.at(0).trimmed();
            QString value=tokens.at(1).trimmed();

            if (keyword.compare("defaultFontSize") == 0) {defaultFontSize=value.toDouble();}
            if (keyword.compare("logFontSize") == 0) {logFontSize=value.toDouble();}
            if (keyword.compare("mainWindowWidth") == 0) {mainWindowWidth=value.toInt();}
            if (keyword.compare("mainWindowHeight") == 0) {mainWindowHeight=value.toInt();}
            if (keyword.compare("mainWindowOriginX") == 0) {mainWindowOriginX=value.toInt();}
            if (keyword.compare("mainWindowOriginY") == 0) {mainWindowOriginY=value.toInt();}
            if (keyword.compare("defaultCoreCount") == 0) {defaultCoreCount=value.toInt();}
        } else {
            std::cout << "Unrecognized line in .OpenParEMg:" << line.toStdString() << std::endl;
        }
    }

    file.close();
    return false;
}

void Configuration::print ()
{
    std::cout << "Configuration:" << std::endl;
    std::cout << "   defaultFontSize=" << defaultFontSize << std::endl;
    std::cout << "   logFontSize=" << logFontSize << std::endl;
    std::cout << "   mainWindowWidth=" << mainWindowWidth << std::endl;
    std::cout << "   mainWindowHeight=" << mainWindowHeight << std::endl;
    std::cout << "   mainWindowOriginX=" << mainWindowOriginX << std::endl;
    std::cout << "   mainWindowOriginY=" << mainWindowOriginY << std::endl;
    std::cout << "   defaultCoreCount=" << defaultCoreCount << std::endl;

}
