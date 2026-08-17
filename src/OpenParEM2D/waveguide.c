////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    waveguide - A calculator for several waveguide types.                   //
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

#include "waveguide.h"

int RECTANGULAR_WAVEGUIDE=0;
int HALF_RECTANGULAR_WAVEGUIDE=0;
int PARTIALLY_FILLED=1;
int COAX=0;
int EIGHTH_COAX=0;

int show_csv_data=1;
int show_fields=0;
int include_losses=0;
int show_steps=0;

// general settings - modified for COAX
double resultMagLimit=1e-8;             // breakover point for "equal" vs. "lessthan" comparisons for results
double fieldMagLimit=1e-3;              // breakover point for "equal" vs. "lessthan" comparisons for fields
double resultEqualErrorLimit=1e-5;      // see also waveguide.c, results.cpp, fieldPoints.cpp
double fieldEqualErrorLimit=3e-3;       // see also waveguide.c, results.cpp, fieldPoints.cpp
double resultLessthanErrorLimit=1e-8;   // see also waveguide.c, results.cpp, fieldPoints.cpp
double fieldLessthanErrorLimit=1e-3;    // see also waveguide.c, results.cpp, fieldPoints.cpp

// global for formatting output with show_csv_data
double current_freq=-1;

void fields_print_line (const char *project, int ifreq, int mode, int *casenum, double frequency, const char *comp, double x, double y, double mag, double phi, double theta, double xVal, double yVal)
{
   if (! show_fields) return;

   if (mag > fieldMagLimit) {

      printf ("%s_%d_%d_%d_field_exact,%g,%d,%smag,%g,%g,equal,%.15g,%.15g\n",project,ifreq,mode,(*casenum)++,frequency,mode,comp,x,y,mag,fieldEqualErrorLimit);

      if (sqrt(xVal*xVal+yVal*yVal) > fieldMagLimit) {
         if (fabs(phi) > fieldMagLimit) printf ("%s_%d_%d_%d_field_exact,%g,%d,%sphi,%g,%g,equal,%.15g,%.15g\n",project,ifreq,mode,(*casenum)++,frequency,mode,comp,x,y,phi,fieldEqualErrorLimit);
         else printf ("%s_%d_%d_%d_field_exact,%g,%d,%sphi,%g,%g,lessthan,%.15g\n",project,ifreq,mode,(*casenum)++,frequency,mode,comp,x,y,fieldLessthanErrorLimit);
      } else {
         // skip since the angle can be anything
      }

      if (fabs(theta) > fieldMagLimit) printf ("%s_%d_%d_%d_field_exact,%g,%d,%stheta,%g,%g,equal,%.15g,%.15g\n",project,ifreq,mode,(*casenum)++,frequency,mode,comp,x,y,theta,fieldEqualErrorLimit);
      else printf ("%s_%d_%d_%d_field_exact,%g,%d,%stheta,%g,%g,lessthan,%.15g\n",project,ifreq,mode,(*casenum)++,frequency,mode,comp,x,y,fieldLessthanErrorLimit);

   } else {
      printf ("%s_%d_%d_%d_field_exact,%g,%d,%smag,%g,%g,lessthan,%.15g\n",project,ifreq,mode,(*casenum)++,frequency,mode,comp,x,y,fieldLessthanErrorLimit);
      //printf ("%s_%d_%d_%d_field_exact,%g,%d,%sphi,%g,%g,lessthan,%.15g\n",project,ifreq,mode,(*casenum)++,frequency,mode,comp,x,y,fieldLessthanErrorLimit);     // skip since the angle can be anything
      //printf ("%s_%d_%d_%d_field_exact,%g,%d,%stheta,%g,%g,lessthan,%.15g\n",project,ifreq,mode,(*casenum)++,frequency,mode,comp,x,y,fieldLessthanErrorLimit);   // skip since the angle can be anything
   }
}

