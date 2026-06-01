

#include "CustomTreeWidgetItem.h"
#include "OPEMg.h"
#include "ui_OPEMg.h"
#include <BRepPrimAPI_MakePrism.hxx>
#include <TopoDS_Iterator.hxx>

////////////////////////////////////////////////////////////////////////////////
// BaseItem
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// RootDrawingItem
////////////////////////////////////////////////////////////////////////////////

void RootDrawingItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->selectAllAction=new QAction("Select All");

    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::rootDrawingShow);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::rootDrawingHide);
    connect(mw->selectAllAction, &QAction::triggered, mw, &OpenParEMg::rootDrawingSelectAll);

    if (mw->isValidRootDrawingShow()) menu->addAction(mw->showAction);
    if (mw->isValidRootDrawingHide()) menu->addAction(mw->hideAction);
    if (mw->isValidRootDrawingSelectAll()) menu->addAction(mw->selectAllAction);
}

////////////////////////////////////////////////////////////////////////////////
// DrawingItem
////////////////////////////////////////////////////////////////////////////////

void DrawingItem::promoteChildren ()
{
    long unsigned int i=0;
    while (i < getChildrenSize()) {
        BaseItem *child=dynamic_cast<BaseItem *>(getChild(i));
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
        BaseItem *child=getChild(i);
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
    mw->activePolywire->setHasArrows(getHasArrows());

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->setPolywire(mw->activePolywire);
    addShapeData(newShapeData);

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
            DrawingItem *newItem=new DrawingItem(0);
            newItem->setMW(mw);
            newItem->setText(0,newExtrude->getName(&(mw->objectCounts)));
            ShapeData *newShapeData=new ShapeData(1,nullptr,newExtrude,newShape);
            newItem->addShapeData(newShapeData);
            mw->itemChangesStack.add(newItem);

            mw->drawing.addChild(newItem);
            newItem->setParentItem(&(mw->drawing));
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
    newItem->setMW(mw);
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
        newItem->copy_depth(rootDrawingItem);
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
                BaseItem* drawingChild=(BaseItem *)takeChild(0);
                rootDrawingItem->insertChild(insertIndex++,drawingChild);
                drawingChild->setParentItem(rootDrawingItem);
                drawingChild->decrease_depth();
                mw->ui->drawingWindow->showItem(drawingChild);

                // set the materials
                if (!text(1).isNull()) {
                    if (!drawingChild->getPolywire()) drawingChild->setText(1,text(1));
                }
            }

            setIsActive(false);
            rootDrawingItem->removeChild(this);
        }

        mw->itemChangesStack.add(this);

        // remove from display and tracking
        mw->ui->drawingWindow->hideItem(this);
        mw->ui->drawingWindow->removeItemFromMap(this);
        mw->ui->drawingWindow->deleteShape(this->getShape());

        // reset the top-level compound
        mw->reprocess(&(mw->drawing));

        mw->drawingChanged=true;
    }
}

DrawingItem* DrawingItem::copyCreate ()
{
    //std::cout << "DrawingItem::copyCreate" << std::endl; std::cout.flush();
    DrawingItem *newItem=new DrawingItem();
    if (!newItem) return nullptr;

    // copy just the current data

    ShapeData *copyShapeData=dataStack.getShapeData()->copyCreate();
    newItem->dataStack.add(copyShapeData);
    newItem->setText(0,this->text(0).append("_copy"));
    newItem->aTrsf=aTrsf;
    newItem->dimTag=dimTag;
    newItem->forShowHide=forShowHide;
    newItem->itemType=itemType;
    newItem->depth=depth;

    return newItem;
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

    if (mw->isValidAssignMaterial()) menu->addAction(mw->assignMaterialAction);
    if (mw->isValidObjectShow()) menu->addAction(mw->showAction);
    if (mw->isValidObjectHide()) menu->addAction(mw->hideAction);
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
        setIsActive(false);

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
        copy_depth(getParentItem());
        increase_depth();
        getParentItem()->addChild(this);

        demoteChildren();
        setIsActive(true);

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

        copy_depth(getParentItem());
        increase_depth();
        getParentItem()->addChild(this);
        setIsActive(true);

        long unsigned int i=0;
        while (i < getChildrenSize()) {
            BaseItem *childItem=getChild(i);
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
        setIsActive(false);
        getParentItem()->removeChild(this);
        mw->findShowTopLevelItem(this,true);
    }
}

