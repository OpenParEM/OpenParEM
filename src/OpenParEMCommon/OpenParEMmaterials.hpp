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
#include <quadmath.h>

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
      bool load (std::string *, inputFile *);
      bool inFrequencyBlock (int);
      keywordPair* get_frequency () {return &frequency;}
      keywordPair* get_relative_permittivity () {return &relative_permittivity;}
      keywordPair* get_relative_permeability () {return &relative_permeability;}
      keywordPair* get_loss () {return &loss;}
      keywordPair* get_Rz () {return &Rz;}
      void set_freespace ();
      void set_copper ();
      int get_startLine () {return startLine;}
      void print (std::string);
      bool check (std::string);
};


class Temperature
{
   private:
      int startLine;                     // inclusive of "Temperature"
      int endLine;                       // inclusive of "EndTemperature"
      std::vector<Frequency *> frequencyList;
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
      std::complex<double> get_eps (double, double, std::string);
      double get_mu (double, double, std::string);
      double get_Rs (double, double, std::string);
      bool load (std::string *, inputFile *, bool);
      void print (std::string);
      bool check (std::string);
      long unsigned int get_frequencyList_size () {return frequencyList.size();}
      Frequency* get_frequency (long unsigned int i) {return frequencyList[i];}
      keywordPair get_er_infinity() {return er_infinity;}
      keywordPair get_delta_er() {return delta_er;}
      keywordPair get_m1() {return m1;}
      keywordPair get_m2() {return m2;}
      keywordPair get_relative_permeability() {return relative_permeability;}
      keywordPair get_loss() {return loss;}
      void set_freespace ();
      void set_FR4 ();
      void set_copper ();
};

class Source
{
   private:
      int startLine;  // inclusive of "Source"
      int endLine;    // inclusive of "EndSource"
      std::vector<int> lineNumberList;
      std::vector<std::string> lineList;
   public:
      Source (int,int);  // startLine,endLine
      bool inSourceBlock (int);
      bool load (inputFile *);
      void print (std::string);
      std::vector<std::string> get_lineList () {return lineList;}
      void set_freespace ();
      void set_FR4 ();
      void set_copper ();
};


class Material
{
   private:
      int startLine;  // inclusive of "Material"
      int endLine;    // inclusive of "EndMaterial"
      std::vector<Temperature *> temperatureList;
      std::vector<Source *> sourceList;
      keywordPair name;
      bool merged=false;
   public:
      Material (int,int);  // startLine,endLine
      Material () {}
      ~Material ();
      bool load (std::string *, inputFile *, bool);
      bool get_merged () {return merged;}
      void set_merged (bool a) {merged=a;}
      bool findTemperatureBlocks (inputFile *, bool);
      bool findSourceBlocks (inputFile *);
      bool inTemperatureBlocks (int);
      bool inSourceBlocks (int);
      keywordPair* get_name () {return &name;}
      void set_name (std::string name_) {name.set_keyword(name_); name.set_value(name_);}
      int get_startLine () {return startLine;}
      Temperature* get_temperature (double, double, std::string);
      std::complex<double> get_eps (double, double, double, std::string);
      double get_mu (double, double, double, std::string);
      double get_Rs (double, double, double, std::string);
      void print (std::string);
      bool check (std::string);
      long unsigned int get_sourceList_size () {return sourceList.size();}
      std::vector<std::string> get_source_lineList (long unsigned int i) {return sourceList[i]->get_lineList();}
      long unsigned int get_temperatureList_size() {return temperatureList.size();}
      Temperature* get_temperature (long unsigned int i) {return temperatureList[i];}
      void set_freespace ();
      void set_FR4 ();
      void set_copper ();
};

class MaterialDatabase
{
   private:
      inputFile inputs;
      std::vector<Material *> materialList;
      double tol=1e-12;     // tolerance for floating point matches
      std::string indent="   ";  // for error messages
      std::string version_name="#OpenParEMmaterials";
      std::string version_value="1.0";
      double isTransferred=false;
   public:
      ~MaterialDatabase();
      bool load_materials (char *, char *, char *, char *, bool);
      bool load (const char *, const char *, bool);
      bool merge (MaterialDatabase *, std::string);
      void clear ();
      void push (Material *a) {materialList.push_back(a);}
      void insert (long unsigned int location, Material *a) {materialList.insert(materialList.begin()+location,a);}
      void print (std::string);
      bool findMaterialBlocks ();
      bool check ();
      Material* get (std::string);
      double get_tol () {return tol;}
      std::string get_indent () {return indent;}
      std::string get_version_name () {return version_name;}
      std::string get_version_value () {return version_value;}
      long unsigned int get_size () {return materialList.size();}
      Material* get_material (long unsigned int i) {return materialList[i];}
};


#endif

