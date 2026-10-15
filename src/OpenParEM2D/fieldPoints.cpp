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

#include "fieldPoints.hpp"

void FieldPoint::copy(FieldPoint *a)
{
   frequency=a->frequency;
   mode=a->mode;
   dim=a->dim;
   x=a->x;
   y=a->y;
   z=a->z;
   overwrite(a);
}

// everything except the fields
bool FieldPoint::compare (FieldPoint *a)
{
   if (!double_compare(frequency,a->frequency,tolerance)) return false;
   if (mode != a->mode) return false;
   if (!double_compare(x,a->x,tolerance)) return false;
   if (!double_compare(y,a->y,tolerance)) return false;
   if (dim == 3 && !double_compare(z,a->z,tolerance)) return false;

   return true;
}

void FieldPoint::overwrite (FieldPoint *a)
{
   Ex_re=a->Ex_re;
   Ex_im=a->Ex_im;
   Ey_re=a->Ey_re;
   Ey_im=a->Ey_im;
   Ez_re=a->Ez_re;
   Ez_im=a->Ez_im;

   Hx_re=a->Hx_re;
   Hx_im=a->Hx_im;
   Hy_re=a->Hy_re;
   Hy_im=a->Hy_im;
   Hz_re=a->Hz_re;
   Hz_im=a->Hz_im;
}

void FieldPoint::save(ofstream *out)
{
   *out << frequency << "," << mode+1 << "," << x << "," << y << ",";
   if (dim == 3) *out << z << ",";
   *out << setprecision(15) << Ex_re << "," << Ex_im << ",";
   *out << setprecision(15) << Ey_re << "," << Ey_im << ",";
   *out << setprecision(15) << Ez_re << "," << Ez_im << ",";
   *out << setprecision(15) << Hx_re << "," << Hx_im << ",";
   *out << setprecision(15) << Hy_re << "," << Hy_im << ",";
   *out << setprecision(15) << Hz_re << "," << Hz_im << endl;
}

void FieldPoint::save_field_component (ofstream *out, const char *casename, int *casenumber, const char *field_component, double mag, double phi, double theta, double xVal, double yVal)
{
   if (mag > fieldMagLimit) {

      // mag
      *out << casename <<  "_" << frequency_unique_index+1 << "_" << mode+1 << "_" << (*casenumber)++ << "_field" << ",";
      *out << frequency << "," << mode+1 << "," << field_component << "mag," << x << "," << y << "," << "equal," << setprecision(15) << mag << "," << setprecision(15) << equalErrorLimit << endl;

      // phi
      if (sqrt(xVal*xVal+yVal*yVal) > fieldMagLimit) {
         if (fabs(phi) > fieldMagLimit) {
            *out << casename <<  "_" << frequency_unique_index+1 << "_" << mode+1 << "_" << (*casenumber)++ << "_field" << ",";
            *out << frequency << "," << mode+1 << "," << field_component << "phi," << x << "," << y << "," << "equal," << setprecision(15) << phi << "," << setprecision(15) << equalErrorLimit << endl;
         } else {
            *out << casename <<  "_" << frequency_unique_index+1 << "_" << mode+1 << "_" << (*casenumber)++ << "_field" << ",";
            *out << frequency << "," << mode+1 << "," << field_component << "phi," << x << "," << y << "," << "lessthan," << setprecision(15) << lessthanErrorLimit << endl;
         }
      }

      // theta
      if (fabs(theta) > fieldMagLimit) {
         *out << casename <<  "_" << frequency_unique_index+1 << "_" << mode+1 << "_" << (*casenumber)++ << "_field" << ",";
         *out << frequency << "," << mode+1 << "," << field_component << "theta," << x << "," << y << "," << "equal," << setprecision(15) << theta << "," << setprecision(15) << equalErrorLimit << endl;
      } else {
         *out << casename <<  "_" << frequency_unique_index+1 << "_" << mode+1 << "_" << (*casenumber)++ << "_field" << ",";
         *out << frequency << "," << mode+1 << "," << field_component << "theta," << x << "," << y << "," << "lessthan," << setprecision(15) << lessthanErrorLimit << endl;
      }

   } else {

      // mag
      *out << casename <<  "_" << frequency_unique_index+1 << "_" << mode+1 << "_" << (*casenumber)++ << "_field" << ",";
      *out << frequency << "," << mode+1 << "," << field_component << "mag," << x << "," << y << "," << "lessthan," << setprecision(15) << lessthanErrorLimit << endl;

   }
}

