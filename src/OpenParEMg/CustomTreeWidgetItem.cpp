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

#include "CustomTreeWidgetItem.h"
#include "CustomComboBox.h"
#include "CustomSpinBox.h"
#include "OPEMg.h"
#include "ui_OPEMg.h"
#include <BRepPrimAPI_MakePrism.hxx>
#include <TopoDS_Iterator.hxx>

////////////////////////////////////////////////////////////////////////////////
// BaseItem
////////////////////////////////////////////////////////////////////////////////

BaseItem::BaseItem () {}

BaseItem::BaseItem (OpenParEMg *mw_, BaseItem *parentItem_)
{
    mw=mw_;
    parentItem=parentItem_;
    itemType=-1;
    setText(0,"BaseItem");
    setForeground(0,Qt::black);

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->set_name(text(0));
    addShapeData(newShapeData);
}

void BaseItem::setForUndoRedo (bool withMidPoints, int shapeOperation)
{
    // clone the item onto itself for undo/redo
    // Do this before deleting the shape below
    ShapeData *newShapeData=getShapeData()->copyCreate();

    bool hasShape=false;
    if (!getShape().IsNull()) hasShape=true;

    bool isDisplayed=false;

    if (hasShape) {
        isDisplayed=mw->ui->drawingWindow->isDisplayed(getShape());

        // remove the old version from display and tracking
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());
    }

    // save the new data
    if (shapeOperation == 0) newShapeData->setEdit();
    else if (shapeOperation == 1) newShapeData->setChangeName();
    addShapeData(newShapeData);

    if (hasShape) {
        // put the new version into display and tracking
        mw->ui->drawingWindow->insertItemToMap(getShape(),this);
        if (isDisplayed) {
            mw->ui->drawingWindow->displayShape(getShape());
            setForeground(0,Qt::gray);
            mw->ui->drawingWindow->showItem(this);
        }
    }

    // reset the selection filters
    mw->startOperation(withMidPoints);
}

void BaseItem::restoreWidgets (BaseItem *baseItem)
{
    if (!baseItem) return;

    if (baseItem && baseItem->is_boundary()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(baseItem);
        if (boundaryItem && boundaryItem->is_boundary()) {
            BaseItem *boundaryType=nullptr;
            BaseItem *boundaryWaveImpedance=nullptr;
            BaseItem *boundaryMaterial=nullptr;

            int i=0;
            while (i < boundaryItem->childCount()) {
                BaseItem *baseItem=dynamic_cast<BaseItem *>(boundaryItem->child(i));
                if (baseItem) {
                    if (baseItem->is_boundaryType()) boundaryType=baseItem;
                    else if (baseItem->is_boundaryWaveImpedance()) boundaryWaveImpedance=baseItem;
                    else if (baseItem->is_boundaryMaterial()) boundaryMaterial=baseItem;
                }
                i++;
            }

            boundaryItem->insertItemWidgets(boundaryType,boundaryWaveImpedance,boundaryMaterial);
        }
    }

    if (baseItem && baseItem->is_port()) {
        PortItem *portItem=dynamic_cast<PortItem *>(baseItem);
        if (portItem && is_port()) {
            portItem->setSolidColor();
        }
    }

    if (baseItem && baseItem->is_impedanceCalculation()) {
        PortItem *portItem=dynamic_cast<PortItem *>(baseItem->getParentItem());
        if (portItem && is_port()) {
            ShapeData *shapeData=portItem->getShapeData();
            QString impedance_calculation=shapeData->get_impedance_calculation();
            portItem->insertImpedanceCalculationWidget(baseItem,impedance_calculation);
        }
    }

    if (baseItem && baseItem->is_impedanceDefinition()) {
        PortItem *portItem=dynamic_cast<PortItem *>(baseItem->getParentItem());
        if (portItem && is_port()) {
            ShapeData *shapeData=portItem->getShapeData();
            QString impedance_definition=shapeData->get_impedance_definition();
            portItem->insertImpedanceDefinitionWidget(baseItem,impedance_definition);
        }
    }

    if (baseItem && baseItem->is_sportNumber()) {
        SportItem *sportItem=dynamic_cast<SportItem *>(baseItem->getParentItem());
        if (sportItem && sportItem->is_sportLabel()) {
            ShapeData *shapeData=baseItem->getShapeData();
            int Sport=shapeData->get_Sport();
            sportItem->insertSportNumberWidget(baseItem,Sport);
        }
    }

    if (baseItem && baseItem->is_scaleValue()) {
        ScaleValueItem *scaleValueItem=dynamic_cast<ScaleValueItem *>(baseItem);
        if (scaleValueItem && scaleValueItem->is_scaleValue()) {
            ShapeData *shapeData=baseItem->getShapeData();
            double scale=shapeData->get_scale();
            scaleValueItem->insertScaleValueWidget(scale);

            // // make sure the scale is on top of the paths
            // std::cout << "place 1" << std::endl; std::cout.flush();
            // ScaleLabelItem *scaleLabelItem=scaleValueItem->getScaleLabelItem();
            // if (scaleLabelItem && scaleLabelItem->is_scaleLabel()) {
            //     std::cout << "place 2" << std::endl; std::cout.flush();
            //     VIItem *viItem=scaleLabelItem->getVIItem();
            //     if (viItem) {
            //         std::cout << "place 3" << std::endl; std::cout.flush();
            //         if (viItem->is_voltage() || viItem->is_current()) {
            //             std::cout << "place 4" << std::endl; std::cout.flush();
            //             int index=viItem->indexOfChild(scaleValueItem);
            //             viItem->takeChild(index);
            //             viItem->insertChild(0,scaleValueItem);
            //         }
            //     }
            // }
        }
    }

    if (baseItem) {
        int i=0;
        while (i < baseItem->childCount()) {
            BaseItem *child=dynamic_cast<BaseItem *>(baseItem->child(i));
            if (child) restoreWidgets(child);
            i++;
        }
    }
}

void BaseItem::startItemChange () {mw->itemChangesStack.startNew();}
void BaseItem::addItemChange () {mw->itemChangesStack.add(this);}

void BaseItem::rename (QString name)
{
    setForUndoRedo(false,1);
    ShapeData *shapeData=getShapeData();
    shapeData->set_name(name);

    mw->itemChangesStack.startNew();
    mw->itemChangesStack.add(this);

    mw->finishOperation(false,1);
}

void BaseItem::expandToItem ()
{
    std::cout << "BaseItem::expandToItem  text(0)=" << text(0).toStdString() << std::endl; std::cout.flush();

    BaseItem *baseItem=getParentItem();
    for (BaseItem *p=baseItem; p; p=p->getParentItem())
    {
        p->setExpanded(true);
    }

    mw->ui->drawingWindow->unselectAllItems();
    setForeground(0,Qt::black);
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->showItem(this);
    mw->ui->drawingWindow->selectItem(this);
    mw->ui->drawingItemTree->scrollToItem(this);
}

void BaseItem::expandToItemPlus1 ()
{
    std::cout << "BaseItem::expandToItem1  text(0)=" << text(0).toStdString() << std::endl; std::cout.flush();

    BaseItem *baseItem=this;
    for (BaseItem *p=baseItem; p; p=p->getParentItem())
    {
        p->setExpanded(true);
    }

    mw->ui->drawingWindow->unselectAllItems();
    setForeground(0,Qt::black);
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->showItem(this);
    mw->ui->drawingWindow->selectItem(this);
    mw->ui->drawingItemTree->scrollToItem(this);
}

BaseItem* BaseItem::findTopLevelItem (BaseItem *parentItem, BaseItem *currentItem)
{
    if (!currentItem) {
        return nullptr;
    }

    BaseItem *currentParentItem=currentItem->getParentItem();
    if (!currentParentItem) {
        return currentItem;
    }

    while (currentParentItem != parentItem) {
        currentItem=currentParentItem;
        currentParentItem=currentItem->getParentItem();
    }

    currentItem->setForeground(0,Qt::black);
    mw->ui->drawingWindow->hideItem(currentItem);
    mw->ui->drawingWindow->showItem(currentItem);

    return currentItem;
}

void BaseItem::undo ()
{
    std::cout << "BaseItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    shapeData->print();

    if (shapeData->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // nothing to do
    } else if (shapeData->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());
        getParentItem()->removeChild(this);
        dataStack.undo();
        expandToItemPlus1();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
        dataStack.undo();
        restoreWidgets(this);
        expandToItemPlus1();
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
        dataStack.undo();

        Handle(AIS_Shape) shape=getShape();
        if (!shape.IsNull()) {
            mw->ui->drawingWindow->displayShape(shape);
            mw->ui->drawingWindow->insertItemToMap(shape,this);
        }

        getParentItem()->addChild(this);
        mw->ui->drawingWindow->showItem(this);
        mw->ui->drawingWindow->activateItem(this);
        restoreWidgets(this);
        expandToItemPlus1();
    } else if (shapeData->isChangeName()) {
        std::cout << "   isChangeName" << std::endl; std::cout.flush();

        bool hasShape=false;
        if (!getShape().IsNull()) hasShape=true;

        if (hasShape) {
            mw->ui->drawingWindow->hideItem(this);
            mw->ui->drawingWindow->removeItemFromMap(this);
            mw->ui->drawingWindow->deleteShape(getShape());
        }

        dataStack.undo();
        setText(0,getShapeData()->get_name());

        if (hasShape) {
            Handle(AIS_Shape) shape=getShape();
            if (!shape.IsNull()) {
                mw->ui->drawingWindow->displayShape(shape);
                mw->ui->drawingWindow->insertItemToMap(shape,this);
            }

            mw->ui->drawingWindow->showItem(this);
            mw->ui->drawingWindow->activateItem(this);
        }

        restoreWidgets(this);
        expandToItem();
    }
}

void BaseItem::redo ()
{
    std::cout << "BaseItem::redo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    ShapeData *next=shapeData->getNext();
    if (!next) return;

    if (next->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // should not occur
    } else if (next->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();
        dataStack.redo();

        Handle(AIS_Shape) shape=getShape();
        if (!shape.IsNull()) {
            mw->ui->drawingWindow->displayShape(shape);
            mw->ui->drawingWindow->insertItemToMap(shape,this);
        }

        getParentItem()->addChild(this);
        mw->ui->drawingWindow->showItem(this);
        mw->ui->drawingWindow->activateItem(this);
        restoreWidgets(this);
        expandToItemPlus1();
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
        dataStack.redo();
        restoreWidgets(this);
        expandToItemPlus1();
    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());
        getParentItem()->removeChild(this);
        dataStack.redo();
    } else if (next->isChangeName()) {
        std::cout << "   isChangeName" << std::endl; std::cout.flush();

        bool hasShape=false;
        if (!getShape().IsNull()) hasShape=true;

        if (hasShape) {
            mw->ui->drawingWindow->hideItem(this);
            mw->ui->drawingWindow->removeItemFromMap(this);
            mw->ui->drawingWindow->deleteShape(getShape());
        }

        dataStack.redo();
        setText(0,getShapeData()->get_name());

        if (hasShape) {
            Handle(AIS_Shape) shape=getShape();
            if (!shape.IsNull()) {
                mw->ui->drawingWindow->displayShape(shape);
                mw->ui->drawingWindow->insertItemToMap(shape,this);
            }

            mw->ui->drawingWindow->showItem(this);
            mw->ui->drawingWindow->activateItem(this);
        }

        restoreWidgets(this);
        expandToItem();
    }
}

////////////////////////////////////////////////////////////////////////////////
// ScaleLabelItem
////////////////////////////////////////////////////////////////////////////////

ScaleLabelItem::ScaleLabelItem (OpenParEMg *mw_, VIItem *parentItem_)
{
    mw=mw_;
    parentItem=parentItem_;
    itemType=12;
    viItem=parentItem_;
    setForeground(0,Qt::black);
    setText(0,"Scale");
    setFlags(flags() & ~Qt::ItemIsEditable);
    setToolTip(0,"Scale factor for the integration path.");

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->set_name(text(0));
    addShapeData(newShapeData);
}

////////////////////////////////////////////////////////////////////////////////
// ScaleValueItem
////////////////////////////////////////////////////////////////////////////////

ScaleValueItem::ScaleValueItem (OpenParEMg *mw_, ScaleLabelItem *parentItem_)
{
    mw=mw_;
    parentItem=parentItem_;
    itemType=13;
    scaleLabelItem=parentItem_;
    setForeground(0,Qt::black);
    setFlags(flags() & ~Qt::ItemIsSelectable);

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->set_scale(1); // default
    addShapeData(newShapeData);
}


void ScaleValueItem::insertScaleValueWidget (double scale)
{
    QDoubleValidator doubleValidator;
    doubleValidator.setBottom(0);

    CustomLineEdit *scaleEdit=new CustomLineEdit();
    const QSignalBlocker blocker(scaleEdit);
    scaleEdit->setValidator(&doubleValidator);
    scaleEdit->setText(QString::number(scale,'g'));
    scaleEdit->set_itemTracker(mw->ui->drawingWindow->get_itemTracker());
    scaleEdit->set_baseItem(this);
    mw->ui->drawingItemTree->setItemWidget(this,0,scaleEdit);

    QObject::connect(scaleEdit,&CustomLineEdit::CustomEditFinished,&textValueChanged);
}

////////////////////////////////////////////////////////////////////////////////
// RootDrawingItem
////////////////////////////////////////////////////////////////////////////////

bool RootDrawingItem::isValidShow ()
{
    int i=0;
    while (i < mw->drawing->childCount()) {
        DrawingItem *child=dynamic_cast<DrawingItem *>(mw->drawing->child(i));
        if (child->isValidShow()) return true;
        i++;
    }
    return false;
}

bool RootDrawingItem::isValidHide ()
{
    int i=0;
    while (i < mw->drawing->childCount()) {
        DrawingItem *child=dynamic_cast<DrawingItem *>(mw->drawing->child(i));
        if (child->isValidHide()) return true;
        i++;
    }
    return false;
}

