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
#include <QObject>
#include "AIS_Shape.hxx"
#include <AIS_InteractiveContext.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include "Polywire.h"
#include "Process.h"

// shape data with history for undo/redo

class ShapeData
{
public:

    ShapeData ()
    {
        // default to noop
        type=0;
        polywire=nullptr;
        process=nullptr;
        prior=nullptr;
        next=nullptr;

        // for conversions
        path=nullptr;
        port=nullptr;
        boundary=nullptr;
    }

    ShapeData (int type_,Polywire *polywire_, Process *process_, Handle(AIS_Shape) shape_)
    {
        type=type_;
        polywire=nullptr;
        process=nullptr;
        prior=nullptr;
        next=nullptr;
        setPolywire(polywire_);
        setProcess(process_);
        setShape(shape_);

        path=nullptr;
        port=nullptr;
        boundary=nullptr;
    }

    ShapeData (ShapeData *shapeData)
    {
        type=shapeData->type;
        polywire=nullptr;
        process=nullptr;
        prior=shapeData->prior;
        next=shapeData->next;
        setPolywire(shapeData->getPolywire());
        setProcess(shapeData->getProcess());
        setShape(shapeData->getShape());

        path=nullptr;
        port=nullptr;
        boundary=nullptr;
    }

    ~ShapeData ()
    {
        if (polywire) {delete polywire; polywire=nullptr;}
        if (process) {delete process; process=nullptr;}
        if (!shape.IsNull()) {shape.Nullify();}

        long unsigned int i=0;
        while (i < arrowHeads.size()) {
            if (!arrowHeads[i].IsNull()) arrowHeads[i].Nullify();
            i++;
        }
        arrowHeads.clear();
    }

    ShapeData* copyCreate ()
    {
        ShapeData *newShapeData=new ShapeData();
        if (newShapeData) {
            newShapeData->type=type;
            newShapeData->polywire=nullptr;
            newShapeData->process=nullptr;
            newShapeData->prior=prior;
            newShapeData->next=next;
            if (polywire) newShapeData->polywire=polywire->copyCreate();
            if (process) newShapeData->process=process->copyCreate();
            if (!shape.IsNull()) newShapeData->shape=new AIS_Shape(shape->Shape());
            long unsigned int i=0;
            while (i < arrowHeads.size()) {
                if (!arrowHeads[i].IsNull()) {
                    Handle(AIS_Shape) arrowHead=new AIS_Shape(arrowHeads[i]->Shape());
                    newShapeData->arrowHeads.push_back(arrowHead);
                }
                i++;
            }
            newShapeData->path=path;
            newShapeData->port=port;
            newShapeData->boundary=boundary;
        }
        return newShapeData;
    }

    void setType (int type_)
    {
        type=type_;
    }

    int getType () {return type;}

    void setShape (Handle(AIS_Shape) shape_)
    {
        if (shape.IsNull()) {
            shape.Nullify();
        }
        shape=shape_;
    }

    void setPolywire (Polywire *polywire_) {
        if (polywire) delete polywire;
        polywire=polywire_;
    }
    void setProcess (Process *process_) {
        if (process) delete process;
        process=process_;
    }

    void set (int type_, Polywire *polywire_, Process *process_, Handle(AIS_Shape) shape_)
    {
        type=type_;
        setPolywire(polywire_);
        setProcess(process_);
        setShape(shape_);
    }

    void set (CustomTreeWidgetItem *path_, CustomTreeWidgetItem *port_, CustomTreeWidgetItem *boundary_) {
        path=path_;
        port=port_;
        boundary=boundary_;
    }

    void set (ShapeData *shapeData)
    {
        if (!shapeData) return;
        type=shapeData->type;
        prior=shapeData->prior;
        next=shapeData->next;
        setPolywire(shapeData->getPolywire());
        setProcess(shapeData->getProcess());
        setShape(shapeData->getShape());
        path=shapeData->path;
        port=shapeData->port;
        boundary=shapeData->boundary;
    }

