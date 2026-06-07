#ifndef ITEMTRACKING_H
#define ITEMTRACKING_H

#include <AIS_InteractiveContext.hxx>
#include "CustomTreeWidgetItem.h"

class ItemVector
{
public:
    ItemVector ()
    {
        currentCount=0;
        currentPathCount=0;
        currentPortCount=0;
        currentBoundaryCount=0;
    }

    void push_back (BaseItem *item)
    {
        if (item) {
            data.push_back(item);
            currentCount++;

            PathItem *pathItem=dynamic_cast<PathItem *>(item);
            if (pathItem && pathItem->is_path()) currentPathCount++;

            if (item->is_port()) currentPortCount++;
            if (item->is_boundary()) currentBoundaryCount++;
        }
    }

    void nullify (long unsigned int index)
    {
        //std::cout << "ItemTracking::nullify  index=" << index << std::endl; std::cout.flush();

        if (data[index]) {
            if (data[index]) {
                if (data[index]->is_path()) currentPathCount--;
                if (data[index]->is_port()) currentPortCount--;
                if (data[index]->is_boundary()) currentBoundaryCount--;
            }

            data[index]=nullptr;
            currentCount--;
        }
    }

    // expecting uniqueness, so stop after item is found
    void nullify (BaseItem *item)
    {
        if (item) {
            long unsigned int i=0;
            while (i < data.size()) {
                if (item == data[i]) {
                    nullify(i);
                    return;
                }
                i++;
            }
        }
    }

    void compact ()
    {
        long unsigned int i=0;
        while (i < data.size()) {
            if (!data[i]) {
                bool found=false;
                long unsigned int j=i+1;
                while (j < data.size()) {
                    if (data[j]) {
                        data[i]=data[j];
                        data[j]=nullptr;
                        found=true;
                        break;
                    }
                    j++;
                }
                if (!found) break;
            }
            i++;
        }
        data.resize(i);
    }

    void clear ()
    {
        data.clear();
        currentCount=0;
        currentPathCount=0;
        currentPortCount=0;
        currentBoundaryCount=0;
    }

    long unsigned int size () {return data.size();}
    long unsigned int count () {return currentCount;}
    long unsigned int pathCount () {return currentPathCount;}
    long unsigned int portCount () {return currentPortCount;}
    long unsigned int boundaryCount () {return currentBoundaryCount;}

    BaseItem* operator[](long unsigned int index) {
        return data[index];
    }

private:
    std::vector<BaseItem *> data;
    long unsigned int currentCount;          // number of non-void entries
    long unsigned int currentPathCount;
    long unsigned int currentPortCount;
    long unsigned int currentBoundaryCount;
};

class ItemTracker : public QObject
{
    Q_OBJECT

public:

    ItemTracker (const Handle(AIS_InteractiveContext)& context) : viewerContext(context)
    {
        showTracking=false;
        hideTracking=false;
        selectTracking=false;
        unselectTracking=false;
        deleteTracking=false;
    }

    ~ItemTracker() {}

    // show

    void reshowVisibleItems () {
        if (showTracking) {std::cout << "ItemTracker::reshowVisibleItems" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < visibleItems.size()) {
            BaseItem *item=visibleItems[i];
            if (item) {
                if (item->is_mesh()) {
                    MeshItem *meshItem=dynamic_cast<MeshItem *>(item);

                    long unsigned int j=0;
                    while (j < meshItem->get_meshEntitiesSize()) {
                        DisplayShape(meshItem->get_meshEntity(j));
                        j++;
                    }
                } else {
                    EraseShape(item->getShape());
                    DisplayShape(item->getShape());
                }
            }
            i++;
        }
    }

    // void showShape (Handle(AIS_Shape) shape)
    // {
    //     if (showTracking) {std::cout << "ItemTracker::showShape" << std::endl; std::cout.flush();}

    //     if (shape.IsNull()) return;
    //     CustomTreeWidgetItem *item=shapeToItemMap[shape];
    //     showItem(item);
    // }

