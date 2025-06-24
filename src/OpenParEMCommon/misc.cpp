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

#include "misc.hpp"

bool is_comment (string a) {
   long unsigned int i;

   // blank line
   if (a.compare("") == 0) return false;

   // first /
   i=0;
   while (i < a.length()) {
      if ((a.data())[i] == ' ' || (a.data())[i] == '\t') {
         // do nothing
      } else {
         if ((a.data())[i] == '/') {i++; break;}
         else return false;
      }
      i++;
   }

   // second /
   if (i < a.length()) {
      if ((a.data())[i] == '/') return true;
   }

   return false;
}

bool is_hashComment (string a) {
   long unsigned int i;

   // blank line
   if (a.compare("") == 0) return false;

   i=0;
   while (i < a.length()) {
      if ((a.data())[i] == ' ' || (a.data())[i] == '\t') {
         // do nothing
      } else {
         if ((a.data())[i] == '#') return true;
      }
      i++;
   }

   return false;
}

void split_on_space (vector<string> *tokens, string a) {
   string buf;
   stringstream ss(a);

   tokens->clear();
   while (ss >> buf) tokens->push_back(buf);
}

bool double_compare (double a, double b, double tol)
{
   if (a == b) return true;
   if (a == 0 && fabs(b) < tol) return true;
   if (b == 0 && fabs(a) < tol) return true;
   if (fabs(a) < tol && fabs(b) < tol) return true;
   if (fabs((a-b)/a) < tol) return true;
   return false;
}

bool complex_compare (complex<double> a, complex<double> b, double tol)
{
   if (a == b) return true;
   if (a == 0 && abs(b) < tol) return true;
   if (b == 0 && abs(a) < tol) return true;
   if (abs(a) < tol && abs(b) < tol) return true;
   if (abs((a-b)/a) < tol) return true;
   return false;
}

double relative_error (double a, double b)
{
   if (a == b) return 0;
   if (b == 0) return abs((a-b)/a);
   return abs((a-b)/b);
}

bool is_double (const char *b) {
   bool foundPeriod=false;
   bool foundE=false;
   bool foundChar=false;
   size_t length;

   // skip trailing white space
   size_t i=strlen(b)-1;
   while (i >= 0) {
      if (b[i] != ' ' && b[i] != '\t') break;
      i--;
   }
   length=i+1;

   // skip leading whitespace
   i=0;
   while (i < length) {
      if (b[i] != ' ' && b[i] != '\t') break;
      i++;
   }

   // skip leading sign
   if (b[i] == '+' || b[i] == '-') i++;

   while (i < length) {
      if (b[i] == '.') {
         if (foundPeriod) return false;
         foundPeriod=true;
      }
      else if (b[i] == ' ' || b[i] == '\t') return false;
      else if (i > 0 && !foundE && (b[i] == '+' || b[i] == '-')) return false;
      else if (isalpha(b[i])) {
         foundChar=true;
         if (b[i] == 'e' || b[i] == 'E') {
            if (foundE) return false;
            foundE=true;

            if (i < length-1 && isdigit(b[i+1])) {foundChar=false; i++;}
            if (i < length-2 && b[i+1] == '-' && isdigit(b[i+2])) {foundChar=false; i+=2;}
            if (i < length-2 && b[i+1] == '+' && isdigit(b[i+2])) {foundChar=false; i+=2;}
         }
      }
      if (foundChar) {return false;}
      i++;
   }
   return true;
}

bool is_double (string *a) {
   return is_double((*a).c_str());
}

bool is_complex (complex<double> a)
{
   if (real(a) != DBL_MAX) return true;
   if (imag(a) != DBL_MAX) return true;
   return false;
}

bool is_int (string *a) {
   const char *b=(*a).c_str();

   // skip leading whitespace
   size_t i=0;
   while (i < strlen(b)) {
      if (b[i] != ' ' && b[i] != '\t') break;
      i++;
   }

   while (i < strlen(b)) {
      if (! isdigit(b[i])) return false;
      i++;
   }
   return true;
}