    void set (Handle(AIS_Shape) shape_)
    {
        if (shape_.IsNull()) return;
        setShape(shape_);
    }

    void pushArrowHead (Handle(AIS_Shape) arrowHead) {arrowHeads.push_back(arrowHead);}

    Polywire* getPolywire () {return polywire;}
    Process* getProcess () {return process;}
    CustomTreeWidgetItem* getPath () {return path;}
    CustomTreeWidgetItem* getPort () {return port;}
    CustomTreeWidgetItem* getBoundary () {return boundary;}
    Handle(AIS_Shape) getShape () {return shape;}
    long unsigned int getArrowHeadsSize () {return arrowHeads.size();}
    Handle(AIS_Shape) getArrowHead (long unsigned int i) {return arrowHeads[i];}

    bool isNoop () {if (type == 0) return true; return false;}
    bool isCreate () {if (type == 1) return true; return false;}
    bool isEdit () {if (type == 2) return true; return false;}
    bool isDelete () {if (type == 3) return true; return false;}
    bool isConvertToPath () {if (type == 4) return true; return false;}
    bool isConvertToPort () {if (type == 5) return true; return false;}
    bool isConvertToBoundary () {if (type == 6) return true; return false;}
    bool isReversePath () {if (type == 7) return true; return false;}

    void setNoop () {type=0;}
    void setCreate () {type=1;}
    void setEdit () {type=2;}
    void setDelete () {type=3;}
    void setConvertToPath () {type=4;}
    void setConvertToPort () {type=5;}
    void setConvertToBoundary () {type=6;}
    void setReversePath () {type=7;}

    void setPrior (ShapeData *prior_) {prior=prior_;}
    void setNext (ShapeData *next_) {next=next_;}

    ShapeData* getPrior () {return prior;}
    ShapeData* getNext () {return next;}

    void print ()
    {
        std::cout << "               ShapeData: " << this << std::endl;
        if (isNoop()) std:: cout << "                  type=noop" << std::endl;
        if (isCreate()) std:: cout << "                  type=create" << std::endl;
        if (isEdit()) std:: cout << "                  type=edit" << std::endl;
        if (isDelete()) std::cout << "                  type=delete" << std::endl;
        if (isConvertToPath()) std::cout << "                  type=convertToPath" << std::endl;
        if (isConvertToPort()) std::cout << "                  type=convertToPort" << std::endl;
        if (isConvertToBoundary()) std::cout << "                  type=convertToBoundary" << std::endl;
        if (isReversePath()) std::cout << "                  type=reversePath" << std::endl;

        if (shape.IsNull()) std::cout << "                  shape=null" << std::endl;
        else std::cout << "                  shape type=" << TopAbs::ShapeTypeToString(shape->Shape().ShapeType()) << std::endl;
        std::cout << "                  arrowHeadCount=" << arrowHeads.size() << std::endl;
        std::cout << "                  polywire=" << polywire << std::endl;
        std::cout << "                  process=" << process << std::endl;
        std::cout << "                  prior=" << prior << std::endl;
        std::cout << "                  next=" << next << std::endl;
        std::cout << "                  path=" << path << std::endl;
        std::cout << "                  port=" << port << std::endl;
        std::cout << "                  boundary=" << boundary << std::endl;
    }

private:
    int type;                                          // 0 - noop; 1 - create; 2 - edit; 3 - delete;
                                                       // 4 - convert to path; 5 - convert to port; 6 - convert to boundary
                                                       // 7 - reverse path direction
    Handle(AIS_Shape) shape;                           // for drawing
    std::vector<Handle(AIS_Shape)> arrowHeads;         // for integration lines to show direction
    Polywire *polywire;                                // Polywire object for this item
    Process *process;                                  // for drawing processing of children
    ShapeData *prior;                                  // prior ShapeData in ShapeDataStack
    ShapeData *next;                                   // next ShapeData in ShapeDataStack

    CustomTreeWidgetItem *path;                        // for conversions from drawing to path, port, or boundary items
    CustomTreeWidgetItem *port;
    CustomTreeWidgetItem *boundary;
};

