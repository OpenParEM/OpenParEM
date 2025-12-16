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

#include <quadmath.h>
#include <petsc.h>
#include <fstream>
#include <string>
#include <unistd.h>
#include <atomic>
#include "mfem.hpp"
#include "project.h"
#include "path.hpp"
#include "OpenParEMmaterials.hpp"
#include "mesh.hpp"
#include "sourcefile.hpp"
#include "misc.hpp"
#include "pattern.hpp"

#ifdef HAS_GUI
//#include <QObject>
#include <QTreeWidget>
#include "CustomOpenGLWidget.h"
#include "CustomTreeWidgetItem.h"
#endif

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

class RotatedMesh : public mfem::Mesh
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

double elapsed_time (std::chrono::steady_clock::time_point, std::chrono::steady_clock::time_point);
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
      std::vector<Gamma *> gammaList;
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
      keywordPair type;                              // surface_impedance | perfect_electric_conductor (PEC) | perfect_magnetic_conductor (PMC) | radiation
      keywordPair material;                          // surface impedance boundary only
      keywordPair wave_impedance;                    // radiation only
      std::vector<keywordPair *> pathNameList;
      std::vector<long unsigned int> pathIndexList;
      std::vector<bool> reverseList;
      mfem::Vector normal;                           // normal facing outward from the 3D volume
      Path *outline=nullptr;                         // outline of the boundary
      int attribute=-1;                              // attribute assigned to the mesh indicating this boundary
      bool assignedToMesh=false;                     // keeps track of whether the boundary was successfully assigned to the mesh
      bool is_default;
      std::vector<Current *> radiationCurrents; // currents for radiation boundaries

#if HAS_GUI
    QDoubleValidator doubleValidator;
    std::unordered_map<Handle(AIS_Shape), CustomTreeWidgetItem*> *drawingToItemMap=nullptr;
#endif

   public:
      Boundary (int,int);
      ~Boundary ();
      bool load (std::string *, inputFile *);
      bool inBlock (int);
      bool check (std::string *, std::vector<Path *>);
      bool assignPathIndices (std::vector<Path *> *);
      bool checkBoundingBox (mfem::Vector *, mfem::Vector *, std::string *, double, std::vector<Path *> *);
      int get_startLine () {return startLine;}
      int get_endLine () {return endLine;}
      bool is_default_boundary () {return is_default;}
      void set_default_boundary () {is_default=true;}
      std::string get_name () {return name.get_value();}
      int get_attribute () {return attribute;}
      int get_pathIndex (int i) {return pathIndexList[i];}
      bool name_is_loaded () {return name.is_loaded();}
      int get_name_lineNumber () {return name.get_lineNumber();}
      int get_wave_impedance_lineNumber () {return wave_impedance.get_lineNumber();}
      mfem::Vector get_normal () {return normal;}
      void set_normal (double nx, double ny, double nz) {normal(0)=nx; normal(1)=ny; normal(2)=nz;}
      void set_name (std::string name_) {name.set_value(name_);}
      void set_type (std::string type_) {type.set_value(type_);}
      void set_material (std::string material_) {material.set_value(material_);}
      void set_attribute (int attribute_) {attribute=attribute_;};
      void set_assignedToMesh () {assignedToMesh=true;}
      bool is_assignedToMesh () {return assignedToMesh;}
      std::string get_type () {return type.get_value();}
      std::string get_material () {return material.get_value();}
      double get_wave_impedance () {return wave_impedance.get_dbl_value();}
      std::string get_pathName (long unsigned int i) {return pathNameList[i]->get_value();}
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
      bool merge (std::vector<Path *> *);
      bool is_point_inside (double, double, double);
      bool is_triangleInside (mfem::DenseMatrix *);
      bool is_overlapPath (std::vector<Path *> *, Path *);
      Boundary* get_matchBoundary (double, double, double, double, double, double);
      void addImpedanceIntegrator (double, double, mfem::ParMesh *, mfem::ParBilinearForm *,
                                   MaterialDatabase *, std::vector<mfem::Array<int> *> &,
                                   std::vector<mfem::ConstantCoefficient *> &, bool);
      bool calculateRadiationCurrents (mfem::ParMesh *, struct projectData *, mfem::Vector, double, double,
                                       mfem::ParGridFunction *, mfem::ParGridFunction *, mfem::ParGridFunction *, mfem::ParGridFunction *);
      void collectRadiationCurrents (std::vector<Current *> *);
      void deleteRadiationCurrents ();
      void print();
      void save (std::ofstream *);
      bool snapToMeshBoundary (std::vector<Path *> *, mfem::Mesh *, std::string);