// dim=2 for 2D and dim=3 for 3D
bool is_point (string *a, int dim)
{
   const char *b=(*a).c_str();

   // count (, ), and comma
   size_t countOpen=0;
   size_t countClose=0;
   size_t countComma=0;
   size_t i=0;
   while (i < strlen(b)) {
      if (b[i] == '(') countOpen++;
      if (b[i] == ')') countClose++;
      if (b[i] == ',') countComma++;
      i++;
   }

   if (countOpen != 1) return false;
   if (countClose != 1) return false;
   if (dim == 2 && countComma != 1) return false;
   if (dim == 3 && countComma != 2) return false;

   // check for leading characters
   i=0;
   while (i < strlen(b)) {
      if (b[i] != ' ' && b[i] != '\t') {
         if (b[i] == '(') break;
         return false;
      }
      i++;
   }

   // find the numbers
   size_t allocSize=256;
   char x[allocSize],y[allocSize],z[allocSize];
   size_t size_x=0, size_y=0, size_z=0;
   bool foundOpen=false;
   bool foundComma=false;
   bool found2ndComma=false;
   while (i < strlen(b)) {
      if (b[i] == '(' || b[i] == ',' || b[i] == ')') {
         if (b[i] == '(') foundOpen=true;
         if (b[i] == ',') {
            if (!foundComma) foundComma=true;
            else found2ndComma=true;
         }
         if (b[i] == ')') break;
      } else {
         if (foundOpen && ! foundComma) {
            x[size_x]=b[i];
            size_x++;
            if (size_x == allocSize) return false;
         }
         if (foundOpen && foundComma) {
            if (!found2ndComma) {
               y[size_y]=b[i];
               size_y++;
               if (size_y == allocSize) return false;
            } else {
               z[size_z]=b[i];
               size_z++;
               if (size_z == allocSize) return false;
            }
         }
      }
      i++;
   }

   // complete the strings
   x[size_x]='\0';
   y[size_y]='\0';
   if (dim == 3) z[size_z]='\0';

   // check for trailing characters
   i++;
   while (i < strlen(b)) {
      if (b[i] != ' ' && b[i] != '\t') return false;
      i++;
   }

   if (! is_double(x)) return false;
   if (! is_double(y)) return false;
   if (dim == 3 && ! is_double(z)) return false;

   return true;
}

bool point_get (string *a, double *x_value, double *y_value, double *z_value, int dim, string indent, int lineNumber_)
{
   const char *b=(*a).c_str();

   size_t allocSize=256;
   char x[allocSize],y[allocSize],z[allocSize];
   size_t size_x=0, size_y=0, size_z=0;
   bool foundOpen=false;
   bool foundComma=false;
   bool found2ndComma=false;
   size_t i=0;
   while (i < strlen(b)) {
      if (b[i] == '(' || b[i] == ',' || b[i] == ')') {
         if (b[i] == '(') foundOpen=true;
         if (b[i] == ',') {
            if (!foundComma) foundComma=true;
            else found2ndComma=true;
         }
         if (b[i] == ')') break;
      } else {
         if (foundOpen && ! foundComma) {
            x[size_x]=b[i];
            size_x++;
            if (size_x == allocSize) return false;
         }
         if (foundOpen && foundComma) {
            if (!found2ndComma) {
               y[size_y]=b[i];
               size_y++;
               if (size_y == allocSize) return false;
            } else {
               z[size_z]=b[i];
               size_z++;
               if (size_z == allocSize) return false;
            }
         }
      }
      i++;
   }

   // complete the strings
   x[size_x]='\0';
   y[size_y]='\0';
   if (dim == 3) z[size_z]='\0';

   // convert to strings to use stod
   string x_str=x;
   string y_str=y;
   string z_str;
   if (dim == 3) z_str=z;

   // get the values
   try {*x_value=stod(x_str);}
   catch (const std::invalid_argument& ia) {
#if HAS_MPI
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1000: %s value at line %d is invalid.\n",indent.c_str(),indent.c_str(),a->c_str(),lineNumber_);
#else
      printf("%s%sERROR1000: %s value at line %d is invalid.\n",indent.c_str(),indent.c_str(),a->c_str(),lineNumber_);
#endif
      return true;
   }

   try {*y_value=stod(y_str);}
   catch (const std::invalid_argument& ia) {
#if HAS_MPI
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1001: %s value at line %d is invalid.\n",indent.c_str(),indent.c_str(),a->c_str(),lineNumber_);
#else
      printf("%s%sERROR1001: %s value at line %d is invalid.\n",indent.c_str(),indent.c_str(),a->c_str(),lineNumber_);
#endif
      return true;
   }

   if (dim == 3) {
      try {*z_value=stod(z_str);}
      catch (const std::invalid_argument& ia) {
#if HAS_MPI
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1002: %s value at line %d is invalid.\n",indent.c_str(),indent.c_str(),a->c_str(),lineNumber_);
#else
         printf("%s%sERROR1002: %s value at line %d is invalid.\n",indent.c_str(),indent.c_str(),a->c_str(),lineNumber_);
#endif
         return true;
      }
   }

   return false;
}

