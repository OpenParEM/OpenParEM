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

#include "pattern.hpp"

// angle between the two great circles (lune) formed by a,b,center and b,c,center
double get_luneAngle (OPEMpoint a, OPEMpoint b, OPEMpoint c, OPEMpoint *center)
{
   a.subtract(center);
   b.subtract(center);
   c.subtract(center);

   // normals
   OPEMpoint *nab=a.cross(&b);
   OPEMpoint *ncb=c.cross(&b);

   // angle between normals
   double angle=nab->get_angle(ncb);

   delete nab;
   delete ncb;

   return angle;
}

// area of the triangle on a sphere defined by a, b, and c
// radius is included for convenience since it can be calculated from a, b, or c and center
double get_areaSphereTriangle (OPEMpoint a, OPEMpoint b, OPEMpoint c, OPEMpoint *center, double radius)
{
   // lune angles
   double AC=get_luneAngle (a,b,c,center);
   double BA=get_luneAngle (b,c,a,center);
   double CB=get_luneAngle (c,a,b,center);

   return pow(radius,2)*(AC+BA+CB-M_PI);
}

///////////////////////////////////////////////////////////////////////////////////////////
// OPEMpoint
///////////////////////////////////////////////////////////////////////////////////////////

OPEMpoint::OPEMpoint (double x_, double y_, double z_)
{
   x=x_;
   y=y_;
   z=z_;

   theta=DBL_MAX;
   phi=DBL_MAX;

   Etheta=std::complex<double>(0,0);
   Ephi=std::complex<double>(0,0);
   Htheta=std::complex<double>(0,0);
   Hphi=std::complex<double>(0,0);

   gain=-DBL_MAX;
   directivity=-DBL_MAX;
}

OPEMpoint::OPEMpoint (OPEMpoint *a)
{
   x=a->x;
   y=a->y;
   z=a->z;

   theta=DBL_MAX;
   phi=DBL_MAX;
 
   Etheta=std::complex<double>(0,0);
   Ephi=std::complex<double>(0,0);
   Htheta=std::complex<double>(0,0);
   Hphi=std::complex<double>(0,0);

   gain=-DBL_MAX;
   directivity=-DBL_MAX;
}

OPEMpoint::OPEMpoint ()
{
   x=0;
   y=0;
   z=0;
   
   theta=DBL_MAX;
   phi=DBL_MAX;

   Etheta=std::complex<double>(0,0);
   Ephi=std::complex<double>(0,0);
   Htheta=std::complex<double>(0,0);
   Hphi=std::complex<double>(0,0);

   gain=-DBL_MAX;
   directivity=-DBL_MAX;
}

// copy from a
void OPEMpoint::copy (OPEMpoint *a)
{
   x=a->x;
   y=a->y;
   z=a->z;
   theta=a->theta;
   phi=a->phi;
   Etheta=a->Etheta;
   Ephi=a->Ephi;
   Htheta=a->Htheta;
   Hphi=a->Hphi;
   gain=a->gain;
   directivity=a->directivity;
}

void OPEMpoint::add (OPEMpoint *a)
{
   x+=a->x;
   y+=a->y;
   z+=a->z;
}

void OPEMpoint::subtract (OPEMpoint *a)
{
   x-=a->x;
   y-=a->y;
   z-=a->z;
}

void OPEMpoint::scale (double scale)
{
   x*=scale;
   y*=scale;
   z*=scale;
}

double OPEMpoint::dot (OPEMpoint *a)
{
   return x*a->x+y*a->y+z*a->z;
}

OPEMpoint* OPEMpoint::cross (OPEMpoint *a)
{
   OPEMpoint *cross=new OPEMpoint(y*a->z-z*a->y,z*a->x-x*a->z,x*a->y-y*a->x);
   return cross;
}

double OPEMpoint::get_angle (OPEMpoint *a)
{
   return acos(dot(a)/(length()*a->length()));
}

double OPEMpoint::get_angle (OPEMpoint *a, OPEMpoint *center)
{
   OPEMpoint *b=new OPEMpoint(a);
   b->subtract(center);

   OPEMpoint *c=new OPEMpoint(this);
   c->subtract(center);

   double angle=acos(b->dot(c)/(b->length()*c->length()));
   delete b;
   delete c;

   return angle;
}

// angle between the two vectors this,a and this,b
double OPEMpoint::get_angle (OPEMpoint a, OPEMpoint b)
{
   a.subtract(this);
   b.subtract(this);
   return a.get_angle(&b);
}

double OPEMpoint::get_signed_angle (OPEMpoint a, OPEMpoint b, OPEMpoint *center)
{
   OPEMpoint *vertex=new OPEMpoint(this);

   vertex->subtract(center);
   a.subtract(center);
   b.subtract(center);
   double angle=vertex->get_angle(a,b);

   OPEMpoint *normal=a.cross(&b);
   if (vertex->dot(normal) < 0) {angle=-angle;}

   delete vertex;
   delete normal;

   return angle;
}

OPEMpoint* OPEMpoint::midpoint (OPEMpoint *a)
{
   double xmid=(x+a->x)/2;
   double ymid=(y+a->y)/2;
   double zmid=(z+a->z)/2;
   OPEMpoint *b=new OPEMpoint(xmid,ymid,zmid);
   return b;
}

void OPEMpoint::midpoint (OPEMpoint *a, OPEMpoint *b)
{
   x=(a->x+b->x)/2;
   y=(a->y+b->y)/2;
   z=(a->z+b->z)/2;
}

bool OPEMpoint::is_match (OPEMpoint *a)
{
   double tol=1e-12;
   if (double_compare(x,a->x,tol) &&
       double_compare(y,a->y,tol) &&
       double_compare(z,a->z,tol)) return true;
   return false;
}

double OPEMpoint::length ()
{
   return sqrt(x*x+y*y+z*z);
}

void OPEMpoint::normalize (double radius)
{
   double len=length();
   x/=len/radius;
   y/=len/radius;
   z/=len/radius;
}

// distance between the two points
double OPEMpoint::distance (OPEMpoint *a)
{
   return sqrt(pow(x-a->x,2)+pow(y-a->y,2)+pow(z-a->z,2));
}

void OPEMpoint::set_theta (OPEMpoint *global_center)
{
   double len=sqrt(pow(x-global_center->x,2)+pow(y-global_center->y,2)+pow(z-global_center->z,2));
   theta=acos((z-global_center->z)/len);
}

void OPEMpoint::set_phi (OPEMpoint *global_center)
{
   phi=atan2(y-global_center->y,x-global_center->x);
}

void OPEMpoint::addMeshVertex (mfem::Mesh *mesh)
{
   mesh->AddVertex(x,y,z);
}

std::complex<double> OPEMpoint::get_fieldValue (std::string quantity)
{
   if (quantity.compare("Etheta") == 0) return Etheta;
   if (quantity.compare("Ephi") == 0) return Ephi;
   if (quantity.compare("Htheta") == 0) return Htheta;
   if (quantity.compare("Hphi") == 0) return Hphi;
   if (quantity.compare("G") == 0) return get_power();
   if (quantity.compare("D") == 0) return get_power();

   return std::complex<double>(-DBL_MAX,-DBL_MAX);
}

void OPEMpoint::sendFieldsTo (int rankTo)
{
   double data=real(Etheta);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1100,PETSC_COMM_WORLD);
   data=imag(Etheta);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1101,PETSC_COMM_WORLD);

   data=real(Ephi);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1102,PETSC_COMM_WORLD);
   data=imag(Ephi);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1103,PETSC_COMM_WORLD);

   data=real(Htheta);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1104,PETSC_COMM_WORLD);
   data=imag(Htheta);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1105,PETSC_COMM_WORLD);

   data=real(Hphi);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1106,PETSC_COMM_WORLD);
   data=imag(Hphi);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1107,PETSC_COMM_WORLD);
}