void FieldPoint::save_as_test(ofstream *out, const char *casename, int *casenumber)
{
   double Emag_re,Emag_im,Ephi_re,Ephi_im,Etheta_re,Etheta_im;
   double Hmag_re,Hmag_im,Hphi_re,Hphi_im,Htheta_re,Htheta_im;

   // spherical coordinates
   // phi is the angle of the vector projected onto the x-y plane to the x-axis
   // theta is the angle from the z-axis to the vector

   Emag_re=sqrt(Ex_re*Ex_re+Ey_re*Ey_re+Ez_re*Ez_re);
   Ephi_re=atan2(Ey_re,Ex_re);
   Etheta_re=atan2(sqrt(Ex_re*Ex_re+Ey_re*Ey_re),Ez_re);
   save_field_component (out,casename,casenumber,"real_E",Emag_re,Ephi_re,Etheta_re,Ex_re,Ey_re);

   Emag_im=sqrt(Ex_im*Ex_im+Ey_im*Ey_im+Ez_im*Ez_im);
   Ephi_im=atan2(Ey_im,Ex_im);
   Etheta_im=atan2(sqrt(Ex_im*Ex_im+Ey_im*Ey_im),Ez_im);
   save_field_component (out,casename,casenumber,"imag_E",Emag_im,Ephi_im,Etheta_im,Ex_im,Ey_im);


   Hmag_re=sqrt(Hx_re*Hx_re+Hy_re*Hy_re+Hz_re*Hz_re);
   Hphi_re=atan2(Hy_re,Hx_re);
   Htheta_re=atan2(sqrt(Hx_re*Hx_re+Hy_re*Hy_re),Hz_re);
   save_field_component (out,casename,casenumber,"real_H",Hmag_re,Hphi_re,Htheta_re,Hx_re,Hy_re);

   Hmag_im=sqrt(Hx_im*Hx_im+Hy_im*Hy_im+Hz_im*Hz_im);
   Hphi_im=atan2(Hy_im,Hx_im);
   Htheta_im=atan2(sqrt(Hx_im*Hx_im+Hy_im*Hy_im),Hz_im);
   save_field_component (out,casename,casenumber,"imag_H",Hmag_im,Hphi_im,Htheta_im,Hx_im,Hy_im);

}

void FieldPoint::print()
{
   cout << "FieldPoint:" << endl;
   cout << "   frequency=" << frequency << endl;
   cout << "   mode=" << mode << endl;
   cout << "   x=" << x << endl;
   cout << "   y=" << y << endl;
   if (dim == 3) cout << "   z=" << z << endl;
   cout << "   Ex_re=" << Ex_re << endl;
   cout << "   Ex_im=" << Ex_im << endl;
   cout << "   Ey_re=" << Ey_re << endl;
   cout << "   Ey_im=" << Ey_im << endl;
   cout << "   Ez_re=" << Ez_re << endl;
   cout << "   Ez_im=" << Ez_im << endl;
   cout << "   Hx_re=" << Hx_re << endl;
   cout << "   Hx_im=" << Hx_im << endl;
   cout << "   Hy_re=" << Hy_re << endl;
   cout << "   Hy_im=" << Hy_im << endl;
   cout << "   Hz_re=" << Hz_re << endl;
   cout << "   Hz_im=" << Hz_im << endl;
   cout << "   tolerance=" << tolerance << endl;
}

void FieldPointDatabase::push(FieldPoint *a)
{
   // see if the point already exists and overwrite it if it does
   bool found=false;
   unsigned long int i=0;
   while (i < fieldPointList.size()) {
      if (fieldPointList[i]->compare(a)) {
         fieldPointList[i]->overwrite(a);
         found=true;
         break;
      }
      i++;
   }

   // keep it if it is new
   if (! found) {
      FieldPoint *newPoint=new FieldPoint();
      newPoint->copy(a);
      fieldPointList.push_back(newPoint);
   }
}

void FieldPointDatabase::save(struct projectData *projData, const char *baseName)
{
   if (fieldPointList.size() == 0) return;

   stringstream ss;
   ss << baseName << "_fields.csv";

   ofstream out;
   out.open(ss.str().c_str(),ofstream::out);
   if (out.is_open()) {
      out << "#OpenParEM2D " << projData->version_major << "." << projData->version_minor << "." << projData->version_patch << endl;
      out << "#frequency,mode,x,y,Ex_re,Ex_im,Ey_re,Ey_im,Ez_re,Ez_im,Hx_re,Hx_im,Hy_re,Hy_im,Hz_re,Hz_im" << endl;
      unsigned long int i=0;
      while (i < fieldPointList.size()) {
         fieldPointList[i]->save(&out);
         i++;
      } 
      out.close();
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2149: Could not open file \"%s\" for writing.\n",ss.str().c_str());
   }
}

