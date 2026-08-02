////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    builder - A geometry builder for OpenParEM2D and OpenParEM3D            //
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

#include "builder.hpp"

///////////////////////////////////////////////////////////////////////////////////////////
// Control
///////////////////////////////////////////////////////////////////////////////////////////

Control::Control (int startLine_, int endLine_, bool checkLimits_)
{
   startLine=startLine_;
   endLine=endLine_;

   // build

   build.push_alias("build");
   build.set_loaded(false);
   build.set_positive_required(false);
   build.set_non_negative_required(false);
   build.set_lowerLimit(0);
   build.set_upperLimit(1e12);
   build.set_checkLimits(checkLimits_);

   // checkLimits

   checkLimits.push_alias("check_limits");
   checkLimits.set_loaded(false);
   checkLimits.set_positive_required(false);
   checkLimits.set_non_negative_required(false);
   checkLimits.set_lowerLimit(0);
   checkLimits.set_upperLimit(1e12);
   checkLimits.set_checkLimits(checkLimits_);
}

void Control::print(string indent)
{
   PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%sControl this=%p\n",startLine,indent.c_str(),indent.c_str(),this);

   if (build.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=%s\n",
                                  build.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),build.get_keyword().c_str(),build.get_value().c_str());
   }

   if (checkLimits.is_loaded()) {
      if (checkLimits.get_bool_value()) {
         PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=true\n",
                                     checkLimits.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),checkLimits.get_keyword().c_str());
      } else {
         PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=false\n",
                                     checkLimits.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),checkLimits.get_keyword().c_str());
      }
   }

   PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%sEndControl\n",endLine,indent.c_str(),indent.c_str());
}

void Control::print_commented(ofstream *out)
{
   *out << "// Control" << endl;
   *out << "//   build=" << build.get_value() << endl;
   if (checkLimits.get_bool_value()) *out << "//   check_limits=true" << endl;
   else *out << "//   check_limits=false" << endl;
   *out << "// EndControl" << endl;
}

bool Control::load (string *indent, inputFile *inputs, bool checkInputs)
{
   bool fail=false;

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {
      string token,value,line;
      line=inputs->get_line(lineNumber);
      get_token_pair(&line,&token,&value,&lineNumber,indent->c_str());

      int recognized=0;
      if (build.match_alias(&token)) {
         if (build.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2000: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,build.get_lineNumber());
            fail=true;
         } else {
            build.set_keyword(token);
            build.set_value(value);
            build.set_lineNumber(lineNumber);
            build.set_loaded(true);
         }
         recognized++;
      }

      if (checkLimits.match_alias(&token)) {
         recognized++;
         if (checkLimits.loadBool(&token, &value, lineNumber)) fail=true;
      }

      // should recognize one keyword
      if (recognized != 1) {
         PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2001: Unrecognized keyword at line %d.\n",indent->c_str(),indent->c_str(),lineNumber);
         fail=true;
      }

      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }
   return fail;
}

bool Control::inBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

bool Control::check (string indent)
{
   bool fail=false;

   if (!build.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2002: RectangularWaveguide block at line %d must specify a build.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!checkLimits.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2003: RectangularWaveguide block at line %d must specify whether to check limits.\n",indent.c_str(),startLine);
      fail=true;
   }

   return fail;
}

///////////////////////////////////////////////////////////////////////////////////////////
// RectangularWaveguide
///////////////////////////////////////////////////////////////////////////////////////////

RectangularWaveguide::RectangularWaveguide (int startLine_, int endLine_, bool checkLimits_)
{
   startLine=startLine_;
   endLine=endLine_;

   // name

   name.push_alias("name");
   name.set_loaded(false);
   name.set_positive_required(false);
   name.set_non_negative_required(false);
   name.set_lowerLimit(0);
   name.set_upperLimit(1e12);
   name.set_checkLimits(checkLimits_);

   // include

   include.push_alias("include");
   include.set_loaded(false);
   include.set_positive_required(false);
   include.set_non_negative_required(false);
   include.set_lowerLimit(0);
   include.set_upperLimit(1e12);
   include.set_checkLimits(checkLimits_);

   // width 

   width.push_alias("width");
   width.set_loaded(false);
   width.set_positive_required(true);
   width.set_non_negative_required(false);
   width.set_lowerLimit(1e-6);
   width.set_upperLimit(0.1);
   width.set_checkLimits(checkLimits_);

   // height 

   height.push_alias("height");
   height.set_loaded(false);
   height.set_positive_required(true);
   height.set_non_negative_required(false);
   height.set_lowerLimit(1e-6);
   height.set_upperLimit(0.1);
   height.set_checkLimits(checkLimits_);

   // material

   material.push_alias("material");
   material.set_loaded(false);
   material.set_positive_required(false);
   material.set_non_negative_required(false);
   material.set_lowerLimit(0);
   material.set_upperLimit(1e12);
   material.set_checkLimits(checkLimits_);

   // default_conductor_material

   default_conductor_material.push_alias("default_conductor_material");
   default_conductor_material.set_loaded(false);
   default_conductor_material.set_positive_required(false);
   default_conductor_material.set_non_negative_required(false);
   default_conductor_material.set_lowerLimit(0);
   default_conductor_material.set_upperLimit(1e12);
   default_conductor_material.set_checkLimits(checkLimits_);

   // conductor_conductor_material_top

   conductor_material_top.push_alias("conductor_material_top");
   conductor_material_top.set_loaded(false);
   conductor_material_top.set_positive_required(false);
   conductor_material_top.set_non_negative_required(false);
   conductor_material_top.set_lowerLimit(0);
   conductor_material_top.set_upperLimit(1e12);
   conductor_material_top.set_checkLimits(checkLimits_);

   // conductor_material_bottom

   conductor_material_bottom.push_alias("conductor_material_bottom");
   conductor_material_bottom.set_loaded(false);
   conductor_material_bottom.set_positive_required(false);
   conductor_material_bottom.set_non_negative_required(false);
   conductor_material_bottom.set_lowerLimit(0);
   conductor_material_bottom.set_upperLimit(1e12);
   conductor_material_bottom.set_checkLimits(checkLimits_);

   // conductor_material_left

   conductor_material_left.push_alias("conductor_material_left");
   conductor_material_left.set_loaded(false);
   conductor_material_left.set_positive_required(false);
   conductor_material_left.set_non_negative_required(false);
   conductor_material_left.set_lowerLimit(0);
   conductor_material_left.set_upperLimit(1e12);
   conductor_material_left.set_checkLimits(checkLimits_);

   // conductor_material_right

   conductor_material_right.push_alias("conductor_material_right");
   conductor_material_right.set_loaded(false);
   conductor_material_right.set_positive_required(false);
   conductor_material_right.set_non_negative_required(false);
   conductor_material_right.set_lowerLimit(0);
   conductor_material_right.set_upperLimit(1e12);
   conductor_material_right.set_checkLimits(checkLimits_);

   // length

   length.push_alias("length");
   length.set_loaded(false);
   length.set_positive_required(false);
   length.set_non_negative_required(true);
   length.set_lowerLimit(0);
   length.set_upperLimit(1);
   length.set_checkLimits(checkLimits_);

   included=nullptr;

   // defaults
   length.set_dbl_value(0);
}

void RectangularWaveguide::print(string indent)
{
   PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%sRectangularWaveguide this=%p\n",startLine,indent.c_str(),indent.c_str(),this);

   if (name.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=%s\n",
                                   name.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),
                                   name.get_keyword().c_str(),name.get_value().c_str());
   }

   if (include.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=%s\n",
                                   include.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),
                                   include.get_keyword().c_str(),include.get_value().c_str());
   }

   if (width.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=%g\n",
                                   width.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),
                                   width.get_keyword().c_str(),width.get_dbl_value());
   }

   if (height.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=%g\n",
                                   height.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),
                                   height.get_keyword().c_str(),height.get_dbl_value());
   }

   if (material.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=%s\n",
                                   material.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),
                                   material.get_keyword().c_str(),material.get_value().c_str());
   }



   if (conductor_material_top.is_loaded() && conductor_material_bottom.is_loaded() && conductor_material_left.is_loaded() && conductor_material_right.is_loaded()) {
      // no need to print
   } else {
      if (default_conductor_material.is_loaded()) {
         PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=%s\n",
                                      default_conductor_material.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),
                                      default_conductor_material.get_keyword().c_str(),default_conductor_material.get_value().c_str());
      }
   }

   if (conductor_material_top.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=%s\n",
                                   conductor_material_top.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),
                                   conductor_material_top.get_keyword().c_str(),conductor_material_top.get_value().c_str());
   }

   if (conductor_material_bottom.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=%s\n",
                                   conductor_material_bottom.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),
                                   conductor_material_bottom.get_keyword().c_str(),conductor_material_bottom.get_value().c_str());
   }

   if (conductor_material_left.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=%s\n",
                                   conductor_material_left.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),
                                   conductor_material_left.get_keyword().c_str(),conductor_material_left.get_value().c_str());
   }

   if (conductor_material_right.is_loaded()) { 
      PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%s%s%s=%s\n",
                                   conductor_material_right.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),
                                   conductor_material_right.get_keyword().c_str(),conductor_material_right.get_value().c_str());
   }

   if (included != nullptr) {
      PetscPrintf(PETSC_COMM_WORLD,"      %s%s%sincluded=%p\n",
                                  indent.c_str(),indent.c_str(),indent.c_str(),included);
   }

   PetscPrintf(PETSC_COMM_WORLD,"%4d: %s%sEndRectangularWaveguide\n",endLine,indent.c_str(),indent.c_str());
}

void RectangularWaveguide::print_commented(ofstream *out)
{
   *out << "// RectangularWaveguide" << endl;
   *out << "//    name=" << name.get_value() << endl;
   if (include.is_loaded()) *out << "//    include=" << include.get_value() << endl;
   *out << "//    width=" << width.get_dbl_value() << endl;
   *out << "//    height=" << height.get_dbl_value() << endl;
   *out << "//    material=" << material.get_value() << endl;

   if (conductor_material_top.is_loaded() && conductor_material_bottom.is_loaded() && conductor_material_left.is_loaded() && conductor_material_right.is_loaded()) {
      // no need to print
   } else {
      *out << "//    default_conductor_material=" << default_conductor_material.get_value() << endl;
   }

   if (conductor_material_top.is_loaded()) *out << "//    conductor_material_top=" << conductor_material_top.get_value() << endl;
   if (conductor_material_bottom.is_loaded()) *out << "//    conductor_material_bottom=" << conductor_material_bottom.get_value() << endl;
   if (conductor_material_left.is_loaded()) *out << "//    conductor_material_left=" << conductor_material_left.get_value() << endl;
   if (conductor_material_right.is_loaded()) *out << "//    conductor_material_right=" << conductor_material_right.get_value() << endl;
   if (length.is_loaded()) *out << "//    length=" << length.get_dbl_value() << endl;

   *out << "// EndRectangularWaveguide" << endl;
}

bool RectangularWaveguide::load (string *indent, inputFile *inputs, bool checkInputs)
{
   bool fail=false;

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {
      string token,value,line;
      line=inputs->get_line(lineNumber);
      get_token_pair(&line,&token,&value,&lineNumber,indent->c_str());

      int recognized=0;
      if (name.match_alias(&token)) {
         if (name.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2004: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,name.get_lineNumber());
            fail=true;
         } else {
            name.set_keyword(token);
            name.set_value(value);
            name.set_lineNumber(lineNumber);
            name.set_loaded(true);
         }
         recognized++;
      }

      if (include.match_alias(&token)) {
         if (include.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2005: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,include.get_lineNumber());
            fail=true;
         } else {
            include.set_keyword(token);
            include.set_value(value);
            include.set_lineNumber(lineNumber);
            include.set_loaded(true);
         }
         recognized++;
      }

      if (width.match_alias(&token)) {
         recognized++;
         if (width.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (height.match_alias(&token)) {
         recognized++;
         if (height.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (material.match_alias(&token)) {
         if (material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2006: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,material.get_lineNumber());
            fail=true;
         } else {
            material.set_keyword(token);
            material.set_value(value);
            material.set_lineNumber(lineNumber);
            material.set_loaded(true);
         }
         recognized++;
      }

      if (default_conductor_material.match_alias(&token)) {
         if (default_conductor_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2007: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,default_conductor_material.get_lineNumber());
            fail=true;
         } else {
            default_conductor_material.set_keyword(token);
            default_conductor_material.set_value(value);
            default_conductor_material.set_lineNumber(lineNumber);
            default_conductor_material.set_loaded(true);
         }
         recognized++;
      }

      if (conductor_material_top.match_alias(&token)) {
         if (conductor_material_top.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2008: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,conductor_material_top.get_lineNumber());
            fail=true;
         } else {
            conductor_material_top.set_keyword(token);
            conductor_material_top.set_value(value);
            conductor_material_top.set_lineNumber(lineNumber);
            conductor_material_top.set_loaded(true);
         }
         recognized++;
      }

      if (conductor_material_bottom.match_alias(&token)) {
         if (conductor_material_bottom.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2009: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,conductor_material_bottom.get_lineNumber());
            fail=true;
         } else {
            conductor_material_bottom.set_keyword(token);
            conductor_material_bottom.set_value(value);
            conductor_material_bottom.set_lineNumber(lineNumber);
            conductor_material_bottom.set_loaded(true);
         }
         recognized++;
      }

      if (conductor_material_left.match_alias(&token)) {
         if (conductor_material_left.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2010: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,conductor_material_left.get_lineNumber());
            fail=true;
         } else {
            conductor_material_left.set_keyword(token);
            conductor_material_left.set_value(value);
            conductor_material_left.set_lineNumber(lineNumber);
            conductor_material_left.set_loaded(true);
         }
         recognized++;
      }

      if (conductor_material_right.match_alias(&token)) {
         if (conductor_material_right.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2011: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,conductor_material_right.get_lineNumber());
            fail=true;
         } else {
            conductor_material_right.set_keyword(token);
            conductor_material_right.set_value(value);
            conductor_material_right.set_lineNumber(lineNumber);
            conductor_material_right.set_loaded(true);
         }
         recognized++;
      }

      if (length.match_alias(&token)) {
         recognized++;
         if (length.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      // should recognize one keyword
      if (recognized != 1) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2012: Unrecognized keyword at line %d.\n",indent->c_str(),lineNumber);
         fail=true;
      }

      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }
   return fail;
}

bool RectangularWaveguide::inBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

void RectangularWaveguide::apply_include ()
{
   if (included == nullptr) return;

   if (included_type == 0) {
      RectangularWaveguide *rw=(RectangularWaveguide *)included;
      if (! width.is_loaded()) if (rw->width.is_loaded()) width.copy(rw->width);
      if (! height.is_loaded()) if (rw->height.is_loaded()) height.copy(rw->height);
      if (! material.is_loaded()) if (rw->material.is_loaded()) material.copy(rw->material);
      if (! default_conductor_material.is_loaded()) if (rw->default_conductor_material.is_loaded()) default_conductor_material.copy(rw->default_conductor_material);
      if (! conductor_material_top.is_loaded()) if (rw->conductor_material_top.is_loaded()) conductor_material_top.copy(rw->conductor_material_top);
      if (! conductor_material_bottom.is_loaded()) if (rw->conductor_material_bottom.is_loaded()) conductor_material_bottom.copy(rw->conductor_material_bottom);
      if (! conductor_material_left.is_loaded()) if (rw->conductor_material_left.is_loaded()) conductor_material_left.copy(rw->conductor_material_left);
      if (! conductor_material_right.is_loaded()) if (rw->conductor_material_right.is_loaded()) conductor_material_right.copy(rw->conductor_material_right);
   } else if (included_type == 1) {
      Strip *strip=(Strip *)included;
      if (! default_conductor_material.is_loaded()) if (strip->get_default_conductor_material().is_loaded()) default_conductor_material.copy(strip->get_default_conductor_material());
   } else if (included_type == 2) {
      CoupledStrip *coupledStrip=(CoupledStrip *)included;
      if (! default_conductor_material.is_loaded()) if (coupledStrip->get_default_conductor_material().is_loaded()) default_conductor_material.copy(coupledStrip->get_default_conductor_material());
   }
}

bool RectangularWaveguide::check (string indent)
{
   bool fail=false;

   if (!name.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2013: RectangularWaveguide block at line %d must specify a name.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!width.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2014: RectangularWaveguide block at line %d must specify a width.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!height.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2015: RectangularWaveguide block at line %d must specify a height.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!material.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2016: RectangularWaveguide block at line %d must specify a material.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!default_conductor_material.is_loaded()) {
      if (conductor_material_top.is_loaded() && conductor_material_bottom.is_loaded() && conductor_material_left.is_loaded() && conductor_material_right.is_loaded()) {
         // ok
      } else {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2017: RectangularWaveguide block at line %d must specify a default conductor material.\n",indent.c_str(),startLine);
         fail=true;
      }
   }

   return fail;
}

bool RectangularWaveguide::checkInclude (string indent)
{
   if (name.is_loaded() && include.is_loaded()) {
      if (name.get_value().compare(include.get_value()) == 0) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2018: RectangularWaveguide block at line %d cannot include itself.\n",indent.c_str(),startLine);
         return true;
      }
   }
   return false;
}

bool RectangularWaveguide::checkLimits()
{
   bool fail=false;

   if (width.limit_check("double")) fail=true;
   if (height.limit_check("double")) fail=true;
   if (length.limit_check("double")) fail=true;

   return fail;
}

void RectangularWaveguide::build_geo (Geo *geo)
{
   double min_dimension=min(width.get_dbl_value(),height.get_dbl_value());

   // 2D

   // create 4 corners of the box
   geo->push_point(new Point(1,-width.get_dbl_value()/2,-height.get_dbl_value()/2,min_dimension/2,true));
   geo->push_point(new Point(2,+width.get_dbl_value()/2,-height.get_dbl_value()/2,min_dimension/2,true));
   geo->push_point(new Point(3,+width.get_dbl_value()/2,+height.get_dbl_value()/2,min_dimension/2,true));
   geo->push_point(new Point(4,-width.get_dbl_value()/2,+height.get_dbl_value()/2,min_dimension/2,true));

   // connect with lines
   geo->push_line(new Line(1,geo->get_point(0),geo->get_point(1),true));
   geo->push_line(new Line(2,geo->get_point(1),geo->get_point(2),true));
   geo->push_line(new Line(3,geo->get_point(2),geo->get_point(3),true));
   geo->push_line(new Line(4,geo->get_point(3),geo->get_point(0),true));

   // create a curve
   Curve *newCurve=new Curve(1,material.get_value(),true);
   newCurve->push_line(geo->get_line(0),false);
   newCurve->push_line(geo->get_line(1),false);
   newCurve->push_line(geo->get_line(2),false);
   newCurve->push_line(geo->get_line(3),false);
   geo->push_curve(newCurve);

   // 3D
   geo->extrude(length.get_dbl_value());
}

void Point::write (ofstream *out)
{
   if (active) *out << setprecision(15) << "Point(" << index << ") = {" << x << "," << y << "," << z << "," << grid << "};" << endl;
}

bool Point::is_equal (Point *test)
{
   if (double_compare (x,test->x,1e-14) && double_compare (y,test->y,1e-14) && double_compare (z,test->z,1e-14)) return true;
   return false;
}

Point* Point::clone ()
{
   Point *newPoint=new Point(-1,x,y,z,grid,true);
   return newPoint;
}

void Point::print()
{
   cout << "Point(" << index<< "):" << endl;
   cout << "   x=" << x << endl;
   cout << "   y=" << y << endl;
   cout << "   z=" << z << endl;
   cout << "   active=" << active << endl;
   cout << "   grid=" << grid << endl;
}

void Line::write (ofstream *out)
{
   if (active) *out << "Line(" << index << ") = {" << first->get_index() << "," << second->get_index() << "};" << endl;
}

bool Line::is_overlap (Line *test)
{
   if (first->is_equal(test->first) && second->is_equal(test->second)) return true;
   if (first->is_equal(test->second) && second->is_equal(test->first)) return true;
   return false;
}

bool Line::is_touching (Line *test)
{
   if (is_overlap(test)) return false;
   if (first->is_equal(test->first)) return true;
   if (first->is_equal(test->second)) return true;
   if (second->is_equal(test->first)) return true;
   if (second->is_equal(test->second)) return true;
   return false;
}

bool Line::is_touching_first (Point *test)
{
   if (first->is_equal(test)) return true;
   return false;
}

bool Line::is_touching_second (Point *test)
{
   if (second->is_equal(test)) return true;
   return false;
}

bool Line::reverse (Line *test, bool line_is_reversed)
{
   if (line_is_reversed) {
      if (first == test->first) return false;
      if (second == test->second) return false;
   } else {
      if (first == test->second) return false;
      if (second == test->first) return false;
   }
   return true;
}

void Curve::write (ofstream *out)
{
   bool is_first=true;

   if (active) {

      *out << "Curve Loop(" << index << ") = {";

      long unsigned int i=0;
      while (i < lineList.size()) {
         if (! is_first) *out << ",";
         is_first=false;

         if (reverseList[i]) *out << -lineList[i]->get_index();
         else *out << lineList[i]->get_index();

         i++;
      }
      *out << "};" << endl;

      *out << "Plane Surface(" << index << ") = {" << index << "};" << endl;
   }
}

void Curve::print ()
{
   cout << "Curve: " << this << endl;
   cout << "   index=" << index << endl;
   cout << "   active=" << active << endl;

   long unsigned int i=0;
   while (i < lineList.size()) {
      Line *line=lineList[i];
      Point *first=line->get_first();
      Point *second=line->get_second();
      cout << "   line: " << line << "  index=" << line->get_index() << " first point: " << first << "  index=" << first->get_index() << ", second point:" << second << "  index=" << second->get_index() << endl;
      i++;
   }
}

bool Curve::is_touching (Line *test)
{
   long unsigned int i=0;
   while (i < lineList.size()) {
      if (lineList[i]->is_touching(test)) return true;
      i++;
   }
   return false;
}

void Curve::merge (Curve *test)
{
   // check trivial cases first
   if (test->get_lineList_size() == 0) return;
   if (lineList.size() == 0) {
      long unsigned int i=0;
      while (i < test->get_lineList_size()) {
         lineList.push_back(test->get_line(i));
         reverseList.push_back(test->get_reverse(i));
         i++;
      }
      return;
   }

   // merge the curves

   vector<Line *> newLineList;
   vector<bool> newReverseList;
   Line *current_line;
   Line *start_line;
   Point *current_point;

   // get the first active line as the starting "seed" line
   bool found=false;
   long unsigned int i=0;
   while (i < lineList.size()) {
      if (lineList[i]->is_active()) {
         current_line=lineList[i];
         start_line=current_line;
         newLineList.push_back(current_line);
         newReverseList.push_back(false);
         current_point=current_line->get_second();
         found=true;
         break;
      }
      i++;
   }
   if (! found) return;

   // tack on lines that touch to form a loop
   found=true;
   while (found) {
      found=false;

      long unsigned int i=0;
      while (i < lineList.size()) {
         if (lineList[i]->is_active() && current_line->get_index() != lineList[i]->get_index()) {
            if (lineList[i]->is_touching_first(current_point)) {
               current_line=lineList[i];
               newLineList.push_back(current_line);
               newReverseList.push_back(false);
               current_point=current_line->get_second();
               found=true;
               break;
            }
            if (lineList[i]->is_touching_second(current_point)) {
               current_line=lineList[i];
               newLineList.push_back(current_line);
               newReverseList.push_back(true);
               current_point=current_line->get_first();
               found=true;
               break;
            }
         }
         i++;
      }

      if (! found) {
         i=0;
         while (i < test->get_lineList_size()) {
            if (test->get_line(i)->is_active() && current_line->get_index() != test->get_line(i)->get_index()) {

               if (test->get_line(i)->is_touching_first(current_point)) {
                  current_line=test->get_line(i);
                  newLineList.push_back(current_line);
                  newReverseList.push_back(false);
                  current_point=current_line->get_second();
                  found=true;
                  break;  
               }       
               if (test->get_line(i)->is_touching_second(current_point)) {
                  current_line=test->get_line(i);
                  newLineList.push_back(current_line);
                  newReverseList.push_back(true);
                  current_point=current_line->get_first();
                  found=true;
                  break;  
               }       
            }
            i++;
         }
      }

      if (!found) cout << "ERROR2019: Failed to close loop when merging curves." << endl;

      // see if done
      if (current_line->get_index() == start_line->get_index()) {
         found=false;
      }
   }

   // transfer back
   lineList.clear();
   reverseList.clear();

   i=0;
   while (i < newLineList.size()-1) {
      lineList.push_back(newLineList[i]);
      reverseList.push_back(newReverseList[i]);
      i++;
   }
}

void Surface::write (ofstream *out)
{
   bool is_first=true;

   *out << "Surface Loop(" << index << ") = {";

   long unsigned int i=0;
   while (i < curveList.size()) {
      if (! is_first) *out << ",";
      is_first=false;

      *out << curveList[i]->get_index();
      i++;
   }
   *out << "};" << endl;

   *out << "Volume(" << index << ") = {" << index << "};" << endl;
}

Point* Geo::find_point (int index)
{
   long unsigned int i=0;
   while (i < pointList.size()) {
      if (pointList[i]->is_active() && pointList[i]->get_index() == index) return pointList[i];
      i++;
   }
   return nullptr;
}

Line* Geo::find_line (int index)
{
   long unsigned int i=0;
   while (i < lineList.size()) {
      if (lineList[i]->is_active() && lineList[i]->get_index() == index) return lineList[i];
      i++;
   }
   return nullptr;
}

// horizontal shift
void Geo::shift (double shift)
{
   long unsigned int i=0;
   while (i < pointList.size()) {
      pointList[i]->shift(shift);
      i++;
   }
}

