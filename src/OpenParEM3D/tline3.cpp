////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    tline3 - A dedicated 3-section transmission line calculator             //
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
//
// tline3 is a dedicated solver for this topology:
//
//           Z1                   C1            C2                  Z2
//    +|---/\/\/\---o-------------|-------------|-------------o---/\/\/\---|+
//  V1 O               Zoa,Ga,La  =  Zob,Gb,Lb  =  Zoc,Gc,Lc               O V2
//    -|------------o-------------|-------------|-------------o------------|-
//
// There are three sections of transmission line, a, b, and c.
// Each section is described by its characteristic impedance, Zo, gamma, G, and length, L.
// C1 and C2 are given capacitive discontinuities.
// V1 and V2 are given excitation voltages with given source impedances Z1 and Z2.
//
// The voltage traveling waves on section "a" are Va+ and Va-,and for sections "b" and "c",
// they are Vb+, Vb-, Vc+, and Vc-.  These are the unknows to be solved.
//
// After solution, with V1=1 and V2=0, the S-parameters are
//   S11=Va-/Va+
//   S21=Vc+*e^(-GcLc)/Va+
// With V1=0 and V2=1, then
//   S12=Va-/(Vc-*e^(+GcLc))
//   S22=Vc+/Vc-

#include <fstream>
#include <iostream>
#include <sstream>
#include <complex>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <chrono>
#include <unistd.h>

#define lapack_int int
#define lapack_complex_double std::complex<double>

extern "C" void matrixZero (lapack_complex_double *, lapack_int);
extern "C" int matrixInverse (lapack_complex_double *, lapack_int);
extern "C" void matrixSetValue (lapack_complex_double *, lapack_int, double, double);
extern "C" void vectorZero (lapack_complex_double *, lapack_int);
extern "C" void vectorSetValue (lapack_complex_double *, lapack_int, double, double);
extern "C" void matrixVectorMultiply (lapack_complex_double *, lapack_complex_double *, lapack_complex_double *, lapack_int);
extern "C" double vectorGetRealValue (lapack_complex_double *, lapack_int);
extern "C" double vectorGetImagValue (lapack_complex_double *, lapack_int);

using namespace std;

