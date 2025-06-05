AC_PREREQ([2.69])
AC_INIT([OpenParEM], [2.0], [support@openparem.org])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR([OpenParEMCommon/path.cpp])
AC_CONFIG_HEADERS([OpenParEMCommon/config.h])
AC_PROG_RANLIB()
AM_PROG_AR()

# Compile with MPI
AC_DEFINE_UNQUOTED([HAS_MPI], [1], [Set to 1 for MPI])

# Checks for programs.
AC_PROG_CXX(mpicxx)
AC_PROG_CC(mpicc)
AC_PROG_INSTALL

# environment variable for the current working directory
AC_ARG_VAR(PWD,[current working directory])

# Checks for libraries.

AC_LANG([C])

AC_CHECK_LIB([c], [malloc], ,AC_MSG_ERROR([cannot link to libc; check config.log]))
AC_CHECK_LIB([m], [cos], ,AC_MSG_ERROR([cannot link to libm; check config.log]))
AC_CHECK_LIB([quadmath], [acosq], ,AC_MSG_ERROR([cannot link to libquadmath; check config.log]))
AC_CHECK_LIB([rt], [clock_getres], ,AC_MSG_ERROR([cannot link to librt; check config.log]))
AC_CHECK_LIB([pthread], [pthread_mutex_init], ,AC_MSG_ERROR([cannot link to libpthread; check config.log]))
AC_CHECK_LIB([dl], [dlvsym], ,AC_MSG_ERROR([cannot link to libdl; check config.log]))

# Paths
#LDFLAGS="$LDFLAGS -L${prefix}/lib -L/usr/lib/x86_64-linux-gnu"
LDFLAGS="$LDFLAGS -L${prefix}/lib -L${prefix}/lib64"
CXXFLAGS="$CXXFLAGS -I${prefix}/include" 
CFLAGS="$CFLAGS -I${prefix}/include"

AC_CHECK_LIB([util], [openpty], ,AC_MSG_ERROR([cannot link to libutil; check config.log]))

AC_CHECK_LIB([z], [deflateInit_], ,AC_MSG_ERROR([cannot link to libz; check config.log]))
AC_CHECK_LIB([event], [event_config_new], ,AC_MSG_ERROR([cannot link to libevent; check config.log]))
AC_CHECK_LIB([hwloc], [hwloc_obj_type_snprintf], ,AC_MSG_ERROR([cannot link to libhwloc; check config.log]))
AC_CHECK_LIB([event_pthreads], [evthread_use_pthreads], ,AC_MSG_ERROR([cannot link to libevent_pthreads; check config.log]))
AC_CHECK_LIB([pmix], [PMIx_Info_load], ,AC_MSG_ERROR([cannot link to libpmix; check config.log]))
AC_CHECK_LIB([open-pal], [opal_event_alloc], ,AC_MSG_ERROR([cannot link to libopen-pal; check config.log]))
AC_CHECK_LIB([open-rte], [orte_util_print_name_args], ,AC_MSG_ERROR([cannot link to libopen-rte; check config.log]))

AC_CHECK_LIB([mpi], [MPI_Init], ,AC_MSG_ERROR([cannot link to libmpi; check config.log]))
AC_CHECK_LIB([mpi_mpifh], [mpi_init_], ,AC_MSG_ERROR([cannot link to libmpi_mpifh; check config.log]))
AC_CHECK_LIB([mpi_usempif08], [mpi_init_], ,AC_MSG_ERROR([cannot link to libmpi_usempif08; check config.log]))
AC_CHECK_LIB([mpi_usempi_ignore_tkr], [mpi_init_], ,AC_MSG_ERROR([cannot link to libmpi_usempi_ignore_tkr; check config.log]))

AC_CHECK_LIB([metis], [METIS_PartGraphKway], ,AC_MSG_ERROR([cannot link to libmetis; check config.log]))
AC_CHECK_LIB([HYPRE], [hypre_ParCSRMatrixCreate], ,AC_MSG_ERROR([cannot link to libHYPRE; check config.log]))

AC_CHECK_LIB([gfortran], [_gfortran_rand], ,AC_MSG_ERROR([cannot link to libgfortran; check config.log]))

AC_LANG([C++])
LIBS="$LIBS -lstdc++"
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
    using namespace std; ]], [[ double *a=new double ]])],
       [       AC_MSG_RESULT([checking for new in -lstdc++... yes])
               AC_DEFINE_UNQUOTED([HAVE_LIBSTDC__], [1], [Set to 1 when libstdc++ is found])
       ],
       [       AC_MSG_RESULT([checking for new in -lstdc++... no])
               AC_MSG_ERROR([cannot link to libstdc++; check config.log])
       ],
       []
)
AC_LANG([C])

