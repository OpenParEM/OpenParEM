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

// Class for reading the $PhysicalNames block from Gmsh mesh files format 2.2

#ifndef MESH_H
#define MESH_H

#include "mfem.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <unistd.h>
#include "petscsys.h"
#include "misc.hpp"
#include "prefix.h"

extern "C" void prefix ();

bool is_comment (std::string);
void split_on_space (std::vector<std::string> *, std::string);
bool check_field_points (const char *, mfem::Mesh *, mfem::ParMesh *, int, int, int, double *, double *, double *);
bool write_attributes (const char *, mfem::ParMesh *);

class MeshMaterialList {
    private:
       std::string GMSH_version_number="2.2";
       std::string regionsFile_version_number="1.0";
       int file_type;
       int data_size;
       std::vector<int> index;       // mesh attribute (-1)
       std::vector<std::string> list;     // material name
       std::vector<bool> active;
    public:
       void set_active (std::vector<int> *);
       bool load (const char *, int);
       void replace_index (int, int);
       int loadGMSH (const char *, int);
       int loadMFEM (const char *);
       bool saveRegionsFile (const char *filename);
       void print ();
       int size ();
       long unsigned int get_index (int);
       std::string get_name (long unsigned int);
};

void reset_attributes (mfem::Mesh *, mfem::ParMesh *, MeshMaterialList *);

class Vertex3D {
   private:
      double x,y,z;
   public:
      Vertex3D (double x_, double y_, double z_) {x=x_; y=y_; z=z_;}
      double get_x() {return x;}
      double get_y() {return y;}
      double get_z() {return z;}
};

class Vertex3Ddatabase {
   private:
      std::vector<Vertex3D *> vertex3DList;
      double tol=1e-12;
   public:
      ~Vertex3Ddatabase ();
      long unsigned int size () {return vertex3DList.size();}
      Vertex3D* get_Vertex3D (long unsigned int i) {return vertex3DList[i];}
      long unsigned int find (Vertex3D *);
      long unsigned int push (Vertex3D *a) {vertex3DList.push_back(a); return vertex3DList.size()-1;}
};

#endif

