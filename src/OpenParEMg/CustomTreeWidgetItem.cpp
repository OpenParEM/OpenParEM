

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

// BaseItem (QTreeWidgetItem *parent = nullptr, int type=Type) : QTreeWidgetItem(parent,type)
// {
//     itemType=-1;
//     depth=0;
//     parentItem=nullptr;
//     hasArrows=false;
//     isActive=false;
// }

BaseItem::BaseItem (OpenParEMg *mw_, BaseItem *parentItem_)
{
    mw=mw_;
    parentItem=parentItem_;
    itemType=-1;
    setText(0,"BaseItem");
    setForeground(0,Qt::black);

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    addShapeData(newShapeData);
}

void BaseItem::restoreWidgets (BaseItem *baseItem)
{
    std::cout << "BaseItem::restoreWidgets  baseItem=" << baseItem << std::endl; std::cout.flush();

    if (!baseItem) return;

    std::cout << "                          itemType=" << baseItem->get_itemType() << std::endl; std::cout.flush();
    std::cout << "                          parent->itemType=" << baseItem->getParentItem()->get_itemType() << std::endl; std::cout.flush();

    std::cout << " sorting enabled=" << mw->ui->drawingItemTree->isSortingEnabled() << std::endl; std::cout.flush();

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

void BaseItem::undo ()
{
    std::cout << "BaseItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
    } else if (shapeData->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
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
    } else if (next->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
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
    addShapeData(newShapeData);
}

void ScaleLabelItem::undo ()
{
    std::cout << "ScaleLabelItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // nothing to do
    } else if (shapeData->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();
        parentItem->removeChild(this);
        dataStack.undo();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
    }
}

void ScaleLabelItem::redo ()
{
    std::cout << "ScaleLabelItem::redo  this=" << this << std::endl; std::cout.flush();

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
        parentItem->addChild(this);
        restoreWidgets(this);
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
    }
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

void ScaleValueItem::undo ()
{
    std::cout << "ScaleItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // nothing to do
    } else if (shapeData->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();


    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

        dataStack.undo();

        CustomLineEdit *scaleEdit=dynamic_cast<CustomLineEdit *>(mw->ui->drawingItemTree->itemWidget(this,0));
        const QSignalBlocker blocker(scaleEdit);
        ShapeData *shapeData=getShapeData();
        scaleEdit->setText(QString::number(shapeData->get_scale()));
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
    }
}

void ScaleValueItem::redo ()
{
    std::cout << "ScaleValueItem::redo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    ShapeData *next=shapeData->getNext();
    if (!next) return;

    if (next->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // should not occur
    } else if (next->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

        dataStack.redo();

        CustomLineEdit *scaleEdit=dynamic_cast<CustomLineEdit *>(mw->ui->drawingItemTree->itemWidget(this,0));
        const QSignalBlocker blocker(scaleEdit);
        ShapeData *shapeData=getShapeData();
        scaleEdit->setText(QString::number(shapeData->get_scale()));
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
    }
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

    connect(mw->showAction, &QAction::triggered, this, &RootDrawingItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &RootDrawingItem::hide);
    connect(mw->selectAllAction, &QAction::triggered, this, &RootDrawingItem::selectAll);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (isValidSelectAll()) menu->addAction(mw->selectAllAction);
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

void DrawingItem::setForUndoRedo ()
{   
    // clone the item onto itself for undo/redo
    // Do this before deleting the shape below
    ShapeData *newShapeData=getShapeData()->copyCreate();

    // remove the old version from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    // save the new data
    newShapeData->setEdit();
    addShapeData(newShapeData);

    // put the new version into display and tracking
    mw->ui->drawingWindow->displayShape(getShape());
    mw->ui->drawingWindow->insertItemToMap(getShape(),this);
    setForeground(0,Qt::gray);
    mw->ui->drawingWindow->showItem(this);

    // reset the selection filters
    mw->startOperation(true);
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
    //mw->workingItem=nullptr;

    // mark as changed
    mw->drawingChanged=true;
}

void DrawingItem::cancelDraw ()
{
    // take care of shapes
    if (!animateShape.IsNull()) animateShape.Nullify();
    if ( mw->activePolywire) {
        mw->activePolywire->deleteRubberband();
        mw->activePolywire=nullptr;
    }

    cancelOperation();
    mw->ui->drawingWindow->set_gridPlane(mw->currentPrivilegedPlane);

    // remove the current undo/redo item
    mw->itemChangesStack.pop_back();

    mw->activeAction=false;

    mw->ui->drawingWindow->updateViewer();
}

void DrawingItem::startMove ()
{
    //std::cout << "DrawingItem::startMove" << std::endl; std::cout.flush();

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        setForUndoRedo();
        resetOperation();
        setAnimate(mw->ui->drawingWindow->get_viewerContext());
        setEnableMove(true);
        mw->itemChangesStack.add(this);
    }

    Process *process=static_cast<Process *>(getProcess());
    if (process) {
        int i=0;
        while (i < childCount()) {
            DrawingItem *processChild=(DrawingItem *)child(i);
            resetOperation();
            setAnimate(mw->ui->drawingWindow->get_viewerContext());
            setEnableMove(true);
            processChild->startMove();
            //mw->ui->drawingWindow->hideItem(processChild);
            i++;
        }
    }

    if (!polywire && !process) {
        setForUndoRedo();
        resetOperation();
        setAnimate(mw->ui->drawingWindow->get_viewerContext());
        setEnableMove(true);
        mw->itemChangesStack.add(this);
    }
}

void DrawingItem::finishMove (gp_Pnt p0_, gp_Pnt p1_)
{
    //std::cout << "DrawingItem::finishMove" << std::endl; std::cout.flush();

    unsetAnimate(mw->ui->drawingWindow->get_viewerContext());

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
    mw->findShowTopLevelItem(this,false);
}

