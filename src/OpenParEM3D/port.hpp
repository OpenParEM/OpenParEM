////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM3D - A fullwave 3D electromagnetic simulator.                  //
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

#ifndef PORT_H
#define PORT_H

#include "mfem.hpp"
#include "petscsys.h"
#include <petsc.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <limits>
#include <filesystem>
#include <unistd.h>
#include "project.h"
#include "path.hpp"
#include "OpenParEMmaterials.hpp"
#include "mesh.hpp"
#include "sourcefile.hpp"
#include "misc.hpp"
#include "pattern.hpp"

using namespace std;
using namespace mfem;

#define lapack_int int
#define lapack_complex_double std::complex<double>

extern "C" void matrixPrint(lapack_complex_double *, lapack_int);
extern "C" void matrixDiagonalPrint(lapack_complex_double *, lapack_int);
extern "C" void matrixSetValue (lapack_complex_double *, lapack_int, double, double);
extern "C" void matrixScaleValue (lapack_complex_double *, lapack_int, double, double);
extern "C" double matrixGetRealValue (lapack_complex_double *, lapack_int);
extern "C" double matrixGetImagValue (lapack_complex_double *, lapack_int);
extern "C" double matrixGetRealScaleValue (lapack_complex_double *, lapack_int, double, double);
extern "C" double matrixGetImagScaleValue (lapack_complex_double *, lapack_int, double, double);
extern "C" void linearPrint(lapack_complex_double *, lapack_int);
extern "C" void matrixTranspose(lapack_complex_double *, lapack_int);
extern "C" void matrixConjugate (lapack_complex_double *, lapack_int);
extern "C" void matrixScale (lapack_complex_double *, double, double, lapack_int);
extern "C" int matrixInverse(lapack_complex_double *, lapack_int);
extern "C" void matrixMultiply(lapack_complex_double *, lapack_complex_double *, lapack_int);
extern "C" void vectorZero (lapack_complex_double *, lapack_int);
extern "C" void vectorSetValue (lapack_complex_double *, lapack_int, double, double);
extern "C" void matrixVectorMultiply (lapack_complex_double *, lapack_complex_double *, lapack_complex_double *, lapack_int);
extern "C" double vectorGetRealValue (lapack_complex_double *, lapack_int);
extern "C" double vectorGetImagValue (lapack_complex_double *, lapack_int);

class RotatedMesh : public Mesh
{
   public:
      ~RotatedMesh();
      void set_spaceDim (int);
      bool rotate (Path *, bool);
};

struct mpi_complex_int {
   double real;
   double imag;
   int location;
};

class BoundaryDatabase;
class Result;
class fem3D;

double elapsed_time (chrono::system_clock::time_point, chrono::system_clock::time_point);
bool isClose (double, double);

class Gamma
{
   private:
      int Sport;
      int modeNumber2D;
      double alpha;
      double beta;
      double frequency;
   public:
      void set (int, int, double , double, double); 
      bool is_match (int, int);
      bool is_match (int, int, double);
      int get_Sport () {return Sport;}
      int get_modeNumber2D () {return modeNumber2D;}
      double get_alpha () {return alpha;}
      double get_beta () {return beta;}
      double get_frequency () {return frequency;}
      void print ();
};

class GammaDatabase
{
   private:
      vector<Gamma *> gammaList;
   public:
      ~GammaDatabase();
      void push (Gamma *gamma) {gammaList.push_back(gamma);}
      Gamma* getGamma (int, int, double);
      void reset ();
      void print ();
};

// Boundary and Mode share a structure in OpenParEM2D.
// Here they are split to accommodate ports more easily.