class ShapeDataStack
{
public:
    ShapeDataStack ()
    {
        // always put a noop at the bottom of the stack
        ShapeData *noop=new ShapeData();
        current=noop;
        shapeDataList.push_back(noop);
    }

    ~ShapeDataStack ()
    {
        long unsigned int i=0;
        while (i < shapeDataList.size()) {
            if (shapeDataList[i]) {delete shapeDataList[i]; shapeDataList[i]=nullptr;}
            i++;
        }
        shapeDataList.clear();
    }

    long unsigned int getSize () {return shapeDataList.size();}

    void add (int action_, Polywire *polywire_, Process *process_, Handle(AIS_Shape) shape_)
    {
        ShapeData *shapeData=new ShapeData(action_,polywire_,process_,shape_);
        add(shapeData);
    }

    void add (ShapeData *shapeData)
    {
        //std::cout << "CustomTreeWidgetItem::add  shapeData=" << shapeData << std::endl; std::cout.flush();
        if (!shapeData) return;

        shapeDataList.push_back(shapeData);
        if (current) {
            shapeData->setPrior(current);
            current->setNext(shapeData);
        }
        current=shapeData;
    }

    // at current location

    void set (int action_, Polywire *polywire_, Process *process_, Handle(AIS_Shape) shape_)
    {
        current->set(action_,polywire_,process_,shape_);
    }

    void set (ShapeData *shapeData_)
    {
        if (!shapeData_) return;
        current->set(shapeData_);
    }

    void set (CustomTreeWidgetItem *path, CustomTreeWidgetItem *port, CustomTreeWidgetItem *boundary)
    {
        current->set(path,port,boundary);
    }

    void setType (int type_)
    {
        current->setType(type_);
    }

    void setShape (Handle(AIS_Shape) shape_)
    {
        if (shape_.IsNull()) return;
        current->set(shape_);
    }

    void setPolywire (Polywire *polywire_)
    {
        if (!polywire_) return;
        current->setPolywire(polywire_);
    }

    void setProcess (Process *process_)
    {
        if (!process_) return;
        current->setProcess(process_);
    }

    void pushArrowHead (Handle(AIS_Shape) arrowHead)
    {
        if (arrowHead.IsNull()) return;
        current->pushArrowHead(arrowHead);
    }

    // from current location


    ShapeData* getShapeData ()
    {
        return current;
    }

    Polywire* getPolywire ()
    {
        if (current) return current->getPolywire();
        return nullptr;
    }

    Process* getProcess ()
    {
        if (current) return current->getProcess();
        return nullptr;
    }

    CustomTreeWidgetItem* getPath ()
    {
        if (current) return current->getPath();
        return nullptr;
    }

    CustomTreeWidgetItem* getPort ()
    {
        if (current) return current->getPort();
        return nullptr;
    }

    CustomTreeWidgetItem* getBoundary ()
    {
        if (current) return current->getBoundary();
        return nullptr;
    }

    Handle(AIS_Shape) getShape ()
    {
        Handle(AIS_Shape) shape;
        if (current) shape=current->getShape();
        return shape;
    }

    long unsigned int getArrowHeadsSize ()
    {
        if (current) return current->getArrowHeadsSize();
        return 0;
    }

    Handle(AIS_Shape) getArrowHead (long unsigned int i)
    {
        Handle(AIS_Shape) arrowHead;
        if (current) arrowHead=current->getArrowHead(i);
        return arrowHead;
    }

    bool hasUndo ()
    {
        bool retval=false;
        if (current && !current->isNoop()) retval=true;
        return retval;
    }

    bool hasRedo ()
    {
        bool retval=false;
        if (current) {
            if (current->getNext() && !current->getNext()->isNoop()) retval=true;
        } else {
            if (shapeDataList.size() > 0) retval=true;
        }
        return retval;
    }

    void undo ()
    {
        if (current) current=current->getPrior();
    }

    void redo ()
    {
        if (current) current=current->getNext();
    }