void fields_print_line_raw (const char *project, int ifreq, int mode, int *casenum, double frequency, const char *comp, double x, double y, double valX, double valY, double valZ)
{
   printf ("%s_%d_%d_%d_field_exact,%g,%d,%sx,%g,%g,equal,%.15g,1e-10\n",project,ifreq,mode,(*casenum)++,frequency,mode,comp,x,y,valX);
   printf ("%s_%d_%d_%d_field_exact,%g,%d,%sy,%g,%g,equal,%.15g,1e-10\n",project,ifreq,mode,(*casenum)++,frequency,mode,comp,x,y,valY);
   printf ("%s_%d_%d_%d_field_exact,%g,%d,%sz,%g,%g,equal,%.15g,1e-10\n",project,ifreq,mode,(*casenum)++,frequency,mode,comp,x,y,valZ);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------
// Coax
//--------------------------------------------------------------------------------------------------------------------------------------------------------------

void coax_fields (double a, double b, double x, double y, double freq, double impedance, double *scale,
                           const char *project, int ifreq, int *casenum)
{
   double pi=4.*atan(1.);
   double r=sqrt(x*x+y*y);
   double Vo=1;
   double Io=Vo/impedance;
   int mode=1;

   double Ex,Ey,Hx,Hy;
   double Emag,Hmag,Ephi,Hphi,Etheta,Htheta;

   // fields

   Ex=Vo/(r*log(b/a))*x/r;
   Ey=Vo/(r*log(b/a))*y/r;

   Hx=-Io/(2*pi*r)*y/r;
   Hy=Io/(2*pi*r)*x/r;

   if (*scale == 0) {
       *scale=Ex;
       if (fabs(Ey) > fabs(*scale)) *scale=Ey;
       if (fabs(Hx) > fabs(*scale)) *scale=Hx;
       if (fabs(Hy) > fabs(*scale)) *scale=Hy;
   }

   // spherical coordinates
   // phi is the angle of the vector projected onto the x-y plane to the x-axis 
   // theta is the angle from the z-axis to the vector

   Emag=sqrt(Ex*Ex+Ey*Ey)/(*scale);
   Hmag=sqrt(Hx*Hx+Hy*Hy)/(*scale);;

   Ephi=atan2(Ey,Ex);
   Hphi=atan2(Hy,Hx);

   Etheta=pi/2;
   Htheta=pi/2;

   fields_print_line (project, ifreq, mode, casenum, freq, "real_E", x, y, Emag, Ephi, Etheta, Ex, Ey);
   fields_print_line (project, ifreq, mode, casenum, freq, "imag_E", x, y, 0, 0, pi/2, 0, 0);

   fields_print_line (project, ifreq, mode, casenum, freq, "real_H", x, y, Hmag, Hphi, Htheta, Hx, Hy);
   fields_print_line (project, ifreq, mode, casenum, freq, "imag_H", x, y, 0, 0 , pi/2, 0, 0);

}

double coax_impedance (double a, double b, double er)
{
   double pi,mu0,eps0,eta;

   pi=4.*atan(1.);
   mu0=4e-7*pi;
   eps0=8.8541878176e-12;
   eta=sqrt(mu0/(er*eps0));

   return eta/(2*pi)*log(b/a);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------
// Rectangular Waveguide
//--------------------------------------------------------------------------------------------------------------------------------------------------------------

void rectWaveguide_fields (double freq, int m, int n, double x, double y, double er, double mur, double beta, double alpha, double kx, double ky, double kc, double complex *scale,
                           const char *project, int ifreq, int mode, int *casenum, char *wavetype)
{
   double pi,eps0,mu0,w;
   double complex B,Ex,Ey,Ez,Hx,Hy,Hz;
   double Emag_re,Ephi_re,Etheta_re,Emag_im,Ephi_im,Etheta_im;
   double Hmag_re,Hphi_re,Htheta_re,Hmag_im,Hphi_im,Htheta_im;

   pi=4.*atan(1.);
   eps0=8.8541878176e-12;
   mu0=4e-7*pi;
   w=2*pi*freq;

   Ex=CMPLX(0,0);
   Ey=CMPLX(0,0);
   Ez=CMPLX(0,0);
   Hx=CMPLX(0,0);
   Hy=CMPLX(0,0);
   Hz=CMPLX(0,0);

   B=CMPLX(0,1);
   double complex kz=CMPLX(beta,-alpha);

   if (strcmp(wavetype,"TE") == 0) {
      Ex=CMPLX(0,1)*w*mu0*mur*ky/(kc*kc)*B*ccos(kx*x)*csin(ky*y);
      Ey=-CMPLX(0,1)*w*mu0*mur*kx/(kc*kc)*B*csin(kx*x)*ccos(ky*y);
      Ez=CMPLX(0,0);  // TE to Z

      Hx=CMPLX(0,1)*kz*kx/(kc*kc)*B*csin(kx*x)*ccos(ky*y);
      Hy=CMPLX(0,1)*kz*ky/(kc*kc)*B*ccos(kx*x)*csin(ky*y);
      Hz=B*ccos(kx*x)*ccos(ky*y);
   } else if (strcmp(wavetype,"TM") == 0) {
      Ex=-CMPLX(0,1)*kz*kx/(kc*kc)*B*ccos(kx*x)*csin(ky*y);
      Ey=-CMPLX(0,1)*kz*ky/(kc*kc)*B*csin(kx*x)*ccos(ky*y);
      Ez=B*csin(kx*x)*csin(ky*y);

      Hx=CMPLX(0,1)*w*eps0*er*ky/(kc*kc)*B*csin(kx*x)*ccos(ky*y);
      Hy=-CMPLX(0,1)*w*eps0*er*kx/(kc*kc)*B*ccos(kx*x)*csin(ky*y);
      Hz=CMPLX(0,0);  // TM to Z
   } else {
      printf ("ERROR2275: Invalid wave type \"%s\".\n",wavetype);
   }

   if (*scale == CMPLX(0,0)) {
       *scale=Ex;
       if (cabs(Ey) > cabs(*scale)) *scale=Ey;
       if (cabs(Ez) > cabs(*scale)) *scale=Ez;
       if (cabs(Hx) > cabs(*scale)) *scale=Hx;
       if (cabs(Hy) > cabs(*scale)) *scale=Hy;
       if (cabs(Hz) > cabs(*scale)) *scale=Hz;
   }

   Ex/=(*scale);
   Ey/=(*scale);
   Ez/=(*scale);

   Hx/=(*scale);
   Hy/=(*scale);
   Hz/=(*scale);

   // spherical coordinates

   Emag_re=sqrt(pow(creal(Ex),2)+pow(creal(Ey),2)+pow(creal(Ez),2));
   Ephi_re=atan2(creal(Ey),creal(Ex));
   Etheta_re=atan2(sqrt(pow(creal(Ex),2)+pow(creal(Ey),2)),creal(Ez));

   Emag_im=sqrt(pow(cimag(Ex),2)+pow(cimag(Ey),2)+pow(cimag(Ez),2));
   Ephi_im=atan2(cimag(Ey),cimag(Ex));
   Etheta_im=atan2(sqrt(pow(cimag(Ex),2)+pow(cimag(Ey),2)),cimag(Ez));

   Hmag_re=sqrt(pow(creal(Hx),2)+pow(creal(Hy),2)+pow(creal(Hz),2));
   Hphi_re=atan2(creal(Hy),creal(Hx));
   Htheta_re=atan2(sqrt(pow(creal(Hx),2)+pow(creal(Hy),2)),creal(Hz));

   Hmag_im=sqrt(pow(cimag(Hx),2)+pow(cimag(Hy),2)+pow(cimag(Hz),2));
   Hphi_im=atan2(cimag(Hy),cimag(Hx));
   Htheta_im=atan2(sqrt(pow(cimag(Hx),2)+pow(cimag(Hy),2)),cimag(Hz));

   fields_print_line (project, ifreq, mode, casenum, freq, "real_E", x, y, Emag_re, Ephi_re, Etheta_re, creal(Ex), creal(Ey));
   fields_print_line (project, ifreq, mode, casenum, freq, "imag_E", x, y, Emag_im, Ephi_im, Etheta_im, cimag(Ex), cimag(Ey));

   fields_print_line (project, ifreq, mode, casenum, freq, "real_H", x, y, Hmag_re, Hphi_re, Htheta_re, creal(Hx), creal(Hy));
   fields_print_line (project, ifreq, mode, casenum, freq, "imag_H", x, y, Hmag_im, Hphi_im, Htheta_im, cimag(Hx), cimag(Hy));

   // raw data dump
   //fields_print_line_raw (project, ifreq, mode, casenum, freq, "real_E", x, y, creal(Ex), creal(Ey), creal(Ez));
   //fields_print_line_raw (project, ifreq, mode, casenum, freq, "imag_E", x, y, cimag(Ex), cimag(Ey), cimag(Ez));

   //fields_print_line_raw (project, ifreq, mode, casenum, freq, "real_H", x, y, creal(Hx), creal(Hy), creal(Hz));
   //fields_print_line_raw (project, ifreq, mode, casenum, freq, "imag_H", x, y, cimag(Hx), cimag(Hy), cimag(Hz));
}

void rectWaveguide_result_print_line (const char *project, int ifreq, int mode, int *casenum, double frequency, const char *comp, double target)
{
   printf ("%s_%d_%d_%d_result_exact,%.15g,%d,%s,",project,ifreq,mode,(*casenum)++,frequency,mode,comp);

   if (fabs(target) > resultMagLimit) {
      printf ("equal,%.15g,%.15g\n",target,resultEqualErrorLimit);
   } else {
      printf ("lessthan,%.15g\n",resultLessthanErrorLimit);
   }
}

// all frequencies in Hz
void rectWaveguide_gamma (struct rectWaveguide *a, double k, double eta, double freq, int m, int n, double impedance_scale,
                          const char *project, int ifreq, int mode, int *casenum)
{
   double pi,ko,kx,ky,kc,fc,mu0,eps0;
   double temp;
   double ZTE,lambda;
   double sigma,Rs;
   double alpha,beta,impedance,lossless_alpha;

   double x,y;

   pi=4.*atan(1.);
   mu0=4e-7*pi;
   eps0=8.8541878176e-12;

   impedance=-1;

   kx=m*pi/a->width;
   ky=n*pi/a->height;
   kc=sqrt(kx*kx+ky*ky);

   // cutoff frequency
   fc=kc/k*freq;

   if (freq <= fc) {
      temp=1-pow(freq/fc,2);
      if (temp < 0) temp=0;
      alpha=kc*sqrt(temp);
      lossless_alpha=alpha;
      beta=0;
   } else {
      temp=1-pow(fc/freq,2);
      if (temp < 0) temp=0;
      alpha=0;
      lossless_alpha=alpha;
      beta=k*sqrt(temp);

      // using definition of 1/2*V^2/Pavg
      // impedance for TE10 mode
      if (a->width > a->height && m == 1 && n == 0) {
         lambda=2*pi/k;
         ZTE=eta/sqrt(1-pow(lambda/(2*a->width),2));
         impedance=0.5*pow(a->height,2)/(a->width*a->height/(4*ZTE));
         impedance*=impedance_scale;   // for when symmetry is utilized
      }

      if (a->width < a->height && m == 0 && n == 1) {
         lambda=2*pi/k;
         ZTE=eta/sqrt(1-pow(lambda/(2*a->height),2));
         impedance=0.5*pow(a->width,2)/(a->width*a->height/(4*ZTE));
         impedance*=impedance_scale;   // for when symmetry is utilized
      }

      // TE modes
      if (include_losses) {
         sigma=5.813e7;   // pure copper
         Rs=sqrt(pi*freq*4e-7*pi/sigma);
         alpha=2*Rs/(a->height*eta*sqrt(1-pow(fc/freq,2)))*((1+a->height/a->width)*pow(fc/freq,2)+(1-pow(fc/freq,2))*
                (a->height/a->width*(a->height/a->width*m*m+n*n)/(pow(a->height*m/a->width,2)+n*n)));

         // TEm0 modes
         alpha=Rs/(a->height*eta*sqrt(1-pow(fc/freq,2)))*(1+2*a->height/a->width*pow(fc/freq,2));
      }
   }

   ko=2*pi*freq*sqrt(mu0*eps0);

   if (show_csv_data) {
      if (fabs(freq-current_freq)/freq > 1e-9) {current_freq=freq; printf ("\n%g",freq);}
      printf (",%d,%d,%.15g,%.15g,%.15g",m,n,-alpha,beta,impedance);
   } else {
      printf ("# m=%1d n=%1d: alpha(dB/m)=%.15g, beta/1000(rad/m)=%.15g, beta/ko=%.15g, impedance=%.15g\n",
                 m,n,alpha*20*log10(exp(1)),beta/1000,beta/ko,impedance);

      rectWaveguide_result_print_line (project, ifreq, mode, casenum, freq, "alpha", alpha*20*log10(exp(1)));
      rectWaveguide_result_print_line (project, ifreq, mode, casenum, freq, "beta", beta/ko);
      if (fabs(impedance+1) > 1e-14) rectWaveguide_result_print_line (project, ifreq, mode, casenum, freq, "real_Z", impedance);
      if (fabs(impedance+1) > 1e-14) rectWaveguide_result_print_line (project, ifreq, mode, casenum, freq, "imag_Z", 0);
   }

   // use the lossless alpha for the fields
   if (mode == 1 || mode == 2 || (RECTANGULAR_WAVEGUIDE && mode == 3) || mode == 4) {
      double complex scale=CMPLX(0,0);

      if (!show_csv_data) printf ("# TE\n");

      x=a->width/5*2; y=a->height/5*2;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");


      x=a->width/5*1; y=a->height/5*1;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");

      x=a->width/5*2; y=a->height/5*1;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");

      x=a->width/5*3; y=a->height/5*1;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");

      x=a->width/5*4; y=a->height/5*1;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");


      x=a->width/5*1; y=a->height/5*2;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");

      x=a->width/5*2; y=a->height/5*2;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");

      x=a->width/5*3; y=a->height/5*2;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");

      x=a->width/5*4; y=a->height/5*2;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");


      x=a->width/5*1; y=a->height/5*3;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");

      x=a->width/5*2; y=a->height/5*3;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");

      x=a->width/5*3; y=a->height/5*3;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");

      x=a->width/5*4; y=a->height/5*3;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");


      x=a->width/5*1; y=a->height/5*4;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");

      x=a->width/5*2; y=a->height/5*4;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");

      x=a->width/5*3; y=a->height/5*4;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");

      x=a->width/5*4; y=a->height/5*4;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TE");
   }

   if ((HALF_RECTANGULAR_WAVEGUIDE && mode == 3) || (RECTANGULAR_WAVEGUIDE && mode == 5)) {

      double complex scale=CMPLX(0,0);

      if (! show_csv_data) printf ("# TM\n");

      x=a->width/5*2; y=a->height/5*2;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");


      x=a->width/5*1; y=a->height/5*1;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");

      x=a->width/5*2; y=a->height/5*1;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");

      x=a->width/5*3; y=a->height/5*1;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");

      x=a->width/5*4; y=a->height/5*1;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");


      x=a->width/5*1; y=a->height/5*2;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");

      x=a->width/5*2; y=a->height/5*2;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");

      x=a->width/5*3; y=a->height/5*2;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");

      x=a->width/5*4; y=a->height/5*2;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");


      x=a->width/5*1; y=a->height/5*3;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");

      x=a->width/5*2; y=a->height/5*3;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");

      x=a->width/5*3; y=a->height/5*3;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");

      x=a->width/5*4; y=a->height/5*3;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");


      x=a->width/5*1; y=a->height/5*4;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");

      x=a->width/5*2; y=a->height/5*4;
      rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");

      x=a->width/5*3; y=a->height/5*4;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");

      x=a->width/5*4; y=a->height/5*4;
      if (RECTANGULAR_WAVEGUIDE) rectWaveguide_fields (freq, m, n, x, y, a->epsr, a->mur, beta, lossless_alpha, kx, ky, kc, &scale, project, ifreq, mode, casenum, "TM");
   }

   return;
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------
// Partially-Filled Rectangular Waveguide
//--------------------------------------------------------------------------------------------------------------------------------------------------------------

void partiallyFilledWaveguide_fields (struct partiallyFilledRectWaveguide *a, double freq,
                                      int m, double alpha, double beta, int TETMselection, double x, double y, double complex *scale, const char *project, int ifreq, int mode, int *casenum)
{
   double pi=4.*atan(1.);
   double mu0=4e-7*pi;
   double eps0=8.8541878176e-12;
   double w=2*pi*freq;

   double kx=m*pi/a->width;
   double k1=w*sqrt(a->epsr1*eps0*a->mur1*mu0);
   double k2=w*sqrt(a->epsr2*eps0*a->mur2*mu0);

   double complex kz=CMPLX(beta,-alpha);
   double complex ky1=csqrt(k1*k1-kx*kx-kz*kz);
   double complex ky2=csqrt(k2*k2-kx*kx-kz*kz);
   double complex Ex,Ey,Ez,Hx,Hy,Hz;
   double complex C1,C2;

   double Emag_re,Ephi_re,Etheta_re,Emag_im,Ephi_im,Etheta_im;
   double Hmag_re,Hphi_re,Htheta_re,Hmag_im,Hphi_im,Htheta_im;

   if (TETMselection == 0) { // TM-to-y
      C1=CMPLX(0,1)*ccos(ky2*(a->height-a->thickness));
      C2=CMPLX(0,1)*ccos(ky1*a->thickness);

      if (y <= a->thickness) {
         Ex=-C1/(CMPLX(0,1)*w*a->epsr1*eps0)*kx*ky1*csin(ky1*y)*ccos(kx*x);
         Ey=C1/(CMPLX(0,1)*w*a->epsr1*eps0)*(k1*k1-ky1*ky1)*ccos(ky1*y)*sin(kx*x);
         Ez=C1/(w*a->epsr1*eps0)*ky1*kz*csin(ky1*y)*sin(kx*x);

         Hx=C1*CMPLX(0,1)*kz*ccos(ky1*y)*sin(kx*x);
         Hy=0;  // TM-to-y
         Hz=C1*kx*ccos(ky1*y)*ccos(kx*x);
      } else {
         Ex=C2/(CMPLX(0,1)*w*a->epsr2*eps0)*kx*ky2*csin(ky2*(a->height-y))*ccos(kx*x);
         Ey=C2/(CMPLX(0,1)*w*a->epsr2*eps0)*(k2*k2-ky2*ky2)*ccos(ky2*(a->height-y))*sin(kx*x);
         Ez=-C2/(w*a->epsr2*eps0)*ky2*kz*csin(ky2*(a->height-y))*sin(kx*x);

         Hx=C2*CMPLX(0,1)*kz*ccos(ky2*(a->height-y))*sin(kx*x);
         Hy=0;  // TM-to-y
         Hz=C2*kx*ccos(ky2*(a->height-y))*ccos(kx*x);
      }
   } else { // TE-to-y
      C1=CMPLX(0,1)*csin(ky2*(a->height-a->thickness));
      C2=CMPLX(0,1)*csin(ky1*a->thickness);

      if (y <= a->thickness) {
         Ex=-C1*CMPLX(0,1)*kz*csin(ky1*y)*ccos(kx*x);
         Ey=0;  // TE-to-y
         Ez=C1*kx*csin(ky1*y)*csin(kx*x);

         Hx=-C1/(CMPLX(0,1)*w*a->mur1*mu0)*kx*ky1*ccos(ky1*y)*csin(kx*x);
         Hy=C1/(CMPLX(0,1)*w*a->mur1*mu0)*(k1*k1-ky1*ky1)*csin(ky1*y)*ccos(kx*x);
         Hz=-C1/(w*a->mur1*mu0)*kz*ky1*ccos(ky1*y)*ccos(kx*x);
      } else {
         Ex=-C2*CMPLX(0,1)*kz*csin(ky2*(a->height-y))*ccos(kx*x);
         Ey=0;  // TE-to-y
         Ez=C2*kx*csin(ky2*(a->height-y))*csin(kx*x);

         Hx=C2/(CMPLX(0,1)*w*a->mur2*mu0)*kx*ky2*ccos(ky2*(a->height-y))*csin(kx*x);
         Hy=C2/(CMPLX(0,1)*w*a->mur2*mu0)*(k2*k2-ky2*ky2)*csin(ky2*(a->height-y))*ccos(kx*x);
         Hz=C2/(w*a->mur2*mu0)*kz*ky2*ccos(ky2*(a->height-y))*ccos(kx*x);
      }
   }

   if (*scale == CMPLX(0,0)) {
      *scale=Ex;
      if (cabs(Ey) > cabs(*scale)) *scale=Ey;
      if (cabs(Ez) > cabs(*scale)) *scale=Ez;
      if (*scale == CMPLX(0,0)) *scale=CMPLX(1,0);
   }

   Ex/=(*scale);
   Ey/=(*scale);
   Ez/=(*scale);

   Hx/=(*scale);
   Hy/=(*scale);
   Hz/=(*scale);

   // spherical coordinates

   Emag_re=sqrt(pow(creal(Ex),2)+pow(creal(Ey),2)+pow(creal(Ez),2));
   Ephi_re=atan2(creal(Ey),creal(Ex));
   Etheta_re=atan2(sqrt(pow(creal(Ex),2)+pow(creal(Ey),2)),creal(Ez));

   Emag_im=sqrt(pow(cimag(Ex),2)+pow(cimag(Ey),2)+pow(cimag(Ez),2));
   Ephi_im=atan2(cimag(Ey),cimag(Ex));
   Etheta_im=atan2(sqrt(pow(cimag(Ex),2)+pow(cimag(Ey),2)),cimag(Ez));

   Hmag_re=sqrt(pow(creal(Hx),2)+pow(creal(Hy),2)+pow(creal(Hz),2));
   Hphi_re=atan2(creal(Hy),creal(Hx));
   Htheta_re=atan2(sqrt(pow(creal(Hx),2)+pow(creal(Hy),2)),creal(Hz));

   Hmag_im=sqrt(pow(cimag(Hx),2)+pow(cimag(Hy),2)+pow(cimag(Hz),2));
   Hphi_im=atan2(cimag(Hy),cimag(Hx));
   Htheta_im=atan2(sqrt(pow(cimag(Hx),2)+pow(cimag(Hy),2)),cimag(Hz));


   fields_print_line (project, ifreq, mode, casenum, freq, "real_E", x, y, Emag_re, Ephi_re, Etheta_re, creal(Ex), creal(Ey));
   fields_print_line (project, ifreq, mode, casenum, freq, "imag_E", x, y, Emag_im, Ephi_im, Etheta_im, cimag(Ex), cimag(Ey));

   fields_print_line (project, ifreq, mode, casenum, freq, "real_H", x, y, Hmag_re, Hphi_re, Htheta_re, creal(Hx), creal(Hy));
   fields_print_line (project, ifreq, mode, casenum, freq, "imag_H", x, y, Hmag_im, Hphi_im, Htheta_im, cimag(Hx), cimag(Hy));

}

// mode error for partially filled rectangular waveguide
// TETMselection = 0 for TM-to-y, const1=eps1, const2=eps2
// TETMselection = 1 for TE-to-y, const1=mu1, const2=mu2
double error (double kzreal, double a, double d, double kx, double k1, double k2, double const1, double const2, int TETMselection)
{
   double err;
   double complex ky1,ky2;
   double complex kz;

   kz=CMPLX(kzreal,0);
   if (kzreal < 0) kz=CMPLX(0,-kzreal);

   ky1=csqrt(k1*k1-kx*kx-kz*kz);
   ky2=csqrt(k2*k2-kx*kx-kz*kz);

   if (TETMselection == 0) {
      err=creal(ky1/const1*ctan(ky1*d)+ky2/const2*ctan(ky2*(a-d)));
   } else {
      err=creal(ky1/const1/ctan(ky1*d)+ky2/const2/ctan(ky2*(a-d)));
   }

   return err;
}


// all for air
// all frequencies in Hz
// TETMselection = 0 for TM-to-y modes, const1=eps1, const2=eps2
// TETMselection = 1 for TE-to-y modes, const1=mu1, const2=mu2
void partiallyFilledRectWaveguide_gamma (struct partiallyFilledRectWaveguide *a, double freq, int m, int n, int TETMselection, const char *project, int ifreq, int mode, int *casenum)
{
   double pi,w;
   double k1,k2;
   double kx;
   double mu0;
   double eps0;
   double limit,step,err0,err1,err2,err3;
   int count;
   double kz1,kz2,kztest;
   double const1,const2;
   double ko;
   double alpha,beta;

   pi=4.*atan(1.);
   mu0=4e-7*pi;
   eps0=8.8541878176e-12;
   w=2*pi*freq;
   ko=w*sqrt(mu0*eps0);

   kz1=0;
   kz2=0;
   err3=0;

   kx=m*pi/a->width;
   k1=w*sqrt(a->epsr1*eps0*a->mur1*mu0);
   k2=w*sqrt(a->epsr2*eps0*a->mur2*mu0);

   if (TETMselection == 0) {
      const1=a->epsr1;
      const2=a->epsr2;
   } else {
      const1=a->mur1;
      const2=a->mur2;
   }

   limit=k1;
   if (k2 > k1) limit=k2;
   step=limit/10000; // 10000
   limit*=100;
   if (show_steps) {limit/=50; step=limit/100;} 
   beta=limit;
   alpha=0;

   err1=error(beta, a->height, a->thickness, kx, k1, k2, const1, const2, TETMselection);
   if (show_steps) printf ("beta/ko=%g err1=%g count=kickoff\n",beta/ko,err1);

   // find the nth zero crossing
   count=0;
   while (count < n) {

      beta-=step;
      err2=error(beta, a->height, a->thickness, kx, k1, k2, const1, const2, TETMselection);
      if (show_steps) printf ("beta/ko=%g err2=%g count=%d\n",beta/ko,err2,count);

      if ((err1 >= 0 && err2 <= 0) || (err1 <= 0 && err2 >= 0)) {

         // part of check to avoid sign swaps due to poles
         err0=err1;
         if (fabs(err2) > fabs(err0)) err0=err2;

         // refine with binary search

         kz1=beta;
         err1=error(kz1, a->height, a->thickness, kx, k1, k2, const1, const2, TETMselection);

         kz2=beta+step;
         err2=error(kz2, a->height, a->thickness, kx, k1, k2, const1, const2, TETMselection);

         while (fabs((kz2-kz1)/kz1) > 1e-15) {
            kztest=(kz1+kz2)/2;
            err3=error(kztest, a->height, a->thickness, kx, k1, k2, const1, const2, TETMselection);
            if (show_steps) printf ("   kztest/ko=%g err3=%g fabs((kz2-kz1)/kz1)=%g\n",kztest/ko,err3,fabs((kz2-kz1)/kz1));

            if ((err3 >= 0 && err1 <= 0) || (err3 <= 0 && err1 >= 0)) {
               kz2=kztest;
               err2=err3;
            } else if ((err3 >= 0 && err2 <= 0) || (err3 <= 0 && err2 >= 0)) {
              kz1=kztest;
              err1=err3;
            } else {
               printf ("ERROR2276: Algorithm failure 2.\n");
               exit (1);
            }
         }

         // check solution
         if (show_steps) printf ("   check: err0=%g err3=%g\n",err0,err3);
         if (fabs(err3) < fabs(err0)) count++;

         // continue
         err2=error(beta, a->height, a->thickness, kx, k1, k2, const1, const2, TETMselection);
      }
 
      err1=err2;
   }

   beta=(kz1+kz2)/2;
   if (beta < 0) {
      alpha=-beta;
      beta=0;
   }

   if (show_csv_data) {
      double impedance=-1;
      if (fabs(freq-current_freq)/freq > 1e-9) {current_freq=freq; printf ("\n%g",freq);}
      printf (",%d,%d,%.15g,%.15g,%.15g",m,n,-alpha,beta,impedance);
   } else {
      rectWaveguide_result_print_line (project, ifreq, mode, casenum, freq, "alpha", alpha*20*log10(exp(1)));
      rectWaveguide_result_print_line (project, ifreq, mode, casenum, freq, "beta", beta/ko);
   }

   // show fields
   // 0.02, 0.009; 0.0045

   double complex scale=CMPLX(0,0);
   double x,y;

   x=0.009; y=0.003;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);


   x=0.004; y=0.002;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);

   x=0.004; y=0.004;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);

   x=0.004; y=0.005;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);

   x=0.004; y=0.007;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);


   x=0.011; y=0.002;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);

   x=0.011; y=0.004;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);

   x=0.011; y=0.005;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);

   x=0.011; y=0.007;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);


   x=0.016; y=0.002;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);

   x=0.016; y=0.004;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);

   x=0.016; y=0.005;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);

   x=0.016; y=0.007;
   partiallyFilledWaveguide_fields (a, freq, m, alpha, beta, TETMselection, x, y, &scale, project, ifreq, mode, casenum);




   return;
} 