void OPEMpoint::recvFieldsFrom (int rankFrom)
{
   double dataRe,dataIm;

   MPI_Recv(&dataRe,1,MPI_DOUBLE,rankFrom,1100,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&dataIm,1,MPI_DOUBLE,rankFrom,1101,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   Etheta=std::complex<double>(dataRe,dataIm);

   MPI_Recv(&dataRe,1,MPI_DOUBLE,rankFrom,1102,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&dataIm,1,MPI_DOUBLE,rankFrom,1103,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   Ephi=std::complex<double>(dataRe,dataIm);

   MPI_Recv(&dataRe,1,MPI_DOUBLE,rankFrom,1104,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&dataIm,1,MPI_DOUBLE,rankFrom,1105,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   Htheta=std::complex<double>(dataRe,dataIm);

   MPI_Recv(&dataRe,1,MPI_DOUBLE,rankFrom,1106,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&dataIm,1,MPI_DOUBLE,rankFrom,1107,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   Hphi=std::complex<double>(dataRe,dataIm);
}

void OPEMpoint::print ()
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank == 0) {

      std::cout << "         OPEMpoint: " << this << std::endl;
      std::cout << "            x=" << x << std::endl;
      std::cout << "            y=" << y << std::endl;
      std::cout << "            z=" << z << std::endl;
      std::cout << "            theta=" << theta << std::endl;
      std::cout << "            phi=" << phi << std::endl;
      std::cout << "            Etheta=" << Etheta << std::endl;
      std::cout << "            Ephi=" << Ephi << std::endl;
      std::cout << "            Htheta=" << Htheta << std::endl;
      std::cout << "            Hphi=" << Hphi << std::endl;
      std::cout << "            gain=" << gain << std::endl;
      std::cout << "            directivity=" << directivity << std::endl;
   }
}

// area is the area on the sphere assigned to this point
// use area=0 for circles
void OPEMpoint::saveHeader (std::ostream *out, double area)
{
   *out << "#theta(deg),phi(deg),";
   if (area > 0) {*out << "area,";}
   *out << "gain,directivity,"
        << "Re(Etheta),Im(Etheta),Re(Ephi),Im(Ephi),Re(Htheta),Im(Htheta),Re(Hphi),Im(Hphi)" << std::endl;
}

void OPEMpoint::save (std::ostream *out, double area)
{
   *out << theta*180/M_PI << ","
        << phi*180/M_PI << ",";
   if (area > 0) {*out << area << ",";}
   *out << gain << ","
        << directivity << ","
        << real(Etheta) << ","
        << imag(Etheta) << ","
        << real(Ephi) << ","
        << imag(Ephi) << ","
        << real(Htheta) << ","
        << imag(Htheta) << ","
        << real(Hphi) << ","
        << imag(Hphi) << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////
// OPEMtriangle
///////////////////////////////////////////////////////////////////////////////////////////

OPEMtriangle::OPEMtriangle (long unsigned int a_, long unsigned int b_, long unsigned int c_)
{
   a=a_;
   b=b_;
   c=c_;

   ab=ULONG_MAX;
   bc=ULONG_MAX;
   ca=ULONG_MAX;

   center=ULONG_MAX;

   area=DBL_MAX;
}

void OPEMtriangle::copy (OPEMtriangle *t)
{
   a=t->a;
   b=t->b;
   c=t->c;
   ab=t->ab;
   bc=t->bc;
   ca=t->ca;
   center=t->center;
   area=t->area;
}

bool OPEMtriangle::has_point_index (long unsigned int index)
{
   if (a == index) return true;
   if (b == index) return true;
   if (c == index) return true;
   return false;
}

OPEMpoint* OPEMtriangle::getCenterOPEMpoint (std::vector<OPEMpoint *> *pointList, OPEMpoint *sphere_center, double radius)
{
   // shift by sphere center
   OPEMpoint *ac=new OPEMpoint((*pointList)[a]);
   ac->subtract(sphere_center);

   OPEMpoint *bc=new OPEMpoint((*pointList)[b]);
   bc->subtract(sphere_center);

   OPEMpoint *cc=new OPEMpoint((*pointList)[c]);
   cc->subtract(sphere_center);

   // center
   OPEMpoint *center=new OPEMpoint(ac);
   center->add(bc);
   center->add(cc);
   center->scale(1./3.); 

   // scale it to put it on the sphere
   center->normalize(radius);

   // shift back
   center->add(sphere_center);

   delete ac;
   delete bc;
   delete cc;

   return center;
}

/*
// using Heron's formula
double OPEMtriangle::get_areaPlanar (vector<OPEMpoint *> *pointList)
{
   double A=(*pointList)[a]->distance((*pointList)[b]);
   double B=(*pointList)[b]->distance((*pointList)[c]);
   double C=(*pointList)[c]->distance((*pointList)[a]);
   double S=0.5*(A+B+C);
   return sqrt(S*(S-A)*(S-B)*(S-C));
}
*/

void OPEMtriangle::calculateArea (std::vector<OPEMpoint *> *pointList, OPEMpoint *sphere_center, double radius)
{
   area=get_areaSphereTriangle((*pointList)[a],(*pointList)[b],(*pointList)[c],sphere_center,radius);
}

void OPEMtriangle::set_metrics (OPEMpoint *sphere_center, double radius, std::vector<OPEMpoint *> *pointList)
{
   (*pointList)[a]->set_theta(sphere_center);
   (*pointList)[a]->set_phi(sphere_center);

   (*pointList)[b]->set_theta(sphere_center);
   (*pointList)[b]->set_phi(sphere_center);

   (*pointList)[c]->set_theta(sphere_center);
   (*pointList)[c]->set_phi(sphere_center);

   OPEMpoint *triangleCenter=getCenterOPEMpoint(pointList,sphere_center,radius);
   triangleCenter->set_theta(sphere_center);
   triangleCenter->set_phi(sphere_center);
   pointList->push_back(triangleCenter);
   center=pointList->size()-1;

   calculateArea(pointList,sphere_center,radius);

   return;
}

bool OPEMtriangle::is_matched ()
{
  if (ab == ULONG_MAX) return false;
  if (bc == ULONG_MAX) return false;
  if (ca == ULONG_MAX) return false;
  return true;
}

// return false if an unmatched edge still exists
// return true if all edges are matched
bool OPEMtriangle::has_shared_edge (OPEMtriangle *t, long unsigned int index, long unsigned int t_index)
{
   // check to see if this triangle is already matched from prior operations
   if (is_matched()) return true;

   // edge ab

   if (a == t->a && b == t->b) {ab=t_index; t->ab=index; return is_matched();}
   if (a == t->b && b == t->a) {ab=t_index; t->ab=index; return is_matched();}

   if (a == t->b && b == t->c) {ab=t_index; t->bc=index; return is_matched();}
   if (a == t->c && b == t->b) {ab=t_index; t->bc=index; return is_matched();}

   if (a == t->c && b == t->a) {ab=t_index; t->ca=index; return is_matched();}
   if (a == t->a && b == t->c) {ab=t_index; t->ca=index; return is_matched();}

   // edge bc 
   
   if (b == t->a && c == t->b) {bc=t_index; t->ab=index; return is_matched();}
   if (b == t->b && c == t->a) {bc=t_index; t->ab=index; return is_matched();}

   if (b == t->b && c == t->c) {bc=t_index; t->bc=index; return is_matched();}
   if (b == t->c && c == t->b) {bc=t_index; t->bc=index; return is_matched();}

   if (b == t->c && c == t->a) {bc=t_index; t->ca=index; return is_matched();}
   if (b == t->a && c == t->c) {bc=t_index; t->ca=index; return is_matched();}

   // edge ca
  
   if (c == t->a && a == t->b) {ca=t_index; t->ab=index; return is_matched();}
   if (c == t->b && a == t->a) {ca=t_index; t->ab=index; return is_matched();}

   if (c == t->b && a == t->c) {ca=t_index; t->bc=index; return is_matched();}
   if (c == t->c && a == t->b) {ca=t_index; t->bc=index; return is_matched();}

   if (c == t->c && a == t->a) {ca=t_index; t->ca=index; return is_matched();}
   if (c == t->a && a == t->c) {ca=t_index; t->ca=index; return is_matched();}

   return false;
}

void OPEMtriangle::print ()
{
   std::cout << "OPEMtriangle: " << this << std::endl;
   std::cout << "   a=" << a << std::endl;
   std::cout << "   b=" << b << std::endl;
   std::cout << "   c=" << c << std::endl;
   std::cout << "   ab=" << ab << std::endl;
   std::cout << "   bc=" << bc << std::endl;
   std::cout << "   ca=" << ca << std::endl;
   std::cout << "   center=" << center << std::endl;
   std::cout << "   area=" << area << std::endl;
   std::cout << "   center data:" << std::endl;
}

double OPEMtriangle::get_theta (std::vector<OPEMpoint *> *pointList)
{
   return (*pointList)[center]->get_theta();
}

double OPEMtriangle::get_phi (std::vector<OPEMpoint *> *pointList)
{
   return (*pointList)[center]->get_phi();
}

void OPEMtriangle::addMeshTriangle (mfem::Mesh *mesh)
{
   mesh->AddTriangle(a,b,c);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Sphere
///////////////////////////////////////////////////////////////////////////////////////////

Sphere::Sphere ()
{
   radius=1;
   center.set_xyz(0,0,0);
}

Sphere* Sphere::clone ()
{
   Sphere *b=new Sphere();
   b->radius=radius;
   b->center.copy(&center);

   long unsigned int i=0;
   while (i < pointList.size()) {
      OPEMpoint *point=new OPEMpoint();
      point->copy(pointList[i]);
      b->pointList.push_back(point);
      i++;
   }

   i=0;
   while (i < triangleList.size()) {
      OPEMtriangle *triangle=new OPEMtriangle();
      triangle->copy(triangleList[i]);
      b->triangleList.push_back(triangle);
      i++;
   }

   return b;
}

bool Sphere::is_match (Sphere *a)
{
   double tol=1e-12;
   if (!a) return false;
   if (!double_compare(radius,a->radius,tol)) return false;
   if (!center.is_match(&(a->center))) return false;
   return true;
}

void Sphere::pushOPEMpoint (double x, double y, double z)
{
   OPEMpoint *point=new OPEMpoint (x,y,z);
   point->add(&center);
   pointList.push_back(point);
}

void Sphere::pushOPEMtriangle (long unsigned int a, long unsigned int b, long unsigned int c)
{
   OPEMtriangle *triangle=new OPEMtriangle (a,b,c);
   triangleList.push_back(triangle);
}

void Sphere::create ()
{
   double X=0.525731112119133606*radius;
   double Z=0.850650808352039932*radius;
   pushOPEMpoint(-X,0,Z);
   pushOPEMpoint(X,0,Z);
   pushOPEMpoint(-X,0,-Z);
   pushOPEMpoint(X,0,-Z);
   pushOPEMpoint(0,Z,X);
   pushOPEMpoint(0,Z,-X);
   pushOPEMpoint(0,-Z,X);
   pushOPEMpoint(0,-Z,-X);   
   pushOPEMpoint(Z,X,0);
   pushOPEMpoint(-Z,X,0);
   pushOPEMpoint(Z,-X,0);
   pushOPEMpoint(-Z,-X,0);

   pushOPEMtriangle(0,4,1);
   pushOPEMtriangle(0,9,4);
   pushOPEMtriangle(9,5,4);
   pushOPEMtriangle(4,5,8);
   pushOPEMtriangle(4,8,1);   
   pushOPEMtriangle(8,10,1);
   pushOPEMtriangle(8,3,10);
   pushOPEMtriangle(5,3,8);
   pushOPEMtriangle(5,2,3);
   pushOPEMtriangle(2,7,3);
   pushOPEMtriangle(7,10,3);
   pushOPEMtriangle(7,6,10);
   pushOPEMtriangle(7,11,6);
   pushOPEMtriangle(11,0,6);
   pushOPEMtriangle(0,1,6);
   pushOPEMtriangle(6,1,10);
   pushOPEMtriangle(9,0,11);
   pushOPEMtriangle(9,11,2);
   pushOPEMtriangle(9,2,5);
   pushOPEMtriangle(7,2,11);
}

long unsigned int Sphere::get_index (OPEMpoint *a)
{
   // see if the point already exists
   long unsigned int i=0;
   while (i < pointList.size()) {
      if (pointList[i]->is_match(a)) {
         delete a;
         a=nullptr;
         return i;
      }
      i++;
   }

   // save it
   pointList.push_back(a);
   return pointList.size()-1;
}

void Sphere::refine (long unsigned int iterations)
{
   long unsigned int k=0;
   while (k < iterations) {

      long unsigned int listSize=triangleList.size();
      long unsigned int i=0;
      while (i < listSize) {

         // corners of the triangle
         OPEMpoint *a=pointList[triangleList[i]->get_a()];
         OPEMpoint *b=pointList[triangleList[i]->get_b()];
         OPEMpoint *c=pointList[triangleList[i]->get_c()];

         // midpoints on the sides of the triangle
         OPEMpoint *ab=a->midpoint(b);
         OPEMpoint *bc=b->midpoint(c);
         OPEMpoint *ca=c->midpoint(a);

         // normalize the lengths
         ab->normalize(radius);
         bc->normalize(radius);
         ca->normalize(radius);

         // indices to the midpoints
         long unsigned int index_ab=get_index(ab);
         long unsigned int index_bc=get_index(bc);
         long unsigned int index_ca=get_index(ca);

         // new triangles
         pushOPEMtriangle(index_ab,triangleList[i]->get_b(),index_bc);
         pushOPEMtriangle(index_ab,index_bc,index_ca);
         pushOPEMtriangle(index_ca,index_bc,triangleList[i]->get_c());

         // convert the original triangle to a smaller one
         triangleList[i]->set_b(index_ab);
         triangleList[i]->set_c(index_ca);

         i++;
      }

      k++;
   }

   calculateAngularResolution();
}

void Sphere::findNeighbors ()
{
   // loop through all the triangles
   long unsigned int i=0;
   while (i < triangleList.size()-1) {
      // skip if the triangle is already matched up with neighbors
      if (! triangleList[i]->is_matched()) {
         // loop through the remaining triangles
         long unsigned int j=i+1;
         while (j < triangleList.size()) {
            // match and check if done with this triangle
            if (triangleList[i]->has_shared_edge(triangleList[j],i,j)) break;
            j++;
         }
      }
      i++;
   }
}

void Sphere::checkNeighbors ()
{
   std::cout << "Sphere::checkNeighbors:" << std::endl;
   long unsigned int i=0;
   while (i < triangleList.size()) {
      if (! triangleList[i]->is_matched()) {
         std::cout << "Unmatched triangle:" << std::endl;
         triangleList[i]->print();
      }
      i++;
   }
}

void Sphere::allocateAreasToPoints ()
{
   // allocate the list and initialize
   long unsigned int i=0;
   while (i < pointList.size()) {
      areaList.push_back(0);
      i++;
   }

   // allocate triangle areas to points
   i=0;
   while (i < triangleList.size()) {

      OPEMpoint a=pointList[triangleList[i]->get_a()];
      OPEMpoint b=pointList[triangleList[i]->get_b()];
      OPEMpoint c=pointList[triangleList[i]->get_c()];
      
      OPEMpoint mid_ab,mid_bc,mid_ca;
      mid_ab.midpoint(&a,&b);
      mid_bc.midpoint(&b,&c);
      mid_ca.midpoint(&c,&a);

      OPEMpoint center_point=pointList[triangleList[i]->get_center()];

      // a
      areaList[triangleList[i]->get_a()]+=get_areaSphereTriangle(a,mid_ab,center_point,&center,radius);
      areaList[triangleList[i]->get_a()]+=get_areaSphereTriangle(a,center_point,mid_ca,&center,radius);

      // b
      areaList[triangleList[i]->get_b()]+=get_areaSphereTriangle(b,mid_bc,center_point,&center,radius);
      areaList[triangleList[i]->get_b()]+=get_areaSphereTriangle(b,center_point,mid_ab,&center,radius);

      // c
      areaList[triangleList[i]->get_c()]+=get_areaSphereTriangle(c,mid_ca,center_point,&center,radius);
      areaList[triangleList[i]->get_c()]+=get_areaSphereTriangle(c,center_point,mid_bc,&center,radius);

      i++;
   }
}

void Sphere::setMetrics ()
{
   long unsigned int i=0;
   while (i < triangleList.size()) {
      triangleList[i]->set_metrics(&center,radius,&pointList);
      i++;
   }
}

double Sphere::getTotalArea ()
{
   double total=0;
   long unsigned int i=0;
   while (i < triangleList.size()) {
      total+=triangleList[i]->get_area();
      i++;
   }
   return total;
}

double Sphere::getMaxAbsValue (std::string quantity)
{
   if (quantity.compare("") == 0) return -DBL_MAX;

   double maxAbsValue=0;
   long unsigned int i=0;
   while (i < pointList.size()) {
      std::complex<double> fieldValue=pointList[i]->get_fieldValue(quantity);

      if (quantity.compare("G") == 0) {
         if (real(fieldValue) > maxAbsValue) maxAbsValue=real(fieldValue);
      } else if (quantity.compare("D") == 0) {
         if (real(fieldValue) > maxAbsValue) maxAbsValue=real(fieldValue);
      } else {
         if (abs(fieldValue) > maxAbsValue) maxAbsValue=abs(fieldValue);
      }

      i++;
   }

   return maxAbsValue;
}

std::complex<double> Sphere::getMaxValue (std::string quantity)
{
   if (quantity.compare("") == 0) return std::complex<double>(-DBL_MAX,-DBL_MAX);

   std::complex<double> maxValue=0;
   long unsigned int i=0;
   while (i < pointList.size()) {
      std::complex<double> fieldValue=pointList[i]->get_fieldValue(quantity);
      if (quantity.compare("G") == 0) {
         if (real(fieldValue) > real(maxValue)) maxValue=fieldValue;
      } else if (quantity.compare("D") == 0) {
         if (real(fieldValue) > real(maxValue)) maxValue=fieldValue;
      } else {
         if (abs(fieldValue) > abs(maxValue)) maxValue=fieldValue;
      }
      i++;
   }

   return maxValue;
}

std::complex<double> Sphere::getRadiatedPower ()
{
   std::complex<double> radiatedPower=0;
   long unsigned int i=0;
   while (i < pointList.size()) {
      radiatedPower+=pointList[i]->get_power()*areaList[i];
      i++;
   }
   return radiatedPower;
}

// Can only plot one quantity on a 3D surface plot
void Sphere::createPatternMesh (struct projectData *projData, double totalArea, std::complex<double> acceptedPower, std::complex<double> radiatedPower,
                                std::string quantity, double frequency, int Sport)
{
   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   // values at the vertices
   std::vector<double> vertexValues;

   // plot limits

   std::complex<double> maxValue=getMaxValue(quantity);
   double scaleMaxValue=0;
   if (quantity.compare("G") == 0){
      scaleMaxValue=real(maxValue/(acceptedPower/totalArea));  // accepted power
      scaleMaxValue=10*log10(scaleMaxValue);
   }
   if (quantity.compare("D") == 0){
      scaleMaxValue=real(maxValue/(radiatedPower/totalArea));  // radiated power
      scaleMaxValue=10*log10(scaleMaxValue);
   }

   double valueMax=-1000;
   while (valueMax-scaleMaxValue < 0) valueMax+=projData->antenna_plot_2D_interval;
   double valueMin=valueMax-projData->antenna_plot_2D_range;

   // calculate the values at the vertices - this is the slow part so it is parallelized
   if (rank == 0) {

      // pre-fill with an error condition
      long unsigned int j=0;
      while (j < pointList.size()) {
         vertexValues.push_back(-DBL_MAX);
         j++;
      }

      // allocate

      int chunk=pointList.size()/size;
      if (chunk == 0) chunk=1;
      std::vector<int> start,stop;
      start.push_back(0);
      stop.push_back(chunk);

      int i=1;
      while (i < size) {
         int nextstart=start[i-1]+chunk;
         if (nextstart > (int)pointList.size()) nextstart=pointList.size();
 
         int nextstop=nextstart+chunk;
         if (nextstop > (int)pointList.size()) nextstop=pointList.size();
          
         start.push_back(nextstart);
         stop.push_back(nextstop);

         i++;
      }
      stop[size-1]=pointList.size();

      // distribute the work
      i=1;
      while (i < size) {
         MPI_Send(&(start[i]),1,MPI_INT,i,100,PETSC_COMM_WORLD);
         MPI_Send(&(stop[i]),1,MPI_INT,i,101,PETSC_COMM_WORLD);
         i++;
      }

      // do the local work
      i=start[0];
      while (i < stop[0]) {
         std::complex<double> value=pointList[i]->get_fieldValue(quantity);
         double scaleValue=0;
         if (quantity.compare("G") == 0) {
            scaleValue=real(value/(acceptedPower/totalArea));   // accepted power
            scaleValue=10*log10(scaleValue);
         } else if (quantity.compare("D") == 0) {
            scaleValue=real(value/(radiatedPower/totalArea));   // radiated power
            scaleValue=10*log10(scaleValue);
         } else {
            scaleValue=abs(value/maxValue);
            scaleValue=20*log10(scaleValue);
         }

         vertexValues[i]=scaleValue;
         i++;
      }

      // collect data
      i=1;
      while (i < size) {
         while (1) {
            int j;
            MPI_Recv(&j,1,MPI_INT,i,102,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
            if (j == -1) break;

            double scaleValue;
            MPI_Recv(&scaleValue,1,MPI_DOUBLE,i,103,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

            vertexValues[j]=scaleValue;
         }
         i++;
      }

   } else {

      // get the assigned range to calculate
      int start,stop;
      MPI_Recv(&start,1,MPI_INT,0,100,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&stop,1,MPI_INT,0,101,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

      // do the work
      int i=start;
      while (i < stop) {

         std::complex<double> value=pointList[i]->get_fieldValue(quantity);
         double scaleValue=0;
         if (quantity.compare("G") == 0){
            scaleValue=real(value/(acceptedPower/totalArea));  // accepted power
            scaleValue=10*log10(scaleValue);
         } else if (quantity.compare("D") == 0){
            scaleValue=real(value/(radiatedPower/totalArea));  // radiated power
            scaleValue=10*log10(scaleValue);
         } else {
            scaleValue=abs(value/maxValue);
            scaleValue=20*log10(scaleValue);
         }

         MPI_Send(&i,1,MPI_INT,0,102,PETSC_COMM_WORLD);
         MPI_Send(&scaleValue,1,MPI_DOUBLE,0,103,PETSC_COMM_WORLD);

         i++;
      }

      // tell rank 0 that there is no more data
      i=-1;
      MPI_Send(&i,1,MPI_INT,0,102,PETSC_COMM_WORLD);
   }

   // build the mesh
   if (rank == 0) {

      // 2D surface in 3D space
      mfem::Mesh *mesh=new mfem::Mesh(2,pointList.size(),triangleList.size(),0,3);

      // apply the values to the pattern
      long unsigned int i=0;
      while (i < vertexValues.size()) {

         double scaleValue=vertexValues[i];

         // limit for polar amplitude plot
         if (!projData->antenna_plot_3D_sphere) {
            scaleValue-=valueMin;
            if (scaleValue < 0) scaleValue=0;
         }

         double theta=pointList[i]->get_theta();
         double phi=pointList[i]->get_phi();

         if (projData->antenna_plot_3D_sphere) scaleValue=1;
         mesh->AddVertex(scaleValue*sin(theta)*cos(phi),scaleValue*sin(theta)*sin(phi),scaleValue*cos(theta));
         i++;
      }

      // add mesh triangles
      i=0;
      while (i < triangleList.size()) {
         triangleList[i]->addMeshTriangle(mesh);
         i++;
      }

      // finalize mesh and have the edge connections calculated
      mesh->FinalizeTriMesh(1);

      // create a finite-element representation of the pattern
      mfem::H1_FECollection fecPattern(1,2);
      mfem::FiniteElementSpace fesPattern(mesh,&fecPattern);
      mfem::GridFunction pattern=mfem::GridFunction(&fesPattern);

      // apply the values to the pattern
      i=0;
      while (i < vertexValues.size()) {
         pattern[i]=vertexValues[i];
         i++;
      }

      // smoothing
      // To view the power plot as smoothed, set "Normal Array" under "Lighting" to "normals". 

      mfem::H1_FECollection fecPatternNormals(1,mesh->Dimension());
      mfem::FiniteElementSpace fesPatternNormals(mesh,&fecPatternNormals,mesh->SpaceDimension());

      mfem::GridFunction normals(&fesPatternNormals);
      normals=0.0;

      mfem::Array<int> multiplicity(normals.Size());
      multiplicity=0;

      int e=0;
      while (e < mesh->GetNE()) {
         mfem::ElementTransformation *T=mesh->GetElementTransformation(e);
         mfem::Array<int> vdofs;
         fesPatternNormals.GetElementVDofs(e,vdofs);
         mfem::IntegrationRule ir=fesPatternNormals.GetFE(e)->GetNodes();
         int i=0;
         while (i < ir.Size()) {
            T->SetIntPoint(&ir[i]);
            mfem::Vector normal_vector(mesh->SpaceDimension());
            CalcOrtho(T->Jacobian(),normal_vector);
            int d=0;
            while (d < mesh->SpaceDimension()) {
               const int vd=vdofs[i+d*ir.Size()];
               normals[vd]+=normal_vector[d];
               ++multiplicity[vd];
               d++;
            }
            i++;
         }
         e++;
      }

      int k=0;
      while (k < normals.Size()) {
         normals[k]/=multiplicity[k];
         k++;
      }

      // ParaView

      std::stringstream ssParaView;
      ssParaView << "ParaView_" << projData->project_name << "_FarField";

      std::stringstream ss;
      ss << quantity;
      ss << "_" << frequency;
      ss << "_SP" << Sport;

      mfem::ParaViewDataCollection *pd=nullptr;
      pd=new mfem::ParaViewDataCollection(ss.str(),mesh);
      pd->SetOwnData(false);
      pd->SetPrefixPath(ssParaView.str());
      pd->RegisterField(quantity.c_str(),&pattern);
      pd->RegisterField("normals",&normals);
      pd->SetLevelsOfDetail(3);
      pd->SetDataFormat(mfem::VTKFormat::ASCII);
      pd->SetHighOrderOutput(true);
      pd->Save();
      delete pd;

      delete mesh;
   }
}

void Sphere::calculateAngularResolution ()
{
   angularResolution=DBL_MAX;
   long unsigned int i=0;
   while (i < pointList.size()-1) {
      long unsigned int j=i+1;
      while (j < pointList.size()-1) {
         double angle=pointList[i]->get_angle(pointList[j],&center);
         if (abs(angle) < angularResolution) angularResolution=abs(angle);
         j++;
      }
      i++;
   }

   double angle=pointList[pointList.size()-1]->get_angle(pointList[0],&center);
   if (abs(angle) < angularResolution) angularResolution=abs(angle);

   angularResolution*=180/M_PI;
}

double Sphere::calculateIsotropicGain (std::complex<double> acceptedPower, double totalArea)
{
   double gain=-DBL_MAX;
   long unsigned int i=0;
   while (i < pointList.size()) {
      // point value
      std::complex<double> gaini=pointList[i]->get_power()/(acceptedPower/totalArea);
      pointList[i]->set_gain (real(gaini));

      // max value
      double gaini_db=10*log10(real(gaini));
      if (gaini_db > gain) gain=gaini_db;
      i++;
   }
   return gain;
}

double Sphere::calculateDirectivity (std::complex<double> radiatedPower, double totalArea)
{
   double directivity=-DBL_MAX;
   long unsigned int i=0;
   while (i < pointList.size()) {
      // point value
      std::complex<double> directivityi=pointList[i]->get_power()/(radiatedPower/totalArea);
      pointList[i]->set_directivity(real(directivityi));

      // max value
      double directivityi_db=10*log10(real(directivityi));
      if (directivityi_db > directivity) directivity=directivityi_db;
      i++;
   }
   return directivity;
}

void Sphere::print ()
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank == 0) {
      std::cout << "   Sphere: " << this << std::endl;
      std::cout << "      radius=" << radius << std::endl;
      std::cout << "      center=(" << center.get_x() << "," << center.get_y() << "," << center.get_z() << ")" << std::endl;
      std::cout << "      pointList=" << &pointList << std::endl;
      long unsigned int i=0;
      while (i < pointList.size()) {
         pointList[i]->print();
         i++;
      }
      std::cout << "      triangleList=" << &triangleList << std::endl;
   }
}

void Sphere::save (std::ostream *out)
{
   if (hasSaved) return;

   *out << "#3D pattern" << std::endl;
   if (pointList.size() > 0) pointList[0]->saveHeader(out,areaList[0]);
   long unsigned int i=0;
   while (i < pointList.size()) {
      pointList[i]->save(out,areaList[i]);
      i++;
   }

   hasSaved=true;
}

Sphere::~Sphere ()
{
   long unsigned int i=0;
   while (i < pointList.size()) {
      if (pointList[i]) delete pointList[i];
      pointList[i]=nullptr;
      i++;
   }
   pointList.clear();

   i=0;
   while (i < triangleList.size()) {
      if (triangleList[i]) delete triangleList[i];
      triangleList[i]=nullptr;
      i++;
   }
   triangleList.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////
// Circle
///////////////////////////////////////////////////////////////////////////////////////////

Circle::Circle ()
{
   radius=1;
   center.set_xyz(0,0,0);
   plane="xy";
   theta=0;
   phi=0;
   latitude=0;
   rotation=0;
   nPoints=360;
}

bool Circle::has_named_plane ()
{
   if (plane.compare("xy") == 0) return true;
   if (plane.compare("xz") == 0) return true;
   if (plane.compare("yz") == 0) return true;
   return false;
}

Circle* Circle::clone ()
{
   Circle *b=new Circle();
   b->radius=radius;
   b->center.copy(&center);
   b->plane=plane;
   b->theta=theta;
   b->phi=phi;
   b->latitude=latitude;
   b->rotation=rotation;
   b->nPoints=nPoints;

   long unsigned int i=0;
   while (i < pointList.size()) {

      OPEMpoint *point=new OPEMpoint();
      point->copy(pointList[i]);
      b->pointList.push_back(point);

      struct point xyz;
      xyz.x=xyzList[i].x;
      xyz.y=xyzList[i].y; 
      xyz.z=xyzList[i].z; 
      xyz.dim=3;
      xyzList.push_back(xyz);

      i++;
   }

   b->angularResolution=angularResolution;

   return b;
}

bool Circle::is_match (Circle *a)
{
   double tol=1e-12;
   if (!a) return false;
   if (!double_compare(radius,a->radius,tol)) return false;
   if (!center.is_match(&(a->center))) return false;
   if (!double_compare(theta,a->theta,tol)) return false;
   if (!double_compare(phi,a->phi,tol)) return false;
   if (!double_compare(latitude,a->latitude,tol)) return false;
   if (!double_compare(rotation,a->rotation,tol)) return false;
   return true;
}

void rotate_point (double theta, double phi, double *x, double *y, double *z)
{
   double xpartial=cos(theta)*(*x)+sin(theta)*(*z);
   double ypartial=*y;
   double zpartial=-sin(theta)*(*x)+cos(theta)*(*z);

   *x=cos(phi)*xpartial-sin(phi)*ypartial;
   *y=sin(phi)*xpartial+cos(phi)*ypartial;
   *z=zpartial;
}

std::string get_axis_label (double x, double y, double z)
{
   std::string label="";

   if (double_compare(x,1,1e-12) && double_compare(y,0,1e-12) && double_compare(z,0,1e-12)) {label="x";}
   if (double_compare(x,-1,1e-12) && double_compare(y,0,1e-12) && double_compare(z,0,1e-12)) {label="-x";}

   if (double_compare(x,0,1e-12) && double_compare(y,1,1e-12) && double_compare(z,0,1e-12)) {label="y";}
   if (double_compare(x,0,1e-12) && double_compare(y,-1,1e-12) && double_compare(z,0,1e-12)) {label="-y";}

   if (double_compare(x,0,1e-12) && double_compare(y,0,1e-12) && double_compare(z,1,1e-12)) {label="z";}
   if (double_compare(x,0,1e-12) && double_compare(y,0,1e-12) && double_compare(z,-1,1e-12)) {label="-z";}

   return label;
}

double get_plot_rotation (std::string *xlabel, std::string *ylabel)
{
   if (xlabel->compare("x") == 0) {
      return 0;
   }

   if (xlabel->compare("-x") == 0) {
      *xlabel="x";
      if (ylabel->compare("y") == 0) *ylabel="-y";
      else if (ylabel->compare("-y") == 0) *ylabel="y";
      else if (ylabel->compare("z") == 0) *ylabel="-z";
      else if (ylabel->compare("-z") == 0) *ylabel="z";
      else if (ylabel->compare("") == 0) *ylabel="";
      return 180;
   }

   if (ylabel->compare("x") == 0) {
      if (xlabel->compare("y") == 0) *ylabel="-y";
      else if (xlabel->compare("-y") == 0) *ylabel="y";
      else if (xlabel->compare("z") == 0) *ylabel="-z";
      else if (xlabel->compare("-z") == 0) *ylabel="z";
      else if (xlabel->compare("") == 0) *ylabel="";
      *xlabel="x";
      return -90;
   }

   if (ylabel->compare("-x") == 0) {
      if (xlabel->compare("y") == 0) *ylabel="y";
      else if (xlabel->compare("-y") == 0) *ylabel="-y";
      else if (xlabel->compare("z") == 0) *ylabel="z";
      else if (xlabel->compare("-z") == 0) *ylabel="-z";
      else if (xlabel->compare("") == 0) *ylabel="";
      *xlabel="x";
      return 90;
   }

   if (xlabel->compare("y") == 0) {
      return 0;
   }

   if (xlabel->compare("-y") == 0) {
      if (ylabel->compare("x") == 0) *ylabel="-x";
      else if (ylabel->compare("-x") == 0) *ylabel="x";
      else if (ylabel->compare("z") == 0) *ylabel="-z";
      else if (ylabel->compare("-z") == 0) *ylabel="z";
      else if (ylabel->compare("") == 0) *ylabel="";
      *xlabel="y";
      return 180;
   }

   if (ylabel->compare("y") == 0) {
      if (xlabel->compare("x") == 0) *ylabel="-x";
      else if (xlabel->compare("-x") == 0) *ylabel="x";
      else if (xlabel->compare("z") == 0) *ylabel="-z";
      else if (xlabel->compare("-z") == 0) *ylabel="z";
      else if (xlabel->compare("") == 0) *ylabel="";
      *xlabel="y";
      return -90;
   }

   if (ylabel->compare("-y") == 0) {
      if (xlabel->compare("x") == 0) *ylabel="x";
      else if (xlabel->compare("-y") == 0) *ylabel="-y";
      else if (xlabel->compare("z") == 0) *ylabel="z";
      else if (xlabel->compare("-z") == 0) *ylabel="-z";
      else if (xlabel->compare("") == 0) *ylabel="";
      *xlabel="y";
      return 90;
   }

   return 0;
}

void rotate_labels (double rotation, std::string *xlabel, std::string *ylabel)
{
   double x=1;
   double y=0;
   double z=0;
   rotate_point(0,rotation,&x,&y,&z);
   double theta=atan2(y,x);

   // 0 degrees
   if (double_compare(abs(theta),0,1e-12)) {
      // no changes
   }

   // +90 degrees
   else if (double_compare(theta,M_PI/2,1e-12)) {
      std::string temp=*ylabel;
      *ylabel=*xlabel;
      if (temp.compare("x") == 0) *xlabel="-x";
      else if (temp.compare("-x") == 0) *xlabel="x";
      else if (temp.compare("y") == 0) *xlabel="-y";
      else if (temp.compare("-y") == 0) *xlabel="y";
      else if (temp.compare("z") == 0) *xlabel="-z";
      else if (temp.compare("-z") == 0) *xlabel="z";
      else *xlabel="";
   }

   // -90 degrees
   else if (double_compare(theta,-M_PI/2,1e-12)) {
      std::string temp=*xlabel;
      *xlabel=*ylabel;
      if (temp.compare("x") == 0) *ylabel="-x";
      else if (temp.compare("-x") == 0) *ylabel="x";
      else if (temp.compare("y") == 0) *ylabel="-y";
      else if (temp.compare("-y") == 0) *ylabel="y";
      else if (temp.compare("z") == 0) *ylabel="-z";
      else if (temp.compare("-z") == 0) *ylabel="z";
      else *ylabel="";
   } 

   // +/-180 degrees
   else if (double_compare(abs(theta),M_PI,1e-12)) {
      std::string temp=*xlabel;
      if (temp.compare("x") == 0) *xlabel="-x";
      else if (temp.compare("-x") == 0) *xlabel="x";
      else if (temp.compare("y") == 0) *xlabel="-y";
      else if (temp.compare("-y") == 0) *xlabel="y";
      else if (temp.compare("z") == 0) *xlabel="-z";
      else if (temp.compare("-z") == 0) *xlabel="z";
      else *xlabel="";

      temp=*ylabel;
      if (temp.compare("x") == 0) *ylabel="-x";
      else if (temp.compare("-x") == 0) *ylabel="x";
      else if (temp.compare("y") == 0) *ylabel="-y";
      else if (temp.compare("-y") == 0) *ylabel="y";
      else if (temp.compare("z") == 0) *ylabel="-z";
      else if (temp.compare("-z") == 0) *ylabel="z";
      else *ylabel="";
   } 

   else {
      *xlabel="";
      *ylabel="";
   }
}

void Circle::create ()
{
   double delAngle=2*M_PI/nPoints;
   double angle=0;

   // loop through the angles
   int i=0;
   while (i < nPoints) {

      // draw the circle on the x-y plane
      double x=radius*cos(angle)*cos(latitude)+center.get_x();
      double y=radius*sin(angle)*cos(latitude)+center.get_y();
      double z=radius*sin(latitude)+center.get_z();

      // save the 2D circle for making reports
      struct point xyz;
      xyz.x=x;
      xyz.y=y;
      xyz.z=0;
      xyz.dim=3;
      xyzList.push_back(xyz);

      // rotate the plane in 3D
      rotate_point(theta,phi,&x,&y,&z);

      // new point
      OPEMpoint *point=new OPEMpoint(x,y,z);
      point->set_theta(&center);
      point->set_phi(&center);
      pointList.push_back(point);

      angle+=delAngle;
      i++;
   }

   calculateAngularResolution();
}

void Circle::createPatternMesh (struct projectData *projData, double totalArea, std::complex<double> acceptedPower, std::complex<double> radiatedPower,
                                Sphere *sphere, std::string quantity, double frequency, int Sport)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   if (rank != 0) return;

   if (quantity.compare("") == 0) return;
   if (!projData->antenna_plot_3D_save) return;

   // 1D surface in 3D space
   mfem::Mesh *mesh=new mfem::Mesh(1,pointList.size(),pointList.size(),0,3);

   // set limits to control plotting range

   std::complex<double> maxValue=sphere->getMaxValue(quantity);
   double scaleMaxValue=0;
   if (quantity.compare("G") == 0){
      scaleMaxValue=real(maxValue/(acceptedPower/totalArea));
      scaleMaxValue=10*log10(scaleMaxValue);
   }
   if (quantity.compare("D") == 0){
      scaleMaxValue=real(maxValue/(radiatedPower/totalArea));
      scaleMaxValue=10*log10(scaleMaxValue);
   }

   double valueMax=-1000;
   while (valueMax-scaleMaxValue < 0) valueMax+=projData->antenna_plot_2D_interval;
   double valueMin=valueMax-projData->antenna_plot_2D_range;

   // vector at the vertices
   std::vector<double> vertexValues;

   // loop for each point
   long unsigned int i=0;
   while (i < pointList.size()) {

      std::complex<double> fieldValue=pointList[i]->get_fieldValue(quantity);

      double scaleValue=0;
      if (quantity.compare("G") == 0){
         scaleValue=real(fieldValue/(acceptedPower/totalArea));
         scaleValue=10*log10(scaleValue);
      } else if (quantity.compare("D") == 0){
         scaleValue=real(fieldValue/(radiatedPower/totalArea));
         scaleValue=10*log10(scaleValue); 
      } else {
         scaleValue=abs(fieldValue/maxValue);
         scaleValue=20*log10(scaleValue);
      }
      vertexValues.push_back(scaleValue);

      scaleValue-=valueMin;
      if (scaleValue < 0) scaleValue=0;

      double theta=pointList[i]->get_theta();
      double phi=pointList[i]->get_phi();
      mesh->AddVertex(scaleValue*sin(theta)*cos(phi),scaleValue*sin(theta)*sin(phi),scaleValue*cos(theta));
      i++;
   }

   // add mesh segments
   i=0;
   while (i < pointList.size()-1) {
      mesh->AddSegment(i,i+1);
      i++;
   }
   mesh->AddSegment(pointList.size()-1,0);

   // finalize mesh
   mesh->FinalizeMesh();

   // finite element representation of the pattern
   mfem::H1_FECollection fecPattern(1,2);
   mfem::FiniteElementSpace fesPattern(mesh,&fecPattern);
   mfem::GridFunction pattern=mfem::GridFunction(&fesPattern);

   // apply the values to the pattern
   i=0;
   while (i < vertexValues.size()) {
      pattern[i]=vertexValues[i];
      i++;
   }

   // ParaView

   std::stringstream ssParaView;
   ssParaView << "ParaView_" << projData->project_name << "_FarField";

   std::stringstream ss;
   ss << "slice";
   ss << "_" << quantity;
   ss << "_" << frequency;
   if (has_named_plane()) ss << "_" << plane;
   else ss << "_" << theta*180/M_PI << "_" << phi*180/M_PI;
   ss << "_" << latitude*180/M_PI;
   ss << "_" << rotation*180/M_PI;
   ss << "_SP" << Sport;

   mfem::ParaViewDataCollection *pd=nullptr;
   pd=new mfem::ParaViewDataCollection(ss.str(),mesh);
   pd->SetOwnData(false);
   pd->SetPrefixPath(ssParaView.str());
   pd->RegisterField(quantity.c_str(),&pattern);
   pd->SetLevelsOfDetail(3);
   pd->SetDataFormat(mfem::VTKFormat::ASCII);
   pd->SetHighOrderOutput(true);
   pd->Save();
   delete pd;

   delete mesh;
}

// ToDo: update with byte counts for offsets
bool Circle::createReport (struct projectData *projData, std::string quantity1, std::string quantity2,
                           std::complex<double> acceptedPower, std::complex<double> radiatedPower, Sphere *sphere, double frequency, int Sport,
                           double totalArea, double gain, double directivity, double radiationEfficiency)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank != 0) return false;

   // directory to hold reports

   std::stringstream farFieldDir;
   farFieldDir << "Report_" << projData->project_name << "_FarField";

   if (!std::filesystem::exists(farFieldDir.str().c_str())) {
      if (! std::filesystem::create_directory(farFieldDir.str().c_str())) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3196: Failed to create report directory \"%s\".\n",farFieldDir.str().c_str());
         return true;
      }
   }

   try {
      std::filesystem::current_path(farFieldDir.str().c_str());
   } catch (std::filesystem::filesystem_error const& ex) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3002: Missing report directory \"%s\".\n",farFieldDir.str().c_str());
      return true;
   }

   int width=2000;
   int height=2000*11/8.5;
   int lineWidth=6;
   double figureScale=0.8;

   double totalSphereArea=sphere->getTotalArea();
   double maxAbsValue=sphere->getMaxAbsValue(quantity1);
   double maxAbsValue2=sphere->getMaxAbsValue(quantity2);
   if (maxAbsValue2 > maxAbsValue) maxAbsValue=maxAbsValue2;

   // set limits to control plotting range

   std::complex<double> maxValue=sphere->getMaxValue(quantity1);
   double scaleMaxValue=0;
   if (quantity1.compare("G") == 0){
      scaleMaxValue=real(maxValue/(acceptedPower/totalArea));
      scaleMaxValue=10*log10(scaleMaxValue);
   }
   if (quantity1.compare("D") == 0){
      scaleMaxValue=real(maxValue/(radiatedPower/totalArea));
      scaleMaxValue=10*log10(scaleMaxValue);
   }

   double valueMax=-1000;
   while (valueMax-scaleMaxValue < 0) valueMax+=projData->antenna_plot_2D_interval;
   double valueMin=valueMax-projData->antenna_plot_2D_range;

   // filename
   std::stringstream ss;
   ss << "radiation_pattern";
   if (quantity1.compare("") != 0) ss << "_" << quantity1;
   if (quantity2.compare("") != 0) ss << "_" << quantity2;
   ss << "_" << frequency;
   if (has_named_plane()) ss << "_" << plane;
   else ss << "_" << theta*180/M_PI << "_" << phi*180/M_PI;
   ss << "_" << latitude*180/M_PI;
   ss << "_" << rotation*180/M_PI;
   ss << "_SP" << Sport;
   ss << ".pdf";

   // report

   bool fail=false;
   std::ofstream patternout(ss.str().c_str());
   if (patternout.is_open()) {
      patternout.precision(15);

      patternout << "%PDF-1.4" << std::endl;
      patternout << "1 0 obj" << std::endl
                 << "   << /Type /Catalog" << std::endl
                 << "      /Outlines 2 0 R" << std::endl
                 << "      /Pages 3 0 R" << std::endl
                 << "   >>" << std::endl
                 << "endobj" << std::endl;
      patternout << "2 0 obj" << std::endl
                 << "   << /Type /Outlines" << std::endl
                 << "   /Count 0" << std::endl
                 << "   >>" << std::endl
                 << "endobj" << std::endl;
      patternout << "3 0 obj" << std::endl
                 << "   << /Type /Pages" << std::endl
                 << "      /Kids [ 4 0 R ]" << std::endl
                 << "      /Count 1" << std::endl
                 << "   >>" << std::endl
                 << "endobj" << std::endl;
      patternout << "4 0 obj" << std::endl
                 << "   << /Type /Page" << std::endl
                 << "      /Parent 3 0 R" << std::endl
                 << "      /MediaBox [ 0 0 " << width << " " << height << " ]" << std::endl
                 << "      /Contents 5 0 R" << std::endl
                 << "      /Resources << /ProcSet 6 0 R" << std::endl
                 << "                    /Font << /F1 7 0 R" << std::endl
                 << "                 >>" << std::endl
                 << "   >> >>" << std::endl
                 << "endobj" << std::endl;

      patternout << "5 0 obj" << std::endl
                 << "   << /Length 883 >>" << std::endl    // not correct
                 << "   stream" << std::endl;

      // set width
      patternout << "      " << lineWidth << " w" << std::endl;

      // determine axis labels 

      double x=1;
      double y=0;
      double z=0;
      rotate_point(theta,phi,&x,&y,&z);
      std::string xlabel=get_axis_label(x,y,z);

      x=0;
      y=1;
      z=0;
      rotate_point(theta,phi,&x,&y,&z);
      std::string ylabel=get_axis_label(x,y,z);

      double plot_rotation_angle=get_plot_rotation(&xlabel,&ylabel)*M_PI/180;

      // adjust for user rotation
      plot_rotation_angle+=rotation;
      rotate_labels (rotation,&xlabel,&ylabel);

      // plots

      int xpinit=0;
      int ypinit=0;

      // quantity1

      long unsigned int i=0;
      while (i < pointList.size()) {

         // unrotated coordinates in the x-y plane
         double x=xyzList[i].x;
         double y=xyzList[i].y;
         double z=0;
         rotate_point(0,plot_rotation_angle,&x,&y,&z);

         double angle=atan2(y,x);

         // plot value

         std::complex<double> fieldValue=pointList[i]->get_fieldValue(quantity1);

         double plotValue=0;
         if (quantity1.compare("G") == 0) plotValue=real(fieldValue);
         else if (quantity1.compare("D") == 0) plotValue=real(fieldValue);
         else plotValue=abs(fieldValue);

         // scale
         if (quantity1.compare("G") == 0) plotValue/=(abs(acceptedPower)/totalSphereArea);
         else if (quantity1.compare("D") == 0) plotValue/=(abs(radiatedPower)/totalSphereArea);
         else plotValue/=maxAbsValue;

         // offset for plotting
         if (quantity1.compare("G") == 0) plotValue=10*log10(plotValue);
         else if (quantity1.compare("D") == 0) plotValue=10*log10(plotValue);
         else plotValue=20*log10(plotValue);
         plotValue-=valueMin;
         if (plotValue < 0) plotValue=0;

         // plot location
         int xp=width/2+plotValue/projData->antenna_plot_2D_range*figureScale*width/2*cos(angle)+0.5;
         int yp=height-width/2+plotValue/projData->antenna_plot_2D_range*figureScale*width/2*sin(angle)+0.5;

         if (i == 0) {
            patternout << "      " << xp << " " << yp << " m" << std::endl;
            xpinit=xp;
            ypinit=yp;
         } else patternout << "      " << xp << " " << yp << " l" << std::endl;

         i++;
      }
      patternout << "      " << xpinit << " " << ypinit << " l" << std::endl;
      patternout << "      S" << std::endl;

      // quantity2

      if (quantity2.compare("") != 0) {

         // set to dash
         patternout << "      " << "[" << lineWidth << " " << lineWidth << "] 0 d" << std::endl;

         i=0;
         while (i < pointList.size()) {

            // unrotated coordinates in the x-y plane
            double x=xyzList[i].x;
            double y=xyzList[i].y;

            double angle=atan2(y,x);

            // plot value

            std::complex<double> fieldValue=pointList[i]->get_fieldValue(quantity2);

            double plotValue=0;
            if (quantity2.compare("G") == 0) plotValue=real(fieldValue);
            else if (quantity2.compare("D") == 0) plotValue=real(fieldValue);
            else plotValue=abs(fieldValue);

            // scale
            if (quantity2.compare("G") == 0) plotValue/=(abs(acceptedPower)/totalSphereArea);
            else if (quantity2.compare("D") == 0) plotValue/=(abs(radiatedPower)/totalSphereArea);
            else plotValue/=maxAbsValue; 

            // offset for plotting
            if (quantity2.compare("G") == 0) plotValue=10*log10(plotValue);
            else if (quantity2.compare("D") == 0) plotValue=10*log10(plotValue);
            else plotValue=20*log10(plotValue);
            plotValue-=valueMin;
            if (plotValue < 0) plotValue=0;

            // plot location
            int xp=width/2+plotValue/projData->antenna_plot_2D_range*figureScale*width/2*cos(angle)+0.5;
            int yp=height-width/2+plotValue/projData->antenna_plot_2D_range*figureScale*width/2*sin(angle)+0.5;

            if (i == 0) {
               patternout << "      " << xp << " " << yp << " m" << std::endl;
               xpinit=xp;
               ypinit=yp;
            } else patternout << "      " << xp << " " << yp << " l" << std::endl;

            i++;
         }
         patternout << "      " << xpinit << " " << ypinit << " l" << std::endl;
         patternout << "      S" << std::endl;
      }

      // set width to 1 point and solid line
      patternout << "      " << "1 w" << std::endl;
      patternout << "      [] 0 d" << std::endl;

      // circles

      double radius=valueMax-valueMin;
      double label=valueMax;
      double k=0.552284749831;
      int cx=width/2+0.5;
      int cy=height-width/2+0.5;
      int circleCount=1;

      bool hasZero=false;
      while (radius > 1e-12) {
         if (label > -1e-5 && label < 1e-5) {hasZero=true; break;}
         radius-=projData->antenna_plot_2D_interval;
         label-=projData->antenna_plot_2D_interval;
      }

      radius=valueMax-valueMin;
      label=valueMax;
      while (radius > 1e-12) {
         int r=radius/projData->antenna_plot_2D_range*figureScale*width/2+0.5;

         patternout << "      " << "1 w" << std::endl;  // width=1pt

         bool solidLine=false;
         bool isZeroCircle=false;
         if (label > -1e-5 && label < 1e-5) isZeroCircle=true;

         if (isZeroCircle) solidLine=true;

         if (circleCount == 1) {
             if (label > -1e-5 && label < 1e-5) solidLine=true;
             if (!hasZero) solidLine=true;
         }

         if (solidLine) patternout << "      [] 0 d" << std::endl;
         else           patternout << "      [1 2] 0 d" << std::endl;

         // circle
         patternout << "      " << cx-r << " " << cy << " m" << std::endl
                    << "      " << cx-r << " " << cy+k*r << " " << cx-k*r << " " << cy+r << " " << cx << " " << cy+r << " c" << std::endl
                    << "      " << cx+k*r << " " << cy+r << " " << cx+r << " " << cy+k*r << " " << cx+r << " " << cy << " c" << std::endl
                    << "      " << cx+r << " " << cy-k*r << " " << cx+k*r << " " << cy-r << " " << cx << " " << cy-r << " c" << std::endl
                    << "      " << cx-k*r << " " << cy-r << " " << cx-r << " " << cy-k*r << " " << cx-r << " " << cy << " c" << std::endl
                    << "      S" << std::endl;

         // label
         patternout << "      BT" << std::endl
                    << "         /F1 28 Tf" << std::endl
                    << "         " << cx-r+10 << " " << cy+10 << " Td" << std::endl
                    << "         (" << label << ") Tj" << std::endl
                    << "      ET" << std::endl;

         radius-=projData->antenna_plot_2D_interval;
         label-=projData->antenna_plot_2D_interval;
         circleCount++;
      }

      // dB label
      radius=valueMax-valueMin;
      int r=radius/projData->antenna_plot_2D_range*figureScale*width/2+0.5;

      patternout << "      BT" << std::endl
                 << "         /F1 28 Tf" << std::endl
                 << "         " << cx-r-60 << " " << cy+10 << " Td" << std::endl
                 << "         (" << "dB" << ") Tj" << std::endl
                 << "      ET" << std::endl;

      // axis labels

      int letterOffset=8;
      patternout << "      BT" << std::endl
         << "         /F1 36 Tf" << std::endl
         << "         " << cx+r+10 << " " << cy-letterOffset << " Td" << std::endl
         << "         (" << xlabel << ") Tj" << std::endl
         << "      ET" << std::endl;
      patternout << "      BT" << std::endl
         << "         /F1 36 Tf" << std::endl
         << "         " << cx-letterOffset << " " << cy+r+10 << " Td" << std::endl
         << "         (" << ylabel << ") Tj" << std::endl
         << "      ET" << std::endl;

      // draw x-axis
      patternout << "      " << "[1 1] 0 d" << std::endl;
      patternout << "      " << cx-r << " " << cy << " m" << std::endl
                 << "      " << cx+r << " " << cy << " l" << std::endl
                 << "      S" << std::endl;

      // draw y-axis
      patternout << "      " << cx << " " << cy-r << " m" << std::endl
                 << "      " << cx << " " << cy+r << " l" << std::endl
                 << "      S" << std::endl;

      int startRow=height-width;
      int startCol=width*(1-figureScale)/2+0.5;
      int rowSpace=75;

      int row=startRow+width*(1-figureScale)/2+0.5;
      int col=startCol;

      if (quantity2.compare("") != 0) {row+=rowSpace;}

      // quantity1 label
      if (quantity1.compare("") != 0) {
         patternout << "      " << lineWidth << " w" << std::endl;
         patternout << "      " << "[] 0 d" << std::endl;

         patternout << "      " << col << " " << row+15 << " m" << std::endl
                    << "      " << col+100 << " " << row+15 << " l" << std::endl
                    << "      S" << std::endl;

         patternout << "      BT" << std::endl
                    << "         /F1 36 Tf" << std::endl
                    << "         " << col+120 << " " << row << " Td" << std::endl
                    << "         (" << quantity1 << ") Tj" << std::endl
                    << "      ET" << std::endl;
      }

      // quantity2 label
      if (quantity2.compare("") != 0) {
         row-=rowSpace;

         patternout << "      " << lineWidth << " w" << std::endl;
         patternout << "      " << "[" << lineWidth << " " << lineWidth << "] 0 d" << std::endl;

         patternout << "      " << col << " " << row+15 << " m" << std::endl
                    << "      " << col+100 << " " << row+15 << " l" << std::endl
                    << "      S" << std::endl;

         patternout << "      BT" << std::endl
                    << "         /F1 36 Tf" << std::endl
                    << "         " << col+120 << " " << row << " Td" << std::endl
                    << "         (" << quantity2 << ") Tj" << std::endl
                    << "      ET" << std::endl;
      }

      if (projData->antenna_plot_2D_annotations) {

         row=startRow;
         col=startCol;

         // frequency

         double displayFrequency=frequency;
         std::string displayFrequencyUnit="Hz";
         if (displayFrequency < 1e6*0.999) {displayFrequency/=1e3; displayFrequencyUnit="kHz";}
         else if (displayFrequency < 1e9*0.999) {displayFrequency/=1e6; displayFrequencyUnit="MHz";}
         else if (displayFrequency < 1e12*0.999) {displayFrequency/=1e9; displayFrequencyUnit="GHz";}
         else if (displayFrequency < 1e15*0.999) {displayFrequency/=1e12; displayFrequencyUnit="THz";}

         patternout << "      BT" << std::endl
                    << "         /F1 36 Tf" << std::endl
                    << "         " << col << " " << row << " Td" << std::endl
                    << "         (frequency: " << displayFrequency << " " << displayFrequencyUnit << ") Tj" << std::endl
                    << "      ET" << std::endl;

         // slice
         row-=rowSpace;
         int tab=100;
         patternout << "      BT" << std::endl
                    << "         /F1 36 Tf" << std::endl
                    << "         " << col << " " << row << " Td" << std::endl
                    << "         (slice:) Tj" << std::endl
                    << "      ET" << std::endl;
         if (has_named_plane()) {
            patternout << "      BT" << std::endl
                       << "         /F1 36 Tf" << std::endl
                       << "         " << col+tab << " " << row << " Td" << std::endl
                       << "         (" << plane << ") Tj" << std::endl
                       << "      ET" << std::endl;
         } else {
            patternout << "      BT" << std::endl
                       << "         /F1 36 Tf" << std::endl
                       << "         " << col+tab << " " << row << " Td" << std::endl
                       << std::setprecision(4) << "         (theta=" << theta*180/M_PI << " deg) Tj" << std::endl
                       << "      ET" << std::endl;
            row-=rowSpace;
            patternout << "      BT" << std::endl
                       << "         /F1 36 Tf" << std::endl
                       << "         " << col+tab << " " << row << " Td" << std::endl
                       << std::setprecision(4) << "         (phi=" << phi*180/M_PI << " deg) Tj" << std::endl
                       << "      ET" << std::endl;
         }
         row-=rowSpace;
         patternout << "      BT" << std::endl
                    << "         /F1 36 Tf" << std::endl
                    << "         " << col+tab << " " << row << " Td" << std::endl
                    << std::setprecision(4) << "         (latitude=" << latitude*180/M_PI << " deg) Tj" << std::endl
                    << "      ET" << std::endl;



         // Sport
         row-=rowSpace;
         patternout << "      BT" << std::endl
                    << "         /F1 36 Tf" << std::endl
                    << "         " << col << " " << row << " Td" << std::endl
                    << "         (S-port: " << Sport << ") Tj" << std::endl
                    << "      ET" << std::endl;

         row=startRow;
         col=startCol+500;

         // gain
         patternout << "      BT" << std::endl
                    << "         /F1 36 Tf" << std::endl
                    << "         " << col << " " << row << " Td" << std::endl
                    << std::setprecision(4) << "         (gain: " << gain << " dBi) Tj" << std::endl
                    << "      ET" << std::endl;

         // directivity
         row-=rowSpace;
         patternout << "      BT" << std::endl
                    << "         /F1 36 Tf" << std::endl
                    << "         " << col << " " << row << " Td" << std::endl
                    << std::setprecision(4) << "         (directivity: " << directivity << " dBi) Tj" << std::endl
                    << "      ET" << std::endl;

         row=startRow;
         col=startCol+1100;

         // circle and sphere resolutions
         patternout << "      BT" << std::endl
                    << "         /F1 36 Tf" << std::endl
                    << "         " << col << " " << row << " Td" << std::endl
                    << std::setprecision(3) << "         (sphere resolution: " << sphere->get_angularResolution() << " deg) Tj" << std::endl
                    << "      ET" << std::endl;

         row-=rowSpace;
         patternout << "      BT" << std::endl
                    << "         /F1 36 Tf" << std::endl
                    << "         " << col << " " << row << " Td" << std::endl
                    << std::setprecision(3) << "         (circle resolution: " << angularResolution << " deg) Tj" << std::endl
                    << "      ET" << std::endl;

      }

      patternout << "   endstream" << std::endl
                 << "endobj" << std::endl;


      patternout << "6 0 obj" << std::endl
                 << "   [ /PDF /Text]" << std::endl
                 << "endobj" << std::endl;

      patternout << "7 0 obj" << std::endl
                 << "   << /Type /Font" << std::endl
                 << "      /Subtype /Type1" << std::endl
                 << "      /BaseFont /Helvetica" << std::endl
                 << "      /Encoding /MacRomanEncoding" << std::endl
                 << "   >>" << std::endl;

      patternout << "xref" << std::endl
                 << "0 8" << std::endl
                 << "0000000000 65535 f" << std::endl
                 << "0000000009 00000 n" << std::endl
                 << "0000000074 00000 n" << std::endl
                 << "0000000120 00000 n" << std::endl
                 << "0000000179 00000 n" << std::endl
                 << "0000000300 00000 n" << std::endl
                 << "0000001532 00000 n" << std::endl
                 << "0000000800 00000 n" << std::endl;

      patternout << "trailer" << std::endl
                 << "   << /Size 8" << std::endl
                 << "      /Root 1 0 R" << std::endl
                 << "   >>" << std::endl
                 << "startxref" << std::endl
                 << "1556" << std::endl                        // not correct
                 << "%%EOF" << std::endl;

      patternout.close();
   } else fail=true;

   // cd back 
   std::filesystem::current_path("../");

   return fail;
}

