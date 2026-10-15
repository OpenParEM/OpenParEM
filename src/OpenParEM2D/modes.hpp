////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM2D - A fullwave 2D electromagnetic simulator.                  //
//    Copyright (C) 2026 Brian Young                                          //
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

#ifndef MODES_H
#define MODES_H

#include "mfem.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <limits>
#include "keywordPair.hpp"
#include "path.hpp"
#include "misc.hpp"
#include "sourcefile.hpp"
#include "OpenParEMmaterials.hpp"

extern "C" void prefix ();
extern "C" char* get_prefix_text ();
extern "C" void set_prefix_text (char *);

using namespace std;
using namespace mfem;

struct EdgeAttribute {
   int border_element;           // index of one of the border elements that uses this data; for debugging; ToDo: remove when appropriate
   long unsigned int boundary;   // index to a boundary
   long unsigned int path;       // index to a path on the boundary
   long unsigned int segment;    // index to a segment of the path
   double x1,y1,x2,y2;           // redundant information for debugging; ToDo: remove when appropriate
};

class BoundaryDatabase;

// Mesh elements can only have one attribute, so use that one attribute to index to a Border that
// contains information to link to multiple boundaries.  A single mesh edge can be part of one or 
// more modes and part of a boundary condition.
//
// Since Borders are built from the local mesh, the BorderDatabase built from the mesh is local.
// Operations with Borders are global, so the local BorderDatabases must be merged into global
// databases that are shared across all ranks.
class Border
{
   private:
      long unsigned int local_attribute;      // mesh attribute indexing to this border in the local version of the border database
      long unsigned int global_attribute;     // mesh attribute indexing to this border after merging the local databases
      int local_rank;                         // rank for the local database version
      struct EdgeAttribute boundary;          // for a BoundaryDatabase for a boundary condition (Zs, PEC, PMC)
      vector<struct EdgeAttribute> mode;      // for a BoundaryDatabase for a mode, indexed by modeNumber
   public:
      Border();
      bool has_boundary();
      bool has_mode();
      bool has_mode(long unsigned int);
      void set(struct EdgeAttribute);
      void set(struct EdgeAttribute, long unsigned int);
      bool exists(struct EdgeAttribute);
      bool exists(struct EdgeAttribute, long unsigned int);
      bool exists_any_mode(struct EdgeAttribute);
      bool exists_test_boundary_only(struct EdgeAttribute);
      bool mode_exists(long unsigned int);
      struct EdgeAttribute get_boundary() {return boundary;}
      struct EdgeAttribute get_mode(long unsigned int);
      void sendToRank (PetscMPIInt);
      void receiveFromRank (PetscMPIInt);
      void set_local_attribute (long unsigned int local_attribute_) {local_attribute=local_attribute_;}
      void set_global_attribute (long unsigned int global_attribute_) {global_attribute=global_attribute_;}
      long unsigned int get_local_attribute() {return local_attribute;}
      long unsigned int get_global_attribute() {return global_attribute;}
      void set_local_rank (int local_rank_) {local_rank=local_rank_;}
      int get_local_rank () {return local_rank;}
      void print ();
};

class BorderDatabase
{
   private:
      vector<Border *> borderList;
   public:
      BorderDatabase ();
      ~BorderDatabase ();
      long unsigned int exists (struct EdgeAttribute);
      long unsigned int exists (struct EdgeAttribute, long unsigned int);
      long unsigned int exists_any_mode (struct EdgeAttribute);
      long unsigned int exists_test_boundary_only (struct EdgeAttribute);
      long unsigned int add (struct EdgeAttribute, int);
      long unsigned int add (struct EdgeAttribute, long unsigned int, int);
      long unsigned int get_size () {return borderList.size();}
      Border* get_border(long unsigned int i) {return borderList[i];}
      Array<int>* build_entry_list (bool, long unsigned int, int);
      long unsigned int get_last_local_attribute () {return borderList[borderList.size()-1]->get_local_attribute();}
      void reassign_mesh_attributes (ParMesh *);
      void merge ();
      void print ();
};

