bin_PROGRAMS = OpenParEM3D simplify3D process3D 
noinst_PROGRAMS = tline3

OpenParEM3D_SOURCES = fem3D.cpp OPEM_L2ZZErrorEstimator.cpp OpenParEM3D.cpp pattern.cpp port.cpp results.cpp fileCleanup.cpp project.c csr.c solveComplexLinearSystem.c fem3D.hpp OPEM_L2ZZErrorEstimator.hpp pattern.hpp port.hpp results.hpp fileCleanup.hpp project.h csr.h solveComplexLinearSystem.h

simplify3D_SOURCES = simplify3D.cpp project.c simplify3D.hpp project.h

process3D_SOURCES = process3D.cpp csr.c solveComplexLinearSystem.c port.cpp fem3D.cpp project.c results.cpp OPEM_L2ZZErrorEstimator.cpp pattern.cpp process3D.hpp csr.h solveComplexLinearSystem.h port.hpp fem3D.hpp project.h results.hpp OPEM_L2ZZErrorEstimator.hpp pattern.hpp

tline3_SOURCES = tline3.cpp

check-local:
	../../scripts/OpenParEM3D_test

