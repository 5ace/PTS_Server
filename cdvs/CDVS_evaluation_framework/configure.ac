dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([MPEG-CDVS], [14.2], [massimo.balestri@telecomitalia.it])
AC_CANONICAL_SYSTEM
AM_PROG_AR
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_CXX_C_O
AC_PROG_INSTALL
AC_OPENMP

# GNU LibTool provides a portable way to build libraries.  AutoMake
# knows how to use it; you just need to activate it.
LT_INIT
# Turn off shared libraries during development, since they
# make the build process take too long.
# LT_INIT([disable-shared])          

AC_C_INLINE
AC_C_RESTRICT
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_HEADER_MAJOR 
AC_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T
AC_CONFIG_SRCDIR([src/extract.cpp])
AC_CONFIG_HEADERS([config.h])

# Checking for libjpeg library and headers required by all CDVS flavors
AC_CHECK_LIB(jpeg, jpeg_start_decompress, [], [AC_MSG_ERROR([libjpeg is not installed])])
AC_CHECK_HEADER(jpeglib.h, [], [AC_MSG_ERROR([libjpeg headers are not installed])])

# Checking if the BFlog flavor has to be built and related dependencies
# (both fftw3 libraries and headersare checked)
AC_ARG_WITH(bflog,
   AS_HELP_STRING([[[--with-bflog]]], [Builds and installs the BFlog-based library]),
   [with_bflog=${withval}], [with_bflog='no']
)
if test "x$with_bflog" = "xyes" ; then
    AC_CHECK_HEADER(fftw3.h, [], [AC_MSG_ERROR([libfftw3 headers are not installed])])
	AC_CHECK_LIB(fftw3, fftw_execute, [], [AC_MSG_ERROR([libfftw3 is not installed])])
fi
AM_CONDITIONAL([WITH_BFLOG], [test x$with_bflog = xyes])

# Checking if the LowMem flavor has to be built
AC_ARG_WITH(lowmem,
   AS_HELP_STRING([[[--with-lowmem]]], [Builds and installs the LowMem-based library]),
   [with_lowmem=${withval}], [with_lowmem='no']
)
AM_CONDITIONAL([WITH_LOWMEM], [test x$with_lowmem = xyes])

AC_CONFIG_FILES([Makefile src/Makefile lib/Makefile src-interop/Makefile shared/Makefile libraries/Makefile libraries/Distrat/Makefile libraries/map/Makefile libraries/resampler/Makefile libraries/timer/Makefile libraries/vlfeat/vl/Makefile libraries/bitstream/Makefile libraries/bitstream/src/Makefile libraries/gmm-fisher/Makefile])
AC_OUTPUT
