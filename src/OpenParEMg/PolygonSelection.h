#ifndef POLYGONSELECTION_H
#define POLYGONSELECTION_H

#include <SelectMgr_Filter.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include "path.hpp"


// class MyCustomFilter : public SelectMgr_Filter {
// public:
//     // Return Standard_True if the owner is allowed to be selected
//     virtual Standard_Boolean IsOk(const Handle(SelectMgr_EntityOwner)& theOwner) const override {
//         if (theOwner.IsNull()) return Standard_False;

//         // Example: Filter by interactive object type
//         Handle(AIS_InteractiveObject) anObj = Handle(AIS_InteractiveObject)::DownCast(theOwner->Selectable());
//         if (!anObj.IsNull() && anObj->IsKind(STANDARD_TYPE(AIS_Shape))) {
//             //return Standard_True; // Only allow AIS_Shape objects
//             Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(anObj);
//             if (!aisShape.IsNull()) {
//                 const TopoDS_Shape& shape = aisShape->Shape();
//                 TopAbs_ShapeEnum type = shape.ShapeType();
//                 std::cout << "TopAbs_ShapeEnum=" << type << std::endl; std::cout.flush();
//                 return Standard_True;
//                 // if (type == TopAbs_VERTEX) {
//                 //     std::cout << "TopAbs_VERTEX" << std::endl; std::cout.flush();
//                 //     return Standard_True;
//                 // }
//             }
//         }
//         return Standard_False;
//     }
// };

class VertexFilter : public SelectMgr_Filter
{
public:
    bool set_outline (Path *);
    virtual Standard_Boolean IsOk (const Handle(SelectMgr_EntityOwner)& theOwner) const;

private:
    std::vector<gp_Pnt> outline;
    gp_Pln plane;
    std::vector<gp_Pnt2d> outline2D;
};


#endif // POLYGONSELECTION_H