#ifdef HAS_GUI
      void draw (struct projectData *, std::vector<Path *> *, CustomOpenGLWidget *, QTreeWidget *, CustomTreeWidgetItem *, MaterialDatabase *);
      void set_drawingToItemMap (std::unordered_map<Handle(AIS_Shape), CustomTreeWidgetItem*> *drawingToItemMap_) {drawingToItemMap=drawingToItemMap_;}
#endif
};

class OPEMIntegrationPoint
{
   private:
      int pointNumber;
      int rank;
      int initialized;
      mfem::DenseMatrix point;
      int elementNumber;
      mfem::IntegrationPoint integrationPoint;
      std::complex<double> fieldX,fieldY,fieldZ;
      mfem::Vector pt;  // working space
   public:
      OPEMIntegrationPoint(int, double, double, double);
      void update (mfem::ParMesh *);
      void get_location (double *x, double *y, double *z);
      void set (double, double, double, double, double, double);
      void get_fields (std::complex<double> *, std::complex<double> *, std::complex<double> *);
      void get_fieldValue (mfem::ParGridFunction *, mfem::ParGridFunction *);  // from grids to local value
      void get_field (std::complex<double> *fieldX_, std::complex<double> *fieldY_, std::complex<double> *fieldZ_) {
         *fieldX_=fieldX; *fieldY_=fieldY, *fieldZ_=fieldZ;
      }
      void resetElementNumber ();
      void send (int);
      void print ();
};

class OPEMIntegrationPointList
{
   private:
      std::vector<OPEMIntegrationPoint *> points;
      bool reverse;
      std::complex<double> integratedValue;
   public:
      ~OPEMIntegrationPointList ();
      void set_reverse (bool reverse_) {reverse=reverse_;}
      bool get_reverse () {return reverse;}
      void push (OPEMIntegrationPoint *a) {points.push_back(a);}
      long unsigned int get_size() {return points.size();}
      OPEMIntegrationPoint* get_point (long unsigned int i) {return points[i];}
      void update (mfem::ParMesh *);
      void get_fieldValues (mfem::ParGridFunction *, mfem::ParGridFunction *);
      void resetElementNumbers ();
      void assemble ();
      void integrate ();
      std::complex<double> get_integratedValue () {return integratedValue;}
      void print ();
};

class IntegrationPath
{
   private:
      int startLine;
      int endLine;
      keywordPair type;                                 // voltage or current
      keywordPair scale;                                // default = 1
      std::vector<keywordPair *> pathNameList;
      std::vector<long unsigned int> pathIndexList;
      std::vector<bool> reverseList;
      std::vector<OPEMIntegrationPointList *> pointsList;
      std::complex<double> integratedValue;

#if HAS_GUI
      QDoubleValidator doubleValidator;
#endif
   public:
      IntegrationPath (int, int);
      ~IntegrationPath ();
      int get_startLine () {return startLine;}
      int get_endLine () {return endLine;}
      std::string get_type () {return type.get_value();}
      double get_scale () {return scale.get_dbl_value();}
      std::vector<long unsigned int>* get_pathIndexList () {return &pathIndexList;}
      std::vector<OPEMIntegrationPointList *>* get_pointsList () {return &pointsList;}
      std::vector<bool>* get_reverseList () {return &reverseList;}
      bool load(std::string *, inputFile *);
      bool inIntegrationPathBlock (int);
      bool check (std::string *, std::vector<Path *> *);
      bool checkBoundingBox(mfem::Vector *, mfem::Vector *, std::string *, double, std::vector<Path *> *);
      bool align (std::string *, std::vector<Path *> *, double *, bool);
      bool assignPathIndices (std::vector<Path *> *);
      void snapToMeshBoundary (std::vector<Path *> *, mfem::Mesh *);
      void resetElementNumbers ();
      void print (std::string);
      void save (std::ofstream *);
      bool is_enclosedByPath (std::vector<Path *> *, Path *);
      void calculateLineIntegral (mfem::ParMesh *, mfem::ParGridFunction *, mfem::ParGridFunction *);
      bool is_voltage() {if (type.get_value().compare("voltage") == 0) return true; return false;}
      bool is_current() {if (type.get_value().compare("current") == 0) return true; return false;}
      std::complex<double> get_integratedValue () {return integratedValue;}
      void set_integratedValue (std::complex<double> integratedValue_) {integratedValue=integratedValue_;}
      void output (std::ofstream *, std::vector<Path *> *, Path *, bool, bool, int);
#ifdef HAS_GUI
      void draw (std::vector<Path *> *, struct point *, CustomOpenGLWidget *, QTreeWidget *, CustomTreeWidgetItem *);
#endif
};