class Boundary
{
   private:
      int startLine;
      int endLine;
      keywordPair name;
      keywordPair type;                    // surface_impedance | perfect_electric_conductor (PEC) | perfect_magnetic_conductor (PMC) | radiation
      keywordPair material;                // surface impedance boundary only
      keywordPair wave_impedance;          // radiation only
      vector<keywordPair *> pathNameList;
      vector<long unsigned int> pathIndexList;
      vector<bool> reverseList;
      Vector normal;                       // normal facing outward from the 3D volume
      Path *outline=nullptr;               // outlien of the boundary
      int attribute=-1;                    // attribute assigned to the mesh indicating this boundary
      bool assignedToMesh=false;           // keeps track of whether the boundary was successfully assigned to the mesh
      bool is_default;
      vector<Current *> radiationCurrents; // currents for radiation boundaries
   public:
      Boundary (int,int);
      ~Boundary ();
      bool load (string *, inputFile *);
      bool inBlock (int);
      bool check (string *, vector<Path *>);
      bool assignPathIndices (vector<Path *> *);
      bool checkBoundingBox (Vector *, Vector *, string *, double, vector<Path *> *);
      int get_startLine () {return startLine;}
      int get_endLine () {return endLine;}
      bool is_default_boundary () {return is_default;}
      void set_default_boundary () {is_default=true;}
      string get_name () {return name.get_value();}
      int get_attribute () {return attribute;}
      int get_pathIndex (int i) {return pathIndexList[i];}
      bool name_is_loaded () {return name.is_loaded();}
      int get_name_lineNumber () {return name.get_lineNumber();}
      int get_wave_impedance_lineNumber () {return wave_impedance.get_lineNumber();}
      Vector get_normal () {return normal;}
      void set_normal (double nx, double ny, double nz) {normal(0)=nx; normal(1)=ny; normal(2)=nz;}
      void set_name (string name_) {name.set_value(name_);}
      void set_type (string type_) {type.set_value(type_);}
      void set_material (string material_) {material.set_value(material_);}
      void set_attribute (int attribute_) {attribute=attribute_;};
      void set_assignedToMesh () {assignedToMesh=true;}
      bool is_assignedToMesh () {return assignedToMesh;}
      string get_type () {return type.get_value();}
      string get_material () {return material.get_value();}
      double get_wave_impedance () {return wave_impedance.get_dbl_value();}
      string get_pathName (long unsigned int i) {return pathNameList[i]->get_value();}
      int get_pathName_lineNumber (long unsigned int i) {return pathNameList[i]->get_lineNumber();}
      bool get_reverse (long unsigned int i) {return reverseList[i];}
      long unsigned int get_path_size () {return pathIndexList.size();}
      long unsigned int get_path (long unsigned int i) {return pathIndexList[i];}
      void push (long unsigned int a) {pathIndexList.push_back(a);}
      bool is_surface_impedance ();
      bool is_perfect_electric_conductor();
      bool is_perfect_magnetic_conductor();
      bool is_radiation ();
      bool is_modal ();
      bool is_line ();
      bool has_attribute (int attribute_) {if (attribute == attribute_) return true; return false;}
      bool merge (vector<Path *> *);
      bool is_point_inside (double, double, double);
      bool is_triangleInside (DenseMatrix *);
      bool is_overlapPath (vector<Path *> *, Path *);
      Boundary* get_matchBoundary (double, double, double, double, double, double);
      void addImpedanceIntegrator (double, double, ParMesh *, ParBilinearForm *,
                                   MaterialDatabase *, vector<Array<int> *> &,
                                   vector<ConstantCoefficient *> &, bool);
      bool calculateRadiationCurrents (ParMesh *, struct projectData *, Vector, double, double,
                                        ParGridFunction *, ParGridFunction *, ParGridFunction *, ParGridFunction *);
      void collectRadiationCurrents (vector<Current *> *);
      void deleteRadiationCurrents ();
      void print();
      bool snapToMeshBoundary (vector<Path *> *, Mesh *, string);
};

class OPEMIntegrationPoint
{
   private:
      int pointNumber;
      int rank;
      int initialized;
      DenseMatrix point;
      int elementNumber;
      IntegrationPoint integrationPoint;
      complex<double> fieldX,fieldY,fieldZ;
      Vector pt;  // working space
   public:
      OPEMIntegrationPoint(int, double, double, double);
      void update (ParMesh *);
      void get_location (double *x, double *y, double *z);
      void set (double, double, double, double, double, double);
      void get_fields (complex<double> *, complex<double> *, complex<double> *);
      void get_fieldValue (ParGridFunction *, ParGridFunction *);  // from grids to local value
      void get_field (complex<double> *fieldX_, complex<double> *fieldY_, complex<double> *fieldZ_) {
         *fieldX_=fieldX; *fieldY_=fieldY, *fieldZ_=fieldZ;
      }
      void resetElementNumber ();
      void send (int);
      void print ();
};

class OPEMIntegrationPointList
{
   private:
      vector<OPEMIntegrationPoint *> points;
      bool reverse;
      complex<double> integratedValue;
   public:
      ~OPEMIntegrationPointList ();
      void set_reverse (bool reverse_) {reverse=reverse_;}
      bool get_reverse () {return reverse;}
      void push (OPEMIntegrationPoint *a) {points.push_back(a);}
      long unsigned int get_size() {return points.size();}
      OPEMIntegrationPoint* get_point (long unsigned int i) {return points[i];}
      void update (ParMesh *);
      void get_fieldValues (ParGridFunction *, ParGridFunction *);
      void resetElementNumbers ();
      void assemble ();
      void integrate ();
      complex<double> get_integratedValue () {return integratedValue;}
      void print ();
};

