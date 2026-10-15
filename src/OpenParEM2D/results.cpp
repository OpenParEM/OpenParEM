////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    openParEM2D - A fullwave 2D electromagnetic simulator.                  //
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

#include "results.hpp"

//---------------------------------------------------------------------------------------------------------------------------------
// Result
//---------------------------------------------------------------------------------------------------------------------------------

// do not support voltage (variable V) or current (variable I) since no testing is done on them
void Result::save_result_component (ofstream *out, const char *casename, int frequency_index, int *casenumber, const char *result_component)
{
   double NpTodB=20*log10(exp(1));
   double eps0=8.8541878176e-12;
   double ko=2*M_PI*frequency*sqrt(4.0e-7*M_PI*eps0);

   // skip if impedance and there are no impedance data
   if ((strcmp(result_component,"real_Z") == 0 || strcmp(result_component,"imag_Z") == 0) &&  Z.size() == 0) return;

   // continue
   long unsigned int mode=0;
   while (mode < (long unsigned int)modeCount) {

      double component=DBL_MAX;
      if (strcmp(result_component,"alpha") == 0) component=real(gamma[mode])*NpTodB+alpha_perturbation[mode]/real(Pz[mode])/2*NpTodB;
      if (strcmp(result_component,"beta") == 0) component=imag(gamma[mode])/ko;
      if (strcmp(result_component,"real_Z") == 0) component=real(get_Z(mode));
      if (strcmp(result_component,"imag_Z") == 0) component=imag(get_Z(mode));

      if (component != DBL_MAX) {
         *out << casename <<  "_" << frequency_index+1 << "_" << mode+1 << "_" << (*casenumber)++ << "_result" << ",";
         *out << frequency << ",";
         *out << mode+1 << ",";
         *out << result_component << ",";

         if (fabs(component) > resultMagLimit) {
            *out << "equal" << ",";
            *out << setprecision(15) << component << ",";
            *out << equalErrorLimit << endl;
         } else {
            *out << "lessthan" << ",";
            *out << lessthanErrorLimit << endl;
         }
      }
      mode++;
   }
}

void Result::save_as_test (ofstream *out, const char *casename, int frequency_index, int *casenumber)
{
   save_result_component(out,casename,frequency_index,casenumber,"alpha");
   save_result_component(out,casename,frequency_index,casenumber,"beta");
   save_result_component(out,casename,frequency_index,casenumber,"real_Z");
   save_result_component(out,casename,frequency_index,casenumber,"imag_Z");
}

void Result::print()
{
   double NpTodB=20*log10(exp(1));
   double eps0=8.8541878176e-12;
   double ko=2*M_PI*frequency*sqrt(4.0e-7*M_PI*eps0);

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"Result:\n");
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   active=%d\n",active);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   iteration=%d\n",iteration);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   frequency=%g\n",frequency);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   modeCount=%ld\n",modeCount);
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   modalImpedanceCalculation=%d\n",modalImpedanceCalculation);

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   alpha=");
   long unsigned int i=0;
   while (i < modeCount) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%g,",real(gamma[i])*NpTodB+alpha_perturbation[i]/real(Pz[i])/2*NpTodB);
      i++;
   }
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"\n");

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   beta/ko=");
   i=0;
   while (i < modeCount) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%g,",imag(gamma[i])/ko);
      i++;
   }
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"\n");

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   impedance=");
   i=0;
   while (i < modeCount) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"(%g;%g),",real(Z[i]),imag(Z[i]));
      i++;
   }
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"\n");

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   voltage=");
   i=0;
   while (i < modeCount) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"(%g;%g),",real(V[i]),imag(V[i]));
      i++;
   }
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"\n");

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   current=");
   i=0;
   while (i < modeCount) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"(%g;%g),",real(I[i]),imag(I[i]));
      i++;
   }
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"\n");

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   Pz=");
   i=0;
   while (i < modeCount) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"(%g;%g),",real(Pz[i]),imag(Pz[i]));
      i++;
   }
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"\n");

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"\n");

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   mesh_size=%d\n",run_stats->get_meshSize());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   matrix_size=%d\n",run_stats->get_matrixSize());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   converged=%d\n",run_stats->get_converged());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   error=%g\n",run_stats->get_error());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   solve_elapsed_time=%g\n",run_stats->get_solve_elapsed().count());
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"   refine_elapsed_time=%g\n",run_stats->get_refine_elapsed().count());
}