bool RootDrawingItem::isValidSelectAll ()
{
    int i=0;
    while (i < mw->drawing->childCount()) {
        DrawingItem *child=dynamic_cast<DrawingItem *>(mw->drawing->child(i));
        if (!child->isSelected()) return true;
        i++;
    }
    return true;
}

void RootDrawingItem::show ()
{
    mw->ui->drawingWindow->hideItem(mw->drawing);
    mw->drawing->setForeground(0,Qt::black);

    int i=0;
    while (i < mw->drawing->childCount()) {
        DrawingItem *child=dynamic_cast<DrawingItem *>(mw->drawing->child(i));
        child->setForeground(0,Qt::gray);
        mw->ui->drawingWindow->showItem(child);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
}

void RootDrawingItem::hide ()
{
    mw->ui->drawingWindow->hideItem(mw->drawing);
    mw->drawing->setForeground(0,Qt::black);

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(2);
}

void RootDrawingItem::selectAll ()
{
    mw->ui->drawingWindow->hideItem(mw->drawing);
    mw->ui->drawingWindow->unselectItem(mw->drawing);
    mw->ui->drawingItemTree->setCurrentItem(nullptr);

    int i=0;
    while (i < mw->drawing->childCount()) {
        DrawingItem *child=dynamic_cast<DrawingItem *>(mw->drawing->child(i));
        mw->ui->drawingWindow->showItem(child);
        mw->ui->drawingWindow->selectItem(child);
        i++;
    }
}

void RootDrawingItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->selectAllAction=new QAction("Select All");
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, this, &RootDrawingItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &RootDrawingItem::hide);
    connect(mw->selectAllAction, &QAction::triggered, this, &RootDrawingItem::selectAll);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (isValidSelectAll()) menu->addAction(mw->selectAllAction);
    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
}

////////////////////////////////////////////////////////////////////////////////
// DrawingItem
////////////////////////////////////////////////////////////////////////////////

DrawingItem::DrawingItem (OpenParEMg *mw_, BaseItem *parentItem_)
{
    mw=mw_;
    parentItem=parentItem_;
    itemType=0;
    setText(0,"DrawingItem");
    setForeground(0,Qt::black);

    depth=0;
    set_dimTag(-1,-1);          // for mesh items; invalid initialization

    p0set=false;
    p1set=false;
    enableMove=false;
    enableStretch=false;
    enableDeletePoint=false;
    enableInsertPoint=false;

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->set_name(text(0));
    addShapeData(newShapeData);
}

void DrawingItem::promoteChildren ()
{
    long unsigned int i=0;
    while (i < getChildrenSize()) {
        DrawingItem *child=dynamic_cast<DrawingItem *>(getChild(i));
        if (child) {
            int index=indexOfChild(child);
            takeChild(index);
            getParentItem()->addChild(child);
            child->setParentItem(getParentItem());
            child->decrease_depth();
            mw->ui->drawingWindow->showItem(child);
        }
        i++;
    }
}

void DrawingItem::demoteChildren ()
{
    long unsigned int i=0;
    while (i < getChildrenSize()) {
        DrawingItem *child=dynamic_cast<DrawingItem *>(getChild(i));
        if (child) {
            int index=getParentItem()->indexOfChild(child);
            getParentItem()->takeChild(index);
            addChild(child);
            child->setParentItem(this);
            child->copy_depth(this);
            child->increase_depth();
        }
        i++;
    }
}

void DrawingItem::cancelOperation ()
{
    //std::cout << "DrawingItem::cancelOperation" << std::endl; std::cout.flush();

    resetOperation();

    // remove animate shape
    unsetAnimate(mw->ui->drawingWindow->get_viewerContext());

    // remove rubberband
    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        polywire->deleteRubberband();
    }
    //{QMessageBox mb; mb.critical(nullptr, "Debug", "place 1"); mw->ui->drawingWindow->updateViewer();}

    // remove the old version from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        dataStack.undo(); // go back to the prior shape data
        mw->reprocess(this);
    }

    Process *process=static_cast<Process *>(getProcess());
    if (process) {
        long unsigned int i=0;
        while (i < getChildrenSize()) {
            // dataStack.undo(); // Do not go back to the prior shape data - operations work on the children
            getChild(i)->cancelOperation();
            i++;
        }
    }

    if (!polywire && !process) {
        dataStack.undo();  // go back to the prior shape data
        mw->reprocess(this);
    }

    setForeground(0,Qt::gray);
    mw->ui->drawingWindow->showItem(this);
    mw->ui->drawingWindow->activateItem(this);
}

void DrawingItem::startDraw ()
{
    gp_Dir normal=mw->ui->drawingWindow->get_normal();
    mw->activePolywire->setNormal(normal.X(),normal.Y(),normal.Z());
    mw->activePolywire->set_viewerContext(mw->ui->drawingWindow->get_viewerContext());
    mw->activePolywire->setDrawEnable(true);
    mw->activePolywire->setHasArrows(false);

    ShapeData *newShapeData=getShapeData();
    newShapeData->setPolywire(mw->activePolywire);

    mw->restrictToDrawingPlane=true;
    mw->activeAction=true;
    mw->clearTreeSelection();
    mw->ui->drawingWindow->set_pickFirstVertex(true);

    mw->startOperation(true);
    mw->itemChangesStack.startNew();
}

void DrawingItem::startLine ()
{
    mw->activePolywire=new Line();
    if (!mw->activePolywire) return;
    startDraw();
}

void DrawingItem::startPolyline ()
{
    mw->activePolywire=new Polyline();
    if (!mw->activePolywire) return;
    startDraw();
}

void DrawingItem::startRectangle ()
{
    mw->activePolywire=new Rectangle();
    if (!mw->activePolywire) return;
    mw->activePolywire->setU(mw->uLocalAxis);
    startDraw();
}

void DrawingItem::startPolycircle ()
{
    mw->activePolywire=new Polycircle();
    if (!mw->activePolywire) return;
    startDraw();
}

void DrawingItem::finishDraw ()
{
    // add the final shape to the shape data
    ShapeData *shapeData=getShapeData();
    shapeData->setShape(shapeData->getPolywire()->get_AIS_Shape());

    // add to the selection tree
    setText(0,mw->activePolywire->getName(&(mw->objectCounts)));
    shapeData->set_name(text(0));
    getParentItem()->addChild(this);

    // put into tracking, display, and select
    mw->ui->drawingWindow->insertItemToMap(getShape(),this);
    mw->ui->drawingWindow->showItem(this);
    mw->ui->drawingWindow->selectItem(this);

    // add to the stack for undo/redo
    mw->itemChangesStack.add(this);

    // put it on the Z-layer to get it higher selection priority
    getShape()->SetZLayer(Graphic3d_ZLayerId_Top);

    // remove rectangle constraint, if present
    Rectangle *rectangle=dynamic_cast<Rectangle *>(shapeData->getPolywire());
    if (rectangle) rectangle->setIsSquare(false);

    // clicked item tracking
    mw->previousClickedItem=mw->clickedItem;
    mw->clickedItem=this;

    // make sure everything is off
    mw->activePolywire->deleteRubberband();
    mw->ui->drawingWindow->removeSelectOnVertex();

    // reset flags
    mw->activeAction=false;
    mw->restrictToDrawingPlane=false;
    mw->activePolywire=nullptr;

    // mark as changed
    mw->drawingChanged=true;
}

void DrawingItem::cancelDraw ()
{
    std::cout << "DrawingItem::cancelDraw" << std::endl; std::cout.flush();

    // take care of shapes
    if (!animateShape.IsNull()) animateShape.Nullify();
    if (mw->activePolywire) {
        mw->activePolywire->deleteRubberband();
        mw->activePolywire=nullptr;
    }

    cancelOperation();
    mw->ui->drawingWindow->set_gridPlane(mw->currentPrivilegedPlane);

    // remove the current undo/redo item
    mw->itemChangesStack.pop_back();

    mw->activeAction=false;

    mw->finishOperation(false,1);
}

void DrawingItem::startMove (bool isAnimate)
{
    std::cout << "DrawingItem::startMove  isAnimate=" << isAnimate << std::endl; std::cout.flush();

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        setForUndoRedo(true,0);
        resetOperation();
        if (isAnimate) setAnimate(mw->ui->drawingWindow->get_viewerContext());
        setEnableMove(true);
        mw->itemChangesStack.add(this);
    }

    Process *process=static_cast<Process *>(getProcess());
    if (process) {
        if (isAnimate) setAnimate(mw->ui->drawingWindow->get_viewerContext());
        int i=0;
        while (i < childCount()) {
            DrawingItem *processChild=(DrawingItem *)child(i);
            resetOperation();
            //if (isAnimate) setAnimate(mw->ui->drawingWindow->get_viewerContext());
            setEnableMove(true);
            processChild->startMove(false);
            i++;
        }
    }

    if (!polywire && !process) {
        setForUndoRedo(true,0);
        resetOperation();
        if (isAnimate) setAnimate(mw->ui->drawingWindow->get_viewerContext());
        setEnableMove(true);
        mw->itemChangesStack.add(this);
    }
}

void DrawingItem::finishMove (gp_Pnt p0_, gp_Pnt p1_)
{
    //std::cout << "DrawingItem::finishMove" << std::endl; std::cout.flush();

    // QString message="DrawingItem::finishMove  ";
    // message.append(text(0));
    // {QMessageBox mb; mb.critical(nullptr, "Debug", message);}

    //unsetAnimate(mw->ui->drawingWindow->get_viewerContext());

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        polywire->shift(p1_,p0_);
        mw->reprocess(this);
        mw->drawingChanged=true;
    }

    Process *process=static_cast<Process *>(getProcess());
    if (process) {
        int i=0;
        while (i < childCount()) {
            DrawingItem *processChild=(DrawingItem *)child(i);
            processChild->finishMove(p0_,p1_);
            mw->drawingChanged=true;
            i++;
        }

        mw->ui->drawingWindow->activateItem(this);
    }

    if (!polywire && !process) {
        reset_transformation();
        TopoDS_Shape shape=moveShape(p0_,p1_,mw->ui->drawingWindow->get_viewerContext());
        Handle(AIS_Shape) newAISshape=new AIS_Shape(shape);

        ShapeData *shapeData=getShapeData();
        shapeData->setShape(newAISshape);

        // add the new item back to the display and tracking
        mw->ui->drawingWindow->insertItemToMap(getShape(),this);

        mw->reprocess(this);
        mw->drawingChanged=true;
    }

    mw->activeAction=false;

    resetOperation();
    unsetAnimate(mw->ui->drawingWindow->get_viewerContext());
}

void DrawingItem::startRotate ()
{
    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        setForUndoRedo(true,0);
        resetOperation();
        mw->itemChangesStack.add(this);
    }

    Process *process=static_cast<Process *>(getProcess());
    if (process) {
        int i=0;
        while (i < childCount()) {
            DrawingItem *processChild=(DrawingItem *)child(i);
            resetOperation();
            processChild->startRotate();
            //mw->ui->drawingWindow->hideItem(processChild);
            i++;
        }
    }

    if (!polywire && !process) {
        setForUndoRedo(true,0);
        resetOperation();
        mw->itemChangesStack.add(this);
    }
}

void DrawingItem::finishRotate (double angle, gp_Pnt startPoint, gp_Pnt endPoint)
{
    // // remove the old version from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    unsetAnimate(mw->ui->drawingWindow->get_viewerContext());

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        polywire->rotate(angle,startPoint,endPoint);
        mw->reprocess(this);
        mw->drawingChanged=true;
    }

    Process *process=static_cast<Process *>(getProcess());
    if (process) {
        int i=0;
        while (i < childCount()) {
            DrawingItem *processChild=(DrawingItem *)child(i);
            processChild->finishRotate(angle,startPoint,endPoint);
            mw->drawingChanged=true;
            i++;
        }

        mw->ui->drawingWindow->activateItem(this);
    }

    if (!polywire && !process) {
        reset_transformation();
        TopoDS_Shape shape=rotateShape(angle,startPoint,endPoint,mw->ui->drawingWindow->get_viewerContext());
        Handle(AIS_Shape) newAISshape=new AIS_Shape(shape);

        ShapeData *shapeData=getShapeData();
        shapeData->setShape(newAISshape);

        // add the new item back to the display and tracking
        mw->ui->drawingWindow->insertItemToMap(getShape(),this);

        mw->reprocess(this);
        mw->drawingChanged=true;
    }

    mw->activeAction=false;

    resetOperation();

    findTopLevelItem(this);
}

void DrawingItem::startStretch ()
{
    setForUndoRedo(false,0);
    resetOperation();
    setEnableStretch(true);

    // get the drawing plane
    mw->currentPrivilegedPlane=mw->ui->drawingWindow->get_gridPlane();

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        gp_Pln plane=polywire->getPlane();
        mw->ui->drawingWindow->set_gridPlane(plane);
    }

    mw->itemChangesStack.add(this);
}

void DrawingItem::finishStretch ()
{
    // remove the old version from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (!polywire) return;

    finishStretchPoint();
}

