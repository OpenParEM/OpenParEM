

#include "CustomTreeWidgetItem.h"
#include "OPEMg.h"


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


