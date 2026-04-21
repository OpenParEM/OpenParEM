////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM2D - A fullwave 2D electromagnetic simulator.                  //
//    Copyright (C) 2025 Brian Young                                          //
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

#include "path.hpp"

#ifdef HAS_GUI
#include <AIS_Shape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include "CustomOpenGLWidget.h"
#endif

// angle between lines ptc,pt1 and ptc,pt2 (always positive)
double angle_between_two_lines (struct point ptc, struct point pt1, struct point pt2)
{
   if (ptc.dim != pt1.dim || ptc.dim != pt2.dim) return -DBL_MAX;
   pt1=point_normalize(point_subtraction(pt1,ptc));
   pt2=point_normalize(point_subtraction(pt2,ptc));
   return acos(point_dot_product(pt1,pt2));
}

// signed angle between lines ptc,pt1 and ptc,pt2
double signed_angle_between_two_lines (struct point ptc, struct point pt1, struct point pt2, struct point normal)
{
   if (ptc.dim != pt1.dim || ptc.dim != pt2.dim) return -DBL_MAX;
   pt1=point_normalize(point_subtraction(pt1,ptc));
   pt2=point_normalize(point_subtraction(pt2,ptc));
   return atan2(point_dot_product(point_cross_product(pt1,pt2),normal),point_dot_product(pt1,pt2));
}

// check if lines pt1,pt2 and pt3,pt4 are parallel
bool are_parallel (struct point pt1, struct point pt2, struct point pt3, struct point pt4, double tolerance)
{
   if (pt1.dim != pt2.dim || pt1.dim != pt3.dim || pt1.dim != pt4.dim) return false;

   struct point pt0;
   pt0.dim=pt1.dim; pt0.x=0; pt0.y=0; pt0.z=0;

   if (abs(angle_between_two_lines(pt0,point_subtraction(pt2,pt1),point_subtraction(pt4,pt3))) < tolerance) return true;
   return false;
}

// using Heron's formula
double triangle_area_Heron (struct point a, struct point b, struct point c)
{
   if (a.dim != b.dim || a.dim != c.dim) return -DBL_MAX;

   double A,B,C;
   if (a.dim == 2) {
      A=sqrt(pow(a.x-b.x,2)+pow(a.y-b.y,2));
      B=sqrt(pow(b.x-c.x,2)+pow(b.y-c.y,2));
      C=sqrt(pow(c.x-a.x,2)+pow(c.y-a.y,2));
   } else {
      A=sqrt(pow(a.x-b.x,2)+pow(a.y-b.y,2)+pow(a.z-b.z,2));
      B=sqrt(pow(b.x-c.x,2)+pow(b.y-c.y,2)+pow(b.z-c.z,2));
      C=sqrt(pow(c.x-a.x,2)+pow(c.y-a.y,2)+pow(c.z-a.z,2));
   }

   double S=0.5*(A+B+C);
   return sqrt(S*(S-A)*(S-B)*(S-C));
}

double triangle_area (struct point a, struct point b, struct point c)
{
   struct point t1=point_subtraction(b,a);
   struct point t2=point_subtraction(c,a);
   return 0.5*point_magnitude(point_cross_product(t1,t2));
}

double tetrahedra_volume (struct point a, struct point b, struct point c, struct point d)
{
   if (a.dim != b.dim || a.dim != c.dim || a.dim != d.dim) return -DBL_MAX;
   if (a.dim != 3) return -DBL_MAX;

   struct point A,B,C;
   A=point_subtraction(b,a);
   B=point_subtraction(c,a);
   C=point_subtraction(d,a);

   return point_dot_product(point_cross_product(A,B),C)/6.;
}

// true if d is inside the triangle formed by a,b,c
bool inside_triangle (struct point a, struct point b, struct point c, struct point d)
{
   if (a.dim != b.dim || a.dim != c.dim || a.dim != d.dim) return false;
   if (a.dim != 3) return false;

   // d must lie within the plane of a,b,c
   double area=triangle_area(a,b,c);
   double volume=tetrahedra_volume(a,b,c,d);
   if (pow(abs(volume),1/3.) > 1e-12*pow(abs(area),1/2.)) return false;

   // triangles must have the same sign

   double a1=triangle_area(d,a,b);
   double a2=triangle_area(d,b,c);
   double a3=triangle_area(d,c,a);

   if (a1 < 0) {
      if (a2 > 0) return false;
      if (a3 > 0) return false;
   } else {
      if (a2 < 0) return false;
      if (a3 < 0) return false;
   }

   return true;
}
// see if pt0 is inside the box formed by opposite corners pt1 and pt2
bool is_point_inside_box (struct point pt0, struct point pt1, struct point pt2, double tolerance)
{
   if (pt0.dim != pt1.dim || pt0.dim != pt2.dim) return false;

   if (pt1.x < pt2.x) {
      if (pt0.x < pt1.x-tolerance) return false;
      if (pt0.x > pt2.x+tolerance) return false;
   } else {
      if (pt0.x < pt2.x-tolerance) return false;
      if (pt0.x > pt1.x+tolerance) return false;
   }

   if (pt1.y < pt2.y) {
      if (pt0.y < pt1.y-tolerance) return false;
      if (pt0.y > pt2.y+tolerance) return false;
   } else {
      if (pt0.y < pt2.y-tolerance) return false;
      if (pt0.y > pt1.y+tolerance) return false;
   }

   if (pt0.dim == 3) {
      if (pt1.z < pt2.z) {
         if (pt0.z < pt1.z-tolerance) return false;
         if (pt0.z > pt2.z+tolerance) return false;
      } else {
         if (pt0.z < pt2.z-tolerance) return false;
         if (pt0.z > pt1.z+tolerance) return false;
      }
   }

   return true;
}

// see if point pt0 falls on the line given by pt1 to pt2
bool is_point_on_line (struct point pt0, struct point pt1, struct point pt2, double tolerance)
{
   if (pt0.dim != pt1.dim || pt0.dim != pt2.dim) return false;
   if (point_comparison(pt0,pt1,tolerance)) return true;
   if (point_comparison(pt0,pt2,tolerance)) return true;
   if (!is_point_inside_box(pt0,pt1,pt2,tolerance)) return false;
   if (abs(triangle_area(pt0,pt1,pt2)) < tolerance) return true;
   return false;
}

bool is_point_on_line_not_ends (struct point pt0, struct point pt1, struct point pt2, double tolerance)
{
   if (pt0.dim != pt1.dim || pt0.dim != pt2.dim) return false;
   if (point_comparison(pt0,pt1,tolerance)) return false;
   if (point_comparison(pt0,pt2,tolerance)) return false;
   if (is_point_on_line(pt0,pt1,pt2,tolerance)) return true;
   return false;
}

// lines are given as pt1,pt2 and pt3,pt4
// check if two lines intersect, not counting the end points
// overlaid lines do not count as intersecting, either
bool do_intersect (struct point pt1, struct point pt2, struct point pt3, struct point pt4, double tolerance)
{
   if (pt1.dim != pt2.dim || pt1.dim != pt3.dim || pt1.dim != pt4.dim) return false;

   // check for identical lines
   if (point_comparison(pt1,pt3,tolerance) && point_comparison(pt2,pt4,tolerance)) return false;
   if (point_comparison(pt1,pt4,tolerance) && point_comparison(pt2,pt3,tolerance)) return false;

   // all 4 points must be in the same plane (automatic for dim=2)
   if (pt1.dim == 3 && abs(tetrahedra_volume(pt1,pt2,pt3,pt4)) > tolerance) return false;


   // line pt1,pt2

   double area1=triangle_area(pt1,pt2,pt3);
   if (abs(area1) < tolerance) return false;  // overlaps end point

   double area2=triangle_area(pt1,pt2,pt4);
   if (abs(area2) < tolerance) return false;  // overlaps end point

   bool found12=false;
   if (area1 < +tolerance && area2 > -tolerance) found12=true;
   if (area1 > -tolerance && area2 < +tolerance) found12=true;

   // line pt3,pt4

   double area3=triangle_area(pt3,pt4,pt1);
   if (abs(area3) < tolerance) return false;  // overlaps end point

   double area4=triangle_area(pt3,pt4,pt2);
   if (abs(area4) < tolerance) return false;  // overlaps end point

   bool found34=false;
   if (area3 < +tolerance && area4 > -tolerance) found34=true;
   if (area3 > -tolerance && area4 < +tolerance) found34=true;

   if (found12 && found34) return true;

   return false;
}

bool test_is_point_on_line (double xt, double yt, double x1, double y1, double x2, double y2, double tolerance)
{
   struct point p0; p0.dim=2; p0.x=xt; p0.y=yt;
   struct point p1; p1.dim=2; p1.x=x1; p1.y=y1;
   struct point p2; p2.dim=2; p2.x=x2; p2.y=y2;

   return is_point_on_line(p0,p1,p2,tolerance);
}

