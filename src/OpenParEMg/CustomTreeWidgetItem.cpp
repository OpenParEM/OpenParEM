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

BaseItem* BaseItem::getRootParent ()
{
    int watchDog=0;
    BaseItem *baseItem=this;

    bool loop=true;
    while (loop) {
        if (baseItem->is_rootDrawing()) break;
        else if (baseItem->is_rootPath()) break;
        else if (baseItem->is_rootPort()) break;
        else if (baseItem->is_rootBoundary()) break;
        else if (baseItem->is_rootMesh()) break;

        baseItem=baseItem->parentItem;

        watchDog++;
        if (watchDog == 100000) {
            break;
            baseItem=nullptr;
        }
    }

    return baseItem;
}

void BaseItem::showDisplayStatus ()
{
    std::cout << "Item " << text(0).toStdString();
    if (foreground(0) == Qt::gray) std::cout << "  hidden";
    else if (foreground(0) == Qt::black) std::cout << "  shown";
    else std::cout << "  invalid";
    if (mw->ui->drawingWindow->isDisplayed(getShape())) {
        std::cout << "  displayed";
    } else std::cout << "  not displayed";
    std::cout << std::endl; std::cout.flush();
}

void BaseItem::alignForegroundColor ()
{
    if (mw->ui->drawingWindow->isDisplayed(getShape())) {
        setForeground(0,Qt::black);
    } else {
        setForeground(0,Qt::gray);
    }
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

void BaseItem::restoreWidgets ()
{
    std::cout << "BaseItem::restoreWidgets" << std::endl; std::cout.flush();

    BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(this);
    if (boundaryItem) {
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

    PortItem *portItem=dynamic_cast<PortItem *>(this);
    if (portItem) {
        portItem->setSolidColor();
    }

    if (is_impedanceCalculation()) {
        PortItem *portItem=dynamic_cast<PortItem *>(getParentItem());
        if (portItem) {
            ShapeData *shapeData=portItem->getShapeData();
            QString impedance_calculation=shapeData->get_impedance_calculation();
            portItem->insertImpedanceCalculationWidget(this,impedance_calculation);
        }
    }

    if (is_impedanceDefinition()) {
        PortItem *portItem=dynamic_cast<PortItem *>(getParentItem());
        if (portItem) {
            ShapeData *shapeData=portItem->getShapeData();
            QString impedance_definition=shapeData->get_impedance_definition();
            portItem->insertImpedanceDefinitionWidget(this,impedance_definition);
        }
    }

    if (is_sportNumber()) {
        SportNumberItem *sportNumberItem=dynamic_cast<SportNumberItem *>(this);
        if (sportNumberItem) {
            ShapeData *shapeData=getShapeData();
            int Sport=shapeData->get_Sport();
            sportNumberItem->insertSportNumberWidget(Sport);
        }
    }

    std::cout << "place 1" << std::endl; std::cout.flush();
    if (is_scaleValue()) {
        std::cout << "place 2" << std::endl; std::cout.flush();
        ScaleValueItem *scaleValueItem=dynamic_cast<ScaleValueItem *>(this);
        if (scaleValueItem) {
            std::cout << "place 3" << std::endl; std::cout.flush();
            ShapeData *shapeData=getShapeData();
            double scale=shapeData->get_scale();
            scaleValueItem->insertScaleValueWidget(scale);
        }
    }

    // process children
    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->restoreWidgets();
        i++;
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
    //std::cout << "BaseItem::expandToItem  text(0)=" << text(0).toStdString() << std::endl; std::cout.flush();

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
    //std::cout << "BaseItem::expandToItem1  text(0)=" << text(0).toStdString() << std::endl; std::cout.flush();

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

    // currentItem->alignForegroundColor();
    // if (currentItem->foreground(0) == Qt::black) mw->ui->drawingWindow->hideItem(currentItem);
    // if (currentItem->foreground(0) == Qt::gray) {
    //     mw->ui->drawingWindow->showItem(currentItem);
    //     mw->ui->drawingWindow->hideItem(currentItem);
    // }
    mw->ui->drawingWindow->showItem(currentItem);

    return currentItem;
}

void BaseItem::undo ()
{
    std::cout << "BaseItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    mw->ui->drawingWindow->unselectAllItems();

    if (shapeData->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // nothing to do
    } else if (shapeData->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());
        promoteChildren();
        getParentItem()->removeChild(this);
        mw->ui->drawingWindow->showItem(this);
        dataStack.undo();
        //expandToItemPlus1();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
        dataStack.undo();
        restoreWidgets();
        //expandToItemPlus1();
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
        dataStack.undo();

        Handle(AIS_Shape) shape=getShape();
        if (!shape.IsNull()) {
            mw->ui->drawingWindow->displayShape(shape);
            mw->ui->drawingWindow->insertItemToMap(shape,this);
        }

        getParentItem()->addChild(this);
        demoteChildren();

        mw->ui->drawingWindow->showItem(this);
        mw->ui->drawingWindow->activateItem(this);
        restoreWidgets();
        //expandToItemPlus1();
    } else if (shapeData->isChangeName()) {
        std::cout << "   isChangeName" << std::endl; std::cout.flush();

        dataStack.undo();
        setText(0,getShapeData()->get_name());

        //expandToItem();
    }

    // add or remove integration path scales as needed
    IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(this);
    if (integrationPathItem) {
        VIItem *viItem=dynamic_cast<VIItem *>(integrationPathItem->getParentItem());
        if (viItem) viItem->addRemoveScale();
    }
}

void BaseItem::redo ()
{
    std::cout << "BaseItem::redo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    mw->ui->drawingWindow->unselectAllItems();

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
        demoteChildren();
        mw->ui->drawingWindow->showItem(this);
        restoreWidgets();
        //expandToItemPlus1();
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();
        dataStack.redo();
        restoreWidgets();
        //expandToItemPlus1();
    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();
        mw->ui->drawingWindow->unselectItem(this);
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());

        promoteChildren();
        getParentItem()->removeChild(this);
        mw->ui->drawingWindow->showItem(this);

        dataStack.redo();
    } else if (next->isChangeName()) {
        std::cout << "   isChangeName" << std::endl; std::cout.flush();

        dataStack.redo();
        setText(0,getShapeData()->get_name());

        //expandToItem();
    }

    // add or remove integration path scales as needed
    IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(this);
    if (integrationPathItem) {
        VIItem *viItem=dynamic_cast<VIItem *>(integrationPathItem->getParentItem());
        if (viItem) viItem->addRemoveScale();
    }
}

