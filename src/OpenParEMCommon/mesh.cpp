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

#include "mesh.hpp"

// check that the field points are within the mesh's bounding box
bool check_field_points (const char *filename, mfem::Mesh *mesh, mfem::ParMesh *pmesh, int order, int dim, int field_points_count,
                         double *field_points_x, double *field_points_y, double *field_points_z)
{
   double tol=1e-12;
   bool fail=false;

   mfem::Vector lowerLeft,upperRight;
   if (mesh) mesh->GetBoundingBox(lowerLeft,upperRight,std::max(order,1));
   if (pmesh) pmesh->GetBoundingBox(lowerLeft,upperRight,std::max(order,1));

   int i=0;
   while (i < field_points_count) {

      bool pointFail=false;

      if (field_points_x[i] < lowerLeft.Elem(0)-tol) pointFail=true;
      if (field_points_x[i] > upperRight.Elem(0)+tol) pointFail=true;

      if (field_points_y[i] < lowerLeft.Elem(1)-tol) pointFail=true;
      if (field_points_y[i] > upperRight.Elem(1)+tol) pointFail=true;

      if (dim == 3) {
         if (field_points_z[i] < lowerLeft.Elem(2)-tol) pointFail=true;
         if (field_points_z[i] > upperRight.Elem(2)+tol) pointFail=true;
      }

      if (! fail && pointFail) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1094: Project file \"%s\":\n",filename);
      }

      if (pointFail) {
         fail=true;
         if (dim == 2) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"          field.point %g,%g falls outside of the mesh bounding box.\n",
                                                   field_points_x[i],field_points_y[i]);
         } else {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"          field.point %g,%g,%g falls outside of the mesh bounding box.\n",
                                                   field_points_x[i],field_points_y[i],field_points_z[i]);
         }
      }

      i++;
   }
   return fail;
}

