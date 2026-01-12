#ifndef ITEMTRACKING_H
#define ITEMTRACKING_H

#include <AIS_InteractiveContext.hxx>
#include "CustomTreeWidgetItem.h"

class ItemTracker : public QObject
{
    Q_OBJECT

public:

    ItemTracker (const Handle(AIS_InteractiveContext)& context) : viewerContext(context) {}

    ~ItemTracker() {}

    bool showTracking=true;
    bool hideTracking=true;
    bool selectTracking=true;
    bool unselectTracking=true;
    bool deleteTracking=true;

    // show

    void reshowVisibleItems () {
        if (showTracking) {std::cout << "ItemTracker::reshowVisibleItems" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < visibleItems.size()) {
            CustomTreeWidgetItem *item=visibleItems[i];
            if (item->is_mesh()) {
                long unsigned int j=0;
                while (j < item->get_meshEntitiesSize()) {
                    DisplayShape(item->get_meshEntity(j),item->get_displayMode(),item->get_selectionMode());
                    j++;
                }
            } else {
                EraseShape(item->get_AIS_Shape());
                DisplayShape(item->get_AIS_Shape(),item->get_displayMode(), item->get_selectionMode());
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

        if (item == nullptr) {
            std::cout << "ASSERT: ItemTracking::showItem passed a null pointer." << std::endl; std::cout.flush();
            return;
        }

        // show item
        if (item->is_rootDrawing()) {
            if (item->foreground(0) == Qt::black) return;
            DisplayShape(item->get_AIS_Shape(),item->get_displayMode(),item->get_selectionMode());
            item->setForeground(0,Qt::black);
            visibleItems.push_back(item);
        } else if (item->is_drawing()) {
            if (item->foreground(0) == Qt::black) return;
            DisplayShape(item->get_AIS_Shape(),item->get_displayMode(),item->get_selectionMode());
            item->setForeground(0,Qt::black);
            visibleItems.push_back(item);
        } else if (item->is_rootPath()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                showItem(child);
                i++;
            }
        } else if (item->is_path()) {
            if (item->foreground(0) == Qt::black) return;

            DisplayShape(item->get_AIS_Shape(),item->get_displayMode(),item->get_selectionMode());
            item->setForeground(0,Qt::black);
            visibleItems.push_back(item);

            long unsigned int i=0;
            while (i < item->get_arrowHeads_size()) {
                DisplayShape(item->get_arrowHead(i),item->get_displayMode(),item->get_selectionMode());
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
        // ToDo: boundary
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
                DisplayShape(item->get_meshEntity(i),item->get_displayMode(),item->get_selectionMode());
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
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                showItem(child);
                i++;
            }
        } else if (item->is_integrationPathSegment()) {
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
        }
    }

    bool isValidShow ()
    {
        if (showTracking) {std::cout << "ItemTracker::isValidShow" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            CustomTreeWidgetItem *item=selectedItems[i];

            // item
            if (item->isValidShow()) return true;

            // children
            if (!item->is_drawing()) {  // skip drawing for speed
                int j=0;
                while (j < item->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                    if (child->isValidShow()) return true;
                    j++;
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

            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                if (child->is_integrationPathSegment()) {
                    if (child->isValidShow()) return true;
                }
                j++;
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
        if (hideTracking) {std::cout << "ItemTracker::hideItem" << std::endl; std::cout.flush();}

        if (item == nullptr) {
            std::cout << "ASSERT: ItemTracking::hideItem passed a null pointer." << std::endl; std::cout.flush();
            return;
        }

        if (item->foreground(0) == Qt::gray) return;

        if (item->is_rootDrawing()) {
            EraseShape(item->get_AIS_Shape());
            item->setForeground(0,Qt::gray);
            removeVisibleItem(item);
        } else if (item->is_drawing()) {
            EraseShape(item->get_AIS_Shape());
            item->setForeground(0,Qt::gray);
            removeVisibleItem(item);
        } else if (item->is_rootPath()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_path()) {
            EraseShape(item->get_AIS_Shape());
            item->setForeground(0,Qt::gray);
            removeVisibleItem(item);

            long unsigned int i=0;
            while (i < item->get_arrowHeads_size()) {
                EraseShape(item->get_arrowHead(i));
                removeVisibleItem(item);
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
            item->setForeground(0,Qt::gray);
            removeVisibleItem(item);

            int i=0;
            while (i < item->linkedItems_size()) {
                CustomTreeWidgetItem *linkedItem=item->get_linkedItem(i);
                hideItem(linkedItem);
                i++;
            }

            i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        // ToDo: boundary
        } else if (item->is_rootMesh()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_mesh()) {
            item->setForeground(0,Qt::gray);
            removeVisibleItem(item);
            long unsigned int i=0;
            while (i < item->get_meshEntitiesSize()){
                EraseShape(item->get_meshEntity(i));
                i++;
            }
        } else if (item->is_sport()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_sportLabel()) {
            // nothing to do
        } else if (item->is_voltage() || item->is_current()) {
            int i=0;
            while (i < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                hideItem(child);
                i++;
            }
        } else if (item->is_integrationPathSegment()) {
            long unsigned int i=0;
            while (i < item->linkedItems_size()) {
                hideItem(item->get_linkedItem(i));
                i++;
            }
            item->setForeground(0,Qt::gray);
            removeVisibleItem(item);
        } else if (item->is_scale()) {
            // nothing to do
        } else if (item->is_impedanceDefinition()) {
            // nothing to do
        } else if (item->is_impedanceCalculation ()) {
            // nothing to do
        } else if (item->is_sportNumber()) {
            // nothing to do
        } else {
            std::cout << "ASSERT: Invalid option in ItemTracking::hideItem" << std::endl; std::cout.flush();
        }
    }

    // hide only selected items
    void hideItems ()
    {
        if (hideTracking) {std::cout << "ItemTracker::hideItems" << std::endl; std::cout.flush();}

        int i=0;
        while (i < selectedItems.size()) {
            hideItem(selectedItems[i]);
            i++;
        }

        selectedItems.clear();
    }

    // hide all items whether selected or not
    void hideAllItems ()
    {
        if (hideTracking) {std::cout << "ItemTracker::hideAllItems" << std::endl; std::cout.flush();}

        int itemCount=visibleItems.size();
        while (visibleItems.size()) {
            hideItem(visibleItems[0]);
            if (itemCount == visibleItems.size()) {
                std::cout << "ASSERT: ItemTracking::hideAllItems failed with " << visibleItems.size() << " items remaining." << std::endl; std::cout.flush();
                break;
            }
            itemCount=visibleItems.size();
        }

        selectedItems.clear();
    }

    bool isValidHide ()
    {
        if (hideTracking) {std::cout << "ItemTracker::isValidHide" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            CustomTreeWidgetItem *item=selectedItems[i];

            // item
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

            int j=0;
            while (j < item->childCount()) {
                CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(j);
                if (child->is_integrationPathSegment()) {
                    if (child->isValidHide()) return true;
                }
                j++;
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

            i++;
        }
        return false;
    }


    // select

    void selectShape (Handle(AIS_Shape) shape)
    {
        if (selectTracking) {std::cout << "ItemTracker::selectShape" << std::endl; std::cout.flush();}

        CustomTreeWidgetItem *item=shapeToItemMap[shape];
        if (item) selectItem(item);  // mesh shapes are not in the map, so need to check for valid item
    }

    void selectItem (CustomTreeWidgetItem *item)
    {
        if (selectTracking) {std::cout << "ItemTracker::selectItem" << std::endl; std::cout.flush();}

        if (item->isSelected()) return;

        if (!item->get_AIS_Shape().IsNull()) {
            SelectShape(item->get_AIS_Shape());
        }
        item->setSelected(Standard_True);
        selectedItems.push_back(item);

        int i=0;
        while (i < item->get_arrowHeads_size()) {
            SelectShape(item->get_arrowHead(i));
            i++;
        }

        long unsigned int j=0;
        while (j < item->linkedItems_size()) {
            selectItem(item->get_linkedItem(j));
            j++;
        }
    }

    bool hasSelectedItems (int type)
    {
        if (selectTracking) {std::cout << "ItemTracker::hasSelectedItems" << std::endl; std::cout.flush();}

        int i=0;
        while (i < selectedItems.size()) {
            if (selectedItems[i]->get_type() == type) return true;
            i++;
        }
        return false;
    }

    bool hasOneSelectedItem ()
    {
        if (selectTracking) {std::cout << "ItemTracker::hasOneSelectedItem" << std::endl; std::cout.flush();}

        if (selectedItems.size() == 1) return true;
        return false;
    }

    bool hasAnySelectedItems ()
    {
        if (selectTracking) {std::cout << "ItemTracker::hasAnySelectedItems" << std::endl; std::cout.flush();}

        if (selectedItems.size() > 0) return true;
        return false;
    }

    bool hasOneFaceSelected ()
    {
        if (selectTracking) {std::cout << "ItemTracker::hasOneFaceSelected" << std::endl; std::cout.flush();}

        //xxx
        int count=0;
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            Handle(AIS_Shape) shape=selectedItems[i]->get_AIS_Shape();
            if (!shape.IsNull()) {
                TopAbs_ShapeEnum shapeType=shape->Shape().ShapeType();
                if (shapeType == TopAbs_FACE) count++;
            }
            i++;
        }
        std::cout << "face count=" << count << std::endl; std::cout.flush();


        if (selectedItems.size() == 1) {
            Handle(AIS_Shape) shape=selectedItems[0]->get_AIS_Shape();
            if (!shape.IsNull()) {
                TopAbs_ShapeEnum shapeType=shape->Shape().ShapeType();
                if (shapeType == TopAbs_FACE) return true;
            }
        }
        return false;
    }


    // unselect

    void unselectAllItems ()
    {
        if (unselectTracking) {std::cout << "ItemTracker::unselectAllItems" << std::endl; std::cout.flush();}

        // unselect all items from the list of selected items
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item->is_mesh()) {
                long unsigned int j=0;
                while (j < item->get_meshEntitiesSize()) {
                    UnselectShape(item->get_meshEntity(j));
                    j++;
                }
            } else if (item->is_sportLabel()) {
                // nothing to do
            } else {
                UnselectShape(item->get_AIS_Shape());

                long unsigned int i=0;
                while (i < item->get_arrowHeads_size()) {
                    UnselectShape(item->get_arrowHead(i));
                    i++;
                }
            }
            item->setSelected(Standard_False);
            i++;
        }
        selectedItems.clear();
    }

    // void unselectShape (Handle(AIS_Shape) shape)
    // {
    //     if (unselectTracking) {std::cout << "ItemTracker::unselectShape" << std::endl; std::cout.flush();}

    //     CustomTreeWidgetItem *item=shapeToItemMap[shape];
    //     unselectItem(item);
    // }

    void unselectItem (CustomTreeWidgetItem *unselectItem)
    {
        if (unselectTracking) {std::cout << "ItemTracker::unselectItem" << std::endl; std::cout.flush();}

        if (!unselectItem) return;

        UnselectShape(unselectItem->get_AIS_Shape());
        unselectItem->setSelected(Standard_False);

        long unsigned int i=0;
        while (i <  unselectItem->get_arrowHeads_size()) {
            UnselectShape(unselectItem->get_arrowHead(i));
            i++;
        }

        // remove from the list of selected items
        i=0;
        while (i < selectedItems.size()) {
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item == unselectItem) {
                selectedItems.erase(selectedItems.begin()+i);
                break;
            }
            i++;
        }
    }


    // delete

    void deleteItem (CustomTreeWidgetItem *item)
    {
        if (deleteTracking) {std::cout << "ItemTracker::deleteItem" << std::endl; std::cout.flush();}

        // if (item->is_mesh()) return;
        // if (item->is_sport()) return;

        // if (item->is_root()) return;

        DeleteItem(item);
        delete item;
    }

    bool isValidDelete ()
    {
        if (deleteTracking) {std::cout << "ItemTracker::isValidDelete" << std::endl; std::cout.flush();}

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item->is_mesh()) {
                // nothing to do
            } else if (item->is_port() || item->is_boundary()) {
                if (!viewerContext->IsDisplayed(item->get_AIS_Shape())) return false;
            } else if (item->is_sportLabel()) {
                // nothing to do
            } else {
                // drawing
                if (!viewerContext->IsDisplayed(item->get_AIS_Shape())) return false;
            }
            i++;
        }
        return true;
    }

