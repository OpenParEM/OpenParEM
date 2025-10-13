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
        long unsigned int i=0;
        while (i < visibleItems.size()) {
            CustomTreeWidgetItem *item=visibleItems[i];
            DisplayShape(item->get_AIS_Shape(),item->get_displayMode(), item->get_selectionMode());
            i++;
        }
    }

    void showShape (Handle(AIS_Shape) shape)
    {
        CustomTreeWidgetItem *item=shapeToItemMap[shape];
        showItem(item);
    }

    void showItem (CustomTreeWidgetItem *item)
    {
        //std::cout << "showItem: before: visibleItems.size()=" << visibleItems.size() << std::endl; std::cout.flush();
        //std::cout << "                  selectedItems.size()=" << selectedItems.size() << std::endl; std::cout.flush();
        // show item
        DisplayShape(item->get_AIS_Shape(),item->get_displayMode(), item->get_selectionMode());
        //SelectShape(item->get_AIS_Shape());
        item->setSelected(Standard_True);
        item->setForeground(0,Qt::black);

        // save to the list of visible items
        visibleItems.push_back(item);
        //selectedItems.push_back(item);
        //std::cout << "showItem: after:  visibleItems.size()=" << visibleItems.size() << std::endl; std::cout.flush();
        //std::cout << "                  selectedItems.size()=" << selectedItems.size() << std::endl; std::cout.flush();
    }

    bool isValidShow ()
    {
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            if (!viewerContext->IsDisplayed(selectedItems[i]->get_AIS_Shape())) return true;
            i++;
        }
        return false;
    }


    // hide

    void hideShape (Handle(AIS_Shape) shape)
    {
        CustomTreeWidgetItem *item=shapeToItemMap[shape];
        hideItem(item);
    }

    void hideItem (CustomTreeWidgetItem *item)
    {
        //std::cout << "hideItem: before: visibleItems.size()=" << visibleItems.size() << std::endl; std::cout.flush();
        //std::cout << "                  selectedItems.size()=" << selectedItems.size() << std::endl; std::cout.flush();

        // unselect
        //unselectItem(item);
        //item->setSelected(Standard_True);

        // hide item
        EraseShape(item->get_AIS_Shape());
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
        //std::cout << "hideItem: after:  visibleItems.size()=" << visibleItems.size() << std::endl; std::cout.flush();
        //std::cout << "                  selectedItems.size()=" << selectedItems.size() << std::endl; std::cout.flush();
    }

    // hide only selected items
    void hideItems () {
        long unsigned int i=0;
        while (i < visibleItems.size()) {
            if (visibleItems[i]->isSelected()) {
                hideItem(visibleItems[i]);
                i--;
            }
            i++;
        }
    }

    // hide all items whether selected or not
    void hideAllItems () {

        // hide all items in the list of visible items
        long unsigned int i=0;
        while (i < visibleItems.size()) {
            EraseShape(visibleItems[i]->get_AIS_Shape());
            visibleItems[i]->setForeground(0,Qt::gray);
            i++;
        }
        visibleItems.clear();
    }

    bool isValidHide ()
    {
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            if (viewerContext->IsDisplayed(selectedItems[i]->get_AIS_Shape())) return true;
            i++;
        }
        return false;
    }


    // select

    void selectShape (Handle(AIS_Shape) shape)
    {
        CustomTreeWidgetItem *item=shapeToItemMap[shape];
        selectItem(item);
    }

    void selectItem (CustomTreeWidgetItem *item)
    {
        //std::cout << "selectItem: before: visibleItems.size()=" << visibleItems.size() << std::endl; std::cout.flush();
        //std::cout << "                    selectedItems.size()=" << selectedItems.size() << std::endl; std::cout.flush();
        // select item
        SelectShape(item->get_AIS_Shape());
        item->setSelected(Standard_True);

        // save to the list of selected items
        selectedItems.push_back(item);
        //std::cout << "selectItem: after: visibleItems.size()=" << visibleItems.size() << std::endl; std::cout.flush();
        //std::cout << "                   selectedItems.size()=" << selectedItems.size() << std::endl; std::cout.flush();
    }

    bool hasSelectedItems ()
    {
        if (selectedItems.size() > 0) return true;
        return false;
    }


    // unselect

    void unselectAllItems () {

        // unselect all items from the list of selected items
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            UnselectShape(selectedItems[i]->get_AIS_Shape());
            selectedItems[i]->setSelected(Standard_False);
            i++;
        }
        selectedItems.clear();
    }

    void unselectShape (Handle(AIS_Shape) shape)
    {
        CustomTreeWidgetItem *item=shapeToItemMap[shape];
        unselectItem(item);
    }

    void unselectItem (CustomTreeWidgetItem *item)
    {
        // unselect item
        UnselectShape(item->get_AIS_Shape());
        item->setSelected(Standard_False);

        // remove from the list of selected items
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            if (selectedItems[i] == item) {
                selectedItems.erase(selectedItems.begin()+i);
                break;
            }
            i++;
        }
    }

    // delete

    void deleteItem (CustomTreeWidgetItem *item)
    {
        if (item->is_root()) return;
        if (item->is_drawing()) drawingCount--;
        if (item->is_port()) portCount--;
        if (item->is_boundary()) boundaryCount--;
        if (item->is_mesh()) meshCount--;
        DeleteItem(item);
        delete item;
    }

    bool isValidDelete ()
    {
        long unsigned int i=0;
        while (i < selectedItems.size()) {
            if (!viewerContext->IsDisplayed(selectedItems[i]->get_AIS_Shape())) return false;
            i++;
        }
        return true;
    }

    // map

    void insertItemToMap (Handle(AIS_Shape) shape, CustomTreeWidgetItem *item)
    {
        if (item->is_drawing()) drawingCount++;
        if (item->is_port()) portCount++;
        if (item->is_boundary()) boundaryCount++;
        if (item->is_mesh()) meshCount++;
        shapeToItemMap.insert({shape,item});
    }

    // reset

    void reset ()
    {
        visibleItems.clear();
        selectedItems.clear();
        shapeToItemMap.clear();
    }

    long unsigned int get_drawingCount ()
    {
        return drawingCount;
    }

private:

    void EraseShape (Handle(AIS_Shape) shape) {
        if (!viewerContext->IsDisplayed(shape)) return;
        viewerContext->Erase(shape,Standard_False);
    }

    void DisplayShape (Handle(AIS_Shape) shape, int displayMode, int selectionMode)
    {
        viewerContext->Display(shape,displayMode,selectionMode,Standard_False);
    }

    void SelectShape (Handle(AIS_Shape) shape)
    {
        if (viewerContext->IsSelected(shape)) return;
        viewerContext->AddOrRemoveSelected(shape,Standard_False);
    }

    void UnselectShape (Handle(AIS_Shape) shape)
    {
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

    Handle(AIS_InteractiveContext) viewerContext;
    std::vector<CustomTreeWidgetItem *> visibleItems;
    std::vector<CustomTreeWidgetItem *> selectedItems;
    std::unordered_map<Handle(AIS_Shape), CustomTreeWidgetItem*> shapeToItemMap;

    long unsigned int drawingCount=0;
    long unsigned int portCount=0;
    long unsigned int boundaryCount=0;
    long unsigned int meshCount=0;
};

#endif // ITEMTRACKING_H