bool test_is_point_on_line (double xt, double yt, double zt, double x1, double y1, double z1, double x2, double y2, double z2, double tolerance)
{
   struct point p0; p0.dim=3; p0.x=xt; p0.y=yt; p0.z=zt;
   struct point p1; p1.dim=3; p1.x=x1; p1.y=y1; p1.z=z1;
   struct point p2; p2.dim=3; p2.x=x2; p2.y=y2; p2.z=z2;
   
   return is_point_on_line(p0,p1,p2,tolerance);
}

void test_is_point_on_line ()
{
  int i=1;
  double x1,y1,z1,x2,y2,z2,tolerance;

  tolerance=1e-8/9;

  // 2D

  x1=1; y1=1; x2=10; y2=10;
  if (test_is_point_on_line (2, 2, x1, y1, x2, y2, tolerance))               PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (1-1e-10, 1-1e-9, x1, y1, x2, y2, tolerance))    PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (10+1e-10, 10+1e-9, x1, y1, x2, y2, tolerance))  PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (2, 2+1e-10, x1, y1, x2, y2, tolerance))         PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (2, 2-1e-10, x1, y1, x2, y2, tolerance))         PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, -2, x1, y1, x2, y2, tolerance))            PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (11, 11, x1, y1, x2, y2, tolerance))            PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, 3, x1, y1, x2, y2, tolerance))              PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (3, 2, x1, y1, x2, y2, tolerance))              PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2+1e-7, 2, x1, y1, x2, y2, tolerance))         PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2-1e-7, 2, x1, y1, x2, y2, tolerance))         PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

  x1=1; y1=-1; x2=10; y2=-10;
  if (test_is_point_on_line (2, -2, x1, y1, x2, y2, tolerance))              PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (1-1e-10, -1-1e-9, x1, y1, x2, y2, tolerance))   PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (10+1e-10, -10+1e-9, x1, y1, x2, y2, tolerance)) PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (2, -2+1e-10, x1, y1, x2, y2, tolerance))        PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (2, -2-1e-10, x1, y1, x2, y2, tolerance))        PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, 2, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (11, -11, x1, y1, x2, y2, tolerance))           PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, -3, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (3, -2, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2+1e-7, -2, x1, y1, x2, y2, tolerance))        PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2-1e-7, -2, x1, y1, x2, y2, tolerance))        PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

  x1=-1; y1=1; x2=-10; y2=10;
  if (test_is_point_on_line (-2, 2, x1, y1, x2, y2, tolerance))              PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-1-1e-10, 1-1e-9, x1, y1, x2, y2, tolerance))   PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-10+1e-10, 10+1e-9, x1, y1, x2, y2, tolerance)) PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-2, 2+1e-10, x1, y1, x2, y2, tolerance))        PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-2, 2-1e-10, x1, y1, x2, y2, tolerance))        PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, -2, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-11, 11, x1, y1, x2, y2, tolerance))           PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, 3, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-3, 2, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2+1e-7, 2, x1, y1, x2, y2, tolerance))        PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2-1e-7, 2, x1, y1, x2, y2, tolerance))        PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

  x1=-1; y1=-1; x2=-10; y2=-10;
  if (test_is_point_on_line (-2, -2, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-1-1e-10, -1-1e-9, x1, y1, x2, y2, tolerance))  PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-10+1e-10, -10+1e-9, x1, y1, x2, y2, tolerance))PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-2, -2+1e-10, x1, y1, x2, y2, tolerance))       PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-2, -2-1e-10, x1, y1, x2, y2, tolerance))       PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, 2, x1, y1, x2, y2, tolerance))              PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-11, -11, x1, y1, x2, y2, tolerance))          PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, -3, x1, y1, x2, y2, tolerance))            PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-3, -2, x1, y1, x2, y2, tolerance))            PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2+1e-7, -2, x1, y1, x2, y2, tolerance))       PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2-1e-7, -2, x1, y1, x2, y2, tolerance))       PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

  x1=-1; y1=0; x2=-10; y2=0;
  if (test_is_point_on_line (-2, 0, x1, y1, x2, y2, tolerance))              PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-1+1e-10, 0, x1, y1, x2, y2, tolerance))        PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-10-1e-10, 0, x1, y1, x2, y2, tolerance))       PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-2, 1e-10, x1, y1, x2, y2, tolerance))          PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-2, -1e-10, x1, y1, x2, y2, tolerance))         PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, 2, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-11, -11, x1, y1, x2, y2, tolerance))          PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, -3, x1, y1, x2, y2, tolerance))            PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, 3, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, 1e-7, x1, y1, x2, y2, tolerance))          PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, -1e7, x1, y1, x2, y2, tolerance))          PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

  x1=1; y1=0; x2=10; y2=0;
  if (test_is_point_on_line (2, 0, x1, y1, x2, y2, tolerance))               PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (1-1e-10, 0, x1, y1, x2, y2, tolerance))         PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (10+1e-10, 0, x1, y1, x2, y2, tolerance))        PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (2, 1e-10, x1, y1, x2, y2, tolerance))           PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (2, -1e-10, x1, y1, x2, y2, tolerance))          PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, 2, x1, y1, x2, y2, tolerance))              PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (11, 0, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, -3, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, 3, x1, y1, x2, y2, tolerance))              PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, 1e-7, x1, y1, x2, y2, tolerance))           PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, -1e7, x1, y1, x2, y2, tolerance))           PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

  x1=0; y1=1; x2=0; y2=10;
  if (test_is_point_on_line (0, 2, x1, y1, x2, y2, tolerance))               PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (0, 1-1e-10, x1, y1, x2, y2, tolerance))         PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (0, 10+1e-10, x1, y1, x2, y2, tolerance))        PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (1e-10, 2, x1, y1, x2, y2, tolerance))           PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-1e-10, 2, x1, y1, x2, y2, tolerance))          PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, 2, x1, y1, x2, y2, tolerance))              PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (0, 11, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, 2, x1, y1, x2, y2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (3, 2, x1, y1, x2, y2, tolerance))              PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (1e-7, 2, x1, y1, x2, y2, tolerance))           PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-1e7, 2, x1, y1, x2, y2, tolerance))           PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

  // 3D

  x1=1; y1=1; z1=1; x2=10; y2=10; z2=10; 
  if (test_is_point_on_line (2, 2, 2, x1, y1, z1, x2, y2, z2, tolerance))                       PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (1-1e-10, 1-1e-9, 1-1e-10, x1, y1, z1, x2, y2, z2, tolerance))      PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (10+1e-10, 10+1e-9, 10+1e-9, x1, y1, z1, x2, y2, z2, tolerance))    PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (2, 2+1e-10, 2, x1, y1, z1, x2, y2, z2, tolerance))                 PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (2, 2-1e-10, 2, x1, y1, z1, x2, y2, z2, tolerance))                 PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, -2, -2, x1, y1, z1, x2, y2, z2, tolerance))                   PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (11, 11, 11, x1, y1, z1, x2, y2, z2, tolerance))                   PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, 3, 2, x1, y1, z1, x2, y2, z2, tolerance))                      PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (3, 2, 2, x1, y1, z1, x2, y2, z2, tolerance))                      PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2+1e-7, 2, 2, x1, y1, z1, x2, y2, z2, tolerance))                 PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2-1e-7, 2, 2, x1, y1, z1, x2, y2, z2, tolerance))                 PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

  x1=1; y1=-1; z1=1; x2=10; y2=-10; z1=1;
  if (test_is_point_on_line (2, -2, 2, x1, y1, z1, x2, y2, z2, tolerance))                      PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (1-1e-10, -1-1e-9, 1-1e-10, x1, y1, z1, x2, y2, z2, tolerance))     PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (10+1e-10, -10+1e-9, 10+1e-10, x1, y1, z1, x2, y2, z2, tolerance))  PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (2, -2+1e-10, 2, x1, y1, z1, x2, y2, z2, tolerance))                PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (2, -2-1e-10, 2, x1, y1, z1, x2, y2, z2, tolerance))                PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, 2, -2, x1, y1, z1, x2, y2, z2, tolerance))                    PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (11, -11, 11, x1, y1, z1, x2, y2, z1, tolerance))                  PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, -3, 2, x1, y1, z1, x2, y2, z2, tolerance))                     PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (3, -2, 3, x1, y1, z1, x2, y2, z2, tolerance))                     PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2+1e-7, -2, 2+1e-7, x1, y1, z1, x2, y2, z2, tolerance))           PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2-1e-7, -2, 2-1e-7, x1, y1, z1, x2, y2, z2, tolerance))           PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

  x1=-1; y1=1; z1=-1; x2=-10; y2=10; z2=-10;
  if (test_is_point_on_line (-2, 2, -2, x1, y1, z1, x2, y2, z2, tolerance))                     PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-1-1e-10, 1-1e-9, -1-1e-10, x1, y1, z1, x2, y2, z2, tolerance))    PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-10+1e-10, 10+1e-9, -10+1e-10, x1, y1, z1, x2, y2, z2, tolerance)) PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-2, 2+1e-10, -2, x1, y1, z1, x2, y2, z2, tolerance))               PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-2, 2-1e-10, -2, x1, y1, z1, x2, y2, z2, tolerance))               PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, -2, 2, x1, y1, z1, x2, y2, z2, tolerance))                     PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-11, 11, -11, x1, y1, z1, x2, y2, z2, tolerance))                 PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, 3, -2, x1, y1, z1, x2, y2, z2, tolerance))                    PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-3, 2, -3, x1, y1, z1, x2, y2, z2, tolerance))                    PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2+1e-7, 2, -2+1e-7, x1, y1, z1, x2, y2, z2, tolerance))          PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2-1e-7, 2, -2-1e-7, x1, y1, z1, x2, y2, z2, tolerance))          PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;


  x1=-1; y1=-1; z1=-1; x2=-10; y2=-10; z2=-10;
  if (test_is_point_on_line (-2, -2, -2, x1, y1, z1, x2, y2, z2, tolerance))                    PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-1-1e-10, -1-1e-9, -1-1e-10, x1, y1, z1, x2, y2, z2, tolerance))   PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-10+1e-10, -10+1e-9, -10+1e-10, x1, y1, z1, x2, y2, z2, tolerance))PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-2, -2+1e-10, -2, x1, y1, z1, x2, y2, z2, tolerance))              PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-2, -2-1e-10, -2, x1, y1, z1, x2, y2, z2, tolerance))              PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, 2, 2, x1, y1, z1, x2, y2, z2, tolerance))                      PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-11, -11, -11, x1, y1, z1, x2, y2, z2, tolerance))                PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, -3, -2, x1, y1, z1, x2, y2, z2, tolerance))                   PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-3, -2, -3, x1, y1, z1, x2, y2, z2, tolerance))                   PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2+1e-7, -2, -2+1e-7, x1, y1, z1, x2, y2, z2, tolerance))         PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2-1e-7, -2, -2-1e-7, x1, y1, z1, x2, y2, z2, tolerance))         PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

  x1=-1; y1=0; z1=-1; x2=-10; y2=0; z2=-10;
  if (test_is_point_on_line (-2, 0, -2, x1, y1, z1, x2, y2, z2, tolerance))                     PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-1+1e-10, 0, -1+1e-10, x1, y1, z1, x2, y2, z2, tolerance))         PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-10-1e-10, 0, -10-1e-10, x1, y1, z1, x2, y2, z2, tolerance))       PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-2, 1e-10, -2, x1, y1, z1, x2, y2, z2, tolerance))                 PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-2, -1e-10, -2, x1, y1, z1, x2, y2, z2, tolerance))                PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, 2, -2, x1, y1, z1, x2, y2, z2, tolerance))                    PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-11, -11, -11, x1, y1, z1, x2, y2, z2, tolerance))                PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, -3, -2, x1, y1, z1, x2, y2, z2, tolerance))                   PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, 3, -2, x1, y1, z1, x2, y2, z2, tolerance))                    PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, 1e-7, -2, x1, y1, z1, x2, y2, z2, tolerance))                 PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, -1e7, -2, x1, y1, z1, x2, y2, z2, tolerance))                 PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

  x1=1; y1=0; z1=1; x2=10; y2=0; z2=10;
  if (test_is_point_on_line (2, 0, 2, x1, y1, z1, x2, y2, z2, tolerance))                       PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (1-1e-10, 0, 1-1e-10, x1, y1, z1, x2, y2, z2, tolerance))           PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (10+1e-10, 0, 10+1e-10, x1, y1, z1, x2, y2, z2, tolerance))         PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (2, 1e-10, 2, x1, y1, z1, x2, y2, z2, tolerance))                   PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (2, -1e-10, 2, x1, y1, z1, x2, y2, z2, tolerance))                  PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, 2, 2, x1, y1, z1, x2, y2, z2, tolerance))                      PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (11, 0, 11, x1, y1, z1, x2, y2, z2, tolerance))                    PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, -3, 2, x1, y1, z1, x2, y2, z2, tolerance))                     PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, 3, 2, x1, y1, z1, x2, y2, z2, tolerance))                      PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, 1e-7, 2, x1, y1, z1, x2, y2, z2, tolerance))                   PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, -1e7, 2, x1, y1, z1, x2, y2, z2, tolerance))                   PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

  x1=0; y1=1; z1=0; x2=0; y2=10; z2=0;
  if (test_is_point_on_line (0, 2, 0, x1, y1, z1, x2, y2, z2, tolerance))                       PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (0, 1-1e-10, 0, x1, y1, z1, x2, y2, z2, tolerance))                 PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (0, 10+1e-10, 0, x1, y1, z1, x2, y2, z2, tolerance))                PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (1e-10, 2, 1e-10, x1, y1, z1, x2, y2, z2, tolerance))               PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (test_is_point_on_line (-1e-10, 2, -1e-10, x1, y1, z1, x2, y2, z2, tolerance))             PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (2, 2, 2, x1, y1, z1, x2, y2, z2, tolerance))                      PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (0, 11, 0, x1, y1, z1, x2, y2, z2, tolerance))                     PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-2, 2, -2, x1, y1, z1, x2, y2, z2, tolerance))                    PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (3, 2, 3, x1, y1, z1, x2, y2, z2, tolerance))                      PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (1e-7, 2, 1e-7, x1, y1, z1, x2, y2, z2, tolerance))                PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;
  if (!test_is_point_on_line (-1e7, 2, -1e-7, x1, y1, z1, x2, y2, z2, tolerance))               PetscPrintf(PETSC_COMM_WORLD,"%d pass\n",i); else PetscPrintf(PETSC_COMM_WORLD,"%d FAIL\n",i); i++;

}