class IntegrationPath
{
   private:
      int startLine;
      int endLine;
      keywordPair type;                                 // voltage or current
      keywordPair scale;                                // default = 1
      vector<keywordPair *> pathNameList;
      vector<long unsigned int> pathIndexList;
      vector<bool> reverseList;
      vector<OPEMIntegrationPointList *> pointsList;
      complex<double> integratedValue;
   public:
      IntegrationPath (int, int);
      ~IntegrationPath ();
      int get_startLine () {return startLine;}
      int get_endLine () {return endLine;}
      string get_type () {return type.get_value();}
      double get_scale () {return scale.get_dbl_value();}
      vector<long unsigned int>* get_pathIndexList () {return &pathIndexList;}
      vector<OPEMIntegrationPointList *>* get_pointsList () {return &pointsList;}
      vector<bool>* get_reverseList () {return &reverseList;}
      bool load(string *, inputFile *);
      bool inIntegrationPathBlock (int);
      bool check (string *, vector<Path *> *);
      bool checkBoundingBox(Vector *, Vector *, string *, double, vector<Path *> *);
      bool align (string *, vector<Path *> *, double *, bool);
      bool assignPathIndices (vector<Path *> *);
      void snapToMeshBoundary (vector<Path *> *, Mesh *);
      void resetElementNumbers ();
      void print(string);
      bool is_enclosedByPath (vector<Path *> *, Path *);
      void calculateLineIntegral (ParMesh *, ParGridFunction *, ParGridFunction *);
      bool is_voltage() {if (type.get_value().compare("voltage") == 0) return true; return false;}
      bool is_current() {if (type.get_value().compare("current") == 0) return true; return false;}
      complex<double> get_integratedValue () {return integratedValue;}
      void set_integratedValue (complex<double> integratedValue_) {integratedValue=integratedValue_;}
      void output (ofstream *, vector<Path *> *, Path *, bool, bool, int);
};

class FieldSet
{
   private:

      // data pulled in from OpenParEM2D
      double *eVecReE=nullptr;
      double *eVecImE=nullptr;
      double *eVecReH=nullptr;
      double *eVecImH=nullptr;
      Vector *eigenVecReEt=nullptr;
      Vector *eigenVecImEt=nullptr;
      Vector *eigenVecReEz=nullptr;
      Vector *eigenVecImEz=nullptr;
      Vector *eigenVecReHt=nullptr;
      Vector *eigenVecImHt=nullptr;
      Vector *eigenVecReHz=nullptr;
      Vector *eigenVecImHz=nullptr;
      double alpha,beta;
      complex<double> impedance,voltage,current,Pz;

      // grid functions to hold the 2D modal fields from the 2D solution
      ParGridFunction *grid2DReEt=nullptr;
      ParGridFunction *grid2DImEt=nullptr;
      ParGridFunction *grid2DReEz=nullptr;
      ParGridFunction *grid2DImEz=nullptr;
      ParGridFunction *grid2DReHt=nullptr;
      ParGridFunction *grid2DImHt=nullptr;
      ParGridFunction *grid2DReHz=nullptr;
      ParGridFunction *grid2DImHz=nullptr;

      // grid functions to hold the 2D modal fields projected onto the 3D space
      // (holds the dof values applied when driving a port)
      ParGridFunction *grid3DReEt=nullptr;
      ParGridFunction *grid3DImEt=nullptr;
      ParGridFunction *grid3DReEz=nullptr;
      ParGridFunction *grid3DImEz=nullptr;
      ParGridFunction *grid3DReHt=nullptr;
      ParGridFunction *grid3DImHt=nullptr;
      ParGridFunction *grid3DReHz=nullptr;
      ParGridFunction *grid3DImHz=nullptr;

      // grid functions to hold the 2D modal solutions projected back onto 2D spaces
      // (to align with the grid functions grid2Dsolution* on the ports)
      ParGridFunction *grid2DmodalReEt=nullptr;
      ParGridFunction *grid2DmodalImEt=nullptr;
      ParGridFunction *grid2DmodalReEz=nullptr;
      ParGridFunction *grid2DmodalImEz=nullptr;
      ParGridFunction *grid2DmodalReHt=nullptr;
      ParGridFunction *grid2DmodalImHt=nullptr;
      ParGridFunction *grid2DmodalReHz=nullptr;
      ParGridFunction *grid2DmodalImHz=nullptr;