//--------------------------------------------------------------------------------------------------------------------------------------------------------------
// Main
//--------------------------------------------------------------------------------------------------------------------------------------------------------------

int main ()
{
   double frequency;
   int m,n;

//----------------------------------------------------------------------------------------------------------
// Rectangular Waveguide
//----------------------------------------------------------------------------------------------------------

   if (RECTANGULAR_WAVEGUIDE) {
      //air:
      //Recommended Frequency Band:8.20 to 12.40 GHz
      //Cutoff Frequency of Lowest Order Mode:6.557 GHz
      //Cutoff Frequency of Upper Mode:13.114 GHz
      //Dimension:0.9 Inches [22.86 mm] x 0.4 Inches [10.16 mm]

      struct rectWaveguide WR90;
      WR90.width=0.02286;
      WR90.height=0.01016;

      // for air
      WR90.epsr=1.0006;
      WR90.mur=1;

      // other
      WR90.mur=2;

      double pi=4.*atan(1.);
      double mu0=4e-7*pi;
      double eps0=8.8541878176e-12;
      double k;
      double eta=sqrt(WR90.mur*mu0/(WR90.epsr*eps0));

      int ifreq,mode;
      int casenum=0;

      double impedance_scale=1;

      /* //for sweeping through modes to get setups to get mode ordering
      ifreq=1; frequency=9e9; k=2*pi*frequency*sqrt(a->mur*mu0*a->epsr*eps0);
      mode=0;
      m=0;
      while (m < 4) {
         n=0;
         while (n < 4) {
            if (!(m == 0 && n == 0)) {
               printf ("m=%d n=%d\n",m,n);
               rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,"test_sweep",ifreq,mode++,&casenum);
            }
            n++;
         }
         m++;
      }
      */

      const char project[8]="WR90";
      ifreq=1;
      frequency=9e9;
      while (frequency < 11e9*(1+1e-12)) {

         k=2*pi*frequency*sqrt(WR90.mur*mu0*WR90.epsr*eps0);
         mode=1;

         m=1; n=0;
         rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,project,ifreq,mode++,&casenum);

         m=2; n=0;
         rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,project,ifreq,mode++,&casenum);

         m=0; n=1;
         rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,project,ifreq,mode++,&casenum);

         m=1; n=1;  // TM
         rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,project,ifreq,mode++,&casenum);

         m=1; n=1; // TE
         rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,project,ifreq,mode++,&casenum);

         //m=3; n=0;
         //rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,project,ifreq,mode++,&casenum);

         ifreq++;
         frequency+=0.25e9;
      }
   } // RECTANGULAR_WAVEGUIDE

