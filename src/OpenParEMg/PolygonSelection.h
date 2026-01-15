#ifndef POLYGONSELECTION_H
#define POLYGONSELECTION_H

#include <SelectMgr_Filter.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include "path.hpp"

class VertexFilter : public SelectMgr_Filter
{
public:
    DEFINE_STANDARD_RTTI_INLINE(VertexFilter, SelectMgr_Filter)

    bool set_outline (Path *);
    bool get_midPoint (TopoDS_Edge& edge, gp_Pnt *);
    virtual Standard_Boolean IsOk (const Handle(SelectMgr_EntityOwner)& theOwner) const override;

private:
    std::vector<gp_Pnt> outline;
    gp_Pln plane;
    std::vector<gp_Pnt2d> outline2D;
};

#endif // POLYGONSELECTION_H
