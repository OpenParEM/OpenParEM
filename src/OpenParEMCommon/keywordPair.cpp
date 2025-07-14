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

#include "OpenParEMmaterials.hpp"

///////////////////////////////////////////////////////////////////////////////////////////
// struct point
///////////////////////////////////////////////////////////////////////////////////////////

struct point set_point (double x, double y, double z, int dim)
{
   struct point a;
   a.dim=dim;
   a.x=x;
   a.y=y;
   a.z=z;
   return a;
}

struct point point_copy (struct point a)
{
   struct point b;
   b.dim=a.dim;
   b.x=a.x;
   b.y=a.y;
   b.z=a.z;
   return b;
}

// c=a-b
struct point point_subtraction (struct point a, struct point b)
{
   struct point c;

   if (a.dim != b.dim) {
      c.dim=a.dim; c.x=-DBL_MAX; c.y=-DBL_MAX; c.z=-DBL_MAX;
      return c;
   }

   c.dim=a.dim;
   c.x=a.x-b.x;
   c.y=a.y-b.y;
   c.z=a.z-b.z;

   return c;
}

// c=a+b
struct point point_addition (struct point a, struct point b)
{
   struct point c;

   if (a.dim != b.dim) {
      c.dim=a.dim; c.x=-DBL_MAX; c.y=-DBL_MAX; c.z=-DBL_MAX;
      return c;
   }

   c.dim=a.dim;
   c.x=a.x+b.x;
   c.y=a.y+b.y;
   c.z=a.z+b.z;

   return c;
}

// c=alpha*a
struct point point_scale (double alpha, struct point a)
{
   struct point c;

   c.dim=a.dim;
   c.x=alpha*a.x;
   c.y=alpha*a.y;
   c.z=alpha*a.z;

   return c;
}

// (a+b)/2
struct point point_midpoint (struct point a, struct point b)
{
   struct point c;

   if (a.dim != b.dim) {
      c.dim=a.dim; c.x=-DBL_MAX; c.y=-DBL_MAX; c.z=-DBL_MAX;
      return c;
   }

   c=point_scale(0.5,point_addition(a,b));

   return c;
}

// c=axb
struct point point_cross_product (struct point a, struct point b)
{
   struct point c;

   if (a.dim != b.dim) {
      c.dim=a.dim; c.x=-DBL_MAX; c.y=-DBL_MAX; c.z=-DBL_MAX;
      return c;
   }

   if (a.dim == 2) {
      c.dim=3;
      c.x=0;
      c.y=0;
      c.z=a.x*b.y-b.x*a.y;
   } else {
      c.dim=3;
      c.x=a.y*b.z-b.y*a.z;
      c.y=-a.x*b.z+b.x*a.z;
      c.z=a.x*b.y-b.x*a.y;
   }

   return c;
}

// c=a.b
double point_dot_product (struct point a, struct point b)
{
   if (a.dim != b.dim) return -DBL_MAX;
   double dotproduct=a.x*b.x+a.y*b.y;
   if (a.dim == 3) dotproduct+=a.z*b.z;
   return dotproduct;
}

// c=mag(a)
double point_magnitude (struct point a)
{
   if (a.dim == 2) return sqrt(pow(a.x,2)+pow(a.y,2));
   return sqrt(pow(a.x,2)+pow(a.y,2)+pow(a.z,2));
}

struct point point_normalize (struct point a)
{
   return point_scale(1/point_magnitude(a),a);
}

bool point_comparison (struct point a, struct point b, double tolerance)
{
   if (a.dim != b.dim) return false;
   if (!double_compare(a.x,b.x,tolerance)) return false;
   if (!double_compare(a.y,b.y,tolerance)) return false;
   if (a.dim == 3 && !double_compare(a.z,b.z,tolerance)) return false;
   return true;
}

void point_print (struct point a)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"struct point:\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   dim=%d\n",a.dim);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   x=%g\n",a.x);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   y=%g\n",a.y);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   z=%g\n",a.z);
}