bool mergePaths (std::vector<Path *> *pathList, std::vector<long unsigned int> *pathIndexList, std::vector<bool> *reverseList, std::string boundaryType, std::string boundaryName, Path **mergedPath, double tol)
{
   bool fail=false;
   long unsigned int i;

   // nothing to work on
   if (pathIndexList->size() == 0) return fail;

   // no merging needed
   if (pathIndexList->size() == 1) {
      *mergedPath=(*pathList)[(*pathIndexList)[0]];
      return fail;
   }

   // merge required

   // none of the paths can be closed: ambiguous as to user's intent
   i=0;
   while (i < pathIndexList->size()) {
      Path *path=(*pathList)[(*pathIndexList)[i]];
      if (path->is_closed()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1095: Path must be open with \"closed\" set to \"false\" at line number %d.\n",path->get_closed_lineNumber());
         fail=true;
      }
      i++;
   }
   if (fail) return fail;

   // set up a new path
   *mergedPath=new Path(-1,-1);

   // merge

   i=0;
   while (i < pathIndexList->size()) {

      Path *path=(*pathList)[(*pathIndexList)[i]];

      if ((*reverseList)[i]) {
         long unsigned int j=path->get_points_size()-1;
         while (j >= 0) {
            // always add the point to a new path
            if ((*mergedPath)->get_points_size() == 0) (*mergedPath)->push_point(path->get_point(j)->clone());           
            else {
                // add if the point does not duplicate the last one
                if (! point_comparison(path->get_point(j)->get_point_value(),(*mergedPath)->get_point((*mergedPath)->get_points_size()-1)->get_point_value(),tol)) {
                    (*mergedPath)->push_point(path->get_point(j)->clone());
                }
            }
            if (j == 0) break;
            j--;
         }
      } else {
         long unsigned int j=0;
         while (j < path->get_points_size()) {
            // always add the point to a new path
            if ((*mergedPath)->get_points_size() == 0) (*mergedPath)->push_point(path->get_point(j)->clone());
            else {
                // add if the point does not duplicate the last one
                if (! point_comparison(path->get_point(j)->get_point_value(),(*mergedPath)->get_point((*mergedPath)->get_points_size()-1)->get_point_value(),tol)) {
                    (*mergedPath)->push_point(path->get_point(j)->clone());
                }
            }
            j++;
         }
      }

      i++;
   }
   // check for closed polygon
   if (point_comparison((*mergedPath)->get_point(0)->get_point_value(),(*mergedPath)->get_point((*mergedPath)->get_points_size()-1)->get_point_value(),tol)) {
      (*mergedPath)->pop_point();
   }

   (*mergedPath)->set_closed(true);

   // check for duplicate points along the path
   i=0;
   while (i < (*mergedPath)->get_points_size()-1) {
      long unsigned int j=i+1;
      while (j < (*mergedPath)->get_points_size()) {
         if (point_comparison((*mergedPath)->get_point(i)->get_point_value(),(*mergedPath)->get_point(j)->get_point_value(),tol)) {
            struct point p=(*mergedPath)->get_point(i)->get_point_value();
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1096: Merged path for %s %s has duplicate point at (%g,%g,%g)\n",
               boundaryType.c_str(),boundaryName.c_str(),p.x,p.y,p.z);
            fail=true;
         }
         j++;
      }
      i++;
   }

   // ToDo - Add a check for crossing lines

   if (fail) {
      delete *mergedPath;
      *mergedPath=nullptr;
      return fail;
   }

   (*mergedPath)->calculateBoundingBox();
   (*mergedPath)->calculateNormal();

   return fail;
}