void BaseItem::save (std::ofstream *out)
{
    if (is_impedanceDefinition()) {
        CustomComboBox *comboZdef=dynamic_cast<CustomComboBox *>(mw->ui->drawingItemTree->itemWidget(this,0));
        if (comboZdef) {
            *out << "   impedance_definition=" << comboZdef->currentText().toStdString() << std::endl;
        }
    } else if (is_impedanceCalculation()) {
        CustomComboBox *comboZcalc=dynamic_cast<CustomComboBox *>(mw->ui->drawingItemTree->itemWidget(this,0));
        if (comboZcalc) {
            *out << "   impedance_calculation=" << comboZcalc->currentText().toStdString() << std::endl;
        }
    } else if (is_boundaryType()) {
        CustomComboBox *comboType=dynamic_cast<CustomComboBox *>(mw->ui->drawingItemTree->itemWidget(this,0));
        if (comboType) {
            std::string boundaryType;
            if (comboType->currentText().compare("PEC") == 0) boundaryType="perfect_electric_conductor";
            else if (comboType->currentText().compare("PMC") == 0) boundaryType="perfect_magnetic_conductor";
            else if (comboType->currentText().compare("Zs") == 0) boundaryType="surface_impedance";
            else if (comboType->currentText().compare("Radiation") == 0) boundaryType="radiation";
            else boundaryType="invalid";
            *out << "   type=" << boundaryType << std::endl;
        }
    } else if (is_boundaryMaterial()) {
        CustomComboBox *itemMaterial=dynamic_cast<CustomComboBox *>(mw->ui->drawingItemTree->itemWidget(this,0));
        if (itemMaterial) {
            if (itemMaterial->count() > 0) {
                if (itemMaterial->currentText().compare("none") != 0) {
                    *out << "   material=" << itemMaterial->currentText().toStdString() << std::endl;
                }
            }
        }
    } else if (is_boundaryWaveImpedance()) {
        CustomLineEdit *textWaveImpedance=dynamic_cast<CustomLineEdit *>(mw->ui->drawingItemTree->itemWidget(this,0));
        if (textWaveImpedance) {
            *out << "   wave_impedance=" << textWaveImpedance->text().toStdString() << std::endl;
        }
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
    std::cout << "ScaleValueItem::insertScaleValueWidget  scale=" << scale << std::endl; std::cout.flush();

    CustomLineEdit *scaleEdit=new CustomLineEdit();
    const QSignalBlocker blocker(scaleEdit);
    scaleEdit->setText(QString::number(scale,'g'));
    scaleEdit->set_itemTracker(mw->ui->drawingWindow->get_itemTracker());
    scaleEdit->set_doubleValidator();
    scaleEdit->set_baseItem(this);
    mw->ui->drawingItemTree->setItemWidget(this,0,scaleEdit);

    QObject::connect(scaleEdit,&CustomLineEdit::CustomEditFinished,&textValueChanged);
}

void ScaleValueItem::undo ()
{
    std::cout << "ScaleValueItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isEdit()) {
        dataStack.undo();
        restoreWidgets();
    } else {
        BaseItem::undo();
    }
}

void ScaleValueItem::redo ()
{
    std::cout << "ScaleValueItem::redo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    ShapeData *next=shapeData->getNext();
    if (!next) return;

    if (next->isEdit()) {
        dataStack.redo();
        restoreWidgets();
    } else {
        BaseItem::redo();
    }
}

void ScaleValueItem::save (std::ofstream *out)
{
    CustomLineEdit *scaleValue=dynamic_cast<CustomLineEdit *>(mw->ui->drawingItemTree->itemWidget(this,0));
    if (scaleValue) {
        std::cout << "         scale=" << scaleValue->text().toStdString() << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
// RootDrawingItem
////////////////////////////////////////////////////////////////////////////////

bool RootDrawingItem::isValidShow ()
{
    int i=0;
    while (i < mw->drawing->childCount()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(mw->drawing->child(i));
        if (drawingItem && drawingItem->isValidShow()) return true;
        i++;
    }
    return false;
}

bool RootDrawingItem::isValidHide ()
{
    int i=0;
    while (i < mw->drawing->childCount()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(mw->drawing->child(i));
        if (drawingItem && drawingItem->isValidHide()) return true;
        i++;
    }
    return false;
}

bool RootDrawingItem::isValidSelectAll ()
{
    int i=0;
    while (i < mw->drawing->childCount()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(mw->drawing->child(i));
        if (drawingItem) {
            if (!drawingItem->isSelected()) return true;
        }
        i++;
    }
    return true;
}

void RootDrawingItem::show (bool update)
{
    int i=0;
    while (i < mw->drawing->childCount()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(mw->drawing->child(i));
        if (drawingItem) {
            mw->ui->drawingWindow->showItem(drawingItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
}

void RootDrawingItem::hide (bool update)
{
    int i=0;
    while (i < mw->drawing->childCount()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(mw->drawing->child(i));
        if (drawingItem) {
            mw->ui->drawingWindow->hideItem(drawingItem);
        }
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
}

void RootDrawingItem::selectAll ()
{
    mw->ui->drawingWindow->hideItem(mw->drawing);
    mw->ui->drawingWindow->unselectItem(mw->drawing);
    mw->ui->drawingItemTree->setCurrentItem(nullptr);

    int i=0;
    while (i < mw->drawing->childCount()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(mw->drawing->child(i));
        if (drawingItem) {
            mw->ui->drawingWindow->showItem(drawingItem);
            mw->ui->drawingWindow->selectItem(drawingItem);
        }
        i++;
    }
}

void RootDrawingItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show All",this);
    mw->hideAction=new QAction("Hide All",this);
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
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(getChild(i));
        if (drawingItem) {
            int index=indexOfChild(drawingItem);
            takeChild(index);
            getParentItem()->addChild(drawingItem);
            drawingItem->setParentItem(getParentItem());
            drawingItem->decrease_depth();
            mw->ui->drawingWindow->showItem(drawingItem);
        }
        i++;
    }
}

void DrawingItem::demoteChildren ()
{
    std::cout << "DrawingItem::demoteChildren" << std::endl; std::cout.flush();

    long unsigned int i=0;
    while (i < getChildrenSize()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(getChild(i));
        if (drawingItem) {
            int index=drawingItem->getParentItem()->indexOfChild(drawingItem);
            drawingItem->getParentItem()->takeChild(index);
            addChild(drawingItem);
            drawingItem->setParentItem(this);
            drawingItem->copy_depth(this);
            drawingItem->increase_depth();

            drawingItem->setText(1,QString());

            mw->ui->drawingWindow->hideItem(drawingItem);
        }
        i++;
    }
}

void DrawingItem::cancelOperation ()
{
    //std::cout << "DrawingItem::cancelOperation" << std::endl; std::cout.flush();

    bool isDisplayed=false;
    if (foreground(0) == Qt::black) isDisplayed=true;

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

    if (isDisplayed) mw->ui->drawingWindow->showItem(this);
    else mw->ui->drawingWindow->hideItem(this);
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
    //std::cout << "DrawingItem::cancelDraw" << std::endl; std::cout.flush();

    // take care of shapes
    if (!animateShape.IsNull()) animateShape.Nullify();
    if (mw->activePolywire) {
        mw->activePolywire->deleteRubberband();
        mw->activePolywire=nullptr;
    }

    //cancelOperation();
    mw->ui->drawingWindow->set_gridPlane(mw->currentPrivilegedPlane);

    // remove the current undo/redo item
    mw->itemChangesStack.pop_back();

    mw->activeAction=false;

    mw->finishOperation(false,1);
}

void DrawingItem::startMove (bool isAnimate)
{
    //std::cout << "DrawingItem::startMove  drawingItem=" << text(0).toStdString() << "  isAnimate=" << isAnimate << std::endl; std::cout.flush();

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
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(child(i));
            if (drawingItem) {
                resetOperation();
                setEnableMove(true);
                drawingItem->startMove(false);
            }
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
    //std::cout << "DrawingItem::finishMove  drawingItem=" << text(0).toStdString() << std::endl; std::cout.flush();

    bool isDisplayed=false;
    if (foreground(0) == Qt::black) isDisplayed=true;

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
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(child(i));
            if (drawingItem) drawingItem->finishMove(p0_,p1_);
            mw->drawingChanged=true;
            i++;
        }
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


    if (isDisplayed) mw->ui->drawingWindow->showItem(this);
    else mw->ui->drawingWindow->hideItem(this);

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
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(child(i));
            if (drawingItem) {
                resetOperation();
                drawingItem->startRotate();
            }
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
    //bool isDisplayed=mw->ui->drawingWindow->isDisplayed(getShape());

    bool isDisplayed=false;
    if (foreground(0) == Qt::black) isDisplayed=true;

    // remove the old version from display and tracking
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
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(child(i));
            if (drawingItem) drawingItem->finishRotate(angle,startPoint,endPoint);
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

    if (isDisplayed) mw->ui->drawingWindow->showItem(this);
    else mw->ui->drawingWindow->hideItem(this);

    // alignForegroundColor();
    // if (isDisplayed && foreground(0) == Qt::gray) mw->ui->drawingWindow->showItem(this);
    // if (!isDisplayed && foreground(0) == Qt::black) mw->ui->drawingWindow->hideItem(this);

    resetOperation();

    //findTopLevelItem(this);
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
    bool isDisplayed=false;
    if (foreground(0) == Qt::black) isDisplayed=true;

    // remove the old version from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (!polywire) return;

    finishStretchPoint();

    if (isDisplayed) mw->ui->drawingWindow->showItem(this);
    else mw->ui->drawingWindow->hideItem(this);
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
    mw->ui->drawingWindow->activateItem(newItem);
    mw->ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);

    // set parent

    RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(parent);
    if (rootDrawingItem) {
        rootDrawingItem->addChild(newItem);
        newItem->setParentItem(rootDrawingItem);
        newItem->set_depth(0);
    }

    DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(parent);
    if (drawingItem) {
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
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(child(i));
            if (drawingItem) {
                DrawingItem *newChild=drawingItem->copy(newItem);
                newChild->setParentItem(newItem);
                newItem->push_child(newChild);
            }
            i++;
        }
    }

    mw->ui->drawingWindow->hideItem(newItem);

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
                DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(child(i));
                if (drawingItem) {
                    polywire=static_cast<Polywire *>(drawingItem->getPolywire());
                    if (polywire) break;
                }
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
    //bool isDisplayed=mw->ui->drawingWindow->isDisplayed(getShape());

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
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(getChild(i));
            if (drawingItem) drawingItem->finishEdit();
            i++;
        }
    }

    if (!polywire && !process) {
        mw->reprocess(this);
    }

    // alignForegroundColor();
    // if (isDisplayed && foreground(0) == Qt::gray) mw->ui->drawingWindow->showItem(this);
    // if (!isDisplayed && foreground(0) == Qt::black) mw->ui->drawingWindow->hideItem(this);

    mw->ui->drawingWindow->showItem(this);

    mw->activeAction=false;

    //findTopLevelItem(this);
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
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(getChild(i));
            if (drawingItem) drawingItem->finishDeletePoint();
            i++;
        }
    }

    if (!polywire && !process) {
        mw->reprocess(this);
        mw->drawingChanged=true;
    }

    resetOperation();
    mw->activeAction=false;
    //findTopLevelItem(this);
    mw->ui->drawingWindow->showItem(this);
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
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(getChild(i));
            if (drawingItem) drawingItem->finishStretchPoint();
            i++;
        }
    }

    if (!polywire && !process) {
        mw->reprocess(this);
        mw->drawingChanged=true;
    }

    resetOperation();
    mw->activeAction=false;
    //findTopLevelItem(this);

    mw->ui->drawingWindow->showItem(this);
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

        //findTopLevelItem(this);
        mw->ui->drawingWindow->showItem(this);
    }
}