    void showItem (BaseItem *item)
    {
        if (showTracking) {std::cout << "ItemTracker::showItem" << std::endl; std::cout.flush();}

        if (!item) return;

        // show item

        if (item->is_rootDrawing()) {
            int i=0;
            while (i < item->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(item->child(i));
                showItem(child);
                i++;
            }
        } else if (item->is_drawing()) {

            if (item->foreground(0) == Qt::gray) {
                DisplayShape(item->getShape());
                item->setForeground(0,Qt::black);
                visibleItems.push_back(item);
            }
        } else if (item->is_rootPath()) {
            int i=0;
            while (i < item->childCount()) {
                BaseItem *child=(BaseItem *) item->child(i);
                showItem(child);
                i++;
            }
        } else if (item->is_path()) {
            if (item->foreground(0) == Qt::black) return;

            DisplayShape(item->getShape());
            item->setForeground(0,Qt::black);
            visibleItems.push_back(item);

            PathItem *pathItem=dynamic_cast<PathItem *>(item);
            if (pathItem && pathItem->is_path()) {
                long unsigned int i=0;
                while (i < pathItem->linkedItems_size()) {
                    showItem(pathItem->get_linkedItem(i));
                    i++;
                }
            }
        } else if (item->is_integrationPathSegment()) {
            if (item->foreground(0) == Qt::black) return;  // avoid infinite loop due to crosslinking of paths

            item->setForeground(0,Qt::black);
            IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(item);
            if (integrationPathItem && integrationPathItem->is_integrationPathSegment()) {
                showItem(integrationPathItem->getPathItem());
            }
        } else if (item->is_rootPort()) {
            int i=0;
            while (i < item->childCount()) {
                BaseItem *childItem=dynamic_cast<BaseItem *>(item->child(i));
                showItem(childItem);
                i++;
            }
        } else if (item->is_port()) {
            if (item->foreground(0) == Qt::black) return;

            item->setForeground(0,Qt::black);

            PortItem *portItem=dynamic_cast<PortItem *>(item);
            showItem(portItem->getPathItem());

            int i=0;
            while (i < item->childCount()) {
                BaseItem *childItem=dynamic_cast<BaseItem *>(item->child(i));
                showItem(childItem);
                i++;
            }
        } else if (item->is_rootBoundary()) {
            int i=0;
            while (i < item->childCount()) {
                BaseItem *childItem=dynamic_cast<BaseItem *>(item->child(i));
                showItem(childItem);
                i++;
            }
        } else if (item->is_boundary()) {
            if (item->foreground(0) == Qt::black) return;

            item->setForeground(0,Qt::black);

            BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(item);
            showItem(boundaryItem->getPathItem());

            int i=0;
            while (i < item->childCount()) {
                BaseItem *childItem=dynamic_cast<BaseItem *>(item->child(i));
                showItem(childItem);
                i++;
            }
        } else if (item->is_rootMesh()) {
            int i=0;
            while (i < item->childCount()) {
                BaseItem *childItem=dynamic_cast<BaseItem *>(item->child(i));
                showItem(childItem);
                i++;
            }
        } else if (item->is_mesh()) {
            if (item->foreground(0) == Qt::black) return;
            item->setForeground(0,Qt::black);
            visibleItems.push_back(item);

            MeshItem *meshItem=dynamic_cast<MeshItem *>(item);
            long unsigned int i=0;
            while (i < meshItem->get_meshEntitiesSize()){
                DisplayShape(meshItem->get_meshEntity(i));
                i++;
            }
        } else if (item->is_sport()) {
            int i=0;
            while (i < item->childCount()) {
                BaseItem *childItem=dynamic_cast<BaseItem *>(item->child(i));
                showItem(childItem);
                i++;
            }
        } else if (item->is_sportLabel()) {
            // nothing to do
        } else if (item->is_voltage() || item->is_current()) {
            if (item->foreground(0) == Qt::black) return;
            int i=0;
            while (i < item->childCount()) {
                BaseItem *childItem=dynamic_cast<BaseItem *>(item->child(i));
                showItem(childItem);
                i++;
            }
        } else if (item->is_integrationPathSegment()) {
            if (item->foreground(0) == Qt::black) return;

            PathItem *pathItem=dynamic_cast<PathItem *>(item);
            long unsigned int i=0;
            while (i < pathItem->linkedItems_size()) {
                PathItem *linkedItem=dynamic_cast<PathItem *>(pathItem->get_linkedItem(i));
                showItem(linkedItem);
                i++;
            }
            item->setForeground(0,Qt::black);
        } else if (item->is_scaleLabel()) {
            // nothing to do
        } else if (item->is_impedanceDefinition()) {
            // nothing to do
        } else if (item->is_impedanceCalculation ()) {
            // nothing to do
        } else if (item->is_sportNumber()) {
            // nothing to do
        } else {
            //xxx
            //std::cout << "ASSERT: Invalid option in ItemTracking::showItem" << std::endl; std::cout.flush();
            //item->print();
        }
    }