class Boundary
{
   private:
      int startLine;
      int endLine;
      keywordPair name;                    // for boundaries only
      keywordPair mode;                    // for modes only (numerical, 0-based)
      keywordPair mode_block_type;         // for modes only (modal, line)
      keywordPair type;                    // boundaries: (surface impedance, perfect electric conductor, perfect magnetic conductor); 
                                           // modes/lines: (voltage, current)
      keywordPair material;                // surface impedance boundary only
      keywordPair scale;                   // modes only, default=1; applied to computed voltages or currents
      keywordPair attribute;
      vector<keywordPair *> pathNameList;
      vector<long unsigned int> pathIndexList;
      vector<bool> reverseList;
   public:
      Boundary (int,int,string);
      ~Boundary();
      bool load (string *, inputFile *, bool, int);
      bool inBlock (int);
      bool check (string *);
      bool align (string *, vector<Path *> *, double *, bool);
      bool align_current_paths (string *, vector<Path *> *, bool);
      bool checkBoundingBox (Vector *, Vector *, string *, double, vector<Path *> *);
      int get_startLine () {return startLine;}
      int get_endLine () {return endLine;}
      string get_name () {return name.get_value();}
      int get_mode () {return mode.get_int_value();}
      bool name_is_loaded () {return name.is_loaded();}
      bool mode_is_loaded () {return mode.is_loaded();}
      int get_name_lineNumber () {return name.get_lineNumber();}
      int get_mode_lineNumber () {return mode.get_lineNumber();}
      int get_attribute () {return attribute.get_int_value();}
      string get_type () {return type.get_value();}
      double get_scale () {return scale.get_dbl_value();}
      string get_material () {return material.get_value();}
      long unsigned int get_pathNameList_size () {return pathNameList.size();}
      string get_pathName (long unsigned int i) {return pathNameList[i]->get_value();}
      int get_pathName_lineNumber (long unsigned int i) {return pathNameList[i]->get_lineNumber();}
      bool get_reverse (long unsigned int i) {return reverseList[i];}
      long unsigned int get_path_size () {return pathIndexList.size();}
      long unsigned int get_path (long unsigned int i) {return pathIndexList[i];}
      void push (long unsigned int a) {pathIndexList.push_back(a);}
      struct EdgeAttribute is_segmentOnPath (int, struct point, struct point, vector<Path *> *);
      void set_attribute (int i) {attribute.set_int_value(i);}
      void markMeshBoundaries (Mesh *, ParMesh *, BorderDatabase *, vector<Path *> *);
      bool is_surface_impedance ();
      bool is_perfect_electric_conductor ();
      bool is_perfect_magnetic_conductor ();
      bool is_mode_voltage ();
      bool is_mode_current ();
      bool is_boundary ();
      bool is_mode ();
      bool is_modal ();
      bool is_line ();
      string get_block_type ();
      string get_mode_name ();
      void print ();
};

class BoundaryDatabase
{
   private:
      inputFile inputs;
      vector<SourceFile *> sourceFileList;
      vector<Path *> pathList;
      vector<Boundary *> boundaryList;
      double tol=1e-14;
      string indent="   ";
      string version_name="#OpenParEMmodes";
      string version_value="1.0";
   public:
      ~BoundaryDatabase ();
      bool inBlocks (int);
      bool check ();
      bool check_scale (Mesh *, ParMesh *, int);
      void subdivide_paths ();
      bool findSourceFileBlocks ();
      bool findPathBlocks ();
      bool findBoundaryBlocks ();
      bool findModeBlocks ();
      bool findLineBlocks ();
      bool load(const char *, bool);
      void mark_boundaries (Mesh *, ParMesh *, BorderDatabase *);
      long unsigned int get_boundary_size () {return boundaryList.size();}
      Boundary* get_boundary (long unsigned int i) {return boundaryList[i];}
      vector<Path *>* get_pathList () {return &pathList;}
      void print ();
};

#endif

