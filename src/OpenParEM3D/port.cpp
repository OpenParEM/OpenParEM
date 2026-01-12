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

#include <quadmath.h>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <csignal>
//#include <atomic>
#include <filesystem>

#include "port.hpp"
#include "fem3D.hpp"
#include "results.hpp"
#include "petscsys.h"

#ifdef HAS_GUI
#include <qapplication.h>
#include "../OpenParEMg/CustomSpinBox.h"
#include <Qt>
#include <QComboBox>
#include <QPushButton>
#include "CustomLineEdit.h"
#include "CustomComboBox.h"
#endif

string convertLogic (int a)
{
   string retval;
   if (a) retval="true";
   else retval="false";
   return retval;
}

string convertLogic (bool a)
{
   string retval;
   if (a) retval="true";
   else retval="false";
   return retval;
}

double edgeLength3D (double *a, double *b)
{
   return sqrt(pow(a[0]-b[0],2)+pow(a[1]-b[1],2)+pow(a[2]-b[2],2));
}

///////////////////////////////////////////////////////////////////////////////////////////
// RotatedMesh
///////////////////////////////////////////////////////////////////////////////////////////

void RotatedMesh::set_spaceDim (int spaceDim_)
{
   spaceDim=spaceDim_;
}

bool RotatedMesh::rotate (Path *rotated, bool spin180degrees)
{
   double coord[3];

   if (rotated == nullptr) {
      cout << "ASSERT: RotatedMesh::rotate passed invalid Path." << endl;
      return true;
   }

   int i=0;
   while (i < vertices.Size()) {
      coord[0]=vertices[i](0);
      coord[1]=vertices[i](1);
      coord[2]=vertices[i](2);

      rotated->rotatePoint(&coord[0],&coord[1],&coord[2],spin180degrees);

      // set z to 0
      coord[2]=0;

      vertices[i].SetCoords(3,coord);

      i++;
   }

   return false;
}

RotatedMesh::~RotatedMesh ()
{
   //cout << "~RotatedMesh: " << this << endl;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Gamma
///////////////////////////////////////////////////////////////////////////////////////////

void Gamma::set (int Sport_, int modeNumber2D_, double alpha_, double beta_, double frequency_)
{
   Sport=Sport_;
   modeNumber2D=modeNumber2D_;
   alpha=alpha_;
   beta=beta_;
   frequency=frequency_;
}

bool Gamma::is_match (int Sport_, int modeNumber2D_)
{
   if (Sport != Sport_) return false;
   if (modeNumber2D != modeNumber2D_) return false;
   return true;
}

bool Gamma::is_match (int Sport_, int modeNumber2D_, double frequency_)
{
   if (Sport != Sport_) return false;
   if (modeNumber2D != modeNumber2D_) return false;
   if (!double_compare(frequency,frequency_,1e-12)) return false;
   return true;
}

void Gamma::print()
{
   cout << "   Gamma:" << endl;
   cout << "      Sport=" << Sport << endl;
   cout << "      modeNumber2D=" << modeNumber2D << endl;
   cout << "      alpha=" << alpha << endl;
   cout << "      beta=" << beta << endl;
   cout << "      frequency=" << frequency << endl;
}

///////////////////////////////////////////////////////////////////////////////////////////
// GammaDatabase
///////////////////////////////////////////////////////////////////////////////////////////

void GammaDatabase::reset ()
{
   long unsigned int i=0;
   while (i < gammaList.size()) {
      delete gammaList[i];
      i++;
   }
   gammaList.clear();
}

Gamma* GammaDatabase::getGamma (int Sport, int modeNumber2D, double frequency)
{
   Gamma* gamma=nullptr;
   double frequencyDifference=DBL_MAX;
   long unsigned int ikeep=-1,max=-1;

   // get the result with the closest frequency
   long unsigned int i=0;
   while (i < gammaList.size()) {
      if (gammaList[i]->is_match(Sport,modeNumber2D)) {
         double gammaFrequency=gammaList[i]->get_frequency();
         if (abs(frequency-gammaFrequency) < frequencyDifference) {
            frequencyDifference=abs(frequency-gammaFrequency);
            ikeep=i;
         }
      }
      i++;
   }

   // only use the result if it is within a factor of 2
   // Getting too far away with an initial guess in OpenParEM2D can cause an inaccurate solution.
   if (ikeep != max && frequency/gammaList[ikeep]->get_frequency() <= 2 &&
                       frequency/gammaList[ikeep]->get_frequency() >= 0.5) gamma=gammaList[ikeep];

   return gamma;
}

void GammaDatabase::print ()
{
   cout << "GammaDatabase: " << this << endl;
   long unsigned int i=0;
   while (i < gammaList.size()) {
      gammaList[i]->print();
      i++;
   }
}

GammaDatabase::~GammaDatabase ()
{
   long unsigned int i=0;
   while (i < gammaList.size()) {
      delete gammaList[i];
      i++;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
// OPEMIntegrationPoint
///////////////////////////////////////////////////////////////////////////////////////////

OPEMIntegrationPoint::OPEMIntegrationPoint (int pointNumber_, double x, double y, double z)
{
   pointNumber=pointNumber_;
   rank=-1;
   initialized=0;
   point.SetSize(3,1);
   point(0,0)=x;
   point(1,0)=y;
   point(2,0)=z;
   elementNumber=-1;
   fieldX=complex<double>(DBL_MAX,DBL_MAX);
   fieldY=complex<double>(DBL_MAX,DBL_MAX);
   fieldZ=complex<double>(DBL_MAX,DBL_MAX);
   pt.SetSize(3);
}

void OPEMIntegrationPoint::get_location (double *x, double *y, double *z)
{
   *x=point(0,0);
   *y=point(1,0);
   *z=point(2,0);
}

void OPEMIntegrationPoint::update (ParMesh *pmesh)
{
   PetscMPIInt rank_;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank_);

   bool search=false;
   ElementTransformation *eltransf;
   InverseElementTransformation *inv_tr=new InverseElementTransformation;

   inv_tr->SetPrintLevel(-1);         // -1 - never print
   inv_tr->SetInitialGuessType(InverseElementTransformation::Center);
   inv_tr->SetSolverType(InverseElementTransformation::Newton);

   pt(0)=point(0,0);
   pt(1)=point(1,0);
   pt(2)=point(2,0);

   if (elementNumber < 0) {
      if (!initialized) search=true;
   } else {
      if (rank == rank_) {
         eltransf=pmesh->GetElementTransformation(elementNumber);
         inv_tr->SetTransformation(*eltransf);
         int res=inv_tr->Transform(pt,integrationPoint);
         if (res != InverseElementTransformation::Inside) search=true;
      }
   }

   if (search) {
      int i=0;
      while (i < pmesh->GetNE()) {
         eltransf=pmesh->GetElementTransformation(i);
         inv_tr->SetTransformation(*eltransf);
         int res=inv_tr->Transform(pt,integrationPoint);
         if (res == InverseElementTransformation::Inside) {
            elementNumber=i;
            rank=rank_;
            break;
         }
         i++;
      }
   }

   initialized=1;

   delete inv_tr;
}

void OPEMIntegrationPoint::set (double ReFieldX, double ImFieldX, double ReFieldY, double ImFieldY, double ReFieldZ, double ImFieldZ)
{
   fieldX=complex<double>(ReFieldX,ImFieldX);
   fieldY=complex<double>(ReFieldY,ImFieldY);
   fieldZ=complex<double>(ReFieldZ,ImFieldZ);
}

void OPEMIntegrationPoint::get_fields (complex<double> *fieldX_, complex<double> *fieldY_, complex<double> *fieldZ_)
{
   *fieldX_=fieldX;
   *fieldY_=fieldY;
   *fieldZ_=fieldZ;
}

void OPEMIntegrationPoint::get_fieldValue (ParGridFunction *grid_re, ParGridFunction *grid_im)
{
   if (elementNumber < 0) return;

   Vector ReValue,ImValue;
   grid_re->GetVectorValue(elementNumber,integrationPoint,ReValue);
   grid_im->GetVectorValue(elementNumber,integrationPoint,ImValue);
   fieldX=complex<double>(ReValue.Elem(0),ImValue.Elem(0));
   fieldY=complex<double>(ReValue.Elem(1),ImValue.Elem(1));
   fieldZ=complex<double>(ReValue.Elem(2),ImValue.Elem(2));
}

void OPEMIntegrationPoint::resetElementNumber ()
{
   elementNumber=-1;
}

void OPEMIntegrationPoint::send(int destination)
{
   MPI_Send(&pointNumber,1,MPI_INT,destination,100,PETSC_COMM_WORLD);
   MPI_Send(&rank,1,MPI_INT,destination,101,PETSC_COMM_WORLD);
   MPI_Send(&elementNumber,1,MPI_INT,destination,103,PETSC_COMM_WORLD);

   double valueReX=real(fieldX);
   MPI_Send(&valueReX,1,MPI_DOUBLE,destination,104,PETSC_COMM_WORLD);

   double valueImX=imag(fieldX);
   MPI_Send(&valueImX,1,MPI_DOUBLE,destination,105,PETSC_COMM_WORLD);

   double valueReY=real(fieldY);
   MPI_Send(&valueReY,1,MPI_DOUBLE,destination,106,PETSC_COMM_WORLD);

   double valueImY=imag(fieldY);
   MPI_Send(&valueImY,1,MPI_DOUBLE,destination,107,PETSC_COMM_WORLD);

   double valueReZ=real(fieldZ);
   MPI_Send(&valueReZ,1,MPI_DOUBLE,destination,108,PETSC_COMM_WORLD);

   double valueImZ=imag(fieldZ);
   MPI_Send(&valueImZ,1,MPI_DOUBLE,destination,109,PETSC_COMM_WORLD);
}

void OPEMIntegrationPoint::print()
{
   cout << "OPEMIntegrationPoint: this=" << this << endl;
   cout << "   pointNumber=" << pointNumber << endl;
   cout << "   rank=" << rank << endl;
   cout << "   initialized=" << initialized << endl;
   cout << "   point=(" << point(0,0) << "," << point(1,0) << "," << point(2,0) << ")" << endl;
   cout << "   elementNumber=" << elementNumber << endl;
   cout << "   integrationPoint=(" << integrationPoint.x << "," << integrationPoint.y << "," << integrationPoint.z << ")" << endl;
   cout << "   fieldX=" << fieldX << endl;
   cout << "   fieldY=" << fieldY << endl;
   cout << "   fieldZ=" << fieldZ << endl;
}

///////////////////////////////////////////////////////////////////////////////////////////
// OPEMIntegrationPointList
///////////////////////////////////////////////////////////////////////////////////////////

void OPEMIntegrationPointList::update (ParMesh *pmesh)
{
   long unsigned int i=0;
   while (i < points.size()) {
      points[i]->update(pmesh);
      i++;
   }
}

void OPEMIntegrationPointList::get_fieldValues (ParGridFunction *grid_re, ParGridFunction *grid_im)
{
   if (!grid_re || !grid_im) return;

   long unsigned int i=0;
   while (i < points.size()) {
      points[i]->get_fieldValue(grid_re,grid_im);
      i++;
   }
}

void OPEMIntegrationPointList::assemble()
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // collect to 0
   if (rank == 0) {
     int i=1;
     while (i < size) {
        long unsigned int j=0;
        while (j < points.size()) {
           int pointNumber_;
           int rank_;
           int elementNumber_;
           double ReFieldX,ImFieldX,ReFieldY,ImFieldY,ReFieldZ,ImFieldZ;

           MPI_Recv(&pointNumber_,1,MPI_INT,i,100,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
           MPI_Recv(&rank_,1,MPI_INT,i,101,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
           MPI_Recv(&elementNumber_,1,MPI_INT,i,103,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
           MPI_Recv(&ReFieldX,1,MPI_DOUBLE,i,104,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
           MPI_Recv(&ImFieldX,1,MPI_DOUBLE,i,105,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
           MPI_Recv(&ReFieldY,1,MPI_DOUBLE,i,106,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
           MPI_Recv(&ImFieldY,1,MPI_DOUBLE,i,107,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
           MPI_Recv(&ReFieldZ,1,MPI_DOUBLE,i,108,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
           MPI_Recv(&ImFieldZ,1,MPI_DOUBLE,i,109,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

           if (elementNumber_ >= 0) {
              points[pointNumber_]->set(ReFieldX,ImFieldX,ReFieldY,ImFieldY,ReFieldZ,ImFieldZ);
           }

           j++;
        }
        i++;
     }
   } else {
      long unsigned int i=0;
      while (i < points.size()) {
         points[i]->send(0);
         i++;
      }
   }

   // send the complete set back out
   if (rank == 0) {
      int i=1;
      while (i < size) {
         long unsigned int j=0;
         while (j < points.size()) {
           points[j]->send(i);
           j++;
         }
         i++;
      }
   } else {
      long unsigned int j=0;
      while (j < points.size()) {
         int pointNumber_;
         int rank_;
         int elementNumber_;
         double ReFieldX,ImFieldX,ReFieldY,ImFieldY,ReFieldZ,ImFieldZ;

         MPI_Recv(&pointNumber_,1,MPI_INT,0,100,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&rank_,1,MPI_INT,0,101,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&elementNumber_,1,MPI_INT,0,103,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&ReFieldX,1,MPI_DOUBLE,0,104,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&ImFieldX,1,MPI_DOUBLE,0,105,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&ReFieldY,1,MPI_DOUBLE,0,106,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&ImFieldY,1,MPI_DOUBLE,0,107,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&ReFieldZ,1,MPI_DOUBLE,0,108,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&ImFieldZ,1,MPI_DOUBLE,0,109,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         points[pointNumber_]->set(ReFieldX,ImFieldX,ReFieldY,ImFieldY,ReFieldZ,ImFieldZ);

         j++;
      }
   }
}

void OPEMIntegrationPointList::integrate ()
{
   double x1,y1,z1;
   double x2,y2,z2;
   double length,nhatx,nhaty,nhatz;
   double segmentLength;
   complex<double> fieldX1,fieldY1,fieldZ1;
   complex<double> fieldX2,fieldY2,fieldZ2; 
   complex<double> valueX,valueY,valueZ;

   integratedValue=complex<double>(0,0);

   nhatx=1; nhaty=1; nhatz=1;
   if (points.size() > 2) {
      points[0]->get_location(&x1,&y1,&z1);
      points[1]->get_location(&x2,&y2,&z2);

      length=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)+(z2-z1)*(z2-z1));
      nhatx=(x2-x1)/length;
      nhaty=(y2-y1)/length;
      nhatz=(z2-z1)/length;
   }

   long unsigned int i=0;
   while (i < points.size()-1) {
      points[i]->get_location(&x1,&y1,&z1);
      points[i+1]->get_location(&x2,&y2,&z2);

      segmentLength=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)+(z2-z1)*(z2-z1));

      points[i]->get_field(&fieldX1,&fieldY1,&fieldZ1);
      points[i+1]->get_field(&fieldX2,&fieldY2,&fieldZ2);

      // enable the calculation to continue in case a point lies just outside the mesh
      if (fieldX1 == complex<double>(DBL_MAX,DBL_MAX) || fieldX2 == complex<double>(DBL_MAX,DBL_MAX) ||
          fieldY1 == complex<double>(DBL_MAX,DBL_MAX) || fieldY2 == complex<double>(DBL_MAX,DBL_MAX) ||
          fieldZ1 == complex<double>(DBL_MAX,DBL_MAX) || fieldZ2 == complex<double>(DBL_MAX,DBL_MAX)) {i++; continue;}

      valueX=(fieldX1+fieldX2)/2;
      valueY=(fieldY1+fieldY2)/2;
      valueZ=(fieldZ1+fieldZ2)/2;

      integratedValue+=(valueX*nhatx+valueY*nhaty+valueZ*nhatz)*segmentLength;

      i++;
   }
}

void OPEMIntegrationPointList::resetElementNumbers ()
{
   long unsigned int i=0;
   while (i < points.size()) {
      points[i]->resetElementNumber();
      i++;
   }
}

void OPEMIntegrationPointList::print()
{
   cout << "reverse=" << reverse << endl;
   long unsigned int i=0;
   while (i < points.size()) {
      points[i]->print();
      i++;
   }
}

OPEMIntegrationPointList::~OPEMIntegrationPointList ()
{
   long unsigned int i=0;
   while (i < points.size()) {
      if (points[i]) delete points[i];
      i++;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Boundary
///////////////////////////////////////////////////////////////////////////////////////////

Boundary::Boundary(int startLine_, int endLine_)
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

   // wave_impedance
   wave_impedance.push_alias("wave_impedance");
   wave_impedance.set_loaded(false);
   wave_impedance.set_positive_required(true);
   wave_impedance.set_non_negative_required(false);
   wave_impedance.set_lowerLimit(1e-6);
   wave_impedance.set_upperLimit(1e9);
   wave_impedance.set_checkLimits(true);

   is_default=false;
   normal.SetSize(3);
   normal(0)=-1; normal(1)=-1; normal(2)=-1;

   modified=false;

#if HAS_GUI
   doubleValidator.setBottom(0);
#endif
}

bool Boundary::load(string *indent, inputFile *inputs)
{
   bool fail=false;
   bool found_first_path=false;

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {

      string token,value,line;
      line=inputs->get_line(lineNumber);
      get_token_pair(&line,&token,&value,&lineNumber,*indent);

      int recognized=0;

      if (name.match_alias(&token)) {
         if (name.is_loaded()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3014: Duplicate entry at line %d for previous entry at line %d.\n",
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

      if (type.match_alias(&token)) {
         if (type.is_loaded()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3015: Duplicate entry at line %d for previous entry at line %d.\n",
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

      if (material.match_alias(&token)) {
         if (material.is_loaded()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3016: Duplicate entry at line %d for previous entry at line %d.\n",
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

      if (wave_impedance.match_alias(&token)) {
         recognized++;
         if (wave_impedance.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (token.compare("path") == 0) {
         if (found_first_path) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3017: Extraneous path= statement at line %d.\n",
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
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3018: Misformatted path at line %d.\n",
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
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3019: Missing path= statement before line %d.\n",
                                                   indent->c_str(),indent->c_str(),lineNumber);
            fail=true;
         }
         recognized++;
      }

      if (token.compare("path-") == 0) {
         if (found_first_path) {
            if (value.substr(0,1).compare("+") == 0 || value.substr(0,1).compare("-") == 0) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3020: Misformatted path at line %d.\n",
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
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3021: Missing path= statement before line %d.\n",
                                                   indent->c_str(),indent->c_str(),lineNumber);
            fail=true;
         }
         recognized++;
      }

      // should recognize one keyword
      if (recognized != 1) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3022: Unrecognized keyword at line %d.\n",
                                                indent->c_str(),indent->c_str(),lineNumber);
         fail=true;
      }
      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }

   // apply defaults

   if (is_radiation() && !wave_impedance.is_loaded()) {
      wave_impedance.set_dbl_value(sqrt(M_PI*4e-7/8.8541878176e-12));
      wave_impedance.set_loaded(true);
   }

   return fail;
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

bool Boundary::is_radiation()
{
   if (type.get_value().compare("radiation") == 0) return true;
   return false;
}

void Boundary::print ()
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Boundary\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   name=%s\n",get_name().c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   type=%s\n",get_type().c_str());
   if (is_surface_impedance() && type.is_loaded()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   material=%s\n",get_material().c_str());}
   if (is_radiation()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   wave_impedance=%g\n",get_wave_impedance());}
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
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   outward normal=(%g,%g,%g)\n",normal(0),normal(1),normal(2));
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   attribute=%d\n",attribute);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   assignedToMesh=%d\n",assignedToMesh);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   outline=%p\n",outline);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   is_default=%s\n",convertLogic(is_default).c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"EndBoundary\n");

   return;
}

void Boundary::save (ofstream *out)
{
    PetscMPIInt rank;
    MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

    modified=false;
    if (rank != 0) return;

    *out << "   Boundary" << endl;
    *out << "      name=" << get_name() << endl;
    *out << "      type=" << get_type() << endl;
    if (is_surface_impedance() && type.is_loaded()) {*out << "      material=" << get_material() << endl;}
    if (is_radiation()) {*out << "      wave_impedance=" << get_wave_impedance() << endl;}
    long unsigned int i=0;
    while (i < pathNameList.size()) {
        if (i == 0) {
            if (reverseList[i]) {*out << "      path=-" << pathNameList[i]->get_value() << endl;}
            else {*out << "      path=" << pathNameList[i]->get_value() << endl;}
        } else {
            if (reverseList[i]) {*out << "      path-=" << pathNameList[i]->get_value() << endl;}
            else {*out << "      path+=" << pathNameList[i]->get_value() << endl;}
        }
        i++;
    }

    *out << "   EndBoundary" << endl;
}

bool Boundary::inBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

bool Boundary::check (string *indent, std::vector<Path *> pathList)
{
   bool fail=false;

   // name
   if (!name.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3023: Boundary block at line %d must specify a name.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   // type
   if (! type.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3024: Block at line %d must specify a type.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      return true;
   }

   // must have a path
   if (pathNameList.size() == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3025: Boundary block at line %d must specify a path.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   // material
   if (is_surface_impedance()) {
      if (!material.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3026: Boundary block at line %d must specify a material.\n",
                                                indent->c_str(),indent->c_str(),startLine);
         fail=true;
      }
   } else if (is_perfect_electric_conductor() || is_perfect_magnetic_conductor() || is_radiation()) {
      if (material.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3027: Boundary block at line %d must not specify a material.\n",
                                                indent->c_str(),indent->c_str(),startLine);
         fail=true;
      }
   }

   // wave_impedance
   if (is_surface_impedance() || is_perfect_electric_conductor() || is_perfect_magnetic_conductor()) {
      if (wave_impedance.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3010: Boundary block at line %d must not specify a wave impedance.\n",
                                                indent->c_str(),indent->c_str(),startLine);
         fail=true;
      }
   }

   // paths exist
   long unsigned int i=0;
   while (i < pathNameList.size()) {
      bool found=false;
      long unsigned int j=0;
      while (j < pathList.size()) {
         if (pathNameList[i]->get_value().compare(pathList[j]->get_name()) == 0) {found=true; break;}
         j++;
      }
      if (! found) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3028: Boundary block at line %d specifies a non-existent path.\n",
                                                 indent->c_str(),indent->c_str(),startLine);
         fail=true;
      }
      i++;
   }

   // paths are not duplicated
   i=0;
   while (i < pathNameList.size()-1) {
      long unsigned int j=i+1;
      while (j < pathNameList.size()) {
         if (pathNameList[i]->get_value().compare(pathNameList[j]->get_value()) == 0) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3029: Boundary block at line %d duplicates path \"%s\".\n",
                                                   indent->c_str(),indent->c_str(),startLine,pathNameList[j]->get_value().c_str());
            fail=true;
         }
         j++;
      }
      i++;
   }

   return fail;
}

bool Boundary::assignPathIndices (vector<Path *> *pathList)
{
   bool fail=false;

   long unsigned int i=0;
   while (i < pathNameList.size()) {
      bool found=false;
      long unsigned int j=0;
      while (j < (*pathList).size()) {
         if ((*pathList)[j]->get_name().compare(pathNameList[i]->get_value()) == 0) {
            pathIndexList.push_back(j);
            found=true;
            break;
         }
         j++;
      }
      if (! found) {
         // errors previously reported
         fail=true;
      }
      i++;
   }

   return fail;
}

bool Boundary::checkBoundingBox (mfem::Vector *lowerLeft, mfem::Vector *upperRight, string *indent, double tol, std::vector<Path *> *pathList)
{
   bool fail=false;

   long unsigned int i=0;
   while (i < pathIndexList.size()) {
      if ((*pathList)[pathIndexList[i]]->checkBoundingBox(lowerLeft, upperRight, indent, tol)) fail=true;
      i++;
   }
   return fail;
}

bool Boundary::merge(vector<Path *> *pathList)
{
   // a path must be defined
   if (pathIndexList.size() == 0) return true;

   // no merge is required
   if (pathIndexList.size() == 1) {
      outline=(*pathList)[pathIndexList[0]];
      return false;
   }

   // merge

   if (mergePaths(pathList,&pathIndexList,&reverseList,"Boundary",get_name(),&outline,1e-12)) return true;
   if (!outline) return true;

   stringstream pathName;
   pathName << "B" << get_name() << "_OpenParEM3D_generated";
   outline->set_name(pathName.str());
   pathList->push_back(outline);

   // update the tracking information

   pathNameList.clear();
   keywordPair *newName=new keywordPair();
   newName->set_value(pathName.str());
   pathNameList.push_back(newName);

   pathIndexList.clear();
   pathIndexList.push_back(pathList->size()-1);

   reverseList.clear();
   reverseList.push_back(false);

   return false;
}

bool Boundary::is_point_inside (double x, double y, double z)
{
   if (!outline) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Boundary::is_point_inside operation on a Boundary without an outline.\n");
      return false;
   }

   struct point p=set_point(x,y,z,3);
   if (outline->is_point_inside (p)) return true;
   return false;
}

bool Boundary::is_triangleInside (DenseMatrix *pointMat)
{
   if (!outline) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Boundary::is_triangleInside operation on a Boundary without an outline.\n");
      return false;
   }

   if (is_point_inside (pointMat->Elem(0,0),pointMat->Elem(1,0),pointMat->Elem(2,0)) &&
       is_point_inside (pointMat->Elem(0,1),pointMat->Elem(1,1),pointMat->Elem(2,1)) &&
       is_point_inside (pointMat->Elem(0,2),pointMat->Elem(1,2),pointMat->Elem(2,2))) {
      return true;
   }
   return false;
}

// Check that there is just one path defining the boundary.
// Used for OpenParEMg.
bool Boundary::has_complex_path ()
{
    if (pathIndexList.size() > 1) return true;
    return false;
}

bool Boundary::is_overlapPath (vector<Path *> *pathList, Path *testPath)
{
   if (!outline) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Boundary::is_overlapPath operation on a Boundary without an outline.\n");
      return false;
   }

   // any point interior constitutes an overlap
   long unsigned int i=0;
   while (i < testPath->get_points_size()) {
      if (outline->is_point_interior(testPath->get_point_value(i))) return true;
      i++;
   }

   // all points inside constitutes an overlap
   bool outside=false;
   i=0;
   while (i < testPath->get_points_size()) {
      if (! outline->is_point_inside(testPath->get_point_value(i))) {
         outside=true;
         break;
      }
      i++;
   }
   if (! outside) return true;

   // crossing lines constitutes an overlap
   if (outline->is_path_overlap(testPath)) return true;

   return false;
}

void Boundary::addImpedanceIntegrator (double frequency, double temperature, mfem::ParMesh *pmesh, mfem::ParBilinearForm *pmblf,
                                       MaterialDatabase *materialDatabase, std::vector<mfem::Array<int> *> &borderAttributesList,
                                       std::vector<mfem::ConstantCoefficient *> &ZconstList, bool isReal)
{
   if (is_perfect_electric_conductor() || is_perfect_magnetic_conductor()) return;
   if (isReal) return;

   mfem::Array<int> *border_attributes=new mfem::Array<int>;
   border_attributes->SetSize(pmesh->bdr_attributes.Max());
   (*border_attributes)=0;                // default to nothing
   (*border_attributes)[attribute-1]=1;   // enable this boundary

   double Rs=DBL_MAX;

   if (is_surface_impedance()) {
      Material *material=materialDatabase->get(get_material());
      std::string indent="    ";
      Rs=material->get_Rs(temperature,frequency,1e-12,indent);
   }

   if (is_radiation()) {
      Rs=get_wave_impedance();
   }

   double coef=2*M_PI*frequency*4e-7*M_PI/Rs;
   ConstantCoefficient *Zconst=new ConstantCoefficient(coef);

   pmblf->AddBoundaryIntegrator(new VectorFEMassIntegrator(*Zconst),*border_attributes);

   // save for later deleting
   borderAttributesList.push_back(border_attributes);
   ZconstList.push_back(Zconst);
}

bool Boundary::calculateRadiationCurrents (ParMesh *pmesh, struct projectData *projData, Vector center, double radiation_beta, double lengthLimit,
                                  ParGridFunction *gridReE, ParGridFunction *gridImE, ParGridFunction *gridReH, ParGridFunction *gridImH)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   bool fail=false;

   if (!is_radiation()) return false;

   // get the normal to this boundary

   Vector PosNormal(3);
   PosNormal(0)=normal(0);
   PosNormal(1)=normal(1);
   PosNormal(2)=normal(2);
   VectorConstantCoefficient PosNormalCoef(PosNormal);

   Vector NegNormal(3);
   NegNormal(0)=-normal(0);
   NegNormal(1)=-normal(1);
   NegNormal(2)=-normal(2);
   VectorConstantCoefficient NegNormalCoef(NegNormal);

   // identify this boundary
   Array<int> border_attributes(1);
   border_attributes[0]=attribute;

   // extract the 2D mesh
   ParSubMesh parSubMeshRadiation=ParSubMesh::CreateFromBoundary(*pmesh,border_attributes);

   // create finite elements for the mesh
   ND_FECollection fec2D_ND(projData->fem_order,parSubMeshRadiation.Dimension());
   ParFiniteElementSpace fes2D_ND(&parSubMeshRadiation,&fec2D_ND);

   // create 2D grids
   ParGridFunction gridReRadEt(&fes2D_ND);
   ParGridFunction gridImRadEt(&fes2D_ND);
   ParGridFunction gridReRadHt(&fes2D_ND);
   ParGridFunction gridImRadHt(&fes2D_ND);

   // transfer the 3D solution to the 2D grids

   ParTransferMap *ReEt=new ParTransferMap(*gridReE,gridReRadEt);
   ReEt->Transfer(*gridReE,gridReRadEt);
   delete ReEt; ReEt=nullptr;

   ParTransferMap *ImEt=new ParTransferMap(*gridImE,gridImRadEt);
   ImEt->Transfer(*gridImE,gridImRadEt);
   delete ImEt; ImEt=nullptr;

   ParTransferMap *ReHt=new ParTransferMap(*gridReH,gridReRadHt);
   ReHt->Transfer(*gridReH,gridReRadHt);
   delete ReHt; ReHt=nullptr;

   ParTransferMap *ImHt=new ParTransferMap(*gridImH,gridImRadHt);
   ImHt->Transfer(*gridImH,gridImRadHt);
   delete ImHt; ImHt=nullptr;

   // calculate J

   RT_FECollection fec2D_RT(projData->fem_order,parSubMeshRadiation.Dimension());
   ParFiniteElementSpace fes2D_RT(&parSubMeshRadiation,&fec2D_RT);

   ParGridFunction gridReJ=ParGridFunction(&fes2D_RT);
   ParGridFunction gridImJ=ParGridFunction(&fes2D_RT);

   ParDiscreteLinearOperator posncross (&fes2D_ND,&fes2D_RT);
   posncross.AddDomainInterpolator(new VectorCrossProductInterpolator(PosNormalCoef));
   posncross.Assemble();
   posncross.Finalize();
   posncross.Mult(gridReRadHt,gridReJ);
   posncross.Mult(gridImRadHt,gridImJ);

   // calculate M

   ParGridFunction gridReM=ParGridFunction(&fes2D_RT);
   ParGridFunction gridImM=ParGridFunction(&fes2D_RT);

   ParDiscreteLinearOperator negncross (&fes2D_ND,&fes2D_RT);
   negncross.AddDomainInterpolator(new VectorCrossProductInterpolator(NegNormalCoef));
   negncross.Assemble();
   negncross.Finalize();
   negncross.Mult(gridReRadEt,gridReM);
   negncross.Mult(gridImRadEt,gridImM);

   // refine the mesh

   double wavelength=2*M_PI/radiation_beta;  // assumed mur=1
   double global_shortest=0;
   double global_longest=DBL_MAX;
   double previous_global_longest=DBL_MAX;

   if (projData->output_show_refining_mesh) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"            radiation boundary \"%s\":\n",get_name().c_str());
   }

   while (global_longest > lengthLimit) {

      double shortest=DBL_MAX;
      double longest=0;

      vector<int> elements;
      int e=0;
      while (e < parSubMeshRadiation.GetNE()) {

         Array<int> vertices;
         parSubMeshRadiation.GetElementVertices(e,vertices);

         double element_shortest=DBL_MAX;
         double element_longest=0;

         int j=0;
         while (j < vertices.Size()) {

            double *coordsj=parSubMeshRadiation.GetVertex(vertices[j]);

            // calculate element lengths
            int k=0;
            while (k < vertices.Size()) {
               if (k != j) {
                  double *coordsk=parSubMeshRadiation.GetVertex(vertices[k]);
                  double length=edgeLength3D(coordsj,coordsk)/wavelength;
                  if (length > element_longest) element_longest=length;
                  if (length < element_shortest) element_shortest=length;
               }
               k++;
            }
            j++;
         }

         // save the element for refinement
         if (element_longest > lengthLimit) {
            elements.push_back(e);
         }

         // keep track of overall sizes
         if (element_longest > longest) longest=element_longest;
         if (element_shortest < shortest) shortest=element_shortest;

         e++;
      }

      // get the global overall sizes
      MPI_Allreduce (&shortest,&global_shortest,1,MPI_DOUBLE,MPI_MIN,PETSC_COMM_WORLD);
      MPI_Allreduce (&longest,&global_longest,1,MPI_DOUBLE,MPI_MAX,PETSC_COMM_WORLD);

      int local_element_count=parSubMeshRadiation.GetNE();
      int global_element_count;
      MPI_Allreduce (&local_element_count,&global_element_count,1,MPI_INT,MPI_SUM,PETSC_COMM_WORLD);

      if (projData->output_show_refining_mesh) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"               number of mesh elements=%d\n",global_element_count);
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"                  shortest element edge per wavelength=%g\n",global_shortest);
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"                  longest element edge per wavelength=%g\n",global_longest);
      }

      if (global_longest <= lengthLimit) break;

      if (global_longest > previous_global_longest) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"            ERROR3069: Failure in mesh refinement for adaptive boundary %s.\n", get_name().c_str());
         fail=true;
      }
      previous_global_longest=global_longest;

      // transfer the data for use with GeneralRefinement
      Array<int> localRefineList(elements.size());
      int i=0;
      while (i < (int)elements.size()) {
         localRefineList[i]=elements[i];
         i++;
      }

      // refine and update
      parSubMeshRadiation.GeneralRefinement(localRefineList);
      fes2D_ND.Update();
      fes2D_RT.Update();
      gridReRadEt.Update();
      gridImRadEt.Update();
      gridReRadHt.Update();
      gridImRadHt.Update();
      gridReJ.Update();
      gridImJ.Update();
      gridReM.Update();
      gridImM.Update();

      if (fail) break;
   }

   /*
   // for debug
   stringstream ss;
   ss << "surface_currents_" << get_name();

   stringstream ssParaView;
   ssParaView << "ParaView_debug";

   ParaViewDataCollection *pd=nullptr;
   pd=new ParaViewDataCollection(ss.str(),&parSubMeshRadiation);
   pd->SetOwnData(false);
   pd->SetPrefixPath(ssParaView.str());
   pd->RegisterField("gridReJ",&gridReJ);
   pd->RegisterField("gridImJ",&gridImJ);
   pd->RegisterField("gridReM",&gridReM);
   pd->RegisterField("gridImM",&gridImM);
   pd->SetLevelsOfDetail(3);
   pd->SetDataFormat(VTKFormat::ASCII);
   pd->SetHighOrderOutput(true);
   pd->Save();
   delete pd;
   */

   // transfer the data for more convenient use
   // [Work-around for the fact that ParSubMesh does not have a constructor.]

   int i=0;
   while (i < parSubMeshRadiation.GetNE()) {

      // field values

      // get the antenna point

      int geom=parSubMeshRadiation.GetElementGeometry(i);
      IntegrationPoint ip=Geometries.GetCenter(geom);     // reference coordinates

      Vector ReJvalue,ImJvalue,ReMvalue,ImMvalue;
      gridReJ.GetVectorValue(i,ip,ReJvalue);
      gridImJ.GetVectorValue(i,ip,ImJvalue);
      gridReM.GetVectorValue(i,ip,ReMvalue);
      gridImM.GetVectorValue(i,ip,ImMvalue);

      double area;
      area=parSubMeshRadiation.GetElementVolume(i);

      complex<double> Jx=complex<double>(ReJvalue(0),ImJvalue(0));
      complex<double> Jy=complex<double>(ReJvalue(1),ImJvalue(1));
      complex<double> Jz=complex<double>(ReJvalue(2),ImJvalue(2));

      complex<double> Mx=complex<double>(ReMvalue(0),ImMvalue(0));
      complex<double> My=complex<double>(ReMvalue(1),ImMvalue(1));
      complex<double> Mz=complex<double>(ReMvalue(2),ImMvalue(2));

      // save

      Vector vecrp;
      parSubMeshRadiation.GetElementCenter(i,vecrp);     // physical coordinates

      Current *current=new Current (vecrp(0),vecrp(1),vecrp(2),Jx,Jy,Jz,Mx,My,Mz,area);
      radiationCurrents.push_back(current);

      i++;
   }

   return fail;
}

