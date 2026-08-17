///////////////////////////////////////////////////////////////////////////////////
//                                                                               //
//    process3D - processor for automation of regression testing of OpenParEM3D  //
//    Copyright (C) 2026 Brian Young                                             //
//                                                                               //
//    This program is free software: you can redistribute it and/or modify       //
//    it under the terms of the GNU General Public License as published by       //
//    the Free Software Foundation, either version 3 of the License, or          //
//    (at your option) any later version.                                        //
//                                                                               //
//    This program is distributed in the hope that it will be useful,            //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of             //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              //
//    GNU General Public License for more details.                               //
//                                                                               //
//    You should have received a copy of the GNU General Public License          //
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.      //
//                                                                               //
///////////////////////////////////////////////////////////////////////////////////

#include "process3D.hpp"

string processingFileName;
string routine;

void printERROR (int lineNumber, int place)
{
   cout << "|ERROR3120: " << processingFileName << ": " << routine << ": Incorrect formatting in line " << lineNumber << " at place " << place << "." << endl;
}

void printERROR2 (int lineNumber)
{
   cout << "|ERROR3121: " << processingFileName << ": " << routine << ": Incorrect number of entries at line " << lineNumber << "." << endl;
}

bool is_valid_testVariable (string test, bool type)
{
   if (type) {
      if (test.compare("gain") == 0) return true;
      if (test.compare("directivity") == 0) return true;
      if (test.compare("radiationEfficiency") == 0) return true;
   } else {
      if (test.compare("magS(dB)") == 0) return true;
      if (test.compare("argS(deg)") == 0) return true;
   }
   return false;
}

