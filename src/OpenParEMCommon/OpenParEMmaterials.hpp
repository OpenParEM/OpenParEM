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

#ifndef MATERIALS_H
#define MATERIALS_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <limits>
#include <cfloat>
#include "fem.hpp"
#include "keywordPair.hpp"
#include "misc.hpp"
#include "prefix.h"

using namespace std;

class Frequency
{
   private:
      int startLine;                     // inclusive of "Frequency"
      int endLine;                       // inclusive of "EndFrequency"
      keywordPair frequency;
      keywordPair relative_permittivity;
      keywordPair relative_permeability;
      keywordPair loss;                  // loss tangent or conductivity
      keywordPair Rz;                    // surface roughness
   public:
      Frequency (int,int,bool);
      Frequency () {}
      bool load (string *, inputFile *);
      bool inFrequencyBlock (int);
      keywordPair* get_frequency () {return &frequency;}
      keywordPair* get_relative_permittivity () {return &relative_permittivity;}
      keywordPair* get_relative_permeability () {return &relative_permeability;}
      keywordPair* get_loss () {return &loss;}
      keywordPair* get_Rz () {return &Rz;}
      int get_startLine () {return startLine;}
      void print (string);
      bool check (string);
};


class Temperature
{
   private:
      int startLine;                     // inclusive of "Temperature"
      int endLine;                       // inclusive of "EndTemperature"
      vector<Frequency *> frequencyList;
      keywordPair temperature;
      keywordPair er_infinity;           // for Debye model - no frequency blocks with Debye and vice versa
      keywordPair delta_er;
      keywordPair m1;
      keywordPair m2;
      keywordPair relative_permeability;
      keywordPair loss;                  // loss tangent or conductivity
   public:
      Temperature (int,int,bool);
      Temperature (){}
      ~Temperature ();
      bool findFrequencyBlocks (inputFile *, bool);
      bool inFrequencyBlocks (int);
      bool inTemperatureBlock (int);
      keywordPair* get_temperature () {return &temperature;}
      Frequency* get_frequency (int i) {return frequencyList[i];}
      int get_startLine () {return startLine;}
      complex<double> get_eps (double, double, string);
      double get_mu (double, double, string);
      double get_Rs (double, double, string);
      bool load (string *, inputFile *, bool);
      void print (string);
      bool check (string);
};

class Source
{
   private:
      int startLine;  // inclusive of "Source"
      int endLine;    // inclusive of "EndSource"
      vector<int> lineNumberList;
      vector<string> lineList;
   public:
      Source (int,int);  // startLine,endLine
      bool inSourceBlock (int);
      bool load (inputFile *);
      void print (string);
};


class Material
{
   private:
      int startLine;  // inclusive of "Material"
      int endLine;    // inclusive of "EndMaterial"
      vector<Temperature *> temperatureList;
      vector<Source *> sourceList;
      keywordPair name;
      bool merged=false;
   public:
      Material (int,int);  // startLine,endLine
      Material () {}
      ~Material ();
      bool load (string *, inputFile *, bool);
      bool get_merged () {return merged;}
      void set_merged (bool a) {merged=a;}
      bool findTemperatureBlocks (inputFile *, bool);
      bool findSourceBlocks (inputFile *);
      bool inTemperatureBlocks (int);
      bool inSourceBlocks (int);
      keywordPair* get_name () {return &name;}
      int get_startLine () {return startLine;}
      Temperature* get_temperature (double, double, string);
      complex<double> get_eps (double, double, double, string);
      double get_mu (double, double, double, string);
      double get_Rs (double, double, double, string);
      void print (string);
      bool check (string);
};

class MaterialDatabase
{
   private:
      inputFile inputs;
      vector<Material *> materialList;
      double tol=1e-12;     // tolerance for floating point matches
      string indent="   ";  // for error messages
      string version_name="#OpenParEMmaterials";
      string version_value="1.0";
      double isTransferred=false;
   public:
      ~MaterialDatabase();
      bool load_materials (char *, char *, char *, char *, bool);
      bool load (const char *, const char *, bool);
      bool merge (MaterialDatabase *, string);
      void push (Material *a) {materialList.push_back(a);}
      void print (string);
      bool findMaterialBlocks ();
      bool check ();
      Material* get (string);
      double get_tol () {return tol;}
      string get_indent () {return indent;}
};


#endif

