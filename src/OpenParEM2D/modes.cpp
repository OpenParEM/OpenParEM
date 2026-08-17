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

#include "modes.hpp"

bool operator==(struct EdgeAttribute a, struct EdgeAttribute b)
{
   if (a.boundary != b.boundary) return false;
   if (a.path != b.path) return false;
   if (a.segment != b.segment) return false;
   //if (!double_compare(a.x1,b.x1,1e-12)) return false;
   //if (!double_compare(a.y1,b.y1,1e-12)) return false;
   //if (!double_compare(a.x2,b.x2,1e-12)) return false;
   //if (!double_compare(a.y2,b.y2,1e-12)) return false;
   return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Boundary
///////////////////////////////////////////////////////////////////////////////////////////

Boundary::Boundary(int startLine_, int endLine_, string mode_block_type_)
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

   // mode/line number (conductor number)
   mode.push_alias("mode");
   mode.push_alias("line");
   mode.set_loaded(false);
   mode.set_positive_required(true);
   mode.set_non_negative_required(false);
   mode.set_lowerLimit(1);
   mode.set_upperLimit(10000);
   mode.set_checkLimits(true);

   // mode_block_type
   mode_block_type.push_alias("mode_block_type");
   mode_block_type.set_value(mode_block_type_);
   mode_block_type.set_loaded(true);
   mode_block_type.set_positive_required(false);
   mode_block_type.set_non_negative_required(false);
   mode_block_type.set_lowerLimit(0);
   mode_block_type.set_upperLimit(0);
   mode_block_type.set_checkLimits(false);

   // type
   type.push_alias("type");
   type.set_loaded(false);
   type.set_positive_required(false);
   type.set_non_negative_required(false);
   type.set_lowerLimit(0);
   type.set_upperLimit(0);
   type.set_checkLimits(false);

   // material
   material.push_alias("material");
   material.set_loaded(false);
   material.set_positive_required(false);
   material.set_non_negative_required(false);
   material.set_lowerLimit(0);
   material.set_upperLimit(0);
   material.set_checkLimits(false);

   // scale
   scale.push_alias("scale");
   scale.set_loaded(false);
   scale.set_positive_required(true);
   scale.set_non_negative_required(false);
   scale.set_lowerLimit(1e-3);
   scale.set_upperLimit(1e3);
   scale.set_checkLimits(true);

   // attribute
   attribute.push_alias("attribute");
   attribute.set_loaded(false);
   attribute.set_positive_required(false);
   attribute.set_non_negative_required(true);
   attribute.set_lowerLimit(0);
   attribute.set_upperLimit(1000000);
   attribute.set_checkLimits(true);

   // set defaults

   scale.set_dbl_value(1);
}

// is_boundary=false => load as mode
// is_boundary=true => load as boundary
bool Boundary::load(string *indent, inputFile *inputs, bool is_boundary, int attribute_)
{
   bool fail=false;
   bool found_first_path=false;

   attribute.set_int_value(attribute_);

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {

      string token,value,line;
      line=inputs->get_line(lineNumber);
      get_token_pair(&line,&token,&value,&lineNumber,*indent);

      int recognized=0;

      if (is_boundary && name.match_alias(&token)) {
         if (name.is_loaded()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2164: Duplicate entry at line %d for previous entry at line %d.\n",
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

      if (!is_boundary && mode.match_alias(&token)) {
         if (token.compare("mode") == 0 && is_line()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2165: \"mode\" defined at line %d should be \"line\".\n",
                                                   indent->c_str(),indent->c_str(),lineNumber);
            fail=true;
         }
         if (token.compare("line") == 0 && is_modal()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2166: \"line\" defined at line %d should be \"mode\".\n",
                                                   indent->c_str(),indent->c_str(),lineNumber);
            fail=true;
         }
         if (!fail && mode.loadInt(&token, &value, lineNumber)) fail=true;
         recognized++;
      }

      if (type.match_alias(&token)) {
         if (type.is_loaded()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2167: Duplicate entry at line %d for previous entry at line %d.\n",
                                                   indent->c_str(),indent->c_str(),lineNumber,type.get_lineNumber());
            fail=true;
         } else {
            type.set_keyword(token);
            type.set_value(value);
            type.set_lineNumber(lineNumber);
            type.set_loaded(true);
         }
         recognized++;
      }

      if (is_boundary && material.match_alias(&token)) {
         if (material.is_loaded()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2168: Duplicate entry at line %d for previous entry at line %d.\n",
                                                   indent->c_str(),indent->c_str(),lineNumber,material.get_lineNumber());
            fail=true;
         } else {
            material.set_keyword(token);
            material.set_value(value);
            material.set_lineNumber(lineNumber);
            material.set_loaded(true);
         }
         recognized++;
      }

      if (scale.match_alias(&token)) {
         recognized++;
         if (scale.loadDouble(&token,&value,lineNumber)) fail=true;
      }

      if (token.compare("path") == 0) {
         if (found_first_path) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2169: Extraneous path= statement at line %d.\n",
                                                   indent->c_str(),indent->c_str(),lineNumber);
            fail=true;
         } else {
            bool reverse=false;
            if (value.substr(0,1).compare("+") == 0) value=value.substr(1);
            else if (value.substr(0,1).compare("-") == 0) {value=value.substr(1); reverse=true;}

            keywordPair *path=new keywordPair();
            path->push_alias("path");
            path->set_keyword(token);
            path->set_value(value);
            path->set_lineNumber(lineNumber);
            path->set_positive_required(false);
            path->set_non_negative_required(false);
            path->set_lowerLimit(0);
            path->set_upperLimit(0);
            path->set_checkLimits(false);
            path->set_loaded(true);

            pathNameList.push_back(path);
            reverseList.push_back(reverse);
         }

         found_first_path=true;
         recognized++;
      }

      if (token.compare("path+") == 0) {
         if (found_first_path) {
            if (value.substr(0,1).compare("+")  == 0 || value.substr(0,1).compare("-") == 0) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2170: Misformatted path at line %d.\n",
                                                      indent->c_str(),indent->c_str(),lineNumber);
               fail=true;
            } else {

               keywordPair *path=new keywordPair();
               path->push_alias("path");
               path->set_keyword(token);
               path->set_value(value);
               path->set_lineNumber(lineNumber);
               path->set_positive_required(false);
               path->set_non_negative_required(false);
               path->set_lowerLimit(0);
               path->set_upperLimit(0);
               path->set_checkLimits(false);
               path->set_loaded(true);

               pathNameList.push_back(path);
               reverseList.push_back(false);
            }
         } else {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2171: Missing path= statement before line %d.\n",
                                                   indent->c_str(),indent->c_str(),lineNumber);
            fail=true;
         }
         recognized++;
      }

      if (token.compare("path-") == 0) {
         if (found_first_path) {
            if (value.substr(0,1).compare("+") == 0 || value.substr(0,1).compare("-") == 0) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2172: Misformatted path at line %d.\n",
                                                      indent->c_str(),indent->c_str(),lineNumber);
               fail=true;
            } else {

               keywordPair *path=new keywordPair();
               path->push_alias("path");
               path->set_keyword(token);
               path->set_value(value);
               path->set_lineNumber(lineNumber);
               path->set_positive_required(false);
               path->set_non_negative_required(false);
               path->set_lowerLimit(0);
               path->set_upperLimit(0);
               path->set_checkLimits(false);
               path->set_loaded(true);

               pathNameList.push_back(path);
               reverseList.push_back(true);
            }
         } else {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2173: Missing path= statement before line %d.\n",
                                                   indent->c_str(),indent->c_str(),lineNumber);
            fail=true;
         }
         recognized++;
      }

      // should recognize one keyword
      if (recognized != 1) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2174: Unrecognized keyword at line %d.\n",
                                                indent->c_str(),indent->c_str(),lineNumber);
         fail=true;
      }
      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }

   return fail;
}