void DrawingItem::startRotate ()
{
    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        setForUndoRedo();
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
        setForUndoRedo();
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
    mw->findShowTopLevelItem(this,false);
}

void DrawingItem::startStretch ()
{
    setForUndoRedo();
    resetOperation();
    setEnableStretch(true);

    // set the drawing plane
    mw->currentPrivilegedPlane=mw->ui->drawingWindow->get_gridPlane();

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (polywire) {
        polywire->drawStretchRubberband();
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
    //polywire->deleteRubberband();

    //mw->finishStretchPoint(this);
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
    setForUndoRedo();

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
    mw->findShowTopLevelItem(this,false);
}

void DrawingItem::startDeletePoint ()
{
    setForUndoRedo();

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
    mw->findShowTopLevelItem(this,false);
}

void DrawingItem::cancelDeletePoint ()
{
    setEnableDeletePoint(false);
}

void DrawingItem::startInsertPoint ()
{
    setForUndoRedo();

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
    mw->findShowTopLevelItem(this,false);
    mw->finishOperation(false,1);
}

void DrawingItem::cancelInsertPoint ()
{
    setEnableInsertPoint(false);
}

void DrawingItem::convertToPolyline ()
{
    setForUndoRedo();

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

        mw->findShowTopLevelItem(this,false);
    }
}

void DrawingItem::del ()
{
    setForUndoRedo();

    // mark as delete
    ShapeData *shapeData=getShapeData();
    shapeData->setDelete();

    // parentItem
    BaseItem *parentItem=getParentItem();
    if (parentItem) {
        RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(parentItem);
        if (rootDrawingItem && rootDrawingItem->is_rootDrawing()) {
            int insertIndex=rootDrawingItem->indexOfChild(this);

            // move children to parent
            while (childCount() > 0) {
                DrawingItem* drawingChild=dynamic_cast<DrawingItem *>(takeChild(0));
                rootDrawingItem->insertChild(insertIndex++,drawingChild);
                drawingChild->setParentItem(rootDrawingItem);
                drawingChild->decrease_depth();
                mw->ui->drawingWindow->showItem(drawingChild);

                // set the materials
                if (!text(1).isNull()) {
                    if (!drawingChild->getPolywire()) drawingChild->setText(1,text(1));
                }
            }

            rootDrawingItem->removeChild(this);
        }

        mw->itemChangesStack.add(this);

        // remove from display and tracking
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(this->getShape());

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
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item) mw->ui->drawingWindow->showItem(item);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(30);
}

void DrawingItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item) mw->ui->drawingWindow->hideItem(item);
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

    viewerContext->Redisplay(shape,Standard_True);

    BRepBuilderAPI_Transform transformer(shape->Shape(),aTrsf,Standard_True);
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
    viewerContext->Remove(animateShape,Standard_True);
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

        dataStack.undo();
        mw->findShowTopLevelItem(this,false);
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
                    BaseItem *childItem=(BaseItem *) child(i);
                    childItem->undo();
                    mw->ui->drawingWindow->hideItem(childItem);
                    i++;
                }
            } else {
                mw->reprocess(this);
                mw->ui->drawingWindow->unselectItem(this);
            }
        }

        mw->findShowTopLevelItem(this,false);
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
        DrawingItem *parentItem=dynamic_cast<DrawingItem *>(getParentItem());
        copy_depth(parentItem);
        increase_depth();
        getParentItem()->addChild(this);

        demoteChildren();

        dataStack.undo();

        mw->reprocess(this);
        mw->ui->drawingWindow->unselectItem(this);
        mw->findShowTopLevelItem(this,false);
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
        mw->ui->drawingWindow->unselectItem(this);
        mw->findShowTopLevelItem(this,false);
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
                    BaseItem *childItem=(BaseItem *) child(i);
                    childItem->redo();
                    i++;
                }
            }
        }

        mw->reprocess(this);
        mw->insertToMapActivateItem(this);
        mw->ui->drawingWindow->unselectItem(this);
        mw->findShowTopLevelItem(this,false);
    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
        // remove this version from display and tracking
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());

        dataStack.redo();

        promoteChildren();
        getParentItem()->removeChild(this);
        mw->findShowTopLevelItem(this,true);
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
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
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
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item) mw->ui->drawingWindow->showItem(item);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(30);
}

void PathItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item) mw->ui->drawingWindow->hideItem(item);
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
    setForUndoRedo();

    // mark as delete
    ShapeData *shapeData=getShapeData();
    shapeData->setDelete();

    // parentItem
    BaseItem *parentItem=getParentItem();
    if (parentItem) {
        RootDrawingItem *rootPathItem=dynamic_cast<RootDrawingItem *>(parentItem);
        if (rootPathItem && rootPathItem->is_rootPath()) {
            rootPathItem->removeChild(this);
        }

        mw->itemChangesStack.add(this);

        // remove from display and tracking
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(this->getShape());

        mw->drawingChanged=true;
    }
}

