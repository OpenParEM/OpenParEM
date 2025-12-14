////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM3g - A GUI for OpenParEM3D                                     //
//    Copyright (C) 2025 Brian Young                                          //
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

#ifndef CUSTOMTREEWIDGETITEM_H
#define CUSTOMTREEWIDGETITEM_H

#include <QTreeWidgetItem>
#include <QMenu>
#include "AIS_Shape.hxx"

class CustomTreeWidgetItem : public QObject, public QTreeWidgetItem {
    Q_OBJECT

public:
    CustomTreeWidgetItem (QTreeWidgetItem *parent = nullptr, int type=Type) : QTreeWidgetItem(parent,type) {
        root=false;         // default to non-root item
        type=0;
        set_dimTag(-1,-1);  // invalid initialization
        displayMode=0;      // 0 - wireframe, 1 - shaded
        selectionMode=0;
    }

    void set_root (bool root_) {root=root_;}
    bool is_root () {return root;}

    void set_type (int type_) {
        type=type_;
        forShowHide=false;
        if (is_drawing()) forShowHide=true;
        if (is_port()) forShowHide=true;
        if (is_boundary()) forShowHide=true;
        if (is_mesh()) forShowHide=true;
        if (is_sport()) forShowHide=true;
        if (is_voltage()) forShowHide=true;
        if (is_current()) forShowHide=true;
        if (is_integrationPathSegment()) forShowHide=true;
    }
    int get_type () {return type;}

    bool is_drawing () {if (type == 0) return true; return false;}
    bool is_port () {if (type == 1) return true; return false;}
    bool is_boundary () {if (type == 2) return true; return false;}
    bool is_mesh () {if (type == 3) return true; return false;}
    bool is_sport () {if (type == 4) return true; return false;}
    bool is_impedanceDefinition () {if (type == 5) return true; return false;}
    bool is_impedanceCalculation () {if (type == 6) return true; return false;}
    bool is_sportNumber () {if (type == 7) return true; return false;}
    bool is_sportNet () {if (type == 8) return true; return false;}
    bool is_voltage () {if (type == 9) return true; return false;}
    bool is_current () {if (type == 10) return true; return false;}
    bool is_scale () {if (type == 11) return true; return false;}
    bool is_scaleValue () {if (type == 12) return true; return false;}
    bool is_integrationPathSegment () {if (type == 13) return true; return false;}

    void set_AIS_Shape (Handle(AIS_Shape) shape_) {shape=shape_;}
    Handle(AIS_Shape) get_AIS_Shape () {return shape;}

    void push_arrowHead (Handle(AIS_Shape) arrowHead) {arrowHeads.push_back(arrowHead);}
    long unsigned int get_arrowHeads_size () {return arrowHeads.size();}
    Handle(AIS_Shape) get_arrowHead (long unsigned int i) {return arrowHeads[i];}

    void set_dimTag (int dim, int tag) {dimTag.first=dim; dimTag.second=tag;}
    void set_dimTag (std::pair<int,int> dimTag_) {dimTag=dimTag_;}
    std::pair<int,int> get_dimTag () {return dimTag;}
    bool is_solid () {if (dimTag.first == 3) return true; return false;}

    std::vector<Handle(AIS_Shape)>* get_meshEntities () {return &meshEntities;}
    long unsigned int get_meshEntitiesSize () {return meshEntities.size();}
    Handle(AIS_Shape) get_meshEntity (long unsigned int i) {return meshEntities[i];}

    int get_displayMode () {return displayMode;}
    void set_displayMode (int displayMode_) {displayMode=displayMode_;}
    int get_selectionMode () {return selectionMode;}
    void set_selectionMode (int selectionMode_) {selectionMode=selectionMode_;}

    void deleteChildren (QTreeWidgetItem *item)
    {
        QList<QTreeWidgetItem*> children=item->takeChildren();
        for (QTreeWidgetItem* child : children) {
            deleteChildren(child);
            delete child;
        }
    }

    bool isValidShow ()
    {
        if (!forShowHide) return false;
        if (foreground(0) == Qt::gray) return true;
        return false;
    }

    bool isValidHide ()
    {
        if (!forShowHide) return false;
        if (foreground(0) == Qt::black) return true;
        return false;
    }

    void print () {
        std::cout << "CustomTreeWidgetItem:" << std::endl;
        if (shape.IsNull()) std::cout << "   shape=null" << std::endl;
        else std::cout << "   shape type=" << shape->Type() << std::endl;
        std::cout << "   root=" << root << std::endl;
        std::cout << "   forShowHide=" << forShowHide << std::endl;
        if (is_drawing()) std::cout << "   type=drawing" << std::endl;
        if (is_port()) std::cout << "   type=port" << std::endl;
        if (is_boundary()) std::cout << "   type=boundary" << std::endl;
        if (is_mesh()) std::cout << "   type=mesh" << std::endl;
        if (is_sport()) std::cout << "   type=sport" << std::endl;
        if (is_impedanceDefinition()) std::cout << "   type=impedanceDefinition" << std::endl;
        if (is_impedanceCalculation()) std::cout << "   type=impedanceCalculation" << std::endl;
        if (is_sportNumber()) std::cout << "   type=sportNumber" << std::endl;
        if (is_sportNet()) std::cout << "   type=sportNet" << std::endl;
        if (is_voltage()) std::cout << "   type=voltage" << std::endl;
        if (is_current()) std::cout << "   type=current" << std::endl;
        if (is_scale()) std::cout << "   type=scale" << std::endl;
        if (is_integrationPathSegment()) std::cout << "   type=integrationPathSegment" << std::endl;
        std::cout << "   dimTag.first=" << dimTag.first << std::endl
                  << "   dimTag.second=" << dimTag.second << std::endl
                  << "   displayMode=" << displayMode << std::endl
                  << "   selectionMode=" << selectionMode << std::endl;
    }

    void reset () {
        deleteChildren(this);
        set_AIS_Shape(nullptr);
        setForeground(0,Qt::black);
        setExpanded(Standard_False);
        meshEntities.clear();
    }

private slots:

private:
    Handle(AIS_Shape) shape;                       // for drawing
    std::vector<Handle(AIS_Shape)> arrowHeads;     // used integration lines
    int displayMode;                               //    0 - wireframe, 1 - shaded
    int selectionMode;                             //    0 - shape, 1 - vertex, ...
    std::vector<Handle(AIS_Shape)> meshEntities;   // for mesh
    std::pair<int,int> dimTag;                     //
    bool root;                                     // false - not a root item, true - is a root item
    bool forShowHide;                              // false - does not participate in item tree show/hide operations; true - does participate
    int type;                                      // 0 - drawing, 1 - port, 2 - boundary, 3 - mesh,
                                                   // 4 - Sport, 5 - impedance definition, 6 - impedance calculation
                                                   // 7 - Sport number, 8 - Sport net,
                                                   // 9 - voltage, 10 - current
                                                   // 11 - scale, 12 - scale value
                                                   // 13 - integration path segment
};

#endif // CUSTOMTREEWIDGETITEM_H