void DrawingItem::del ()
{
    // remove from display and tracking
    mw->ui->drawingWindow->unselectItem(this);
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    // mark as delete
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setDelete();
    addShapeData(newShapeData);

    // parentItem
    RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(parentItem);
    if (rootDrawingItem) {
        int insertIndex=rootDrawingItem->indexOfChild(this);

        // move children to parent
        while (childCount() > 0) {
            DrawingItem* drawingItem=dynamic_cast<DrawingItem *>(takeChild(0));
            if (drawingItem) {
                rootDrawingItem->insertChild(insertIndex++,drawingItem);
                drawingItem->setParentItem(rootDrawingItem);
                drawingItem->decrease_depth();
                mw->ui->drawingWindow->showItem(drawingItem);

                // set the materials
                if (!text(1).isNull()) {
                    if (!drawingItem->getPolywire()) {
                        drawingItem->setText(1,text(1));
                    }
                }
            }
        }

        rootDrawingItem->removeChild(this);
    }

    mw->itemChangesStack.add(this);

    // reset the top-level compound
    mw->reprocess(mw->drawing);

    mw->drawingChanged=true;
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

void DrawingItem::show (bool update)
{
    mw->ui->drawingWindow->showItem(this);

    if (update) {
        mw->ui->drawingWindow->updateViewer();
    }
}

void DrawingItem::hide (bool update)
{
    mw->ui->drawingWindow->hideItem(this);

    int i=0;
    while (i < childCount()) {
        DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(child(i));
        if (drawingItem) drawingItem->hide(update);
        i++;
    }

    if (update) {
        mw->ui->drawingWindow->updateViewer();
    }
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
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->assignMaterialAction, &QAction::triggered, mw, &OpenParEMg::assignMaterial);
    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::showDrawingItems);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::hideDrawingItems);
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
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

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
    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
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
    //std::cout << "DrawingItem::createPath" << std::endl; std::cout.flush();

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

    mw->ui->drawingWindow->unselectAllItems();

    bool isDisplayed=false;
    if (foreground(0) == Qt::black) isDisplayed=true;

    if (shapeData->isNoop()) {
        std::cout << "   isNoop" << std::endl; std::cout.flush();
        // nothing to do
    } else if (shapeData->isCreate()) {
        std::cout << "   isCreate" << std::endl; std::cout.flush();

        // remove the item
        mw->ui->drawingWindow->unselectItem(this);
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(getShape());

        getParentItem()->removeChild(this);
        promoteChildren();

        // int i=0;
        // while (i < getChildrenSize()) {
        //     BaseItem *child=getChild(i);
        //     if (child) {
        //         child->expandToItem();
        //     }
        //     i++;
        // }

        dataStack.undo();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

        mw->ui->drawingWindow->unselectItem(this);
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
                    DrawingItem *childItem=dynamic_cast<DrawingItem *>(child(i));
                    if (childItem) {
                        childItem->undo();
                        mw->ui->drawingWindow->hideItem(childItem);
                    }
                    i++;
                }
            } else {
                mw->reprocess(this);
                //mw->ui->drawingWindow->unselectItem(this);
            }
        }

        if (isDisplayed) mw->ui->drawingWindow->showItem(this);
        else mw->ui->drawingWindow->hideItem(this);

        //mw->ui->drawingWindow->showItem(this);

        //BaseItem *baseItem=findTopLevelItem(this);
        //baseItem->expandToItem();
        //expandToItem();
    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();

        dataStack.undo();

        DrawingItem *parentItem=dynamic_cast<DrawingItem *>(getParentItem());
        copy_depth(parentItem);

        increase_depth();
        getParentItem()->addChild(this);
        demoteChildren();

        mw->reprocess(this);

        mw->ui->drawingWindow->hideItem(this);
        RootDrawingItem *rootParentItem=dynamic_cast<RootDrawingItem *>(getParentItem());
        if (rootParentItem) mw->ui->drawingWindow->showItem(this);
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

    mw->ui->drawingWindow->unselectAllItems();

    bool isDisplayed=false;
    if (foreground(0) == Qt::black) isDisplayed=true;

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
        demoteChildren();

        mw->reprocess(this);

        mw->ui->drawingWindow->hideItem(this);
        RootDrawingItem *rootParentItem=dynamic_cast<RootDrawingItem *>(getParentItem());
        if (rootParentItem) mw->ui->drawingWindow->showItem(this);

        //BaseItem *baseItem=findTopLevelItem(this);
        //baseItem->expandToItem();
        //expandToItem();
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

        mw->ui->drawingWindow->unselectItem(this);
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
                    DrawingItem *childItem=dynamic_cast<DrawingItem *>(child(i));
                    if (childItem) {
                        childItem->redo();
                    }
                    i++;
                }
            }
        }

        mw->reprocess(this);
        //mw->insertToMapActivateItem(this);
        mw->ui->drawingWindow->insertItemToMap(getShape(),this);
        mw->ui->drawingWindow->activateItem(this);

        if (isDisplayed) mw->ui->drawingWindow->showItem(this);
        else mw->ui->drawingWindow->hideItem(this);
        //mw->ui->drawingWindow->showItem(this);

        //BaseItem *baseItem=findTopLevelItem(this);
        //baseItem->expandToItem();
        //expandToItem();
    } else if (next->isDelete()) {
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
        PathItem *pathItem=dynamic_cast<PathItem *>(mw->path->child(i));
        if (pathItem && pathItem->foreground(0) == Qt::gray) return true;
        i++;
    }
    return false;
}

