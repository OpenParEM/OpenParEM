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
#include <AIS_InteractiveContext.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include "Polywire.h"
#include "Process.h"

class CustomTreeWidgetItem : public QObject, public QTreeWidgetItem {
    Q_OBJECT

public:
    CustomTreeWidgetItem (QTreeWidgetItem *parent = nullptr, int type=Type) : QTreeWidgetItem(parent,type)
    {
        itemType=0;
        set_dimTag(-1,-1);  // for mesh items; invalid initialization
        OPEMobject=nullptr;
        polywire=nullptr;
        process=nullptr;
        p0set=false;
        p1set=false;
        enableMove=false;
        enableStretch=false;
        enableDeletePoint=false;
        enableInsertPoint=false;
    }

    ~CustomTreeWidgetItem ()
    {
        if (polywire) delete polywire;
        if (process) delete process;
    }

    void set_OPEMobject (void *pointer) {OPEMobject=pointer;}
    void* get_OPEMobject () {return OPEMobject;}

    void set_Polywire (Polywire *polywire_) {polywire=polywire_;}
    Polywire* get_Polywire () {return polywire;}

    void set_Process (Process *process_) {process=process_;}
    Process* get_Process () {return process;}

    long unsigned int linkedItems_size () {return linkedItems.size();}
    void push_linkedItem (CustomTreeWidgetItem *linkedItem) {linkedItems.push_back(linkedItem);}
    CustomTreeWidgetItem* get_linkedItem (long unsigned int i) {return linkedItems[i];}

    void set_itemType (int itemType_)
    {
        itemType=itemType_;
        forShowHide=false;
        if (is_drawing()) forShowHide=true;
        if (is_port()) forShowHide=true;
        if (is_boundary()) forShowHide=true;
        if (is_mesh()) forShowHide=true;
        if (is_path()) forShowHide=true;
        if (is_integrationPathSegment()) forShowHide=true;
        if (is_rootDrawing()) forShowHide=true;
        if (is_rootPort()) forShowHide=true;
        if (is_rootBoundary()) forShowHide=true;
        if (is_rootMesh()) forShowHide=true;
    }
    int get_itemType () {return itemType;}

    bool is_drawing () {if (itemType == 0) return true; return false;}
    bool is_port () {if (itemType == 1) return true; return false;}
    bool is_boundary () {if (itemType == 2) return true; return false;}
    bool is_mesh () {if (itemType == 3) return true; return false;}
    bool is_path () {if (itemType == 4) return true; return false;}
    bool is_sport () {if (itemType == 5) return true; return false;}
    bool is_impedanceDefinition () {if (itemType == 6) return true; return false;}
    bool is_impedanceCalculation () {if (itemType == 7) return true; return false;}
    bool is_sportLabel () {if (itemType == 8) return true; return false;}
    bool is_sportNumber () {if (itemType == 9) return true; return false;}
    bool is_voltage () {if (itemType == 10) return true; return false;}
    bool is_current () {if (itemType == 11) return true; return false;}
    bool is_scale () {if (itemType == 12) return true; return false;}
    bool is_scaleValue () {if (itemType == 13) return true; return false;}
    bool is_integrationPathSegment () {if (itemType == 14) return true; return false;}
    bool is_rootDrawing () {if (itemType == 100) return true; return false;}
    bool is_rootPort () {if (itemType == 101) return true; return false;}
    bool is_rootBoundary () {if (itemType == 102) return true; return false;}
    bool is_rootMesh () {if (itemType == 103) return true; return false;}
    bool is_rootPath () {if (itemType == 104) return true; return false;}
    bool is_root ()
    {
        if (is_rootDrawing()) return true;
        if (is_rootPort()) return true;
        if (is_rootBoundary()) return true;
        if (is_rootMesh()) return true;
        if (is_rootPath()) return true;
        return false;
    }