// write x,y,z,attribute extracted from the mesh for the boundaries
// view with ParaView
bool write_attributes (const char *baseName, mfem::ParMesh *pmesh)
{
   PetscMPIInt size,rank;
   MPI_Comm_size(PETSC_COMM_WORLD, &size);
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   std::vector<double> x,y,z;
   std::vector<int> attribute;
   int dim=pmesh->Dimension();

   mfem::DenseMatrix pointMat;
   if (dim == 2) pointMat.SetSize(3,2);
   if (dim == 3) pointMat.SetSize(3,3);

   // loop through the local boundary elements
   int i=0;
   while (i < pmesh->GetNBE()) {

      int attr=pmesh->GetBdrAttribute(i);
      pmesh->GetBdrPointMatrix(i,pointMat);

      if (dim == 2) {
         x.push_back(pointMat(0,0));
         y.push_back(pointMat(1,0));
         attribute.push_back(attr);

         x.push_back(pointMat(0,1));
         y.push_back(pointMat(1,1));
         attribute.push_back(attr);

         x.push_back(pointMat(0,2));
         y.push_back(pointMat(1,2));
         attribute.push_back(attr);
      }

      if (dim == 3) {
         x.push_back(pointMat(0,0));
         y.push_back(pointMat(1,0));
         z.push_back(pointMat(2,0));
         attribute.push_back(attr);

         x.push_back(pointMat(0,1));
         y.push_back(pointMat(1,1));
         z.push_back(pointMat(2,1));
         attribute.push_back(attr);

         x.push_back(pointMat(0,2));
         y.push_back(pointMat(1,2));
         z.push_back(pointMat(2,2));
         attribute.push_back(attr);
      }

      i++;
   }

   // collect at 0
   if (rank == 0) {
      int i=1;
      while (i < size) {
         int transfer_size=0;
         MPI_Recv(&transfer_size,1,MPI_INT,i,100,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         // x
         double *pack=(double *)malloc(transfer_size*sizeof(double));
         MPI_Recv(pack,transfer_size,MPI_DOUBLE,i,101,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         int j=0;
         while (j < transfer_size) {
            x.push_back(pack[j]);
            j++;
         }

         // y
         MPI_Recv(pack,transfer_size,MPI_DOUBLE,i,102,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         j=0;
         while (j < transfer_size) {
            y.push_back(pack[j]);
            j++;
         }

         // z
         if (dim == 3) {
            MPI_Recv(pack,transfer_size,MPI_DOUBLE,i,103,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
            int j=0;
            while (j < transfer_size) {
               z.push_back(pack[j]);
               j++;
            }
         }
         if (pack) {free(pack); pack=nullptr;}

         // attribute
         int *packi=(int *)malloc(transfer_size*sizeof(int));
         MPI_Recv(packi,transfer_size,MPI_INT,i,104,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         j=0;
         while (j < transfer_size) {
            attribute.push_back(packi[j]);
            j++;
         }
         if (packi) {free(packi); packi=nullptr;}

         i++;
      }
   } else {
      int local_size=x.size();
      MPI_Send(&local_size,1,MPI_INT,0,100,PETSC_COMM_WORLD);

      // x
      double *pack=(double *)malloc(local_size*sizeof(double));
      long unsigned int i=0;
      while (i < local_size) {
         pack[i]=x[i];
         i++;
      }
      MPI_Send(pack,local_size,MPI_DOUBLE,0,101,PETSC_COMM_WORLD);

      // y
      i=0;
      while (i < local_size) {
         pack[i]=y[i];
         i++;
      }
      MPI_Send(pack,local_size,MPI_DOUBLE,0,102,PETSC_COMM_WORLD);

      // z
      if (dim == 3) {
         i=0;
         while (i < local_size) {
            pack[i]=z[i];
            i++;
         }
         MPI_Send(pack,local_size,MPI_DOUBLE,0,103,PETSC_COMM_WORLD);
      }
      if (pack) {free(pack); pack=nullptr;}

      // attribute
      int *packi=(int *)malloc(local_size*sizeof(int));
      i=0;
      while (i < local_size) {
         packi[i]=attribute[i];
         i++;
      }
      MPI_Send(packi,local_size,MPI_INT,0,104,PETSC_COMM_WORLD);
      if (packi) {free(packi); packi=nullptr;}
   }

   MPI_Barrier(PETSC_COMM_WORLD);

   // save the data from 0

   if (rank == 0) {

      std::stringstream ss;
      ss << baseName << "_attributes.csv";

      std::ofstream CSV;
      CSV.open(ss.str().c_str(),std::ofstream::out);
      if (CSV.is_open()) {

         if (dim == 2) CSV << "\"X\",\"Y\",\"attribute\"" << std::endl;
         if (dim == 3) CSV << "\"X\",\"Y\",\"Z\",\"attribute\"" << std::endl;

         int i=0;
         while (i < (int)x.size()) {
            if (dim == 2) CSV << x[i] << "," << y[i] << "," << attribute[i] << std::endl;
            if (dim == 3) CSV << x[i] << "," << y[i] << "," << z[i] << "," << attribute[i] << std::endl;
            i++;
         }
         CSV.close();
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1127: Failed to open file \"%s\" for writing.\n",ss.str().c_str());
         return true;
      }
   }

   MPI_Barrier(PETSC_COMM_WORLD);

   return false;
}

void push_if_unique (std::vector<int> *data, int value)
{
   long unsigned int i=0;
   while (i < data->size()) {
      if ((*data)[i] == value) return;
      i++;
   }
   data->push_back(value);
}

// reset the attributes so that they start with 1 and increase without gaps [required by MFEM]
void reset_attributes (mfem::Mesh *mesh, mfem::ParMesh *pmesh, MeshMaterialList *meshMaterials)
{
   PetscMPIInt size,rank;
   MPI_Comm_size(PETSC_COMM_WORLD, &size);
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   std::vector<int> attributes;

   // make a list of unique attributes then sort

   int NE=-1;
   if (mesh) NE=mesh->GetNE();
   if (pmesh) NE=pmesh->GetNE();

   int i=0;
   while (i < NE) {
      int attribute=-1;
      if (mesh) attribute=mesh->GetAttribute(i);
      if (pmesh) attribute=pmesh->GetAttribute(i);
      push_if_unique (&attributes,attribute);
      i++;
   }

   // make a global list

   //collect at zero
   if (rank == 0) {
      int i=1;
      while (i < size) {
         int transfer_size;
         MPI_Recv(&transfer_size,1,MPI_INT,i,100,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int k=0;
         while (k < transfer_size) {
            int transfer_value;
            MPI_Recv(&transfer_value,1,MPI_INT,i,101,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
            push_if_unique (&attributes,transfer_value);
            k++;
         }
 
         i++;
      }
   } else {
      int local_size=attributes.size();
      MPI_Send(&local_size,1,MPI_INT,0,100,PETSC_COMM_WORLD);

      int i=0;
      while (i < local_size) {
         int local_value=attributes[i];
         MPI_Send(&local_value,1,MPI_INT,0,101,PETSC_COMM_WORLD);
         i++;
      }
   }

   // distribute
   if (rank == 0) {

      sort(attributes.begin(),attributes.end());

      int i=1;
      while (i < size) {
         int local_size=attributes.size();
         MPI_Send(&local_size,1,MPI_INT,i,102,PETSC_COMM_WORLD);

         int j=0;
         while (j < local_size) {
            int local_value=attributes[j];
            MPI_Send(&local_value,1,MPI_INT,i,103,PETSC_COMM_WORLD);
            j++;
         }

         i++;
      }
   } else {
      attributes.clear();

      int transfer_size;
      MPI_Recv(&transfer_size,1,MPI_INT,0,102,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      int k=0;
      while (k < transfer_size) {
         int transfer_value;
         MPI_Recv(&transfer_value,1,MPI_INT,0,103,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         attributes.push_back(transfer_value);
         k++;
      }
   }

   // activate used materials
   meshMaterials->set_active(&attributes);

   // assign new attributes
   int new_attribute=1;
   long unsigned int k=0;
   while (k < attributes.size()) {

      // update the mesh
      int j=0;
      while (j < NE) {
         int attribute=-1;
         if (mesh) attribute=mesh->GetAttribute(j);
         if (pmesh) attribute=pmesh->GetAttribute(j);
         if (attribute == attributes[k]) {
            if (mesh) mesh->SetAttribute(j,new_attribute);
            if (pmesh) pmesh->SetAttribute(j,new_attribute);
         }
         j++;
      }

      // update the mesh materials
      meshMaterials->replace_index(attributes[k]-1,new_attribute-1);

      new_attribute++;
      k++;
   }

   // recalculate the support data structures
   if (mesh) mesh->SetAttributes();
   if (pmesh) pmesh->SetAttributes();
}

///////////////////////////////////////////////////////////////////////////////////////////
// MeshMaterialList
///////////////////////////////////////////////////////////////////////////////////////////

void MeshMaterialList::clear ()
{
   index.clear();
   list.clear();
   active.clear();
}

void MeshMaterialList::set_active (std::vector<int> *active_attributes)
{
   long unsigned int i=0;
   while (i < index.size()) {
      active[i]=false;
      i++;
   }

   i=0;
   while (i < index.size()) {
      long unsigned int j=0;
      while (j < active_attributes->size()) {
         if (index[i] == (*active_attributes)[j]-1) {
            active[i]=true;
            break;
         }
         j++;
      }
      i++;
   }
}

bool MeshMaterialList::load (const char *filename, int dimension)
{
   bool fail=false;
   PetscMPIInt size,rank;
   MPI_Comm_size(PETSC_COMM_WORLD, &size);
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   std::ifstream meshFile;
   std::string line;

   std::stringstream ss;
   ss << filename;

   if (std::filesystem::exists(ss.str().c_str())) {
      // serial mesh
      meshFile.open(ss.str().c_str(),std::ifstream::in);
   } else {
      // parallel mesh
      ss << "." << std::setw(6) << std::setfill('0') << rank;
      if (std::filesystem::exists(ss.str().c_str())) {
         meshFile.open(ss.str().c_str(),std::ifstream::in);
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1079: Mesh file for \"%s\" is not available for reading.\n",filename);
         return true;
      }
   }

   if (meshFile.is_open()) {

      if (getline(meshFile,line)) {
         if (line.compare("MFEM mesh v1.0") == 0 || line.compare("MFEM mesh v1.2") == 0) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"   loading MFEM mesh format\n");
            if (loadMFEM (filename)) fail=true;
         } else if (line.compare("$MeshFormat") == 0) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"   loading gmsh mesh format\n");
            if (loadGMSH(filename,dimension)) fail=true;
         } else {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1076: Unrecognized mesh format in file \"%s\".\n",filename);
            fail=true;
         }
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1077: Unable to read file \"%s\".\n",filename);
         fail=true;
      }
      meshFile.close();
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1034: File \"%s\" is not available for reading.\n",filename);
      fail=true;
   }

   return fail;
}

