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

#ifndef FIELDPOINTS_H
#define FIELDPOINTS_H

#include "mfem.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "misc.hpp"
#include "project.h"

extern "C" void prefix ();
extern "C" char* get_prefix_text ();
extern "C" void set_prefix_text (char *);

using namespace std;
using namespace mfem;

class FieldPoint {
   private:
      double frequency;
      unsigned long int mode;
      int dim;
      double x,y,z;
      double Ex_re,Ey_re,Ez_re;
      double Ex_im,Ey_im,Ez_im;
      double Hx_re,Hy_re,Hz_re;
      double Hx_im,Hy_im,Hz_im;
      double tolerance=1e-14;            // for comparing doubles
      int frequency_unique_index;
      double fieldMagLimit=1e-8;        // breakover point for "equal" vs. "lessthan" comparisons for fields
      double equalErrorLimit=1e-12;      // see also waveguide.c, results.cpp, fieldPoints.cpp
      double lessthanErrorLimit=1e-12;   // see also waveguide.c, results.cpp, fieldPoints.cpp
   public:
      void set_frequency (double frequency_) {frequency=frequency_;}
      void set_mode (unsigned long int mode_) {mode=mode_;}
      void set_dim (int dim_) {dim=dim_;}
      void set_x (double x_) {x=x_;}
      void set_y (double y_) {y=y_;}
      void set_z (double z_) {z=z_;}

      void set (double Ex_re_, double Ex_im_, double Ey_re_, double Ey_im_, double Ez_re_, double Ez_im_,
                double Hx_re_, double Hx_im_, double Hy_re_, double Hy_im_, double Hz_re_, double Hz_im_) {
         Ex_re=Ex_re_; Ex_im=Ex_im_; Ey_re=Ey_re_; Ey_im=Ey_im_; Ez_re=Ez_re_; Ez_im=Ez_im_;
         Hx_re=Hx_re_; Hx_im=Hx_im_; Hy_re=Hy_re_; Hy_im=Hy_im_; Hz_re=Hz_re_; Hz_im=Hz_im_;
      }

      void set_Ex_re (double Ex_re_) {Ex_re=Ex_re_;}
      void set_Ex_im (double Ex_im_) {Ex_im=Ex_im_;}
      void set_Ey_re (double Ey_re_) {Ey_re=Ey_re_;}
      void set_Ey_im (double Ey_im_) {Ey_im=Ey_im_;}
      void set_Ez_re (double Ez_re_) {Ez_re=Ez_re_;}
      void set_Ez_im (double Ez_im_) {Ez_im=Ez_im_;}
      void set_Hx_re (double Hx_re_) {Hx_re=Hx_re_;}
      void set_Hx_im (double Hx_im_) {Hx_im=Hx_im_;}
      void set_Hy_re (double Hy_re_) {Hy_re=Hy_re_;}
      void set_Hy_im (double Hy_im_) {Hy_im=Hy_im_;}
      void set_Hz_re (double Hz_re_) {Hz_re=Hz_re_;}
      void set_Hz_im (double Hz_im_) {Hz_im=Hz_im_;}
      void set_frequency_unique_index(int i) {frequency_unique_index=i;}
      void get (double *Ex_re_, double *Ex_im_, double *Ey_re_, double *Ey_im_, double *Ez_re_, double *Ez_im_,
                double *Hx_re_, double *Hx_im_, double *Hy_re_, double *Hy_im_, double *Hz_re_, double *Hz_im_) {
         *Ex_re_=Ex_re; *Ex_im_=Ex_im; *Ey_re_=Ey_re; *Ey_im_=Ey_im; *Ez_re_=Ez_re; *Ez_im_=Ez_im;
         *Hx_re_=Hx_re; *Hx_im_=Hx_im; *Hy_re_=Hy_re; *Hy_im_=Hy_im; *Hz_re_=Hz_re; *Hz_im_=Hz_im;
      }
      double get_frequency() {return frequency;}
      unsigned long int get_mode() {return mode;}
      double get_tolerance() {return tolerance;}
      bool compare (FieldPoint *);
      void overwrite (FieldPoint *);
      void copy (FieldPoint *);
      void save (ofstream *);
      void save_field_component (ofstream *, const char *, int *, const char *, double, double, double, double, double);
      void save_as_test(ofstream *, const char *, int *);
      void print();
};

class FieldPointDatabase {
   private:
      vector<FieldPoint *> fieldPointList;
   public:
      ~FieldPointDatabase();
      FieldPoint* get_FieldPoint(long unsigned int i) {return fieldPointList[i];}
      void push(FieldPoint *);
      void save(struct projectData *, const char *);
      void save_as_test(const char *, const char *);
      void normalize();
      void print();
};

#endif