bool RootPathItem::isValidHide ()
{
    int i=0;
    while (i < mw->path->childCount()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(mw->path->child(i));
        if (pathItem && pathItem->foreground(0) == Qt::black) return true;
        i++;
    }
    return false;
}

void RootPathItem::show (bool update)
{
    int i=0;
    while (i < mw->path->childCount()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(mw->path->child(i));
        if (pathItem) pathItem->show(false);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
}

void RootPathItem::hide (bool update)
{
    int i=0;
    while (i < mw->path->childCount()) {
        PathItem *pathItem=dynamic_cast<PathItem *>(mw->path->child(i));
        if (pathItem) pathItem->hide(false);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
}

void RootPathItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show All",this);
    mw->hideAction=new QAction("Hide All",this);
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

void PathItem::show (bool update)
{
    mw->ui->drawingWindow->showItem(this);

    int i=0;
    while (i < linkedItems_size()) {
        BaseItem *baseItem=get_linkedItem(i);
        if (baseItem) baseItem->setForeground(0,Qt::black);
        i++;
    }

    if (update) {
        mw->ui->drawingWindow->updateViewer();
    }
}

void PathItem::hide (bool update)
{
    mw->ui->drawingWindow->hideItem(this);

    int i=0;
    while (i < linkedItems_size()) {
        BaseItem *baseItem=get_linkedItem(i);
        if (baseItem) baseItem->setForeground(0,Qt::gray);
        i++;
    }

    if (update) {
        mw->ui->drawingWindow->updateViewer();
    }
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
    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::showPathItems);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::hidePathItems);
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
    RootPathItem *rootPathItem=dynamic_cast<RootPathItem *>(parentItem);
    if (rootPathItem) {
        rootPathItem->removeChild(this);
    }

    mw->itemChangesStack.add(this);
    mw->drawingChanged=true;
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

    if (shapeData->isReversePath()) {
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
        mw->ui->drawingWindow->showItem(this);
    } else {
        BaseItem::undo();
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
        mw->ui->drawingWindow->showItem(this);
        //expandToItem();
    } else {
        BaseItem::redo();
    }
}

void PathItem::reverse ()
{
    // remove the old version from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    // mark
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setReversePath();
    addShapeData(newShapeData);

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
        mw->ui->drawingWindow->showItem(this);
        mw->ui->drawingWindow->activateItem(this);

        mw->projectChanged=true;

        // add to the stack for undo/redo
        mw->itemChangesStack.add(this);
    }
}

void PathItem::save (std::ofstream *out)
{
    if (path) path->save(out);
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

void IntegrationPathItem::show (bool update)
{
    pathItem->show(update);
    setForeground(0,Qt::black);
}

void IntegrationPathItem::hide (bool update)
{
    pathItem->hide(update);
    setForeground(0,Qt::gray);
}

void IntegrationPathItem::showMenu (QMenu *menu)
{
    mw->renameAction=new QAction("Flip Sign",this);
    mw->deleteAction=new QAction("Delete",this);
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);

    connect(mw->renameAction, &QAction::triggered, mw, &OpenParEMg::flipSignIntegrationPathItems);
    connect(mw->deleteAction, &QAction::triggered, mw, &OpenParEMg::deleteIntegrationPathItems);
    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::showIntegrationPathItems);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::hideIntegrationPathItems);

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
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setDelete();
    addShapeData(newShapeData);

    VIItem *viItem=dynamic_cast<VIItem *>(parentItem);
    if (viItem) {
        viItem->removeChild(this);
        viItem->addRemoveScale();

        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
        }

        mw->itemChangesStack.add(this);
        mw->projectChanged=true;
    }
}

void IntegrationPathItem::flipSign ()
{
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setChangeName();
    addShapeData(newShapeData);

    QChar direction=newShapeData->get_name().front();
    QString newName;
    if (direction == QChar('+')) newName="-";
    if (direction == QChar('-')) newName="+";
    newName.append(newShapeData->get_name().sliced(1));

    newShapeData->set_name(newName);
    setText(0,newName);

    mw->itemChangesStack.add(this);
}

void IntegrationPathItem::save (std::ofstream *out)
{
    *out << "         path=" << text(0).toStdString() << std::endl;
}

