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

#include "OpenParEMmaterials.hpp"
#include <cstring>


///////////////////////////////////////////////////////////////////////////////////////////
// Frequency
///////////////////////////////////////////////////////////////////////////////////////////

Frequency::Frequency (int startLine_, int endLine_, bool checkLimits_)
{
   startLine=startLine_;
   endLine=endLine_;

   // frequency

   frequency.push_alias("frequency");
   frequency.push_alias("freq");
   frequency.push_alias("f");
   frequency.set_loaded(false);
   frequency.set_positive_required(true);
   frequency.set_non_negative_required(false);
   frequency.set_lowerLimit(0);
   frequency.set_upperLimit(1e12);
   frequency.set_checkLimits(checkLimits_);

   // relative permittivity

   relative_permittivity.push_alias("relative_permittivity");
   relative_permittivity.push_alias("er");
   relative_permittivity.push_alias("epsr");
   relative_permittivity.set_loaded(false);
   relative_permittivity.set_positive_required(true);
   relative_permittivity.set_non_negative_required(false);
   relative_permittivity.set_lowerLimit(1);
   relative_permittivity.set_upperLimit(1e6);
   relative_permittivity.set_checkLimits(checkLimits_);

   // relative permeability

   relative_permeability.push_alias("relative_permeability");
   relative_permeability.push_alias("mur");
   relative_permeability.set_loaded(false);
   relative_permeability.set_positive_required(true);
   relative_permeability.set_non_negative_required(false);
   relative_permeability.set_lowerLimit(1);
   relative_permeability.set_upperLimit(1e6);
   relative_permeability.set_checkLimits(checkLimits_);

   // loss

   loss.push_alias("loss_tangent");
   loss.push_alias("tand");
   loss.push_alias("tandel");
   loss.push_alias("conductivity");
   loss.push_alias("sigma");
   loss.set_loaded(false);
   loss.set_positive_required(false);
   loss.set_non_negative_required(true);
   loss.set_lowerLimit(0);
   loss.set_upperLimit(1e8);  // much too high for loss tangent, unavoidable
   loss.set_checkLimits(checkLimits_);

   // Rz

   Rz.push_alias("Rz");
   Rz.set_loaded(false);
   Rz.set_positive_required(false);
   Rz.set_non_negative_required(true);
   Rz.set_lowerLimit(0);
   Rz.set_upperLimit(0.0001);
   Rz.set_checkLimits(checkLimits_);
}

void Frequency::print (std::string indent)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%sFrequency\n",startLine,indent.c_str(),indent.c_str());

   if (frequency.is_loaded()) {
      if (frequency.is_any()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s%s=any\n",frequency.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),frequency.get_keyword().c_str());
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s%s=%g\n",
                                                frequency.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),frequency.get_keyword().c_str(),frequency.get_dbl_value());
      }
   }

   if (relative_permittivity.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s%s=%g\n",
                                             relative_permittivity.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),relative_permittivity.get_keyword().c_str(),relative_permittivity.get_dbl_value());
   }

   if (relative_permeability.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s%s=%g\n",
                                             relative_permeability.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),relative_permeability.get_keyword().c_str(),relative_permeability.get_dbl_value());
   }

   if (loss.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s%s=%g\n",
                                             loss.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),loss.get_keyword().c_str(),loss.get_dbl_value());
   }

   if (Rz.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s%s=%g\n",
                                             Rz.get_lineNumber(),indent.c_str(),indent.c_str(),indent.c_str(),Rz.get_keyword().c_str(),Rz.get_dbl_value());
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%sEndFrequency\n",endLine,indent.c_str(),indent.c_str());
}