struct EdgeAttribute Boundary::is_segmentOnPath (int border_element, struct point p1, struct point p2, vector<Path *> *pathList)
{
   long unsigned int max=-1;
   long unsigned int i=0;
   struct EdgeAttribute attribute;

   attribute.border_element=-1;
   attribute.boundary=-1;
   attribute.path=-1;
   attribute.segment=-1;
   attribute.x1=DBL_MAX;
   attribute.y1=DBL_MAX;
   attribute.x2=DBL_MAX;
   attribute.y2=DBL_MAX;

   while (i < pathIndexList.size()) {
      long unsigned int segment_index=(*pathList)[pathIndexList[i]]->is_segmentOnLine(p1,p2);
      if (segment_index != max) {
         attribute.border_element=border_element;
         attribute.boundary=get_attribute();
         attribute.path=i;
         attribute.segment=segment_index;
         attribute.x1=(*pathList)[pathIndexList[i]]->get_point_value(segment_index).x;
         attribute.y1=(*pathList)[pathIndexList[i]]->get_point_value(segment_index).y;
         attribute.x2=(*pathList)[pathIndexList[i]]->get_point_value(segment_index+1).x;
         attribute.y2=(*pathList)[pathIndexList[i]]->get_point_value(segment_index+1).y;
         return attribute;
      }
      i++;
   }
   return attribute;
}

bool Boundary::is_surface_impedance()
{
   if (type.get_value().compare("surface_impedance") == 0) return true;
   return false;
}

bool Boundary::is_perfect_electric_conductor()
{
   if (type.get_value().compare("perfect_electric_conductor") == 0) return true;
   return false;
}

bool Boundary::is_perfect_magnetic_conductor()
{
   if (type.get_value().compare("perfect_magnetic_conductor") == 0) return true;
   return false;
}

bool Boundary::is_mode_voltage()
{
   if (type.get_value().compare("voltage") == 0) return true;
   return false;
}

bool Boundary::is_mode_current()
{
   if (type.get_value().compare("current") == 0) return true;
   return false;
}

bool Boundary::is_boundary()
{
   if (is_surface_impedance()) return true;
   if (is_perfect_electric_conductor()) return true;
   if (is_perfect_magnetic_conductor()) return true;
   return false;
}

bool Boundary::is_modal()
{
   if (mode_block_type.get_value().compare("modal") == 0) return true;
   return false;
}

bool Boundary::is_line()
{
   if (mode_block_type.get_value().compare("line") == 0) return true;
   return false;
}

bool Boundary::is_mode()
{
   if (is_modal()) return true;
   if (is_line()) return true;
   return false;
}

string Boundary::get_block_type()
{
   if (is_modal()) return "Mode";
   if (is_line()) return "Line";
   return "null";
}

string Boundary::get_mode_name()
{
   if (is_modal()) return "mode";
   if (is_line()) return "line";
   return "null";
}

void Boundary::print()
{
   if (is_boundary()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"Boundary\n");
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"   name=%s\n",get_name().c_str());
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"   type=%s\n",get_type().c_str());
      if (is_surface_impedance() && type.is_loaded()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   material=%s\n",get_material().c_str());}
      long unsigned int i=0;
      while (i < pathNameList.size()) {
         if (i == 0) {
            if (reverseList[i]) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   path=-%s\n",pathNameList[i]->get_value().c_str());}
            else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   path=%s\n",pathNameList[i]->get_value().c_str());}
         } else {
            if (reverseList[i]) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   path-=%s\n",pathNameList[i]->get_value().c_str());}
            else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   path+=%s\n",pathNameList[i]->get_value().c_str());}
         }
         i++;
      }
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"   [attribute=%d]\n",attribute.get_int_value());
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"EndBoundary\n");
      return;
   }

   if (is_mode()) {
      if (is_modal()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"Mode\n");
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"   mode=%d\n",get_mode());
      }
      if (is_line()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"Line\n");
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"   line=%d\n",get_mode());
      }
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"   type=%s\n",get_type().c_str());
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"   scale=%g\n",get_scale());
      long unsigned int i=0;
      while (i < pathNameList.size()) {
         if (i == 0) {
            if (reverseList[i]) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   path=-%s\n",pathNameList[i]->get_value().c_str());}
            else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   path=%s\n",pathNameList[i]->get_value().c_str());}
         } else {
            if (reverseList[i]) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   path-=%s\n",pathNameList[i]->get_value().c_str());}
            else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   path+=%s\n",pathNameList[i]->get_value().c_str());}
         }
         i++;
      }
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"   [attribute=%d]\n",attribute.get_int_value());
      if (is_modal()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"EndMode\n");}
      if (is_line()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"EndLine\n");}
      return;
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Boundary::print() did not find valid type.\n");
   return;
}

bool Boundary::inBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

bool Boundary::check(string *indent)
{
   bool fail=false;

   if (! type.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2175: Block at line %d must specify a type.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      return true;
   }

   if (is_boundary()) {
      if (!name.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2176: Boundary block at line %d must specify a name.\n",
                                                indent->c_str(),indent->c_str(),startLine);
         fail=true;
      }

      if (mode.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2177: Boundary block at line %d must not specify a mode or line.\n",
                                                indent->c_str(),indent->c_str(),startLine);
         fail=true;
      }

      if (pathNameList.size() == 0) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2178: Boundary block at line %d must specify at least one path.\n",
                                                indent->c_str(),indent->c_str(),startLine);
         fail=true;
      }

      if (is_surface_impedance()) {
         if (!material.is_loaded()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2179: Boundary block at line %d must specify a material.\n",
                                                   indent->c_str(),indent->c_str(),startLine);
            fail=true;
         }
      } else if (is_perfect_electric_conductor() || is_perfect_magnetic_conductor()) {
         if (material.is_loaded()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2180: Boundary block at line %d must not specify a material.\n",
                                                   indent->c_str(),indent->c_str(),startLine);
            fail=true;
         }
      }
   } else if (is_mode()) {

      if (name.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2181: %s block at line %d not must specify a name.\n",
                                                indent->c_str(),indent->c_str(),get_block_type().c_str(),startLine);
         fail=true;
      }

      if (! mode.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2182: %s block at line %d must specify a %s.\n",
                                                indent->c_str(),indent->c_str(),get_block_type().c_str(),startLine,get_mode_name().c_str());
         fail=true;
      }

      if (material.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2183: %s block at line %d not must specify a material.\n",
                                                indent->c_str(),indent->c_str(),get_block_type().c_str(),startLine);
         fail=true;
      }

      // scale is optional

      if (pathNameList.size() == 0) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2184: %s block at line %d must specify at least one path.\n",
                                                indent->c_str(),indent->c_str(),get_block_type().c_str(),startLine);
         fail=true;
      }
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2185: type at line %d is invalid.\n",
                                             indent->c_str(),indent->c_str(),type.get_lineNumber());
      fail=true;
   }

   return fail;
}