   public:
      ~FieldSet();
      bool loadSolution (string *, string, size_t, size_t, int);
      bool scaleSolution ();
      void flip2DmodalSign ();
      void build2Dgrids (ParFiniteElementSpace *, ParFiniteElementSpace *);
      void build3Dgrids (ParFiniteElementSpace *, ParFiniteElementSpace *);
      void build2DModalGrids (ParFiniteElementSpace *, ParFiniteElementSpace *);
      void fillIntegrationPoints (vector<Path *> *, vector<long unsigned int> *, vector<OPEMIntegrationPointList *> *, vector<bool> *);
      void transfer_2Dsolution_2Dgrids_to_3Dgrids ();
      void transfer_2Dsolution_3Dgrids_to_2Dgrids ();
      void save2DParaView (ParSubMesh *, struct projectData *, double, bool, int);
      void save3DParaView (ParMesh *, struct projectData *, double, bool, int);
      void save2DModalParaView (ParSubMesh *, struct projectData *, double, bool, int);
      void populateGamma (double, GammaDatabase *, int, int);
      void reset ();
      double get_alpha () {return alpha;}
      double get_beta () {return beta;}
      complex<double> get_voltage () {return voltage;}
      complex<double> get_current () {return current;}
      complex<double> get_Pz () {return Pz;}
      complex<double> get_impedance () {return impedance;}
      void set_alpha(double alpha_) {alpha=alpha_;}
      void set_beta(double beta_) {beta=beta_;}
      void set_impedance (double ReZ, double ImZ) {impedance=complex<double>(ReZ,ImZ);}
      void set_voltage (double ReV, double ImV) {voltage=complex<double>(ReV,ImV);}
      void set_current (double ReI, double ImI) {current=complex<double>(ReI,ImI);}
      void set_Pz (double RePz, double ImPz) {Pz=complex<double>(RePz,ImPz);}
      ParGridFunction* get_grid3DReEt() {return grid3DReEt;}
      ParGridFunction* get_grid3DImEt() {return grid3DImEt;}
      ParGridFunction* get_grid3DReHt() {return grid3DReHt;}
      ParGridFunction* get_grid3DImHt() {return grid3DImHt;}
      ParGridFunction* get_grid2DmodalReEt() {return grid2DmodalReEt;}
      ParGridFunction* get_grid2DmodalImEt() {return grid2DmodalImEt;}
      ParGridFunction* get_grid2DmodalReEz() {return grid2DmodalReEz;}
      ParGridFunction* get_grid2DmodalImEz() {return grid2DmodalImEz;}
      ParGridFunction* get_grid2DmodalReHt() {return grid2DmodalReHt;}
      ParGridFunction* get_grid2DmodalImHt() {return grid2DmodalImHt;}
      ParGridFunction* get_grid2DmodalReHz() {return grid2DmodalReHz;}
      ParGridFunction* get_grid2DmodalImHz() {return grid2DmodalImHz;}
};

// Mode or Line 
class Mode
{
   private:
      int startLine;
      int endLine;
      keywordPair Sport;                              // integer value for the S-parameter port number
      keywordPair net;                                // net name
      vector<IntegrationPath *> integrationPathList;  // voltage or current or both
      FieldSet fields;
      int modeNumber2D;                               // mode number used for the 2D solution
      string calculation;                             // modal | line - for output formatting
      
      // for S-parameter calculation
      vector<complex<double>> Cp;                     // C plus for direction split with a unique value for each driving set
      vector<complex<double>> Cm;                     // C minus for direction split with a unique value for each driving set
      vector<complex<double>> weight;                 // weight for each driving set
      bool net_is_updated=false;                      // flag to prevent updating net names more than once

