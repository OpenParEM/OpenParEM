Note: The regression tests are set up to exercise and to check setop options for OpenParEM2D, so the setups do not necessarily
      demonstrate the best way to set a up a problem.

1. check on project files

   proj_search has output.show.license | grep true
   proj_search has debug.show.mode.definitions | grep true
   proj_search has debug.show.impedance.details | grep true
   proj_search has debug.tempfiles.keep | grep true
   proj_search has project.save.fields | grep true
   proj_search has test.create.cases | grep true
   proj_search has debug.tempfiles.keep | grep true

   Generally, these should be false to minimize disk usage.

2. To set up a project:

   If derived comparisons are desired, then run with test.create.cases set to true.
   Then:
      mpirun -np OpenParEM2D *.proj
      mv *_prototype_test_cases.csv *_derived_test_cases.csv
      Edit the file *_derived_test_cases.csv as desired.
      Set test.create.cases to false

3. To check a project for errors:

   process2D.sh project.proj N

4. To check errors in all projects, from the regression directory:

   regression >& regression.log

5. To check the files for issues, run:

   ./check.sh

6. To add projects to the regression suite, add a line to

   regression_case_list.txt

7. To clean up directories after a regression run, from the regression directory:

   find -type d -exec project_cleanup.sh {} \;

   WARNING: Will remove 'regression_results.csv' and 'regression.log', so save these first.

   
