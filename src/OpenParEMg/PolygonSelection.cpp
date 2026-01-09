#include "PolygonSelection.h"

#include "AIS_PlanePolygon.hxx"

#include <Select3D_SensitiveSegment.hxx>

IMPLEMENT_STANDARD_RTTIEXT(AIS_PlanePolygon, AIS_InteractiveObject)

static bool IsPointInsidePolygon(
    const gp_Pnt2d& p,
    const std::vector<gp_Pnt2d>& poly)
{
    bool inside = false;
    const int n = (int)poly.size();

    for (int i = 0, j = n - 1; i < n; j = i++)
    {
        const gp_Pnt2d& pi = poly[i];
        const gp_Pnt2d& pj = poly[j];

        if (((pi.Y() > p.Y()) != (pj.Y() > p.Y())) &&
            (p.X() < (pj.X() - pi.X()) * (p.Y() - pi.Y()) /
                             (pj.Y() - pi.Y()) + pi.X()))
        {
            inside = !inside;
        }
    }
    return inside;
}

static gp_Pnt2d ProjectToPlane2d(const gp_Pln& plane, const gp_Pnt& p)
{
    gp_Ax3 ax = plane.Position();
    gp_Vec v(ax.Location(), p);

    return gp_Pnt2d(
        v.Dot(ax.XDirection()),
        v.Dot(ax.YDirection()));
}



AIS_PlanePolygon::AIS_PlanePolygon(
    const gp_Pln& thePlane,
    const std::vector<gp_Pnt2d>& thePolygon2d)
    : myPlane(thePlane)
{
    gp_Ax3 ax = myPlane.Position();

    myVertices.reserve(thePolygon2d.size());

    for (const gp_Pnt2d& p2d : thePolygon2d)
    {
        gp_Pnt p3d =
            ax.Location()
                .Translated(ax.XDirection() * p2d.X())
                .Translated(ax.YDirection() * p2d.Y());

        myVertices.push_back(p3d);
    }
}

void AIS_PlanePolygon::ComputeSelection(
    const Handle(SelectMgr_Selection)& theSel,
    const Standard_Integer)
{
    theSel->Clear();

    const int n = static_cast<int>(myVertices.size());
    if (n < 2)
        return;

    for (int i = 0; i < n; ++i)
    {
        int j = (i + 1) % n;

        theSel->Add(
            new Select3D_SensitiveSegment(
                this,
                myVertices[i],
                myVertices[j]));
    }
}

PolygonSelection::PolygonSelection() {}