   public:
      Mode(int,int,string);
      ~Mode();
      int get_startLine() {return startLine;}
      int get_endLine() {return endLine;}
      string get_net() {return net.get_value();}
      void set_net (string name) {net.set_value(name); net.set_loaded(true);}
      bool net_is_loaded () {return net.is_loaded();}
      int get_Sport() {return Sport.get_int_value();}
      int get_Sport_lineNumber() {return Sport.get_lineNumber();}
      string get_type (long unsigned int i) {return integrationPathList[i]->get_type();}
      double get_alpha() {return fields.get_alpha();}
      double get_beta() {return fields.get_beta();}
      complex<double> get_impedance () {return fields.get_impedance();}
      complex<double> get_voltage () {return fields.get_voltage();}
      complex<double> get_current () {return fields.get_current();}
      complex<double> get_Pz () {return fields.get_Pz();}
      complex<double> get_Cp (int i) {return Cp[i];}
      complex<double> get_Cm (int i) {return Cm[i];}
      complex<double> get_weight (int i) {return weight[i];}
      int get_modeNumber2D() {return modeNumber2D;}
      void set_modeNumber2D (int modeNumber2D_) {modeNumber2D=modeNumber2D_;}
      void set_alpha(double alpha_) {fields.set_alpha(alpha_);}
      void set_beta(double beta_) {fields.set_beta(beta_);}
      void set_impedance (double ReZ, double ImZ) {fields.set_impedance(ReZ,ImZ);}
      bool has_voltage ();
      bool has_current ();
      void set_voltage (double ReV, double ImV) {fields.set_voltage(ReV,ImV);}
      void set_current (double ReI, double ImI) {fields.set_current(ReI,ImI);}
      void set_Pz (double RePz, double ImPz) {fields.set_Pz(RePz,ImPz);}
      bool is_modal() {if (calculation.compare("modal") == 0) return true; return false;}
      bool is_line() {if (calculation.compare("line") == 0) return true; return false;}
      bool inIntegrationPathBlocks (int);
      bool findIntegrationPathBlocks(inputFile *);
      bool load(string *, inputFile *);
      void flip2DmodalSign () {fields.flip2DmodalSign();}
      bool inModeBlock (int);
      bool check(string *, vector<Path *> *, bool, long unsigned int);
      bool align_current_paths (string *, vector<Path *> *, bool);
      bool checkBoundingBox (Vector *, Vector *, string *, double, vector<Path *> *);
      bool assignPathIndices(vector<Path *> *);
      bool is_enclosedByPath (vector<Path *> *, Path *, long unsigned int *);
      void print(string);
      void output (ofstream *, vector<Path *> *, Path *, bool);
      bool loadSolution (string *, string, size_t, size_t);
      bool scaleSolution ();
      void printSolution ();
      void build2Dgrids (ParFiniteElementSpace *, ParFiniteElementSpace *);
      void build3Dgrids (ParFiniteElementSpace *, ParFiniteElementSpace *);
      void build2DModalGrids (ParFiniteElementSpace *, ParFiniteElementSpace *);
      void fillX (Vec *, Vec *, Array<int> *, HYPRE_BigInt *, int);
      void fillIntegrationPoints (vector<Path *> *);
      IntegrationPath* get_voltageIntegrationPath ();
      IntegrationPath* get_currentIntegrationPath ();
      void calculateLineIntegrals (ParMesh *, fem3D *);
      void calculateLineIntegrals (ParMesh *, fem3D *, IntegrationPath *, IntegrationPath *);
      void alignDirections (ParMesh *, fem3D *, IntegrationPath *, IntegrationPath *);
      void addWeight (complex<double> value) {weight.push_back(value);}
      void setWeight (int drivingSet, complex<double> value) {weight[drivingSet]=value;}
      complex<double> getWeight (long unsigned int i) {return weight[i];}
      void calculateSplits (ParFiniteElementSpace *, ParGridFunction *, ParGridFunction *, ParGridFunction *,
                            ParGridFunction *, ParGridFunction *, ParGridFunction *, ParGridFunction *, ParGridFunction *, 
                            Vector);
      complex<double> calculatePowerIn (int);
      complex<double> calculatePowerOut (int);
      void set_net_is_updated () {net_is_updated=true;}
      bool get_net_is_updated () {return net_is_updated;}
      void transfer_2Dsolution_2Dgrids_to_3Dgrids ();
      void transfer_2Dsolution_3Dgrids_to_2Dgrids ();
      void save2DParaView (ParSubMesh *, struct projectData *, double, bool);
      void save3DParaView (ParMesh *, struct projectData *, double, bool);
      void save2DModalParaView (ParSubMesh *, struct projectData *, double, bool);
      void resetElementNumbers ();
      void snapToMeshBoundary (vector<Path *> *, Mesh *);
      void populateGamma (double, GammaDatabase *);
      void reset ();
};

class PortAttribute
{
   private:
      int attribute;                     // assigned attribute
      int adjacent_element_attribute;    // attribute of the element adjacent to the boundary element - for assigning materials at the port
   public:
      PortAttribute (int attribute_, int adjacent_element_attribute_) {
         attribute=attribute_;
         adjacent_element_attribute=adjacent_element_attribute_;
      }
      int get_attribute () {return attribute;}
      int get_adjacent_element_attribute () {return adjacent_element_attribute;}
      void set_adjacent_element_attribute (int adjacent_element_attribute_) {adjacent_element_attribute=adjacent_element_attribute_;}
      bool has_attribute (int);
      void print (string);
};

class DifferentialPair
{
   private:
      int startLine;
      int endLine;
      keywordPair Sport_P;
      keywordPair Sport_N;
   public:
      DifferentialPair (int, int);
      bool is_loaded ();
      int get_startLine() {return startLine;}
      int get_endLine() {return endLine;}
      int get_Sport_P () {return Sport_P.get_int_value();}
      int get_Sport_N () {return Sport_N.get_int_value();}
      bool inDifferentialPairBlock (int);
      bool check(string *);
      bool load (string *, inputFile *);
      void print (string);
};

