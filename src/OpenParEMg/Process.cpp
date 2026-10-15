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

#include "Process.h"
#include <AIS_InteractiveContext.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>

Process::Process (QWidget *parent)
    : QWidget{parent}
{}

QString Extrude::getTip (double conversionFactor)
{
    QString tip="Extrude";

    tip.append("\nlength=");
    tip.append(QString::number(length*conversionFactor,'g',15));

    return tip;
}

Extrude* Extrude::copyCreate ()
{
    Extrude* newExtrude=new Extrude();
    newExtrude->modified=modified;
    newExtrude->length=length;
    return newExtrude;
}

QString Extrude::getName (ObjectCounts *objectCounts) {
    objectCounts->extrude++;
    QString name="Extrude";
    name.append(QString::number(objectCounts->extrude));
    return name;
}

void Extrude::startSave (std::ofstream *out, QString name, QString material, int level)
{
    std::string space;
    int i=0;
    while (i < level) {
        space.append("   ");
        i++;
    }

    *out << space << "Extrude" << std::endl;
    if (!name.isEmpty()) {
        *out << space << "   name=" << name.toStdString() << std::endl;
    }
    if (!material.isEmpty()) {
        *out << space << "   material=" << material.toStdString() << std::endl;
    }
    *out << space << "   length=" << length << std::endl;
}

void Extrude::endSave (std::ofstream *out, int level)
{
    std::string space;
    int i=0;
    while (i < level) {
        space.append("   ");
        i++;
    }

    *out << space << "EndExtrude" << std::endl;
}

QString Merge::getTip (double conversionFactor)
{
    QString tip="Merge";

    return tip;
}

Merge* Merge::copyCreate ()
{
    Merge* newMerge=new Merge();
    newMerge->modified=modified;

    return newMerge;
}

QString Merge::getName (ObjectCounts *objectCounts) {
    objectCounts->merge++;
    QString name="Merge";
    name.append(QString::number(objectCounts->merge));
    return name;
}

void Merge::startSave (std::ofstream *out, QString name, QString material, int level)
{
    std::string space;
    int i=0;
    while (i < level) {
        space.append("   ");
        i++;
    }

    *out << space << "Merge" << std::endl;
    if (!name.isEmpty()) {
        *out << space << "   name=" << name.toStdString() << std::endl;
    }
    if (!material.isEmpty()) {
        *out << space << "   material=" << material.toStdString() << std::endl;
    }
}

void Merge::endSave (std::ofstream *out, int level)
{
    std::string space;
    int i=0;
    while (i < level) {
        space.append("   ");
        i++;
    }

    *out << space << "EndMerge" << std::endl;
}

QString Subtract::getTip (double conversionFactor)
{
    QString tip="Subtract";

    return tip;
}

Subtract* Subtract::copyCreate ()
{
    Subtract* newSubtract=new Subtract();
    newSubtract->modified=modified;
    return newSubtract;
}

QString Subtract::getName (ObjectCounts *objectCounts) {
    objectCounts->subtract++;
    QString name="Subtract";
    name.append(QString::number(objectCounts->subtract));
    return name;
}

void Subtract::startSave (std::ofstream *out, QString name, QString material, int level)
{
    std::string space;
    int i=0;
    while (i < level) {
        space.append("   ");
        i++;
    }

    *out << space << "Subtract" << std::endl;
    if (!name.isEmpty()) {
        *out << space << "   name=" << name.toStdString() << std::endl;
    }
    if (!material.isEmpty()) {
        *out << space << "   material=" << material.toStdString() << std::endl;
    }
}

void Subtract::endSave (std::ofstream *out, int level)
{
    std::string space;
    int i=0;
    while (i < level) {
        space.append("   ");
        i++;
    }

    *out << space << "EndSubtract" << std::endl;
}
