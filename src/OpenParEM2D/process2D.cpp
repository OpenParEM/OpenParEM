////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    process2D - processor to automation of regression testing of OpenParEM2D//
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

#include "process2D.hpp"

string processingFileName;
string routine;

void printError (int lineNumber, int place)
{
   cout << "ERROR2216: " << processingFileName << ": " << routine << ": Incorrect formatting in line " << lineNumber << " at place " << place << "." << endl;
}

void printError2 (int lineNumber)
{
   cout << "ERROR2217: " << processingFileName << ": " << routine << ": Incorrect number of entries at line " << lineNumber << "." << endl;
}

void printError3 (string variable, int lineNumber)
{
   cout << "ERROR2218: " << processingFileName << ": " << routine << ": " << variable << " is incorrectly formatted at line " << lineNumber << "." << endl;
}

bool is_resultVariable (string test)
{
   if (test.compare("alpha") == 0) return true;
   if (test.compare("beta") == 0) return true;
   if (test.compare(0,6,"real_Z") == 0) return true;
   if (test.compare(0,6,"imag_Z") == 0) return true;

   return false;
}

bool is_fieldVariable (string test)
{
   if (test.compare("real_Emag") == 0) return true;
   if (test.compare("real_Ephi") == 0) return true;
   if (test.compare("real_Etheta") == 0) return true;

   if (test.compare("imag_Emag") == 0) return true;
   if (test.compare("imag_Ephi") == 0) return true;
   if (test.compare("imag_Etheta") == 0) return true;

   if (test.compare("real_Hmag") == 0) return true;
   if (test.compare("real_Hphi") == 0) return true;
   if (test.compare("real_Htheta") == 0) return true;

   if (test.compare("imag_Hmag") == 0) return true;
   if (test.compare("imag_Hphi") == 0) return true;
   if (test.compare("imag_Htheta") == 0) return true;

   return false;
}

bool is_valid_testVariable (string test)
{
   if (is_resultVariable(test)) return true;
   if (is_fieldVariable(test)) return true;
   return false;
}

