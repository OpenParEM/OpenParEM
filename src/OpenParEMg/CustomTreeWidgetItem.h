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
#include "path.hpp"

class DrawingItem;
class Port;
class Mode;
class IntegrationPath;
class Boundary;
class CustomLineEdit;
class VIItem;
class SportNumberItem;
class DiffPairItem;

// shape data with history for undo/redo

class ShapeData
{
public:

    ShapeData ()
    {
        // default to noop
        type=0;
        Sport=0;
        scale=1;
        name="name";
        polywire=nullptr;
        process=nullptr;
        prior=nullptr;
        next=nullptr;
    }

    ~ShapeData ()
    {
        if (polywire) {delete polywire; polywire=nullptr;}
        if (process) {delete process; process=nullptr;}
        if (!shape.IsNull()) {shape.Nullify();}
    }

    void copy (ShapeData *shapeData) {

        if (!shapeData) return;

        type=shapeData->type;

        if (polywire) {delete polywire; polywire=nullptr;}
        if (shapeData->polywire) {polywire=shapeData->polywire->copyCreate();}

        if (process) {delete process; process=nullptr;}
        if (shapeData->process) {process=shapeData->process->copyCreate();}

        name=shapeData->name;
        impedance_definition=shapeData->impedance_definition;
        impedance_calculation=shapeData->impedance_calculation;
        boundary_type=shapeData->boundary_type;
        boundary_material=shapeData->boundary_material;
        wave_impedance=shapeData->wave_impedance;
        Sport=shapeData->Sport;
        scale=shapeData->scale;
        //prior=shapeData->prior;
        //next=shapeData->next;

        if (!shape.IsNull()) shape.Nullify();
        if (!shapeData->getShape().IsNull()) {
            shape=new AIS_Shape(shapeData->getShape()->Shape());
        }
    }

    ShapeData* copyCreate ()
    {
        ShapeData *newShapeData=new ShapeData();
        if (newShapeData) {
            newShapeData->type=type;
            newShapeData->polywire=nullptr;
            newShapeData->process=nullptr;
            newShapeData->name=name;
            newShapeData->impedance_definition=impedance_definition;
            newShapeData->impedance_calculation=impedance_calculation;
            newShapeData->boundary_type=boundary_type;
            newShapeData->boundary_material=boundary_material;
            newShapeData->wave_impedance=wave_impedance;
            newShapeData->Sport=Sport;
            newShapeData->scale=scale;
            newShapeData->prior=prior;
            newShapeData->next=next;
            if (polywire) newShapeData->polywire=polywire->copyCreate();
            if (process) newShapeData->process=process->copyCreate();
            if (!shape.IsNull()) newShapeData->shape=new AIS_Shape(shape->Shape());
        }
        return newShapeData;
    }

    // void setType (int type_)
    // {
    //     type=type_;
    // }

    //int getType () {return type;}

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

    // void set (int type_, Polywire *polywire_, Process *process_, Handle(AIS_Shape) shape_)
    // {
    //     type=type_;
    //     setPolywire(polywire_);
    //     setProcess(process_);
    //     setShape(shape_);
    // }

    // void set (ShapeData *shapeData)
    // {
    //     if (!shapeData) return;
    //     type=shapeData->type;
    //     prior=shapeData->prior;
    //     next=shapeData->next;
    //     setPolywire(shapeData->getPolywire());
    //     setProcess(shapeData->getProcess());
    //     setShape(shapeData->getShape());
    // }

    void set (Handle(AIS_Shape) shape_)
    {
        if (shape_.IsNull()) return;
        setShape(shape_);
    }

    Polywire* getPolywire () {return polywire;}
    Process* getProcess () {return process;}
    Handle(AIS_Shape) getShape () {return shape;}

    bool isNoop () {if (type == 0) return true; return false;}
    bool isCreate () {if (type == 1) return true; return false;}
    bool isEdit () {if (type == 2) return true; return false;}
    bool isDelete () {if (type == 3) return true; return false;}
    bool isConvertToPath () {if (type == 4) return true; return false;}
    bool isConvertToPort () {if (type == 5) return true; return false;}
    bool isConvertToBoundary () {if (type == 6) return true; return false;}
    bool isReversePath () {if (type == 7) return true; return false;}
    bool isChangeName () {if (type == 8) return true; return false;}