void PathItem::undo ()
{
    std::cout << "PathItem::undo  this=" << this << std::endl; std::cout.flush();

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

        dataStack.undo();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

        // mw->ui->drawingWindow->hideItem(this);
        // mw->ui->drawingWindow->removeItemFromMap(this);
        // mw->ui->drawingWindow->deleteShape(getShape());

        // dataStack.undo();

        // ShapeData *shapeData=getShapeData();
        // if (shapeData) {
        //     Process *process=static_cast<Process *>(shapeData->getProcess());
        //     if (process) {
        //         int i=0;
        //         while (i < childCount()) {
        //             BaseItem *childItem=(BaseItem *) child(i);
        //             childItem->undo();
        //             mw->ui->drawingWindow->hideItem(childItem);
        //             i++;
        //         }
        //     } else {
        //         mw->reprocess(this);
        //         mw->ui->drawingWindow->unselectItem(this);
        //     }
        // }

        // mw->findShowTopLevelItem(this,false);
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();

        dataStack.undo();

        Path *path=static_cast<Path *>(getPath());
        if (path) path->setIsUsed(true);

        BaseItem *parentItem=getParentItem();
        if (parentItem) parentItem->addChild(this);
        //mw->path.addChild(this);

        mw->ui->drawingWindow->insertItemToMap(getShape(),this);
        mw->ui->drawingWindow->displayShape(getShape());
        mw->ui->drawingWindow->activateItem(this);
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

            // do not reverse the direction of the shape: already in expected order
            //getShapeData()->getPolywire()->reverseOrder();

            // update the shape
            setShape(getShapeData()->getPolywire()->get_AIS_Shape());
        }

        mw->ui->drawingWindow->insertItemToMap(getShape(),this);
        mw->ui->drawingWindow->displayShape(getShape());
        mw->ui->drawingWindow->activateItem(this);
    }
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
        std::cout << "   recalculating arrow shape" << std::endl; std::cout.flush();
        polywire->setHasArrows(show);
        shapeData->setShape(polywire->get_AIS_Shape());
    }

    mw->ui->drawingWindow->displayShape(getShape());
    mw->ui->drawingWindow->insertItemToMap(getShape(),this);
    mw->ui->drawingWindow->showItem(this);
    mw->ui->drawingWindow->activateItem(this);

    hasArrows=show;
}

void PathItem::redo ()
{
    std::cout << "PathItem::redo  this=" << this << std::endl; std::cout.flush();

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

        BaseItem *parentItem=getParentItem();
        if (parentItem) parentItem->addChild(this);
        //mw->path.addChild(this);

        Path *path=static_cast<Path *>(getPath());
        if (path) path->setIsUsed(true);

        mw->ui->drawingWindow->displayShape(getShape());
        mw->ui->drawingWindow->insertItemToMap(getShape(),this);
        setForeground(0,Qt::gray);
        mw->ui->drawingWindow->showItem(this);
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
        // mw->ui->drawingWindow->hideItem(this);
        // mw->ui->drawingWindow->removeItemFromMap(this);
        // mw->ui->drawingWindow->deleteShape(getShape());

        // dataStack.redo();

        // ShapeData *shapeData=getShapeData();
        // if (shapeData) {
        //     Process *process=static_cast<Process *>(shapeData->getProcess());
        //     if (process) {
        //         int i=0;
        //         while (i < childCount()) {
        //             BaseItem *childItem=(BaseItem *) child(i);
        //             childItem->redo();
        //             i++;
        //         }
        //     }
        // }

        // mw->reprocess(this);
        // mw->insertToMapActivateItem(this);
        // mw->ui->drawingWindow->unselectItem(this);
        // mw->findShowTopLevelItem(this,false);
    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
        // // remove this version from display and tracking
        // mw->ui->drawingWindow->hideItem(this);
        // mw->ui->drawingWindow->removeItemFromMap(this);
        // mw->ui->drawingWindow->deleteShape(getShape());

        // dataStack.redo();

        // promoteChildren();
        // getParentItem()->removeChild(this);
        // mw->findShowTopLevelItem(this,true);
    } else if (next->isReversePath()) {
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

            // do not reverse the direction of the shape: already in expected order
            //getShapeData()->getPolywire()->reverseOrder();

            // update the shape
            setShape(getShapeData()->getPolywire()->get_AIS_Shape());
        }

        mw->ui->drawingWindow->insertItemToMap(getShape(),this);
        mw->ui->drawingWindow->displayShape(getShape());
        mw->ui->drawingWindow->activateItem(this);
    }
}

void PathItem::reverse ()
{
    //setForUndoRedo();

    ShapeData *shapeData=getShapeData();
    shapeData->setReversePath();

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

IntegrationPathItem::IntegrationPathItem (OpenParEMg *mw_, BaseItem *parentItem_)
{
    mw=mw_;
    parentItem=parentItem_;
    itemType=14;
    setText(0,"IntegrationPathItem");
    setForeground(0,Qt::black);

    integrationPath=nullptr;
    pathItem=nullptr;
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
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item) mw->ui->drawingWindow->showItem(item);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(30);
}

void IntegrationPathItem::hide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item) mw->ui->drawingWindow->hideItem(item);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
    mw->setMenusI(34);
}

void IntegrationPathItem::showMenu (QMenu *menu)
{
    mw->removeAction=new QAction("Remove",this);
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);

    //connect(removeAction, &QAction::triggered, this, &IntegrationPathItem::removeIntegrationPathItems);
    connect(mw->showAction, &QAction::triggered, this, &IntegrationPathItem::show);
    connect(mw->hideAction, &QAction::triggered, this, &IntegrationPathItem::hide);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    menu->addAction(mw->removeAction);
}

void IntegrationPathItem::undo ()
{
    std::cout << "IntegrationPathItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // nothing to do
    } else if (shapeData->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();
        mw->ui->drawingWindow->unselectItem(this);
        mw->ui->drawingWindow->hideItem(this);
        getParentItem()->removeChild(this);

        // remove linked item
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
        }

        dataStack.undo();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
    }
}