void Geo::merge (Geo *geo)
{
   // points
   long unsigned int i=0;
   while (i < geo->get_pointList_size()) {
      pointList.push_back(geo->get_point(i));
      i++;
   }

   // lines
   i=0;
   while (i < geo->get_lineList_size()) {
      lineList.push_back(geo->get_line(i));
      i++;
   }

   // curves
   i=0;
   while (i < geo->get_curveList_size()) {
      curveList.push_back(geo->get_curve(i));
      i++;
   }

   // inactivate line overlaps
   i=0;
   while (i < lineList.size()-1) {
      if (lineList[i]->is_active()) {
         long unsigned int j=i+1;
         while (j < lineList.size()) {
            if (lineList[j]->is_active()) {
               if (lineList[i]->is_overlap(lineList[j])) {
                  lineList[i]->set_inactive();
                  lineList[j]->set_inactive();
               }
            }
            j++;
         }
      }
      i++;
   }

   // eliminate point overlaps
   i=0;
   while (i < pointList.size()-1) {
      if (pointList[i]->is_active()) {
         long unsigned int j=i+1;
         while (j < pointList.size()) {
            if (pointList[j]->is_active()) {
               if (pointList[i]->is_equal(pointList[j])) {

                  // replace the point j with point i in the lines
                  long unsigned int k=0;
                  while (k < lineList.size()) {
                     if (lineList[k]->get_first()->is_equal(pointList[i])) lineList[k]->set_first(pointList[i]);
                     if (lineList[k]->get_second()->is_equal(pointList[i])) lineList[k]->set_second(pointList[i]);
                     k++;
                  }
                  pointList[j]->set_inactive();
               }
            }
            j++;
         }
      }
      i++;
   }

   // merge the second half of the list into the first half
   i=0;
   while (i < curveList.size()/2) {
      curveList[i]->merge(curveList[i+curveList.size()/2]);
      curveList[i+curveList.size()/2]->set_inactive();
      i++;
   } 
}

void Geo::offset_indices (int offset)
{
   // points
   long unsigned int i=0;
   while (i < pointList.size()) {
      pointList[i]->offset_index(offset);
      i++;
   }

   // lines
   i=0;
   while (i < lineList.size()) {
      lineList[i]->offset_index(offset);
      i++;
   }

   // curves
   i=0;
   while (i < curveList.size()) {
      curveList[i]->offset_index(offset);
      i++;
   }
}

long unsigned int Geo::find_point (Point *point)
{
   long unsigned int max=-1;
   long unsigned int i=0;
   while (i < pointList.size()) {
      if (pointList[i]->is_active() && pointList[i]->is_equal(point)) return i;
      i++;
   }
   return max;
}

long unsigned int Geo::find_line (Line *line)
{
   long unsigned int max=-1;
   long unsigned int i=0;
   while (i < lineList.size()) {
      if (lineList[i]->is_active() && lineList[i]->get_first() == line->get_first() && lineList[i]->get_second() == line->get_second()) return i;
      if (lineList[i]->is_active() && lineList[i]->get_first() == line->get_second() && lineList[i]->get_second() == line->get_first()) return i;
      i++;
   }
   return max;
}

long unsigned int Geo::find_curve (Curve *test_curve)
{
   long unsigned int max=-1;
   long unsigned int i=0;
   while (i < curveList.size()) {
      Curve *curve=curveList[i];
      if (curve->is_active()) {

         vector<bool> match;
         long unsigned int k=0;
         while (k < test_curve->get_lineList_size()) {
            match.push_back(false);
            k++;
         }

         long unsigned int j=0;
         while (j < curve->get_lineList_size()) {
            Line *line=curve->get_line(j);

            long unsigned int k=0;
            while (k < test_curve->get_lineList_size()) {
               Line *test_line=test_curve->get_line(k);
               if (line->get_first() == test_line->get_first() && line->get_second() == test_line->get_second()) {match[k]=true; break;}
               if (line->get_first() == test_line->get_second() && line->get_second() == test_line->get_first()) {match[k]=true; break;}
               k++;
            }

            j++;
         }

         bool matchCurve=true;
         k=0;
         while (k < test_curve->get_lineList_size()) {
            if (!match[k]) matchCurve=false;
            k++;
         }

         if (matchCurve) return i;
      }
      i++;
   }
   return max;
}

int Geo::get_next_point_index ()
{
   int index=0;
   long unsigned int i=0;
   while (i < pointList.size()) {
      if (pointList[i]->is_active() && pointList[i]->get_index() > index) index=pointList[i]->get_index();
      i++;
   }
   index++;
   return index;
}

int Geo::get_next_line_index ()
{
   int index=0;
   long unsigned int i=0;
   while (i < lineList.size()) {
      if (lineList[i]->is_active() && lineList[i]->get_index() > index) index=lineList[i]->get_index();
      i++;
   }
   index++;
   return index;
}

int Geo::get_next_curve_index ()
{
   int index=0;
   long unsigned int i=0;
   while (i < curveList.size()) {
      if (curveList[i]->is_active() && curveList[i]->get_index() > index) index=curveList[i]->get_index();
      i++;
   }
   index++;
   return index;
}

int Geo::get_next_surface_index ()
{
   int index=0;
   long unsigned int i=0;
   while (i < surfaceList.size()) {
      if (surfaceList[i]->get_index() > index) index=surfaceList[i]->get_index();
      i++;
   }
   index++;
   return index;
}

void Geo::extrude (double length)
{
   if (length == 0) return;

   long unsigned int max=-1;

   // loop through each curve
   long unsigned int num2Dcurves=curveList.size();
   long unsigned int i=0;
   while (i < num2Dcurves) {
      Curve *curve=curveList[i];
      if (curve->is_active()) {

         Surface *surface=new Surface (get_next_surface_index(),curve->get_name(),true);
         surfaceList.push_back(surface);

         // add this curve for the front side
         surface->push_curve(curve);

         // create a curve for the back side
         string backside_name=curve->get_name()+"_backside";
         Curve *backsideCurve=new Curve (get_next_curve_index(),backside_name,true);
         curveList.push_back(backsideCurve);
         surface->push_curve(backsideCurve);

         // loop through each line to make curves for the sides
         long unsigned int j=0;
         while (j < curve->get_lineList_size()) {
            Line *line=curve->get_line(j);
            if (line->is_active()) {

               // create new points for depth

               Point *p1=line->get_first()->clone();
               if (!p1->is_active()) cout << "ASSERT: found inactive Point in active Line" << endl;
               p1->set_z(length);
               long unsigned int index=find_point(p1);
               if (index == max) {
                  p1->set_index(get_next_point_index());
                  pointList.push_back(p1);
               } else {
                  delete p1;
                  p1=pointList[index];
               }

               Point *p2=line->get_second()->clone();
               if (!p2->is_active()) cout << "ASSERT: found inactive Point in active Line" << endl;
               p2->set_z(length);
               index=find_point(p2);
               if (index == max) {
                  p2->set_index(get_next_point_index());
                  pointList.push_back(p2);
               } else {
                  delete p2;
                  p2=pointList[index];
               }

               // create 3 new lines to make a curve

               Line *l1=new Line(-1,line->get_first(),p1,true);
               index=find_line(l1);
               if (index == max) {
                  l1->set_index(get_next_line_index());
                  lineList.push_back(l1);
               } else {
                  delete l1;
                  l1=lineList[index];
               }

               Line *l2=new Line(-1,p1,p2,true);
               index=find_line(l2);
               if (index == max) {
                  l2->set_index(get_next_line_index());
                  lineList.push_back(l2);
               } else {
                  delete l2;
                  l2=lineList[index];
               }

               Line *l3=new Line(-1,p2,line->get_second(),true);
               index=find_line(l3);
               if (index == max) {
                  l3->set_index(get_next_line_index());
                  lineList.push_back(l3);
               } else {
                  delete l3;
                  l3=lineList[index];
               }

               // make the curve
               stringstream ss;
               ss << curve->get_name() << "_" << j;
               Curve *newCurve=new Curve(-1,ss.str(),true);
               newCurve->push_line(line,false);
               bool reverse1=line->reverse(l1,false);
               newCurve->push_line(l1,reverse1);
               bool reverse2=l1->reverse(l2,reverse1);
               newCurve->push_line(l2,reverse2);
               bool reverse3=l2->reverse(l3,reverse2);
               newCurve->push_line(l3,reverse3);

               index=find_curve(newCurve);
               if (index == max) {
                  newCurve->set_index(get_next_curve_index());
                  curveList.push_back(newCurve);
               } else {
                  delete newCurve;
                  newCurve=curveList[index];
               }

               // add the curve to the surface
               surface->push_curve(newCurve);

               // extend the backside curve
               backsideCurve->push_line(l2,curve->get_reverse(j));
            }
            j++;
         }
      }
      i++;
   }
}

void Geo::write (ofstream *out)
{
   // points
   long unsigned int i=0;
   while (i < pointList.size()) {
      pointList[i]->write(out);
      i++;
   }

   // lines
   i=0;
   while (i < lineList.size()) {
      lineList[i]->write(out);
      i++;
   }

   // curves
   i=0;
   while (i < curveList.size()) {
      curveList[i]->write(out);
      i++;
   }

   // surfaces 
   i=0;
   while (i < surfaceList.size()) {
      surfaceList[i]->write(out);
      i++;
   }

   if (surfaceList.size() > 0) {
      long unsigned int i=0;
      while (i < surfaceList.size()) {
         *out << "Physical Volume(\"" << curveList[i]->get_name() << "\") = {" << curveList[i]->get_index() << "};" << endl;
         i++;
      }
   } else {
      long unsigned int i=0;
      while (i < curveList.size()) {
         if (curveList[i]->is_active()) {
            *out << "Physical Surface(\"" << curveList[i]->get_name() << "\"," << curveList[i]->get_index() << ") = {" 
                                          << curveList[i]->get_index() << "};" << endl;
         }
         i++;
      }
   }
}

void Geo::audit ()
{
   cout << "Geo Point Audit:" << endl;

   int unassigned_count=0;
   int active_count=0;
   long unsigned int i=0;
   while (i < pointList.size()) {
      if (pointList[i]->get_index() == -1) unassigned_count++;
      if (pointList[i]->is_active()) active_count++;
      i++;
   }
   cout << "   total number of points = " << pointList.size() << endl;
   cout << "   number of active points = " << active_count << endl;
   cout << "   number of unassigned points = " << unassigned_count << endl; 

   double min_distance=1e300;
   double max_distance=0;
   i=0;
   while (i < pointList.size()-1) {
      long unsigned int j=i+1;
      while (j < pointList.size()) {
         if (pointList[i]->is_active() && pointList[j]->is_active()) {
            double distance=pointList[i]->distance(pointList[j]);
            if (distance > max_distance) max_distance=distance;
            if (distance < min_distance) min_distance=distance;

            if (pointList[i]->is_equal(pointList[j])) {
               cout << "   duplicate point for indices " << pointList[i]->get_index() << " and " << pointList[j]->get_index() << endl;
            }
         }
         j++;
      }
      i++;
   } 
   cout << "   minimum point spacing = " << min_distance << endl;
   cout << "   maximum point spacing = " << max_distance << endl;

   cout << "Geo Line Audit:" << endl;

   unassigned_count=0;
   active_count=0;
   i=0;
   while (i < lineList.size()) {
      if (lineList[i]->get_index() == -1) unassigned_count++;
      if (lineList[i]->is_active()) active_count++;
      i++;
   }
   cout << "   total number of lines = " << lineList.size() << endl;
   cout << "   number of active lines = " << active_count << endl;
   cout << "   number of unassigned lines = " << unassigned_count << endl;

   i=0;
   while (i < lineList.size()) {
      if (lineList[i]->is_active()) {
         Point *first=lineList[i]->get_first();
         Point *second=lineList[i]->get_second();
         if (!first->is_active()) cout << "   Line " << lineList[i]->get_index() << " uses an inactive first point." << endl;
         if (!second->is_active()) cout << "   Line " << lineList[i]->get_index() << " uses an inactive second point." << endl;
      }
      i++;
   }
}

bool RectangularWaveguide::write_geo (string indent, Control *control, Geo *geo)
{
   bool fail=false;
   stringstream ssFilename;
   ssFilename << name.get_value() << ".geo";

   ofstream out;
   out.open (ssFilename.str().c_str(),ofstream::out);
   if (out.is_open()) {
      cout << "Writing \"" << ssFilename.str() << "\" ..." << endl;

      control->print_commented(&out);
      print_commented(&out);

      geo->write(&out);

      out.close();
   } else {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2020: Failed to open \"%s\" for writing.\n",
                                   indent.c_str(),ssFilename.str().c_str());
      fail=true;
   }
   return fail;
}

bool RectangularWaveguide::write_modes_and_boundaries (string indent, Control *control)
{
   if (length.get_dbl_value() != 0) return false;

   bool fail=false;
   stringstream ssFilename;
   ssFilename << name.get_value() << "_modes.txt";

   ofstream modes;
   modes.open (ssFilename.str().c_str(),ofstream::out);
   if (modes.is_open()) {
      cout << "Writing \"" << ssFilename.str() << "\" ..." << endl;

      modes << "#OpenParEMmodes 1.0" << endl << endl;

      control->print_commented(&modes);
      print_commented(&modes);

      modes << endl;
      modes << "File" << endl;
      modes << "   name=generated_by_builder" << endl;
      modes << "EndFile" << endl;

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=center_line" << endl;
      modes << setprecision(15) << "   point=(" << 0 << "," << -height.get_dbl_value()/2 << ")" << endl;
      modes << setprecision(15) << "   point=(" << 0 << "," << +height.get_dbl_value()/2 << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      modes << endl;
      modes << "Mode" << endl;
      modes << "   mode=1" << endl;
      modes << "   type=voltage" << endl;
      modes << "   path=center_line" << endl;
      modes << "EndMode" << endl;

      string boundary_material;

      // top

      if (conductor_material_top.is_loaded()) boundary_material=conductor_material_top.get_value();
      else boundary_material=default_conductor_material.get_value();
     
      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=conductor_top" << endl;
         modes << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << ")" << endl;
         modes << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=conductor_top" << endl;
         if (boundary_material.compare("PMC") == 0) {
            modes << "   type=perfect_magnetic_conductor" << endl;
         } else {
            modes << "   type=surface_impedance" << endl;
            modes << "   material=" << boundary_material << endl;
         }
         modes << "   path=conductor_top" << endl;
         modes << "EndBoundary" << endl;
      }

      // bottom

      if (conductor_material_bottom.is_loaded()) boundary_material=conductor_material_bottom.get_value();
      else boundary_material=default_conductor_material.get_value();
     
      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=conductor_bottom" << endl;
         modes << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << ")" << endl;
         modes << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=conductor_bottom" << endl;
         if (boundary_material.compare("PMC") == 0) {
            modes << "   type=perfect_magnetic_conductor" << endl;
         } else {
            modes << "   type=surface_impedance" << endl;
            modes << "   material=" << boundary_material << endl;
         }
         modes << "   path=conductor_bottom" << endl;
         modes << "EndBoundary" << endl;
      }

      // left

      if (conductor_material_left.is_loaded()) boundary_material=conductor_material_left.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=conductor_left" << endl;
         modes << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << ")" << endl;
         modes << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=conductor_left" << endl;
         if (boundary_material.compare("PMC") == 0) {
            modes << "   type=perfect_magnetic_conductor" << endl;
         } else {
            modes << "   type=surface_impedance" << endl;
            modes << "   material=" << boundary_material << endl;
         }
         modes << "   path=conductor_left" << endl;
         modes << "EndBoundary" << endl;
      }

      // right 

      if (conductor_material_right.is_loaded()) boundary_material=conductor_material_right.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) { 
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=conductor_right" << endl;
         modes << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << ")" << endl;
         modes << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=conductor_right" << endl;
         if (boundary_material.compare("PMC") == 0) {
            modes << "   type=perfect_magnetic_conductor" << endl;
         } else {
            modes << "   type=surface_impedance" << endl;
            modes << "   material=" << boundary_material << endl;
         }
         modes << "   path=conductor_right" << endl;
         modes << "EndBoundary" << endl;
      }

      modes.close();
   } else {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2021: Failed to open \"%s\" for writing.\n",
                                   indent.c_str(),ssFilename.str().c_str());
      fail=true;
   }
   return fail;
}

bool RectangularWaveguide::write_ports_and_boundaries (string indent, Control *control)
{
   if (length.get_dbl_value() == 0) return false;

   bool fail=false;
   stringstream ssFilename;
   ssFilename << name.get_value() << "_ports.txt";

   ofstream ports;
   ports.open (ssFilename.str().c_str(),ofstream::out);
   if (ports.is_open()) {
      cout << "Writing \"" << ssFilename.str() << "\" ..." << endl;

      ports << "#OpenParEMports 1.0" << endl << endl;

      control->print_commented(&ports);
      print_commented(&ports);

      ports << endl;
      ports << "File" << endl;
      ports << "   name=generated_by_builder" << endl;
      ports << "EndFile" << endl;

      // voltage paths

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=V1" << endl;
      ports << setprecision(15) << "   point=(" << 0 << "," << -height.get_dbl_value()/2 << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << 0 << "," << +height.get_dbl_value()/2 << "," << 0 << ")" << endl;
      ports << "   closed=false" << endl;
      ports << "EndPath" << endl;

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=V2" << endl;
      ports << setprecision(15) << "   point=(" << 0 << "," << -height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << 0 << "," << +height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
      ports << "   closed=false" << endl;
      ports << "EndPath" << endl;

      // port paths

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=port1" << endl;
      ports << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << "," << 0 << ")" << endl;
      ports << "   closed=true" << endl;
      ports << "EndPath" << endl;

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=port2" << endl;
      ports << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
      ports << "   closed=true" << endl;
      ports << "EndPath" << endl;

      // ports

      ports << endl;
      ports << "Port" << endl;
      ports << "   name=1" << endl;
      ports << "   path=+port1" << endl;
      ports << "      impedance_definition=PV" << endl;
      ports << "      impedance_calculation=line" << endl;
      ports << "   Line" << endl;
      ports << "      Sport=1" << endl;
      ports << "      net=in" << endl;
      ports << "      IntegrationPath" << endl;
      ports << "         type=voltage" << endl;
      ports << "         path=+V1" << endl;
      ports << "      EndIntegrationPath" << endl;
      ports << "   EndLine" << endl;
      ports << "EndPort" << endl;

      ports << endl;
      ports << "Port" << endl;
      ports << "   name=2" << endl;
      ports << "   path=+port2" << endl;
      ports << "      impedance_definition=PV" << endl;
      ports << "      impedance_calculation=line" << endl;
      ports << "   Line" << endl;
      ports << "      Sport=2" << endl;
      ports << "      net=out" << endl;
      ports << "      IntegrationPath" << endl;
      ports << "         type=voltage" << endl;
      ports << "         path=+V2" << endl;
      ports << "      EndIntegrationPath" << endl;
      ports << "   EndLine" << endl;
      ports << "EndPort" << endl;

      string boundary_material;

      // top

      if (conductor_material_top.is_loaded()) boundary_material=conductor_material_top.get_value();
      else boundary_material=default_conductor_material.get_value();
  
      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=conductor_top" << endl;
         ports << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=conductor_top" << endl;
         if (boundary_material.compare("PMC") == 0) {
            ports << "   type=perfect_magnetic_conductor" << endl;
         } else {
            ports << "   type=surface_impedance" << endl;
            ports << "   material=" << boundary_material << endl;
         }
         ports << "   path=conductor_top" << endl;
         ports << "EndBoundary" << endl;
      }

      // bottom

      if (conductor_material_bottom.is_loaded()) boundary_material=conductor_material_bottom.get_value();
      else boundary_material=default_conductor_material.get_value();
  
      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=conductor_bottom" << endl;
         ports << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=conductor_bottom" << endl;
         if (boundary_material.compare("PMC") == 0) {
            ports << "   type=perfect_magnetic_conductor" << endl;
         } else {
            ports << "   type=surface_impedance" << endl;
            ports << "   material=" << boundary_material << endl;
         }
         ports << "   path=conductor_bottom" << endl;
         ports << "EndBoundary" << endl;
      }

      // left

      if (conductor_material_left.is_loaded()) boundary_material=conductor_material_left.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=conductor_left" << endl;
         ports << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=conductor_left" << endl;
         if (boundary_material.compare("PMC") == 0) {
            ports << "   type=perfect_magnetic_conductor" << endl;
         } else {
            ports << "   type=surface_impedance" << endl;
            ports << "   material=" << boundary_material << endl;
         }
         ports << "   path=conductor_left" << endl;
         ports << "EndBoundary" << endl;
      }

      // right

      if (conductor_material_right.is_loaded()) boundary_material=conductor_material_right.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=conductor_right" << endl;
         ports << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << -height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << +width.get_dbl_value()/2 << "," << +height.get_dbl_value()/2 << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=conductor_right" << endl;
         if (boundary_material.compare("PMC") == 0) {
            ports << "   type=perfect_magnetic_conductor" << endl;
         } else {
            ports << "   type=surface_impedance" << endl;
            ports << "   material=" << boundary_material << endl;
         }
         ports << "   path=conductor_right" << endl;
         ports << "EndBoundary" << endl;
      }

      ports.close();
   } else {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2236: Failed to open \"%s\" for writing.\n",
                                   indent.c_str(),ssFilename.str().c_str());
      fail=true;
   }
   return fail;
}

bool write_OpenParEM2D_proj_text (string indent, string name, int mode_count, string impedance_definition, string impedance_calculation)
{
   bool fail=false;

   stringstream ssProj;
   ssProj << name << ".proj";

   ofstream proj;
   proj.open (ssProj.str().c_str(),ofstream::out);
   if (proj.is_open()) {
      cout << "Writing \"" << ssProj.str() << "\" ..." << endl;

      proj << "#OpenParEM2Dproject 1.0" << endl;
      proj << endl;
      proj << "// template settings: change as needed" << endl;
      proj << endl;
      proj << "project.save.fields               true" << endl;
      proj << endl;
      proj << "mesh.file                         " << name << ".msh" << endl;
      proj << "fem.order                         4" << endl;
      proj << endl;
      if (impedance_calculation.compare("modal") == 0) {
         proj << "mode.definition.file              " << name << "_modes.txt" << endl;
      }
      if (impedance_calculation.compare("line") == 0) {
         proj << "mode.definition.file              " << name << "_lines.txt" << endl;
      }
      proj << endl;
      proj << "materials.global.path             " << endl;
      proj << "materials.global.name             " << endl;
      proj << "materials.local.path              " << endl;
      proj << "materials.local.name              materials.txt   // change to point at the project materials file" << endl;
      proj << endl;
      proj << "refinement.frequency              high" << endl;
      proj << "refinement.variable               |Zo|" << endl;
      proj << "refinement.iteration.min          3" << endl;
      proj << "refinement.iteration.max          50" << endl;
      proj << "refinement.required.passes        3" << endl;
      proj << "refinement.tolerance              1e-7" << endl;
      proj << endl;
      proj << "frequency.plan.linear             9e9,10e9,1e9" << endl;
      proj << endl;
      proj << "solution.modes                    " << mode_count << endl;
      proj << "solution.temperature              20" << endl;
      proj << "solution.tolerance                1e-12" << endl;
      proj << "solution.iteration.limit          10000" << endl;
      proj << "solution.modes.buffer             0" << endl;
      proj << "solution.impedance.definition     " << impedance_definition << endl;
      proj << "solution.impedance.calculation    " << impedance_calculation << endl;
      proj << "solution.shift.invert             true" << endl;
      proj << "solution.initial.guess.level      2" << endl;
      proj << endl;
      proj << "output.show.refining.mesh         false" << endl;
      proj << "output.show.postprocessing        false" << endl;
      proj << "output.show.iterations            false" << endl;
      proj << "output.show.license               false" << endl;

      proj.close();
   } else {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2022: Failed to open \"%s\" for writing.\n",
                                   indent.c_str(),ssProj.str().c_str());
      fail=true;
   }
   return fail;
}

bool RectangularWaveguide::write_OpenParEM2D_proj(string indent)
{
   if (length.get_dbl_value() != 0) return false;
   return write_OpenParEM2D_proj_text (indent,name.get_value(),1,"PV","modal");
}

bool write_OpenParEM3D_proj_text (string indent, string name, keywordPair *default_conductor_material)
{
   bool fail=false;

   stringstream ssProj;
   ssProj << name << ".proj";

   ofstream proj;
   proj.open (ssProj.str().c_str(),ofstream::out);
   if (proj.is_open()) {
      cout << "Writing \"" << ssProj.str() << "\" ..." << endl;

      proj << "#OpenParEM3Dproject 1.0" << endl;
      proj << endl;
      proj << "// template settings: change as needed" << endl;
      proj << endl;
      proj << "project.save.fields               true" << endl;
      proj << "mesh.file                         " << name << ".msh" << endl;
      proj << "//mesh.quality.limit                1e6" << endl;
      proj << "//mesh.refinement.fraction          0.025" << endl;
      proj << "fem.order                         4" << endl;
      proj << "port.definition.file              " << name << "_ports.txt" << endl;
      proj << "materials.global.path             " << endl;
      proj << "materials.global.name             " << endl;
      proj << "materials.local.path              " << endl;
      proj << "materials.local.name              materials.txt   // change to point at the project materials file" << endl;
      if (default_conductor_material->is_loaded() && default_conductor_material->get_value().compare("PEC") != 0 ) {
         proj << "materials.default.boundary        " << default_conductor_material->get_value() << endl;
      }
      proj << "refinement.frequency              high" << endl;
      proj << "refinement.iteration.min          3" << endl;
      proj << "refinement.iteration.max          50" << endl;
      proj << "refinement.required.passes        1" << endl;
      proj << "refinement.relative.tolerance     0.001" << endl;
      proj << "refinement.absolute.tolerance     1e-7" << endl;
      proj << "refinement.variable               SandE" << endl;
      proj << "frequency.plan.linear             9e9,10e9,1e9" << endl;
      proj << "reference.impedance               0" << endl;
      proj << "touchstone.format                 DB" << endl;
      proj << "solution.temperature              20" << endl;
      proj << "solution.2D.tolerance             1e-14" << endl;
      proj << "solution.3D.tolerance             1e-14" << endl;
      proj << "solution.iteration.limit          10000" << endl;
      proj << "solution.modes.buffer             0" << endl;
      proj << "solution.shift.invert             true" << endl;
      proj << "solution.initial.guess.level      2" << endl;
      proj << "output.show.refining.mesh         false" << endl;
      proj << "output.show.postprocessing        false" << endl;
      proj << "output.show.iterations            false" << endl;
      proj << "output.show.license               false" << endl;

      proj.close();
   } else {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2191: Failed to open \"%s\" for writing.\n",
                                   indent.c_str(),ssProj.str().c_str());
      fail=true;
   }
   return fail;
}