///////////////////////////////////////////////////////////////////////////////////////////
// Path
///////////////////////////////////////////////////////////////////////////////////////////

Path::Path(int startLine_, int endLine_)
{
   startLine=startLine_;
   endLine=endLine_;

   // name
   name.push_alias("name");
   name.set_loaded(false);
   name.set_positive_required(false);
   name.set_non_negative_required(false);
   name.set_lowerLimit(0);
   name.set_upperLimit(0);
   name.set_checkLimits(false);

   // closed
   closed.push_alias("closed");
   closed.set_loaded(false);
   closed.set_positive_required(false);
   closed.set_non_negative_required(false);
   closed.set_lowerLimit(0);
   closed.set_upperLimit(0);
   closed.set_checkLimits(false);

   rotated=false;

   theta=0;
   cos_theta_=cos(theta);
   sin_theta_=sin(theta);

   phi=0;
   cos_phi_=cos(phi);
   sin_phi_=sin(phi);

   hasNormal=false;
   normal.dim=3; normal.x=-2; normal.y=-2; normal.z=-2;

   hasOutput=false;
   modified=true;

#if HAS_GUI
   item=nullptr;
   portItem=nullptr;
#endif
}

struct point Path::get_point_value (long unsigned int i)
{
   struct point error;
   error.dim=3; error.x=-DBL_MAX; error.y=-DBL_MAX; error.z=-DBL_MAX;

   if (i < points.size()) return points[i]->get_point_value();
   if (points.size() > 0 && is_closed() && i == points.size()) return points[0]->get_point_value();

   return error;
}

void Path::offset (struct point delp)
{
   long unsigned int i=0;
   while (i < points.size()) {
      struct point p=points[i]->get_point_value();
      p=point_addition(p,delp);
      points[i]->set_point_value(p);
      i++;
   }
}

// return true if the point is close
bool Path::compare (long unsigned int i, keywordPair test_point)
{
   struct point a=points[i]->get_point_value();
   struct point b=test_point.get_point_value();
   return point_comparison(a,b,tol);
}

void Path::print (std::string indent)
{
   int dim=0;
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sPath %p\n",indent.c_str(),this);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   name=%s\n",indent.c_str(),get_name().c_str());
   long unsigned int i=0;
   while (i < points.size()) {
      struct point p=points[i]->get_point_value();
      if (p.dim == 2) {dim=2; prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   point=(%g,%g)\n",indent.c_str(),p.x,p.y);}
      if (get_point_dim(i) == 3) {dim=3; prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   point=(%g,%g,%g)\n",indent.c_str(),p.x,p.y,p.z);}
      i++;
   }
   if (closed.get_bool_value()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   closed=true\n",indent.c_str());}
   else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   closed=false\n",indent.c_str());}
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   tol=%g\n",indent.c_str(),tol);
   if (rotated) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   rotated=true\n",indent.c_str());}
   else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   rotated=false\n",indent.c_str());}
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   theta=%g\n",indent.c_str(),theta);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   phi=%g\n",indent.c_str(),phi);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   sin_theta_=%g\n",indent.c_str(),sin_theta_);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   cos_theta_=%g\n",indent.c_str(),cos_theta_);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   sin_phi_=%g\n",indent.c_str(),sin_phi_);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   cos_phi_=%g\n",indent.c_str(),cos_phi_);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   xmax=%g\n",indent.c_str(),xmax);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   xmin=%g\n",indent.c_str(),xmin);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   ymax=%g\n",indent.c_str(),ymax);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   ymin=%g\n",indent.c_str(),ymin);
   if (dim == 3) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   zmax=%g\n",indent.c_str(),zmax);}
   if (dim == 3) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   zmin=%g\n",indent.c_str(),zmin);}
   if (hasNormal) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   hasNormal=true\n",indent.c_str());}
   else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   hasNormal=false\n",indent.c_str());}
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   normal.x=%g\n",indent.c_str(),normal.x);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   normal.y=%g\n",indent.c_str(),normal.y);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   normal.z=%g\n",indent.c_str(),normal.z);
   if (hasOutput) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   hasOutput=true\n",indent.c_str());}
   else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   hasOutput=false\n",indent.c_str());}
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sEndPath\n",indent.c_str());
}

void Path::save (std::ofstream *out)
{
    PetscMPIInt rank;
    MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

    modified=false;
    if (rank != 0) return;

    int dim=0;
    *out << "Path" << std::endl;
    *out << "   name=" << get_name() << std::endl;
    *out << std::setprecision(15);
    long unsigned int i=0;
    while (i < points.size()) {
        struct point p=points[i]->get_point_value();
        if (p.dim == 2) {*out << "   point=(" << p.x << "," << p.y << ")" << std::endl;}
        if (get_point_dim(i) == 3) {*out << "   point=(" << p.x << "," << p.y << "," << p.z << ")" << std::endl;}
        i++;
    }
    if (closed.get_bool_value()) {*out << "   closed=true" << std::endl;}
    else {*out << "   closed=false" << std::endl;}

    *out << "EndPath" << std::endl;
    *out << std::endl;
}

bool Path::output (std::ofstream *out, int force_dim)
{
   if (hasOutput) return false;

   *out << "Path" << std::endl;
   *out << "   name=" << get_name() << std::endl;
   long unsigned int i=0;
   while (i < points.size()) {
      struct point p=points[i]->get_point_value();
      if (force_dim == 2) {*out << std::setprecision(16) << "   point=(" << p.x << "," << p.y << ")" << std::endl;}
      if (force_dim == 3) {*out << std::setprecision(16) << "   point=(" << p.x << "," << p.y << "," << p.z << ")" << std::endl;}
      i++;
   }
   if (closed.get_bool_value()) *out << "   closed=true" << std::endl;
   else *out << "   closed=false" << std::endl;

   *out << "EndPath" << std::endl;

   hasOutput=true;

   return true;
}

bool Path::load(int dim, std::string *indent, inputFile *inputs)
{
   bool fail=false;

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {

      std::string token,value,line;
      line=inputs->get_line(lineNumber);
      get_token_pair(&line,&token,&value,&lineNumber,*indent);

      int recognized=0;

      if (name.match_alias(&token)) {
         if (name.is_loaded()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1097: Duplicate entry at line %d for previous entry at line %d.\n",
                                                  indent->c_str(),indent->c_str(),lineNumber,name.get_lineNumber());
            fail=true;
         } else {
            name.set_keyword(token);
            name.set_value(value);
            name.set_lineNumber(lineNumber);
            name.set_loaded(true);
         }
         recognized++;
      }

      if (token.compare("point") == 0) {
         keywordPair *point=new keywordPair;
         point->push_alias("point");
         point->set_keyword(token);
         point->set_value(value);
         point->set_lineNumber(lineNumber);
         point->set_positive_required(false);
         point->set_non_negative_required(false);
         point->set_lowerLimit(-100);
         point->set_upperLimit(100);
         point->set_checkLimits(true);
         point->set_loaded(false);
         point->set_point_value(-DBL_MAX,-DBL_MAX,-DBL_MAX);

         if (point->loadPoint(dim,&token,&value,lineNumber)) delete point;
         else points.push_back(point);

         recognized++;
      }

      if (closed.match_alias(&token)) {
         recognized++;
         if (closed.loadBool(&token, &value, lineNumber)) fail=true;
      }

      // should recognize one keyword
      if (recognized != 1) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1098: Unrecognized keyword at line %d.\n",indent->c_str(),indent->c_str(),lineNumber);
         fail=true;
      }
      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }

   calculateBoundingBox();
   calculateNormal();

   return fail;
}

bool Path::checkBoundingBox(mfem::Vector *lowerLeft, mfem::Vector *upperRight, std::string *indent, double tol)
{
   bool fail=false;

   long unsigned int i=0;
   while (i < points.size()) {

      bool point_fail=false;

      struct point p=points[i]->get_point_value();

      if (p.x < lowerLeft->Elem(0)-tol) point_fail=true;
      if (p.x > upperRight->Elem(0)+tol) point_fail=true;

      if (p.y < lowerLeft->Elem(1)-tol) point_fail=true;
      if (p.y > upperRight->Elem(1)+tol) point_fail=true;

      if (p.dim == 3) {
         if (p.z < lowerLeft->Elem(2)-tol) point_fail=true;
         if (p.z > upperRight->Elem(2)+tol) point_fail=true;
      }

      if (point_fail) {
         if (p.dim == 2) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1099: Path block at line %d has point (%g,%g) outside of the mesh bounding box.\n",
                                                   indent->c_str(),indent->c_str(),startLine,p.x,p.y);
         }
         if (p.dim == 3) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1100: Path block at line %d has point (%g,%g,%g) outside of the mesh bounding box.\n",
                                                   indent->c_str(),indent->c_str(),startLine,p.x,p.y,p.z);
         }
         fail=true;
      }

      i++;
   }

   return fail;
}