void IntegrationPathItem::redo ()
{
    std::cout << "IntegrationPathItem::redo  this=" << this << std::endl; std::cout.flush();

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

        getParentItem()->addChild(this);

        mw->ui->drawingWindow->showItem(this);
        mw->ui->drawingWindow->activateItem(this);

        // add linked item
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->push_linkedItem(this);
        }
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
    }
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
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
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
    setForeground(0,Qt::black);

    if (pathItem) {

        // process the path
        pathItem->push_linkedItem(this);
        mw->convertPathToFace(pathItem);
        pathItem->showArrows(false);

        Handle(AIS_Shape) shape=pathItem->getShape();
        if (!shape.IsNull()) {
            shape->SetTransparency(0);
            shape->SetMaterial(Graphic3d_NameOfMaterial_Plastered);
            if (boundary_type_ == 0) shape->SetColor(Quantity_NOC_GREENYELLOW);
            else if (boundary_type_ == 1) shape->SetColor(Quantity_NOC_CYAN);
            else if (boundary_type_ == 2) shape->SetColor(Quantity_NOC_GOLDENROD);
            else if (boundary_type_ == 3) shape->SetColor(Quantity_NOC_CORNFLOWERBLUE);
            mw->setShaded(shape);
        }
    }

    // type

    BaseItem *itemType=new BaseItem(mw,this);
    itemType->setFlags(itemType->flags() | Qt::ItemIsEditable);
    itemType->setToolTip(0,"Boundary type.");
    addChild(itemType);

    CustomComboBox *comboType=new CustomComboBox();
    const QSignalBlocker blockerZdef(comboType);
    comboType->addItem("PEC");
    comboType->addItem("PMC");
    comboType->addItem("Zs");
    comboType->addItem("Radiation");
    comboType->set_portItem(nullptr);
    comboType->set_boundaryItem(this);
    comboType->set_type(2);

    comboType->setCurrentIndex(boundary_type_);
    mw->ui->drawingItemTree->setItemWidget(itemType,0,comboType);

    QObject::connect(comboType,&CustomComboBox::CustomCurrentIndexChanged,&comboIndexChanged);
    QObject::connect(comboType,&CustomComboBox::CustomCurrentIndexChanged,mw->relay,&Relay::setMenus);
    QObject::connect(comboType,&CustomComboBox::CustomCurrentIndexChanged,mw->relay,&Relay::updateViewer);

    // boundary type dependent data

    // material

    BaseItem *itemMaterial=new BaseItem(mw,this);
    itemMaterial->setFlags(itemMaterial->flags() | Qt::ItemIsEditable);
    itemMaterial->setToolTip(0,"Boundary material.");
    addChild(itemMaterial);

    CustomComboBox *comboMaterial=new CustomComboBox();
    const QSignalBlocker blockerMaterial(comboMaterial);
    if (mw->materialDatabase) {
        long unsigned int i=0;
        while (i < mw->materialDatabase->get_size()) {
            Material *material=mw->materialDatabase->get_material(i);
            // Todo: add only conductors
            comboMaterial->addItem(QString::fromStdString(material->get_name()->get_value()));
            i++;
        }
    }
    mw->ui->drawingItemTree->setItemWidget(itemMaterial,0,comboMaterial);

    QObject::connect(comboMaterial,&CustomComboBox::CustomCurrentTextChanged, &comboTextChanged);
    QObject::connect(comboMaterial,&CustomComboBox::CustomCurrentIndexChanged,mw->relay,&Relay::setMenus);

    // wave impedance

    BaseItem *itemWaveImpedance=new BaseItem(mw,this);
    itemWaveImpedance->setFlags(itemWaveImpedance->flags() | Qt::ItemIsEditable);
    itemWaveImpedance->setToolTip(0,"Wave impedance in Ohms.");
    addChild(itemWaveImpedance);

    QDoubleValidator doubleValidator;
    doubleValidator.setBottom(0);

    CustomLineEdit *textWaveImpedance=new CustomLineEdit();
    const QSignalBlocker blockerWaveImpedance(textWaveImpedance);
    textWaveImpedance->setText(QString::number(wave_impedance_));
    textWaveImpedance->setValidator(&doubleValidator);
    textWaveImpedance->set_baseItem(this);
    mw->ui->drawingItemTree->setItemWidget(itemWaveImpedance,0,textWaveImpedance);

    //xxx
    QObject::connect(textWaveImpedance,&CustomLineEdit::CustomEditFinished,&textValueChanged);


    // set initial visibility
    std::cout << "boundary_type_=" << boundary_type_ << std::endl; std::cout.flush();
    if (boundary_type_ == 0) {  // PEC
        itemWaveImpedance->setHidden(true);
        itemMaterial->setHidden(true);
    } else if (boundary_type_ == 1) {  // PMC
        itemWaveImpedance->setHidden(true);
        itemMaterial->setHidden(true);
    } else if (boundary_type_ == 2) {  // Zs
        itemWaveImpedance->setHidden(true);
    } else if (boundary_type_ == 3) {  // radiation
        std::cout << "   itemMaterial=" << itemMaterial << std::endl; std::cout.flush();
        itemMaterial->setHidden(true);
    }

    // set the CustomTreeWidget items so they can be hidden as needed depending on type
    comboType->set_itemMaterial(itemMaterial);
    comboType->set_itemWaveImpedance(itemWaveImpedance);

    // set the shape color
    PathItem *pathItem=getPathItem();
    if (pathItem) {
        Handle(AIS_Shape) shape=pathItem->getShape();
        if (!shape.IsNull()) {
            shape->SetTransparency(0);
            shape->SetMaterial(Graphic3d_NameOfMaterial_Plastered);
            if (boundary_type_ == 0) shape->SetColor(Quantity_NOC_GREENYELLOW);
            else if (boundary_type_ == 1) shape->SetColor(Quantity_NOC_CYAN);
            else if (boundary_type_ == 2) shape->SetColor(Quantity_NOC_GOLDENROD);
            else if (boundary_type_ == 3) shape->SetColor(Quantity_NOC_CORNFLOWERBLUE);
            mw->setShaded(shape);
        }
    }

    mw->itemChangesStack.add(this);
}