//----------------------------------------------------------------------------------------------------------
// Half Rectangular Waveguide
//----------------------------------------------------------------------------------------------------------

   if (HALF_RECTANGULAR_WAVEGUIDE) {
      //Recommended Frequency Band:8.20 to 12.40 GHz
      //Cutoff Frequency of Lowest Order Mode:6.557 GHz
      //Cutoff Frequency of Upper Mode:13.114 GHz
      //Dimension:0.9 Inches [22.86 mm] x 0.4 Inches [10.16 mm]

      struct rectWaveguide WR90;
      WR90.width=0.02286;
      WR90.height=0.01016;

      // for air
      WR90.epsr=1.0006;
      WR90.mur=1;

      // other
      //WR90.mur=2;

      double pi=4.*atan(1.);
      double mu0=4e-7*pi;
      double eps0=8.8541878176e-12;
      double k;
      double eta=sqrt(WR90.mur*mu0/(WR90.epsr*eps0));

      int ifreq,mode;
      int casenum=0;

      double impedance_scale=2;   // PMC boundary is used to cut the problem in half

      /* //for sweeping through modes to get setups to get mode ordering
      ifreq=1; frequency=9e9; k=2*pi*frequency*sqrt(a->mur*mu0*a->epsr*eps0);
      mode=0;
      m=0;
      while (m < 4) {
         n=0;
         while (n < 4) {
            if (!(m == 0 && n == 0)) {
               printf ("m=%d n=%d\n",m,n);
               rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,"test_sweep",ifreq,mode++,&casenum);
            }
            n++;
         }
         m++;
      }
      */

      const char project[8]="WR90";
      ifreq=1;
      frequency=9e9;
      while (frequency < 10e9*(1+1e-12)) {

         k=2*pi*frequency*sqrt(WR90.mur*mu0*WR90.epsr*eps0);
         mode=1;

         m=1; n=0;
         rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,project,ifreq,mode++,&casenum);

         //m=2; n=0;
         //rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,project,ifreq,mode++,&casenum);

         //m=0; n=1;
         //rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,project,ifreq,mode++,&casenum);

         m=1; n=1;  // TE
         rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,project,ifreq,mode++,&casenum);

         m=1; n=1; // TM
         rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,project,ifreq,mode++,&casenum);

         m=3; n=0;
         rectWaveguide_gamma (&WR90,k,eta,frequency,m,n,impedance_scale,project,ifreq,mode++,&casenum);

         ifreq++;
         frequency+=1e9;
      }
   } // HALF_RECTANGULAR_WAVEGUIDE

