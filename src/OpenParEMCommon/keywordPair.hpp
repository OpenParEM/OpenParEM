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

#ifndef KEYWORD_H
#define KEYWORD_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <limits>
#include <cfloat>
#include "misc.hpp"
#include "prefix.h"

extern "C" void prefix ();

void get_token_pair (std::string *, std::string *, std::string *, int *, std::string);
bool is_double (std::string *);
bool is_int (std::string *);
bool double_compare (double, double, double);

struct point {
   int dim;
   double x;
   double y;
   double z;
};

// ToDo: refactor the code in this file to use the new, more general, calculations using struct point below

struct point set_point (double, double, double, int);
struct point point_subtraction (struct point, struct point);
struct point point_addition (struct point, struct point);
struct point point_scale (double, struct point);
struct point point_midpoint (struct point, struct point);
struct point point_cross_product (struct point, struct point);

class keywordPair
{
   private:
      std::vector<std::string> aliases;  // token name plus aliases ex. "frequency", "freq", "f"
      std::string keyword;               // the text value from the input file
      std::string value;                 // the text value from the input file
      int lineNumber;

      int int_value;
      double dbl_value;
      bool bool_value;
      struct point point_value;

      bool loaded;

      double lowerLimit;
      double upperLimit;
      bool positive_required;
      bool non_negative_required;

      std::string indent="   ";  // for error messages
      double dbl_tolerance=1e-14;
      bool checkLimits=true;
   public:
      void push_alias (std::string a) {aliases.push_back(a);}
      bool match_alias (std::string *);
      void set_keyword (std::string a) {keyword=a;}
      void set_value (std::string a) {value=a;}
      void set_int_value (int i) {int_value=i;}
      void set_point_value_dim (int dim_) {point_value.dim=dim_;}
      void set_point_value (double x, double y) {point_value.x=x; point_value.y=y; point_value.dim=2;}
      void set_point_value (double x, double y, double z) {point_value.x=x; point_value.y=y; point_value.z=z; point_value.dim=3;}
      void set_point_value (struct point p) {point_value.x=p.x; point_value.y=p.y; point_value.z=p.z; point_value.dim=p.dim;}
      void set_dbl_value (double a) {dbl_value=a;}
      void set_bool_value (bool a) {bool_value=a;}
      void set_lineNumber (int a) {lineNumber=a;}
      void set_loaded (bool a) {loaded=a;}
      void set_positive_required (bool a) {positive_required=a;}
      void set_non_negative_required (bool a) {non_negative_required=a;}
      void set_lowerLimit (double a) {lowerLimit=a;}
      void set_upperLimit (double a) {upperLimit=a;}
      void set_checkLimits (bool a) {checkLimits=a;}
      bool is_loaded () {return loaded;}
      std::string get_keyword () {return keyword;}
      std::string get_value () {return value;}
      int get_lineNumber () {return lineNumber;}
      int get_int_value () {return int_value;}
      struct point get_point_value () {return point_value;}
      int get_point_value_dim () {return point_value.dim;}
      bool is_close_point (keywordPair *, double);
      bool is_close_point (struct point, double);
      double distance_to_point (struct point);
      double get_dbl_value () {return dbl_value;}
      bool get_bool_value () {return bool_value;}
      bool int_compare (keywordPair *);
      bool dbl_compare (keywordPair *);
      bool value_compare (keywordPair *);
      bool point_compare (keywordPair *);
      double get_point_distance (keywordPair *);
      bool loadBool (std::string *, std::string *, int);
      bool loadInt (std::string *, std::string *, int);
      bool loadDouble (std::string *, std::string *, int);
      bool loadPoint (int, std::string *, std::string *, int);
      bool int_limit_checks (std::string *, int);
      bool dbl_limit_checks (std::string *, int);
      bool point_limit_checks (std::string *, int);
      bool limit_check (std::string);

      void copy (keywordPair a);
      keywordPair* clone ();

      bool is_any () {
         if (value.compare("any") == 0) return true;
         return false;
      }
      void print ();
};

#endif