// void BoundaryItem::startItemChange () {mw->itemChangesStack.startNew();}
// void BoundaryItem::addItemChange () {mw->itemChangesStack.add(this);}

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
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
}

void BoundaryItem::del ()
{
    //setForUndoRedo();

    // mark as delete
    ShapeData *shapeData=getShapeData();
    shapeData->setDelete();

    // parentItem
    BaseItem *parentItem=getParentItem();
    if (parentItem) {
        RootBoundaryItem *rootBoundaryItem=dynamic_cast<RootBoundaryItem *>(parentItem);
        if (rootBoundaryItem && rootBoundaryItem->is_rootBoundary()) {
            rootBoundaryItem->removeChild(this);
        }

        // restore the arrows on the path
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            mw->ui->drawingWindow->showItem(pathItem);
            mw->ui->drawingWindow->activateItem(pathItem);
            mw->ui->drawingWindow->selectItem(pathItem);
            pathItem->showArrows(true);
        }

        mw->itemChangesStack.add(this);
        mw->drawingChanged=true;
    }
}

void BoundaryItem::undo ()
{
    std::cout << "BoundaryItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // nothing to do
    } else if (shapeData->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();

        // restore the arrows on the path
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            mw->ui->drawingWindow->showItem(pathItem);
            mw->ui->drawingWindow->activateItem(pathItem);
            mw->ui->drawingWindow->selectItem(pathItem);
            pathItem->showArrows(true);
        }

        // remove the item
        getParentItem()->removeChild(this);

        dataStack.undo();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

        dataStack.undo();

        // remove all children
        while (childCount() > 0) {
            BaseItem *baseItem=dynamic_cast<BaseItem *>(child(0));
            if (baseItem) {
                int index=indexOfChild(baseItem);
                this->takeChild(index);
                delete baseItem;
            }
        }

        populate(nullptr);

    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();

        dataStack.undo();

        mw->boundary->addChild(this);
        setForeground(0,Qt::gray);
        mw->ui->drawingWindow->showItem(this);

        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->showArrows(false);
        }

        // delete any previous children
        while (childCount() > 0) {
            QTreeWidgetItem* child=takeChild(0);
            delete child;
        }

        // rebuild the item from scratch
        populate(nullptr);
    }
}

void BoundaryItem::redo ()
{
    std::cout << "BoundaryItem::redo  this=" << this << std::endl; std::cout.flush();

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

        // remove all children
        while (childCount() > 0) {
            BaseItem *baseItem=dynamic_cast<BaseItem *>(child(0));
            if (baseItem) {
                int index=indexOfChild(baseItem);
                this->takeChild(index);
                delete baseItem;
            }
        }

        populate(nullptr);

        mw->boundary->addChild(this);

    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
        dataStack.redo();

        // remove all children
        while (childCount() > 0) {
            BaseItem *baseItem=dynamic_cast<BaseItem *>(child(0));
            if (baseItem) {
                int index=indexOfChild(baseItem);
                this->takeChild(index);
                delete baseItem;
            }
        }

        populate(nullptr);

        mw->port->addChild(this);
    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();

        dataStack.redo();

        // remove the item
        getParentItem()->removeChild(this);

        // restore the arrows on the path
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            mw->ui->drawingWindow->showItem(pathItem);
            mw->ui->drawingWindow->activateItem(pathItem);
            mw->ui->drawingWindow->selectItem(pathItem);
            pathItem->showArrows(true);
        }
    }
}

