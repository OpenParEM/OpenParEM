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

#ifndef PATH_H
#define PATH_H

#include "mfem.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <limits>
#include "petscsys.h"
#include "keywordPair.hpp"
#include "misc.hpp"
#include "prefix.h"

using namespace std;
using namespace mfem;

struct point point_copy (struct point);
struct point point_subtraction (struct point, struct point);
struct point point_addition (struct point, struct point);
double point_magnitude (struct point);
struct point point_cross_product (struct point, struct point);
double point_dot_product (struct point, struct point);
struct point point_cross_product (struct point, struct point);
double point_magnitude (struct point);
struct point point_normalize (struct point);
bool point_comparison (struct point, struct point, double);
void point_print (struct point);

class Path {
   private:
      int startLine;
      int endLine;
      keywordPair name;
      vector<keywordPair *> points;
      keywordPair closed;
      double tol=1e-11; // 1e-11
      bool hasNormal;
      struct point normal;
      bool rotated;        // true if this path is rotated parallel to the x-y plane
      double theta;        // angle from z-axis to the normal vector of the plane in which the path sits
      double phi;          // rotation from the x-axis in the x-y plane to the normal vector of the plane
      double sin_theta_;
      double cos_theta_;
      double sin_phi_;
      double cos_phi_;
      double xmax,xmin;
      double ymax,ymin;
      double zmax,zmin;
      bool hasOutput;
   public:
      Path (int, int);
      ~Path ();
      bool load (int, string *, inputFile *);
      bool inBlock (int);
      bool check (string *);
      bool checkBoundingBox (Vector *, Vector *, string *, double);
      string get_name () {return name.get_value();}
      bool name_is_loaded () {return name.is_loaded();}
      int get_name_lineNumber () {return name.get_lineNumber();}
      bool get_closed () {return closed.get_bool_value();}
      int get_closed_lineNumber () {return closed.get_lineNumber();}
      int get_startLine () {return startLine;}
      int get_endLine () {return endLine;}
      long unsigned int get_points_size () {return points.size();}
      keywordPair* get_point (long unsigned int i) {return points[i];}
      void offset (struct point);
      struct point get_point_value (long unsigned int);
      int get_point_dim (long unsigned int i) {return points[i]->get_point_value_dim();}
      void push_point (keywordPair *point) {points.push_back(point);}
      void pop_point () {points.pop_back();}
      bool compare (long unsigned int i, keywordPair test_point);
      void set_closed (bool value) {closed.set_bool_value(value); closed.set_loaded(true);}
      bool is_closed () {return closed.get_bool_value();}
      void set_name (string name_) {name.set_value(name_); name.set_loaded(true);}
      void set_hasOutput () {hasOutput=true;}
      void unset_hasOutput () {hasOutput=false;}
      keywordPair* get_startPoint () {return points[0];}
      keywordPair* get_endPoint () {if (closed.get_bool_value()) return points[0]; return points[points.size()-1];}
      long unsigned int is_segmentOnLine (struct point, struct point);
      bool does_line_intersect (struct point, struct point);
      bool is_path_overlap (Path *);
      void subdivide (Path *);
      double sum_of_angles (struct point);
      bool calculateNormal ();
      struct point get_normal () {return normal;}
      void set_normal (struct point normal_) {normal=point_copy(normal_);}
      bool is_rotated () {return rotated;}
      Path* rotateToXYplane ();
      void rotatePoint (double *, double *, double *, bool);
      void rotatePoint (struct point *);
      void rotatePoint (struct point *, bool);
      void rotateToPath (Path *);
      void rotateToPath (Path *, bool);
      bool is_point_on_path (struct point);
      bool is_point_inside (struct point);
      bool is_point_interior (struct point);
      bool is_path_inside (Path *);
      void test_is_point_inside_m ();
      void test_is_point_inside_mr ();
      void test_is_point_inside_sqr2 ();
      Path* clone ();
      void calculateBoundingBox ();
      void print (string);
      bool output (ofstream *, int);
      bool snapToMeshBoundary (Mesh *);
      double area ();
      void reverseOrder ();
      bool lineIntersects (struct point, struct point);
      struct point getInsidePoint ();
};

bool mergePaths (vector<Path *> *, vector<long unsigned int> *, vector<bool> *, string, string, Path **, double);

#endif