class Port
{
   private:
      int startLine;
      int endLine;
      keywordPair name;                            // alphanumeric name 
      vector<keywordPair *> pathNameList;
      vector<long unsigned int> pathIndexList;     // outline of the port
      vector<bool> reverseList;
      keywordPair impedance_definition;            // VI, PV, or PI
      keywordPair impedance_calculation;           // modal | line
      vector<Mode *> modeList;
      vector<DifferentialPair *> differentialPairList;
      vector<PortAttribute *> attributeList;
      bool assignedToMesh=false;                   // keeps track of whether the port was successfully assigned to the mesh
//      bool appliedPortABCreal=false;               // keeps track of whether the port absorbing boundary condition has been applied to the real part
//      bool appliedPortABCimag=false;               // keeps track of whether the port absorbing boundary condition has been applied to the imag part

      ND_FECollection *fec2D_ND=nullptr;           // Et
      ParFiniteElementSpace *fes2D_ND=nullptr;     // on 2D ParMesh

      H1_FECollection *fec2D_H1=nullptr;           // Ez
      ParFiniteElementSpace *fes2D_H1=nullptr;     // on 2D ParMesh

      L2_FECollection *fec2D_L2=nullptr;           // for S-parameter calculations using modal projections
      ParFiniteElementSpace *fes2D_L2=nullptr;     // on 2D ParMesh

      bool spin180degrees;
      size_t t_size, z_size;
      string meshFilename="";
      string modesFilename="";
      Path *outline=nullptr;                       // port outline for 3D operations
      Path *rotated_outline=nullptr;               // port outline rotated to x-y plane for 2D operations
      Vector normal;                               // normal facing outward from the 3D volume
      Vector rotated_normal;                       // outward facing normal after rotation for the rotated 2D mesh
      lapack_complex_double* Ti;                   // for conversion between modal and line currents
      lapack_complex_double* Tv;                   // for conversion between modal and line voltages
      int TiTvSize;

      Array<int> *ess_tdof_list=nullptr;           // on 3D mesh and space
      HYPRE_BigInt *offset;

      // grid functions to hold the 3D solutions on the 2D ports
      ParGridFunction *grid2DsolutionReEt=nullptr;
      ParGridFunction *grid2DsolutionImEt=nullptr;
      ParGridFunction *grid2DsolutionReEz=nullptr;
      ParGridFunction *grid2DsolutionImEz=nullptr;
      ParGridFunction *grid2DsolutionReHt=nullptr;
      ParGridFunction *grid2DsolutionImHt=nullptr;
      ParGridFunction *grid2DsolutionReHz=nullptr;
      ParGridFunction *grid2DsolutionImHz=nullptr;