///////////////////////////////////////////////////////////////////////////////////////////
// keywordPair
///////////////////////////////////////////////////////////////////////////////////////////

bool keywordPair::int_limit_checks (string *keyword, int lineNumber)
{
   if (positive_required && int_value <= 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1017: %s at line %d is required to be positive.\n",
                                             indent.c_str(),indent.c_str(),keyword->c_str(),lineNumber);
      return false;
   }

   if (non_negative_required && int_value < 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1018: %s at line %d is required to be non-negative.\n",
                                             indent.c_str(),indent.c_str(),keyword->c_str(),lineNumber);
      return false;
   }

   if (int_value < lowerLimit) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1019: %s at line %d is required to be >= %g.\n",
                                             indent.c_str(),indent.c_str(),keyword->c_str(),lineNumber,lowerLimit);
      return false;
   }

   if (int_value > upperLimit) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1020: %s at line %d is required to be <= %g.\n",
                                             indent.c_str(),indent.c_str(),keyword->c_str(),lineNumber,upperLimit);
      return false;
   }

   return true;
}

bool keywordPair::dbl_limit_checks (string *keyword, int lineNumber)
{
   if (positive_required && dbl_value <= 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1021: %s at line %d is required to be positive.\n",
                                             indent.c_str(),indent.c_str(),keyword->c_str(),lineNumber);
      return false; 
   }

   if (non_negative_required && dbl_value < 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1022: %s at line %d is required to be non-negative.\n",
                                             indent.c_str(),indent.c_str(),keyword->c_str(),lineNumber);
      return false;
   }

   if (dbl_value < lowerLimit*(1-dbl_tolerance)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1023: %s at line %d is required to be >= %g.\n",
                                             indent.c_str(),indent.c_str(),keyword->c_str(),lineNumber,lowerLimit);
      return false;
   }

   if (dbl_value > upperLimit*(1+dbl_tolerance)) { 
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1024: %s at line %d is required to be <= %g.\n",
                                             indent.c_str(),indent.c_str(),keyword->c_str(),lineNumber,upperLimit);
      return false;
   }

   return true;
}

bool keywordPair::point_limit_checks (string *keyword, int lineNumber)
{
   if (positive_required && (point_value.x <= 0 || point_value.y <= 0 || (point_value.dim == 3 && point_value.z <= 0))) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1025: %s at line %d is required to be positive.\n",
                                             indent.c_str(),indent.c_str(),keyword->c_str(),lineNumber);
      return false;
   }

   if (non_negative_required && (point_value.x < 0 || point_value.y < 0 || (point_value.dim == 3 && point_value.z < 0))) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1026: %s at line %d is required to be non-negative.\n",
                                             indent.c_str(),indent.c_str(),keyword->c_str(),lineNumber);
      return false;
   }

   if (point_value.x < lowerLimit*(1-dbl_tolerance) || point_value.y < lowerLimit*(1-dbl_tolerance) || (point_value.dim == 3 && point_value.z < lowerLimit*(1-dbl_tolerance))) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1027: %s at line %d is required to be >= %g.\n",
                                             indent.c_str(),indent.c_str(),keyword->c_str(),lineNumber,lowerLimit);

      return false;
   }

   if (point_value.x > upperLimit*(1+dbl_tolerance) || point_value.y > upperLimit*(1+dbl_tolerance) || (point_value.dim == 3 && point_value.z > upperLimit*(1+dbl_tolerance))) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1028: %s at line %d is required to be <= %g.\n",
                                             indent.c_str(),indent.c_str(),keyword->c_str(),lineNumber,upperLimit);
      return false;
   }

   return true;
}

