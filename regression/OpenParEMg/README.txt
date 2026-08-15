Stress tests and regression using macro files.  Note that the macro language is not fully developed with all possible options,
plus it has very limited error detection and reporting.

Run the macros from within OpenParEMg.


--------------------------------------
regression.opm
--------------------------------------

The file runs the OpenParEM3D regression suite using OpenParEMg.  Check output to the terminal for pass/fail results,
waiving cases that just miss their targets.

--------------------------------------
stress_test.opm
--------------------------------------

The file stress_test.opm is for stress testing the user interface by exercising drawing functions in long loops.
A successful result is one where OpenParEMg completes the macro while memory consumption barely grows.

--------------------------------------
WR90/straight/macro_build/straight.opm
--------------------------------------

The straight.opm macro is for testing the complete building of a real simulation, running it, and checking the results.
A successful result is one where OpenParEMg completes the macro while memory consumption stays static and the check_results.sh
script produces no errors.


--------------------------------------
memory usage logging
--------------------------------------

To log memory consumtion during a long run, execute the following in a terminal while the macro is running:
top -b -d 10 | grep --line-buffered "MiB Mem" > top.log

Plot the result for used memory to evaluate memory consumption.