    bool isValidShow ()
    {
        if (showTracking) {std::cout << "ItemTracker::isValidShow" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            BaseItem *item=selectedItems[i];
            if (item) {
                if (item->isValidShow()) return true;

                // children
                if (!item->is_drawing()) {
                    int j=0;
                    while (j < item->childCount()) {
                        BaseItem *child=dynamic_cast<BaseItem *>(item->child(j));
                        if (child->isValidShow()) return true;
                        j++;
                    }
                }
            }
            i++;
        }

        return false;
    }

    bool isVIValidShow ()
    {
        if (showTracking) {std::cout << "ItemTracker::isVIValidShow" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            BaseItem *item=selectedItems[i];
            if (item) {
                int j=0;
                while (j < item->childCount()) {
                    BaseItem *child=dynamic_cast<BaseItem *>(item->child(j));
                    if (child->is_integrationPathSegment()) {
                        if (child->isValidShow()) return true;
                    }
                    j++;
                }
            }
            i++;
        }

        return false;
    }

    bool isNetValidShow ()
    {
        if (showTracking) {std::cout << "ItemTracker::isNetValidShow" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            BaseItem *item=selectedItems[i];
            if (item) {
                int j=0;
                while (j < item->childCount()) {
                    BaseItem *child=dynamic_cast<BaseItem *>(item->child(j));
                    if (child->is_voltage() || child->is_current()) {
                        int k=0;
                        while (k < child->childCount()) {
                            BaseItem *grandChild=dynamic_cast<BaseItem *>(child->child(k));
                            if (grandChild->is_integrationPathSegment()) {
                                if (grandChild->isValidShow()) {
                                    return true;
                                }
                            }
                            k++;
                        }

                    }
                    j++;
                }
            }
            i++;
        }

        return false;
    }


    // hide

    // void hideShape (Handle(AIS_Shape) shape)
    // {
    //     if (hideTracking) {std::cout << "ItemTracker::hideShape" << std::endl; std::cout.flush();}

    //     if (shape.IsNull()) return;
    //     CustomTreeWidgetItem *item=shapeToItemMap[shape];
    //     hideItem(item);
    // }