void BoundaryItem::populate (Boundary *boundary)
{
    //std::cout << "BoundaryItem::populate" << std::endl; std::cout.flush();

    QDoubleValidator doubleValidator;
    doubleValidator.setBottom(0);

    int boundary_type;
    QString boundary_material;
    double wave_impedance;
    ShapeData *shapeData=shapeData=getShapeData();

    boundary_type=shapeData->get_boundary_type();
    boundary_material=shapeData->get_boundary_material();
    wave_impedance=shapeData->get_wave_impedance();


    // type

    BaseItem *itemType=new BaseItem(mw,this);
    addChild(itemType);

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

    // boundary type dependent data

    // material

    BaseItem *itemMaterial=new BaseItem(mw,this);
    itemMaterial->setFlags(itemMaterial->flags() | Qt::ItemIsEditable);
    itemMaterial->setToolTip(0,"Boundary material.");
    addChild(itemMaterial);

    CustomComboBox *comboMaterial=new CustomComboBox();
    const QSignalBlocker blockerMaterial(comboMaterial);
    if (mw->materialDatabase) {
        long unsigned int i=0;
        while (i < mw->materialDatabase->get_size()) {
            Material *material=mw->materialDatabase->get_material(i);
            // Todo: add only conductors
            comboMaterial->addItem(QString::fromStdString(material->get_name()->get_value()));
            i++;
        }
    }
    mw->ui->drawingItemTree->setItemWidget(itemMaterial,0,comboMaterial);

    QObject::connect(comboMaterial,&CustomComboBox::CustomCurrentTextChanged, &comboTextChanged);
    QObject::connect(comboMaterial,&CustomComboBox::CustomCurrentIndexChanged,mw->relay,&Relay::setMenus);

    // wave impedance

    BaseItem *itemWaveImpedance=new BaseItem(mw,this);
    itemWaveImpedance->setFlags(itemWaveImpedance->flags() | Qt::ItemIsEditable);
    itemWaveImpedance->setToolTip(0,"Wave impedance in Ohms.");
    addChild(itemWaveImpedance);

    CustomLineEdit *textWaveImpedance=new CustomLineEdit();
    const QSignalBlocker blockerWaveImpedance(textWaveImpedance);
    textWaveImpedance->setText(QString::number(wave_impedance));
    textWaveImpedance->setValidator(&doubleValidator);
    textWaveImpedance->set_baseItem(this);
    mw->ui->drawingItemTree->setItemWidget(itemWaveImpedance,0,textWaveImpedance);

    //xxx
    QObject::connect(textWaveImpedance,&CustomLineEdit::CustomEditFinished,&textValueChanged);


    // set initial visibility
    std::cout << "boundary_type=" << boundary_type << std::endl; std::cout.flush();
    if (boundary_type == 0) {
        itemWaveImpedance->setHidden(true);
        itemMaterial->setHidden(true);
    } else if (boundary_type == 1) {
        itemWaveImpedance->setHidden(true);
        itemMaterial->setHidden(true);
    } else if (boundary_type == 2) {
        itemWaveImpedance->setHidden(true);
    } else if (boundary_type == 3) {
        std::cout << "   itemMaterial=" << itemMaterial << std::endl; std::cout.flush();
        itemMaterial->setHidden(true);
    }

    // set the CustomTreeWidget items so they can be hidden as needed depending on type
    comboType->set_itemMaterial(itemMaterial);
    comboType->set_itemWaveImpedance(itemWaveImpedance);

    // set the shape color
    PathItem *pathItem=getPathItem();
    if (pathItem) {
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

    // if (boundary->is_default_boundary()) {
    //     QString textDefault="default";
    //     BaseItem *itemDefault=new BaseItem(0);
    //     itemDefault->setMW(mw);
    //     itemDefault->setParentItem(this);
    //     itemDefault->setText(0,textDefault);
    //     addChild(itemDefault);
    // }
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
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
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
    setForeground(0,Qt::black);

    setToolTip(0,"Port name.");

    if (pathItem) {

        // process the path
        pathItem->push_linkedItem(this);
        mw->convertPathToFace(pathItem);
        pathItem->showArrows(false);

        Handle(AIS_Shape) shape=pathItem->getShape();
        if (!shape.IsNull()) {
            shape->SetColor(Quantity_NOC_MINTCREAM);
            shape->SetTransparency(0.25);
            shape->SetMaterial(Graphic3d_NameOfMaterial_Plastered);
            mw->setShaded(shape);
        }
    }

    // impedance definition
    addImpedanceDefinitionItem();

    // impedance calculation
    addImpedanceCalculationItem();

    // add one default mode since at least one mode is required
    ModeItem *newModeItem=new ModeItem(mw,this);
    addChild(newModeItem);
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
    std::cout << "place 3" << std::endl; std::cout.flush();

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
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
}

void PortItem::del ()
{
    //setForUndoRedo();

    // mark as delete
    ShapeData *shapeData=getShapeData();
    shapeData->setDelete();

    // parentItem
    BaseItem *parentItem=getParentItem();
    if (parentItem) {
        RootPortItem *rootPortItem=dynamic_cast<RootPortItem *>(parentItem);
        if (rootPortItem && rootPortItem->is_rootPort()) {
            rootPortItem->removeChild(this);
        }

        // restore the arrows on the path
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            mw->ui->drawingWindow->showItem(pathItem);
            mw->ui->drawingWindow->activateItem(pathItem);
            mw->ui->drawingWindow->selectItem(pathItem);
            pathItem->showArrows(true);
        }

        mw->drawingChanged=true;
    }
}