void Boundary::collectRadiationCurrents (vector<Current *> *collectedCurrents)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   int varCount=16;

   if (rank == 0) {

      // collect currents from this rank
      int i=0;
      while (i < (int)radiationCurrents.size()) {
         Current *current=radiationCurrents[i]->clone();
         collectedCurrents->push_back(current);
         i++;
      }
      deleteRadiationCurrents();

      // collect the currents from other ranks
      i=1;
      while (i < size) {
         int length;
         MPI_Recv(&length,1,MPI_INT,i,10000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         double *pack=(double *)malloc(length*sizeof(double));
         MPI_Recv(pack,length,MPI_DOUBLE,i,10001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int k=0;
         while (k < length/varCount) {
            Current *current=new Current(pack[k*varCount+0],
                                         pack[k*varCount+1],
                                         pack[k*varCount+2],
                                         std::complex(pack[k*varCount+4],pack[k*varCount+5]),
                                         std::complex(pack[k*varCount+6],pack[k*varCount+7]),
                                         std::complex(pack[k*varCount+8],pack[k*varCount+9]),
                                         std::complex(pack[k*varCount+10],pack[k*varCount+11]),
                                         std::complex(pack[k*varCount+12],pack[k*varCount+13]),
                                         std::complex(pack[k*varCount+14],pack[k*varCount+15]),
                                         pack[k*varCount+3]);
            collectedCurrents->push_back(current);
            k++;
         }

         if (pack) free(pack);
         i++;
      }
   } else {

      // pack the data for efficient sending/receiving
      int length=varCount*radiationCurrents.size();
      double *pack=(double *)malloc(length*sizeof(double));
      MPI_Send(&length,1,MPI_INT,0,10000,PETSC_COMM_WORLD);

      long unsigned int i=0;
      while (i < radiationCurrents.size()) {
         pack[varCount*i+0]=radiationCurrents[i]->get_x();
         pack[varCount*i+1]=radiationCurrents[i]->get_y();
         pack[varCount*i+2]=radiationCurrents[i]->get_z();
         pack[varCount*i+3]=radiationCurrents[i]->get_area();
         pack[varCount*i+4]=real(radiationCurrents[i]->get_Jx());
         pack[varCount*i+5]=imag(radiationCurrents[i]->get_Jx());
         pack[varCount*i+6]=real(radiationCurrents[i]->get_Jy());
         pack[varCount*i+7]=imag(radiationCurrents[i]->get_Jy());
         pack[varCount*i+8]=real(radiationCurrents[i]->get_Jz());
         pack[varCount*i+9]=imag(radiationCurrents[i]->get_Jz());
         pack[varCount*i+10]=real(radiationCurrents[i]->get_Mx());
         pack[varCount*i+11]=imag(radiationCurrents[i]->get_Mx());
         pack[varCount*i+12]=real(radiationCurrents[i]->get_My());
         pack[varCount*i+13]=imag(radiationCurrents[i]->get_My());
         pack[varCount*i+14]=real(radiationCurrents[i]->get_Mz());
         pack[varCount*i+15]=imag(radiationCurrents[i]->get_Mz());
         i++;
      }

      MPI_Send(pack,length,MPI_DOUBLE,0,10001,PETSC_COMM_WORLD); 

      if (pack) free(pack);
      deleteRadiationCurrents();
   }

}

/* very slow in MPICH
void Boundary::collectRadiationCurrents (vector<Current *> *collectedCurrents)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   if (rank == 0) {

      // collect currents from this rank
      int i=0;
      while (i < (int)radiationCurrents.size()) {
         Current *current=radiationCurrents[i]->clone();
         collectedCurrents->push_back(current);
         i++;
      }

      // collect currents from other ranks
      i=1;
      while (i < size) {
         int count;
         MPI_Recv(&count,1,MPI_INT,i,10000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         int j=0;
         while (j < count) {
            Current *current=new Current();
            current->recvFrom(i);
            collectedCurrents->push_back(current);
            j++;
         }
         i++;
      }
   } else {
      int count=radiationCurrents.size();
      MPI_Send(&count,1,MPI_INT,0,10000,PETSC_COMM_WORLD);

      // send to rank 0
      long unsigned int i=0;
      while (i < radiationCurrents.size()) {
         radiationCurrents[i]->sendTo(0);
         i++;
      }
   }

   // clear the local currents
   deleteRadiationCurrents();
}
*/

void Boundary::deleteRadiationCurrents ()
{
   long unsigned int i=0;
   while (i < radiationCurrents.size()) {
      delete radiationCurrents[i];
      i++;
   }

   radiationCurrents.clear();
}

bool Boundary::snapToMeshBoundary (vector<Path *> *pathList, Mesh *mesh, string indent)
{
   if (pathIndexList.size() != 1) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Boundary::snapToMeshBoundary operation on a Boundary with an invalid path definition.\n");
      return false;
   }

   // snap the boundary to the mesh boundary
   Path *path=(*pathList)[pathIndexList[0]];
   if (path->snapToMeshBoundary(mesh)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3178: Boundary \"%s\" failed to snap to the mesh boundary.\n",
                                             indent.c_str(),indent.c_str(),get_name().c_str());
      return true;
   }

   return false;
}

#ifdef HAS_GUI

void Boundary::draw (Relay *relay, struct projectData *projData, BoundaryDatabase *boundaryDatabase, CustomOpenGLWidget *drawingWindow, QTreeWidget *drawingItemTree,
                     CustomTreeWidgetItem *boundaryWidgetItem, MaterialDatabase *materialDatabase)
{
    // name

    QString textName=QString::fromStdString(get_name());
    CustomTreeWidgetItem *itemName=new CustomTreeWidgetItem(0);
    itemName->setText(0,textName);
    itemName->set_type(2);
    itemName->setForeground(0,Qt::gray);
    itemName->setFlags(itemName->flags() | Qt::ItemIsEditable);
    boundaryWidgetItem->addChild(itemName);
    itemName->set_OPEMobject(this);

    // link paths -  assumes there is only one path, which is checked when loading the database
    Path *path=boundaryDatabase->get_pathList()[pathIndexList[0]];

    // attach item
    int j=0;
    while (j < boundaryWidgetItem->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) boundaryWidgetItem->child(j);
        if (child->get_OPEMojbect() == path) {
            itemName->push_linkedItem(child);
            child->push_linkedItem(itemName);
        }
        j++;
    }
    drawingWindow->showItem(itemName);

    // type

    CustomTreeWidgetItem *itemType=new CustomTreeWidgetItem(0);
    itemType->setFlags(itemType->flags() | Qt::ItemIsEditable);
    itemType->setToolTip(0,"Boundary type.");
    itemName->addChild(itemType);

    CustomComboBox *comboType=new CustomComboBox();
    comboType->addItem("PEC");
    comboType->addItem("PMC");
    comboType->addItem("Zs");
    comboType->addItem("Radiation");
    comboType->set_port(nullptr);
    comboType->set_boundary(this);
    comboType->set_boundaryDatabase(boundaryDatabase);
    comboType->set_type(2);
    if (is_perfect_electric_conductor()) comboType->setCurrentIndex(0);
    if (is_perfect_magnetic_conductor()) comboType->setCurrentIndex(1);
    if (is_surface_impedance()) comboType->setCurrentIndex(2);
    if (is_radiation()) comboType->setCurrentIndex(3);
    drawingItemTree->setItemWidget(itemType,0,comboType);

    QObject::connect(comboType, &CustomComboBox::CustomCurrentIndexChanged,&comboIndexChanged);
    QObject::connect(comboType,&CustomComboBox::CustomCurrentIndexChanged,relay,&Relay::setMenus);

    // boundary type dependent data

    // material

    CustomTreeWidgetItem *itemMaterial=new CustomTreeWidgetItem(0);
    itemMaterial->setFlags(itemMaterial->flags() | Qt::ItemIsEditable);
    itemMaterial->setToolTip(0,"Boundary material.");
    itemName->addChild(itemMaterial);

    CustomComboBox *comboMaterial=new CustomComboBox();
    if (materialDatabase) {
        long unsigned int i=0;
        while (i < materialDatabase->get_size()) {
            Material *material=materialDatabase->get_material(i);
            // Todo: add only conductors
            comboMaterial->addItem(QString::fromStdString(material->get_name()->get_value()));
            i++;
        }
    }
    drawingItemTree->setItemWidget(itemMaterial,0,comboMaterial);

    QObject::connect(comboMaterial, &CustomComboBox::CustomCurrentTextChanged, &comboTextChanged);
    QObject::connect(comboMaterial,&CustomComboBox::CustomCurrentIndexChanged,relay,&Relay::setMenus);

    // wave impedance

    CustomTreeWidgetItem *itemWaveImpedance=new CustomTreeWidgetItem(0);
    itemWaveImpedance->setFlags(itemWaveImpedance->flags() | Qt::ItemIsEditable);
    itemWaveImpedance->setToolTip(0,"Wave impedance in Ohms.");
    itemName->addChild(itemWaveImpedance);

    CustomLineEdit *textWaveImpedance=new CustomLineEdit();
    textWaveImpedance->setText(QString::number(get_wave_impedance()));
    textWaveImpedance->setValidator(&doubleValidator);
    drawingItemTree->setItemWidget(itemWaveImpedance,0,textWaveImpedance);


    // set initial visibility
    if (is_surface_impedance()) itemWaveImpedance->setHidden(true);
    if (is_radiation()) itemMaterial->setHidden(true);

    // set the CustomTreeWidget items so they can be hidden as needed depending on type
    comboType->set_itemMaterial(itemMaterial);
    comboType->set_itemWaveImpedance(itemWaveImpedance);

    if (is_default) {
        QString textDefault="default";
        CustomTreeWidgetItem *itemDefault=new CustomTreeWidgetItem(0);
        itemDefault->setText(0,textDefault);
        itemName->addChild(itemDefault);
    }
}
#endif

Boundary::~Boundary()
{
   deleteRadiationCurrents();
}

///////////////////////////////////////////////////////////////////////////////////////////
// Integration Path
///////////////////////////////////////////////////////////////////////////////////////////

IntegrationPath::IntegrationPath (int startLine_, int endLine_)
{
   startLine=startLine_;
   endLine=endLine_;

   // type
   type.push_alias("type");
   type.set_loaded(false);
   type.set_positive_required(false);
   type.set_non_negative_required(false);
   type.set_lowerLimit(0);
   type.set_upperLimit(0);
   type.set_checkLimits(false);

   // scale
   scale.push_alias("scale");
   scale.set_loaded(false);
   scale.set_positive_required(true);
   scale.set_non_negative_required(false);
   scale.set_lowerLimit(1e-3);
   scale.set_upperLimit(1e3);
   scale.set_checkLimits(true);

   // defaults
   scale.set_dbl_value(1);
   modified=false;

#if HAS_GUI
   item=nullptr;
   doubleValidator.setBottom(0);
#endif
}

IntegrationPath::IntegrationPath (vector<Path *> *pathList, vector<Path *> *pathsToAdd, string type_)
{
   startLine=0;
   endLine=0;

   // type
   type.push_alias("type");
   type.set_value(type_);
   type.set_loaded(true);
   type.set_positive_required(false);
   type.set_non_negative_required(false);
   type.set_lowerLimit(0);
   type.set_upperLimit(0);
   type.set_checkLimits(false);

   // scale
   scale.push_alias("scale");
   scale.set_loaded(false);
   scale.set_positive_required(true);
   scale.set_non_negative_required(false);
   scale.set_lowerLimit(1e-3);
   scale.set_upperLimit(1e3);
   scale.set_checkLimits(true);

   // defaults
   scale.set_dbl_value(1);

   // data

   long unsigned int i=0;
   while (i < pathsToAdd->size()) {

       // path name
       keywordPair *path=new keywordPair();
       path->push_alias("path");
       path->set_value((*pathsToAdd)[i]->get_name());
       path->set_positive_required(false);
       path->set_non_negative_required(false);
       path->set_lowerLimit(0);
       path->set_upperLimit(0);
       path->set_checkLimits(false);
       path->set_loaded(true);
       pathNameList.push_back(path);

       // path index in the path database
       long unsigned int j=0;
       while (j < pathList->size()) {
           if ((*pathsToAdd)[i] == (*pathList)[j]) {
              pathIndexList.push_back(j);
           }
           j++;
       }

       // reverse list - user can manually correct, if needed
       reverseList.push_back(false);

       i++;
   }

   modified=true;

#if HAS_GUI
   item=nullptr;
   doubleValidator.setBottom(0);
#endif
}

void IntegrationPath::addPaths (vector<Path *> *pathList, vector<Path *> *pathsToAdd)
{
   // data

   long unsigned int i=0;
   while (i < pathsToAdd->size()) {

       // path name
       keywordPair *path=new keywordPair();
       path->push_alias("path");
       path->set_value((*pathsToAdd)[i]->get_name());
       path->set_positive_required(false);
       path->set_non_negative_required(false);
       path->set_lowerLimit(0);
       path->set_upperLimit(0);
       path->set_checkLimits(false);
       path->set_loaded(true);
       pathNameList.push_back(path);

       // path index in the path database
       long unsigned int j=0;
       while (j < pathList->size()) {
           if ((*pathsToAdd)[i] == (*pathList)[j]) {
              pathIndexList.push_back(j);
           }
           j++;
       }

       // reverse list - user can manually correct, if needed
       reverseList.push_back(false);

       i++;
   }

   modified=true;
}

bool IntegrationPath::is_modified ()
{
    if (modified) return true;
    return false;
}

bool IntegrationPath::load(string *indent, inputFile *inputs)
{
   bool fail=false;
   bool found_first_path=false;

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {

      string token,value,line;
      line=inputs->get_line(lineNumber);
      get_token_pair(&line,&token,&value,&lineNumber,*indent);

      int recognized=0;

      if (type.match_alias(&token)) {
         if (type.is_loaded()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3030: Duplicate entry at line %d for previous entry at line %d.\n",
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

      if (scale.match_alias(&token)) {
         recognized++;
         if (scale.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (token.compare("path") == 0) {
         if (found_first_path) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3031: Extraneous path= statement at line %d.\n",
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
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3032: Misformatted path at line %d.\n",
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
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3033: Missing path= statement before line %d.\n",
                                                   indent->c_str(),indent->c_str(),lineNumber);
            fail=true;
         }
         recognized++;
      }

      if (token.compare("path-") == 0) {
         if (found_first_path) {
            if (value.substr(0,1).compare("+") == 0 || value.substr(0,1).compare("-") == 0) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3034: Misformatted path at line %d.\n",
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
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3035: Missing path= statement before line %d.\n",
                                                   indent->c_str(),indent->c_str(),lineNumber);
            fail=true;
         }
         recognized++;
      }

      // should recognize one keyword
      if (recognized != 1) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3036: Unrecognized keyword at line %d.\n",
                                                indent->c_str(),indent->c_str(),lineNumber);
         fail=true;
      }
      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }

   return fail;
}

bool IntegrationPath::inIntegrationPathBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

bool IntegrationPath::check (string *indent, vector<Path *> *pathList)
{
   bool fail=false;

   // type
   if (type.is_loaded()) {
      if (!is_voltage() && !is_current()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3038: Input at line %d must be \"voltage\" or \"current\".\n",
                                                indent->c_str(),indent->c_str(),type.get_lineNumber());
         fail=true;
      }
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3039: IntegreationPath block at line %d must specify a type.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   // scale is optional

   // at least one path is specified
   if (pathNameList.size() == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3040: IntegrationPath block at line %d must specify a path.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   // paths exist
   long unsigned int i=0;
   while (i < pathNameList.size()) {
      bool found=false;
      long unsigned int j=0;
      while (j < pathList->size()) {
         if (pathNameList[i]->get_value().compare((*pathList)[j]->get_name()) == 0) {found=true; break;}
         j++;
      }
      if (! found) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3041: IntegrationPath block at line %d specifies a non-existent path.\n",
                                                indent->c_str(),indent->c_str(),startLine);
         fail=true;
      }
      i++;
   }

   // paths are not duplicated
   i=0;
   while (pathNameList.size() > 0 && i < pathNameList.size()-1) {
      long unsigned int j=i+1;
      while (j < pathNameList.size()) {
         if (pathNameList[i]->get_value().compare(pathNameList[j]->get_value()) == 0) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3042: IntegrationPath block at line %d duplicates path \"%s\".\n",
                                                   indent->c_str(),indent->c_str(),startLine,pathNameList[j]->get_value().c_str());
            fail=true;
         }
         j++;
      }
      i++;
   }

   return fail;
}

bool IntegrationPath::checkBoundingBox (Vector *lowerLeft, Vector *upperRight, string *indent, double tol, vector<Path *> *pathList)
{
   bool fail=false;
   long unsigned int i=0;
   while (i < pathIndexList.size()) {
      if ((*pathList)[pathIndexList[i]]->checkBoundingBox(lowerLeft, upperRight, indent, tol)) fail=true;
      i++;
   }
   return fail;
}

bool IntegrationPath::align (string *indent, vector<Path *> *pathList, double *area, bool check_closed_loop)
{
   bool fail=false;
   bool start_new_path;
   double path_area;
   long unsigned int path_index;
   Path *path;
   double xend,yend,zend;
   struct point p0,pend;
   vector<long unsigned int> pathComponents;
   vector<bool> used;

   if (pathIndexList.size() == 0) {fail=true; return fail;}

   long unsigned int i=0;
   while (i < pathIndexList.size()) {
      used.push_back(false);
      i++;
   }

   *area=0;

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

   start_new_path=true;
   path_area=0;

   i=0;
   while (i < pathIndexList.size()+1) {   // max possible number of loops + 1, enables topology error check

      // get a path
      path=nullptr;
      long unsigned int j=0;
      while (j < used.size()) {
         if (!used[j]) {
            if (start_new_path) {

               path_area=0;

               // pick a path with a dangling end, if that exists

               Path *tpj=(*pathList)[pathIndexList[j]];
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
/*
                     if (double_compare(tpk->get_point_x(0),tpj->get_point_x(0),1e-12) &&
                         double_compare(tpk->get_point_y(0),tpj->get_point_y(0),1e-12) &&
                         double_compare(tpk->get_point_z(0),tpj->get_point_z(0),1e-12)) match_start_count++;
                     if (double_compare(tpk->get_point_x(0),tpj->get_point_x(tpj->get_points_size()-1),1e-12) &&
                         double_compare(tpk->get_point_y(0),tpj->get_point_y(tpj->get_points_size()-1),1e-12) &&
                         double_compare(tpk->get_point_z(0),tpj->get_point_z(tpj->get_points_size()-1),1e-12)) match_end_count++;
                     if (double_compare(tpk->get_point_x(tpk->get_points_size()-1),tpj->get_point_x(0),1e-12) &&
                         double_compare(tpk->get_point_y(tpk->get_points_size()-1),tpj->get_point_y(0),1e-12) &&
                         double_compare(tpk->get_point_z(tpk->get_points_size()-1),tpj->get_point_z(0),1e-12)) match_start_count++;
                     if (double_compare(tpk->get_point_x(tpk->get_points_size()-1),tpj->get_point_x(tpj->get_points_size()-1),1e-12) &&
                         double_compare(tpk->get_point_y(tpk->get_points_size()-1),tpj->get_point_y(tpj->get_points_size()-1),1e-12) &&
                         double_compare(tpk->get_point_z(tpk->get_points_size()-1),tpj->get_point_z(tpj->get_points_size()-1),1e-12)) match_end_count++;
*/
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
                  break;
               }

               if ((match_start_count == 0 && match_end_count == 1) || is_last) {
                  path_index=j;
                  path=(*pathList)[pathIndexList[path_index]];
                  used[path_index]=true;
                  break;
               }
            } else {
               Path *tpj=(*pathList)[pathIndexList[j]];

               struct point pend=set_point(xend,yend,zend,3);
               if (point_comparison(tpj->get_point_value(0),pend,1e-12)) {
                  path_index=j;
                  path=tpj;
                  used[path_index]=true;
                  break;
               }

               if (point_comparison(tpj->get_point_value(tpj->get_points_size()-1),pend,1e-12)) {
                  path_index=j;
                  path=tpj;
                  path->reverseOrder();
                  used[path_index]=true;
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
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3186: IntegrationPath block at line %d is not closed.\n",
                                                      indent->c_str(),indent->c_str(),startLine);
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
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3193: IntegrationPath block at line %d has mixed segment directions.\n",
                                                   indent->c_str(),indent->c_str(),startLine);
            fail=true;
         }

         break;
      }

      // keep track of paths in this loop for reversing direction, if necessary
      pathComponents.push_back(path_index);

      if (start_new_path) {
         start_new_path=false;
         p0=path->get_point_value(0);
      }

      pend=path->get_point_value(path->get_points_size()-1);
      xend=pend.x;
      yend=pend.y;
      zend=pend.z;

      path_area+=path->area();

      // check if this loop is completed by comparing the end vs. the start
      if (point_comparison(p0,pend,1e-12)) {

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
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3190: IntegrationPath block at line %d has mixed segment directions.\n",
                                                   indent->c_str(),indent->c_str(),startLine);
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
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3185: IntegrationPath block at line %d topology error for path %ld.\n",
                                              indent->c_str(),indent->c_str(),startLine,i+1);
      fail=true;
   }

   // check for unused paths - should not occur
   i=0;
   while (i < used.size()) {
      if (!used[i]) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3183: IntegrationPath block at line %d topology error for path %ld.\n",
                                                 indent->c_str(),indent->c_str(),startLine,i+1);
         fail=true;
      }
      i++;
   }

   return fail;
}

/* original all-in-one algorithm
// for currents, paths must form 1 or more closed loops
bool IntegrationPath::check_current_paths (string *indent, vector<Path *> *pathList, bool check_closed_loop)
{
   bool fail=false;

cout << "using old IntegrationPath::check_current_paths" << endl;

   if (!is_current()) return fail;

   vector<bool> closed;
   vector<bool> connectedStart;
   vector<bool> connectedEnd;

   // to keep track of what has been looked at
   long unsigned int i=0;
   while (i < pathIndexList.size()) {
      if ((*pathList)[pathIndexList[i]]->is_closed()) {
         closed.push_back(true);
         connectedStart.push_back(true);
         connectedEnd.push_back(true);
      } else {
         closed.push_back(false);
         connectedStart.push_back(false);
         connectedEnd.push_back(false);
      }
      i++;
   }

   // line up the ends of the open sections
   i=0;
   while (pathIndexList.size() > 0 && i < pathIndexList.size()-1) {
      if (! closed[i]) {

         long unsigned int j=i+1;
         while (j < pathIndexList.size()) {
            if (! closed[j]) {

               // start to start
               if ((*pathList)[pathIndexList[i]]->get_startPoint()->point_compare((*pathList)[pathIndexList[j]]->get_startPoint())) {
                  if (! connectedStart[j]) {
                     connectedStart[i]=true;
                     connectedStart[j]=true;
                  } else {
                     struct point p=(*pathList)[pathIndexList[j]]->get_startPoint()->get_point_value();
                     prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3043: Mode block at line %d topology error at (%g,%g,%g).\n",
                                                            indent->c_str(),indent->c_str(),startLine,p.x,p.y,p.z);
                     fail=true;
                  }
               }

               // start to end
               if ((*pathList)[pathIndexList[i]]->get_startPoint()->point_compare((*pathList)[pathIndexList[j]]->get_endPoint())) {
                  if (! connectedEnd[j]) {
                     connectedStart[i]=true;
                     connectedEnd[j]=true;
                  } else {
                     struct point p=(*pathList)[pathIndexList[j]]->get_endPoint()->get_point_value();
                     prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3044: Mode block at line %d topology error at (%g,%g,%g).\n",
                                                            indent->c_str(),indent->c_str(),startLine,p.x,p.y,p.z);
                     fail=true;
                  }
               }

               // end to start
               if ((*pathList)[pathIndexList[i]]->get_endPoint()->point_compare((*pathList)[pathIndexList[j]]->get_startPoint())) {
                  if (! connectedStart[j]) {
                     connectedEnd[i]=true;
                     connectedStart[j]=true;
                  } else {
                     struct point p=(*pathList)[pathIndexList[j]]->get_startPoint()->get_point_value();
                     prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3045: Mode block at line %d topology error at (%g,%g,%g).\n",
                                                            indent->c_str(),indent->c_str(),startLine,p.x,p.y,p.z);
                     fail=true;
                  }
               }

               // end to end
               if ((*pathList)[pathIndexList[i]]->get_endPoint()->point_compare((*pathList)[pathIndexList[j]]->get_endPoint())) {
                  if (! connectedEnd[j]) {
                     connectedEnd[i]=true;
                     connectedEnd[j]=true;
                  } else {
                     struct point p=(*pathList)[pathIndexList[j]]->get_endPoint()->get_point_value();
                     prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3046: Mode block at line %d topology error at (%g,%g,%g).\n",
                                                            indent->c_str(),indent->c_str(),startLine,p.x,p.y,p.z);
                     fail=true;
                  }
               }

            }
            j++;
         }
      }
      i++;
   }

   // check for dangling ends
   if (check_closed_loop) {
      i=0;
      while (i < pathIndexList.size()) {
         if (! closed[i]) {
            if (! connectedStart[i]) {
               struct point p=(*pathList)[pathIndexList[i]]->get_startPoint()->get_point_value();
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3047: Mode block at line %d topology error with dangling point at (%g,%g,%g).\n",
                                                      indent->c_str(),indent->c_str(),startLine,p.x,p.y,p.z);
               fail=true;
            }
            if (! connectedEnd[i]) {
               struct point p=(*pathList)[pathIndexList[i]]->get_endPoint()->get_point_value();
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3048: Mode block at line %d topology error with dangling point at (%g,%g,%g).\n",
                                                      indent->c_str(),indent->c_str(),startLine,p.x,p.y,p.z);
               fail=true;
            }
         }
         i++;
      }
   }

   return fail;
}
*/

bool IntegrationPath::assignPathIndices (vector<Path *> *pathList)
{
   bool fail=false;

   long unsigned int i=0;
   while (i < pathNameList.size()) {
      bool found=false;
      long unsigned int j=0;
      while (j < (*pathList).size()) {
         if ((*pathList)[j]->get_name().compare(pathNameList[i]->get_value()) == 0) {
            pathIndexList.push_back(j);
            found=true;
            break;
         }
         j++;
      }
      if (! found) {
         // errors previously reported
         fail=true;
      }
      i++;
   }

   return fail;
}

// snap the path to the mesh boundary, where possible
void IntegrationPath::snapToMeshBoundary (vector<Path *> *pathList, Mesh *mesh)
{
   long unsigned int i=0;
   while (i < pathIndexList.size()) {
      Path *path=(*pathList)[pathIndexList[i]];
      path->snapToMeshBoundary(mesh);
      i++;
   }
}

bool IntegrationPath::is_enclosedByPath (vector<Path *> *pathList, Path *testPath)
{
   long unsigned int i=0;
   while (i < pathIndexList.size()) {
      if (! testPath->is_path_inside((*pathList)[pathIndexList[i]])) return false;
      i++;
   }
   return true;
}

// numerically integrate
void IntegrationPath::calculateLineIntegral (ParMesh *pmesh, ParGridFunction *grid_re, ParGridFunction *grid_im)
{
   if (!grid_re || !grid_im) return;

   integratedValue=complex<double>(0,0);
   long unsigned int i=0;
   while (i < pointsList.size()) {
      OPEMIntegrationPointList *points=pointsList[i];
      points->update(pmesh);
      points->get_fieldValues(grid_re,grid_im);
      points->assemble();
      points->integrate();

      if (points->get_reverse()) integratedValue-=get_scale()*points->get_integratedValue();
      else                       integratedValue+=get_scale()*points->get_integratedValue();

      i++;
   }

   if (is_voltage()) integratedValue=-integratedValue;
}

void IntegrationPath::resetElementNumbers ()
{
   long unsigned int i=0;
   while (i < pointsList.size()) {
      pointsList[i]->resetElementNumbers();
      i++;
   }
}

void IntegrationPath::print (string indent)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sIntegrationPath %p\n",indent.c_str(),this);

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   type=%s\n",indent.c_str(),get_type().c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   scale=%g\n",indent.c_str(),get_scale());

   long unsigned int i=0;
   while (i < pathNameList.size()) {
      if (i == 0) {
         if (reverseList[i]) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   path=-%s\n",indent.c_str(),pathNameList[i]->get_value().c_str());}
         else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   path=%s\n",indent.c_str(),pathNameList[i]->get_value().c_str());}
      } else {
         if (reverseList[i]) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   path-=%s\n",indent.c_str(),pathNameList[i]->get_value().c_str());}
         else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   path+=%s\n",indent.c_str(),pathNameList[i]->get_value().c_str());}
      }
      i++;
   }

   i=0;
   while (i < pointsList.size()) {
      pointsList[i]->print();
      i++;
   }

#if HAS_GUI
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   item=%p\n",indent.c_str(),item);
#endif
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sEndIntegrationPath\n",indent.c_str());

   return;
}

void IntegrationPath::save (ofstream *out)
{
    PetscMPIInt rank;
    MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

    modified=false;
    if (rank != 0) return;

    *out << "      IntegrationPath" << endl;
    *out << "         type=" << get_type() << endl;
    if (!double_compare(get_scale(),1,1e-14)) {*out << "         scale=" << get_scale() << endl;}

    long unsigned int i=0;
    while (i < pathNameList.size()) {
        if (i == 0) {
            if (reverseList[i]) {*out << "         path=-" << pathNameList[i]->get_value() << endl;}
            else {*out << "         path=" << pathNameList[i]->get_value() << endl;}
        } else {
            if (reverseList[i]) {*out << "         path-=" << pathNameList[i]->get_value() << endl;}
            else {*out << "         path+=" << pathNameList[i]->get_value() << endl;}
        }
        i++;
    }

    *out << "      EndIntegrationPath" << endl;
}

void IntegrationPath::output (ofstream *out, vector<Path *> *pathList, Path *rotatedPath, bool spin180degrees, bool isModal, int modeNumber2D)
{
   Path *path;

   // paths
   long unsigned int i=0;
   while (i < pathIndexList.size()) {
      path=(*pathList)[pathIndexList[i]]->clone();
      path->rotateToPath(rotatedPath,spin180degrees);
      if (path->output(out,2)) *out << endl;  // drop the z component since the path is rotated
      delete path;
      (*pathList)[pathIndexList[i]]->set_hasOutput();
      i++;
   }

   // mode
   if (isModal) *out << "Mode" << endl;
   else         *out << "Line" << endl;
   if (isModal) *out << "   mode=" << modeNumber2D << endl;
   else         *out << "   line=" << modeNumber2D << endl;
   *out << "   type=" << get_type() << endl;
   *out << "   scale=" << get_scale() << endl;
   i=0;
   while (i < pathNameList.size()) {
      if (i == 0) {
         if (reverseList[0]) *out << "   path=-" << pathNameList[0]->get_value() << endl;
         else *out << "   path=" << pathNameList[0]->get_value() << endl;
      } else {
         if (reverseList[i]) *out << "   path-=" << pathNameList[i]->get_value() << endl;
         else *out << "   path+=" << pathNameList[i]->get_value() << endl;
      }
      i++;
   }
   if (isModal) *out << "EndMode" << endl;
   else         *out << "EndLine" << endl << endl;
}

void IntegrationPath::assignPathNormals (struct point normal, std::vector<Path *> *pathList)
{
    long unsigned int i=0;
    while (i < pathIndexList.size()) {
        (*pathList)[pathIndexList[i]]->assignPathNormal(normal);
        i++;
    }
}

#ifdef HAS_GUI