void IntegrationPathItem::saveN (std::ofstream *out)
{
    QChar direction=text(0).front();
    QString name=text(0).slice(1);
    *out << "         path" << direction.toLatin1() << "=" << name.toStdString() << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// RootBoundaryItem
////////////////////////////////////////////////////////////////////////////////

bool RootBoundaryItem::isValidShow ()
{
    int i=0;
    while (i < mw->boundary->childCount()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(mw->boundary->child(i));
        if (boundaryItem && boundaryItem->foreground(0) == Qt::gray) return true;
        i++;
    }
    return false;
}

bool RootBoundaryItem::isValidHide ()
{
    int i=0;
    while (i < mw->boundary->childCount()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(mw->boundary->child(i));
        if (boundaryItem && boundaryItem->foreground(0) == Qt::black) return true;
        i++;
    }
    return false;
}

void RootBoundaryItem::show (bool update)
{
    int i=0;
    while (i < mw->boundary->childCount()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(mw->boundary->child(i));
        if (boundaryItem) boundaryItem->show(false);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
}

void RootBoundaryItem::hide (bool update)
{
    int i=0;
    while (i < mw->boundary->childCount()) {
        BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(mw->boundary->child(i));
        if (boundaryItem) boundaryItem->hide(false);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
}

void RootBoundaryItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show All",this);
    mw->hideAction=new QAction("Hide All",this);
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

    // cross link
    if (pathItem) pathItem->push_linkedItem(this);
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
    //std::cout << "BoundaryItem::insertItemWidgets" << std::endl; std::cout.flush();

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
        textWaveImpedance->set_doubleValidator();
        textWaveImpedance->set_baseItem(this);
        mw->ui->drawingItemTree->setItemWidget(itemWaveImpedance,0,textWaveImpedance);

        QObject::connect(textWaveImpedance,&CustomLineEdit::CustomEditFinished,&textValueChanged);
    }

    // material

    if (itemMaterial) {
        CustomComboBox *comboMaterial=new CustomComboBox();
        const QSignalBlocker blockerMaterial(comboMaterial);
        if (mw->materialDatabase) {
            long unsigned int i=0;
            while (i < mw->materialDatabase->get_size()) {
                Material *material=mw->materialDatabase->get_material(i);
                if (material->is_conductor()) {
                    comboMaterial->addItem(QString::fromStdString(material->get_name()->get_value()));
                }
                i++;
            }
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
    if (foreground(0) == Qt::gray) return true;
    return false;
}

bool BoundaryItem::isValidHide ()
{
    if (foreground(0) == Qt::black) return true;
    return false;
}

void BoundaryItem::show (bool update)
{
    pathItem->show(update);
    setForeground(0,Qt::black);
}

void BoundaryItem::hide (bool update)
{
    pathItem->hide(update);
    setForeground(0,Qt::gray);
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

    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::showBoundaryItems);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::hideBoundaryItems);
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
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setDelete();
    addShapeData(newShapeData);

    RootBoundaryItem *rootBoundaryItem=dynamic_cast<RootBoundaryItem *>(parentItem);
    if (rootBoundaryItem) {
        rootBoundaryItem->removeChild(this);

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

void BoundaryItem::save (std::ofstream *out)
{
    *out << "Boundary" << std::endl;
    *out << "   name=" << text(0).toStdString() << std::endl;

    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->save(out);
        i++;
    }

    if (pathItem) {
        Path *boundaryPath=pathItem->getPath();
        if (boundaryPath) {
            *out << "   path=" << boundaryPath->get_name() << std::endl;
        }
    }

    *out << "EndBoundary" << std::endl;
    *out << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// RootPortItem
////////////////////////////////////////////////////////////////////////////////

bool RootPortItem::isValidShow ()
{
    int i=0;
    while (i < mw->port->childCount()) {
        PortItem *portItem=dynamic_cast<PortItem *>(mw->port->child(i));
        if (portItem && portItem->foreground(0) == Qt::gray) return true;
        i++;
    }
    return false;
}

bool RootPortItem::isValidHide ()
{
    int i=0;
    while (i < mw->port->childCount()) {
        PortItem *portItem=dynamic_cast<PortItem *>(mw->port->child(i));
        if (portItem && portItem->foreground(0) == Qt::black) return true;
        i++;
    }
    return false;
}

void RootPortItem::show (bool update)
{
    int i=0;
    while (i < mw->port->childCount()) {
        PortItem *portItem=dynamic_cast<PortItem *>(mw->port->child(i));
        if (portItem) portItem->show(false);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
}

void RootPortItem::hide (bool update)
{
    int i=0;
    while (i < mw->port->childCount()) {
        PortItem *portItem=dynamic_cast<PortItem *>(mw->port->child(i));
        if (portItem) portItem->hide(false);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
}

void RootPortItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show All",this);
    mw->hideAction=new QAction("Hide All",this);
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

void PortItem::insertImpedanceDefinitionWidget (BaseItem *impedanceDefinitionItem, QString impedance_definition)
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
    mw->ui->drawingItemTree->setItemWidget(impedanceDefinitionItem,0,comboZdef);
    impedanceDefinitionItem->setSizeHint(0,comboZdef->sizeHint());  // size hint for scaling; do not need to do the other combobox

    QObject::connect(comboZdef,&CustomComboBox::CustomCurrentIndexChanged,&comboIndexChanged);
    QObject::connect(comboZdef,&CustomComboBox::CustomCurrentIndexChanged,mw->relay,&Relay::setMenus);
}

void PortItem::addImpedanceDefinitionItem ()
{
    ShapeData *shapeData=getShapeData();
    QString impedance_definition=shapeData->get_impedance_definition();

    BaseItem *impedanceDefinitionItem=new BaseItem(mw,this);
    impedanceDefinitionItem->set_itemType(6);
    impedanceDefinitionItem->setFlags(impedanceDefinitionItem->flags() & ~Qt::ItemIsSelectable);
    impedanceDefinitionItem->setToolTip(0,"Impedance definition for calculating characteristic impedance.");
    addChild(impedanceDefinitionItem);

    insertImpedanceDefinitionWidget(impedanceDefinitionItem,impedance_definition);
}

void PortItem::insertImpedanceCalculationWidget (BaseItem *impedanceCalculationItem, QString impedance_calculation)
{
    //std::cout << "PortItem::insertImpedanceCalculationWidget" << std::endl; std::cout.flush();

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
    mw->ui->drawingItemTree->setItemWidget(impedanceCalculationItem,0,comboZcalc);

    // check for differential pairs
    int i=0;
    while (i < childCount()) {
        DiffPairItem *diffPairItem=dynamic_cast<DiffPairItem *>(child(i));
        if (diffPairItem) {
            //diffPairItem->enableZcalcControl(false);  // reminder that this exists
            comboZcalc->setEnabled(false);
            break;
        }
        i++;
    }

    QObject::connect(comboZcalc,&CustomComboBox::CustomCurrentIndexChanged,&comboIndexChanged);
    QObject::connect(comboZcalc,&CustomComboBox::CustomCurrentIndexChanged,mw->relay,&Relay::setMenus);
}

void PortItem::addImpedanceCalculationItem ()
{
    ShapeData *shapeData=getShapeData();
    QString impedance_calculation=shapeData->get_impedance_calculation();

    BaseItem *impedanceCalculationItem=new BaseItem(mw,this);
    impedanceCalculationItem->set_itemType(7);
    impedanceCalculationItem->setFlags(impedanceCalculationItem->flags() & ~Qt::ItemIsSelectable);
    impedanceCalculationItem->setToolTip(0,"Impedance calculation using modal or line integration paths.");
    addChild(impedanceCalculationItem);

    insertImpedanceCalculationWidget(impedanceCalculationItem,impedance_calculation);
}

bool PortItem::isValidShow ()
{
    if (foreground(0) == Qt::gray) return true;
    return false;
}

bool PortItem::isValidHide ()
{
    if (foreground(0) == Qt::black) return true;
    return false;
}

void PortItem::show (bool update)
{
    pathItem->show(update);
    setForeground(0,Qt::black);

    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->hide(update);
        i++;
    }
}

void PortItem::hide (bool update)
{
    pathItem->hide(update);
    setForeground(0,Qt::gray);

    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->hide(update);
        i++;
    }
}

void PortItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->unselectAction=new QAction("Unselect",this);
    mw->renameAction=new QAction("Rename",this);
    mw->insertAction=new QAction("Insert S-port",this);
    mw->deleteAction=new QAction("Delete",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::showPortItems);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::hidePortItems);
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
    // mark
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setDelete();
    addShapeData(newShapeData);

    // parentItem
    BaseItem *parentItem=getParentItem();
    if (parentItem) {
        RootPortItem *rootPortItem=dynamic_cast<RootPortItem *>(parentItem);
        if (rootPortItem) {
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

int PortItem::get_SportCount ()
{
    int SportCount=0;
    int i=0;
    while (i < childCount()) {
        ModeItem *modeItem=dynamic_cast<ModeItem *>(child(i));
        if (modeItem) {
            int testSportCount=modeItem->get_SportCount();
            if (testSportCount > SportCount) SportCount=testSportCount;
        }
        i++;
    }
    return SportCount;
}

void PortItem::save (std::ofstream *out)
{
    *out << "Port" << std::endl;
    *out << "   name=" << text(0).toStdString() << std::endl;

    if (pathItem) {
        Path *portPath=pathItem->getPath();
        if (portPath) {
            *out << "   path=" << portPath->get_name() << std::endl;
        }
    }

    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->save(out);
        i++;
    }

    *out << "EndPort" << std::endl;
    *out << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// ModeItem
////////////////////////////////////////////////////////////////////////////////

ModeItem::ModeItem (OpenParEMg *mw_, BaseItem *parentItem_, bool dummyFill)
{
    mw=mw_;
    parentItem=parentItem_;
    itemType=5;

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    addShapeData(newShapeData);

    // get the current largest Sport number
    int Sport=0;
    mw->largestSportNumber (mw->port,&Sport);
    Sport++;

    QString name="net";
    name.append(QString::number(Sport));
    setText(0,name);
    newShapeData->set_name(text(0));
    setForeground(0,Qt::black);

    setToolTip(0,"Mode and its net name.");
    setFlags(flags() & ~Qt::ItemIsEditable);

    if (dummyFill) {
        SportItem *newSportItem=new SportItem(mw,this,Sport);
        addChild(newSportItem);

        VIItem *newVoltageItem=new VIItem(mw,this,10);
        addChild(newVoltageItem);

        VIItem *newCurrentItem=new VIItem(mw,this,11);
        addChild(newCurrentItem);
    }
}

bool ModeItem::isValidShow ()
{
    return mw->ui->drawingWindow->isNetValidShow();
}

bool ModeItem::isValidHide ()
{
    return mw->ui->drawingWindow->isNetValidHide();
}

void ModeItem::show (bool update)
{
    setForeground(0,Qt::black);

    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->show(update);
        i++;
    }

    if (update) {
        mw->ui->drawingWindow->updateViewer();
    }
}

void ModeItem::hide (bool update)
{
    setForeground(0,Qt::gray);

    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->hide(update);
        i++;
    }

    if (update) {
        mw->ui->drawingWindow->updateViewer();
    }
}

bool ModeItem::isValidDelete ()
{
    if (parentItem->is_diffpair()) return false;
    return true;
}

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
    if (parentItem->is_diffpair()) return;

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setDelete();
    addShapeData(newShapeData);

    unlinkPaths(this);
    parentItem->removeChild(this);

    mw->itemChangesStack.add(this);

    mw->clickedItem=nullptr;
    mw->previousClickedItem=nullptr;

    mw->ui->drawingWindow->updateViewer();
}

void ModeItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->renameAction=new QAction("Rename",this);
    mw->createDiffpairAction=new QAction("Create Differential Pair",this);
    mw->deleteAction=new QAction("Delete",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::showModeItems);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::hideModeItems);
    connect(mw->renameAction, &QAction::triggered, mw, &OpenParEMg::renameSportNet);
    connect(mw->createDiffpairAction, &QAction::triggered, mw, &OpenParEMg::createDiffPairItem);
    connect(mw->deleteAction, &QAction::triggered, mw, &OpenParEMg::deleteModeItems);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (mw->ui->drawingWindow->get_selectedItems_count() == 1) menu->addAction(mw->renameAction);
    if (mw->isValidCreateDiffPair()) menu->addAction(mw->createDiffpairAction);
    if (isValidDelete()) menu->addAction(mw->deleteAction);
    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
}

int ModeItem::get_SportCount ()
{
    int SportCount=0;
    int i=0;
    while (i < childCount()) {
        SportItem *sportItem=dynamic_cast<SportItem *>(child(i));
        if (sportItem && sportItem->is_sportLabel()) {
            int testSportCount=sportItem->get_SportCount();
            if (testSportCount > SportCount) SportCount=testSportCount;
        }
        i++;
    }
    return SportCount;
}

void ModeItem::save (std::ofstream *out)
{
    PortItem *portItem=nullptr;
    if (parentItem->is_port()) {
        portItem=dynamic_cast<PortItem *>(parentItem);
    } else if (parentItem->is_diffpair()) {
        portItem=dynamic_cast<PortItem *>(parentItem->getParentItem());
    }

    if (!portItem) return;

    QString calculation="Mode";
    if (portItem && portItem->is_port()) {
        ShapeData *shapeData=portItem->getShapeData();
        calculation=shapeData->get_impedance_calculation();
        if (calculation.compare("line") == 0) calculation="Line";
        else if (calculation.compare("mode") == 0) calculation="Mode";
    }

    ShapeData *shapeData=getShapeData();
    *out << "   " << calculation.toStdString() << std::endl;
    *out << "      net=" << text(0).toStdString() << std::endl;
    *out << "      Sport=" << shapeData->get_Sport() << std::endl;

    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->save(out);
        i++;
    }

    *out << "   End" << calculation.toStdString() << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// SportItem
////////////////////////////////////////////////////////////////////////////////

SportItem::SportItem (OpenParEMg *mw_, ModeItem *modeItem_, int Sport)
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
    ShapeData *shapeData=sportNumberItem->getShapeData();
    shapeData->set_Sport(Sport);
    addChild(sportNumberItem);

    // spin box for changing the port number
    sportNumberItem->insertSportNumberWidget(Sport);
}

bool SportItem::isValidShow () {return false;}
bool SportItem::isValidHide () {return false;}

void SportItem::show (bool update)
{
    setForeground(0,Qt::black);
}

void SportItem::hide (bool update)
{
    setForeground(0,Qt::gray);
}

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

int SportItem::get_SportCount ()
{
    int SportCount=0;
    int i=0;
    while (i < childCount()) {
        SportNumberItem *sportNumberItem=dynamic_cast<SportNumberItem *>(child(i));
        if (sportNumberItem && sportNumberItem->is_sportNumber()) {
            int testSportCount=sportNumberItem->get_SportCount();
            if (testSportCount > SportCount) SportCount=testSportCount;
        }
        i++;
    }
    return SportCount;
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
void SportNumberItem::show (bool) {}
void SportNumberItem::hide (bool) {}

void SportNumberItem::showMenu (QMenu *menu)
{
}

void SportNumberItem::insertSportNumberWidget (int Sport)
{
    CustomSpinBox *sportNumber=new CustomSpinBox();
    const QSignalBlocker blocker(sportNumber);
    sportNumber->set_itemTracker(mw->ui->drawingWindow->get_itemTracker());
    sportNumber->set_sportNumberItem(this);
    sportNumber->setMinimum(1);
    sportNumber->setValue(Sport);
    mw->ui->drawingItemTree->setItemWidget(this,0,sportNumber);

    QObject::connect(sportNumber,&CustomSpinBox::CustomValueChanged,&spinValueChanged);
    QObject::connect(sportNumber,&CustomSpinBox::CustomValueChanged,mw->relay,&Relay::setMenus);
}

int SportNumberItem::get_SportCount ()
{
    int SportCount=0;
    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) {
            CustomSpinBox *sportNumber=dynamic_cast<CustomSpinBox *>(mw->ui->drawingItemTree->itemWidget(baseItem,0));
            if (sportNumber) {
                SportCount=sportNumber->value();
                break;
            }
            i++;
        }
    }
    return SportCount;
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
    scaleLabelItem=nullptr;

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