    //void setNoop () {type=0;}
    void setCreate () {type=1;}
    void setEdit () {type=2;}
    void setDelete () {type=3;}
    //void setConvertToPath () {type=4;}
    //void setConvertToPort () {type=5;}
    //void setConvertToBoundary () {type=6;}
    void setReversePath () {type=7;}
    void setChangeName () {type=8;}

    void set_name (QString name_) {name=name_;}
    //void set_impedance_definition (std::string impedance_definition_) {impedance_definition=QString::fromStdString(impedance_definition_);}
    //void set_impedance_calculation (std::string impedance_calculation_) {impedance_calculation=QString::fromStdString(impedance_calculation_);}
    void set_impedance_definition (QString impedance_definition_) {impedance_definition=impedance_definition_;}
    void set_impedance_calculation (QString impedance_calculation_) {impedance_calculation=impedance_calculation_;}
    QString get_impedance_definition () {return impedance_definition;}
    QString get_impedance_calculation () {return impedance_calculation;}
    QString get_name () {return name;}

    void set_boundary_type (int boundary_type_) {boundary_type=boundary_type_;}
    int get_boundary_type () {return boundary_type;}

    void set_boundary_material (QString boundary_material_) {boundary_material=boundary_material_;}
    QString get_boundary_material () {return boundary_material;}

    void set_wave_impedance (double wave_impedance_) {wave_impedance=wave_impedance_;}
    double get_wave_impedance () {return wave_impedance;}

    void set_Sport (int Sport_) {Sport=Sport_;}
    int get_Sport () {return Sport;}

    void set_scale (double scale_) {scale=scale_;}
    double get_scale () {return scale;}

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
        if (isChangeName()) std::cout << "                  type=changeName" << std::endl;


        if (shape.IsNull()) std::cout << "                  shape=null" << std::endl;
        else {
            if (shape->Shape().IsNull()) std::cout << "                  shape->Shape()=null" << std::endl;
            else {
                std::cout << "                  shape type=" << TopAbs::ShapeTypeToString(shape->Shape().ShapeType()) << std::endl;
            }
        }
        std::cout << "                  polywire=" << polywire << std::endl;
        std::cout << "                  process=" << process << std::endl;
        std::cout << "                  name=" << name.toStdString() << std::endl;
        std::cout << "                  impedance_definition=" << impedance_definition.toStdString() << std::endl;
        std::cout << "                  impedance_calculation=" << impedance_calculation.toStdString() << std::endl;
        std::cout << "                  boundary_type=" << boundary_type<< std::endl;
        std::cout << "                  boundary_material=" << boundary_material.toStdString() << std::endl;
        std::cout << "                  wave_impedance=" << wave_impedance << std::endl;
        std::cout << "                  Sport=" << Sport << std::endl;
        std::cout << "                  scale=" << scale << std::endl;
        std::cout << "                  prior=" << prior << std::endl;
        std::cout << "                  next=" << next << std::endl;
    }