    void set_AIS_Shape (Handle(AIS_Shape) shape_) {shape=shape_;}
    Handle(AIS_Shape) get_AIS_Shape () {return shape;}
    void delete_AIS_Shape (Handle(AIS_InteractiveContext) viewerContext)
    {
        if (shape.IsNull()) return;
        viewerContext->Remove(shape,Standard_True);
        shape.Nullify();
    }

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

    void removeLinkedItem (CustomTreeWidgetItem *item)
    {
        std::cout << "CustomTreeWidgetItem::removeLinkedItem" << std::endl; std::cout.flush();

        long unsigned int i=0;
        while (i < linkedItems.size()) {
            if (linkedItems[i] == item) linkedItems.erase(linkedItems.begin()+i);
            i++;
        }
    }

    bool hasParent ()
    {
        CustomTreeWidgetItem *parentItem=(CustomTreeWidgetItem *)this->QTreeWidgetItem::parent();
        if (!parentItem) return false;
        if (parentItem->is_rootDrawing()) return false;
        return true;
    }

    gp_Trsf getTrsf () {return aTrsf;}
    void setTrsf (gp_Trsf aTrsf_) {aTrsf=aTrsf_;}


    TopoDS_Shape moveShape (gp_Pnt p1, gp_Pnt p2, Handle(AIS_InteractiveContext) viewerContext)
    {
        gp_Trsf step;
        step.SetTranslation(p1,p2);
        aTrsf=step*aTrsf;
        shape->SetLocalTransformation(aTrsf);

        viewerContext->Redisplay(shape,Standard_True);

        BRepBuilderAPI_Transform transformer(shape->Shape(),aTrsf,Standard_True);
        return transformer.Shape();
    }

    void moveAnimateShape (gp_Pnt p1, gp_Pnt p2, Handle(AIS_InteractiveContext) viewerContext)
    {
        if (animateShape.IsNull()) return;

        gp_Trsf step;
        step.SetTranslation(p1,p2);
        aTrsf=step*aTrsf;
        animateShape->SetLocalTransformation(aTrsf);

        viewerContext->Redisplay(animateShape,Standard_True);
    }


    TopoDS_Shape rotateShape (double &angleDegrees, gp_Pnt &p1, gp_Pnt &p2, Handle(AIS_InteractiveContext) viewerContext)
    {
        gp_Dir dir(gp_Vec(p1,p2));
        gp_Ax1 axis(p1,dir);
        double angleRadians=angleDegrees*M_PI/180;

        gp_Trsf rotate;
        rotate.SetRotation(axis,angleRadians);

        BRepBuilderAPI_Transform transformer(shape->Shape(),rotate);
        return transformer.Shape();
    }

    void reset_transformation ()
    {
        aTrsf=gp_Trsf();
    }

    void setAnimate (Handle(AIS_InteractiveContext) viewerContext)
    {
        if (shape.IsNull()) return;

        if (!animateShape.IsNull()) {
            viewerContext->Remove(animateShape,Standard_True);
            animateShape.Nullify();
        }

        animateShape=new AIS_Shape(shape->Shape());
        viewerContext->Display(animateShape,AIS_WireFrame,-1,Standard_True);  // non-selectable
    }

    void unsetAnimate (Handle(AIS_InteractiveContext) viewerContext)
    {
        //std::cout << "unsetAnimate" << std::endl; std::cout.flush();
        if (animateShape.IsNull()) return;
        viewerContext->Remove(animateShape,Standard_True);
        animateShape.Nullify();
    }

    void setP0 (gp_Pnt p0_) {p0=p0_; p0set=true;}
    void setP1 (gp_Pnt p1_) {p1=p1_; p1set=true;}
    bool hasP0 () {return p0set;}
    bool hasP1 () {return p1set;}
    gp_Pnt getP0 () {return p0;}
    gp_Pnt getP1 () {return p1;}
    void resetP0P1 () {p0set=false; p1set=false;}

    void setEnableMove (bool enableMove_) {enableMove=enableMove_;}
    bool getEnableMove () {return enableMove;}