void DrawingItem::extrude ()
{
    // Do not set because the DrawingItem itself is not modified
    // setForUndoRedo();

    TopoDS_Shape extrudeShape=getShape()->Shape();
    if (extrudeShape.IsNull()) return;

    // pick off the face to exclude any extra vertices added for selection convenience
    if (extrudeShape.ShapeType() == TopAbs_COMPOUND) {
        TopoDS_Iterator it(extrudeShape);
        for (; it.More(); it.Next()) {
            TopoDS_Shape subShape=it.Value();
            if (subShape.ShapeType() == TopAbs_FACE) {
                extrudeShape=subShape;
                break;
            }
        }
    }

    // extrude
    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {

        // set direction
        polywire->setReverseExtrusionDirection(false);
        if (mw->extrusionDirection.Magnitude() > 1e-12) {
            if (polywire->getNormal().IsOpposite(mw->extrusionDirection,1.5)) {
                polywire->setReverseExtrusionDirection(true);
            }
        }

        // scale it
        gp_Vec scaledVec=gp_Vec(polywire->getNormal())*mw->length;
        if (polywire->getReverseExtrusionDirection()) scaledVec=-scaledVec;

        // extrude it
        BRepPrimAPI_MakePrism aPrism(extrudeShape,scaledVec);
        if (aPrism.IsDone()) {

            Handle(AIS_Shape) newShape=new AIS_Shape(aPrism);

            // define the process
            Extrude *newExtrude=new Extrude();
            newExtrude->set_length(mw->length);

            // add it
            DrawingItem *newItem=new DrawingItem(mw,mw->drawing);
            newItem->setText(0,newExtrude->getName(&(mw->objectCounts)));
            ShapeData *shapeData=newItem->getShapeData();
            shapeData->setProcess(newExtrude);
            shapeData->setShape(newShape);
            shapeData->set_name(newItem->text(0));
            mw->itemChangesStack.add(newItem);

            mw->drawing->addChild(newItem);
            newExtrude=nullptr;

            mw->ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);

            // add the object to the child list for undo/redo
            newItem->push_child(this);
            newItem->demoteChildren();

            // hide/show

            mw->ui->drawingWindow->hideItem(this);
            mw->ui->drawingWindow->unselectItem(this);

            mw->ui->drawingWindow->showItem(newItem);
            mw->ui->drawingWindow->activateItem(newItem);
            mw->ui->drawingWindow->selectItem(newItem);

            mw->previousClickedItem=mw->clickedItem;
            mw->clickedItem=newItem;

            mw->drawingChanged=true;
        }
    }
    resetOperation();
    mw->activeAction=false;
    //mw->finishOperation(false,1);
}

DrawingItem* DrawingItem::copy (BaseItem *parent)
{
    DrawingItem *newItem=copyCreate();
    mw->itemChangesStack.add(newItem);

    ShapeData *shapeData=newItem->getShapeData();
    shapeData->setCreate();

    Polywire *polywire=static_cast<Polywire *>(newItem->getPolywire());
    if (polywire) {
        newItem->getShape()->SetZLayer(Graphic3d_ZLayerId_Top);
    }

    // set for display
    newItem->setForeground(0,Qt::black);
    mw->ui->drawingWindow->activateItem(newItem);
    mw->ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);

    // set parent

    RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(parent);
    if (rootDrawingItem && rootDrawingItem->is_rootDrawing()) {
        rootDrawingItem->addChild(newItem);
        newItem->setParentItem(rootDrawingItem);
        newItem->set_depth(0);
    }

    DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(parent);
    if (drawingItem && drawingItem->is_drawing()) {
        drawingItem->addChild(newItem);
        newItem->setParentItem(drawingItem);
        newItem->copy_depth(drawingItem);
        newItem->increase_depth();
    }

    // children for processes
    Process *process=static_cast<Process *>(newItem->getProcess());
    if (process) {
        int i=0;
        while (i < childCount()) {
            DrawingItem *processChild=(DrawingItem *)child(i);
            if (processChild) {
                DrawingItem *newChild=processChild->copy(newItem);
                newChild->setParentItem(newItem);
                newItem->push_child(newChild);
            }
            i++;
        }
    }

    return newItem;
}

void DrawingItem::startEdit ()
{
    setForUndoRedo(false,0);

    // polywire to edit
    Polywire *polywire=static_cast<Polywire *>(getPolywire());

    // supporting undo/redo
    mw->lineEdit=nullptr;
    mw->rectangleEdit=nullptr;
    mw->polycircleEdit=nullptr;

    Line *line=dynamic_cast<Line *>(polywire);
    if (line) {
        mw->lineEdit=line->copyCreate();
        if (mw->lineEditForm) delete mw->lineEditForm;
        mw->lineEditForm=new LineEditForm();
        mw->lineEditForm->set_conversionFactor(mw->getConversionFactor());
        mw->lineEditForm->set_drawingWindow(mw->ui->drawingWindow);
        mw->lineEditForm->set_polywire(mw->lineEdit);
        mw->lineEditForm->set_relay(mw->relay);
        mw->lineEditForm->setModal(false);
        mw->ui->drawingWindow->hideItem(this);
        connect(mw,&OpenParEMg::sendPnt,mw->lineEditForm,&LineEditForm::pickVertexFinished);
        mw->lineEditForm->show();
    }

    Rectangle *rectangle=dynamic_cast<Rectangle *>(polywire);
    if (rectangle) {
        mw->rectangleEdit=rectangle->copyCreate();
        if (mw->rectangleEditForm) delete mw->rectangleEditForm;
        mw->rectangleEditForm=new RectangleEditForm();
        mw->rectangleEditForm->set_conversionFactor(mw->getConversionFactor());
        mw->rectangleEditForm->set_drawingWindow(mw->ui->drawingWindow);
        mw->rectangleEditForm->set_polywire(mw->rectangleEdit);
        mw->rectangleEditForm->set_relay(mw->relay);
        mw->rectangleEditForm->setModal(false);
        mw->ui->drawingWindow->hideItem(this);
        connect(mw,&OpenParEMg::sendPnt,mw->rectangleEditForm,&RectangleEditForm::pickVertexFinished);
        mw->rectangleEditForm->show();
    }

    Polycircle *polycircle=dynamic_cast<Polycircle *>(polywire);
    if (polycircle) {
        mw->polycircleEdit=polycircle->copyCreate();
        if (mw->polycircleEditForm) delete mw->polycircleEditForm;
        mw->polycircleEditForm=new PolycircleEditForm();
        mw->polycircleEditForm->set_conversionFactor(mw->getConversionFactor());
        mw->polycircleEditForm->set_drawingWindow(mw->ui->drawingWindow);
        mw->polycircleEditForm->set_Polycircle(mw->polycircleEdit);
        mw->polycircleEditForm->set_relay(mw->relay);
        mw->polycircleEditForm->setModal(false);
        mw->ui->drawingWindow->hideItem(this);
        connect(mw,&OpenParEMg::sendPnt,mw->polycircleEditForm,&PolycircleEditForm::pickVertexFinished);
        mw->polycircleEditForm->show();
    }

    Process *process=static_cast<Process *>(getProcess());
    if (process) {
        Extrude *extrude=dynamic_cast<Extrude *>(process);
        if (extrude) {
            Polywire *polywire=nullptr;
            int i=0;
            while (i < childCount()) {
                DrawingItem *processChild=(DrawingItem *)child(i);
                polywire=static_cast<Polywire *>(processChild->getPolywire());
                if (polywire) break;
                i++;
            }

            if (polywire) {
                if (mw->lengthEditForm) delete mw->lengthEditForm;
                mw->lengthEditForm=new LengthInputForm();
                mw->lengthEditForm->set_conversionFactor(mw->getConversionFactor());
                mw->length=extrude->get_length();
                mw->lengthEditForm->set_length(&(mw->length));
                mw->lengthEditForm->set_extrusionDirection(&(mw->extrusionDirection));
                mw->lengthEditForm->set_drawingWindow(mw->ui->drawingWindow);
                mw->lengthEditForm->set_relay(mw->relay);
                mw->lengthEditForm->setModal(false);
                mw->lengthEditForm->show();
            }
        }
    }

    mw->itemChangesStack.add(this);
}

void DrawingItem::finishEdit ()
{
    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        Line *line=dynamic_cast<Line *>(polywire);
        Rectangle *rectangle=dynamic_cast<Rectangle *>(polywire);
        Polycircle *polycircle=dynamic_cast<Polycircle *>(polywire);
        if (line) setPolywire(mw->lineEdit);
        else if (rectangle) setPolywire(mw->rectangleEdit);
        else if (polycircle) setPolywire(mw->polycircleEdit);

        mw->reprocess(this);
    }

    Process *process=static_cast<Process *>(getProcess());
    if (process) {
        Extrude *extrude=dynamic_cast<Extrude *>(process);
        if (extrude) extrude->set_length(mw->length);

        int i=0;
        while (i < getChildrenSize()) {
            DrawingItem *processChild=(DrawingItem *)getChild(i);
            processChild->finishEdit();
            i++;
        }
    }

    if (!polywire && !process) {
        mw->reprocess(this);
    }

    mw->activeAction=false;

    findTopLevelItem(this);
}

void DrawingItem::startDeletePoint ()
{
    setForUndoRedo(false,0);

    Handle(AIS_Shape) shape=getShape();
    if (!shape.IsNull()) {
        // set the selected shape to be the only selectable shape
        // includes selecting just on vertices of the shape and not midpoints
        //ui->drawingWindow->set_activeShape(shape);

        // set the drawing plane
        mw->currentPrivilegedPlane=mw->ui->drawingWindow->get_gridPlane();
        //restrictToDrawingPlane=true;

        Polywire *polywire=static_cast<Polywire *>(getPolywire());
        if (polywire) {
            resetOperation();
            setEnableDeletePoint(true);
            gp_Pln plane=polywire->getPlane();
            mw->ui->drawingWindow->set_gridPlane(plane);
            mw->itemChangesStack.add(this);
        }
    }
}

void DrawingItem::finishDeletePoint ()
{
    // remove the old version from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        gp_Pnt p0=getP0();
        polywire->deletePoint(p0);
        mw->reprocess(this);
        mw->drawingChanged=true;
    }

    Process *process=static_cast<Process *>(getProcess());
    if (process) {
        int i=0;
        while (i < getChildrenSize()) {
            DrawingItem *processChild=(DrawingItem *)getChild(i);
            processChild->finishDeletePoint();
            i++;
        }
    }

    if (!polywire && !process) {
        mw->reprocess(this);
        mw->drawingChanged=true;
    }

    resetOperation();
    mw->activeAction=false;
    findTopLevelItem(this);
    mw->finishOperation(false,1);
}

void DrawingItem::cancelDeletePoint ()
{
    setEnableDeletePoint(false);
}

void DrawingItem::startInsertPoint ()
{
    setForUndoRedo(false,0);

    Handle(AIS_Shape) shape=getShape();
    if (!shape.IsNull()) {
        // set the selected shape to be the only selectable shape
        // includes selecting just on vertices of the shape and not midpoints
        //ui->drawingWindow->set_activeShape(shape);

        // set the drawing plane
        mw->currentPrivilegedPlane=mw->ui->drawingWindow->get_gridPlane();
        //restrictToDrawingPlane=true;

        Polywire *polywire=static_cast<Polywire *>(getPolywire());
        if (polywire) {
            resetOperation();
            setEnableInsertPoint(true);
            gp_Pln plane=polywire->getPlane();
            mw->ui->drawingWindow->set_gridPlane(plane);
            mw->itemChangesStack.add(this);
        }
    }
}

void DrawingItem::finishInsertPoint ()
{
    // remove the old version from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {

        // insert
        gp_Pnt p0=getP0();
        polywire->insertPoint(p0);

        // stretch
        mw->startOperation(true);  // re-run with mid-point selection
        polywire->setEditIndex(p0);
        polywire->setCurrentMousePosition(p0);
        mw->ui->drawingWindow->set_pickSecondVertex(true);

        // finishStretchPoint completes the operation
    }
}

void DrawingItem::finishStretchPoint ()
{
    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        polywire->deleteRubberband();
        gp_Pnt pnt=getP1();
        polywire->setEditPoint(pnt);

        Rectangle *rectangle=dynamic_cast<Rectangle *>(polywire);
        if (rectangle) {
            if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
                rectangle->setIsSquare(true);
            } else {
                rectangle->setIsSquare(false);
            }
        }

        mw->reprocess(this);
        mw->drawingChanged=true;
    }

    Process *process=static_cast<Process *>(getProcess());
    if (process) {
        int i=0;
        while (i < getChildrenSize()) {
            DrawingItem *processChild=(DrawingItem *)getChild(i);
            processChild->finishStretchPoint();
            i++;
        }
    }

    if (!polywire && !process) {
        mw->reprocess(this);
        mw->drawingChanged=true;
    }

    resetOperation();
    mw->activeAction=false;
    findTopLevelItem(this);
    mw->finishOperation(false,1);
}

void DrawingItem::cancelInsertPoint ()
{
    setEnableInsertPoint(false);
}

void DrawingItem::convertToPolyline ()
{
    setForUndoRedo(false,0);

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {

        // remove the old version from display and tracking
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());

        // convert
        Polyline *newPolyline=polywire->convert();
        setPolywire(newPolyline);
        mw->reprocess(this);
        getShape()->SetZLayer(Graphic3d_ZLayerId_Top);
        mw->ui->drawingWindow->activateSelectItem(this);
        mw->itemChangesStack.add(this);
        mw->drawingChanged=true;

        findTopLevelItem(this);
    }
}

void DrawingItem::del ()
{
    // remove from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    // mark as delete
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setDelete();
    addShapeData(newShapeData);

    // parentItem
    BaseItem *parentItem=getParentItem();
    if (parentItem) {
        RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(parentItem);
        if (rootDrawingItem && rootDrawingItem->is_rootDrawing()) {
            // int insertIndex=rootDrawingItem->indexOfChild(this);

            // // move children to parent
            // while (childCount() > 0) {
            //     DrawingItem* drawingChild=dynamic_cast<DrawingItem *>(takeChild(0));
            //     rootDrawingItem->insertChild(insertIndex++,drawingChild);
            //     drawingChild->setParentItem(rootDrawingItem);
            //     drawingChild->decrease_depth();
            //     mw->ui->drawingWindow->showItem(drawingChild);

            //     // set the materials
            //     if (!text(1).isNull()) {
            //         if (!drawingChild->getPolywire()) {
            //             drawingChild->setText(1,text(1));
            //         }
            //     }
            // }

            rootDrawingItem->removeChild(this);
        }

        mw->itemChangesStack.add(this);

        // reset the top-level compound
        mw->reprocess(mw->drawing);

        mw->drawingChanged=true;
    }
}