bool RectangularWaveguide::write_OpenParEM3D_proj(string indent)
{
   if (length.get_dbl_value() == 0) return false;
   return write_OpenParEM3D_proj_text (indent,name.get_value(),&default_conductor_material);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Strip
///////////////////////////////////////////////////////////////////////////////////////////

Strip::Strip (int startLine_, int endLine_, bool checkLimits_)
{
   startLine=startLine_;
   endLine=endLine_;

   // name

   name.push_alias("name");
   name.set_loaded(false);
   name.set_positive_required(false);
   name.set_non_negative_required(false);
   name.set_lowerLimit(0);
   name.set_upperLimit(1e12);
   name.set_checkLimits(checkLimits_);

   // include

   include.push_alias("include");
   include.set_loaded(false);
   include.set_positive_required(false);
   include.set_non_negative_required(false);
   include.set_lowerLimit(0);
   include.set_upperLimit(1e12);
   include.set_checkLimits(checkLimits_);

   // use_symmetry

   use_symmetry.push_alias("use_symmetry");
   use_symmetry.set_loaded(false);
   use_symmetry.set_positive_required(false);
   use_symmetry.set_non_negative_required(false);
   use_symmetry.set_lowerLimit(0);
   use_symmetry.set_upperLimit(1e12);
   use_symmetry.set_checkLimits(checkLimits_);

   // upper_material

   upper_material.push_alias("upper_material");
   upper_material.set_loaded(false);
   upper_material.set_positive_required(false);
   upper_material.set_non_negative_required(false);
   upper_material.set_lowerLimit(0);
   upper_material.set_upperLimit(1e12);
   upper_material.set_checkLimits(checkLimits_);

   // upper_thickness

   upper_thickness.push_alias("upper_thickness");
   upper_thickness.set_loaded(false);
   upper_thickness.set_positive_required(false);
   upper_thickness.set_non_negative_required(true);
   upper_thickness.set_lowerLimit(0);
   upper_thickness.set_upperLimit(0.1);
   upper_thickness.set_checkLimits(checkLimits_);

   // soldermask_thickness

   soldermask_thickness.push_alias("soldermask_thickness");
   soldermask_thickness.set_loaded(false);
   soldermask_thickness.set_positive_required(true);
   soldermask_thickness.set_non_negative_required(false);
   soldermask_thickness.set_lowerLimit(1e-6);
   soldermask_thickness.set_upperLimit(0.1);
   soldermask_thickness.set_checkLimits(checkLimits_);

   // soldermask_material

   soldermask_material.push_alias("soldermask_material");
   soldermask_material.set_loaded(false);
   soldermask_material.set_positive_required(false);
   soldermask_material.set_non_negative_required(false);
   soldermask_material.set_lowerLimit(0);
   soldermask_material.set_upperLimit(1e12);
   soldermask_material.set_checkLimits(checkLimits_);

   // lower_thickness

   lower_thickness.push_alias("lower_thickness");
   lower_thickness.set_loaded(false);
   lower_thickness.set_positive_required(true);
   lower_thickness.set_non_negative_required(false);
   lower_thickness.set_lowerLimit(1e-6);
   lower_thickness.set_upperLimit(0.1);
   lower_thickness.set_checkLimits(checkLimits_);

   // lower_material

   lower_material.push_alias("lower_material");
   lower_material.set_loaded(false);
   lower_material.set_positive_required(false);
   lower_material.set_non_negative_required(false);
   lower_material.set_lowerLimit(0);
   lower_material.set_upperLimit(1e12);
   lower_material.set_checkLimits(checkLimits_);

   // trace_thickness

   trace_thickness.push_alias("trace_thickness");
   trace_thickness.set_loaded(false);
   trace_thickness.set_positive_required(true);
   trace_thickness.set_non_negative_required(false);
   trace_thickness.set_lowerLimit(1e-6);
   trace_thickness.set_upperLimit(0.1);
   trace_thickness.set_checkLimits(checkLimits_);

   // trace_width

   trace_width.push_alias("trace_width");
   trace_width.set_loaded(false);
   trace_width.set_positive_required(true);
   trace_width.set_non_negative_required(false);
   trace_width.set_lowerLimit(1e-6);
   trace_width.set_upperLimit(0.1);
   trace_width.set_checkLimits(checkLimits_);

   // trace_etch_angle

   trace_etch_angle.push_alias("trace_etch_angle");
   trace_etch_angle.set_loaded(false);
   trace_etch_angle.set_positive_required(true);
   trace_etch_angle.set_non_negative_required(false);
   trace_etch_angle.set_lowerLimit(30);
   trace_etch_angle.set_upperLimit(150);
   trace_etch_angle.set_checkLimits(checkLimits_);

   // default_conductor_material

   default_conductor_material.push_alias("default_conductor_material");
   default_conductor_material.set_loaded(false);
   default_conductor_material.set_positive_required(false);
   default_conductor_material.set_non_negative_required(false);
   default_conductor_material.set_lowerLimit(0);
   default_conductor_material.set_upperLimit(1e12);
   default_conductor_material.set_checkLimits(checkLimits_);

   // trace_material_top

   trace_material_top.push_alias("trace_material_top");
   trace_material_top.set_loaded(false);
   trace_material_top.set_positive_required(false);
   trace_material_top.set_non_negative_required(false);
   trace_material_top.set_lowerLimit(0);
   trace_material_top.set_upperLimit(1e12);
   trace_material_top.set_checkLimits(checkLimits_);

   // trace_material_bottom

   trace_material_bottom.push_alias("trace_material_bottom");
   trace_material_bottom.set_loaded(false);
   trace_material_bottom.set_positive_required(false);
   trace_material_bottom.set_non_negative_required(false);
   trace_material_bottom.set_lowerLimit(0);
   trace_material_bottom.set_upperLimit(1e12);
   trace_material_bottom.set_checkLimits(checkLimits_);

   // trace_material_sides

   trace_material_sides.push_alias("trace_material_sides");
   trace_material_sides.set_loaded(false);
   trace_material_sides.set_positive_required(false);
   trace_material_sides.set_non_negative_required(false);
   trace_material_sides.set_lowerLimit(0);
   trace_material_sides.set_upperLimit(1e12);
   trace_material_sides.set_checkLimits(checkLimits_);

   // upper_groundplane_material

   upper_groundplane_material.push_alias("upper_groundplane_material");
   upper_groundplane_material.set_loaded(false);
   upper_groundplane_material.set_positive_required(false);
   upper_groundplane_material.set_non_negative_required(false);
   upper_groundplane_material.set_lowerLimit(0);
   upper_groundplane_material.set_upperLimit(1e12);
   upper_groundplane_material.set_checkLimits(checkLimits_);

   // lower_groundplane_material

   lower_groundplane_material.push_alias("lower_groundplane_material");
   lower_groundplane_material.set_loaded(false);
   lower_groundplane_material.set_positive_required(false);
   lower_groundplane_material.set_non_negative_required(false);
   lower_groundplane_material.set_lowerLimit(0);
   lower_groundplane_material.set_lowerLimit(1e12);
   lower_groundplane_material.set_checkLimits(checkLimits_);

   // left_side_material

   left_side_material.push_alias("left_side_material");
   left_side_material.set_loaded(false);
   left_side_material.set_positive_required(false);
   left_side_material.set_non_negative_required(false);
   left_side_material.set_lowerLimit(0);
   left_side_material.set_upperLimit(1e12);
   left_side_material.set_checkLimits(checkLimits_);

   // left_side_gap

   left_side_gap.push_alias("left_side_gap");
   left_side_gap.set_loaded(false);
   left_side_gap.set_positive_required(true);
   left_side_gap.set_non_negative_required(false);
   left_side_gap.set_lowerLimit(1e-6);
   left_side_gap.set_upperLimit(0.1);
   left_side_gap.set_checkLimits(checkLimits_);

   // right_side_material

   right_side_material.push_alias("right_side_material");
   right_side_material.set_loaded(false);
   right_side_material.set_positive_required(false);
   right_side_material.set_non_negative_required(false);
   right_side_material.set_lowerLimit(0);
   right_side_material.set_upperLimit(1e12);
   right_side_material.set_checkLimits(checkLimits_);

   // right_side_gap

   right_side_gap.push_alias("right_side_gap");
   right_side_gap.set_loaded(false);
   right_side_gap.set_positive_required(true);
   right_side_gap.set_non_negative_required(false);
   right_side_gap.set_lowerLimit(1e-6);
   right_side_gap.set_upperLimit(0.1);
   right_side_gap.set_checkLimits(checkLimits_);

   // length

   length.push_alias("length");
   length.set_loaded(false);
   length.set_positive_required(false);
   length.set_non_negative_required(true);
   length.set_lowerLimit(0);
   length.set_upperLimit(1);
   length.set_checkLimits(checkLimits_);

   included=nullptr;

   // defaults
   length.set_dbl_value(0);
}

void Strip::print_commented(ofstream *out)
{
   *out << "// Strip" << endl;
   *out << "//    name=" << name.get_value() << endl;
   if (include.is_loaded()) *out << "//    include=" << include.get_value() << endl;
   if (use_symmetry.get_bool_value()) *out << "//    use_symmetry=true" << endl;
   else *out << "//    use_symmetry=false" << endl;
   *out << "//    upper_material=" << upper_material.get_value() << endl;
   *out << "//    upper_thickness=" << upper_thickness.get_dbl_value() << endl;
   *out << "//    lower_material=" << lower_material.get_value() << endl;
   *out << "//    lower_thickness=" << lower_thickness.get_dbl_value() << endl;
   if (left_side_material.is_loaded()) *out << "//    left_side_material=" << left_side_material.get_value() << endl;
   *out << "//    left_side_gap=" << left_side_gap.get_dbl_value() << endl;
   if (right_side_material.is_loaded()) *out << "//    right_side_material=" << right_side_material.get_value() << endl;
   *out << "//    right_side_gap=" << right_side_gap.get_dbl_value() << endl;
   if (soldermask_thickness.is_loaded()) {
      *out << "//    soldermask_thickness=" << soldermask_thickness.get_dbl_value() << endl;
      if (soldermask_material.is_loaded()) *out << "//    soldermask_material=" << soldermask_material.get_value() << endl;
   }
   *out << "//    trace_thickness=" << trace_thickness.get_dbl_value() << endl;
   *out << "//    trace_width=" << trace_width.get_dbl_value() << endl;
   *out << "//    trace_etch_angle=" << trace_etch_angle.get_dbl_value() << endl;
   if (trace_material_bottom.is_loaded() && trace_material_top.is_loaded() && trace_material_sides.is_loaded() && 
       upper_groundplane_material.is_loaded() && lower_groundplane_material.is_loaded() && left_side_material.is_loaded() && right_side_material.is_loaded()) {
      // no need to print
   } else {
      *out << "//    default_conductor_material=" << default_conductor_material.get_value() << endl;
   }
   if (trace_material_bottom.is_loaded()) *out << "//    trace_material_bottom=" << trace_material_bottom.get_value() << endl;
   if (trace_material_top.is_loaded()) *out << "//    trace_material_top=" << trace_material_top.get_value() << endl;
   if (trace_material_sides.is_loaded()) *out << "//    trace_material_sides=" << trace_material_sides.get_value() << endl;
   if (upper_groundplane_material.is_loaded()) *out << "//    upper_groundplane_material=" << upper_groundplane_material.get_value() << endl;
   if (lower_groundplane_material.is_loaded()) *out << "//    lower_groundplane_material=" << lower_groundplane_material.get_value() << endl;
   if (length.is_loaded()) *out << "//    length=" << length.get_dbl_value() << endl;

   *out << "// EndStrip" << endl;
}

bool Strip::load (string *indent, inputFile *inputs, bool checkInputs)
{
   bool fail=false;

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {
      string token,value,line;
      line=inputs->get_line(lineNumber);
      get_token_pair(&line,&token,&value,&lineNumber,indent->c_str());

      int recognized=0;
      if (name.match_alias(&token)) {
         if (name.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2023: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,name.get_lineNumber());
            fail=true;
         } else {
            name.set_keyword(token);
            name.set_value(value);
            name.set_lineNumber(lineNumber);
            name.set_loaded(true);
         }
         recognized++;
      }

      if (include.match_alias(&token)) {
         if (include.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2024: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,include.get_lineNumber());
            fail=true;
         } else {
            include.set_keyword(token);
            include.set_value(value);
            include.set_lineNumber(lineNumber);
            include.set_loaded(true);
         }
         recognized++;
      }

      if (use_symmetry.match_alias(&token)) {
         recognized++;
         if (use_symmetry.loadBool(&token, &value, lineNumber)) fail=true;
      }

      if (upper_material.match_alias(&token)) {
         if (upper_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2025: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,upper_material.get_lineNumber());
            fail=true;
         } else {
            upper_material.set_keyword(token);
            upper_material.set_value(value);
            upper_material.set_lineNumber(lineNumber);
            upper_material.set_loaded(true);
         }
         recognized++;
      }

      if (upper_thickness.match_alias(&token)) {
         recognized++;
         if (upper_thickness.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (soldermask_thickness.match_alias(&token)) {
         recognized++;
         if (soldermask_thickness.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (soldermask_material.match_alias(&token)) {
         if (soldermask_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2026: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,soldermask_material.get_lineNumber());
            fail=true;
         } else {
            soldermask_material.set_keyword(token);
            soldermask_material.set_value(value);
            soldermask_material.set_lineNumber(lineNumber);
            soldermask_material.set_loaded(true);
         }
         recognized++;
      }

      if (lower_thickness.match_alias(&token)) {
         recognized++;
         if (lower_thickness.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (lower_material.match_alias(&token)) {
         if (lower_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2027: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,lower_material.get_lineNumber());
            fail=true;
         } else {
            lower_material.set_keyword(token);
            lower_material.set_value(value);
            lower_material.set_lineNumber(lineNumber);
            lower_material.set_loaded(true);
         }
         recognized++;
      }

      if (left_side_gap.match_alias(&token)) {
         recognized++;
         if (left_side_gap.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (left_side_material.match_alias(&token)) {
         if (left_side_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2028: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,left_side_material.get_lineNumber());
            fail=true;
         } else {
            left_side_material.set_keyword(token);
            left_side_material.set_value(value);
            left_side_material.set_lineNumber(lineNumber);
            left_side_material.set_loaded(true);
         }
         recognized++;
      }

      if (right_side_gap.match_alias(&token)) {
         recognized++;
         if (right_side_gap.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (right_side_material.match_alias(&token)) {
         if (right_side_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2029: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,right_side_material.get_lineNumber());
            fail=true;
         } else {
            right_side_material.set_keyword(token);
            right_side_material.set_value(value);
            right_side_material.set_lineNumber(lineNumber);
            right_side_material.set_loaded(true);
         }
         recognized++;
      }

      if (trace_thickness.match_alias(&token)) {
         recognized++;
         if (trace_thickness.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (trace_width.match_alias(&token)) {
         recognized++;
         if (trace_width.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (trace_etch_angle.match_alias(&token)) {
         recognized++;
         if (trace_etch_angle.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (default_conductor_material.match_alias(&token)) {
         if (default_conductor_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2030: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,default_conductor_material.get_lineNumber());
            fail=true;
         } else {
            default_conductor_material.set_keyword(token);
            default_conductor_material.set_value(value);
            default_conductor_material.set_lineNumber(lineNumber);
            default_conductor_material.set_loaded(true);
         }
         recognized++;
      }

      if (trace_material_top.match_alias(&token)) {
         if (trace_material_top.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2031: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,trace_material_top.get_lineNumber());
            fail=true;
         } else {
            trace_material_top.set_keyword(token);
            trace_material_top.set_value(value);
            trace_material_top.set_lineNumber(lineNumber);
            trace_material_top.set_loaded(true);
         }
         recognized++;
      }

      if (trace_material_bottom.match_alias(&token)) {
         if (trace_material_bottom.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2032: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,trace_material_bottom.get_lineNumber());
            fail=true;
         } else {
            trace_material_bottom.set_keyword(token);
            trace_material_bottom.set_value(value);
            trace_material_bottom.set_lineNumber(lineNumber);
            trace_material_bottom.set_loaded(true);
         }
         recognized++;
      }

      if (trace_material_sides.match_alias(&token)) {
         if (trace_material_sides.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2033: Duplicate entry at line %d for previous entry at line %d.\n", 
                                         indent->c_str(),lineNumber,trace_material_sides.get_lineNumber());
            fail=true;
         } else {
            trace_material_sides.set_keyword(token);
            trace_material_sides.set_value(value);
            trace_material_sides.set_lineNumber(lineNumber);
            trace_material_sides.set_loaded(true);
         }
         recognized++;
      }

      if (upper_groundplane_material.match_alias(&token)) {
         if (upper_groundplane_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2034: Duplicate entry at line %d for previous entry at line %d.\n", 
                                         indent->c_str(),lineNumber,upper_groundplane_material.get_lineNumber());
            fail=true;
         } else {
            upper_groundplane_material.set_keyword(token);
            upper_groundplane_material.set_value(value);
            upper_groundplane_material.set_lineNumber(lineNumber);
            upper_groundplane_material.set_loaded(true);
         }
         recognized++;
      }

      if (lower_groundplane_material.match_alias(&token)) {
         if (lower_groundplane_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2035: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,lower_groundplane_material.get_lineNumber());
            fail=true;
         } else {
            lower_groundplane_material.set_keyword(token);
            lower_groundplane_material.set_value(value);
            lower_groundplane_material.set_lineNumber(lineNumber);
            lower_groundplane_material.set_loaded(true);
         }
         recognized++;
      }

      if (length.match_alias(&token)) {
         recognized++;
         if (length.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      // should recognize one keyword
      if (recognized != 1) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2036: Unrecognized keyword at line %d.\n",indent->c_str(),lineNumber);
         fail=true;
      }

      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }
   return fail;
}

bool Strip::inBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

void Strip::apply_include ()
{
   if (included == nullptr) return;

   if (included_type == 0) {
      RectangularWaveguide *rw=(RectangularWaveguide *)included;
      if (! default_conductor_material.is_loaded()) if (rw->get_default_conductor_material().is_loaded()) default_conductor_material.copy(rw->get_default_conductor_material());
   } else if (included_type == 1) {
      Strip *strip=(Strip *)included;
      if (! use_symmetry.is_loaded()) if (strip->use_symmetry.is_loaded()) use_symmetry.copy(strip->use_symmetry);
      if (! upper_material.is_loaded()) if (strip->upper_material.is_loaded()) upper_material.copy(strip->upper_material);
      if (! upper_thickness.is_loaded()) if (strip->upper_thickness.is_loaded()) upper_thickness.copy(strip->upper_thickness);
      if (! soldermask_thickness.is_loaded()) if (strip->soldermask_thickness.is_loaded()) soldermask_thickness.copy(strip->soldermask_thickness);
      if (! soldermask_material.is_loaded()) if (strip->soldermask_material.is_loaded()) soldermask_material.copy(strip->soldermask_material);
      if (! lower_thickness.is_loaded()) if (strip->lower_thickness.is_loaded()) lower_thickness.copy(strip->lower_thickness);
      if (! lower_material.is_loaded()) if (strip->lower_material.is_loaded()) lower_material.copy(strip->lower_material);
      if (! trace_thickness.is_loaded()) if (strip->trace_thickness.is_loaded()) trace_thickness.copy(strip->trace_thickness);
      if (! trace_width.is_loaded()) if (strip->trace_width.is_loaded()) trace_width.copy(strip->trace_width);
      if (! trace_etch_angle.is_loaded()) if (strip->trace_etch_angle.is_loaded()) trace_etch_angle.copy(strip->trace_etch_angle);
      if (! default_conductor_material.is_loaded()) if (strip->default_conductor_material.is_loaded()) default_conductor_material.copy(strip->default_conductor_material);
      if (! trace_material_bottom.is_loaded()) if (strip->trace_material_bottom.is_loaded()) trace_material_bottom.copy(strip->trace_material_bottom);
      if (! trace_material_top.is_loaded()) if (strip->trace_material_top.is_loaded()) trace_material_top.copy(strip->trace_material_top);
      if (! trace_material_sides.is_loaded()) if (strip->trace_material_sides.is_loaded()) trace_material_sides.copy(strip->trace_material_sides);
      if (! upper_groundplane_material.is_loaded()) if (strip->upper_groundplane_material.is_loaded()) upper_groundplane_material.copy(strip->upper_groundplane_material);
      if (! lower_groundplane_material.is_loaded()) if (strip->lower_groundplane_material.is_loaded()) lower_groundplane_material.copy(strip->lower_groundplane_material);
      if (! left_side_material.is_loaded()) if (strip->left_side_material.is_loaded()) left_side_material.copy(strip->left_side_material);
      if (! left_side_gap.is_loaded()) if (strip->left_side_gap.is_loaded()) left_side_gap.copy(strip->left_side_gap);
      if (! right_side_material.is_loaded()) if (strip->right_side_material.is_loaded()) right_side_material.copy(strip->right_side_material);
      if (! right_side_gap.is_loaded()) if (strip->right_side_gap.is_loaded()) right_side_gap.copy(strip->right_side_gap);
   } else if (included_type == 2) {
      CoupledStrip *coupledStrip=(CoupledStrip *)included;
      if (! upper_material.is_loaded()) if (coupledStrip->get_upper_material().is_loaded()) upper_material.copy(coupledStrip->get_upper_material());
      if (! upper_thickness.is_loaded()) if (coupledStrip->get_upper_thickness().is_loaded()) upper_thickness.copy(coupledStrip->get_upper_thickness());
      if (! soldermask_thickness.is_loaded()) if (coupledStrip->get_soldermask_thickness().is_loaded()) soldermask_thickness.copy(coupledStrip->get_soldermask_thickness());
      if (! soldermask_material.is_loaded()) if (coupledStrip->get_soldermask_material().is_loaded()) soldermask_material.copy(coupledStrip->get_soldermask_material());
      if (! lower_thickness.is_loaded()) if (coupledStrip->get_lower_thickness().is_loaded()) lower_thickness.copy(coupledStrip->get_lower_thickness());
      if (! lower_material.is_loaded()) if (coupledStrip->get_lower_material().is_loaded()) lower_material.copy(coupledStrip->get_lower_material());
      if (! trace_width.is_loaded()) if (coupledStrip->get_trace_left_width().is_loaded()) trace_width.copy(coupledStrip->get_trace_left_width());
      if (! trace_thickness.is_loaded()) if (coupledStrip->get_trace_thickness().is_loaded()) trace_thickness.copy(coupledStrip->get_trace_thickness());
      if (! trace_etch_angle.is_loaded()) if (coupledStrip->get_trace_etch_angle().is_loaded()) trace_etch_angle.copy(coupledStrip->get_trace_etch_angle());
      if (! default_conductor_material.is_loaded()) if (coupledStrip->get_default_conductor_material().is_loaded()) default_conductor_material.copy(coupledStrip->get_default_conductor_material());
      if (! trace_material_bottom.is_loaded()) if (coupledStrip->get_trace_material_bottom().is_loaded()) trace_material_bottom.copy(coupledStrip->get_trace_material_bottom());
      if (! trace_material_top.is_loaded()) if (coupledStrip->get_trace_material_top().is_loaded()) trace_material_top.copy(coupledStrip->get_trace_material_top());
      if (! trace_material_sides.is_loaded()) if (coupledStrip->get_trace_material_sides().is_loaded()) trace_material_sides.copy(coupledStrip->get_trace_material_sides());
      if (! upper_groundplane_material.is_loaded()) if (coupledStrip->get_upper_groundplane_material().is_loaded()) upper_groundplane_material.copy(coupledStrip->get_upper_groundplane_material());
      if (! lower_groundplane_material.is_loaded()) if (coupledStrip->get_lower_groundplane_material().is_loaded()) lower_groundplane_material.copy(coupledStrip->get_lower_groundplane_material());
      if (! left_side_material.is_loaded()) if (coupledStrip->get_left_side_material().is_loaded()) left_side_material.copy(coupledStrip->get_left_side_material());
      if (! left_side_gap.is_loaded()) if (coupledStrip->get_left_side_gap().is_loaded()) left_side_gap.copy(coupledStrip->get_left_side_gap());
      if (! right_side_material.is_loaded()) if (coupledStrip->get_right_side_material().is_loaded()) right_side_material.copy(coupledStrip->get_right_side_material());
      if (! right_side_gap.is_loaded()) if (coupledStrip->get_right_side_gap().is_loaded()) right_side_gap.copy(coupledStrip->get_right_side_gap());
   }
}

bool Strip::check (string indent)
{
   bool fail=false;

   if (!name.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2037: Strip block at line %d must specify a name.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!use_symmetry.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2038: Strip block at line %d must specify a value for \"use_symmetry\".\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!upper_thickness.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2039: Strip block at line %d must specify an upper thickness.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!upper_material.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2040: Strip block at line %d must specify an upper material.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (soldermask_thickness.is_loaded() && !soldermask_material.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2041: Strip block at line %d must specify a soldermask material.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!soldermask_thickness.is_loaded() && soldermask_material.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2042: Strip block at line %d must not specify a soldermask material.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!lower_thickness.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2043: Strip block at line %d must specify a lower thickness.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!lower_material.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2044: Strip block at line %d must specify a lower material.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!left_side_gap.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2045: Strip block at line %d must specify a left side gap.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!right_side_gap.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2046: Strip block at line %d must specify a right side gap.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!trace_thickness.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2047: Strip block at line %d must specify a trace thickness.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!trace_width.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2048: Strip block at line %d must specify a trace width.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!trace_etch_angle.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2049: Strip block at line %d must specify a trace etch angle.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!default_conductor_material.is_loaded()) {
      if (trace_material_top.is_loaded() && trace_material_bottom.is_loaded() && trace_material_sides.is_loaded() && 
          upper_groundplane_material.is_loaded() && lower_groundplane_material.is_loaded() && left_side_material.is_loaded() && right_side_material.is_loaded()) {
         // ok
      } else {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2050: Strip block at line %d must specify a default conductor material.\n",indent.c_str(),startLine);
         fail=true;
      }
   }

   // gmsh fails to mesh a section if the name is duplicated

   if (lower_material.is_loaded() && upper_material.is_loaded()) {
      if (lower_material.get_value().compare(upper_material.get_value()) == 0) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2051: Strip block at line %d must specify different names for the uppper and lower materials [properties can match].\n",indent.c_str(),startLine);
         fail=true;
      }
   }

   if (lower_material.is_loaded() && soldermask_material.is_loaded()) {
      if (lower_material.get_value().compare(soldermask_material.get_value()) == 0) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2052: Strip block at line %d must specify different names for the lower and soldermask materials [properties can match].\n",indent.c_str(),startLine);
         fail=true;
      }
   }

   if (upper_material.is_loaded() && soldermask_material.is_loaded()) {
      if (upper_material.get_value().compare(soldermask_material.get_value()) == 0) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2053: Strip block at line %d must specify different names for the uppper and soldermask materials [properties can match].\n",indent.c_str(),startLine);
         fail=true;
      }
   }

   return fail;
}

bool Strip::checkInclude (string indent)
{
   if (name.is_loaded() && include.is_loaded()) {
      if (name.get_value().compare(include.get_value()) == 0) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2054: Strip block at line %d cannot include itself.\n",indent.c_str(),startLine);
         return true;
      }
   }
   return false;
}

bool Strip::checkLimits()
{
   bool fail=false;

   if (upper_thickness.limit_check("double")) fail=true;
   if (left_side_gap.limit_check("double")) fail=true;
   if (right_side_gap.limit_check("double")) fail=true;
   if (soldermask_thickness.is_loaded() && soldermask_thickness.limit_check("double")) fail=true;
   if (lower_thickness.limit_check("double")) fail=true;
   if (trace_thickness.limit_check("double")) fail=true;
   if (trace_width.limit_check("double")) fail=true;
   if (trace_etch_angle.limit_check("double")) fail=true;
   if (length.limit_check("double")) fail=true;

   return fail;
}


bool Strip::check_geo (Geo *geo)
{
   bool fail=false;

   // horizontal

   if (geo->find_point(2)->get_x() <= geo->find_point(1)->get_x()) fail=true;
   if (geo->find_point(9)->get_x() <= geo->find_point(8)->get_x()) fail=true;
   if (geo->find_point(29)->get_x() <= geo->find_point(28)->get_x()) fail=true;

   if (geo->find_point(3)->get_x() >= geo->find_point(4)->get_x()) fail=true;
   if (geo->find_point(13)->get_x() >= geo->find_point(14)->get_x()) fail=true;
   if (geo->find_point(23)->get_x() >= geo->find_point(24)->get_x()) fail=true;

   if (!use_symmetry.get_bool_value() && geo->find_point(5)->get_x() <= geo->find_point(4)->get_x()) fail=true;
   if (!use_symmetry.get_bool_value() && geo->find_point(15)->get_x() <= geo->find_point(14)->get_x()) fail=true;
   if (!use_symmetry.get_bool_value() && geo->find_point(25)->get_x() <= geo->find_point(24)->get_x()) fail=true;

   if (!use_symmetry.get_bool_value() && geo->find_point(6)->get_x() >= geo->find_point(7)->get_x()) fail=true;
   if (!use_symmetry.get_bool_value() && geo->find_point(19)->get_x() >= geo->find_point(20)->get_x()) fail=true;
   if (!use_symmetry.get_bool_value() && geo->find_point(29)->get_x() >= geo->find_point(30)->get_x()) fail=true;

   // vertical

   if (geo->find_point(24)->get_y() >= geo->find_point(29)->get_y()) fail=true;

   // soldermask
   if (soldermask_thickness.is_loaded() && soldermask_thickness.get_dbl_value() > 0) {

      // horizontal

      if (geo->find_point(32)->get_x() <= geo->find_point(31)->get_x()) fail=true;
      if (geo->find_point(37)->get_x() >= geo->find_point(38)->get_x()) fail=true;
      if (!use_symmetry.get_bool_value() && geo->find_point(39)->get_x() <= geo->find_point(38)->get_x()) fail=true;
      if (!use_symmetry.get_bool_value() && geo->find_point(44)->get_x() >= geo->find_point(45)->get_x()) fail=true;

      // vertical

      if (geo->find_point(38)->get_y() >= geo->find_point(29)->get_y()) fail=true;
   }

   return fail;
}


/*

strip without soldermask:

points:

28---------------------------------29---------------------------------30
|                                                                      |
|                            upper_material                            |
|                                                                      |
|                   21-22-23-------24-------25-26-27                   |
|                   /             trace           \                    |
08------------09-10-11-12-13-------14-------15-16-17-18-19------------20
|                            lower_material                            |
01---------------02-03-------------04-------------05-06---------------07

lines:

.--------------31------------------.----------------32-----------------.
|                                                                      |
|                                  34                                  |
29                                                                    30 
|                   .23.24.---25---.---26---.27.28.                    |
|                   21                            22                   |
.------09-----.10.11.12.13.---14---.---15---.16.17.18.19.-----20-------.
07                                 33                                 08 
.------01--------.02.------03------.------04------.05.-------06--------.


strip with soldermask:

points: add new points starting at 31

28---------------------------------29---------------------------------30
|                                                                      |
|                            upper_material                            |
|                                                                      |
|                  35-36-37--------38--------39-40-41                  |
|                 /            soldermask            \                 |
31--------32-33-34  21-22-23-------24-------25-26-27  42-43-44--------45
|                   /             trace           \                    |
08------------09-10-11-12-13-------14-------15-16-17-18-19------------20
|                            lower_material                            |
01---------------02-03-------------04-------------05-06---------------07

lines: add new lines starting at 35.  Line definitions for 29, 30, and 34 change.

.--------------31------------------.----------------32-----------------.
|                                                                      |
|                                  34                                  |
29                                                                    30
|                 .41.42.----43-----.----44----.45.46.                 |
|               40                 51                 47               |
.----37---.38.39.    .23.24.---25---.---26---.27.28.    .48.49.---50---.
35                  21                            22                  36
.------09-----.10.11.12.13.---14---.---15---.16.17.18.19.-----20-------.
07                                 33                                 08
.------01--------.02.------03------.------04------.05.-------06--------.


*/

void Strip::build_geo (Geo *geo, bool check_error)
{
   double pi=4.*atan(1.);
   double etch_offset=trace_thickness.get_dbl_value()/tan(trace_etch_angle.get_dbl_value()*pi/180.);

   double trace_grid_inset=trace_thickness.get_dbl_value();

   // avoid spilling out of the trace width
   if (trace_width.get_dbl_value()-2*etch_offset-6*trace_grid_inset <= 0) {
      trace_grid_inset=(trace_width.get_dbl_value()-2*etch_offset)/6;
   }

   // avoid spilling out of the trace width
   if (trace_width.get_dbl_value()-2*etch_offset-4*trace_grid_inset <= 0) {
      trace_grid_inset=(trace_width.get_dbl_value()-2*etch_offset)/4;
   }

   // avoid overlaps for thick metals
   if (trace_grid_inset*3 > trace_width.get_dbl_value()/2) trace_grid_inset=trace_width.get_dbl_value()/6;

   // avoid spilling outside the side gaps
   if (trace_grid_inset*3 > left_side_gap.get_dbl_value()) trace_grid_inset=left_side_gap.get_dbl_value()/3;
   if (trace_grid_inset*3 > right_side_gap.get_dbl_value()) trace_grid_inset=right_side_gap.get_dbl_value()/3;

   double soldermask_offset=0;
   if (soldermask_thickness.is_loaded()) soldermask_offset=soldermask_thickness.get_dbl_value()/tan((180-trace_etch_angle.get_dbl_value())/2.*pi/180.);

   double gs1=min(left_side_gap.get_dbl_value()+right_side_gap.get_dbl_value()+trace_width.get_dbl_value(),upper_thickness.get_dbl_value()+lower_thickness.get_dbl_value())/4;
   double gs2=lower_thickness.get_dbl_value();
   double gs3=trace_thickness.get_dbl_value();

   bool not_symmetric=true;
   if (use_symmetry.get_bool_value()) not_symmetric=false;

   bool soldermask=false;
   if (soldermask_thickness.is_loaded())soldermask=true;

   geo->push_point(new Point(1, -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2, 0, gs1, true));
   geo->push_point(new Point(2, -trace_width.get_dbl_value()/2-min(lower_thickness.get_dbl_value(),left_side_gap.get_dbl_value()/2), 0, gs1, true));
   geo->push_point(new Point(3, -trace_width.get_dbl_value()/2, 0, gs2, true));
   geo->push_point(new Point(4, 0, 0, gs2, true));
   geo->push_point(new Point(5, trace_width.get_dbl_value()/2, 0, gs2, not_symmetric));
   geo->push_point(new Point(6, trace_width.get_dbl_value()/2+min(lower_thickness.get_dbl_value(),right_side_gap.get_dbl_value()/2), 0, gs1, not_symmetric)); 
   geo->push_point(new Point(7, right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2, 0, gs1, not_symmetric));
   geo->push_point(new Point(8, -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2, lower_thickness.get_dbl_value(), gs1, true));
   geo->push_point(new Point(9, -trace_width.get_dbl_value()/2-2*trace_grid_inset, lower_thickness.get_dbl_value(), gs1, true));
   geo->push_point(new Point(10, -trace_width.get_dbl_value()/2-trace_grid_inset, lower_thickness.get_dbl_value(), gs3, true));
   geo->push_point(new Point(11, -trace_width.get_dbl_value()/2, lower_thickness.get_dbl_value(), gs3, true));
   geo->push_point(new Point(12, -trace_width.get_dbl_value()/2+trace_grid_inset, lower_thickness.get_dbl_value(), gs3, true));
   geo->push_point(new Point(13, -trace_width.get_dbl_value()/2+2*trace_grid_inset, lower_thickness.get_dbl_value(), gs2, true));
   geo->push_point(new Point(14, 0, lower_thickness.get_dbl_value(), gs2, true));
   geo->push_point(new Point(15, trace_width.get_dbl_value()/2-2*trace_grid_inset, lower_thickness.get_dbl_value(), gs2, not_symmetric));
   geo->push_point(new Point(16, trace_width.get_dbl_value()/2-trace_grid_inset, lower_thickness.get_dbl_value(), gs3, not_symmetric));
   geo->push_point(new Point(17, trace_width.get_dbl_value()/2, lower_thickness.get_dbl_value(), gs3, not_symmetric));
   geo->push_point(new Point(18, trace_width.get_dbl_value()/2+trace_grid_inset, lower_thickness.get_dbl_value(), gs3, not_symmetric));
   geo->push_point(new Point(19, trace_width.get_dbl_value()/2+2*trace_grid_inset, lower_thickness.get_dbl_value(), gs1, not_symmetric));
   geo->push_point(new Point(20, right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2, lower_thickness.get_dbl_value(), gs1, not_symmetric));
   geo->push_point(new Point(21, -trace_width.get_dbl_value()/2+etch_offset, lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs3, true));
   geo->push_point(new Point(22, -trace_width.get_dbl_value()/2+etch_offset+trace_grid_inset, lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs3, true));
   geo->push_point(new Point(23, -trace_width.get_dbl_value()/2+etch_offset+2*trace_grid_inset, lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs2, true));
   geo->push_point(new Point(24, 0, lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs2, true));
   geo->push_point(new Point(25, trace_width.get_dbl_value()/2-etch_offset-2*trace_grid_inset, lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs2, not_symmetric));
   geo->push_point(new Point(26, trace_width.get_dbl_value()/2-etch_offset-trace_grid_inset, lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs3, not_symmetric));
   geo->push_point(new Point(27, trace_width.get_dbl_value()/2-etch_offset, lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs3, not_symmetric));
   geo->push_point(new Point(28, -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2, lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value(), gs1, true));
   geo->push_point(new Point(29, 0, lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value(), gs1, true));
   geo->push_point(new Point(30, right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2, lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value(), gs1, not_symmetric));
   geo->push_point(new Point(31, -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value(), gs1, soldermask));
   geo->push_point(new Point(32, -trace_width.get_dbl_value()/2-soldermask_offset-2*trace_grid_inset, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value(), gs1, soldermask));
   geo->push_point(new Point(33, -trace_width.get_dbl_value()/2-soldermask_offset-trace_grid_inset, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value(), gs3, soldermask));
   geo->push_point(new Point(34, -trace_width.get_dbl_value()/2-soldermask_offset, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value(), gs3, soldermask));
   geo->push_point(new Point(35, -trace_width.get_dbl_value()/2-soldermask_offset+etch_offset, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs3, soldermask));
   geo->push_point(new Point(36, -trace_width.get_dbl_value()/2-soldermask_offset+etch_offset+trace_grid_inset, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs3, soldermask));
   geo->push_point(new Point(37, -trace_width.get_dbl_value()/2-soldermask_offset+etch_offset+2*trace_grid_inset, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs1, soldermask));
   geo->push_point(new Point(38, 0, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs1, soldermask));
   geo->push_point(new Point(39, +trace_width.get_dbl_value()/2+soldermask_offset-etch_offset-2*trace_grid_inset, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs1, not_symmetric && soldermask));
   geo->push_point(new Point(40, +trace_width.get_dbl_value()/2+soldermask_offset-etch_offset-trace_grid_inset, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs3, not_symmetric && soldermask));
   geo->push_point(new Point(41, +trace_width.get_dbl_value()/2+soldermask_offset-etch_offset, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value()+trace_thickness.get_dbl_value(), gs3, not_symmetric && soldermask));
   geo->push_point(new Point(42, +trace_width.get_dbl_value()/2+soldermask_offset, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value(), gs3, not_symmetric && soldermask));
   geo->push_point(new Point(43, +trace_width.get_dbl_value()/2+soldermask_offset+trace_grid_inset, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value(), gs3, not_symmetric && soldermask));
   geo->push_point(new Point(44, +trace_width.get_dbl_value()/2+soldermask_offset+2*trace_grid_inset, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value(), gs1, not_symmetric && soldermask));
   geo->push_point(new Point(45, right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2, lower_thickness.get_dbl_value()+soldermask_thickness.get_dbl_value(), gs1, not_symmetric && soldermask));

   // improve point positioning
   if (soldermask) {
      double start=geo->find_point(31)->get_x();
      double end=geo->find_point(34)->get_x();
      if (geo->find_point(32)->get_x() <= start) {
         geo->find_point(32)->set_x(start+(end-start)/3);
         geo->find_point(33)->set_x(start+2*(end-start)/3);
      }

      if (!use_symmetry.get_bool_value()) {
         start=geo->find_point(42)->get_x();
         end=geo->find_point(45)->get_x();
         if (geo->find_point(44)->get_x() >= end) {
            geo->find_point(43)->set_x(start+(end-start)/3);
            geo->find_point(44)->set_x(start+2*(end-start)/3);
         }
      }
   }

   // take out dip, if necessary
   if (soldermask) {
      if (geo->find_point(34)->get_x() <= geo->find_point(31)->get_x()) {
         double gap=geo->find_point(35)->get_x()-geo->find_point(31)->get_x();
         geo->find_point(31)->set_y(geo->find_point(35)->get_y());
         geo->find_point(32)->set_x(geo->find_point(31)->get_x()+gap/4);
         geo->find_point(32)->set_y(geo->find_point(35)->get_y());
         geo->find_point(33)->set_x(geo->find_point(31)->get_x()+2*gap/4);
         geo->find_point(33)->set_y(geo->find_point(35)->get_y());
         geo->find_point(34)->set_x(geo->find_point(31)->get_x()+3*gap/4);
         geo->find_point(34)->set_y(geo->find_point(35)->get_y());
      }

      if (!use_symmetry.get_bool_value()) {
         if (geo->find_point(45)->get_x() <= geo->find_point(42)->get_x()) {
            double gap=geo->find_point(45)->get_x()-geo->find_point(41)->get_x();
            geo->find_point(42)->set_x(geo->find_point(41)->get_x()+gap/4);
            geo->find_point(42)->set_y(geo->find_point(41)->get_y());
            geo->find_point(43)->set_x(geo->find_point(41)->get_x()+2*gap/4);
            geo->find_point(43)->set_y(geo->find_point(41)->get_y());
            geo->find_point(44)->set_x(geo->find_point(41)->get_x()+3*gap/4);
            geo->find_point(44)->set_y(geo->find_point(41)->get_y());
            geo->find_point(45)->set_y(geo->find_point(41)->get_y());
         }
      }
   }


   geo->push_line(new Line(1, geo->get_point(0), geo->get_point(1), true));
   geo->push_line(new Line(2, geo->get_point(1), geo->get_point(2), true));
   geo->push_line(new Line(3, geo->get_point(2), geo->get_point(3), true));
   geo->push_line(new Line(4, geo->get_point(3), geo->get_point(4), not_symmetric));
   geo->push_line(new Line(5, geo->get_point(4), geo->get_point(5), not_symmetric));
   geo->push_line(new Line(6, geo->get_point(5), geo->get_point(6), not_symmetric));
   geo->push_line(new Line(7, geo->get_point(0), geo->get_point(7), true));
   geo->push_line(new Line(8, geo->get_point(6), geo->get_point(19), not_symmetric));
   geo->push_line(new Line(9, geo->get_point(7), geo->get_point(8), true));
   geo->push_line(new Line(10, geo->get_point(8), geo->get_point(9), true));
   geo->push_line(new Line(11, geo->get_point(9), geo->get_point(10), true));
   geo->push_line(new Line(12, geo->get_point(10), geo->get_point(11), true));
   geo->push_line(new Line(13, geo->get_point(11), geo->get_point(12), true));
   geo->push_line(new Line(14, geo->get_point(12), geo->get_point(13), true));
   geo->push_line(new Line(15, geo->get_point(13), geo->get_point(14), not_symmetric));
   geo->push_line(new Line(16, geo->get_point(14), geo->get_point(15), not_symmetric));
   geo->push_line(new Line(17, geo->get_point(15), geo->get_point(16), not_symmetric));
   geo->push_line(new Line(18, geo->get_point(16), geo->get_point(17), not_symmetric));
   geo->push_line(new Line(19, geo->get_point(17), geo->get_point(18), not_symmetric));
   geo->push_line(new Line(20, geo->get_point(18), geo->get_point(19), not_symmetric));
   geo->push_line(new Line(21, geo->get_point(10), geo->get_point(20), true));
   geo->push_line(new Line(22, geo->get_point(16), geo->get_point(26), not_symmetric));
   geo->push_line(new Line(23, geo->get_point(20), geo->get_point(21), true));
   geo->push_line(new Line(24, geo->get_point(21), geo->get_point(22), true));
   geo->push_line(new Line(25, geo->get_point(22), geo->get_point(23), true));
   geo->push_line(new Line(26, geo->get_point(23), geo->get_point(24), not_symmetric));
   geo->push_line(new Line(27, geo->get_point(24), geo->get_point(25), not_symmetric));
   geo->push_line(new Line(28, geo->get_point(25), geo->get_point(26), not_symmetric));
   geo->push_line(new Line(29, geo->get_point(30), geo->get_point(27), soldermask));
   geo->push_line(new Line(29, geo->get_point(7), geo->get_point(27), ! soldermask));
   geo->push_line(new Line(30, geo->get_point(44), geo->get_point(29), not_symmetric && soldermask));
   geo->push_line(new Line(30, geo->get_point(19), geo->get_point(29), not_symmetric && ! soldermask));
   geo->push_line(new Line(31, geo->get_point(27), geo->get_point(28), true));
   geo->push_line(new Line(32, geo->get_point(28), geo->get_point(29), not_symmetric));
   geo->push_line(new Line(33, geo->get_point(3), geo->get_point(13), ! not_symmetric));
   geo->push_line(new Line(34, geo->get_point(37), geo->get_point(28), ! not_symmetric && soldermask));
   geo->push_line(new Line(34, geo->get_point(23), geo->get_point(28), ! not_symmetric && ! soldermask));
   geo->push_line(new Line(35, geo->get_point(7), geo->get_point(30), soldermask));
   geo->push_line(new Line(36, geo->get_point(19), geo->get_point(44), not_symmetric && soldermask));
   geo->push_line(new Line(37, geo->get_point(30), geo->get_point(31), soldermask));
   geo->push_line(new Line(38, geo->get_point(31), geo->get_point(32), soldermask));
   geo->push_line(new Line(39, geo->get_point(32), geo->get_point(33), soldermask));
   geo->push_line(new Line(40, geo->get_point(33), geo->get_point(34), soldermask));
   geo->push_line(new Line(41, geo->get_point(34), geo->get_point(35), soldermask));
   geo->push_line(new Line(42, geo->get_point(35), geo->get_point(36), soldermask));
   geo->push_line(new Line(43, geo->get_point(36), geo->get_point(37), soldermask));
   geo->push_line(new Line(51, geo->get_point(23), geo->get_point(37), ! not_symmetric && soldermask));
   geo->push_line(new Line(44, geo->get_point(37), geo->get_point(38), not_symmetric && soldermask));
   geo->push_line(new Line(45, geo->get_point(38), geo->get_point(39), not_symmetric && soldermask));
   geo->push_line(new Line(46, geo->get_point(39), geo->get_point(40), not_symmetric && soldermask));
   geo->push_line(new Line(47, geo->get_point(40), geo->get_point(41), not_symmetric && soldermask));
   geo->push_line(new Line(48, geo->get_point(41), geo->get_point(42), not_symmetric && soldermask));
   geo->push_line(new Line(49, geo->get_point(42), geo->get_point(43), not_symmetric && soldermask));
   geo->push_line(new Line(50, geo->get_point(43), geo->get_point(44), not_symmetric && soldermask));

   // lower
   Curve *newCurve=new Curve(1,lower_material.get_value(),true);
   if (use_symmetry.get_bool_value()) {
      newCurve->push_line(geo->find_line(1),false);
      newCurve->push_line(geo->find_line(2),false);
      newCurve->push_line(geo->find_line(3),false);
      newCurve->push_line(geo->find_line(33),false);
      newCurve->push_line(geo->find_line(14),true);
      newCurve->push_line(geo->find_line(13),true);
      newCurve->push_line(geo->find_line(12),true);
      newCurve->push_line(geo->find_line(11),true);
      newCurve->push_line(geo->find_line(10),true);
      newCurve->push_line(geo->find_line(9),true);
      newCurve->push_line(geo->find_line(7),true);
   } else {
      newCurve->push_line(geo->find_line(1),false);
      newCurve->push_line(geo->find_line(2),false);
      newCurve->push_line(geo->find_line(3),false);
      newCurve->push_line(geo->find_line(4),false);
      newCurve->push_line(geo->find_line(5),false);
      newCurve->push_line(geo->find_line(6),false);
      newCurve->push_line(geo->find_line(8),false);
      newCurve->push_line(geo->find_line(20),true);
      newCurve->push_line(geo->find_line(19),true);
      newCurve->push_line(geo->find_line(18),true);
      newCurve->push_line(geo->find_line(17),true);
      newCurve->push_line(geo->find_line(16),true);
      newCurve->push_line(geo->find_line(15),true);
      newCurve->push_line(geo->find_line(14),true);
      newCurve->push_line(geo->find_line(13),true);
      newCurve->push_line(geo->find_line(12),true);
      newCurve->push_line(geo->find_line(11),true);
      newCurve->push_line(geo->find_line(10),true);
      newCurve->push_line(geo->find_line(9),true);
      newCurve->push_line(geo->find_line(7),true);
   }
   geo->push_curve(newCurve);

   // upper
   newCurve=new Curve(2,upper_material.get_value(),true);
   if (soldermask) {
      if (use_symmetry.get_bool_value()) {
         newCurve->push_line(geo->find_line(37),false);
         newCurve->push_line(geo->find_line(38),false);
         newCurve->push_line(geo->find_line(39),false);
         newCurve->push_line(geo->find_line(40),false);
         newCurve->push_line(geo->find_line(41),false);
         newCurve->push_line(geo->find_line(42),false);
         newCurve->push_line(geo->find_line(43),false);
         newCurve->push_line(geo->find_line(34),false);
         newCurve->push_line(geo->find_line(31),true);
         newCurve->push_line(geo->find_line(29),true);
      } else {
         newCurve->push_line(geo->find_line(37),false);
         newCurve->push_line(geo->find_line(38),false);
         newCurve->push_line(geo->find_line(39),false);
         newCurve->push_line(geo->find_line(40),false);
         newCurve->push_line(geo->find_line(41),false);
         newCurve->push_line(geo->find_line(42),false);
         newCurve->push_line(geo->find_line(43),false);
         newCurve->push_line(geo->find_line(44),false);
         newCurve->push_line(geo->find_line(45),false);
         newCurve->push_line(geo->find_line(46),false);
         newCurve->push_line(geo->find_line(47),false);
         newCurve->push_line(geo->find_line(48),false);
         newCurve->push_line(geo->find_line(49),false);
         newCurve->push_line(geo->find_line(50),false);
         newCurve->push_line(geo->find_line(30),false);
         newCurve->push_line(geo->find_line(32),true);
         newCurve->push_line(geo->find_line(31),true);
         newCurve->push_line(geo->find_line(29),true);
      }
   } else {
      if (use_symmetry.get_bool_value()) {
         newCurve->push_line(geo->find_line(9),false);
         newCurve->push_line(geo->find_line(10),false);
         newCurve->push_line(geo->find_line(11),false);
         newCurve->push_line(geo->find_line(21),false);
         newCurve->push_line(geo->find_line(23),false);
         newCurve->push_line(geo->find_line(24),false);
         newCurve->push_line(geo->find_line(25),false);
         newCurve->push_line(geo->find_line(34),false);
         newCurve->push_line(geo->find_line(31),true);
         newCurve->push_line(geo->find_line(29),true);
      } else {
         newCurve->push_line(geo->find_line(9),false);
         newCurve->push_line(geo->find_line(10),false);
         newCurve->push_line(geo->find_line(11),false);
         newCurve->push_line(geo->find_line(21),false);
         newCurve->push_line(geo->find_line(23),false);
         newCurve->push_line(geo->find_line(24),false);
         newCurve->push_line(geo->find_line(25),false);
         newCurve->push_line(geo->find_line(26),false);
         newCurve->push_line(geo->find_line(27),false);
         newCurve->push_line(geo->find_line(28),false);
         newCurve->push_line(geo->find_line(22),true);
         newCurve->push_line(geo->find_line(18),false);
         newCurve->push_line(geo->find_line(19),false);
         newCurve->push_line(geo->find_line(20),false);
         newCurve->push_line(geo->find_line(30),false);
         newCurve->push_line(geo->find_line(32),true);
         newCurve->push_line(geo->find_line(31),true);
         newCurve->push_line(geo->find_line(29),true);
      }
   }
   geo->push_curve(newCurve);

   // soldermask
   newCurve=new Curve(-1,soldermask_material.get_value(),false);
   if (soldermask) {
      newCurve->set_active();
      newCurve->set_index(3);
      if (use_symmetry.get_bool_value()) {
         newCurve->push_line(geo->find_line(9),false);
         newCurve->push_line(geo->find_line(10),false);
         newCurve->push_line(geo->find_line(11),false);
         newCurve->push_line(geo->find_line(21),false);
         newCurve->push_line(geo->find_line(23),false);
         newCurve->push_line(geo->find_line(24),false);
         newCurve->push_line(geo->find_line(25),false);
         newCurve->push_line(geo->find_line(51),false);
         newCurve->push_line(geo->find_line(43),true);
         newCurve->push_line(geo->find_line(42),true);
         newCurve->push_line(geo->find_line(41),true);
         newCurve->push_line(geo->find_line(40),true);
         newCurve->push_line(geo->find_line(39),true);
         newCurve->push_line(geo->find_line(38),true);
         newCurve->push_line(geo->find_line(37),true);
         newCurve->push_line(geo->find_line(35),true);
      } else {
         newCurve->push_line(geo->find_line(9),false);
         newCurve->push_line(geo->find_line(10),false);
         newCurve->push_line(geo->find_line(11),false);
         newCurve->push_line(geo->find_line(21),false);
         newCurve->push_line(geo->find_line(23),false);
         newCurve->push_line(geo->find_line(24),false);
         newCurve->push_line(geo->find_line(25),false);
         newCurve->push_line(geo->find_line(26),false);
         newCurve->push_line(geo->find_line(27),false);
         newCurve->push_line(geo->find_line(28),false);
         newCurve->push_line(geo->find_line(22),true);
         newCurve->push_line(geo->find_line(18),false);
         newCurve->push_line(geo->find_line(19),false);
         newCurve->push_line(geo->find_line(20),false);
         newCurve->push_line(geo->find_line(36),false);
         newCurve->push_line(geo->find_line(50),true);
         newCurve->push_line(geo->find_line(49),true);
         newCurve->push_line(geo->find_line(48),true);
         newCurve->push_line(geo->find_line(47),true);
         newCurve->push_line(geo->find_line(46),true);
         newCurve->push_line(geo->find_line(45),true);
         newCurve->push_line(geo->find_line(44),true);
         newCurve->push_line(geo->find_line(43),true);
         newCurve->push_line(geo->find_line(42),true);
         newCurve->push_line(geo->find_line(41),true);
         newCurve->push_line(geo->find_line(40),true);
         newCurve->push_line(geo->find_line(39),true);
         newCurve->push_line(geo->find_line(38),true);
         newCurve->push_line(geo->find_line(37),true);
         newCurve->push_line(geo->find_line(35),true);
      }
   }
   geo->push_curve(newCurve);

   if (check_error && check_geo(geo)) {
      PetscPrintf(PETSC_COMM_WORLD,"ERROR2055: Invalid constructed geometry.  Check the geo file output in gmsh.\n");
   }

   // 3D
   geo->extrude(length.get_dbl_value());
}

bool Strip::write_geo (string indent, Control *control, Geo *geo)
{
   bool fail=false;
   stringstream ssFilename;
   ssFilename << name.get_value() << ".geo";

   ofstream out;
   out.open (ssFilename.str().c_str(),ofstream::out);
   if (out.is_open()) {
      cout << "Writing \"" << ssFilename.str() << "\" ..." << endl;

      control->print_commented(&out);
      print_commented(&out);

      geo->write(&out);

      out.close();
   } else {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2056: Failed to open \"%s\" for writing.\n",
                                   indent.c_str(),ssFilename.str().c_str());
      fail=true;
   }
   return fail;
}