    // map

    void insertItemToMap (Handle(AIS_Shape) shape, CustomTreeWidgetItem *item)
    {
        //std::cout << "ItemTracker::insertItemToMap" << std::endl; std::cout.flush();

        if (shape.IsNull()) return;
        if (item->is_mesh()) return;
        if (item->is_sportLabel()) return;

        shapeToItemMap.insert({shape,item});
    }

    // reset

    void reset ()
    {
        //std::cout << "ItemTracker::reset" << std::endl; std::cout.flush();

        visibleItems.clear();
        selectedItems.clear();
        shapeToItemMap.clear();
    }

private:

    void EraseShape (Handle(AIS_Shape) shape) {
        if (shape.IsNull()) return;
        if (!viewerContext->IsDisplayed(shape)) return;
        viewerContext->Erase(shape,Standard_False);
    }

    void DisplayShape (Handle(AIS_Shape) shape, int displayMode, int selectionMode)
    {
        if (shape.IsNull()) return;
        viewerContext->Display(shape,displayMode,selectionMode,Standard_False);
    }

    void SelectShape (Handle(AIS_Shape) shape)
    {
        if (shape.IsNull()) return;
        if (viewerContext->IsSelected(shape)) return;
        viewerContext->AddOrRemoveSelected(shape,Standard_False);
    }