void IntegrationPath::draw (Relay *relay, BoundaryDatabase *boundaryDatabase, CustomOpenGLWidget *drawingWindow,
                            QTreeWidget *drawingItemTree, CustomTreeWidgetItem *pathItemTree, CustomTreeWidgetItem *itemVI)
{
    // scale

    CustomTreeWidgetItem *itemScale=new CustomTreeWidgetItem(0);
    itemScale->setText(0,"scale");
    itemScale->set_type(12);
    itemScale->setFlags(itemVI->flags() & ~Qt::ItemIsEditable);
    itemScale->setToolTip(0,"Scale factor for the integration path.");
    itemVI->addChild(itemScale);

    //QString enabledBackground="background: rgb(255,255,255);";
    //QString disabledBackground="background: rgb(240,240,240);";

    CustomTreeWidgetItem *itemScaleValue=new CustomTreeWidgetItem(0);
    itemScaleValue->set_type(13);
    itemScaleValue->setFlags(itemScale->flags() & ~Qt::ItemIsSelectable);
    itemScale->addChild(itemScaleValue);

    CustomLineEdit *scaleEdit=new CustomLineEdit();
    scaleEdit->setText(QString::number(get_scale(),'g'));
    scaleEdit->set_itemTracker(drawingWindow->get_itemTracker());
    scaleEdit->set_integrationPath(this);
    scaleEdit->set_boundaryDatabase(boundaryDatabase);
    //scaleEdit->setAlignment(Qt::AlignLeft);
    //scaleEdit->setStyleSheet(enabledBackground);
    scaleEdit->setValidator(&doubleValidator);
    drawingItemTree->setItemWidget(itemScaleValue,0,scaleEdit);

    // paths

    long unsigned int i=0;
    while (i < pathIndexList.size()) {

        // signed name
        QString name="+";
        if (reverseList[i]) name="-";
        name.append(pathNameList[i]->get_value().c_str());

        // tree item
        CustomTreeWidgetItem *itemSegment=new CustomTreeWidgetItem(0);
        //itemSegment->set_AIS_Shape(drawingShape);
        itemSegment->setText(0,name);
        itemSegment->set_type(14);
        itemSegment->setForeground(0,Qt::gray);
        itemSegment->setFlags(itemVI->flags() & ~Qt::ItemIsEditable);
        itemSegment->setToolTip(0,"Path segment for integration.");

        // attach path
        Path *path=boundaryDatabase->get_pathList()[pathIndexList[i]];
        itemSegment->set_OPEMobject((void *)path);

        // attach item
        int j=0;
        while (j < pathItemTree->childCount()) {
            CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) pathItemTree->child(j);
            if (child->get_OPEMojbect() == path) {
                itemSegment->push_linkedItem(child);
                child->push_linkedItem(itemSegment);
            }
            j++;
        }

        itemVI->addChild(itemSegment);
        drawingWindow->showItem(itemSegment);

        i++;
    }

    QObject::connect(scaleEdit,&CustomLineEdit::CustomTextChanged,&textValueChanged);
    QObject::connect(scaleEdit,&CustomLineEdit::CustomTextChanged,relay,&Relay::setMenus);
}

void IntegrationPath::crossLink (std::vector<Path *> *pathList, CustomTreeWidgetItem *portItem)
{
    long unsigned int i=0;
    while (i < pathIndexList.size()) {
        (*pathList)[pathIndexList[i]]->set_portItem(portItem);
        i++;
    }
}

#endif

IntegrationPath::~IntegrationPath ()
{
   long unsigned int i=0;
   while (i < pathNameList.size()) {
      if (pathNameList[i]) delete pathNameList[i];
      i++;
   }

   i=0;
   while (i < pointsList.size()) {
      if (pointsList[i]) delete pointsList[i];
      i++;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
// FieldSet 
///////////////////////////////////////////////////////////////////////////////////////////

bool FieldSet::loadSolution(string *directory, string portName, size_t t_size, size_t z_size, int modeNumber2D)
{
   char filename[128];
   size_t vecSize;
   
   // cd to the project directory

   stringstream projDirectory;
   projDirectory << *directory << "/S" << portName;
   
   try {
      std::filesystem::current_path(projDirectory.str().c_str());
   } catch (std::filesystem::filesystem_error const& ex) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3049: Missing project directory for the 2D solution of Port %s.\n",portName.c_str());
      return true;
   }
   
   // cd to the temp directory

   stringstream tempDirectory;
   tempDirectory << "temp_S" << portName;
   
   try {
      std::filesystem::current_path(tempDirectory.str().c_str());
   } catch (std::filesystem::filesystem_error const& ex) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3050: Missing temporary directory for the 2D solution of Port %s.\n",portName.c_str());
      std::filesystem::current_path("../../"); 
      return true;
   }
   
   // Efield

   sprintf(filename,"Efield_mode_%d.dat",modeNumber2D);
   ifstream ssEigenVecE;
   ssEigenVecE.open(filename,ifstream::in);
   if (ssEigenVecE.is_open()) {
      if (eVecReE) free(eVecReE);
      if (eVecImE) free(eVecImE);
      bool fail=loadData (&ssEigenVecE,&eVecReE,&eVecImE,&vecSize,filename);
      if (! fail) {
     
         if (vecSize == t_size+z_size) {
     
            // for building 2D grids
     
            // Et
     
            if (eigenVecReEt) delete eigenVecReEt;
            eigenVecReEt=new Vector(eVecReE,t_size);
     
            if (eigenVecImEt) delete eigenVecImEt;
            eigenVecImEt=new Vector (eVecImE,t_size);
            // Ez
     
            if (eigenVecReEz) delete eigenVecReEz;
            eigenVecReEz=new Vector; 
            eigenVecReEz->SetDataAndSize(eVecReE+t_size,z_size);

            if (eigenVecImEz) delete eigenVecImEz;
            eigenVecImEz=new Vector;
            eigenVecImEz->SetDataAndSize(eVecImE+t_size,z_size);

         } else {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: FieldSet::loadSolution: Mismatched data sizes.\n");
            std::filesystem::current_path("../../../");
            fail=true;
         }
      }
      ssEigenVecE.close();
      if (fail) return true;
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3051: Unable to open file \"%s\" for reading.\n",filename);
      std::filesystem::current_path("../../../");
      return true;
   }

   // Hfield

   sprintf(filename,"Hfield_mode_%d.dat",modeNumber2D);
   ifstream ssEigenVecH;
   ssEigenVecH.open(filename,ifstream::in);
   if (ssEigenVecH.is_open()) {
      if (eVecReH) free(eVecReH);
      if (eVecImH) free(eVecImH);
      bool fail=loadData (&ssEigenVecH,&eVecReH,&eVecImH,&vecSize,filename);
      if (! fail) {

         if (vecSize == t_size+z_size) {

            // Ht

            if (eigenVecReHt) delete eigenVecReHt;
            eigenVecReHt=new Vector (eVecReH,t_size);

            if (eigenVecImHt) delete eigenVecImHt;
            eigenVecImHt=new Vector (eVecImH,t_size);

            // Hz

            if (eigenVecReHz) delete eigenVecReHz;
            eigenVecReHz=new Vector;
            eigenVecReHz->SetDataAndSize(eVecReH+t_size,z_size);

            if (eigenVecImHz) delete eigenVecImHz;
            eigenVecImHz=new Vector;
            eigenVecImHz->SetDataAndSize(eVecImH+t_size,z_size);

         } else {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: FieldSet::loadSolution: Mismatched data sizes.\n");
            std::filesystem::current_path("../../../");
            fail=true;
         }
      }
      ssEigenVecE.close();
      if (fail) return true;
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3052: Unable to open file \"%s\" for reading.\n",filename);
      std::filesystem::current_path("../../../");
      return true;
   }

   // cd back to the 3D project directory
   std::filesystem::current_path("../../../");

   return false;
}

// scale by the largest Et component
bool FieldSet::scaleSolution ()
{
   double mag2,magMax=0;
   double indexMax=0;

   // find the max Et value
   int i=0;
   while (i < eigenVecReEt->Size()) {
      mag2=(*eigenVecReEt)[i]*(*eigenVecReEt)[i]+(*eigenVecImEt)[i]*(*eigenVecImEt)[i];
      if (mag2 > magMax) {indexMax=i; magMax=mag2;}
      i++;
   }
   complex<double> maxEt=complex<double>((*eigenVecReEt)(indexMax),(*eigenVecImEt)(indexMax));

   // scale Et, Ht

   i=0;
   while (i < eigenVecReEt->Size()) {
      complex<double> E=complex<double>((*eigenVecReEt)(i),(*eigenVecImEt)(i))/maxEt;
      (*eigenVecReEt)(i)=real(E);
      (*eigenVecImEt)(i)=imag(E);

      complex<double> H=complex<double>((*eigenVecReHt)(i),(*eigenVecImHt)(i))/maxEt;
      (*eigenVecReHt)(i)=real(H);
      (*eigenVecImHt)(i)=imag(H);

      i++;
   }

   // scale Ez, Hz
   i=0;
   while (i < eigenVecReEz->Size()) {
      complex<double> E=complex<double>((*eigenVecReEz)(i),(*eigenVecImEz)(i))/maxEt;
      (*eigenVecReEz)(i)=real(E);
      (*eigenVecImEz)(i)=imag(E);

      complex<double> H=complex<double>((*eigenVecReHz)(i),(*eigenVecImHz)(i))/maxEt;
      (*eigenVecReHz)(i)=real(H);
      (*eigenVecImHz)(i)=imag(H);

      i++;
   }

   return false;
}

void FieldSet::build2Dgrids (ParFiniteElementSpace *fes_ND, ParFiniteElementSpace *fes_H1)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   HYPRE_BigInt *offset_ND=fes_ND->GetTrueDofOffsets();
   HYPRE_BigInt *offset_H1=fes_H1->GetTrueDofOffsets();

   // E

   grid2DReEt=new ParGridFunction(fes_ND);
   *grid2DReEt=0.0;
   if (offset_ND[0] < offset_ND[1]) {
      Vector local(offset_ND[1]-offset_ND[0]);
      int i=0;
      while (i < offset_ND[1]-offset_ND[0]) {
         local.Elem(i)=eigenVecReEt->Elem(offset_ND[0]+i);
         i++;
      }
      grid2DReEt->Distribute(&local);
   }

   grid2DImEt=new ParGridFunction(fes_ND);
   *grid2DImEt=0.0;
   if (offset_ND[0] < offset_ND[1]) {
      Vector local(offset_ND[1]-offset_ND[0]);
      int i=0;
      while (i < offset_ND[1]-offset_ND[0]) {
         local.Elem(i)=eigenVecImEt->Elem(offset_ND[0]+i);
         i++;
      }
      grid2DImEt->Distribute(&local);
   }

   grid2DReEz=new ParGridFunction(fes_H1);
   *grid2DReEz=0.0;
   if (offset_H1[0] < offset_H1[1]) {
      Vector local(offset_H1[1]-offset_H1[0]);
      int i=0;
      while (i < offset_H1[1]-offset_H1[0]) {
         local.Elem(i)=eigenVecReEz->Elem(offset_H1[0]+i);
         i++;
      }
      grid2DReEz->Distribute(&local);
   }

   grid2DImEz=new ParGridFunction(fes_H1);
   *grid2DImEz=0.0;
   if (offset_H1[0] < offset_H1[1]) {
      Vector local(offset_H1[1]-offset_H1[0]);
      int i=0;
      while (i < offset_H1[1]-offset_H1[0]) {
         local.Elem(i)=eigenVecImEz->Elem(offset_H1[0]+i);
         i++;
      }
      grid2DImEz->Distribute(&local);
   }

   // H

   grid2DReHt=new ParGridFunction(fes_ND);
   *grid2DReHt=0.0;
   if (offset_ND[0] < offset_ND[1]) {
      Vector local(offset_ND[1]-offset_ND[0]);
      int i=0;
      while (i < offset_ND[1]-offset_ND[0]) {
         local.Elem(i)=eigenVecReHt->Elem(offset_ND[0]+i);
         i++;
      }
      grid2DReHt->Distribute(&local);
   }

   grid2DImHt=new ParGridFunction(fes_ND);
   *grid2DImHt=0.0;
   if (offset_ND[0] < offset_ND[1]) {
      Vector local(offset_ND[1]-offset_ND[0]);
      int i=0;
      while (i < offset_ND[1]-offset_ND[0]) {
         local.Elem(i)=eigenVecImHt->Elem(offset_ND[0]+i);
         i++;
      }
      grid2DImHt->Distribute(&local);
   }

   grid2DReHz=new ParGridFunction(fes_H1);
   *grid2DReHz=0.0;
   if (offset_H1[0] < offset_H1[1]) {
      Vector local(offset_H1[1]-offset_H1[0]);
      int i=0;
      while (i < offset_H1[1]-offset_H1[0]) {
         local.Elem(i)=eigenVecReHz->Elem(offset_H1[0]+i);
         i++;
      }
      grid2DReHz->Distribute(&local);
   }

   grid2DImHz=new ParGridFunction(fes_H1);
   *grid2DImHz=0.0;
   if (offset_H1[0] < offset_H1[1]) {
      Vector local(offset_H1[1]-offset_H1[0]);
      int i=0;
      while (i < offset_H1[1]-offset_H1[0]) {
         local.Elem(i)=eigenVecImHz->Elem(offset_H1[0]+i);
         i++;
      }
      grid2DImHz->Distribute(&local);
   }
}

void FieldSet::build3Dgrids (ParFiniteElementSpace *fes_ND, ParFiniteElementSpace *fes_H1)
{
   // E

   grid3DReEt=new ParGridFunction(fes_ND);
   *grid3DReEt=0.0;

   grid3DImEt=new ParGridFunction(fes_ND);
   *grid3DImEt=0.0;

   grid3DReEz=new ParGridFunction(fes_H1);
   *grid3DReEz=0.0;

   grid3DImEz=new ParGridFunction(fes_H1);
   *grid3DImEz=0.0;

   // H

   grid3DReHt=new ParGridFunction(fes_ND);
   *grid3DReHt=0.0;

   grid3DImHt=new ParGridFunction(fes_ND);
   *grid3DImHt=0.0;

   grid3DReHz=new ParGridFunction(fes_H1);
   *grid3DReHz=0.0;

   grid3DImHz=new ParGridFunction(fes_H1);
   *grid3DImHz=0.0;
}

void FieldSet::build2DModalGrids (ParFiniteElementSpace *fes_ND, ParFiniteElementSpace *fes_H1)
{
   // E

   grid2DmodalReEt=new ParGridFunction(fes_ND);
   *grid2DmodalReEt=0.0;

   grid2DmodalImEt=new ParGridFunction(fes_ND);
   *grid2DmodalImEt=0.0;

   grid2DmodalReEz=new ParGridFunction(fes_H1);
   *grid2DmodalReEz=0.0;

   grid2DmodalImEz=new ParGridFunction(fes_H1);
   *grid2DmodalImEz=0.0;

   // H

   grid2DmodalReHt=new ParGridFunction(fes_ND);
   *grid2DmodalReHt=0.0;

   grid2DmodalImHt=new ParGridFunction(fes_ND);
   *grid2DmodalImHt=0.0;

   grid2DmodalReHz=new ParGridFunction(fes_H1);
   *grid2DmodalReHz=0.0;

   grid2DmodalImHz=new ParGridFunction(fes_H1);
   *grid2DmodalImHz=0.0;
}

void FieldSet::fillIntegrationPoints (vector<Path *> *pathList, vector<long unsigned int> *pathIndexList,
                                      vector<OPEMIntegrationPointList *> *pointsList, vector<bool> *reverseList)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   double x1,y1,z1,x2,y2,z2;

   int pointsCount=100;     // 100, number of points along each line segment for integration

   // loop through the paths
   long unsigned int iPath=0;
   while (iPath < pathIndexList->size()) {

      Path *path=(*pathList)[(*pathIndexList)[iPath]];

      // loop through the path segments

      long unsigned int limit=path->get_points_size();
      if (path->is_closed()) limit++;

      struct point p2=path->get_point_value(0);
      x2=p2.x;
      y2=p2.y;
      z2=p2.z;

      long unsigned int j=1;
      while (j < limit) {

         x1=x2;
         y1=y2;
         z1=z2;

         if (path->is_closed()) {
            if (j < limit-1) {
               struct point p2=path->get_point_value(j);
               x2=p2.x;
               y2=p2.y;
               z2=p2.z;
            } else {
               struct point p2=path->get_point_value(0);
               x2=p2.x;
               y2=p2.y;
               z2=p2.z;
            }
         } else {
            struct point p2=path->get_point_value(j);
            x2=p2.x;
            y2=p2.y;
            z2=p2.z;
         }

         // get points along the integration line
         OPEMIntegrationPointList *points=new OPEMIntegrationPointList();
         points->set_reverse((*reverseList)[iPath]);
         int i=0;
         while (i < pointsCount) {
            OPEMIntegrationPoint *point=new OPEMIntegrationPoint(i,x1+i*(x2-x1)/(pointsCount-1),
                                                                   y1+i*(y2-y1)/(pointsCount-1),
                                                                   z1+i*(z2-z1)/(pointsCount-1));
            points->push(point);
            i++;
         }

         pointsList->push_back(points);

         j++;
      }

      iPath++;
   }
}

void FieldSet::transfer_2Dsolution_2Dgrids_to_3Dgrids ()
{
   // E

   ParTransferMap *port_to_full_ReEt=new ParTransferMap(*grid2DReEt,*grid3DReEt);
   port_to_full_ReEt->Transfer(*grid2DReEt,*grid3DReEt);
   delete port_to_full_ReEt; port_to_full_ReEt=nullptr;

   ParTransferMap *port_to_full_ImEt=new ParTransferMap(*grid2DImEt,*grid3DImEt);
   port_to_full_ImEt->Transfer(*grid2DImEt,*grid3DImEt);
   delete port_to_full_ImEt; port_to_full_ImEt=nullptr;

   ParTransferMap *port_to_full_ReEz=new ParTransferMap(*grid2DReEz,*grid3DReEz);
   port_to_full_ReEz->Transfer(*grid2DReEz,*grid3DReEz);
   delete port_to_full_ReEz; port_to_full_ReEz=nullptr;

   ParTransferMap *port_to_full_ImEz=new ParTransferMap(*grid2DImEz,*grid3DImEz);
   port_to_full_ImEz->Transfer(*grid2DImEz,*grid3DImEz);
   delete port_to_full_ImEz; port_to_full_ImEz=nullptr;

   // H  

   ParTransferMap *port_to_full_ReHt=new ParTransferMap(*grid2DReHt,*grid3DReHt);
   port_to_full_ReHt->Transfer(*grid2DReHt,*grid3DReHt);
   delete port_to_full_ReHt; port_to_full_ReHt=nullptr;

   ParTransferMap *port_to_full_ImHt=new ParTransferMap(*grid2DImHt,*grid3DImHt);
   port_to_full_ImHt->Transfer(*grid2DImHt,*grid3DImHt);
   delete port_to_full_ImHt; port_to_full_ImHt=nullptr;

   ParTransferMap *port_to_full_ReHz=new ParTransferMap(*grid2DReHz,*grid3DReHz);
   port_to_full_ReHz->Transfer(*grid2DReHz,*grid3DReHz);
   delete port_to_full_ReHz; port_to_full_ReHz=nullptr;

   ParTransferMap *port_to_full_ImHz=new ParTransferMap(*grid2DImHz,*grid3DImHz);
   port_to_full_ImHz->Transfer(*grid2DImHz,*grid3DImHz);
   delete port_to_full_ImHz; port_to_full_ImHz=nullptr;
}

// transfer from 3D back to 2D to capture the orientation operations applied by MFEM
void FieldSet::transfer_2Dsolution_3Dgrids_to_2Dgrids ()
{
   // E

   ParTransferMap *port_to_full_ReEt=new ParTransferMap(*grid3DReEt,*grid2DmodalReEt);
   port_to_full_ReEt->Transfer(*grid3DReEt,*grid2DmodalReEt);
   delete port_to_full_ReEt; port_to_full_ReEt=nullptr;

   ParTransferMap *port_to_full_ImEt=new ParTransferMap(*grid3DImEt,*grid2DmodalImEt);
   port_to_full_ImEt->Transfer(*grid3DImEt,*grid2DmodalImEt);
   delete port_to_full_ImEt; port_to_full_ImEt=nullptr;

   ParTransferMap *port_to_full_ReEz=new ParTransferMap(*grid3DReEz,*grid2DmodalReEz);
   port_to_full_ReEz->Transfer(*grid3DReEz,*grid2DmodalReEz);
   delete port_to_full_ReEz; port_to_full_ReEz=nullptr;

   ParTransferMap *port_to_full_ImEz=new ParTransferMap(*grid3DImEz,*grid2DmodalImEz);
   port_to_full_ImEz->Transfer(*grid3DImEz,*grid2DmodalImEz);
   delete port_to_full_ImEz; port_to_full_ImEz=nullptr;

   // H

   ParTransferMap *port_to_full_ReHt=new ParTransferMap(*grid3DReHt,*grid2DmodalReHt);
   port_to_full_ReHt->Transfer(*grid3DReHt,*grid2DmodalReHt);
   delete port_to_full_ReHt; port_to_full_ReHt=nullptr;

   ParTransferMap *port_to_full_ImHt=new ParTransferMap(*grid3DImHt,*grid2DmodalImHt);
   port_to_full_ImHt->Transfer(*grid3DImHt,*grid2DmodalImHt);
   delete port_to_full_ImHt; port_to_full_ImHt=nullptr;

   ParTransferMap *port_to_full_ReHz=new ParTransferMap(*grid3DReHz,*grid2DmodalReHz);
   port_to_full_ReHz->Transfer(*grid3DReHz,*grid2DmodalReHz);
   delete port_to_full_ReHz; port_to_full_ReHz=nullptr;

   ParTransferMap *port_to_full_ImHz=new ParTransferMap(*grid3DImHz,*grid2DmodalImHz);
   port_to_full_ImHz->Transfer(*grid3DImHz,*grid2DmodalImHz);
   delete port_to_full_ImHz; port_to_full_ImHz=nullptr;
}

void FieldSet::save2DParaView(ParSubMesh *psubmesh2D, struct projectData *projData, double frequency, bool add_extension, int Sport)
{
   if (!projData->debug_save_port_fields) return;

   stringstream ss;
   ss << projData->project_name << "_frequency_" << frequency << "_Sport_" << Sport;

   stringstream ssParaView;
   ssParaView << "ParaView_2D_port_" << projData->project_name;
   if (add_extension) ssParaView << "_test";

   ParaViewDataCollection *pd=nullptr;
   pd=new ParaViewDataCollection(ss.str(),psubmesh2D);
   pd->SetOwnData(false);
   pd->SetPrefixPath(ssParaView.str());
   pd->RegisterField("grid2DReEt",grid2DReEt);
   pd->RegisterField("grid2DReEz",grid2DReEz);
   pd->RegisterField("grid2DImEt",grid2DImEt);
   pd->RegisterField("grid2DImEz",grid2DImEz);
   pd->RegisterField("grid2DReHt",grid2DReHt);
   pd->RegisterField("grid2DReHz",grid2DReHz);
   pd->RegisterField("grid2DImHt",grid2DImHt);
   pd->RegisterField("grid2DImHz",grid2DImHz);
   pd->SetLevelsOfDetail(3);
   pd->SetDataFormat(VTKFormat::ASCII);
   pd->SetHighOrderOutput(true);
   pd->Save();
   delete pd;
}

void FieldSet::save3DParaView(ParMesh *pmesh, struct projectData *projData, double frequency, bool add_extension, int Sport)
{
   if (!projData->debug_save_port_fields) return;

   stringstream ss;
   ss << projData->project_name << "_frequency_" << frequency << "_Sport_" << Sport;

   stringstream ssParaView;
   ssParaView << "ParaView_3D_port_" << projData->project_name;
   if (add_extension) ssParaView << "_test";

   ParaViewDataCollection *pd=nullptr;
   pd=new ParaViewDataCollection(ss.str(),pmesh);
   pd->SetOwnData(false);
   pd->SetPrefixPath(ssParaView.str());
   pd->RegisterField("grid3DReEt",grid3DReEt);
   pd->RegisterField("grid3DImEt",grid3DImEt);
   pd->RegisterField("grid3DReEz",grid3DReEz);
   pd->RegisterField("grid3DImEz",grid3DImEz);
   pd->RegisterField("grid3DReHt",grid3DReHt);
   pd->RegisterField("grid3DImHt",grid3DImHt);
   pd->RegisterField("grid3DReHz",grid3DReHz);
   pd->RegisterField("grid3DImHz",grid3DImHz);
   pd->SetLevelsOfDetail(3);
   pd->SetDataFormat(VTKFormat::ASCII);
   pd->SetHighOrderOutput(true);
   pd->Save();
   delete pd;
}

void FieldSet::save2DModalParaView (ParSubMesh *psubmesh2D, struct projectData *projData, double frequency, bool add_extension, int Sport)
{
   if (!projData->debug_save_port_fields) return;

   stringstream ss;
   ss << projData->project_name << "_frequency_" << frequency << "_Sport_" << Sport;

   stringstream ssParaView;
   ssParaView << "ParaView_modal_2D_" << projData->project_name;
   if (add_extension) ssParaView << "_test";

   ParaViewDataCollection *pd=nullptr;
   pd=new ParaViewDataCollection(ss.str(),psubmesh2D);
   pd->SetOwnData(false);
   pd->SetPrefixPath(ssParaView.str());
   pd->RegisterField("grid2DmodalReEt",grid2DmodalReEt);
   pd->RegisterField("grid2DmodalImEt",grid2DmodalImEt);
   pd->RegisterField("grid2DmodalReEz",grid2DmodalReEz);
   pd->RegisterField("grid2DmodalImEz",grid2DmodalImEz);
   pd->RegisterField("grid2DmodalReHt",grid2DmodalReHt);
   pd->RegisterField("grid2DmodalImHt",grid2DmodalImHt);
   pd->RegisterField("grid2DmodalReHz",grid2DmodalReHz);
   pd->RegisterField("grid2DmodalImHz",grid2DmodalImHz);
   pd->SetLevelsOfDetail(3);
   pd->SetDataFormat(VTKFormat::ASCII);
   pd->SetHighOrderOutput(true);
   pd->Save();
   delete pd;
}

void FieldSet::populateGamma (double frequency, GammaDatabase *gammaDatabase, int modeNumber2D, int Sport)
{
   Gamma *gamma=new Gamma();
   gamma->set(Sport,modeNumber2D,alpha,beta,frequency);
   gammaDatabase->push(gamma);
}

void flipSign (ParGridFunction *a)
{
   HypreParVector *hpv=a->GetTrueDofs();
   (*hpv)*=-1;
   a->Distribute(hpv);
   delete hpv;
}

void FieldSet::flip2DmodalSign ()
{
   flipSign(grid2DReEt);
   flipSign(grid2DImEt);
   flipSign(grid2DReEz);
   flipSign(grid2DImEz);
   flipSign(grid2DReHt);
   flipSign(grid2DImHt);
   flipSign(grid2DReHz);
   flipSign(grid2DImHz);

   flipSign(grid3DReEt);
   flipSign(grid3DImEt);
   flipSign(grid3DReEz);
   flipSign(grid3DImEz);
   flipSign(grid3DReHt);
   flipSign(grid3DImHt);
   flipSign(grid3DReHz);
   flipSign(grid3DImHz);

   flipSign(grid2DmodalReEt);
   flipSign(grid2DmodalImEt);
   flipSign(grid2DmodalReEz);
   flipSign(grid2DmodalImEz);
   flipSign(grid2DmodalReHt);
   flipSign(grid2DmodalImHt);
   flipSign(grid2DmodalReHz);
   flipSign(grid2DmodalImHz);

}

void FieldSet::reset()
{
   if (eVecReE) {free(eVecReE); eVecReE=nullptr;}
   if (eVecImE) {free(eVecImE); eVecImE=nullptr;}
   if (eVecReH) {free(eVecReH); eVecReH=nullptr;}
   if (eVecImH) {free(eVecImH); eVecImH=nullptr;}

   if (eigenVecReEt) {delete eigenVecReEt; eigenVecReEt=nullptr;}
   if (eigenVecImEt) {delete eigenVecImEt; eigenVecImEt=nullptr;}
   if (eigenVecReEz) {delete eigenVecReEz; eigenVecReEz=nullptr;}
   if (eigenVecImEz) {delete eigenVecImEz; eigenVecImEz=nullptr;}
   if (eigenVecReHt) {delete eigenVecReHt; eigenVecReHt=nullptr;}
   if (eigenVecImHt) {delete eigenVecImHt; eigenVecImHt=nullptr;}
   if (eigenVecReHz) {delete eigenVecReHz; eigenVecReHz=nullptr;}
   if (eigenVecImHz) {delete eigenVecImHz; eigenVecImHz=nullptr;}

   if (grid2DReEt) {delete grid2DReEt; grid2DReEt=nullptr;}
   if (grid2DImEt) {delete grid2DImEt; grid2DImEt=nullptr;}
   if (grid2DReEz) {delete grid2DReEz; grid2DReEz=nullptr;}
   if (grid2DImEz) {delete grid2DImEz; grid2DImEz=nullptr;}
   if (grid2DReHt) {delete grid2DReHt; grid2DReHt=nullptr;}
   if (grid2DImHt) {delete grid2DImHt; grid2DImHt=nullptr;}
   if (grid2DReHz) {delete grid2DReHz; grid2DReHz=nullptr;}
   if (grid2DImHz) {delete grid2DImHz; grid2DImHz=nullptr;}

   if (grid3DReEt) {delete grid3DReEt; grid3DReEt=nullptr;}
   if (grid3DImEt) {delete grid3DImEt; grid3DImEt=nullptr;}
   if (grid3DReEz) {delete grid3DReEz; grid3DReEz=nullptr;}
   if (grid3DImEz) {delete grid3DImEz; grid3DImEz=nullptr;}
   if (grid3DReHt) {delete grid3DReHt; grid3DReHt=nullptr;}
   if (grid3DImHt) {delete grid3DImHt; grid3DImHt=nullptr;}
   if (grid3DReHz) {delete grid3DReHz; grid3DReHz=nullptr;}
   if (grid3DImHz) {delete grid3DImHz; grid3DImHz=nullptr;}

   if (grid2DmodalReEt) {delete grid2DmodalReEt; grid2DmodalReEt=nullptr;}
   if (grid2DmodalImEt) {delete grid2DmodalImEt; grid2DmodalImEt=nullptr;}
   if (grid2DmodalReEz) {delete grid2DmodalReEz; grid2DmodalReEz=nullptr;}
   if (grid2DmodalImEz) {delete grid2DmodalImEz; grid2DmodalImEz=nullptr;}
   if (grid2DmodalReHt) {delete grid2DmodalReHt; grid2DmodalReHt=nullptr;}
   if (grid2DmodalImHt) {delete grid2DmodalImHt; grid2DmodalImHt=nullptr;}
   if (grid2DmodalReHz) {delete grid2DmodalReHz; grid2DmodalReHz=nullptr;}
   if (grid2DmodalImHz) {delete grid2DmodalImHz; grid2DmodalImHz=nullptr;}
}

FieldSet::~FieldSet()
{
   if (eVecReE) {free(eVecReE); eVecReE=nullptr;}
   if (eVecImE) {free(eVecImE); eVecImE=nullptr;}
   if (eVecReH) {free(eVecReH); eVecReH=nullptr;}
   if (eVecImH) {free(eVecImH); eVecImH=nullptr;}

   if (eigenVecReEt) {delete eigenVecReEt; eigenVecReEt=nullptr;}
   if (eigenVecImEt) {delete eigenVecImEt; eigenVecImEt=nullptr;}
   if (eigenVecReEz) {delete eigenVecReEz; eigenVecReEz=nullptr;}
   if (eigenVecImEz) {delete eigenVecImEz; eigenVecImEz=nullptr;}
   if (eigenVecReHt) {delete eigenVecReHt; eigenVecReHt=nullptr;}
   if (eigenVecImHt) {delete eigenVecImHt; eigenVecImHt=nullptr;}
   if (eigenVecReHz) {delete eigenVecReHz; eigenVecReHz=nullptr;}
   if (eigenVecImHz) {delete eigenVecImHz; eigenVecImHz=nullptr;}

   if (grid2DReEt) {delete grid2DReEt; grid2DReEt=nullptr;}
   if (grid2DImEt) {delete grid2DImEt; grid2DImEt=nullptr;}
   if (grid2DReEz) {delete grid2DReEz; grid2DReEz=nullptr;}
   if (grid2DImEz) {delete grid2DImEz; grid2DImEz=nullptr;}
   if (grid2DReHt) {delete grid2DReHt; grid2DReHt=nullptr;}
   if (grid2DImHt) {delete grid2DImHt; grid2DImHt=nullptr;}
   if (grid2DReHz) {delete grid2DReHz; grid2DReHz=nullptr;}
   if (grid2DImHz) {delete grid2DImHz; grid2DImHz=nullptr;}

   if (grid3DReEt) {delete grid3DReEt; grid3DReEt=nullptr;}
   if (grid3DImEt) {delete grid3DImEt; grid3DImEt=nullptr;}
   if (grid3DReEz) {delete grid3DReEz; grid3DReEz=nullptr;}
   if (grid3DImEz) {delete grid3DImEz; grid3DImEz=nullptr;}
   if (grid3DReHt) {delete grid3DReHt; grid3DReHt=nullptr;}
   if (grid3DImHt) {delete grid3DImHt; grid3DImHt=nullptr;}
   if (grid3DReHz) {delete grid3DReHz; grid3DReHz=nullptr;}
   if (grid3DImHz) {delete grid3DImHz; grid3DImHz=nullptr;}

   if (grid2DmodalReEt) {delete grid2DmodalReEt; grid2DmodalReEt=nullptr;}
   if (grid2DmodalImEt) {delete grid2DmodalImEt; grid2DmodalImEt=nullptr;}
   if (grid2DmodalReEz) {delete grid2DmodalReEz; grid2DmodalReEz=nullptr;}
   if (grid2DmodalImEz) {delete grid2DmodalImEz; grid2DmodalImEz=nullptr;}
   if (grid2DmodalReHt) {delete grid2DmodalReHt; grid2DmodalReHt=nullptr;}
   if (grid2DmodalImHt) {delete grid2DmodalImHt; grid2DmodalImHt=nullptr;}
   if (grid2DmodalReHz) {delete grid2DmodalReHz; grid2DmodalReHz=nullptr;}
   if (grid2DmodalImHz) {delete grid2DmodalImHz; grid2DmodalImHz=nullptr;}
}

///////////////////////////////////////////////////////////////////////////////////////////
// Mode
///////////////////////////////////////////////////////////////////////////////////////////

Mode::Mode(int startLine_, int endLine_, std::string calculation_)
{
   startLine=startLine_;
   endLine=endLine_;

   // mode
   Sport.push_alias("Sport");
   Sport.set_loaded(false);
   Sport.set_positive_required(true);
   Sport.set_non_negative_required(false);
   Sport.set_lowerLimit(1);
   Sport.set_upperLimit(100);
   Sport.set_checkLimits(true);

   // net
   net.push_alias("net");
   net.set_loaded(false);
   net.set_positive_required(false);
   net.set_non_negative_required(false);
   net.set_lowerLimit(0);
   net.set_upperLimit(0);
   net.set_checkLimits(false);

   // defaults

   calculation=calculation_;
   modified=false;
#ifdef HAS_GUI
   item=nullptr;
#endif
}

bool Mode::is_modified ()
{
    if (modified) return true;

    long unsigned int i=0;
    while (i < integrationPathList.size()) {
        if (integrationPathList[i]->is_modified()) return true;
        i++;
    }

    return false;
}

bool Mode::inIntegrationPathBlocks (int lineNumber)
{
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->inIntegrationPathBlock(lineNumber)) return true;
      i++;
   }
   return false;
}

bool Mode::findIntegrationPathBlocks(inputFile *inputs)
{
   bool fail=false;
   int start_lineNumber=startLine;
   int stop_lineNumber=endLine;
   int block_start,block_stop;

   while (start_lineNumber < stop_lineNumber) {
      if (inputs->findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "IntegrationPath", "EndIntegrationPath", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            IntegrationPath *newIntegrationPath=new IntegrationPath(block_start,block_stop);
            integrationPathList.push_back(newIntegrationPath);
         }
      }
      start_lineNumber=inputs->get_next_lineNumber(block_stop);
   }
   return fail;
}

