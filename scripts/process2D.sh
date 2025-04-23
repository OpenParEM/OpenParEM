#!/bin/sh

cleanup="true"
#cleanup="false"

# first argument is the *.proj file name
# third argument is the number of processors to use

#--------------------------------------------------------------------------
# input processing
#--------------------------------------------------------------------------

# check for no arguments
if [ $# -eq 0 ]
then
   echo "ERROR: process2D.sh missing command line arguments"
   exit 1
fi

# see if the provided file exists
if [ -f $1 ]
then
    echo "Processing project file \""$1"\"."
else
    echo "ERROR: File \""$1"\" does not exist."
    exit 1
fi
projectFile=$1
projectName="${projectFile%.*}"

# check the second argument
if [ $# -eq 1 ]
then
   echo "ERROR: The number of processors to use must be provided."
   exit 1
fi
numProc=$2

# check for too many arguments
if [ $# -eq 3 ]
then
   echo "ERROR: process2D.sh has too many arguments."
   exit 1
fi

# see if there are field points called out
count=$(grep -wc "field.point" $1)
if [ $count -eq 0 ]; then
   hasFields="false"
else
   hasFields="true"
fi

# outputs to this point:
#    projectFile - project file from the command line
#    projectName - project name from the *.proj file
#    runType     - type of run to make
#    hasFields   - indicates whether field points are called out in *.proj

# delete old files to ensure no stale data if something goes wrong
rm -f $projectName"_results.csv"
rm -f $projectName"_fields.csv"
rm -f $projectName"_test_cases.csv"
rm -f $projectName"_test_results.csv"
rm -f $projectName"_test_results.log"

# create a temporary project file
testProjectFile=$projectName"_test.proj"
cp $projectFile $testProjectFile

if [ ! -f $testProjectFile ]
then
   echo "ERROR: Failed to create the temporary project file \""$testProjectFile"\"."
   exit 1
fi

testProjectName="${testProjectFile%.*}"

# add test control
echo "test.create.cases              true" >> $testProjectFile
echo "test.show.audit                true" >> $testProjectFile
echo "test.show.detailed.cases       false" >> $testProjectFile

# run the job
echo "process2D.sh: mpirun -np "$numProc" OpenParEM2D "$testProjectFile
mpirun -np $numProc OpenParEM2D $testProjectFile 

# check for files

if [ ! -f $testProjectName"_results.csv" ]
then
   echo "ERROR: File \""$testProjectName"_results.csv\" is missing."
   exit 1
fi

if [ $hasFields = "true" -a ! -f $testProjectName"_fields.csv" ]
then
   echo "ERROR: File \""$testProjectName"_fields.csv\" is missing."
   exit 1
fi

hasExactCases="false"
if [ -f $projectName"_exact_test_cases.csv" ]
then
   hasExactCases="true"
fi

hasDerivedCases="false"
if [ -f $projectName"_derived_test_cases.csv" ]
then
   hasDerivedCases="true"
fi

# create the final test case file

hasCases="false"
if [ $hasExactCases = "true" ]
then
   cat $projectName"_exact_test_cases.csv" > $projectName"_test_cases.csv"
   hasCases="true"

   if [ $hasDerivedCases = "true" ]
   then
      cat $projectName"_derived_test_cases.csv" >> $projectName"_test_cases.csv"
   fi
else
   if [ $hasDerivedCases = "true" ]
   then
      cat $projectName"_derived_test_cases.csv" > $projectName"_test_cases.csv"
      hasCases="true"
   fi
fi

# run the tests

if [ $hasCases = "true" ]
then
   if [ $hasFields = "true" ]
   then
      echo "process2D.sh: process2D "$testProjectFile" "$projectName"_test_cases.csv"" "$testProjectName"_results.csv"" "$testProjectName"_fields.csv"" > "$testProjectName"_results.log"
      process2D $testProjectFile $projectName"_test_cases.csv" $testProjectName"_results.csv" $testProjectName"_fields.csv" > $testProjectName"_results.log"
      retVal=$?

      if [ $retVal = 1 ]
      then
         echo "ERROR: At least one regression test failed to evaluate for project "$PWD"/"$projectFile"."
      fi

      if [ $retVal = 2 ]; then
         echo "WARNING: At least one regression test failed its error limit for project "$PWD"/"$projectFile"."
      fi
   else
      echo "process2D.sh: process2D "$testProjectFile" "$projectName"_test_cases.csv"" "$testProjectName"_results.csv"" > "$testProjectName"_results.log"
      process2D $testProjectFile $projectName"_test_cases.csv" $testProjectName"_results.csv" > $testProjectName"_results.log"
      retVal=$?

      if [ $retVal = 1 ]
      then
         echo "ERROR: At least one regression test failed to evaluate for project "$PWD"/"$projectFile"."
      fi

      if [ $retVal = 2 ]; then
         echo "WARNING: At least one regression test failed its error limit for project "$PWD"/"$projectFile"."
      fi
   fi
else
   echo "ERROR: No test cases to run."
   exit 1
fi

# clean up
if [ $cleanup = "true" ]
then
   rm -f $testProjectFile
fi

exit 0