//----------------------------------------------------------------------------------------------------------
// Partially-Filled Rectangular Waveguide
//----------------------------------------------------------------------------------------------------------

   if (PARTIALLY_FILLED) {

      // Roger F. Harrington, "Time-Harmonic Electromagnetic Fields", 1961.
      // Example shown in Fig. 4-7 on page 161.

      struct partiallyFilledRectWaveguide Harrington;
      Harrington.width=0.02;
      Harrington.height=Harrington.width*0.45;
      Harrington.thickness=Harrington.height*0.5;
      Harrington.epsr1=2.45;
      Harrington.epsr2=1;
      Harrington.mur1=1;
      Harrington.mur2=1;

      //Harrington.mur1=2;

      int TM=0;
      int TE=1;

      int ifreq,mode;
      int casenum=0;

      /* for sweeping through modes to get setups to get mode ordering
      ifreq=1; frequency=9e9;
      mode=0;
      m=0;
      while (m < 3) {
         n=1;
         while (n < 4) {
            if (!show_csv_data) printf ("m=%d n=%d TM\n",m,n);
            partiallyFilledRectWaveguide_gamma (&Harrington,frequency,m,n,TM,"test_sweep",ifreq,mode,&casenum);

            if (!show_csv_data) printf ("m=%d n=%d TE\n",m,n);
            partiallyFilledRectWaveguide_gamma (&Harrington,frequency,m,n,TE,"test_sweep",ifreq,mode,&casenum);
            mode++;
            n++;
         }
         m++;
      }
      */

      ifreq=1; frequency=6e9;
      while (frequency < 9e9*(1+1e-12)) {

         m=1; n=1; mode=1;
         partiallyFilledRectWaveguide_gamma (&Harrington,frequency,m,n,TM,"Harrington",ifreq,mode,&casenum);

         m=2; n=1; mode=2;
         partiallyFilledRectWaveguide_gamma (&Harrington,frequency,m,n,TM,"Harrington",ifreq,mode,&casenum);

         m=0; n=1; mode=3;
         partiallyFilledRectWaveguide_gamma (&Harrington,frequency,m,n,TE,"Harrington",ifreq,mode,&casenum);

         m=1; n=2; mode=4;
         if (frequency > 12.5e9) mode=5;
         partiallyFilledRectWaveguide_gamma (&Harrington,frequency,m,n,TM,"Harrington",ifreq,mode,&casenum);

         m=1; n=1; mode=5;
         if (frequency > 12.5e9) mode=4;
         partiallyFilledRectWaveguide_gamma (&Harrington,frequency,m,n,TE,"Harrington",ifreq,mode,&casenum);

         ifreq++;
         frequency+=0.25e9;

      }
   } // PARTIALLY_FILLED