bool Frequency::load (std::string *indent, inputFile *inputs)
{
   bool fail=false;

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {
      std::string token,value,line;
      line=inputs->get_line(lineNumber);
      get_token_pair(&line,&token,&value,&lineNumber,indent->c_str());

      int recognized=0;
      if (relative_permittivity.match_alias(&token)) {
         recognized++;
         if (relative_permittivity.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (relative_permeability.match_alias(&token)) {
         recognized++;
         if (relative_permeability.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (loss.match_alias(&token)) {
         recognized++;
         if (loss.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (Rz.match_alias(&token)) {
         recognized++;
         if (Rz.loadDouble(&token, &value, lineNumber)) fail=true;
      }

      if (frequency.match_alias(&token)) {
         if (frequency.is_loaded()) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1050: Duplicate entry at line %d for previous entry at line %d.\n",
                                                   indent->c_str(),lineNumber,frequency.get_lineNumber());
            fail=true;
         } else {
            if (value.compare("any") == 0) {
               frequency.set_keyword(token);
               frequency.set_value(value);
               frequency.set_lineNumber(lineNumber);
               frequency.set_loaded(true);
            } else {
               if (frequency.loadDouble(&token, &value, lineNumber)) fail=true;
            }
         }
         recognized++;
      }

      // should recognize one keyword
      if (recognized != 1) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1052: Unrecognized keyword at line %d.\n",indent->c_str(),lineNumber);
         fail=true;
      }

      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }
   return fail;
}

bool Frequency::inFrequencyBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

bool Frequency::check (std::string indent)
{
   bool fail=false;

   if (!frequency.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1053: Frequency block at line %d must specify a frequency.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!relative_permittivity.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1054: Frequency block at line %d must specify a relative permitivitty.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!relative_permeability.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1055: Frequency block at line %d must specify a relative permeability.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!loss.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1056: Frequency block at line %d must specify a loss tangent or a conductivity.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (!Rz.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1057: Frequency block at line %d must specify Rz.\n",indent.c_str(),startLine);
      fail=true;
   }

   return fail;
}

void Frequency::set_freespace ()
{
    frequency.set_keyword("frequency");
    frequency.set_value("any");
    frequency.set_loaded(true);
    frequency.set_lineNumber(0);

    relative_permittivity.set_keyword("relative_permittivity");
    relative_permittivity.set_dbl_value(1);
    relative_permittivity.set_loaded(true);
    relative_permittivity.set_lineNumber(0);

    relative_permeability.set_keyword("relative_permeability");
    relative_permeability.set_dbl_value(1);
    relative_permeability.set_loaded(true);
    relative_permeability.set_lineNumber(0);

    loss.set_keyword("tand");
    loss.set_dbl_value(0);
    loss.set_loaded(true);
    loss.set_lineNumber(0);

    Rz.set_keyword("Rz");
    Rz.set_dbl_value(0);
    Rz.set_loaded(true);
    Rz.set_lineNumber(0);
}

void Frequency::set_copper ()
{
    frequency.set_keyword("frequency");
    frequency.set_value("any");
    frequency.set_loaded(true);
    frequency.set_lineNumber(0);

    relative_permittivity.set_keyword("relative_permittivity");
    relative_permittivity.set_dbl_value(1);
    relative_permittivity.set_loaded(true);
    relative_permittivity.set_lineNumber(0);

    relative_permeability.set_keyword("relative_permeability");
    relative_permeability.set_dbl_value(1);
    relative_permeability.set_loaded(true);
    relative_permeability.set_lineNumber(0);

    loss.set_keyword("conductivity");
    loss.set_dbl_value(5.813e7);
    loss.set_loaded(true);
    loss.set_lineNumber(0);

    Rz.set_keyword("Rz");
    Rz.set_dbl_value(0);
    Rz.set_loaded(true);
    Rz.set_lineNumber(0);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Temperature
///////////////////////////////////////////////////////////////////////////////////////////

Temperature::Temperature (int startLine_, int endLine_, bool checkLimits_)
{
   startLine=startLine_;
   endLine=endLine_;

   // temperature

   temperature.push_alias("temperature");
   temperature.push_alias("temp");
   temperature.push_alias("t");
   temperature.set_loaded(false);
   temperature.set_positive_required(false);
   temperature.set_non_negative_required(false);
   temperature.set_lowerLimit(-273.15);
   temperature.set_upperLimit(1e5);
   temperature.set_checkLimits(checkLimits_);

   // er_infinity

   er_infinity.push_alias("er_infinity");
   er_infinity.push_alias("epsr_infinity");
   er_infinity.set_loaded(false);
   er_infinity.set_positive_required(true);
   er_infinity.set_non_negative_required(false);
   er_infinity.set_lowerLimit(1);
   er_infinity.set_upperLimit(1e6);
   er_infinity.set_checkLimits(checkLimits_);

   // delta_er

   delta_er.push_alias("delta_er");
   delta_er.push_alias("delta_epsr");
   delta_er.set_loaded(false);
   delta_er.set_positive_required(true);
   delta_er.set_non_negative_required(false);
   delta_er.set_lowerLimit(0);
   delta_er.set_upperLimit(1e6);
   delta_er.set_checkLimits(checkLimits_);

   // m1

   m1.push_alias("m1");
   m1.set_loaded(false);
   m1.set_positive_required(false);
   m1.set_non_negative_required(true);
   m1.set_lowerLimit(0);
   m1.set_upperLimit(100);
   m1.set_checkLimits(checkLimits_);

   // m2

   m2.push_alias("m2");
   m2.set_loaded(false);
   m2.set_positive_required(false);
   m2.set_non_negative_required(true);
   m2.set_lowerLimit(0);
   m2.set_upperLimit(100);
   m2.set_checkLimits(checkLimits_);

   // relative permeability

   relative_permeability.push_alias("relative_permeability");
   relative_permeability.push_alias("mur");
   relative_permeability.set_loaded(false);
   relative_permeability.set_positive_required(true);
   relative_permeability.set_non_negative_required(false);
   relative_permeability.set_lowerLimit(1);
   relative_permeability.set_upperLimit(1e6);
   relative_permeability.set_checkLimits(checkLimits_);

   // loss

   loss.push_alias("loss_tangent");
   loss.push_alias("tand");
   loss.push_alias("tandel");
   loss.push_alias("conductivity");
   loss.push_alias("sigma");
   loss.set_loaded(false);
   loss.set_positive_required(false);
   loss.set_non_negative_required(true);
   loss.set_lowerLimit(0);
   loss.set_upperLimit(1e8);  // much too high for loss tangent, unavoidable
   loss.set_checkLimits(checkLimits_);
}

void Temperature::print (std::string indent)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %sTemperature\n",startLine,indent.c_str());

   if (temperature.is_loaded()) {
      if (temperature.is_any()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s=any\n",
                                                temperature.get_lineNumber(),indent.c_str(),indent.c_str(),temperature.get_keyword().c_str());
      } else {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s=%g\n",
                                                temperature.get_lineNumber(),indent.c_str(),indent.c_str(),temperature.get_keyword().c_str(),temperature.get_dbl_value());
      }
   }

   if (er_infinity.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s=%g\n",
                                             er_infinity.get_lineNumber(),indent.c_str(),indent.c_str(),er_infinity.get_keyword().c_str(),er_infinity.get_dbl_value());
   }

   if (delta_er.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s=%g\n",
                                             delta_er.get_lineNumber(),indent.c_str(),indent.c_str(),delta_er.get_keyword().c_str(),delta_er.get_dbl_value());
   }

   if (m1.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s=%g\n",
                                             m1.get_lineNumber(),indent.c_str(),indent.c_str(),m1.get_keyword().c_str(),m1.get_dbl_value());
   }

   if (m2.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s=%g\n",
                                             m2.get_lineNumber(),indent.c_str(),indent.c_str(),m2.get_keyword().c_str(),m2.get_dbl_value());
   }

   if (relative_permeability.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s=%g\n",
                                             relative_permeability.get_lineNumber(),indent.c_str(),indent.c_str(),relative_permeability.get_keyword().c_str(),relative_permeability.get_dbl_value());
   }

   if (loss.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s=%d\n",
                                             loss.get_lineNumber(),indent.c_str(),indent.c_str(),loss.get_keyword().c_str(),loss.get_lineNumber());
   }

   long unsigned int i=0;
   while (i < frequencyList.size()) {
      frequencyList[i]->print(indent);
      i++;
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %sEndTemperature\n",endLine,indent.c_str());
}

bool Temperature::findFrequencyBlocks(inputFile *inputs, bool checkLimits)
{
   bool fail=false;
   int start_lineNumber=startLine;
   int stop_lineNumber=endLine;
   int block_start,block_stop;

   while (start_lineNumber < stop_lineNumber) {
      if (inputs->findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                 "Frequency","EndFrequency",false)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            Frequency *newFrequency=new Frequency(block_start,block_stop,checkLimits);
            frequencyList.push_back(newFrequency);
         }
      }
      start_lineNumber=inputs->get_next_lineNumber(block_stop);
   }
   return fail;
}

bool Temperature::inFrequencyBlocks(int lineNumber)
{
   long unsigned int i=0;
   while (i < frequencyList.size()) {
      if (frequencyList[i]->inFrequencyBlock(lineNumber)) return true;
      i++;
   }
   return false;
}

