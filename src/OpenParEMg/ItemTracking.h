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

    void push_back (CustomTreeWidgetItem *item)
    {
        if (item) {
            data.push_back(item);
            currentCount++;
            if (item->is_path()) currentPathCount++;
            if (item->is_port()) currentPortCount++;
            if (item->is_boundary()) currentBoundaryCount++;
        }
    }

    void nullify (long unsigned int index)
    {
        //std::cout << "ItemTracking::nullify  index=" << index << std::endl; std::cout.flush();

        if (data[index]) {
            if (data[index]->is_path()) currentPathCount--;
            if (data[index]->is_port()) currentPortCount--;
            if (data[index]->is_boundary()) currentBoundaryCount--;

            data[index]=nullptr;
            currentCount--;
        }
    }

    // expecting uniqueness, so stop after item is found
    void nullify (CustomTreeWidgetItem *item)
    {
        //std::cout << "ItemTracking::nullify  item=" << item << std::endl; std::cout.flush();
        if (item) {
            long unsigned int i=0;
            while (i < data.size()) {
                if (item == data[i]) {
                    nullify(i);
                    return;
                }
                i++;
            }
            //std::cout << "ASSERT: ItemVector::nullify did not find item=" << item << std::endl; std::cout.flush();
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

    CustomTreeWidgetItem* operator[](long unsigned int index) {
        return data[index];
    }

private:
    std::vector<CustomTreeWidgetItem *> data;
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
            CustomTreeWidgetItem *item=visibleItems[i];
            if (item) {
                if (item->is_mesh()) {
                    long unsigned int j=0;
                    while (j < item->get_meshEntitiesSize()) {
                        DisplayShape(item->get_meshEntity(j));
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

    void showItem (CustomTreeWidgetItem *item)
    {
        if (showTracking) {std::cout << "ItemTracker::showItem" << std::endl; std::cout.flush();}

        if (!item) return;

        // show item
        if (item->is_rootDrawing()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
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
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                showItem(child);
                i++;
            }
        } else if (item->is_path()) {
            if (item->foreground(0) == Qt::black) return;

            DisplayShape(item->getShape());
            item->setForeground(0,Qt::black);
            visibleItems.push_back(item);

            long unsigned int i=0;
            while (i < item->getArrowHeadsSize()) {
                DisplayShape(item->getArrowHead(i));
                i++;
            }

            i=0;
            while (i < item->linkedItems_size()) {
                CustomTreeWidgetItem *linkedItem=item->get_linkedItem(i);
                showItem(linkedItem);
                i++;
            }
        } else if (item->is_rootPort()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                showItem(child);
                i++;
            }
        } else if (item->is_port()) {
            if (item->foreground(0) == Qt::black) return;

            item->setForeground(0,Qt::black);

            int i=0;
            while (i < item->linkedItems_size()) {
                CustomTreeWidgetItem *linkedItem=item->get_linkedItem(i);
                showItem(linkedItem);
                i++;
            }

            i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                showItem(child);
                i++;
            }
        } else if (item->is_rootBoundary()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                showItem(child);
                i++;
            }
        } else if (item->is_boundary()) {
            if (item->foreground(0) == Qt::black) return;

            item->setForeground(0,Qt::black);

            int i=0;
            while (i < item->linkedItems_size()) {
                CustomTreeWidgetItem *linkedItem=item->get_linkedItem(i);
                showItem(linkedItem);
                i++;
            }

            i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                showItem(child);
                i++;
            }
        } else if (item->is_rootMesh()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                showItem(child);
                i++;
            }
        } else if (item->is_mesh()) {
            if (item->foreground(0) == Qt::black) return;
            item->setForeground(0,Qt::black);
            visibleItems.push_back(item);
            long unsigned int i=0;
            while (i < item->get_meshEntitiesSize()){
                DisplayShape(item->get_meshEntity(i));
                i++;
            }
        } else if (item->is_sport()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                showItem(child);
                i++;
            }
        } else if (item->is_sportLabel()) {
            // nothing to do
        } else if (item->is_voltage() || item->is_current()) {
            if (item->foreground(0) == Qt::black) return;
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                showItem(child);
                i++;
            }
        } else if (item->is_integrationPathSegment()) {
            if (item->foreground(0) == Qt::black) return;
            long unsigned int i=0;
            while (i < item->linkedItems_size()) {
                CustomTreeWidgetItem *linkedItem=item->get_linkedItem(i);
                showItem(linkedItem);
                i++;
            }
            item->setForeground(0,Qt::black);
        } else if (item->is_scale()) {
            // nothing to do
        } else if (item->is_impedanceDefinition()) {
            // nothing to do
        } else if (item->is_impedanceCalculation ()) {
            // nothing to do
        } else if (item->is_sportNumber()) {
            // nothing to do
        } else {
            std::cout << "ASSERT: Invalid option in ItemTracking::showItem" << std::endl; std::cout.flush();
            item->print();
        }
    }

    bool isValidShow ()
    {
        if (showTracking) {std::cout << "ItemTracker::isValidShow" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item) {
                if (item->isValidShow()) return true;

                // children
                if (!item->is_drawing()) {
                    int j=0;
                    while (j < item->childCount()) {
                        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
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
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item) {
                int j=0;
                while (j < item->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
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
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item) {
                int j=0;
                while (j < item->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                    if (child->is_voltage() || child->is_current()) {
                        int k=0;
                        while (k < child->childCount()) {
                            CustomTreeWidgetItem *grandChild=(CustomTreeWidgetItem *) child->child(k);
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

    void hideItem (CustomTreeWidgetItem *item)
    {
        if (hideTracking) {std::cout << "ItemTracker::hideItem  item=" << item << std::endl; std::cout.flush();}

        if (!item) return;

        // custom hide
        if (item->is_rootDrawing()) {
            EraseShape(item->getShape());
            nullifyVisibleItem(item);

            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
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
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_rootPath()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_path()) {
            if (item->foreground(0) == Qt::gray) return;  // avoid infinite loop due to crosslinking of paths

            EraseShape(item->getShape());
            item->setForeground(0,Qt::gray);
            nullifyVisibleItem(item);

            long unsigned int i=0;
            while (i < item->getArrowHeadsSize()) {
                EraseShape(item->getArrowHead(i));
                nullifyVisibleItem(item);
                i++;
            }

            i=0;
            while (i < item->linkedItems_size()) {
                hideItem(item->get_linkedItem(i));
                i++;
            }
        } else if (item->is_rootPort()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_port()) {
            if (item->foreground(0) == Qt::gray) return;  // avoid infinite loop due to crosslinking of paths

            item->setForeground(0,Qt::gray);
            nullifyVisibleItem(item);

            int i=0;
            while (i < item->linkedItems_size()) {
                CustomTreeWidgetItem *linkedItem=item->get_linkedItem(i);
                if (item->is_port()) {
                    hideItem(linkedItem);
                }
                i++;
            }

            i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_rootBoundary()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_boundary()) {
            if (item->foreground(0) == Qt::gray) return;  // avoid infinite loop due to crosslinking of paths

            item->setForeground(0,Qt::gray);
            nullifyVisibleItem(item);

            int i=0;
            while (i < item->linkedItems_size()) {
                CustomTreeWidgetItem *linkedItem=item->get_linkedItem(i);
                if (item->is_boundary()) {
                    hideItem(linkedItem);
                }
                i++;
            }

            i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_rootMesh()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_mesh()) {
            if (item->foreground(0) == Qt::gray) return;   // avoid infinite loop due to crosslinking of paths

            item->setForeground(0,Qt::gray);
            nullifyVisibleItem(item);
            long unsigned int i=0;
            while (i < item->get_meshEntitiesSize()){
                EraseShape(item->get_meshEntity(i));
                i++;
            }
        } else if (item->is_sport()) {
            if (item->foreground(0) == Qt::gray) return;

            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_sportLabel()) {
            // nothing to do
        } else if (item->is_voltage() || item->is_current()) {
            if (item->foreground(0) == Qt::gray) return;

            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_integrationPathSegment()) {
            if (item->foreground(0) == Qt::gray) return;

            long unsigned int i=0;
            while (i < item->linkedItems_size()) {
                hideItem(item->get_linkedItem(i));
                i++;
            }
            item->setForeground(0,Qt::gray);
            nullifyVisibleItem(item);
        } else if (item->is_scale()) {
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

    bool isVisibleItem (CustomTreeWidgetItem *item)
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
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item) {
                if (item->isValidHide()) return true;

                // children
                if (!item->is_drawing()) {   // skip drawing for speed
                    int j=0;
                    while (j < item->childCount()) {
                        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
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
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item) {
                int j=0;
                while (j < item->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
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
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item) {
                int j=0;
                while (j < item->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);

                    if (child->is_voltage() || child->is_current()) {
                        int k=0;
                        while (k < child->childCount()) {
                            CustomTreeWidgetItem *grandChild=(CustomTreeWidgetItem *) child->child(k);
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

        CustomTreeWidgetItem *item=shapeToItemMap[shape];
        if (item) {
            //showItem(item);
            selectItem(item);  // mesh shapes are not in the map, so need to check for valid item
        }
    }

    void activateSelectItem (CustomTreeWidgetItem *item)
    {
        //std::cout << "ItemTracking::activateSelectItem" << std::endl; std::cout.flush();

        if (!item) return;

        if (!item->getShape().IsNull()) {
            viewerContext->Activate(item->getShape());
        }

        long unsigned int i=0;
        while (i < item->getArrowHeadsSize()) {
            if (!item->getArrowHead(i).IsNull()) {
                viewerContext->Activate(item->getArrowHead(i));
            }
            i++;
        }

        selectItem(item);
    }

    // assumes it is already in the tracker
    void refreshSelectedItem (CustomTreeWidgetItem *item) {
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

    void selectItem (CustomTreeWidgetItem *item)
    {
        if (selectTracking) {std::cout << "ItemTracker::selectItem" << std::endl; std::cout.flush();}
        if (!item) return;

        // see if the item is already selected
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            if (item == selectedItems[i]) {
                SelectShape(item->getShape());

                long unsigned int j=0;
                while (j < item->getArrowHeadsSize()) {
                    SelectShape(item->getArrowHead(j));
                    j++;
                }

                return;
            }
            i++;
        }

        if (item->is_rootDrawing()) {
            item->setSelected(Standard_True);
            selectedItems.push_back(item);  // xxx
        } else {
            if (!item->getShape().IsNull()) {
                SelectShape(item->getShape());
            }
            item->setSelected(Standard_True);
            selectedItems.push_back(item);

            long unsigned int i=0;
            while (i < item->getArrowHeadsSize()) {
                SelectShape(item->getArrowHead(i));
                i++;
            }

            i=0;
            while (i < item->linkedItems_size()) {
                selectItem(item->get_linkedItem(i));
                i++;
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

        CustomTreeWidgetItem *item=shapeToItemMap[shape];
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

    // void unselectShape (Handle(AIS_Shape) shape)
    // {
    //     if (unselectTracking) {std::cout << "ItemTracker::unselectShape" << std::endl; std::cout.flush();}

    //     CustomTreeWidgetItem *item=shapeToItemMap[shape];
    //     unselectItem(item);
    // }

    void unselectItem (CustomTreeWidgetItem *item)
    {
        if (unselectTracking) {std::cout << "ItemTracker::unselectItem" << std::endl; std::cout.flush();}

        if (!item) return;

        if (item->is_rootDrawing()) {
            item->setSelected(Standard_False);
        } else {
            UnselectShape(item->getShape());
            item->setSelected(Standard_False);

            long unsigned int i=0;
            while (i <  item->getArrowHeadsSize()) {
                UnselectShape(item->getArrowHead(i));
                i++;
            }

            // remove from the list of selected items
            selectedItems.nullify(item);
        }
    }

    void unselectItem (CustomTreeWidgetItem *item, long unsigned int index)
    {
        if (unselectTracking) {std::cout << "ItemTracker::unselectItem" << std::endl; std::cout.flush();}

        if (!item) return;

        if (item->is_rootDrawing()) {
            item->setSelected(Standard_False);

            // remove from the list of selected items
            selectedItems.nullify(index);
        } else {
            UnselectShape(item->getShape());
            item->setSelected(Standard_False);

            long unsigned int i=0;
            while (i <  item->getArrowHeadsSize()) {
                UnselectShape(item->getArrowHead(i));
                i++;
            }

            // remove from the list of selected items
            selectedItems.nullify(index);
        }
    }

    void unselectItem (long unsigned int index)
    {
        if (unselectTracking) {std::cout << "ItemTracker::unselectItem" << std::endl; std::cout.flush();}

        CustomTreeWidgetItem *item=selectedItems[index];
        if (!item) return;

        if (item->is_rootDrawing()) {
            item->setSelected(Standard_False);

            // remove from the list of selected items
            selectedItems.nullify(index);
        } else {
            UnselectShape(item->getShape());
            item->setSelected(Standard_False);

            long unsigned int i=0;
            while (i <  item->getArrowHeadsSize()) {
                UnselectShape(item->getArrowHead(i));
                i++;
            }

            // remove from the list of selected items
            selectedItems.nullify(index);
        }
    }


    // delete

    void deleteItem (CustomTreeWidgetItem *item)
    {
        if (deleteTracking) {std::cout << "ItemTracker::deleteItem  item=" << item << std::endl; std::cout.flush();}

        if (!item) return;
        // if (item->is_mesh()) return;
        // if (item->is_sport()) return;

        // if (item->is_root()) return;

        DeleteItem(item);
        if (!item->is_rootDrawing()) {
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
            CustomTreeWidgetItem *item=selectedItems[i];
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

                        CustomTreeWidgetItem *parent=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
                        if (parent) {
                            // parent must be a COMPOUND
                            //if (parent->getShape()->Shape().ShapeType() == TopAbs_COMPOUND) count++;

                            // parent must be the item Drawing
                            if (parent->text(0).compare("Drawing") != 0) performCheck=false;
                        }
                    }
                }
            }
            i++;
        }

        if (!performCheck) return false;

        // checks
        i=0;
        while (i < selectedItems.size()) {
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item) {
                if (item->is_mesh()) {
                    // nothing to do
                } else if (item->is_port() || item->is_boundary()) {
                    if (!viewerContext->IsDisplayed(item->getShape())) return false;
                } else if (item->is_sportLabel()) {
                    // nothing to do
                } else {
                    // drawing
                    if (!viewerContext->IsDisplayed(item->getShape())) return false;

                    CustomTreeWidgetItem *parent=(CustomTreeWidgetItem *)item->QTreeWidgetItem::parent();
                    if (parent) {
                        // ToDo: probably have to generalize this at some point

                        // parent must be a COMPOUND
                        //if (parent->get_AIS_Shape()->Shape().ShapeType() != TopAbs_COMPOUND) return false;

                        // parent must be the item drawing
                        if (parent->text(0).compare("Drawing") != 0) return false;
                    }
                }
            }
            i++;
        }

        return true;
    }

    // map

    void insertItemToMap (Handle(AIS_Shape) shape, CustomTreeWidgetItem *item)
    {
        //std::cout << "ItemTracker::insertItemToMap" << std::endl; std::cout.flush();

        std::cout << "insertItemToMap:" << std::endl; std::cout.flush();
        if (!item) return;
        std::cout << "   has item" << std::endl; std::cout.flush();
        if (shape.IsNull()) return;
        std::cout << "   has shape" << std::endl; std::cout.flush();
        if (item->is_mesh()) return;
        if (item->is_sportLabel()) return;

        if (shape.IsNull()) {std::cout << "ItemTracker::insertItemToMap  ERROR inserting null shape" << std::endl; std::cout.flush();}

        std::cout << "   insert" << std::endl; std::cout.flush();
        shapeToItemMap.insert({shape,item});
    }

    void removeItemFromMap (CustomTreeWidgetItem *item)
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

    std::vector<CustomTreeWidgetItem *> getVisibleDrawingItems ()
    {
        //std::cout << "ItemTracking::getVisibleDrawingItems" << std::endl; std::cout.flush();

        std::vector<CustomTreeWidgetItem *> copyVisibleItems;
        long unsigned int i=0;
        while (i < visibleItems.size()) {
            if (visibleItems[i] && visibleItems[i]->is_drawing()) {
                copyVisibleItems.push_back(visibleItems[i]);
            }
            i++;
        }

        return copyVisibleItems;
    }

    long unsigned int getSelectedItemsSize () {return selectedItems.size();}
    CustomTreeWidgetItem* getSelectedItem (long unsigned int i) {return selectedItems[i];}
    long unsigned int getSelectedItemsCount () {return selectedItems.count();}
    void compactSelectedItems () {selectedItems.compact();}

    long unsigned int getVisibleItemsSize () {return visibleItems.size();}
    CustomTreeWidgetItem* getVisibleItem (long unsigned int i) {return visibleItems[i];}
    long unsigned int getVisibleItemsCount () {return visibleItems.count();}
    void compactVisibleItems () {visibleItems.compact();}

    void printStats () {
        std::cout << "   Tracker Stats:" << std::endl;
        std::cout << "      visible size = " << visibleItems.size() << std::endl;
        std::cout << "      selected size = " << selectedItems.size() << std::endl; std::cout.flush();
        std::cout << "      visible count = " << visibleItems.count() << std::endl;
        std::cout << "      selected count = " << selectedItems.count() << std::endl; std::cout.flush();
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

    void DeleteItem (CustomTreeWidgetItem *item)
    {
        //std::cout << "ItemTrackign::DeleteItem  item=" << item << std::endl; std::cout.flush();

        if (!item) return;

        unselectItem(item);
        hideItem(item);

        // remove the AIS_Shape from the viewer
        viewerContext->Remove(item->getShape(),Standard_False);
        shapeToItemMap.erase(item->getShape());
        item->getShape().Nullify();

        // remove arrow heads from the viewer
        long unsigned int j=0;
        while (j < item->getArrowHeadsSize()) {
            viewerContext->Remove(item->getArrowHead(j),Standard_False);
            shapeToItemMap.erase(item->getArrowHead(j));
            item->getArrowHead(j).Nullify();
            j++;
        }

        // process children
        int i=0;
        while (i < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
            DeleteItem(child);
            i++;
        }
    }

    void nullifyVisibleItem (CustomTreeWidgetItem *item)
    {
        visibleItems.nullify(item);
    }

    Handle(AIS_InteractiveContext) viewerContext;
    ItemVector visibleItems;
    ItemVector selectedItems;
    std::unordered_map<Handle(AIS_Shape), CustomTreeWidgetItem*> shapeToItemMap;

    // for debug
    bool showTracking;
    bool hideTracking;
    bool selectTracking;
    bool unselectTracking;
    bool deleteTracking;
};

#endif // ITEMTRACKING_H