//xxx
IntegrationPath* Mode::addIntegrationPath (vector<Path *> *pathList, vector<Path *> *pathsToAdd, string type)
{
    IntegrationPath *integrationPath=new IntegrationPath(pathList,pathsToAdd,type);
    integrationPathList.push_back(integrationPath);
    modified=true;
    return integrationPath;
}

bool Mode::load(string *indent, inputFile *inputs)
{
   bool fail=false;

   // blocks

   if (findIntegrationPathBlocks(inputs)) fail=true;

   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->load(indent, inputs)) fail=true;
      i++;
   }

   // keywords

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {

      if (!inIntegrationPathBlocks(lineNumber)) {

         string token,value,line;
         line=inputs->get_line(lineNumber);
         get_token_pair(&line,&token,&value,&lineNumber,*indent);

         int recognized=0;

         if (Sport.match_alias(&token)) {
            recognized++;
            if (Sport.loadInt(&token, &value, lineNumber)) fail=true;
         }

         if (net.match_alias(&token)) {
            if (net.is_loaded()) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3006: Duplicate entry at line %d for previous entry at line %d.\n",
                                                      indent->c_str(),indent->c_str(),lineNumber,net.get_lineNumber());
               fail=true;
            } else {
               net.set_keyword(token);
               net.set_value(value);
               net.set_lineNumber(lineNumber);
               net.set_loaded(true);
            }
            recognized++;
         }

         // should recognize one keyword
         if (recognized != 1) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3174: Unrecognized keyword at line %d.\n",
                                                   indent->c_str(),indent->c_str(),lineNumber);
            fail=true;
         }
      }
      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }

   return fail;
}

bool Mode::inModeBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

bool Mode::check (std::string *indent, std::vector<Path *> *pathList, bool is_modal, long unsigned int modeCount)
{
   bool fail=false;

   // Sport
   if (!Sport.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3037: Mode block at line %d must specify an Sport number.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   // net is optional

   // integration paths

   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->check(indent,pathList)) fail=true;
      i++;
   }

   if (fail) return fail;

   bool foundVoltage=false;
   bool foundCurrent=false;
   i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->is_voltage()) {
         if (foundVoltage) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3152: IntegrationPath block at line %d incorrectly specifies an additional voltage path.\n",
                                                   indent->c_str(),indent->c_str(),integrationPathList[i]->get_startLine());
            fail=true;
         } else  foundVoltage=true;
      }
      if (integrationPathList[i]->is_current()) {
         if (foundCurrent) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3142: IntegrationPath block at line %d incorrectly specifies an additional current path.\n",
                                                   indent->c_str(),indent->c_str(),integrationPathList[i]->get_startLine());
            fail=true;
         } else  foundCurrent=true;
      } 
      i++;
   }

   // line calculation requires voltage and current if there is more than one mode
   // The line calculation applies an algorithm using both to extract the voltages and currents for the impedance.
   // For mode calculations, it is assumed that the user has the correct setup, so both are not needed.
   if (is_modal) {
      if (!foundVoltage && !foundCurrent) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3138: Mode block at line %d must specify a voltage or current integration path.\n",
                                                indent->c_str(),indent->c_str(),startLine);
         fail=true;
      }
   } else {
      if (modeCount > 1 && !(foundVoltage && foundCurrent)) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3130: Mode block at line %d must specify both voltage and current integration paths.\n",
                                                indent->c_str(),indent->c_str(),startLine);
         fail=true;
      }
   }

   return fail;
}

bool Mode::checkBoundingBox (Vector *lowerLeft, Vector *upperRight, string *indent, double tol, vector<Path *> *pathList)
{
   bool fail=false;

   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->checkBoundingBox(lowerLeft,upperRight,indent,tol,pathList)) fail=true;
      i++;
   }

   return fail;
}

bool Mode::has_voltage ()
{
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->get_type().compare("voltage") == 0) return true;
      i++;
   }

   return false;
}

bool Mode::has_current ()
{
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->get_type().compare("current") == 0) return true;
      i++;
   }

   return false;
}

bool Mode::align_current_paths (string *indent, vector<Path *> *pathList, bool check_closed_loop)
{
   bool fail=false;

   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->is_current()) {
         double path_area=0;
         fail=integrationPathList[i]->align(indent,pathList,&path_area,check_closed_loop);

         // should not occur
         if (path_area < -1e-14) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3103: Mode block at line %d has a path defined in the clockwise direction.\n",
                                                   indent->c_str(),indent->c_str(),startLine);
            fail=true;
         }
      }

      i++;
   }

   return fail;
}

void Mode::print (string indent)
{
   if (is_modal()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sMode %p\n",indent.c_str(),this);}
   if (is_line()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sLine %p\n",indent.c_str(),this);}

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   Sport=%d\n",indent.c_str(),get_Sport());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   net=%s\n",indent.c_str(),get_net().c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   calculation=%s\n",indent.c_str(),calculation.c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   modeNumber2D=%d\n",indent.c_str(),modeNumber2D);

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   weight: ",indent.c_str());
   long unsigned int i=0;
   while (i < weight.size()) {
      PetscPrintf(PETSC_COMM_WORLD,"(%g,%g),",real(weight[i]),imag(weight[i]));
      i++;
   }
   PetscPrintf(PETSC_COMM_WORLD,"\n");

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   Cp: ",indent.c_str());
   i=0;
   while (i < Cp.size()) {
      PetscPrintf(PETSC_COMM_WORLD,"(%g,%g),",real(Cp[i]),imag(Cp[i]));
      i++;
   }
   PetscPrintf(PETSC_COMM_WORLD,"\n");

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   Cm: ",indent.c_str());
   i=0;
   while (i < Cm.size()) {
      PetscPrintf(PETSC_COMM_WORLD,"(%g,%g),",real(Cm[i]),imag(Cm[i]));
      i++;
   }
   PetscPrintf(PETSC_COMM_WORLD,"\n");

   if (is_modal()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sEndMode\n",indent.c_str());}
   if (is_line()) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sEndLine\n",indent.c_str());}

   return;
}

void Mode::save (std::ofstream *out)
{
    PetscMPIInt rank;
    MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

    modified=false;
    if (rank != 0) return;

    if (is_modal()) {*out << "   Mode" << endl;}
    if (is_line()) {*out << "   Line" << endl;}

    *out << "      Sport=" << get_Sport() << endl;
    if (net_is_loaded()) {*out << "      net=" << get_net() << endl;}

    long unsigned int i=0;
    while (i < integrationPathList.size()) {
        integrationPathList[i]->save(out);
        i++;
    }

    if (is_modal()) {*out << "   EndMode" << endl;}
    if (is_line()) {*out << "   EndLine" << endl;}
}

void printMatlabComplex(double re, double im)
{
   cout << setprecision(16) << scientific;
   cout << re;
   if (im >= 0) cout << " + ";
   else cout << " - ";
   cout << abs(im);
   cout << "i";
   cout << endl;
}

bool Mode::assignPathIndices(vector<Path *> *pathList)
{
   bool fail=false;

   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->assignPathIndices(pathList)) {fail=true; break;}
      i++;
   }

   return fail;
}

bool Mode::is_enclosedByPath (vector<Path *> *pathList, Path *testPath, long unsigned int *index)
{
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      *index=i;
      if (!integrationPathList[i]->is_enclosedByPath(pathList,testPath)) return false;
      i++;
   }
   return true;
}

void Mode::output (ofstream *out, vector<Path *> *pathList, Path *rotatedPath, bool spin180degrees)
{
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      integrationPathList[i]->output(out,pathList,rotatedPath,spin180degrees,is_modal(),modeNumber2D);
      i++;
   }
}

bool Mode::loadSolution (string *directory, string portName, size_t t_size, size_t z_size)
{
   return fields.loadSolution(directory,portName,t_size,z_size,modeNumber2D);
}

bool Mode::scaleSolution ()
{
   return fields.scaleSolution();
}

// X and Xdofs are partitioned on A
// grid3DReEt and grid3DImEt are partitioned on fespace_ND
// ess_tdof_port_list is partitioned on fespace_ND
void Mode::fillX (Vec *X, Vec *Xdofs, Array<int> *ess_tdof_port_list, HYPRE_BigInt *offset_port, int drivingSet)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // check for inclusion
   if (weight[drivingSet-1] == 0) return;

   // local tdof data
   HypreParVector *hypreRe=fields.get_grid3DReEt()->GetTrueDofs();
   HypreParVector *hypreIm=fields.get_grid3DImEt()->GetTrueDofs();

   // global tdof data
   Vector *data_re=hypreRe->GlobalVector();
   Vector *data_im=hypreIm->GlobalVector();

   // ess_tdof global data
   int local_size=ess_tdof_port_list->Size();
   int global_size=data_re->Size();
   vector<int> global_ess(global_size);
   global_ess.assign(global_size,0);

   // collect ess marker data at zero
   if (rank == 0) {

      // local
      int i=0;
      while (i < local_size) {
         global_ess[offset_port[0]+(*ess_tdof_port_list)[i]]=1;
         i++;
      }

      // collected
      i=1;
      while (i < size) {
         int transfer_size=0;
         MPI_Recv(&transfer_size,1,MPI_INT,i,10000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         int *pack=(int *)malloc(transfer_size*sizeof(int));
         MPI_Recv(pack,transfer_size,MPI_INT,i,10001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         int k=0;
         while (k < transfer_size) {
            global_ess[pack[k]]=1;
            k++;
         }
         if (pack) free(pack);
         i++;
      }
   } else {
      MPI_Send(&local_size,1,MPI_INT,0,10000,PETSC_COMM_WORLD);

      int *pack=(int *)malloc(local_size*sizeof(int));
      int i=0; 
      while (i < local_size) {
         pack[i]=offset_port[0]+(*ess_tdof_port_list)[i];
         i++;
      }
      MPI_Send(pack,local_size,MPI_INT,0,10001,PETSC_COMM_WORLD);
      if (pack) free(pack);
   }

   // send global to all ranks

   if (rank == 0) {
      int *pack=(int *)malloc(global_size*sizeof(int));
      int i=0;
      while (i < global_size) {
         pack[i]=global_ess[i];
         i++;
      }

      i=1;
      while (i < size) {
         MPI_Send(pack,global_size,MPI_INT,i,20001,PETSC_COMM_WORLD);
         i++;
      }
      if (pack) free(pack);
   } else {
      int *pack=(int *)malloc(global_size*sizeof(int));
      MPI_Recv(pack,global_size,MPI_INT,0,20001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      int i=0;
      while (i < global_size) {
         global_ess[i]=pack[i];
         i++;
      }
      if (pack) free(pack);
   }

   // transfer to X
   PetscScalar scale=weight[drivingSet-1];
   PetscScalar value;
   PetscInt low,high;
   VecGetOwnershipRange(*X,&low,&high);
   int i=0;
   while (i < global_size) {
      if (global_ess[i] == 1 && i >= low && i < high) {
         value=(*data_re)[i]*scale+PETSC_i*(*data_im)[i]*scale;
         VecSetValue(*X,i,value,ADD_VALUES);
         VecSetValue(*Xdofs,i,1,ADD_VALUES);
      }
      i++;
   }

   VecAssemblyBegin(*X);
   VecAssemblyBegin(*Xdofs);
   VecAssemblyEnd(*X);
   VecAssemblyEnd(*Xdofs);

   delete data_re; data_re=nullptr;
   delete data_im; data_im=nullptr;

   delete hypreRe;
   delete hypreIm;
}

void Mode::build2Dgrids (ParFiniteElementSpace *fes_ND, ParFiniteElementSpace *fes_H1)
{
   fields.build2Dgrids(fes_ND,fes_H1);
}

void Mode::build3Dgrids (ParFiniteElementSpace *fes_ND, ParFiniteElementSpace *fes_H1)
{
   fields.build3Dgrids(fes_ND,fes_H1);
}

void Mode::build2DModalGrids (ParFiniteElementSpace *fes_ND, ParFiniteElementSpace *fes_H1)
{
   fields.build2DModalGrids(fes_ND,fes_H1);
}

void Mode::fillIntegrationPoints (vector<Path *> *pathList)
{
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      fields.fillIntegrationPoints(pathList,integrationPathList[i]->get_pathIndexList(),integrationPathList[i]->get_pointsList(),
                                            integrationPathList[i]->get_reverseList());
      i++;
   }
}

IntegrationPath* Mode::get_voltageIntegrationPath ()
{
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->is_voltage()) return integrationPathList[i];
      i++;
   }
   return nullptr;
}

IntegrationPath* Mode::get_currentIntegrationPath ()
{
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->is_current()) return integrationPathList[i];
      i++;
   }
   return nullptr;
}

// integrate using the paths attached to the Mode
void Mode::calculateLineIntegrals (ParMesh *pmesh, fem3D *fem)
{
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->is_voltage()) {
         integrationPathList[i]->calculateLineIntegral(pmesh,fields.get_grid3DReEt(),fields.get_grid3DImEt());
      }

      if (integrationPathList[i]->is_current()) {
         integrationPathList[i]->calculateLineIntegral(pmesh,fields.get_grid3DReHt(),fields.get_grid3DImHt());
      }

      i++;
   }
}

// integrate using a given path
void Mode::calculateLineIntegrals (ParMesh *pmesh, fem3D *fem, IntegrationPath *Vpath, IntegrationPath *Ipath)
{
   if (Vpath) Vpath->calculateLineIntegral(pmesh,fields.get_grid3DReEt(),fields.get_grid3DImEt());
   if (Ipath) Ipath->calculateLineIntegral(pmesh,fields.get_grid3DReHt(),fields.get_grid3DImHt());
}

// align the directions of the voltages or currents
void Mode::alignDirections (ParMesh *pmesh, fem3D *fem, IntegrationPath *Vpath, IntegrationPath *Ipath)
{
   // use the supplied Vpath and Ipath

   calculateLineIntegrals (pmesh,fem,Vpath,Ipath);

   // favor voltage over current

   if (Vpath) {
      if (abs(arg(Vpath->get_integratedValue())) > M_PI/2) {
         flip2DmodalSign();
      }
      return;
   }

   if (Ipath) {
      if (abs(arg(Ipath->get_integratedValue())) > M_PI/2) {
         flip2DmodalSign();
      }
      return;
   }

   // use the paths defined for the mode

   bool hasVoltage=false;
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->is_voltage()) hasVoltage=true;
      i++;
   }

   i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]->is_voltage()) {
         if (abs(arg(integrationPathList[i]->get_integratedValue())) > M_PI/2) {
            flip2DmodalSign();
            return;
         }
      }

      if (!hasVoltage && integrationPathList[i]->is_current()) {
         if (abs(arg(integrationPathList[i]->get_integratedValue())) > M_PI/2) {
            flip2DmodalSign();
         }
      }
      i++;
   }
}

// use n x tangential field
complex<double> tangentialInnerProduct(VectorCoefficient *ReA, VectorCoefficient *ImA,
                                       VectorCoefficient *ReB, VectorCoefficient *ImB,
                                       ParGridFunction grid_fes2D_L2,
                                       ParLinearForm *lf, VectorConstantCoefficient *normal)
{
   VectorCrossProductCoefficient ReAt(*normal,*ReA);
   VectorCrossProductCoefficient ImAt(*normal,*ImA);

   VectorCrossProductCoefficient ReBt(*normal,*ReB);
   VectorCrossProductCoefficient ImBt(*normal,*ImB);

   InnerProductCoefficient ReRe(ReAt,ReBt);
   InnerProductCoefficient ReIm(ReAt,ImBt);
   InnerProductCoefficient ImRe(ImAt,ReBt);
   InnerProductCoefficient ImIm(ImAt,ImBt);

   SumCoefficient Re(ReRe,ImIm,1,+1);
   SumCoefficient Im(ReIm,ImRe,1,-1);

   grid_fes2D_L2.ProjectCoefficient(Re);
   double r=(*lf)(grid_fes2D_L2);

   grid_fes2D_L2.ProjectCoefficient(Im);
   double i=(*lf)(grid_fes2D_L2);

   return complex<double>(r,i);
}

complex<double> normalProduct(Coefficient *ReA, Coefficient *ImA,
                              Coefficient *ReB, Coefficient *ImB,
                              ParGridFunction grid_fes2D_L2,
                              ParLinearForm *lf)
{
   ProductCoefficient ReRe(*ReA,*ReB);
   ProductCoefficient ReIm(*ReA,*ImB);
   ProductCoefficient ImRe(*ImA,*ReB);
   ProductCoefficient ImIm(*ImA,*ImB);

   SumCoefficient Re(ReRe,ImIm,1,+1);
   SumCoefficient Im(ReIm,ImRe,1,-1);

   grid_fes2D_L2.ProjectCoefficient(Re);
   double r=(*lf)(grid_fes2D_L2);

   grid_fes2D_L2.ProjectCoefficient(Im);
   double i=(*lf)(grid_fes2D_L2);

   return complex<double>(r,i);
}

complex<double> normalProduct(Coefficient *ReA, Coefficient *ImA,
                              VectorCoefficient *ReB, VectorCoefficient *ImB,
                              ParGridFunction grid_fes2D_L2,
                              ParLinearForm *lf, VectorConstantCoefficient *normal)
{
   InnerProductCoefficient ReBn(*normal,*ReB);
   InnerProductCoefficient ImBn(*normal,*ImB);

   return normalProduct (ReA,ImA,&ReBn,&ImBn,grid_fes2D_L2,lf);
}

void Mode::calculateSplits (ParFiniteElementSpace *fes2D_L2,
                            ParGridFunction *grid2DsolutionReEt, ParGridFunction *grid2DsolutionImEt,
                            ParGridFunction *grid2DsolutionReEz, ParGridFunction *grid2DsolutionImEz,
                            ParGridFunction *grid2DsolutionReHt, ParGridFunction *grid2DsolutionImHt,
                            ParGridFunction *grid2DsolutionReHz, ParGridFunction *grid2DsolutionImHz,
                            Vector normal)
{
   VectorConstantCoefficient vccNormal(normal);

   ParGridFunction fes2D_L2_grid=ParGridFunction(fes2D_L2);

   ConstantCoefficient one(1.0);
   ParLinearForm lf(fes2D_L2);
   lf.AddDomainIntegrator(new DomainLFIntegrator(one));
   lf.Assemble();

   // mode

   VectorGridFunctionCoefficient ReEmt(fields.get_grid2DmodalReEt());
   VectorGridFunctionCoefficient ImEmt(fields.get_grid2DmodalImEt());

   VectorGridFunctionCoefficient ReHmt(fields.get_grid2DmodalReHt());
   VectorGridFunctionCoefficient ImHmt(fields.get_grid2DmodalImHt());

   // solution

   VectorGridFunctionCoefficient ReEst(grid2DsolutionReEt);
   VectorGridFunctionCoefficient ImEst(grid2DsolutionImEt);

   VectorGridFunctionCoefficient ReHst(grid2DsolutionReHt);
   VectorGridFunctionCoefficient ImHst(grid2DsolutionImHt);

   // splits

   complex<double> e0=tangentialInnerProduct(&ReEmt,&ImEmt,&ReEst,&ImEst,fes2D_L2_grid,&lf,&vccNormal);
   complex<double> e2=tangentialInnerProduct(&ReEmt,&ImEmt,&ReEmt,&ImEmt,fes2D_L2_grid,&lf,&vccNormal);

   complex<double> h0=tangentialInnerProduct(&ReHmt,&ImHmt,&ReHst,&ImHst,fes2D_L2_grid,&lf,&vccNormal);
   complex<double> h2=tangentialInnerProduct(&ReHmt,&ImHmt,&ReHmt,&ImHmt,fes2D_L2_grid,&lf,&vccNormal);

   Cp.push_back(0.5*(e0/e2+h0/h2));
   Cm.push_back(0.5*(e0/e2-h0/h2)); 
}

// Supports only driving set single (so weights are not supported)
// ToDo: generalize for other driving sets
complex<double> Mode::calculatePowerIn (int Sport_)
{
   return Cm[Sport_-1]*conj(Cm[Sport_-1])*get_Pz();
}

complex<double> Mode::calculatePowerOut (int Sport_)
{
   return Cp[Sport_-1]*conj(Cp[Sport_-1])*get_Pz();
}

void Mode::transfer_2Dsolution_2Dgrids_to_3Dgrids ()
{
   fields.transfer_2Dsolution_2Dgrids_to_3Dgrids();
}

// transfer from 3D back to 2D to capture the orientation operations applied by MFEM
void Mode::transfer_2Dsolution_3Dgrids_to_2Dgrids ()
{
   fields.transfer_2Dsolution_3Dgrids_to_2Dgrids();
}

void Mode::save2DParaView (ParSubMesh *psubmesh2D, struct projectData *projData, double frequency, bool add_extension)
{
   fields.save2DParaView(psubmesh2D,projData,frequency,add_extension,get_Sport());
}

void Mode::save3DParaView (ParMesh *pmesh, struct projectData *projData, double frequency, bool add_extension)
{
   fields.save3DParaView(pmesh,projData,frequency,add_extension,get_Sport());
}

void Mode::save2DModalParaView (ParSubMesh *psubmesh2D, struct projectData *projData, double frequency, bool add_extension)
{
   fields.save2DModalParaView(psubmesh2D,projData,frequency,add_extension,get_Sport());
}

void Mode::resetElementNumbers ()
{
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      integrationPathList[i]->resetElementNumbers();
      i++;
   }
}

// snap the path to the mesh boundary, where possible
void Mode::snapToMeshBoundary (vector<Path *> *pathList, Mesh *mesh)
{
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      integrationPathList[i]->snapToMeshBoundary(pathList,mesh);
      i++;
   }
}

void Mode::populateGamma (double frequency, GammaDatabase *gammaDatabase)
{
   fields.populateGamma(frequency,gammaDatabase,modeNumber2D,get_Sport());
}

void Mode::reset()
{
   fields.reset();
   Cp.clear();
   Cm.clear();
   weight.clear();
}

void Mode::assignPathNormals (struct point normal, std::vector<Path *> *pathList)
{
    long unsigned int i=0;
    while (i < integrationPathList.size()) {
       integrationPathList[i]->assignPathNormals(normal,pathList);
       i++;
    }
}

#ifdef HAS_GUI

void Mode::draw (Relay *relay, BoundaryDatabase *boundaryDatabase, CustomOpenGLWidget *drawingWindow,
                 QTreeWidget *drawingItemTree, CustomTreeWidgetItem *pathItemTree, CustomTreeWidgetItem *itemName)
{
    // net

    QString netname;
    if (net_is_loaded()) netname=QString::fromStdString(get_net());
    else {
        netname="net";
        netname.append(QString::number(get_Sport()));
    }

    item=new CustomTreeWidgetItem(0);
    item->setText(0,netname);
    item->set_type(5);
    item->setToolTip(0,"Mode and its net name.");
    item->setForeground(0,Qt::black);
    item->setFlags(itemName->flags() & ~Qt::ItemIsEditable);
    itemName->addChild(item);
    item->set_OPEMobject(this);

    // S port
    CustomTreeWidgetItem *itemSport=new CustomTreeWidgetItem(0);
    itemSport->setText(0,"S Port");
    itemSport->set_type(8);
    itemSport->setFlags(itemName->flags() & ~Qt::ItemIsEditable);
    itemSport->setToolTip(0,"S-port number for the mode.");
    itemSport->setForeground(0,Qt::black);
    item->addChild(itemSport);

    // Sport number

    CustomTreeWidgetItem *itemSportValue=new CustomTreeWidgetItem(0);
    itemSportValue->set_type(9);
    itemSportValue->setToolTip(0,"S-parameter port number.");
    itemSportValue->setForeground(0,Qt::black);
    itemSportValue->setFlags(itemName->flags() & ~Qt::ItemIsSelectable);
    itemSport->addChild(itemSportValue);

    CustomSpinBox *sportNumber=new CustomSpinBox();
    sportNumber->set_itemTracker(drawingWindow->get_itemTracker());
    sportNumber->set_mode(this);
    sportNumber->set_boundaryDatabase(boundaryDatabase);
    sportNumber->setMinimum(1);
    sportNumber->setValue(get_Sport());
    drawingItemTree->setItemWidget(itemSportValue,0,sportNumber);

    //xxx
    QObject::connect(sportNumber,&CustomSpinBox::CustomValueChanged,&spinValueChanged);
    QObject::connect(sportNumber,&CustomSpinBox::CustomValueChanged,relay,&Relay::setMenus);


    // voltage integration paths

    CustomTreeWidgetItem *itemVoltage=new CustomTreeWidgetItem(0);
    itemVoltage->setText(0,"voltage");
    itemVoltage->set_type(10);
    itemVoltage->setFlags(item->flags() & ~Qt::ItemIsEditable);
    itemVoltage->setToolTip(0,"Voltage integration path.");
    itemVoltage->setForeground(0,Qt::black);
    item->addChild(itemVoltage);

    long unsigned int i=0;
    while (i < integrationPathList.size()) {
        if (integrationPathList[i]->is_voltage()) {
            integrationPathList[i]->draw(relay,boundaryDatabase,drawingWindow,drawingItemTree,pathItemTree,itemVoltage);
        }
        i++;
    }

    // current integration paths

    CustomTreeWidgetItem *itemCurrent=new CustomTreeWidgetItem(0);
    itemCurrent->setText(0,"current");
    itemCurrent->set_type(11);
    itemCurrent->setFlags(item->flags() & ~Qt::ItemIsEditable);
    itemCurrent->setToolTip(0,"Current integration path.");
    itemCurrent->setForeground(0,Qt::black);
    item->addChild(itemCurrent);

    i=0;
    while (i < integrationPathList.size()) {
        if (integrationPathList[i]->is_current()) {
            integrationPathList[i]->draw(relay,boundaryDatabase,drawingWindow,drawingItemTree,pathItemTree,itemCurrent);
        }
        i++;
    }
}

void Mode::crossLink (std::vector<Path *> *pathList, CustomTreeWidgetItem *portItem)
{
    long unsigned int i=0;
    while (i < integrationPathList.size()) {
        integrationPathList[i]->crossLink(pathList,portItem);
        i++;
    }
}

#endif

Mode::~Mode()
{
   long unsigned int i=0;
   while (i < integrationPathList.size()) {
      if (integrationPathList[i]) delete integrationPathList[i];
      i++;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
// DifferentialPair
///////////////////////////////////////////////////////////////////////////////////////////

DifferentialPair::DifferentialPair (int startLine_, int endLine_)
{
   startLine=startLine_;
   endLine=endLine_;

   // Sport_P
   Sport_P.push_alias("Sport_P");
   Sport_P.set_loaded(false);
   Sport_P.set_positive_required(true);
   Sport_P.set_non_negative_required(false);
   Sport_P.set_lowerLimit(1);
   Sport_P.set_upperLimit(100);
   Sport_P.set_checkLimits(true);

   // Sport_N
   Sport_N.push_alias("Sport_N");
   Sport_N.set_loaded(false);
   Sport_N.set_positive_required(true);
   Sport_N.set_non_negative_required(false);
   Sport_N.set_lowerLimit(1);
   Sport_N.set_upperLimit(100);
   Sport_N.set_checkLimits(true);

   modified=false;

#ifdef HAS_GUI
   item=nullptr;
#endif
}

bool DifferentialPair::load(string *indent, inputFile *inputs)
{
   bool fail=false;

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {

      string token,value,line;
      line=inputs->get_line(lineNumber);
      get_token_pair(&line,&token,&value,&lineNumber,*indent);

      int recognized=0;

      if (Sport_P.match_alias(&token)) {
         recognized++;
         if (Sport_P.loadInt(&token, &value, lineNumber)) fail=true;
      }

      if (Sport_N.match_alias(&token)) {
         recognized++;
         if (Sport_N.loadInt(&token, &value, lineNumber)) fail=true;
      }

      // should recognize one keyword
      if (recognized != 1) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3192: Unrecognized keyword at line %d.\n",
                                                indent->c_str(),indent->c_str(),lineNumber);
         fail=true;
      }
      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }

   return fail;
}

bool DifferentialPair::is_loaded ()
{
   if (!Sport_P.is_loaded()) return false;
   if (!Sport_N.is_loaded()) return false;
   return true;
}

bool DifferentialPair::inDifferentialPairBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

bool DifferentialPair::check(string *indent)
{
   bool fail=false;

   // Sport_P
   if (!Sport_P.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3104: DifferentialPair block at line %d must specify an Sport_P number.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   // Sport_N
   if (!Sport_N.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3154: DifferentialPair block at line %d must specify an Sport_N number.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   // Cannot be same Sport
   if (Sport_P.is_loaded() && Sport_N.is_loaded() && Sport_P.int_compare(&Sport_N)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3184: DifferentialPair block at line %d specifies the same Sport_P and Sport_N numbers.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   return fail;
}

void DifferentialPair::print (string indent)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sDifferentialPair\n",indent.c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   Sport_P=%d\n",indent.c_str(),Sport_P.get_int_value());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   Sport_N=%d\n",indent.c_str(),Sport_N.get_int_value());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sEndDifferentialPair\n",indent.c_str());
   return;
}

void DifferentialPair::save (ofstream *out)
{
    PetscMPIInt rank;
    MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

    modified=false;
    if (rank != 0) return;

    *out << "   DifferentialPair" << endl;
    *out << "      Sport_P=" << Sport_P.get_int_value() << endl;
    *out << "      Sport_N=" << Sport_N.get_int_value() << endl;
    *out << "   EndDifferentialPair" << endl;
    return;
}

///////////////////////////////////////////////////////////////////////////////////////////
// PortAttribute
///////////////////////////////////////////////////////////////////////////////////////////

void PortAttribute::print (string indent)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sPortAttribute\n",indent.c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   attribute=%d\n",indent.c_str(),attribute);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s   adjacent_element_attribute=%d\n",indent.c_str(),adjacent_element_attribute);
}

bool PortAttribute::has_attribute (int attribute_)
{
   if (attribute == attribute_) return true;
   return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Port
///////////////////////////////////////////////////////////////////////////////////////////

Port::Port (int startLine_, int endLine_)
{   
   startLine=startLine_;
   endLine=endLine_;

   // port
   name.push_alias("name");
   name.set_loaded(false);
   name.set_positive_required(false);
   name.set_non_negative_required(false);
   name.set_lowerLimit(0);
   name.set_upperLimit(0);
   name.set_checkLimits(false);

   // impedance_definition
   impedance_definition.push_alias("impedance_definition");
   impedance_definition.set_loaded(false);
   impedance_definition.set_positive_required(false);
   impedance_definition.set_non_negative_required(false);
   impedance_definition.set_lowerLimit(0);
   impedance_definition.set_upperLimit(0);
   impedance_definition.set_checkLimits(false);

   // impedance_calculation
   impedance_calculation.push_alias("impedance_calculation");
   impedance_calculation.set_loaded(false);
   impedance_calculation.set_positive_required(false);
   impedance_calculation.set_non_negative_required(false);
   impedance_calculation.set_lowerLimit(0);
   impedance_calculation.set_upperLimit(0);
   impedance_calculation.set_checkLimits(false);

   normal.SetSize(3);
   normal=0.0;

   rotated_normal.SetSize(3);
   rotated_normal=0.0;

   Ti=nullptr;
   Tv=nullptr;
   TiTvSize=0;

   modified=false;

#ifdef HAS_GUI
   item=nullptr;
#endif
}

bool Port::is_modified ()
{
    if (modified) return true;

    long unsigned int i=0;
    while (i < modeList.size()) {
        if (modeList[i]->is_modified()) return true;
        i++;
    }

    i=0;
    while (i < differentialPairList.size()) {
        if (differentialPairList[i]->is_modified()) return true;
        i++;
    }

    return false;
}

bool Port::netNameExists (string netName)
{
    long unsigned int i=0;
    while (i < modeList.size()) {
        if (modeList[i]->get_net().compare(netName) == 0) return true;
        i++;
    }
    return false;
}

bool Port::load (string *indent, inputFile *inputs)
{
   bool fail=false;
   bool found_first_path=false;

   // blocks

   // modes
   if (findModeBlocks(inputs)) fail=true;
   if (findLineBlocks(inputs)) fail=true;
   long unsigned int i=0;
   while (i < modeList.size()) {
      if (modeList[i]->load(indent,inputs)) fail=true;
      i++;
   }

   // differential pairs
   if (findDifferentialPairBlocks(inputs)) fail=true;
   i=0;
   while (i < differentialPairList.size()) {
      if (differentialPairList[i]->load(indent,inputs)) fail=true;
      i++;
   }

   // keywords

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {

      if (!inModeBlocks(lineNumber) && !inDifferentialPairBlocks(lineNumber)) {

         string token,value,line;
         line=inputs->get_line(lineNumber);
         get_token_pair(&line,&token,&value,&lineNumber,*indent);

         int recognized=0;

         if (name.match_alias(&token)) {
            if (name.is_loaded()) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3053: Duplicate entry at line %d for previous entry at line %d.\n",
                                                      indent->c_str(),indent->c_str(),lineNumber,impedance_definition.get_lineNumber());
               fail=true;
            } else {
               name.set_keyword(token);
               name.set_value(value);
               name.set_lineNumber(lineNumber);
               name.set_loaded(true);
            }
            recognized++;
         }

         if (impedance_definition.match_alias(&token)) {
            if (impedance_definition.is_loaded()) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3054: Duplicate entry at line %d for previous entry at line %d.\n",
                                                      indent->c_str(),indent->c_str(),lineNumber,impedance_definition.get_lineNumber());
               fail=true;
            } else {
               impedance_definition.set_keyword(token);
               impedance_definition.set_value(value);
               impedance_definition.set_lineNumber(lineNumber);
               impedance_definition.set_loaded(true);
            }
            recognized++;
         }

         if (impedance_calculation.match_alias(&token)) {
            if (impedance_calculation.is_loaded()) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3055: Duplicate entry at line %d for previous entry at line %d.\n",
                                                      indent->c_str(),indent->c_str(),lineNumber,impedance_calculation.get_lineNumber());
               fail=true;
            } else {
               impedance_calculation.set_keyword(token);
               impedance_calculation.set_value(value);
               impedance_calculation.set_lineNumber(lineNumber);
               impedance_calculation.set_loaded(true);
            }
            recognized++;
         }

         if (token.compare("path") == 0) {
            if (found_first_path) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3057: Extraneous path= statement at line %d.\n",
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
                  prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3058: Misformatted path at line %d.\n",
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
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3059: Missing path= statement before line %d.\n",
                                                      indent->c_str(),indent->c_str(),lineNumber);
               fail=true;
            }
            recognized++;
         }

         if (token.compare("path-") == 0) {
            if (found_first_path) {
               if (value.substr(0,1).compare("+") == 0 || value.substr(0,1).compare("-") == 0) {
                  prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3060: Misformatted path at line %d.\n",
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
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3061: Missing path= statement before line %d.\n",
                                                      indent->c_str(),indent->c_str(),lineNumber);
               fail=true;
            }
            recognized++;
         }

         // should recognize one keyword
         if (recognized != 1) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3062: Unrecognized keyword at line %d.\n",
                                                   indent->c_str(),indent->c_str(),lineNumber);
            fail=true;
         }
      }
      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }

   return fail;
}