bool Strip::write_modes_and_boundaries (string indent, Control *control)
{
   if (length.get_dbl_value() != 0) return false;

   bool fail=false;
   stringstream ssFilename;
   ssFilename << name.get_value() << "_lines.txt";

   ofstream modes;
   modes.open (ssFilename.str().c_str(),ofstream::out);
   if (modes.is_open()) {
      cout << "Writing \"" << ssFilename.str() << "\" ..." << endl;

      modes << "#OpenParEMmodes 1.0" << endl << endl;

      control->print_commented(&modes);
      print_commented(&modes);

      modes << endl;
      modes << "File" << endl;
      modes << "   name=generated_by_builder" << endl;
      modes << "EndFile" << endl;

      string boundary_material;

      double pi=4.*atan(1.);
      double etch_offset=trace_thickness.get_dbl_value()/tan(trace_etch_angle.get_dbl_value()*pi/180.);

      // voltage

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=voltage_line" << endl;
      modes << setprecision(15) << "   point=(" << 0 << "," << 0 << ")" << endl;
      modes << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      modes << endl;
      modes << "Mode" << endl;
      modes << "   mode=1" << endl;
      modes << "   type=voltage" << endl;
      modes << "   path=voltage_line" << endl;
      modes << "EndMode" << endl;

      // symmetry plane

      if (use_symmetry.get_bool_value()) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=symmetry_line" << endl;
         modes << setprecision(15) << "   point=(" << 0 << "," << 0 << ")" << endl;
         modes << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=symmetry" << endl;
         modes << "   type=perfect_magnetic_conductor" << endl;
         modes << "   path=symmetry_line" << endl;
         modes << "EndBoundary" << endl;
      }

      // impedance boundaries 

      // trace top

      if (trace_material_top.is_loaded()) boundary_material=trace_material_top.get_value();
      else boundary_material=default_conductor_material.get_value();

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=trace_top" << endl;
      modes << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
      if (use_symmetry.get_bool_value()) modes << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
      else modes << setprecision(15) << "   point=(" << +trace_width.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=trace_top" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=trace_top" << endl;
         modes << "EndBoundary" << endl;
      }

      // trace bottom

      if (trace_material_bottom.is_loaded()) boundary_material=trace_material_bottom.get_value();
      else boundary_material=default_conductor_material.get_value();

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=trace_bottom" << endl;
      modes << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      if (use_symmetry.get_bool_value()) modes << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      else modes << setprecision(15) << "   point=(" << +trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=trace_bottom" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=trace_bottom" << endl;
         modes << "EndBoundary" << endl;
      }

      // trace left

      if (trace_material_sides.is_loaded()) boundary_material=trace_material_sides.get_value();
      else boundary_material=default_conductor_material.get_value();

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=trace_left" << endl;
      modes << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=trace_left" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=trace_left" << endl;
         modes << "EndBoundary" << endl;
      }

      // trace right

      if (trace_material_sides.is_loaded()) boundary_material=trace_material_sides.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (!use_symmetry.get_bool_value()) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=trace_right" << endl;
         modes << setprecision(15) << "   point=(" << trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
         modes << setprecision(15) << "   point=(" << trace_width.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;
      }

      if (!use_symmetry.get_bool_value() && boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=trace_right" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=trace_right" << endl;
         modes << "EndBoundary" << endl;
      }

      // lower groundplane

      if (lower_groundplane_material.is_loaded()) boundary_material=lower_groundplane_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=ground_plane" << endl;
         modes << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2 << "," << 0 << ")" << endl;
         if (use_symmetry.get_bool_value()) modes << setprecision(15) << "   point=(" << 0 << "," << 0 << ")" << endl;
         else modes << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2 << "," << 0 << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=ground_plane" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=ground_plane" << endl;
         modes << "EndBoundary" << endl;
      }

      // upper groundplane

      if (upper_groundplane_material.is_loaded()) boundary_material=upper_groundplane_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=upper_groundplane" << endl;
         modes << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << ")" << endl;
         if (use_symmetry.get_bool_value()) modes << setprecision(15) << "   point=(" << 0 << "," <<  lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << ")" << endl;
         else modes << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value()  << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=upper_groundplane" << endl;
         if (boundary_material.compare("PMC") == 0) {
            modes << "   type=perfect_magnetic_conductor" << endl;
         } else {
            modes << "   type=surface_impedance" << endl;
            modes << "   material=" << boundary_material << endl;
         }
         modes << "   path=upper_groundplane" << endl;
         modes << "EndBoundary" << endl;
      }

      // left side

      if (left_side_material.is_loaded()) boundary_material=left_side_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=left_side" << endl;
         modes << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2 << "," << 0 << ")" << endl;
         modes << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value()  << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=left_side" << endl;
         if (boundary_material.compare("PMC") == 0) {
           modes << "   type=perfect_magnetic_conductor" << endl;
         } else {
            modes << "   type=surface_impedance" << endl;
            modes << "   material=" << boundary_material << endl;
         }
         modes << "   path=left_side" << endl;
         modes << "EndBoundary" << endl;
      }

      // right side

      if (right_side_material.is_loaded()) boundary_material=right_side_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0 && !use_symmetry.get_bool_value()) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=right_side" << endl;
         modes << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2 << "," << 0 << ")" << endl;
         modes << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value()  << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=right_side" << endl;
         if (boundary_material.compare("PMC") == 0) {
           modes << "   type=perfect_magnetic_conductor" << endl;
         } else {
            modes << "   type=surface_impedance" << endl;
            modes << "   material=" << boundary_material << endl;
         }
         modes << "   path=right_side" << endl;
         modes << "EndBoundary" << endl;
      }

      // current
      if (use_symmetry.get_bool_value()) {
         modes << endl;
         modes << "Mode" << endl;
         modes << "   mode=1" << endl;
         modes << "   type=current" << endl;
         modes << "   path=+trace_top" << endl;
         modes << "   path+=trace_bottom" << endl;
         modes << "   path+=trace_left" << endl;
         modes << "EndMode" << endl;
      } else {
         modes << endl;
         modes << "Mode" << endl;
         modes << "   mode=1" << endl;
         modes << "   type=current" << endl;
         modes << "   path=+trace_top" << endl;
         modes << "   path+=trace_right" << endl;
         modes << "   path+=trace_bottom" << endl;
         modes << "   path+=trace_left" << endl;
         modes << "EndMode" << endl;
      }

      modes.close();
   } else {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2057: Failed to open \"%s\" for writing.\n",
                                   indent.c_str(),ssFilename.str().c_str());
      fail=true;
   }
   return fail;
}