bool Path::inBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

bool Path::check(std::string *indent)
{
   bool fail=false;

   if (!name.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1101: Path block at line %d must specify a name.\n",indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   if (!closed.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1102: Path block at line %d must specify \"closed\".\n",indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   if (points.size() == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1103: Path block at line %d must specify points.\n",indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   if (points.size() == 1) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1104: Path block at line %d must specify more than one point.\n",indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   if (points.size() == 2 && closed.get_bool_value()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1105: Path block at line %d cannot be closed with just two points.\n",indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   return fail;
}

// returns the segment index for which the line given by pt1,pt2 falls
long unsigned int Path::is_segmentOnLine (struct point pt1, struct point pt2)
{
   long unsigned int i=0;
   while (points.size() > 0 && i < points.size()-1) {
      if (is_point_on_line(pt1,points[i]->get_point_value(),points[i+1]->get_point_value(),1e-8) &&
          is_point_on_line(pt2,points[i]->get_point_value(),points[i+1]->get_point_value(),1e-8)) return i;
      i++;
   }

   if (is_closed()) {
      if (is_point_on_line(pt1,points[points.size()-1]->get_point_value(),points[0]->get_point_value(),1e-8) &&
          is_point_on_line(pt2,points[points.size()-1]->get_point_value(),points[0]->get_point_value(),1e-8)) return points.size()-1;
   }

   long unsigned int max=-1;
   return max;
}

double Path::sum_of_angles (struct point pt)
{
   double theta1=0;
   long unsigned int i=0;
   while (i < points.size()-1) {
      theta1+=signed_angle_between_two_lines (pt,points[i]->get_point_value(),points[i+1]->get_point_value(),normal);
      i++;
   }
   if (is_closed()) {
      theta1+=signed_angle_between_two_lines (pt,points[i]->get_point_value(),points[0]->get_point_value(),normal);
   }
   return theta1;
}

// eliminate partial overlaps of paths by subdividing
// crossing paths are ok
void Path::subdivide (Path *test)
{
   bool modified=false;
   std::vector<keywordPair *> newPoints;

   if (points.size() == 0) return;
   if (test->points.size() == 0) return;

   newPoints.push_back(test->points[0]->clone());

   long unsigned int i=0;
   while (i < points.size()-1) {

      long unsigned int j=0;
      while (j < test->points.size()-1) {

         bool break_on_1=false;
         bool break_on_2=false;

         bool parallel=are_parallel (test->points[j]->get_point_value(),test->points[j+1]->get_point_value(),
                                     points[i]->get_point_value(),points[i+1]->get_point_value(),1e-12);

         if (parallel && is_point_on_line_not_ends(test->points[j]->get_point_value(),points[i]->get_point_value(),points[i+1]->get_point_value(),1e-8)) break_on_1=true;
         if (parallel && is_point_on_line_not_ends(test->points[j+1]->get_point_value(),points[i]->get_point_value(),points[i+1]->get_point_value(),1e-8)) break_on_2=true;

         if (break_on_1) {
            if (break_on_2) {
               // segment is fully enclosed

               // maintain ordering along the line
               if (points[i]->get_point_distance(test->points[j]) < points[i]->get_point_distance(test->points[j+1])) {
                  if (! points[points.size()-1]->is_close_point (test->points[j],1e-12)) {
                     newPoints.push_back(test->points[j]->clone());
                  }
                  newPoints.push_back(test->points[j+1]->clone());
               } else {
                  if (! points[points.size()-1]->is_close_point (test->points[j+1],1e-12)) {
                     newPoints.push_back(test->points[j+1]->clone());
                  }
                  newPoints.push_back(test->points[j]->clone());
               }
               modified=true;
            } else {
               // partial overlap - break the segment at test point 1
               newPoints.push_back(test->points[j]->clone());
               modified=true;
            }
         } else {
            if (break_on_2) {
               // partial overlap - break the segment at test point 2
               newPoints.push_back(test->points[j+1]->clone());
               modified=true;
            } else {
               // nothing to do
            }
         }

         j++;
      }

      // finish the segment
      newPoints.push_back(points[i+1]->clone());

      i++;
   }

   if (modified) {
      long unsigned int k=0;
      while (k < points.size()) {
         delete points[k];
         k++;
      }
      points.clear();

      k=0;
      while (k < newPoints.size()) {
         points.push_back(newPoints[k]);
         k++;
      }
   } else {
      long unsigned int k=0;
      while (k < newPoints.size()) {
         delete newPoints[k];
         k++;
      }
   }
}

Path* Path::clone()
{
   Path *newPath=new Path(startLine,endLine);
   newPath->name=name;
   newPath->closed=closed;
   newPath->tol=tol;
   newPath->rotated=rotated;
   newPath->theta=theta;
   newPath->phi=phi;
   newPath->sin_theta_=sin_theta_;
   newPath->cos_theta_=cos_theta_;
   newPath->sin_phi_=sin_phi_;
   newPath->cos_phi_=cos_phi_;
   newPath->xmax=xmax;
   newPath->xmin=xmin;
   newPath->ymax=ymax;
   newPath->ymin=ymin;
   newPath->zmax=zmax;
   newPath->zmin=zmin;
   newPath->hasNormal=hasNormal;
   newPath->normal=point_copy(normal);

   long unsigned int i=0;
   while (i < points.size()) {
      newPath->points.push_back(points[i]->clone());
      i++;
   }

   newPath->hasOutput=hasOutput;

   return newPath;
}

void Path::calculateBoundingBox()
{
   xmax=-DBL_MAX;
   xmin=DBL_MAX;
   ymax=-DBL_MAX;
   ymin=DBL_MAX;
   zmax=-DBL_MAX;
   zmin=DBL_MAX;
   long unsigned int i=0;
   while (i < points.size()) {
      struct point p=points[i]->get_point_value();
      if (p.x > xmax) xmax=p.x;
      if (p.x < xmin) xmin=p.x;
      if (p.y > ymax) ymax=p.y;
      if (p.y < ymin) ymin=p.y;
      if (p.z > zmax) zmax=p.z;
      if (p.z < zmin) zmin=p.z;
      i++;
   }
}

// calculate a normal to the path
// assumes that the path is planar
bool Path::calculateNormal ()
{
   if (points.size() == 0) return true;

   // exit if 0, 1, or 2 points - cannot determine a normal vector
   if (points.size() < 3) {
      //prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Path::calculateNormal passed path with fewer than 3 points.\n");
      return true;
   }

   // find the two consecutive segments with the angle between them closest to 90 degrees
   double theta1=0;
   double smallest_difference=1e30;
   long unsigned int index=0;
   long unsigned int i=1;
   while (i < points.size()-1) {
      theta1=angle_between_two_lines(points[i]->get_point_value(),points[i-1]->get_point_value(),points[i+1]->get_point_value());
      if (abs(abs(theta1)-M_PI/2) < smallest_difference) {smallest_difference=abs(abs(theta1)-M_PI/2); index=i;}
      i++;
   }

   if (is_closed()) {
      i=0;
      theta1=angle_between_two_lines (points[i]->get_point_value(),points[points.size()-1]->get_point_value(),points[i+1]->get_point_value());
      if (abs(abs(theta1)-M_PI/2) < smallest_difference) {smallest_difference=abs(abs(theta1)-M_PI/2); index=i;}

      i=points.size()-1;
      theta1=angle_between_two_lines (points[i]->get_point_value(),points[i-1]->get_point_value(),points[0]->get_point_value());
      if (abs(abs(theta1)-M_PI/2) < smallest_difference) {smallest_difference=abs(abs(theta1)-M_PI/2); index=i;}
   }

   // cannot continue if the points are colinear - cannot determine a normal vector
   if (double_compare(abs(theta1),M_PI,tol)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Path::calculateNormal passed a colinear path.\n");
      return true;
   }

   // find the normal to these two segments

   struct point pt1,ptc,pt2;
   if (index == 0) {
      pt1=points[points.size()-1]->get_point_value();
      ptc=points[0]->get_point_value();
      pt2=points[index+1]->get_point_value();
   } else if (index == points.size()-1) {
      pt1=points[points.size()-2]->get_point_value();
      ptc=points[points.size()-1]->get_point_value();
      pt2=points[0]->get_point_value();
   } else {
      pt1=points[index-1]->get_point_value();
      ptc=points[index]->get_point_value();
      pt2=points[index+1]->get_point_value();
   }

   pt1=point_subtraction(pt1,ptc);
   pt2=point_subtraction(pt2,ptc);

   normal=point_normalize(point_cross_product(pt2,pt1));

   hasNormal=true;

   return false;
}

Path* Path::rotateToXYplane ()
{
   if (!hasNormal) {
      return nullptr;
   }

   // see if already rotated
   if (rotated) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Path::rotateToXYplane passed a rotated path.\n");
      return nullptr;
   }

   // rotated structure
   Path *rotated=this->clone();
   rotated->rotated=true;

   // rotation angles

   // phi
   rotated->phi=-atan2(rotated->normal.y,rotated->normal.x);

   // theta
   struct point pt0; pt0.dim=3; pt0.x=0; pt0.y=0; pt0.z=0;
   struct point ptz; ptz.dim=3; ptz.x=0; ptz.y=0; ptz.z=1;
   rotated->theta=-angle_between_two_lines(pt0,rotated->normal,ptz);

   // pre-calculation
   rotated->cos_theta_=cos(rotated->theta);
   rotated->sin_theta_=sin(rotated->theta);
   rotated->cos_phi_=cos(rotated->phi);
   rotated->sin_phi_=sin(rotated->phi);

   long unsigned int i=0;
   while (i < rotated->points.size()) {
      struct point p=rotated->points[i]->get_point_value();
      rotated->rotatePoint(&p);
      rotated->points[i]->set_point_value(p);
      i++;
   }

   // check that the path is planar - all z components should be close
   i=1;
   while (i < rotated->points.size()) {
      if (! double_compare(rotated->points[i]->get_point_value().z,rotated->points[0]->get_point_value().z,rotated->tol)) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Path::rotateToXYplane passed a 3D path that is not planar.\n");
      }
      i++;
   }

   rotated->calculateBoundingBox();
   rotated->calculateNormal();

   return rotated;
}

// rotate an extra 180 degrees around the y axis
void Path::rotatePoint (double *x, double *y, double *z, bool spin180degrees)
{
   // nothing to do
   if (! rotated) return;

   double scale=1;
   if (spin180degrees) scale=-1;

   // rotate the given test point by the plane's theta and phi angles
   // see Path::rotateToXYplane for notes on the calculation
   double xr=scale*cos_theta_*cos_phi_*(*x)-scale*cos_theta_*sin_phi_*(*y)+scale*sin_theta_*(*z);
   double yr=sin_phi_*(*x)+cos_phi_*(*y);
   double zr=-scale*sin_theta_*cos_phi_*(*x)+scale*sin_theta_*sin_phi_*(*y)+scale*cos_theta_*(*z);


   *x=xr;
   *y=yr;
   *z=zr;
}

//  (px,py,pz) - point to be rotated
//  (px',py',pz') - rotated point
//  rotation matrix for z x rotation matrix for y x point:
//  [px']   [  cos(theta)  0  sin(theta) ] [ cos(phi)  -sin(phi)  0 ] [px]
//  [py'] = [      0       1      0      ] [ sin(phi)   cos(phi)  0 ] [py]
//  [pz']   [ -sin(theta)  0  cos(theta) ] [     0          0     1 ] [pz]
//
//  multiplied out to avoid the hassle of setting up matrix*matrix then matrix*vector multiplications
//  not too bad with all the zeros
void Path::rotatePoint (struct point *pt)
{
   // nothing to do
   if (! rotated) return;

   // rotate the given test point by the plane's phi and theta angles
   // see Path::rotateToXYplane for notes on the calculation
   double xr=cos_theta_*cos_phi_*(pt->x)-cos_theta_*sin_phi_*(pt->y)+sin_theta_*(pt->z);
   double yr=sin_phi_*(pt->x)+cos_phi_*(pt->y);
   double zr=-sin_theta_*cos_phi_*(pt->x)+sin_theta_*sin_phi_*(pt->y)+cos_theta_*(pt->z);

   pt->x=xr;
   pt->y=yr;
   pt->z=zr;
}

// rotate an extra 180 degrees around the y axis
void Path::rotatePoint (struct point *pt, bool spin180degrees)
{
   // nothing to do
   if (! rotated) return;

   double scale=1;
   if (spin180degrees) scale=-1;

   // rotate the given test point by the plane's phi and theta angles
   // see Path::rotateToXYplane for notes on the calculation
   double xr=scale*cos_theta_*cos_phi_*(pt->x)-scale*cos_theta_*sin_phi_*(pt->y)+scale*sin_theta_*(pt->z);
   double yr=sin_phi_*(pt->x)+cos_phi_*(pt->y);
   double zr=-scale*sin_theta_*cos_phi_*(pt->x)+scale*sin_theta_*sin_phi_*(pt->y)+scale*cos_theta_*(pt->z);

   pt->x=xr;
   pt->y=yr;
   pt->z=zr;
}

// rotate to a given path's rotation
void Path::rotateToPath (Path *rotatedPath)
{
   if (!rotatedPath->rotated) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Path::rotateToPath passed a path that is not rotated.\n");
      return;
   }

   if (rotated) return;

   long unsigned int i=0;
   while (i < points.size()) {
      struct point p=get_point_value(i);
      rotatedPath->rotatePoint(&p);
      points[i]->set_point_value(p);
      i++;
   }

   calculateBoundingBox();
   calculateNormal();
   rotated=true;

   return;
}

