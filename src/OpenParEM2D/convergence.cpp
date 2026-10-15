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

#include "convergence.hpp"

Convergence::Convergence(int number_of_passes_, double tolerance_, int type_)
{
   number_of_passes=number_of_passes_;
   type=type_;
   tolerance=tolerance_;
   converged=false;
   if (type == NONE) converged=true;
   last_error=-1;
}

void Convergence::push (double value)
{
   // save the data
   results.push_back(value);

   // calculate convergence

   if (results.size() <= number_of_passes) return;

   bool failed=false;
   long unsigned int passes=0;
   while (passes < number_of_passes) {
      int index=results.size()-number_of_passes+passes;
      last_error=fabs((results[index]-results[index-1])/results[index]);
      if (last_error > tolerance) failed=true;
      passes++;
   }

   if (failed) converged=false;
   else converged=true;
}

bool Convergence::has_progress ()
{
   bool has_progress=false;

   long unsigned int passes=0;
   while (passes < number_of_passes) {
      int index=results.size()-number_of_passes+passes;

      if (index > 0) {
         has_progress=true;
         break;
      }

      passes++;
   }
   return has_progress;
}

void Convergence::show_progress ()
{
   long unsigned int passes=0;
   while (passes < number_of_passes) {
      int index=results.size()-number_of_passes+passes;

      if (index > 0) {
         last_error=fabs((results[index]-results[index-1])/results[index]);

         if (last_error <= tolerance) {
            PetscPrintf(PETSC_COMM_WORLD,"            %g < target: %g\n",last_error,tolerance);
         } else {
            PetscPrintf(PETSC_COMM_WORLD,"            %g > target: %g\n",last_error,tolerance);
         }
      }

      passes++;
   }
}

void ConvergenceDatabase::initialize (int size, int number_of_passes, char *refinement_variable, char *refinement_tolerance)
{
  int type;
  double tolerance;

  int i=1; 
  while (i <= size) {
     type=get_refinement_variable(refinement_variable,i);
     tolerance=get_refinement_tolerance(refinement_tolerance,i);
     Convergence *conv=new Convergence(number_of_passes,tolerance,type);
     convergenceList.push_back(conv);
     i++;
  }
}

bool ConvergenceDatabase::is_converged()
{
   long unsigned int i=0;
   while (i < convergenceList.size()) {
      if (! convergenceList[i]->is_converged()) return false;
      i++;
   }
   return true;
}

int ConvergenceDatabase::is_converged_count()
{
   int count=0;
   long unsigned int i=0;
   while (i < convergenceList.size()) {
      if (convergenceList[i]->is_converged()) count++;
      i++;
   }
   return count;
}

bool ConvergenceDatabase::is_converged(int mode)
{
   if (convergenceList[mode]->is_converged()) return true;
   return false;
}

void ConvergenceDatabase::set_not_converged()
{
   long unsigned int i=0;
   while (i < convergenceList.size()) {
      convergenceList[i]->set_not_converged();
      i++;
   }
}

double ConvergenceDatabase::get_last_error()
{
   double error=0;
   long unsigned int i=0;
   while (i < convergenceList.size()) {
      if (fabs(convergenceList[i]->get_last_error()) > fabs(error)) error=convergenceList[i]->get_last_error();
      i++;
   }
   return error;
}

void ConvergenceDatabase::show_progress()
{
   // see if there is any progress to show
   bool has_progress=false;
   long unsigned int i=0;
   while (i < convergenceList.size()) {
      if (convergenceList[i]->has_progress()) {has_progress=true; break;}
      i++;
   }
   if (! has_progress) return;

   // show progress

   PetscPrintf(PETSC_COMM_WORLD,"      Convergence progress:\n");

   i=0;
   while (i < convergenceList.size()) {
      PetscPrintf(PETSC_COMM_WORLD,"         mode %ld",i+1);
      if (convergenceList[i]->get_type() == NONE) PetscPrintf(PETSC_COMM_WORLD," none\n");
      else if (convergenceList[i]->get_type() == ALPHA) PetscPrintf(PETSC_COMM_WORLD," alpha:\n");
      else if (convergenceList[i]->get_type() == BETA) PetscPrintf(PETSC_COMM_WORLD," beta:\n");
      else if (convergenceList[i]->get_type() == ABSGAMMA) PetscPrintf(PETSC_COMM_WORLD," |gamma|:\n");
      else if (convergenceList[i]->get_type() == ABSZO) PetscPrintf(PETSC_COMM_WORLD," |Zo|:\n");
      else if (convergenceList[i]->get_type() == REZO) PetscPrintf(PETSC_COMM_WORLD," Re(Zo):\n");
      else if (convergenceList[i]->get_type() == IMZO) PetscPrintf(PETSC_COMM_WORLD," Im(Zo):\n");

      convergenceList[i]->show_progress();
      i++;
   }
}

void ConvergenceDatabase::push(int mode, complex<double> Zo, double alpha, double beta, double alpha_perturbation, double Pz)
{
   if (convergenceList[mode]->get_type() == ALPHA) convergenceList[mode]->push(alpha+alpha_perturbation/Pz/2);
   else if (convergenceList[mode]->get_type() == BETA) convergenceList[mode]->push(beta);
   else if (convergenceList[mode]->get_type() == ABSGAMMA) convergenceList[mode]->push(sqrt(alpha*alpha+beta*beta));
   else if (convergenceList[mode]->get_type() == ABSZO) convergenceList[mode]->push(abs(Zo));
   else if (convergenceList[mode]->get_type() == REZO) convergenceList[mode]->push(real(Zo));
   else if (convergenceList[mode]->get_type() == IMZO) convergenceList[mode]->push(imag(Zo));
}

ConvergenceDatabase::~ConvergenceDatabase()
{
   long unsigned int i=0;
   while (i < convergenceList.size()) {
      delete convergenceList[i];
      i++;
   }
}