private:
    int type;                                          // 0 - noop; 1 - create; 2 - edit; 3 - delete;
                                                       // 4 - convert to path; 5 - convert to port; 6 - convert to boundary
                                                       // 7 - reverse path direction
                                                       // 8 - change name

    // for drawing
    Handle(AIS_Shape) shape;                           // drawing shape
    Polywire *polywire;                                // Polywire object for this item
    Process *process;                                  // for drawing processing of children

    // for widget reconstruction
    QString name;                                      // name for the item
    QString impedance_definition;                      // VI, PV, PI, invalid
    QString impedance_calculation;                     // line, modal
    int boundary_type;                                 // 0 - PEC, 1 - PMC, 2 - surface_impedance, 3 - radiation
    QString boundary_material;                         // material for the surface impedance
    double wave_impedance;                             // for radiation boundary
    int Sport;                                         // S-parameter port number
    double scale;                                      // scale factor for integration paths

    ShapeData *prior;                                  // prior ShapeData in ShapeDataStack
    ShapeData *next;                                   // next ShapeData in ShapeDataStack
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

    //long unsigned int getSize () {return shapeDataList.size();}

    // void add (int action_, Polywire *polywire_, Process *process_, Handle(AIS_Shape) shape_)
    // {
    //     ShapeData *shapeData=new ShapeData(action_,polywire_,process_,shape_);
    //     add(shapeData);
    // }

    void add (ShapeData *shapeData)
    {
        if (!shapeData) return;

        shapeDataList.push_back(shapeData);
        if (current) {
            shapeData->setPrior(current);
            current->setNext(shapeData);
        }
        current=shapeData;
    }

    // at current location

    // void set (int action_, Polywire *polywire_, Process *process_, Handle(AIS_Shape) shape_)
    // {
    //     current->set(action_,polywire_,process_,shape_);
    // }

    // void set (ShapeData *shapeData_)
    // {
    //     if (!shapeData_) return;
    //     current->set(shapeData_);
    // }

    // void setType (int type_)
    // {
    //     current->setType(type_);
    // }

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

    // void setProcess (Process *process_)
    // {
    //     if (!process_) return;
    //     current->setProcess(process_);
    // }

    // void setName (QString name)
    // {
    //     current->set_name(name);
    // }

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

    Handle(AIS_Shape) getShape ()
    {
        Handle(AIS_Shape) shape;
        if (current) shape=current->getShape();
        return shape;
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
            if (shapeDataList[i]) {
                delete shapeDataList[i];
                shapeDataList[i]=nullptr;
            }
            i++;
        }
        shapeDataList.clear();
        current=nullptr;

        // always put a noop at the bottom of the stack
        ShapeData *noop=new ShapeData();
        current=noop;
        shapeDataList.push_back(noop);
    }

    // void pop ()
    // {
    //     if (current == shapeDataList[shapeDataList.size()-1]) {
    //         long unsigned int i=0;
    //         while (i < shapeDataList.size()) {
    //             if (shapeDataList[i]->getNext() == current) {
    //                 shapeDataList[i]->setNext(nullptr);
    //             }
    //             if (shapeDataList[i]->getPrior() == current) {
    //                 shapeDataList[i]->setPrior(nullptr);
    //             }
    //             i++;
    //         }

    //         if (shapeDataList[shapeDataList.size()-1]) {
    //             delete shapeDataList[shapeDataList.size()-1];
    //             shapeDataList[shapeDataList.size()-1]=nullptr;
    //         }
    //         shapeDataList.pop_back();
    //         current=shapeDataList[shapeDataList.size()-1];
    //     }
    // }

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

class BaseItem : public QObject, public QTreeWidgetItem {
    Q_OBJECT

public:

    BaseItem ();
    BaseItem (OpenParEMg *, BaseItem *);

    BaseItem* getRootParent ();

    bool isModified () {return modified;}
    void setModified (bool);
    void reset_modifiedSinceMeshRegen () {modifiedSinceMeshRegen=false;}
    bool isModifiedSinceMeshRegen () {return modifiedSinceMeshRegen;}

    void showDisplayStatus ();
    void alignForegroundColor ();

    void startItemChange ();
    void addItemChange ();

    void restoreWidgets ();
    void setForUndoRedo (bool, int);

    virtual void rename (QString);
    virtual bool isValidShow () {return false;}
    virtual bool isValidHide () {return false;}
    virtual void show (bool) {}
    virtual void hide (bool) {}
    virtual void showMenu (QMenu *) {}

    void addShapeData (ShapeData *shapeData_) {dataStack.add(shapeData_);}
    void setShape (Handle(AIS_Shape) shape_) {dataStack.setShape(shape_);}
    void setPolywire (Polywire *polywire_) {dataStack.setPolywire(polywire_);}

    virtual bool hasP0 () {return false;}
    virtual bool hasP1 () {return false;}

    virtual void setP0 (gp_Pnt p0) {}
    virtual void setP1 (gp_Pnt p1) {}