    void reset ()
    {
        long unsigned int i=0;
        while (i < shapeDataList.size()) {
            if (shapeDataList[i]) {delete shapeDataList[i]; shapeDataList[i]=nullptr;}
            i++;
        }
        shapeDataList.clear();
        current=nullptr;
    }

    void pop ()
    {
        if (current == shapeDataList[shapeDataList.size()-1]) {
            long unsigned int i=0;
            while (i < shapeDataList.size()) {
                if (shapeDataList[i]->getNext() == current) {
                    shapeDataList[i]->setNext(nullptr);
                }
                if (shapeDataList[i]->getPrior() == current) {
                    shapeDataList[i]->setPrior(nullptr);
                }
                i++;
            }

            if (shapeDataList[shapeDataList.size()-1]) {
                delete shapeDataList[shapeDataList.size()-1];
                shapeDataList[shapeDataList.size()-1]=nullptr;
            }
            shapeDataList.pop_back();
            current=shapeDataList[shapeDataList.size()-1];
        }
    }

    void print ()
    {
        std::cout << "            ShapeDataStack: " << this << std::endl;
        long unsigned int i=0;
        while (i < shapeDataList.size()) {
            shapeDataList[i]->print();
            i++;
        }
        std::cout << "            current=" << current << std::endl;
    }

private:
    std::vector<ShapeData *> shapeDataList;
    ShapeData *current;
};

class CustomTreeWidgetItem : public QObject, public QTreeWidgetItem {
    Q_OBJECT

public:
    CustomTreeWidgetItem (QTreeWidgetItem *parent = nullptr, int type=Type) : QTreeWidgetItem(parent,type)
    {
        itemType=0;                 // default to drawing
        forShowHide=true;
        set_dimTag(-1,-1);          // for mesh items; invalid initialization
        OPEMobject=nullptr;
        depth=0;
        parent=nullptr;
    }

    QString get_name () {return text(0);}
    QString get_material () {return text(1);}
    void copy_depth (CustomTreeWidgetItem *item) {depth=item->depth;}
    int get_depth () {return depth;}
    void increase_depth () {depth++;}
    void decrease_depth () {depth--;}

    void set_OPEMobject (void *pointer) {OPEMobject=pointer;}
    void* get_OPEMobject () {return OPEMobject;}

    void addShapeData (ShapeData *shapeData_) {dataStack.add(shapeData_);}
    void setShape (Handle(AIS_Shape) shape_) {dataStack.setShape(shape_);}
    void setPolywire (Polywire *polywire_) {dataStack.setPolywire(polywire_);}
    void setProcess (Process *process_) {dataStack.setProcess(process_);}

    ShapeData* getShapeData () {return dataStack.getShapeData();}
    Polywire* getPolywire () {return dataStack.getPolywire();}
    Process* getProcess () {return dataStack.getProcess();}
    Handle(AIS_Shape) getShape () {return dataStack.getShape();}

    void undo () {dataStack.undo();}
    void redo () {dataStack.redo();}
    void pop () {dataStack.pop();}

    void set_Material (QString material_) {material=material_;}
    QString get_Material () {return material;}

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


    void set_dimTag (int dim, int tag) {dimTag.first=dim; dimTag.second=tag;}
    void set_dimTag (std::pair<int,int> dimTag_) {dimTag=dimTag_;}
    std::pair<int,int> get_dimTag () {return dimTag;}
    bool is_solid () {if (dimTag.first == 3) return true; return false;}

    std::vector<Handle(AIS_Shape)>* get_meshEntities () {return &meshEntities;}
    long unsigned int get_meshEntitiesSize () {return meshEntities.size();}
    Handle(AIS_Shape) get_meshEntity (long unsigned int i) {return meshEntities[i];}