class FieldSet
{
   private:

      // data pulled in from OpenParEM2D
      double *eVecReE=nullptr;
      double *eVecImE=nullptr;
      double *eVecReH=nullptr;
      double *eVecImH=nullptr;
      mfem::Vector *eigenVecReEt=nullptr;
      mfem::Vector *eigenVecImEt=nullptr;
      mfem::Vector *eigenVecReEz=nullptr;
      mfem::Vector *eigenVecImEz=nullptr;
      mfem::Vector *eigenVecReHt=nullptr;
      mfem::Vector *eigenVecImHt=nullptr;
      mfem::Vector *eigenVecReHz=nullptr;
      mfem::Vector *eigenVecImHz=nullptr;
      double alpha,beta;
      std::complex<double> impedance,voltage,current,Pz;

      // grid functions to hold the 2D modal fields from the 2D solution
      mfem::ParGridFunction *grid2DReEt=nullptr;
      mfem::ParGridFunction *grid2DImEt=nullptr;
      mfem::ParGridFunction *grid2DReEz=nullptr;
      mfem::ParGridFunction *grid2DImEz=nullptr;
      mfem::ParGridFunction *grid2DReHt=nullptr;
      mfem::ParGridFunction *grid2DImHt=nullptr;
      mfem::ParGridFunction *grid2DReHz=nullptr;
      mfem::ParGridFunction *grid2DImHz=nullptr;

      // grid functions to hold the 2D modal fields projected onto the 3D space
      // (holds the dof values applied when driving a port)
      mfem::ParGridFunction *grid3DReEt=nullptr;
      mfem::ParGridFunction *grid3DImEt=nullptr;
      mfem::ParGridFunction *grid3DReEz=nullptr;
      mfem::ParGridFunction *grid3DImEz=nullptr;
      mfem::ParGridFunction *grid3DReHt=nullptr;
      mfem::ParGridFunction *grid3DImHt=nullptr;
      mfem::ParGridFunction *grid3DReHz=nullptr;
      mfem::ParGridFunction *grid3DImHz=nullptr;

      // grid functions to hold the 2D modal solutions projected back onto 2D spaces
      // (to align with the grid functions grid2Dsolution* on the ports)
      mfem::ParGridFunction *grid2DmodalReEt=nullptr;
      mfem::ParGridFunction *grid2DmodalImEt=nullptr;
      mfem::ParGridFunction *grid2DmodalReEz=nullptr;
      mfem::ParGridFunction *grid2DmodalImEz=nullptr;
      mfem::ParGridFunction *grid2DmodalReHt=nullptr;
      mfem::ParGridFunction *grid2DmodalImHt=nullptr;
      mfem::ParGridFunction *grid2DmodalReHz=nullptr;
      mfem::ParGridFunction *grid2DmodalImHz=nullptr;