void PortItem::populate (Port *port)
{
    //std::cout << "PortItem::populate" << std::endl; std::cout.flush();

    QString impedance_definition;
    QString impedance_calculation;
    ShapeData *shapeData;
    if (port) {
        shapeData=getShapeData();
        shapeData->set_impedance_definition(port->get_impedance_definition());
        shapeData->set_impedance_calculation(port->get_impedance_calculation());
    } else {
        shapeData=getShapeData();
    }

    impedance_definition=shapeData->get_impedance_definition();
    impedance_calculation=shapeData->get_impedance_calculation();

    // impedance definition

    BaseItem *itemImpedanceDefinition=new BaseItem(mw,this);
    itemImpedanceDefinition->set_itemType(6);
    itemImpedanceDefinition->setFlags(itemImpedanceDefinition->flags() & ~Qt::ItemIsSelectable);
    itemImpedanceDefinition->setToolTip(0,"Impedance definition for calculating characteristic impedance.");
    insertChild(0,itemImpedanceDefinition);

    //CustomComboBox *comboZdef=port->get_comboZdef();
    //comboZdef=new CustomComboBox();
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

    // impedance calculation

    BaseItem *itemImpedanceCalculation=new BaseItem(mw,this);
    itemImpedanceCalculation->set_itemType(7);
    itemImpedanceCalculation->setFlags(itemImpedanceCalculation->flags() & ~Qt::ItemIsSelectable);
    itemImpedanceCalculation->setToolTip(0,"Impedance calculation using modal or line integration paths.");
    insertChild(1,itemImpedanceCalculation);

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

// void PortItem::addModeItem ()
// {
//     int Sport=0;
//     ModeItem *newModeItem=new ModeItem(0);
//     if (newModeItem) {
//         ShapeData *newShapeData=newModeItem->getShapeData()->copyCreate();
//         newShapeData->setCreate();
//         newModeItem->setMW(mw);
//         newModeItem->setParentItem(this);
//         newModeItem->set_itemType(5);
//         newModeItem->setToolTip(0,"Mode and its net name.");
//         newModeItem->addShapeData(newShapeData);

//         newModeItem->setPortItem(this);
//         addChild(newModeItem);

//         // set the S-parameter port number to the next available
//         mw->largestSportNumber(&(mw->port),&Sport);
//         Sport++;
//         newShapeData->set_Sport(Sport);

//         // unique net name based on Sport number
//         QString net="net";
//         net.append(QString::number(newShapeData->get_Sport()));
//         newModeItem->setText(0,net);

//         mw->itemChangesStack.add(newModeItem);
//     }

//     BaseItem *newSportItem=nullptr;
//     if (newModeItem) {
//         newSportItem=new BaseItem(0);
//         if (newSportItem) {
//             ShapeData *newShapeData=newSportItem->getShapeData()->copyCreate();
//             newShapeData->setCreate();
//             newSportItem->setMW(mw);
//             newSportItem->setParentItem(newModeItem);
//             newSportItem->setText(0,"S Port");
//             newSportItem->setToolTip(0,"S-parameter port number for the mode.");
//             newSportItem->set_itemType(8);
//             newSportItem->addShapeData(newShapeData);
//             newModeItem->addChild(newSportItem);
//         }
//         mw->itemChangesStack.add(newSportItem);
//     }

//     BaseItem *newSportNumberItem=nullptr;
//     if (newSportItem) {
//         newSportNumberItem=new BaseItem(0);
//         if (newSportNumberItem) {
//             ShapeData *newShapeData=newSportNumberItem->getShapeData()->copyCreate();
//             newShapeData->setCreate();
//             newSportNumberItem->setMW(mw);
//             newSportNumberItem->setParentItem(newSportItem);
//             newSportNumberItem->set_itemType(9);
//             newSportNumberItem->addShapeData(newShapeData);
//             newSportItem->addChild(newSportNumberItem);

//             CustomSpinBox *sportNumber=new CustomSpinBox();
//             sportNumber->set_itemTracker(mw->ui->drawingWindow->get_itemTracker());
//             sportNumber->set_modeItem(newModeItem);
//             sportNumber->setMinimum(1);
//             sportNumber->setValue(Sport);
//             mw->ui->drawingItemTree->setItemWidget(newSportNumberItem,0,sportNumber);

//             QObject::connect(sportNumber,&CustomSpinBox::CustomValueChanged,&spinValueChanged);
//             QObject::connect(sportNumber,&CustomSpinBox::CustomValueChanged,mw->relay,&Relay::setMenus);

//             mw->itemChangesStack.add(newSportNumberItem);
//         }
//     }

//     if (newModeItem) {
//         BaseItem *newVoltageItem=new BaseItem(0);
//         if (newVoltageItem) {
//             ShapeData *newShapeData=newVoltageItem->getShapeData()->copyCreate();
//             newShapeData->setCreate();
//             newVoltageItem->setMW(mw);
//             newVoltageItem->setParentItem(newModeItem);
//             newVoltageItem->set_itemType(10);
//             newVoltageItem->setText(0,"voltage");
//             newVoltageItem->setToolTip(0,"Voltage integration path.");
//             newVoltageItem->addShapeData(newShapeData);
//             newModeItem->addChild(newVoltageItem);

//             mw->itemChangesStack.add(newVoltageItem);
//         }

//         BaseItem *newCurrentItem=new BaseItem(0);
//         if (newCurrentItem) {
//             ShapeData *newShapeData=newCurrentItem->getShapeData()->copyCreate();
//             newShapeData->setCreate();
//             newCurrentItem->setMW(mw);
//             newCurrentItem->setParentItem(newModeItem);
//             newCurrentItem->set_itemType(11);
//             newCurrentItem->setText(0,"current");
//             newCurrentItem->setToolTip(0,"Current integration path.");
//             newCurrentItem->addShapeData(newShapeData);
//             newModeItem->addChild(newCurrentItem);

//             mw->itemChangesStack.add(newCurrentItem);
//         }
//     }
// }

void PortItem::undo ()
{
    std::cout << "PortItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // nothing to do
    } else if (shapeData->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();

        // restore the arrows on the path
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            mw->ui->drawingWindow->showItem(pathItem);
            mw->ui->drawingWindow->activateItem(pathItem);
            mw->ui->drawingWindow->selectItem(pathItem);
            pathItem->showArrows(true);
        }

        // remove the item
        getParentItem()->removeChild(this);

        dataStack.undo();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
        dataStack.undo();

        // remove some children
        int i=0;
        while (i < childCount()) {
            BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
            if (baseItem && baseItem->is_impedanceCalculation()) {
                int index=indexOfChild(baseItem);
                this->takeChild(index);
                delete baseItem;
                break;
            }
            i++;
        }

        i=0;
        while (i < childCount()) {
            BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
            if (baseItem && baseItem->is_impedanceDefinition()) {
                int index=indexOfChild(baseItem);
                this->takeChild(index);
                delete baseItem;
                break;
            }
            i++;
        }

        populate(nullptr);
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();

        // dataStack.undo();

        // // rebuild the item from scratch
        // Port *aPort=getPort();
        // if (aPort) {
        //     // remove all children
        //     foreach(auto i, takeChildren()) delete i;

        //     // draw
        //     aPort->draw(mw->relay,&(mw->projData),mw->boundaryDatabase,mw,mw->ui->drawingWindow,mw->ui->drawingItemTree,&(mw->path),&(mw->port),this);
        // }
    }
}