bool Boundary::align (string *indent, vector<Path *> *pathList, double *area, bool check_closed_loop)
{
   bool fail=false;
   long unsigned int path_index;
   Path *path;
   struct point pt0,ptend;
   vector<long unsigned int> pathComponents;

   if (pathIndexList.size() == 0) {fail=true; return fail;}

   pt0.dim=2; pt0.z=0;
   ptend.dim=2; ptend.z=0;

   // initialized the used markers
   vector<bool> used;
   long unsigned int i=0;
   while (i < pathIndexList.size()) {
      used.push_back(false);
      i++;
   }

   // initializee areas
   *area=0;
   double path_area=0;

   // add the closed paths
   i=0;
   while (i < pathIndexList.size()) {
      path=(*pathList)[pathIndexList[i]];
      if (path->get_closed()) {
         path_area=path->area();
         if (path_area < 0) {
            path_area=-path_area;
            path->reverseOrder();
         }
         (*area)+=path_area;
         used[i]=true;
      }
      i++;
   }

   // do the rest by linking the pieces together
   // cycle until all paths are used

   bool start_new_path=true;
   path_area=0;

   i=0;
   while (i < pathIndexList.size()+1) {   // max possible number of loops + 1, enables topology error check

      // get a path
      path=nullptr;
      long unsigned int j=0;
      while (j < used.size()) {
         if (!used[j]) {

            Path *tpj=(*pathList)[pathIndexList[j]];

            if (start_new_path) {

               path_area=0;

               // pick a path with a dangling end, if that exists

               int match_start_count=0;
               int match_end_count=0;
               long unsigned int k=0;
               while (k < used.size()) {
                  if (!used[k] && k != j) {
                     Path *tpk=(*pathList)[pathIndexList[k]];
                     if (point_comparison(tpk->get_point_value(0),tpj->get_point_value(0),1e-12)) match_start_count++;
                     if (point_comparison(tpk->get_point_value(0),tpj->get_point_value(tpj->get_points_size()-1),1e-12)) match_end_count++;
                     if (point_comparison(tpk->get_point_value(tpk->get_points_size()-1),tpj->get_point_value(0),1e-12)) match_start_count++;
                     if (point_comparison(tpk->get_point_value(tpk->get_points_size()-1),tpj->get_point_value(tpj->get_points_size()-1),1e-12)) match_end_count++;
                  }
                  k++;
               }

               // see if this is the last remaining available path
               bool is_last=true;
               k=j+1;
               while (k < used.size()) {
                  if (!used[k]) is_last=false;
                  k++;
               }

               // keep the dangling path or the last unused path

               if (match_start_count == 1 && match_end_count == 0) {
                  path_index=j;
                  path=(*pathList)[pathIndexList[path_index]];
                  path->reverseOrder();
                  used[path_index]=true;
//                  start_new_path=false;
//                  ptend=path->get_point_value(path->get_points_size()-1);
                  break;
               }

               if ((match_start_count == 0 && match_end_count == 1) || is_last) {
                  path_index=j;
                  path=(*pathList)[pathIndexList[path_index]];
                  used[path_index]=true;
//                  start_new_path=false;
//                  ptend=path->get_point_value(0);
                  break;
               }

            } else {
               if (point_comparison(tpj->get_point_value(0),ptend,1e-12)) {
                  path_index=j;
                  path=tpj;
                  used[path_index]=true;
//                  ptend=path->get_point_value(path->get_points_size()-1);
                  break;
               }

               if (point_comparison(tpj->get_point_value(tpj->get_points_size()-1),ptend,1e-12)) {
                  path_index=j;
                  path=tpj;
                  path->reverseOrder();
                  used[path_index]=true;
//                  ptend=path->get_point_value(0);
                  break;
               }
            }
         }
         j++;
      }

      // see if finished
      if (!path) {
         if (!start_new_path) {
            if (check_closed_loop) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2145: %s block at line %d is not closed.\n",
                                                      indent->c_str(),indent->c_str(),get_block_type().c_str(),startLine);
               fail=true;
            } // else, the area is not defined for a non-closed loop, so the computed area is a figure-of-merit
         }

         // reverse path if clockwise
         if (path_area < 0) {
            path_area=-path_area;
            long unsigned int j=0;
            while (j < pathComponents.size()) {
               (*pathList)[pathIndexList[pathComponents[j]]]->reverseOrder();
               j++;
            }
         }
         (*area)+=path_area;

         // check that all the segments have the same direction
         int count_forward=0;
         int count_reverse=0;
         long unsigned int j=0;
         while (j < pathComponents.size()) {
            if (reverseList[pathComponents[j]]) count_reverse++;
            else count_forward++;
            j++;
         }

         if (count_forward*count_reverse != 0) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2146: %s block at line %d has mixed segment directions.\n",
                                                   indent->c_str(),indent->c_str(),get_block_type().c_str(),startLine);
            fail=true;
         }

         break;
      }

      // keep track of paths in this loop for reversing direction, if necessary
      pathComponents.push_back(path_index);

      if (start_new_path) {
         start_new_path=false;
         pt0=path->get_point_value(0);
//         ptend=path->get_point_value(path->get_points_size()-1);
      }

      ptend=path->get_point_value(path->get_points_size()-1);

      path_area+=path->area();

      // check if this loop is completed by comparing the end vs. the start
      if (point_comparison(pt0,ptend,1e-12)) {

         // reverse path if clockwise
         if (path_area < 0) {
            path_area=-path_area;
            long unsigned int j=0;
            while (j < pathComponents.size()) {
               (*pathList)[pathIndexList[pathComponents[j]]]->reverseOrder();
               j++;
            }
         }

         (*area)+=path_area;

         // check that all the segments have the same direction
         int count_forward=0;
         int count_reverse=0;
         long unsigned int j=0;
         while (j < pathComponents.size()) {
            if (reverseList[pathComponents[j]]) count_reverse++;
            else count_forward++;
            j++;
         }

         if (!fail && count_forward*count_reverse != 0) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2187: %s block at line %d has mixed segment directions.\n",
                                                   indent->c_str(),indent->c_str(),get_block_type().c_str(),startLine);
            fail=true;
         }

         pathComponents.clear();
         start_new_path=true;
         path_area=0;
      }

      i++;
   }

   // topology failure
   if (i == pathIndexList.size()+1) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2148: %s block at line %d topology error for path %ld.\n",
                                              indent->c_str(),indent->c_str(),get_block_type().c_str(),startLine,i+1);
      fail=true;
   }

   // check for unused paths - should not occur
   i=0;
   while (i < used.size()) {
      if (!used[i]) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2237: %s block at line %d topology error for path %ld.\n",
                                                 indent->c_str(),indent->c_str(),get_block_type().c_str(),startLine,i+1);
         fail=true;
      }
      i++;
   }

   return fail;
}

// For currents, paths must be counterclockwise for postive currents in the z-direction.
// Optionally, closed paths can be required.
bool Boundary::align_current_paths (string *indent, vector<Path *> *pathList, bool check_closed_loop)
{
   bool fail=false;

   if (is_mode_current()) {
      double path_area=0;
      fail=align(indent,pathList,&path_area,check_closed_loop);

      // should not occur
      if (path_area < -1e-14) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2186: %s block at line %d has the path defined in the clockwise direction.\n",
                                                indent->c_str(),indent->c_str(),get_block_type().c_str(),startLine);
         fail=true;
      }
   }

   return fail;
}

bool Boundary::checkBoundingBox (Vector *lowerLeft, Vector *upperRight, string *indent, double tol, vector<Path *> *pathList)
{
   bool fail=false;

   long unsigned int i=0;
   while (i < pathIndexList.size()) {
      if ((*pathList)[pathIndexList[i]]->checkBoundingBox(lowerLeft,upperRight,indent,tol)) fail=true;
      i++;
   }
   return fail;
}