   public:
      Port(int,int);
      ~Port();
      int get_startLine () {return startLine;}
      int get_endLine () {return endLine;}
      string get_name () {return name.get_value();}
      int get_name_lineNumber () {return name.get_lineNumber();}
      long unsigned int get_modeCount ();
      int get_SportCount ();
      int get_minSportCount ();
      int get_maxSportCount ();
      int get_attribute (int);
      int get_last_attribute (int);
      int get_adjacent_element_attribute (int);
      Path* get_outline () {return outline;}
      Path* get_rotated_outline () {return rotated_outline;}
      int get_pathIndex (int i) {return pathIndexList[i];}
      void push_portAttribute (PortAttribute *portAttribute) {attributeList.push_back(portAttribute);};
      void set_assignedToMesh () {assignedToMesh=true;}
      bool is_assignedToMesh () {return assignedToMesh;}
      bool is_modal () {if (impedance_calculation.get_value().compare("modal") == 0) return true; return false;}
      bool is_line () {if (impedance_calculation.get_value().compare("line") == 0) return true; return false;}
      bool is_mixed_mode () {if (differentialPairList.size() == 0) return false; return true;}
      bool has_attribute (int);
      bool load (string *, inputFile *);
      bool inBlock (int);
      bool inModeBlocks (int);
      bool findModeBlocks (inputFile *);
      bool inDifferentialPairBlocks (int);
      bool findLineBlocks (inputFile *);
      bool findDifferentialPairBlocks (inputFile *);
      bool check (string *, vector<Path *> *, bool);
      bool assignPathIndices (vector<Path *> *);
      bool checkBoundingBox(Vector *, Vector *, string *, double, vector<Path *> *);
      vector<int> get_SportList ();
      void push(long unsigned int a) {pathIndexList.push_back(a);}
      bool merge(vector<Path *> *);
      bool is_point_inside (double, double, double);
      bool is_triangleInside (DenseMatrix *);
      bool is_modePathInside (string *, vector<Path *> *);
      bool createRotated(vector<Path *> *, string);
      bool create2Dmesh (int, ParMesh *, vector<ParSubMesh> *, long unsigned int, double);
      void saveMesh (MeshMaterialList *, string *, ParSubMesh *);
      bool postProcessMesh (string, string);
      void save2Dsetup(struct projectData *, string *, double, Gamma *);
      void saveModeFile (struct projectData *, vector<Path *> *, BoundaryDatabase *);
      void set_filenames();
      bool is_overlapPath (Path *);
      Vector& get_normal() {return normal;}
      Vector get_rotated_normal() {return rotated_normal;}
      bool createDirectory(string *);
      void set2DModeNumbers();
      int get_last_attribute ();
      void print();
      void printSolution (string);
      void printPaths(vector<Path *> *);
      bool solve(string *);
      bool loadSolution(string *,double);
      bool loadSizes_tz (string *);
      bool loadTiTv (string *);
      bool has_Ti () {if (Ti) return true; else return false;}
      bool has_Tv () {if (Tv) return true; else return false;}
      double get_ReTi (int row, int col) {return matrixGetRealValue(Ti,row+TiTvSize*col);}
      double get_ImTi (int row, int col) {return matrixGetImagValue(Ti,row+TiTvSize*col);}
      double get_ReTv (int row, int col) {return matrixGetRealValue(Tv,row+TiTvSize*col);}
      double get_ImTv (int row, int col) {return matrixGetImagValue(Tv,row+TiTvSize*col);}
      int get_TiTvSize () {return TiTvSize;}
      void print_Ti () {matrixPrint(Ti,TiTvSize);}
      void print_Tv () {matrixPrint(Tv,TiTvSize);}
      bool uses_current ();
      bool uses_voltage ();
      bool load_modeMetrics (string *, double);
      void build2Dgrids ();
      void build3Dgrids (ParFiniteElementSpace *, ParFiniteElementSpace *);
      void build2DSolutionGrids ();
      void build2DModalGrids ();
      void build_essTdofList (ParFiniteElementSpace *, ParMesh *);
      bool addPortIntegrators (ParMesh *, ParBilinearForm *, PWConstCoefficient *, PWConstCoefficient *, PWConstCoefficient *, vector<Array<int> *> &,
                               vector<ConstantCoefficient *> &, vector<ConstantCoefficient *> &, vector<ConstantCoefficient *> &, vector<ConstantCoefficient *> &,
                               bool, int, bool, string);
      void fillX (Vec *, Vec *, int);
      void extract2Dmesh (ParMesh *, vector<ParSubMesh> *);
      void addWeight (complex<double>);
      void calculateSplits ();
      bool isDriving (int);
      Mode* getDrivingMode (int);
      void fillIntegrationPoints (vector<Path *> *);
      void calculateLineIntegrals (ParMesh *, fem3D *);
      void alignDirections (ParMesh *, fem3D *);
      void transfer_2Dsolution_2Dgrids_to_3Dgrids ();
      void transfer_2Dsolution_3Dgrids_to_2Dgrids ();
      void transfer_3Dsolution_3Dgrids_to_2Dgrids (fem3D *);
      void save2DParaView (ParSubMesh *, struct projectData *, double, bool);
      void save3DParaView (ParMesh *, struct projectData *, double, bool);
      void save2DSolutionParaView (ParSubMesh *, struct projectData *, double, int, bool);
      void save2DModalParaView (ParSubMesh *, struct projectData *, double, bool);
      void resetElementNumbers ();
      bool snapToMeshBoundary (vector<Path *> *, Mesh *, string);
      void populateGamma (double, GammaDatabase *);
      void reset ();
      void aggregateDifferentialPairList (vector<DifferentialPair *> *);
      void buildAggregateModeList (vector<Mode *> *);
      bool has_mode (Mode *, long unsigned int *);
};