// rotate to a given path's rotation with spin
void Path::rotateToPath (Path *rotatedPath, bool spin180degrees)
{
   if (!rotatedPath->rotated) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Path::rotateToPath passed a path that is not rotated.\n");
      return;
   }

   if (rotated) return;

   long unsigned int i=0;
   while (i < points.size()) {
      struct point p=points[i]->get_point_value();
      rotatedPath->rotatePoint(&p,spin180degrees);
      points[i]->set_point_value(p);
      i++;
   }

   calculateBoundingBox();
   calculateNormal();
   rotated=true;

   return;
}

// define interior as NOT including the lines themselves
bool Path::is_point_interior (struct point p1)
{
   if (points.size() < 2) return false;

   if (is_point_on_path(p1)) return false;

   if (rotated) rotatePoint(&p1);

   // all tetrahedral volumes must be zero to be in the same plane
   long unsigned int i=0;
   while (i < points.size()-2) {
      struct point p2=points[i]->get_point_value();
      struct point p3=points[i+1]->get_point_value();
      struct point p4=points[i+2]->get_point_value();
      if (abs(tetrahedra_volume(p1,p2,p3,p4)) > tol) return false;

      i++;
   }

   // the point must make a complete 360 turn to be inside
   double theta1=abs(sum_of_angles(p1));
   if (double_compare(theta1,2*M_PI,tol)) return true;

   return false;
}

bool Path::is_point_on_path (struct point p)
{
   if (points.size() == 0) return false;

   if (rotated) rotatePoint(&p);

   long unsigned int i=0;
   while (i < points.size()-1) {
      if (is_point_on_line(p,points[i]->get_point_value(),points[i+1]->get_point_value(),tol)) return true;
      i++;
   }

   if (is_closed()) {
      if (is_point_on_line(p,points[i]->get_point_value(),points[0]->get_point_value(),tol)) return true;
   }

   return false;
}

// define inside as including the lines themselves
bool Path::is_point_inside (struct point p)
{
   if (is_point_on_path(p)) return true;
   if (is_point_interior(p)) return true;
   return false;
}

bool Path::does_line_intersect (struct point pt1, struct point pt2)
{
   long unsigned int i;
   if (points.size() == 0) return false;

   // check each segment
   i=0;
   while (i < points.size()-1) {
      if (do_intersect (points[i]->get_point_value(),points[i+1]->get_point_value(),pt1,pt2,tol)) return true;
      i++;
   }

   if (is_closed()) {
      if (do_intersect (points[i]->get_point_value(),points[0]->get_point_value(),pt1,pt2,tol)) return true;
   }

   return false;
}

bool Path::is_path_overlap (Path *test)
{
   if (test->points.size() == 0) return false;

   long unsigned int i=0;
   while (i < test->points.size()-1) {
      if (does_line_intersect (test->get_point_value(i),test->get_point_value(i+1))) {
         return true;
      }
      i++;
   }

   if (is_closed()) {
      if (does_line_intersect (test->get_point_value(i),test->get_point_value(0))) {
         return true;
      }
   }

   return false;
}

bool Path::is_path_inside (Path *test)
{
   // check for points inside the path
   long unsigned int i=0;
   while (i < test->points.size()) {
      if (! is_point_inside(test->get_point_value(i))) return false;
      i++;
   }

   // check for crossing lines for the case where overlapping paths do not place a point inside another
   if (is_path_overlap(test)) return false;

   return true;
}


