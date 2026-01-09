#ifndef POLYGONSELECTION_H
#define POLYGONSELECTION_H

#pragma once

#include <AIS_InteractiveObject.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <vector>

class AIS_PlanePolygon : public AIS_InteractiveObject
{
public:
    AIS_PlanePolygon(const gp_Pln& thePlane,
                     const std::vector<gp_Pnt2d>& thePolygon2d);

    DEFINE_STANDARD_RTTIEXT(AIS_PlanePolygon, AIS_InteractiveObject)

    const gp_Pln& Plane() const { return myPlane; }
    const std::vector<gp_Pnt>& Vertices() const { return myVertices; }

protected:
    void Compute(const Handle(PrsMgr_PresentationManager)&,
                 const Handle(Prs3d_Presentation)&,
                 const Standard_Integer) override {}

    void ComputeSelection(const Handle(SelectMgr_Selection)&,
                          const Standard_Integer) override;

private:
    gp_Pln myPlane;
    std::vector<gp_Pnt> myVertices;
};

#endif // POLYGONSELECTION_H
