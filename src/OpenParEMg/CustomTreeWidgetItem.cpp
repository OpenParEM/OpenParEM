

#include "CustomTreeWidgetItem.h"
#include "OPEMg.h"
#include "ui_OPEMg.h"
#include <BRepPrimAPI_MakePrism.hxx>
#include <TopoDS_Iterator.hxx>


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

void DrawingItem::setForUndoRedo ()
{
    // clone the item onto itself for undo/redo
    ShapeData *newShapeData=getShapeData()->copyCreate();

    // remove the old version from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    // save the new version for current usage
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

void DrawingItem::startDraw ()
{
    gp_Dir normal=mw->ui->drawingWindow->get_normal();
    mw->activePolywire->setNormal(normal.X(),normal.Y(),normal.Z());
    mw->activePolywire->set_viewerContext(mw->ui->drawingWindow->get_viewerContext());
    mw->activePolywire->setDrawEnable(true);

    ShapeData *newShapeData=getShapeData()->copyCreate();
    newShapeData->setCreate();
    newShapeData->setPolywire(mw->activePolywire);
    addShapeData(newShapeData);

    mw->restrictToDrawingPlane=true;
    mw->startOperation(true);
    mw->activeAction=true;
    mw->itemChangesStack.startNew();
    mw->ui->drawingWindow->set_pickFirstVertex(true);
    mw->clearTreeSelection();
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
    mw->drawing.addChild(this);
    setParent(&(mw->drawing));

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
    activeAction=false;
    mw->restrictToDrawingPlane=false;
    mw->activePolywire=nullptr;
    mw->workingItem=nullptr;

    // mark as changed
    mw->drawingChanged=true;

    // final universal cleanup
    mw->finishOperation(false,13);
}

void DrawingItem::cancelDraw ()
{
    // take care of shapes
    if (!animateShape.IsNull()) animateShape.Nullify();
    if ( mw->activePolywire) {
        mw->activePolywire->deleteRubberband();
        mw->activePolywire=nullptr;
    }

    // remove the in-process ShapeData
    pop();

    // remove the current undo/redo item
    mw->itemChangesStack.pop_back();

    mw->activeAction=false;

    mw->ui->drawingWindow->updateViewer();
}

void DrawingItem::startMove ()
{
    setForUndoRedo();

    // enable move
    resetOperation();
    setAnimate(mw->ui->drawingWindow->get_viewerContext());
    setEnableMove(true);

    // add to the stack for undo/redo
    mw->itemChangesStack.add(this);
}

void DrawingItem::finishMove (gp_Pnt p0_, gp_Pnt p1_)
{
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

    activeAction=false;

    resetOperation();
    mw->findShowTopLevelItem(this,false);
}

void DrawingItem::startRotate ()
{
    setForUndoRedo();
    resetOperation();
    mw->itemChangesStack.add(this);
}

void DrawingItem::finishRotate (double angle, gp_Pnt startPoint, gp_Pnt endPoint)
{
    // remove the old version from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

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
            i++;
        }
    }

    if (!polywire && !process) {
        TopoDS_Shape shape=rotateShape(angle,startPoint,endPoint,mw->ui->drawingWindow->get_viewerContext());
        Handle(AIS_Shape) newAISshape=new AIS_Shape(shape);

        ShapeData *shapeData=getShapeData();
        shapeData->setShape(newAISshape);

        mw->reprocess(this);
        mw->drawingChanged=true;
    }

    resetOperation();
    activeAction=false;
    mw->findShowTopLevelItem(this,false);
}

void DrawingItem::startStretch ()
{
    setForUndoRedo();

    Handle(AIS_Shape) shape=getShape();
    if (!shape.IsNull()) {

        // set the drawing plane
        mw->currentPrivilegedPlane=mw->ui->drawingWindow->get_gridPlane();

        // set the polywire for stretch
        Polywire *polywire=static_cast<Polywire *>(getPolywire());
        if (polywire) {
            resetOperation();
            setEnableStretch(true);
            gp_Pln plane=polywire->getPlane();
            mw->ui->drawingWindow->set_gridPlane(plane);
            mw->itemChangesStack.add(this);
        }
    }
}

void DrawingItem::finishStretch ()
{
    // remove the old version from display and tracking
    mw->ui->drawingWindow->hideItem(this);
    mw->ui->drawingWindow->removeItemFromMap(this);
    mw->ui->drawingWindow->deleteShape(getShape());

    Polywire *polywire=static_cast<Polywire *>(getPolywire());
    if (!polywire) return;
    polywire->deleteRubberband();

    // modify the clone
    mw->finishStretchPoint(this);
}

void DrawingItem::extrude ()
{
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
            newItem->setParent(&(mw->drawing));
            newExtrude=nullptr;

            // move the object in the selection tree
            int index=mw->drawing.indexOfChild(this);
            mw->drawing.takeChild(index);
            newItem->addChild(this);
            this->setParent(newItem);
            depth++;

            mw->ui->drawingWindow->insertItemToMap(newItem->getShape(),newItem);

            // add the object to the child list for undo/redo
            newItem->push_child(this);

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
    activeAction=false;
}

DrawingItem* DrawingItem::copy (CustomTreeWidgetItem *parent)
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
    if (rootDrawingItem) {
        rootDrawingItem->addChild(newItem);
        newItem->setParent(rootDrawingItem);
        newItem->copy_depth(rootDrawingItem);
    }

    DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(parent);
    if (drawingItem) {
        drawingItem->addChild(newItem);
        newItem->setParent(drawingItem);
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
                newChild->setParent(newItem);
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
        if (extrude) {

            // clone the child so that undo/redo works properly
            int i=0;
            while (i < childCount()) {
                DrawingItem *processChild=(DrawingItem *)child(i);
                Polywire *polywire=static_cast<Polywire *>(processChild->getPolywire());
                if (polywire) {
                    ShapeData *newShapeData=processChild->getShapeData()->copyCreate();
                    newShapeData->setEdit();
                    processChild->addShapeData(newShapeData);
                }
                i++;
            }

            extrude->set_length(mw->length);
            mw->reprocess(this);
        }
    }

    activeAction=false;
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
    if (!polywire) return;

    gp_Pnt p0=getP0();
    polywire->deletePoint(p0);
    mw->reprocess(this);
    activeAction=false;
    resetOperation();

    mw->ui->drawingWindow->set_gridPlane(mw->currentPrivilegedPlane);
    mw->drawingChanged=true;
    mw->findShowTopLevelItem(this,false);
}

DrawingItem* DrawingItem::copyCreate ()
{
    std::cout << "DrawingItem::copyCreate" << std::endl; std::cout.flush();
    DrawingItem *newItem=new DrawingItem();

    // copy just the current data

    ShapeData *copyShapeData=dataStack.getShapeData()->copyCreate();
    newItem->dataStack.add(copyShapeData);
    newItem->setText(0,this->text(0).append("_copy"));
    newItem->aTrsf=aTrsf;
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
    connect(mw->convertToPathAction, &QAction::triggered, mw, &OpenParEMg::convertToPath);
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
    connect(mw->convertToPortAction, &QAction::triggered, mw, &OpenParEMg::convertToPort);
    connect(mw->convertToBoundaryAction, &QAction::triggered, mw, &OpenParEMg::convertToBoundary);
    connect(mw->cancelAction, &QAction::triggered, mw, &OpenParEMg::cancelDrawingMenu);

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