// Assumes the path:
// Path
//    name=m
//    point=(0,0,3)
//    point=(1,2,3)
//    point=(2,1,3)
//    point=(3,2,3)
//    point=(4,0,3)
//    point=(4,-1,3)
//    point=(0,-1,3)
//    closed=true
// EndPath
//
// the is value and expected values should be the same for all cases
//
void Path::test_is_point_inside_m ()
{
   double xt,yt,zt;
   std::string expected="";

   if (points.size() < 3) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Path::test_is_point_inside was passed a point or a line.\n");
      return;
   }

   xt=1; yt=0.5; zt=3; expected="inside";
   struct point p; p.dim=3; p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1; yt=1; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1; yt=1.5; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=3; yt=1.5; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2; yt=1.001; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0.5; yt=1.5; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2; yt=2; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0.5; yt=2; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1; yt=2; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=-1; yt=0; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1; yt=0; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2; yt=0.9999; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=3.00001; yt=2; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=3; yt=2.0001; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=3; yt=1.9999; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2.9999; yt=2; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.5; yt=1.5; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.5; yt=1.4999; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.5; yt=1.50001; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2; yt=-1; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2; yt=-1.0001; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=3; yt=0.0001; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=3; yt=-0.0001; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2.5; yt=1.5; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2.4999; yt=1.5; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2.50001; yt=1.5; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=3.75; yt=1; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;
}

// Assumes the path:
// Path
//    name=mr
//    point=(0,3,0)
//    point=(2,3,1)
//    point=(1,3,2)
//    point=(2,3,3)
//    point=(0,3,4)
//    point=(-1,3,4)
//    point=(-1,3,0)
//    closed=true
// EndPath
//
// the is value and expected values should be the same for all cases
//
void Path::test_is_point_inside_mr ()
{
   double xt,yt,zt;
   std::string expected="";

   if (points.size() < 3) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Path::test_is_point_inside was passed a point or a line.\n");
      return;
   }

   xt=0.5; yt=3; zt=1; expected="inside";
   struct point p; p.dim=3; p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1; yt=3; zt=1; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.5; yt=3; zt=1; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.5; yt=3; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.001; yt=3; zt=2; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.5; yt=3; zt=0.5; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2; yt=3; zt=2; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2; yt=3; zt=0.5; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2; yt=3; zt=1; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0; yt=3; zt=-1; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0; yt=3; zt=1; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0.9999; yt=3; zt=2; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2; yt=3; zt=3.00001; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2.0001; yt=3; zt=3; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.9999; yt=3; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=2; yt=3; zt=2.99999; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.5; yt=3; zt=1.5; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.4999; yt=3; zt=1.5; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.50001; yt=3; zt=1.5; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=-1; yt=3; zt=2; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=-1.0001; yt=3; zt=2; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0.0001; yt=3; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=-0.0001; yt=3; zt=3; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.5; yt=3; zt=2.5; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.5; yt=3; zt=2.49999; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1.5; yt=3; zt=2.50001; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=1; yt=3; zt=3.75; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;
}

// Assumes the path:
//Path
//   name=sqr2
//   point=(0.183012701892219,1.36602540378444,1.04903810567666)
//   point=(0.616025403784439,2.23205080756888,0.799038105676658)
//   point=(-0.133974596215561,2.73205080756888,1.23205080756888)
//   point=(-0.566987298107781,1.86602540378444,1.48205080756888)
//   closed=true
//EndPath
//
// the is value and expected values should be the same for all cases
//
void Path::test_is_point_inside_sqr2 ()
{
   double xt,yt,zt;
   std::string expected="";

   if (points.size() < 3) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Path::test_is_point_inside was passed a point or a line.\n");
      return;
   }

   xt=0.0245190528383291; yt=2.04903810567666; zt=1.14054445662277; expected="inside";
   struct point p; p.dim=3; p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0.182695714594112; yt=1.36739142918822; zt=1.04922111837855; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=-0.130804723234483; yt=2.71839055353103; zt=1.23022068054996; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0.399519052838329; yt=1.79903810567666; zt=0.924038105676658; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=-0.350480947161671; yt=2.29903810567666; zt=1.35705080756888; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=-0.191987298107781; yt=1.61602540378444; zt=1.26554445662277; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0.241025403784439; yt=2.48205080756888; zt=1.01554445662277; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=-0.555157171088858; yt=1.86968565782228; zt=1.47522068054995; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0.604195276765517; yt=2.22839055353103; zt=0.80586823269558; expected="inside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0.182579689190327; yt=1.36515937838065; zt=1.04928810567666; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0.620355530803361; yt=2.24071106160672; zt=0.796538105676658; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=-0.425480947161671; yt=2.34903810567666; zt=1.4003520777581; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=0.245355530803361; yt=2.49071106160672; zt=1.01304445662277; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;

   xt=-0.0334936490538903; yt=0.933012701892219; zt=1.17403810567666; expected="outside";
   p.x=xt; p.y=yt; p.z=zt;
   if (is_point_inside (p)) std::cout << "(" << xt << "," << yt << "," << zt << ") is inside, expected=" << expected << std::endl;
   else std::cout << "(" << xt << "," << yt << "," << zt << ") is outside, expected=" << expected << std::endl;
}

// return true if there is a snap
bool Path::snapToPoint (struct point p)
{
    double tol=1e-08;

    // nothing to do
    if (points.size() == 0) return false;

    // check for dimensional alignment
    if (p.dim != points[0]->get_point_value_dim()) {
        prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Path::snapToPoint passed mismatched dimensions.\n");
        return false;
    }

    long unsigned int i=0;
    while (i < points.size()) {
        if (points[i]->is_close_point(p,tol)) {
            points[i]->set_point_value(p);
            return true;
        }
        i++;
    }

    return false;
}

bool Path::snapToMeshBoundary (mfem::Mesh *mesh)
{
   double tol=1e-08;
   bool fail=false;
   mfem::DenseMatrix pointMat(3,3);

   // nothing to do
   if (points.size() == 0) return false;

   // check for dimensional alignment
   if (mesh->Dimension() != points[0]->get_point_value_dim()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Path::snapToMesh passed mismatched dimensions.\n");
      return true;
   }

   long unsigned int i=0;
   while (i < points.size()) {
      bool found_point=false;
      int j=0;
      while (j < mesh->GetNBE()) {
         if (mesh->GetBdrElementType(j) == mfem::Element::TRIANGLE) {
            mesh->GetBdrPointMatrix(j,pointMat);
            struct point p1; p1.dim=3; p1.x=pointMat.Elem(0,0); p1.y=pointMat.Elem(1,0); p1.z=pointMat.Elem(2,0);
            struct point p2; p2.dim=3; p2.x=pointMat.Elem(0,1); p2.y=pointMat.Elem(1,1); p2.z=pointMat.Elem(2,1);
            struct point p3; p3.dim=3; p3.x=pointMat.Elem(0,2); p3.y=pointMat.Elem(1,2); p3.z=pointMat.Elem(2,2);

            if (points[i]->is_close_point(p1,tol)) {
               points[i]->set_point_value(p1);
               found_point=true;
               break;
            } else if (points[i]->is_close_point(p2,tol)) {
               points[i]->set_point_value(p2);
               found_point=true;
               break;
            } else if (points[i]->is_close_point(p3,tol)) {
               points[i]->set_point_value(p3);
               found_point=true;
               break;
            } 
         }
         j++;
      }
      if (!found_point) {
         fail=true;
      }
      i++;
   }

   return fail;
}

double Path::area ()
{
   Path *path=nullptr;
   bool allocatedPath=false;
   double area=0;

   if (points.size() == 0) return 0;
   if (points.size() == 2) return 0;

   int dim=points[0]->get_point_value().dim;
   if (dim == 2) path=this;
   else {
      if (rotated) path=this;
      else {
         path=rotateToXYplane();
         allocatedPath=true;
      }
   }

   // 2D from now on

   long unsigned int j=0;
   while (j < path->points.size()-1) {
      struct point p1=path->points[j]->get_point_value();
      struct point p2=path->points[j+1]->get_point_value();
      area+=point_cross_product(p1,p2).z;
      j++;
   }

   if (get_closed()) {
      struct point p1=path->points[points.size()-1]->get_point_value();
      struct point p2=path->points[0]->get_point_value();
      area+=point_cross_product(p1,p2).z;
   }

   if (allocatedPath) delete path;

   area/=2;

   return area;
}

void Path::reverseOrder ()
{
   std::vector<keywordPair *> reversed_points;

   long unsigned int i=0;
   while (i < points.size()) {
      reversed_points.push_back(points[points.size()-1-i]);
      i++;
   }

   i=0;
   while (i < points.size()) {
      points[i]=reversed_points[i];
      i++;
   }

   // post-operations to align with Path::load
   calculateBoundingBox();
   calculateNormal();   
}