DrawingItem* DrawingItem::copyCreate ()
{
    //std::cout << "DrawingItem::copyCreate" << std::endl; std::cout.flush();

    DrawingItem *newItem=new DrawingItem(mw,parentItem);
    if (!newItem) return nullptr;

    // copy just the current data

    ShapeData *shapeData=newItem->getShapeData();
    shapeData->copy(getShapeData());
    newItem->setText(0,this->text(0).append("_copy"));
    shapeData->set_name(newItem->text(0));
    newItem->aTrsf=aTrsf;
    newItem->dimTag=dimTag;
    newItem->itemType=itemType;
    newItem->depth=depth;

    return newItem;
}

bool DrawingItem::isValidShow ()
{
    if (foreground(0) == Qt::gray) return true;
    return false;
}

bool DrawingItem::isValidHide ()
{
    if (foreground(0) == Qt::black) return true;
    return false;
}

void DrawingItem::show ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem) mw->ui->drawingWindow->showItem(baseItem);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(30);
}

void DrawingItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem) mw->ui->drawingWindow->hideItem(baseItem);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(34);
}

void DrawingItem::showMenu (QMenu *menu)
{
    mw->assignMaterialAction=new QAction("Assign Material");
    mw->showAction=new QAction("Show");
    mw->hideAction=new QAction("Hide");
    mw->editAction=new QAction("Edit");
    mw->moveAction=new QAction("Move");
    mw->stretchAction=new QAction("Stretch");
    mw->deletePointAction=new QAction("Delete Point");
    mw->insertPointAction=new QAction("Insert Point");
    mw->closePolylineAction=new QAction("Close Polyline");
    mw->openPolylineAction=new QAction("Open Polyline");
    mw->convertToPolylineAction=new QAction("Convert to Polyline");
    mw->convertToPathAction=new QAction("Convert to Path");
    mw->rotateAction=new QAction("Rotate");
    mw->unselectAction=new QAction("Unselect");
    mw->copyAction=new QAction("Copy");
    mw->renameAction=new QAction("Rename",this);
    mw->deleteAction=new QAction("Delete");
    mw->setPlaneAction=new QAction("Set Drawing Plane");
    mw->setPlaneAxisAction=new QAction("Set Drawing Plane with Axis");
    mw->createPathAction=new QAction("Create Path");
    mw->createPathAction->setToolTip("Create a path using the selected face.");
    mw->createPortAction=new QAction("Create Port");
    mw->createPortAction->setToolTip("Create a path and port using the selected face.");
    mw->createBoundaryAction=new QAction("Create Boundary");
    mw->createBoundaryAction->setToolTip("Create a path and boundary using the selected face.");
    mw->extrudeAction=new QAction("Extrude");
    mw->extrudeAction->setToolTip("Extrude the selected polywires along each normal to create solid objects.");
    mw->mergeAction=new QAction("Merge");
    mw->mergeAction->setToolTip("Merge two solid objects.");
    mw->subtractAction=new QAction("Subtract");
    mw->subtractAction->setToolTip("Subtract the second selected solid object from the first selected solid object.");
    mw->convertToPortAction=new QAction("Convert to Port");
    mw->convertToPortAction->setToolTip("Convert the selected polywires to ports with matching paths.");
    mw->convertToBoundaryAction=new QAction("Convert to Boundary");
    mw->convertToBoundaryAction->setToolTip("Convert the selected polywires to boundaries with matching paths.");
    mw->cancelAction=new QAction("Cancel");

    connect(mw->assignMaterialAction, &QAction::triggered, mw, &OpenParEMg::assignMaterial);
    connect(mw->showAction, &QAction::triggered, this, &DrawingItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &DrawingItem::hide);
    connect(mw->editAction, &QAction::triggered, mw, &OpenParEMg::editObject);
    connect(mw->moveAction, &QAction::triggered, mw, &OpenParEMg::moveObject);
    connect(mw->stretchAction, &QAction::triggered, mw, &OpenParEMg::stretchObject);
    connect(mw->deletePointAction, &QAction::triggered, mw, &OpenParEMg::deletePoint);
    connect(mw->insertPointAction, &QAction::triggered, mw, &OpenParEMg::insertPoint);
    connect(mw->closePolylineAction, &QAction::triggered, mw, &OpenParEMg::closeExistingPolyline);
    connect(mw->openPolylineAction, &QAction::triggered, mw, &OpenParEMg::openExistingPolyline);
    connect(mw->convertToPolylineAction, &QAction::triggered, mw, &OpenParEMg::convertToPolyline);
    connect(mw->convertToPathAction, &QAction::triggered, mw, &OpenParEMg::convertDrawingToPath);
    connect(mw->rotateAction, &QAction::triggered, mw, &OpenParEMg::rotateObject);
    connect(mw->unselectAction, &QAction::triggered, mw, &OpenParEMg::unselectDrawingItems);
    connect(mw->renameAction, &QAction::triggered, mw, &OpenParEMg::renameDrawingItems);
    connect(mw->deleteAction, &QAction::triggered, mw, &OpenParEMg::deleteDrawingItems);
    connect(mw->copyAction, &QAction::triggered, mw, &OpenParEMg::copyDrawingItems);
    connect(mw->setPlaneAction, &QAction::triggered, mw, &OpenParEMg::setPlaneToFace);
    connect(mw->setPlaneAxisAction, &QAction::triggered, mw, &OpenParEMg::setPlaneToFaceAxis);
    connect(mw->createPathAction, &QAction::triggered, mw, &OpenParEMg::createPath);
    connect(mw->createPortAction, &QAction::triggered, mw, &OpenParEMg::createPortFromFace);
    connect(mw->createBoundaryAction, &QAction::triggered, mw, &OpenParEMg::createBoundaryFromFace);
    connect(mw->extrudeAction, &QAction::triggered, mw, &OpenParEMg::extrudePolywire);
    connect(mw->mergeAction, &QAction::triggered, mw, &OpenParEMg::mergeSolids);
    connect(mw->subtractAction, &QAction::triggered, mw, &OpenParEMg::subtractSolids);
    connect(mw->convertToPortAction, &QAction::triggered, mw, &OpenParEMg::convertDrawingToPort);
    connect(mw->convertToBoundaryAction, &QAction::triggered, mw, &OpenParEMg::convertDrawingToBoundary);
    connect(mw->cancelAction, &QAction::triggered, mw, &OpenParEMg::cancelMenu);

    if (mw->isValidAssignMaterial()) menu->addAction(mw->assignMaterialAction);
    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (mw->isValidCopy()) menu->addAction(mw->copyAction);
    if (mw->isValidRenameDrawingItems()) menu->addAction(mw->renameAction);
    if (mw->isValidObjectDelete()) menu->addAction(mw->deleteAction);
    if (mw->isValidSetPlane()) menu->addAction(mw->setPlaneAction);
    if (mw->isValidSetPlane()) menu->addAction(mw->setPlaneAxisAction);
    if (mw->isValidCreatePath()) menu->addAction(mw->createPathAction);
    if (mw->isValidCreatePortFromFace()) menu->addAction(mw->createPortAction);
    if (mw->isValidCreateBoundaryFromFace()) menu->addAction(mw->createBoundaryAction);
    if (mw->isValidObjectEdit()) menu->addAction(mw->editAction);
    if (mw->isValidObjectMove()) menu->addAction(mw->moveAction);
    if (mw->isValidObjectStretch()) menu->addAction(mw->stretchAction);
    if (mw->isValidInsertPoint()) menu->addAction(mw->insertPointAction);
    if (mw->isValidDeletePoint()) menu->addAction(mw->deletePointAction);
    if (mw->isValidCloseExistingPolyline()) menu->addAction(mw->closePolylineAction);
    if (mw->isValidOpenExistingPolyline()) menu->addAction(mw->openPolylineAction);
    if (mw->isValidRotateObject()) menu->addAction(mw->rotateAction);
    if (mw->isValidExtrudePolywire()) menu->addAction(mw->extrudeAction);
    if (mw->isValidMergeSolids()) menu->addAction(mw->mergeAction);
    if (mw->isValidSubtractSolids()) menu->addAction(mw->subtractAction);
    if (mw->isValidConvertToPolyline()) menu->addAction(mw->convertToPolylineAction);
    if (mw->isValidConvertToPath()) menu->addAction(mw->convertToPathAction);
    if (mw->isValidConvertToPort()) menu->addAction(mw->convertToPortAction);
    if (mw->isValidConvertToBoundary()) menu->addAction(mw->convertToBoundaryAction);
    menu->addAction(mw->cancelAction);
}

TopoDS_Shape DrawingItem::moveShape (gp_Pnt p1, gp_Pnt p2, Handle(AIS_InteractiveContext) viewerContext)
{
    Handle(AIS_Shape) shape=getShape();

    gp_Trsf step;
    step.SetTranslation(p1,p2);
    aTrsf=step*aTrsf;
    shape->SetLocalTransformation(aTrsf);

    viewerContext->Redisplay(shape,Standard_False);  // Standard_True

    BRepBuilderAPI_Transform transformer(shape->Shape(),aTrsf,Standard_False);  // Standard_True
    return transformer.Shape();
}

void DrawingItem::setAnimate (Handle(AIS_InteractiveContext) viewerContext)
{
    Handle(AIS_Shape) shape=dataStack.getShapeData()->getShape();
    if (shape.IsNull()) return;

    if (!animateShape.IsNull()) {
        viewerContext->Remove(animateShape,Standard_True);
        animateShape.Nullify();
    }

    animateShape=new AIS_Shape(shape->Shape());
    viewerContext->Display(animateShape,AIS_WireFrame,-1,Standard_True);  // non-selectable
}

void DrawingItem::unsetAnimate (Handle(AIS_InteractiveContext) viewerContext)
{
    //std::cout << "unsetAnimate" << std::endl; std::cout.flush();
    if (animateShape.IsNull()) return;
    viewerContext->Remove(animateShape,Standard_False);  //xxx Standard_True
    animateShape.Nullify();
}

void DrawingItem::moveAnimateShape (gp_Pnt p1, gp_Pnt p2, Handle(AIS_InteractiveContext) viewerContext)
{
    if (animateShape.IsNull()) return;

    gp_Trsf step;
    step.SetTranslation(p1,p2);
    aTrsf=step*aTrsf;
    animateShape->SetLocalTransformation(aTrsf);

    viewerContext->Redisplay(animateShape,Standard_True);
}

PathItem* DrawingItem::createPath (bool hasArrows)
{
    std::cout << "DrawingItem::createPath" << std::endl; std::cout.flush();

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (!polywire) return nullptr;

    // default path name
    QString pathName=text(0);
    mw->uniqueifyPathName(pathName);

    // path name placed in a keywordPair
    keywordPair *kwPathName=new keywordPair();
    kwPathName->set_keyword("path");
    kwPathName->set_value(pathName.toStdString());
    kwPathName->set_lineNumber(0);
    kwPathName->set_loaded(true);

    // new path for the path database
    Path *newPath=new Path(0,0);
    newPath->set_name(pathName.toStdString());
    newPath->is_modified();
    newPath->set_normal(polywire->getNormal());
    newPath->addWirePoints(polywire->buildWire());

    // create a path item
    PathItem *newPathItem=new PathItem(mw,mw->path);
    if (newPathItem) {
        ShapeData *newShapeData=newPathItem->getShapeData()->copyCreate();
        newShapeData->setPolywire(polywire->copyCreate());
        newShapeData->getPolywire()->setHasArrows(hasArrows);
        newShapeData->setShape(newShapeData->getPolywire()->get_AIS_Shape());
        newPathItem->setText(0,pathName);
        newShapeData->set_name(newPathItem->text(0));
        newPathItem->setPath(newPath);
        newPathItem->addShapeData(newShapeData);

        mw->path->addChild(newPathItem);
        mw->itemChangesStack.add(newPathItem);

        // show the new PathItem
        mw->ui->drawingWindow->displayShape(newPathItem->getShape());
        mw->ui->drawingWindow->insertItemToMap(newPathItem->getShape(),newPathItem);
        newPathItem->setForeground(0,Qt::gray);
        mw->ui->drawingWindow->showItem(newPathItem);
        mw->ui->drawingWindow->selectItem(newPathItem);
    }

    return newPathItem;
}

BaseItem* DrawingItem::findTopLevelItem (BaseItem *baseItem)
{
    return baseItem->findTopLevelItem(mw->drawing,baseItem);
}

void DrawingItem::undo ()
{
    std::cout << "DrawingItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // nothing to do
    } else if (shapeData->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();
        // remove the item
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());
        getParentItem()->removeChild(this);

        promoteChildren();

        int i=0;
        while (i < getChildrenSize()) {
            BaseItem *child=getChild(i);
            if (child) {
                child->expandToItem();
            }
            i++;
        }

        dataStack.undo();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());

        dataStack.undo();

        ShapeData *shapeData=getShapeData();
        if (shapeData) {
            Process *process=static_cast<Process *>(shapeData->getProcess());
            if (process) {
                int i=0;
                while (i < childCount()) {
                    BaseItem *childItem=dynamic_cast<BaseItem *>(child(i));
                    childItem->undo();
                    mw->ui->drawingWindow->hideItem(childItem);
                    i++;
                }
            } else {
                mw->reprocess(this);
                mw->ui->drawingWindow->unselectItem(this);
            }
        }

        BaseItem *baseItem=findTopLevelItem(this);
        baseItem->expandToItem();
    } else if (shapeData->isDelete()) {
        // std::cout << "   isDelete" << std::endl; std::cout.flush();
        // DrawingItem *parentItem=dynamic_cast<DrawingItem *>(getParentItem());
        // copy_depth(parentItem);
        // increase_depth();
        // getParentItem()->addChild(this);

        // demoteChildren();

        // dataStack.undo();

        // mw->reprocess(this);
        // BaseItem *baseItem=findTopLevelItem(this);
        // baseItem->expandToItem();
        BaseItem::undo();
    } else if (shapeData->isChangeName()) {
        BaseItem::undo();
    }
}