    void hideItem (BaseItem *item)
    {
        if (hideTracking) {std::cout << "ItemTracker::hideItem  item=" << item << std::endl; std::cout.flush();}

        if (!item) return;

        // custom hide

        if (item->is_rootDrawing()) {
            EraseShape(item->getShape());
            nullifyVisibleItem(item);

            int i=0;
            while (i < item->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(item->child(i));
                hideItem(child);
                i++;
            }
        } else if (item->is_drawing()) {

            // hide item
            EraseShape(item->getShape());
            item->setForeground(0,Qt::gray);
            nullifyVisibleItem(item);

            // hide children
            int i=0;
            while (i < item->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(item->child(i));
                hideItem(child);
                i++;
            }
        } else if (item->is_rootPath()) {
            int i=0;
            while (i < item->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(item->child(i));
                hideItem(child);
                i++;
            }
        } else if (item->is_path()) {
            if (item->foreground(0) == Qt::gray) return;  // avoid infinite loop due to crosslinking of paths

            EraseShape(item->getShape());
            item->setForeground(0,Qt::gray);
            nullifyVisibleItem(item);

            PathItem *pathItem=dynamic_cast<PathItem *>(item);
            long unsigned int i=0;
            while (i < pathItem->linkedItems_size()) {
                hideItem(pathItem->get_linkedItem(i));
                i++;
            }
        } else if (item->is_integrationPathSegment()) {
            if (item->foreground(0) == Qt::gray) return;  // avoid infinite loop due to crosslinking of paths

            item->setForeground(0,Qt::gray);
            IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(item);
            if (integrationPathItem && integrationPathItem->is_integrationPathSegment()) {
                showItem(integrationPathItem->getPathItem());
            }
        } else if (item->is_rootPort()) {
            int i=0;
            while (i < item->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(item->child(i));
                hideItem(child);
                i++;
            }
        } else if (item->is_port()) {
            if (item->foreground(0) == Qt::gray) return;  // avoid infinite loop due to crosslinking of paths

            item->setForeground(0,Qt::gray);
            nullifyVisibleItem(item);

            PortItem *portItem=dynamic_cast<PortItem *>(item);
            hideItem(portItem->getPathItem());
        } else if (item->is_rootBoundary()) {
            int i=0;
            while (i < item->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(item->child(i));
                hideItem(child);
                i++;
            }
        } else if (item->is_boundary()) {
            if (item->foreground(0) == Qt::gray) return;  // avoid infinite loop due to crosslinking of paths

            item->setForeground(0,Qt::gray);
            nullifyVisibleItem(item);

            BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(item);
            hideItem(boundaryItem->getPathItem());
        } else if (item->is_rootMesh()) {
            int i=0;
            while (i < item->childCount()) {
                BaseItem *child=dynamic_cast<BoundaryItem *>(item->child(i));
                hideItem(child);
                i++;
            }
        } else if (item->is_mesh()) {
            if (item->foreground(0) == Qt::gray) return;   // avoid infinite loop due to crosslinking of paths

            item->setForeground(0,Qt::gray);
            nullifyVisibleItem(item);

            MeshItem *meshItem=dynamic_cast<MeshItem *>(item);
            long unsigned int i=0;
            while (i < meshItem->get_meshEntitiesSize()){
                EraseShape(meshItem->get_meshEntity(i));
                i++;
            }
        } else if (item->is_sport()) {
            if (item->foreground(0) == Qt::gray) return;

            int i=0;
            while (i < item->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(item->child(i));
                hideItem(child);
                i++;
            }
        } else if (item->is_sportLabel()) {
            // nothing to do
        } else if (item->is_voltage() || item->is_current()) {
            if (item->foreground(0) == Qt::gray) return;

            int i=0;
            while (i < item->childCount()) {
                BaseItem *child=dynamic_cast<BaseItem *>(item->child(i));
                hideItem(child);
                i++;
            }
        } else if (item->is_integrationPathSegment()) {
            if (item->foreground(0) == Qt::gray) return;

            PathItem *pathItem=dynamic_cast<PathItem *>(item);
            long unsigned int i=0;
            while (i < pathItem->linkedItems_size()) {
                hideItem(pathItem->get_linkedItem(i));
                i++;
            }
            item->setForeground(0,Qt::gray);
            nullifyVisibleItem(item);
        } else if (item->is_scaleLabel()) {
            // nothing to do
        } else if (item->is_scaleValue()) {
            // nothing to do
        } else if (item->is_impedanceDefinition()) {
            // nothing to do
        } else if (item->is_impedanceCalculation ()) {
            // nothing to do
        } else if (item->is_sportNumber()) {
            // nothing to do
        } else {
            std::cout << "ASSERT: Invalid option in ItemTracking::hideItem  itemType=" << item->get_itemType() << std::endl; std::cout.flush();
        }
    }

    // hide only selected items
    void hideItems ()
    {
        if (hideTracking) {std::cout << "ItemTracker::hideItems" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            hideItem(selectedItems[i]);
            i++;
        }
    }

    // hide all items whether selected or not
    void hideAllItems ()
    {
        if (hideTracking) {std::cout << "ItemTracker::hideAllItems" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < visibleItems.size()) {
            hideItem(visibleItems[i]);
            i++;
        }
    }

    bool isVisibleItem (BaseItem *item)
    {
        if (!item) return false;

        long unsigned int i;
        while (i < visibleItems.size()) {
            if (visibleItems[i] == item) return true;
            i++;
        }

        return false;
    }