////////////////////////////////////////////////////////////////////////////////
// RootPathItem
////////////////////////////////////////////////////////////////////////////////

void RootPathItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::showRootPathItems);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::hideRootPathItems);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (mw->isValidRootPathShow()) menu->addAction(mw->showAction);
    if (mw->isValidRootPathHide()) menu->addAction(mw->hideAction);
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
}

////////////////////////////////////////////////////////////////////////////////
// PathItem
////////////////////////////////////////////////////////////////////////////////

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

    if (mw->isValidShowPath()) menu->addAction(mw->showAction);
    if (mw->isValidHidePath()) menu->addAction(mw->hideAction);
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
        setIsActive(false);

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
        setIsActive(false);

        Path *path=static_cast<Path *>(getPath());
        if (path) path->setIsUsed(false);

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

        mw->path.addChild(this);

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
        dataStack.redo();

        mw->path.addChild(this);
        setIsActive(true);

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
    setForUndoRedo();

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
// RootBoundaryItem
////////////////////////////////////////////////////////////////////////////////

void RootBoundaryItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::rootBoundaryShow);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::rootBoundaryHide);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (mw->isValidRootBoundaryShow()) menu->addAction(mw->showAction);
    if (mw->isValidRootBoundaryHide()) menu->addAction(mw->hideAction);
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
}

////////////////////////////////////////////////////////////////////////////////
// BoundaryItem
////////////////////////////////////////////////////////////////////////////////

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

    if (mw->isValidShowPath()) menu->addAction(mw->showAction);
    if (mw->isValidHidePath()) menu->addAction(mw->hideAction);
    if (mw->ui->drawingWindow->hasBoundarySelectedItems()) menu->addAction(mw->unselectAction);
    if (mw->ui->drawingWindow->get_boundarySelectedCount() == 1) menu->addAction(mw->renameAction);
    menu->addAction(mw->deleteAction);
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
}

void BoundaryItem::del ()
{
    setForUndoRedo();

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

        setIsActive(false);

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

        // remove the item
        getParentItem()->removeChild(this);
        setIsActive(false);

        // restore the arrows on the path
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            mw->ui->drawingWindow->showItem(pathItem);
            mw->ui->drawingWindow->activateItem(pathItem);
            mw->ui->drawingWindow->selectItem(pathItem);
            pathItem->showArrows(true);
        }

        dataStack.undo();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();

        dataStack.undo();

        mw->boundary.addChild(this);
        setForeground(0,Qt::gray);
        mw->ui->drawingWindow->showItem(this);
        setIsActive(true);

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
        Boundary *boundary=getBoundary();
        if (boundary) {
            boundary->draw(mw->relay,&(mw->projData),mw->boundaryDatabase,mw,mw->ui->drawingWindow,mw->ui->drawingItemTree,
                           &(mw->path),&(mw->boundary),mw->materialDatabase,this);
        }
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
        dataStack.redo();
        mw->boundary.addChild(this);
        setForeground(0,Qt::gray);
        mw->ui->drawingWindow->showItem(this);
        setIsActive(true);

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
        Boundary *boundary=getBoundary();
        if (boundary) {
            boundary->draw(mw->relay,&(mw->projData),mw->boundaryDatabase,mw,mw->ui->drawingWindow,mw->ui->drawingItemTree,
                           &(mw->path),&(mw->boundary),mw->materialDatabase,this);
        }
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();

        dataStack.redo();

        // remove the item
        getParentItem()->removeChild(this);
        setIsActive(false);

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

////////////////////////////////////////////////////////////////////////////////
// RootPortItem
////////////////////////////////////////////////////////////////////////////////