void VIItem::show (bool update)
{
    setForeground(0,Qt::black);

    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->show(update);
        i++;
    }

    if (update) {
        mw->ui->drawingWindow->updateViewer();
    }
}

void VIItem::hide (bool update)
{
    setForeground(0,Qt::gray);

    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->hide(update);
        i++;
    }

    if (update) {
        mw->ui->drawingWindow->updateViewer();
    }
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

    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::showVIItems);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::hideVIItems);
    connect(mw->drawPathAction, &QAction::triggered, this, &VIItem::drawLinePath);
    connect(mw->drawPolylineAction, &QAction::triggered, this, &VIItem::drawPolylinePath);
    connect(mw->insertAction, &QAction::triggered, mw, &OpenParEMg::insertSelectedPaths);
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
            if (baseItem->is_path()) pathCount++;
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
    if (!pathItem) return false;
    Path *portPath=pathItem->getPath();
    if (!portPath) return false;

    i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        BaseItem *baseItem=mw->ui->drawingWindow->get_selectedItem(i);
        if (baseItem && baseItem->is_path()) {
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

void VIItem::createIntegrationPathItemFromPath (PathItem *pathItem)
{
    //std::cout << "VIItem::createIntegrationPathItemFromPath" << std::endl; std::cout.flush();

    // create an integration path item
    IntegrationPathItem *newIntegrationPathItem=new IntegrationPathItem(mw,this,pathItem);
    if (newIntegrationPathItem) {
        ShapeData *shapeData=newIntegrationPathItem->getShapeData();

        QString name="+";
        name.append(pathItem->text(0));
        newIntegrationPathItem->setText(0,name);
        shapeData->set_name(newIntegrationPathItem->text(0));

        addChild(newIntegrationPathItem);
        mw->itemChangesStack.add(newIntegrationPathItem);

        pathItem->push_linkedItem(newIntegrationPathItem);
        newIntegrationPathItem->setPathItem(pathItem);
    }
    return ;
}

PathItem* VIItem::createIntegrationPathItemFromDrawing (DrawingItem *drawingItem, bool hasArrows)
{
    //std::cout << "VIItem::createIntegrationPathItemFromDrawing" << std::endl; std::cout.flush();

    PathItem *newPathItem=drawingItem->createPath(hasArrows);
    createIntegrationPathItemFromPath(newPathItem);
    return newPathItem;
}

bool VIItem::hasScale ()
{
    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem && baseItem->is_scaleLabel()) return true;
        i++;
    }
    return false;
}

bool VIItem::hasIntegrationPathItem ()
{
    int i=0;
    while (i < childCount()) {
        BaseItem *integrationPathItem=dynamic_cast<BaseItem *>(child(i));
        if (integrationPathItem && integrationPathItem->is_integrationPathSegment()) return true;
        i++;
    }
    return false;
}

void VIItem::addScaleItem ()
{
    std::cout << "VIItem::addScaleItem" << std::endl; std::cout.flush();

    if (hasScale()) return;

    if (scaleLabelItem) {
        insertChild(0,scaleLabelItem);
    } else {

        // label
        scaleLabelItem=new ScaleLabelItem(mw,this);
        insertChild(0,scaleLabelItem);

        // value

        ScaleValueItem *scaleValueItem=new ScaleValueItem(mw,scaleLabelItem);
        scaleLabelItem->insertChild(0,scaleValueItem);

        ShapeData *shapeData=scaleValueItem->getShapeData();
        scaleValueItem->insertScaleValueWidget(shapeData->get_scale());
    }
}

void VIItem::removeScaleItem ()
{
    if (!hasScale()) return;
    removeChild(scaleLabelItem);
}

void VIItem::addRemoveScale ()
{
    if (hasScale()) {
        if (hasIntegrationPathItem()) {
            // nothing to do
        } else {
            removeScaleItem();
        }
    } else {
        if (hasIntegrationPathItem()) {
            addScaleItem();
        } else {
            // nothing to do
        }
    }
}

void VIItem::save (std::ofstream *out)
{
    if (childCount() == 0) return;

    *out << "      IntegrationPath" << std::endl;

    if (is_voltage()) *out << "         type=voltage" << std::endl;
    if (is_current()) *out << "         type=current" << std::endl;

    int count=0;
    int i=0;
    while (i < childCount()) {
        IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(child(i));
        if (integrationPathItem) {
            if (count == 0) integrationPathItem->save(out);
            else integrationPathItem->saveN(out);
            count++;
        }
        i++;
    }

    *out << "      EndIntegrationPath" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// ScaleLabelItem
////////////////////////////////////////////////////////////////////////////////

void ScaleLabelItem::show (bool update)
{
    setForeground(0,Qt::black);
}

void ScaleLabelItem::hide (bool update)
{
    setForeground(0,Qt::gray);
}

void ScaleLabelItem::save (std::ofstream *out)
{
    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->save(out);
        i++;
    }
}

////////////////////////////////////////////////////////////////////////////////
// DiffPairItem
////////////////////////////////////////////////////////////////////////////////

DiffPairItem::DiffPairItem (OpenParEMg *mw_, PortItem *portItem_, ModeItem *pModeItem_, ModeItem *nModeItem_)
{
    mw=mw_;
    parentItem=portItem_;
    itemType=15;
    portItem=portItem_;

    setFlags(flags() & ~Qt::ItemIsEditable);
    setForeground(0,Qt::black);
    setText(0,"Differential Pair");

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->set_name(text(0));
    addShapeData(newShapeData);

    children.push_back(pModeItem_);
    children.push_back(nModeItem_);
}

bool DiffPairItem::isValidShow ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        DiffPairItem *diffPairItem=dynamic_cast<DiffPairItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (diffPairItem && diffPairItem->is_diffpair()) {
            int j=0;
            while (j < childCount()) {
                if (child(j)) {
                    if (child(j)->foreground(0) == Qt::gray) return true;
                }
                j++;
            }
        }
        i++;
    }
    return false;
}

bool DiffPairItem::isValidHide ()
{
    long unsigned int i=0;
    while (i < mw->ui->drawingWindow->get_selectedItems_size()) {
        DiffPairItem *diffPairItem=dynamic_cast<DiffPairItem *>(mw->ui->drawingWindow->get_selectedItem(i));
        if (diffPairItem && diffPairItem->is_diffpair()) {
            int j=0;
            while (j < childCount()) {
                if (child(j)) {
                    if (child(j)->foreground(0) == Qt::black) return true;
                }
                j++;
            }
        }
        i++;
    }
    return false;
}