    bool isValidHide ()
    {
        if (hideTracking) {std::cout << "ItemTracker::isValidHide" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            BaseItem *item=selectedItems[i];
            if (item) {
                if (item->isValidHide()) return true;

                // children
                DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(item);
                if (!(drawingItem && drawingItem->is_drawing())) {   // skip drawing for speed
                    int j=0;
                    while (j < item->childCount()) {
                        BaseItem *child=dynamic_cast<BaseItem *>(item->child(j));
                        if (child->isValidHide()) return true;
                        j++;
                    }
                }
            }
            i++;
        }
        return false;
    }

    bool isVIValidHide ()
    {
        if (hideTracking) {std::cout << "ItemTracker::isVIValidHide" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            BaseItem *item=selectedItems[i];
            if (item) {
                int j=0;
                while (j < item->childCount()) {
                    BaseItem *child=dynamic_cast<BaseItem *>(item->child(j));
                    if (child->is_integrationPathSegment()) {
                        if (child->isValidHide()) return true;
                    }
                    j++;
                }
            }
            i++;
        }
        return false;
    }

    bool isNetValidHide ()
    {
        if (hideTracking) {std::cout << "ItemTracker::isNetValidHide" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            BaseItem *item=selectedItems[i];
            if (item) {
                int j=0;
                while (j < item->childCount()) {
                    BaseItem *child=dynamic_cast<BaseItem *>(item->child(j));

                    if (child->is_voltage() || child->is_current()) {
                        int k=0;
                        while (k < child->childCount()) {
                            BaseItem *grandChild=dynamic_cast<BaseItem *>(child->child(k));
                            if (grandChild->is_integrationPathSegment()) {
                                if (grandChild->isValidHide()) {
                                    return true;
                                }
                            }
                            k++;
                        }
                    }
                    j++;
                }
            }
            i++;
        }
        return false;
    }


    // select

    void selectItemShape (Handle(AIS_Shape) shape)
    {
        if (selectTracking) {std::cout << "ItemTracker::selectShape" << std::endl; std::cout.flush();}

        BaseItem *item=shapeToItemMap[shape];
        if (item) {
            //showItem(item);
            selectItem(item);  // mesh shapes are not in the map, so need to check for valid item
        }
    }

    void activateSelectItem (BaseItem *item)
    {
        //std::cout << "ItemTracking::activateSelectItem" << std::endl; std::cout.flush();

        if (!item) return;

        if (!item->getShape().IsNull()) {
            viewerContext->Activate(item->getShape());
        }

        selectItem(item);
    }

    // assumes it is already in the tracker
    void refreshSelectedItem (BaseItem *item) {
        if (!item) return;
        SelectShape(item->getShape());
    }