bool Port::inBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

bool Port::inModeBlocks (int lineNumber)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      if (modeList[i]->inModeBlock(lineNumber)) return true;
      i++;
   }
   return false;
}

bool Port::findModeBlocks (inputFile *inputs)
{
   bool fail=false;
   int start_lineNumber=startLine;
   int stop_lineNumber=endLine;
   int block_start,block_stop;

   while (start_lineNumber < stop_lineNumber) {
      if (inputs->findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "Mode", "EndMode", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            Mode *newMode=new Mode(block_start,block_stop,"modal");
            modeList.push_back(newMode);
         }
      }
      start_lineNumber=inputs->get_next_lineNumber(block_stop);
   }
   return fail;
}

bool Port::findLineBlocks (inputFile *inputs)
{
   bool fail=false;
   int start_lineNumber=startLine;
   int stop_lineNumber=endLine;
   int block_start,block_stop;

   while (start_lineNumber < stop_lineNumber) {
      if (inputs->findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "Line", "EndLine", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            Mode *newMode=new Mode(block_start,block_stop,"line");
            modeList.push_back(newMode);
         }
      }
      start_lineNumber=inputs->get_next_lineNumber(block_stop);
   }
   return fail;
}

bool Port::inDifferentialPairBlocks (int lineNumber)
{
   long unsigned int i=0;
   while (i < differentialPairList.size()) {
      if (differentialPairList[i]->inDifferentialPairBlock(lineNumber)) return true;
      i++;
   }
   return false;
}

bool Port::findDifferentialPairBlocks (inputFile *inputs)
{
   bool fail=false;
   int start_lineNumber=startLine;
   int stop_lineNumber=endLine;
   int block_start,block_stop;

   while (start_lineNumber < stop_lineNumber) {
      if (inputs->findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "DifferentialPair", "EndDifferentialPair", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            DifferentialPair *newDifferentialPair=new DifferentialPair(block_start,block_stop);
            differentialPairList.push_back(newDifferentialPair);
         }
      }
      start_lineNumber=inputs->get_next_lineNumber(block_stop);
   }
   return fail;
}

bool Port::check (string *indent, vector<Path *> *pathList, bool check_closed_loop)
{
   bool fail=false;

   if (!name.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3063: Port block at line %d must specify a name.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   // impedance_definition
   if (impedance_definition.is_loaded()) {
      if (!(impedance_definition.get_value().compare("VI") == 0 || 
            impedance_definition.get_value().compare("PV") == 0 ||
            impedance_definition.get_value().compare("PI") == 0)) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3064: Input at line %d must be \"VI\", \"PV\", or \"PI\".\n",
                                                indent->c_str(),indent->c_str(),impedance_definition.get_lineNumber());
         fail=true;      
      }
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3065: Port block at line %d must specify an impedance definition.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   // impedance_calculation
   if (impedance_calculation.is_loaded()) {
      if (!(impedance_calculation.get_value().compare("modal") == 0 ||
            impedance_calculation.get_value().compare("line") == 0)) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3066: Input at line %d must be \"modal\" or \"line\".\n",
                                                indent->c_str(),indent->c_str(),impedance_calculation.get_lineNumber());
         fail=true;
      }
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3067: Port block at line %d must specify an impedance calculation.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   // must have a path
   if (pathNameList.size() == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3070: Port block at line %d must specify a path.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   // paths exist
   long unsigned int i=0;
   while (i < pathNameList.size()) {
      bool found=false;
      long unsigned int j=0;
      while (j < pathList->size()) {
         if (pathNameList[i]->get_value().compare((*pathList)[j]->get_name()) == 0) {found=true; break;}
         j++;
      }
      if (! found) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3071: Port block at line %d specifies a non-existent path.\n",
                                                indent->c_str(),indent->c_str(),startLine);
         fail=true;
      }
      i++;
   }

   // paths are not duplicated
   i=0;
   while (pathNameList.size() > 0 && i < pathNameList.size()-1) {
      long unsigned int j=i+1;
      while (j < pathNameList.size()) {
         if (pathNameList[i]->get_value().compare(pathNameList[j]->get_value()) == 0) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3072: Port block at line %d duplicates path \"%s\".\n",
                                                   indent->c_str(),indent->c_str(),startLine,pathNameList[j]->get_value().c_str());
            fail=true;
         }
         j++;
      }
      i++;
   }

   // Mode

   // must have at least one mode
   if (modeList.size() == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3097: Port block at line %d must specify at least one mode.\n",
                                             indent->c_str(),indent->c_str(),startLine);
      fail=true;
   }

   // mode checks
   i=0;
   while (i < modeList.size()) {
      if (modeList[i]->check(indent,pathList,is_modal(),modeList.size())) fail=true;
      if (modeList[i]->align_current_paths(indent,pathList,check_closed_loop)) fail=true;
      i++;
   }

   // each mode must have a voltage definition for S-parameter calculation even if the PI impedance definition is used
   if (!fail) {
      bool found=false;
      i=0;
      while (i < modeList.size()) {
         if (modeList[i]->has_voltage()) {found=true; break;}
         i++;
      }
      if (!found) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3074: Port block at line %d must specify a voltage line.\n",
                                                indent->c_str(),indent->c_str(),startLine);
         fail=true;
      }
   }

   // modes must be enclosed by the port boundary
   if (!fail && !is_modePathInside (indent,pathList)) fail=true;

   // differential pair checks

   // require line impedance definition
   if (differentialPairList.size() > 0 && impedance_calculation.get_value().compare("line") != 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3084: Input at line %d must be \"line\" when differential pairs are specified.\n",
                                             indent->c_str(),indent->c_str(),impedance_calculation.get_lineNumber());
      fail=true;
   }

   // differential pair checks
   i=0;
   while (i < differentialPairList.size()) {
      if (differentialPairList[i]->check(indent)) fail=true;
      i++;
   }

   // differential pair ports must exist
   i=0;
   while (i < differentialPairList.size()) {

      if (differentialPairList[i]->is_loaded()) {

         // Sport_P
         bool found=false;
         long unsigned int j=0;
         while (j < modeList.size()) {
            if (differentialPairList[i]->get_Sport_P() == modeList[j]->get_Sport()) {found=true; break;}
            j++;
         }
         if (!found) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3096: DifferentialPair block at line %d calls for an unspecified Mode or Line Sport for Sport_P.\n",
                                                   indent->c_str(),indent->c_str(),differentialPairList[i]->get_startLine());
            fail=true;
         }

         // Sport_N
         found=false;
         j=0;
         while (j < modeList.size()) {
            if (differentialPairList[i]->get_Sport_N() == modeList[j]->get_Sport()) {found=true; break;}
            j++;
         }
         if (!found) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3073: DifferentialPair block at line %d calls for an unspecified Mode or Line Sport for Sport_N.\n",
                                                   indent->c_str(),indent->c_str(),differentialPairList[i]->get_startLine());
            fail=true;
         }
      }

      i++;
   }

   return fail;
}

// Check that there is just one path defining the port.
// Used for OpenParEMg.
bool Port::has_complex_path ()
{
    if (pathIndexList.size() > 1) return true;
    return false;
}

bool Port::is_overlapPath (Path *testPath)
{
   if (!outline) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Port::is_overlapPath operation on a Port without an outline.\n");
      return false;
   }

   // any point interior constitutes an overlap
   long unsigned int i=0;
   while (i < testPath->get_points_size()) {
      if (outline->is_point_interior(testPath->get_point_value(i))) return true;
      i++;
   }

   // all points inside constitutes an overlap
   bool outside=false;
   i=0;
   while (i < testPath->get_points_size()) {
      if (! outline->is_point_inside(testPath->get_point_value(i))) {
         outside=true;
         break;
      }
      i++;
   }
   if (! outside) return true;

   // crossing lines constitutes an overlap
   if (outline->is_path_overlap(testPath)) return true;

   return false;
}

bool Port::assignPathIndices (vector<Path *> *pathList)
{
   bool fail=false;

   long unsigned int i=0;
   while (i < pathNameList.size()) {
      bool found=false;
      long unsigned int j=0;
      while (j < pathList->size()) {
         if ((*pathList)[j]->get_name().compare(pathNameList[i]->get_value()) == 0) {
            pathIndexList.push_back(j);
            found=true;
            break;
         }
         j++;
      }
      if (! found) {
         // errors previously reported
         fail=true;
      }
      i++;
   }

   i=0;
   while (i < modeList.size()) {
      if (modeList[i]->assignPathIndices(pathList)) fail=true;
      i++;
   }

   return fail;
}

vector<int> Port::get_SportList ()
{
   vector<int> SportList;

   long unsigned int i=0;
   while (i < modeList.size()) {
      SportList.push_back(modeList[i]->get_Sport());
      i++;
   }

   return SportList;
}

bool Port::checkBoundingBox (Vector *lowerLeft, Vector *upperRight, string *indent, double tol, vector<Path *> *pathList)
{
   bool fail=false;

   long unsigned int i=0;
   while (i < pathIndexList.size()) {
      if ((*pathList)[pathIndexList[i]]->checkBoundingBox(lowerLeft, upperRight, indent, tol)) fail=true;
      i++;
   }

   i=0;
   while (i < modeList.size()) {
      if (modeList[i]->checkBoundingBox(lowerLeft,upperRight,indent,tol,pathList)) fail=true;
      i++;
   }

   return fail;
}

void Port::set2DModeNumbers ()
{
   int modeNumber=1;
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->set_modeNumber2D(modeNumber);
      modeNumber++;
      i++;
   }
}

void Port::print ()
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Port\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   name=%s\n",get_name().c_str());

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

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   impedance_definition=%s\n",impedance_definition.get_value().c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   impedance_calculation=%s\n",impedance_calculation.get_value().c_str());

   i=0;
   while (i < modeList.size()) {
      modeList[i]->print("   ");
      i++;
   }

   i=0;
   while (i < differentialPairList.size()) {
      differentialPairList[i]->print("   ");
      i++;
   }

   i=0;
   while (i < attributeList.size()) {
      attributeList[i]->print("   ");
      i++;
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   assignedToMesh=%d\n",assignedToMesh);
//   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   appliedPortABCreal=%d\n",appliedPortABCreal);
//   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   appliedPortABCimag=%d\n",appliedPortABCimag);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   spin180degrees=%d\n",spin180degrees);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   meshFilename=%s\n",meshFilename.c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   modesFilename=%s\n",modesFilename.c_str());
   if (outline) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"   outline=%p:\n",outline);
      outline->print("      ");
   } else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   outline=%p\n",outline);}
   if (rotated_outline) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"   rotated_outline=%p:\n",rotated_outline);
      rotated_outline->print("      ");
   } else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"   rotated_outline=%p\n",rotated_outline);}
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   outward normal=(%g,%g,%g)\n",normal(0),normal(1),normal(2));
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   rotated outward normal=(%g,%g,%g)\n",rotated_normal(0),rotated_normal(1),rotated_normal(2));

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Ti: TiTvSize=%d Ti=%p\n",TiTvSize,Ti); 
   if (Ti) { 
      int m=0;
      while (m < TiTvSize) {
         int n=0;
         while (n < TiTvSize) {
            double realVal=matrixGetRealValue(Ti,m+n*TiTvSize);
            double imagVal=matrixGetImagValue(Ti,m+n*TiTvSize);
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"      Ti[%d,%d]=(%g,%g)\n",m,n,realVal,imagVal);
            n++;
         }
         m++;
      }
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Tv: TiTvSize=%d Tv=%p\n",TiTvSize,Tv);
   if (Tv) {
      int m=0;
      while (m < TiTvSize) {
         int n=0;
         while (n < TiTvSize) {
            double realVal=matrixGetRealValue(Tv,m+n*TiTvSize);
            double imagVal=matrixGetImagValue(Tv,m+n*TiTvSize);
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"      Tv[%d,%d]=(%g,%g)\n",m,n,realVal,imagVal);
            n++;
         }
         m++;
      }
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"EndPort\n");

   return;
}

void Port::save (std::ofstream *out)
{
    PetscMPIInt rank;
    MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

    modified=false;
    if (rank != 0) return;

    *out << "Port" << endl;
    *out << "   name=" << get_name() << endl;
    long unsigned int i=0;
    while (i < pathNameList.size()) {
        if (i == 0) {
            if (reverseList[i]) {*out << "   path=-" << pathNameList[i]->get_value() << endl;}
            else {*out << "   path=" << pathNameList[i]->get_value() << endl;}
        } else {
            if (reverseList[i]) {*out << "   path-=" << pathNameList[i]->get_value() << endl;}
            else {*out << "   path+=" << pathNameList[i]->get_value() << endl;}
        }
        i++;
    }

    *out << "   impedance_definition=" << impedance_definition.get_value() << endl;
    *out << "   impedance_calculation=" << impedance_calculation.get_value() << endl;

    i=0;
    while (i < modeList.size()) {
        modeList[i]->save(out);
        i++;
    }

    i=0;
    while (i < differentialPairList.size()) {
        differentialPairList[i]->save(out);
        i++;
    }

    *out << "EndPort" << endl;
    *out << std::endl;

    return;
}

void Port::printPaths (vector<Path *> *pathList)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Port=%s\n",get_name().c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Paths:\n");
   long unsigned int i=0;
   while (i < pathIndexList.size()) {
      (*pathList)[pathIndexList[i]]->print("      ");
      i++;
   }
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Outline Path:\n");
   if (outline) outline->print("      ");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Rotated Path:\n");
   if (rotated_outline) rotated_outline->print("      ");
}

bool Port::createDirectory (string *tempDirectory)
{
   if (std::filesystem::exists(tempDirectory->c_str())) {
      stringstream PortDir;
      PortDir << *tempDirectory << "/" << "S" << get_name();
      if (std::filesystem::create_directory(PortDir.str().c_str())) return false;
      else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3075: Failed to create the port working directory \"%s\".\n",
                                                   PortDir.str().c_str());}
   }
   return true;
}

void Port::saveMesh (MeshMaterialList *materials, string *directory, ParSubMesh *parSubMesh)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   stringstream parSaveMeshFilename,processed_parSaveMeshFilename;
   parSaveMeshFilename << *directory << "/S" << get_name() << "/par" << meshFilename;
   if (size > 1)  parSaveMeshFilename << "." << setw(6) << setfill('0') << rank;

   processed_parSaveMeshFilename << *directory << "/S" << get_name() << "/" << meshFilename;
   if (size > 1) processed_parSaveMeshFilename << "." << setw(6) << setfill('0') << rank;

   bool useSerial=false;
   if (useSerial) {
      ofstream serout(processed_parSaveMeshFilename.str().c_str());
      if (serout.is_open()) {
         serout.precision(15);

         RotatedMesh *mesh2D=new RotatedMesh();
         Mesh *mesh2Dbase;
         mesh2Dbase=mesh2D;
         *mesh2Dbase=parSubMesh->GetSerialMesh(0);
         mesh2D->rotate(rotated_outline,spin180degrees);
         mesh2D->set_spaceDim(2);
         mesh2D->Print(serout);
         delete mesh2D;

         serout.close();
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3076: Failed to open file \"%s\" for writing.\n",
                                                parSaveMeshFilename.str().c_str());
      }
   } else {

      // save
      ofstream parout(parSaveMeshFilename.str().c_str());
      if (parout.is_open()) {
         parout.precision(15);
         parSubMesh->ParPrint(parout);
         parout.close();
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3077: Failed to open file \"%s\" for writing.\n",
                                                parSaveMeshFilename.str().c_str());
      }

      // post-process the mesh to make it fully 2D and to flip the direction the port faces, if needed
      if (postProcessMesh(parSaveMeshFilename.str(),processed_parSaveMeshFilename.str())) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3078: Failed to post-process mesh file \"%s\".\n",
                                                parSaveMeshFilename.str().c_str());
      }
   }

   // save the materials associated with the mesh
   if (rank == 0) {
      stringstream saveRegionsFilename;
      saveRegionsFilename << *directory << "/S" << get_name() << "/materials_for_" << meshFilename;
      materials->saveRegionsFile(saveRegionsFilename.str().c_str());
   }
}

bool Port::postProcessMesh (string input_filename, string temp_output_filename)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   ifstream INPUT;
   INPUT.open(input_filename.c_str(),ifstream::in);
   if (!INPUT.is_open()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3079: Failed to open file \"%s\" for reading.\n",input_filename.c_str());
      return true;
   }

   ofstream OUTPUT;
   OUTPUT.open(temp_output_filename.c_str(),ofstream::out);
   if (!OUTPUT.is_open()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3080: Failed to open file \"%s\" for writing.\n",temp_output_filename.c_str());
      INPUT.close();
      return true;
   }

   // process line-by-line
   bool inVertices=false;
   int dimension=-1;
   int vertexCount=-1;
   int vertexLineCount=0;
   string line;
   while (std::getline(INPUT,line)) {

      if (inVertices) {
        if (vertexLineCount == 0) {
           vertexCount=stoi(line);
           OUTPUT << vertexCount << endl;
        } else if (vertexLineCount == 1) {
           dimension=stoi(line);
           if (dimension == 3) dimension--;
           OUTPUT << dimension << endl;
        } else {

           // read the coordinate
           double x,y,z;
           int coordCount=0;
           stringstream ssLine(line);
           string value;
           while (std::getline(ssLine,value,' ')) {
              if (coordCount == 0) x=stod(value);
              else if (coordCount == 1) y=stod(value);
              else if (coordCount == 2) z=stod(value);
              coordCount++;
           }

           // rotate so the normal points in the +z direction
           rotated_outline->rotatePoint(&x,&y,&z,spin180degrees);

           // skip z on output
           OUTPUT << setprecision(15) << x << " " << y << endl;

           ssLine.str("");
           ssLine.clear();
        }

        vertexLineCount++;
        if (vertexLineCount-2 == vertexCount) inVertices=false;

      } else {
         OUTPUT << line << endl;
      }

      if (line.compare("vertices") == 0) {
         inVertices=true;
      }
   }

   INPUT.close();
   OUTPUT.close();

   return false;
}

void Port::save2Dsetup (struct projectData *projData, string *directory, double frequency, Gamma *gamma)
{
   stringstream filename;
   filename << *directory << "/S" << get_name() << "/S" << get_name() << ".proj";

   std::filesystem::path global_p(projData->materials_global_path);
   std::filesystem::path local_p(projData->materials_local_path);

   ofstream out;
   out.open(filename.str().c_str(),ofstream::out);

   if (out.is_open()) {
      out << "#OpenParEM2Dproject 1.0" << endl << endl;
      out << "project.save.fields                      true" << endl << endl;            // must save fields, so override the proj file
      out << "mesh.file                                " << meshFilename << endl;
      out << "mesh.refinement.fraction                 " << projData->mesh_2D_refinement_fraction << endl;
      out << "mesh.uniform_refinement.count            " << "0" << endl;
      out << "mesh.enable.refine                       " << "0" << endl << endl; 
      out << "fem.order                                " << projData->fem_order << endl << endl;
      out << "mode.definition.file                     " << modesFilename << endl << endl;

      if (global_p.is_relative()) {
         out << "materials.global.path                    " << "../../" << projData->materials_global_path << endl;
      }
      if (global_p.is_absolute()) {
         out << "materials.global.path                    " << projData->materials_global_path << endl;
      }
      out << "materials.global.name                    " << projData->materials_global_name << endl;

      if (local_p.is_relative()) {
         out << "materials.local.path                    " << "../../" << projData->materials_local_path << endl;
      }
      if (local_p.is_absolute()) {
         out << "materials.local.path                    " << projData->materials_local_path << endl;
      }
      out << "materials.local.name                    " << projData->materials_local_name << endl;

      out << "materials.check.limits                   " << projData->materials_check_limits << endl << endl;
      // do not independently refine the ports
      // The refinement variables are included in case the user wants to use refinement when running OpenParEM2D manually.
      out << "refinement.frequency                     none" << endl;
      out << "refinement.variable                      |Zo|" << endl;
      out << "refinement.iteration.min                 " << projData->refinement_iteration_min << endl;
      out << "refinement.iteration.max                 " << projData->refinement_iteration_max << endl;
      out << "refinement.required.passes               " << projData->refinement_required_passes << endl;
      out << "refinement.tolerance                     " << projData->refinement_relative_tolerance << endl << endl;

      out << setprecision(15) << "frequency.plan.point                     " << frequency << endl << endl;

      out << "solution.modes                           " << get_modeCount() << endl;
      out << "solution.temperature                     " << projData->solution_temperature << endl;
      out << "solution.tolerance                       " << projData->solution_2D_tolerance << endl;
      out << "solution.iteration.limit                 " << projData->solution_iteration_limit << endl;
      out << "solution.modes.buffer                    " << projData->solution_modes_buffer << endl;
      out << "solution.check.closed.loop               " << convertLogic(projData->solution_check_closed_loop) << endl;
      out << "solution.impedance.definition            " << impedance_definition.get_value() << endl;
      out << "solution.impedance.calculation           " << impedance_calculation.get_value() << endl;
      out << "solution.accurate.residual               " << convertLogic(projData->solution_accurate_residual) << endl;
      out << "solution.shift.invert                    " << convertLogic(projData->solution_shift_invert) << endl;
      out << "solution.initial.guess.level             " << projData->solution_initial_guess_level << endl;
      if (gamma && gamma->get_alpha() > 0) {
         out << "solution.initial.alpha                   " << gamma->get_alpha() << endl;
      }
      if (gamma && gamma->get_beta() > 0) {
         // align the freqeuncy correction and multiplier with OpenParEM2D
         out << "solution.initial.beta                    " << gamma->get_beta()*frequency/gamma->get_frequency()*2 << endl;
      }
      out << "solution.shift.factor                    " << projData->solution_shift_factor << endl << endl;

      out << "output.show.refining.mesh                " << convertLogic(projData->output_show_refining_mesh) << endl;
      out << "output.show.iterations                   " << convertLogic(projData->output_show_iterations) << endl;
      out << "output.show.license                      " << convertLogic(projData->output_show_license) << endl << endl;

      out << "test.create.cases                        " << convertLogic(projData->test_create_cases) << endl;
      out << "test.show.audit                          " << "false" << endl;
      out << "test.show.detailed.cases                 " << convertLogic(projData->test_show_detailed_cases) << endl << endl;

      out << "debug.show.memory                        " << convertLogic(projData->debug_show_memory) << endl;
      out << "debug.show.project                       " << convertLogic(projData->debug_show_project) << endl;
      out << "debug.show.frequency.plan                " << convertLogic(projData->debug_show_frequency_plan) << endl;
      out << "debug.show.materials                     " << convertLogic(projData->debug_show_materials) << endl;
      out << "debug.show.mode.definitions              " << convertLogic(projData->debug_show_port_definitions) << endl;
      out << "debug.show.impedance.details             " << convertLogic(projData->debug_show_impedance_details) << endl;
      out << "debug.skip.solve                         " << "false" << endl;
      out << "debug.tempfiles.keep                     " << "true" << endl << endl;

      out.close();
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3081: Failed to open file \"%s\" for writing.\n",filename.str().c_str());
   }
}

void Port::saveModeFile (struct projectData *projData, vector<Path *> *pathList, BoundaryDatabase *boundaryDatabase)
{
   int pathNumber=1;   
   double x1,y1,z1,x2,y2,z2;
   double xr1,yr1,zr1,xr2,yr2,zr2;

   if (pathIndexList.size() != 1) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Port::saveModeFile operation on a Port with an invalid path definition.\n");
   }

   if (!rotated_outline) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Port::saveModeFile operation on a Port without a rotated path.\n");
   }

   Path *path=(*pathList)[pathIndexList[0]];

   if (path->get_points_size() == 0) return;

   // set up the save file
   stringstream modeFilename;
   if (is_modal()) modeFilename << boundaryDatabase->get_tempDirectory() << "/S" << get_name() << "/S" << get_name() << "_modes.txt";
   else modeFilename << boundaryDatabase->get_tempDirectory() << "/S" << get_name() << "/S" << get_name() << "_lines.txt";

   ofstream modeFile;
   modeFile.open(modeFilename.str().c_str(),ofstream::out);

   if (modeFile.is_open()) {

      modeFile << "#OpenParEMmodes 1.0" << endl << endl << "File" << endl << "   name=generated by OpenParEM3D" << endl << "EndFile" << endl << endl;

      // boundaries

      struct point p1=path->get_point_value(0);
      x1=p1.x; y1=p1.y; z1=p1.z;

      long unsigned int i=0;
      while (i < path->get_points_size()) {
         if (!path->is_closed() && i == path->get_points_size()-1) break;

         struct point p2;
         if (i < path->get_points_size()-1) p2=path->get_point_value(i+1);
         else p2=path->get_point_value(0);
         x2=p2.x; y2=p2.y; z2=p2.z;

         Boundary* boundary=boundaryDatabase->get_matchBoundary (x1,y1,z1,x2,y2,z2);
         if (boundary) {

            xr1=x1; yr1=y1; zr1=z1;
            xr2=x2; yr2=y2; zr2=z2;

            rotated_outline->rotatePoint(&xr1,&yr1,&zr1,spin180degrees);
            rotated_outline->rotatePoint(&xr2,&yr2,&zr2,spin180degrees);

            modeFile << "Path" << endl;
            modeFile << "   name=path" << pathNumber << endl;
            modeFile << setprecision(16) << "   point=(" << xr1 << "," << yr1 << ")" << endl;
            modeFile << setprecision(16) << "   point=(" << xr2 << "," << yr2 << ")" << endl;
            modeFile << "   closed=false" << endl;
            modeFile << "EndPath" << endl << endl;

            modeFile << "Boundary" << endl;
            modeFile << "   name=boundary" << pathNumber << endl;
            if (boundary->is_perfect_electric_conductor()) modeFile << "   type=perfect_electric_conductor" << endl;
            if (boundary->is_perfect_magnetic_conductor()) modeFile << "   type=perfect_magnetic_conductor" << endl;
            if (boundary->is_surface_impedance()) {
               modeFile << "   type=surface_impedance" << endl;
               modeFile << "   material=" << boundary->get_material() << endl;
            }
            if (boundary->is_radiation()) {
               // A proper surface impedance boundary is not yet implemented in OpenParEM2D.
               // Assume PEC.
               // ToDo: update this when the surface impedance boundary is available.
               modeFile << "   type=perfect_electric_conductor" << endl;
            }
            modeFile << "   path=path" << pathNumber << endl;
            modeFile << "EndBoundary" << endl << endl;

            pathNumber++;
         }

         x1=x2; y1=y2; z1=z2;

         i++;
      }

      // modes
      i=0;
      while (i < modeList.size()) {
         modeList[i]->output(&modeFile,pathList,rotated_outline,spin180degrees);
         i++;
      }
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3082: Failed to open file \"%s\" for writing.\n",modeFilename.str().c_str());
   }
}

bool Port::merge(vector<Path *> *pathList)
{
   // a path must be defined
   if (pathIndexList.size() == 0) return true;

   // no merge is required
   if (pathIndexList.size() == 1) {
      outline=(*pathList)[pathIndexList[0]];
      return false;
   }

   // merge

   stringstream boundaryName;
   boundaryName << get_name();

   Path *mergedPath=nullptr;
   if (mergePaths(pathList,&pathIndexList,&reverseList,"Port",boundaryName.str(),&mergedPath,1e-12)) return true;
   if (!mergedPath) return true;

   stringstream pathName;
   pathName << "S" << get_name() << "_OpenParEM3D_generated";
   mergedPath->set_name(pathName.str());
   outline=mergedPath;
   pathList->push_back(mergedPath);

   // update the tracking information

   pathNameList.clear();
   keywordPair *newName=new keywordPair();
   newName->set_value(pathName.str());
   pathNameList.push_back(newName);

   pathIndexList.clear();
   pathIndexList.push_back(pathList->size()-1);

   reverseList.clear();
   reverseList.push_back(false);

   return false;
}

bool Port::createRotated (vector<Path *> *pathList, string indent)
{
   if (!outline) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Port::createRotated operation on a Port without an outline.\n");
      return false;
   }

   rotated_outline=outline->rotateToXYplane();

   if (! rotated_outline) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3140: Port at line %d does not form a closed polygon with nonzero area.\n",
                                             indent.c_str(),indent.c_str(),startLine);
      return true;
   }
   return false;
}

bool Port::is_point_inside (double x, double y, double z)
{
   if (!outline) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Port::is_point_inside operation on a Port without an outline.\n");
      return false;
   }

   struct point p=set_point(x,y,z,3);
   if (outline->is_point_inside (p)) return true;
   return false;
}

bool Port::is_triangleInside (DenseMatrix *pointMat)
{
   if (is_point_inside (pointMat->Elem(0,0),pointMat->Elem(1,0),pointMat->Elem(2,0)) &&
       is_point_inside (pointMat->Elem(0,1),pointMat->Elem(1,1),pointMat->Elem(2,1)) &&
       is_point_inside (pointMat->Elem(0,2),pointMat->Elem(1,2),pointMat->Elem(2,2))) {
      return true;
   }
   return false;
}

bool Port::is_modePathInside (string *indent, vector<Path *> *pathList)
{
   bool fail=false;

   if (pathIndexList.size() != 1) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Port::is_modePathInside operation on a Port with an invalid path definition.\n");
      return false;
   }

   long unsigned i=0;
   while (i < modeList.size()) {
      long unsigned int index=-1; 
      if (! modeList[i]->is_enclosedByPath(pathList,outline,&index)) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3083: Port %s Mode %d of type \"%s\" is not enclosed within the port.\n",
            indent->c_str(),indent->c_str(),get_name().c_str(),modeList[i]->get_Sport(),modeList[i]->get_type(index).c_str());
         fail=true;
      }
      i++;
   }

   if (fail) return false;
   return true;
}

bool Port::has_attribute (int attribute)
{
   long unsigned int i=0;
   while (i < attributeList.size()) {
      if (attributeList[i]->has_attribute(attribute)) return true;
      i++;
   }
   return false;
}

bool Port::create2Dmesh (int order, ParMesh *mesh3D, vector<ParSubMesh> *parSubMeshesPort, long unsigned int parSubMeshIndex, double tol)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   bool match;
   int foundNormal;
   int adjacentElemIndex,info;
   DenseMatrix pointMat2D(3,3),pointMat3D(3,3);
   Array<int> used3D(3),used2D(3);

   // find the normal
   foundNormal=0;
   int i=0;
   while (i < mesh3D->GetNBE()) {
      if (has_attribute(mesh3D->GetBdrAttribute(i))) {
         if (!foundNormal) {
            ElementTransformation *elemTr=mesh3D->GetBdrElementTransformation(i);
            elemTr->SetIntPoint(&Geometries.GetCenter(elemTr->GetGeometryType()));
            CalcOrtho(elemTr->Jacobian(), normal);

            double mag=sqrt(normal(0)*normal(0)+normal(1)*normal(1)+normal(2)*normal(2));
            normal(0)/=mag;
            normal(1)/=mag;
            normal(2)/=mag;

            foundNormal=1;
         }
      }
      i++;
   }

   // put the normal onto all ranks

   if (rank == 0) {
      double nx,ny,nz;

      i=1;
      while (i < size) {
         MPI_Recv(&nx,1,MPI_DOUBLE,i,0,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&ny,1,MPI_DOUBLE,i,1,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
         MPI_Recv(&nz,1,MPI_DOUBLE,i,2,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

         if (nx != 0 || ny != 0 || nz != 0) {
            normal(0)=nx;
            normal(1)=ny;
            normal(2)=nz;
            foundNormal=1;
         }

         i++;
      }

   } else {
      MPI_Send(&(normal(0)),1,MPI_DOUBLE,0,0,PETSC_COMM_WORLD);
      MPI_Send(&(normal(1)),1,MPI_DOUBLE,0,1,PETSC_COMM_WORLD);
      MPI_Send(&(normal(2)),1,MPI_DOUBLE,0,2,PETSC_COMM_WORLD);
   }

   MPI_Bcast(&(normal(0)),1,MPI_DOUBLE,0,PETSC_COMM_WORLD);
   MPI_Bcast(&(normal(1)),1,MPI_DOUBLE,0,PETSC_COMM_WORLD);
   MPI_Bcast(&(normal(2)),1,MPI_DOUBLE,0,PETSC_COMM_WORLD);
   MPI_Bcast(&foundNormal,1,MPI_INT,0,PETSC_COMM_WORLD);

   if (!foundNormal) {
     prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3180: The Port \"%s\" boundary is inconsistent with the general setup.\n",get_name().c_str());
     return true;
   }

   // reset the boundary attributes from indicating the port to indicating the material
   i=0;
   while (i < mesh3D->GetNBE()) {

      mesh3D->GetBdrPointMatrix(i,pointMat3D);

      // find the matching border element from the 2D mesh
      int j=0;
      while (j < (*parSubMeshesPort)[parSubMeshIndex].GetNE()) {

         (*parSubMeshesPort)[parSubMeshIndex].GetPointMatrix(j,pointMat2D);

         used3D=0; used2D=0;

         int m=0;
         while (m < 3) {
            int n=0;
            while (n < 3) {
               struct point pm=set_point(pointMat3D.Elem(0,m),pointMat3D.Elem(1,m),pointMat3D.Elem(2,m),3);
               struct point pn=set_point(pointMat2D.Elem(0,n),pointMat2D.Elem(1,n),pointMat2D.Elem(2,n),3);
               if (point_comparison(pm,pn,tol)) {
                  used3D[m]=1;
                  used2D[n]=1;
                  break;
               }
               n++;
            }
            m++;
         }

         match=true;
         m=0;
         while (m < 3) {
            if (used3D[m] == 0) {match=false; break;}
            if (used2D[m] == 0) {match=false; break;}
            m++;
         }

         if (match) {
            mesh3D->GetBdrElementAdjacentElement (i,adjacentElemIndex,info);
            (*parSubMeshesPort)[parSubMeshIndex].SetAttribute(j,mesh3D->GetAttribute(adjacentElemIndex));
            break;
         }
         j++;
      }
      i++;
   }

   // create finite element collections and spaces for this 2D port

   fec2D_ND=new ND_FECollection(order,(*parSubMeshesPort)[parSubMeshIndex].Dimension());
   fes2D_ND=new ParFiniteElementSpace(&((*parSubMeshesPort)[parSubMeshIndex]),fec2D_ND);

   fec2D_H1=new H1_FECollection(order,(*parSubMeshesPort)[parSubMeshIndex].Dimension());
   fes2D_H1=new ParFiniteElementSpace(&((*parSubMeshesPort)[parSubMeshIndex]),fec2D_H1);

   fec2D_L2=new L2_FECollection(order,(*parSubMeshesPort)[parSubMeshIndex].Dimension());
   fes2D_L2=new ParFiniteElementSpace(&((*parSubMeshesPort)[parSubMeshIndex]),fec2D_L2);

   // rotate the outward normal
   struct point p=set_point(normal(0),normal(1),normal(2),3);
   rotated_outline->rotatePoint(&p);
   rotated_normal(0)=p.x;
   rotated_normal(1)=p.y;
   rotated_normal(2)=p.z;

   // spin 180 degrees about the y axis if needed to ensure that the outward normal points in the +z direction
   spin180degrees=false;
   if (rotated_normal(2) < 0) spin180degrees=true;

   set_filenames();

   return false;
}