// Mark mesh borders with an attribute indexing into the border database.
// The border database includes indexes into the boundary database for both boundaries and modes.
// The mesh is local, so the border database is local.
void Boundary::markMeshBoundaries (Mesh *mesh, ParMesh *pmesh, BorderDatabase *borderDatabase, vector<Path *> *pathList)
{
   long unsigned int max=-1;
   struct EdgeAttribute test_attribute;

   int nbe=-1;
   if (mesh) nbe=mesh->GetNBE();
   if (pmesh) nbe=pmesh->GetNBE();

   // loop through the mesh border elements
   int i=0;
   while (i < nbe) {

      // get the border vertices
      Array<int> vertices;
      if (mesh) mesh->GetBdrElementVertices(i,vertices);
      if (pmesh) pmesh->GetBdrElementVertices(i,vertices);

      // loop through the vertices and see if they fall on the path of the boundary
      if (vertices.Size() == 2) {
         double *vertex0,*vertex1;
         if (mesh) {
            vertex0=mesh->GetVertex(vertices[0]);
            vertex1=mesh->GetVertex(vertices[1]);
         }
         if (pmesh) {
            vertex0=pmesh->GetVertex(vertices[0]);
            vertex1=pmesh->GetVertex(vertices[1]);
         }
         struct point p1; p1.dim=2; p1.x=vertex0[0]; p1.y=vertex0[1];
         struct point p2; p2.dim=2; p2.x=vertex1[0]; p2.y=vertex1[1];

         test_attribute=is_segmentOnPath(i,p1,p2,pathList);

         if (test_attribute.boundary != max) {
            int current_bdrAttribute=-1;
            if (mesh) current_bdrAttribute=mesh->GetBdrAttribute(i);
            if (pmesh) current_bdrAttribute=pmesh->GetBdrAttribute(i);
            int new_bdrAttribute;
            if (is_boundary()) new_bdrAttribute=borderDatabase->add(test_attribute,current_bdrAttribute);
            else new_bdrAttribute=borderDatabase->add(test_attribute,get_mode(),current_bdrAttribute);
            if (mesh) mesh->SetBdrAttribute(i,new_bdrAttribute);
            if (pmesh) pmesh->SetBdrAttribute(i,new_bdrAttribute);
         }
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Boundary::markMeshBoundaries found boundary edge without two points.\n");
      }
      i++;
   }
   return;
}

Boundary::~Boundary()
{
   long unsigned int i=0;
   while (i < pathNameList.size()) {
      delete pathNameList[i];
      i++;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
// BoundaryDatabase
///////////////////////////////////////////////////////////////////////////////////////////

void BoundaryDatabase::mark_boundaries (Mesh *mesh, ParMesh *pmesh, BorderDatabase *borderDatabase)
{
   // pre-set the mesh attributes to 1 - assumed PEC if there are no BorderDatabase elements
   //                                    Note the MFEM does not allow 0 as an attribute, so starting with 1 instead.
   // The mesh attribute is an index into a BorderDatabase that in turn holds indices into a BoundaryDatabase

   if (mesh) {
      int j=0;
      while (j < mesh->GetNBE()) {
         mesh->SetBdrAttribute(j,1);
         j++;
      }
   }

   if (pmesh) {
      int j=0;
      while (j < pmesh->GetNBE()) {
         pmesh->SetBdrAttribute(j,1);
         j++;
      }
   }

   // set the attributes to index into a BorderDatabase
   // the BorderDatabase is set up in the process
   // The boundary database is global, but the generated borderDatabase is local.
   long unsigned int i=0;
   while (i < boundaryList.size()) {
      boundaryList[i]->markMeshBoundaries(mesh,pmesh,borderDatabase,&pathList);
      i++;
   }

   return;
}

bool BoundaryDatabase::findSourceFileBlocks ()
{
   bool fail=false;
   int start_lineNumber=inputs.get_first_lineNumber();
   int stop_lineNumber=inputs.get_last_lineNumber();
   int block_start,block_stop;

   // skip the version line, which must be the first line
   start_lineNumber=inputs.get_next_lineNumber(start_lineNumber);

   while (start_lineNumber < stop_lineNumber) {
      if (inputs.findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "File", "EndFile", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            SourceFile *newSourceFile=new SourceFile(block_start,block_stop);
            sourceFileList.push_back(newSourceFile);
         }
      }
      start_lineNumber=inputs.get_next_lineNumber(block_stop);
   }
   return fail;
}

bool BoundaryDatabase::findPathBlocks ()
{
   bool fail=false;
   int start_lineNumber=inputs.get_first_lineNumber();
   int stop_lineNumber=inputs.get_last_lineNumber();
   int block_start,block_stop;

   // skip the version line, which must be the first line
   start_lineNumber=inputs.get_next_lineNumber(start_lineNumber);

   while (start_lineNumber < stop_lineNumber) {
      if (inputs.findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "Path", "EndPath", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            Path *newPath=new Path(block_start,block_stop);
            pathList.push_back(newPath);
         }
      }
      start_lineNumber=inputs.get_next_lineNumber(block_stop);
   }
   return fail;
}

bool BoundaryDatabase::findBoundaryBlocks ()
{
   bool fail=false;
   int start_lineNumber=inputs.get_first_lineNumber();
   int stop_lineNumber=inputs.get_last_lineNumber();
   int block_start,block_stop;

   // skip the version line, which must be the first line
   start_lineNumber=inputs.get_next_lineNumber(start_lineNumber);

   while (start_lineNumber < stop_lineNumber) {
      if (inputs.findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "Boundary", "EndBoundary", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            Boundary *newBoundary=new Boundary(block_start,block_stop,"");
            boundaryList.push_back(newBoundary);
         }
      }
      start_lineNumber=inputs.get_next_lineNumber(block_stop);
   }
   return fail;
}

bool BoundaryDatabase::findModeBlocks ()
{
   bool fail=false;
   int start_lineNumber=inputs.get_first_lineNumber();
   int stop_lineNumber=inputs.get_last_lineNumber();
   int block_start,block_stop;

   // skip the version line, which must be the first line
   start_lineNumber=inputs.get_next_lineNumber(start_lineNumber);

   while (start_lineNumber < stop_lineNumber) {
      if (inputs.findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "Mode", "EndMode", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            Boundary *newBoundary=new Boundary(block_start,block_stop,"modal");
            boundaryList.push_back(newBoundary);
         }
      }
      start_lineNumber=inputs.get_next_lineNumber(block_stop);
   }
   return fail;
}

bool BoundaryDatabase::findLineBlocks ()
{
   bool fail=false;
   int start_lineNumber=inputs.get_first_lineNumber();
   int stop_lineNumber=inputs.get_last_lineNumber();
   int block_start,block_stop;

   // skip the version line, which must be the first line
   start_lineNumber=inputs.get_next_lineNumber(start_lineNumber);

   while (start_lineNumber < stop_lineNumber) {
      if (inputs.findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "Line", "EndLine", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            Boundary *newBoundary=new Boundary(block_start,block_stop,"line");
            boundaryList.push_back(newBoundary);
         }
      }
      start_lineNumber=inputs.get_next_lineNumber(block_stop);
   }
   return fail;
}

bool BoundaryDatabase::load(const char *filename, bool check_closed_loop) {
   bool fail=false;
   string line;
   int dim=2;

   if (strcmp(filename,"") == 0) return false;  // this file is optional

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   loading mode definition file \"%s\"\n",filename);

   if (inputs.load(filename)) return true;
   inputs.createCrossReference();

   if (inputs.checkVersion(version_name, version_value)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2192: Version mismatch.  Expecting the first line to be: %s %s\n",
                                             indent.c_str(),indent.c_str(),version_name.c_str(),version_value.c_str());
      return true;
   }

   // Source
   if (findSourceFileBlocks()) {fail=true;}

   long unsigned int i=0;
   while (i < sourceFileList.size()) {
      if (sourceFileList[i]->load(&indent,&inputs)) fail=true;
      i++;
   }

   // Path
   if (findPathBlocks()) fail=true;

   i=0;
   while (i < pathList.size()) {
      if (pathList[i]->load(dim,&indent,&inputs)) fail=true;
      i++;
   }

   // Boundary

   if (findBoundaryBlocks()) fail=true;
   long unsigned int boundaryCount=boundaryList.size();

   i=0;
   while (i < boundaryCount) {
      if (boundaryList[i]->load(&indent,&inputs,true,i)) fail=true;
      i++;
   }

   // Mode
   if (findModeBlocks()) fail=true;

   // Line
   if (findLineBlocks()) fail=true;

   i=boundaryCount;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->load(&indent,&inputs,false,i)) fail=true;
      i++;
   }

   // database checks
   if (check()) fail=true;

   // fill out with path pointers for later convenience
   i=0;
   while (i < boundaryList.size()) {
      long unsigned int j=0;
      while (j < boundaryList[i]->get_pathNameList_size()) {
         bool found=false;
         long unsigned int k=0;
         while (k < pathList.size()) {
            if (pathList[k]->get_name().compare(boundaryList[i]->get_pathName(j)) == 0) {
               boundaryList[i]->push(k);
               found=true;
               break;
            }
            k++;
         }
         if (! found) {
            // errors previously reported
            fail=true;
         }
         j++;
      }
      i++;
   }

   // additional Mode alignment
   i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->align_current_paths(&indent,&pathList,check_closed_loop)) fail=true;
      i++;
   }

   // subdivide the paths to eliminate partial overlaps
   subdivide_paths();

   if (fail) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2193: Failed to load mode definitions.\n",indent.c_str(),indent.c_str());}

   return fail;
}