//----------------------------------------------------------------------------------------------------------
// Coax
//----------------------------------------------------------------------------------------------------------

   if (COAX) {
      double a=0.406;
      double b=1.480;
      double er=2.26;
      double scale;
      double frequency;
      int ifreq;
      int casenum=0;
      double x,y;

      // adjust the limits due to the approximation of the curved boundary 
      resultMagLimit=1e-8;
      fieldMagLimit=1e-3;
      resultEqualErrorLimit=0.003;
      fieldEqualErrorLimit=0.006;
      resultLessthanErrorLimit=1e-8;
      fieldLessthanErrorLimit=0.004;

      // adjust for chords marking the circular profile
      // a=a*(1-cos(pi/4/7)/4);   // 7 segments in 90 degrees, take the quarter point as average
      // b=b*(1-cos(pi/4/23)/4);  // 23 segments in 90 degrees

      double impedance=coax_impedance (a,b,er);

      ifreq=1; frequency=1e7;
      while (frequency < 10e9*(1+1e-12)) {

         rectWaveguide_result_print_line ("coax", ifreq, 1, &casenum, frequency, "alpha", 0);
         rectWaveguide_result_print_line ("coax", ifreq, 1, &casenum, frequency, "beta", sqrt(er));
         rectWaveguide_result_print_line ("coax", ifreq, 1, &casenum, frequency, "real_Z", impedance);
         rectWaveguide_result_print_line ("coax", ifreq, 1, &casenum, frequency, "imag_Z", 0);

         scale=0;

         x=0; y=0.0008;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         x=0; y=0.0011;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         x=0; y=0.0014;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);


         x=0; y=-0.0008;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         x=0; y=-0.0011;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         x=0; y=-0.0014;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);


         x=0.00065; y=0.00065;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         x=0.0009; y=0.0009;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);


         x=0.00065; y=-0.00065;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         x=0.0009; y=-0.0009;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);


         x=-0.00065; y=0.00065;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         x=-0.0009; y=0.0009;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);


         x=-0.00065; y=-0.00065;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         x=-0.0009; y=-0.0009;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         ifreq++;
         frequency*=10;
      }

   } // COAX