bool Temperature::load(std::string *indent, inputFile *inputs, bool checkInputs)
{
   bool fail=false;

   // Frequency blocks - must find before keywords

   if (findFrequencyBlocks(inputs, checkInputs)) fail=true;

   long unsigned int i=0;
   while (i < frequencyList.size()) {
      if (frequencyList[i]->load(indent,inputs)) fail=true;
      i++;
   }

   // now the keyword pairs

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {

      if (!inFrequencyBlocks(lineNumber)) {
         std::string token,value,line;
         line=inputs->get_line(lineNumber);
         get_token_pair(&line,&token,&value,&lineNumber,*indent);

         int recognized=0;
         if (frequencyList.size() == 0) {

            if (er_infinity.match_alias(&token)) {
               recognized++;
               if (er_infinity.loadDouble(&token, &value, lineNumber)) fail=true;
            }

            if (delta_er.match_alias(&token)) {
               recognized++;
               if (delta_er.loadDouble(&token, &value, lineNumber)) fail=true;
            }

            if (m1.match_alias(&token)) {
               recognized++;
               if (m1.loadDouble(&token, &value, lineNumber)) fail=true;
            }

            if (m2.match_alias(&token)) {
               recognized++;
               if (m2.loadDouble(&token, &value, lineNumber)) fail=true;
            }

            if (relative_permeability.match_alias(&token)) {
               recognized++;
               if (relative_permeability.loadDouble(&token, &value, lineNumber)) fail=true;
            }

            if (loss.match_alias(&token)) {
               recognized++;
               if (loss.loadDouble(&token, &value, lineNumber)) fail=true;
            }

         } else {
            if (er_infinity.match_alias(&token) || delta_er.match_alias(&token) ||
                m1.match_alias(&token) || m2.match_alias(&token) ||
                relative_permeability.match_alias(&token) || loss.match_alias(&token)) {
                prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1058: Debye variable at line %d is not allowed with frequency blocks defined.\n",
                                                       indent->c_str(),lineNumber);
                fail=true;
            }
         }

         if (temperature.match_alias(&token)) {
            if (temperature.is_loaded()) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1059: Duplicate entry at line %d for previous entry at line %d.\n",
                                                      indent->c_str(),lineNumber,temperature.get_lineNumber());
               fail=true;
            } else {
               if (value.compare("any") == 0) {
                  temperature.set_keyword(token);
                  temperature.set_value(value);
                  temperature.set_lineNumber(lineNumber);
                  temperature.set_loaded(true);
               } else {
                  if (temperature.loadDouble(&token, &value, lineNumber)) fail=true;
               }
            }
            recognized++;
         }

         // should recognize one keyword
         if (recognized != 1) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1060: Unrecognized keyword at line %d.\n",indent->c_str(),lineNumber);
            fail=true;
         }
      }

      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }

   return fail;
}

bool Temperature::inTemperatureBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

bool Temperature::check(std::string indent)
{
   bool fail=false;

   if (!temperature.is_loaded()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1061: Temperature block at line %d must specify a temperature.\n",
                                             indent.c_str(),startLine);
      fail=true;
   }

   if (frequencyList.size() == 0) {
      if (!er_infinity.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1062: Temperature block at line %d must specify an er_infinity.\n",
                                                indent.c_str(),startLine);
         fail=true;
      }

      if (!delta_er.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1063: Temperature block at line %d must specify a delta_er.\n",
                                                indent.c_str(),startLine);
         fail=true;
      }

      if (!m1.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1064: Temperature block at line %d must specify an m1.\n",
                                                indent.c_str(),startLine);
         fail=true;
      }

      if (!m2.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1065: Temperature block at line %d must specify an m2.\n",
                                                indent.c_str(),startLine);
         fail=true;
      }

      if (!relative_permeability.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1066: Temperature block at line %d must specify a relative permeability.\n",
                                                indent.c_str(),startLine);
         fail=true;
      }

      if (!loss.is_loaded()) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1067: Temperature block at line %d must specify a loss tangent or a conductivity.\n",
                                                indent.c_str(),startLine);
         fail=true;
      }

   }

   // frequency block checks
   long unsigned int i=0;
   while (i < frequencyList.size()) {

      // single block checks
      if (frequencyList[i]->check(indent)) fail=true;

      // cross-block checks
      Frequency *test=get_frequency(i);
      long unsigned int j=i+1; 
      while (i < frequencyList.size()-1 && j < frequencyList.size()) {
         Frequency *check=get_frequency(j);

         if (test->get_frequency()->is_any()) {
            if (check->get_frequency()->is_any()) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1068: Temperature block at line %d incorrectly specifies another frequency block with frequency=any at line %d.\n",
                                                      indent.c_str(),startLine,check->get_startLine());
               fail=true;
            } else {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1069: Temperature block at line %d incorrectly specifies a frequency block at line %d after specifying \"any\" at line %d.\n",
                                                      indent.c_str(),startLine,check->get_startLine(),test->get_frequency()->get_lineNumber());
               fail=true;
            }
         } else {
            if (check->get_frequency()->is_any()) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1070: Temperature block at line %d incorrectly specifies a frequency block at line %d after specifying \"any\" at line %d.\n",
                                                      indent.c_str(),startLine,test->get_startLine(),check->get_frequency()->get_lineNumber());
               fail=true;
            } else {
               if (test->get_frequency()->dbl_compare(check->get_frequency())) {
                  prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1071: Temperature block at line %d has frequency blocks with duplicated frequencies at lines %d and %d.\n",
                                                         indent.c_str(),startLine,test->get_frequency()->get_lineNumber(),check->get_frequency()->get_lineNumber());
                  fail=true;
               }
            }
         }
         j++;
      }
      i++;
   }

   return fail;
}