bool is_valid_testFunction (string test)
{
   if (test.compare("equal") == 0) return true;
   if (test.compare("lessthan") == 0) return true;
   return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------
// TestCase
//-------------------------------------------------------------------------------------------------------------------------------------

void TestCase::errorEvaluation (bool is_angular)
{
   double pi=4.*atan(1.);
   double error1,error2,error3;
   double thresholdCheck1,thresholdCheck2,thresholdCheck3,thresholdCheck;

   passed=false;

   if (testFunction.compare("equal") == 0) {
      if (expectedValue == 0) error=fabs(foundValue);
      else {
         if (is_angular) {
            error1=fabs((foundValue-expectedValue)/expectedValue);
            error2=fabs((foundValue+2*pi-expectedValue)/expectedValue);
            error3=fabs((foundValue-2*pi-expectedValue)/expectedValue);
            error=min(min(error1,error2),error3);
         } else error=fabs((foundValue-expectedValue)/expectedValue);
      }
      if (error <= tolerance) passed=true;
   } else if (testFunction.compare("lessthan") == 0) {
      error=0;
      if (is_angular) {
         thresholdCheck1=abs(foundValue);
         thresholdCheck2=abs(foundValue+2*pi);
         thresholdCheck3=abs(foundValue-2*pi);
         thresholdCheck=min(min(thresholdCheck1,thresholdCheck2),thresholdCheck3);
      } else thresholdCheck=abs(foundValue);
      if (thresholdCheck <= threshold) passed=true;
   }

   evaluated=true;
}

// don't evaluate on voltage
void TestCase::evaluate(ResultDatabase *resultDatabase, EMfieldDatabase *emfieldDatabase)
{
   bool loaded=false;
   bool is_angular=false;
   double testValue;
   string fieldVariables[12]={"real_Emag","imag_Emag","real_Ephi","imag_Ephi","real_Etheta","imag_Etheta","real_Hmag","imag_Hmag","real_Hphi","imag_Hphi","real_Htheta","imag_Htheta"};
   bool angularVariables[12]={false,false,true,true,true,true,false,false,true,true,true,true};
   string magVariables[12]={"none","none","real_Emag","imag_Emag","real_Emag","imag_Emag","none","none","real_Hmag","imag_Hmag","real_Hmag","imag_Hmag"};

   if (is_resultVariable(testVariable) && resultDatabase) {

      if (testVariable.compare("alpha") == 0) {
         complex<double> gamma=resultDatabase->get_gamma(frequency,mode-1);
         if (is_complex(gamma)) {
            foundValue=real(gamma);
            loaded=true;
         }
      } else if (testVariable.compare("beta") == 0) {
         complex<double> gamma=resultDatabase->get_gamma(frequency,mode-1);
         if (is_complex(gamma)) {
            foundValue=imag(gamma);
            loaded=true;
         }
      } else if (testVariable.compare(0,6,"real_Z") == 0) {
         complex<double> impedance=resultDatabase->get_impedance(frequency,mode-1);
         if (is_complex(impedance)) {
            foundValue=real(impedance);
            loaded=true;
         }
      } else if (testVariable.compare(0,6,"imag_Z") == 0) {
         complex<double> impedance=resultDatabase->get_impedance(frequency,mode-1);
         if (is_complex(impedance)) {
            foundValue=imag(impedance);
            loaded=true;
         }
      } else {cout << "ASSERT: TestCase::evaluate" << endl;}

   } else if (is_fieldVariable(testVariable) && emfieldDatabase) {
      int i=0;
      while (i < 12) {
         if (testVariable.compare(fieldVariables[i]) == 0) {
            if (emfieldDatabase->get_EMfield (fieldVariables[i].c_str(),frequency,mode,x,y,&testValue)) {
               if (angularVariables[i]) is_angular=true;
               foundValue=testValue;
               loaded=true;
               break;
            }
         }
         i++;
      }
   }

   if (loaded) errorEvaluation(is_angular);
   else {
      cout << "ASSERT: Testcase failed to evaluate." << endl;
      printAllFormatted();
   }
}

void TestCase::print_as_testcase()
{
   cout << name << ",";
   cout << frequency << ",";
   cout << mode << ",";
   cout << testVariable << ",";
   if (is_fieldVariable(testVariable)) {
      cout << x << ",";
      cout << y << ",";
   }
   cout << testFunction << ",";

   if (testFunction.compare("equal") == 0) {
      cout << setprecision(15) << expectedValue << ",";
      cout << tolerance << endl;
   } else if (testFunction.compare("lessthan") == 0) {
       cout << threshold << endl;
   }
}

void TestCase::print()
{
   cout << name << ",";
   cout << frequency << ",";
   cout << mode << ",";
   cout << testVariable << ",";
   if (is_fieldVariable(testVariable)) {
      cout << x << ",";
      cout << y << ",";
   }
   cout << testFunction << ",";
   if (testFunction.compare("equal") == 0) cout << setprecision(15) << expectedValue << ",";
   cout << setprecision(15) << foundValue << ",";

   if (testFunction.compare("equal") == 0) {
      cout << error << ",";
      cout << tolerance << ",";
   } else if (testFunction.compare("lessthan") == 0) {
       cout << threshold << ",";
   }
   cout << passed << endl;
}

void TestCase::printAllFormatted()
{
   cout << name << endl;
   cout << "   frequency=" << frequency << endl;
   cout << "   mode=" << mode << endl;
   if (is_fieldVariable(testVariable)) {
      cout << "   x=" << x << endl;
      cout << "   y=" << y << endl;
   }
   cout << "   testVariable=" << testVariable << endl;
   cout << "   testFunction=" << testFunction << endl;
   if (testFunction.compare("equal") == 0) cout << setprecision(15) << "   expectedValue=" << expectedValue << endl;
   cout << setprecision(15) << "   foundValue=" << foundValue << endl;
   if (testFunction.compare("equal") == 0) {
      cout << setprecision(15) << "   error=" << error << endl;
      cout << "   tolerance=" << tolerance  << endl;
   } else if (testFunction.compare("lessthan") == 0) {
      cout << "   threshold=" << threshold  << endl;
   }
   cout << "   passed=" << passed << endl;
   cout << "   evaluated=" << evaluated << endl;
}

void TestCase::show_evaluation(ostream *out)
{
   if (evaluated) {
      *out << name << ",";
      if (passed) *out << "pass" << ",";
      else *out << "FAIL" << ",";
      *out << frequency << ",";
      *out << mode << ",";
      if (is_fieldVariable(testVariable)) {
         *out << x << ",";
         *out << y << ",";
      }
      *out << testVariable << ",";
      *out << testFunction << ",";
      if (testFunction.compare("equal") == 0) *out << setprecision(15) << expectedValue << ",";
      *out << setprecision(15) << foundValue << ",";
      if (testFunction.compare("equal") == 0) {
         *out << setprecision(15) << error << ",";
         *out << tolerance << ",";
      } else if (testFunction.compare("lessthan") == 0) {
         *out << threshold << ",";
      }
      *out << endl;
   }
}

void TestCase::audit(string variableType, string function, unsigned long int *passed_, unsigned long int *failed_, unsigned long int *evaluated_, unsigned long int *count,
                     double *min_error, double *max_error, double *sum_error, unsigned long int *max_error_case, unsigned long int i)
{
   if ((variableType.compare("field") == 0 && is_fieldVariable(testVariable)) ||
       (variableType.compare("result") == 0 && is_resultVariable(testVariable))) {
      if (function.compare("equal") == 0 && testFunction.compare("equal") == 0) {

         if (abs(error) < abs(*min_error)) *min_error=error;
         if (abs(error) >= abs(*max_error)) {*max_error=error; *max_error_case=i;}
         (*sum_error)+=abs(error);

         if (passed) (*passed_)++;
         else (*failed_)++;

         if (evaluated) (*evaluated_)++;

         (*count)++;

      } else if (function.compare("lessthan") == 0 && testFunction.compare("lessthan") == 0) {
         double value=abs(foundValue);
      
         if (abs(value) < abs(*min_error)) *min_error=value;
         if (abs(value) >= abs(*max_error)) {*max_error=value; *max_error_case=i;}
         (*sum_error)+=abs(value);

         if (passed) (*passed_)++;
         else (*failed_)++;

         if (evaluated) (*evaluated_)++;

         (*count)++;
      }
   }
}

double TestCase::get_error_or_tolerance()
{
   if (testFunction.compare("equal") == 0) return error;
   if (testFunction.compare("lessthan") == 0) return foundValue;
   return -1;  // should not happen
}



//-------------------------------------------------------------------------------------------------------------------------------------
// TestCaseDatabase
//-------------------------------------------------------------------------------------------------------------------------------------

void TestCaseDatabase::print()
{
   cout << "# TestCaseDatabase::print" << endl;
   unsigned long int i=0;
   while (i < testCaseList.size()) {
      testCaseList[i]->print();
      i++;
   }
}

void TestCaseDatabase::print_as_testcase()
{
   cout << "# TestCaseDatabase::print_as_testcase" << endl;
   unsigned long int i=0;
   while (i < testCaseList.size()) {
      testCaseList[i]->print_as_testcase();
      i++;
   }
}

void TestCaseDatabase::audit(string variableType, string function)
{
   unsigned long int count=0;
   unsigned long int passed=0;
   unsigned long int failed=0;
   unsigned long int evaluated=0;
   double min_error=DBL_MAX;
   double max_error=0;
   double avg_error=0;
   unsigned long int max_error_case=-1;

   unsigned long int i=0;
   while (i < testCaseList.size()) {
      testCaseList[i]->audit(variableType, function, &passed, &failed, &evaluated, &count, &min_error, &max_error, &avg_error, &max_error_case, i);
      i++;
   }
   if (count > 0) avg_error/=count;

   if (passed+failed != count) cout << "ERROR2219: passed+failed != case count." << endl;
   if (evaluated != count) cout << "ERROR2220: evaluated != case count." << endl;

   if (function.compare("equal") == 0) {
      cout << "Process Audit Results for \"" << variableType << "\" with test \"" << function << "\":" << endl;
      cout << "   passed=" << passed << endl;
      cout << "   failed=" << failed << endl;
      if (count > 0) {
         cout << "   evaluated=" << evaluated << endl;
         cout << "   min_error=" << min_error << endl;
         cout << "   avg_error=" << avg_error << endl;
         cout << "   max_error=" << max_error << " : ";
         testCaseList[max_error_case]->print();
      }
   } else if (function.compare("lessthan") == 0) {
      cout << "Process Audit Results for \"" << variableType << "\" with test \"" << function << "\":" << endl;
      cout << "   passed=" << passed << endl;
      cout << "   failed=" << failed << endl;
      if (count > 0) {
         cout << "   evaluated=" << evaluated << endl;
         cout << "   min_value=" << min_error << endl;
         cout << "   avg_value=" << avg_error << endl;
         cout << "   max_value=" << max_error << " : ";
         testCaseList[max_error_case]->print();
      }
   }
}

void TestCaseDatabase::printAllFormatted ()
{
   unsigned long int i=0;
   while (i < testCaseList.size()) {
      testCaseList[i]->printAllFormatted();
      i++;
   }
}

void TestCaseDatabase::show_evaluation (ostream *out)
{
   *out << "# TestCaseDatabase::show_evaluation" << endl;
   *out << "# name,status,frequency,mode,";
   *out << "[fieldVariable:x,y,]";
   *out << "testVariable,testFunction,[equal:expectedValue,]foundValue,";
   *out << "[equal:error,tolerance | lessthan:threshold" << endl;

   unsigned long int i=0;
   while (i < testCaseList.size()) {
      testCaseList[testCaseList[i]->get_index()]->show_evaluation(out);
      i++;
   }
}

// return false on good load
// return true on bad load
bool TestCaseDatabase::loadTestCases (const char *filename)
{
   bool fail=false;
   bool is_field;
   int lineNumber=0;
   string line;
   ifstream inFile;
   inFile.open(filename,ifstream::in);

   processingFileName=filename;
   routine="TestCaseDatabase::loadTestCases";

   if (inFile.is_open()) {

      while (getline(inFile,line)) {
         lineNumber++;

         // skip blank lines
         if (line.compare("") != 0) {

            // skip comment lines
            if (! is_hashComment(line)) {

               // chop off comments
               line=line.substr(0,line.find("//",0));

               TestCase *testCase=new TestCase;

               is_field=false;

               // parse csv
               stringstream sstream(line);
               string entry;
               int count=0;
               while (std::getline(sstream, entry, ',')) {
                  if (count == 0) {
                     testCase->set_name(entry);
                  } else if (count == 1) {
                     if (is_double(&entry)) testCase->set_frequency(atof(entry.c_str()));
                     else {printError(lineNumber,count+1); fail=true;}
                  } else if (count == 2) {
                     if (is_int(&entry)) testCase->set_mode(stoi(entry));
                     else {printError(lineNumber,count+1); fail=true;}
                  } else if (count == 3) {
                     if (is_valid_testVariable(entry)) {
                        testCase->set_testVariable(entry);
                        if (is_fieldVariable(entry)) is_field=true;
                     } else {printError(lineNumber,count+1); fail=true;}
                  } else if (count == 4) {
                     if (is_field) {
                        if (is_double(&entry)) testCase->set_x(atof(entry.c_str()));
                        else {printError(lineNumber,count+1); fail=true;}
                     } else {
                        if (is_valid_testFunction(entry)) testCase->set_testFunction(entry);
                        else {printError(lineNumber,count+1); fail=true;}
                     }
                  } else if (count == 5) {
                     if (is_field) {
                        if (is_double(&entry)) testCase->set_y(atof(entry.c_str()));
                        else {printError(lineNumber,count+1); fail=true;}
                     } else {
                        if (testCase->get_testFunction().compare("equal") == 0) {
                           if (is_double(&entry)) testCase->set_expectedValue(atof(entry.c_str()));
                           else {printError(lineNumber,count+1); fail=true;}
                        } else if (testCase->get_testFunction().compare("lessthan") == 0) {
                           if (is_double(&entry)) testCase->set_threshold(atof(entry.c_str()));
                           else {printError(lineNumber,count+1); fail=true;}
                        }
                        else {printError(lineNumber,count+1); fail=true;}
                     }
                  } else if (count == 6) {
                     if (is_field) {
                        if (is_valid_testFunction(entry)) testCase->set_testFunction(entry);
                        else {printError(lineNumber,count+1); fail=true;}
                     } else {
                        if (testCase->get_testFunction().compare("equal") == 0) {
                           if (is_double(&entry)) testCase->set_tolerance(atof(entry.c_str()));
                           else {printError(lineNumber,count+1); fail=true;}
                        }
                     }
                  } else if (count == 7) {
                     if (is_field) {
                        if (testCase->get_testFunction().compare("equal") == 0) {
                           if (is_double(&entry)) testCase->set_expectedValue(atof(entry.c_str()));
                           else {printError(lineNumber,count+1); fail=true;}
                        } else if (testCase->get_testFunction().compare("lessthan") == 0) {
                           if (is_double(&entry)) testCase->set_threshold(atof(entry.c_str()));
                           else {printError(lineNumber,count+1); fail=true;}
                        }
                     }
                  } else if (count == 8) {
                     if (is_field) {
                        if (testCase->get_testFunction().compare("equal") == 0) {
                           if (is_double(&entry)) testCase->set_tolerance(atof(entry.c_str()));
                           else {printError(lineNumber,count+1); fail=true;}
                        }
                     }
                  }

                  count++;
               } 
               if ((!is_field && count == 6) || (!is_field && count == 7) || (is_field && count == 8) || (is_field && count == 9)) {
                  testCase->set_passed(false);
                  testCase->set_evaluated(false);
                  testCaseList.push_back(testCase);
               } else {
                  delete testCase;
                  printError2(lineNumber);
                  fail=true;
               }
            }
         }
      }
      inFile.close();
   } else {
      cout << "ERROR2221: Unable to open file \"" << filename << "\" for reading." << endl;
      return true;
   }

   return fail;
}

void TestCaseDatabase::evaluate(ResultDatabase *resultDatabase, EMfieldDatabase *emfieldDatabase)
{
   long unsigned int i=0;
   while (i < testCaseList.size()) {
      testCaseList[i]->evaluate(resultDatabase,emfieldDatabase);
      i++;
   }
}

void TestCaseDatabase::sort(bool sort)
{
   bool found;
   long unsigned int index_i, index_j;

   // set up
   long unsigned int i=0;
   while (i < testCaseList.size()) {
      testCaseList[i]->set_index(i);
      i++;
   }

   // sort
   i=0;
   while (sort && i < testCaseList.size()-1) {
      found=true;
      while (found) {
         found=false;
         long unsigned int j=i+1;
         while (j < testCaseList.size()) {
            index_i=testCaseList[i]->get_index();
            index_j=testCaseList[j]->get_index();
            if (fabs(testCaseList[index_i]->get_error_or_tolerance()) < fabs(testCaseList[index_j]->get_error_or_tolerance())) {
               found=true;
               testCaseList[i]->set_index(index_j);
               testCaseList[j]->set_index(index_i);
            }
            j++;
         }
      }
      i++;
   }
}

bool TestCaseDatabase::all_evaluated()
{
   long unsigned int i=0;
   while (i < testCaseList.size()) {
      if (!testCaseList[i]->get_evaluated()) return false;
      i++;
   }
   return true;
}

bool TestCaseDatabase::all_passed()
{
   long unsigned int i=0;
   while (i < testCaseList.size()) {
      if (!testCaseList[i]->get_passed()) return false;
      i++;
   }
   return true;
}

TestCaseDatabase::~TestCaseDatabase()
{
   unsigned long int i=0;
   while (i < testCaseList.size()) {
      delete testCaseList[i];
      i++;
   }
}

//-------------------------------------------------------------------------------------------------------------------------------------
// Result
//-------------------------------------------------------------------------------------------------------------------------------------

complex<double> Result::get_gamma (double frequency_, long unsigned int mode_)
{
   if (fabs((frequency-frequency_)/frequency) > 1e-14) return complex<double>(DBL_MAX,DBL_MAX);

   if (mode_ <  gamma.size()) return gamma[mode_];
   cout << "ASSERT: Result::get_gamma invalid mode" << endl;
   return complex<double>(DBL_MAX,DBL_MAX);
}

complex<double> Result::get_impedance (double frequency_, long unsigned int mode_)
{
   if (fabs((frequency-frequency_)/frequency) > 1e-14) return complex<double>(DBL_MAX,DBL_MAX);

   if (mode_ <  impedance.size()) return impedance[mode_];
   cout << "ASSERT: Result::get_impedance invalid mode" << endl;
   return complex<double>(DBL_MAX,DBL_MAX);
}

complex<double> Result::get_voltage (double frequency_, long unsigned int mode_)
{
   if (fabs((frequency-frequency_)/frequency) > 1e-14) return complex<double>(DBL_MAX,DBL_MAX);

   if (mode_ <  voltage.size()) return voltage[mode_];
   cout << "ASSERT: Result::get_voltage invalid mode" << endl;
   return complex<double>(DBL_MAX,DBL_MAX);
}

complex<double> Result::get_current (double frequency_, long unsigned int mode_)
{
   if (fabs((frequency-frequency_)/frequency) > 1e-14) return complex<double>(DBL_MAX,DBL_MAX);

   if (mode_ <  current.size()) return current[mode_];
   cout << "ASSERT: Result::get_current invalid mode" << endl;
   return complex<double>(DBL_MAX,DBL_MAX);
}

complex<double> Result::get_Pz (double frequency_, long unsigned int mode_)
{
   if (fabs((frequency-frequency_)/frequency) > 1e-14) return complex<double>(DBL_MAX,DBL_MAX);

   if (mode_ <  Pz.size()) return Pz[mode_];
   cout << "ASSERT: Result::get_Pz invalid mode" << endl;
   return complex<double>(DBL_MAX,DBL_MAX);
}

void Result::print()
{
   cout << setprecision(15) << frequency
        << ","
        << iteration
        << ","
        << mesh_size
        << ","
        << matrix_size
        << ","
        << converged
        << ","
        << final_error
        << ","
        << elapsed_time
        << ","
        << modalImpedanceCalculation
        << ","
        << modeCount
        << ",";

   long unsigned int i=0;
   while (i < modeCount) {

      // gamma
      cout << setprecision(15) << real(gamma[i])
           << ","
           << setprecision(15) << imag(gamma[i])
           << ",";

      // impedance
      cout << setprecision(15) << real(impedance[i])
           << ","
           << setprecision(15) << imag(impedance[i])
           << ",";

      // voltage
      cout << setprecision(15) << real(voltage[i])
           << ","
           << setprecision(15) << imag(voltage[i])
           << ",";

      // current
      cout << setprecision(15) << real(current[i])
           << ","
           << setprecision(15) << imag(current[i])
           << ",";

      // Pz
      cout << setprecision(15) << real(Pz[i])
           << ","
           << setprecision(15) << imag(Pz[i])
           << ",";

      i++;
   }

   cout << endl;
}

//-------------------------------------------------------------------------------------------------------------------------------------
// ResultDatabase
//-------------------------------------------------------------------------------------------------------------------------------------

complex<double> ResultDatabase::get_gamma (double frequency, int mode) {
   unsigned long int i=0;
   while (i < resultList.size()) {
      if (is_complex(resultList[i]->get_gamma(frequency,mode))) {
         return resultList[i]->get_gamma(frequency,mode);
      }
      i++;
   }
   return complex<double>(DBL_MAX,DBL_MAX);
}

complex<double> ResultDatabase::get_impedance (double frequency, long unsigned int mode) {
   unsigned long int i=0;
   while (i < resultList.size()) {
      if (is_complex(resultList[i]->get_impedance(frequency,mode))) {
         return resultList[i]->get_impedance(frequency,mode);
      }
      i++;
   }
   return complex<double>(DBL_MAX,DBL_MAX);
}

complex<double> ResultDatabase::get_voltage (double frequency, long unsigned int mode) {
   unsigned long int i=0;
   while (i < resultList.size()) {
      if (is_complex(resultList[i]->get_voltage(frequency,mode))) {
         return resultList[i]->get_voltage(frequency,mode);
      }
      i++;
   }
   return complex<double>(DBL_MAX,DBL_MAX);
}

complex<double> ResultDatabase::get_current (double frequency, long unsigned int mode) {
   unsigned long int i=0;
   while (i < resultList.size()) {
      if (is_complex(resultList[i]->get_current(frequency,mode))) {
         return resultList[i]->get_current(frequency,mode);
      }
      i++;
   }
   return complex<double>(DBL_MAX,DBL_MAX);
}

complex<double> ResultDatabase::get_Pz (double frequency, long unsigned int mode) {
   unsigned long int i=0;
   while (i < resultList.size()) {
      if (is_complex(resultList[i]->get_Pz(frequency,mode))) {
         return resultList[i]->get_Pz(frequency,mode);
      }
      i++;
   }
   return complex<double>(DBL_MAX,DBL_MAX);
}

void ResultDatabase::print()
{
   unsigned long int i=0;
   while (i < resultList.size()) {
      resultList[i]->print();
      i++;
   }
}

// return false on good load
// return true on bad load
bool ResultDatabase::loadResults (const char *filename)
{
   bool fail=false;
   int lineNumber=0;
   string line;
   ifstream inFile;
   inFile.open(filename,ifstream::in);

   processingFileName=filename;
   routine="ResultDatabase::loadResults";

   if (inFile.is_open()) {

      while (getline(inFile,line)) {
         lineNumber++;

         // skip blank lines
         if (line.compare("") != 0) {

            // skip comment lines
            if (! is_hashComment(line)) {

               // chop off comments
               line=line.substr(0,line.find("//",0));

               Result *result=new Result;
               vector<string> *csvInputs=new vector<string>;

               // parse csv
               stringstream sstream(line);
               string entry;
               while (std::getline(sstream, entry, ',')) {
                  csvInputs->push_back(entry);
               }

               if (csvInputs->size() == 0) {
                  cout << "ERROR2222: File \"" << filename << "\" parsed into zero tokens." << endl;
                  inFile.close();
                  return true;
               }

               if (csvInputs->size() < 9) {
                  cout << "ERROR2223: File \"" << filename << "\" has too few tokens." << endl;
                  inFile.close();
                  return true;
               }

               long unsigned int index=0;
               if (index < csvInputs->size() && is_double(&(*csvInputs)[index])) result->set_frequency(atof((*csvInputs)[index].c_str()));
               else printError3 ("frequency",lineNumber);

               if (++index < csvInputs->size() && is_int(&(*csvInputs)[index])) result->set_iteration(atoi((*csvInputs)[index].c_str()));
               else printError3 ("iteration",lineNumber);

               if (++index < csvInputs->size() && is_int(&(*csvInputs)[index])) result->set_mesh_size(atoi((*csvInputs)[index].c_str()));
               else printError3 ("mesh_size",lineNumber);

               if (++index < csvInputs->size() && is_int(&(*csvInputs)[index])) result->set_matrix_size(atoi((*csvInputs)[index].c_str()));
               else printError3 ("matrix_size",lineNumber);

               if (++index < csvInputs->size() && is_int(&(*csvInputs)[index])) result->set_converged(atoi((*csvInputs)[index].c_str()));
               else printError3 ("converged",lineNumber);

               if (++index < csvInputs->size() && is_double(&(*csvInputs)[index])) result->set_final_error(atof((*csvInputs)[index].c_str()));
               else printError3 ("final_error",lineNumber);

               if (++index < csvInputs->size() && is_double(&(*csvInputs)[index])) result->set_elapsed_time(atof((*csvInputs)[index].c_str()));
               else printError3 ("elapsed_time",lineNumber);

               if (++index < csvInputs->size() && is_int(&(*csvInputs)[index])) result->set_modalImpedanceCalculation(atoi((*csvInputs)[index].c_str()));
               else printError3 ("modalImpedanceCalculation",lineNumber);

               if (++index < csvInputs->size() && is_int(&(*csvInputs)[index])) result->set_modeCount(atoi((*csvInputs)[index].c_str()));
               else printError3 ("modeCount",lineNumber);

               long unsigned int i=0;
               while (i < result->get_modeCount()) {

                  // gamma
                  double real_gamma=0;
                  double imag_gamma=0;
                  if (!(++index < csvInputs->size()) || processInputNumber((*csvInputs)[index],&real_gamma)) printError3 ("real_gamma",lineNumber);
                  if (!(++index < csvInputs->size()) || processInputNumber((*csvInputs)[index],&imag_gamma)) printError3 ("imag_gamma",lineNumber);
                  result->push_gamma(complex<double>(real_gamma,imag_gamma));

                  // impedance
                  double real_Z=0;
                  double imag_Z=0;
                  if (!(++index < csvInputs->size()) || processInputNumber((*csvInputs)[index],&real_Z)) printError3 ("real_Z",lineNumber);
                  if (!(++index < csvInputs->size()) || processInputNumber((*csvInputs)[index],&imag_Z)) printError3 ("imag_Z",lineNumber);
                  result->push_impedance(complex<double>(real_Z,imag_Z));

                  // voltage
                  double real_V=0;
                  double imag_V=0;
                  if (!(++index < csvInputs->size()) || processInputNumber((*csvInputs)[index],&real_V)) printError3 ("real_V",lineNumber);
                  if (!(++index < csvInputs->size()) || processInputNumber((*csvInputs)[index],&imag_V)) printError3 ("imag_V",lineNumber);
                  result->push_voltage(complex<double>(real_V,imag_V));

                  // current 
                  double real_I=0;
                  double imag_I=0;
                  if (!(++index < csvInputs->size()) || processInputNumber((*csvInputs)[index],&real_I)) printError3 ("real_I",lineNumber);
                  if (!(++index < csvInputs->size()) || processInputNumber((*csvInputs)[index],&imag_I)) printError3 ("imag_I",lineNumber);
                  result->push_current(complex<double>(real_I,imag_I));

                  // Pz 
                  double real_Pz=0;
                  double imag_Pz=0;
                  if (!(++index < csvInputs->size()) || processInputNumber((*csvInputs)[index],&real_Pz)) printError3 ("real_Pz",lineNumber);
                  if (!(++index < csvInputs->size()) || processInputNumber((*csvInputs)[index],&imag_Pz)) printError3 ("imag_Pz",lineNumber);
                  result->push_Pz(complex<double>(real_Pz,imag_Pz));

                  i++;
               }

               resultList.push_back(result);
               delete csvInputs;
            }
         }
      }
      inFile.close();
   } else {
      cout << "ERROR2224: Unable to open file \"" << filename << "\" for reading." << endl;
      return true;
   }

   return fail;
}

ResultDatabase::~ResultDatabase()
{
   long unsigned int i=0;
   while (i < resultList.size()) {
      delete resultList[i];
      i++;
   }
}

//-------------------------------------------------------------------------------------------------------------------------------------
// EMfield
//-------------------------------------------------------------------------------------------------------------------------------------

bool EMfield::get_EMfield (string component, double frequency_, unsigned long int mode_, double x_, double y_, double *value)
{
   *value=DBL_MAX;
   if (fabs((frequency-frequency_)/frequency) > 1e-14) return false;
   if (mode_ !=  mode) return false;
   if (fabs((x-x_)/x) > 1e-14) return false;
   if (fabs((y-y_)/y) > 1e-14) return false;

   if (component.compare("Ex_re") == 0) *value=real(Ex);
   else if (component.compare("Ex_im") == 0) *value=imag(Ex);
   else if (component.compare("Ey_re") == 0) *value=real(Ey);
   else if (component.compare("Ey_im") == 0) *value=imag(Ey);
   else if (component.compare("Ez_re") == 0) *value=real(Ez);
   else if (component.compare("Ez_im") == 0) *value=imag(Ez);
   else if (component.compare("Hx_re") == 0) *value=real(Hx);
   else if (component.compare("Hx_im") == 0) *value=imag(Hx);
   else if (component.compare("Hy_re") == 0) *value=real(Hy);
   else if (component.compare("Hy_im") == 0) *value=imag(Hy);
   else if (component.compare("Hz_re") == 0) *value=real(Hz);
   else if (component.compare("Hz_im") == 0) *value=imag(Hz);
   else if (component.compare("real_Emag") == 0) *value=sqrt(pow(real(Ex),2)+pow(real(Ey),2)+pow(real(Ez),2));
   else if (component.compare("real_Ephi") == 0) *value=atan2(real(Ey),real(Ex));
   else if (component.compare("real_Etheta") == 0) *value=atan2(sqrt(pow(real(Ex),2)+pow(real(Ey),2)),real(Ez));
   else if (component.compare("imag_Emag") == 0) *value=sqrt(pow(imag(Ex),2)+pow(imag(Ey),2)+pow(imag(Ez),2));
   else if (component.compare("imag_Ephi") == 0) *value=atan2(imag(Ey),imag(Ex));
   else if (component.compare("imag_Etheta") == 0) *value=atan2(sqrt(pow(imag(Ex),2)+pow(imag(Ey),2)),imag(Ez));
   else if (component.compare("real_Hmag") == 0) *value=sqrt(pow(real(Hx),2)+pow(real(Hy),2)+pow(real(Hz),2));
   else if (component.compare("real_Hphi") == 0) *value=atan2(real(Hy),real(Hx));
   else if (component.compare("real_Htheta") == 0) *value=atan2(sqrt(pow(real(Hx),2)+pow(real(Hy),2)),real(Hz));
   else if (component.compare("imag_Hmag") == 0) *value=sqrt(pow(imag(Hx),2)+pow(imag(Hy),2)+pow(imag(Hz),2));
   else if (component.compare("imag_Hphi") == 0) *value=atan2(imag(Hy),imag(Hx));
   else if (component.compare("imag_Htheta") == 0) *value=atan2(sqrt(pow(imag(Hx),2)+pow(imag(Hy),2)),imag(Hz));
   else {cout << "ASSERT: EMfield::getEMfield" << endl; return false;}

   return true;
}

void EMfield::print()
{
   cout << frequency << ",";
   cout << mode << ",";
   cout << x << ",";
   cout << y << ",";
   cout << setprecision(15) << real(Ex) << ",";
   cout << setprecision(15) << imag(Ex) << ",";
   cout << setprecision(15) << real(Ey) << ",";
   cout << setprecision(15) << imag(Ey) << ",";
   cout << setprecision(15) << real(Ez) << ",";
   cout << setprecision(15) << imag(Ez) << ",";
   cout << setprecision(15) << real(Hx) << ",";
   cout << setprecision(15) << imag(Hx) << ",";
   cout << setprecision(15) << real(Hy) << ",";
   cout << setprecision(15) << imag(Hy) << ",";
   cout << setprecision(15) << real(Hz) << ",";
   cout << setprecision(15) << imag(Hz) << endl;
}

//-------------------------------------------------------------------------------------------------------------------------------------
// EMfieldDatabase
//-------------------------------------------------------------------------------------------------------------------------------------

void EMfieldDatabase::print()
{
   cout << "# EMfieldDatabase::print" << endl;
   unsigned long int i=0;
   while (i < EMfieldList.size()) {
      EMfieldList[i]->print();
      i++;
   }
}

bool EMfieldDatabase::get_EMfield (string component, double frequency, unsigned long int mode, double x, double y, double *value)
{
   unsigned long int i=0;
   while (i < EMfieldList.size()) {
      if (EMfieldList[i]->get_EMfield(component,frequency,mode,x,y,value)) return true;
      i++;
   }
   return false;
}


// return false on good load
// return true on bad load
bool EMfieldDatabase::loadEMfields (const char *filename)
{
   bool fail=false;
   int lineNumber=0;
   string line;
   ifstream inFile;
   inFile.open(filename,ifstream::in);

   processingFileName=filename;
   routine="EMfieldDatabase::loadEMfields";

   double realField;
   double imagField;
   realField=0;
   imagField=0;

   if (inFile.is_open()) {

      while (getline(inFile,line)) {
         lineNumber++;

         // skip blank lines
         if (line.compare("") != 0) {

            // skip comment lines
            if (! is_hashComment(line)) {

               // chop off comments
               line=line.substr(0,line.find("//",0));

               EMfield *emfield=new EMfield;

               // parse csv
               stringstream sstream(line);
               string entry;
               int count=0;
               while (std::getline(sstream, entry, ',')) {

                  if (count == 0) {
                     if (is_double(&entry)) emfield->set_frequency(atof(entry.c_str()));
                     else {printError(lineNumber,count+1); fail=true;}
                  } else if (count == 1) {
                     if (is_int(&entry)) emfield->set_mode(stoi(entry));
                     else {printError(lineNumber,count+1); fail=true;}
                  } else if (count == 2) { 
                     if (is_double(&entry)) emfield->set_x(atof(entry.c_str()));
                     else {printError(lineNumber,count+1); fail=true;}
                  } else if (count == 3) {
                     if (is_double(&entry)) emfield->set_y(atof(entry.c_str()));
                     else {printError(lineNumber,count+1); fail=true;}

                  } else if (count == 4) {
                     if (is_double(&entry)) realField=atof(entry.c_str());
                     else {printError(lineNumber,count+1); fail=true;}
                  } else if (count == 5) {
                     if (is_double(&entry)) {
                        imagField=atof(entry.c_str());
                        emfield->set_Ex(complex<double>(realField,imagField));
                     } else {printError(lineNumber,count+1); fail=true;}

                  } else if (count == 6) {
                     if (is_double(&entry)) realField=atof(entry.c_str());
                     else {printError(lineNumber,count+1); fail=true;}
                  } else if (count == 7) {
                     if (is_double(&entry)) {
                        imagField=atof(entry.c_str());
                        emfield->set_Ey(complex<double>(realField,imagField));
                     } else {printError(lineNumber,count+1); fail=true;}

                  } else if (count == 8) {
                     if (is_double(&entry)) realField=atof(entry.c_str());
                     else {printError(lineNumber,count+1); fail=true;}
                  } else if (count == 9) {
                     if (is_double(&entry)) {
                        imagField=atof(entry.c_str());
                        emfield->set_Ez(complex<double>(realField,imagField));
                     } else {printError(lineNumber,count+1); fail=true;}

                  } else if (count == 10) {
                     if (is_double(&entry)) realField=atof(entry.c_str());
                     else {printError(lineNumber,count+1); fail=true;}
                  } else if (count == 11) {
                     if (is_double(&entry)) {
                        imagField=atof(entry.c_str());
                        emfield->set_Hx(complex<double>(realField,imagField));
                     } else {printError(lineNumber,count+1); fail=true;}

                  } else if (count == 12) {
                     if (is_double(&entry)) realField=atof(entry.c_str());
                     else {printError(lineNumber,count+1); fail=true;}
                  } else if (count == 13) {
                     if (is_double(&entry)) {
                        imagField=atof(entry.c_str());
                        emfield->set_Hy(complex<double>(realField,imagField));
                     } else {printError(lineNumber,count+1); fail=true;}

                  } else if (count == 14) {
                     if (is_double(&entry)) realField=atof(entry.c_str());
                     else {printError(lineNumber,count+1); fail=true;}
                  } else if (count == 15) {
                     if (is_double(&entry)) {
                        imagField=atof(entry.c_str());
                        emfield->set_Hz(complex<double>(realField,imagField));
                     } else {printError(lineNumber,count+1); fail=true;}
                  }

                  count++;
               }
               if (count == 16) {
                  EMfieldList.push_back(emfield);
               } else {
                  delete emfield;
                  printError2(lineNumber);
                  fail=true;
               }
            }
         }
      }
      inFile.close();
   } else {
      cout << "ERROR2225: Unable to open file \"" << filename << "\" for reading." << endl;
      return true;
   }

   return fail;
}

EMfieldDatabase::~EMfieldDatabase()
{
   long unsigned int i=0;
   while (i < EMfieldList.size()) {
      delete EMfieldList[i];
      i++;
   }
}

void exit_on_fail(string filename)
{
   cout << "ERROR2226: Check the results file \"" << filename << "\"." << endl;

   ofstream out;
   out.open(filename.c_str(),ofstream::out);
   if (out.is_open()) {
      char buf[1024];
      if (getcwd(buf,1024) == NULL) cout << "ASSERT: buffer overflow in exit_on_fail." << endl;
      out << buf << ",FAIL,-1,-1,-1" << endl;
      out.close();
   } else {
      cout << "ERROR2227: Failed to open test results file \"" << filename << "\" for writing." << endl;
   }
   exit(1);
}

int main (int argc, char *argv[])
{
   struct projectData projData;
   TestCaseDatabase testCaseDatabase;
   ResultDatabase resultDatabase;
   EMfieldDatabase emfieldDatabase;
   PetscMPIInt size,rank;

   if (argc != 4 && argc != 5) {
      char buf[1024];
      if (getcwd(buf,1024) == NULL) cout << "ASSERT: buffer overflow in main." << endl;
      cout << buf << ",FAIL,-1,-1,-1" << endl;
      exit(1);
   }

   // Initialize Petsc and MPI
   PetscInitializeNoArguments();
   MPI_Comm_size(PETSC_COMM_WORLD, &size);
   MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

   // load the project file
   const char *projFile;
   projFile=argv[1];

   init_project (&projData);
   if (load_project_file (projFile, &projData, "   ")) {
      cout << "ERROR2228: Failed to load project file \"" << projFile << "\" for reading." << endl;
      exit(1);
   }
   if (projData.debug_show_project) {print_project (&projData,"      ");}

   // set up for the output
   string testResultsFile=projData.project_name;
   testResultsFile+="_test_results.csv";

   // remove the old file to prevent viewing stale data
   if (rank == 0) {
      if (std::filesystem::exists(testResultsFile.c_str())) {
        std::filesystem::remove_all(testResultsFile.c_str());
      }
   }
   MPI_Barrier(PETSC_COMM_WORLD);

   // define some file names
   string testCasesFile=argv[2];
   string simResultsFile=argv[3];
   string simFieldsFile="undefined";
   bool hasSimFields=false;
   if (argc == 5) {
      hasSimFields=true;
      simFieldsFile=argv[4];
   }

   if (testCaseDatabase.loadTestCases (testCasesFile.c_str())) exit_on_fail(testResultsFile);
   if (! testCaseDatabase.is_populated()) exit_on_fail(testResultsFile);
   //testCaseDatabase.print();

   if (resultDatabase.loadResults (simResultsFile.c_str())) exit_on_fail(testResultsFile);
   if (! resultDatabase.is_populated()) exit_on_fail(testResultsFile);
   //resultDatabase.print();

   if (hasSimFields && emfieldDatabase.loadEMfields(simFieldsFile.c_str())) exit_on_fail(testResultsFile);
   if (hasSimFields && ! emfieldDatabase.is_populated()) exit_on_fail(testResultsFile);
   //emfieldDatabase.print();

   testCaseDatabase.evaluate(&resultDatabase,&emfieldDatabase);

   if (projData.test_show_detailed_cases) testCaseDatabase.printAllFormatted();
   if (projData.test_show_audit) {
      testCaseDatabase.audit("result","equal");
      testCaseDatabase.audit("result","lessthan");
      testCaseDatabase.audit("field","equal");
      testCaseDatabase.audit("field","lessthan");
   }

   testCaseDatabase.sort(true);

   ofstream out;
   out.open(testResultsFile.c_str(),ofstream::out);
   if (out.is_open()) {
      testCaseDatabase.show_evaluation(&out);
      out.close();
   } else {
      cout << "ERROR2229: Failed to test results file \"" << testResultsFile << "\" for writing." << endl;
      exit_on_fail(testResultsFile);
   }

   PetscFinalize();

   if (!testCaseDatabase.all_evaluated()) return 1;
   if (!testCaseDatabase.all_passed()) return 2;
   return 0;
}


