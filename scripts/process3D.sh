#!/bin/sh

# first argument is the *.proj file name
# third argument is the number of processors to use

# find which MPI library is being used
HAS_OPENMPI=0
HAS_MPICH=0
which mpirun > /dev/null 2>&1
if [ $? -eq 0 ]; then
   output=$(mpirun --version 2>/dev/null)

   echo "$output" | grep -qi "Open MPI"
   if [ $? -eq 0 ]; then
      HAS_OPENMPI=1
   fi

   echo "$output" | grep -qi "OpenRTE"
   if [ $? -eq 0 ]; then
      HAS_OPENMPI=1
   fi

   echo "$output" | grep -qi "MPICH"
   if [ $? -eq 0 ]; then
      HAS_MPICH=1
   fi
fi
echo "HAS_OPENMPI="$HAS_OPENMPI
echo "HAS_MPICH="$HAS_MPICH

#--------------------------------------------------------------------------
# input processing
#--------------------------------------------------------------------------

# check for no arguments
if [ $# -eq 0 ]
then
   echo "ERROR: process3D.sh missing command line arguments"
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
   echo "ERROR: process3D.sh has too many arguments."
   exit 1
fi

# outputs to this point:
#    projectFile - project file from the command line
#    projectName - project name from the *.proj file
#    runType     - type of run to make

# delete old files to ensure no stale data if something goes wrong
rm -f $projectName"_results.csv"
rm -f $projectName"_FarField_results.csv"
rm -f $projectName"_results.log"

# run the job
if [ "$HAS_OPENMPI" = "1" ]; then
   echo "process3D.sh: mpirun -np "$numProc" --oversubscribe OpenParEM3D "$projectFile
   mpirun -np $numProc --oversubscribe OpenParEM3D $projectFile 
fi

if [ "$HAS_MPICH" = "1" ]; then
   echo "process3D.sh: mpirun -np "$numProc" OpenParEM3D "$projectFile
   mpirun -np $numProc OpenParEM3D $projectFile
fi

# check for files

if [ ! -f $projectName"_results.csv" ]
then
   echo "ERROR: File \""$projectName"_results.csv\" is missing."
   exit 1
fi

hasFarField=0
if [ -f $projectName"_FarField_results.csv" ]
then
   hasFarField=1
fi

# run the tests

if [ $hasFarField -eq 1 ]
then
   echo "process3D.sh: process3D "$projectFile" "$projectName"_test_cases.csv"" "$projectName"_results.csv"" "$projectName"_FarField_results.csv"" > "$projectName"_results.log"
   process3D $projectFile $projectName"_test_cases.csv" $projectName"_results.csv" $projectName"_FarField_results.csv" > $projectName"_results.log"
   retVal=$?

   if [ $retVal = 1 ]; then
      echo "ERROR: At least one regression test failed its error limit for project "$PWD"/"$projectFile"."
   fi

   if [ $retVal = 2 ]; then
      echo "WARNING: At least one regression test failed its error limit for project "$PWD"/"$projectFile"."
   fi
else
   echo "process3D.sh: process3D "$projectFile" "$projectName"_test_cases.csv"" "$projectName"_results.csv"" > "$projectName"_results.log"
   process3D $projectFile $projectName"_test_cases.csv" $projectName"_results.csv" > $projectName"_results.log"
   retVal=$?

   if [ $retVal = 1 ]; then
      echo "ERROR: At least one regression test failed its error limit for project "$PWD"/"$projectFile"."
   fi

   if [ $retVal = 2 ]; then
      echo "WARNING: At least one regression test failed its error limit for project "$PWD"/"$projectFile"."
   fi
fi

exit 0