std::complex<double> Temperature::get_eps(double frequency_, double tolerance, std::string indent)
{
   double eps;
   double loss_;
   std::complex<double> complex_eps=std::complex<double>(-DBL_MAX,0);
   double eps0=8.8541878176e-12;

   double freq_low=0;         // for linear interpolation - extrapolation is not supported
   double eps_low;            // ToDo - replace with spline interpolation or
   double loss_low;           //        with a Hilbert Transform algorithm for improved causality
   double freq_high=DBL_MAX;  //
   double eps_high;           // 
   double loss_high;          //
   double freq_test;          //
   bool found_low=false;      //
   bool found_high=false;     //

   if (frequencyList.size() > 0) {

      // frequency list
      bool found=false;
      long unsigned int i=0;
      while (i < frequencyList.size()) {

         // any
         if (frequencyList[i]->get_frequency()->get_value().compare("any") == 0) {
            eps=frequencyList[i]->get_relative_permittivity()->get_dbl_value();
            loss_=frequencyList[i]->get_loss()->get_dbl_value();
            found=true;
            break;
         }

         // exact match
         if (double_compare (frequency_, frequencyList[i]->get_frequency()->get_dbl_value(), tolerance)) {
            eps=frequencyList[i]->get_relative_permittivity()->get_dbl_value();
            loss_=frequencyList[i]->get_loss()->get_dbl_value();
            found=true;
            break;
         }

         // linear interoplation, if needed
         freq_test=frequencyList[i]->get_frequency()->get_dbl_value();

         if (freq_test < frequency_ && freq_test > freq_low) {
            freq_low=freq_test;
            eps_low=frequencyList[i]->get_relative_permittivity()->get_dbl_value();
            loss_low=frequencyList[i]->get_loss()->get_dbl_value();
            found_low=true;
         }

         if (freq_test > frequency_ && freq_test < freq_high) {
            freq_high=freq_test;
            eps_high=frequencyList[i]->get_relative_permittivity()->get_dbl_value();
            loss_high=frequencyList[i]->get_loss()->get_dbl_value();
            found_high=true;
         }

         i++;
      }

      // linear interpolation
      if (! found && found_low && found_high) {
         found=true;
         eps=eps_low+(frequency_-freq_low)/(freq_high-freq_low)*(eps_high-eps_low);
         loss_=loss_low+(frequency_-freq_low)/(freq_high-freq_low)*(loss_high-loss_low);
      }

      // wrap up
      if (found) {
         if (frequencyList[i]->get_loss()->get_keyword().compare("loss_tangent") == 0 ||
             frequencyList[i]->get_loss()->get_keyword().compare("tand") == 0 || 
             frequencyList[i]->get_loss()->get_keyword().compare("tandel") == 0) {
            complex_eps=std::complex<double>(eps*eps0,-loss_*eps*eps0);            // loss tangent
         } else {
            complex_eps=std::complex<double>(eps*eps0,-loss_/(2*M_PI*frequency_)); // conductivity
         }
      }

   } else {

      // Debye model

      // do the calculation in conductivity using the sigma variable
      double sigma;
      eps=relative_permeability.get_dbl_value();
      loss_=loss.get_dbl_value();

      if (loss.get_keyword().compare("loss_tangent") == 0 ||
          loss.get_keyword().compare("tand") == 0 ||
          loss.get_keyword().compare("tandel") == 0) {
         sigma=2*M_PI*frequency_*eps*eps0*loss_;   // loss tangent
      } else {
         sigma=loss_;                              // conductivity
      }

      std::complex<double> t1=std::complex<double>(pow(10,m1.get_dbl_value()),2*M_PI*frequency_);
      std::complex<double> t2=std::complex<double>(pow(10,m2.get_dbl_value()),2*M_PI*frequency_);
      std::complex<double> denom=std::complex<double>(log(10),0);
      std::complex<double> conductivity_term=std::complex<double>(0,-sigma/(2*M_PI*frequency_));
      std::complex<double> infinity_term=std::complex<double>(er_infinity.get_dbl_value()*eps0,0);
      std::complex<double> delta_term=std::complex<double>(delta_er.get_dbl_value()*eps0/(m2.get_dbl_value()-m1.get_dbl_value()),0);

      complex_eps=infinity_term+delta_term*log(t2/t1)/denom+conductivity_term;
   }

   return complex_eps;
}

double Temperature::get_mu(double frequency_, double tolerance, std::string indent)
{
   double mu=-DBL_MAX;

   double freq_low=0;         // for linear interpolation - extrapolation is not supported
   double mu_low=1;           // ToDo - replace with spline interpolation or
   double freq_high=DBL_MAX;  //        with a Hilbert Transform algorithm for improved causality
   double mu_high=1;          //
   double freq_test;          //
   bool found_low=false;      //
   bool found_high=false;     //

   if (frequencyList.size() > 0) {

      // frequency list
      bool found=false;
      long unsigned int i=0;
      while (i < frequencyList.size()) {

         // any
         if (frequencyList[i]->get_frequency()->get_value().compare("any") == 0) {
            mu=frequencyList[i]->get_relative_permeability()->get_dbl_value();
            found=true;
            break;
         }

         // exact match
         if (double_compare (frequency_, frequencyList[i]->get_frequency()->get_dbl_value(), tolerance)) {
            mu=frequencyList[i]->get_relative_permeability()->get_dbl_value();
            found=true;
            break;
         }

         // linear interoplation, if needed
         freq_test=frequencyList[i]->get_frequency()->get_dbl_value();

         if (freq_test < frequency_ && freq_test > freq_low) {
            freq_low=freq_test;
            mu_low=frequencyList[i]->get_relative_permeability()->get_dbl_value();
            found_low=true;
         }

         if (freq_test > frequency_ && freq_test < freq_high) {
            freq_high=freq_test;
            mu_high=frequencyList[i]->get_relative_permeability()->get_dbl_value();
            found_high=true;
         }

         i++;
      }

      // linear interpolation
      if (! found && found_low && found_high) {
         found=true;
         mu=mu_low+(frequency_-freq_low)/(freq_high-freq_low)*(mu_high-mu_low);
      }

      // wrap up
      if (found) mu=4e-7*M_PI*mu;

   } else {

      // Debye model
      mu=4e-7*M_PI*relative_permeability.get_dbl_value();
   }

   return mu;
}