void DrawingItem::redo ()
{
    std::cout << "DrawingItem::redo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    ShapeData *next=shapeData->getNext();
    if (!next) return;

    if (next->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // should not occur
    } else if (next->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();
        dataStack.redo();

        DrawingItem *parentItem=dynamic_cast<DrawingItem *>(getParentItem());
        copy_depth(parentItem);

        increase_depth();
        getParentItem()->addChild(this);

        long unsigned int i=0;
        while (i < getChildrenSize()) {
            DrawingItem *childItem=dynamic_cast<DrawingItem *>(getChild(i));
            if (childItem) {
                int index=getParentItem()->indexOfChild(childItem);
                if (index >= 0) {
                    getParentItem()->takeChild(index);
                    addChild(childItem);
                    childItem->setParentItem(this);
                    childItem->copy_depth(this);
                    childItem->increase_depth();
                } else {
                    mw->ui->drawingWindow->insertItemToMap(childItem->getShape(),childItem);
                    mw->ui->drawingWindow->displayShape(childItem->getShape());
                    mw->ui->drawingWindow->activateItem(childItem);
                    addChild(childItem);
                    childItem->setParentItem(this);
                    childItem->copy_depth(this);
                    childItem->increase_depth();
                    mw->reprocess(childItem);
                }
            }
            i++;
        }

        mw->reprocess(this);
        BaseItem *baseItem=findTopLevelItem(this);
        baseItem->expandToItem();
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());

        dataStack.redo();

        ShapeData *shapeData=getShapeData();
        if (shapeData) {
            Process *process=static_cast<Process *>(shapeData->getProcess());
            if (process) {
                int i=0;
                while (i < childCount()) {
                    BaseItem *childItem=dynamic_cast<BaseItem *>(child(i));
                    childItem->redo();
                    i++;
                }
            }
        }

        mw->reprocess(this);
        mw->insertToMapActivateItem(this);
        BaseItem *baseItem=findTopLevelItem(this);
        baseItem->expandToItem();
    } else if (next->isDelete()) {
        // std::cout << "   isDelete" << std::endl; std::cout.flush();
        // // remove the item
        // mw->ui->drawingWindow->hideItem(this);
        // mw->ui->drawingWindow->removeItemFromMap(this);
        // mw->ui->drawingWindow->deleteShape(getShape());
        // getParentItem()->removeChild(this);

        // promoteChildren();

        // int i=0;
        // while (i < getChildrenSize()) {
        //     BaseItem *child=getChild(i);
        //     if (child) {
        //         child->expandToItem();
        //     }
        //     i++;
        // }
        BaseItem::redo();
    } else if (next->isChangeName()) {
        BaseItem::redo();
    }
}

////////////////////////////////////////////////////////////////////////////////
// RootPathItem
////////////////////////////////////////////////////////////////////////////////

bool RootPathItem::isValidShow ()
{
    int i=0;
    while (i < mw->path->childCount()) {
        PathItem *child=dynamic_cast<PathItem *>(mw->path->child(i));
        if (child && child->foreground(0) == Qt::gray) return true;
        i++;
    }
    return false;
}

bool RootPathItem::isValidHide ()
{
    int i=0;
    while (i < mw->path->childCount()) {
        PathItem *child=dynamic_cast<PathItem *>(mw->path->child(i));
        if (child && child->foreground(0) == Qt::black) return true;
        i++;
    }
    return false;
}

void RootPathItem::show ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        RootPathItem *rootPathItem=dynamic_cast<RootPathItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (rootPathItem && is_rootPath()) {
            mw->ui->drawingWindow->showItem(rootPathItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(5);
}

void RootPathItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        RootPathItem *rootPathItem=dynamic_cast<RootPathItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (rootPathItem && rootPathItem->is_rootPath()) {
            mw->ui->drawingWindow->hideItem(rootPathItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(6);
}

void RootPathItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, this, &RootPathItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &RootPathItem::hide);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
}

////////////////////////////////////////////////////////////////////////////////
// PathItem
////////////////////////////////////////////////////////////////////////////////

PathItem::PathItem (OpenParEMg *mw_, BaseItem *parentItem_)
{
    mw=mw_;
    parentItem=parentItem_;
    itemType=4;
    setText(0,"PathItem");
    setForeground(0,Qt::black);
    path=nullptr;
    hasArrows=true;

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->set_name(text(0));
    addShapeData(newShapeData);
}

bool PathItem::isValidShow ()
{
    if (foreground(0) == Qt::gray) return true;
    return false;
}

bool PathItem::isValidHide ()
{
    if (foreground(0) == Qt::black) return true;
    return false;
}

void PathItem::show ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem) mw->ui->drawingWindow->showItem(baseItem);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(30);
}

void PathItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem) mw->ui->drawingWindow->hideItem(baseItem);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(34);
}

void PathItem::showMenu (QMenu *menu)
{
    mw->createPortAction=new QAction("Create Port");
    mw->createPortAction->setToolTip("Create a port from the path.");
    mw->createBoundaryAction=new QAction("Create Boundary");
    mw->createBoundaryAction->setToolTip("Create a bounary from the path.");
    mw->reversePathAction=new QAction("Reverse Direction");
    mw->reversePathAction->setToolTip("Reverse the direction of the path.");
    mw->renameAction=new QAction("Rename",this);
    mw->deleteAction=new QAction("Delete",this);
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->cancelAction=new QAction("Cancel");

    connect(mw->createPortAction, &QAction::triggered, mw, &OpenParEMg::createPortFromPath);
    connect(mw->createBoundaryAction, &QAction::triggered, mw, &OpenParEMg::createBoundaryFromPath);
    connect(mw->reversePathAction, &QAction::triggered, mw, &OpenParEMg::reversePathItems);
    connect(mw->renameAction, &QAction::triggered, mw, &OpenParEMg::renamePathItems);
    connect(mw->deleteAction, &QAction::triggered, mw, &OpenParEMg::deletePathItems);
    connect(mw->showAction, &QAction::triggered, this, &PathItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &PathItem::hide);
    connect(mw->cancelAction, &QAction::triggered, mw, &OpenParEMg::cancelMenu);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (mw->ui->drawingWindow->get_pathSelectedCount() == 1) menu->addAction(mw->renameAction);
    if (mw->isValidCreatePortFromPath()) menu->addAction(mw->createPortAction);
    if (mw->isValidCreateBoundaryFromPath()) menu->addAction(mw->createBoundaryAction);
    if (mw->isValidReversePath()) menu->addAction(mw->reversePathAction);
    if (mw->isValidDeletePath()) menu->addAction(mw->deleteAction);
    menu->addAction(mw->cancelAction);
}

void PathItem::del ()
{
    // remove from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    // mark as delete
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setDelete();
    addShapeData(newShapeData);

    // parentItem
    BaseItem *parentItem=getParentItem();
    if (parentItem) {
        RootPathItem *rootPathItem=dynamic_cast<RootPathItem *>(parentItem);
        if (rootPathItem && rootPathItem->is_rootPath()) {
            int insertIndex=rootPathItem->indexOfChild(this);
            rootPathItem->removeChild(this);
        }

        mw->itemChangesStack.add(this);
        mw->drawingChanged=true;
    }
}

BaseItem* PathItem::findTopLevelItem (BaseItem *baseItem)
{
    return baseItem->findTopLevelItem(mw->path,baseItem);
}

void PathItem::undo ()
{
    std::cout << "PathItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isCreate()) {

        Path *path=static_cast<Path *>(getPath());
        if (path) path->setIsUsed(false);

        // remove linked items
        long unsigned int i=0;
        while (i < linkedItems_size()) {
            BaseItem *linkedItem=get_linkedItem(i);
            if (linkedItem) {
                BaseItem *parentItem=linkedItem->getParentItem();
                if (parentItem) {
                    parentItem->removeChild(linkedItem);
                }
            }
            i++;
        }
    } else if (shapeData->isReversePath()) {
        std::cout << "   isReversePath" << std::endl; std::cout.flush();
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());

        dataStack.undo();

        // reverse the path
        Path *path=static_cast<Path *>(getPath());
        if (path) {
            // reverse the path in the path database
            path->reverseOrder();

            // update the shape
            setShape(getShapeData()->getPolywire()->get_AIS_Shape());
        }

        mw->ui->drawingWindow->insertItemToMap(getShape(),this);
        mw->ui->drawingWindow->displayShape(getShape());
        mw->ui->drawingWindow->activateItem(this);
    }

    BaseItem::undo();
}

void PathItem::showArrows (bool show)
{
    //std::cout << "PathItem::showArrows  show=" << show << std::endl; std::cout.flush();

    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    ShapeData *shapeData=this->getShapeData();
    Polywire *polywire=getPolywire();
    if (polywire) {
        polywire->setHasArrows(show);
        shapeData->setShape(polywire->get_AIS_Shape());
    }

    mw->ui->drawingWindow->displayShape(getShape());
    mw->ui->drawingWindow->insertItemToMap(getShape(),this);
    mw->ui->drawingWindow->showItem(this);
    mw->ui->drawingWindow->activateItem(this);

    hasArrows=show;
}

void PathItem::rename (QString name)
{
    BaseItem::rename(name);

    // change the names of the linked items
    long unsigned int i=0;
    while (i < linkedItems_size()) {
        BaseItem *baseItem=get_linkedItem(i);
        if (baseItem->is_integrationPathSegment()) {
            ShapeData *newShapeData=baseItem->getShapeData()->copyCreate();
            newShapeData->setChangeName();

            // new name, preserving the sign
            QChar direction=newShapeData->get_name().front();
            QString integrationText=direction;
            integrationText.append(name);
            baseItem->setText(0,integrationText);
            newShapeData->set_name(integrationText);

            baseItem->addShapeData(newShapeData);

            mw->itemChangesStack.add(baseItem);
        }
        i++;
    }
}

void PathItem::redo ()
{
    std::cout << "PathItem::redo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    ShapeData *next=shapeData->getNext();
    if (!next) return;

    if (next->isReversePath()) {
        std::cout << "   isReversePath" << std::endl; std::cout.flush();
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());

        dataStack.redo();

        // reverse the path
        Path *path=static_cast<Path *>(getPath());
        if (path) {
            // reverse the path in the path database
            path->reverseOrder();

            // update the shape
            setShape(getShapeData()->getPolywire()->get_AIS_Shape());
        }

        mw->ui->drawingWindow->insertItemToMap(getShape(),this);
        mw->ui->drawingWindow->displayShape(getShape());
        mw->ui->drawingWindow->activateItem(this);
        expandToItem();
    }

    BaseItem::redo();
}

void PathItem::reverse ()
{
    // mark
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setReversePath();
    addShapeData(newShapeData);

    // remove the old version from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    // reverse the path
    Path *path=static_cast<Path *>(getPath());
    if (path) {

        // reverse the path in the path database
        path->reverseOrder();

        // reverse the direction of the shape
        getShapeData()->getPolywire()->reverseOrder();

        // update the shape
        setShape(getShapeData()->getPolywire()->get_AIS_Shape());

        mw->ui->drawingWindow->insertItemToMap(getShape(),this);
        mw->ui->drawingWindow->displayShape(getShape());
        mw->ui->drawingWindow->activateItem(this);

        mw->projectChanged=true;

        // add to the stack for undo/redo
        mw->itemChangesStack.add(this);
    }
}

////////////////////////////////////////////////////////////////////////////////
// IntegrationPathItem
////////////////////////////////////////////////////////////////////////////////

IntegrationPathItem::IntegrationPathItem (OpenParEMg *mw_, BaseItem *parentItem_, PathItem *pathItem_)
{
    mw=mw_;
    parentItem=parentItem_;
    itemType=14;
    setText(0,"IntegrationPathItem");
    setForeground(0,Qt::black);

    pathItem=pathItem_;

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->set_name(text(0));
    addShapeData(newShapeData);
}

bool IntegrationPathItem::isValidShow ()
{
    if (foreground(0) == Qt::gray) return true;
    return false;
}

bool IntegrationPathItem::isValidHide ()
{
    if (foreground(0) == Qt::black) return true;
    return false;
}

void IntegrationPathItem::show ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem) mw->ui->drawingWindow->showItem(baseItem);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(30);
}

void IntegrationPathItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem) mw->ui->drawingWindow->hideItem(baseItem);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(34);
}

void IntegrationPathItem::showMenu (QMenu *menu)
{
    mw->renameAction=new QAction("Flip Sign",this);
    mw->deleteAction=new QAction("Delete",this);
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);

    connect(mw->renameAction, &QAction::triggered, this, &IntegrationPathItem::flipSign);
    connect(mw->deleteAction, &QAction::triggered, mw, &OpenParEMg::deleteIntegrationPathItems);
    connect(mw->showAction, &QAction::triggered, this, &IntegrationPathItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &IntegrationPathItem::hide);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    menu->addAction(mw->renameAction);
    menu->addAction(mw->deleteAction);
}

void IntegrationPathItem::undo ()
{
    std::cout << "IntegrationPathItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isCreate()) {
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
        }
    } else if (shapeData->isDelete()) {
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->push_linkedItem(this);
        }
    }

    BaseItem::undo();
}

void IntegrationPathItem::redo ()
{
    std::cout << "IntegrationPathItem::redo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    ShapeData *next=shapeData->getNext();
    if (!next) return;

    if (next->isCreate()) {
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->push_linkedItem(this);
        }
    } else if (next->isDelete()) {
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
        }
    }

    BaseItem::redo();
}