//----------------------------------------------------------------------------------------------------------
// 1/8 Coax
//----------------------------------------------------------------------------------------------------------

   if (EIGHTH_COAX) {
      double a=0.406;
      double b=1.480;
      double er=2.26;
      double scale;
      double frequency;
      int ifreq;
      int casenum=0;
      double x,y;

      resultEqualErrorLimit=5e-3;
      fieldLessthanErrorLimit=8e-3;
      fieldEqualErrorLimit=8e-3;

      double impedance=coax_impedance (a,b,er);
      impedance*=8;

      ifreq=1; frequency=1e7;
      while (frequency < 10e9*(1+1e-12)) {

         rectWaveguide_result_print_line ("coax", ifreq, 1, &casenum, frequency, "alpha", 0);
         rectWaveguide_result_print_line ("coax", ifreq, 1, &casenum, frequency, "beta", sqrt(er));
         rectWaveguide_result_print_line ("coax", ifreq, 1, &casenum, frequency, "real_Z", impedance);
         rectWaveguide_result_print_line ("coax", ifreq, 1, &casenum, frequency, "imag_Z", 0);

         scale=0;

         x=0.0008; y=0.0003;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);


         x=0.0005; y=0.0002;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         x=0.0006; y=0.00023;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         x=0.0012; y=0.00045;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         x=0.0013; y=0.0005;
         coax_fields (a, b, x, y, frequency, impedance, &scale, "coax", ifreq, &casenum);

         ifreq++;
         frequency*=10;
      }

   } // EIGHTH_COAX


   return 0;
}