void get_token_pair (string *line, string *token, string *value, int *lineNumber, string indent) {
   string test;
   int count=0;

   stringstream ss(*line);
   while (getline(ss,test,'=')) {
      if (count == 0) *token=test;
      else if (count == 1) *value=test;
      else {
#if HAS_MPI
         prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1003: Incorrectly formatted entry at line %d.\n",indent.c_str(),indent.c_str(),*lineNumber);
#else
         printf("%s%sERROR1003: Incorrectly formatted entry at line %d.\n",indent.c_str(),indent.c_str(),*lineNumber);
#endif
      }
      count++;
   }

   if (count < 2) {
#if HAS_MPI
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1004: Missing value at line %d.\n",indent.c_str(),indent.c_str(),*lineNumber);
#else
      printf("%s%sERROR1004: Missing value at line %d.\n",indent.c_str(),indent.c_str(),*lineNumber);
#endif
   }

   // chop off white space from front and back

   size_t first=(*token).find_first_not_of(" \t");
   size_t last=(*token).find_last_not_of(" \t");
   *token=(*token).substr(first,last-first+1);

   if (value->length() > 0) {
      first=(*value).find_first_not_of(" \t");
      last=(*value).find_last_not_of(" \t");
      *value=(*value).substr(first,last-first+1);
   }

   return;
}

string processOutputNumber (double a)
{
   if (a == DBL_MAX) return "NA";
   if (a == -DBL_MAX) return "NA";

   stringstream ss;
   ss << setprecision(15) << a;
   return ss.str();
}

bool processInputNumber (string a, double *b)
{
   if (a.compare("NA") == 0) *b=DBL_MAX;
   else {
      if (is_double(&a)) *b=stod(a);
      else return true;
   }
   return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
// inputFile
///////////////////////////////////////////////////////////////////////////////////////////

// return true on fail
bool inputFile::load(const char *filename)
{
   int lineNumber=0;
   string line;

   if (strcmp(filename,"") == 0) return true;

   ifstream materialFile;
   materialFile.open(filename,ifstream::in);

   if (materialFile.is_open()) {

      while (getline(materialFile,line)) {
         lineNumber++;

         // skip blank lines
         if (line.compare("") != 0) {

            // skip comment lines
            if (! is_comment(line)) {

               // chop off comments
               line=line.substr(0,line.find("//",0));

               // chop off white space from front and back
               size_t first=line.find_first_not_of(" \t");
               size_t last=line.find_last_not_of(" \t");

               if (first <= last) {
                  line=line.substr(first,last-first+1);

                  // save the line and the line number
                  lineTextList.push_back(line);
                  lineNumberList.push_back(lineNumber);
               }
            }
         }
      }
   } else {
#if HAS_MPI
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"ERROR1005: File \"%s\" could not be opened for reading.\n",filename);
#else
      printf("ERROR1005: File \"%s\" could not be opened for reading.\n",filename);
#endif
      return true;
   }
   return false;
}

void inputFile::createCrossReference()
{
   // initialize
   long unsigned int max=lineNumberList[lineNumberList.size()-1];
   long unsigned int i=0;
   while (i <= max) {
      crossReferenceList.push_back(-1);
      i++;
   }

   // index
   i=0;
   while (i < lineNumberList.size()) {
      crossReferenceList[lineNumberList[i]]=i;
      i++;
   }
}

// return true on fail
bool inputFile::checkVersion(string name, string version)
{
   string line,token,value,test;

   if (lineTextList.size() == 0) return true;

   line=lineTextList[0];

   // chop off comments
   line=line.substr(0,line.find("//",0));

   // chop off white space from front and back
   size_t first=line.find_first_not_of(" \t");
   size_t last=line.find_last_not_of(" \t");
   line=line.substr(first,last-first+1);

   // name
   first=line.find_first_not_of(" \t");
   last=line.find_first_of(" \t");
   token=line.substr(first,last-first);

   if (token.compare(name) != 0) return true;

   // version
   first=line.find_first_of(" \t");
   last=line.find_last_not_of(" \t");
   value=line.substr(first+1,last-first+1);

   first=value.find_first_not_of(" \t");
   last=value.find_last_not_of(" \t");
   value=value.substr(first,last-first+1);

   if (value.compare(version) != 0) return true;

   return false;
}

int inputFile::get_first_lineNumber(){
   if (lineNumberList.size() > 0) return lineNumberList[0];
   return -1;
}

int inputFile::get_last_lineNumber() {
   if (lineNumberList.size() > 0) return lineNumberList[lineNumberList.size()-1];
   return -1;
}

