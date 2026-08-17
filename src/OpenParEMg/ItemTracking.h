////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEMg - A GUI for OpenParEM3D                                      //
//    Copyright (C) 2026 Brian Young                                          //
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

    // report duplicates
    void duplicateAudit () {
        std::cout << "   duplicates:" << std::endl; std::cout.flush();
        int limit=data.size(); limit--;
        int i=0;
        while (i < limit) {
            if (data[i]) {
                long unsigned int j=i+1;
                while (j < data.size()) {
                    if (data[j]) {
                        if (data[i] == data[j]) {
                            std::cout << "      " << data[i]->text(0).toStdString() << std::endl;
                        }
                    }
                    j++;
                }
            }
            i++;
        }
    }

    // report items that do not appear in itemVector
    void crossDuplicateAudit (ItemVector *itemVector)
    {
        std::cout << "   missing entries:" << std::endl; std::cout.flush();
        long unsigned int i=0;
        while (i < data.size()) {
            if (data[i]) {
                bool found;
                long unsigned int j=0;
                while (j < itemVector->size()) {
                    if ((*itemVector)[j]) {
                        if (data[i] == (*itemVector)[j]) {
                            found=true;
                            break;
                        }
                    }
                    j++;
                }
                if (!found) {
                    std::cout << "      " << data[i]->text(0).toStdString() << std::endl;
                }
            }
            i++;
        }
    }

    // report items that are not actually selected
    void selectedItemAudit (Handle(AIS_InteractiveContext) viewerContext)
    {
        std::cout << "   unselected items:" << std::endl; std::cout.flush();
        long unsigned int i=0;
        while (i < data.size()) {
            if (data[i]) {
                if (!viewerContext->IsSelected(data[i]->getShape())) {
                    std::cout << "      " << data[i]->text(0).toStdString() << std::endl;
                }
            }
            i++;
        }
    }

    // each item must be in the map
    void itemInMapAudit (std::unordered_map<Handle(AIS_Shape), BaseItem*> *shapeToItemMap)
    {
        std::cout << "   missing entries:" << std::endl; std::cout.flush();
        long unsigned int i=0;
        while (i < data.size()) {
            if (data[i]) {
                bool found=false;
                for (const auto& pair : *shapeToItemMap) {
                    if (pair.second) {
                        if (data[i] == pair.second) {
                            found=true;
                            break;
                        }
                    }
                }
                if (!found) {
                    std::cout << "      " << data[i]->text(0).toStdString() << std::endl;
                }
            }
            i++;
        }
    }

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

    // show

    void showItem (BaseItem *item)
    {
        if (showTracking) {std::cout << "ItemTracker::showItem" << std::endl; std::cout.flush();}

        if (!item) return;
        DisplayShape(item->getShape());
        item->setForeground(0,Qt::black);
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

    void hideItem (BaseItem *item)
    {
        if (hideTracking) {std::cout << "ItemTracker::hideItem  item=" << item << std::endl; std::cout.flush();}

        if (!item) return;
        //if (item->foreground(0) == Qt::gray) return;
        EraseShape(item->getShape());
        item->setForeground(0,Qt::gray);
        //nullifyVisibleItem(item);
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
            selectItem(item);  // mesh shapes are not in the map, so need to check for valid item
        }
    }

    void activateSelectItem (BaseItem *item)
    {
        if (!item) return;

        if (!item->getShape().IsNull()) {
            viewerContext->Activate(item->getShape());
        }

        selectItem(item);
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

    void simpleSelectDrawingItem (DrawingItem *drawingItem)
    {
        if (!drawingItem) return;

        if (!drawingItem->getShape().IsNull()) {
            SelectShape(drawingItem->getShape());
        }
        selectedItems.push_back(drawingItem);
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
        } else if (item->is_drawing()) {
            if (!item->getShape().IsNull()) {
                SelectShape(item->getShape());
            }
            item->setSelected(Standard_True);
            selectedItems.push_back(item);
        } else {

            // select the item
            if (!item->getShape().IsNull()) {
                SelectShape(item->getShape());
            }
            item->setSelected(Standard_True);
            selectedItems.push_back(item);

            // select the linked items for paths
            PathItem *pathItem=dynamic_cast<PathItem *>(item);
            if (pathItem) {
                long unsigned int i=0;
                while (i < pathItem->linkedItems_size()) {
                    if (pathItem->get_linkedItem(i)) {
                        if (!pathItem->get_linkedItem(i)->getShape().IsNull()) {
                            SelectShape(pathItem->get_linkedItem(i)->getShape());
                        }
                        pathItem->get_linkedItem(i)->setSelected(Standard_True);
                        selectedItems.push_back(pathItem->get_linkedItem(i));
                    }
                    i++;
                }
            }

            // select the path item

            IntegrationPathItem *integrationPathItem=dynamic_cast<IntegrationPathItem *>(item);
            if (integrationPathItem) {
                PathItem *pathItem=integrationPathItem->getPathItem();
                if (pathItem) {
                    if (!pathItem->getShape().IsNull()) {
                        SelectShape(pathItem->getShape());
                    }
                    pathItem->setSelected(Standard_True);
                    selectedItems.push_back(pathItem);
                }
            }

            PortItem *portItem=dynamic_cast<PortItem *>(item);
            if (portItem) {
                PathItem *pathItem=portItem->getPathItem();
                if (pathItem) {
                    if (!pathItem->getShape().IsNull()) {
                        SelectShape(pathItem->getShape());
                    }
                    pathItem->setSelected(Standard_True);
                    selectedItems.push_back(pathItem);
                }
            }

            BoundaryItem *boundaryItem=dynamic_cast<BoundaryItem *>(item);
            if (boundaryItem) {
                PathItem *pathItem=boundaryItem->getPathItem();
                if (pathItem) {
                    if (!pathItem->getShape().IsNull()) {
                        SelectShape(pathItem->getShape());
                    }
                    pathItem->setSelected(Standard_True);
                    selectedItems.push_back(pathItem);
                }
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

    int get_pathSelectedCount () {return selectedItems.pathCount();}
    int get_portSelectedCount () {return selectedItems.portCount();}
    int get_boundarySelectedCount () {return selectedItems.boundaryCount();}

    // unselect

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

    bool isValidDelete ()
    {
        if (deleteTracking) {std::cout << "ItemTracker::isValidDelete" << std::endl; std::cout.flush();}

        // do not enable deletion of subshapes
        if (selectedItems.size() == 0) return false;

        // only top-level drawing items can be deleted
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            DrawingItem *drawingItem=dynamic_cast<DrawingItem *>(selectedItems[i]);
            if (drawingItem && drawingItem->is_drawing()) {
                BaseItem *parentItem=dynamic_cast<BaseItem *>(drawingItem->QTreeWidgetItem::parent());
                if (parentItem && !parentItem->is_rootDrawing()) return false;
            }
            i++;
        }

        return true;
    }


    // map

    void insertItemToMap (Handle(AIS_Shape) shape, BaseItem *item)
    {
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
        selectedItems.clear();
        shapeToItemMap.clear();
    }

    long unsigned int getSelectedItemsSize () {return selectedItems.size();}
    BaseItem* getSelectedItem (long unsigned int i) {return selectedItems[i];}
    long unsigned int getSelectedItemsCount () {return selectedItems.count();}
    void compactSelectedItems () {selectedItems.compact();}

    //long unsigned int getVisibleItemsSize () {return visibleItems.size();}
    //BaseItem* getVisibleItem (long unsigned int i) {return visibleItems[i];}
    //long unsigned int getVisibleItemsCount () {return visibleItems.count();}
    //void compactVisibleItems () {visibleItems.compact();}

    void printStats () {
        std::cout << "   Tracker Stats:" << std::endl;
        //std::cout << "      visible size = " << visibleItems.size() << std::endl;
        std::cout << "      selected size = " << selectedItems.size() << std::endl; std::cout.flush();
        //std::cout << "      visible count = " << visibleItems.count() << std::endl;
        std::cout << "      selected count = " << selectedItems.count() << std::endl; std::cout.flush();
    }

    void audit ()
    {
        std::cout << "***************************************************" << std::endl;
        // std::cout << "Visible Items Audit:" << std::endl;
        // visibleItems.duplicateAudit();

        std::cout << "Selected Items Audit:" << std::endl;
        selectedItems.duplicateAudit();

        // std::cout << "Selected vs. Visible Items Audit" << std::endl;
        // selectedItems.crossDuplicateAudit(&visibleItems);

        // std::cout << "Visible Items vs. Shape-to-Item Map Audit" << std::endl;
        // visibleItems.itemInMapAudit(&shapeToItemMap);

        std::cout << "Selected Items vs. Shape-to-Item Map Audit" << std::endl;
        selectedItems.itemInMapAudit(&shapeToItemMap);

        std::cout << "Selected Items vs. Drawing Items Selected Audit" << std::endl;
        selectedItems.selectedItemAudit(viewerContext);

        std::cout << "Shape-to-Item Map vs. Shape-to-Item Map Audit" << std::endl;
        std::cout << "   duplicates" << std::endl;
        for (const auto& pair1 : shapeToItemMap) {
            if (pair1.second) {
                int count=0;
                for (const auto& pair2 : shapeToItemMap) {
                    if (pair2.second) {
                        if (pair1.second == pair2.second) count++;
                    }
                }
                if (count > 1) std::cout << "      " << pair1.second->text(0).toStdString() << std::endl;
            }
        }

        std::cout << "***************************************************" << std::endl;
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
        if (shape.IsNull()) {
            return;
        }

        if (viewerContext->IsSelected(shape)) {
            return;
        }
        viewerContext->AddOrRemoveSelected(shape,Standard_True);
    }

    void UnselectShape (Handle(AIS_Shape) shape)
    {
        if (shape.IsNull()) return;
        if (!viewerContext->IsSelected(shape)) return;
        viewerContext->AddOrRemoveSelected(shape,Standard_False);
    }

    Handle(AIS_InteractiveContext) viewerContext;
    //ItemVector visibleItems;
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