bool Strip::write_ports_and_boundaries (string indent, Control *control)
{
   if (length.get_dbl_value() == 0) return false;

   bool fail=false;
   stringstream ssFilename;
   ssFilename << name.get_value() << "_ports.txt";

   ofstream ports;
   ports.open (ssFilename.str().c_str(),ofstream::out);
   if (ports.is_open()) {
      cout << "Writing \"" << ssFilename.str() << "\" ..." << endl;

      ports << "#OpenParEMports 1.0" << endl << endl;

      control->print_commented(&ports);
      print_commented(&ports);

      ports << endl;
      ports << "File" << endl;
      ports << "   name=generated_by_builder" << endl;
      ports << "EndFile" << endl;

      string boundary_material;

      double pi=4.*atan(1.);
      double etch_offset=trace_thickness.get_dbl_value()/tan(trace_etch_angle.get_dbl_value()*pi/180.);

      // voltage paths

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=V1" << endl;
      ports << setprecision(15) << "   point=(" << 0 << "," << 0 << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << "   closed=false" << endl;
      ports << "EndPath" << endl;

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=V2" << endl;
      ports << setprecision(15) << "   point=(" << 0 << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << "   closed=false" << endl;
      ports << "EndPath" << endl;

      // current paths

      if (use_symmetry.get_bool_value()) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=I1" << endl;
         ports << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=false" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Path" << endl;
         ports << "   name=I2" << endl;
         ports << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << "   closed=false" << endl;
         ports << "EndPath" << endl;
      } else {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=I1" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << +trace_width.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << +trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Path" << endl;
         ports << "   name=I2" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << +trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << +trace_width.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;
      }

      // port paths

      double right=trace_width.get_dbl_value()/2+right_side_gap.get_dbl_value();
      if (use_symmetry.get_bool_value()) right=0;

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=port1" << endl;
      ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2-left_side_gap.get_dbl_value() << "," << 0 << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2-left_side_gap.get_dbl_value() << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << right << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << right << "," << 0 << "," << 0 << ")" << endl;
      ports << "   closed=true" << endl;
      ports << "EndPath" << endl;

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=port2" << endl;
      ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2-left_side_gap.get_dbl_value() << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2-left_side_gap.get_dbl_value() << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << right << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << right << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
      ports << "   closed=true" << endl;
      ports << "EndPath" << endl;

      // ports

      ports << endl;
      ports << "Port" << endl;
      ports << "   name=1" << endl;
      ports << "   path=+port1" << endl;
      ports << "      impedance_definition=PI" << endl;
      ports << "      impedance_calculation=line" << endl;
      ports << "   Line" << endl;
      ports << "      Sport=1" << endl;
      ports << "      net=in" << endl;
      ports << "      IntegrationPath" << endl;
      ports << "         type=voltage" << endl;
      ports << "         path=+V1" << endl;
      ports << "      EndIntegrationPath" << endl;
      ports << "      IntegrationPath" << endl;
      ports << "         type=current" << endl;
      ports << "         path=+I1" << endl;
      ports << "      EndIntegrationPath" << endl;
      ports << "   EndLine" << endl;
      ports << "EndPort" << endl;

      ports << endl;
      ports << "Port" << endl;
      ports << "   name=2" << endl;
      ports << "   path=+port2" << endl;
      ports << "      impedance_definition=PI" << endl;
      ports << "      impedance_calculation=line" << endl;
      ports << "   Line" << endl;
      ports << "      Sport=2" << endl;
      ports << "      net=out" << endl;
      ports << "      IntegrationPath" << endl;
      ports << "         type=voltage" << endl;
      ports << "         path=+V2" << endl;
      ports << "      EndIntegrationPath" << endl;
      ports << "      IntegrationPath" << endl;
      ports << "         type=current" << endl;
      ports << "         path=+I2" << endl;
      ports << "      EndIntegrationPath" << endl;
      ports << "   EndLine" << endl;
      ports << "EndPort" << endl;

      // impedance boundaries

      // trace top

      if (trace_material_top.is_loaded()) boundary_material=trace_material_top.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {

         ports << endl;
         ports << "Path" << endl;
         ports << "   name=trace_top" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         if (use_symmetry.get_bool_value()) ports << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         else ports << setprecision(15) << "   point=(" << +trace_width.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         if (use_symmetry.get_bool_value()) ports << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         else ports << setprecision(15) << "   point=(" << +trace_width.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=trace_top" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=trace_top" << endl;
         ports << "EndBoundary" << endl;
      }
      // trace bottom

      if (trace_material_bottom.is_loaded()) boundary_material=trace_material_bottom.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {

         ports << endl;
         ports << "Path" << endl;
         ports << "   name=trace_bottom" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         if (use_symmetry.get_bool_value()) ports << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         else ports << setprecision(15) << "   point=(" << +trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         if (use_symmetry.get_bool_value()) ports << setprecision(15) << "   point=(" << 0 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         else ports << setprecision(15) << "   point=(" << +trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=trace_bottom" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=trace_bottom" << endl;
         ports << "EndBoundary" << endl;
      }

      // trace left

      if (trace_material_sides.is_loaded()) boundary_material=trace_material_sides.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {

         ports << endl;
         ports << "Path" << endl;
         ports << "   name=trace_left" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_width.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=trace_left" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=trace_left" << endl;
         ports << "EndBoundary" << endl;
      }
      // trace right

      if (trace_material_sides.is_loaded()) boundary_material=trace_material_sides.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (!use_symmetry.get_bool_value()  && boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=trace_right" << endl;
         ports << setprecision(15) << "   point=(" << trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_width.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_width.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=trace_right" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=trace_right" << endl;
         ports << "EndBoundary" << endl;
      }

      // lower groundplane

      if (lower_groundplane_material.is_loaded()) boundary_material=lower_groundplane_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=ground_plane" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2 << "," << 0 << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2 << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
         if (use_symmetry.get_bool_value()) ports << setprecision(15) << "   point=(" << 0 << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
         else ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2 << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
         if (use_symmetry.get_bool_value()) ports << setprecision(15) << "   point=(" << 0 << "," << 0 << "," << 0 << ")" << endl;
         else ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2 << "," << 0 << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=ground_plane" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=ground_plane" << endl;
         ports << "EndBoundary" << endl;
      }
      // upper groundplane

      if (upper_groundplane_material.is_loaded()) boundary_material=upper_groundplane_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=upper_groundplane" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         if (use_symmetry.get_bool_value()) ports << setprecision(15) << "   point=(" << 0 << "," <<  lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         else ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value()  << "," << length.get_dbl_value() << ")" << endl;
         if (use_symmetry.get_bool_value()) ports << setprecision(15) << "   point=(" << 0 << "," <<  lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         else ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value()  << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=upper_groundplane" << endl;
         if (boundary_material.compare("PMC") == 0) {
            ports << "   type=perfect_magnetic_conductor" << endl;
         } else {
            ports << "   type=surface_impedance" << endl;
            ports << "   material=" << boundary_material << endl;
         }
         ports << "   path=upper_groundplane" << endl;
         ports << "EndBoundary" << endl;
      }

      // left side

      if (left_side_material.is_loaded()) boundary_material=left_side_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=left_side" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2 << "," << 0 << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2 << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=left_side" << endl;
         if (boundary_material.compare("PMC") == 0) {
           ports << "   type=perfect_magnetic_conductor" << endl;
         } else {
            ports << "   type=surface_impedance" << endl;
            ports << "   material=" << boundary_material << endl;
         }
         ports << "   path=left_side" << endl;
         ports << "EndBoundary" << endl;
      }
      // right side

      if (right_side_material.is_loaded()) boundary_material=right_side_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0 && !use_symmetry.get_bool_value()) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=right_side" << endl;
         ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2 << "," << 0 << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2 << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_width.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=right_side" << endl;
         if (boundary_material.compare("PMC") == 0) {
           ports << "   type=perfect_magnetic_conductor" << endl;
         } else {
            ports << "   type=surface_impedance" << endl;
            ports << "   material=" << boundary_material << endl;
         }
         ports << "   path=right_side" << endl;
         ports << "EndBoundary" << endl;
      }

      ports.close();
   } else {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2190: Failed to open \"%s\" for writing.\n",
                                   indent.c_str(),ssFilename.str().c_str());
      fail=true;
   }
   return fail;
}

bool Strip::write_OpenParEM2D_proj(string indent)
{
   if (length.get_dbl_value() != 0) return false;
   return write_OpenParEM2D_proj_text (indent,name.get_value(),1,"PI","line");
}

bool Strip::write_OpenParEM3D_proj(string indent)
{
   if (length.get_dbl_value() == 0) return false;
   return write_OpenParEM3D_proj_text (indent,name.get_value(),&default_conductor_material);
}

///////////////////////////////////////////////////////////////////////////////////////////
// CoupledStrip
///////////////////////////////////////////////////////////////////////////////////////////