   public:
      ~FieldSet();
      bool loadSolution (std::string *, std::string, size_t, size_t, int);
      bool scaleSolution ();
      void flip2DmodalSign ();
      void build2Dgrids (mfem::ParFiniteElementSpace *, mfem::ParFiniteElementSpace *);
      void build3Dgrids (mfem::ParFiniteElementSpace *, mfem::ParFiniteElementSpace *);
      void build2DModalGrids (mfem::ParFiniteElementSpace *, mfem::ParFiniteElementSpace *);
      void fillIntegrationPoints (std::vector<Path *> *, std::vector<long unsigned int> *, std::vector<OPEMIntegrationPointList *> *, std::vector<bool> *);
      void transfer_2Dsolution_2Dgrids_to_3Dgrids ();
      void transfer_2Dsolution_3Dgrids_to_2Dgrids ();
      void save2DParaView (mfem::ParSubMesh *, struct projectData *, double, bool, int);
      void save3DParaView (mfem::ParMesh *, struct projectData *, double, bool, int);
      void save2DModalParaView (mfem::ParSubMesh *, struct projectData *, double, bool, int);
      void populateGamma (double, GammaDatabase *, int, int);
      void reset ();
      double get_alpha () {return alpha;}
      double get_beta () {return beta;}
      std::complex<double> get_voltage () {return voltage;}
      std::complex<double> get_current () {return current;}
      std::complex<double> get_Pz () {return Pz;}
      std::complex<double> get_impedance () {return impedance;}
      void set_alpha(double alpha_) {alpha=alpha_;}
      void set_beta(double beta_) {beta=beta_;}
      void set_impedance (double ReZ, double ImZ) {impedance=std::complex<double>(ReZ,ImZ);}
      void set_voltage (double ReV, double ImV) {voltage=std::complex<double>(ReV,ImV);}
      void set_current (double ReI, double ImI) {current=std::complex<double>(ReI,ImI);}
      void set_Pz (double RePz, double ImPz) {Pz=std::complex<double>(RePz,ImPz);}
      mfem::ParGridFunction* get_grid3DReEt() {return grid3DReEt;}
      mfem::ParGridFunction* get_grid3DImEt() {return grid3DImEt;}
      mfem::ParGridFunction* get_grid3DReHt() {return grid3DReHt;}
      mfem::ParGridFunction* get_grid3DImHt() {return grid3DImHt;}
      mfem::ParGridFunction* get_grid2DmodalReEt() {return grid2DmodalReEt;}
      mfem::ParGridFunction* get_grid2DmodalImEt() {return grid2DmodalImEt;}
      mfem::ParGridFunction* get_grid2DmodalReEz() {return grid2DmodalReEz;}
      mfem::ParGridFunction* get_grid2DmodalImEz() {return grid2DmodalImEz;}
      mfem::ParGridFunction* get_grid2DmodalReHt() {return grid2DmodalReHt;}
      mfem::ParGridFunction* get_grid2DmodalImHt() {return grid2DmodalImHt;}
      mfem::ParGridFunction* get_grid2DmodalReHz() {return grid2DmodalReHz;}
      mfem::ParGridFunction* get_grid2DmodalImHz() {return grid2DmodalImHz;}
};

// Mode or Line 
class Mode
{
   private:
      int startLine;
      int endLine;
      keywordPair Sport;                                   // integer value for the S-parameter port number
      keywordPair net;                                     // net name
      std::vector<IntegrationPath *> integrationPathList;  // voltage or current or both
      FieldSet fields;
      int modeNumber2D;                                    // mode number used for the 2D solution
      std::string calculation;                             // modal | line - for output formatting
      
      // for S-parameter calculation
      std::vector<std::complex<double>> Cp;                // C plus for direction split with a unique value for each driving set
      std::vector<std::complex<double>> Cm;                // C minus for direction split with a unique value for each driving set
      std::vector<std::complex<double>> weight;            // weight for each driving set
      bool net_is_updated=false;                           // flag to prevent updating net names more than once