double Temperature::get_Rs(double frequency_, double tolerance, std::string indent)
{
   double Rs=-DBL_MAX;
   double loss_;
   double mur_;
   double Rz_;
   std::string loss_tangent="loss_tangent";

   double freq_low=0;         // for linear interpolation - extrapolation is not supported
   double loss_low;           // ToDo - replace with spline interpolation or
   double mur_low;            //        with a Hilbert Transform algorithm for improved causality
   double Rz_low;             //
   double freq_high=DBL_MAX;  //
   double loss_high;          //
   double mur_high;           //
   double Rz_high;            //
   double freq_test;          //
   bool found_low=false;      //
   bool found_high=false;     //

   if (frequencyList.size() > 0) {

      // frequency list
      bool found=false;
      long unsigned int i=0;
      while (i < frequencyList.size()) {

         // any
         if (frequencyList[i]->get_frequency()->get_value().compare("any") == 0) {
            loss_=frequencyList[i]->get_loss()->get_dbl_value();
            mur_=frequencyList[i]->get_relative_permeability()->get_dbl_value();
            Rz_=frequencyList[i]->get_Rz()->get_dbl_value();
            found=true;
            break;
         }

         // exact match
         if (double_compare (frequency_, frequencyList[i]->get_frequency()->get_dbl_value(), tolerance)) {
            loss_=frequencyList[i]->get_loss()->get_dbl_value();
            mur_=frequencyList[i]->get_relative_permeability()->get_dbl_value();
            Rz_=frequencyList[i]->get_Rz()->get_dbl_value();
            found=true;
            break;
         }

         // linear interoplation, if needed
         freq_test=frequencyList[i]->get_frequency()->get_dbl_value();

         if (freq_test < frequency_ && freq_test > freq_low) {
            freq_low=freq_test;
            loss_low=frequencyList[i]->get_loss()->get_dbl_value();
            mur_low=frequencyList[i]->get_relative_permeability()->get_dbl_value();
            Rz_low=frequencyList[i]->get_Rz()->get_dbl_value();
            found_low=true;
         }

         if (freq_test > frequency_ && freq_test < freq_high) {
            freq_high=freq_test;
            loss_high=frequencyList[i]->get_loss()->get_dbl_value();
            mur_high=frequencyList[i]->get_relative_permeability()->get_dbl_value();
            Rz_high=frequencyList[i]->get_Rz()->get_dbl_value();
            found_high=true;
         }

         i++;
      }

      // linear interpolation
      if (! found && found_low && found_high) {
         found=true;
         loss_=loss_low+(frequency_-freq_low)/(freq_high-freq_low)*(loss_high-loss_low);
         mur_=mur_low+(frequency_-freq_low)/(freq_high-freq_low)*(mur_high-mur_low);
         Rz_=Rz_low+(frequency_-freq_low)/(freq_high-freq_low)*(Rz_high-Rz_low);
      }

      // wrap up
      if (found) {
         if (frequencyList[i]->get_loss()->get_keyword().compare("loss_tangent") == 0 ||
             frequencyList[i]->get_loss()->get_keyword().compare("tand") == 0 ||
             frequencyList[i]->get_loss()->get_keyword().compare("tandel") == 0) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%s%sERROR1072: Attempt to use a dielectric model for an Rs calculation.\n",indent.c_str(),indent.c_str(),indent.c_str());
         } else {

            Rs=sqrt(M_PI*frequency_*4e-7*M_PI*mur_/loss_);

            // apply a correction factor for surface roughness using equation (5) from:
            //     Vladimir Dmitriev-Zdorov and Lambert Simonovich, "Causal Version of Conductor Roughness Models and
            //     its Effect on Characteristics of Transmission Lines", 2017 IEEE 26th Conference on Electrical Performance
            //     of Electronic Packaging and Systems (EPEPS), 2017.

            double r=Rz_/(2*sqrt(3)*2*(1+sqrt(2)));
            double Aflat=36*r*r;
            double delta=sqrt(1/(M_PI*frequency_*4e-7*M_PI*mur_*loss_));
            double factor;
            if (Rz_ == 0) factor=1;
            else factor=1+84*(M_PI*r*r/Aflat)/(1+delta/r+delta*delta/(2*r*r));

            Rs*=factor;
         }
      }
   } else {

      // Debye model
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%s%sERROR1073: Attempt to use a Debye model for an Rs calculation.\n",indent.c_str(),indent.c_str(),indent.c_str());
   }

   return Rs;
}

void Temperature::set_freespace ()
{
    Frequency *newFrequency=new Frequency(0,0,false);
    newFrequency->set_freespace();
    frequencyList.push_back(newFrequency);

    temperature.set_keyword("temperature");
    temperature.set_value("any");
    temperature.set_loaded(true);
    temperature.set_lineNumber(0);
}

void Temperature::set_FR4 ()
{
    temperature.set_keyword("temperature");
    temperature.set_value("any");
    temperature.set_loaded(true);
    temperature.set_lineNumber(0);

    er_infinity.set_keyword("er_infinity");
    er_infinity.set_dbl_value(4.27);
    er_infinity.set_loaded(true);
    er_infinity.set_lineNumber(0);

    delta_er.set_keyword("delta_er");
    delta_er.set_dbl_value(1.12);
    delta_er.set_loaded(true);
    delta_er.set_lineNumber(0);

    m1.set_keyword("m1");
    m1.set_dbl_value(4);
    m1.set_loaded(true);
    m1.set_lineNumber(0);

    m2.set_keyword("m2");
    m2.set_dbl_value(12);
    m2.set_loaded(true);
    m2.set_lineNumber(0);

    relative_permeability.set_keyword("mur");
    relative_permeability.set_dbl_value(1);
    relative_permeability.set_loaded(true);
    relative_permeability.set_lineNumber(0);

    loss.set_keyword("conductivity");
    loss.set_dbl_value(8e-11);
    loss.set_loaded(true);
    loss.set_lineNumber(0);
}

void Temperature::set_copper ()
{
    Frequency *newFrequency=new Frequency(0,0,false);
    newFrequency->set_copper();
    frequencyList.push_back(newFrequency);

    temperature.set_keyword("temperature");
    temperature.set_value("");
    temperature.set_dbl_value(20);
    temperature.set_loaded(true);
    temperature.set_lineNumber(0);
}