void DiffPairItem::show (bool update)
{
    setForeground(0,Qt::black);

    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->show(update);
        i++;
    }
}

void DiffPairItem::hide (bool update)
{
    setForeground(0,Qt::gray);

    int i=0;
    while (i < childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(child(i));
        if (baseItem) baseItem->hide(update);
        i++;
    }
}

void DiffPairItem::showMenu (QMenu *menu)
{

    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->deleteAction=new QAction("Delete",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::showDiffPairItems);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::hideDiffPairItems);
    connect(mw->deleteAction, &QAction::triggered, mw, &OpenParEMg::deleteDiffPairItems);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
    if (isValidDelete()) menu->addAction(mw->deleteAction);
    if (mw->clickedItem) {
        if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
        if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
    }
}

bool DiffPairItem::isValidDelete ()
{
    return true;
}

void DiffPairItem::del ()
{
    // mark as delete
    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setDelete();
    addShapeData(newShapeData);

    PortItem *portItem=dynamic_cast<PortItem *>(parentItem);
    if (portItem) {
        promoteChildren();
        portItem->removeChild(this);
        mw->projectChanged=true;
    }

    enableZcalcControl(true);

    mw->itemChangesStack.add(this);
}

void DiffPairItem::promoteChildren ()
{
    long unsigned int i=0;
    while (i < getChildrenSize()) {
        ModeItem *modeItem=dynamic_cast<ModeItem *>(getChild(i));
        if (modeItem) {
            int index=indexOfChild(modeItem);
            takeChild(index);
            getParentItem()->addChild(modeItem);
            modeItem->setParentItem(getParentItem());
            modeItem->restoreWidgets();
            modeItem->show(true);
        }
        i++;
    }
}

void DiffPairItem::demoteChildren ()
{
    long unsigned int i=0;
    while (i < getChildrenSize()) {
        ModeItem *modeItem=dynamic_cast<ModeItem *>(getChild(i));
        if (modeItem) {
            int index=modeItem->getParentItem()->indexOfChild(modeItem);
            modeItem->getParentItem()->takeChild(index);
            addChild(modeItem);
            modeItem->setParentItem(this);
            modeItem->restoreWidgets();
            modeItem->show(true);
        }
        i++;
    }
}

void DiffPairItem::undo ()
{
    std::cout << "DiffPairItem::undo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    if (shapeData->isCreate()) {
        dataStack.undo();
        promoteChildren();
        mw->ui->drawingWindow->hideItem(this);
        parentItem->removeChild(this);
        enableZcalcControl(true);
    } else if (shapeData->isDelete()) {
        dataStack.undo();
        parentItem->addChild(this);
        demoteChildren();
        enableZcalcControl(false);
        mw->ui->drawingWindow->showItem(this);
    } else {
        BaseItem::undo();
    }
}

void DiffPairItem::redo ()
{
    std::cout << "DiffPairItem::redo  this=" << this << std::endl; std::cout.flush();

    ShapeData *shapeData=getShapeData();
    if (!shapeData) return;

    ShapeData *next=shapeData->getNext();
    if (!next) return;

    if (next->isCreate()) {
        dataStack.redo();
        parentItem->addChild(this);
        demoteChildren();
        enableZcalcControl(false);
        mw->ui->drawingWindow->showItem(this);
    } else if (next->isDelete()) {
        dataStack.redo();
        promoteChildren();
        mw->ui->drawingWindow->hideItem(this);
        parentItem->removeChild(this);
        enableZcalcControl(true);
    } else {
        BaseItem::redo();
    }
}

void DiffPairItem::enableZcalcControl (bool state)
{
    PortItem *portItem=dynamic_cast<PortItem *>(parentItem);
    if (!portItem) return;

    int i=0;
    while (i < portItem->childCount()) {
        BaseItem *baseItem=dynamic_cast<BaseItem *>(portItem->child(i));
        if (baseItem && baseItem->is_impedanceCalculation()) {
            CustomComboBox *comboZcalc=dynamic_cast<CustomComboBox *>(mw->ui->drawingItemTree->itemWidget(baseItem,0));
            if (comboZcalc) {
                comboZcalc->setEnabled(state);
                break;
            }
        }
        i++;
    }
}

void DiffPairItem::save (std::ofstream *out)
{
    std::vector<int> Sport;
    int i=0;
    while (i < childCount()) {
        ModeItem *modeItem=dynamic_cast<ModeItem *>(child(i));
        if (modeItem && modeItem->is_sport()) {
            ShapeData *shapeData=modeItem->getShapeData();
            Sport.push_back(shapeData->get_Sport());

            modeItem->save(out);
        }
        i++;
    }

    if (Sport.size() != 2) return;

    *out << "   DifferentialPair" << std::endl;
    *out << "      Sport_P=" << Sport[0] << std::endl;
    *out << "      Sport_N=" << Sport[1] << std::endl;
    *out << "   EndDifferentialPair" << std::endl;
}


////////////////////////////////////////////////////////////////////////////////
// RootMeshItem
////////////////////////////////////////////////////////////////////////////////

bool RootMeshItem::isValidShow ()
{
    int i=0;
    while (i < mw->mesh->childCount()) {
        MeshItem *meshItem=dynamic_cast<MeshItem *>(mw->mesh->child(i));
        if (meshItem && meshItem->foreground(0) == Qt::gray) return true;
        i++;
    }
    return false;
}

bool RootMeshItem::isValidHide ()
{
    int i=0;
    while (i < mw->mesh->childCount()) {
        MeshItem *meshItem=dynamic_cast<MeshItem *>(mw->mesh->child(i));
        if (meshItem && meshItem->foreground(0) == Qt::black) return true;
        i++;
    }
    return false;
}

void RootMeshItem::show (bool update)
{
    int i=0;
    while (i < childCount()) {
        MeshItem *meshItem=dynamic_cast<MeshItem *>(child(i));
        if (meshItem) meshItem->show(false);
        i++;
    }

    mw->ui->drawingWindow->updateViewer();
}

void RootMeshItem::hide (bool update)
{
    int i=0;
    while (i < childCount()) {
        MeshItem *meshItem=dynamic_cast<MeshItem *>(child(i));
        if (meshItem) meshItem->hide(false);

        i++;
    }

    mw->ui->drawingWindow->updateViewer();
}

void RootMeshItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show All",this);
    mw->hideAction=new QAction("Hide All",this);
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

void MeshItem::show (bool update)
{
    long unsigned int i=0;
    while (i < meshEntities.size()) {
        mw->ui->drawingWindow->displayShape(meshEntities[i]);
        i++;
    }

    setForeground(0,Qt::black);

    if (update) {
        mw->ui->drawingWindow->updateViewer();
    }
}

void MeshItem::hide (bool update)
{
    long unsigned int i=0;
    while (i < meshEntities.size()) {
        mw->ui->drawingWindow->removeShape(meshEntities[i]);
        i++;
    }

    setForeground(0,Qt::gray);

    if (update) {
        mw->ui->drawingWindow->updateViewer();
    }
}

void MeshItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);

    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::showMeshItems);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::hideMeshItems);

    if (isValidShow()) menu->addAction(mw->showAction);
    if (isValidHide()) menu->addAction(mw->hideAction);
}