CoupledStrip::CoupledStrip (int startLine_, int endLine_, bool checkLimits_)
{
   startLine=startLine_;
   endLine=endLine_;

   // name

   name.push_alias("name");
   name.set_loaded(false);
   name.set_positive_required(false);
   name.set_non_negative_required(false);
   name.set_lowerLimit(0);
   name.set_upperLimit(1e12);
   name.set_checkLimits(checkLimits_);

   // include

   include.push_alias("include");
   include.set_loaded(false);
   include.set_positive_required(false);
   include.set_non_negative_required(false);
   include.set_lowerLimit(0);
   include.set_upperLimit(1e12);
   include.set_checkLimits(checkLimits_);

   // solution_impedance_calculation

   solution_impedance_calculation.push_alias("solution_impedance_calculation");
   solution_impedance_calculation.set_loaded(false);
   solution_impedance_calculation.set_positive_required(false);
   solution_impedance_calculation.set_non_negative_required(false);
   solution_impedance_calculation.set_lowerLimit(0);
   solution_impedance_calculation.set_upperLimit(1e12);
   solution_impedance_calculation.set_checkLimits(checkLimits_);

   // upper_material

   upper_material.push_alias("upper_material");
   upper_material.set_loaded(false);
   upper_material.set_positive_required(false);
   upper_material.set_non_negative_required(false);
   upper_material.set_lowerLimit(0);
   upper_material.set_upperLimit(1e12);
   upper_material.set_checkLimits(checkLimits_);

   // upper_thickness

   upper_thickness.push_alias("upper_thickness");
   upper_thickness.set_loaded(false);
   upper_thickness.set_positive_required(false);
   upper_thickness.set_non_negative_required(true);
   upper_thickness.set_lowerLimit(0);
   upper_thickness.set_upperLimit(0.1);
   upper_thickness.set_checkLimits(checkLimits_);

   // soldermask_thickness

   soldermask_thickness.push_alias("soldermask_thickness");
   soldermask_thickness.set_loaded(false);
   soldermask_thickness.set_positive_required(true);
   soldermask_thickness.set_non_negative_required(false);
   soldermask_thickness.set_lowerLimit(1e-6);
   soldermask_thickness.set_upperLimit(0.1);
   soldermask_thickness.set_checkLimits(checkLimits_);

   // soldermask_material

   soldermask_material.push_alias("soldermask_material");
   soldermask_material.set_loaded(false);
   soldermask_material.set_positive_required(false);
   soldermask_material.set_non_negative_required(false);
   soldermask_material.set_lowerLimit(0);
   soldermask_material.set_upperLimit(1e12);
   soldermask_material.set_checkLimits(checkLimits_);

   // lower_thickness

   lower_thickness.push_alias("lower_thickness");
   lower_thickness.set_loaded(false);
   lower_thickness.set_positive_required(true);
   lower_thickness.set_non_negative_required(false);
   lower_thickness.set_lowerLimit(1e-6);
   lower_thickness.set_upperLimit(0.1);
   lower_thickness.set_checkLimits(checkLimits_);

   // lower_material

   lower_material.push_alias("lower_material");
   lower_material.set_loaded(false);
   lower_material.set_positive_required(false);
   lower_material.set_non_negative_required(false);
   lower_material.set_lowerLimit(0);
   lower_material.set_upperLimit(1e12);
   lower_material.set_checkLimits(checkLimits_);

   // trace_left_width

   trace_left_width.push_alias("trace_left_width");
   trace_left_width.set_loaded(false);
   trace_left_width.set_positive_required(true);
   trace_left_width.set_non_negative_required(false);
   trace_left_width.set_lowerLimit(1e-6);
   trace_left_width.set_upperLimit(0.1);
   trace_left_width.set_checkLimits(checkLimits_);

   // trace_right_width

   trace_right_width.push_alias("trace_right_width");
   trace_right_width.set_loaded(false);
   trace_right_width.set_positive_required(false);
   trace_right_width.set_non_negative_required(true);
   trace_right_width.set_lowerLimit(0);
   trace_right_width.set_upperLimit(0.1);
   trace_right_width.set_checkLimits(checkLimits_);

   // trace_thickness

   trace_thickness.push_alias("trace_thickness");
   trace_thickness.set_loaded(false);
   trace_thickness.set_positive_required(true);
   trace_thickness.set_non_negative_required(false);
   trace_thickness.set_lowerLimit(1e-6);
   trace_thickness.set_upperLimit(0.1);
   trace_thickness.set_checkLimits(checkLimits_);

   // trace_air_gap

   trace_air_gap.push_alias("trace_air_gap");
   trace_air_gap.set_loaded(false);
   trace_air_gap.set_positive_required(true);
   trace_air_gap.set_non_negative_required(false);
   trace_air_gap.set_lowerLimit(1e-6);
   trace_air_gap.set_upperLimit(0.1);
   trace_air_gap.set_checkLimits(checkLimits_);

   // trace_etch_angle

   trace_etch_angle.push_alias("trace_etch_angle");
   trace_etch_angle.set_loaded(false);
   trace_etch_angle.set_positive_required(true);
   trace_etch_angle.set_non_negative_required(false);
   trace_etch_angle.set_lowerLimit(30);
   trace_etch_angle.set_upperLimit(150);
   trace_etch_angle.set_checkLimits(checkLimits_);

   // default_conductor_material

   default_conductor_material.push_alias("default_conductor_material");
   default_conductor_material.set_loaded(false);
   default_conductor_material.set_positive_required(false);
   default_conductor_material.set_non_negative_required(false);
   default_conductor_material.set_lowerLimit(0);
   default_conductor_material.set_upperLimit(1e12);
   default_conductor_material.set_checkLimits(checkLimits_);

   // trace_material_top

   trace_material_top.push_alias("trace_material_top");
   trace_material_top.set_loaded(false);
   trace_material_top.set_positive_required(false);
   trace_material_top.set_non_negative_required(false);
   trace_material_top.set_lowerLimit(0);
   trace_material_top.set_upperLimit(1e12);
   trace_material_top.set_checkLimits(checkLimits_);

   // trace_material_bottom

   trace_material_bottom.push_alias("trace_material_bottom");
   trace_material_bottom.set_loaded(false);
   trace_material_bottom.set_positive_required(false);
   trace_material_bottom.set_non_negative_required(false);
   trace_material_bottom.set_lowerLimit(0);
   trace_material_bottom.set_upperLimit(1e12);
   trace_material_bottom.set_checkLimits(checkLimits_);

   // trace_material_sides

   trace_material_sides.push_alias("trace_material_sides");
   trace_material_sides.set_loaded(false);
   trace_material_sides.set_positive_required(false);
   trace_material_sides.set_non_negative_required(false);
   trace_material_sides.set_lowerLimit(0);
   trace_material_sides.set_upperLimit(1e12);
   trace_material_sides.set_checkLimits(checkLimits_);

   // upper_groundplane_material

   upper_groundplane_material.push_alias("upper_groundplane_material");
   upper_groundplane_material.set_loaded(false);
   upper_groundplane_material.set_positive_required(false);
   upper_groundplane_material.set_non_negative_required(false);
   upper_groundplane_material.set_lowerLimit(0);
   upper_groundplane_material.set_upperLimit(1e12);
   upper_groundplane_material.set_checkLimits(checkLimits_);

   // lower_groundplane_material

   lower_groundplane_material.push_alias("lower_groundplane_material");
   lower_groundplane_material.set_loaded(false);
   lower_groundplane_material.set_positive_required(false);
   lower_groundplane_material.set_non_negative_required(false);
   lower_groundplane_material.set_lowerLimit(0);
   lower_groundplane_material.set_lowerLimit(1e12);
   lower_groundplane_material.set_checkLimits(checkLimits_);

   // left_side_material

   left_side_material.push_alias("left_side_material");
   left_side_material.set_loaded(false);
   left_side_material.set_positive_required(false);
   left_side_material.set_non_negative_required(false);
   left_side_material.set_lowerLimit(0);
   left_side_material.set_upperLimit(1e12);
   left_side_material.set_checkLimits(checkLimits_);

   // left_side_gap

   left_side_gap.push_alias("left_side_gap");
   left_side_gap.set_loaded(false);
   left_side_gap.set_positive_required(true);
   left_side_gap.set_non_negative_required(false);
   left_side_gap.set_lowerLimit(1e-6);
   left_side_gap.set_upperLimit(0.1);
   left_side_gap.set_checkLimits(checkLimits_);

   // right_side_material

   right_side_material.push_alias("right_side_material");
   right_side_material.set_loaded(false);
   right_side_material.set_positive_required(false);
   right_side_material.set_non_negative_required(false);
   right_side_material.set_lowerLimit(0);
   right_side_material.set_upperLimit(1e12);
   right_side_material.set_checkLimits(checkLimits_);

   // right_side_gap

   right_side_gap.push_alias("right_side_gap");
   right_side_gap.set_loaded(false);
   right_side_gap.set_positive_required(true);
   right_side_gap.set_non_negative_required(false);
   right_side_gap.set_lowerLimit(1e-6);
   right_side_gap.set_upperLimit(0.1);
   right_side_gap.set_checkLimits(checkLimits_);

   // length

   length.push_alias("length");
   length.set_loaded(false);
   length.set_positive_required(false);
   length.set_non_negative_required(true);
   length.set_lowerLimit(0);
   length.set_upperLimit(1);
   length.set_checkLimits(checkLimits_);

   included=nullptr;

   // defaults
   length.set_dbl_value(0);
   solution_impedance_calculation.set_value("line");
}

void CoupledStrip::print_commented(ofstream *out)
{
   *out << "// CoupledStrip" << endl;
   *out << "//    name=" << name.get_value() << endl;
   if (include.is_loaded()) *out << "//    include=" << include.get_value() << endl;
   *out << "//    upper_material=" << upper_material.get_value() << endl;
   *out << "//    solution_impedance_calculation=" << solution_impedance_calculation.get_value() << endl;
   *out << "//    upper_thickness=" << upper_thickness.get_dbl_value() << endl;
   *out << "//    lower_material=" << lower_material.get_value() << endl;
   *out << "//    lower_thickness=" << lower_thickness.get_dbl_value() << endl;
   if (left_side_material.is_loaded()) *out << "//    left_side_material=" << left_side_material.get_value() << endl;
   *out << "//    left_side_gap=" << left_side_gap.get_dbl_value() << endl;
   if (right_side_material.is_loaded()) *out << "//    right_side_material=" << right_side_material.get_value() << endl;
   *out << "//    right_side_gap=" << right_side_gap.get_dbl_value() << endl;
   if (soldermask_thickness.is_loaded()) {
      *out << "//    soldermask_thickness=" << soldermask_thickness.get_dbl_value() << endl;
      if (soldermask_material.is_loaded()) *out << "//    soldermask_material=" << soldermask_material.get_value() << endl;
   }
   *out << "//    trace_left_width=" << trace_left_width.get_dbl_value() << endl;
   if (trace_right_width.get_dbl_value() == 0) *out << "//    trace_right_width=trace_left_width" << endl;
   else *out << "//    trace_right_width=" << trace_right_width.get_dbl_value() << endl;
   *out << "//    trace_thickness=" << trace_thickness.get_dbl_value() << endl;
   *out << "//    trace_air_gap=" << trace_air_gap.get_dbl_value() << endl;
   *out << "//    trace_etch_angle=" << trace_etch_angle.get_dbl_value() << endl;
   if (trace_material_bottom.is_loaded() && trace_material_top.is_loaded() && trace_material_sides.is_loaded() && 
       upper_groundplane_material.is_loaded() && lower_groundplane_material.is_loaded() && left_side_material.is_loaded() && right_side_material.is_loaded()) {
      // no need to print
   } else {
      *out << "//    default_conductor_material=" << default_conductor_material.get_value() << endl;
   }
   if (trace_material_bottom.is_loaded()) *out << "//    trace_material_bottom=" << trace_material_bottom.get_value() << endl;
   if (trace_material_top.is_loaded()) *out << "//    trace_material_top=" << trace_material_top.get_value() << endl;
   if (trace_material_sides.is_loaded()) *out << "//    trace_material_sides=" << trace_material_sides.get_value() << endl;
   if (upper_groundplane_material.is_loaded()) *out << "//    upper_groundplane_material=" << upper_groundplane_material.get_value() << endl;
   if (lower_groundplane_material.is_loaded()) *out << "//    lower_groundplane_material=" << lower_groundplane_material.get_value() << endl;
   if (length.is_loaded()) *out << "//    length=" << length.get_dbl_value() << endl;

   *out << "// EndCoupledStrip" << endl;
}

bool CoupledStrip::load (string *indent, inputFile *inputs, bool checkInputs)
{
   bool fail=false;

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {
      string token,value,line;
      line=inputs->get_line(lineNumber);
      get_token_pair(&line,&token,&value,&lineNumber,indent->c_str());

      int recognized=0;
      if (name.match_alias(&token)) {
         if (name.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2058: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,name.get_lineNumber());
            fail=true;
         } else {
            name.set_keyword(token);
            name.set_value(value);
            name.set_lineNumber(lineNumber);
            name.set_loaded(true);
         }
         recognized++;
      }

      if (include.match_alias(&token)) {
         if (include.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2060: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,include.get_lineNumber());
            fail=true;
         } else {
            include.set_keyword(token);
            include.set_value(value);
            include.set_lineNumber(lineNumber);
            include.set_loaded(true);
         }
         recognized++;
      }

      if (solution_impedance_calculation.match_alias(&token)) {
         if (solution_impedance_calculation.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2059: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,solution_impedance_calculation.get_lineNumber());
            fail=true;
         } else {
            solution_impedance_calculation.set_keyword(token);
            solution_impedance_calculation.set_value(value);
            solution_impedance_calculation.set_lineNumber(lineNumber);
            solution_impedance_calculation.set_loaded(true);
         }
         recognized++;
      }

      if (upper_material.match_alias(&token)) {
         if (upper_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2061: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,upper_material.get_lineNumber());
            fail=true;
         } else {
            upper_material.set_keyword(token);
            upper_material.set_value(value);
            upper_material.set_lineNumber(lineNumber);
            upper_material.set_loaded(true);
         }
         recognized++;
      }

      if (upper_thickness.match_alias(&token)) {
         recognized++;
         if (upper_thickness.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (soldermask_thickness.match_alias(&token)) {
         recognized++;
         if (soldermask_thickness.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (soldermask_material.match_alias(&token)) {
         if (soldermask_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2062: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,soldermask_material.get_lineNumber());
            fail=true;
         } else {
            soldermask_material.set_keyword(token);
            soldermask_material.set_value(value);
            soldermask_material.set_lineNumber(lineNumber);
            soldermask_material.set_loaded(true);
         }
         recognized++;
      }

      if (lower_thickness.match_alias(&token)) {
         recognized++;
         if (lower_thickness.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (lower_material.match_alias(&token)) {
         if (lower_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2063: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,lower_material.get_lineNumber());
            fail=true;
         } else {
            lower_material.set_keyword(token);
            lower_material.set_value(value);
            lower_material.set_lineNumber(lineNumber);
            lower_material.set_loaded(true);
         }
         recognized++;
      }

      if (left_side_gap.match_alias(&token)) {
         recognized++;
         if (left_side_gap.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (left_side_material.match_alias(&token)) {
         if (left_side_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2064: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,left_side_material.get_lineNumber());
            fail=true;
         } else {
            left_side_material.set_keyword(token);
            left_side_material.set_value(value);
            left_side_material.set_lineNumber(lineNumber);
            left_side_material.set_loaded(true);
         }
         recognized++;
      }

      if (right_side_gap.match_alias(&token)) {
         recognized++;
         if (right_side_gap.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (right_side_material.match_alias(&token)) {
         if (right_side_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2065: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,right_side_material.get_lineNumber());
            fail=true;
         } else {
            right_side_material.set_keyword(token);
            right_side_material.set_value(value);
            right_side_material.set_lineNumber(lineNumber);
            right_side_material.set_loaded(true);
         }
         recognized++;
      }

      if (trace_left_width.match_alias(&token)) {
         recognized++;
         if (trace_left_width.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (trace_right_width.match_alias(&token)) {
         recognized++;
         if (trace_right_width.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (trace_thickness.match_alias(&token)) {
         recognized++;
         if (trace_thickness.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (trace_air_gap.match_alias(&token)) {
         recognized++;
         if (trace_air_gap.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (trace_etch_angle.match_alias(&token)) {
         recognized++;
         if (trace_etch_angle.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (default_conductor_material.match_alias(&token)) {
         if (default_conductor_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2066: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,default_conductor_material.get_lineNumber());
            fail=true;
         } else {
            default_conductor_material.set_keyword(token);
            default_conductor_material.set_value(value);
            default_conductor_material.set_lineNumber(lineNumber);
            default_conductor_material.set_loaded(true);
         }
         recognized++;
      }

      if (trace_material_top.match_alias(&token)) {
         if (trace_material_top.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2067: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,trace_material_top.get_lineNumber());
            fail=true;
         } else {
            trace_material_top.set_keyword(token);
            trace_material_top.set_value(value);
            trace_material_top.set_lineNumber(lineNumber);
            trace_material_top.set_loaded(true);
         }
         recognized++;
      }

      if (trace_material_bottom.match_alias(&token)) {
         if (trace_material_bottom.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2068: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,trace_material_bottom.get_lineNumber());
            fail=true;
         } else {
            trace_material_bottom.set_keyword(token);
            trace_material_bottom.set_value(value);
            trace_material_bottom.set_lineNumber(lineNumber);
            trace_material_bottom.set_loaded(true);
         }
         recognized++;
      }

      if (trace_material_sides.match_alias(&token)) {
         if (trace_material_sides.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2069: Duplicate entry at line %d for previous entry at line %d.\n", 
                                         indent->c_str(),lineNumber,trace_material_sides.get_lineNumber());
            fail=true;
         } else {
            trace_material_sides.set_keyword(token);
            trace_material_sides.set_value(value);
            trace_material_sides.set_lineNumber(lineNumber);
            trace_material_sides.set_loaded(true);
         }
         recognized++;
      }

      if (upper_groundplane_material.match_alias(&token)) {
         if (upper_groundplane_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2070: Duplicate entry at line %d for previous entry at line %d.\n", 
                                         indent->c_str(),lineNumber,upper_groundplane_material.get_lineNumber());
            fail=true;
         } else {
            upper_groundplane_material.set_keyword(token);
            upper_groundplane_material.set_value(value);
            upper_groundplane_material.set_lineNumber(lineNumber);
            upper_groundplane_material.set_loaded(true);
         }
         recognized++;
      }

      if (lower_groundplane_material.match_alias(&token)) {
         if (lower_groundplane_material.is_loaded()) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2071: Duplicate entry at line %d for previous entry at line %d.\n",
                                         indent->c_str(),lineNumber,lower_groundplane_material.get_lineNumber());
            fail=true;
         } else {
            lower_groundplane_material.set_keyword(token);
            lower_groundplane_material.set_value(value);
            lower_groundplane_material.set_lineNumber(lineNumber);
            lower_groundplane_material.set_loaded(true);
         }
         recognized++;
      }

      if (length.match_alias(&token)) {
         recognized++;
         if (length.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      // should recognize one keyword
      if (recognized != 1) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2072: Unrecognized keyword at line %d.\n",indent->c_str(),lineNumber);
         fail=true;
      }

      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }
   return fail;
}

bool CoupledStrip::inBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

void CoupledStrip::apply_include ()
{
   if (included == nullptr) return;

   if (included_type == 0) {
      RectangularWaveguide *rw=(RectangularWaveguide *)included;
      if (! default_conductor_material.is_loaded()) if (rw->get_default_conductor_material().is_loaded()) default_conductor_material.copy(rw->get_default_conductor_material());
   } else if (included_type == 1) {
      Strip *strip=(Strip *)included;
      if (! upper_material.is_loaded()) if (strip->get_upper_material().is_loaded()) upper_material.copy(strip->get_upper_material());
      if (! upper_thickness.is_loaded()) if (strip->get_upper_thickness().is_loaded()) upper_thickness.copy(strip->get_upper_thickness());
      if (! soldermask_thickness.is_loaded()) if (strip->get_soldermask_thickness().is_loaded()) soldermask_thickness.copy(strip->get_soldermask_thickness());
      if (! soldermask_material.is_loaded()) if (strip->get_soldermask_material().is_loaded()) soldermask_material.copy(strip->get_soldermask_material());
      if (! lower_thickness.is_loaded()) if (strip->get_lower_thickness().is_loaded()) lower_thickness.copy(strip->get_lower_thickness());
      if (! lower_material.is_loaded()) if (strip->get_lower_material().is_loaded()) lower_material.copy(strip->get_lower_material());
      if (! trace_left_width.is_loaded()) if (strip->get_trace_width().is_loaded()) trace_left_width.copy(strip->get_trace_width());
      if (! trace_thickness.is_loaded()) if (strip->get_trace_thickness().is_loaded()) trace_thickness.copy(strip->get_trace_thickness());
      if (! trace_etch_angle.is_loaded()) if (strip->get_trace_etch_angle().is_loaded()) trace_etch_angle.copy(strip->get_trace_etch_angle());
      if (! default_conductor_material.is_loaded()) if (strip->get_default_conductor_material().is_loaded()) default_conductor_material.copy(strip->get_default_conductor_material());
      if (! trace_material_bottom.is_loaded()) if (strip->get_trace_material_bottom().is_loaded()) trace_material_bottom.copy(strip->get_trace_material_bottom());
      if (! trace_material_top.is_loaded()) if (strip->get_trace_material_top().is_loaded()) trace_material_top.copy(strip->get_trace_material_top());
      if (! trace_material_sides.is_loaded()) if (strip->get_trace_material_sides().is_loaded()) trace_material_sides.copy(strip->get_trace_material_sides());
      if (! upper_groundplane_material.is_loaded()) if (strip->get_upper_groundplane_material().is_loaded()) upper_groundplane_material.copy(strip->get_upper_groundplane_material());
      if (! lower_groundplane_material.is_loaded()) if (strip->get_lower_groundplane_material().is_loaded()) lower_groundplane_material.copy(strip->get_lower_groundplane_material());
      if (! left_side_material.is_loaded()) if (strip->get_left_side_material().is_loaded()) left_side_material.copy(strip->get_left_side_material());
      if (! left_side_gap.is_loaded()) if (strip->get_left_side_gap().is_loaded()) left_side_gap.copy(strip->get_left_side_gap());
      if (! right_side_material.is_loaded()) if (strip->get_right_side_material().is_loaded()) right_side_material.copy(strip->get_right_side_material());
      if (! right_side_gap.is_loaded()) if (strip->get_right_side_gap().is_loaded()) right_side_gap.copy(strip->get_right_side_gap());
   } else if (included_type == 2) {
      CoupledStrip *coupledStrip=(CoupledStrip *)included;
      if (! upper_material.is_loaded()) if (coupledStrip->upper_material.is_loaded()) upper_material.copy(coupledStrip->upper_material);
      if (! upper_thickness.is_loaded()) if (coupledStrip->upper_thickness.is_loaded()) upper_thickness.copy(coupledStrip->upper_thickness);
      if (! soldermask_thickness.is_loaded()) if (coupledStrip->soldermask_thickness.is_loaded()) soldermask_thickness.copy(coupledStrip->soldermask_thickness);
      if (! soldermask_material.is_loaded()) if (coupledStrip->soldermask_material.is_loaded()) soldermask_material.copy(coupledStrip->soldermask_material);
      if (! lower_thickness.is_loaded()) if (coupledStrip->lower_thickness.is_loaded()) lower_thickness.copy(coupledStrip->lower_thickness);
      if (! lower_material.is_loaded()) if (coupledStrip->lower_material.is_loaded()) lower_material.copy(coupledStrip->lower_material);
      if (! trace_left_width.is_loaded()) if (coupledStrip->trace_left_width.is_loaded()) trace_left_width.copy(coupledStrip->trace_left_width);
      if (! trace_right_width.is_loaded()) if (coupledStrip->trace_right_width.is_loaded()) trace_right_width.copy(coupledStrip->trace_right_width);
      if (! trace_thickness.is_loaded()) if (coupledStrip->trace_thickness.is_loaded()) trace_thickness.copy(coupledStrip->trace_thickness);
      if (! trace_air_gap.is_loaded()) if (coupledStrip->trace_air_gap.is_loaded()) trace_air_gap.copy(coupledStrip->trace_air_gap);
      if (! trace_etch_angle.is_loaded()) if (coupledStrip->trace_etch_angle.is_loaded()) trace_etch_angle.copy(coupledStrip->trace_etch_angle);
      if (! default_conductor_material.is_loaded()) if (coupledStrip->default_conductor_material.is_loaded()) default_conductor_material.copy(coupledStrip->default_conductor_material);
      if (! trace_material_bottom.is_loaded()) if (coupledStrip->trace_material_bottom.is_loaded()) trace_material_bottom.copy(coupledStrip->trace_material_bottom);
      if (! trace_material_top.is_loaded()) if (coupledStrip->trace_material_top.is_loaded()) trace_material_top.copy(coupledStrip->trace_material_top);
      if (! trace_material_sides.is_loaded()) if (coupledStrip->trace_material_sides.is_loaded()) trace_material_sides.copy(coupledStrip->trace_material_sides);
      if (! upper_groundplane_material.is_loaded()) if (coupledStrip->upper_groundplane_material.is_loaded()) upper_groundplane_material.copy(coupledStrip->upper_groundplane_material);
      if (! lower_groundplane_material.is_loaded()) if (coupledStrip->lower_groundplane_material.is_loaded()) lower_groundplane_material.copy(coupledStrip->lower_groundplane_material);
      if (! left_side_material.is_loaded()) if (coupledStrip->left_side_material.is_loaded()) left_side_material.copy(coupledStrip->left_side_material);
      if (! left_side_gap.is_loaded()) if (coupledStrip->left_side_gap.is_loaded()) left_side_gap.copy(coupledStrip->left_side_gap);
      if (! right_side_material.is_loaded()) if (coupledStrip->right_side_material.is_loaded()) right_side_material.copy(coupledStrip->right_side_material);
      if (! right_side_gap.is_loaded()) if (coupledStrip->right_side_gap.is_loaded()) right_side_gap.copy(coupledStrip->right_side_gap);
   }
}

bool CoupledStrip::check (string indent)
{
   bool fail=false;

   if (!name.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2073: CoupledStrip block at line %d must specify a name.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!upper_thickness.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2074: CoupledStrip block at line %d must specify an upper thickness.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!upper_material.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2075: CoupledStrip block at line %d must specify an upper material.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (soldermask_thickness.is_loaded() && !soldermask_material.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2076: CoupledStrip block at line %d must specify a soldermask material.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!soldermask_thickness.is_loaded() && soldermask_material.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2077: CoupledStrip block at line %d must not specify a soldermask material.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!lower_thickness.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2078: CoupledStrip block at line %d must specify a lower thickness.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!lower_material.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2079: CoupledStrip block at line %d must specify a lower material.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!left_side_gap.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2080: CoupledStrip block at line %d must specify a left side gap.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!right_side_gap.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2081: CoupledStrip block at line %d must specify a right side gap.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!trace_left_width.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2082: CoupledStrip block at line %d must specify a width for the left trace.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!trace_right_width.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2083: CoupledStrip block at line %d must specify a width for the right trace.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!trace_thickness.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2084: CoupledStrip block at line %d must specify a trace thickness.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!trace_air_gap.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2085: CoupledStrip block at line %d must specify an air gap.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!trace_etch_angle.is_loaded()) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2086: CoupledStrip block at line %d must specify a trace etch angle.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!default_conductor_material.is_loaded()) {
      if (trace_material_top.is_loaded() && trace_material_bottom.is_loaded() && trace_material_sides.is_loaded() && 
          upper_groundplane_material.is_loaded() && lower_groundplane_material.is_loaded() && left_side_material.is_loaded() && right_side_material.is_loaded()) {
         // ok
      } else {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2087: CoupledStrip block at line %d must specify a default conductor material.\n",indent.c_str(),startLine);
         fail=true;
      }
   }

   // gmsh fails to mesh a section if the name is duplicated
   
   if (lower_material.is_loaded() && upper_material.is_loaded()) {
      if (lower_material.get_value().compare(upper_material.get_value()) == 0) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2088: CoupledStrip block at line %d must specify different names for the uppper and lower materials [properties can match].\n",indent.c_str(),startLine);
         fail=true;
      }
   }

   if (lower_material.is_loaded() && soldermask_material.is_loaded()) {
      if (lower_material.get_value().compare(soldermask_material.get_value()) == 0) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2089: CoupledStrip block at line %d must specify different names for the lower and soldermask materials [properties can match].\n",indent.c_str(),startLine);
         fail=true;
      }
   }

   if (upper_material.is_loaded() && soldermask_material.is_loaded()) {
      if (upper_material.get_value().compare(soldermask_material.get_value()) == 0) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2090: CoupledStrip block at line %d must specify different names for the uppper and soldermask materials [properties can match].\n",indent.c_str(),startLine);
         fail=true;
      }
   }

   if (solution_impedance_calculation.is_loaded()) {
      if (solution_impedance_calculation.get_value().compare("modal") != 0 && solution_impedance_calculation.get_value().compare("line") != 0) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2189: CoupledStrip block at line %d must specify \"solution_impedance_calculation\" as either \"modal\" or \"line\".\n",indent.c_str(),startLine);
         fail=true;
      }
   }

   return fail;
}

bool CoupledStrip::checkInclude(string indent)
{
   if (name.is_loaded() && include.is_loaded()) {
      if (name.get_value().compare(include.get_value()) == 0) {
         PetscPrintf(PETSC_COMM_WORLD,"%sERROR2091: CoupledStrip block at line %d cannot include itself.\n",indent.c_str(),startLine);
         return true;
      }
   }
   return false;
}

bool CoupledStrip::checkLimits()
{
   bool fail=false;

   if (upper_thickness.limit_check("double")) fail=true;
   if (left_side_gap.limit_check("double")) fail=true;
   if (right_side_gap.limit_check("double")) fail=true;
   if (soldermask_thickness.is_loaded() && soldermask_thickness.limit_check("double")) fail=true;
   if (lower_thickness.limit_check("double")) fail=true;
   if (trace_thickness.limit_check("double")) fail=true;
   if (trace_left_width.limit_check("double")) fail=true;
   if (trace_right_width.limit_check("double")) fail=true;
   if (trace_air_gap.limit_check("double")) fail=true;
   if (trace_etch_angle.limit_check("double")) fail=true;
   if (length.limit_check("double")) fail=true;

   return fail;
}

// Build a Strip for trace_left and a Strip for trace_right, adjust,
// then merge to two to get the coupled transmission line.
void CoupledStrip::build_geo (Geo *geo)
{
   Strip *left=new Strip (0, 0, true);
   Strip *right=new Strip (0, 0, true);

   left->set_use_symmetry(false);
   left->set_default_conductor_material(default_conductor_material);
   left->set_left_side_gap(left_side_gap);
   left->set_left_side_material(left_side_material);
   left->set_right_side_gap(trace_air_gap);                      // modify
   left->set_right_side_material(right_side_material);
   left->set_upper_thickness(upper_thickness);
   left->set_upper_material(upper_material);
   left->set_soldermask_thickness(soldermask_thickness);
   left->set_soldermask_material(soldermask_material);
   left->set_lower_thickness(lower_thickness);
   left->set_lower_material(lower_material);
   left->set_trace_thickness(trace_thickness);
   left->set_trace_width(trace_left_width);                      // modify
   left->set_trace_etch_angle(trace_etch_angle);
   left->set_trace_material_top(trace_material_top);
   left->set_trace_material_bottom(trace_material_bottom);
   left->set_trace_material_sides(trace_material_sides);
   left->set_upper_groundplane_material(upper_groundplane_material);
   left->set_lower_groundplane_material(lower_groundplane_material);

   right->set_use_symmetry(false);
   right->set_default_conductor_material(default_conductor_material);
   right->set_left_side_gap(trace_air_gap);                      // modify
   right->set_left_side_material(left_side_material);
   right->set_right_side_gap(right_side_gap);
   right->set_right_side_material(right_side_material);
   right->set_upper_thickness(upper_thickness);
   right->set_upper_material(upper_material);
   right->set_soldermask_thickness(soldermask_thickness);
   right->set_soldermask_material(soldermask_material);
   right->set_lower_thickness(lower_thickness);
   right->set_lower_material(lower_material);
   right->set_trace_thickness(trace_thickness);

   if (trace_right_width.get_dbl_value() == 0) trace_right_width.copy(trace_left_width); // modify
   right->set_trace_width(trace_right_width);

   right->set_trace_etch_angle(trace_etch_angle);
   right->set_trace_material_top(trace_material_top);
   right->set_trace_material_bottom(trace_material_bottom);
   right->set_trace_material_sides(trace_material_sides);
   right->set_upper_groundplane_material(upper_groundplane_material);
   right->set_lower_groundplane_material(lower_groundplane_material);

   // adjust the side gaps so that the total equals the air gap
   left->split_right_side_gap();
   right->split_left_side_gap();

   // build
   left->build_geo(geo,true);
   Geo right_geo;
   right->build_geo(&right_geo,false);

   // align
   geo->shift(-left->get_trace_width().get_dbl_value()/2-left->get_right_side_gap().get_dbl_value());
   right_geo.shift(right->get_trace_width().get_dbl_value()/2+right->get_left_side_gap().get_dbl_value());

   // offset the indices
   right_geo.offset_indices(100);

   // merge these
   geo->merge(&right_geo);

   // 3D
   geo->extrude(length.get_dbl_value());
}

bool CoupledStrip::write_geo (string indent, Control *control, Geo *geo)
{
   bool fail=false;
   stringstream ssFilename;
   ssFilename << name.get_value() << ".geo";

   ofstream out;
   out.open (ssFilename.str().c_str(),ofstream::out);
   if (out.is_open()) {
      cout << "Writing \"" << ssFilename.str() << "\" ..." << endl;

      control->print_commented(&out);
      print_commented(&out);

      geo->write(&out);

      out.close();
   } else {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2092: Failed to open \"%s\" for writing.\n",
                                   indent.c_str(),ssFilename.str().c_str());
      fail=true;
   }
   return fail;
}

bool CoupledStrip::write_modes_and_boundaries (string indent, Control *control)
{
   if (length.get_dbl_value() != 0) return false;

   bool fail=false;
   stringstream ssFilename;
   if (solution_impedance_calculation.get_value().compare("modal") == 0) ssFilename << name.get_value() << "_modes.txt";
   if (solution_impedance_calculation.get_value().compare("line") == 0) ssFilename << name.get_value() << "_lines.txt";

   ofstream modes;
   modes.open (ssFilename.str().c_str(),ofstream::out);
   if (modes.is_open()) {
      cout << "Writing \"" << ssFilename.str() << "\" ..." << endl;

      modes << "#OpenParEMmodes 1.0" << endl << endl;

      control->print_commented(&modes);
      print_commented(&modes);

      modes << endl;
      modes << "File" << endl;
      modes << "   name=generated_by_builder" << endl;
      modes << "EndFile" << endl;

      string boundary_material;

      double pi=4.*atan(1.);
      double etch_offset=trace_thickness.get_dbl_value()/tan(trace_etch_angle.get_dbl_value()*pi/180.);

      // voltage left

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=voltage_line_left" << endl;
      modes << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()/2-trace_air_gap.get_dbl_value()/2 << "," << 0 << ")" << endl;
      modes << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()/2-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      // voltage right

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=voltage_line_right" << endl;
      modes << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()/2+trace_air_gap.get_dbl_value()/2 << "," << 0 << ")" << endl;
      modes << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()/2+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      // Modes and Lines

      if (solution_impedance_calculation.get_value().compare("modal") == 0) {

         // even voltage
         modes << endl;
         modes << "Mode" << endl;
         modes << "   mode=1" << endl;
         modes << "   scale=0.5" << endl;
         modes << "   type=voltage" << endl;
         modes << "   path=voltage_line_left" << endl;
         modes << "   path+=voltage_line_right" << endl;
         modes << "EndMode" << endl;

         // odd voltage
         modes << endl;
         modes << "Mode" << endl;
         modes << "   mode=2" << endl;
         modes << "   type=voltage" << endl;
         modes << "   path=voltage_line_left" << endl;
         modes << "   path-=voltage_line_right" << endl;
         modes << "EndMode" << endl;

         // even current
         modes << endl;
         modes << "Mode" << endl;
         modes << "   mode=1" << endl;
         modes << "   type=current" << endl;
         modes << "   path=+trace_left_top" << endl;
         modes << "   path+=trace_left_right_side" << endl;
         modes << "   path+=trace_left_bottom" << endl;
         modes << "   path+=trace_left_left_side" << endl;
         modes << "   path+=trace_right_top" << endl;
         modes << "   path+=trace_right_right_side" << endl;
         modes << "   path+=trace_right_bottom" << endl;
         modes << "   path+=trace_right_left_side" << endl;
         modes << "EndMode" << endl;

         // odd current
         modes << endl;
         modes << "Mode" << endl;
         modes << "   mode=2" << endl;
         modes << "   type=current" << endl;
         modes << "   scale=0.5" << endl;
         modes << "   path=+trace_left_top" << endl;
         modes << "   path+=trace_left_right_side" << endl;
         modes << "   path+=trace_left_bottom" << endl;
         modes << "   path+=trace_left_left_side" << endl;
         modes << "   path-=trace_right_top" << endl;
         modes << "   path-=trace_right_right_side" << endl;
         modes << "   path-=trace_right_bottom" << endl;
         modes << "   path-=trace_right_left_side" << endl;
         modes << "EndMode" << endl;
      }

      if (solution_impedance_calculation.get_value().compare("line") == 0) {
         // left voltage
         modes << endl;
         modes << "Line" << endl;
         modes << "   line=1" << endl;
         modes << "   type=voltage" << endl;
         modes << "   path=voltage_line_left" << endl;
         modes << "EndLine" << endl;

         // right voltage
         modes << endl;
         modes << "Line" << endl;
         modes << "   line=2" << endl;
         modes << "   type=voltage" << endl;
         modes << "   path=voltage_line_right" << endl;
         modes << "EndLine" << endl;

         // left current
         modes << endl;
         modes << "Line" << endl;
         modes << "   line=1" << endl;
         modes << "   type=current" << endl;
         modes << "   path=+trace_left_top" << endl;
         modes << "   path+=trace_left_right_side" << endl;
         modes << "   path+=trace_left_bottom" << endl;
         modes << "   path+=trace_left_left_side" << endl;
         modes << "EndLine" << endl;

         // right current
         modes << endl;
         modes << "Line" << endl;
         modes << "   line=2" << endl;
         modes << "   type=current" << endl;
         modes << "   path=+trace_right_top" << endl;
         modes << "   path+=trace_right_right_side" << endl;
         modes << "   path+=trace_right_bottom" << endl;
         modes << "   path+=trace_right_left_side" << endl;
         modes << "EndLine" << endl;
      }

      // impedance boundaries 

      // trace left top

      if (trace_material_top.is_loaded()) boundary_material=trace_material_top.get_value();
      else boundary_material=default_conductor_material.get_value();

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=trace_left_top" << endl;
      modes << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
      modes << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=trace_left_top" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=trace_left_top" << endl;
         modes << "EndBoundary" << endl;
      }

      // trace right top

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=trace_right_top" << endl;
      modes << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
      modes << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=trace_right_top" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=trace_right_top" << endl;
         modes << "EndBoundary" << endl;
      }

      // trace left bottom

      if (trace_material_bottom.is_loaded()) boundary_material=trace_material_bottom.get_value();
      else boundary_material=default_conductor_material.get_value();

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=trace_left_bottom" << endl;
      modes << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=trace_left_bottom" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=trace_left_bottom" << endl;
         modes << "EndBoundary" << endl;
      }

      // trace right bottom

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=trace_right_bottom" << endl;
      modes << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=trace_right_bottom" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=trace_right_bottom" << endl;
         modes << "EndBoundary" << endl;
      }

      // trace left left side

      if (trace_material_sides.is_loaded()) boundary_material=trace_material_sides.get_value();
      else boundary_material=default_conductor_material.get_value();

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=trace_left_left_side" << endl;
      modes << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=trace_left_left_side" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=trace_left_left_side" << endl;
         modes << "EndBoundary" << endl;
      }

      // trace left right side

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=trace_left_right_side" << endl;
      modes << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=trace_left_right_side" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=trace_left_right_side" << endl;
         modes << "EndBoundary" << endl;
      }

      // trace right left side

      if (trace_material_sides.is_loaded()) boundary_material=trace_material_sides.get_value();
      else boundary_material=default_conductor_material.get_value();

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=trace_right_left_side" << endl;
      modes << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=trace_right_left_side" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=trace_right_left_side" << endl;
         modes << "EndBoundary" << endl;
      }

      // trace right right side

      modes << endl;
      modes << "Path" << endl;
      modes << "   name=trace_right_right_side" << endl;
      modes << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << ")" << endl;
      modes << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << ")" << endl;
      modes << "   closed=false" << endl;
      modes << "EndPath" << endl;

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=trace_right_right_side" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=trace_right_right_side" << endl;
         modes << "EndBoundary" << endl;
      }

      // lower groundplane

      if (lower_groundplane_material.is_loaded()) boundary_material=lower_groundplane_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=ground_plane" << endl;
         modes << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << 0 << ")" << endl;
         modes << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << 0 << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=ground_plane" << endl;
         modes << "   type=surface_impedance" << endl;
         modes << "   material=" << boundary_material << endl;
         modes << "   path=ground_plane" << endl;
         modes << "EndBoundary" << endl;
      }

      // upper groundplane

      if (upper_groundplane_material.is_loaded()) boundary_material=upper_groundplane_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=upper_groundplane" << endl;
         modes << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << ")" << endl;
         modes << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value()  << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=upper_groundplane" << endl;
         if (boundary_material.compare("PMC") == 0) {
            modes << "   type=perfect_magnetic_conductor" << endl;
         } else {
            modes << "   type=surface_impedance" << endl;
            modes << "   material=" << boundary_material << endl;
         }
         modes << "   path=upper_groundplane" << endl;
         modes << "EndBoundary" << endl;
      }

      // left side

      if (left_side_material.is_loaded()) boundary_material=left_side_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=left_side" << endl;
         modes << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << 0 << ")" << endl;
         modes << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value()  << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=left_side" << endl;
         if (boundary_material.compare("PMC") == 0) {
           modes << "   type=perfect_magnetic_conductor" << endl;
         } else {
            modes << "   type=surface_impedance" << endl;
            modes << "   material=" << boundary_material << endl;
         }
         modes << "   path=left_side" << endl;
         modes << "EndBoundary" << endl;
      }

      // right side

      if (right_side_material.is_loaded()) boundary_material=right_side_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         modes << endl;
         modes << "Path" << endl;
         modes << "   name=right_side" << endl;
         modes << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << 0 << ")" << endl;
         modes << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value()  << ")" << endl;
         modes << "   closed=false" << endl;
         modes << "EndPath" << endl;

         modes << endl;
         modes << "Boundary" << endl;
         modes << "   name=right_side" << endl;
         if (boundary_material.compare("PMC") == 0) {
           modes << "   type=perfect_magnetic_conductor" << endl;
         } else {
            modes << "   type=surface_impedance" << endl;
            modes << "   material=" << boundary_material << endl;
         }
         modes << "   path=right_side" << endl;
         modes << "EndBoundary" << endl;
      }

      modes.close();
   } else {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2093: Failed to open \"%s\" for writing.\n",
                                   indent.c_str(),ssFilename.str().c_str());
      fail=true;
   }
   return fail;
}