    void refreshSelectedItems ()
    {
        viewerContext->ClearSelected(Standard_False);
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            if (selectedItems[i]) {
                SelectShape(selectedItems[i]->getShape());
            }
            i++;
        }
    }

    void selectItem (BaseItem *item)
    {
        if (selectTracking) {std::cout << "ItemTracker::selectItem" << std::endl; std::cout.flush();}
        if (!item) return;

        // see if the item is already selected
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            if (item == selectedItems[i]) {
                SelectShape(item->getShape());
                return;
            }
            i++;
        }

        if (item->is_rootDrawing()) {
            item->setSelected(Standard_True);
            selectedItems.push_back(item);
        } else {
            if (!item->getShape().IsNull()) {
                SelectShape(item->getShape());
            }
            item->setSelected(Standard_True);
            selectedItems.push_back(item);

            PathItem *pathItem=dynamic_cast<PathItem *>(item);
            if (pathItem) {
                if (pathItem->is_path() || pathItem->is_integrationPathSegment()) {
                    long unsigned int i=0;
                    while (i < pathItem->linkedItems_size()) {
                        selectItem(pathItem->get_linkedItem(i));
                        i++;
                    }
                }
            }

            IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(item);
            if (integrationPathItem && integrationPathItem->is_integrationPathSegment()) {
                PathItem *pathItem=integrationPathItem->getPathItem();
                if (pathItem) selectItem(pathItem);
            }

            PortItem *portItem=dynamic_cast<PortItem *>(item);
            if (portItem && portItem->is_port()) {
                PathItem *pathItem=portItem->getPathItem();
                std::cout << "pathItem=" << pathItem << std::endl; std::cout.flush();
                if (pathItem) selectItem(pathItem);
            }

            BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(item);
            if (boundaryItem && boundaryItem->is_boundary()) {
                PathItem *pathItem=boundaryItem->getPathItem();
                if (pathItem) selectItem(pathItem);
            }
        }
    }

    bool hasSelectedItems (int itemType)
    {
        if (selectTracking) {std::cout << "ItemTracker::hasSelectedItems" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            if (selectedItems[i] && selectedItems[i]->get_itemType() == itemType) return true;
            i++;
        }
        return false;
    }

    bool hasOneSelectedItem ()
    {
        if (selectTracking) {std::cout << "ItemTracker::hasOneSelectedItem" << std::endl; std::cout.flush();}

        if (selectedItems.count() == 1) return true;
        return false;
    }

    bool hasAnySelectedItems ()
    {
        if (selectTracking) {std::cout << "ItemTracker::hasAnySelectedItems" << std::endl; std::cout.flush();}

        if (selectedItems.count() > 0) return true;
        return false;
    }

    bool hasOneFaceSelected ()
    {
        if (selectTracking) {std::cout << "ItemTracker::hasOneFaceSelected" << std::endl; std::cout.flush();}

        int count=0;
        long unsigned int index;
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            if (selectedItems[i]) {
                Handle(AIS_Shape) shape=selectedItems[i]->getShape();
                if (!shape.IsNull()) {
                    TopAbs_ShapeEnum shapeType=shape->Shape().ShapeType();
                    if (shapeType == TopAbs_FACE) {
                        count++;
                        index=i;
                    }
                }
            }
            i++;
        }

        if (count == 1) {
            Handle(AIS_Shape) shape=selectedItems[index]->getShape();
            if (!shape.IsNull()) {
                TopAbs_ShapeEnum shapeType=shape->Shape().ShapeType();
                if (shapeType == TopAbs_FACE) return true;
            }
        }
        return false;
    }

    int get_pathSelectedCount () {return selectedItems.pathCount();}
    int get_portSelectedCount () {return selectedItems.portCount();}
    int get_boundarySelectedCount () {return selectedItems.boundaryCount();}

    // unselect

    void unselectItemShape (Handle(AIS_Shape) shape)
    {
        if (selectTracking) {std::cout << "ItemTracker::unselectShape" << std::endl; std::cout.flush();}

        BaseItem *item=shapeToItemMap[shape];
        if (item) {
            //showItem(item);
            unselectItem(item);  // mesh shapes are not in the map, so need to check for valid item
        }
    }

    void unselectAllItems ()
    {
        if (unselectTracking) {std::cout << "ItemTracker::unselectAllItems" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            unselectItem(i);
            i++;
        }
    }

    void unselectItem (BaseItem *item)
    {
        if (unselectTracking) {std::cout << "ItemTracker::unselectItem" << std::endl; std::cout.flush();}

        if (!item) return;

        RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(item);
        if (rootDrawingItem && rootDrawingItem->is_rootDrawing()) {
            rootDrawingItem->setSelected(Standard_False);
        } else {
            UnselectShape(item->getShape());
            item->setSelected(Standard_False);

            // remove from the list of selected items
            selectedItems.nullify(item);
        }
    }

    void unselectItem (BaseItem *item, long unsigned int index)
    {
        if (unselectTracking) {std::cout << "ItemTracker::unselectItem" << std::endl; std::cout.flush();}

        if (!item) return;


        RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(item);
        if (rootDrawingItem && rootDrawingItem->is_rootDrawing()) {
            rootDrawingItem->setSelected(Standard_False);

            // remove from the list of selected items
            selectedItems.nullify(index);
        } else {
            UnselectShape(item->getShape());
            item->setSelected(Standard_False);

            // remove from the list of selected items
            selectedItems.nullify(index);
        }
    }

    void unselectItem (long unsigned int index)
    {
        if (unselectTracking) {std::cout << "ItemTracker::unselectItem" << std::endl; std::cout.flush();}

        BaseItem *item=selectedItems[index];
        if (!item) return;

        if (item->is_rootDrawing()) {
            item->setSelected(Standard_False);

            // remove from the list of selected items
            selectedItems.nullify(index);
        } else {
            UnselectShape(item->getShape());
            item->setSelected(Standard_False);

            // remove from the list of selected items
            selectedItems.nullify(index);
        }
    }


    // delete

    void deleteItem (BaseItem *item)
    {
        if (deleteTracking) {std::cout << "ItemTracker::deleteItem  item=" << item << std::endl; std::cout.flush();}

        if (!item) return;
        // if (item->is_mesh()) return;
        // if (item->is_sport()) return;

        // if (item->is_root()) return;

        DeleteItem(item);

        RootDrawingItem *rootDrawingItem=dynamic_cast<RootDrawingItem *>(item);
        if (!(rootDrawingItem && rootDrawingItem->is_rootDrawing())) {
            delete item;
            item=nullptr;
        }
    }

    bool isValidDelete ()
    {
        if (deleteTracking) {std::cout << "ItemTracker::isValidDelete" << std::endl; std::cout.flush();}

        // do not enable deletion of subshapes
        if (selectedItems.size() == 0) return false;

        bool performCheck=true;

        // count the items
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            BaseItem *item=selectedItems[i];
            if (item) {
                if (item->is_mesh()) {
                    // nothing to do
                } else if (item->is_port() || item->is_boundary()) {
                    if (!viewerContext->IsDisplayed(item->getShape())) performCheck=false;
                } else if (item->is_sportLabel()) {
                    // nothing to do
                } else {
                    // drawing
                    if (viewerContext->IsDisplayed(item->getShape())) {

                        BaseItem *parent=dynamic_cast<BaseItem *>(item->QTreeWidgetItem::parent());
                        if (parent && !parent->is_rootDrawing()) {
                            // parent must be a COMPOUND
                            //if (parent->getShape()->Shape().ShapeType() == TopAbs_COMPOUND) count++;

                            performCheck=false;
                        }
                    }
                }
            }
            i++;
        }

        if (!performCheck) {
            return false;
        }

        // checks
        i=0;
        while (i < selectedItems.size()) {
            BaseItem *item=selectedItems[i];
            if (item) {
                if (item->is_mesh()) {
                    // nothing to do
                } else if (item->is_port() || item->is_boundary()) {
                    if (!viewerContext->IsDisplayed(item->getShape())) return false;
                } else if (item->is_sportLabel()) {
                    // nothing to do
                } else {
                    // drawing
                    if (!viewerContext->IsDisplayed(item->getShape())) {
                        return false;
                    }

                    BaseItem *parent=dynamic_cast<BaseItem *>(item->QTreeWidgetItem::parent());
                    if (parent && !parent->is_rootDrawing()) {
                        // ToDo: probably have to generalize this at some point

                        // parent must be a COMPOUND
                        //if (parent->get_AIS_Shape()->Shape().ShapeType() != TopAbs_COMPOUND) return false;

                        return false;
                    }
                }
            }
            i++;
        }

        return true;
    }

    // map

    void insertItemToMap (Handle(AIS_Shape) shape, BaseItem *item)
    {
        //std::cout << "ItemTracker::insertItemToMap" << std::endl; std::cout.flush();

        if (!item) return;
        if (shape.IsNull()) return;
        if (item->is_mesh()) return;
        if (item->is_sportLabel()) return;

        if (shape.IsNull()) {std::cout << "ItemTracker::insertItemToMap  ERROR inserting null shape" << std::endl; std::cout.flush();}

        shapeToItemMap.insert({shape,item});
    }

    void removeItemFromMap (BaseItem *item)
    {
        if (!item) return;
        Handle(AIS_Shape) shape=item->getShape();
        if (shape.IsNull()) return;

        // hide the object
        if (viewerContext->IsDisplayed(shape)) viewerContext->Erase(shape,Standard_False);

        // remove it from the hash
        shapeToItemMap.erase(shape);
    }

    // reset

    void reset ()
    {
        //std::cout << "ItemTracker::reset" << std::endl; std::cout.flush();

        visibleItems.clear();
        selectedItems.clear();
        shapeToItemMap.clear();
    }

    std::vector<BaseItem *> getVisibleDrawingItems ()
    {
        //std::cout << "ItemTracking::getVisibleDrawingItems" << std::endl; std::cout.flush();

        std::vector<BaseItem *> copyVisibleItems;
        long unsigned int i=0;
        while (i < visibleItems.size()) {
            if (visibleItems[i]) {
                DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(visibleItems[i]);
                if (drawingItem && drawingItem->is_drawing()) {
                    copyVisibleItems.push_back(drawingItem);
                }
            }
            i++;
        }

        return copyVisibleItems;
    }

    long unsigned int getSelectedItemsSize () {return selectedItems.size();}
    BaseItem* getSelectedItem (long unsigned int i) {return selectedItems[i];}
    long unsigned int getSelectedItemsCount () {return selectedItems.count();}
    void compactSelectedItems () {selectedItems.compact();}

    long unsigned int getVisibleItemsSize () {return visibleItems.size();}
    BaseItem* getVisibleItem (long unsigned int i) {return visibleItems[i];}
    long unsigned int getVisibleItemsCount () {return visibleItems.count();}
    void compactVisibleItems () {visibleItems.compact();}

    void printStats () {
        std::cout << "   Tracker Stats:" << std::endl;
        std::cout << "      visible size = " << visibleItems.size() << std::endl;
        std::cout << "      selected size = " << selectedItems.size() << std::endl; std::cout.flush();
        std::cout << "      visible count = " << visibleItems.count() << std::endl;
        std::cout << "      selected count = " << selectedItems.count() << std::endl; std::cout.flush();
    }

    bool isInMap (Handle(AIS_Shape) shape) {
        BaseItem *item=shapeToItemMap[shape];
        if (item) return true;
        return false;
    }