    virtual void finishStretchPoint () {}
    virtual void finishDeletePoint () {}
    virtual void finishInsertPoint () {}

    virtual void cancelOperation () {}

    ShapeData* getShapeData () {return dataStack.getShapeData();}
    Polywire* getPolywire () {return dataStack.getPolywire();}
    Process* getProcess () {return dataStack.getProcess();}
    Handle(AIS_Shape) getShape () {return dataStack.getShape();}

    virtual bool getEnableMove () {return false;}
    virtual bool getEnableStretch () {return false;}
    virtual bool getEnableDeletePoint () {return false;}
    virtual bool getEnableInsertPoint () {return false;}

    virtual void del () {}

    BaseItem* findTopLevelItem (BaseItem *, BaseItem *);
    virtual BaseItem* findTopLevelItem (BaseItem *) {return nullptr;}

    virtual void undo ();
    virtual void redo ();
    //void pop () {dataStack.pop();}

    void set_itemType (int itemType_) {itemType=itemType_;}
    int get_itemType () {return itemType;}

    bool is_undefined () {if (itemType == -1) return true; return false;}
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
    bool is_scaleLabel () {if (itemType == 12) return true; return false;}
    bool is_scaleValue () {if (itemType == 13) return true; return false;}
    bool is_integrationPathSegment () {if (itemType == 14) return true; return false;}
    bool is_diffpair () {if (itemType == 15) return true; return false;}
    bool is_boundaryType () {if (itemType == 20) return true; return false;}
    bool is_boundaryWaveImpedance () {if (itemType == 21) return true; return false;}
    bool is_boundaryMaterial () {if (itemType == 22) return true; return false;}
    bool is_rootDrawing () {if (itemType == 100) return true; return false;}
    bool is_rootPort () {if (itemType == 101) return true; return false;}
    bool is_rootBoundary () {if (itemType == 102) return true; return false;}
    bool is_rootMesh () {if (itemType == 103) return true; return false;}
    bool is_rootPath () {if (itemType == 104) return true; return false;}

