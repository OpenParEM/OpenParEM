#include "PolygonSelection.h"
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <gce_MakePln.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <BRep_Tool.hxx>


bool IsPointOnPlane (const gp_Pln& plane, const gp_Pnt& p, Standard_Real tol = Precision::Confusion())
{
    return plane.Distance(p) <= tol;
}

gp_Pnt2d ProjectToPlane2d (const gp_Pln& plane, const gp_Pnt& p)
{
    const gp_Ax3& ax = plane.Position();
    gp_Vec v(ax.Location(), p);

    return gp_Pnt2d(v.Dot(ax.XDirection()),v.Dot(ax.YDirection()));
}

bool IsPointOnSegment2d (const gp_Pnt2d& p, const gp_Pnt2d& a, const gp_Pnt2d& b, Standard_Real tol)
{
    gp_Vec2d ap(a, p);
    gp_Vec2d ab(a, b);

    Standard_Real cross = ap.Crossed(ab);
    if (Abs(cross) > tol) return false;

    Standard_Real dot = ap.Dot(ab);
    if (dot < -tol || dot > ab.SquareMagnitude() + tol) return false;

    return true;
}

bool IsPointInsideOrOnPolygon2d (const gp_Pnt2d& p, const std::vector<gp_Pnt2d>& poly, Standard_Real tol = Precision::Confusion())
{
    const int n = static_cast<int>(poly.size());
    if (n < 3) return false;

    bool inside = false;

    int i = 0;
    int j = n - 1;

    while (i < n)
    {
        const gp_Pnt2d& pi = poly[i];
        const gp_Pnt2d& pj = poly[j];

        // --- Boundary test ---
        if (IsPointOnSegment2d(p, pi, pj, tol))
            return true;

        // --- Ray casting test ---
        const bool intersect =
            ((pi.Y() > p.Y()) != (pj.Y() > p.Y())) &&
            (p.X() < (pj.X() - pi.X()) *
                             (p.Y() - pi.Y()) /
                             (pj.Y() - pi.Y()) + pi.X());

        if (intersect)
            inside = !inside;

        j = i;
        ++i;
    }

    return inside;
}







bool FindNonCollinearTriplet(
    const std::vector<gp_Pnt>& pts,
    gp_Pnt& p1,
    gp_Pnt& p2,
    gp_Pnt& p3,
    Standard_Real tol = Precision::Confusion())
{
    const int n = static_cast<int>(pts.size());
    if (n < 3)
        return false;

    p1 = pts[0];

    int i = 1;
    while (i < n - 1)
    {
        int j = i + 1;
        while (j < n)
        {
            gp_Vec v1(p1, pts[i]);
            gp_Vec v2(p1, pts[j]);

            if (v1.Crossed(v2).Magnitude() > tol)
            {
                p2 = pts[i];
                p3 = pts[j];
                return true;
            }
            ++j;
        }
        ++i;
    }

    return false;
}

bool ComputePlaneFromPolygon (const std::vector<gp_Pnt>& poly, gp_Pln& plane, Standard_Real tol=Precision::Confusion())
{
    gp_Pnt p1, p2, p3;
    if (!FindNonCollinearTriplet(poly, p1, p2, p3, tol)) return true;
    plane=gce_MakePln(p1, p2, p3).Value();
    return false;
}

bool IsPolygonPlanar (const std::vector<gp_Pnt>& poly, const gp_Pln& plane, Standard_Real tol=Precision::Confusion())
{
    const int n = static_cast<int>(poly.size());
    if (n < 3) return true;

    int i = 0;
    while (i < n) {
        if (plane.Distance(poly[i]) > tol) return true;
        i++;
    }

    return false;
}

Standard_Boolean VertexFilter::IsOk (const Handle(SelectMgr_EntityOwner)& theOwner) const
{
    //std::cout << "VertexFilter::IsOk" << std::endl; std::cout.flush();
    Handle(StdSelect_BRepOwner) aBRepOwner = Handle(StdSelect_BRepOwner)::DownCast(theOwner);
    if (aBRepOwner.IsNull() || !aBRepOwner->HasShape()) return Standard_False;
    //std::cout << " aBRepOwner" << std::endl; std::cout.flush();

    const TopoDS_Shape& aShape = aBRepOwner->Shape();
    //std::cout << "   shape type=" << aShape.ShapeType() << std::endl; std::cout.flush();
    if (aShape.ShapeType() == TopAbs_VERTEX) {
        //std::cout << "   found TopAbs_VERTEX" << std::endl; std::cout.flush();

        // get the vertex
        const TopoDS_Vertex vertex = TopoDS::Vertex(aShape);
        const gp_Pnt pnt = BRep_Tool::Pnt(vertex);

        // see if it is on the plane
        if (!IsPointOnPlane(plane,pnt)) return Standard_False;

        // get a 2D point on the plane
        gp_Pnt2d pnt2D=ProjectToPlane2d(plane,pnt);

        // see if the point is in or on the polygon
        if (IsPointInsideOrOnPolygon2d(pnt2D,outline2D)) return Standard_True;
    }

    return Standard_False;
}

bool VertexFilter::set_outline (Path *path_outline) {

    outline.clear();

    // convert from Path to gp_Pnt vector

    long unsigned int i=0;
    while (i < path_outline->get_points_size()) {
        double x=path_outline->get_point(i)->get_point_value().x;
        double y=path_outline->get_point(i)->get_point_value().y;
        double z=path_outline->get_point(i)->get_point_value().z;
        gp_Pnt p(x,y,z);
        outline.push_back(p);
        i++;
    }

    if (path_outline->is_closed()) {
        double x=path_outline->get_point(0)->get_point_value().x;
        double y=path_outline->get_point(0)->get_point_value().y;
        double z=path_outline->get_point(0)->get_point_value().z;
        gp_Pnt p(x,y,z);
        outline.push_back(p);
    }

    // convert to a gp_Pln
    if (ComputePlaneFromPolygon(outline,plane)) return Standard_True;

    // make sure it is planar
    if (IsPolygonPlanar(outline,plane)) return Standard_True;

    // get points in 2D
    outline2D.clear();
    i=0;
    while (i < outline.size()) {
        gp_Pnt2d pnt2D=ProjectToPlane2d (plane,outline[i]);
        outline2D.push_back(pnt2D);
        i++;
    }

    return Standard_False;
}