// append to the file
void FieldPointDatabase::save_as_test(const char *baseName, const char *casename)
{
   stringstream ss;
   ss << baseName << "_prototype_test_cases.csv";
   int casenumber=0;
   if (fieldPointList.size() == 0) return;

   ofstream out;
   out.open(ss.str().c_str(),ofstream::app);
   if (out.is_open()) {
      out << "# FieldPointDatabase::save_as_test" << endl;

      unsigned long int i=0;
      while (i < fieldPointList.size()) {
         fieldPointList[i]->save_as_test(&out,casename,&casenumber);
         i++;
      }
      out.close();
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2150: Could not open file \"%s\" for writing.\n",ss.str().c_str());
   }
}

void FieldPointDatabase::normalize()
{
   vector<double> frequencies;
   vector<unsigned long int> modes;

   // find the unique frequencies
   unsigned long int i=0;
   while (i < fieldPointList.size()) {
      bool found=false;
      unsigned long int j=0;
      while (j < frequencies.size()) {
         if (fabs((fieldPointList[i]->get_frequency()-frequencies[j])/frequencies[j]) < fieldPointList[i]->get_tolerance()) {found=true; break;}
         j++;  
      }
      if (! found) {
         frequencies.push_back(fieldPointList[i]->get_frequency());
      }
      i++;
   }

   // set frequency_unique_index for use in writing out test cases
   i=0;
   while (i < frequencies.size()) {
      unsigned long int j=0;
      while (j < fieldPointList.size()) {
         if (fabs((fieldPointList[j]->get_frequency()-frequencies[i])/frequencies[i]) < fieldPointList[j]->get_tolerance()) {
            fieldPointList[j]->set_frequency_unique_index(i);
         }
         j++;
      }
      i++;
   }

   // find the largest mode number at each frequency
   i=0;
   while (i < frequencies.size()) {
      unsigned long int mode=0;
      unsigned long int j=0;
      while (j < fieldPointList.size()) {
         if (fabs((fieldPointList[j]->get_frequency()-frequencies[i])/frequencies[i]) < fieldPointList[j]->get_tolerance()) {
            if (fieldPointList[j]->get_mode() > mode) mode=fieldPointList[j]->get_mode();
         }
         j++;
      }
      modes.push_back(mode);
      i++;
   }

   // normalize the field solutions for all (x,y) at each frequency and mode
   // use the field at the first (x,y) found in the database
   // normalize by the largest field component
   i=0;
   while (i < frequencies.size()) {

      unsigned long int j=0;
      while (j <= modes[i]) {

         complex<double> scale=complex<double>(0,0);
         unsigned long int k=0;
         while (k < fieldPointList.size()) {
            if (fabs((fieldPointList[k]->get_frequency()-frequencies[i])/frequencies[i]) < fieldPointList[k]->get_tolerance() &&
                fieldPointList[k]->get_mode() == j) {
               double Ex_re,Ex_im,Ey_re,Ey_im,Ez_re,Ez_im,Hx_re,Hx_im,Hy_re,Hy_im,Hz_re,Hz_im;
               fieldPointList[k]->get(&Ex_re,&Ex_im,&Ey_re,&Ey_im,&Ez_re,&Ez_im,&Hx_re,&Hx_im,&Hy_re,&Hy_im,&Hz_re,&Hz_im);

               complex<double> Ex=complex<double>(Ex_re,Ex_im);
               complex<double> Ey=complex<double>(Ey_re,Ey_im);
               complex<double> Ez=complex<double>(Ez_re,Ez_im);
               complex<double> Hx=complex<double>(Hx_re,Hx_im);
               complex<double> Hy=complex<double>(Hy_re,Hy_im);
               complex<double> Hz=complex<double>(Hz_re,Hz_im);

               if (scale == complex<double>(0,0)) {
                  scale=Ex;
                  if (abs(Ey) > abs(scale)) scale=Ey;
                  if (abs(Ez) > abs(scale)) scale=Ez;
                  if (abs(Hx) > abs(scale)) scale=Hx;
                  if (abs(Hy) > abs(scale)) scale=Hy;
                  if (abs(Hz) > abs(scale)) scale=Hz;
               }

               fieldPointList[k]->set(real(Ex/scale),imag(Ex/scale),real(Ey/scale),imag(Ey/scale),real(Ez/scale),imag(Ez/scale),
                                      real(Hx/scale),imag(Hx/scale),real(Hy/scale),imag(Hy/scale),real(Hz/scale),imag(Hz/scale));
            }
            k++;
         }
         j++;
      }
      i++;
   }
}

void FieldPointDatabase::print()
{
   long unsigned int i=0;
   while (i < fieldPointList.size()) {
      fieldPointList[i]->print();
      i++;
   }
}

FieldPointDatabase::~FieldPointDatabase ()
{
   long unsigned int i=0;
   while (i < fieldPointList.size()) {
      delete fieldPointList[i];
      i++;
   }
}