int main(int argc, char *argv[])
{
   double pi=4.*atan(1.);
   double frequency;
   complex<double> Z1,Z2;
   double C1,C2;
   complex<double> Zoa,Zob,Zoc;
   complex<double> Ga,Gb,Gc;
   double La,Lb,Lc;

   // exect a csv text file on the command line as the first argument
   if (argc != 2) {
      cout << "Usage: tline3d filename.csv" << endl;
      cout << endl;
      cout << "tline3 is a dedicated S-parameter solver for a cascade of 3 transmission lines with two shunt capacitive discontinuities." << endl;
      cout << "The csv file provides the needed parameters to solve, and the 2-port S-parameters are output." << endl; 
      exit (0);
   }

   // open the input file
   ifstream CSV;
   CSV.open(argv[1],ifstream::in);
   if (!CSV.is_open()) {
      cout << "ERROR3191: Unable to open file \"" << argv[1] << "\" for reading." << endl;
   }

   // header for the output
   cout << "#frequency,S11(dB),S11(deg),S12(dB),S12(deg),S21(dB),S21(deg),S22(dB),S22(deg)" << endl;

   // loop through the input file
   string line;
   while (getline(CSV,line)) {

      // skip comment lines
      if (line.compare(0,1,"#") == 0) continue;

      // load the settings

      // frequency,ReZ1,ImZ1,ReZoa,ImZoa,ReGa,ImGa,La,C1,ReZob,ImZob,ReGb,ImGb,Lb,C2,ReZoc,ImZoc,ReGc,ImGc,Lc,ReZ2,ImZ2
      double ReZ1,ImZ1,ReZoa,ImZoa,ReGa,ImGa,ReZob,ImZob,ReGb,ImGb,ReZoc,ImZoc,ReGc,ImGc,ReZ2,ImZ2;
      int count=0;
      stringstream ssLine(line);
      string value;
      while (std::getline(ssLine,value,',')) {
         if (count == 0) frequency=stod(value);
         else if (count == 1) ReZ1=stod(value);
         else if (count == 2) ImZ1=stod(value);

         else if (count == 3) ReZoa=stod(value);
         else if (count == 4) ImZoa=stod(value);
         else if (count == 5) ReGa=stod(value);
         else if (count == 6) ImGa=stod(value);
         else if (count == 7) La=stod(value);

         else if (count == 8) C1=stod(value);

         else if (count == 9) ReZob=stod(value);
         else if (count == 10) ImZob=stod(value);
         else if (count == 11) ReGb=stod(value);
         else if (count == 12) ImGb=stod(value);
         else if (count == 13) Lb=stod(value);

         else if (count == 14) C2=stod(value);

         else if (count == 15) ReZoc=stod(value);
         else if (count == 16) ImZoc=stod(value);
         else if (count == 17) ReGc=stod(value);
         else if (count == 18) ImGc=stod(value);
         else if (count == 19) Lc=stod(value);

         else if (count == 20) ReZ2=stod(value);
         else if (count == 21) ImZ2=stod(value);

         count ++;
      }
      if (count != 22) continue;

      Z1=complex<double>(ReZ1,ImZ1);

      Zoa=complex<double>(ReZoa,ImZoa);
      Ga=complex<double>(ReGa,ImGa);

      Zob=complex<double>(ReZob,ImZob);
      Gb=complex<double>(ReGb,ImGb);
 
      Zoc=complex<double>(ReZoc,ImZoc);
      Gc=complex<double>(ReGc,ImGc);

      Z2=complex<double>(ReZ2,ImZ2);
 
      // create matrix and known and unknown vectors

      int n=6;
      lapack_complex_double *A=(lapack_complex_double *) malloc(n*n*sizeof(lapack_complex_double));
      matrixZero(A,n);

      lapack_complex_double *b=(lapack_complex_double *) malloc(n*sizeof(lapack_complex_double));
      vectorZero(b,n);

      lapack_complex_double *x=(lapack_complex_double *) malloc(n*sizeof(lapack_complex_double));
      // x: Va+,Va-,Vb+,Vb-,Vc+,Vc-

      // fill matrix

      complex<double> j=complex<double>(0,1);
      double w=2*pi*frequency;
      complex<double> val;

      // row 0
      val=1.+Z1/Zoa; matrixSetValue(A,0+0*n,real(val),imag(val));
      val=1.-Z1/Zoa; matrixSetValue(A,0+1*n,real(val),imag(val));

      // row 1
      val=+exp(-Ga*La)*(1.-j*w*C1*Zoa); matrixSetValue(A,1+0*n,real(val),imag(val));
      val=-exp(+Ga*La)*(1.+j*w*C1*Zoa); matrixSetValue(A,1+1*n,real(val),imag(val));
      val=-Zoa/Zob; matrixSetValue(A,1+2*n,real(val),imag(val));
      val=+Zoa/Zob; matrixSetValue(A,1+3*n,real(val),imag(val));

      // row 2
      val=exp(-Ga*La); matrixSetValue(A,2+0*n,real(val),imag(val));
      val=exp(+Ga*La); matrixSetValue(A,2+1*n,real(val),imag(val));
      val=-1; matrixSetValue(A,2+2*n,real(val),imag(val));
      val=-1; matrixSetValue(A,2+3*n,real(val),imag(val));

      // row 3
      val=+exp(-Gb*Lb)*(1.-j*w*C2*Zob); matrixSetValue(A,3+2*n,real(val),imag(val));
      val=-exp(+Gb*Lb)*(1.+j*w*C2*Zob); matrixSetValue(A,3+3*n,real(val),imag(val));
      val=-Zob/Zoc; matrixSetValue(A,3+4*n,real(val),imag(val));
      val=+Zob/Zoc; matrixSetValue(A,3+5*n,real(val),imag(val));

      // row 4
      val=exp(-Gb*Lb); matrixSetValue(A,4+2*n,real(val),imag(val));
      val=exp(+Gb*Lb); matrixSetValue(A,4+3*n,real(val),imag(val));
      val=-1; matrixSetValue(A,4+4*n,real(val),imag(val));
      val=-1; matrixSetValue(A,4+5*n,real(val),imag(val));

      // row 5
      val=exp(-Gc*Lc)*(1.-Z2/Zoc); matrixSetValue(A,5+4*n,real(val),imag(val));
      val=exp(+Gc*Lc)*(1.+Z2/Zoc); matrixSetValue(A,5+5*n,real(val),imag(val));

      // Invert
      matrixInverse(A,n);

      // solve for prime quantities
      // V1=1; V2=0;

      vectorSetValue(b,0,1,0);
      vectorSetValue(b,n-1,0,0);

      matrixVectorMultiply(A,b,x,n); 

      complex<double> a1p=complex<double>(vectorGetRealValue(x,0),vectorGetImagValue(x,0))/sqrt(Zoa);
      complex<double> b1p=complex<double>(vectorGetRealValue(x,1),vectorGetImagValue(x,1))/sqrt(Zoa);

      complex<double> a2p=complex<double>(vectorGetRealValue(x,5),vectorGetImagValue(x,5))*exp(+Gc*Lc)/sqrt(Zoc);
      complex<double> b2p=complex<double>(vectorGetRealValue(x,4),vectorGetImagValue(x,4))*exp(-Gc*Lc)/sqrt(Zoc);

      // solve for double prime quantities
      // V1=0; V2=1;

      vectorSetValue(b,0,0,0);
      vectorSetValue(b,n-1,1,0);

      matrixVectorMultiply(A,b,x,n);

      complex<double> a1pp=complex<double>(vectorGetRealValue(x,0),vectorGetImagValue(x,0))/sqrt(Zoa);
      complex<double> b1pp=complex<double>(vectorGetRealValue(x,1),vectorGetImagValue(x,1))/sqrt(Zoa);

      complex<double> a2pp=complex<double>(vectorGetRealValue(x,5),vectorGetImagValue(x,5))*exp(+Gc*Lc)/sqrt(Zoc);
      complex<double> b2pp=complex<double>(vectorGetRealValue(x,4),vectorGetImagValue(x,4))*exp(-Gc*Lc)/sqrt(Zoc);

      // solve for S - These are unnormalized S-parameters where the values of Z1 and Z2 have no impact.

      int m=4;
      lapack_complex_double *B=(lapack_complex_double *) malloc(m*m*sizeof(lapack_complex_double));
      matrixZero(B,m);

      lapack_complex_double *p=(lapack_complex_double *) malloc(m*sizeof(lapack_complex_double));
      vectorZero(p,m);

      lapack_complex_double *S=(lapack_complex_double *) malloc(m*sizeof(lapack_complex_double));
      // S: S11, S12, S21, S22

      // row 0
      matrixSetValue(B,0+0*m,real(a1p),imag(a1p));
      matrixSetValue(B,0+1*m,real(a2p),imag(a2p));
      vectorSetValue(p,0,real(b1p),imag(b1p));

      // row 1
      matrixSetValue(B,1+2*m,real(a1p),imag(a1p));
      matrixSetValue(B,1+3*m,real(a2p),imag(a2p));
      vectorSetValue(p,1,real(b2p),imag(b2p));
 
      // row 2
      matrixSetValue(B,2+0*m,real(a1pp),imag(a1pp));
      matrixSetValue(B,2+1*m,real(a2pp),imag(a2pp));
      vectorSetValue(p,2,real(b1pp),imag(b1pp));

      // row 3
      matrixSetValue(B,3+2*m,real(a1pp),imag(a1pp));
      matrixSetValue(B,3+3*m,real(a2pp),imag(a2pp));
      vectorSetValue(p,3,real(b2pp),imag(b2pp));

      // Invert
      matrixInverse(B,m);

      // get S
      matrixVectorMultiply(B,p,S,m);
      complex<double> S11=complex<double>(vectorGetRealValue(S,0),vectorGetImagValue(S,0));
      complex<double> S12=complex<double>(vectorGetRealValue(S,1),vectorGetImagValue(S,1));
      complex<double> S21=complex<double>(vectorGetRealValue(S,2),vectorGetImagValue(S,2));
      complex<double> S22=complex<double>(vectorGetRealValue(S,3),vectorGetImagValue(S,3));

      // calculate normalized S-parameters to Z1 and Z2

      // Note that the code and method called by ResultDatabase::renormalize is not used here:
      // the custom implementation here serves as a check on the results there.

      // can re-use the prior matrices and vectors of size m=4
      matrixZero(B,m);
      vectorZero(p,m);
      // p: b3, b4, a1, a2

      // row 0
      val=1./sqrt(Z1); matrixSetValue(B,0+0*m,real(val),imag(val));
      val=1./sqrt(Zoa)*(1.-S11); matrixSetValue(B,0+2*m,real(val),imag(val));
      val=-1./sqrt(Zoa)*S12; matrixSetValue(B,0+3*m,real(val),imag(val));

      // row 1
      val=-sqrt(Z1); matrixSetValue(B,1+0*m,real(val),imag(val));
      val=sqrt(Zoa)*(1.+S11); matrixSetValue(B,1+2*m,real(val),imag(val));
      val=sqrt(Zoa)*S12; matrixSetValue(B,1+3*m,real(val),imag(val));

      // row 2
      val=1./sqrt(Z2); matrixSetValue(B,2+1*m,real(val),imag(val));
      val=-1./sqrt(Zoc)*S21; matrixSetValue(B,2+2*m,real(val),imag(val));
      val=1./sqrt(Zoc)*(1.-S22); matrixSetValue(B,2+3*m,real(val),imag(val));

      // row 3
      val=-sqrt(Z2); matrixSetValue(B,3+1*m,real(val),imag(val));
      val=sqrt(Zoc)*S21; matrixSetValue(B,3+2*m,real(val),imag(val));
      val=sqrt(Zoc)*(1.+S22); matrixSetValue(B,3+3*m,real(val),imag(val));

      // Invert
      matrixInverse(B,m);

      // a3=1, a4=0
      val=1./sqrt(Z1); vectorSetValue(p,0,real(val),imag(val));
      val=sqrt(Z1); vectorSetValue(p,1,real(val),imag(val));
      val=0; vectorSetValue(p,2,real(val),imag(val));
      val=0; vectorSetValue(p,3,real(val),imag(val));

      matrixVectorMultiply(B,p,S,m);

      complex<double> S11n=complex<double>(vectorGetRealValue(S,0),vectorGetImagValue(S,0));
      complex<double> S21n=complex<double>(vectorGetRealValue(S,1),vectorGetImagValue(S,1));

      // a3=0, a4=1
      val=0; vectorSetValue(p,0,real(val),imag(val));
      val=0; vectorSetValue(p,1,real(val),imag(val));
      val=1./sqrt(Z2); vectorSetValue(p,2,real(val),imag(val));
      val=sqrt(Z2); vectorSetValue(p,3,real(val),imag(val));

      matrixVectorMultiply(B,p,S,m);

      complex<double> S22n=complex<double>(vectorGetRealValue(S,1),vectorGetImagValue(S,1));
      complex<double> S12n=complex<double>(vectorGetRealValue(S,0),vectorGetImagValue(S,0));

      // output to screen in csv format

      cout << setprecision(10)
           << "unnormalized"
           << ", " << frequency
           << ", " << 20*log10(abs(S11)) << "," << arg(S11)*180/pi
           << ", " << 20*log10(abs(S12)) << "," << arg(S12)*180/pi
           << ", " << 20*log10(abs(S21)) << "," << arg(S21)*180/pi
           << ", " << 20*log10(abs(S22)) << "," << arg(S22)*180/pi
           << endl;

      cout << setprecision(10)
           << "  normalized"
           << ", " << frequency
           << ", " << 20*log10(abs(S11n)) << "," << arg(S11n)*180/pi
           << ", " << 20*log10(abs(S12n)) << "," << arg(S12n)*180/pi
           << ", " << 20*log10(abs(S21n)) << "," << arg(S21n)*180/pi
           << ", " << 20*log10(abs(S22n)) << "," << arg(S22n)*180/pi
           << endl;

      // sanity check for lossless networks where the sum must be equal to 1

      //cout << "|S11|^2+|S21|^2=" << S11*conj(S11)+S21*conj(S21) << endl;
      //cout << "|S22|^2+|S12|^2=" << S22*conj(S22)+S12*conj(S12) << endl;

      //cout << "|S11n|^2+|S21n|^2=" << S11n*conj(S11n)+S21n*conj(S21n) << endl;
      //cout << "|S22n|^2+|S12n|^2=" << S22n*conj(S22n)+S12n*conj(S12n) << endl;

      free(A);
      free(b);
      free(x);

      free(B);
      free(p);
      free(S);
   }

   CSV.close();

   return 0;
}