   public:
      Mode(int,int,std::string);
      ~Mode();
      int get_startLine() {return startLine;}
      int get_endLine() {return endLine;}
      std::string get_net() {return net.get_value();}
      void set_net (std::string name) {net.set_value(name); net.set_loaded(true);}
      bool net_is_loaded () {return net.is_loaded();}
      int get_Sport() {return Sport.get_int_value();}
      int get_Sport_lineNumber() {return Sport.get_lineNumber();}
      std::string get_type (long unsigned int i) {return integrationPathList[i]->get_type();}
      double get_alpha() {return fields.get_alpha();}
      double get_beta() {return fields.get_beta();}
      std::complex<double> get_impedance () {return fields.get_impedance();}
      std::complex<double> get_voltage () {return fields.get_voltage();}
      std::complex<double> get_current () {return fields.get_current();}
      std::complex<double> get_Pz () {return fields.get_Pz();}
      std::complex<double> get_Cp (int i) {return Cp[i];}
      std::complex<double> get_Cm (int i) {return Cm[i];}
      std::complex<double> get_weight (int i) {return weight[i];}
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
      bool load(std::string *, inputFile *);
      void flip2DmodalSign () {fields.flip2DmodalSign();}
      bool inModeBlock (int);
      bool check(std::string *, std::vector<Path *> *, bool, long unsigned int);
      bool align_current_paths (std::string *, std::vector<Path *> *, bool);
      bool checkBoundingBox (mfem::Vector *, mfem::Vector *, std::string *, double, std::vector<Path *> *);
      bool assignPathIndices(std::vector<Path *> *);
      bool is_enclosedByPath (std::vector<Path *> *, Path *, long unsigned int *);
      void print (std::string);
      void save (std::ofstream *);
      void output (std::ofstream *, std::vector<Path *> *, Path *, bool);
      bool loadSolution (std::string *, std::string, size_t, size_t);
      bool scaleSolution ();
      void printSolution ();
      void build2Dgrids (mfem::ParFiniteElementSpace *, mfem::ParFiniteElementSpace *);
      void build3Dgrids (mfem::ParFiniteElementSpace *, mfem::ParFiniteElementSpace *);
      void build2DModalGrids (mfem::ParFiniteElementSpace *, mfem::ParFiniteElementSpace *);
      void fillX (Vec *, Vec *, mfem::Array<int> *, HYPRE_BigInt *, int);
      void fillIntegrationPoints (std::vector<Path *> *);
      IntegrationPath* get_voltageIntegrationPath ();
      IntegrationPath* get_currentIntegrationPath ();
      void calculateLineIntegrals (mfem::ParMesh *, fem3D *);
      void calculateLineIntegrals (mfem::ParMesh *, fem3D *, IntegrationPath *, IntegrationPath *);
      void alignDirections (mfem::ParMesh *, fem3D *, IntegrationPath *, IntegrationPath *);
      void addWeight (std::complex<double> value) {weight.push_back(value);}
      void setWeight (int drivingSet, std::complex<double> value) {weight[drivingSet]=value;}
      std::complex<double> getWeight (long unsigned int i) {return weight[i];}
      void calculateSplits (mfem::ParFiniteElementSpace *, mfem::ParGridFunction *, mfem::ParGridFunction *, mfem::ParGridFunction *,
                            mfem::ParGridFunction *, mfem::ParGridFunction *, mfem::ParGridFunction *, mfem::ParGridFunction *, mfem::ParGridFunction *,
                            mfem::Vector);
      std::complex<double> calculatePowerIn (int);
      std::complex<double> calculatePowerOut (int);
      void set_net_is_updated () {net_is_updated=true;}
      bool get_net_is_updated () {return net_is_updated;}
      void transfer_2Dsolution_2Dgrids_to_3Dgrids ();
      void transfer_2Dsolution_3Dgrids_to_2Dgrids ();
      void save2DParaView (mfem::ParSubMesh *, struct projectData *, double, bool);
      void save3DParaView (mfem::ParMesh *, struct projectData *, double, bool);
      void save2DModalParaView (mfem::ParSubMesh *, struct projectData *, double, bool);
      void resetElementNumbers ();
      void snapToMeshBoundary (std::vector<Path *> *, mfem::Mesh *);
      void populateGamma (double, GammaDatabase *);
      void reset ();
#ifdef HAS_GUI
      void draw (std::vector<Path *> *, struct point *, CustomOpenGLWidget *, QTreeWidget *, CustomTreeWidgetItem *);
#endif
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
      void print (std::string);
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
      bool check(std::string *);
      bool load (std::string *, inputFile *);
      void print (std::string);
      void save (std::ofstream *out);
};


class Port
{
   private:
      int startLine;
      int endLine;
      keywordPair name;                            // alphanumeric name 
      std::vector<keywordPair *> pathNameList;
      std::vector<long unsigned int> pathIndexList;     // outline of the port
      std::vector<bool> reverseList;
      keywordPair impedance_definition;            // VI, PV, or PI
      keywordPair impedance_calculation;           // modal | line
      std::vector<Mode *> modeList;
      std::vector<DifferentialPair *> differentialPairList;
      std::vector<PortAttribute *> attributeList;
      bool assignedToMesh=false;                   // keeps track of whether the port was successfully assigned to the mesh
//      bool appliedPortABCreal=false;               // keeps track of whether the port absorbing boundary condition has been applied to the real part
//      bool appliedPortABCimag=false;               // keeps track of whether the port absorbing boundary condition has been applied to the imag part

      mfem::ND_FECollection *fec2D_ND=nullptr;           // Et
      mfem::ParFiniteElementSpace *fes2D_ND=nullptr;     // on 2D ParMesh

      mfem::H1_FECollection *fec2D_H1=nullptr;           // Ez
      mfem::ParFiniteElementSpace *fes2D_H1=nullptr;     // on 2D ParMesh

      mfem::L2_FECollection *fec2D_L2=nullptr;           // for S-parameter calculations using modal projections
      mfem::ParFiniteElementSpace *fes2D_L2=nullptr;     // on 2D ParMesh