void Circle::calculateAngularResolution ()
{
   angularResolution=DBL_MAX;
   long unsigned int i=0;
   while (i < pointList.size()) {
      OPEMpoint *next;
      if (i == pointList.size()-1) next=pointList[0];
      else next=pointList[i+1];
      double angle=pointList[i]->get_angle(next,&center);
      if (abs(angle) < angularResolution) angularResolution=abs(angle);
      i++;
   }

   angularResolution*=180/M_PI;
}

double Circle::calculateIsotropicGain (std::complex<double> acceptedPower, double totalArea)
{
   double gain=-DBL_MAX;
   long unsigned int i=0;
   while (i < pointList.size()) {
      // point value
      std::complex<double> gaini=pointList[i]->get_power()/(acceptedPower/totalArea);
      pointList[i]->set_gain (real(gaini));

      // max value
      double gaini_db=10*log10(real(gaini));
      if (gaini_db > gain) gain=gaini_db;
      i++;
   }
   return gain;
}

double Circle::calculateDirectivity (std::complex<double> radiatedPower, double totalArea)
{
   double directivity=-DBL_MAX;
   long unsigned int i=0;
   while (i < pointList.size()) {
      // point value
      std::complex<double> directivityi=pointList[i]->get_power()/(radiatedPower/totalArea);
      pointList[i]->set_directivity(real(directivityi));

      // max value
      double directivityi_db=10*log10(real(directivityi));
      if (directivityi_db > directivity) directivity=directivityi_db;
      i++;
   }
   return directivity;
}