    void setEnableStretch (bool enableStretch_) {enableStretch=enableStretch_;}
    bool getEnableStretch () {return enableStretch;}

    void setEnableDeletePoint (bool enableDeletePoint_) {enableDeletePoint=enableDeletePoint_;}
    bool getEnableDeletePoint () {return enableDeletePoint;}

    void setEnableInsertPoint (bool enableInsertPoint_) {enableInsertPoint=enableInsertPoint_;}
    bool getEnableInsertPoint () {return enableInsertPoint;}

    CustomTreeWidgetItem* copyCreate ()
    {
        std::cout << "CustomTreeWidgetItem::copyCreate()" << std::endl; std::cout.flush();
        CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem();
        if (!shape.IsNull()) {newItem->shape=new AIS_Shape(shape->Shape());}
        newItem->setText(0,this->text(0));
        long unsigned int i=0;
        while (i < arrowHeads.size()) {
            if (!arrowHeads[i].IsNull()) {newItem->arrowHeads.push_back(new AIS_Shape(arrowHeads[i]->Shape()));}
            i++;
        }
        newItem->dimTag=dimTag;  // ToDo: rescan and set to unique value for meshing
        newItem->forShowHide=forShowHide;
        newItem->itemType=itemType;
        newItem->OPEMobject=OPEMobject;
        if (polywire) {newItem->polywire=polywire->copyCreate();}
        if (process) {newItem->process=process->copyCreate();}
        i=0;
        while (i < linkedItems.size()) {
            newItem->linkedItems.push_back(linkedItems[i]);
            i++;
        }
        std::cout << "exit CustomTreeWidgetItem::copyCreate()" << std::endl; std::cout.flush();
        return newItem;
    }

    void print_itemType ()
    {
        if (is_rootDrawing()) std::cout << "root drawing" << std::endl;
        if (is_drawing()) std::cout << "drawing" << std::endl;
        if (is_rootBoundary()) std::cout << "root boundary" << std::endl;
        if (is_boundary()) std::cout << "boundary" << std::endl;
        if (is_rootPort()) std::cout << "root port" << std::endl;
        if (is_port()) std::cout << "port" << std::endl;
        if (is_rootMesh()) std::cout << "root mesh" << std::endl;
        if (is_mesh()) std::cout << "mesh" << std::endl;
        if (is_rootPath()) std::cout << "root path" << std::endl;
        if (is_path()) std::cout << "path" << std::endl;
        if (is_sport()) std::cout << "Sport" << std::endl;
        if (is_impedanceDefinition()) std::cout << "impedance definition" << std::endl;
        if (is_impedanceCalculation()) std::cout << "impedance calculation" << std::endl;
        if (is_sportLabel()) std::cout << "sport label" << std::endl;
        if (is_sportNumber()) std::cout << "sport number" << std::endl;
        if (is_voltage()) std::cout << "voltage" << std::endl;
        if (is_current()) std::cout << "current" << std::endl;
        if (is_scale()) std::cout << "scale" << std::endl;
        if (is_scaleValue()) std::cout << "scaleValue" << std::endl;
        if (is_integrationPathSegment()) std::cout << "integration path segment" << std::endl;
    }