// boundary conditions and ports
class BoundaryDatabase
{
   private:
      inputFile inputs;
      vector<SourceFile *> sourceFileList;
      vector<Path *> pathList;
      vector<Boundary *> boundaryList;
      vector<Port *> portList;
      double tol=1e-14;
      string tempDirectory="";
      string indent="   ";
      string version_name="#OpenParEMports";
      string version_value="1.0";
      string drivingSetName="";
      vector<Current *> radiationCurrents;     //  Aggregated list from Boundary and copied across ranks
   public:
      ~BoundaryDatabase();
      void set_tempDirectory(string tempDirectory_) {tempDirectory=tempDirectory_;}
      string get_tempDirectory() {return tempDirectory;}
      void set_drivingSetName (string drivingSetName_) {drivingSetName=drivingSetName_;}
      string get_drivingSetName () {return drivingSetName;}
      void assignAttributes (Mesh *);
      Boundary* get_defaultBoundary ();
      long unsigned int get_boundaryListSize() {return boundaryList.size();}
      Boundary* get_boundary (long unsigned int i) {return boundaryList[i];}
      bool markMeshBoundaries (Mesh *mesh);
      bool createDefaultBoundary (struct projectData *, Mesh *, MaterialDatabase *, BoundaryDatabase *);
      bool inBlocks (int);
      bool findSourceFileBlocks ();
      bool findPathBlocks ();
      bool findBoundaryBlocks ();
      bool findPortBlocks ();
      bool load (const char *, bool);
      bool check (bool);
      bool checkSportNumbering ();
      bool check_scale (Mesh *, int);
      bool check_overlaps ();
      bool alignRadiationNormals ();
      void subdivide_paths ();
      void print ();
      bool is_line ();
      bool is_modal ();
      bool is_mixed_mode ();
      bool create2Dmeshes (int, ParMesh *, vector<ParSubMesh> *);
      vector<Path *> get_pathList () {return pathList;}
      Path* get_path (long unsigned int i) {return pathList[i];}
      int getLastAttribute ();
      void savePortMeshes (MeshMaterialList *, vector<ParSubMesh> *);
      void save2Dsetups (struct projectData *, double, GammaDatabase *);
      void saveModeFiles (struct projectData *);
      Boundary* get_matchBoundary (double, double, double, double, double, double);
      bool createPortDirectories ();
      bool solvePorts (int, ParMesh *, vector<ParSubMesh> *, double, MeshMaterialList *, struct projectData *, GammaDatabase *);
      bool loadPortSolutions (double);
      void populateGamma (double, GammaDatabase *);
      void printPortSolutions ();
      int get_totalModeCount ();
      int get_SportCount ();
      int get_maxDofCount ();
      void set2DModeNumbers ();
      void build_portEssTdofLists (ParFiniteElementSpace *, ParMesh *);
      bool addPortIntegrators (ParMesh *, ParBilinearForm *, PWConstCoefficient *, PWConstCoefficient *, PWConstCoefficient *, vector<Array<int> *> &,
                               vector<ConstantCoefficient *> &, vector<ConstantCoefficient *> &, vector<ConstantCoefficient *> &, vector<ConstantCoefficient *> &,
                               bool, int, bool, string); 
      void addImpedanceIntegrators (double, double, ParMesh *, ParBilinearForm *, MaterialDatabase *,
                                    vector<Array<int> *> &, vector<ConstantCoefficient *> &, bool);
      void fillX (Vec *, Vec *, int);
      void showPortDofCounts ();
      void extract2Dmesh (ParMesh *, vector<ParSubMesh> *);
      void createDrivingSets ();
      void calculateSplits ();
      Mode* getDrivingMode (int);
      void fillIntegrationPoints ();
      void calculateLineIntegrals (ParMesh *, fem3D *);
      void alignDirections (ParMesh *, fem3D *);
      bool calculateAcceptedPower (int, complex<double> *);
      PetscErrorCode calculateS (Result *);
      void build2Dgrids ();
      void build2DModalGrids ();
      void build3Dgrids (ParFiniteElementSpace *, ParFiniteElementSpace *);
      void build2DSolutionGrids ();
      void buildGrids (fem3D *);
      void transfer_2Dsolution_2Dgrids_to_3Dgrids ();
      void transfer_2Dsolution_3Dgrids_to_2Dgrids ();
      void transfer_3Dsolution_3Dgrids_to_2Dgrids (fem3D *);
      void save2DParaView (vector<ParSubMesh> *, struct projectData *, double, bool);
      void save3DParaView (ParMesh *, struct projectData *, double, bool);
      void save2DSolutionParaView (vector<ParSubMesh> *, struct projectData *, double, int, bool);
      void save2DModalParaView (vector<ParSubMesh> *, struct projectData *, double, bool);
      bool solve2Dports (ParMesh *, vector<ParSubMesh> *, struct projectData *, double, MeshMaterialList *, GammaDatabase *);
      void resetElementNumbers ();
      bool snapToMeshBoundary (Mesh *);
      PetscErrorCode build_M (Mat *, vector<DifferentialPair *> *, BoundaryDatabase *);
      void aggregateDifferentialPairList (vector<DifferentialPair *> *);
      bool buildAggregateModeList (vector<Mode *> *);
      long unsigned int get_portList_size () {return portList.size();}
      Port* get_port (long unsigned int i) {return portList[i];}
      bool get_port_from_mode (Mode *, Port **, long unsigned int *);
      void reset();
      bool has_Ti ();
      bool has_Tv ();
      Port* get_port (Mode *);
      bool hasRadiationBoundary ();
      bool calculateRadiationCurrents (ParMesh *, struct projectData *, Vector, double, double,
                                       ParGridFunction *, ParGridFunction *, ParGridFunction *, ParGridFunction *);
      void collectRadiationCurrents ();
      void deleteRadiationCurrents ();
      void calculateFarField (double, Vector, double, double, vector<OPEMpoint *> *, long unsigned int, long unsigned int);
      void calculateFarField (double, Vector, double, double, vector<OPEMpoint *> *);

};

#endif