bool CoupledStrip::write_ports_and_boundaries (string indent, Control *control)
{
   if (length.get_dbl_value() == 0) return false;

   bool fail=false;
   stringstream ssFilename;
   ssFilename << name.get_value() << "_ports.txt";

   ofstream ports;
   ports.open (ssFilename.str().c_str(),ofstream::out);
   if (ports.is_open()) {
      cout << "Writing \"" << ssFilename.str() << "\" ..." << endl;

      ports << "#OpenParEMports 1.0" << endl << endl;

      control->print_commented(&ports);
      print_commented(&ports);

      ports << endl;
      ports << "File" << endl;
      ports << "   name=generated_by_builder" << endl;
      ports << "EndFile" << endl;

      string boundary_material;

      double pi=4.*atan(1.);
      double etch_offset=trace_thickness.get_dbl_value()/tan(trace_etch_angle.get_dbl_value()*pi/180.);

      // voltage left paths

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=V1L" << endl;
      ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()/2-trace_air_gap.get_dbl_value()/2 << "," << 0 << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()/2-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << "   closed=false" << endl;
      ports << "EndPath" << endl;

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=V2L" << endl;
      ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()/2-trace_air_gap.get_dbl_value()/2 << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()/2-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << "   closed=false" << endl;
      ports << "EndPath" << endl;

      // voltage right paths

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=V1R" << endl;
      ports << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()/2+trace_air_gap.get_dbl_value()/2 << "," << 0 << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()/2+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << "   closed=false" << endl;
      ports << "EndPath" << endl;

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=V2R" << endl;
      ports << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()/2+trace_air_gap.get_dbl_value()/2 << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()/2+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << "   closed=false" << endl;
      ports << "EndPath" << endl;

      // current left paths

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=I1L" << endl;
      ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << "   closed=true" << endl;
      ports << "EndPath" << endl;

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=I2L" << endl;
      ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << "   closed=true" << endl;
      ports << "EndPath" << endl;

      // current right paths

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=I1R" << endl;
      ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2+trace_right_width.get_dbl_value()-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2+trace_right_width.get_dbl_value() << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << "   closed=true" << endl;
      ports << "EndPath" << endl;

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=I2R" << endl;
      ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2+trace_right_width.get_dbl_value()-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2+trace_right_width.get_dbl_value() << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << "   closed=true" << endl;
      ports << "EndPath" << endl;

      // port paths

      double left=left_side_gap.get_dbl_value()+trace_left_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2;
      double right=right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2;

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=port1" << endl;
      ports << setprecision(15) << "   point=(" << -left << "," << 0 << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << -left << "," << lower_thickness.get_dbl_value()+ upper_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << right << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << 0 << ")" << endl;
      ports << setprecision(15) << "   point=(" << right << "," << 0 << "," << 0 << ")" << endl;
      ports << "   closed=true" << endl;
      ports << "EndPath" << endl;

      ports << endl;
      ports << "Path" << endl;
      ports << "   name=port2" << endl;
      ports << setprecision(15) << "   point=(" << -left << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << -left << "," << lower_thickness.get_dbl_value()+ upper_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << right << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
      ports << setprecision(15) << "   point=(" << right << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
      ports << "   closed=true" << endl;
      ports << "EndPath" << endl;

      // ports

      if (solution_impedance_calculation.get_value().compare("modal") == 0) {
         ports << endl;
         ports << "Port" << endl;
         ports << "   name=1" << endl;
         ports << "   path=+port1" << endl;
         if (double_compare(trace_left_width.get_dbl_value(),trace_right_width.get_dbl_value(),1e-12)) {
            cout << "Note: Using PI definition for impedance for symmetric trace widths." << endl;
            ports << "   impedance_definition=PI" << endl;
         } else {
            cout << "Note: Using PV definition for impedance for asymmetric trace widths." << endl;
            ports << "   impedance_definition=PV" << endl;
         }
         ports << "   impedance_calculation=modal" << endl;
         ports << "   Mode" << endl;
         ports << "      Sport=1" << endl;
         ports << "      net=common_in" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=voltage" << endl;
         ports << "         scale=0.5" << endl;
         ports << "         path=+V1L" << endl;
         ports << "         path+=V1R" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=current" << endl;
         ports << "         path=+I1L" << endl;
         ports << "         path+=I1R" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "   EndMode" << endl;
         ports << "   Mode" << endl;
         ports << "      Sport=2" << endl;
         ports << "      net=differential_in" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=voltage" << endl;
         ports << "         path=+V1L" << endl;
         ports << "         path-=V1R" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=current" << endl;
         ports << "         scale=0.5" << endl;   // correct only if symmetric
         ports << "         path=+I1L" << endl;
         ports << "         path-=I1R" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "   EndMode" << endl;
         ports << "EndPort" << endl;

         ports << endl;
         ports << "Port" << endl;
         ports << "   name=2" << endl;
         ports << "   path=+port2" << endl;
         ports << "   impedance_definition=PI" << endl;
         ports << "   impedance_calculation=modal" << endl;
         ports << "   Mode" << endl;
         ports << "      Sport=3" << endl;
         ports << "      net=common_out" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=voltage" << endl;
         ports << "         scale=0.5" << endl;
         ports << "         path=+V2L" << endl;
         ports << "         path+=V2R" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=current" << endl;
         ports << "         path=-I2L" << endl;
         ports << "         path-=I2R" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "   EndMode" << endl;
         ports << "   Mode" << endl;
         ports << "      Sport=4" << endl;
         ports << "      net=differential_out" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=voltage" << endl;
         ports << "         path=+V2L" << endl;
         ports << "         path-=V2R" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=current" << endl;
         ports << "         scale=0.5" << endl;   // correct only if symmetric
         ports << "         path=-I2L" << endl;
         ports << "         path+=I2R" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "   EndMode" << endl;
         ports << "EndPort" << endl;
      }

      if (solution_impedance_calculation.get_value().compare("line") == 0) {
         ports << endl;
         ports << "Port" << endl;
         ports << "   name=1" << endl;
         ports << "   path=+port1" << endl;
         ports << "   impedance_definition=PI" << endl;
         ports << "   impedance_calculation=line" << endl;
         ports << "   Line" << endl;
         ports << "      Sport=1" << endl;
         ports << "      net=in_P" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=voltage" << endl;
         ports << "         path=V1L" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=current" << endl;
         ports << "         path=I1L" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "   EndLine" << endl;
         ports << "   Line" << endl;
         ports << "      Sport=2" << endl;
         ports << "      net=in_N" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=voltage" << endl;
         ports << "         path=V1R" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=current" << endl;
         ports << "         path=I1R" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "   EndLine" << endl;
         ports << "   DifferentialPair" << endl;
         ports << "      Sport_P=1" << endl;
         ports << "      Sport_N=2" << endl;
         ports << "   EndDifferentialPair" << endl;
         ports << "EndPort" << endl;

         ports << endl;
         ports << "Port" << endl;
         ports << "   name=2" << endl;
         ports << "   path=+port2" << endl;
         ports << "   impedance_definition=PI" << endl;
         ports << "   impedance_calculation=line" << endl;
         ports << "   Line" << endl;
         ports << "      Sport=3" << endl;
         ports << "      net=out_P" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=voltage" << endl;
         ports << "         path=V2L" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=current" << endl;
         ports << "         path=-I2L" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "   EndLine" << endl;
         ports << "   Line" << endl;
         ports << "      Sport=4" << endl;
         ports << "      net=out_N" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=voltage" << endl;
         ports << "         path=V2R" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "      IntegrationPath" << endl;
         ports << "         type=current" << endl;
         ports << "         path=-I2R" << endl;
         ports << "      EndIntegrationPath" << endl;
         ports << "   EndLine" << endl;
         ports << "   DifferentialPair" << endl;
         ports << "      Sport_P=3" << endl;
         ports << "      Sport_N=4" << endl;
         ports << "   EndDifferentialPair" << endl;
         ports << "EndPort" << endl;
      }

      // impedance boundaries

      // trace left top

      if (trace_material_top.is_loaded()) boundary_material=trace_material_top.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=trace_left_top" << endl;
         ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=trace_left_top" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=trace_left_top" << endl;
         ports << "EndBoundary" << endl;
      }

      // trace right top

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=trace_right_top" << endl;
         ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=trace_right_top" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=trace_right_top" << endl;
         ports << "EndBoundary" << endl;
      }

      // trace left bottom

      if (trace_material_bottom.is_loaded()) boundary_material=trace_material_bottom.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=trace_left_bottom" << endl;
         ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=trace_left_bottom" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=trace_left_bottom" << endl;
         ports << "EndBoundary" << endl;
      }

      // trace right bottom

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=trace_right_bottom" << endl;
         ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=trace_right_bottom" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=trace_right_bottom" << endl;
         ports << "EndBoundary" << endl;
      }

      // trace left left side

      if (trace_material_sides.is_loaded()) boundary_material=trace_material_sides.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=trace_left_left_side" << endl;
         ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=trace_left_left_side" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=trace_left_left_side" << endl;
         ports << "EndBoundary" << endl;
      }

      // trace left right side

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=trace_left_right_side" << endl;
         ports << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=trace_left_right_side" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=trace_left_right_side" << endl;
         ports << "EndBoundary" << endl;
      }

      // trace right left side

      if (trace_material_sides.is_loaded()) boundary_material=trace_material_sides.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=trace_right_left_side" << endl;
         ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_air_gap.get_dbl_value()/2+etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=trace_right_left_side" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=trace_right_left_side" << endl;
         ports << "EndBoundary" << endl;
      }

      // trace right right side

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=trace_right_right_side" << endl;
         ports << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2-etch_offset << "," << lower_thickness.get_dbl_value()+trace_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=trace_right_right_side" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=trace_right_right_side" << endl;
         ports << "EndBoundary" << endl;
      }

      // lower groundplane

      if (lower_groundplane_material.is_loaded()) boundary_material=lower_groundplane_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=ground_plane" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << 0 << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << 0 << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=ground_plane" << endl;
         ports << "   type=surface_impedance" << endl;
         ports << "   material=" << boundary_material << endl;
         ports << "   path=ground_plane" << endl;
         ports << "EndBoundary" << endl;
      }

      // upper groundplane

      if (upper_groundplane_material.is_loaded()) boundary_material=upper_groundplane_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=upper_groundplane" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value()  << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value()  << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=upper_groundplane" << endl;
         if (boundary_material.compare("PMC") == 0) {
            ports << "   type=perfect_magnetic_conductor" << endl;
         } else {
            ports << "   type=surface_impedance" << endl;
            ports << "   material=" << boundary_material << endl;
         }
         ports << "   path=upper_groundplane" << endl;
         ports << "EndBoundary" << endl;
      }

      // left side

      if (left_side_material.is_loaded()) boundary_material=left_side_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=left_side" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << 0 << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << -left_side_gap.get_dbl_value()-trace_left_width.get_dbl_value()-trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=left_side" << endl;
         if (boundary_material.compare("PMC") == 0) {
           ports << "   type=perfect_magnetic_conductor" << endl;
         } else {
            ports << "   type=surface_impedance" << endl;
            ports << "   material=" << boundary_material << endl;
         }
         ports << "   path=left_side" << endl;
         ports << "EndBoundary" << endl;
      }

      // right side

      if (right_side_material.is_loaded()) boundary_material=right_side_material.get_value();
      else boundary_material=default_conductor_material.get_value();

      if (boundary_material.compare("PEC") != 0) {
         ports << endl;
         ports << "Path" << endl;
         ports << "   name=right_side" << endl;
         ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << 0 << "," << 0 << ")" << endl;
         ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << 0 << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << length.get_dbl_value() << ")" << endl;
         ports << setprecision(15) << "   point=(" << right_side_gap.get_dbl_value()+trace_right_width.get_dbl_value()+trace_air_gap.get_dbl_value()/2 << "," << lower_thickness.get_dbl_value()+upper_thickness.get_dbl_value() << "," << 0 << ")" << endl;
         ports << "   closed=true" << endl;
         ports << "EndPath" << endl;

         ports << endl;
         ports << "Boundary" << endl;
         ports << "   name=right_side" << endl;
         if (boundary_material.compare("PMC") == 0) {
           ports << "   type=perfect_magnetic_conductor" << endl;
         } else {
            ports << "   type=surface_impedance" << endl;
            ports << "   material=" << boundary_material << endl;
         }
         ports << "   path=right_side" << endl;
         ports << "EndBoundary" << endl;
      }

      ports.close();
   } else {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2188: Failed to open \"%s\" for writing.\n",
                                   indent.c_str(),ssFilename.str().c_str());
      fail=true;
   }
   return fail;
}