    void deleteChildren (QTreeWidgetItem *item)
    {
        if (!item) return;
        QList<QTreeWidgetItem*> children=item->takeChildren();
        for (QTreeWidgetItem* child : children) {
            if (child) {
                deleteChildren(child);
                delete child;
            }
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
        //std::cout << "CustomTreeWidgetItem::removeLinkedItem" << std::endl; std::cout.flush();

        long unsigned int i=0;
        while (i < linkedItems.size()) {
            if (linkedItems[i] == item) linkedItems.erase(linkedItems.begin()+i);
            i++;
        }
    }


    TopoDS_Shape rotateShape (double &angleDegrees, gp_Pnt &p1, gp_Pnt &p2, Handle(AIS_InteractiveContext) viewerContext)
    {
        gp_Dir dir(gp_Vec(p1,p2));
        gp_Ax1 axis(p1,dir);
        double angleRadians=angleDegrees*M_PI/180;

        gp_Trsf rotate;
        rotate.SetRotation(axis,angleRadians);

        BRepBuilderAPI_Transform transformer(dataStack.getShapeData()->getShape()->Shape(),rotate);
        return transformer.Shape();
    }









    CustomTreeWidgetItem* copyCreate ()
    {
        std::cout << "CustomTreeWidgetItem::copyCreate()" << std::endl; std::cout.flush();
        CustomTreeWidgetItem *newItem=new CustomTreeWidgetItem();

        // copy just the current data

        ShapeData *copyShapeData=dataStack.getShapeData()->copyCreate();
        newItem->dataStack.add(copyShapeData);
        newItem->setText(0,this->text(0).append("_copy"));
        newItem->dimTag=dimTag;
        newItem->forShowHide=forShowHide;
        newItem->itemType=itemType;
        newItem->OPEMobject=OPEMobject;
        newItem->depth=depth;
        long unsigned int i=0;
        while (i < linkedItems.size()) {
            newItem->linkedItems.push_back(linkedItems[i]);
            i++;
        }
        return newItem;
    }

    long unsigned int getArrowHeadsSize () {return dataStack.getArrowHeadsSize();}
    Handle(AIS_Shape) getArrowHead (long unsigned int i) {return dataStack.getArrowHead(i);}
    void pushArrowHead (Handle(AIS_Shape) arrowHead) {dataStack.pushArrowHead(arrowHead);}

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
        dataStack.print();
        std::cout << "   forShowHide=" << forShowHide << std::endl;
        std::cout << "   OPEMobject=" << OPEMobject << std::endl;
        if (material.isNull()) std::cout << "   material=null" << std::endl;
        else std::cout << "   material=" << material.toStdString() << std::endl;
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
        if (is_sport()) std::cout << "   item=sportNet" << std::endl;
        if (is_voltage()) std::cout << "   itemType=voltage" << std::endl;
        if (is_current()) std::cout << "   itemType=current" << std::endl;
        if (is_scale()) std::cout << "   itemType=scale" << std::endl;
        if (is_scaleValue()) std::cout << "   itemType=scaleValue" << std::endl;
        if (is_integrationPathSegment()) std::cout << "   itemType=integrationPathSegment" << std::endl;
        std::cout << "   dimTag.first=" << dimTag.first << std::endl
                  << "   dimTag.second=" << dimTag.second << std::endl;
    }



    void reset ()
    {
        //std::cout << "CustomTreeWidgetItem::reset  item=" << this << std::endl; std::cout.flush();
        dataStack.reset();
        parent=nullptr;
        children.clear();
        setForeground(0,Qt::black);
        setExpanded(Standard_False);
        meshEntities.clear();
        OPEMobject=nullptr;
        material.clear();
        linkedItems.clear();

        while (childCount() > 0) {
            CustomTreeWidgetItem *childItem=(CustomTreeWidgetItem *) child(0);
            if (childItem) {
                childItem->reset();
                removeChild(childItem);
                delete childItem;
            }
        }
    }

    bool hasUndo () {return dataStack.hasUndo();}
    bool hasRedo () {return dataStack.hasRedo();}

    void setParent (CustomTreeWidgetItem *parent_) {parent=parent_;}
    CustomTreeWidgetItem* getParent () {return parent;}

    void clearChildren () {children.clear();}
    void push_child (CustomTreeWidgetItem *child) {children.push_back(child);}
    long unsigned int getChildrenSize () {return children.size();}
    CustomTreeWidgetItem* getChild (long unsigned int i) {return children[i];}