void RootPortItem::showMenu (QMenu *menu)
{
    mw->showAction=new QAction("Show",this);
    mw->hideAction=new QAction("Hide",this);
    mw->expandAllAction=new QAction("Expand All",this);
    mw->collapseAllAction=new QAction("Collapse All",this);

    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::rootPortShow);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::rootPortHide);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (mw->isValidRootPortShow()) menu->addAction(mw->showAction);
    if (mw->isValidRootPortHide()) menu->addAction(mw->hideAction);
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
}

////////////////////////////////////////////////////////////////////////////////
// PortItem
////////////////////////////////////////////////////////////////////////////////

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

    connect(mw->showAction, &QAction::triggered, mw, &OpenParEMg::showPortItems);
    connect(mw->hideAction, &QAction::triggered, mw, &OpenParEMg::hidePortItems);
    connect(mw->unselectAction, &QAction::triggered, mw, &OpenParEMg::unselectPortItems);
    connect(mw->insertAction, &QAction::triggered, mw, &OpenParEMg::insertModeItems);
    connect(mw->renameAction, &QAction::triggered, mw, &OpenParEMg::renamePortItems);
    connect(mw->deleteAction, &QAction::triggered, mw, &OpenParEMg::deletePortItems);
    connect(mw->expandAllAction, &QAction::triggered, mw, &OpenParEMg::expandAllItems);
    connect(mw->collapseAllAction, &QAction::triggered, mw, &OpenParEMg::collapseAllItems);

    if (mw->isValidShowPath()) menu->addAction(mw->showAction);
    if (mw->isValidHidePath()) menu->addAction(mw->hideAction);
    if (mw->ui->drawingWindow->hasPortSelectedItems()) menu->addAction(mw->unselectAction);
    if (mw->ui->drawingWindow->get_portSelectedCount() == 1) menu->addAction(mw->renameAction);
    menu->addAction(mw->insertAction);
    menu->addAction(mw->deleteAction);
    if (!mw->clickedItem->isExpanded()) menu->addAction(mw->expandAllAction);
    if (mw->clickedItem->isExpanded()) menu->addAction(mw->collapseAllAction);
}

void PortItem::del ()
{
    setForUndoRedo();

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

        setIsActive(false);

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

        // remove the item
        getParentItem()->removeChild(this);
        setIsActive(false);

        // restore the arrows on the path
        PathItem *pathItem=getPathItem();
        if (pathItem) {
            pathItem->removeLinkedItem(this);
            mw->ui->drawingWindow->showItem(pathItem);
            mw->ui->drawingWindow->activateItem(pathItem);
            mw->ui->drawingWindow->selectItem(pathItem);
            pathItem->showArrows(true);
        }

        dataStack.undo();
    } else if (shapeData->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

    } else if (shapeData->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();

        dataStack.undo();

        mw->port.addChild(this);
        setForeground(0,Qt::gray);
        mw->ui->drawingWindow->showItem(this);
        setIsActive(true);

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
        Port *port=getPort();
        if (port) {
            port->draw(mw->relay,&(mw->projData),mw->boundaryDatabase,mw,mw->ui->drawingWindow,mw->ui->drawingItemTree,&(mw->path),&(mw->port),this);
        }
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
        mw->port.addChild(this);
        setForeground(0,Qt::gray);
        mw->ui->drawingWindow->showItem(this);
        setIsActive(true);

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
        Port *port=getPort();
        if (port) {
            port->draw(mw->relay,&(mw->projData),mw->boundaryDatabase,mw,mw->ui->drawingWindow,mw->ui->drawingItemTree,&(mw->path),&(mw->port),this);
        }
    } else if (next->isEdit()) {
        std::cout << "   isEdit" << std::endl; std::cout.flush();

    } else if (next->isDelete()) {
        std::cout << "   isDelete" << std::endl; std::cout.flush();

        // remove the item
        getParentItem()->removeChild(this);
        setIsActive(false);

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