bool keywordPair::limit_check (string type)
{
   bool fail=false;

   if (type.compare("int") == 0) {
      if (! int_limit_checks (&keyword, lineNumber)) fail=true;
   } else if (type.compare("double") == 0) {
      if (! dbl_limit_checks (&keyword, lineNumber)) fail=true;
   } else if (type.compare("point") == 0) {
      if (! point_limit_checks (&keyword, lineNumber)) fail=true;
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: bad selection in keywordPair::limit_check\n");
   }

   return fail;
}

bool keywordPair::match_alias (string *token)
{
   long unsigned int i=0;
   while (i < aliases.size()) {
      if (aliases[i].compare(*token) == 0) return true;
      i++;
   }
   return false;
}

bool keywordPair::loadBool (string *token, string *value_, int lineNumber_)
{
   // check for duplicate
   if (loaded) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1029: Duplicate entry at line %d for previous entry at line %d.\n",
                                             indent.c_str(),indent.c_str(),lineNumber_,lineNumber);
      return true;
   }

   // check for a boolean
   if (value_->compare("true") != 0 && value_->compare("false") != 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1030: %s value at line %d is invalid.\n",
                                             indent.c_str(),indent.c_str(),token->c_str(),lineNumber_);
      return true;
   }

   // get the value
   if (value_->compare("true") == 0) bool_value=true;
   else bool_value=false;

   // save it
   keyword=*token;
   value=*value_;
   lineNumber=lineNumber_;
   loaded=true;

   return false;
}

bool keywordPair::loadInt (string *token, string *value_, int lineNumber_)
{
   // check for duplicate
   if (loaded) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1031: Duplicate entry at line %d for previous entry at line %d.\n",
                                             indent.c_str(),indent.c_str(),lineNumber_,lineNumber);
      return true;
   }

   // check for a pure number
   if (!is_int(value_)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1032: %s value at line %d is invalid.\n",
                                             indent.c_str(),indent.c_str(),token->c_str(),lineNumber_);
      return true;
   }

   // get the value
   try {int_value=stoi(*value_);}
   catch (const std::invalid_argument& ia) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1033: %s value at line %d is invalid.\n",
                                             indent.c_str(),indent.c_str(),token->c_str(),lineNumber_);
      return true;
   }

   // check the limits
   if (checkLimits && ! int_limit_checks (token, lineNumber_)) return true;

   // save it
   keyword=*token;
   value=*value_;
   lineNumber=lineNumber_;
   loaded=true;

   return false;
}

bool keywordPair::loadDouble (string *token, string *value_, int lineNumber_)
{
   // check for duplicate
   if (loaded) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1112: Duplicate entry at line %d for previous entry at line %d.\n",
                                             indent.c_str(),indent.c_str(),lineNumber_,lineNumber);
      return true;
   }

   // check for a pure number
   if (!is_double(value_)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1111: %s value at line %d is invalid.\n",
                                             indent.c_str(),indent.c_str(),token->c_str(),lineNumber_);
      return true;
   }

   // get the value
   try {dbl_value=stod(*value_);}
   catch (const std::invalid_argument& ia) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1113: %s value at line %d is invalid.\n",
                                              indent.c_str(),indent.c_str(),token->c_str(),lineNumber_);
      return true;
   }

   // check the limits
   if (checkLimits && ! dbl_limit_checks (token, lineNumber_)) return true;

   // save it
   keyword=*token;
   value=*value_;
   lineNumber=lineNumber_;
   loaded=true;

   return false;
}

bool keywordPair::loadPoint (int dim, string *token, string *value_, int lineNumber_)
{
   // check for duplicate
   if (loaded) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1114: Duplicate entry at line %d for previous entry at line %d.\n",
                                             indent.c_str(),indent.c_str(),lineNumber_,lineNumber);
      return true;
   }

   // check
   if (!is_point(value_,dim)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1115: %s value at line %d is invalid.\n",
                                             indent.c_str(),indent.c_str(),token->c_str(),lineNumber_);
      return true;
   }

   // get values
   if (point_get (value_, &point_value.x, &point_value.y, &point_value.z, dim, indent, lineNumber_)) return true;
   point_value.dim=dim;

   // check the limits
   if (checkLimits && ! point_limit_checks (token, lineNumber_)) return true;

   point_value.dim=dim;
   loaded=true;

   return false;
}