AC_CHECK_LIB([fblas], [cswap_], ,AC_MSG_ERROR([cannot link to libfblas; check config.log]))
AC_CHECK_LIB([flapack], [cungqr_], ,AC_MSG_ERROR([cannot link to libflapack; check config.log]))
AC_CHECK_LIB([scalapack], [pbchkvect], ,AC_MSG_ERROR([cannot link to libscalapack; check config.log]))
AC_CHECK_LIB([scotcherr], [SCOTCH_errorPrint], ,AC_MSG_ERROR([cannot link to libscotcherr; check config.log]))
AC_CHECK_LIB([scotch], [scotchfstratinit_], ,AC_MSG_ERROR([cannot link to libscotch; check config.log]))
AC_CHECK_LIB([ptscotcherr], [SCOTCH_errorPrint], ,AC_MSG_ERROR([cannot link to libptscotcherr; check config.log]))
AC_CHECK_LIB([ptscotch], [scotchfdgraphorderinit_], ,AC_MSG_ERROR([cannot link to libptscotch; check config.log]))
AC_CHECK_LIB([ptscotcherrexit], [SCOTCH_errorPrint], ,AC_MSG_ERROR([cannot link to libptscotcherrexit; check config.log]))
AC_CHECK_LIB([pord], [printFactorMtx], ,AC_MSG_ERROR([cannot link to libpord; check config.log]))
AC_CHECK_LIB([esmumps], [esmumps], ,AC_MSG_ERROR([cannot link to libesmumps; check config.log]))
AC_CHECK_LIB([ptesmumps], [esmumpsv], ,AC_MSG_ERROR([cannot link to libptesmumps; check config.log]))
AC_CHECK_LIB([mumps_common], [mumps_abort_], ,AC_MSG_ERROR([cannot link to libmumps_common; check config.log]))
AC_CHECK_LIB([cmumps], [cmumps_root_solve_], ,AC_MSG_ERROR([cannot link to libcmumps; check config.log]))
AC_CHECK_LIB([dmumps], [dmumps_gather_root_], ,AC_MSG_ERROR([cannot link to libdmumps; check config.log]))
AC_CHECK_LIB([smumps], [smumps_gather_root_], ,AC_MSG_ERROR([cannot link to libsmumps; check config.log]))
AC_CHECK_LIB([zmumps], [zmumps_gather_root_], ,AC_MSG_ERROR([cannot link to libzmumps; check config.log]))
AC_CHECK_LIB([petscsys], [PetscObjectComm], ,AC_MSG_ERROR([cannot link to libpetscsys; check config.log]))
AC_CHECK_LIB([petscvec], [vecsum_], ,AC_MSG_ERROR([cannot link to libpetscvec; check config.log]))
AC_CHECK_LIB([petscmat], [matcreateaij_], ,AC_MSG_ERROR([cannot link to libpetscmat; check config.log]))
AC_CHECK_LIB([petscdm], [DMFieldCreateDS], ,AC_MSG_ERROR([cannot link to libpetscdm; check config.log]))
AC_CHECK_LIB([petscksp], [PCFieldSplitSetOffDiagUseAmat], ,AC_MSG_ERROR([cannot link to libpetscksp; check config.log]))
AC_CHECK_LIB([petscsnes], [SNESLineSearchCreate_Shell], ,AC_MSG_ERROR([cannot link to libpetscsnes; check config.log]))
AC_CHECK_LIB([petsctao], [TaoView], ,AC_MSG_ERROR([cannot link to libpetsctao; check config.log]))
AC_CHECK_LIB([petscts], [dmplexlandauijacobian_], ,AC_MSG_ERROR([cannot link to libpetscts; check config.log]))

AC_CHECK_LIB([slepcsys], [SlepcInitialize], ,AC_MSG_ERROR([cannot link to libslepcsys; check config.log]))
AC_CHECK_LIB([slepclme], [LMECreate_Krylov], ,AC_MSG_ERROR([cannot link to libslepclme; check config.log]))
AC_CHECK_LIB([slepceps], [EPSCreate_KrylovSchur], ,AC_MSG_ERROR([cannot link to libslepceps; check config.log]))
AC_CHECK_LIB([slepcmfn], [MFNCreate_Krylov], ,AC_MSG_ERROR([cannot link to libslepcmfn; check config.log]))
AC_CHECK_LIB([slepcpep], [PEPSTOARSetDimensions], ,AC_MSG_ERROR([cannot link to libslepcpep; check config.log]))
AC_CHECK_LIB([slepcnep], [NEPView], ,AC_MSG_ERROR([cannot link to libslepcnep; check config.log]))
AC_CHECK_LIB([slepcsvd], [SVDView], ,AC_MSG_ERROR([cannot link to libslepcsvd; check config.log]))

AC_LANG([C++])
LIBS="$LIBS -lmfem -lHYPRE -lmetis"
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
    #include "mfem.hpp"
    using namespace mfem; ]], [[ HypreParMatrix *mat; ]])],
       [       AC_MSG_RESULT([checking for HypreParMatrix in -lmfem... yes])
               AC_DEFINE_UNQUOTED([HAVE_LIBMFEM], [1], [Set to 1 when libmfem is found])
       ],
       [       AC_MSG_RESULT([checking for HypreParMatrix in -lmfem... no])
               AC_MSG_ERROR([cannot link to libmfem; check config.log])
       ],
       []
)
AC_LANG([C])

AC_LANG([C++])
LDFLAGS="$LDFLAGS -L$PWD/OpenParEMCommon"
LIBS="$LIBS -lOpenParEMCommon -lpetscsys -ldl -lmpi_mpifh -lgfortran -lquadmath"
AC_LANG([C])

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdlib.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([pow sqrt getcwd strdup strtoull])

# force use of c++17
m4_include([ax_cxx_compile_stdcxx.m4])
AX_CXX_COMPILE_STDCXX([17],[noext],[mandatory])

AC_CONFIG_FILES([Makefile OpenParEMCommon/Makefile OpenParEM2D/Makefile OpenParEM3D/Makefile builder/Makefile])
AC_OUTPUT