void MeshMaterialList::replace_index (int old_index, int new_index)
{
   long unsigned int i=0;
   while (i < index.size()) {
      if (active[i] && (index[i] == old_index)) index[i]=new_index;
      i++;
   }
}

// parse the msh file for the information in the $PhysicalNames block
int MeshMaterialList::loadGMSH (const char *filename, int dimension)
{
   long unsigned int materialCount=0;
   int lineCount=0;
   bool startedFormat=false,completedFormat=false,loadedFormat=false;
   bool startedNames=false,completedNames=false,loadedEntryCount=false;
   std::string line,version_number;
   std::vector<std::string> tokens;
   size_t pos1,pos2;

   // the mesh file in Gmsh 2.2 format 
   std::ifstream meshFile;
   meshFile.open(filename,std::ifstream::in);

   if (meshFile.is_open()) {

      while (getline(meshFile,line)) {
         lineCount++;

         // skip blank lines
         if (line.compare("") != 0) {

            // skip comment lines
            if (! is_comment(line)) {

               // chop off comments
               line=line.substr(0,line.find("//",0));

               if (startedFormat) {
                  if (line.compare("$EndMeshFormat") == 0) {
                     startedFormat=false; completedFormat=true;

                     if (GMSH_version_number.compare(version_number) != 0) {
                        prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1035: Incorrect mesh format of %s in file \"%s\".\n",version_number.c_str(),filename);
                        meshFile.close();
                        return 1;
                     }

                     if (file_type != 0) {
                        prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1036: Binary file form is not supported for file \"%s\".\n",filename);
                        meshFile.close();
                        return 1;
                     }
                  } else {
                     if (!loadedFormat) {
                        split_on_space (&tokens,line);
                        if (tokens.size() != 3) {
                           prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1037: Incorrect number of tokens in file \"%s\" at line %d.\n",filename,lineCount);
                           meshFile.close();
                           return 1;
                        }

                        version_number=tokens[0];
                        file_type=std::stoi(tokens[1]);
                        data_size=std::stoi(tokens[2]);

                        tokens.clear();
                        loadedFormat=true;
                     }
                  }
               }

               if (startedNames) {
                  if (line.compare("$EndPhysicalNames") == 0) {
                     startedNames=false; completedNames=true;

                     if (materialCount != list.size()) {
                        prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1038: Incorrect format in $PhysicalNames block in file \"%s\" at line %d\n",filename,lineCount);
                        meshFile.close();
                        return 1;
                     }
                  } else {

                     if (! loadedEntryCount) {
                        loadedEntryCount=true;
                     } else {
                        split_on_space (&tokens,line);
                        if (tokens.size() != 3) {
                           prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1039: Incorrect number of tokens in file \"%s\" at line %d.\n",filename,lineCount);
                           meshFile.close();
                           return 1;
                        }
                        int dim=std::stoi(tokens[0]);
                        if (dim != dimension) {
                           prefix(); PetscPrintf(PETSC_COMM_WORLD,"Warning: Dimension %d!=2 in file \"%s\" at line %d.\n",dim,filename,lineCount);
                        }

                        index.push_back(stoi(tokens[1])-1);
                        active.push_back(true);

                        // strip off "
                        pos1=tokens[2].find("\"",0);
                        if (pos1 >= 0) {
                           pos2=tokens[2].rfind("\"",tokens[2].length());
                           tokens[2]=tokens[2].substr(pos1+1,pos2-pos1-1);
                        }

                        list.push_back(tokens[2]);
                        tokens.clear();
                        materialCount++;
                     }
                  }
               }

               if (line.compare("$MeshFormat") == 0) startedFormat=true;
               if (line.compare("$PhysicalNames") == 0) {startedNames=true; materialCount=0;}
            }
         }
      }

      if (meshFile.bad()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1040: Error while reading file \"%s\".\n",filename);
         meshFile.close();
         return 1;
      }
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1041: File \"%s\" is not available for reading.\n",filename);
      return 1;
   }

   int retval=0;

   if (! completedFormat) {
      if (startedFormat) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1042: $MeshFormat block is missing the $EndMeshFormat statement in file \"%s\".\n",filename);}
      else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1043: $EndMeshFormat block is missing in file \"%s\".\n",filename);}
      retval=1;
   }

   if (! completedNames) {
      if (startedNames) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1044: $PhysicalNames block is missing the $EndPhysicalNames statement in file \"%s\".\n",filename);}
      else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1045: $PhysicalNames block is missing in file \"%s\".\n",filename);}
      retval=1;
   }

   meshFile.close();

   return retval;
}