Temperature::~Temperature()
{
   long unsigned int i=0;
   while (i < frequencyList.size()) {
      if (frequencyList[i]) {delete frequencyList[i]; frequencyList[i]=nullptr;}
      i++;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Source
///////////////////////////////////////////////////////////////////////////////////////////

Source::Source (int startLine_, int endLine_)
{
   startLine=startLine_;
   endLine=endLine_;
}

void Source::print(std::string indent)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %sSource\n",startLine,indent.c_str());

   long unsigned int i=0;
   while (i < lineNumberList.size()) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s%s\n",lineNumberList[i],indent.c_str(),indent.c_str(),lineList[i].c_str());
      i++;
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %sEndSource\n",endLine,indent.c_str());
}

bool Source::load(inputFile *inputs)
{
   int start_lineNumber=inputs->get_next_lineNumber(startLine);
   int stop_lineNumber=inputs->get_previous_lineNumber(endLine);

   while (start_lineNumber <= stop_lineNumber) {
      lineNumberList.push_back(start_lineNumber);
      lineList.push_back(inputs->get_line(start_lineNumber));
      start_lineNumber=inputs->get_next_lineNumber(start_lineNumber);
   }
   return false;
}

bool Source::inSourceBlock (int lineNumber)
{
   if (lineNumber >= startLine && lineNumber <= endLine) return true;
   return false;
}

void Source::set_freespace ()
{
    lineNumberList.push_back(0);
    lineList.push_back("physics definition");
}

void Source::set_FR4 ()
{
    lineNumberList.push_back(0);
    lineNumberList.push_back(0);
    lineNumberList.push_back(0);

    lineList.push_back("A.R. Djordjevi, R.M Biljie, V.D. Likar-Dmiljanic, T.K. Sarkar,");
    lineList.push_back("\"Wideband Frequency-Domain Characterization of FR-4 and Time-Domain Causality,\"");
    lineList.push_back("IEEE Trans. Electromagnetic Compatibility, Nov. 2001, pp. 662-667.");
}

void Source::set_copper ()
{
    lineNumberList.push_back(0);
    lineList.push_back("David M. Pozar, \"Microwave Engineering,\" Addison-Wesley Publishing Company, 1990, p.714.");
}

///////////////////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////////////////


Material::Material (int startLine_, int endLine_)
{
   startLine=startLine_;
   endLine=endLine_;

   name.push_alias("name");
   name.set_loaded(false);
   name.set_positive_required(false);
   name.set_non_negative_required(false);
   name.set_lowerLimit(0);
   name.set_upperLimit(0);
   name.set_checkLimits(false);

   type.push_alias("type");
   type.set_loaded(false);
   type.set_positive_required(false);
   type.set_non_negative_required(false);
   type.set_lowerLimit(0);
   type.set_upperLimit(0);
   type.set_checkLimits(false);
}

void Material::print(std::string indent)
{
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: Material",startLine);
   if (merged) PetscPrintf(PETSC_COMM_WORLD,"(merged)");
   PetscPrintf(PETSC_COMM_WORLD,"\n");

   if (isLocal) {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: isLocal=true\n",name.get_lineNumber());}
   else {prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: isLocal=false\n",name.get_lineNumber());}

   if (name.is_loaded()) {
       prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s\n",name.get_lineNumber(),indent.c_str(),name.get_value().c_str());
   }

   if (type.is_loaded()) {
       prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s%s\n",type.get_lineNumber(),indent.c_str(),type.get_value().c_str());
   }

   long unsigned int i=0;
   while (i < temperatureList.size()) {
      temperatureList[i]->print(indent);
      i++;
   }

   i=0;
   while (i < sourceList.size()) {
      sourceList[i]->print(indent);
      i++;
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: EndMaterial\n",endLine);
}

bool Material::is_conductor ()
{
    if (type.get_value().compare("conductor") == 0) return true;
    return false;
}

bool Material::is_dielectric ()
{
    if (type.get_value().compare("dielectric") == 0) return true;
    return false;
}

bool Material::findTemperatureBlocks(inputFile *inputs, bool checkLimits)
{
   bool fail=false;
   int start_lineNumber=startLine;
   int stop_lineNumber=endLine;
   int block_start,block_stop;

   while (start_lineNumber < stop_lineNumber) {
      if (inputs->findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                 "Temperature", "EndTemperature", false)) {
         fail=true;
      } else {
         if (block_start >=0 && block_stop >= 0) {
            Temperature *newTemperature=new Temperature(block_start,block_stop,checkLimits);
            temperatureList.push_back(newTemperature);
         }
      }
      start_lineNumber=inputs->get_next_lineNumber(block_stop);
   }
   return fail;
}

bool Material::findSourceBlocks(inputFile *inputs)
{
   bool fail=false;
   int start_lineNumber=startLine;
   int stop_lineNumber=endLine;
   int block_start,block_stop;

   while (start_lineNumber < stop_lineNumber) {
      if (inputs->findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                            "Source", "EndSource", false)) {
         fail=true;
      } else {
         if (block_start >=0 && block_stop >= 0) {
            Source *newSource=new Source(block_start,block_stop);
            sourceList.push_back(newSource);
         }
      }
      start_lineNumber=inputs->get_next_lineNumber(block_stop);
   }
   return fail;
}

bool Material::inTemperatureBlocks(int lineNumber)
{
   long unsigned int i=0;
   while (i < temperatureList.size()) {
      if (temperatureList[i]->inTemperatureBlock(lineNumber)) return true;
      i++;
   }
   return false;
}

bool Material::inSourceBlocks(int lineNumber)
{
   long unsigned int i=0;
   while (i < sourceList.size()) {
      if (sourceList[i]->inSourceBlock(lineNumber)) return true;
      i++;
   }
   return false;
}

bool Material::load(std::string *indent, inputFile *inputs, bool checkInputs, bool isLocal_)
{
   bool fail=false;
   isLocal=isLocal_;

   // Temperature blocks
   if (findTemperatureBlocks(inputs, checkInputs)) fail=true;

   long unsigned int i=0;
   while (i < temperatureList.size()) {
      if (temperatureList[i]->load(indent,inputs,checkInputs)) fail=true;
      i++;
   }

   // Souce blocks
   if (findSourceBlocks(inputs)) fail=true;

   i=0;
   while (i < sourceList.size()) {
      if (sourceList[i]->load(inputs)) fail=true;
      i++;
   }

   //  now the keyword pairs

   int lineNumber=inputs->get_next_lineNumber(startLine);
   int stopLineNumber=inputs->get_previous_lineNumber(endLine);
   while (lineNumber <= stopLineNumber) {

      if (!inTemperatureBlocks(lineNumber) && !inSourceBlocks(lineNumber)) {
         std::string token,value,line;
         line=inputs->get_line(lineNumber);
         get_token_pair(&line,&token,&value,&lineNumber,*indent);

         int recognized=0;
         if (name.match_alias(&token)) {
            if (name.is_loaded()) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1074: Duplicate entry at line %d for previous entry at line %d.\n",
                                                      indent->c_str(),lineNumber,name.get_lineNumber());
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
                 prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1074: Duplicate entry at line %d for previous entry at line %d.\n",
                             indent->c_str(),lineNumber,type.get_lineNumber());
                 fail=true;
             } else {
                 type.set_keyword(token);
                 type.set_value(value);
                 type.set_lineNumber(lineNumber);
                 type.set_loaded(true);
             }
             recognized++;
         }

         // should recognize one keyword
         if (recognized != 1) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1075: Unrecognized keyword at line %d.\n",indent->c_str(),lineNumber);
            fail=true;
         }
      }
      lineNumber=inputs->get_next_lineNumber(lineNumber);
   }

   return fail;
}