void Circle::print ()
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   print (rank);
}

void Circle::print (PetscMPIInt rank_)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (rank == rank_) {
      std::cout << "   Circle: " << this << "  rank=" << rank_ << std::endl;
      std::cout << "      radius=" << radius << std::endl;
      std::cout << "      center=(" << center.get_x() << "," << center.get_y() << "," << center.get_z() << ")" << std::endl;
      std::cout << "      plane=" << plane << std::endl;
      std::cout << "      theta=" << theta << std::endl;
      std::cout << "      phi=" << phi << std::endl;
      std::cout << "      latitude=" << latitude << std::endl;
      std::cout << "      rotation=" << rotation << std::endl;
      std::cout << "      nPoints=" << nPoints << std::endl;
      std::cout << "      xyzList=" << &xyzList << std::endl;
      std::cout << "      pointList=" << &pointList << std::endl;
      long unsigned int i=0;
      while (i < pointList.size()) {
         pointList[i]->print();
         i++;
      }
   }
}

void Circle::save (std::ostream *out)
{
   if (hasSaved) return;

   *out << "#2D pattern"
        << ", plane=" << plane 
        << ", theta=" << theta*180/M_PI << "(deg)"
        << ", phi=" << phi*180/M_PI << "(deg)"
        << ", latitude=" << latitude*180/M_PI << "(deg)"
        << ", rotation=" << rotation*180/M_PI << "(deg)" << std::endl;
   if (pointList.size() > 0) pointList[0]->saveHeader(out,0);
   long unsigned int i=0;
   while (i < pointList.size()) {
      pointList[i]->save(out,0);
      i++;
   }

   hasSaved=true;
}