    void deleteChildren (QTreeWidgetItem *item)
    {
        if (!item) return;
        QList<QTreeWidgetItem*> children=item->takeChildren();
        for (QTreeWidgetItem* child : std::as_const(children)) {
            if (child) {
                deleteChildren(child);
                delete child;
            }
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

    void print_itemType ()
    {
        if (is_undefined()) std::cout << "undefined" << std::endl;
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
        if (is_scaleLabel()) std::cout << "scale label" << std::endl;
        if (is_scaleValue()) std::cout << "scale value" << std::endl;
        if (is_integrationPathSegment()) std::cout << "integration path segment" << std::endl;
        if (is_diffpair()) std::cout << "differential pair" << std::endl;
        if (is_boundaryType()) std::cout << "boundary type" << std::endl;
        if (is_boundaryWaveImpedance()) std::cout << "boundary wave impedance" << std::endl;
        if (is_boundaryMaterial()) std::cout << "boundary wave material" << std::endl;
    }

    void print ()
    {
        std::cout << "BaseItem:" << std::endl;
        dataStack.print();
        std::cout << "   itemType=" << itemType << std::endl;
        if (is_undefined()) std::cout << "   itemType=undefined" << std::endl;
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
        if (is_scaleLabel()) std::cout << "   itemType=scale" << std::endl;
        if (is_scaleValue()) std::cout << "   itemType=scaleValue" << std::endl;
        if (is_integrationPathSegment()) std::cout << "   itemType=integrationPathSegment" << std::endl;
        if (is_diffpair()) std::cout << "   itemType=diffpair" << std::endl;
        if (is_boundaryType()) std::cout << "   itemType=boundary type" << std::endl;
        if (is_boundaryWaveImpedance()) std::cout << "   itemType=boundary wave impedance" << std::endl;
        if (is_boundaryMaterial()) std::cout << "   itemType=boundary wave material" << std::endl;
    }

    void reset ()
    {
        dataStack.reset();
        parentItem=nullptr;
        children.clear();
        setForeground(0,Qt::black);
        setExpanded(Standard_False);

        while (childCount() > 0) {
            BaseItem *childItem=dynamic_cast<BaseItem *>(child(0));
            if (childItem) {
                childItem->reset();
                removeChild(childItem);
                delete childItem;
            }
        }
    }

    virtual bool hasUndo () {return dataStack.hasUndo();}
    virtual bool hasRedo () {return dataStack.hasRedo();}

    BaseItem* getParentItem () {return parentItem;}

    void push_child (BaseItem *child) {children.push_back(child);}
    long unsigned int getChildrenSize () {return children.size();}
    BaseItem* getChild (long unsigned int i) {return children[i];}

    virtual void promoteChildren () {}
    virtual void demoteChildren () {}

    virtual void save (std::ofstream *);

    void addSport (long unsigned int);
    void removeSport (long unsigned int);
    bool isAssignedSport (long unsigned int);
    long unsigned int nextSport ();

private slots:

protected:
    OpenParEMg *mw;
    ShapeDataStack dataStack;                          // history for undo/redo
    BaseItem *parentItem;                              // parent for undo/redo
    std::vector<BaseItem *> children;                  // children for undo/redo
    int itemType;                                      // 0 - drawing, 1 - port, 2 - boundary, 3 - mesh, 4 - path
                                                       // 5 - Sport (net), 6 - impedance definition, 7 - impedance calculation
                                                       // 8 - Sport label, 9 - Sport number,
                                                       // 10 - voltage, 11 - current
                                                       // 12 - scale label, 13 - scale value
                                                       // 14 - integration path segment
                                                       // 15 - differential pair
                                                       // 20 - boundary type
                                                       // 21 - boundary wave impedance
                                                       // 22 - boundary material
                                                       // 100 - root drawing item
                                                       // 101 - root port item
                                                       // 102 - root boundary item
                                                       // 103 - root mesh item
                                                       // 104 - root path item

    bool modified;                                     // markes item as modified;  true on creation
    bool modifiedSinceMeshRegen;                       // supplemental modification tracker to determine need for mesh regeneration
};

class ScaleLabelItem : public BaseItem
{
    Q_OBJECT

public:
    ScaleLabelItem (OpenParEMg *mw_, VIItem *parentItem_);

    //void setVIItem (VIItem *viItem_) {viItem=viItem_;}
    //VIItem* getVIItem () {return viItem;}

    void hide (bool) override;
    void show (bool) override;

    void save (std::ofstream *) override;

private:
    VIItem *viItem;
};

class ScaleValueItem : public BaseItem
{
    Q_OBJECT

public:
    ScaleValueItem (OpenParEMg *mw_, ScaleLabelItem *parentItem_);

    void insertScaleValueWidget (double);
    void undo () override;
    void redo () override;
    bool hasUndo () override {return dataStack.hasUndo();}
    bool hasRedo () override {return dataStack.hasRedo();}

    void save (std::ofstream *) override;

private:
    ScaleLabelItem *scaleLabelItem;
};


class RootDrawingItem : public BaseItem
{
    Q_OBJECT

public:
    RootDrawingItem (OpenParEMg *mw_)
    {
        mw=mw_;
        itemType=100;
        parentItem=nullptr;
        modified=false;
        setForeground(0,Qt::black);
    }

    bool isValidShow () override;
    bool isValidHide () override;
    bool isValidSelectAll ();
    void show (bool) override;
    void hide (bool) override;
    void selectAll ();
    void showMenu (QMenu *) override;

private:

};

class DrawingItem : public BaseItem
{
    Q_OBJECT

public:
    DrawingItem () {}
    DrawingItem (OpenParEMg *, BaseItem *);

    void setParentItem (BaseItem *parentItem_) {parentItem=parentItem_;}
    BaseItem* getParentItem () {return parentItem;}

    QString get_material () {return text(1);}
    void set_Material (QString material_) {material=material_;}

    void set_dimTag (int dim, int tag) {dimTag.first=dim; dimTag.second=tag;}
    std::pair<int,int> get_dimTag () {return dimTag;}

    void copy_depth (DrawingItem *item) {
        if (item) depth=item->depth;
        else depth=0;
    }
    void set_depth (int depth_) {depth=depth_;}
    int get_depth () {return depth;}
    void increase_depth () {depth++;}
    void decrease_depth () {depth--;}

    DrawingItem* copyCreate ();

    void promoteChildren () override;
    void demoteChildren () override;

    void cancelOperation () override;

    void startDraw ();
    void startLine ();
    void startPolyline ();
    void startRectangle ();
    void startPolycircle ();
    void finishDraw ();
    void cancelDraw ();

    void startMove (bool isAnimate=true);
    void finishMove (gp_Pnt, gp_Pnt);

    void startRotate ();
    void finishRotate (double, gp_Pnt, gp_Pnt);

    void startStretch ();
    void finishStretch ();

    void extrude ();
    DrawingItem* copy (BaseItem *);

    void startEdit ();
    void finishEdit ();

    void startDeletePoint ();
    void finishDeletePoint () override;
    void cancelDeletePoint ();

    void startInsertPoint ();
    void finishInsertPoint () override;
    void finishStretchPoint () override;
    void cancelInsertPoint ();

    void convertToPolyline ();

    void del () override;

    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;

    void setEnableMove (bool enableMove_) {enableMove=enableMove_;}
    bool getEnableMove () override {return enableMove;}

    void setEnableStretch (bool enableStretch_) {enableStretch=enableStretch_;}
    bool getEnableStretch () override {return enableStretch;}

    void setEnableDeletePoint (bool enableDeletePoint_) {enableDeletePoint=enableDeletePoint_;}
    bool getEnableDeletePoint () override {return enableDeletePoint;}

    void setEnableInsertPoint (bool enableInsertPoint_) {enableInsertPoint=enableInsertPoint_;}
    bool getEnableInsertPoint () override {return enableInsertPoint;}

    void setP0 (gp_Pnt p0_) override {p0=p0_; p0set=true;}
    void setP1 (gp_Pnt p1_) override {p1=p1_; p1set=true;}
    bool hasP0 () override {return p0set;}
    bool hasP1 () override {return p1set;}
    gp_Pnt getP0 () {return p0;}
    gp_Pnt getP1 () {return p1;}

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

    PathItem* createPath (bool);

    BaseItem* findTopLevelItem (BaseItem *) override;

    void undo () override;
    void redo () override;

    bool hasUndo () override {return dataStack.hasUndo();}
    bool hasRedo () override {return dataStack.hasRedo();}

protected:
    QString material;                                  // material for this item - only valid for top-level SOLID and COMPOUND
    std::pair<int,int> dimTag;                         // supports meshing with gmsh
    int depth;                                         // item depth in the tree for saving formatted drawing files

    Handle(AIS_Shape) animateShape;                    // temporary shape for animation during moving
    gp_Trsf aTrsf;
    gp_Pnt p0,p1;                                      // for move operations
    bool p0set,p1set;
    bool enableMove;
    bool enableStretch;
    bool enableDeletePoint;
    bool enableInsertPoint;
};

class RootPathItem : public BaseItem
{
    Q_OBJECT

public:
    RootPathItem (OpenParEMg *mw_)
    {
        mw=mw_;
        itemType=104;
        parentItem=nullptr;
        modified=false;
        setForeground(0,Qt::black);
    }

    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;
    bool isUniquePathName (QString);
    QString getUniquePathName ();
    QString getUniquePathName (QString);

private:

};

class PathItem : public DrawingItem
{
    Q_OBJECT

public:
    PathItem (OpenParEMg *, BaseItem *);

    ~PathItem ()
    {
        if (path) delete path;
    }

    long unsigned int linkedItems_size () {return linkedItems.size();}
    void push_linkedItem (BaseItem *linkedItem) {linkedItems.push_back(linkedItem);}
    BaseItem* get_linkedItem (long unsigned int i) {return linkedItems[i];}

    void removeLinkedItem (BaseItem *item)
    {
        long unsigned int i=0;
        while (i < linkedItems.size()) {
            if (linkedItems[i] == item) linkedItems.erase(linkedItems.begin()+i);
            i++;
        }
    }

    //void clearLinkedItems () {linkedItems.clear();}

    //void setHasArrows (bool hasArrows_)  {hasArrows=hasArrows_;}
    //bool getHasArrows () {return hasArrows;}

    void rename (QString) override;

    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;
    void del () override;
    void setPath (Path *path_) {path=path_;}
    Path* getPath () {return path;}
    BaseItem* findTopLevelItem (BaseItem *) override;
    void undo () override;
    void redo () override;
    bool hasUndo () override {return dataStack.hasUndo();}
    bool hasRedo () override {return dataStack.hasRedo();}
    void reverse ();
    void showArrows (bool);
    void save (std::ofstream *) override;

private:
    Path *path;                            // related path from the boundary database
    std::vector<BaseItem *> linkedItems;   // items that use this path item
    bool hasArrows;
};

class IntegrationPathItem : public BaseItem
{
    Q_OBJECT

public:   
    IntegrationPathItem (OpenParEMg *, BaseItem *, PathItem *);

    void setPathItem (PathItem *pathItem_) {pathItem=pathItem_;}
    PathItem* getPathItem () {return pathItem;}

    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;
    void undo () override;
    void redo () override;
    bool hasUndo () override {return dataStack.hasUndo();}
    bool hasRedo () override {return dataStack.hasRedo();}

    void del () override;
    void flipSign ();

    void save (std::ofstream *) override;
    void saveN (std::ofstream *);
private:
    PathItem *pathItem;                  // base path for this integration path
};

class RootBoundaryItem : public BaseItem
{
    Q_OBJECT

public:
    RootBoundaryItem (OpenParEMg *mw_)
    {
        mw=mw_;
        itemType=102;
        parentItem=nullptr;
        modified=false;
        setForeground(0,Qt::black);
    }

    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;
    bool isUniqueBoundaryName (QString);
    QString getUniqueBoundaryName ();

private:

};

class BoundaryItem : public BaseItem
{
    Q_OBJECT

public:
    BoundaryItem (OpenParEMg *, PathItem *, int, double, QString);

    void setSolidColor ();
    void insertItemWidgets (BaseItem *, BaseItem *, BaseItem *);
    void resetWidgets ();
    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;
    void del () override;
    //void setPathItem (PathItem *pathItem_) {pathItem=pathItem_;}
    PathItem* getPathItem () {return pathItem;}
    void undo () override;
    void redo () override;
    bool hasUndo () override {return dataStack.hasUndo();}
    bool hasRedo () override {return dataStack.hasRedo();}

    void save (std::ofstream *) override;

private:
    PathItem *pathItem;   // PathItem associated with this boundary
};

class RootPortItem : public BaseItem
{
    Q_OBJECT

public:
    RootPortItem (OpenParEMg *mw_)
    {
        mw=mw_;
        itemType=101;
        parentItem=nullptr;
        modified=false;
        setForeground(0,Qt::black);
    }

    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;

    int get_SportCount ();

    bool isUniquePortName (QString);
    bool isUniqueNetName (QString);
    QString getUniquePortName ();

private:

};

class PortItem : public BaseItem
{
    Q_OBJECT

public:
    PortItem (OpenParEMg *, PathItem *, QString, QString);

    void setSolidColor ();
    void insertImpedanceDefinitionWidget (BaseItem *, QString);
    void addImpedanceDefinitionItem ();
    void insertImpedanceCalculationWidget (BaseItem *, QString);
    void addImpedanceCalculationItem ();

    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;
    void del () override;
    void setPathItem (PathItem *pathItem_) {pathItem=pathItem_;}
    PathItem* getPathItem () {return pathItem;}
    void undo () override;
    void redo () override;
    bool hasUndo () override {return dataStack.hasUndo();}
    bool hasRedo () override {return dataStack.hasRedo();}

    int get_SportCount ();
    void save (std::ofstream *) override;

    bool isValid ();

    bool hasNet (QString);

private:
    PathItem *pathItem;   // PathItem associated with this port
};

class ModeItem : public BaseItem
{
    Q_OBJECT

public:
    ModeItem () {}
    ModeItem (OpenParEMg *, BaseItem *, bool);

    // void startItemChange () override;
    // void addItemChange () override;

    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    bool hasUndo () override {return dataStack.hasUndo();}
    bool hasRedo () override {return dataStack.hasRedo();}

    bool isValidDelete ();
    void unlinkPaths (BaseItem *);
    void del () override;
    void showMenu (QMenu *) override;
    //void setPortItem (PortItem *portItem_) {portItem=portItem_;}
    //PortItem* getPortItem () {return portItem;}

    void setParentItem (BaseItem *baseItem) {parentItem=baseItem;}

    int get_SportCount ();
    int get_Sport ();

    void save (std::ofstream *) override;

private:
    //PortItem *portItem;   // PortItem associated with this port
};

class SportItem : public BaseItem
{
    Q_OBJECT

public:
    SportItem () {}
    SportItem (OpenParEMg *, ModeItem *, int);

    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;
    //void setPortItem (ModeItem *modeItem_) {modeItem=modeItem_;}
    //ModeItem* getModeItem () {return modeItem;}

    int get_SportCount ();

private:
    ModeItem *modeItem;
};

class SportNumberItem : public BaseItem
{
    Q_OBJECT

public:
    SportNumberItem () {}
    SportNumberItem (OpenParEMg *, SportItem *);

    // void startItemChange () override;
    // void addItemChange () override;

    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;
    void undo () override;
    void redo () override;

    void insertSportNumberWidget (int);
    int get_Sport ();

private:
    SportItem *sportItem;
};

class VIItem : public BaseItem
{
    Q_OBJECT

public:
    VIItem () {}
    VIItem (OpenParEMg *, ModeItem *, int);
    bool isValidShow () override;
    bool isValidHide () override;
    bool isValidDrawPath ();
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;
    bool setPlane (gp_Pln &plane);
    void drawLinePath ();
    void drawPolylinePath ();
    bool isValidInsertSelectedPath ();
    bool hasScale ();
    bool hasIntegrationPathItem ();
    void addScaleItem ();
    void removeScaleItem ();
    void addRemoveScale ();
    IntegrationPathItem* createIntegrationPathItemFromPath (PathItem *);
    IntegrationPathItem* createIntegrationPathItemFromDrawing (DrawingItem *, bool);
    //void convertItemToPath (DrawingItem *, bool);
    ModeItem* getModeItem () {return modeItem;}
    PortItem* getPortItem ();

    void save (std::ofstream *) override;

private:
    ModeItem *modeItem;
    ScaleLabelItem *scaleLabelItem;
};

class DiffPairItem : public BaseItem
{
    Q_OBJECT

public:
    DiffPairItem (OpenParEMg *, PortItem *, ModeItem *, ModeItem *);
    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void undo () override;
    void redo () override;
    bool hasUndo () override {return dataStack.hasUndo();}
    bool hasRedo () override {return dataStack.hasRedo();}
    void showMenu (QMenu *) override;
    //void setPortItem (PortItem *portItem_) {portItem=portItem_;}
    //PortItem* getPortItem () {return portItem;}

    bool isValidDelete ();
    void del () override;

    void promoteChildren () override;
    void demoteChildren () override;

    void enableZcalcControl (bool);
    void save (std::ofstream *) override;

private:
    PortItem *portItem;
};


class RootMeshItem : public BaseItem
{
    Q_OBJECT

public:
    RootMeshItem (OpenParEMg *mw_)
    {
        mw=mw_;
        itemType=103;
        parentItem=nullptr;
        modified=false;
        setForeground(0,Qt::black);
    }
    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;

private:

};

class MeshItem : public BaseItem
{
    Q_OBJECT

public:
    MeshItem (OpenParEMg *);

    bool isValidShow () override;
    bool isValidHide () override;
    void show (bool) override;
    void hide (bool) override;
    void showMenu (QMenu *) override;

private:
};


#endif // CUSTOMTREEWIDGETITEM_H