void IntegrationPathItem::del ()
{
    std::cout << "IntegrationPathItem::del" << std::endl; std::cout.flush();

    // remove from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setDelete();
    addShapeData(newShapeData);

    mw->itemChangesStack.add(this);

    if (pathItem) {
        pathItem->removeLinkedItem(this);

        if (pathItem->linkedItems_size() == 0) {
            pathItem->del();
        }
    }

    if (parentItem) {
        parentItem->removeChild(this);

        // look for other integration paths
        bool found=false;
        int i=0;
        while (i < parentItem->childCount()) {
            IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(parentItem->child(i));
            if (integrationPathItem && integrationPathItem->is_integrationPathSegment()) {
                found=true;
                break;
            }
            i++;
        }

        // remove the scale
        if (!found) {
            int i=0;
            while (i < parentItem->childCount()) {
                ScaleLabelItem *scaleLabelItem=dynamic_cast<ScaleLabelItem *>(parentItem->child(i));
                if (scaleLabelItem && scaleLabelItem->is_scaleLabel()) {
                    ShapeData *newShapeData=scaleLabelItem->getShapeData()->copyCreate();
                    newShapeData->setDelete();
                    scaleLabelItem->addShapeData(newShapeData);
                    mw->itemChangesStack.add(scaleLabelItem);

                    BaseItem *scaleLabelParentItem=scaleLabelItem->getParentItem();
                    if (scaleLabelParentItem) {
                        scaleLabelParentItem->removeChild(scaleLabelItem);
                    }
                }
                i++;
            }
        }
    }

    mw->finishOperation(true,1);

    std::cout << "exit IntegrationPathItem::del" << std::endl; std::cout.flush();
}

void IntegrationPathItem::flipSign ()
{
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setChangeName();
    addShapeData(newShapeData);

    startItemChange();
    addItemChange();

    QChar direction=newShapeData->get_name().front();
    QString newName;
    if (direction == QChar('+')) newName="-";
    if (direction == QChar('-')) newName="+";
    newName.append(newShapeData->get_name().sliced(1));

    newShapeData->set_name(newName);
    setText(0,newName);
}

////////////////////////////////////////////////////////////////////////////////
// RootBoundaryItem
////////////////////////////////////////////////////////////////////////////////

bool RootBoundaryItem::isValidShow ()
{
    int i=0;
    while (i < mw->boundary->childCount()) {
        RootBoundaryItem *child=dynamic_cast<RootBoundaryItem *>(mw->boundary->child(i));
        if (child && child->is_rootBoundary()) {
            if (child->foreground(0) == Qt::gray) return true;
        }
        i++;
    }
    return false;
}

bool RootBoundaryItem::isValidHide ()
{
    int i=0;
    while (i < mw->boundary->childCount()) {
        RootBoundaryItem *child=dynamic_cast<RootBoundaryItem *>(mw->boundary->child(i));
        if (child && child->is_rootBoundary()) {
            if (child->foreground(0) == Qt::black) return true;
        }
        i++;
    }
    return false;
}

void RootBoundaryItem::show ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        RootBoundaryItem *rootBoundaryItem=dynamic_cast<RootBoundaryItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (rootBoundaryItem && rootBoundaryItem->is_rootBoundary()) {
            mw->ui->drawingWindow->showItem(rootBoundaryItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(9);
}

void RootBoundaryItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        RootBoundaryItem *rootBoundaryItem=dynamic_cast<RootBoundaryItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (rootBoundaryItem && rootBoundaryItem->is_rootBoundary()) {
            mw->ui->drawingWindow->hideItem(rootBoundaryItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(11);
}

void RootBoundaryItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, this, &RootBoundaryItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &RootBoundaryItem::hide);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
}

////////////////////////////////////////////////////////////////////////////////
// BoundaryItem
////////////////////////////////////////////////////////////////////////////////

BoundaryItem::BoundaryItem (OpenParEMg *mw_, PathItem *pathItem_, int boundary_type_, double wave_impedance_, QString boundary_material_)
{
    mw=mw_;
    parentItem=mw->boundary;
    itemType=2;
    pathItem=pathItem_;

    setFlags(flags() | Qt::ItemIsEditable);
    setToolTip(0,"Boundary type.");

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->set_wave_impedance(wave_impedance_);
    newShapeData->set_boundary_type(boundary_type_);
    newShapeData->set_boundary_material(boundary_material_);
    addShapeData(newShapeData);

    QString name="boundary";
    name.append(QString::number(mw->boundary->childCount()+1));
    setText(0,name);
    newShapeData->set_name(text(0));
    setForeground(0,Qt::black);

    setSolidColor();

    // type

    BaseItem *itemType=new BaseItem(mw,this);
    itemType->set_itemType(20);
    itemType->setFlags(itemType->flags() | Qt::ItemIsEditable);
    itemType->setToolTip(0,"Boundary type.");
    addChild(itemType);

    // wave impedance
    BaseItem *itemWaveImpedance=new BaseItem(mw,this);
    itemWaveImpedance->set_itemType(21);
    itemWaveImpedance->setFlags(itemWaveImpedance->flags() | Qt::ItemIsEditable);
    itemWaveImpedance->setToolTip(0,"Wave impedance in Ohms.");
    addChild(itemWaveImpedance);

    // material
    BaseItem *itemMaterial=new BaseItem(mw,this);
    itemMaterial->set_itemType(22);
    itemMaterial->setFlags(itemMaterial->flags() | Qt::ItemIsEditable);
    itemMaterial->setToolTip(0,"Boundary material.");
    addChild(itemMaterial);

    // insert the widgets
    insertItemWidgets(itemType,itemWaveImpedance,itemMaterial);
}

void BoundaryItem::setSolidColor ()
{
    PathItem *pathItem=getPathItem();
    if (pathItem) {
        ShapeData *shapeData=getShapeData();
        int boundary_type=shapeData->get_boundary_type();

        Handle(AIS_Shape) shape=pathItem->getShape();
        if (!shape.IsNull()) {
            shape->SetTransparency(0);
            shape->SetMaterial(Graphic3d_NameOfMaterial_Plastered);
            if (boundary_type == 0) shape->SetColor(Quantity_NOC_GREENYELLOW);
            else if (boundary_type == 1) shape->SetColor(Quantity_NOC_CYAN);
            else if (boundary_type == 2) shape->SetColor(Quantity_NOC_GOLDENROD);
            else if (boundary_type == 3) shape->SetColor(Quantity_NOC_CORNFLOWERBLUE);
            mw->setShaded(shape);
        }
    }
}

void BoundaryItem::insertItemWidgets (BaseItem *itemType, BaseItem *itemWaveImpedance, BaseItem *itemMaterial)
{
    std::cout << "BoundaryItem::insertItemWidgets" << std::endl; std::cout.flush();

    QDoubleValidator doubleValidator;
    doubleValidator.setBottom(0);

    ShapeData *shapeData=getShapeData();
    double wave_impedance=shapeData->get_wave_impedance();
    int boundary_type=shapeData->get_boundary_type();
    QString boundary_material=shapeData->get_boundary_material();

    // type

    if (itemType) {
        CustomComboBox *comboType=new CustomComboBox();
        const QSignalBlocker blockerZdef(comboType);
        comboType->addItem("PEC");
        comboType->addItem("PMC");
        comboType->addItem("Zs");
        comboType->addItem("Radiation");
        comboType->set_portItem(nullptr);
        comboType->set_boundaryItem(this);
        comboType->set_type(2);

        comboType->setCurrentIndex(boundary_type);
        mw->ui->drawingItemTree->setItemWidget(itemType,0,comboType);

        QObject::connect(comboType,&CustomComboBox::CustomCurrentIndexChanged,&comboIndexChanged);
        QObject::connect(comboType,&CustomComboBox::CustomCurrentIndexChanged,mw->relay,&Relay::setMenus);
        QObject::connect(comboType,&CustomComboBox::CustomCurrentIndexChanged,mw->relay,&Relay::updateViewer);

        // set the CustomTreeWidget items so they can be hidden as needed depending on type
        comboType->set_itemMaterial(itemMaterial);
        comboType->set_itemWaveImpedance(itemWaveImpedance);
    }

    // wave impedance

    if (itemWaveImpedance) {
        CustomLineEdit *textWaveImpedance=new CustomLineEdit();
        const QSignalBlocker blockerWaveImpedance(textWaveImpedance);
        textWaveImpedance->setText(QString::number(wave_impedance));
        textWaveImpedance->setValidator(&doubleValidator);
        textWaveImpedance->set_baseItem(this);
        mw->ui->drawingItemTree->setItemWidget(itemWaveImpedance,0,textWaveImpedance);

        QObject::connect(textWaveImpedance,&CustomLineEdit::CustomEditFinished,&textValueChanged);
    }

    // material

    if (itemMaterial) {
        CustomComboBox *comboMaterial=new CustomComboBox();
        const QSignalBlocker blockerMaterial(comboMaterial);
        if (mw->materialDatabase) {
            // ToDo:: re-implement
            // long unsigned int i=0;
            // while (i < mw->materialDatabase->get_size()) {
            //     Material *material=mw->materialDatabase->get_material(i);
            //     // Todo: add only conductors
            //     comboMaterial->addItem(QString::fromStdString(material->get_name()->get_value()));
            //     i++;
            // }
            comboMaterial->addItem("none");
        }
        mw->ui->drawingItemTree->setItemWidget(itemMaterial,0,comboMaterial);
        comboMaterial->setCurrentText(boundary_material);

        QObject::connect(comboMaterial,&CustomComboBox::CustomCurrentTextChanged, &comboTextChanged);
        QObject::connect(comboMaterial,&CustomComboBox::CustomCurrentIndexChanged,mw->relay,&Relay::setMenus);
    }

    resetWidgets();
}

bool BoundaryItem::isValidShow ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BoundaryItem *portItem=dynamic_cast<BoundaryItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (portItem && portItem->is_port()) {
            PathItem *pathItem=portItem->getPathItem();
            if (pathItem && pathItem->is_path()) {
                if (pathItem->isValidShow()) return true;
            }
        }
        i++;
    }
    return false;
}

bool BoundaryItem::isValidHide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (boundaryItem && boundaryItem->is_boundary()) {
            PathItem *pathItem=boundaryItem->getPathItem();
            if (pathItem && pathItem->is_path()) {
                if (pathItem->isValidHide()) return true;
            }
        }
        i++;
    }
    return false;
}

void BoundaryItem::show ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (boundaryItem && boundaryItem->is_boundary()) {
            mw->ui->drawingWindow->showItem(boundaryItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(10);
}

void BoundaryItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (boundaryItem && boundaryItem->is_boundary()) {
            mw->ui->drawingWindow->hideItem(boundaryItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(10);
}

void BoundaryItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->unselectAction=new QAction("Unselect",this);
    mw->renameAction=new QAction("Rename",this);
    mw->deleteAction=new QAction("Delete",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, this, &BoundaryItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &BoundaryItem::hide);
    connect(mw->unselectAction, &QAction::triggered, mw, &OpenParEMg::unselectBoundaryItems);
    connect(mw->renameAction, &QAction::triggered, mw, &OpenParEMg::renameBoundaryItems);
    connect(mw->deleteAction, &QAction::triggered, mw, &OpenParEMg::deleteBoundaryItems);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (mw->ui->drawingWindow->hasBoundarySelectedItems()) menu->addAction(mw->unselectAction);
    if (mw->ui->drawingWindow->get_boundarySelectedCount() == 1) menu->addAction(mw->renameAction);
    menu->addAction(mw->deleteAction);
    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
}

void BoundaryItem::del ()
{
    // remove from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    // mark
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setDelete();
    addShapeData(newShapeData);

    // parentItem
    BaseItem *parentItem=getParentItem();
    if (parentItem) {
        RootBoundaryItem *rootBoundaryItem=dynamic_cast<RootBoundaryItem *>(parentItem);
        if (rootBoundaryItem && rootBoundaryItem->is_rootBoundary()) {
            rootBoundaryItem->removeChild(this);
        }

        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            pathItem->showArrows(true);
        }

        mw->itemChangesStack.add(this);
        mw->projectChanged=true;
    }
}

void BoundaryItem::resetWidgets ()
{
    //std::cout << "BoundaryItem::resetWidgets" << std::endl; std::cout.flush();

    BaseItem *boundaryType=nullptr;
    BaseItem *boundaryWaveImpedance=nullptr;
    BaseItem *boundaryMaterial=nullptr;

    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) {
            if (baseItem->is_boundaryType()) boundaryType=baseItem;
            else if (baseItem->is_boundaryWaveImpedance()) boundaryWaveImpedance=baseItem;
            else if (baseItem->is_boundaryMaterial()) boundaryMaterial=baseItem;
        }
        i++;
    }

    ShapeData *shapeData=getShapeData();
    int boundary_type=shapeData->get_boundary_type();
    double wave_impedance=shapeData->get_wave_impedance();
    QString boundary_material=shapeData->get_boundary_material();

    CustomComboBox *comboType=nullptr;
    if (boundaryType) {
        comboType=dynamic_cast<CustomComboBox *>(mw->ui->drawingItemTree->itemWidget(boundaryType,0));
        if (comboType) {
            const QSignalBlocker blocker(comboType);
            comboType->setCurrentIndex(boundary_type);
        }
    }

    CustomLineEdit *itemWaveImpedance=nullptr;
    if (boundaryWaveImpedance) {
        itemWaveImpedance=dynamic_cast<CustomLineEdit *>(mw->ui->drawingItemTree->itemWidget(boundaryWaveImpedance,0));
        if (itemWaveImpedance) {
            const QSignalBlocker blocker(itemWaveImpedance);
            itemWaveImpedance->setText(QString::number(wave_impedance));
        }
    }

    CustomComboBox *itemMaterial=nullptr;
    if (boundaryMaterial) {
        itemMaterial=dynamic_cast<CustomComboBox *>(mw->ui->drawingItemTree->itemWidget(boundaryMaterial,0));
        if (itemMaterial) {
            const QSignalBlocker blockerMaterial(itemMaterial);
            //itemMaterial->setCurrentText(boundary_material);
            itemMaterial->setCurrentText("none");
        }
    }

    // set visibility
    if (boundaryWaveImpedance && boundaryMaterial) {
        if (boundary_type == 0) {  // PEC
            boundaryWaveImpedance->setHidden(true);
            boundaryMaterial->setHidden(true);
        } else if (boundary_type == 1) {  // PMC
            boundaryWaveImpedance->setHidden(true);
            boundaryMaterial->setHidden(true);
        } else if (boundary_type == 2) {  // Zs
            boundaryWaveImpedance->setHidden(true);
            boundaryMaterial->setHidden(false);
        } else if (boundary_type == 3) {  // radiation
            boundaryWaveImpedance->setHidden(false);
            boundaryMaterial->setHidden(true);
        }
    }

    // set the shape color
    setSolidColor();
}