    void print ()
    {
        std::cout << "CustomTreeWidgetItem:" << std::endl;
        if (shape.IsNull()) std::cout << "   shape=null" << std::endl;
        else std::cout << "   shape type=" << TopAbs::ShapeTypeToString(shape->Shape().ShapeType()) << std::endl;
        //else std::cout << "   shape type=" << shape->Type() << std::endl;
        std::cout << "   forShowHide=" << forShowHide << std::endl;
        std::cout << "   OPEMobject=" << OPEMobject << std::endl;
        std::cout << "   polywire=" << polywire << std::endl;
        std::cout << "   itemType=" << itemType << std::endl;
        if (is_rootDrawing()) std::cout << "   itemType=rootDrawing" << std::endl;
        if (is_rootPort()) std::cout << "   itemType=rootPort" << std::endl;
        if (is_rootBoundary()) std::cout << "   itemType=rootBoundary" << std::endl;
        if (is_rootMesh()) std::cout << "   itemType=rootMesh" << std::endl;
        if (is_rootPath()) std::cout << "   itemType=rootPath" << std::endl;
        if (is_drawing()) std::cout << "   itemType=drawing" << std::endl;
        if (is_port()) std::cout << "   itemType=port" << std::endl;
        if (is_boundary()) std::cout << "   itemType=boundary" << std::endl;
        if (is_mesh()) std::cout << "   itemType=mesh" << std::endl;
        if (is_path()) std::cout << "   itemType=path" << std::endl;
        if (is_sportLabel()) std::cout << "   itemType=sport" << std::endl;
        if (is_impedanceDefinition()) std::cout << "   itemType=impedanceDefinition" << std::endl;
        if (is_impedanceCalculation()) std::cout << "   itemType=impedanceCalculation" << std::endl;
        if (is_sportNumber()) std::cout << "   itemType=sportNumber" << std::endl;
        if (is_sport()) std::cout << "   itemType=sportNet" << std::endl;
        if (is_voltage()) std::cout << "   itemType=voltage" << std::endl;
        if (is_current()) std::cout << "   itemType=current" << std::endl;
        if (is_scale()) std::cout << "   itemType=scale" << std::endl;
        if (is_scaleValue()) std::cout << "   itemType=scaleValue" << std::endl;
        if (is_integrationPathSegment()) std::cout << "   itemType=integrationPathSegment" << std::endl;
        std::cout << "   dimTag.first=" << dimTag.first << std::endl
                  << "   dimTag.second=" << dimTag.second << std::endl;
    }

    void resetOperation ()
    {
        aTrsf=gp_Trsf();
        p0set=false;
        p1set=false;
        enableMove=false;
        enableStretch=false;
        enableDeletePoint=false;
        enableInsertPoint=false;
    }

    void reset ()
    {
        deleteChildren(this);
        set_AIS_Shape(nullptr);
        setForeground(0,Qt::black);
        setExpanded(Standard_False);
        arrowHeads.clear();
        meshEntities.clear();
        OPEMobject=nullptr;
        polywire=nullptr;
        linkedItems.clear();
    }

private slots:

private:
    Handle(AIS_Shape) shape;                           // for drawing
    Handle(AIS_Shape) animateShape;                    // temporary shape for animation during moving
    gp_Trsf aTrsf;
    std::vector<Handle(AIS_Shape)> arrowHeads;         // for integration lines to show direction
    std::vector<Handle(AIS_Shape)> meshEntities;       // for mesh
    std::pair<int,int> dimTag;                         //
    bool forShowHide;                                  // false - does not participate in item tree show/hide operations; true - does participate
    int itemType;                                      // 0 - drawing, 1 - port, 2 - boundary, 3 - mesh, 4 - path
                                                       // 5 - Sport (net), 6 - impedance definition, 7 - impedance calculation
                                                       // 8 - Sport label, 9 - Sport number,
                                                       // 10 - voltage, 11 - current
                                                       // 12 - scale, 13 - scale value
                                                       // 14 - integration path segment
                                                       // 100 - root drawing item
                                                       // 101 - root port item
                                                       // 102 - root boundary item
                                                       // 103 - root mesh item
                                                       // 104 - root path item
    void *OPEMobject;                                  // a pointer to an item in the boundary database
                                                       // *path, *mode, *boundary, etc
                                                       // cast to the correct object type
    Polywire *polywire;                                // Polywire object for this item
    Process *process;                                  // for drawing processing of children
                                                       // cast tp the correct process type
    std::vector<CustomTreeWidgetItem *> linkedItems;   // link to path items, if any

    gp_Pnt p0,p1;                                      // for move operations
    bool p0set,p1set;
    bool enableMove;
    bool enableStretch;
    bool enableDeletePoint;
    bool enableInsertPoint;
};

#endif // CUSTOMTREEWIDGETITEM_H