int MeshMaterialList::size()
{
   return list.size();
}

long unsigned int MeshMaterialList::get_index (int attribute)
{
   long unsigned int i=0;
   while (i < index.size()) {
      if (active[i] && index[i] == attribute) return i;
      i++;
   }

   PetscPrintf(PETSC_COMM_WORLD,"ASSERT: MeshMaterialList::get_index failed to find data.\n");
   return 0;
}

int MeshMaterialList::get_index (long unsigned int m)
{
   if (m >= 0 && m < list.size() && active[m]) return index[m];
   return -1;
}

std::string MeshMaterialList::get_name (long unsigned int m)
{
   std::string a="ERROR1046: out of bounds";
   if (m >= 0 && m < list.size() && active[m]) return list[m];
   return a;
}

void MeshMaterialList::print ()
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"MeshMaterialList:\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   GMSH_version_number=%s\n",GMSH_version_number.c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   regionsFile_version_number=%s\n",regionsFile_version_number.c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   file-type=%d\n",file_type);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   data-size=%d\n",data_size);
   
   long unsigned int i=0;
   while (i < list.size()) {
      std::string active_state="false";
      if (active[i]) active_state="true";
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"   %d %s active=%s\n",index[i],list[i].c_str(),active_state.c_str());
      i++;
   }
}