bool Material::check(std::string indent)
{
   bool fail=false;

   if (name.is_loaded()) {
      if (name.get_value().compare("") == 0) {
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1080: name is blank at line %d.\n",indent.c_str(),name.get_lineNumber());
         fail=true;
      }
   } else {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1081: Material block at line %d must specify a name.\n",indent.c_str(),startLine);
      fail=true;
   }

   if (type.is_loaded()) {
       if (type.get_value().compare("conductor") != 0 && type.get_value().compare("dielectric") != 0) {
           prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1080: type must be \"conductor\" or \"dielectric\" at line %d.\n",indent.c_str(),type.get_lineNumber());
           fail=true;
       }
   } else {
       prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1081: Material block at line %d must specify a type.\n",indent.c_str(),startLine);
       fail=true;
   }

   // no Temperature blocks
   if (temperatureList.size() == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1082: Material block at line %d must specify at least one Temperature block.\n",indent.c_str(),startLine);
      fail=true;
   }

   // no Source blocks
   if (sourceList.size() == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1083: Material block at line %d must specify at least one Source block.\n",indent.c_str(),startLine);
      fail=true;
   }

   // temperature block checks
   long unsigned int i=0;
   while (i < temperatureList.size()) {

      // single block checks
      if (temperatureList[i]->check(indent)) fail=true;

      // cross-block checks
      long unsigned int j=i+1;
      while (i < temperatureList.size()-1 && j < temperatureList.size()) {
         if (temperatureList[i]->get_temperature()->is_any()) {
            if (temperatureList[j]->get_temperature()->is_any()) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1084: Material block at line %d incorrectly specifies another temperature block with temperature=any at line %d\n",
                                                      indent.c_str(),startLine,temperatureList[j]->get_startLine());
               fail=true;
            } else {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1085: Material block at line %d incorrectly specifies a temperature block at line %d after specifying \"any\" at line %d.\n",
                                                      indent.c_str(),startLine,temperatureList[j]->get_startLine(),temperatureList[i]->get_temperature()->get_lineNumber());
               fail=true;
            }
         } else {
            if (temperatureList[j]->get_temperature()->is_any()) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1086: Material block at line %d incorrectly specifies a temperature block at line %d after specifying \"any\" at line %d.\n",
                                                      indent.c_str(),startLine,temperatureList[i]->get_startLine(),temperatureList[j]->get_temperature()->get_lineNumber());
               fail=true;
            } else {
               if (temperatureList[i]->get_temperature()->dbl_compare(temperatureList[j]->get_temperature())) {
                  prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1087: Material block at line %d has temperature blocks with duplicated temperatures at lines %d and %d.\n",
                                                         indent.c_str(),startLine,temperatureList[i]->get_temperature()->get_lineNumber(),temperatureList[j]->get_temperature()->get_lineNumber());
                  fail=true;
               }
            }
         }
         j++;
      }
      i++;
   }

   return fail;
}

// Find the temperature block matching the given temperature.
// Do not interpolate or extrapolate.
Temperature* Material::get_temperature(double temperature_, double tolerance, std::string indent)
{
   long unsigned int i=0;
   while (i < temperatureList.size()) {

      // any
      if (temperatureList[i]->get_temperature()->get_value().compare("any") == 0) {
         return temperatureList[i];
      }

      // exact match
      if (double_compare (temperature_, temperatureList[i]->get_temperature()->get_dbl_value(), tolerance)) {
         return temperatureList[i];
      }

      i++;
   }

   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%s%sERROR1088: Failed to find a temperature entry for material \"%s\" at a temperature of %g degrees.\n",
                                          indent.c_str(),indent.c_str(),indent.c_str(),name.get_value().c_str(),temperature_);

   return nullptr;
}

std::complex<double> Material::get_eps(double temperature_, double frequency, double tolerance, std::string indent)
{
   Temperature *temperature;
   std::complex<double> complex_eps=std::complex<double>(-DBL_MAX,0);

   temperature=get_temperature(temperature_, tolerance, indent);
   if (temperature) complex_eps=temperature->get_eps(frequency, tolerance, indent);

   return complex_eps;
}

double Material::get_mu(double temperature_, double frequency, double tolerance, std::string indent)
{
   Temperature *temperature;
   double mu=-DBL_MAX;

   temperature=get_temperature(temperature_, tolerance, indent);
   if (temperature) mu=temperature->get_mu(frequency, tolerance, indent);

   return mu;
}

double Material::get_Rs(double temperature_, double frequency, double tolerance, std::string indent)
{
   Temperature *temperature;
   double Rs=-DBL_MAX;

   temperature=get_temperature(temperature_, tolerance, indent);
   if (temperature) Rs=temperature->get_Rs(frequency, tolerance, indent);

   return Rs;
}

Material::~Material ()
{
   long unsigned int i=0;
   while (i < temperatureList.size()) {
       if (temperatureList[i]) {delete temperatureList[i]; temperatureList[i]=nullptr;}
      i++;
   }

   i=0;
   while (i < sourceList.size()) {
       if (sourceList[i]) {delete sourceList[i]; sourceList[i]=nullptr;}
      i++;
   }
}

void Material::set_freespace ()
{
    name.set_keyword("name");
    name.set_value("freespace");
    name.set_loaded(true);
    name.set_lineNumber(0);

    type.set_keyword("type");
    type.set_value("dielectric");
    type.set_loaded(true);
    type.set_lineNumber(0);

    Temperature* newTemperature=new Temperature(0,0,false);
    newTemperature->set_freespace();
    temperatureList.push_back(newTemperature);

    Source* newSource=new Source(0,0);
    newSource->set_freespace();
    sourceList.push_back(newSource);
}

void Material::set_FR4 ()
{
    name.set_keyword("name");
    name.set_value("FR4");
    name.set_loaded(true);
    name.set_lineNumber(0);

    type.set_keyword("type");
    type.set_value("dielectric");
    type.set_loaded(true);
    type.set_lineNumber(0);

    Temperature* newTemperature=new Temperature(0,0,false);
    newTemperature->set_FR4();
    temperatureList.push_back(newTemperature);

    Source* newSource=new Source(0,0);
    newSource->set_FR4();
    sourceList.push_back(newSource);
}

void Material::set_copper ()
{
    name.set_keyword("name");
    name.set_value("copper");
    name.set_loaded(true);
    name.set_lineNumber(0);

    type.set_keyword("type");
    type.set_value("conductor");
    type.set_loaded(true);
    type.set_lineNumber(0);

    Temperature* newTemperature=new Temperature(0,0,false);
    newTemperature->set_copper();
    temperatureList.push_back(newTemperature);

    Source* newSource=new Source(0,0);
    newSource->set_copper();
    sourceList.push_back(newSource);
}

///////////////////////////////////////////////////////////////////////////////////////////
// MaterialDatabase
///////////////////////////////////////////////////////////////////////////////////////////