void Port::set_filenames()
{
   stringstream ssMeshFilename;
   ssMeshFilename << "S" << get_name() << "_mesh2D.msh";
   meshFilename=ssMeshFilename.str();

   stringstream ssModesFilename;
   if (is_modal()) ssModesFilename << "S" << get_name() << "_modes.txt";
   else ssModesFilename << "S" << get_name() << "_lines.txt";
   modesFilename=ssModesFilename.str();
}

void eh (MPI_Comm *comm, int *err, ...)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3085: Failed to launch OpenParEM2D.\n");
}

bool Port::solve(string *directory)
{
   PetscMPIInt size,rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // current directory
   std::filesystem::path currentPath=std::filesystem::current_path();
   std::string workingDir=currentPath.string();

   // OpenParEM2D directory
   workingDir.append("/");
   workingDir.append(*directory);
   workingDir.append("/S");
   workingDir.append(get_name());

   // argv
   char *argv[2];

   char project[64];
   sprintf(project,"S%s.proj",get_name().c_str());

   argv[0]=project;
   argv[1]=nullptr;

   int *error_codes=(int *)malloc(size*sizeof(int));

   // launch the jobs from rank 0
   MPI_Errhandler errorHandler;
   MPI_Comm_create_errhandler(eh,&errorHandler);
   MPI_Comm_set_errhandler(PETSC_COMM_WORLD,errorHandler);

   MPI_Comm MPI_PORT_COMM;
   MPI_Comm_spawn ("OpenParEM2D",argv,size,MPI_INFO_NULL,0,PETSC_COMM_WORLD,&MPI_PORT_COMM,error_codes);

   // check that all processes spawned
   bool fail=false;
   int i=0;
   while (i < size) {
       if (error_codes[i] == MPI_ERR_SPAWN) {
           fail=true;
           break;
       }
       i++;
   }

   if (error_codes) {free(error_codes); error_codes=nullptr;}

   if (fail) {
       prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3086: OpenParEM2D failed to launch for \"%s\"\n",project);
       return true;
   }

   // send workingDir
   int length=workingDir.length();
   if (rank == 0) {
      int i=0;
      while (i < size) {
         MPI_Send(&length,1,MPI_INT,i,10,MPI_PORT_COMM);
         MPI_Send(workingDir.c_str(),length,MPI_CHAR,i,11,MPI_PORT_COMM);
         i++;
      }
   }

   // wait for the 2D simulations to finish
   PetscSynchronizedFlush(MPI_PORT_COMM,PETSC_STDOUT);
   int flag=0;
   while (!flag) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      MPI_Iprobe(rank,100000,MPI_PORT_COMM,&flag,MPI_STATUS_IGNORE);
   } 

   // get the status of the 2D simulations 
   int fail2D=0;
   MPI_Recv(&fail2D,1,MPI_INT,rank,100000,MPI_PORT_COMM,MPI_STATUS_IGNORE);

   if (fail2D) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3087: OpenParEM2D error in execution for Port \"%s\".\n",get_name().c_str());
      fail=true;
   }

   // unblock OpenParEM2D
   if (rank == 0) {
      int signal;
      MPI_Send(&signal,1,MPI_INT,0,200000,MPI_PORT_COMM);
   }

   MPI_Comm_disconnect(&MPI_PORT_COMM);

   MPI_Comm_set_errhandler(PETSC_COMM_WORLD,MPI_ERRORS_RETURN);
   MPI_Errhandler_free(&errorHandler);

   return fail;
}

bool Port::uses_current()
{
   if (impedance_definition.get_value().compare("VI") == 0) return true;
   if (impedance_definition.get_value().compare("PI") == 0) return true;
   return false;
}

bool Port::uses_voltage()
{
   if (impedance_definition.get_value().compare("VI") == 0) return true;
   if (impedance_definition.get_value().compare("PV") == 0) return true;
   return false;
}

bool Port::loadSizes_tz (string *directory)
{
   char filename[128];

   PetscMPIInt size;
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // cd to the project directory

   stringstream projDirectory;
   projDirectory << *directory << "/S" << get_name();

   try {
      std::filesystem::current_path(projDirectory.str().c_str());
   } catch (std::filesystem::filesystem_error const& ex) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3088: Missing project directory for the 2D solution of Port %s.\n",get_name().c_str());
      return true;
   }

   // cd to the temp directory

   stringstream tempDirectory;
   tempDirectory << "temp_S" << get_name();

   try {
      std::filesystem::current_path(tempDirectory.str().c_str());
   } catch (std::filesystem::filesystem_error const& ex) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3089: Missing temporary directory for the 2D solution of Port %s.\n",get_name().c_str());
      std::filesystem::current_path("../../");
      return true;
   }

   // t_size

   sprintf(filename,"St_mat.S%s.%05d",get_name().c_str(),size-1);
   ifstream ss_size_t;
   ss_size_t.open(filename,ifstream::in);
   if (ss_size_t.is_open()) {
      bool fail=false;
      string line;
      if (getline(ss_size_t,line)) {
         bool found=false;
         stringstream sstream(line);
         string entry;
         int count=0;
         while (std::getline(sstream,entry,' ')) {
            if (count == 3 && is_int(&entry)) {
               found=true;
               t_size=stoi(entry)+1;
            }
            count++;
         }
         if (! found) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3090: Failed to parse data in file \"%s\".\n",filename);
            fail=true;
         }
      } else {
          prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3091: Failed to read data in file \"%s\".\n",filename);
          fail=true;
      }
      ss_size_t.close();
      if (fail) return true;
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3092: Unable to open file \"%s\" for reading.\n",filename);
      std::filesystem::current_path("../../../");
      return true;
   }

   // z_size

   sprintf(filename,"Tz_eps_re_mat.S%s.%05d",get_name().c_str(),size-1);
   ifstream ss_size_z;
   ss_size_z.open(filename,ifstream::in);
   if (ss_size_z.is_open()) {
      bool fail=false;
      string line;
      if (getline(ss_size_z,line)) {
         bool found=false;
         stringstream sstream(line);
         string entry;
         int count=0;
         while (std::getline(sstream,entry,' ')) {
            if (count == 3 && is_int(&entry)) {
               found=true;
               z_size=stoi(entry)+1;
            }
            count++;
         }
         if (! found) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3093: Failed to parse data in file \"%s\".\n",filename);
            fail=true;
         }
      } else {   
          prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3094: Failed to read data in file \"%s\".\n",filename);
          fail=true;
      }
      ss_size_z.close();
      if (fail) return true;
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3095: Unable to open file \"%s\" for reading.\n",filename);
      std::filesystem::current_path("../../../");
      return true;
   }

   // cd back to the 3D project directory
   std::filesystem::current_path("../../../");

   return false;
}

bool Port::load_modeMetrics (string *directory, double frequency)
{
   bool fail=false;
   char filename[128];
   double NpTodB=20*log10(exp(1));
   double eps0=8.8541878176e-12;
   double ko=2*M_PI*frequency*sqrt(4.0e-7*M_PI*eps0);
   int entry=0;

   PetscMPIInt size;
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // cd to the project directory

   stringstream projDirectory;
   projDirectory << *directory << "/S" << get_name();

   try {
      std::filesystem::current_path(projDirectory.str().c_str());
   } catch (std::filesystem::filesystem_error const& ex) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3098: Missing project directory for the 2D solution of Port %s.\n",get_name().c_str());
      return true;
   }

   // load the data for the given frequency

   vector<string> tokenList;
   sprintf(filename,"S%s_results.csv",get_name().c_str());
   ifstream CSV;
   CSV.open(filename,ifstream::in);
   if (CSV.is_open()) {
      string line;
      while (getline(CSV,line)) {
         stringstream ssLine(line);
         string value;
         bool load=false;
         bool is_first=true;
         while (std::getline(ssLine,value,',')) {
            if (load) tokenList.push_back(value);
            if (is_first) {
               if (is_double(&value) && double_compare(stod(value),frequency,1e-12)) load=true;
               if (! load) break;
               is_first=false;
            }
         }

         ssLine.str("");
         ssLine.clear();
         if (load) break;
      }
      CSV.close();
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3099: Unable to open file \"%s\" for reading.\n",filename);
      std::filesystem::current_path("../../");
      return true;
   }

   // pull out the needed data
   if (tokenList.size() > 0) {
      int modal_impedance_calculation=0;
      int mode_count=0;
      double alpha=0,beta=0,ReZ=0,ImZ=0,ReV=0,ImV=0,ReI=0,ImI=0,RePz=0,ImPz=0;

      // impedance type and mode count

      entry=6;

      if (tokenList.size() > 8) {
         if (is_int(&tokenList[entry])) modal_impedance_calculation=stoi(tokenList[entry]); else fail=true;
         entry++;
         if (is_int(&tokenList[entry])) mode_count=stoi(tokenList[entry]); else fail=true;
         entry++;
      } else fail=true;

      if (mode_count != (int)get_modeCount()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Port::load_modeMetrics mismatched mode counts for Port \"%s\".\n",get_name().c_str());
         fail=true;
      }

      if ((is_modal() && modal_impedance_calculation == 0) ||
          (!is_modal() && modal_impedance_calculation == 1)) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Port::load_modeMetrics mismatched impedance calculation for Port \"%s\".\n",get_name().c_str());
         fail=true;
      }

      int i=0;
      while (i < mode_count) {
         if (entry+10 < (int)tokenList.size()+1) {

            // alpha
            if (processInputNumber(tokenList[entry],&alpha)) fail=true;  // dB/m
            alpha/=NpTodB;   // MKS units
            entry++;

            // beta
            if (processInputNumber(tokenList[entry],&beta)) fail=true;   // beta/ko
            beta*=ko;        // MKS units
            entry++;

            // impedance 
            if (processInputNumber(tokenList[entry],&ReZ)) fail=true;
            entry++;
            if (processInputNumber(tokenList[entry],&ImZ)) fail=true;
            entry++;

            // voltage
            if (processInputNumber(tokenList[entry],&ReV)) fail=true;
            entry++;
            if (processInputNumber(tokenList[entry],&ImV)) fail=true;
            entry++;

            // current
            if (processInputNumber(tokenList[entry],&ReI)) fail=true;
            entry++;
            if (processInputNumber(tokenList[entry],&ImI)) fail=true;
            entry++;

            // Pz
            if (processInputNumber(tokenList[entry],&RePz)) fail=true;
            entry++;
            if (processInputNumber(tokenList[entry],&ImPz)) fail=true;
            entry++;

            // save with the mode
            long unsigned int j=0;
            while (j < modeList.size()) {
               if (i+1 == modeList[j]->get_modeNumber2D()) {
                  modeList[j]->set_alpha(alpha);
                  modeList[j]->set_beta(beta);
                  modeList[j]->set_impedance(ReZ,ImZ);
                  modeList[j]->set_voltage(ReV,ImV);
                  modeList[j]->set_current(ReI,ImI);
                  modeList[j]->set_Pz(RePz,ImPz);
               }
               j++;
            }

         } else {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Port::load_modeMetrics found insufficient data for Port \"%s\".\n",get_name().c_str());
            fail=true;
         }
         i++;
      }
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3105: Missing computed results for Port \"%s\".\n",get_name().c_str());
   }

   // cd back to the 3D project directory
   std::filesystem::current_path("../../");

   return fail;
}

bool Port::loadSolution (string *directory, double frequency)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   loadSizes_tz(directory);
   load_modeMetrics(directory,frequency);
   loadTiTv(directory);

   long unsigned int i=0;
   while (i < modeList.size()) {
      if (modeList[i]->loadSolution(directory,get_name(),t_size,z_size)) return true;
      if (modeList[i]->scaleSolution()) return true;
      i++;
   }

   return false;
}

bool Port::loadTiTv (string *directory)
{
   // cd to the project directory

   stringstream projDirectory;
   projDirectory << *directory << "/S" << get_name();

   try {
      std::filesystem::current_path(projDirectory.str().c_str());
   } catch (std::filesystem::filesystem_error const& ex) {
      return true;
   }

   // cd to the temp directory

   stringstream tempDirectory;
   tempDirectory << "temp_S" << get_name();

   try {
      std::filesystem::current_path(tempDirectory.str().c_str());
   } catch (std::filesystem::filesystem_error const& ex) {
      std::filesystem::current_path("../../");
      return true;
   }

   // load

   ifstream TiTv;
   TiTv.open("TiTv.dat",ifstream::in);
   if (TiTv.is_open()) {

      int lineCount=0;
      bool inTi=false;
      bool inTv=false;
      int n=-1;

      string line;
      while (getline(TiTv,line)) {

         stringstream ssLine(line);

         if (inTi) {
            if (n >= 0) {
               if (Ti == nullptr) Ti=(lapack_complex_double *) malloc(n*n*sizeof(lapack_complex_double));

               int row=-1,col=-1;
               double realVal=-DBL_MAX,imagVal=-DBL_MAX;
               string value;
               int index=0;
               while (std::getline(ssLine,value,',')) {
                  if (index == 0) row=stoi(value);
                  if (index == 1) col=stoi(value);
                  if (index == 2) realVal=stod(value);
                  if (index == 3) imagVal=stod(value);
                  index++;
               }

               matrixSetValue(Ti,row+col*n,realVal,imagVal);
               lineCount++;
               if (lineCount == n*n) {
                  inTi=false;
                  n=-1;
               }
            } else {
               n=stoi(line);
               TiTvSize=n;
            }
         }
         if (line.compare("Ti:") == 0) {
            inTi=true;
            lineCount=0;
         }

         if (inTv) {
            if (n >= 0) {
               if (Tv == nullptr) Tv=(lapack_complex_double *) malloc(n*n*sizeof(lapack_complex_double));

               int row=-1,col=-1;
               double realVal=-DBL_MAX,imagVal=-DBL_MAX;
               string value;
               int index=0;
               while (std::getline(ssLine,value,',')) {
                  if (index == 0) row=stoi(value);
                  if (index == 1) col=stoi(value);
                  if (index == 2) realVal=stod(value);
                  if (index == 3) imagVal=stod(value);
                  index++;
               }

               matrixSetValue(Tv,row+col*n,realVal,imagVal);
               lineCount++;
               if (lineCount == n*n) {
                  inTv=false;
                  n=-1;
               }
            } else {
               n=stoi(line);
               TiTvSize=n;
            }
         }
         if (line.compare("Tv:") == 0) {
            inTv=true;
            lineCount=0;
         }

         ssLine.str("");
         ssLine.clear();
      }
      TiTv.close();
   } 

   // cd back to the 3D project directory
   std::filesystem::current_path("../../../");

   return false;
}

void Port::build2Dgrids()
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->build2Dgrids(fes2D_ND,fes2D_H1);
      i++;
   }
}

void Port::build3Dgrids(ParFiniteElementSpace *fes3D_ND, ParFiniteElementSpace *fes3D_H1)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->build3Dgrids(fes3D_ND,fes3D_H1);
      i++;
   }
}

void Port::build2DModalGrids()
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->build2DModalGrids(fes2D_ND,fes2D_H1);
      i++;
   }
}

void Port::build2DSolutionGrids ()
{
   grid2DsolutionReEt=new ParGridFunction(fes2D_ND);
   *grid2DsolutionReEt=0.0;

   grid2DsolutionImEt=new ParGridFunction(fes2D_ND);
   *grid2DsolutionImEt=0.0;

   grid2DsolutionReEz=new ParGridFunction(fes2D_L2);
   *grid2DsolutionReEz=0.0;

   grid2DsolutionImEz=new ParGridFunction(fes2D_L2);
   *grid2DsolutionImEz=0.0;

   grid2DsolutionReHt=new ParGridFunction(fes2D_ND);
   *grid2DsolutionReHt=0.0;

   grid2DsolutionImHt=new ParGridFunction(fes2D_ND);
   *grid2DsolutionImHt=0.0;

   grid2DsolutionReHz=new ParGridFunction(fes2D_L2);
   *grid2DsolutionReHz=0.0;

   grid2DsolutionImHz=new ParGridFunction(fes2D_L2);
   *grid2DsolutionImHz=0.0;
}

long unsigned int Port::get_modeCount ()
{
   return modeList.size();
}

int Port::get_SportCount()
{
   int count=0;
   long unsigned int i=0;
   while (i < modeList.size()) {
      if (modeList[i]->get_Sport() > count) count=modeList[i]->get_Sport();
      i++;
   }
   return count;
}

int Port::get_minSportCount ()
{
   int count=INT_MAX;
   long unsigned int i=0;
   while (i < modeList.size()) {
      if (modeList[i]->get_Sport() < count) count=modeList[i]->get_Sport();
      i++;
   }
   return count;
}

int Port::get_maxSportCount ()
{
   int count=0;
   long unsigned int i=0;
   while (i < modeList.size()) {
      if (modeList[i]->get_Sport() > count) count=modeList[i]->get_Sport();
      i++;
   }
   return count;
}

int Port::get_attribute (int adjacent_element_attribute)
{
   long unsigned int i=0;
   while (i < attributeList.size()) {
      if (attributeList[i]->get_adjacent_element_attribute() == adjacent_element_attribute) {
         return attributeList[i]->get_attribute();
      }
      i++;
   }

   // assign
   i=0;
   while (i < attributeList.size()) {
      if (attributeList[i]->get_adjacent_element_attribute() < 0) {
         attributeList[i]->set_adjacent_element_attribute(adjacent_element_attribute);
         return attributeList[i]->get_attribute();
      }
      i++;
   } 

   // out of slots - should not happen
   cout << "ASSERT: Port::get_attribute out of slots." << endl;

   return -1;
}

int Port::get_last_attribute ()
{
   int attribute=-1;

   long unsigned int i=0;
   while (i < attributeList.size()) {
      if (attributeList[i]->get_attribute() > attribute) attribute=attributeList[i]->get_attribute();
      i++;
   }

   return attribute;
}

int Port::get_adjacent_element_attribute (int attribute)
{
   long unsigned int i=0;
   while (i < attributeList.size()) {
      if (attributeList[i]->get_attribute() == attribute) {
         return attributeList[i]->get_adjacent_element_attribute();
      }
      i++;
   }
   return -1;
}

void Port::printSolution (string indent)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank == 0) {
      cout << indent << "Port " << get_name() << endl;
      cout << indent << indent << "t_size=" << t_size << endl;
      cout << indent << indent << "z_size=" << z_size << endl;

      long unsigned int i=0;
      while (i < modeList.size()) {
//ToDo: need to re-write printSolution for Mode
//         modeList[i]->printSolution();
         i++;
      }
   }
}

void Port::build_essTdofList (ParFiniteElementSpace *fespace, ParMesh *pmesh)
{
   Array<int> border_attributes;
   border_attributes.SetSize(pmesh->bdr_attributes.Max());
   border_attributes=0;

   // enable this port
   long unsigned int i=0;
   while (i < attributeList.size()) {
      if (attributeList[i]->get_adjacent_element_attribute() >= 0) {
         border_attributes[attributeList[i]->get_attribute()-1]=1;
      }
      i++;
   }

   if (ess_tdof_list) delete ess_tdof_list;
   ess_tdof_list=new Array<int>;
   fespace->GetEssentialTrueDofs(border_attributes, *ess_tdof_list);
   offset=fespace->GetTrueDofOffsets();
}

void Port::fillX (Vec *X, Vec *Xdofs, int drivingSet)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->fillX(X,Xdofs,ess_tdof_list,offset,drivingSet);
      i++;
   }
}

bool Port::addPortIntegrators (ParMesh *pmesh, ParBilinearForm *pmblf, PWConstCoefficient *Inv_mur, PWConstCoefficient *neg_ko2_Re_er, PWConstCoefficient *neg_ko2_Im_er,
                               vector<Array<int> *> &borderAttributesList,
                               vector<ConstantCoefficient *> &ReC1ConstList, vector<ConstantCoefficient *> &ImC1ConstList,
                               vector<ConstantCoefficient *> &ReC2ConstList, vector<ConstantCoefficient *> &ImC2ConstList,
                               bool isReal, int drivingSet, bool solution_check_homogeneous, string indent)
{
   if (isDriving(drivingSet)) return false;

   // set the gamma to the largest for the port to avoid passivity violations
   bool found=false;
   complex<double> max_gamma=complex<double>(0,0);
   complex<double> min_gamma=complex<double>(DBL_MAX,DBL_MAX);
   long unsigned int i=0;
   while (i < modeList.size()) {
      complex<double> test=complex<double>(modeList[i]->get_alpha(),modeList[i]->get_beta());
      if (abs(test) > abs(max_gamma)) max_gamma=test;
      if (abs(test) < abs(min_gamma)) min_gamma=test;
      found=true;
      i++;
   }
   if (!found) {
      cout << "ASSERT: Port::addPortIntegrators failed to find a mode to use." << endl;
   }

   // check for homogenous setup for multi-mode ports
   if (modeList.size() > 1 && solution_check_homogeneous) {
      double tolerance=0.05;   // seems reasonable but no calculation to back it up
      bool fail=false;
      if (abs(max_gamma) == 0) {   // should not happen
         if (abs(min_gamma) > tolerance) fail=true;
      } else {
         if (abs((max_gamma-min_gamma)/max_gamma) > tolerance) fail=true;
      }
      if (fail) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%s%sERROR3182: Port \"%s\" is insufficiently homogeneous.\n",
                                                indent.c_str(),indent.c_str(),indent.c_str(),get_name().c_str());
         return true;
      }
   }

   // cycle through areas with unique materials
   i=0;
   while (i < attributeList.size()) {
      if (attributeList[i]->get_adjacent_element_attribute() >= 0) {

         // set 1/(mur*max_gamma) for this area

         complex<double> compVal=(*Inv_mur)(attributeList[i]->get_adjacent_element_attribute())/max_gamma;

         double ReC1=real(compVal);
         double ImC1=imag(compVal);

         ConstantCoefficient *ReC1Const=new ConstantCoefficient(ReC1);
         ConstantCoefficient *ImC1Const=new ConstantCoefficient(ImC1);

         // set -epsr*ko^2/max_gamma for this area

         compVal=complex<double>((*neg_ko2_Re_er)(attributeList[i]->get_adjacent_element_attribute()),
                                 (*neg_ko2_Im_er)(attributeList[i]->get_adjacent_element_attribute()))/max_gamma;

         double ReC2=real(compVal);
         double ImC2=imag(compVal);

         ConstantCoefficient *ReC2Const=new ConstantCoefficient(ReC2);
         ConstantCoefficient *ImC2Const=new ConstantCoefficient(ImC2);

         // enable each section of the port corresponding to each unique material area
         Array<int> *border_attributes=new Array<int>;
         border_attributes->SetSize(pmesh->bdr_attributes.Max());
         *border_attributes=0;
         (*border_attributes)[attributeList[i]->get_attribute()-1]=1;

         // set the first-order absorbing boundary condition
         if (isReal) {
            pmblf->AddBoundaryIntegrator(new CurlCurlIntegrator(*ReC1Const),*border_attributes);
            pmblf->AddBoundaryIntegrator(new VectorFEMassIntegrator(*ReC2Const),*border_attributes);
         } else {
            pmblf->AddBoundaryIntegrator(new CurlCurlIntegrator(*ImC1Const),*border_attributes);
            pmblf->AddBoundaryIntegrator(new VectorFEMassIntegrator(*ImC2Const),*border_attributes);
         }

         // save for later deleting
         borderAttributesList.push_back(border_attributes);
         ReC1ConstList.push_back(ReC1Const);
         ImC1ConstList.push_back(ImC1Const);
         ReC2ConstList.push_back(ReC2Const);
         ImC2ConstList.push_back(ImC2Const);
      }
      i++;
   }

   return false;
}

void Port::extract2Dmesh (ParMesh *pmesh, vector<ParSubMesh> *parSubMeshesPort)
{
   int count=0;
   long unsigned int i=0;
   while (i < attributeList.size()) {
      if (attributeList[i]->get_adjacent_element_attribute() >= 0) count++;
      i++;
   }

   Array<int> border_attributes;
   border_attributes.SetSize(count);

   int index=0;
   i=0;
   while (i < attributeList.size()) {
      if (attributeList[i]->get_adjacent_element_attribute() >= 0) {
         border_attributes[index]=attributeList[i]->get_attribute();
         index++;
      }
      i++;
   }

   ParSubMesh pmesh2D=ParSubMesh::CreateFromBoundary(*pmesh,border_attributes);
   parSubMeshesPort->push_back(pmesh2D);
}

void Port::addWeight (complex<double> value)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->addWeight(value);
      i++;
   }
}

void Port::calculateSplits ()
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->calculateSplits(fes2D_L2,grid2DsolutionReEt,grid2DsolutionImEt,grid2DsolutionReEz,grid2DsolutionImEz,grid2DsolutionReHt,
                                   grid2DsolutionImHt,grid2DsolutionReHz,grid2DsolutionImHz,normal);
      i++;
   }
}

bool Port::isDriving (int drivingSet)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      if (modeList[i]->get_weight(drivingSet-1) != 0) return true;
      i++;
   }
   return false;
}

Mode* Port::getDrivingMode (int drivingSport)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      if (modeList[i]->get_Sport() == drivingSport) {
         return modeList[i];
      }
      i++;
   }
   return nullptr;
}

void Port::fillIntegrationPoints (vector<Path *> *pathList)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->fillIntegrationPoints(pathList);
      i++;
   }
}

void Port::calculateLineIntegrals (ParMesh *pmesh, fem3D *fem)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->calculateLineIntegrals(pmesh,fem);
      i++;
   }
}

void Port::alignDirections (ParMesh *pmesh, fem3D *fem)
{
   complex<double> VkeepValue=complex<double>(-DBL_MAX,-DBL_MAX);
   complex<double> IkeepValue=complex<double>(-DBL_MAX,-DBL_MAX);
   IntegrationPath *Vpath=nullptr;
   IntegrationPath *Ipath=nullptr;

   // use the first mode for the alignment direction
   if (is_line()) {
      if (modeList.size() > 0) {
         Vpath=modeList[0]->get_voltageIntegrationPath();
         Ipath=modeList[0]->get_currentIntegrationPath();
      }
   }

   if (Vpath) VkeepValue=Vpath->get_integratedValue();
   if (Ipath) IkeepValue=Ipath->get_integratedValue();

   // align
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->alignDirections(pmesh,fem,Vpath,Ipath);
      i++;
   }

   // restore values
   if (Vpath) Vpath->set_integratedValue(VkeepValue);
   if (Ipath) Ipath->set_integratedValue(IkeepValue);
}

void Port::transfer_2Dsolution_2Dgrids_to_3Dgrids ()
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->transfer_2Dsolution_2Dgrids_to_3Dgrids();
      i++;    
   }
}

void Port::transfer_2Dsolution_3Dgrids_to_2Dgrids ()
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->transfer_2Dsolution_3Dgrids_to_2Dgrids();
      i++;
   }
}

void Port::transfer_3Dsolution_3Dgrids_to_2Dgrids (fem3D *fem)
{
   // E

   ParTransferMap *full_to_port_ReEt=new ParTransferMap(*(fem->get_gridReE()),*grid2DsolutionReEt);
   full_to_port_ReEt->Transfer(*(fem->get_gridReE()),*grid2DsolutionReEt);
   delete full_to_port_ReEt; full_to_port_ReEt=nullptr;

   ParTransferMap *full_to_port_ImEt=new ParTransferMap(*(fem->get_gridImE()),*grid2DsolutionImEt);
   full_to_port_ImEt->Transfer(*(fem->get_gridImE()),*grid2DsolutionImEt);
   delete full_to_port_ImEt; full_to_port_ImEt=nullptr;

//   ParTransferMap *full_to_port_ReEz=new ParTransferMap(*(fem->get_gridReEz()),*grid2DsolutionReEz);
//   full_to_port_ReEz->Transfer(*(fem->get_gridReEz()),*grid2DsolutionReEz);
//   delete full_to_port_ReEz; full_to_port_ReEz=nullptr;

//   ParTransferMap *full_to_port_ImEz=new ParTransferMap(*(fem->get_gridImEz()),*grid2DsolutionImEz);
//   full_to_port_ImEz->Transfer(*(fem->get_gridImEz()),*grid2DsolutionImEz);
//   delete full_to_port_ImEz; full_to_port_ImEz=nullptr;

   // H

   ParTransferMap *full_to_port_ReHt=new ParTransferMap(*(fem->get_gridReH()),*grid2DsolutionReHt);
   full_to_port_ReHt->Transfer(*(fem->get_gridReH()),*grid2DsolutionReHt);
   delete full_to_port_ReHt; full_to_port_ReHt=nullptr;

   ParTransferMap *full_to_port_ImHt=new ParTransferMap(*(fem->get_gridImH()),*grid2DsolutionImHt);
   full_to_port_ImHt->Transfer(*(fem->get_gridImH()),*grid2DsolutionImHt);
   delete full_to_port_ImHt; full_to_port_ImHt=nullptr;

//   ParTransferMap *full_to_port_ReHz=new ParTransferMap(*(fem->get_gridReHz()),*grid2DsolutionReHz);
//   full_to_port_ReHz->Transfer(*(fem->get_gridReHz()),*grid2DsolutionReHz);
//   delete full_to_port_ReHz; full_to_port_ReHz=nullptr;

//   ParTransferMap *full_to_port_ImHz=new ParTransferMap(*(fem->get_gridImHz()),*grid2DsolutionImHz);
//   full_to_port_ImHz->Transfer(*(fem->get_gridImHz()),*grid2DsolutionImHz);
//   delete full_to_port_ImHz; full_to_port_ImHz=nullptr;
}

void Port::save2DParaView (ParSubMesh *psubmesh2D, struct projectData *projData, double frequency, bool add_extension)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->save2DParaView(psubmesh2D,projData,frequency,add_extension);
      i++;
   }
}

void Port::save3DParaView(ParMesh *pmesh, struct projectData *projData, double frequency, bool add_extension)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->save3DParaView(pmesh,projData,frequency,add_extension);
      i++;
   }
}

void Port::save2DSolutionParaView(ParSubMesh *psubmesh2D, struct projectData *projData, double frequency, int drivingSport, bool add_extension)
{
   if (!projData->debug_save_port_fields) return;

   stringstream ss;
   ss << projData->project_name << "_frequency_" << frequency << "_port_" << get_name() << "_driving_mode_" << drivingSport;

   stringstream ssParaView;
   ssParaView << "ParaView_solution_2D_" << projData->project_name;
   if (add_extension) ssParaView << "_test";

   ParaViewDataCollection *pd=nullptr;
   pd=new ParaViewDataCollection(ss.str(),psubmesh2D);
   pd->SetOwnData(false);
   pd->SetPrefixPath(ssParaView.str());
   pd->RegisterField("grid2DsolutionReEt",grid2DsolutionReEt);
   pd->RegisterField("grid2DsolutionImEt",grid2DsolutionImEt);
   pd->RegisterField("grid2DsolutionReEz",grid2DsolutionReEz);
   pd->RegisterField("grid2DsolutionImEz",grid2DsolutionImEz);
   pd->RegisterField("grid2DsolutionReHt",grid2DsolutionReHt);
   pd->RegisterField("grid2DsolutionImHt",grid2DsolutionImHt);
   pd->RegisterField("grid2DsolutionReHz",grid2DsolutionReHz);
   pd->RegisterField("grid2DsolutionImHz",grid2DsolutionImHz);
   pd->SetLevelsOfDetail(3);
   pd->SetDataFormat(VTKFormat::ASCII);
   pd->SetHighOrderOutput(true);
   pd->Save();
   delete pd;
}

void Port::save2DModalParaView (ParSubMesh *psubmesh2D, struct projectData *projData, double frequency, bool add_extension)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->save2DModalParaView(psubmesh2D,projData,frequency,add_extension);
      i++;
   }
}

void Port::resetElementNumbers ()
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->resetElementNumbers();
      i++;
   }
}

bool Port::snapToMeshBoundary (vector<Path *> *pathList, Mesh *mesh, string indent)
{
   if (pathIndexList.size() != 1) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Port::snapToMeshBoundary operation on a Port with an invalid path definition.\n");
      return false;
   }

   // snap the port to the mesh boundary
   Path *path=(*pathList)[pathIndexList[0]];
   if (path->snapToMeshBoundary(mesh)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3102: Port \"%s\" failed to snap to the mesh.\n",
                                             indent.c_str(),indent.c_str(),get_name().c_str());
      return true;
   }

   // snap the mode path to the mesh boundary
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->snapToMeshBoundary(pathList,mesh);
      i++;
   }

   return false;
}

void Port::populateGamma (double frequency, GammaDatabase *gammaDatabase)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->populateGamma(frequency,gammaDatabase);
      i++;
   }
}

void Port::reset()
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      modeList[i]->reset();
      i++;
   }

   if (fec2D_ND) {delete fec2D_ND; fec2D_ND=nullptr;}
   if (fes2D_ND) {delete fes2D_ND; fes2D_ND=nullptr;}

   if (fec2D_H1) {delete fec2D_H1; fec2D_H1=nullptr;}
   if (fes2D_H1) {delete fes2D_H1; fes2D_H1=nullptr;}

   if (fec2D_L2) {delete fec2D_L2; fec2D_L2=nullptr;}
   if (fes2D_L2) {delete fes2D_L2; fes2D_L2=nullptr;}

   meshFilename="";
   modesFilename="";

   // do not delete rotated

   outline=nullptr;

   if (ess_tdof_list) {delete ess_tdof_list; ess_tdof_list=nullptr;}

   if (grid2DsolutionReEt) {delete grid2DsolutionReEt; grid2DsolutionReEt=nullptr;}
   if (grid2DsolutionImEt) {delete grid2DsolutionImEt; grid2DsolutionImEt=nullptr;}
   if (grid2DsolutionReEz) {delete grid2DsolutionReEz; grid2DsolutionReEz=nullptr;}
   if (grid2DsolutionImEz) {delete grid2DsolutionImEz; grid2DsolutionImEz=nullptr;}
   if (grid2DsolutionReHt) {delete grid2DsolutionReHt; grid2DsolutionReHt=nullptr;}
   if (grid2DsolutionImHt) {delete grid2DsolutionImHt; grid2DsolutionImHt=nullptr;}
   if (grid2DsolutionReHz) {delete grid2DsolutionReHz; grid2DsolutionReHz=nullptr;}
   if (grid2DsolutionImHz) {delete grid2DsolutionImHz; grid2DsolutionImHz=nullptr;}
}