bool keywordPair::int_compare (keywordPair *test)
{
   if (int_value == test->int_value) return true;
   return false;
}

bool keywordPair::dbl_compare (keywordPair *test)
{
   if (dbl_value == test->dbl_value) return true;
   if (dbl_value == 0 && fabs(test->dbl_value) < dbl_tolerance) return true;
   if (test->dbl_value == 0 && fabs(dbl_value) < dbl_tolerance) return true;
   if (fabs((dbl_value-test->dbl_value)/dbl_value) < dbl_tolerance) return true;
   return false;
}

bool keywordPair::value_compare (keywordPair *test)
{
   if (value.compare(test->value) == 0) return true;
   return false;
}

bool keywordPair::point_compare (keywordPair *a)
{
   if (point_value.dim != a->point_value.dim) return false;
   if (! double_compare(point_value.x,a->point_value.x,dbl_tolerance)) return false;
   if (! double_compare(point_value.y,a->point_value.y,dbl_tolerance)) return false;
   if (point_value.dim == 3 && ! double_compare(point_value.z,a->point_value.z,dbl_tolerance)) return false;
   return true;
}

double keywordPair::get_point_distance (keywordPair *p)
{
   if (point_value.dim != p->point_value.dim) return -DBL_MAX;
   struct point a=get_point_value();
   struct point b=p->get_point_value();
   return point_magnitude(point_subtraction(a,b));
}

bool keywordPair::is_close_point (keywordPair *a)
{
   return point_comparison(get_point_value(),a->get_point_value(),1e-12);
}

// note the change in tolerance from above
bool keywordPair::is_close_point (struct point p)
{
   return point_comparison(get_point_value(),p,1e-8);
}

double keywordPair::distance_to_point (struct point b)
{
   struct point a=get_point_value();
   a=point_subtraction(a,b);
   return point_magnitude(a);
}

void keywordPair::copy (keywordPair a)
{
   aliases.clear();
   long unsigned int i=0;
   while (i < a.aliases.size()) {
      aliases.push_back(a.aliases[i]);
      i++;
   }

   keyword=a.keyword;
   value=a.value;
   lineNumber=a.lineNumber;
   int_value=a.int_value;
   dbl_value=a.dbl_value;
   bool_value=a.bool_value;
   point_value=a.point_value;
   loaded=a.loaded;
   lowerLimit=a.lowerLimit;
   upperLimit=a.upperLimit;
   positive_required=a.positive_required;
   non_negative_required=a.non_negative_required;
   indent=a.indent;
   dbl_tolerance=a.dbl_tolerance;
   checkLimits=a.checkLimits;
}

keywordPair* keywordPair::clone ()
{
   keywordPair *b=new keywordPair();
   b->copy(*this);
   return b;
}

void keywordPair::print()
{
  long unsigned int i=0;
  while (i < aliases.size()) {
     prefix(); PetscPrintf(PETSC_COMM_WORLD,"alias: %s\n",aliases[i].c_str());
     i++;
  }
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"keyword: %s\n",keyword.c_str());
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"value: %s\n",value.c_str());
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"lineNumber: %d\n",lineNumber);
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"int_value: %d\n",int_value);
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"dbl_value: %g\n",dbl_value);
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"loaded: %d\n",loaded);
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"lowerLimit: %g\n",lowerLimit);
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"upperLimit: %g\n",upperLimit);
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"postive_required: %d\n",positive_required);
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"non_negative_required: %d\n",non_negative_required);
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"indent: [%s]\n",indent.c_str());
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"dbl_tolerance: %g\n",dbl_tolerance);
  prefix(); PetscPrintf(PETSC_COMM_WORLD,"checkLimits: %d\n",checkLimits);
}

