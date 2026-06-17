#include "PolygonSelection.h"
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <gce_MakePln.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>


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

// bool FindNonCollinearTriplet (const std::vector<gp_Pnt>& pts, gp_Pnt& p1, gp_Pnt& p2, gp_Pnt& p3, Standard_Real tol = Precision::Confusion())
// {
//     const int n = static_cast<int>(pts.size());
//     if (n < 3)
//         return false;

//     p1 = pts[0];

//     int i = 1;
//     while (i < n - 1)
//     {
//         int j = i + 1;
//         while (j < n)
//         {
//             gp_Vec v1(p1, pts[i]);
//             gp_Vec v2(p1, pts[j]);

//             if (v1.Crossed(v2).Magnitude() > tol)
//             {
//                 p2 = pts[i];
//                 p3 = pts[j];
//                 return true;
//             }
//             ++j;
//         }
//         ++i;
//     }

//     return false;
// }

// bool ComputePlaneFromPolygon (const std::vector<gp_Pnt>& poly, gp_Pln& plane, Standard_Real tol=Precision::Confusion())
// {
//     gp_Pnt p1, p2, p3;
//     if (!FindNonCollinearTriplet(poly, p1, p2, p3, tol)) return true;
//     plane=gce_MakePln(p1, p2, p3).Value();
//     return false;
// }

// bool IsPolygonPlanar (const std::vector<gp_Pnt>& poly, const gp_Pln& plane, Standard_Real tol=Precision::Confusion())
// {
//     const int n = static_cast<int>(poly.size());
//     if (n < 3) return true;

//     int i = 0;
//     while (i < n) {
//         if (plane.Distance(poly[i]) > tol) return true;
//         i++;
//     }

//     return false;
// }

// bool VertexFilter::get_midPoint (TopoDS_Edge& edge, gp_Pnt *midPoint)
// {
//     // first point must be on the plane
//     TopoDS_Vertex v1=TopExp::FirstVertex(edge);
//     const gp_Pnt pnt1=BRep_Tool::Pnt(v1);
//     if (!IsPointOnPlane(plane,pnt1)) return Standard_False;

//     // second point must be on the plane
//     TopoDS_Vertex v2=TopExp::LastVertex(edge);
//     const gp_Pnt pnt2=BRep_Tool::Pnt(v2);
//     if (!IsPointOnPlane(plane,pnt2)) return Standard_False;

//     // mid point
//     midPoint->SetCoord((pnt1.X()+pnt2.X())/2.0,(pnt1.Y()+pnt2.Y())/2.0,(pnt1.Z()+pnt2.Z())/2.0);
//     gp_Pnt2d pntmid2D=ProjectToPlane2d(plane,*midPoint);

//     // point must be on or in the outline
//     if (IsPointInsideOrOnPolygon2d(pntmid2D,outline2D)) return Standard_True;

//     return Standard_False;
// }

Standard_Boolean VertexFilter::IsOk (const Handle(SelectMgr_EntityOwner)& theOwner) const
{
    Handle(StdSelect_BRepOwner) aBRepOwner = Handle(StdSelect_BRepOwner)::DownCast(theOwner);
    if (aBRepOwner.IsNull() || !aBRepOwner->HasShape()) return Standard_False;

    const TopoDS_Shape& aShape=aBRepOwner->Shape();

    // vertices
    if (aShape.ShapeType() == TopAbs_VERTEX) {

        // get the vertex
        const TopoDS_Vertex vertex=TopoDS::Vertex(aShape);
        const gp_Pnt pnt=BRep_Tool::Pnt(vertex);

        // see if it is on the plane
        if (!IsPointOnPlane(plane,pnt)) return Standard_False;

        // get a 2D point on the plane
        gp_Pnt2d pnt2D=ProjectToPlane2d(plane,pnt);

        // see if the point is in or on the polygon
        if (IsPointInsideOrOnPolygon2d(pnt2D,outline2D)) return Standard_True;
    }

    // edges
    if (aShape.ShapeType() == TopAbs_EDGE) {
        TopoDS_Vertex v1,v2;
        TopExp::Vertices(TopoDS::Edge(aShape),v1,v2);

        // first point
        gp_Pnt p1=BRep_Tool::Pnt(v1);
        if (!IsPointOnPlane(plane,p1)) return Standard_False;

        // second point
        gp_Pnt p2=BRep_Tool::Pnt(v2);
        if (!IsPointOnPlane(plane,p2)) return Standard_False;

        // midpoint
        gp_Pnt pntmid((p1.X()+p2.X())/2.0,(p1.Y()+p2.Y())/2.0,(p1.Z()+p2.Z())/2.0);

        // get a 2D point on the plane
        gp_Pnt2d pntmid2D=ProjectToPlane2d(plane,pntmid);

        // see if the point is in or on the polygon
        if (IsPointInsideOrOnPolygon2d(pntmid2D,outline2D)) return Standard_True;
    }

    return Standard_False;
}

// bool VertexFilter::set_outline (Path *path_outline)
// {
//     if (!path_outline) return Standard_True;
//     outline.clear();

//     // convert from Path to gp_Pnt vector

//     long unsigned int i=0;
//     while (i < path_outline->get_points_size()) {
//         double x=path_outline->get_point(i)->get_point_value().x;
//         double y=path_outline->get_point(i)->get_point_value().y;
//         double z=path_outline->get_point(i)->get_point_value().z;
//         gp_Pnt p(x,y,z);
//         outline.push_back(p);
//         i++;
//     }

//     if (path_outline->is_closed()) {
//         double x=path_outline->get_point(0)->get_point_value().x;
//         double y=path_outline->get_point(0)->get_point_value().y;
//         double z=path_outline->get_point(0)->get_point_value().z;
//         gp_Pnt p(x,y,z);
//         outline.push_back(p);
//     }

//     // convert to a gp_Pln
//     if (ComputePlaneFromPolygon(outline,plane)) return Standard_True;

//     // make sure it is planar
//     if (IsPolygonPlanar(outline,plane)) return Standard_True;

//     // get points in 2D
//     outline2D.clear();
//     i=0;
//     while (i < outline.size()) {
//         gp_Pnt2d pnt2D=ProjectToPlane2d (plane,outline[i]);
//         outline2D.push_back(pnt2D);
//         i++;
//     }

//     return Standard_False;
// }