void Port::aggregateDifferentialPairList (vector<DifferentialPair *> *aggregateList)
{
   long unsigned int i=0;
   while (i < differentialPairList.size()) {
      aggregateList->push_back(differentialPairList[i]);
      i++;
   }
}

void Port::buildAggregateModeList (vector<Mode *> *aggregateModeList)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      aggregateModeList->push_back(modeList[i]);
      i++;
   }
}

bool Port::has_mode (Mode *mode, long unsigned int *index)
{
   long unsigned int i=0;
   while (i < modeList.size()) {
      if (modeList[i] == mode) {
         *index=i;
         return true;
      }
      i++;
   }
   return false;
}

void Port::deleteMode (std::string net)
{
    long unsigned int i=0;
    while (i < modeList.size()) {
        if (modeList[i]->get_net().compare(net) == 0) {
            delete modeList[i];
            modeList.erase(modeList.begin(),modeList.begin()+i);
        }
        i++;
    }
}

void Port::assignPathNormals (std::vector<Path *> *pathList)
{
    long unsigned int i=0;
    while (i < modeList.size()) {
        modeList[i]->assignPathNormals(outline->get_normal(),pathList);
        i++;
    }
}

bool Port::isPathInside (Path *path)
{
    if (!outline) return false;
    return outline->is_path_inside(path);
}

#ifdef HAS_GUI
void comboIndexChanged (int index, Port *port, Boundary *boundary, BoundaryDatabase *boundaryDatabase, int type, CustomTreeWidgetItem *itemMaterial, CustomTreeWidgetItem *itemWaveImpedance) {

    // Port: impedance definition
    if (port && type == 0) {
        if (index == 0) port->set_impedance_definition("VI");
        if (index == 1) port->set_impedance_definition("PV");
        if (index == 2) port->set_impedance_definition("PI");
        port->set_modified();
        boundaryDatabase->set_modified();
    }

    // Port: impedance calculation
    if (port && type == 1) {
        if (index == 0) port->set_impedance_calculation("line");
        if (index == 1) port->set_impedance_calculation("modal");
        port->set_modified();
        boundaryDatabase->set_modified();
    }

    // Boundary: type
    if (boundary && type == 2) {
        if (index == 0) {
            boundary->set_type("perfect_electric_conductor");
            if (itemMaterial) itemMaterial->setHidden(true);
            if (itemWaveImpedance) itemWaveImpedance->setHidden(true);
        }
        if (index == 1) {
            boundary->set_type("perfect_magnetic_conductor");
            if (itemMaterial) itemMaterial->setHidden(true);
            if (itemWaveImpedance) itemWaveImpedance->setHidden(true);
        }
        if (index == 2) {
            boundary->set_type("surface_impedance");
            if (itemMaterial) itemMaterial->setHidden(false);
            if (itemWaveImpedance) itemWaveImpedance->setHidden(true);
        }
        if (index == 3) {
            boundary->set_type("radiation");
            if (itemMaterial) itemMaterial->setHidden(true);
            if (itemWaveImpedance) itemWaveImpedance->setHidden(false);
        }
        boundary->set_modified();
        boundaryDatabase->set_modified();
    }

    return;
}

void comboTextChanged (QString text, Boundary *boundary, BoundaryDatabase *boundaryDatabase)
{
    if (boundary) {
        boundary->set_material(text.toStdString());
        boundary->set_modified();
        boundaryDatabase->set_modified();
    }
}

void spinValueChanged (int value, Mode *mode, BoundaryDatabase *boundaryDatabase)
{
    if (mode) {
        mode->set_Sport(value);
        mode->set_modified();
        boundaryDatabase->set_modified();
    }
}

void textValueChanged (QString text, IntegrationPath *integrationPath, BoundaryDatabase *boundaryDatabase)
{
    std::cout << "textValueChanged" << std::endl; std::cout.flush();
    if (integrationPath) {
        std::cout << "   has integrationPath" << std::endl; std::cout.flush();
        integrationPath->set_scale(text.toDouble());
        integrationPath->set_modified();
        boundaryDatabase->set_modified();
    }
}

void Port::draw (Relay *relay, struct projectData *projData, BoundaryDatabase *boundaryDatabase, CustomOpenGLWidget *drawingWindow,
                 QTreeWidget *drawingItemTree, CustomTreeWidgetItem *pathWidgetItem, CustomTreeWidgetItem *portWidgetItem)
{
    // name

    CustomTreeWidgetItem *itemName=new CustomTreeWidgetItem(0);
    itemName->setText(0,get_name().c_str());
    itemName->set_type(1);
    itemName->setForeground(0,Qt::gray);
    itemName->setFlags(itemName->flags() & ~Qt::ItemIsEditable);
    itemName->setToolTip(0,"Port name.");
    portWidgetItem->addChild(itemName);
    itemName->set_OPEMobject(this);
    set_item(itemName);

    // link paths -  assumes there is only one path, which is checked when loading the database
    Path *path=boundaryDatabase->get_pathList()[pathIndexList[0]];

    // attach item
    int j=0;
    while (j < pathWidgetItem->childCount()) {
        CustomTreeWidgetItem *child=(CustomTreeWidgetItem *) pathWidgetItem->child(j);
        if (child->get_OPEMojbect() == path) {
            itemName->push_linkedItem(child);
            child->push_linkedItem(itemName);
        }
        j++;
    }
    drawingWindow->showItem(itemName);

    // impedance definition

    CustomTreeWidgetItem *itemImpedanceDefinition=new CustomTreeWidgetItem(0);
    itemImpedanceDefinition->set_type(6);
    itemImpedanceDefinition->setFlags(itemImpedanceDefinition->flags() & ~Qt::ItemIsSelectable);
    itemImpedanceDefinition->setToolTip(0,"Impedance definition for calculating characteristic impedance.");
    itemName->addChild(itemImpedanceDefinition);

    CustomComboBox *comboZdef=new CustomComboBox();
    comboZdef->set_itemTracker(drawingWindow->get_itemTracker());
    comboZdef->addItem("VI");
    comboZdef->addItem("PV");
    comboZdef->addItem("PI");
    comboZdef->set_port(this);
    comboZdef->set_boundary(nullptr);
    comboZdef->set_boundaryDatabase(boundaryDatabase);
    comboZdef->set_type(0);
    if (impedance_definition.get_value().compare("VI") == 0) comboZdef->setCurrentIndex(0);
    if (impedance_definition.get_value().compare("PV") == 0) comboZdef->setCurrentIndex(1);
    if (impedance_definition.get_value().compare("PI") == 0) comboZdef->setCurrentIndex(2);
    drawingItemTree->setItemWidget(itemImpedanceDefinition,0,comboZdef);

    QObject::connect(comboZdef,&CustomComboBox::CustomCurrentIndexChanged,&comboIndexChanged);
    QObject::connect(comboZdef,&CustomComboBox::CustomCurrentIndexChanged,relay,&Relay::setMenus);

    // impedance calculation

    CustomTreeWidgetItem *itemImpedanceCalculation=new CustomTreeWidgetItem(0);
    itemImpedanceCalculation->set_type(7);
    itemImpedanceCalculation->setFlags(itemImpedanceDefinition->flags() & ~Qt::ItemIsSelectable);
    itemImpedanceCalculation->setToolTip(0,"Impedance calculation using modal or line integration paths.");
    itemName->addChild(itemImpedanceCalculation);

    CustomComboBox *comboZcalc=new CustomComboBox();
    comboZcalc->set_itemTracker(drawingWindow->get_itemTracker());
    comboZcalc->addItem("line");
    comboZcalc->addItem("modal");
    comboZcalc->set_port(this);
    comboZcalc->set_type(1);
    comboZcalc->set_boundary(nullptr);
    comboZcalc->set_boundaryDatabase(boundaryDatabase);
    if (is_line()) comboZcalc->setCurrentIndex(0);
    if (is_modal()) comboZcalc->setCurrentIndex(1);
    drawingItemTree->setItemWidget(itemImpedanceCalculation,0,comboZcalc);

    QObject::connect(comboZcalc, &CustomComboBox::CustomCurrentIndexChanged, &comboIndexChanged);
    QObject::connect(comboZcalc,&CustomComboBox::CustomCurrentIndexChanged,relay,&Relay::setMenus);

    // modes
    struct point outline_normal=outline->get_normal();
    long unsigned int i=0;
    while (i < modeList.size()) {
        modeList[i]->draw(relay,boundaryDatabase,drawingWindow,drawingItemTree,pathWidgetItem,itemName);
        i++;
    }
}

void Port::crossLink (std::vector<Path *> *pathList)
{
    if (outline) outline->set_portItem(item);

    long unsigned int i=0;
    while (i < modeList.size()) {
        modeList[i]->crossLink(pathList,item);
        i++;
    }
}

#endif

Port::~Port ()
{
   long unsigned int i=0;
   while (i < pathNameList.size()) {
      if (pathNameList[i]) delete pathNameList[i];
      i++;
   }

   i=0;
   while (i < modeList.size()) {
      if (modeList[i]) {
         delete modeList[i];
         modeList[i]=nullptr;
      }
      i++;
   }

   i=0;
   while (i < attributeList.size()) {
      if (attributeList[i]) {
         delete attributeList[i];
         attributeList[i]=nullptr;
      }
      i++;
   }

   i=0;
   while (i < differentialPairList.size()) {
      delete differentialPairList[i];
      i++;
   }

   if (rotated_outline) {delete rotated_outline;}

   if (fec2D_ND) {delete fec2D_ND; fec2D_ND=nullptr;}
   if (fes2D_ND) {delete fes2D_ND; fes2D_ND=nullptr;}

   if (fec2D_H1) {delete fec2D_H1; fec2D_H1=nullptr;}
   if (fes2D_H1) {delete fes2D_H1; fes2D_H1=nullptr;}

   if (fec2D_L2) {delete fec2D_L2; fec2D_L2=nullptr;}
   if (fes2D_L2) {delete fes2D_L2; fes2D_L2=nullptr;}

   if (ess_tdof_list) {delete ess_tdof_list; ess_tdof_list=nullptr;}

   if (Ti) {free(Ti); Ti=nullptr;}
   if (Tv) {free(Tv); Tv=nullptr;}
}

///////////////////////////////////////////////////////////////////////////////////////////
// BoundaryDatabase
///////////////////////////////////////////////////////////////////////////////////////////

bool BoundaryDatabase::is_modified ()
{
    if (modified) return true;

    long unsigned int i=0;
    while (i < pathList.size()) {
        if (pathList[i]->is_modified()) return true;
        i++;
    }

    i=0;
    while (i < boundaryList.size()) {
        if (boundaryList[i]->is_modified()) return true;
        i++;
    }

    i=0;
    while (i < portList.size()) {
        if (portList[i]->is_modified()) return true;
        i++;
    }

    return false;
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
            Boundary *newBoundary=new Boundary(block_start,block_stop);
            boundaryList.push_back(newBoundary);
         }
      }
      start_lineNumber=inputs.get_next_lineNumber(block_stop);
   }
   return fail;
}

bool BoundaryDatabase::findPortBlocks ()
{
   bool fail=false;
   int start_lineNumber=inputs.get_first_lineNumber();
   int stop_lineNumber=inputs.get_last_lineNumber();
   int block_start,block_stop;

   // skip the version line, which must be the first line
   start_lineNumber=inputs.get_next_lineNumber(start_lineNumber);

   while (start_lineNumber < stop_lineNumber) {
      if (inputs.findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "Port", "EndPort", false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            Port *newPort=new Port(block_start,block_stop);
            portList.push_back(newPort);
         }
      }
      start_lineNumber=inputs.get_next_lineNumber(block_stop);
   }
   return fail;
}

bool BoundaryDatabase::load (const char *filename, bool check_closed_loop) {
   bool fail=false;
   int dim=3;

   if (strcmp(filename,"") == 0) return false;  // this file is optional

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   loading port definition file \"%s\"\n",filename);

   if (inputs.load(filename)) return true;
   if (inputs.get_size() == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3106: File is has no valid content.\n",
                                             indent.c_str(),indent.c_str());
      return true;
   }
   inputs.createCrossReference();

   if (inputs.checkVersion(version_name, version_value)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3100: Version mismatch.  Expecting the first line to be: %s %s\n",
                                             indent.c_str(),indent.c_str(),version_name.c_str(),version_value.c_str());
      return true;
   }

   // Source
   if (findSourceFileBlocks()) {fail=true;}

   long unsigned int i=0;
   while (i < sourceFileList.size()) {
      if (sourceFileList[i]->load(&indent, &inputs)) fail=true;
      i++;
   }

   // Path
   if (findPathBlocks()) fail=true;

   i=0;
   while (i < pathList.size()) {
      if (pathList[i]->load(dim,&indent, &inputs)) fail=true;
      i++;
   }

   // Boundary

   if (findBoundaryBlocks()) fail=true;

   i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->load(&indent, &inputs)) {
         fail=true;
      } else {
         boundaryList[i]->assignPathIndices(&pathList);
         if (!boundaryList[i]->is_default_boundary()) {
            if (boundaryList[i]->merge(&pathList)) fail=true;
         }
      }
      i++;
   }

   // Port

   if (findPortBlocks()) fail=true;

   i=0;
   while (i < portList.size()) {
      if (portList[i]->load(&indent, &inputs)) fail=true;
      else {
         portList[i]->assignPathIndices(&pathList);
         if (portList[i]->merge(&pathList)) fail=true;
         else {
            if (portList[i]->createRotated(&pathList,indent)) fail=true;
         }
      }
      i++;
   }

   // database checks
   if (check(check_closed_loop)) fail=true;

   // subdivide the paths to eliminate partial overlaps
// ToDo
// This is corrupting the path.  Need to fix.
//   if (!fail) subdivide_paths();


   if (fail) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3107: Failed to load port definitions.\n",indent.c_str(),indent.c_str());}

   return fail;
}

void BoundaryDatabase::print ()
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s %s\n",version_name.c_str(),version_value.c_str());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"drivingSetName=%s\n",drivingSetName.c_str());

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

   i=0;
   while (i < portList.size()) {
      portList[i]->print();
      i++;
   }
}

void BoundaryDatabase::save (std::ofstream *out)
{
    PetscMPIInt rank;
    MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

    modified=false;
    if (rank != 0) return;

    *out << version_name << " " << version_value << endl;
    *out << endl;

    long unsigned int i=0;
    while (i < sourceFileList.size()) {
        sourceFileList[i]->save(out);
        i++;
    }

    i=0;
    while (i < pathList.size()) {
        pathList[i]->save(out);
        i++;
    }

    i=0;
    while (i < boundaryList.size()) {
        boundaryList[i]->save(out);
        i++;
    }

    i=0;
    while (i < portList.size()) {
        portList[i]->save(out);
        i++;
    }
}

bool BoundaryDatabase::inBlocks (int lineNumber)
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

   i=0;
   while (i < portList.size()) {
      if (portList[i]->inBlock(lineNumber)) return true;
      i++;
   }

   return false;
}


bool BoundaryDatabase::check (bool check_closed_loop)
{
   bool fail=false;

   // Source
   if (sourceFileList.size() > 1) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3108: Only one File block is allowed.\n",indent.c_str(),indent.c_str());
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
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3109: name at line %d duplicates the name at line %d.\n",
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
      if (boundaryList[i]->check(&indent,pathList)) fail=true;

      // cross block checks

      // duplicated names
      long unsigned int j=i+1;
      while (boundaryList.size() > 0 && i < boundaryList.size()-1 && j < boundaryList.size()) {
         if (boundaryList[i]->name_is_loaded() && boundaryList[j]->name_is_loaded()) {
            if (boundaryList[i]->get_name().compare(boundaryList[j]->get_name()) == 0) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3110: name at line %d duplicates the name at line %d.\n",
                                                      indent.c_str(),indent.c_str(),boundaryList[j]->get_name_lineNumber(),boundaryList[i]->get_name_lineNumber());
               fail=true;
            }
         }
         j++;
      }

      i++;
   }

   // radiation boundaries must have the same wave impedance
   double wave_impedance=0;
   int wave_impedance_lineNumber=-1;
   i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->is_radiation()) {
         double wave_impedance_test=boundaryList[i]->get_wave_impedance();
         if (wave_impedance == 0) {
            wave_impedance=wave_impedance_test;
            wave_impedance_lineNumber=boundaryList[i]->get_wave_impedance_lineNumber();
         } else {
            if (!double_compare(wave_impedance,wave_impedance_test,1e-6)) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3195: wave impedance at line %d does not match that from line %d.\n",
                                                      indent.c_str(),indent.c_str(),boundaryList[i]->get_wave_impedance_lineNumber(),
                                                      wave_impedance_lineNumber);
               fail=true;
            }
         }
      }
      i++;
   }

   // Port

   i=0;
   while (i < portList.size()) {
      if (portList[i]->check(&indent,&pathList,check_closed_loop)) fail=true;
      i++;
   }

   // check for extraneous text
   i=1;  // skip the first line, which is the version information
   while (i < inputs.get_size()) {
      if (! inBlocks(inputs.get_lineNumber(i))) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3111: Invalid input at line %d.\n",indent.c_str(),indent.c_str(),inputs.get_lineNumber(i));
         fail=true;
      }
      i++;
   }

   return fail;
}

// check that the S-parameter ports are legally numbered across all ports and modes
// i.e. sequential starting at 1 with no duplicates or gaps
bool BoundaryDatabase::checkSportNumbering()
{
   bool fail=false;

   vector<int> subList;
   vector<int> fullList;

   long unsigned int i=0;
   while (i < portList.size()) {
      subList=portList[i]->get_SportList();

      long unsigned int j=0;
      while (j < subList.size()) {
         fullList.push_back(subList[j]);
         j++;
      }

      subList.clear();

      i++;
   }

   sort(fullList.begin(),fullList.end());

   if (fullList.size() == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3112: No S-parameter ports are defined.\n",indent.c_str(),indent.c_str());
      fail=true;
   }

   if (!fail && fullList[0] != 1) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3113: S-parameter ports must start numbering with 1.\n",indent.c_str(),indent.c_str());
      fail=true;
   }

   if (fullList.size() > 0) {
      i=0;
      while (i < fullList.size()-1) {
         if (fullList[i] != fullList[i+1]-1) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3114: S-parameter ports are not numbered sequentially.\n",indent.c_str(),indent.c_str());
            fail=true;
         }
         i++;
      }
   }

   return fail;
}

// Make sure the paths fall within the bounding box
// to catch scaling errors.
bool BoundaryDatabase::check_scale (Mesh *mesh, int order)
{
   bool fail=false;

   Vector lowerLeft,upperRight;
   mesh->GetBoundingBox(lowerLeft,upperRight,max(order,1));

   long unsigned int i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->checkBoundingBox(&lowerLeft,&upperRight,&indent,tol,&pathList)) fail=true;
      i++;
   }

   i=0;
   while (i < portList.size()) {
      if (portList[i]->checkBoundingBox(&lowerLeft,&upperRight,&indent,tol,&pathList)) fail=true;
      i++;
   }

   if (fail) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"         Bounding box: (%g,%g,%g) to (%g,%g,%g)\n",
         lowerLeft[0],lowerLeft[1],lowerLeft[2],upperRight[0],upperRight[1],upperRight[2]);
   }

   return fail;
}

bool BoundaryDatabase::check_overlaps ()
{
   bool fail=false;

   // ports cannot overlap ports
   long unsigned int i=0;
   while (i < portList.size()) {
      long unsigned int j=0;
      while (j < portList.size()) {
         if (i != j && portList[i]->is_overlapPath(pathList[portList[j]->get_pathIndex(0)])) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3115: Port %s and Port %s overlap.\n",
                                                   indent.c_str(),indent.c_str(),portList[i]->get_name().c_str(),portList[j]->get_name().c_str());
            fail=true;
         }
         j++;
      }
      i++;
   }

   // ports cannot overlap boundaries
   i=0;
   while (i < portList.size()) {
      long unsigned int j=0;
      while (j < boundaryList.size()) {
         if (boundaryList[j]->is_overlapPath(&pathList,pathList[portList[i]->get_pathIndex(0)])) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3116: Port %s and Boundary %s overlap.\n",
                                                   indent.c_str(),indent.c_str(),portList[i]->get_name().c_str(),boundaryList[j]->get_name().c_str());
            fail=true;
         }
         j++;
      }
      i++;
   }

   // boundaries cannot overlap boundaries
   i=0;
   while (i < boundaryList.size()) {
      long unsigned int j=0;
      while (j < boundaryList.size()) {
         if (i != j && boundaryList[i]->is_overlapPath(&pathList,pathList[boundaryList[j]->get_pathIndex(0)])) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3117: Boundary %s and Boundary %s overlap.\n",
                                                   indent.c_str(),indent.c_str(),boundaryList[i]->get_name().c_str(),boundaryList[j]->get_name().c_str());
            fail=true;
         }
         j++;
      }
      i++;
   }

   return fail;
}

// Check that there is just one path defining the boundary.
// Used for OpenParEMg.
bool BoundaryDatabase::has_complex_path ()
{
    long unsigned int i=0;
    while (i < portList.size()) {
        if (portList[i]->has_complex_path()) return true;
        i++;
    }


    i=0;
    while (i < boundaryList.size()) {
        if (boundaryList[i]->has_complex_path()) return true;
        i++;
    }

    return false;
}

bool BoundaryDatabase::portNameExists (string name)
{
    long unsigned int i=0;
    while (i < portList.size()) {
        if (portList[i]->get_name().compare(name) == 0) return true;
        i++;
    }
    return false;
}

bool BoundaryDatabase::netNameExists (string name)
{
    long unsigned int i=0;
    while (i < portList.size()) {
        if (portList[i]->netNameExists(name)) return true;
        i++;
    }
    return false;
}

bool BoundaryDatabase::pathNameExists (string name)
{
    long unsigned int i=0;
    while (i < pathList.size()) {
        if (pathList[i]->get_name().compare(name) == 0) return true;
        i++;
    }
    return false;
}

// flip normals to point outward for radiation boundaries
bool BoundaryDatabase::alignRadiationNormals ()
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   bool fail=false;

   // check for each radiation boundary
   long unsigned int i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->is_radiation()) {

         // create a line for the normal
         Path *path1=pathList[boundaryList[i]->get_pathIndex(0)];
         struct point p=path1->get_normal();
         double nx=p.x;
         double ny=p.y;
         double nz=p.z;
         struct point normal=set_point(nx,ny,nz,3);
         struct point a=path1->getInsidePoint();
         struct point b=point_addition(a,point_scale(1e6*pow(path1->area(),1/3.),normal));

         // check for wrong direction
         bool flip=false;
         long unsigned int j=0;
         while (j < boundaryList.size()) {
            if (boundaryList[j]->is_radiation() && j != i) {
               Path *path2=pathList[boundaryList[j]->get_pathIndex(0)];
               if (path2->lineIntersects(a,b)) {
                  flip=true;
                  break;
               }
            }
            j++;
         }

         if (flip) boundaryList[i]->set_normal(-nx,-ny,-nz);
         else boundaryList[i]->set_normal(nx,ny,nz);

         // check that the new direction is clear

         Vector VectorNormal=boundaryList[i]->get_normal();
         normal=set_point(VectorNormal(0),VectorNormal(1),VectorNormal(2),3);
         b=point_addition(a,point_scale(1e6*pow(path1->area(),1/3.),normal));

         flip=false;
         j=0;
         while (j < boundaryList.size()) {
            if (boundaryList[j]->is_radiation() && j != i) {
               Path *path2=pathList[boundaryList[j]->get_pathIndex(0)];
               if (path2->lineIntersects(a,b)) {
                  flip=true;
                  break;
               }
            }
            j++;
         }

         if (flip) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3141: Radiation boundary block at line %d is not on the outside surface.\n",
                                                   boundaryList[i]->get_startLine());
            fail=true;
         }
      }
      i++;
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

bool BoundaryDatabase::is_line ()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      if (portList[i]->is_modal()) return false;
      i++;
   }
   return true;
}

bool BoundaryDatabase::is_modal ()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      if (portList[i]->is_line()) return false;
      i++;
   }
   return true;
}

bool BoundaryDatabase::is_mixed_mode ()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      if (portList[i]->is_mixed_mode()) return true;
      i++;
   }
   return false;
}

bool BoundaryDatabase::create2Dmeshes (int order, ParMesh *mesh3D, vector<ParSubMesh> *parSubMeshesPort)
{
   bool fail=false;
   long unsigned int i=0;
   while (i < portList.size()) {
      if (portList[i]->create2Dmesh(order,mesh3D,parSubMeshesPort,i,tol)) fail=true;
      i++;
   }
   return fail;
}

// assign unique attributes for the ports and boundaries
void BoundaryDatabase::assignAttributes (Mesh *mesh)
{
   // max number of material regions
   int regions=mesh->attributes.Max();

   // 0 is not allowed by MFEM
   // 1 is reserved for PEC as the default
   // so start with 2 unless a default boundary has been set

   int attribute=getLastAttribute()+1;
   if (attribute < 2) attribute=2;

   long unsigned int i=0;
   while (i < portList.size()) {
      long unsigned int j=1;
      while (j <= (long unsigned int)regions) {
         PortAttribute *newPortAttribute=new PortAttribute(attribute,-1);  // not yet assigned to an element; may not use all of these
         portList[i]->push_portAttribute(newPortAttribute);
         attribute++;
         j++;
      }
      i++;
   }

   i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->is_perfect_electric_conductor()) {
         boundaryList[i]->set_attribute(1);  // PEC is always 1
      } else {
         boundaryList[i]->set_attribute(attribute);   // PMC gets an attribute, but nothing is done with it (the natural boundary condition)
         attribute++;
      }
      i++;
   }
}

Boundary* BoundaryDatabase::get_defaultBoundary ()
{
   long unsigned int i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->is_default_boundary()) return boundaryList[i];
      i++;
   }
   return nullptr;
}

// mark the mesh boundaries with attributes into the ports and boundaries
bool BoundaryDatabase::markMeshBoundaries (Mesh *mesh)
{
   DenseMatrix pointMat(3,3);

   // mark everything with a default

   int default_attribute=1;  // PEC
   Boundary *default_boundary=get_defaultBoundary();
   if (default_boundary) {
      default_attribute=default_boundary->get_attribute();
      default_boundary->set_assignedToMesh();
   }

   int i=0;
   while (i < mesh->GetNBE()) {
      mesh->SetBdrAttribute(i,default_attribute);
      i++;
   }

   // set boundaries to ports and non-default boundaries - overwrites the default attribute

   // loop through the mesh boundary elements
   i=0;
   while (i < mesh->GetNBE()) {

      int attribute=-1;
      if (mesh->GetBdrElementType(i) == Element::TRIANGLE) {

         int adjacent_element_number;
         int info;
         mesh->GetBdrElementAdjacentElement (i,adjacent_element_number,info);
         int adjacent_element_attribute=mesh->GetAttribute(adjacent_element_number);

         mesh->GetBdrPointMatrix(i,pointMat);

         // loop through the ports
         long unsigned int j=0;
         while (j < portList.size()) {
            if (portList[j]->is_triangleInside(&pointMat)) {
                attribute=portList[j]->get_attribute(adjacent_element_attribute);
                portList[j]->set_assignedToMesh();
                break;
            }
            j++;
         }

         // loop through the boundaries, excluding the default
         j=0;
         while (j < boundaryList.size()) {
            if (!boundaryList[j]->is_default_boundary()) {
               if (boundaryList[j]->is_triangleInside(&pointMat)) { 
                   attribute=boundaryList[j]->get_attribute();
                   boundaryList[j]->set_assignedToMesh();
                   break;
               }
            }
            j++;
         }
      }

      if (attribute > 0) {
         mesh->SetBdrAttribute(i,attribute);
      }

      i++;
   }

   // check that all ports and boundaries are assigned

   bool missingAssignment=false;

   long unsigned int j=0;
   while (j < portList.size()) {
      if (!portList[j]->is_assignedToMesh()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3181: Port \"%s\" could not be assigned to the mesh boundary.\n",
                                                indent.c_str(),indent.c_str(),portList[j]->get_name().c_str());
         missingAssignment=true;
      }
      j++;
   }

   j=0;
   while (j < boundaryList.size()) {
      if (!boundaryList[j]->is_assignedToMesh()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR3179: Boundary \"%s\" could not be assigned to the mesh boundary.\n",
                                                indent.c_str(),indent.c_str(),boundaryList[j]->get_name().c_str());
         missingAssignment=true;
      }
      j++;
   }

   mesh->SetAttributes(); // recalculates the support data structures

   return missingAssignment;
}

int BoundaryDatabase::getLastAttribute ()
{
   int attribute=-1;

   long unsigned int i=0;
   while (i < portList.size()) {
      if (portList[i]->get_last_attribute() > attribute) attribute=portList[i]->get_last_attribute();
      i++;
   }

   i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->get_attribute() > attribute) attribute=boundaryList[i]->get_attribute();
      i++;
   } 

   return attribute;
}

bool BoundaryDatabase::createDefaultBoundary (struct projectData *projData, Mesh *mesh,
                                               MaterialDatabase *materialDatabase, BoundaryDatabase *boundaryDatabase)
{
   // nothing to do if PEC
   if (strcmp(projData->materials_default_boundary,"PEC") == 0) return false;

   // get the material
   string default_material_name=projData->materials_default_boundary;
   Material *default_material=materialDatabase->get(default_material_name);
   if (! default_material) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,
         "%sERROR3007: \"%s\" specified by material.default.boundary in the project file does not exist in the materials database.\n",
         indent.c_str(),projData->materials_default_boundary);
      return true;
   }

   stringstream ss;
   ss << projData->materials_default_boundary << "_default_boundary";

   // create a Boundary to hold the default boundary - note that this includes a reduced set of information
   int new_attribute=getLastAttribute()+1;
   if (new_attribute < 2) new_attribute=2;  // 0 is not allowed in MFEM, and 1 is reserved for PEC
   Boundary *default_boundary=new Boundary(0,0);
   default_boundary->set_attribute(new_attribute);
   default_boundary->set_name(ss.str());
   default_boundary->set_default_boundary();
   default_boundary->set_type("surface_impedance");
   default_boundary->set_material(default_material_name);
   boundaryList.push_back(default_boundary);

   return false;
}

void BoundaryDatabase::savePortMeshes (MeshMaterialList *materials, vector<ParSubMesh> *parSubMeshesPort)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->saveMesh(materials,&tempDirectory,&((*parSubMeshesPort)[i]));
      i++;
   }
}

// The initial guess is for the first 2D mode.
// Can only provide one initial guess to an eigenvalue solution [OpenParEM2D] that (optionally) produces several modes on output.
void BoundaryDatabase::save2Dsetups (struct projectData *projData, double frequency, GammaDatabase *gammaDatabase)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->save2Dsetup(projData,&tempDirectory,frequency,gammaDatabase->getGamma(portList[i]->get_minSportCount(),1,frequency));
      i++;
   }
}

void BoundaryDatabase::saveModeFiles (struct projectData *projData)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->saveModeFile (projData,&pathList,this);
      i++;
   }
}

// return the boundary that encloses the segment defined by (x1,y1,z1) to (x2,y2,z2) [unrotated]
Boundary* BoundaryDatabase::get_matchBoundary (double x1, double y1, double z1, double x2, double y2, double z2)
{
   long unsigned int i=0;
   while (i < boundaryList.size()) {
      if (!boundaryList[i]->is_default_boundary()) {
         if (boundaryList[i]->is_point_inside(x1,y1,z1) && boundaryList[i]->is_point_inside(x2,y2,z2)) return boundaryList[i];
      }
      i++;
   }
   return nullptr;
}

bool BoundaryDatabase::createPortDirectories ()
{
   bool fail=false;
   long unsigned int i=0;
   while (i < portList.size()) {
      if (portList[i]->createDirectory(&tempDirectory)) fail=true;
      i++;
   }
   return fail;
}

void BoundaryDatabase::extract2Dmesh(ParMesh *pmesh, vector<ParSubMesh> *parSubMeshesPort)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->extract2Dmesh(pmesh,parSubMeshesPort);
      i++;
   }
}

bool BoundaryDatabase::solvePorts (int fem_order, ParMesh *pmesh, vector<ParSubMesh> *parSubMeshesPort, double frequency, MeshMaterialList *meshMaterials,
                                   struct projectData *projData, GammaDatabase *gammaDatabase)
{
   bool fail=false;
   PetscMPIInt rank;
   chrono::duration<double> elapsed;
   chrono::steady_clock::time_point start;
   chrono::steady_clock::time_point current;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (create2Dmeshes(fem_order,pmesh,parSubMeshesPort)) {fail=true; return fail;}

   savePortMeshes(meshMaterials,parSubMeshesPort);
   if (rank == 0) {
      save2Dsetups(projData,frequency,gammaDatabase);
      saveModeFiles (projData);
   }

   long unsigned int i=0;
   while (i < portList.size()) {

      //prefix(); PetscPrintf(PETSC_COMM_WORLD,"         ------------------------------------------------------------------------------------------------------------------------------------\n");
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"         Port \"%s\" ...\n",portList[i]->get_name().c_str());
      //prefix(); PetscPrintf(PETSC_COMM_WORLD,"         ------------------------------------------------------------------------------------------------------------------------------------\n");

      // run the 2D simulation
      if (portList[i]->solve(&tempDirectory)) fail=true;

      // verify that the lock file is removed
      stringstream ssLock;
      ssLock << tempDirectory << "/" << "S" << portList[i]->get_name() << "/" << "." << portList[i]->get_name() << ".lock";
      start=chrono::steady_clock::now();
      while (std::filesystem::exists(ssLock.str().c_str())) {
         current=chrono::steady_clock::now();
         elapsed=current-start;
         if (elapsed.count() > 60) { // wait up to 60 seconds
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3118: OpenParEM2D lock file is present, implying a failed 2D port simulation.\n");
            fail=true;
            break;
         }
      }

      i++;
   }

   if (!fail && loadPortSolutions(frequency)) fail=true;
   if (!fail) populateGamma(frequency,gammaDatabase);

   return fail;
}

bool BoundaryDatabase::loadPortSolutions (double frequency)
{
   bool fail=false;
   long unsigned int i=0;
   while (i < portList.size()) {
      if (portList[i]->loadSolution(&tempDirectory,frequency)) fail=true;
      i++;
   }
   return fail;
}

void BoundaryDatabase::populateGamma (double frequency, GammaDatabase *gammaDatabase)
{
   gammaDatabase->reset();

   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->populateGamma(frequency,gammaDatabase);
      i++;
   }
}

void BoundaryDatabase::printPortSolutions()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->printSolution("   ");
      i++;
   }
}

int BoundaryDatabase::get_totalModeCount()
{
   int count=0;
   long unsigned int i=0;
   while (i < portList.size()) {
      count+=portList[i]->get_modeCount();
      i++;
   }
   return count;
}