int MeshMaterialList::loadMFEM (const char *filename)
{
   int lineCount=0;
   std::string line;
   std::vector<std::string> tokens;
   size_t pos1,pos2;

   std::stringstream regionsFilename;
   regionsFilename << "materials_for_" << filename;

   std::ifstream regionsFile;
   regionsFile.open(regionsFilename.str().c_str(),std::ifstream::in);

   if (regionsFile.is_open()) {

      std::stringstream ss;
      ss << "OpenParEM Regions v" << regionsFile_version_number;

      // first line is the version
      getline(regionsFile,line);
      lineCount++;
      if (line.compare(ss.str()) != 0) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1047: Incorrect regions format of \"%s\" in file \"%s\" at line 1.\n",ss.str().c_str(),filename);
         regionsFile.close();
         return 1;
      }

      // parse the rest
      while (getline(regionsFile,line)) {
         lineCount++;

         // skip blank lines
         if (line.compare("") != 0) {

            // skip comment lines
            if (! is_comment(line)) {

               // chop off comments
               line=line.substr(0,line.find("//",0));

               split_on_space (&tokens,line);
               if (tokens.size() != 2) {
                  prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1048: Incorrect number of tokens in file \"%s\" at line %d.\n",filename,lineCount);
                  regionsFile.close();
                  return 1;
               }

               index.push_back(stoi(tokens[0]));
               active.push_back(true);

               // strip off "
               pos1=tokens[1].find("\"",0);
               if (pos1 >= 0) {
                  pos2=tokens[1].rfind("\"",tokens[1].length());
                  tokens[1]=tokens[1].substr(pos1+1,pos2-pos1-1);
               }

               list.push_back(tokens[1]);

               tokens.clear();
            }
         }
      }

      if (regionsFile.bad()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1049: Error while reading file \"%s\".\n",filename);
         regionsFile.close();
         return 1;
      }

      regionsFile.close();
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1078: File \"%s\" is not available for reading.\n",regionsFilename.str().c_str());
      return 1;
   }

   return 0;
}

bool MeshMaterialList::saveRegionsFile (const char *filename)
{
   std::ofstream regionsFile;
   regionsFile.open(filename,std::ofstream::out);

   if (regionsFile.is_open()) {
      regionsFile << "OpenParEM Regions v" << regionsFile_version_number << std::endl;

      long unsigned int i=0;
      while (i < index.size()) {
         if (active[i]) {
            regionsFile << index[i] << " " << list[i] << std::endl;
         }
         i++;
      } 

      regionsFile.close();
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1051: Cannot open file \"%s\" for writing.\n",filename);
      return true;
   }

   return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Vertex3Ddatabase
///////////////////////////////////////////////////////////////////////////////////////////

long unsigned int Vertex3Ddatabase::find (Vertex3D *vertex3D)
{
   long unsigned int max=-1;
   long unsigned int i=0;
   while (i < vertex3DList.size()) {
      if (double_compare(vertex3D->get_x(),vertex3DList[i]->get_x(),tol) &&
          double_compare(vertex3D->get_y(),vertex3DList[i]->get_y(),tol) &&
          double_compare(vertex3D->get_z(),vertex3DList[i]->get_z(),tol)) {return i;}
      i++;
   }
   return max;
}

Vertex3Ddatabase::~Vertex3Ddatabase()
{
   long unsigned int i=0;
   while (i < vertex3DList.size()) {
      delete vertex3DList[i];
      i++;
   }
}