Circle::~Circle ()
{
   long unsigned int i=0;
   while (i < pointList.size()) {
      if (pointList[i]) delete pointList[i];
      pointList[i]=nullptr;
      i++;
   }
   pointList.clear();
}


///////////////////////////////////////////////////////////////////////////////////////////
// Current
///////////////////////////////////////////////////////////////////////////////////////////

Current::Current (double x_, double y_, double z_,
                  std::complex<double> Jx_, std::complex<double> Jy_, std::complex<double> Jz_,
                  std::complex<double> Mx_, std::complex<double> My_, std::complex<double> Mz_,
                  double area_)
{
   x=x_;
   y=y_;
   z=z_;
   Jx=Jx_;
   Jy=Jy_;
   Jz=Jz_;
   Mx=Mx_;
   My=My_;
   Mz=Mz_;
   area=area_;
}

double Current::length ()
{
   return sqrt(x*x+y*y+z*z);
}

double Current::dotproduct (double x_, double y_, double z_)
{
   return x*x_+y*y_+z*z_;
}

Current* Current::clone ()
{
   Current *a=new Current();
   a->x=x;
   a->y=y;
   a->z=z;
   a->area=area;
   a->Jx=Jx;
   a->Jy=Jy;
   a->Jz=Jz;
   a->Mx=Mx;
   a->My=My;
   a->Mz=Mz;
   return a;
}