private:

    void EraseShape (Handle(AIS_Shape) shape) {
        if (shape.IsNull()) return;
        if (!viewerContext->IsDisplayed(shape)) return;
        viewerContext->Erase(shape,Standard_False);
    }

    void DisplayShape (Handle(AIS_Shape) shape)
    {
        if (shape.IsNull()) return;
        viewerContext->Display(shape,Standard_False);
    }

    void SelectShape (Handle(AIS_Shape) shape)
    {
        //std::cout << "ItemTracking::SelectShape" << std::endl; std::cout.flush();
        if (shape.IsNull()) {
            return;
        }

        if (viewerContext->IsSelected(shape)) {
            return;
        }
        viewerContext->AddOrRemoveSelected(shape,Standard_True);
    }

    void ActivateSelectShape (Handle(AIS_Shape) shape)
    {
        //std::cout << "ItemTracking::SelectShape" << std::endl; std::cout.flush();
        if (shape.IsNull()) {
            return;
        }

        if (viewerContext->IsSelected(shape)) {
            return;
        }
        viewerContext->Activate(shape);
        viewerContext->AddOrRemoveSelected(shape,Standard_True);
    }

    void UnselectShape (Handle(AIS_Shape) shape)
    {
        if (shape.IsNull()) return;
        if (!viewerContext->IsSelected(shape)) return;
        viewerContext->AddOrRemoveSelected(shape,Standard_False);
    }

    void DeleteItem (BaseItem *item)
    {
        //std::cout << "ItemTrackign::DeleteItem  item=" << item << std::endl; std::cout.flush();

        if (!item) return;

        unselectItem(item);
        hideItem(item);

        // remove the AIS_Shape from the viewer
        viewerContext->Remove(item->getShape(),Standard_False);
        shapeToItemMap.erase(item->getShape());
        item->getShape().Nullify();

        // process children
        int i=0;
        while (i < item->childCount()) {
            BaseItem *child=dynamic_cast<BaseItem *>(item->child(i));
            DeleteItem(child);
            i++;
        }
    }

    void nullifyVisibleItem (BaseItem *item)
    {
        visibleItems.nullify(item);
    }

    Handle(AIS_InteractiveContext) viewerContext;
    ItemVector visibleItems;
    ItemVector selectedItems;
    std::unordered_map<Handle(AIS_Shape), BaseItem*> shapeToItemMap;

    // for debug
    bool showTracking;
    bool hideTracking;
    bool selectTracking;
    bool unselectTracking;
    bool deleteTracking;
};

#endif // ITEMTRACKING_H