    void convertPathToFace ()
    {
        Handle(AIS_Shape) shape=getShape();
        if (shape.IsNull()) return;

    }

private slots:

public:
    bool activeAction;                                 // for undo/redo, an active operation such as move, edit, stretch, etc. is in progress
    ShapeDataStack dataStack;                          // drawing object data with history for undo/redo
    CustomTreeWidgetItem *parent;                      // parent for undo/redo
    std::vector<CustomTreeWidgetItem *> children;      // children for undo/redo


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
    QString material;                                  // material for this item - only valid for top-level SOLID and COMPOUND
    std::vector<CustomTreeWidgetItem *> linkedItems;   // link to path items, if any



    int depth;                                         // item depth in the tree for saving formatted drawing files
};

class BaseItem : public CustomTreeWidgetItem
{
    Q_OBJECT

public:
    explicit BaseItem (QObject *parent = nullptr) {}

    void setMW (OpenParEMg *mw_) {mw=mw_;}

protected:
    OpenParEMg *mw;

};

class RootDrawingItem : public BaseItem
{
    Q_OBJECT

public:
    explicit RootDrawingItem (QObject *parent = nullptr) {}
    void showMenu (QMenu *);

private:

};

class DrawingItem : public BaseItem
{
    Q_OBJECT

public:
    explicit DrawingItem (QObject *parent = nullptr)
    {
        p0set=false;
        p1set=false;
        enableMove=false;
        enableStretch=false;
        enableDeletePoint=false;
        enableInsertPoint=false;
    }

    DrawingItem* copyCreate ();

    void startDraw ();
    void startLine ();
    void startPolyline ();
    void startRectangle ();
    void startPolycircle ();
    void finishDraw ();
    void cancelDraw ();

    void startMove ();
    void finishMove (gp_Pnt, gp_Pnt);

    void extrude ();

    void startEdit ();
    void finishEdit ();

    void showMenu (QMenu *);

    void setEnableMove (bool enableMove_) {enableMove=enableMove_;}
    bool getEnableMove () {return enableMove;}

    void setEnableStretch (bool enableStretch_) {enableStretch=enableStretch_;}
    bool getEnableStretch () {return enableStretch;}

    void setEnableDeletePoint (bool enableDeletePoint_) {enableDeletePoint=enableDeletePoint_;}
    bool getEnableDeletePoint () {return enableDeletePoint;}

    void setEnableInsertPoint (bool enableInsertPoint_) {enableInsertPoint=enableInsertPoint_;}
    bool getEnableInsertPoint () {return enableInsertPoint;}

    void setP0 (gp_Pnt p0_) {p0=p0_; p0set=true;}
    void setP1 (gp_Pnt p1_) {p1=p1_; p1set=true;}
    bool hasP0 () {return p0set;}
    bool hasP1 () {return p1set;}
    gp_Pnt getP0 () {return p0;}
    gp_Pnt getP1 () {return p1;}
    void resetP0P1 () {p0set=false; p1set=false;}

    gp_Trsf getTrsf () {return aTrsf;}
    void setTrsf (gp_Trsf aTrsf_) {aTrsf=aTrsf_;}
    void reset_transformation () {aTrsf=gp_Trsf();}

    TopoDS_Shape moveShape (gp_Pnt, gp_Pnt, Handle(AIS_InteractiveContext));
    void setAnimate (Handle(AIS_InteractiveContext));
    void unsetAnimate (Handle(AIS_InteractiveContext));
    void moveAnimateShape (gp_Pnt, gp_Pnt, Handle(AIS_InteractiveContext));

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

private:
    Handle(AIS_Shape) animateShape;                    // temporary shape for animation during moving
    gp_Trsf aTrsf;
    gp_Pnt p0,p1;                                      // for move operations
    bool p0set,p1set;
    bool enableMove;
    bool enableStretch;
    bool enableDeletePoint;
    bool enableInsertPoint;

};

#endif // CUSTOMTREEWIDGETITEM_H