    void UnselectShape (Handle(AIS_Shape) shape)
    {
        if (shape.IsNull()) return;
        if (!viewerContext->IsSelected(shape)) return;
        viewerContext->AddOrRemoveSelected(shape,Standard_False);
    }

    void DeleteItem (CustomTreeWidgetItem *item)
    {
        unselectItem(item);
        hideItem(item);

        // remove the AIS_Shape
        viewerContext->Remove(item->get_AIS_Shape(),Standard_False);
        shapeToItemMap.erase(item->get_AIS_Shape());
        item->get_AIS_Shape().Nullify();

        // process children
        int i=0;
        while (i < item->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *)item->child(i);
            DeleteItem(child);
            i++;
        }
    }

    // remove from the list of visible items
    void removeVisibleItem (CustomTreeWidgetItem *item)
    {
        long unsigned int i=0;
        while (i < visibleItems.size()) {
            if (visibleItems[i] == item) {
                visibleItems.erase(visibleItems.begin()+i);
                return;
            }
            i++;
        }
    }

    Handle(AIS_InteractiveContext) viewerContext;
    std::vector<CustomTreeWidgetItem *> visibleItems;
    std::vector<CustomTreeWidgetItem *> selectedItems;
    std::unordered_map<Handle(AIS_Shape), CustomTreeWidgetItem*> shapeToItemMap;
};

#endif // ITEMTRACKING_H