void BoundaryDatabase::print()
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s %s\n",version_name.c_str(),version_value.c_str());

   long unsigned int i=0;
   while (i < sourceFileList.size()) {
      sourceFileList[i]->print();
      i++;
   }

   i=0;
   while (i < pathList.size()) {
      pathList[i]->print("");
      i++;
   }

   i=0;
   while (i < boundaryList.size()) {
      boundaryList[i]->print();
      i++;
   }
}

bool BoundaryDatabase::inBlocks(int lineNumber)
{
   long unsigned int i=0;
   while (i < pathList.size()) {
      if (pathList[i]->inBlock(lineNumber)) return true;
      i++;
   }

   i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->inBlock(lineNumber)) return true;
      i++;
   }

   i=0;
   while (i < sourceFileList.size()) {
      if (sourceFileList[i]->inBlock(lineNumber)) return true;
      i++;
   }

   return false;
}


bool BoundaryDatabase::check ()
{
   bool fail=false;

   // Source
   if (sourceFileList.size() > 1) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2194: Only one File block is allowed.\n",indent.c_str(),indent.c_str());
      fail=true;
   }

   long unsigned int i=0;
   while (i < sourceFileList.size()) {
      if (sourceFileList[i]->check (&indent)) fail=true;
      i++;
   }

   // Path
   i=0;
   while (i < pathList.size()) {

      // individual block checks
      if (pathList[i]->check(&indent)) fail=true;

      // cross block checks

      // duplicated names
      long unsigned int j=i+1;
      while (pathList.size() > 0 && i < pathList.size()-1 && j < pathList.size()) {
         if (pathList[i]->name_is_loaded() && pathList[j]->name_is_loaded()) {
            if (pathList[i]->get_name().compare(pathList[j]->get_name()) == 0) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2195: name at line %d duplicates the name at line %d.\n",
                                            indent.c_str(),indent.c_str(),pathList[j]->get_name_lineNumber(),pathList[i]->get_name_lineNumber());
               fail=true;
            }
         }
         j++;
      }

      i++;
   }

   // Boundary
   i=0;
   while (i < boundaryList.size()) {

      // individual block checks
      if (boundaryList[i]->check(&indent)) fail=true;

      // cross block checks

      if (boundaryList[i]->is_boundary()) {
         // duplicated names
         long unsigned int j=i+1;
         while (boundaryList.size() > 0 && i < boundaryList.size()-1 && j < boundaryList.size()) {
            if (boundaryList[i]->name_is_loaded() && boundaryList[j]->name_is_loaded()) {
               if (boundaryList[i]->get_name().compare(boundaryList[j]->get_name()) == 0) {
                  prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2196: name at line %d duplicates the name at line %d.\n",
                                               indent.c_str(),indent.c_str(),boundaryList[j]->get_name_lineNumber(),boundaryList[i]->get_name_lineNumber());
                  fail=true;
               }
            }
            j++;
         }
      }

      // paths exist
      long unsigned int j=0;
      while (j < boundaryList[i]->get_pathNameList_size()) {
         bool found=false;
         long unsigned int k=0;
         while (k < pathList.size()) {
            if (boundaryList[i]->get_pathName(j).compare(pathList[k]->get_name()) == 0) {found=true; break;}
            k++;
         }
         if (! found) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2197: path at line %d is not defined by a Path block.\n",
                                                   indent.c_str(),indent.c_str(),boundaryList[i]->get_pathName_lineNumber(j));
            fail=true;
         }

         j++;
      }

      // paths are not duplicated
      j=0;
      while (boundaryList[i]->get_pathNameList_size() < 0 && j < boundaryList[i]->get_pathNameList_size()-1) {
         long unsigned int k=j+1;
         while (k < boundaryList[i]->get_pathNameList_size()) {
            if (boundaryList[i]->get_pathName(j).compare(boundaryList[i]->get_pathName(k)) == 0) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2198: path at line %d duplicates the path at line %d.\n",
                                            indent.c_str(),indent.c_str(),boundaryList[i]->get_pathName_lineNumber(k),boundaryList[i]->get_pathName_lineNumber(j));
               fail=true;
            }
            k++;
         }
         j++; 
      }

      i++;
   }

   // only one voltage and one current per mode
   int highestModeNumber=0;
   i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->is_mode() && boundaryList[i]->get_mode() > highestModeNumber) highestModeNumber=boundaryList[i]->get_mode();
      i++;
   }

   int modeNumber=1;
   while (modeNumber <= highestModeNumber) {
      bool foundVoltage=false;
      bool foundCurrent=false;
      long unsigned int i=0;
      while (i < boundaryList.size()) {
         if (boundaryList[i]->is_mode() && boundaryList[i]->get_mode() == modeNumber) {
            if (boundaryList[i]->is_mode_voltage()) {
               if (!foundVoltage) foundVoltage=true;
               else {
                  prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2199: %s block at line %d makes an extraneous voltage definition for mode %d.\n",
                                               indent.c_str(),indent.c_str(),boundaryList[i]->get_block_type().c_str(),boundaryList[i]->get_startLine(),boundaryList[i]->get_mode());
                  fail=true;
               }
            }

            if (boundaryList[i]->is_mode_current()) {
               if (!foundCurrent) foundCurrent=true;
               else {
                  prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2200: %s block at line %d makes an extraneous current definition for mode %d.\n",
                                               indent.c_str(),indent.c_str(),boundaryList[i]->get_block_type().c_str(),boundaryList[i]->get_startLine(),boundaryList[i]->get_mode());
                  fail=true;
               }
            }
         }
         i++;
      }
      modeNumber++;
   }

   // mode numbering
   // check voltage and current separately
   vector<int> modeNumbers_voltage;
   vector<int> modeNumbers_current;
   i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->is_mode_voltage()) modeNumbers_voltage.push_back(0);
      if (boundaryList[i]->is_mode_current()) modeNumbers_current.push_back(0);
      i++;    
   }
   modeNumbers_voltage.push_back(0);  // one more since the mode numbers are not zero based
   modeNumbers_current.push_back(0);  // one more since the mode numbers are not zero based

   i=0;
   while (i < boundaryList.size()) {

      // voltage
      if (boundaryList[i]->is_mode_voltage() && boundaryList[i]->mode_is_loaded()) {
         if (boundaryList[i]->get_mode() < (int)modeNumbers_voltage.size()) {
            modeNumbers_voltage[boundaryList[i]->get_mode()]++;
         } else {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2201: %s number at line number %d is too large.\n",
                                                   indent.c_str(),indent.c_str(),boundaryList[i]->get_block_type().c_str(),boundaryList[i]->get_mode_lineNumber());
            fail=true;
         }
      }

      // current
      if (boundaryList[i]->is_mode_current() && boundaryList[i]->mode_is_loaded()) {
         if (boundaryList[i]->get_mode() < (int)modeNumbers_current.size()) {
            modeNumbers_current[boundaryList[i]->get_mode()]++;
         } else {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2202: %s number at line number %d is too large.\n",
                                                   indent.c_str(),indent.c_str(),boundaryList[i]->get_block_type().c_str(),boundaryList[i]->get_mode_lineNumber());
            fail=true;
         }
      }

      i++;
   }

   // voltage
   long unsigned int lastNonZero_voltage=modeNumbers_voltage.size()-1;
   while (modeNumbers_voltage.size() > 0 && lastNonZero_voltage > 0) {
      if (modeNumbers_voltage[lastNonZero_voltage] > 0) break;
      lastNonZero_voltage--;
   }

   // current
   long unsigned int lastNonZero_current=modeNumbers_current.size()-1;
   while (modeNumbers_current.size() > 0 && lastNonZero_current > 0) {
      if (modeNumbers_current[lastNonZero_current] > 0) break;
      lastNonZero_current--;
   }   

   // voltage
   i=1;
   while (i <= lastNonZero_voltage) {
      if (modeNumbers_voltage[i] == 0) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2203: Modes/Lines are not sequentially numbered starting at 1.\n",indent.c_str(),indent.c_str());
         fail=true;
         break;
      }
      i++;
   }

   // current
   i=1;
   while (i <= lastNonZero_current) {
      if (modeNumbers_current[i] == 0) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2204: Modes/Lines are not sequentially numbered starting at 1.\n",indent.c_str(),indent.c_str());
         fail=true;
         break;
      }
      i++;
   }

   // check for extraneous text
   i=1;  // skip the first line, which is the version information
   while (i < inputs.get_size()) {
      if (! inBlocks(inputs.get_lineNumber(i))) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2205: Invalid input at line %d.\n",indent.c_str(),indent.c_str(),inputs.get_lineNumber(i));
         fail=true;
      }
      i++;
   }

   return fail;
}