bool MaterialDatabase::findMaterialBlocks()
{
   bool fail=false;
   int start_lineNumber=inputs.get_first_lineNumber();
   int stop_lineNumber=inputs.get_last_lineNumber();
   int block_start,block_stop;

   // skip the version line, which must be the first line
   start_lineNumber=inputs.get_next_lineNumber(start_lineNumber);

   while (start_lineNumber < stop_lineNumber) {
      if (inputs.findBlock(start_lineNumber,stop_lineNumber, &block_start, &block_stop,
                                "Material", "EndMaterial", true)) {
         fail=true;
      } else {
         if (block_start >= 0 && block_stop >= 0) {
            Material *newMaterial=new Material(block_start,block_stop);
            materialList.push_back(newMaterial);
         }
      }
      start_lineNumber=inputs.get_next_lineNumber(block_stop);
   }
   return fail;
}

// return true on fail
bool MaterialDatabase::load (const char *path, const char *filename, bool checkInputs, bool isLocal)
{
   if (!path) return true;
   if (!filename) return true;
   if (strlen(path) == 0) return true;
   if (strlen(filename) == 0) return true;

   // check for path/filename separator

   bool pathHasSeparator=false;
   if (path[strlen(path)-1] == '/') pathHasSeparator=true;

   bool filenameHasSeparator=false;
   if (filename[0] == '/') filenameHasSeparator=true;

   // assemble the full path name

   char *fullPathName=nullptr;
   fullPathName=(char *)malloc((strlen(path)+strlen(filename)+2)*sizeof(char));
   if (!fullPathName) return true;

   if (pathHasSeparator) {
      if (filenameHasSeparator) {
         sprintf (fullPathName,"%s%s",path,filename);
      } else {
         sprintf (fullPathName,"%s%s",path,filename);
      }
   } else {
      if (filenameHasSeparator) {
         sprintf (fullPathName,"%s%s",path,filename);
      } else {
         sprintf (fullPathName,"%s/%s",path,filename);
      }
   }
   prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sloading materials file \"%s\"\n",indent.c_str(),fullPathName);
 
   bool fail=false;
   if (inputs.load(fullPathName)) {if (fullPathName) free(fullPathName); fullPathName=nullptr; return true;}
   if (fullPathName) {free(fullPathName); fullPathName=nullptr;}
   inputs.createCrossReference();

   if (inputs.checkVersion(version_name, version_value)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1089: Version mismatch.  Expecting the first line to be: %s %s\n",
                                             indent.c_str(),indent.c_str(),version_name.c_str(),version_value.c_str());
      return true;
   }

   // load the materials file

   if (findMaterialBlocks()) fail=true;

   long unsigned int i=0;
   while (i < materialList.size()) {
      if (materialList[i]->load(&indent, &inputs, checkInputs,isLocal)) fail=true;
      i++;
   }

   if (check()) fail=true;

   if (fail) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1090: Failed to load materials.\n",indent.c_str(),indent.c_str());
      return fail;
   }

   return fail;
};

Material* MaterialDatabase::get(std::string name)
{
    // remove any uniqueification that may have been applied
    size_t pos=name.rfind("_OPEM_RESERVED_");
    std::string strippedName=name.substr(0,pos);

   long unsigned int i=0;
   while (i < materialList.size()) {
      //if (materialList[i]->get_name()->get_value().compare(name) == 0) return materialList[i];
       if (materialList[i]->get_name()->get_value().compare(strippedName) == 0) return materialList[i];
      i++;
   }
   return nullptr;
}

void MaterialDatabase::print(std::string indent)
{
   long unsigned int i=0;
   while (i < materialList.size()) {
      materialList[i]->print(indent);
      i++;
   }
}

bool MaterialDatabase::check()
{
   bool fail=false;

   if (materialList.size() == 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1091: No materials loaded.\n",indent.c_str());
      fail=true;
   }

   long unsigned int i=0;
   while (i < materialList.size()) {

      // single block checks
      if (materialList[i]->check(indent)) fail=true;

      // cross-block checks
      long unsigned int j=i+1;
      while (i < materialList.size()-1 && j < materialList.size()) {
         if (materialList[i]->get_name()->get_value().compare(materialList[j]->get_name()->get_value()) == 0) {
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1092: name at line %d duplicates the name at line %d.\n",
                                                   indent.c_str(),materialList[j]->get_name()->get_lineNumber(),materialList[i]->get_name()->get_lineNumber());
            fail=true;
         }
         j++;
      }
      i++;
   }

   return fail;
}

bool MaterialDatabase::load_materials (char *global_path, char *global_name, char *local_path, char *local_name, bool check_limits)
{
   bool global=true;
   if (strlen(global_name) != 0) {
      global=load(global_path,global_name,check_limits,false);
   }

   bool local=true;
   MaterialDatabase localMaterialDatabase;
   if (strlen(local_name) != 0) {
      local=localMaterialDatabase.load(local_path,local_name,check_limits,true);
   }

   if (!local) {
      if (merge(&localMaterialDatabase,get_indent())) return true;
   }

   if (!global || !local) return false;
   return true;
}

// MaterialDatabase takes ownership of the contents of db
bool MaterialDatabase::merge(MaterialDatabase *db, std::string indent)
{
   // checks for nothing to do
   if (!db) return false;
   if (db->materialList.size() == 0) return false;

   // check for version alignment
   if (version_name.compare(db->version_name) != 0 ||
       version_value.compare(db->version_value) != 0) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sERROR1093: Material database merge attempted with different versions.\n",indent.c_str());
      return true;
   }

   // merge in db

   if (! double_compare(tol,db->tol,db->tol)) {
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sReplacing tol with the merged database value.\n",indent.c_str());
   }

   // add the new materials while checking for duplicates
   long unsigned int i=0;
   while (i < db->materialList.size()) {
      bool found=false;
      long unsigned int j=0;
      while (j < materialList.size()) {
         if (! materialList[j]->get_merged()) {
            if (db->materialList[i]->get_name()->get_value().compare(materialList[j]->get_name()->get_value()) == 0) {
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%sReplacing duplicate material \"%s\" with the material from the local material database.\n",
                                                      indent.c_str(),db->materialList[i]->get_name()->get_value().c_str());
               delete materialList[j];
               materialList[j]=db->materialList[i];
               materialList[j]->set_merged(true);
               found=true;
               break;
            }
         }
         j++;
      }

      if (!found) {
         push(db->materialList[i]);
         materialList[materialList.size()-1]->set_merged(true);   // data ownership stays with db
      }
      i++;
   }

   db->isTransferred=true;
   return 0;
}

void MaterialDatabase::clear ()
{
    long unsigned int i=0;
    while (i < materialList.size()) {
        if (materialList[i]) {delete materialList[i]; materialList[i]=nullptr;}
        i++;
    }
    materialList.clear();
    isTransferred=false;
    inputs.clear();
}

MaterialDatabase::~MaterialDatabase()
{
   if (isTransferred) return;
   clear();
}