int BoundaryDatabase::get_SportCount()
{
   int count=0;
   long unsigned int i=0;
   while (i < portList.size()) {
      if (portList[i]->get_SportCount() > count) count=portList[i]->get_SportCount();
      i++;
   }
   return count;
}

void BoundaryDatabase::set2DModeNumbers()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->set2DModeNumbers();
      i++;
   }
}

void BoundaryDatabase::build_portEssTdofLists (ParFiniteElementSpace *fespace, ParMesh *pmesh)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->build_essTdofList(fespace,pmesh);
      i++;
   }
}

void BoundaryDatabase::fillX (Vec *X, Vec *Xdofs, int drivingSet)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->fillX(X,Xdofs,drivingSet);
      i++;
   }
}

bool BoundaryDatabase::addPortIntegrators (ParMesh *pmesh, ParBilinearForm *pmblf, PWConstCoefficient *Inv_mur, PWConstCoefficient *neg_ko2_Re_er, PWConstCoefficient *neg_ko2_Im_er, 
                                           vector<Array<int> *> &borderAttributesList, vector<ConstantCoefficient *> &ReInvGammaConstList, vector<ConstantCoefficient *> &ImInvGammaConstList,
                                           vector<ConstantCoefficient *> &RekConstList, vector<ConstantCoefficient *> &ImkConstList,
                                           bool isReal, int drivingSport, bool solution_check_homogeneous, string indent)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      if (portList[i]->addPortIntegrators(pmesh,pmblf,Inv_mur,neg_ko2_Re_er,neg_ko2_Im_er,
                                          borderAttributesList,ReInvGammaConstList,ImInvGammaConstList,RekConstList,ImkConstList,
                                          isReal,drivingSport,solution_check_homogeneous,indent)) {
         return true;
      }
      i++;
   }
   return false;
}

void BoundaryDatabase::addImpedanceIntegrators (double frequency, double temperature, ParMesh *pmesh, ParBilinearForm *pmblf, MaterialDatabase *materialDatabase,
                                                vector<Array<int> *> &borderAttributesList, vector<ConstantCoefficient *> &ZconstList, bool isReal)
{
   long unsigned int i=0;
   while (i < boundaryList.size()) {
      boundaryList[i]->addImpedanceIntegrator(frequency,temperature,pmesh,pmblf,materialDatabase,borderAttributesList,ZconstList,isReal);
      i++;
   }
}

bool is_vectorMatch (vector<Mode *> *v1, vector<Mode *> *v2)
{
   if (v1 == nullptr) return false;
   if (v2 == nullptr) return false;
   if (v1->size() != v2->size()) return false;

   long unsigned int i=0;
   while (i < v1->size()) {
      if ((*v1)[i] != (*v2)[i]) return false;
      i++;
   }

   return true;
}

// Driving sets for S-parameters are generalized to enable calculating S-parameters with
// different excitations.
void BoundaryDatabase::createDrivingSets ()
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   // ToDo: create a keyword/value pair so that the driving set can be selected in the project setup file.
   int setType=0;

   int portCount=get_SportCount();

   // driving set "single"
   // drive one port at a time
   if (setType == 0) {

      set_drivingSetName("S-port");

      // create portCount weights per mode form sets of weights
      // initialize all weights to zero
      int i=0;
      while (i < portCount) {
         long unsigned int j=0;
         while (j < portList.size()) {
            portList[j]->addWeight(complex<double>(0,0));
            j++;
         }
         i++;
      }

      // set one Sport driving at a time
      i=0;
      while (i < portCount) {
         Mode *drivingMode=getDrivingMode(i+1);
         drivingMode->setWeight(i,complex<double>(1,0));
         i++;
      }
   }

   // driving set "multiple"
   // drive single-mode ports one port at a time and
   // multimode ports with orthogonal combinations of the port modes
   // Note: not thoroughly tested
   if (setType == 1) {

      set_drivingSetName("set");

      // create portCount weights per mode form sets of weights
      // initialize all weights to zero
      int i=0;
      while (i < portCount) {
         long unsigned int j=0;
         while (j < portList.size()) {
            portList[j]->addWeight(complex<double>(0,0));
            j++;
         }
         i++;
      }

      // keep track of the modes per port

      vector<vector<Mode *> *> modeArray;
      i=0;
      while (i < portCount) {

         vector<Mode *> *modeVector=new vector<Mode *>;

         Mode *modei=getDrivingMode(i+1);
         Port *porti=get_port(modei);

         int j=0;
         while (j < portCount) {
            Mode *modej=getDrivingMode(j+1);
            Port *portj=get_port(modej);
            if (portj == porti) modeVector->push_back(modej);
            j++;
         }

         modeArray.push_back(modeVector);

         i++;
      }

      // count the number of times a particular weight pattern previously appears
      vector<int> counts;
      i=0;
      while (i < portCount) {
         counts.push_back(0);

         long unsigned int j=0;
         while ((int)j < i) {
            if (is_vectorMatch(modeArray[i],modeArray[j])) counts[i]++;
            j++;
         }

         i++;
      }

      // set weights
      i=0;
      while (i < portCount) {
         Mode *modei=getDrivingMode(i+1);

         vector<Mode *> *testVector=modeArray[i];
         long unsigned int j=0;
         while (j < testVector->size()) {
            Mode *modej=(*testVector)[j];
            modei->setWeight(modej->get_Sport()-1,complex<double>(1,0));

            if (counts[i] > 0 && (int)j == counts[i]) {
               modei->setWeight(modej->get_Sport()-1,complex<double>(-1,0));
            }

            j++;
         }

         i++;
      }

      // clean up
      long unsigned int j=0;
      while (j < modeArray.size()) {
         delete modeArray[j];
         j++;
      }
   }

   // driving set "single-ended"
   // drive single-mode ports one port at a time and
   // multimode ports with weights from the Tv vectors
   // to obtain single-ended driving
   if (setType == 2) {

      set_drivingSetName("single-ended");

      // ToDo
   }

   // debug output to verify the sets
   if (false) {
      int i=0;
      while (i < portCount) {
          Mode *modei=getDrivingMode(i+1);
          if (rank == 0) cout << "Sport=" << modei->get_Sport() << " weights:" << endl;
          int j=0;
          while (j < portCount) {
             if (rank == 0) cout << "   " << modei->getWeight(j) << endl;
             j++;
          }
          i++;
      }
   }
}

void BoundaryDatabase::calculateSplits ()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->calculateSplits();
      i++;
   }
}

bool BoundaryDatabase::calculateAcceptedPower (int Sport, complex<double> *acceptedPower)
{
   int portCount=get_SportCount();
   complex<double> powerIn=0;
   complex<double> powerOut=0;

   Mode *mode=getDrivingMode(Sport);
   if (!mode) return true;

   // loop through the ports
   int i=1;
   while (i <= portCount) {
      if (i == Sport) powerIn+=mode->calculatePowerIn(i);
      if (i != Sport) powerOut+=mode->calculatePowerOut(i);
      i++;
   } 

   // accepted power
   // The accepted power is either dissipated or radiated.
   *acceptedPower=powerIn-powerOut;

   return false;
}

PetscErrorCode BoundaryDatabase::calculateS (Result *result)
{
   PetscErrorCode ierr=0;

   int portCount=get_SportCount();
   int size=portCount*portCount;

   // for a*S=b
   lapack_complex_double *a=(lapack_complex_double *) malloc(size*size*sizeof(lapack_complex_double));       // matrix
   lapack_complex_double *S=(lapack_complex_double *) malloc(size*sizeof(lapack_complex_double));            // vector
   lapack_complex_double *b=(lapack_complex_double *) malloc(size*sizeof(lapack_complex_double));            // vector

   matrixZero(a,size);
   vectorZero(S,size);
   vectorZero(b,size);

   // b

   // loop through the drivingSets
   int i=0;
   while (i < portCount) {

      // loop through the S-ports
      int j=0;
      while (j < portCount) {
         Mode *mode=getDrivingMode(j+1);
         complex<double> value=mode->get_Cp(i)*mode->get_voltage()/sqrt(mode->get_impedance());
         vectorSetValue(b,i*portCount+j,real(value),imag(value));
         j++;
      }
      i++;
   }

   // a

   // loop through the drivingSets
   i=0;
   while (i < portCount) {

      // loop through the S-ports
      int j=0;
      while (j < portCount) {

         // loop through the S-ports
         int k=0;
         while (k < portCount) {
            Mode *mode=getDrivingMode(k+1);
            complex<double> value=mode->get_Cm(i)*mode->get_voltage()/sqrt(mode->get_impedance());
            int row=i*portCount+j;
            int col=j*portCount+k;
            matrixSetValue(a,row+col*portCount*portCount,real(value),imag(value));
            k++;
         }

         j++;
      }
      i++;
   }

   // invert
   matrixInverse(a,size);

   // solve for S
   matrixVectorMultiply(a,b,S,size);

   // transfer to a Mat

   Mat *MatS=result->get_S();
   ierr=MatCreate(PETSC_COMM_WORLD,MatS); if (ierr) return ierr;
   ierr=MatSetType(*MatS,MATDENSE); if (ierr) return ierr;
   ierr=MatSetSizes(*MatS,PETSC_DECIDE,PETSC_DECIDE,portCount,portCount); if (ierr) return ierr;
   ierr=MatZeroEntries(*MatS); if (ierr) return ierr;

   PetscInt low,high;
   ierr=MatGetOwnershipRange(*MatS,&low,&high); if (ierr) return ierr;

   int k=0;
   i=0;
   while (i < portCount) {
      int j=0;
      while (j < portCount) {
         PetscScalar Sparam=vectorGetRealValue(S,k)+PETSC_i*vectorGetImagValue(S,k);
         if (i >= low && i < high) {
            ierr=MatSetValue(*MatS,i,j,Sparam,INSERT_VALUES); if (ierr) return ierr;
         }
         j++;
         k++;
      }
      i++;
   }
   ierr=MatAssemblyBegin(*MatS,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;
   ierr=MatAssemblyEnd(*MatS,MAT_FINAL_ASSEMBLY); if (ierr) return ierr;

   // fill out Zo
   i=0;
   while (i < portCount) {
      Mode *mode=getDrivingMode(i+1);
      result->push_Zo(mode->get_impedance());
      i++;
   }

   // clean up
   if (a) {free(a); a=nullptr;}
   if (S) {free(S); S=nullptr;}
   if (b) {free(b); S=nullptr;}

   return ierr;
}

Mode* BoundaryDatabase::getDrivingMode (int drivingSport)
{
   Mode *mode=nullptr;

   long unsigned int i=0;
   while (i < portList.size()) {
      mode=portList[i]->getDrivingMode(drivingSport);
      if (mode) break;
      i++;
   }

   return mode;
}

void BoundaryDatabase::fillIntegrationPoints ()
{
   long unsigned int i=0;
   while (i < portList.size()) {
         portList[i]->fillIntegrationPoints(&pathList);
      i++;
   }
}

void BoundaryDatabase::calculateLineIntegrals (ParMesh *pmesh, fem3D *fem)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->calculateLineIntegrals(pmesh,fem);
      i++;
   }
}

void BoundaryDatabase::alignDirections (ParMesh *pmesh, fem3D *fem)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->alignDirections(pmesh,fem);
      i++;
   }
}

void BoundaryDatabase::build2Dgrids()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->build2Dgrids();
      i++;
   }
}

void BoundaryDatabase::build2DModalGrids()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->build2DModalGrids();
      i++;
   }
}

void BoundaryDatabase::build3Dgrids(ParFiniteElementSpace *fes3D_ND, ParFiniteElementSpace *fes3D_H1)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->build3Dgrids(fes3D_ND,fes3D_H1);
      i++;
   }
}

void BoundaryDatabase::build2DSolutionGrids()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->build2DSolutionGrids();
      i++;
   }
}

void BoundaryDatabase::transfer_2Dsolution_2Dgrids_to_3Dgrids()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->transfer_2Dsolution_2Dgrids_to_3Dgrids();
      i++;
   }
}

void BoundaryDatabase::transfer_2Dsolution_3Dgrids_to_2Dgrids()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->transfer_2Dsolution_3Dgrids_to_2Dgrids();
      i++;
   }
}

void BoundaryDatabase::transfer_3Dsolution_3Dgrids_to_2Dgrids(fem3D *fem)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->transfer_3Dsolution_3Dgrids_to_2Dgrids(fem);
      i++;
   }
}

void BoundaryDatabase::save2DParaView(vector<ParSubMesh> *parSubMeshesPort, struct projectData *projData, double frequency, bool add_extension)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->save2DParaView(&((*parSubMeshesPort)[i]),projData,frequency,add_extension);
      i++;
   }
}

void BoundaryDatabase::save3DParaView(ParMesh *pmesh, struct projectData *projData, double frequency, bool add_extension)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->save3DParaView(pmesh,projData,frequency,add_extension);
      i++;
   }
}

void BoundaryDatabase::save2DSolutionParaView(vector<ParSubMesh> *parSubMeshesPort, struct projectData *projData, double frequency, int drivingSport, bool add_extension)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->save2DSolutionParaView(&((*parSubMeshesPort)[i]),projData,frequency,drivingSport,add_extension);
      i++;
   }
}

void BoundaryDatabase::save2DModalParaView(vector<ParSubMesh> *parSubMeshesPort, struct projectData *projData, double frequency, bool add_extension)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->save2DModalParaView(&((*parSubMeshesPort)[i]),projData,frequency,add_extension);
      i++;
   }
}

void BoundaryDatabase::buildGrids (fem3D *fem)
{
   build3Dgrids(fem->get_fespace_ND(),fem->get_fespace_H1());
   transfer_2Dsolution_2Dgrids_to_3Dgrids();
   build2DModalGrids();
   transfer_2Dsolution_3Dgrids_to_2Dgrids();
   build2DSolutionGrids();
}

bool BoundaryDatabase::solve2Dports (ParMesh *pmesh, vector<ParSubMesh> *parSubMeshesPort, struct projectData *projData,
                                     double frequency, MeshMaterialList *meshMaterials, GammaDatabase *gammaDatabase)
{
   bool fail=false;
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   // extract 2D meshes for the ports from the 3D mesh
   parSubMeshesPort->clear();
   extract2Dmesh(pmesh,parSubMeshesPort);

   // solve 2D ports

   stringstream ssTemp;
   ssTemp << "temp_" << projData->project_name;
   set_tempDirectory(ssTemp.str());

   if (rank == 0) {
      if (std::filesystem::exists(get_tempDirectory().c_str())) {
        std::filesystem::remove_all(get_tempDirectory().c_str());
      }

      // create the temp directory
      if (! std::filesystem::create_directory(get_tempDirectory().c_str())) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3119: Failed to create results directory \"%s\".\n",get_tempDirectory().c_str());
         fail=true;
      }
   }
   if (fail) return fail;

   if (rank == 0 && createPortDirectories()) fail=true;
   if (fail) return fail;

   if (solvePorts(projData->fem_order,pmesh,parSubMeshesPort,frequency,meshMaterials,projData,gammaDatabase)) fail=true;
   if (fail) return fail;

   build2Dgrids();

   save2DParaView(parSubMeshesPort,projData,frequency,false);

   return fail;
}

void BoundaryDatabase::resetElementNumbers ()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->resetElementNumbers();
      i++;
   }
}

bool BoundaryDatabase::snapToMeshBoundary (Mesh *mesh)
{
   bool fail=false;

   long unsigned int i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->snapToMeshBoundary(&pathList,mesh,indent)) fail=true;
      i++;
   }

   i=0;
   while (i < portList.size()) {
      if (portList[i]->snapToMeshBoundary(&pathList,mesh,indent)) fail=true;
      i++;
   }

   return fail;
}

PetscErrorCode BoundaryDatabase::build_M (Mat *M, vector<DifferentialPair *> *differentialPairList, BoundaryDatabase *boundaryDatabase)
{
   PetscErrorCode ierr=0;

   // total number of ports
   int portCount=get_SportCount();

   // M

   ierr=MatCreate(PETSC_COMM_WORLD,M); if (ierr) return 1;
   ierr=MatSetType(*M,MATDENSE); if (ierr) return 2;
   ierr=MatSetSizes(*M,PETSC_DECIDE,PETSC_DECIDE,portCount,portCount); if (ierr) return 3;
   ierr=MatZeroEntries(*M); if (ierr) return 6;

   PetscInt low,high;
   ierr=MatGetOwnershipRange(*M,&low,&high); if (ierr) return 6;

   // set all to single-ended
   int m=0;
   while (m < portCount) {
      if (m >= low && m < high) {
         ierr=MatSetValue(*M,m,m,1.0,INSERT_VALUES); if (ierr) return 7;
      }
      m++;
   }

   // set differential pairs
   long unsigned int i=0;
   while (i < differentialPairList->size()) {
      int p=(*differentialPairList)[i]->get_Sport_P()-1;
      int n=(*differentialPairList)[i]->get_Sport_N()-1;

      // update the net names

      Mode *mode_P=boundaryDatabase->getDrivingMode(p+1);
      Mode *mode_N=boundaryDatabase->getDrivingMode(n+1);

      stringstream ssP,ssN;
      if (mode_P->net_is_loaded()) {ssP << mode_P->get_net(); ssN << mode_P->get_net();}
      else {ssP << "net" << mode_P->get_Sport(); ssN << "net" << mode_P->get_Sport();}
      ssP << "_"; ssN << "_";
      if (mode_N->net_is_loaded()) {ssP << mode_N->get_net(); ssN << mode_N->get_net();}
      else {ssP << "net" << mode_N->get_Sport(); ssP << "net" << mode_N->get_Sport();}
      ssP << "_common"; ssN << "_differential";

      if (!mode_P->get_net_is_updated()) {mode_P->set_net(ssP.str()); mode_P->set_net_is_updated();}
      if (!mode_N->get_net_is_updated()) {mode_N->set_net(ssN.str()); mode_N->set_net_is_updated();}

      i++;
   }

   ierr=MatAssemblyBegin(*M,MAT_FINAL_ASSEMBLY); if (ierr) return 12;
   ierr=MatAssemblyEnd(*M,MAT_FINAL_ASSEMBLY); if (ierr) return 13;

   return ierr;
}

void BoundaryDatabase::aggregateDifferentialPairList (vector<DifferentialPair *> *aggregateList)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->aggregateDifferentialPairList(aggregateList);
      i++;
   }
}

void BoundaryDatabase::reset ()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->reset();
      i++;
   }

   i=0;
   while (i < pathList.size()) {
      pathList[i]->unset_hasOutput();
      i++;
   }

   tempDirectory="";
}

bool BoundaryDatabase::buildAggregateModeList (vector<Mode *> *aggregateModeList)
{
   // build the list
   long unsigned int i=0;
   while (i < portList.size()) {
      portList[i]->buildAggregateModeList(aggregateModeList);
      i++;
   }

   // sort them in order of Sport
   i=0;
   while (i < aggregateModeList->size()-1) {
      long unsigned int j=i+1;
      while (j < aggregateModeList->size()) {
         if ((*aggregateModeList)[j]->get_Sport() < (*aggregateModeList)[i]->get_Sport()) {
            Mode *temp=(*aggregateModeList)[j];
            (*aggregateModeList)[j]=(*aggregateModeList)[i];
            (*aggregateModeList)[i]=temp;
         }
         j++;
      }
      i++;
   }

   // check for inconsistent Sport numbering
   i=0;
   while (i < aggregateModeList->size()) {
      if ((*aggregateModeList)[i]->get_Sport() != (int)i+1) {
         PetscPrintf(PETSC_COMM_WORLD,"ASSERT: BoundaryDatabase::buildAggregateModeList found inconsistent Sport assignments.\n");
         return true;
      }
      i++;
   }

   return false;
}

// given mode, return port and index
bool BoundaryDatabase::get_port_from_mode (Mode *mode, Port **port, long unsigned int *index)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      if (portList[i]->has_mode(mode,index)) {
         *port=portList[i];
         return false;
      }
      i++;
   }
   return true;
}

bool BoundaryDatabase::has_Ti ()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      if (!portList[i]->has_Ti()) return false;
      i++;
   }
   return true;
}

bool BoundaryDatabase::has_Tv ()
{
   long unsigned int i=0;
   while (i < portList.size()) {
      if (!portList[i]->has_Tv()) return false;
      i++;
   }
   return true;
}

Port* BoundaryDatabase::get_port (Mode *mode)
{
   Port *port=nullptr;
   long unsigned int index=0;

   long unsigned int i=0;
   while (i < portList.size()) {
      if (portList[i]->has_mode(mode,&index)) {
         port=portList[i];
         break;
      }
      i++;
   }

   return port;
}

Port* BoundaryDatabase::get_port (std::string portName)
{
   long unsigned int i=0;
   while (i < portList.size()) {
      if (portList[i]->get_name().compare(portName) == 0) return portList[i];
      i++;
   }
   return nullptr;
}

bool BoundaryDatabase::hasRadiationBoundary ()
{
   long unsigned int i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->is_radiation()) return true;
      i++;
   }
   return false;
}

bool BoundaryDatabase::calculateRadiationCurrents (ParMesh *pmesh, struct projectData *projData, Vector center, double radiation_beta, double lengthLimit,
                              ParGridFunction *gridReE, ParGridFunction *gridImE, ParGridFunction *gridReH, ParGridFunction *gridImH)
{
   bool fail=false;

   long unsigned int i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]->calculateRadiationCurrents(pmesh,projData,center,radiation_beta,lengthLimit,gridReE,gridImE,gridReH,gridImH)) fail=true;
      i++;
   }

   return fail;
}

void BoundaryDatabase::collectRadiationCurrents ()
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   int varCount=16;

   // collect at rank 0
   long unsigned int i=0;
   while (i < boundaryList.size()) {
      boundaryList[i]->collectRadiationCurrents(&radiationCurrents);
      i++;
   }

   // distribute
   if (rank == 0) {

      int length=varCount*radiationCurrents.size();
      double *pack=(double *)malloc(length*sizeof(double));
      long unsigned int i=0;
      while (i < radiationCurrents.size()) {
         pack[varCount*i+0]=radiationCurrents[i]->get_x();
         pack[varCount*i+1]=radiationCurrents[i]->get_y();
         pack[varCount*i+2]=radiationCurrents[i]->get_z();
         pack[varCount*i+3]=radiationCurrents[i]->get_area();
         pack[varCount*i+4]=real(radiationCurrents[i]->get_Jx());
         pack[varCount*i+5]=imag(radiationCurrents[i]->get_Jx());
         pack[varCount*i+6]=real(radiationCurrents[i]->get_Jy());
         pack[varCount*i+7]=imag(radiationCurrents[i]->get_Jy());
         pack[varCount*i+8]=real(radiationCurrents[i]->get_Jz());
         pack[varCount*i+9]=imag(radiationCurrents[i]->get_Jz());
         pack[varCount*i+10]=real(radiationCurrents[i]->get_Mx());
         pack[varCount*i+11]=imag(radiationCurrents[i]->get_Mx());
         pack[varCount*i+12]=real(radiationCurrents[i]->get_My());
         pack[varCount*i+13]=imag(radiationCurrents[i]->get_My());
         pack[varCount*i+14]=real(radiationCurrents[i]->get_Mz());
         pack[varCount*i+15]=imag(radiationCurrents[i]->get_Mz());
         i++;
      }

      int j=1;
      while (j < size) {
         MPI_Send(&length,1,MPI_INT,j,2000,PETSC_COMM_WORLD);
         MPI_Send(pack,length,MPI_DOUBLE,j,2001,PETSC_COMM_WORLD);
         j++;
      }

      if (pack) free(pack);
   } else {
      int length;
      MPI_Recv(&length,1,MPI_INT,0,2000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      double *pack=(double *)malloc(length*sizeof(double));
      MPI_Recv(pack,length,MPI_DOUBLE,0,2001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      int k=0;
      while (k < length/varCount) {
         Current *current=new Current(pack[k*varCount+0],
                                      pack[k*varCount+1],
                                      pack[k*varCount+2],
                                      std::complex(pack[k*varCount+4],pack[k*varCount+5]),
                                      std::complex(pack[k*varCount+6],pack[k*varCount+7]),
                                      std::complex(pack[k*varCount+8],pack[k*varCount+9]),
                                      std::complex(pack[k*varCount+10],pack[k*varCount+11]),
                                      std::complex(pack[k*varCount+12],pack[k*varCount+13]),
                                      std::complex(pack[k*varCount+14],pack[k*varCount+15]),
                                      pack[k*varCount+3]);
         radiationCurrents.push_back(current);
         k++;
      }

      if (pack) free(pack);
   }
}

void BoundaryDatabase::deleteRadiationCurrents ()
{
   long unsigned int i=0;
   while (i < boundaryList.size()) {
      boundaryList[i]->deleteRadiationCurrents();
      i++;
   }

   i=0;
   while (i < radiationCurrents.size()) {
      delete radiationCurrents[i];
      i++;
   }

   radiationCurrents.clear();
}

// Vector r is the far-field point; vecr is the vector location and r is the magnitude
// Vector rp is the point on the antenna; vecrp is the vector location and rp is the magnitude
//
// variables and formulas follow:
// Constantine Balanis, Advanced Engineering Electromagnetics, John Wiley and Sons, 1989, pp. 288-289.
void BoundaryDatabase::calculateFarField (double r, Vector center, double radiation_beta, double wave_impedance,
                                          vector<OPEMpoint *> *pointList, long unsigned int start, long unsigned int stop)
{
   complex<double> Ntheta,Nphi,Ltheta,Lphi;
   complex<double> Jx,Jy,Jz,Mx,My,Mz;
   double cos_psi;
   complex<double> exp_rp;

   double inv_wave_impedance=1.0/wave_impedance;

   // pre-calculation to speed up inner loop
   vector<double> xp,yp,zp,rp,inv_rp;
   long unsigned int k=0;
   while (k < radiationCurrents.size()) {
      xp.push_back(radiationCurrents[k]->get_x()-center(0));
      yp.push_back(radiationCurrents[k]->get_y()-center(1));
      zp.push_back(radiationCurrents[k]->get_z()-center(2));
      rp.push_back(sqrt(xp[k]*xp[k]+yp[k]*yp[k]+zp[k]*zp[k]));
      inv_rp.push_back(1.0/rp[k]);
      k++;
   }

   complex<double> j=complex<double>(0,1);
   long unsigned int i=start;
   while (i < stop) {

      // get the far-field point
      double x=(*pointList)[i]->get_x()-center(0);
      double y=(*pointList)[i]->get_y()-center(1);
      double z=(*pointList)[i]->get_z()-center(2);
      r=sqrt(pow(x,2)+pow(y,2)+pow(z,2)); // should equal the passed r value, ToDo: check
      double inv_r=1.0/r;

      double phi=atan2(y,x);
      double theta=acos(z/r);

      double cos_theta=cos(theta);
      double sin_theta=sin(theta);
      double cos_phi=cos(phi);
      double sin_phi=sin(phi);

      complex<double> exp_r=j*radiation_beta*exp(-j*radiation_beta*r)/(4*M_PI*r);

      complex<double> Ephi=0;
      complex<double> Etheta=0;
      complex<double> Hphi=0;
      complex<double> Htheta=0;

      // inner loop simplified for speed
      long unsigned int k=0;
      while (k < radiationCurrents.size()) {

         Jx=radiationCurrents[k]->get_Jx();
         Jy=radiationCurrents[k]->get_Jy();
         Jz=radiationCurrents[k]->get_Jz();

         Mx=radiationCurrents[k]->get_Mx();
         My=radiationCurrents[k]->get_My();
         Mz=radiationCurrents[k]->get_Mz();

         // psi is angle between r and rp
         // only cos(psi) is needed
         cos_psi=(x*xp[k]+y*yp[k]+z*zp[k])*inv_r*inv_rp[k];

         exp_rp=exp(j*radiation_beta*rp[k]*cos_psi)*radiationCurrents[k]->get_area();

         Ntheta=Jx*cos_theta*cos_phi+Jy*cos_theta*sin_phi-Jz*sin_theta;
         Nphi=-Jx*sin_phi+Jy*cos_phi;
         Ltheta=Mx*cos_theta*cos_phi+My*cos_theta*sin_phi-Mz*sin_theta;
         Lphi=-Mx*sin_phi+My*cos_phi;

         Etheta-=(Lphi+wave_impedance*Ntheta)*exp_rp;
         Ephi+=(Ltheta-wave_impedance*Nphi)*exp_rp;
         Htheta+=(Nphi-Ltheta*inv_wave_impedance)*exp_rp;
         Hphi-=(Ntheta+Lphi*inv_wave_impedance)*exp_rp;

         k++;
      }
      (*pointList)[i]->set_fields(exp_r*Ephi,exp_r*Etheta,exp_r*Hphi,exp_r*Htheta);
      i++;
   }
}

void BoundaryDatabase::calculateFarField (double r, Vector center, double radiation_beta, double wave_impedance, vector<OPEMpoint *> *pointList)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   if (rank == 0) {
      int chunk=pointList->size()/size;
      if (chunk == 0) chunk=1;
      vector<int> start,stop;
      start.push_back(0);
      stop.push_back(chunk);

      int i=1;
      while (i < size) {
         int nextstart=start[i-1]+chunk;
         if (nextstart > (int)pointList->size()) nextstart=pointList->size();

         int nextstop=nextstart+chunk;
         if (nextstop > (int)pointList->size()) nextstop=pointList->size();
          
         start.push_back(nextstart);
         stop.push_back(nextstop);

         i++;
      }
      stop[size-1]=pointList->size();

      // distribute the work
      i=1;
      while (i < size) {
         MPI_Send(&(start[i]),1,MPI_INT,i,100,PETSC_COMM_WORLD);
         MPI_Send(&(stop[i]),1,MPI_INT,i,101,PETSC_COMM_WORLD);
         i++;
      }

      // do the local work
      calculateFarField(r,center,radiation_beta,wave_impedance,pointList,start[0],stop[0]);

      // collect the data
      i=1;
      while (i < size) {
         int j=start[i];
         while (j < stop[i]) {
            (*pointList)[j]->recvFieldsFrom(i);
            j++;
         }
         i++;
      }

   } else {

      // get the assigned range to calculate
      int start,stop;
      MPI_Recv(&start,1,MPI_INT,0,100,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&stop,1,MPI_INT,0,101,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      // do the work
      calculateFarField(r,center,radiation_beta,wave_impedance,pointList,start,stop);

      // send the results to rank 0
      int i=start;
      while (i < stop) {
         (*pointList)[i]->sendFieldsTo(0);
         i++;
      }
   }

   // distribute across ranks

   if (rank == 0) {
      int i=1;
      while (i < size) {
         long unsigned int j=0;
         while (j < pointList->size()) {
            (*pointList)[j]->sendFieldsTo(i);
            j++;
         }
         i++;
      }
   } else {
      long unsigned int j=0;
      while (j < pointList->size()) {
         (*pointList)[j]->recvFieldsFrom(0);
         j++;
      }
   }
}

void BoundaryDatabase::deletePort (string name)
{
    long unsigned int i=0;
    while (i < portList.size()) {
        if (portList[i]->get_name().compare(name) == 0) {
            delete portList[i];
            portList.erase(portList.begin()+i);
            modified=true;
            break;
        }
        i++;
    }
}

// Straight lines do no have normals, but a normal is needed to orient arrow heads for display in OpenParEMg.
// Look for paths without normals and assign the normal from the associated port to the path.
void BoundaryDatabase::assignPathNormals ()
{
    long unsigned int i=0;
    while (i < portList.size()) {
        portList[i]->assignPathNormals(&pathList);
        i++;
    }
}

Port* BoundaryDatabase::get_matchingPort (Path *path)
{
    long unsigned int i=0;
    while (i < portList.size()) {
        if (portList[i]->isPathInside(path)) return portList[i];
        i++;
    }
    return nullptr;
}

#ifdef HAS_GUI
void BoundaryDatabase::draw (Relay *relay, struct projectData *projData, CustomOpenGLWidget *drawingWindow, QTreeWidget *drawingItemTree,
                             CustomTreeWidgetItem *pathTreeItem, CustomTreeWidgetItem *portTreeItem, CustomTreeWidgetItem *boundaryTreeItem,
                             MaterialDatabase *materialDatabase)
{
    //emit relay->triggered();

    // paths
    long unsigned int i=0;
    while (i < pathList.size()) {
        pathList[i]->create_item(drawingWindow,pathTreeItem);
        i++;
    }

    // ports
    i=0;
    while (i < portList.size()) {
        // create the AIS_Shape and the item, but the shape must be added to the drawing elsewhere
        portList[i]->draw(relay,projData,this,drawingWindow,drawingItemTree,pathTreeItem,portTreeItem);
        i++;
    }

    // boundaries
    i=0;
    while (i < boundaryList.size()) {
        boundaryList[i]->draw(relay,projData,this,drawingWindow,drawingItemTree,boundaryTreeItem,materialDatabase);
        i++;
    }
}

// assumes that the port is in the boundary database
void BoundaryDatabase::draw_port (Relay *relay, Port *port, struct projectData *projData, CustomOpenGLWidget *drawingWindow, QTreeWidget *drawingItemTree,
                                  CustomTreeWidgetItem *pathTreeItem, CustomTreeWidgetItem *portTreeItem, CustomTreeWidgetItem *boundaryTreeItem, MaterialDatabase *materialDatabase)
{
    port->draw(relay,projData,this,drawingWindow,drawingItemTree,pathTreeItem,portTreeItem);
}

// void BoundaryDatabase::set_drawingToItemMap (std::unordered_map<Handle(AIS_Shape), CustomTreeWidgetItem*> *drawingToItemMap)
// {
//     // ports
//     long unsigned int i=0;
//     while (i < portList.size()) {
//         portList[i]->set_drawingToItemMap(drawingToItemMap);
//         i++;
//     }

//     // boundaries
//     i=0;
//     while (i < boundaryList.size()) {
//         boundaryList[i]->set_drawingToItemMap(drawingToItemMap);
//         i++;
//     }
//}

void BoundaryDatabase::crossLink ()
{
    long unsigned int i=0;
    while (i < portList.size()) {
        portList[i]->crossLink(&pathList);
        i++;
    }
}

#endif

BoundaryDatabase::~BoundaryDatabase ()
{
   long unsigned int i=0;
   while (i < sourceFileList.size()) {
      if (sourceFileList[i]) {
         delete sourceFileList[i];
         sourceFileList[i]=nullptr;
      }
      i++;
   }

   i=0;
   while (i < portList.size()) {
      if (portList[i]) {
         delete portList[i];
         portList[i]=nullptr;
      }
      i++;
   }

   i=0;
   while (i < boundaryList.size()) {
      if (boundaryList[i]) {
         delete boundaryList[i];
         boundaryList[i]=nullptr;
      }
      i++;
   }

   i=0;
   while (i < pathList.size()) {
      if (pathList[i]) {
         delete pathList[i];
         pathList[i]=nullptr;
      }
      i++;    
   }

   i=0;
   while (i < radiationCurrents.size()) {
      delete radiationCurrents[i];
      radiationCurrents[i]=nullptr;
      i++;
   }

}