void Current::sendTo (int rankTo)
{
   MPI_Send(&x,1,MPI_DOUBLE,rankTo,1000,PETSC_COMM_WORLD);
   MPI_Send(&y,1,MPI_DOUBLE,rankTo,1001,PETSC_COMM_WORLD);
   MPI_Send(&z,1,MPI_DOUBLE,rankTo,1002,PETSC_COMM_WORLD);
   MPI_Send(&area,1,MPI_DOUBLE,rankTo,1003,PETSC_COMM_WORLD);

   double data=real(Jx);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1004,PETSC_COMM_WORLD);
   data=imag(Jx);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1005,PETSC_COMM_WORLD);

   data=real(Jy);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1006,PETSC_COMM_WORLD);
   data=imag(Jy);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1007,PETSC_COMM_WORLD);

   data=real(Jz);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1008,PETSC_COMM_WORLD);
   data=imag(Jz);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1009,PETSC_COMM_WORLD);

   data=real(Mx);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1010,PETSC_COMM_WORLD);
   data=imag(Mx);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1011,PETSC_COMM_WORLD);

   data=real(My);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1012,PETSC_COMM_WORLD);
   data=imag(My);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1013,PETSC_COMM_WORLD);

   data=real(Mz);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1014,PETSC_COMM_WORLD);
   data=imag(Mz);
   MPI_Send(&data,1,MPI_DOUBLE,rankTo,1015,PETSC_COMM_WORLD);
}