      bool spin180degrees;
      size_t t_size, z_size;
      std::string meshFilename="";
      std::string modesFilename="";
      Path *outline=nullptr;                       // port outline for 3D operations
      Path *rotated_outline=nullptr;               // port outline rotated to x-y plane for 2D operations
      mfem::Vector normal;                         // normal facing outward from the 3D volume
      mfem::Vector rotated_normal;                 // outward facing normal after rotation for the rotated 2D mesh
      lapack_complex_double* Ti;                   // for conversion between modal and line currents
      lapack_complex_double* Tv;                   // for conversion between modal and line voltages
      int TiTvSize;

      mfem::Array<int> *ess_tdof_list=nullptr;           // on 3D mesh and space
      HYPRE_BigInt *offset;

      // grid functions to hold the 3D solutions on the 2D ports
      mfem::ParGridFunction *grid2DsolutionReEt=nullptr;
      mfem::ParGridFunction *grid2DsolutionImEt=nullptr;
      mfem::ParGridFunction *grid2DsolutionReEz=nullptr;
      mfem::ParGridFunction *grid2DsolutionImEz=nullptr;
      mfem::ParGridFunction *grid2DsolutionReHt=nullptr;
      mfem::ParGridFunction *grid2DsolutionImHt=nullptr;
      mfem::ParGridFunction *grid2DsolutionReHz=nullptr;
      mfem::ParGridFunction *grid2DsolutionImHz=nullptr;

#ifdef HAS_GUI
      //std::unordered_map<Handle(AIS_Shape), CustomTreeWidgetItem*> *drawingToItemMap=nullptr;
#endif

   public:
      Port(int,int);
      ~Port();
      int get_startLine () {return startLine;}
      int get_endLine () {return endLine;}
      std::string get_name () {return name.get_value();}
      int get_name_lineNumber () {return name.get_lineNumber();}
      std::string get_impedance_definition () {return impedance_definition.get_value();}
      std::string get_impedance_calculation () {return impedance_calculation.get_value();}
      void set_impedance_definition (std::string value) {impedance_definition.set_value(value);}
      void set_impedance_calculation (std::string value) {impedance_calculation.set_value(value);}
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
      bool load (std::string *, inputFile *);
      bool inBlock (int);
      bool inModeBlocks (int);
      bool findModeBlocks (inputFile *);
      bool inDifferentialPairBlocks (int);
      bool findLineBlocks (inputFile *);
      bool findDifferentialPairBlocks (inputFile *);
      bool check (std::string *, std::vector<Path *> *, bool);
      bool assignPathIndices (std::vector<Path *> *);
      bool checkBoundingBox(mfem::Vector *, mfem::Vector *, std::string *, double, std::vector<Path *> *);
      std::vector<int> get_SportList ();
      void push(long unsigned int a) {pathIndexList.push_back(a);}
      bool merge(std::vector<Path *> *);
      bool is_point_inside (double, double, double);
      bool is_triangleInside (mfem::DenseMatrix *);
      bool is_modePathInside (std::string *, std::vector<Path *> *);
      bool createRotated(std::vector<Path *> *, std::string);
      bool create2Dmesh (int, mfem::ParMesh *, std::vector<mfem::ParSubMesh> *, long unsigned int, double);
      void saveMesh (MeshMaterialList *, std::string *, mfem::ParSubMesh *);
      bool postProcessMesh (std::string, std::string);
      void save2Dsetup(struct projectData *, std::string *, double, Gamma *);
      void saveModeFile (struct projectData *, std::vector<Path *> *, BoundaryDatabase *);
      void set_filenames();
      bool is_overlapPath (Path *);
      mfem::Vector& get_normal() {return normal;}
      mfem::Vector get_rotated_normal() {return rotated_normal;}
      bool createDirectory(std::string *);
      void set2DModeNumbers();
      int get_last_attribute ();
      void print ();
      void save (std::ofstream *);
      void printSolution (std::string);
      void printPaths(std::vector<Path *> *);
      bool solve (std::string *);
      bool loadSolution (std::string *,double);
      bool loadSizes_tz (std::string *);
      bool loadTiTv (std::string *);
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
      bool load_modeMetrics (std::string *, double);
      void build2Dgrids ();
      void build3Dgrids (mfem::ParFiniteElementSpace *, mfem::ParFiniteElementSpace *);
      void build2DSolutionGrids ();
      void build2DModalGrids ();
      void build_essTdofList (mfem::ParFiniteElementSpace *, mfem::ParMesh *);
      bool addPortIntegrators (mfem::ParMesh *, mfem::ParBilinearForm *, mfem::PWConstCoefficient *, mfem::PWConstCoefficient *, mfem::PWConstCoefficient *, std::vector<mfem::Array<int> *> &,
                               std::vector<mfem::ConstantCoefficient *> &, std::vector<mfem::ConstantCoefficient *> &, std::vector<mfem::ConstantCoefficient *> &, std::vector<mfem::ConstantCoefficient *> &,
                               bool, int, bool, std::string);
      void fillX (Vec *, Vec *, int);
      void extract2Dmesh (mfem::ParMesh *, std::vector<mfem::ParSubMesh> *);
      void addWeight (std::complex<double>);
      void calculateSplits ();
      bool isDriving (int);
      Mode* getDrivingMode (int);
      void fillIntegrationPoints (std::vector<Path *> *);
      void calculateLineIntegrals (mfem::ParMesh *, fem3D *);
      void alignDirections (mfem::ParMesh *, fem3D *);
      void transfer_2Dsolution_2Dgrids_to_3Dgrids ();
      void transfer_2Dsolution_3Dgrids_to_2Dgrids ();
      void transfer_3Dsolution_3Dgrids_to_2Dgrids (fem3D *);
      void save2DParaView (mfem::ParSubMesh *, struct projectData *, double, bool);
      void save3DParaView (mfem::ParMesh *, struct projectData *, double, bool);
      void save2DSolutionParaView (mfem::ParSubMesh *, struct projectData *, double, int, bool);
      void save2DModalParaView (mfem::ParSubMesh *, struct projectData *, double, bool);
      void resetElementNumbers ();
      bool snapToMeshBoundary (std::vector<Path *> *, mfem::Mesh *, std::string);
      void populateGamma (double, GammaDatabase *);
      void reset ();
      void aggregateDifferentialPairList (std::vector<DifferentialPair *> *);
      void buildAggregateModeList (std::vector<Mode *> *);
      bool has_mode (Mode *, long unsigned int *);
#ifdef HAS_GUI
      void draw (struct projectData *, std::vector<Path *> *, CustomOpenGLWidget *, QTreeWidget *, CustomTreeWidgetItem *);
#endif
};