void PortItem::redo ()
{
    std::cout << "PortItem::redo  this=" << this << std::endl; std::cout.flush();

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


        // // remove some children
        // int i=0;
        // while (i < childCount()) {
        //     BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        //     if (baseItem && baseItem->is_impedanceCalculation()) {
        //         int index=indexOfChild(baseItem);
        //         this->takeChild(index);
        //         delete baseItem;
        //         break;
        //     }
        //     i++;
        // }

        // i=0;
        // while (i < childCount()) {
        //     BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        //     if (baseItem && baseItem->is_impedanceDefinition()) {
        //         int index=indexOfChild(baseItem);
        //         this->takeChild(index);
        //         delete baseItem;
        //         break;
        //     }
        //     i++;
        // }

        // populate(nullptr);

        mw->port->addChild(this);
        restoreWidgets(this);

        //xxx
        //{mw->expandAllItems(); mw->ui->drawingWindow->updateViewer(); QMessageBox mb; mb.critical(nullptr, "Debug", "place y3");}
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
        dataStack.redo();

        // remove some children
        int i=0;
        while (i < childCount()) {
            BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
            if (baseItem && baseItem->is_impedanceCalculation()) {
                int index=indexOfChild(baseItem);
                this->takeChild(index);
                delete baseItem;
                break;
            }
            i++;
        }

        i=0;
        while (i < childCount()) {
            BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
            if (baseItem && baseItem->is_impedanceDefinition()) {
                int index=indexOfChild(baseItem);
                this->takeChild(index);
                delete baseItem;
                break;
            }
            i++;
        }

        populate(nullptr);
    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();

        // restore the arrows on the path
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            mw->ui->drawingWindow->showItem(pathItem);
            mw->ui->drawingWindow->activateItem(pathItem);
            mw->ui->drawingWindow->selectItem(pathItem);
            pathItem->showArrows(true);
        }

        // delete the item
        getParentItem()->removeChild(this);
    }
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

// void ModeItem::startItemChange () {mw->itemChangesStack.startNew();}
// void ModeItem::addItemChange () {mw->itemChangesStack.add(this);}

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
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_sport()) {
            mw->ui->drawingWindow->showItem(item);

            int j=0;
            while (j < item->childCount()) {
                BaseItem *child=(BaseItem *) item->child(j);
                if (child->is_voltage() || child->is_current()) {
                    //child->setForeground(0,Qt::black);
                    int k=0;
                    while (k < child->childCount()) {
                        BaseItem *grandChild=(BaseItem *) child->child(k);
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
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_sport()) {
            int j=0;
            while (j < item->childCount()) {
                BaseItem *child=(BaseItem *) item->child(j);
                if (child->is_voltage() || child->is_current()) {
                    //child->setForeground(0,Qt::gray);
                    int k=0;
                    while (k < child->childCount()) {
                        BaseItem *grandChild=(BaseItem *) child->child(k);
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
    mw->itemChangesStack.startNew();
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
    connect(mw->deleteAction, &QAction::triggered, this, &ModeItem::del);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (mw->ui->drawingWindow->get_selectedItems_count() == 1) menu->addAction(mw->renameAction);
    if (isValidDelete()) menu->addAction(mw->deleteAction);
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
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

    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
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

// void SportNumberItem::startItemChange () {mw->itemChangesStack.startNew();}
// void SportNumberItem::addItemChange () {mw->itemChangesStack.add(this);}

bool SportNumberItem::isValidShow () {return false;}
bool SportNumberItem::isValidHide () {return false;}
void SportNumberItem::show () {}
void SportNumberItem::hide () {}

void SportNumberItem::showMenu (QMenu *menu)
{
    // mw->expandAllAction=new QAction("Expand All",this);
    // mw->collapseAllAction=new QAction("Collapse All",this);

    // connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    // connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    // if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    // if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
}

void SportNumberItem::undo ()
{
    std::cout << "SportNumberItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // nothing to do
    } else if (shapeData->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

        dataStack.undo();

        CustomSpinBox *sportNumber=dynamic_cast<CustomSpinBox *>(mw->ui->drawingItemTree->itemWidget(this,0));
        const QSignalBlocker blocker(sportNumber);
        ShapeData *shapeData=getShapeData();
        sportNumber->setValue(shapeData->get_Sport());
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
    }
}

void SportNumberItem::redo ()
{
    std::cout << "SportNumberItem::redo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    ShapeData *next=shapeData->getNext();
    if (!next) return;

    if (next->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // should not occur
    } else if (next->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

        dataStack.redo();

        CustomSpinBox *sportNumber=dynamic_cast<CustomSpinBox *>(mw->ui->drawingItemTree->itemWidget(this,0));
        const QSignalBlocker blocker(sportNumber);
        ShapeData *shapeData=getShapeData();
        sportNumber->setValue(shapeData->get_Sport());
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
    }
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
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item) {
            if (item->is_voltage() || item->is_current()) {
                mw->ui->drawingWindow->showItem(item);
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
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item) {
            if (item->is_voltage() || item->is_current()) {
                mw->ui->drawingWindow->hideItem(item);
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
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
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
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item) {
            if (item->is_voltage() || item->is_current()) {VIitem=item; VIcount++;}

            PathItem *pathItem=dynamic_cast<PathItem *>(item);
            if (pathItem && pathItem->is_path()) pathCount++;
        }
        i++;
    }

    if (VIcount != 1) return false;
    if (pathCount == 0) return false;

    // check that the paths are within the port

    ModeItem *modeItem=dynamic_cast<ModeItem *>(VIitem->QTreeWidgetItem::parent());
    PortItem *portItem=dynamic_cast<PortItem *>(modeItem->QTreeWidgetItem::parent());

    // port outline
    PathItem *pathItem=portItem->getPathItem();
    if (pathItem) return false;
    Path *portPath=pathItem->getPath();
    if (!portPath) return false;

    i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item) {
            PathItem *pathItem=dynamic_cast<PathItem *>(item);
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

void VIItem::insertSelectedPath ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item) {
            if (item->is_voltage() || item->is_current()) mw->insertIntegrationPath(item);
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
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_rootMesh()) {
            int j=0;
            while (j < item->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(item->child(j));
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
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_rootMesh()) {
            int j=0;
            while (j < item->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(item->child(j));
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
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
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
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_mesh()) {
            mw->ui->drawingWindow->showItem(item);
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
        BaseItem *item=mw->ui->drawingWindow->get_selectedItem(i);
        if (item && item->is_mesh()) {
            mw->ui->drawingWindow->hideItem(item);
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