int inputFile::get_previous_lineNumber (int lineNumber) {
   long unsigned int i=lineNumberList.size()-1;
   while (i > 0) {
      if (lineNumberList[i] == lineNumber) return lineNumberList[i-1];
      i--;
   }
   return lineNumberList[i];
}

int inputFile::get_next_lineNumber (int lineNumber) {
   long unsigned int i=0;
   while (i < lineNumberList.size()-1) {
      if (lineNumberList[i] == lineNumber) return lineNumberList[i+1];
      i++;
   }
   return lineNumberList[i];
}

string inputFile::get_line (int lineNumber) {
   return lineTextList[crossReferenceList[lineNumber]];
}

// find the starting and stopping line numbers for a block inclusive of the keywords
bool inputFile::findBlock(int search_startLine, int search_stopLine,
                         int *block_startLine, int *block_stopLine,
                         string initiator, string terminator, bool reportUnmatchedText)
{
   bool fail=false;
   *block_startLine=-1;
   *block_stopLine=-1;

   // skip to the search range
   long unsigned int i=0;
   while (i < lineNumberList.size()) {
      if (lineNumberList[i] >= search_startLine) break;
      i++;
   }

   // find the start of the block
   while (i < lineNumberList.size()) {
      if (lineNumberList[i] <= search_stopLine) {
         if (lineTextList[i].compare(initiator) == 0) {
            *block_startLine=lineNumberList[i];
            break;
         } else if (lineTextList[i].compare(terminator) == 0) {
#if HAS_MPI
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1006: \"%s\" found at line %d is missing an opening \"%s\" keyword.\n",
                                                   indent.c_str(),indent.c_str(),terminator.c_str(),lineNumberList[i],initiator.c_str());
#else
            printf("%s%sERROR1006: \"%s\" found at line %d is missing an opening \"%s\" keyword.\n",
                   indent.c_str(),indent.c_str(),terminator.c_str(),lineNumberList[i],initiator.c_str());
#endif
            *block_stopLine=lineNumberList[i];
            return true;
         } else {
            if (reportUnmatchedText) {
#if HAS_MPI
               prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1007: Invalid entry at line %d.\n",indent.c_str(),indent.c_str(),lineNumberList[i] );
#else
               printf("%s%sERROR1007: Invalid entry at line %d.\n",indent.c_str(),indent.c_str(),lineNumberList[i] );
#endif
               fail=true;
            }
         }
      }
      i++;
   }
   if (fail) return true;

   // no block found
   if (*block_startLine < 0) {
      *block_stopLine=search_stopLine;
      return false;
   }

   // find the end of the block
   i++;
   while (i < lineNumberList.size()) {
      if (lineNumberList[i] <= search_stopLine) {
         if (lineTextList[i].compare(terminator) == 0) {
            *block_stopLine=lineNumberList[i];
            break;
         }

         // check for missing terminator
         if (lineTextList[i].compare(initiator) == 0) {
#if HAS_MPI
            prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1008: \"%s\" block at line %d is incorrectly terminated at line %d.\n",
                                                   indent.c_str(),indent.c_str(),initiator.c_str(),*block_startLine,lineNumberList[i]);
#else
            printf("%s%sERROR1008: \"%s\" block at line %d is incorrectly terminated at line %d.\n",
                   indent.c_str(),indent.c_str(),initiator.c_str(),*block_startLine,lineNumberList[i]);
#endif
            *block_stopLine=get_previous_lineNumber(lineNumberList[i]);
            return true;
         }
      }
      i++;
   }

   // missing block terminator
   if (*block_stopLine < 0) {
#if HAS_MPI
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%s%sERROR1009: \"%s\" block at line %d is missing its terminator \"%s\".\n",
                                             indent.c_str(),indent.c_str(),initiator.c_str(),*block_startLine,terminator.c_str());
#else
      printf("%s%sERROR1009: \"%s\" block at line %d is missing its terminator \"%s\".\n",
             indent.c_str(),indent.c_str(),initiator.c_str(),*block_startLine,terminator.c_str());
#endif
      *block_stopLine=search_stopLine;
      return true;
   }

   return false;
}

void inputFile::print()
{
   long unsigned int i=0;
   while (i < lineTextList.size()) {
#if HAS_MPI
      prefix(); PetscPrintf(PETSC_COMM_WORLD,"%d: %s\n",lineNumberList[i],lineTextList[i].c_str());
#else
      printf("%d: %s\n",lineNumberList[i],lineTextList[i].c_str());
#endif
      i++;
   }
}

void inputFile::clear()
{
    lineTextList.clear();
    lineNumberList.clear();
    crossReferenceList.clear();
}