// boundary conditions and ports
class BoundaryDatabase
{
   private:
      inputFile inputs;
      std::vector<SourceFile *> sourceFileList;
      std::vector<Path *> pathList;
      std::vector<Boundary *> boundaryList;
      std::vector<Port *> portList;
      double tol=1e-14;
      std::string tempDirectory="";
      std::string indent="   ";
      std::string version_name="#OpenParEMports";
      std::string version_value="1.0";
      std::string drivingSetName="";
      std::vector<Current *> radiationCurrents;     //  Aggregated list from Boundary and copied across ranks

      bool modified=false;
   public:
      ~BoundaryDatabase();
      void set_tempDirectory(std::string tempDirectory_) {tempDirectory=tempDirectory_;}
      std::string get_tempDirectory() {return tempDirectory;}
      void set_drivingSetName (std::string drivingSetName_) {drivingSetName=drivingSetName_;}
      std::string get_drivingSetName () {return drivingSetName;}
      void assignAttributes (mfem::Mesh *);
      Boundary* get_defaultBoundary ();
      long unsigned int get_boundaryListSize() {return boundaryList.size();}
      Boundary* get_boundary (long unsigned int i) {return boundaryList[i];}
      bool markMeshBoundaries (mfem::Mesh *mesh);
      bool createDefaultBoundary (struct projectData *, mfem::Mesh *, MaterialDatabase *, BoundaryDatabase *);
      bool inBlocks (int);
      bool findSourceFileBlocks ();
      bool findPathBlocks ();
      bool findBoundaryBlocks ();
      bool findPortBlocks ();
      bool load (const char *, bool);
      bool check (bool);
      bool checkSportNumbering ();
      bool check_scale (mfem::Mesh *, int);
      bool check_overlaps ();
      bool alignRadiationNormals ();
      void subdivide_paths ();
      void print ();
      void save (std::ofstream *);
      bool is_line ();
      bool is_modal ();
      bool is_mixed_mode ();
      bool create2Dmeshes (int, mfem::ParMesh *, std::vector<mfem::ParSubMesh> *);
      std::vector<Path *> get_pathList () {return pathList;}
      Path* get_path (long unsigned int i) {return pathList[i];}
      int getLastAttribute ();
      void savePortMeshes (MeshMaterialList *, std::vector<mfem::ParSubMesh> *);
      void save2Dsetups (struct projectData *, double, GammaDatabase *);
      void saveModeFiles (struct projectData *);
      Boundary* get_matchBoundary (double, double, double, double, double, double);
      bool createPortDirectories ();
      bool solvePorts (int, mfem::ParMesh *, std::vector<mfem::ParSubMesh> *, double, MeshMaterialList *, struct projectData *, GammaDatabase *);
      bool loadPortSolutions (double);
      void populateGamma (double, GammaDatabase *);
      void printPortSolutions ();
      int get_totalModeCount ();
      int get_SportCount ();
      int get_maxDofCount ();
      void set2DModeNumbers ();
      void build_portEssTdofLists (mfem::ParFiniteElementSpace *, mfem::ParMesh *);
      bool addPortIntegrators (mfem::ParMesh *, mfem::ParBilinearForm *, mfem::PWConstCoefficient *, mfem::PWConstCoefficient *, mfem::PWConstCoefficient *, std::vector<mfem::Array<int> *> &,
                               std::vector<mfem::ConstantCoefficient *> &, std::vector<mfem::ConstantCoefficient *> &, std::vector<mfem::ConstantCoefficient *> &, std::vector<mfem::ConstantCoefficient *> &,
                               bool, int, bool, std::string);
      void addImpedanceIntegrators (double, double, mfem::ParMesh *, mfem::ParBilinearForm *, MaterialDatabase *,
                                    std::vector<mfem::Array<int> *> &, std::vector<mfem::ConstantCoefficient *> &, bool);
      void fillX (Vec *, Vec *, int);
      void showPortDofCounts ();
      void extract2Dmesh (mfem::ParMesh *, std::vector<mfem::ParSubMesh> *);
      void createDrivingSets ();
      void calculateSplits ();
      Mode* getDrivingMode (int);
      void fillIntegrationPoints ();
      void calculateLineIntegrals (mfem::ParMesh *, fem3D *);
      void alignDirections (mfem::ParMesh *, fem3D *);
      bool calculateAcceptedPower (int, std::complex<double> *);
      PetscErrorCode calculateS (Result *);
      void build2Dgrids ();
      void build2DModalGrids ();
      void build3Dgrids (mfem::ParFiniteElementSpace *, mfem::ParFiniteElementSpace *);
      void build2DSolutionGrids ();
      void buildGrids (fem3D *);
      void transfer_2Dsolution_2Dgrids_to_3Dgrids ();
      void transfer_2Dsolution_3Dgrids_to_2Dgrids ();
      void transfer_3Dsolution_3Dgrids_to_2Dgrids (fem3D *);
      void save2DParaView (std::vector<mfem::ParSubMesh> *, struct projectData *, double, bool);
      void save3DParaView (mfem::ParMesh *, struct projectData *, double, bool);
      void save2DSolutionParaView (std::vector<mfem::ParSubMesh> *, struct projectData *, double, int, bool);
      void save2DModalParaView (std::vector<mfem::ParSubMesh> *, struct projectData *, double, bool);
      bool solve2Dports (mfem::ParMesh *, std::vector<mfem::ParSubMesh> *, struct projectData *, double, MeshMaterialList *, GammaDatabase *);
      void resetElementNumbers ();
      bool snapToMeshBoundary (mfem::Mesh *);
      PetscErrorCode build_M (Mat *, std::vector<DifferentialPair *> *, BoundaryDatabase *);
      void aggregateDifferentialPairList (std::vector<DifferentialPair *> *);
      bool buildAggregateModeList (std::vector<Mode *> *);
      long unsigned int get_portList_size () {return portList.size();}
      Port* get_port (long unsigned int i) {return portList[i];}
      bool get_port_from_mode (Mode *, Port **, long unsigned int *);
      void reset();
      bool has_Ti ();
      bool has_Tv ();
      Port* get_port (Mode *);
      bool hasRadiationBoundary ();
      bool calculateRadiationCurrents (mfem::ParMesh *, struct projectData *, mfem::Vector, double, double,
                                       mfem::ParGridFunction *, mfem::ParGridFunction *, mfem::ParGridFunction *, mfem::ParGridFunction *);
      void collectRadiationCurrents ();
      void deleteRadiationCurrents ();
      void calculateFarField (double, mfem::Vector, double, double, std::vector<OPEMpoint *> *, long unsigned int, long unsigned int);
      void calculateFarField (double, mfem::Vector, double, double, std::vector<OPEMpoint *> *);
      void deletePort (std::string);
#ifdef HAS_GUI
      void draw (struct projectData *, CustomOpenGLWidget *, QTreeWidget *, CustomTreeWidgetItem *, CustomTreeWidgetItem *, MaterialDatabase *);
#endif
};

#endif
