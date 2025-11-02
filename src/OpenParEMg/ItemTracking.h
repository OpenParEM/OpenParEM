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

    // show

    void reshowVisibleItems () {
        std::cout << "ItemTracker::reshowVisibleItems" << std::endl; std::cout.flush();

        long unsigned int i=0;
        while (i < visibleItems.size()) {
            CustomTreeWidgetItem *item=visibleItems[i];
            if (item) {
                if (item->is_mesh()) {
                    long unsigned int j=0;
                    while (j < item->get_meshEntitiesSize()){
                        DisplayShape(item->get_meshEntity(j),item->get_displayMode(),item->get_selectionMode());
                        j++;
                    }
                } else {
                    DisplayShape(item->get_AIS_Shape(),item->get_displayMode(), item->get_selectionMode());
                }
            }
            i++;
        }
    }

    void showShape (Handle(AIS_Shape) shape)
    {
        std::cout << "ItemTracker::showShape" << std::endl; std::cout.flush();

        CustomTreeWidgetItem *item=shapeToItemMap[shape];
        if (item) showItem(item);
    }

    void showItem (CustomTreeWidgetItem *item)
    {
        std::cout << "ItemTracker::showItem" << std::endl; std::cout.flush();

        if (!item) return;

        // show item
        if (item->is_mesh()) {
            if (item->is_root()) {
                int i=0;
                while (i < item->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                    showItem(child);
                    unselectItem(child);
                    i++;
                }
            } else {
                long unsigned int i=0;
                while (i < item->get_meshEntitiesSize()){
                    DisplayShape(item->get_meshEntity(i),item->get_displayMode(), item->get_selectionMode());
                    i++;
                }
            }
        } else {
            DisplayShape(item->get_AIS_Shape(),item->get_displayMode(), item->get_selectionMode());
        }

        item->setSelected(Standard_True);
        item->setForeground(0,Qt::black);

        // save to the list of visible items
        visibleItems.push_back(item);
    }

    bool isValidShow ()
    {
        std::cout << "ItemTracker::isValidShow" << std::endl; std::cout.flush();
        std::cout << "ItemTracker::isValidShow  selectedItems.size()=" << selectedItems.size() << std::endl; std::cout.flush();

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item) {
                if (item->is_mesh()) {
                    if (item->is_root()) {
                        int i=0;
                        while (i < item->childCount()) {
                            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                            if (child->foreground(0) == Qt::gray) return true;
                            i++;
                        }
                    } else {
                        if (item->foreground(0) == Qt::gray) return true;
                    }
                } else {
                    if (!viewerContext->IsDisplayed(item->get_AIS_Shape())) return true;
                }
            }
            i++;
        }
        return false;
    }


    // hide

    void hideShape (Handle(AIS_Shape) shape)
    {
        std::cout << "ItemTracker::hideShape" << std::endl; std::cout.flush();

        CustomTreeWidgetItem *item=shapeToItemMap[shape];
        hideItem(item);
    }

    void hideItem (CustomTreeWidgetItem *item)
    {
        std::cout << "ItemTracker::hideItem" << std::endl; std::cout.flush();

        if (!item) return;

        // hide item
        if (item->is_mesh()) {
            if (item->is_root()) {
                int i=0;
                while (i < item->childCount()) {
                    CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                    hideItem(child);
                    i++;
                }
            } else {
                long unsigned int i=0;
                while (i < item->get_meshEntitiesSize()){
                    EraseShape(item->get_meshEntity(i));
                    i++;
                }
            }
        } else {
            EraseShape(item->get_AIS_Shape());
        }
        item->setForeground(0,Qt::gray);

        // remove from the list of visible items
        long unsigned int i=0;
        while (i < visibleItems.size()) {
            if (visibleItems[i] == item) {
                visibleItems.erase(visibleItems.begin()+i);
                break;
            }
            i++;
        }
    }

    // hide only selected items
    void hideItems ()
    {
        std::cout << "ItemTracker::hideItems" << std::endl; std::cout.flush();
        std::cout << "ItemTracker::hideItems  visibleItems.size()=" << visibleItems.size() << std::endl; std::cout.flush();

        int i=0;
        while (i < visibleItems.size()) {
            CustomTreeWidgetItem *item=visibleItems[i];
            if (item) {
                if (item->isSelected()) {
                    hideItem(item);
                    i--;
                }
            }
            i++;
        }
    }

    // hide all items whether selected or not
    void hideAllItems ()
    {
        std::cout << "ItemTracker::hideAllItems" << std::endl; std::cout.flush();

        // hide all items in the list of visible items
        long unsigned int i=0;
        while (i < visibleItems.size()) {
            CustomTreeWidgetItem *item=visibleItems[i];
            if (item) {
                if (item->is_mesh()) {
                    long unsigned int j=0;
                    while (j < item->get_meshEntitiesSize()){
                        EraseShape(item->get_meshEntity(j));
                        j++;
                    }
                } else {
                    EraseShape(item->get_AIS_Shape());
                }
                item->setForeground(0,Qt::gray);
            }
            i++;
        }
        visibleItems.clear();
    }

    bool isValidHide ()
    {
        std::cout << "ItemTracker::isValidHide" << std::endl; std::cout.flush();

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item) {
                if (item->is_mesh()) {
                    if (item->is_root()) {
                        int i=0;
                        while (i < item->childCount()) {
                            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) item->child(i);
                            if (child->foreground(0) == Qt::black) return true;
                            i++;
                        }
                    } else {
                        if (item->foreground(0) == Qt::black) return true;
                    }
                } else {
                    if (viewerContext->IsDisplayed(item->get_AIS_Shape())) return true;
                }
            }
            i++;
        }
        return false;
    }


    // select

    void selectShape (Handle(AIS_Shape) shape)
    {
        std::cout << "ItemTracker::selectShape" << std::endl; std::cout.flush();

        CustomTreeWidgetItem *item=shapeToItemMap[shape];
        if (item) selectItem(item);
    }

    void selectItem (CustomTreeWidgetItem *item)
    {
        std::cout << "ItemTracker::selectItem" << std::endl; std::cout.flush();

        if (!item) return;
        if (item->is_mesh()) return;

        item->setSelected(Standard_True);
        selectedItems.push_back(item);
    }

    bool hasSelectedItems ()
    {
        std::cout << "ItemTracker::hasSelectedItems" << std::endl; std::cout.flush();

        if (selectedItems.size() > 0) {
            std::cout << "place 1" << std::endl; std::cout.flush();
            return true;
        }
        std::cout << "place 2" << std::endl; std::cout.flush();
        return false;
    }

    bool hasOneFaceSelected ()
    {
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
        std::cout << "ItemTracker::unselectAllItems" << std::endl; std::cout.flush();

        // unselect all items from the list of selected items
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item) {
                if (item->is_mesh()) {
                    long unsigned int j=0;
                    while (j < item->get_meshEntitiesSize()) {
                        UnselectShape(item->get_meshEntity(j));
                        j++;
                    }
                } else {
                    UnselectShape(item->get_AIS_Shape());
                }
                item->setSelected(Standard_False);
            }
            i++;
        }
        selectedItems.clear();
    }

    void unselectShape (Handle(AIS_Shape) shape)
    {
        std::cout << "ItemTracker::unselectShape" << std::endl; std::cout.flush();

        CustomTreeWidgetItem *item=shapeToItemMap[shape];
        if (item) unselectItem(item);
    }

    void unselectItem (CustomTreeWidgetItem *unselectItem)
    {
        std::cout << "ItemTracker::unselectItem" << std::endl; std::cout.flush();

        if (!unselectItem) return;
        if (unselectItem->is_mesh()) return;


        UnselectShape(unselectItem->get_AIS_Shape());
        unselectItem->setSelected(Standard_False);

        // remove from the list of selected items
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item) {
                if (item == unselectItem) {
                    selectedItems.erase(selectedItems.begin()+i);
                    break;
                }
            }
            i++;
        }
    }


    // delete

    void deleteItem (CustomTreeWidgetItem *item)
    {
        std::cout << "ItemTracker::deleteItem" << std::endl; std::cout.flush();

        if (!item) return;

        if (!item->is_mesh()) {
            if (item->is_root()) return;
            DeleteItem(item);
            delete item;
        }
    }

    bool isValidDelete ()
    {
        std::cout << "ItemTracker::isValidDelete" << std::endl; std::cout.flush();

        long unsigned int i=0;
        while (i < selectedItems.size()) {
            CustomTreeWidgetItem *item=selectedItems[i];
            if (item) {
                if (!item->is_mesh()) {
                    if (!viewerContext->IsDisplayed(item->get_AIS_Shape())) return false;
                }
            }
            i++;
        }
        return true;
    }

    // map

    void insertItemToMap (Handle(AIS_Shape) shape, CustomTreeWidgetItem *item)
    {
        std::cout << "ItemTracker::insertItemToMap" << std::endl; std::cout.flush();

        if (!item) return;
        if (shape.IsNull()) return;

        if (!item->is_mesh()) {
            shapeToItemMap.insert({shape,item});
        }
    }

    // reset

    void reset ()
    {
        std::cout << "ItemTracker::reset" << std::endl; std::cout.flush();

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
        if (!item) return;

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

    Handle(AIS_InteractiveContext) viewerContext;
    std::vector<CustomTreeWidgetItem *> visibleItems;
    std::vector<CustomTreeWidgetItem *> selectedItems;
    std::unordered_map<Handle(AIS_Shape), CustomTreeWidgetItem*> shapeToItemMap;
};

#endif // ITEMTRACKING_H