Result::~Result()
{
   delete run_stats;
}

//---------------------------------------------------------------------------------------------------------------------------------
// ResultDatabase
//---------------------------------------------------------------------------------------------------------------------------------

void ResultDatabase::push(Result *Result_) {

   // inactivate any prior data at this frequency
   long unsigned int i=0;
   while (i < results.size()) {
      if (results[i]->is_active() && fabs(results[i]->get_frequency()-Result_->get_frequency())/Result_->get_frequency() <= tol) {
         results[i]->set_inactive();
      }
      i++;
   }

   // save the result
   results.push_back(Result_);

   // keep a sorted list of unique frequencies

   // unique?
   bool found=false;
   i=0;
   while (i < unique_frequencies.size()) {
      if (fabs(unique_frequencies[i]-Result_->get_frequency())/unique_frequencies[i] <= tol) {
         found=true;
         break;
      }
      i++;
   }

   if (! found) {

      // save
      unique_frequencies.push_back(Result_->get_frequency());

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

long unsigned int ResultDatabase::get_mode_count(double frequency) {
   long unsigned int mode_max=0;

   long unsigned int i=0;
   while (i < results.size()) {
      if (fabs(results[i]->get_frequency()-frequency)/results[i]->get_frequency() < tol) {
         if (results[i]->get_modeCount() > mode_max) mode_max=results[i]->get_modeCount();
      }
      i++;
   }
   return mode_max;
}

// get the active result
Result* ResultDatabase::get_Result(double frequency)
{
   long unsigned int i=0;
   while (i < results.size()) {
      if (results[i]->is_active() && fabs(results[i]->get_frequency()-frequency)/results[i]->get_frequency() <= tol) {
         return results[i];
      }
      i++;
   }
   return nullptr;
}

int ResultDatabase::get_iterationTotal(double frequency)
{
   int sum=0;
   long unsigned int i=0;
   while (i < results.size()) {
      if (fabs(results[i]->get_frequency()-frequency)/results[i]->get_frequency() <= tol) sum++;
      i++;
   }
   return sum;
}

// get the active result for a given iteration
Result* ResultDatabase::get_Result(double frequency, int iteration)
{
   long unsigned int i=0;
   while (i < results.size()) {
      if (results[i]->is_active() && fabs(results[i]->get_frequency()-frequency)/results[i]->get_frequency() <= tol) {
         if (results[i]->get_iteration() == iteration) {
            return results[i];
         }
      }
      i++;
   }
   return nullptr;
}

// update the frequency result with convergence data
void ResultDatabase::update_convergence(double frequency, bool converged, double error)
{
   long unsigned int i=0;
   while (i < results.size()) {
      if (results[i]->is_active() && fabs(results[i]->get_frequency()-frequency)/results[i]->get_frequency() <= tol) {
         run_statistics *run_stats=results[i]->get_run_stats();
         run_stats->set_converged(converged);
         run_stats->set_error(error);
      }
      i++;
   }
}

void ResultDatabase::save(struct projectData *projData, const char *projFile)
{
   double NpTodB=20*log10(exp(1));
   double ko;
   double eps0=8.8541878176e-12;

   stringstream ss;
   ss << projFile << "_results.csv";

   ofstream out;
   out.open(ss.str().c_str(),ofstream::out);

   if (out.is_open()) {

      // sweep through the frequencies
      bool printedHeader=false;
      long unsigned int i=0;
      while (i < unique_frequencies.size()) {

         ko=2*M_PI*unique_frequencies[i]*sqrt(4.0e-7*M_PI*eps0);

         chrono::duration<double> elapsed_time=this->frequency_elapsed(unique_frequencies[i]);

         // get the active result
         Result *result=this->get_Result(unique_frequencies[i]);

         // header line
         if (!printedHeader) {
            printedHeader=true;
            out << "#OpenParEM2D " << projData->version_major << "." << projData->version_minor << "." << projData->version_patch << endl;
            out << "#frequency,iteration,mesh size,matrix size,converged,final error,elapsed time (s),modal impedance calculation,mode count";

            long unsigned int j=0;
            while (j < result->get_modeCount()) {
               out << ",alpha[" << j+1 << "] db/m";
               out << ",beta[" << j+1 << "]/ko";
               out << ",real(Z[" << j+1 << "])";
               out << ",imag(Z[" << j+1 << "])";
               out << ",real(V[" << j+1 << "])";
               out << ",imag(V[" << j+1 << "])";
               out << ",real(I[" << j+1 << "])";
               out << ",imag(I[" << j+1 << "])";
               out << ",real(Pz[" << j+1 << "])";
               out << ",imag(Pz[" << j+1 << "])";
               j++;
            }
            out << endl;
         }

         // data
         out << setprecision(15) << unique_frequencies[i]
             << "," << get_iterationTotal(unique_frequencies[i])
             << "," << result->get_run_stats()->get_meshSize()
             << "," << result->get_run_stats()->get_matrixSize()
             << "," << result->get_run_stats()->get_converged()
             << "," << result->get_run_stats()->get_error()
             << "," << elapsed_time.count()
             << "," << result->get_modalImpedanceCalculation()
             << "," << this->get_mode_count(unique_frequencies[i]);

         long unsigned int j=0;
         while (j < result->get_modeCount()) {
            out << setprecision(15) << "," << real(result->get_gamma(j))*NpTodB+result->get_alpha_perturbation(j)/real(result->get_Pz(j))/2*NpTodB
                                    << "," << imag(result->get_gamma(j))/ko
                                    << "," << processOutputNumber(real(result->get_Z(j)))
                                    << "," << processOutputNumber(imag(result->get_Z(j)))
                                    << "," << processOutputNumber(real(result->get_V(j)))
                                    << "," << processOutputNumber(imag(result->get_V(j)))
                                    << "," << processOutputNumber(real(result->get_I(j)))
                                    << "," << processOutputNumber(imag(result->get_I(j)))
                                    << "," << processOutputNumber(real(result->get_Pz(j)))
                                    << "," << processOutputNumber(imag(result->get_Pz(j)));
            j++;
         }
         out << endl;

         i++;
      }

      out.close();
   } else {
       prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2272: Failed to open file \"%s\" for writing.\n",ss.str().c_str());
   }
}

void ResultDatabase::save_as_test(const char *baseName, const char *casename)
{
   stringstream ss;
   ss << baseName << "_prototype_test_cases.csv";

   ofstream out;
   out.open(ss.str().c_str(),ofstream::out);
   int casenumber=0;

   if (out.is_open()) {
      out << "# ResultDatabase::save_as_test" << endl;

      // sweep through the frequencies
      long unsigned int i=0;
      while (i < unique_frequencies.size()) {
         Result *result=this->get_Result(unique_frequencies[i]);  // gets the active result
         if (result) result->save_as_test (&out, casename, i, &casenumber);
         else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"ASSERT: Failed to find a result at frequency %g.\n",unique_frequencies[i]);}
         i++;
      }

      out.close();
   } else {
       prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR2273: Failed to open file \"%s\" for writing.\n",ss.str().c_str());
   }
}

void ResultDatabase::print()
{
   long unsigned int i=0;
   while (i < results.size()) {
      results[i]->print();
      i++;
   }
}

// get the overall run time for a given frequency including all iterations and modes
chrono::duration<double> ResultDatabase::frequency_elapsed(double frequency)
{
   bool started=false;
   chrono::duration<double> total_elapsed=std::chrono::seconds(0);
   long unsigned int i=0;
   while (i < results.size()) {

      if (fabs(results[i]->get_frequency()-frequency)/results[i]->get_frequency() <= tol) {
         run_statistics *run_stats=results[i]->get_run_stats();
         if (started) total_elapsed+=run_stats->get_end_refine_time()-run_stats->get_start_time();
         else {
            total_elapsed=run_stats->get_end_refine_time()-run_stats->get_start_time();
            started=true;
         }
      }
      i++;
   }

   return total_elapsed;
}

ResultDatabase::~ResultDatabase ()
{
   long unsigned int i=0;
   while (i < results.size()) {
      delete results[i];
      i++;
   }
}