void Current::recvFrom (int rankFrom)
{
   MPI_Recv(&x,1,MPI_DOUBLE,rankFrom,1000,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&y,1,MPI_DOUBLE,rankFrom,1001,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&z,1,MPI_DOUBLE,rankFrom,1002,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&area,1,MPI_DOUBLE,rankFrom,1003,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);

   double dataRe,dataIm;

   MPI_Recv(&dataRe,1,MPI_DOUBLE,rankFrom,1004,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&dataIm,1,MPI_DOUBLE,rankFrom,1005,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   Jx=std::complex<double>(dataRe,dataIm);

   MPI_Recv(&dataRe,1,MPI_DOUBLE,rankFrom,1006,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&dataIm,1,MPI_DOUBLE,rankFrom,1007,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   Jy=std::complex<double>(dataRe,dataIm);

   MPI_Recv(&dataRe,1,MPI_DOUBLE,rankFrom,1008,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&dataIm,1,MPI_DOUBLE,rankFrom,1009,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   Jz=std::complex<double>(dataRe,dataIm);

   MPI_Recv(&dataRe,1,MPI_DOUBLE,rankFrom,1010,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&dataIm,1,MPI_DOUBLE,rankFrom,1011,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   Mx=std::complex<double>(dataRe,dataIm);

   MPI_Recv(&dataRe,1,MPI_DOUBLE,rankFrom,1012,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&dataIm,1,MPI_DOUBLE,rankFrom,1013,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   My=std::complex<double>(dataRe,dataIm);

   MPI_Recv(&dataRe,1,MPI_DOUBLE,rankFrom,1014,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   MPI_Recv(&dataIm,1,MPI_DOUBLE,rankFrom,1015,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   Mz=std::complex<double>(dataRe,dataIm);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Pattern
///////////////////////////////////////////////////////////////////////////////////////////

Pattern::Pattern (double frequency_, int Sport_, int iteration_, std::complex<double> acceptedPower_, int dim_,
                  std::string quantity1_, std::string quantity2_, std::string plane_,
                  double theta_, double phi_, double latitude_, double rotation_, Circle *circle_, Sphere *sphere_)
{
   quantity1=quantity1_;
   quantity2=quantity2_;
   plane=plane_;
   theta=theta_;
   phi=phi_;
   latitude=latitude_;
   rotation=rotation_;

   active=true;
   frequency=frequency_;
   Sport=Sport_;
   dim=dim_;
   iteration=iteration_;
   acceptedPower=acceptedPower_;

   circle=circle_;
   sphere=sphere_;

   totalArea=-DBL_MAX;
   radiatedPower=-DBL_MAX;
   gain=-DBL_MAX;
   directivity=-DBL_MAX;
   radiationEfficiency=-DBL_MAX;
}

void Pattern::copy_additional_data (Pattern *a)
{
   totalArea=a->totalArea;
   radiatedPower=a->radiatedPower;
   acceptedPower=a->acceptedPower;
   gain=a->gain;
   directivity=a->directivity;
   radiationEfficiency=a->radiationEfficiency;
}

Pattern* Pattern::clone ()
{
   Circle *cloneCircle=nullptr;
   if (circle) cloneCircle=circle->clone();

   Sphere *cloneSphere=nullptr;
   if (sphere) cloneSphere=sphere->clone();

   Pattern *b=new Pattern(frequency,Sport,radiationEfficiency,dim,iteration,quantity1,quantity2,plane,
                          theta,phi,latitude,rotation,cloneCircle,cloneSphere);
   b->active=active;
   b->copy_additional_data(this);

   return b;
}

bool Pattern::is_match (Circle *a)
{
   if (circle) {
      return circle->is_match(a);
   }
   return false;
}

bool Pattern::is_match (Sphere *a)
{
   if (sphere) {
      return sphere->is_match(a);
   }
   return false;
}

bool Pattern::is_match (double frequency_, std::string quantity_)
{
   if (!double_compare(frequency,frequency_,1e-12)) return false;
   if (quantity1.compare(quantity_) == 0) return true;
   return false;
}

bool Pattern::is_match (double frequency_, int iteration_)
{
   if (!double_compare(frequency,frequency_,1e-12)) return false;
   if (iteration != iteration_) return false;
   return true;
}

bool Pattern::is_match (double frequency_, int Sport_, int iteration_)
{
   if (iteration != iteration_) return false;
   if (!double_compare(frequency,frequency_,1e-12)) return false;
   if (Sport != Sport_) return false;
   return true;
}

bool Pattern::is_match (double frequency_, int Sport_, int iteration_, std::string quantity_)
{
   if (iteration != iteration_) return false;
   if (!double_compare(frequency,frequency_,1e-12)) return false;
   if (Sport != Sport_) return false;
   if (quantity1.compare(quantity_) != 0) return false;
   return true;
}

bool Pattern::is_match (double frequency_, int Sport_, std::string quantity_)
{
   if (!double_compare(frequency,frequency_,1e-12)) return false;
   if (Sport != Sport_) return false;
   if (quantity1.compare(quantity_) != 0) return false;
   return true;
}

// Is current a prior version of this?
bool Pattern::is_prior_version (Pattern *current)
{
   if (!current) return false;
   if (current->dim != dim) return false;
   if (current->iteration-1 != iteration) return false;
   if (!double_compare(current->frequency,frequency,1e-12)) return false;
   if (current->Sport != Sport) return false;
   if (current->quantity1.compare(quantity1) != 0) return false; 
   return true;
}

void Pattern::calculateTotalArea ()
{
   if (totalArea != -DBL_MAX) return;
   if (sphere) totalArea=sphere->getTotalArea();
}

void Pattern::calculateRadiatedPower ()
{
   if (radiatedPower != -DBL_MAX) return;
   if (sphere) radiatedPower=sphere->getRadiatedPower();
}

// ToDo: generalize for Gt and Gp
void Pattern::calculateIsotropicGain ()
{
   if (gain != -DBL_MAX) return;
   if (dim == 2 && circle) gain=circle->calculateIsotropicGain (acceptedPower,totalArea);
   if (dim == 3 && sphere) gain=sphere->calculateIsotropicGain (acceptedPower,totalArea);
}

void Pattern::calculateDirectivity ()
{
   if (directivity != -DBL_MAX) return;
   if (dim == 2 && circle) directivity=circle->calculateDirectivity (radiatedPower,totalArea);
   if (dim == 3 && sphere) directivity=sphere->calculateDirectivity (radiatedPower,totalArea);
}

// calculated using "IEEE Standard for Definition of Terms for Antennas",
// IEEE Std 145-2013
void Pattern::calculateRadiationEfficiency ()
{
   if (radiationEfficiency != -DBL_MAX) return;
   if (dim != 3) return;
   if (quantity1.compare("G") != 0 && quantity1.compare("D") != 0) return;
   radiationEfficiency=real(radiatedPower/acceptedPower);
}

void Pattern::save3DParaView (struct projectData *projData, double frequency_, int iteration_)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (dim != 3) return;
   if (acceptedPower == 0) return;
   if (totalArea == 0) return;
   if (!double_compare(frequency,frequency_,1e-12)) return;
   if (iteration != iteration_) return;

   if (projData->antenna_plot_3D_save) {
      sphere->createPatternMesh (projData,totalArea,acceptedPower,radiatedPower,quantity1,frequency,Sport);
   }

   return;
}

void Pattern::save2DParaView (struct projectData *projData, double frequency_, int iteration_)
{
   if (dim != 2) return;
   if (acceptedPower == 0) return;
   if (totalArea == 0) return;
   if (!double_compare(frequency,frequency_,1e-12)) return;
   if (iteration != iteration_) return;

   if (projData->antenna_plot_2D_save) {
      circle->createPatternMesh (projData,totalArea,acceptedPower,radiatedPower,sphere,quantity1,frequency,Sport);
      circle->createPatternMesh (projData,totalArea,acceptedPower,radiatedPower,sphere,quantity2,frequency,Sport);
      circle->createReport(projData,quantity1,quantity2,acceptedPower,radiatedPower,sphere,frequency,Sport,totalArea,gain,directivity,radiationEfficiency);
   }
}

void Pattern::print (PetscMPIInt rank_)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank); 

   bool verbose=false;

   MPI_Barrier(PETSC_COMM_WORLD);
   if (rank == rank_) {
      std::cout << "Pattern: " << this << "  rank=" << rank_ << std::endl;
      std::cout << "   active=" << active << std::endl;
      std::cout << "   iteration=" << iteration << std::endl;
      std::cout << "   frequency=" << frequency << std::endl;
      std::cout << "   Sport=" << Sport << std::endl;
      std::cout << "   dim=" << dim << std::endl;
      std::cout << "   quantity1=" << quantity1 << std::endl;
      std::cout << "   quantity2=" << quantity2 << std::endl;
      std::cout << "   plane=" << plane << std::endl;
      std::cout << "   theta=" << theta << std::endl;
      std::cout << "   phi=" << phi << std::endl;
      std::cout << "   latitude=" << latitude << std::endl;
      std::cout << "   rotation=" << rotation << std::endl;
      std::cout << "   circle=" << circle << std::endl;
      if (verbose && circle) circle->print();
      std::cout << "   sphere=" << sphere << std::endl;
      if (verbose && sphere) sphere->print();
      std::cout << "   totalArea=" << totalArea << std::endl;
      std::cout << "   radiatedPower=" << radiatedPower << std::endl;
      std::cout << "   acceptedPower=" << acceptedPower << std::endl;
      std::cout << "   gain=" << gain << std::endl;
      std::cout << "   directivity=" << directivity << std::endl;
      std::cout << "   radiationEfficiency=" << radiationEfficiency << std::endl;
   }
   MPI_Barrier(PETSC_COMM_WORLD);
}

void Pattern::save (std::ostream *out)
{
   if (!active) return;

   // header
   if (dim == 3 && sphere && !sphere->get_hasSaved()) {
      *out << "#pattern"
           << ", frequency=" << frequency
           << ", Sport=" << Sport
           << ", gain=" << gain
           << ", directivity=" << directivity
           << ", radiationEfficiency=" << radiationEfficiency
           << std::endl;
   }

   // results
   if (dim == 2 && circle) circle->save(out);
   if (dim == 3 && sphere) sphere->save(out);
}

void Pattern::save_as_test (std::ofstream *out, const char *casename, int frequency_index, int *casenumber)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   *out << casename << "_" << frequency_index+1 << "_" << (*casenumber)++ << "_farfield" << ",";
   *out << std::setprecision(15) << frequency << ",";
   *out << "gain," << Sport << ",equal," << gain << "," << equalMagLimit << std::endl;

   *out << casename << "_" << frequency_index+1 << "_" << (*casenumber)++ << "_farfield" << ",";
   *out << std::setprecision(15) << frequency << ",";
   *out << "directivity," << Sport << ",equal," << directivity << "," << equalMagLimit << std::endl;

   *out << casename << "_" << frequency_index+1 << "_" << (*casenumber)++ << "_farfield" << ",";
   *out << std::setprecision(15) << frequency << ",";
   *out << "radiationEfficiency," << Sport << ",equal," << radiationEfficiency << "," << equalMagLimit << std::endl;
}

Pattern::~Pattern ()
{
   if (circle) {delete circle; circle=nullptr;}
   if (sphere) {delete sphere; sphere=nullptr;}
}

///////////////////////////////////////////////////////////////////////////////////////////
// PatternDatabase
///////////////////////////////////////////////////////////////////////////////////////////

void PatternDatabase::addPattern (double frequency, int Sport, int iteration, std::complex<double> acceptedPower,
                int dim, std::string quantity1, std::string quantity2, std::string plane,
                double theta, double phi, double latitude, double rotation, Circle *circle, Sphere *sphere)
{
   Pattern *pattern=new Pattern(frequency,Sport,iteration,acceptedPower,dim,
                                quantity1,quantity2,plane,theta,phi,latitude,rotation,circle,sphere);
   patternList.push_back(pattern);

   // make the prior iteration inactive
   long unsigned int i=0;
   while (i< patternList.size()) {
      if (patternList[i]->is_prior_version(pattern)) patternList[i]->set_inactive();
      i++;
   }

   // keep a sorted list of unique frequencies

   // unique?
   bool found=false;
   i=0;
   while (i < unique_frequencies.size()) {
      if (fabs(unique_frequencies[i]-frequency)/unique_frequencies[i] <= tol) {
         found=true;
         break;
      }
      i++;
   }

   if (! found) {

      // save
      unique_frequencies.push_back(frequency);

      // ascending sort
      found=true;
      while (found) {
         found=false;
         i=0;
         while (i < unique_frequencies.size()-1) {
            long unsigned int j=i+1;
            while (j < unique_frequencies.size()) {
               if (unique_frequencies[j] < unique_frequencies[i]) {
                  found=true;
                  double temp=unique_frequencies[i];
                  unique_frequencies[i]=unique_frequencies[j];
                  unique_frequencies[j]=temp;
               }
               j++;
            }
            i++;
         }
      }
   }
}

Pattern* PatternDatabase::get_Pattern (double frequency, int Sport)
{
   long unsigned int i=0;
   while (i < patternList.size()) {
      if (patternList[i]->is_active() &&
          patternList[i]->get_Sport() == Sport &&
          double_compare(patternList[i]->get_frequency(),frequency,1e-12)) return patternList[i];
      i++;
   }
   return nullptr;
}

void PatternDatabase::calculateTotalArea (double frequency, int iteration)
{
   long unsigned int i=0;
   while (i< patternList.size()) {
      if (patternList[i]->is_match(frequency,iteration)) patternList[i]->calculateTotalArea();
      i++;
   }
}

void PatternDatabase::calculateRadiatedPower (double frequency, int iteration)
{
   long unsigned int i=0;
   while (i< patternList.size()) {
      if (patternList[i]->is_match(frequency,iteration)) patternList[i]->calculateRadiatedPower();
      i++;
   }
}  

void PatternDatabase::calculateIsotropicGain (double frequency, int iteration)
{
   long unsigned int i=0;
   while (i< patternList.size()) {
      if (patternList[i]->is_match(frequency,iteration)) {
         patternList[i]->calculateIsotropicGain();
      }
      i++;
   }
}

void PatternDatabase::calculateDirectivity (double frequency, int iteration)
{
   long unsigned int i=0;
   while (i< patternList.size()) {
      if (patternList[i]->is_match(frequency,iteration)) {
         patternList[i]->calculateDirectivity();
      }
      i++;
   }
}

void PatternDatabase::calculateRadiationEfficiency (double frequency, int iteration)
{
   long unsigned int i=0;
   while (i< patternList.size()) {
      if (patternList[i]->is_match(frequency,iteration)) {
         patternList[i]->calculateRadiationEfficiency();
      }
      i++;
   }
}

// see if a circle exists and return it if it does
Circle* PatternDatabase::get_circle (double frequency, int Sport, int iteration, Circle *a)
{
   Circle *circle=nullptr;
   long unsigned int i=0;
   while (i< patternList.size()) {
      if (patternList[i]->is_match(frequency,Sport,iteration) && patternList[i]->is_match(a)) {
         circle=patternList[i]->get_circle();
         break;
      }
      i++;
   }
   return circle;
}

// see if a sphere exists and return it if it does
Sphere* PatternDatabase::get_sphere (double frequency, int Sport, int iteration, Sphere *a)
{
   Sphere *sphere=nullptr;
   long unsigned int i=0;
   while (i< patternList.size()) {
      if (patternList[i]->is_match(frequency,Sport,iteration)) {
         sphere=patternList[i]->get_sphere();
         break;
      }
      i++;
   }
   return sphere;
}

// populate the sphere for 2D patterns for convenience
void PatternDatabase::populate_sphere (double frequency, int Sport, int iteration)
{
   Pattern *pattern=nullptr;
   long unsigned int i=0;
   while (i< patternList.size()) {
      pattern=patternList[i];
      if (pattern->is_3D() && pattern->is_match(frequency,Sport,iteration)) break;
      i++;
   }

   if (!pattern) return;
   if (!pattern->get_sphere()) return;

   i=0;
   while (i< patternList.size()) {
      if (patternList[i]->is_2D() && pattern->is_match(frequency,Sport,iteration)) {
         patternList[i]->set_sphere(pattern->get_sphere());
         patternList[i]->copy_additional_data(pattern);  // only the totalArea is valid when PatternDatabase::populate_sphere is called
      }
      i++;
   }
}

void PatternDatabase::saveParaView (struct projectData *projData, double frequency, int iteration)
{
   long unsigned int i=0;
   while (i< patternList.size()) {
      patternList[i]->set_hasSavedSphere(false);
      patternList[i]->set_hasSavedCircle(false);
      i++;
   }

   i=0;
   while (i< patternList.size()) {
      patternList[i]->save3DParaView(projData,frequency,iteration);
      patternList[i]->save2DParaView(projData,frequency,iteration);
      i++;
   }
}

double PatternDatabase::get_gain (double frequency, int Sport, int iteration)
{
   std::string quantity="G";
   double gain=-DBL_MAX;
   long unsigned int i=0;
   while (i< patternList.size()) {
      if (patternList[i]->is_match(frequency,Sport,iteration,quantity)) {
         gain=patternList[i]->get_gain();
         break;
      }
      i++;
   }
   return gain;
}

double PatternDatabase::get_directivity (double frequency, int Sport, int iteration)
{
   std::string quantity="D";
   double directivity=-DBL_MAX;
   long unsigned int i=0;
   while (i< patternList.size()) {
      if (patternList[i]->is_match(frequency,Sport,iteration,quantity)) {
         directivity=patternList[i]->get_directivity();
         break;
      }
      i++;
   }
   return directivity;
}

double PatternDatabase::get_radiationEfficiency (double frequency, int Sport, int iteration)
{
   std::string quantityG="G";
   std::string quantityD="D";
   double radiationEfficiency=-DBL_MAX;
   long unsigned int i=0;
   while (i< patternList.size()) {
      if (patternList[i]->is_match(frequency,Sport,iteration,quantityG)) {
         radiationEfficiency=patternList[i]->get_radiationEfficiency();
         break;
      }
      if (patternList[i]->is_match(frequency,Sport,iteration,quantityD)) {
         radiationEfficiency=patternList[i]->get_radiationEfficiency();
         break;
      }
      i++;
   }
   return radiationEfficiency;
}

bool PatternDatabase::saveCSV (struct projectData *projData, std::vector<double> *unique_frequencies, int SportCount, bool allIterations)
{
   if (patternList.size() == 0) return false;

   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   long unsigned int i=0;
   while (i< patternList.size()) {
      patternList[i]->set_hasSavedSphere(false);
      patternList[i]->set_hasSavedCircle(false);
      i++;
   }

   int isFail=0;

   std::stringstream ss;
   ss << projData->project_name << "_FarField_results.csv";

   std::ofstream out;

   if (rank == 0) {
      out.open(ss.str().c_str(),std::ofstream::out);
      if (!out.is_open()) isFail=1;

      int k=1;
      while (k < size) {
         MPI_Send(&isFail,1,MPI_INT,k,300,PETSC_COMM_WORLD);
         k++;
      }
   } else {
      MPI_Recv(&isFail,1,MPI_INT,0,300,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   }

   if (isFail) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3197: Failed to open file \"%s\" for writing.\n",ss.str().c_str());
      return true;
   }

   std::string quantityG="G";
   std::string quantityD="D";

   if (rank == 0) {

      // header
      if (allIterations) out << "#prior iterations pre-pended by #" << std::endl;
      out << "#S-port,frequency";

      double scale=1;
      if (strcmp(projData->touchstone_frequency_unit,"Hz") == 0) {out << "(Hz),"; scale=1;}
      if (strcmp(projData->touchstone_frequency_unit,"kHz") == 0) {out << "(kHz),"; scale=1e-3;}
      if (strcmp(projData->touchstone_frequency_unit,"MHz") == 0) {out << "(MHz),"; scale=1e-6;}
      if (strcmp(projData->touchstone_frequency_unit,"GHz") == 0) {out << "(GHz),"; scale=1e-9;}

      out << "gain,directivity,radiation efficiency" << std::endl;

      int Sport=1;
      while (Sport <= SportCount) {
         long unsigned int j=0;
         while (j < unique_frequencies->size()) {
            long unsigned int k=0;
            while (k < patternList.size()) {
               bool found=false;
               if (patternList[k]->is_3D() && patternList[k]->is_match((*unique_frequencies)[j],Sport,quantityG)) found=true;
               if (patternList[k]->is_3D() && patternList[k]->is_match((*unique_frequencies)[j],Sport,quantityD)) found=true;

               if (found) {
                  bool printLine=false;
                  if (patternList[k]->is_active()) {
                     printLine=true;
                  } else {
                     if (allIterations) {
                        printLine=true;
                        out << "#";
                     }
                  }

                  if (patternList[k]->get_hasSavedSphere()) printLine=false;
                  if (printLine) {
                     out << std::setprecision(15) << Sport << ","
                                             << (*unique_frequencies)[j]*scale << "," 
                                             << patternList[k]->get_gain() << "," 
                                             << patternList[k]->get_directivity() << "," 
                                             << patternList[k]->get_radiationEfficiency() << std::endl;
                     patternList[k]->set_hasSavedSphere(true);
                  }
               }
               k++;
            }
            j++;
         }
         Sport++;
      }
      out.close();
   }

   return false;
}

bool PatternDatabase::loadCSV (std::string filename, struct projectData *projData)
{
   bool fail=false;

   std::ifstream CSV;
   CSV.open(filename,std::ifstream::in);
   if (CSV.is_open()) {
      std::string line;
      while (getline(CSV,line)) {
         if (line.compare("") != 0 && line.substr(0,1).compare("#") != 0) {

            int Sport=-1;
            double frequency=-DBL_MAX;
            double gain=-DBL_MAX;
            double directivity=-DBL_MAX;
            double radiationEfficiency=-DBL_MAX;

            std::stringstream ssLine(line);
            std::string value;

            int count=1;
            while (std::getline(ssLine,value,',')) {
               if (count == 1) Sport=stoi(value);
               if (count == 2) {
                  frequency=stod(value);
                  double scale=1;
                  if (strcmp(projData->touchstone_frequency_unit,"Hz") == 0) scale=1;
                  if (strcmp(projData->touchstone_frequency_unit,"kHz") == 0) scale=1e3;
                  if (strcmp(projData->touchstone_frequency_unit,"MHz") == 0) scale=1e6;
                  if (strcmp(projData->touchstone_frequency_unit,"GHz") == 0) scale=1e9;
                  frequency*=scale;
               }
               if (count == 3) gain=stod(value);
               if (count == 4) directivity=stod(value);
               if (count == 5) radiationEfficiency=stod(value);
               count++;
            }

            std::string quantity1="";
            std::string quantity2="";
            std::string plane="";
            Pattern *newPattern=new Pattern(frequency,Sport,0,std::complex<double>(-DBL_MAX,-DBL_MAX),3,
                 quantity1,quantity2,plane,-DBL_MAX,-DBL_MAX,-DBL_MAX,-DBL_MAX,nullptr,nullptr);
            newPattern->set_gain(gain);
            newPattern->set_directivity(directivity);
            newPattern->set_radiationEfficiency(radiationEfficiency);

            patternList.push_back(newPattern);
         }
      }
      CSV.close();
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3198: Unable to open file \"%s\" for reading.\n",filename.c_str());
      fail=true;
   }

   return fail;
}

void PatternDatabase::reset ()
{
   long unsigned int i=0;
   while (i < patternList.size()) {
      if (patternList[i]) {
         Sphere *sphere=patternList[i]->get_sphere();
         if (sphere) {
            long unsigned int j=i+1;
            while (j < patternList.size()) {
               if (patternList[j]->get_sphere() == sphere) patternList[j]->set_sphere(nullptr);
               j++;
            }
         }

         Circle *circle=patternList[i]->get_circle();
         if (circle) {
            long unsigned int j=i+1;
            while (j < patternList.size()) {
               if (patternList[j]->get_circle() == circle) patternList[j]->set_circle(nullptr);
               j++;
            }
         }
      
         delete patternList[i];
         patternList[i]=nullptr;
      }
      i++;
   }
   patternList.clear();
}

void PatternDatabase::print (PetscMPIInt rank)
{
   if (rank == 0) {
      std::cout << "PatternDatabase: " << this << std::endl;
   }

   long unsigned int i=0;
   while (i< patternList.size()) {
      patternList[i]->print(rank);
      i++;
   }
}

void PatternDatabase::save (struct projectData *projData)
{
   PetscMPIInt rank;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   if (!projData->antenna_plot_raw_save) return;

   if (rank == 0) {
      if (patternList.size() == 0) return;

      std::ofstream out;
      std::stringstream ss;
      ss << projData->project_name << "_FarField.csv";
      out.open(ss.str().c_str(),std::ofstream::out);
      if (!out.is_open()) return;

      long unsigned int i=0;
      while (i < patternList.size()) {
         patternList[i]->save(&out);
         i++;
      }

      out.close();
   }
}

void PatternDatabase::save_as_test (std::string testFilename, struct projectData *projData, int SportCount, int *casenumber)
{
   if (patternList.size() == 0) return;

   PetscMPIInt rank,size;
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
   MPI_Comm_size(PETSC_COMM_WORLD, &size);

   int isFail=0;
   std::ofstream out;

   if (rank == 0) {
      out.open(testFilename.c_str(),std::ofstream::app);
      if (!out.is_open()) isFail=1;

      int k=1;
      while (k < size) {
         MPI_Send(&isFail,1,MPI_INT,k,300,PETSC_COMM_WORLD);
         k++;
      }
   } else {
      MPI_Recv(&isFail,1,MPI_INT,0,300,PETSC_COMM_WORLD,MPI_STATUS_IGNORE);
   }

   if (isFail) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR3194: Failed to open file \"%s\" for writing.\n",testFilename.c_str());
      return;
   }

   if (rank == 0) out << "# PatternDatabase::save_as_test" << std::endl;

   long unsigned int i=0;
   while (i < unique_frequencies.size()) {
      int j=0;
      while (j < SportCount) {
         Pattern *pattern=this->get_Pattern(unique_frequencies[i],j+1);  // gets the active result
         if (pattern) pattern->save_as_test (&out,projData->project_name,i,casenumber);
         else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Failed to find a pattern at frequency %g for Sport %d.\n",unique_frequencies[i],j+1);}
         j++;
      }
      i++;
   }

   if (rank == 0) out.close();

   return;
}

PatternDatabase::~PatternDatabase ()
{
   long unsigned int i=0;
   while (i< patternList.size()) {
      if (patternList[i]) delete patternList[i];
      patternList[i]=nullptr;
      i++;
   }
   patternList.clear();
}