bool CoupledStrip::write_OpenParEM2D_proj (string indent)
{
   if (length.get_dbl_value() != 0) return false;

   string solution_impedance;
   if (solution_impedance_calculation.get_value().compare("modal") == 0) solution_impedance="modal";
   if (solution_impedance_calculation.get_value().compare("line") == 0) solution_impedance="line";

   return write_OpenParEM2D_proj_text (indent,name.get_value(),2,"PI",solution_impedance.c_str());
}

bool CoupledStrip::write_OpenParEM3D_proj (string indent)
{
   if (length.get_dbl_value() == 0) return false;
   return write_OpenParEM3D_proj_text (indent,name.get_value(),&default_conductor_material);
}

///////////////////////////////////////////////////////////////////////////////////////////
// StructureDatabase
///////////////////////////////////////////////////////////////////////////////////////////

bool StructureDatabase::findBlocks(bool checkLimits)
{
   bool fail=false;
   int start_lineNumber,stop_lineNumber;
   int block_start,block_stop;

   // Control

   // reset
   start_lineNumber=inputs.get_first_lineNumber();
   stop_lineNumber=inputs.get_last_lineNumber();

   // skip the version line, which must be the first line
   start_lineNumber=inputs.get_next_lineNumber(start_lineNumber);

   while (start_lineNumber < stop_lineNumber) {
      if (inputs.findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "Control", "EndControl", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            Control *newControl=new Control(block_start,block_stop,checkLimits);
            controlList.push_back(newControl);
         }
      }
      start_lineNumber=inputs.get_next_lineNumber(block_stop);
   }

   // RectangularWaveguide

   // reset
   start_lineNumber=inputs.get_first_lineNumber();
   stop_lineNumber=inputs.get_last_lineNumber();

   // skip the version line, which must be the first line
   start_lineNumber=inputs.get_next_lineNumber(start_lineNumber);

   while (start_lineNumber < stop_lineNumber) {
      if (inputs.findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "RectangularWaveguide", "EndRectangularWaveguide", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            RectangularWaveguide *newRectangularWaveguide=new RectangularWaveguide(block_start,block_stop,checkLimits);
            rectangularWaveguideList.push_back(newRectangularWaveguide);
         }
      }
      start_lineNumber=inputs.get_next_lineNumber(block_stop);
   }

   // Strip

   // reset
   start_lineNumber=inputs.get_first_lineNumber();
   stop_lineNumber=inputs.get_last_lineNumber();

   // skip the version line, which must be the first line
   start_lineNumber=inputs.get_next_lineNumber(start_lineNumber);

   while (start_lineNumber < stop_lineNumber) {
      if (inputs.findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "Strip", "EndStrip", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            Strip *newStrip=new Strip(block_start,block_stop,checkLimits);
            stripList.push_back(newStrip);
         }
      }
      start_lineNumber=inputs.get_next_lineNumber(block_stop);
   }

   // CoupledStrip

   // reset
   start_lineNumber=inputs.get_first_lineNumber();
   stop_lineNumber=inputs.get_last_lineNumber();

   // skip the version line, which must be the first line
   start_lineNumber=inputs.get_next_lineNumber(start_lineNumber);

   while (start_lineNumber < stop_lineNumber) {
      if (inputs.findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "CoupledStrip", "EndCoupledStrip", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            CoupledStrip *newCoupledStrip=new CoupledStrip(block_start,block_stop,checkLimits);
            coupledStripList.push_back(newCoupledStrip);
         }
      }
      start_lineNumber=inputs.get_next_lineNumber(block_stop);
   }


   return fail;
}

bool StructureDatabase::set_include()
{
   bool fail=false;

   // RectangularWaveguide

   long unsigned int i=0;
   while (i < rectangularWaveguideList.size()) {

      if (rectangularWaveguideList[i]->get_include().is_loaded()) {

         bool found=false;

         // Rectangular Waveguide
         long unsigned int j=0;
         while (j < rectangularWaveguideList.size()) {
            if (i != j && rectangularWaveguideList[i]->get_include().get_value().compare(rectangularWaveguideList[j]->get_name().get_value()) == 0) {
               rectangularWaveguideList[i]->set_included(rectangularWaveguideList[j]);
               rectangularWaveguideList[i]->set_included_type(0);
               found=true;
               break;
            }
            j++;
         }

         // Strip
         j=0;
         while (j < stripList.size()) {
            if (! found && rectangularWaveguideList[i]->get_include().get_value().compare(stripList[j]->get_name().get_value()) == 0) {
               rectangularWaveguideList[i]->set_included(stripList[j]);
               rectangularWaveguideList[i]->set_included_type(1);
               found=true;
               break;
            }
            j++;
         }

         // CoupledStrip
         j=0;
         while (j < coupledStripList.size()) {
            if (! found && rectangularWaveguideList[i]->get_include().get_value().compare(coupledStripList[j]->get_name().get_value()) == 0) {
               rectangularWaveguideList[i]->set_included(coupledStripList[j]);
               rectangularWaveguideList[i]->set_included_type(2);
               found=true;
               break;
            }
            j++;
         }

         if (! found) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2095: Include \"%s\" at line %d does not exist.\n",
                                         indent.c_str(),rectangularWaveguideList[i]->get_include().get_value().c_str(),rectangularWaveguideList[i]->get_include().get_lineNumber());
            fail=true;
         }
      }
      i++;
   }

   // Strip

   i=0;
   while (i < stripList.size()) {

      if (stripList[i]->get_include().is_loaded()) {

         bool found=false;

         // Strip
         long unsigned int j=0;
         while (j < stripList.size()) {
            if (i != j && stripList[i]->get_include().get_value().compare(stripList[j]->get_name().get_value()) == 0) {
               stripList[i]->set_included(stripList[j]);
               stripList[i]->set_included_type(1);
               found=true;
               break;
            }
            j++;
         }

         // Rectangular Waveguide
         j=0;
         while (j < rectangularWaveguideList.size()) {
            if (! found && stripList[i]->get_include().get_value().compare(rectangularWaveguideList[j]->get_name().get_value()) == 0) {
               stripList[i]->set_included(rectangularWaveguideList[j]);
               stripList[i]->set_included_type(0);
               found=true;
               break;
            }
            j++;
         }

         // CoupledStrip
         j=0;
         while (j < coupledStripList.size()) {
            if (! found && stripList[i]->get_include().get_value().compare(coupledStripList[j]->get_name().get_value()) == 0) {
               stripList[i]->set_included(coupledStripList[j]);
               stripList[i]->set_included_type(2);
               found=true;
               break;
            }
            j++;
         }

         if (! found) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2096: Include \"%s\" at line %d does not exist.\n",
                                         indent.c_str(),stripList[i]->get_include().get_value().c_str(),stripList[i]->get_include().get_lineNumber());
            fail=true;
         }
      }
      i++;
   }

   // CoupledStrip

   i=0;
   while (i < coupledStripList.size()) {

      if (coupledStripList[i]->get_include().is_loaded()) {

         bool found=false;

         // CoupledStrip
         long unsigned int j=0;
         while (j < coupledStripList.size()) {
            if (i != j && coupledStripList[i]->get_include().get_value().compare(coupledStripList[j]->get_name().get_value()) == 0) {
               coupledStripList[i]->set_included(coupledStripList[j]);
               coupledStripList[i]->set_included_type(2);
               found=true;
               break;
            }
            j++;
         }

         // RectangularWaveguide
         j=0;
         while (j < rectangularWaveguideList.size()) {
            if (! found && coupledStripList[i]->get_include().get_value().compare(rectangularWaveguideList[j]->get_name().get_value()) == 0) {
               coupledStripList[i]->set_included(rectangularWaveguideList[j]);
               coupledStripList[i]->set_included_type(0);
               found=true;
               break;
            }
            j++;
         }

         // Strip
         j=0;
         while (j < stripList.size()) {
            if (! found && coupledStripList[i]->get_include().get_value().compare(stripList[j]->get_name().get_value()) == 0) {
               coupledStripList[i]->set_included(stripList[j]);
               coupledStripList[i]->set_included_type(1);
               found=true;
               break;
            }
            j++;
         }

         if (! found) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2097: Include \"%s\" at line %d does not exist.\n",
                                         indent.c_str(),coupledStripList[i]->get_include().get_value().c_str(),coupledStripList[i]->get_include().get_lineNumber());
            fail=true;
         }
      }
      i++;
   }

   return fail;
}

void StructureDatabase::apply_includes()
{
   // RectangularWaveguide
   long unsigned int i=0;
   while (i < rectangularWaveguideList.size()) {
      rectangularWaveguideList[i]->apply_include();
      i++;
   }

   // Strip
   i=0;
   while (i < stripList.size()) {
      stripList[i]->apply_include();
      i++;
   }

   // CoupledStrip
   i=0;
   while (i < coupledStripList.size()) {
      coupledStripList[i]->apply_include();
      i++;
   }

}

// return true on fail
bool StructureDatabase::load(const char *path, const char *filename, bool checkInputs)
{
   // assemble the full path name
   char *fullPathName=(char *)malloc((strlen(path)+strlen(filename)+1)*sizeof(char));
   if (!fullPathName) return 1;
   sprintf (fullPathName,"%s%s",path,filename);
   PetscPrintf(PETSC_COMM_WORLD,"loading structures file \"%s\"\n",fullPathName);
 
   bool fail=false;
   if (inputs.load(fullPathName)) {if (fullPathName) free(fullPathName); fullPathName=nullptr; return true;}
   if (fullPathName) {free(fullPathName); fullPathName=nullptr;}
   inputs.createCrossReference();

   if (inputs.checkVersion(version_name, version_value)) {
      PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2098: Version mismatch.  Expecting the first line to be: %s %s\n",
                                   indent.c_str(),indent.c_str(),version_name.c_str(),version_value.c_str());
      return true;
   }

   // find the various blocks
   if (findBlocks(checkInputs)) fail=true;

   // load the various block types

   // Control
   long unsigned int i=0;
   while (i < controlList.size()) {
      if (controlList[i]->load(&indent, &inputs, checkInputs)) fail=true;
      i++;
   }

   // RectangularWaveguide
   i=0;
   while (i < rectangularWaveguideList.size()) {
      if (rectangularWaveguideList[i]->load(&indent, &inputs, checkInputs)) fail=true;
      i++;
   }

   // Strip
   i=0;
   while (i < stripList.size()) {
      if (stripList[i]->load(&indent, &inputs, checkInputs)) fail=true;
      i++;
   }

   // CoupledStrip
   i=0;
   while (i < coupledStripList.size()) {
      if (coupledStripList[i]->load(&indent, &inputs, checkInputs)) fail=true;
      i++;
   }

   return fail;
};

RectangularWaveguide* StructureDatabase::get_rectangularWaveguide(string name)
{
   long unsigned int i=0;
   while (i < rectangularWaveguideList.size()) {
      if (rectangularWaveguideList[i]->get_name().get_value().compare(name) == 0) return rectangularWaveguideList[i];
      i++;
   }
   return nullptr;
}

bool StructureDatabase::inBlocks(int lineNumber)
{
   long unsigned int i=0;
   while (i < controlList.size()) {
      if (controlList[i]->inBlock(lineNumber)) return true;
      i++;
   }

   i=0;
   while (i < rectangularWaveguideList.size()) {
      if (rectangularWaveguideList[i]->inBlock(lineNumber)) return true;
      i++;
   }

   i=0;
   while (i < stripList.size()) {
      if (stripList[i]->inBlock(lineNumber)) return true;
      i++;
   }

   i=0;
   while (i < coupledStripList.size()) {
      if (coupledStripList[i]->inBlock(lineNumber)) return true;
      i++;
   }

   return false;
}

void StructureDatabase::print()
{

   long unsigned int i=0;
   while (i < controlList.size()) {
      controlList[i]->print(indent);
      PetscPrintf(PETSC_COMM_WORLD,"\n");
      i++;
   }

   i=0;
   while (i < rectangularWaveguideList.size()) {
      rectangularWaveguideList[i]->print(indent);
      PetscPrintf(PETSC_COMM_WORLD,"\n");
      i++;
   }
}

bool StructureDatabase::check()
{
   bool fail=false;

   // Control

   if (controlList.size() == 0) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2099: One \"Contro/EndControl\" block must be specified.\n",indent.c_str());
      fail=true;
   }

   if (controlList.size() > 1) {
      PetscPrintf(PETSC_COMM_WORLD,"%sERROR2100: Only one \"Control/EndControl\" block can be specified.\n",indent.c_str());
      fail=true;
   }

   long unsigned int i=0;
   while (i < controlList.size()) {

      // single block checks
      if (controlList[i]->check(indent)) fail=true;

      i++;
   }

   // RectangularWaveguide
   i=0;
   while (i < rectangularWaveguideList.size()) {

      // single block checks
      if (rectangularWaveguideList[i]->check(indent)) fail=true;

      // cross-block checks
      long unsigned int j=i+1;
      while (i < rectangularWaveguideList.size()-1 && j < rectangularWaveguideList.size()) {
         if (rectangularWaveguideList[i]->get_name().get_value().compare(rectangularWaveguideList[j]->get_name().get_value()) == 0) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2101: name at line %d duplicates the name at line %d.\n",
                                         indent.c_str(),rectangularWaveguideList[j]->get_name().get_lineNumber(),rectangularWaveguideList[i]->get_name().get_lineNumber());
            fail=true;
         }
         j++;
      }

      j=0;
      while (j < stripList.size()) {
         if (rectangularWaveguideList[i]->get_name().get_value().compare(stripList[j]->get_name().get_value()) == 0) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2102: name at line %d duplicates the name at line %d.\n",
                                         indent.c_str(),stripList[j]->get_name().get_lineNumber(),rectangularWaveguideList[i]->get_name().get_lineNumber());
            fail=true;
         }
         j++;
      }

      j=0;
      while (j < coupledStripList.size()) {
         if (rectangularWaveguideList[i]->get_name().get_value().compare(coupledStripList[j]->get_name().get_value()) == 0) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2103: name at line %d duplicates the name at line %d.\n",
                                         indent.c_str(),coupledStripList[j]->get_name().get_lineNumber(),rectangularWaveguideList[i]->get_name().get_lineNumber());
            fail=true;
         }
         j++;
      }

      i++;
   }

   // Strip
   i=0;
   while (i < stripList.size()) {

      // single block checks
      if (stripList[i]->check(indent)) fail=true;

      // cross-block checks
      long unsigned int j=i+1;
      while (i < stripList.size()-1 && j < stripList.size()) {
         if (stripList[i]->get_name().get_value().compare(stripList[j]->get_name().get_value()) == 0) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2104: name at line %d duplicates the name at line %d.\n",
                                         indent.c_str(),stripList[j]->get_name().get_lineNumber(),stripList[i]->get_name().get_lineNumber());
            fail=true;
         }
         j++;
      }

      j=0;
      while (j < coupledStripList.size()) {
         if (stripList[i]->get_name().get_value().compare(coupledStripList[j]->get_name().get_value()) == 0) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2105: name at line %d duplicates the name at line %d.\n",
                                         indent.c_str(),coupledStripList[j]->get_name().get_lineNumber(),stripList[i]->get_name().get_lineNumber());
            fail=true;
         }
         j++;
      }

      i++;
   }

   // CoupledStrip
   i=0;
   while (i < coupledStripList.size()) {

      // single block checks
      if (coupledStripList[i]->check(indent)) fail=true;

      // cross-block checks
      long unsigned int j=i+1;
      while (i < coupledStripList.size()-1 && j < coupledStripList.size()) {
         if (coupledStripList[i]->get_name().get_value().compare(coupledStripList[j]->get_name().get_value()) == 0) {
            PetscPrintf(PETSC_COMM_WORLD,"%sERROR2106: name at line %d duplicates the name at line %d.\n",
                                         indent.c_str(),coupledStripList[j]->get_name().get_lineNumber(),coupledStripList[i]->get_name().get_lineNumber());
            fail=true;
         }
         j++;
      }
      i++;
   }

   // check for extraneous text
   i=1;  // skip the first line, which is the version information
   while (i < inputs.get_size()) {
      if (! inBlocks(inputs.get_lineNumber(i))) {
         PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR2107: Invalid input at line %d.\n",indent.c_str(),indent.c_str(),inputs.get_lineNumber(i));
         fail=true;
      }
      i++;
   }

   return fail;
}

bool StructureDatabase::checkInclude()
{
   bool fail=false;

   // RectangularWaveguide
   long unsigned int i=0;
   while (i < rectangularWaveguideList.size()) {
      if (rectangularWaveguideList[i]->checkInclude(indent)) fail=true;
      i++;
   }

   // Strip
   i=0;
   while (i < stripList.size()) {
      if (stripList[i]->checkInclude(indent)) fail=true;
      i++;
   }

   // CoupledStrip
   i=0;
   while (i < coupledStripList.size()) {
      if (coupledStripList[i]->checkInclude(indent)) fail=true;
      i++;
   }

   return fail;
}

bool StructureDatabase::checkLimits()
{
   bool fail=false;

   if (! controlList[0]->get_checkLimits().get_bool_value()) return fail;

   // RectangularWaveguide
   long unsigned int i=0;
   while (i < rectangularWaveguideList.size()) {
      if (rectangularWaveguideList[i]->checkLimits()) fail=true;
      i++;
   }

   // Strip
   i=0;
   while (i < stripList.size()) {
      if (stripList[i]->checkLimits()) fail=true;
      i++;
   }

   // CoupledStrip
   i=0;
   while (i < coupledStripList.size()) {
      if (coupledStripList[i]->checkLimits()) fail=true;
      i++;
   }

   return fail;
}

bool StructureDatabase::build_geo()
{
   // RectangularWaveguide
   long unsigned int i=0;
   while (i < rectangularWaveguideList.size()) {
      if (rectangularWaveguideList[i]->get_name().get_value().compare(controlList[0]->get_build().get_value()) == 0) {
         rectangularWaveguideList[i]->build_geo(&geo);
         return false;
      }
      i++;
   }

   // Strip
   i=0;
   while (i < stripList.size()) {
      if (stripList[i]->get_name().get_value().compare(controlList[0]->get_build().get_value()) == 0) {
         stripList[i]->build_geo(&geo,true);
         return false;
      }
      i++;
   }

   // CoupledStrip
   i=0;
   while (i < coupledStripList.size()) {
      if (coupledStripList[i]->get_name().get_value().compare(controlList[0]->get_build().get_value()) == 0) {
         coupledStripList[i]->build_geo(&geo);
         return false;
      }
      i++;
   }

   PetscPrintf(PETSC_COMM_WORLD,"%sERROR2108: build target \"%s\" not found.\n",
                                 indent.c_str(),controlList[0]->get_build().get_value().c_str());

   return true;
}

bool StructureDatabase::write_geo()
{
   // RectangularWaveguide
   long unsigned int i=0;
   while (i < rectangularWaveguideList.size()) {
      if (rectangularWaveguideList[i]->get_name().get_value().compare(controlList[0]->get_build().get_value()) == 0) {
         if (rectangularWaveguideList[i]->write_geo(indent,controlList[0],&geo)) return true;
         return false;
      }
      i++;
   }

   // Strip
   i=0;
   while (i < stripList.size()) {
      if (stripList[i]->get_name().get_value().compare(controlList[0]->get_build().get_value()) == 0) {
         if (stripList[i]->write_geo(indent,controlList[0],&geo)) return true;
         return false;
      }
      i++;
   }

   // CoupledStrip
   i=0;
   while (i < coupledStripList.size()) {
      if (coupledStripList[i]->get_name().get_value().compare(controlList[0]->get_build().get_value()) == 0) {
         if (coupledStripList[i]->write_geo(indent,controlList[0],&geo)) return true;
         return false;
      }
      i++;
   }

   PetscPrintf(PETSC_COMM_WORLD,"%sERROR2109: build target \"%s\" not found.\n",
                                 indent.c_str(),controlList[0]->get_build().get_value().c_str());

   return true;
}

bool StructureDatabase::write_modes_and_boundaries()
{
   // RectangularWaveguide
   long unsigned int i=0;
   while (i < rectangularWaveguideList.size()) {
      if (rectangularWaveguideList[i]->get_name().get_value().compare(controlList[0]->get_build().get_value()) == 0) {
         if (rectangularWaveguideList[i]->write_modes_and_boundaries(indent,controlList[0])) return true;
         if (rectangularWaveguideList[i]->write_ports_and_boundaries(indent,controlList[0])) return true;
         return false;
      }
      i++;
   }

   // Strip
   i=0;
   while (i < stripList.size()) {
      if (stripList[i]->get_name().get_value().compare(controlList[0]->get_build().get_value()) == 0) {
         if (stripList[i]->write_modes_and_boundaries(indent,controlList[0])) return true;
         if (stripList[i]->write_ports_and_boundaries(indent,controlList[0])) return true;
         return false;
      }
      i++;
   }

   // CoupledStrip
   i=0;
   while (i < coupledStripList.size()) {
      if (coupledStripList[i]->get_name().get_value().compare(controlList[0]->get_build().get_value()) == 0) {
         if (coupledStripList[i]->write_modes_and_boundaries(indent,controlList[0])) return true;
         if (coupledStripList[i]->write_ports_and_boundaries(indent,controlList[0])) return true;
         return false;
      }
      i++;
   }

   PetscPrintf(PETSC_COMM_WORLD,"%sERROR2110: build target \"%s\" not found.\n",
                                 indent.c_str(),controlList[0]->get_build().get_value().c_str());

   return true;
}

bool StructureDatabase::write_proj()
{
   // RectangularWaveguide
   long unsigned int i=0;
   while (i < rectangularWaveguideList.size()) {
      if (rectangularWaveguideList[i]->get_name().get_value().compare(controlList[0]->get_build().get_value()) == 0) {
         if (rectangularWaveguideList[i]->write_OpenParEM2D_proj(indent)) return true;
         if (rectangularWaveguideList[i]->write_OpenParEM3D_proj(indent)) return true;
         return false;
      }
      i++;
   }

   // Strip
   i=0;
   while (i < stripList.size()) {
      if (stripList[i]->get_name().get_value().compare(controlList[0]->get_build().get_value()) == 0) {
         if (stripList[i]->write_OpenParEM2D_proj(indent)) return true;
         if (stripList[i]->write_OpenParEM3D_proj(indent)) return true;
         return false;
      }
      i++;
   }

   // CoupledStrip
   i=0;
   while (i < coupledStripList.size()) {
      if (coupledStripList[i]->get_name().get_value().compare(controlList[0]->get_build().get_value()) == 0) {
         if (coupledStripList[i]->write_OpenParEM2D_proj(indent)) return true;
         if (coupledStripList[i]->write_OpenParEM3D_proj(indent)) return true;
         return false;
      }
      i++;
   }

   PetscPrintf(PETSC_COMM_WORLD,"%sERROR2111: build target \"%s\" not found.\n",
                                 indent.c_str(),controlList[0]->get_build().get_value().c_str());

   return true;
}

StructureDatabase::~StructureDatabase()
{
   long unsigned int i=0;
   while (i < controlList.size()) {
      delete controlList[i];
      i++;
   }

   i=0;
   while (i < rectangularWaveguideList.size()) {
      delete rectangularWaveguideList[i];
      i++;
   }

   i=0;
   while (i < stripList.size()) {
      delete stripList[i];
      i++;
   }

   i=0;
   while (i < coupledStripList.size()) {
      delete coupledStripList[i];
      i++;
   }

}

///////////////////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////////////////

void printHelp ()
{
   PetscPrintf(PETSC_COMM_WORLD,"Usage: builder [-h] [-help] filename\n");
   PetscPrintf(PETSC_COMM_WORLD,"       -h|-help   Print this helpfile.\n");
   PetscPrintf(PETSC_COMM_WORLD,"       Builder is a utility program for OpenParEM2D and OpenParEM3D to build geometries and write setups\n");
   PetscPrintf(PETSC_COMM_WORLD,"       for common transmission line and waveguide types.\n");
}

int main (int argc, char *argv[])
{
   string project_name,modes_filename;
   StructureDatabase structureDatabase;

   // Initialize Petsc and MPI
   PetscInitializeNoArguments();

   int i=0;
   while (i < argc) {
      if (strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"-help") == 0) {
         printHelp();
         exit (0);
      }
      i++;
   }

   if (argc != 2) {
      printHelp();
      exit (1);
   }

   if (structureDatabase.load("./",argv[1],false)) exit (1);  // don't check limits during loading
   if (structureDatabase.checkInclude()) exit(1);
   if (structureDatabase.set_include()) exit(1);
   structureDatabase.apply_includes();
   if (structureDatabase.check()) exit(1);
   if (structureDatabase.checkLimits()) exit(1);

   // write the needed files to run OpenParEM2D or OpenParEM3D

   if (! structureDatabase.build_geo()) {
      structureDatabase.write_geo();
      structureDatabase.write_modes_and_boundaries();
      structureDatabase.write_proj();
      //structureDatabase.audit();
   }

   PetscFinalize();
   
   return 0;
}