void BoundaryItem::undo ()
{
    std::cout << "BoundaryItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isCreate()) {
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            pathItem->showArrows(true);
        }

    } else if (shapeData->isDelete()) {
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->push_linkedItem(this);
            pathItem->showArrows(false);
        }
    }

    BaseItem::undo();
}

void BoundaryItem::redo ()
{
    std::cout << "BoundaryItem::redo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    ShapeData *next=shapeData->getNext();
    if (!next) return;

    if (next->isCreate()) {
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->push_linkedItem(this);
            pathItem->showArrows(false);
        }
    } else if (next->isDelete()) {
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            pathItem->showArrows(true);
        }
    }

    BaseItem::redo();
}

////////////////////////////////////////////////////////////////////////////////
// RootPortItem
////////////////////////////////////////////////////////////////////////////////

bool RootPortItem::isValidShow ()
{
    int i=0;
    while (i < mw->port->childCount()) {
        PortItem *child=dynamic_cast<PortItem *>(mw->port->child(i));
        if (child && child->is_port()) {
            if (child->foreground(0) == Qt::gray) return true;
        }
        i++;
    }
    return false;
}

bool RootPortItem::isValidHide ()
{
    int i=0;
    while (i < mw->port->childCount()) {
        PortItem *child=dynamic_cast<PortItem *>(mw->port->child(i));
        if (child && child->is_port()) {
            if (child->foreground(0) == Qt::black) return true;
        }
        i++;
    }
    return false;
}

void RootPortItem::show ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        RootPortItem *rootPortItem=dynamic_cast<RootPortItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (rootPortItem && rootPortItem->is_rootPort()) {
            mw->ui->drawingWindow->showItem(rootPortItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(9);
}

void RootPortItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        RootPortItem *rootPortItem=dynamic_cast<RootPortItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (rootPortItem && rootPortItem->is_rootPort()) {
            mw->ui->drawingWindow->hideItem(rootPortItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(11);
}

void RootPortItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, this, &RootPortItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &RootPortItem::hide);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
}

////////////////////////////////////////////////////////////////////////////////
// PortItem
////////////////////////////////////////////////////////////////////////////////

PortItem::PortItem (OpenParEMg *mw_, PathItem *pathItem_, QString impedance_calculation_, QString impedance_definition_)
{
    mw=mw_;
    parentItem=mw->port;
    itemType=1;
    pathItem=pathItem_;

    int Sport=mw->port->childCount()+1;

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->set_impedance_calculation(impedance_calculation_);
    newShapeData->set_impedance_definition(impedance_definition_);
    newShapeData->set_Sport(Sport);
    addShapeData(newShapeData);

    QString name="port";
    name.append(QString::number(Sport));
    setText(0,name);
    newShapeData->set_name(text(0));
    setForeground(0,Qt::black);

    setToolTip(0,"Port name.");

    if (pathItem) {

        // process the path
        pathItem->push_linkedItem(this);
        mw->convertPathToFace(pathItem);
        pathItem->showArrows(false);
        setSolidColor();
    }

    // impedance definition
    addImpedanceDefinitionItem();

    // impedance calculation
    addImpedanceCalculationItem();

    // add one default mode since at least one mode is required
    ModeItem *newModeItem=new ModeItem(mw,this);
    addChild(newModeItem);
}

void PortItem::setSolidColor ()
{
    PathItem *pathItem=getPathItem();
    if (pathItem) {
        Handle(AIS_Shape) shape=pathItem->getShape();
        if (!shape.IsNull()) {
            shape->SetColor(Quantity_NOC_MINTCREAM);
            shape->SetTransparency(0.25);
            shape->SetMaterial(Graphic3d_NameOfMaterial_Plastered);
            mw->setShaded(shape);
        }
    }
}

void PortItem::insertImpedanceDefinitionWidget (BaseItem *itemImpedanceDefinition, QString impedance_definition)
{
    CustomComboBox *comboZdef=new CustomComboBox();
    const QSignalBlocker blockerZdef(comboZdef);
    comboZdef->set_itemTracker(mw->ui->drawingWindow->get_itemTracker());
    comboZdef->addItem("VI");       // 0
    comboZdef->addItem("PV");       // 1
    comboZdef->addItem("PI");       // 2
    comboZdef->addItem("invalid");  // 3
    comboZdef->set_portItem(this);
    comboZdef->set_boundaryItem(nullptr);
    comboZdef->set_type(0);
    if (impedance_definition.compare("VI") == 0) comboZdef->setCurrentIndex(0);
    else if (impedance_definition.compare("PV") == 0) comboZdef->setCurrentIndex(1);
    else if (impedance_definition.compare("PI") == 0) comboZdef->setCurrentIndex(2);
    else comboZdef->setCurrentIndex(3);
    mw->ui->drawingItemTree->setItemWidget(itemImpedanceDefinition,0,comboZdef);
    itemImpedanceDefinition->setSizeHint(0,comboZdef->sizeHint());  // size hint for scaling; do not need to do the other combobox

    QObject::connect(comboZdef,&CustomComboBox::CustomCurrentIndexChanged,&comboIndexChanged);
    QObject::connect(comboZdef,&CustomComboBox::CustomCurrentIndexChanged,mw->relay,&Relay::setMenus);
}

void PortItem::addImpedanceDefinitionItem ()
{
    ShapeData *shapeData=getShapeData();
    QString impedance_definition=shapeData->get_impedance_definition();

    BaseItem *itemImpedanceDefinition=new BaseItem(mw,this);
    itemImpedanceDefinition->set_itemType(6);
    itemImpedanceDefinition->setFlags(itemImpedanceDefinition->flags() & ~Qt::ItemIsSelectable);
    itemImpedanceDefinition->setToolTip(0,"Impedance definition for calculating characteristic impedance.");
    addChild(itemImpedanceDefinition);

    insertImpedanceDefinitionWidget(itemImpedanceDefinition,impedance_definition);
}

void PortItem::insertImpedanceCalculationWidget (BaseItem *itemImpedanceCalculation, QString impedance_calculation)
{
    CustomComboBox *comboZcalc=new CustomComboBox();
    const QSignalBlocker blockerZcalc(comboZcalc);
    comboZcalc->set_itemTracker(mw->ui->drawingWindow->get_itemTracker());
    comboZcalc->addItem("line");
    comboZcalc->addItem("modal");
    comboZcalc->set_portItem(this);
    comboZcalc->set_type(1);
    comboZcalc->set_boundaryItem(nullptr);

    if (impedance_calculation.compare("line") == 0) comboZcalc->setCurrentIndex(0);
    else if (impedance_calculation.compare("modal") == 0) comboZcalc->setCurrentIndex(1);
    mw->ui->drawingItemTree->setItemWidget(itemImpedanceCalculation,0,comboZcalc);

    QObject::connect(comboZcalc,&CustomComboBox::CustomCurrentIndexChanged,&comboIndexChanged);
    QObject::connect(comboZcalc,&CustomComboBox::CustomCurrentIndexChanged,mw->relay,&Relay::setMenus);
}

void PortItem::addImpedanceCalculationItem ()
{
    ShapeData *shapeData=getShapeData();
    QString impedance_calculation=shapeData->get_impedance_calculation();

    BaseItem *itemImpedanceCalculation=new BaseItem(mw,this);
    itemImpedanceCalculation->set_itemType(7);
    itemImpedanceCalculation->setFlags(itemImpedanceCalculation->flags() & ~Qt::ItemIsSelectable);
    itemImpedanceCalculation->setToolTip(0,"Impedance calculation using modal or line integration paths.");
    addChild(itemImpedanceCalculation);

    insertImpedanceCalculationWidget(itemImpedanceCalculation,impedance_calculation);
}

bool PortItem::isValidShow ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        PortItem *portItem=dynamic_cast<PortItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (portItem && portItem->is_port()) {
            PathItem *pathItem=portItem->getPathItem();
            if (pathItem && pathItem->is_path()) {
                if (pathItem->isValidShow()) return true;
            }
        }
        i++;
    }
    return false;
}

bool PortItem::isValidHide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        PortItem *portItem=dynamic_cast<PortItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (portItem && portItem->is_port()) {
            PathItem *pathItem=portItem->getPathItem();
            if (pathItem && pathItem->is_path()) {
                if (pathItem->isValidHide()) return true;
            }
        }
        i++;
    }
    return false;
}

void PortItem::show ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        PortItem *portItem=dynamic_cast<PortItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (portItem && portItem->is_port()) {
            mw->ui->drawingWindow->showItem(portItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(10);
}

void PortItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        PortItem *portItem=dynamic_cast<PortItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (portItem && portItem->is_port()) {
            mw->ui->drawingWindow->hideItem(portItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(10);
}

void PortItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->unselectAction=new QAction("Unselect",this);
    mw->renameAction=new QAction("Rename",this);
    mw->insertAction=new QAction("Insert Mode",this);
    mw->deleteAction=new QAction("Delete",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, this, &PortItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &PortItem::hide);
    connect(mw->unselectAction, &QAction::triggered, mw, &OpenParEMg::unselectPortItems);
    connect(mw->insertAction, &QAction::triggered, mw, &OpenParEMg::insertModeItems);
    connect(mw->renameAction, &QAction::triggered, mw, &OpenParEMg::renamePortItems);
    connect(mw->deleteAction, &QAction::triggered, mw, &OpenParEMg::deletePortItems);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (mw->ui->drawingWindow->hasPortSelectedItems()) menu->addAction(mw->unselectAction);
    if (mw->ui->drawingWindow->get_portSelectedCount() == 1) menu->addAction(mw->renameAction);
    menu->addAction(mw->insertAction);
    menu->addAction(mw->deleteAction);
    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
}

void PortItem::del ()
{
    // remove from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    // mark
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setDelete();
    addShapeData(newShapeData);

    // parentItem
    BaseItem *parentItem=getParentItem();
    if (parentItem) {
        RootPortItem *rootPortItem=dynamic_cast<RootPortItem *>(parentItem);
        if (rootPortItem && rootPortItem->is_rootPort()) {
            rootPortItem->removeChild(this);
        }

        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            pathItem->showArrows(true);
        }

        mw->itemChangesStack.add(this);
        mw->projectChanged=true;
    }
}

void PortItem::undo ()
{
    std::cout << "PortItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isCreate()) {
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            pathItem->showArrows(true);
        }
    } else if (shapeData->isDelete()) {
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->push_linkedItem(this);
            pathItem->showArrows(false);
        }
    }

    BaseItem::undo();
}

void PortItem::redo ()
{
    std::cout << "PortItem::redo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    ShapeData *next=shapeData->getNext();
    if (!next) return;

    if (next->isCreate()) {
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->push_linkedItem(this);
            pathItem->showArrows(false);
        }
    } else if (next->isDelete()) {
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            pathItem->showArrows(true);
        }
    }

    BaseItem::redo();
}

////////////////////////////////////////////////////////////////////////////////
// ModeItem
////////////////////////////////////////////////////////////////////////////////

ModeItem::ModeItem (OpenParEMg *mw_, PortItem *portItem_)
{
    mw=mw_;
    parentItem=portItem_;
    itemType=5;
    portItem=portItem_;

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    addShapeData(newShapeData);

    // transfer the Sport number from the port to the mode
    if (portItem_) {
        ShapeData *shapeData=portItem_->getShapeData();
        newShapeData->set_Sport(shapeData->get_Sport());
    }

    QString name="net";
    name.append(QString::number(newShapeData->get_Sport()));
    setText(0,name);
    newShapeData->set_name(text(0));
    setForeground(0,Qt::black);

    setToolTip(0,"Mode and its net name.");
    setFlags(flags() & ~Qt::ItemIsEditable);

    SportItem *newSportItem=new SportItem(mw,this);
    addChild(newSportItem);

    VIItem *newVoltageItem=new VIItem(mw,this,10);
    addChild(newVoltageItem);

    VIItem *newCurrentItem=new VIItem(mw,this,11);
    addChild(newCurrentItem);
}

bool ModeItem::isValidShow ()
{
    return mw->ui->drawingWindow->isNetValidShow();
}

bool ModeItem::isValidHide ()
{
    return mw->ui->drawingWindow->isNetValidHide();
}

void ModeItem::show ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem && baseItem->is_sport()) {
            mw->ui->drawingWindow->showItem(baseItem);

            int j=0;
            while (j < baseItem->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(baseItem->child(j));
                if (child->is_voltage() || child->is_current()) {
                    //child->setForeground(0,Qt::black);
                    int k=0;
                    while (k < child->childCount()) {
                        BaseItem *grandChild=dynamic_cast<BaseItem *>(child->child(k));
                        if (grandChild->is_integrationPathSegment()) {
                            mw->ui->drawingWindow->showItem(grandChild);
                            grandChild->setForeground(0,Qt::black);
                        }
                        k++;
                    }
                }
                j++;
            }
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(29);
}

void ModeItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem && baseItem->is_sport()) {
            int j=0;
            while (j < baseItem->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(baseItem->child(j));
                if (child->is_voltage() || child->is_current()) {
                    //child->setForeground(0,Qt::gray);
                    int k=0;
                    while (k < child->childCount()) {
                        BaseItem *grandChild=dynamic_cast<BaseItem *>(child->child(k));
                        if (grandChild->is_integrationPathSegment()) {
                            mw->ui->drawingWindow->hideItem(grandChild);
                            grandChild->setForeground(0,Qt::gray);
                        }
                        k++;
                    }
                }
                j++;
            }
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(33);
}

bool ModeItem::isValidDelete () {return true;}

void ModeItem::unlinkPaths (BaseItem *baseItem)
{
    if (!baseItem) return;

    IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(baseItem);
    if (integrationPathItem && is_integrationPathSegment()) {
        PathItem *pathItem=integrationPathItem->getPathItem();
        pathItem->removeLinkedItem(integrationPathItem);
        // ToDo: null out the path item in integrationPathItem?
    }

    int i=0;
    while (i < baseItem->childCount()) {
        BaseItem *childItem=dynamic_cast<BaseItem *>(baseItem->child(i));
        unlinkPaths(childItem);
        i++;
    }
}

void ModeItem::del ()
{
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setDelete();
    addShapeData(newShapeData);

    unlinkPaths(this);
    PortItem *portItem=getPortItem();
    if (portItem) {
        portItem->removeChild(this);
    }

    mw->itemChangesStack.add(this);

    mw->clickedItem=nullptr;
    mw->previousClickedItem=nullptr;

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(28);
}

void ModeItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->renameAction=new QAction("Rename",this);
    mw->deleteAction=new QAction("Delete",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, this, &ModeItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &ModeItem::hide);
    connect(mw->renameAction, &QAction::triggered, mw, &OpenParEMg::renameSportNet);
    connect(mw->deleteAction, &QAction::triggered, mw, &OpenParEMg::deleteModeItems);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (mw->ui->drawingWindow->get_selectedItems_count() == 1) menu->addAction(mw->renameAction);
    if (isValidDelete()) menu->addAction(mw->deleteAction);
    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
}

////////////////////////////////////////////////////////////////////////////////
// SportItem
////////////////////////////////////////////////////////////////////////////////

SportItem::SportItem (OpenParEMg *mw_, ModeItem *modeItem_)
{
    mw=mw_;
    parentItem=modeItem_;
    itemType=8;
    modeItem=modeItem_;
    setText(0,"S Port");
    setForeground(0,Qt::black);
    setFlags(flags() & ~Qt::ItemIsEditable);
    setToolTip(0,"S-parameter port number for the mode.");

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    addShapeData(newShapeData);

    SportNumberItem *sportNumberItem=new SportNumberItem(mw,this);
    addChild(sportNumberItem);

    // get the next S-parameter port number
    int Sport=0;
    ModeItem *modeItem=getModeItem();
    if (modeItem) {
        PortItem *portItem=modeItem->getPortItem();
        if (portItem) {
            ShapeData *shapeData=portItem->getShapeData();
            Sport=shapeData->get_Sport();
        }
    }

    // spin box for changing the port number
    insertSportNumberWidget(sportNumberItem,Sport);
}

void SportItem::insertSportNumberWidget (BaseItem *baseItem, int Sport)
{
    SportNumberItem *sportNumberItem=dynamic_cast<SportNumberItem *>(baseItem);

    CustomSpinBox *sportNumber=new CustomSpinBox();
    const QSignalBlocker blocker(sportNumber);
    sportNumber->set_itemTracker(mw->ui->drawingWindow->get_itemTracker());
    sportNumber->set_sportNumberItem(sportNumberItem);
    sportNumber->setMinimum(1);
    sportNumber->setValue(Sport);
    mw->ui->drawingItemTree->setItemWidget(baseItem,0,sportNumber);

    QObject::connect(sportNumber,&CustomSpinBox::CustomValueChanged,&spinValueChanged);
    QObject::connect(sportNumber,&CustomSpinBox::CustomValueChanged,mw->relay,&Relay::setMenus);
}

bool SportItem::isValidShow () {return false;}
bool SportItem::isValidHide () {return false;}
void SportItem::show () {}
void SportItem::hide () {}

void SportItem::showMenu (QMenu *menu)
{
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
}

////////////////////////////////////////////////////////////////////////////////
// SportNumberItem
////////////////////////////////////////////////////////////////////////////////

SportNumberItem::SportNumberItem (OpenParEMg *mw_, SportItem *sportItem_)
{
    mw=mw_;
    parentItem=sportItem_;
    itemType=9;
    sportItem=sportItem_;

    setForeground(0,Qt::black);
    setFlags(flags() & ~Qt::ItemIsSelectable);
    setToolTip(0,"S-parameter port number.");

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    addShapeData(newShapeData);

    setToolTip(0,"S-parameter port number.");
}

bool SportNumberItem::isValidShow () {return false;}
bool SportNumberItem::isValidHide () {return false;}
void SportNumberItem::show () {}
void SportNumberItem::hide () {}

void SportNumberItem::showMenu (QMenu *menu)
{
}

////////////////////////////////////////////////////////////////////////////////
// VIItem
////////////////////////////////////////////////////////////////////////////////

VIItem::VIItem (OpenParEMg *mw_, ModeItem *modeItem_, int itemType_)
{
    mw=mw_;
    parentItem=modeItem_;
    itemType=itemType_;
    modeItem=modeItem_;
    parentItem=modeItem_;

    if (itemType_ == 10) {
        setText(0,"voltage");
        setToolTip(0,"Voltage integration path.");
    } else if (itemType_ == 11) {
        setText(0,"current");
        setToolTip(0,"Current integration path.");
    }

    setFlags(flags() & ~Qt::ItemIsEditable);
    setForeground(0,Qt::black);

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->set_name(text(0));
    addShapeData(newShapeData);
}

bool VIItem::isValidShow ()
{
    if (foreground(0) == Qt::gray) return true;
    return false;
}

bool VIItem::isValidHide ()
{
    if (foreground(0) == Qt::black) return true;
    return false;
}

bool VIItem::isValidDrawPath ()
{
    if (mw->ui->drawingWindow->get_selectedItems_count() == 1 && mw->clickedItem->foreground(0) == Qt::black) return true;
    return false;
}

void VIItem::show ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem) {
            if (baseItem->is_voltage() || baseItem->is_current()) {
                mw->ui->drawingWindow->showItem(baseItem);
            }
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(30);
}

void VIItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem) {
            if (baseItem->is_voltage() || baseItem->is_current()) {
                mw->ui->drawingWindow->hideItem(baseItem);
            }
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(34);
}

void VIItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->drawPathAction=new QAction("Draw Line Path");
    mw->drawPolylineAction=new QAction("Draw Polyline Path");
    mw->insertAction=new QAction("Add Path");
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, this, &VIItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &VIItem::hide);
    connect(mw->drawPathAction, &QAction::triggered, this, &VIItem::drawLinePath);
    connect(mw->drawPolylineAction, &QAction::triggered, this, &VIItem::drawPolylinePath);
    connect(mw->insertAction, &QAction::triggered, this, &VIItem::insertSelectedPath);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (isValidDrawPath()) menu->addAction(mw->drawPathAction);
    if (isValidDrawPath()) menu->addAction(mw->drawPolylineAction);
    if (isValidInsertSelectedPath()) menu->addAction(mw->insertAction);
    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
}

void VIItem::drawLinePath ()
{
    mw->isIntegrationPath=true;
    mw->workingItem=mw->clickedItem;

    mw->currentDrawingItem=new DrawingItem(mw,mw->drawing);
    mw->currentDrawingItem->startLine();
}

void VIItem::drawPolylinePath ()
{
    mw->isIntegrationPath=true;
    mw->workingItem=mw->clickedItem;

    mw->currentDrawingItem=new DrawingItem(mw,mw->drawing);
    mw->currentDrawingItem->startPolyline();
}

bool VIItem::isValidInsertSelectedPath ()
{
    int VIcount=0;
    BaseItem *VIitem;
    int pathCount=0;

    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem) {
            if (baseItem->is_voltage() || baseItem->is_current()) {VIitem=baseItem; VIcount++;}

            PathItem *pathItem=dynamic_cast<PathItem *>(baseItem);
            if (pathItem && pathItem->is_path()) pathCount++;
        }
        i++;
    }

    if (VIcount != 1) return false;
    if (pathCount == 0) return false;

    // check that the paths are within the port

    ModeItem *modeItem=dynamic_cast<ModeItem *>(VIitem->getParentItem());
    PortItem *portItem=dynamic_cast<PortItem *>(modeItem->getParentItem());

    // port outline
    PathItem *pathItem=portItem->getPathItem();
    if (pathItem) return false;
    Path *portPath=pathItem->getPath();
    if (!portPath) return false;

    i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem) {
            PathItem *pathItem=dynamic_cast<PathItem *>(baseItem);
            if (pathItem && pathItem->is_path()) {
                Path *path=pathItem->getPath();
                if (!portPath->is_path_inside(path)) {
                    return false;
                }
            }
        }
        i++;
    }

    return true;
}

PathItem* VIItem::createIntegrationPathItemFromDrawing (DrawingItem *drawingItem, bool hasArrows)
{
    std::cout << "VIItem::createIntegrationPathItemFromDrawing" << std::endl; std::cout.flush();

    // create a path item
    PathItem *newPathItem=drawingItem->createPath(hasArrows);

    // create an integration path item
    IntegrationPathItem *newIntegrationPathItem=new IntegrationPathItem(mw,this,newPathItem);
    if (newIntegrationPathItem) {
        ShapeData *newShapeData=newIntegrationPathItem->getShapeData()->copyCreate();
        newShapeData->setCreate();
        newIntegrationPathItem->addShapeData(newShapeData);

        QString name="+";
        name.append(newPathItem->text(0));
        newIntegrationPathItem->setText(0,name);
        newShapeData->set_name(newIntegrationPathItem->text(0));

        addChild(newIntegrationPathItem);
        mw->itemChangesStack.add(newIntegrationPathItem);

        newPathItem->push_linkedItem(newIntegrationPathItem);
    }
    return newPathItem;
}

void VIItem::convertItemToPath (DrawingItem *drawingItem, bool useArrows)
{
    std::cout << "VIItem::convertItemToPath" << std::endl; std::cout.flush();

    PathItem *pathItem=createIntegrationPathItemFromDrawing(drawingItem,useArrows);
    if (pathItem) {
        drawingItem->del();
    }
}

void VIItem::insertSelectedPath ()
{
    std::cout << "VIItem::insertSelectedPath" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem) {
            if (baseItem->is_voltage() || baseItem->is_current()) mw->insertIntegrationPath(baseItem);
        }
        i++;
    }
}

bool VIItem::hasScale ()
{
    int i=0;
    while (i < childCount()) {
        BaseItem *scaleLabelItem=dynamic_cast<BaseItem *>(child(i));
        if (scaleLabelItem && scaleLabelItem->is_scaleLabel()) return true;
        i++;
    }
    return false;
}

ScaleLabelItem* VIItem::addScaleItem ()
{
    // label
    ScaleLabelItem *scaleLabelItem=new ScaleLabelItem(mw,this);
    insertChild(0,scaleLabelItem);

    // value

    ScaleValueItem *scaleValueItem=new ScaleValueItem(mw,scaleLabelItem);
    scaleLabelItem->addChild(scaleValueItem);

    ShapeData *shapeData=scaleValueItem->getShapeData();
    scaleValueItem->insertScaleValueWidget(shapeData->get_scale());

    return scaleLabelItem;
}

////////////////////////////////////////////////////////////////////////////////
// RootMeshItem
////////////////////////////////////////////////////////////////////////////////

bool RootMeshItem::isValidShow ()
{
    int i=0;
    while (i < mw->mesh->childCount()) {
        BaseItem *child=dynamic_cast<BaseItem *>(mw->mesh->child(i));
        if (child->foreground(0) == Qt::gray) return true;
        i++;
    }
    return false;
}

bool RootMeshItem::isValidHide ()
{
    int i=0;
    while (i < mw->mesh->childCount()) {
        BaseItem *child=dynamic_cast<BaseItem *>(mw->mesh->child(i));
        if (child->foreground(0) == Qt::black) return true;
        i++;
    }
    return false;
}

void RootMeshItem::show ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem && baseItem->is_rootMesh()) {
            int j=0;
            while (j < baseItem->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(baseItem->child(j));
                mw->ui->drawingWindow->showItem(child);
                child->setForeground(0,Qt::black);
                j++;
            }
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(13);
}

void RootMeshItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem && baseItem->is_rootMesh()) {
            int j=0;
            while (j < baseItem->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(baseItem->child(j));
                mw->ui->drawingWindow->hideItem(child);
                child->setForeground(0,Qt::gray);
                j++;
            }
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(14);
}

void RootMeshItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, this, &RootMeshItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &RootMeshItem::hide);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
}

////////////////////////////////////////////////////////////////////////////////
// MeshItem
////////////////////////////////////////////////////////////////////////////////

MeshItem::MeshItem (OpenParEMg *mw_)
{
    mw=mw_;
    parentItem=mw->mesh;
    itemType=3;
    setText(0,"MeshItem");
    setForeground(0,Qt::black);
}

bool MeshItem::isValidShow ()
{
    if (foreground(0) == Qt::gray) return true;
    return false;
}

bool MeshItem::isValidHide ()
{
    if (foreground(0) == Qt::black) return true;
    return false;
}

void MeshItem::show ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem && baseItem->is_mesh()) {
            mw->ui->drawingWindow->showItem(baseItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(15);
}

void MeshItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem && baseItem->is_mesh()) {
            mw->ui->drawingWindow->hideItem(baseItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(16);
}

void MeshItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);

    connect(mw->showAction, &QAction::triggered, this, &MeshItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &MeshItem::hide);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
}