// Make sure the paths fall within the bounding box
// to catch scaling errors.
bool BoundaryDatabase::check_scale (Mesh *mesh, ParMesh *pmesh, int order)
{
   bool fail=false;

   Vector lowerLeft,upperRight;
   if (mesh) mesh->GetBoundingBox(lowerLeft,upperRight,max(order,1));
   if (pmesh) pmesh->GetBoundingBox(lowerLeft,upperRight,max(order,1));

   long unsigned int i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->checkBoundingBox(&lowerLeft,&upperRight,&indent,tol,&pathList)) fail=true;
      i++;
   }

   if (fail) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"         Bounding box: (%g,%g) to (%g,%g)\n",
                                             lowerLeft[0],lowerLeft[1],upperRight[0],upperRight[1]);
   }

   return fail;
}

// remove overlaps in paths
void BoundaryDatabase::subdivide_paths ()
{
   long unsigned int i=0;
   while (i < pathList.size()) {
      long unsigned int j=0;
      while (j < pathList.size()) {
         if (i != j) pathList[i]->subdivide(pathList[j]);
         j++;
      }
      i++;
   }
}

BoundaryDatabase::~BoundaryDatabase ()
{
   long unsigned int i=0;
   while (i < sourceFileList.size()) {
      delete sourceFileList[i];
      i++;
   }

   i=0;
   while (i < boundaryList.size()) {
      delete boundaryList[i];
      i++;
   }

   i=0;
   while (i < pathList.size()) {
      delete pathList[i];
      i++;    
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Border
///////////////////////////////////////////////////////////////////////////////////////////

Border::Border()
{
   boundary.boundary=-1;
   boundary.path=-1;
   boundary.segment=-1;
}

bool Border::has_boundary()
{
   long unsigned int max=-1;
   if (boundary.boundary == max) return false;
   return true;
}

// any mode
bool Border::has_mode()
{
   if (mode.size() > 0) return true;
   return false;
}

struct EdgeAttribute Border::get_mode(long unsigned int i)
{
   struct EdgeAttribute temp;
   temp.border_element=-1;
   temp.boundary=-1;
   temp.path=-1;
   temp.segment=-1;
   temp.x1=DBL_MAX;
   temp.y1=DBL_MAX;
   temp.x2=DBL_MAX;
   temp.y2=DBL_MAX;

   if (i >= mode.size()) return temp;
   return mode[i];
}

// specified modeNumber
bool Border::has_mode(long unsigned int modeNumber)
{
   long unsigned int max=-1;

   if (mode.size() < modeNumber+1) return false;
   if (mode[modeNumber].boundary == max) return false;
   return true;
}

// for boundary
void Border::set(struct EdgeAttribute attribute)
{
   boundary=attribute;
}

// for mode 
void Border::set(struct EdgeAttribute attribute, long unsigned int modeNumber)
{
   struct EdgeAttribute temp;
   temp.border_element=-1;
   temp.boundary=-1;
   temp.path=-1;
   temp.segment=-1;
   temp.x1=DBL_MAX;
   temp.y1=DBL_MAX;
   temp.x2=DBL_MAX;
   temp.y2=DBL_MAX;

   // make sure there is enough space
   long unsigned int i=mode.size();
   while (i <= modeNumber) {
      mode.push_back(temp);
      i++;
   }

   mode[modeNumber]=attribute;
}

// for boundaries
bool Border::exists(struct EdgeAttribute test_attribute)
{
   if (boundary == test_attribute) return true;
   return false;
}

// for modes
bool Border::exists(struct EdgeAttribute test_attribute, long unsigned int modeNumber)
{
   if (! (modeNumber < mode.size())) return false;
   if (mode[modeNumber] == test_attribute) return true;
   return false;
}

// for modes
bool Border::exists_any_mode(struct EdgeAttribute test_attribute)
{
   long unsigned int i=0;
   while (i < mode.size()) {
      if (mode[i] == test_attribute) return true;
      i++;
   }
   return false;
}

// for modes
bool Border::exists_test_boundary_only(struct EdgeAttribute test_attribute)
{
   long unsigned int i=0;
   while (i < mode.size()) {
      if (mode[i].boundary == test_attribute.boundary) return true;
      i++;
   }
   return false;
}

void Border::sendToRank (PetscMPIInt rank)
{
   MPI_Send(&(boundary.border_element),1,MPI_INT,rank,2,PETSC_COMM_WORLD);
   MPI_Send(&(boundary.boundary),1,MPI_UNSIGNED_LONG,rank,3,PETSC_COMM_WORLD);
   MPI_Send(&(boundary.path),1,MPI_UNSIGNED_LONG,rank,4,PETSC_COMM_WORLD);
   MPI_Send(&(boundary.segment),1,MPI_UNSIGNED_LONG,rank,5,PETSC_COMM_WORLD);
   MPI_Send(&local_attribute,1,MPI_UNSIGNED_LONG,rank,6,PETSC_COMM_WORLD);
   MPI_Send(&global_attribute,1,MPI_UNSIGNED_LONG,rank,7,PETSC_COMM_WORLD);
   MPI_Send(&local_rank,1,MPI_INT,rank,8,PETSC_COMM_WORLD);
   MPI_Send(&(boundary.x1),1,MPI_DOUBLE,rank,9,PETSC_COMM_WORLD);
   MPI_Send(&(boundary.y1),1,MPI_DOUBLE,rank,10,PETSC_COMM_WORLD);
   MPI_Send(&(boundary.x2),1,MPI_DOUBLE,rank,11,PETSC_COMM_WORLD);
   MPI_Send(&(boundary.y2),1,MPI_DOUBLE,rank,12,PETSC_COMM_WORLD);

   long unsigned int mode_size=mode.size();
   MPI_Send(&mode_size,1,MPI_UNSIGNED_LONG,rank,13,PETSC_COMM_WORLD);

   long unsigned int i=0;
   while (i < mode_size) {
      MPI_Send(&(mode[i].border_element),1,MPI_INT,rank,14,PETSC_COMM_WORLD);
      MPI_Send(&(mode[i].boundary),1,MPI_UNSIGNED_LONG,rank,15,PETSC_COMM_WORLD);
      MPI_Send(&(mode[i].path),1,MPI_UNSIGNED_LONG,rank,16,PETSC_COMM_WORLD);
      MPI_Send(&(mode[i].segment),1,MPI_UNSIGNED_LONG,rank,17,PETSC_COMM_WORLD);
      MPI_Send(&(mode[i].x1),1,MPI_DOUBLE,rank,18,PETSC_COMM_WORLD);
      MPI_Send(&(mode[i].y1),1,MPI_DOUBLE,rank,19,PETSC_COMM_WORLD);
      MPI_Send(&(mode[i].x2),1,MPI_DOUBLE,rank,20,PETSC_COMM_WORLD);
      MPI_Send(&(mode[i].y2),1,MPI_DOUBLE,rank,21,PETSC_COMM_WORLD);
      i++;
   }
}

void Border::receiveFromRank (PetscMPIInt rank)
{
   MPI_Recv(&(boundary.border_element),1,MPI_INT,rank,2,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&(boundary.boundary),1,MPI_UNSIGNED_LONG,rank,3,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&(boundary.path),1,MPI_UNSIGNED_LONG,rank,4,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&(boundary.segment),1,MPI_UNSIGNED_LONG,rank,5,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&local_attribute,1,MPI_UNSIGNED_LONG,rank,6,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&global_attribute,1,MPI_UNSIGNED_LONG,rank,7,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&local_rank,1,MPI_INT,rank,8,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&(boundary.x1),1,MPI_DOUBLE,rank,9,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&(boundary.y1),1,MPI_DOUBLE,rank,10,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&(boundary.x2),1,MPI_DOUBLE,rank,11,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&(boundary.y2),1,MPI_DOUBLE,rank,12,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

   long unsigned int mode_size;
   MPI_Recv(&mode_size,1,MPI_UNSIGNED_LONG,rank,13,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

   long unsigned int i=0;
   while (i < mode_size) {
      struct EdgeAttribute new_mode;
      MPI_Recv(&(new_mode.border_element),1,MPI_INT,rank,14,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&(new_mode.boundary),1,MPI_UNSIGNED_LONG,rank,15,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&(new_mode.path),1,MPI_UNSIGNED_LONG,rank,16,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&(new_mode.segment),1,MPI_UNSIGNED_LONG,rank,17,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&(new_mode.x1),1,MPI_DOUBLE,rank,18,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&(new_mode.y1),1,MPI_DOUBLE,rank,19,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&(new_mode.x2),1,MPI_DOUBLE,rank,20,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&(new_mode.y2),1,MPI_DOUBLE,rank,21,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      mode.push_back(new_mode);
      i++;
   }
}

void Border::print()
{
   cout << "Border=" << this << endl;

   cout << "   local_attribute=" << local_attribute 
        << "   global_attribute=" << global_attribute
        << "   local_rank=" << local_rank << endl;

   cout << "   boundary.border_element=" << boundary.border_element
        << "   boundary.boundary=" << boundary.boundary 
        << "   boundary.path=" << boundary.path
        << "   boundary.segment=" << boundary.segment 
        << "   boundary.x1=" << boundary.x1
        << "   boundary.y1=" << boundary.y1
        << "   boundary.x2=" << boundary.x2
        << "   boundary.y2=" << boundary.y2 << endl;


   long unsigned int i=0;
   while (i < mode.size()) {
      cout << "   mode[" << i << "].border_element=" << mode[i].border_element
           << "   mode[" << i << "].boundary=" << mode[i].boundary
           << "   mode[" << i << "].path=" << mode[i].path
           << "   mode[" << i << "].segment=" << mode[i].segment
           << "   mode[" << i << "].x1=" << mode[i].x1
           << "   mode[" << i << "].y1=" << mode[i].y1
           << "   mode[" << i << "].x2=" << mode[i].x2
           << "   mode[" << i << "].y2=" << mode[i].y2 << endl;

      i++;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
// BorderDatabase
///////////////////////////////////////////////////////////////////////////////////////////

BorderDatabase::BorderDatabase()
{
   // MFEM does not support border attributes of 0 on the mesh, so create a dummy entry for the 0 slot.
   // This dummy border is never accessed.
   Border *border0=new Border;
   borderList.push_back(border0);

   // The attribute of 1 for the mesh border attributes indicates the default PEC boundary condition,
   // so create another dummy entry for the 1 slot that points to nothing.  That will yield the PEC boundary.
   Border *border1=new Border;
   borderList.push_back(border1);
}

// for boundaries
long unsigned int BorderDatabase::exists(struct EdgeAttribute test_attribute)
{
   long unsigned int i=0;
   while (i < borderList.size()) {
      if (borderList[i]->exists(test_attribute)) return i;
      i++;
   }
   return -1;
}

// for modes
long unsigned int BorderDatabase::exists(struct EdgeAttribute test_attribute, long unsigned int modeNumber)
{
   long unsigned int i=0;
   while (i < borderList.size()) {
      if (borderList[i]->exists(test_attribute,modeNumber)) return i;
      i++;
   }
   return -1;
}

// for modes
long unsigned int BorderDatabase::exists_any_mode(struct EdgeAttribute test_attribute)
{
   long unsigned int i=0;
   while (i < borderList.size()) {
      if (borderList[i]->exists_any_mode(test_attribute)) return i;
      i++;
   }
   return -1;
}

// for modes
long unsigned int BorderDatabase::exists_test_boundary_only(struct EdgeAttribute test_attribute)
{
   long unsigned int i=0;
   while (i < borderList.size()) {
      if (borderList[i]->exists_test_boundary_only(test_attribute)) return i;
      i++;
   }
   return -1;
}

/* current
// for boundaries
void BorderDatabase::add(struct EdgeAttribute edgeAttribute, int existing_bdr_attribute)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   long unsigned int max=-1;
   long unsigned int border_index=-1;

   if (existing_bdr_attribute == 1) {
      // see if it exists already
      border_index=exists(edgeAttribute);
      if (border_index == max) {
         // does not exist
         // create a new slot
         Border *border=new Border;
         border->set(edgeAttribute);
         borderList.push_back(border);
         border->set_local_attribute(borderList.size()-1);
         border->set_global_attribute(borderList.size()-1);
         border->set_local_rank(rank);
      } else {
         // do nothing - already here
      }
   } else {
      // attribute already assigned
      // save here
      borderList[existing_bdr_attribute]->set(edgeAttribute);
      borderList[existing_bdr_attribute]->set_local_attribute(existing_bdr_attribute);
      borderList[existing_bdr_attribute]->set_global_attribute(existing_bdr_attribute);
   }
}
*/

// prior modified for new data
long unsigned int BorderDatabase::add(struct EdgeAttribute edgeAttribute, int existing_bdr_attribute)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   long unsigned int max=-1;
   long unsigned int border_index=-1;

   if (existing_bdr_attribute == 1) {
      // see if it exists already
      border_index=exists(edgeAttribute);
      if (border_index == max) {
         // does not exist
         // create a new slot
         Border *border=new Border;
         border->set(edgeAttribute);
         borderList.push_back(border);
         border_index=borderList.size()-1;
         border->set_local_attribute(border_index);
         border->set_global_attribute(border_index);
         border->set_local_rank(rank);
      } else {
         // do nothing - already here
      }
   } else {
      // attribute already assigned
      // save here
      border_index=existing_bdr_attribute;
      borderList[existing_bdr_attribute]->set(edgeAttribute);
      borderList[existing_bdr_attribute]->set_local_attribute(existing_bdr_attribute);
      borderList[existing_bdr_attribute]->set_global_attribute(existing_bdr_attribute);
   }
   return border_index;
}

/* current
// for modes
void BorderDatabase::add(struct EdgeAttribute edgeAttribute, long unsigned int modeNumber, int existing_bdr_attribute)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   long unsigned int max=-1;
   long unsigned int border_index=-1;

   if (existing_bdr_attribute == 1) {
      // see if this case exists already
      border_index=exists(edgeAttribute,modeNumber);
      if (border_index == max) {
         // does not exist
         // see if it exists for other mode numbers
         border_index=exists_any_mode(edgeAttribute);
         if (border_index == max) {
            // does not exist
            // create a new slot
            Border *border=new Border;
            border->set(edgeAttribute,modeNumber);
            borderList.push_back(border);
            border->set_local_attribute(borderList.size()-1);
            border->set_global_attribute(borderList.size()-1);
            border->set_local_rank(rank);
         } else {
            // exists with other mode numbers
            // save here with new mode number
            borderList[existing_bdr_attribute]->set(edgeAttribute,modeNumber);
            borderList[existing_bdr_attribute]->set_local_attribute(existing_bdr_attribute);
            borderList[existing_bdr_attribute]->set_global_attribute(existing_bdr_attribute);
         }
      } else {
         // exists - do nothing
      }
   } else {
      // attribute already assigned

      // see if the mode space is empty
      if (borderList[existing_bdr_attribute]->has_mode()) {
         // something is here
         // see if this case already exists
         if (borderList[existing_bdr_attribute]->exists(edgeAttribute,modeNumber)) {
            // exists - do nothing
         } else {
            // save here with new mode
            borderList[existing_bdr_attribute]->set(edgeAttribute,modeNumber);
            borderList[existing_bdr_attribute]->set_local_attribute(existing_bdr_attribute);
            borderList[existing_bdr_attribute]->set_global_attribute(existing_bdr_attribute);
         }
      } else {
         // empty
         // save here
         borderList[existing_bdr_attribute]->set(edgeAttribute,modeNumber);
         borderList[existing_bdr_attribute]->set_local_attribute(existing_bdr_attribute);
         borderList[existing_bdr_attribute]->set_global_attribute(existing_bdr_attribute);
      }
   }
}
*/

// prior modified for new data
// for modes
long unsigned int BorderDatabase::add(struct EdgeAttribute edgeAttribute, long unsigned int modeNumber, int existing_bdr_attribute)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   long unsigned int max=-1;
   long unsigned int border_index=-1;

   if (existing_bdr_attribute == 1) {
      // see if this case exists already
      border_index=exists(edgeAttribute,modeNumber);
      if (border_index == max) {
         // does not exist
         // see if it exists for other mode numbers
         border_index=exists_any_mode(edgeAttribute);
         if (border_index == max) {
            // does not exist
            // create a new slot
            Border *border=new Border;
            border->set(edgeAttribute,modeNumber);
            borderList.push_back(border);
            border_index=borderList.size()-1;
            border->set_local_attribute(border_index);
            border->set_global_attribute(border_index);
            border->set_local_rank(rank);
         } else {
            // exists with other mode numbers
            // save here with new mode number
            border_index=existing_bdr_attribute;
            borderList[existing_bdr_attribute]->set(edgeAttribute,modeNumber);
            borderList[existing_bdr_attribute]->set_local_attribute(existing_bdr_attribute);
            borderList[existing_bdr_attribute]->set_global_attribute(existing_bdr_attribute);
         }
      } else {
         // exists - do nothing
      }
   } else {
      // attribute already assigned

      // see if the mode space is empty
      if (borderList[existing_bdr_attribute]->has_mode()) {
         // something is here
         // see if this case already exists
         if (borderList[existing_bdr_attribute]->exists(edgeAttribute,modeNumber)) {
            // exists - do nothing
            border_index=existing_bdr_attribute;
         } else {
            // save here with new mode
            border_index=existing_bdr_attribute;
            borderList[existing_bdr_attribute]->set(edgeAttribute,modeNumber);
            borderList[existing_bdr_attribute]->set_local_attribute(existing_bdr_attribute);
            borderList[existing_bdr_attribute]->set_global_attribute(existing_bdr_attribute);
         }
      } else {
         // empty
         // save here
         border_index=existing_bdr_attribute;
         borderList[existing_bdr_attribute]->set(edgeAttribute,modeNumber);
         borderList[existing_bdr_attribute]->set_local_attribute(existing_bdr_attribute);
         borderList[existing_bdr_attribute]->set_global_attribute(existing_bdr_attribute);
      }
   }
   return border_index;
}


// return an Array that includes the borderDatabase entries that reference a given boundary
Array<int>* BorderDatabase::build_entry_list (bool is_boundary, long unsigned int test_index, int modeNumber)
{
   Array<int> *entry_list=new Array<int>;

   long unsigned int i=0;
   while (i < borderList.size()) {

      if (is_boundary && borderList[i]->has_boundary() && borderList[i]->get_boundary().boundary == test_index) {
         entry_list->Append(i);
      }

      if (! is_boundary && borderList[i]->has_mode() && borderList[i]->get_mode(modeNumber).boundary == test_index) {
         entry_list->Append(i);
      }
      
      i++;
   }

   return entry_list;
}

// convert the local border database to global
void BorderDatabase::merge ()
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // send to rank 0

   if (rank == 0) {
      int i=1;
      while (i < size) {
         long unsigned int border_size;
         MPI_Recv(&border_size,1,MPI_UNSIGNED_LONG,i,1,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         long unsigned int j=2;
         while (j < border_size) {
            Border *border=new Border();
            border->receiveFromRank (i);
            borderList.push_back(border);
            j++;
         }
         i++;
      }
   } else {
      long unsigned int border_size=borderList.size();
      MPI_Send(&border_size,1,MPI_UNSIGNED_LONG,0,1,PETSC_COMM_WORLD);

      long unsigned int i=2;  // 0 already has the required first two entries (see the constructor)
      while (i < border_size) {
         borderList[i]->sendToRank(0);
         i++;
      }
   }

   // set the global attributes
   if (rank == 0) {
      long unsigned int i=0;
      while (i < borderList.size()) {
         borderList[i]->set_global_attribute(i);
         i++;
      }
   }

   // broadcast

   if (rank == 0) {

      long unsigned int i=1;
      while (i < (long unsigned int)size) {
         long unsigned int border_size=borderList.size();
         MPI_Send(&border_size,1,MPI_UNSIGNED_LONG,i,1,PETSC_COMM_WORLD);

         long unsigned int j=0;
         while (j < borderList.size()) {
            borderList[j]->sendToRank(i);
            j++;
         }

         i++;
      }

   } else {

      // remove the existing data

      long unsigned int i=0;
      while (i < borderList.size()) {
         if (borderList[i]) delete borderList[i];
         i++;
      }
      borderList.clear();

      // refill with data from 0

      long unsigned int border_size;
      MPI_Recv(&border_size,1,MPI_UNSIGNED_LONG,0,1,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      i=0;
      while (i < border_size) {
         Border *border=new Border;
         border->receiveFromRank (0);
         borderList.push_back(border);
         i++;
      }
   }
}

void BorderDatabase::reassign_mesh_attributes (ParMesh *pmesh)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   int i=0;
   while (i < pmesh->GetNBE()) {
      long unsigned int j=0;
      while (j < borderList.size()) {
         if (rank == borderList[j]->get_local_rank()) {
            if (pmesh->GetBdrAttribute(i) == (int)borderList[j]->get_local_attribute()) {
               pmesh->SetBdrAttribute(i,borderList[j]->get_global_attribute());
               break;
            }
         }
         j++;
      }
      i++;
   }
}

void BorderDatabase::print()
{
   long unsigned int i=0;
   while (i < borderList.size()) {
      borderList[i]->print();
      i++;
   }
}

BorderDatabase::~BorderDatabase()
{
   long unsigned int i=0;
   while (i < borderList.size()) {
      delete borderList[i];
      i++;
   }
}