bool is_valid_testFunction (string test)
{
   if (test.compare("equal") == 0) return true;
   if (test.compare("lessthan") == 0) return true;
   if (test.compare("greaterthan") == 0) return true;
   return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------
// TestCase
//-------------------------------------------------------------------------------------------------------------------------------------

void TestCase::print_as_testcase ()
{
   cout << name << ",";
   cout << setprecision(15) << frequency << ",";
   cout << testVariable << ",";
   if (!type) cout << portOut << ",";
   cout << portIn << ",";
   cout << testFunction << ",";
   if (testFunction.compare("equal") == 0) {
      cout << expectedValue << ",";
      cout << tolerance << endl;
   } else if (testFunction.compare("lessthan") == 0) {
      cout << threshold << endl;
   } else if (testFunction.compare("greaterthan") == 0) {
      cout << threshold << endl;
   } else {
      cout << "|ERROR" << endl;
   }
}

void TestCase::printAllFormatted ()
{
   cout << "name=" << name << endl;
   cout << "   frequency=" << frequency << endl;
   cout << "   testVariable=" << testVariable << endl;
   if (!type) cout << "   portOut=" << portOut << endl;
   cout << "   portIn=" << portIn << endl;
   cout << "   testFunction=" << testFunction << endl;
   cout << "   foundValue=" << foundValue << endl;
   if (testFunction.compare("equal") == 0) {
      cout << "   error=" << error << endl;
      cout << "   expectedValue=" << expectedValue << endl;
      cout << "   tolerance=" << tolerance << endl;
   }
   if (testFunction.compare("lessthan") == 0) {
      cout << "   threshold=" << threshold << endl;
   }
   if (testFunction.compare("greaterthan") == 0) {
      cout << "   threshold=" << threshold << endl;
   }
   cout << "   passed=" << passed << endl;
   cout << "   evaluated=" << evaluated << endl;
}

void TestCase::evaluate (ResultDatabase *testResultDatabase, PatternDatabase *testPatternDatabase)
{
   double error1,error2,error3;

   passed=false;
   evaluated=false;

   Result *result=testResultDatabase->get_Result(frequency);
   if (result) {
      if (testVariable.compare("magS(dB)") == 0) {
         foundValue=real(result->get_Sij(portIn-1,portOut-1));

         if (testFunction.compare("equal") == 0) {
            if (expectedValue == 0) error=abs(foundValue);
            else error=abs((foundValue-expectedValue)/expectedValue);
            if (error <= tolerance) passed=true;
         }

         if (testFunction.compare("lessthan") == 0) {
            if (foundValue <= threshold) passed=true;
         }

         if (testFunction.compare("greaterthan") == 0) {
            if (foundValue >= threshold) passed=true;
         }
      }

      if (testVariable.compare("argS(deg)") == 0) {
         foundValue=imag(result->get_Sij(portIn-1,portOut-1));

         if (testFunction.compare("equal") == 0) {
            if (expectedValue == 0) error=abs(foundValue);
            else {
               error1=abs((foundValue-expectedValue)/expectedValue);
               error2=abs((foundValue+360-expectedValue)/expectedValue);
               error3=abs((foundValue-360-expectedValue)/expectedValue);
               error=min(min(error1,error2),error3);
            }
            if (error <= tolerance) passed=true;
         }

         if (testFunction.compare("lessthan") == 0) {
            while (foundValue >= 180) foundValue-=360;
            while (foundValue <=-180) foundValue+=360;
            if (foundValue <= threshold) passed=true;
         }

         if (testFunction.compare("greaterthan") == 0) {
            while (foundValue >= 180) foundValue-=360;
            while (foundValue <=-180) foundValue+=360;
            if (foundValue >= threshold) passed=true;
         }
      }
   }

   Pattern *pattern=testPatternDatabase->get_Pattern(frequency,portIn);
   if (pattern) {
      if (testVariable.compare("gain") == 0) {
         foundValue=pattern->get_gain();

         if (testFunction.compare("equal") == 0) {
            if (expectedValue == 0) error=abs(foundValue);
            else error=abs((foundValue-expectedValue)/expectedValue);
            if (error <= tolerance) passed=true;
         }

         if (testFunction.compare("lessthan") == 0) {
            if (foundValue <= threshold) passed=true;
         }

         if (testFunction.compare("greaterthan") == 0) {
            if (foundValue >= threshold) passed=true;
         }
      }

      if (testVariable.compare("directivity") == 0) {
         foundValue=pattern->get_directivity();

         if (testFunction.compare("equal") == 0) {
            if (expectedValue == 0) error=abs(foundValue);
            else error=abs((foundValue-expectedValue)/expectedValue);
            if (error <= tolerance) passed=true;
         }

         if (testFunction.compare("lessthan") == 0) {
            if (foundValue <= threshold) passed=true;
         }

         if (testFunction.compare("greaterthan") == 0) {
            if (foundValue >= threshold) passed=true;
         }
      }

      if (testVariable.compare("radiationEfficiency") == 0) {
         foundValue=pattern->get_radiationEfficiency();

         if (testFunction.compare("equal") == 0) {
            if (expectedValue == 0) error=abs(foundValue);
            else error=abs((foundValue-expectedValue)/expectedValue);
            if (error <= tolerance) passed=true;
         }

         if (testFunction.compare("lessthan") == 0) {
            if (foundValue <= threshold) passed=true;
         }

         if (testFunction.compare("greaterthan") == 0) {
            if (foundValue >= threshold) passed=true;
         }
      }
   }

   evaluated=true;
}

double TestCase::get_error_or_tolerance()
{
   if (testFunction.compare("equal") == 0) return error;
   if (testFunction.compare("lessthan") == 0) return foundValue;
   if (testFunction.compare("greaterthan") == 0) return foundValue;
   return -1;  // should not happen
}

void TestCase::show_evaluation(ostream *out)
{
   if (evaluated) {
      *out << name << ",";
      if (passed) *out << "pass" << ",";
      else *out << "FAIL" << ",";
      *out << setprecision(15) << frequency << ",";
      if (!type) *out << portOut << ",";
      *out << portIn << ",";
      *out << testVariable << ",";
      *out << testFunction << ",";
      if (testFunction.compare("equal") == 0) *out << setprecision(15) << expectedValue << ",";
      *out << setprecision(15) << foundValue << ",";
      if (testFunction.compare("equal") == 0) {
         *out << setprecision(15) << error << ",";
         *out << tolerance << ",";
      } else if (testFunction.compare("lessthan") == 0) {
         *out << threshold << ",";
      } else if (testFunction.compare("greaterthan") == 0) {
         *out << threshold << ",";
      }
      *out << endl;
   }
}

//-------------------------------------------------------------------------------------------------------------------------------------
// TestCaseDatabase
//-------------------------------------------------------------------------------------------------------------------------------------

bool TestCaseDatabase::load (const char *filename)
{
   bool fail=false;
   bool type=false;
   int lineNumber=0;
   string line;
   ifstream inFile;
   inFile.open(filename,ifstream::in);

   processingFileName=filename;
   routine="TestCaseDatabase::load";

   if (inFile.is_open()) {

      while (getline(inFile,line)) {
         lineNumber++;

         // skip blank lines
         if (line.compare("") != 0) {

            // determine the type: S-parameters or antenna metrics
            if (line.compare("# ResultDatabase::save_as_test") == 0) type=false;  // S-parameters
            if (line.compare("# PatternDatabase::save_as_test") == 0) type=true;  // antenna metrics

            // skip comment lines
            if (! is_hashComment(line)) {

               // chop off comments
               line=line.substr(0,line.find("//",0));

               TestCase *testCase=new TestCase;
               testCase->set_type(type);

               // parse csv
               stringstream sstream(line);
               string entry;
               int count=0;
               while (std::getline(sstream, entry, ',')) {
                  if (count == 0) {
                     testCase->set_name(entry);
                  } else if (count == 1) {
                     if (is_double(&entry)) testCase->set_frequency(atof(entry.c_str()));
                     else {printERROR(lineNumber,count+1); fail=true;}
                  } else if (count == 2) {
                     if (is_valid_testVariable(entry,type)) testCase->set_testVariable(entry);
                     else {printERROR(lineNumber,count+1); fail=true;}
                  } else if (count == 3) {
                     if (is_int(&entry)) {
                        if (type) {
                           testCase->set_portIn(stoi(entry));
                           count++;
                        } else testCase->set_portOut(stoi(entry));
                     } else {printERROR(lineNumber,count+1); fail=true;}
                  } else if (count == 4) {
                     if (is_int(&entry)) testCase->set_portIn(stoi(entry));
                     else {printERROR(lineNumber,count+1); fail=true;}
                  } else if (count == 5) {
                     if (is_valid_testFunction(entry)) testCase->set_testFunction(entry);
                     else {printERROR(lineNumber,count+1); fail=true;}
                  } else if (count == 6) {
                     if (testCase->get_testFunction().compare("equal") == 0) {
                        if (is_double(&entry)) testCase->set_expectedValue(atof(entry.c_str()));
                        else {printERROR(lineNumber,count+1); fail=true;}
                     } else if (testCase->get_testFunction().compare("lessthan") == 0) {
                        if (is_double(&entry)) testCase->set_threshold(atof(entry.c_str()));
                        else {printERROR(lineNumber,count+1); fail=true;}
                     } else if (testCase->get_testFunction().compare("greaterthan") == 0) {
                        if (is_double(&entry)) testCase->set_threshold(atof(entry.c_str()));
                        else {printERROR(lineNumber,count+1); fail=true;}
                     } else {printERROR(lineNumber,count+1); fail=true;}
                  } else if (count == 7) {
                     if (testCase->get_testFunction().compare("equal") == 0) {
                        if (is_double(&entry)) testCase->set_tolerance(atof(entry.c_str()));
                        else {printERROR(lineNumber,count+1); fail=true;}
                     }
                  }

                  count++;
               }

               bool good=false;
               if (testCase->get_testFunction().compare("equal") == 0 && count == 8) good=true;
               if (testCase->get_testFunction().compare("lessthan") == 0 && count == 7) good=true;
               if (testCase->get_testFunction().compare("greaterthan") == 0 && count == 7) good=true;

               if (good) {
                  testCase->set_passed(false);
                  testCase->set_evaluated(false);
                  testCaseList.push_back(testCase);
               } else {
                  delete testCase;
                  printERROR2(lineNumber);
                  fail=true;
               }
            }
         }
      }
      inFile.close();
   } else {
      cout << "|ERROR3122: Unable to open file \"" << filename << "\" for reading." << endl;
      return true;
   }

   return fail;
}

void TestCaseDatabase::evaluate (ResultDatabase *testResultDatabase, PatternDatabase *testPatternDatabase)
{
   long unsigned int i=0;
   while (i < testCaseList.size()) {
      testCaseList[i]->evaluate(testResultDatabase,testPatternDatabase);
      i++;
   }
}

void TestCaseDatabase::print_as_testcase ()
{
   cout << "# TestCaseDatabase::print_as_testcase" << endl;
   unsigned long int i=0;
   while (i < testCaseList.size()) {
      testCaseList[i]->print_as_testcase();
      i++;
   }
}

void TestCaseDatabase::printAllFormatted()
{
   unsigned long int i=0;
   while (i < testCaseList.size()) {
      testCaseList[i]->printAllFormatted();
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

void TestCaseDatabase::show_evaluation(ostream *out)
{
   *out << "# name,status,frequency,portOut,portIn,";
   *out << "testVariable,testFunction,[equal:expectedValue,]foundValue,";
   *out << "[equal:error,tolerance | lessthan:threshold | greaterthan:threshold" << endl;

   unsigned long int i=0;
   while (i < testCaseList.size()) {
      testCaseList[testCaseList[i]->get_index()]->show_evaluation(out);
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


//-------------------------------------------------------------------------------------------------------------------------------------
// main
//-------------------------------------------------------------------------------------------------------------------------------------

void exit_on_fail(string filename)
{
   cout << "|ERROR3123: Check the results file \"" << filename << "\"." << endl;

   ofstream out;
   out.open(filename.c_str(),ofstream::out);
   if (out.is_open()) {
      char buf[1024];
      if (getcwd(buf,1024) == NULL) cout << "ASSERT: buffer overflow in exit_on_fail." << endl;
      out << buf << ",FAIL,-1,-1,-1" << endl;
      out.close();
   } else {
      cout << "|ERROR3124: Failed to open test results file \"" << filename << "\" for writing." << endl;
   }
   exit(1);
}

// usage: process3D projectFile testCaseFile SresultsFile [FFresultsFile]
// FFresultsFile only applies for antenna problems
int main (int argc, char *argv[])
{
   struct projectData projData;
   TestCaseDatabase testCaseDatabase;
   ResultDatabase testResultDatabase;    // S-parameters;    does not use all data elements
   PatternDatabase testPatternDatabase;  // antenna metrics; does not use all data elements
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
      cout << "|ERROR3125: Failed to load project file \"" << projFile << "\"." << endl;
      exit(1);
   }
   if (projData.debug_show_project) {print_project (&projData,"      ");}

   // set up for the output
   string testResultsFile=projData.project_name;
   testResultsFile+="_test_results.csv";

   // remove the old file to prevent viewing stale data
   if (std::filesystem::exists(testResultsFile.c_str())) {
     std::filesystem::remove_all(testResultsFile.c_str());
   }

   // load test cases
   string testCasesFile=argv[2];
   if (testCaseDatabase.load (testCasesFile.c_str())) exit_on_fail(testResultsFile);
   //testCaseDatabase.print_as_testcase();

   // load S-parameters
   string SresultsFile=argv[3];
   if (testResultDatabase.loadCSV(SresultsFile.c_str())) exit_on_fail(testResultsFile);

   // load antenna metrics
   string FFresultsFile="";
   if (argc == 5) {
      string FFresultsFile=argv[4];
      testPatternDatabase.loadCSV(FFresultsFile.c_str(),&projData);
      //testPatternDatabase.print(0);
   }

   // process test cases
   testCaseDatabase.evaluate(&testResultDatabase,&testPatternDatabase);
   if (projData.test_show_detailed_cases) testCaseDatabase.printAllFormatted();
   testCaseDatabase.sort(true);

   ofstream out;
   out.open(testResultsFile.c_str(),ofstream::out);
   if (out.is_open()) {
      testCaseDatabase.show_evaluation(&out);
      out.close();
   } else {
      cout << "|ERROR3126: Failed to test results file \"" << testResultsFile << "\" for writing." << endl;
      exit_on_fail(testResultsFile);
   }

   //ToDo: fix the MPI error when this is un-commented
   //PetscFinalize();

   if (!testCaseDatabase.all_evaluated()) return 1;
   if (!testCaseDatabase.all_passed()) return 2;
   return 0;
}