// return true if the line given by point a to point b intersects the path
bool Path::lineIntersects (struct point a, struct point b)
{
   // do not check for paths that are lines
   if (points.size() <= 2) return false;

   double volume_a=0;
   double volume_b=0;

   struct point p1=points[0]->get_point_value();
   long unsigned int i=1;
   while (i < points.size()-1) {
      struct point p2=points[i]->get_point_value();
      struct point p3=points[i+1]->get_point_value();

      volume_a+=tetrahedra_volume(a,p1,p2,p3);
      volume_b+=tetrahedra_volume(b,p1,p2,p3);

      i++;
   }

   if (volume_a > 0 && volume_b > 0) return false;
   if (volume_a < 0 && volume_b < 0) return false;

   bool sign=false;
   i=0;
   while (i < points.size()-1) {
      struct point p1=points[i]->get_point_value();
      struct point p2=points[i+1]->get_point_value();
      double volume=tetrahedra_volume(a,p1,p2,b);

      // co-linear, so must intersect
      if (volume == 0) return true;

      if (i == 0) {
         sign=true;
         if (volume < 0) sign=false;
      } else {
         if (sign) {
            if (volume < 0) return false;
         } else {
            if (volume > 0) return false;
         }
      }

      i++;
   }

   return true;
}

// get a point inside the path that is not on the periphery
struct point Path::getInsidePoint ()
{
   struct point error_point;
   error_point.x=-1;
   error_point.y=-1;
   error_point.z=-1;
   error_point.dim=3;

   double path_area=area();
   double area_tolerance=path_area*1e-6;

   // do not check for paths that are lines
   if (points.size() <= 2) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Path::getInsidePoint passed invalid path.\n");
      return error_point;
   }

   long unsigned int i=0;
   while (i < points.size()-2) {
      struct point p1=points[i]->get_point_value();
      struct point p2=points[i+1]->get_point_value();
      struct point p3=points[i+2]->get_point_value();

      double area=triangle_area(p1,p2,p3);

      if (abs(area) > area_tolerance) {
         struct point p4=point_midpoint(p1,point_midpoint(p2,p3));
         if (inside_triangle(p4,p1,p2,p3)) return p4;
      }

      i++;
   }

   // should not get to here
   return error_point;
}

void Path::assignPathNormal (struct point normal_)
{
    if (hasNormal) return;
    normal=point_copy(normal_);
    hasNormal=true;
}

#ifdef HAS_GUI

void Path::set_normal (gp_Vec a)
{
    normal.x=a.X();
    normal.y=a.Y();
    normal.z=a.Z();
    normal.dim=3;
}

TopoDS_Wire Path::create_TopoDS_Wire ()
{
    BRepBuilderAPI_MakeWire wireBuilder;

    if (points.size() < 2) return TopoDS_Wire(); // null wire

    gp_Pnt v1(points[0]->get_point_value().x,points[0]->get_point_value().y,points[0]->get_point_value().z);
    gp_Pnt v2;
    long unsigned int i=0;
    while (i < points.size()-1) {
        v2.SetCoord(points[i+1]->get_point_value().x,points[i+1]->get_point_value().y,points[i+1]->get_point_value().z);
        wireBuilder.Add(BRepBuilderAPI_MakeEdge(v1,v2));
        v1=v2;
        i++;
    }

    if (is_closed()) {
        gp_Pnt v2(points[0]->get_point_value().x,points[0]->get_point_value().y,points[0]->get_point_value().z);
        wireBuilder.Add(BRepBuilderAPI_MakeEdge(v1,v2));
    }

    if (!wireBuilder.IsDone()) return TopoDS_Wire();

    return wireBuilder.Wire();
}

// create path from face
void Path::addFacePoints (TopoDS_Face face)
{
    TopTools_MapOfShape seen;
    std::vector<TopoDS_Vertex> orderedVertices;

    TopoDS_Wire outerWire=BRepTools::OuterWire(face);
    BRepTools_WireExplorer wireExplorer(outerWire,face);

    while (wireExplorer.More()) {
        TopoDS_Vertex vertex=wireExplorer.CurrentVertex();

        // avoid duplicates from shared edges / seam edges
        if (!seen.Contains(vertex)) {
            seen.Add(vertex);
            orderedVertices.push_back(vertex);
        }

        wireExplorer.Next();
    }

    long unsigned int i=0;
    while (i < orderedVertices.size()) {
        gp_Pnt pnt=BRep_Tool::Pnt(orderedVertices[i]);

        keywordPair *point=new keywordPair();
        point->set_point_value(pnt.X(),pnt.Y(),pnt.Z());

        points.push_back(point);

        i++;
    }

    set_closed(true);
    calculateNormal();
}

// create path from wire
void Path::addWirePoints (TopoDS_Wire wire)
{
    std::vector<TopoDS_Vertex> vertices;
    TopoDS_Vertex lastVertex;

    // get the vertices from the wire
    for (TopExp_Explorer exp(wire, TopAbs_EDGE); exp.More(); exp.Next()) {
        TopoDS_Edge edge=TopoDS::Edge(exp.Current());
        TopoDS_Vertex v1,v2;
        TopExp::Vertices(edge,v1,v2);

        if (vertices.empty()) {
            vertices.push_back(v1);
            vertices.push_back(v2);
            lastVertex=v2;
        } else {
            if (!v1.IsSame(lastVertex)) vertices.push_back(v1);
            vertices.push_back(v2);
            lastVertex=v2;
        }
    }

    // convert to keyword pairs for the path

    set_closed(wire.Closed());
    long unsigned int upperLimit=vertices.size();
    if (is_closed()) upperLimit--;

    long unsigned int i=0;
    while (i < upperLimit) {
        gp_Pnt pnt=BRep_Tool::Pnt(vertices[i]);
        keywordPair *point=new keywordPair();

        point->set_point_value(pnt.X(),pnt.Y(),pnt.Z());
        points.push_back(point);

        i++;
    }
}

// create path from edge
void Path::addEdgePoints (TopoDS_Edge edge)
{
    TopoDS_Vertex v1,v2;
    TopExp::Vertices(edge,v1,v2);

    // v1
    gp_Pnt pnt=BRep_Tool::Pnt(v1);
    keywordPair *point=new keywordPair();
    point->set_point_value(pnt.X(),pnt.Y(),pnt.Z());
    points.push_back(point);

    // v2
    pnt=BRep_Tool::Pnt(v2);
    point=new keywordPair();
    point->set_point_value(pnt.X(),pnt.Y(),pnt.Z());
    points.push_back(point);
}

void Path::create_item (CustomOpenGLWidget *drawingWindow, CustomTreeWidgetItem *parentItem)
{
    item=new CustomTreeWidgetItem(0);
    item->set_itemType(4);
    item->set_OPEMobject(this);
    item->setText(0,QString::fromStdString(get_name()));
    item->setForeground(0,Qt::black);

    // shape
    TopoDS_Wire wire=create_TopoDS_Wire();
    Handle(AIS_Shape) shape=new AIS_Shape(wire);
    ShapeData *newShapeData=new ShapeData(1,nullptr,nullptr,shape);
    item->addShapeData(newShapeData);

    // arrow heads

    // shortest segment
    double shortestLength=DBL_MAX;
    long unsigned int i=0;
    while (i < points.size()) {
        keywordPair *from=points[i];
        keywordPair *to=nullptr;

        if (i < points.size()-1) {
            to=points[i+1];
        } else {
            if (is_closed()) to=points[0];
        }

        if (to) {
            double length=point_magnitude(point_subtraction(from->get_point_value(),to->get_point_value()));
            if (length > 0 && length < shortestLength) shortestLength=length;
        }
        i++;
    }

    // make arrows
    i=0;
    while (i < points.size()) {
        keywordPair *from=points[i];
        keywordPair *to=nullptr;

        if (i < points.size()-1) {
            to=points[i+1];
        } else {
            if (is_closed()) to=points[0];
        }

        if (to) {
            struct point center=point_midpoint(from->get_point_value(),to->get_point_value());
            struct point centerOffset=point_scale(shortestLength/20,point_normalize(point_subtraction(center,from->get_point_value())));
            struct point arrowOffset=point_scale(2,point_cross_product(normal,centerOffset));

            keywordPair *tip=new keywordPair();
            tip->set_point_value(point_addition(center,centerOffset));

            keywordPair *p1=new keywordPair ();
            p1->set_point_value(point_subtraction(point_subtraction(center,centerOffset),arrowOffset));

            keywordPair *p2=new keywordPair ();
            p2->set_point_value(point_addition(point_subtraction(center,centerOffset),arrowOffset));

            Path arrowHead(0,0);
            arrowHead.set_closed(false);
            arrowHead.push_point(p1);
            arrowHead.push_point(tip);
            arrowHead.push_point(p2);

            Handle(AIS_Shape) arrowDrawingShape=new AIS_Shape (arrowHead.create_TopoDS_Wire());
            item->pushArrowHead(arrowDrawingShape);
        }
        i++;
    }

    parentItem->addChild(item);
}

#endif

Path::~Path ()
{
   long unsigned int i=0;
   while (i < points.size()) {
      delete points[i];
      i++;
   }
}
