#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(compcoll, 1.0, yhfudev@gmail.com)
AC_CONFIG_SRCDIR([src/compcoll.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(-Wall subdir-objects)

#magic for conditional check in Makefile:
MK=''; AC_SUBST(MK)
SED=sed

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

#AC_PROG_LIBTOOL
# Checks for libraries.
#LT_PREREQ([2.2])
#LT_INIT([shared static])
#LT_INIT([disable-static])

# Checks for libraries.

#AC_CHECK_LIB([bfd], [bfd_openr])

#AC_CHECK_LIB([z], [zlibVersion])
#PKG_CHECK_MODULES([ZLIB], [zlib])
#AC_SUBST(ZLIB_CFLAGS)
#AC_SUBST(ZLIB_LIBS)

#AC_CHECK_LIB([xml2], [main])
#PKG_CHECK_MODULES([LIBXML2], [libxml-2.0])
#AC_CHECK_LIB([xslt], [main])
#PKG_CHECK_MODULES([LIBXSLT], [libxslt])




dnl Disable doc generation with doxygen option
AC_ARG_ENABLE(doxygen-doc,
              [AC_HELP_STRING([--disable-doxygen-doc],
                              [disable document generation with doxygen])],
              , enable_doxygen_doc="yes")
if test "x$enable_doxygen_doc" = "xyes"; then
  AC_CHECK_PROG(DOXYGEN,doxygen,doxygen,no)
  if test "x$DOXYGEN" = "xno"; then
    enable_doxygen_doc="no"
  fi
fi
dnl where to install the doxygen-generated HTML doc
AC_ARG_WITH(html-docdir,
            [AC_HELP_STRING([--with-html-docdir=DIR],
                            [where to install the doxyten-generated HTML doc [PREFIX/share/doc/libseederdict/html]])],
            [htmldocdir="$withval"], [htmldocdir=\$\{prefix\}/share/doc/libseederdict/html])
AC_SUBST(htmldocdir)
AM_CONDITIONAL(ENABLE_DOXYGEN_DOC,test "x$enable_doxygen_doc" = "xyes")


# debug
AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug],[Compile the debug version (default: disabled)]),
	[enable_debug=$enableval],
	[enable_debug=no])
AM_CONDITIONAL([DEBUG], [test $enable_debug = "yes"])
if test "x$enable_debug" = "xyes"; then
  changequote({,})
  CFLAGS=`echo "$CFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  CXXFLAGS=`echo "$CXXFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  changequote([,])
  dnl add -O0 only if GCC or ICC is used
  if test "$GCC" = "yes" || test "$ICC" = "yes"; then
    CFLAGS="$CFLAGS -g -O0 -Werror"
    CXXFLAGS="$CXXFLAGS -g -O0 -Werror"
  fi
else
  changequote({,})
  CFLAGS=`echo "$CFLAGS" | $SED -e 's/-g//g'`
  CXXFLAGS=`echo "$CXXFLAGS" | $SED -e 's/-g//g'`
  changequote([,])
fi

AC_ARG_WITH([iconv],
        AC_HELP_STRING([--with-iconv],
                [Use the libiconv (default=no)]),[
	with_iconv=$withval
],[
	with_iconv=no
])

have_iconv=no
# if no iconv, then icu ...
if test x$with_iconv = xno; then
    PKG_CHECK_EXISTS(icu,[
            PKG_CHECK_MODULES([LIBICU], [icu])
        ], [
            PKG_CHECK_MODULES([LIBICU], [icu-i18n])
        ])
    #AC_SUBST(LIBICU_CFLAGS)
    #AC_SUBST(LIBICU_LIBS)

else
    LIBICONV_INCLUDEDIR=""
    for d in \
        $with_iconv/include \
        $prefix/include \
        /usr/include
    do
        if test -f "$d/iconv.h" ; then
	    LIBICONV_INCLUDEDIR="$d"
	    break
        fi
    done
    if test "x$LIBICONV_INCLUDEDIR" = "x"; then
        echo "checking for iconv.h...no"
        have_iconv=no

    else
        echo "checking for ucnv_err.h...yes [$LIBICONV_INCLUDEDIR]"
        have_iconv=yes
        AM_CONDITIONAL([USE_ICONV], [test $have_iconv = "yes"]) # AC_DEFINE(USE_ICONV,1,[Defined when iconv can be used.])
    fi
fi

if test x$have_iconv = xno; then
    AM_CONDITIONAL([USE_ICONV], [test $have_iconv = "yes"])
    LIBICU_INCLUDEDIR=""
    for d in 			\
        $prefix/include/unicode \
        /usr/include/unicode \
        /usr/include/x86_64-linux-gnu/unicode \
        /usr/include/i386-linux-gnu/unicode \
        /usr/include
    do
        if test -f "$d/ucnv_err.h" ; then
          LIBICU_INCLUDEDIR="$d"
          break
        fi
    done
    if test "x$LIBICU_INCLUDEDIR" = "x"; then
        AC_MSG_ERROR([You should have iconv or ICU header files! Install libicu-dev])
        have_iconv=no
    else
        echo "checking for ucnv_err.h...yes"
        have_iconv=yes
    fi
    dnl AC_DEFINE(HAVE_ICULIB,1,[Defined when libicu can be used.])
    dnl AC_CHECK_LIB(icu, ucsdet_open, [],
    dnl         [AC_MSG_ERROR([library 'icu' is required for ICU]), [ `icu-config --ldflags` ]])
else
PKG_CHECK_MODULES([LIBCHSETDET], [libchsetdet])
#AC_CHECK_LIB([chsetdet], [ucsdet_open])
AC_SUBST(LIBCHSETDET_CFLAGS)
AC_SUBST(LIBCHSETDET_LIBS)

fi
AM_CONDITIONAL([BUILD_WITH_ICULIB], [test "$have_iconv" = "yes"])




#dnl Check for needed wrappers 
#dnl    - if command line is not specified, then the path value is taken
#AC_ARG_WITH(xml2-config, AC_HELP_STRING([[--with-xml2-config=FILE]], [Path to xml2-config program]), 
	    #[ XML2="$withval" ], [ AC_PATH_PROG(XML2, xml2-config, [], [$PATH:/usr/sbin/]) ])
#AC_ARG_WITH(xslt-config, AC_HELP_STRING([[--with-xslt-config=FILE]], [Path to xslt-config program]), 
	    #[ XSLT="$withval" ], [ AC_PATH_PROG(XSLT, xslt-config, [], [$PATH:/usr/sbin/]) ])

#dnl # Check if we found the wrapper 
#if test x$XML2 == x; then
	#AC_MSG_ERROR([Couldn't find 'xml2-config' in PATH. Please specify '--with-xml2-config' to configure.])
#fi
#if test x$XSLT == x; then
	#AC_MSG_ERROR([Couldn't find 'xslt-config' in PATH. Please specify '--with-xslt-config' to configure.])
#fi

#dnl # What a shame !@!! gcc-2.95 does not 
#dnl # like spaces between -l and the name of the library!
#AC_CHECK_LIB([xml2], [xmlParseMemory], [ XML2F='yes' ], AC_MSG_ERROR([Couldn't find libxml2 ]), [ `$XML2 --libs` ])
#AC_CHECK_LIB([xslt], [xsltApplyStylesheet], [ XSLTF='yes' ], AC_MSG_ERROR([Couldn't find libxslt ]), [ `$XSLT --libs` ])

#if test "$XML2F" = "yes"; then
  #CPPFLAGS="`$XML2 --cflags` $CPPFLAGS";

  #dnl # Check for libxml2 headers
  #AC_CHECK_HEADERS([libxml/uri.h libxml/tree.h libxml/parserInternals.h],,
    		  #AC_MSG_ERROR([Some or all required header files for libxml2 are missing. ]))

  #dnl # Check if we have threading enabled 
  #AC_CHECK_LIB([xml2], [xmlGetGlobalState], [ 
          #LIBS="$LIBS"  
	  #AC_DEFINE(HAVE_LIBXML_THREADS,, [A version of libxml2 with multithreading support is being used])
	#], [ 
	#if test "$INTERACTIVE" = "yes"; then
  	  #AC_MSG_WARN([ ] 
#[******************************************************]
#[* Your libxml2 is not compiled with multithreading support. ]
#[* Please make sure to read the `README` _carefully_.]
#[* Compilation will go on anyway - but don't expect me to]
#[* believe your bug reports. Hit a key to continue.]
#[******************************************************]);
	  #read
	#else
  	  #AC_MSG_WARN([ ] 
#[******************************************************]
#[* Your libxml2 is not compiled with multithreading support. ]
#[* Please make sure to read the `README` _carefully_.]
#[* Compilation will go on anyway - but don't expect me to]
#[* believe your bug reports. ] 
#[******************************************************]);
	#fi], [ `$XML2 --libs` ])

  #dnl # Check if libxml2 patch has been applyed
  #AC_CHECK_LIB([xml2], [xmlSetGlobalState], [ 
          #LIBS="$LIBS" 
          #AC_DEFINE(HAVE_LIBXML_HACK,, [A version of libxml2 that does support setGlobalState is being used])
	#], [
  	  #AC_MSG_WARN([ ] 
#[******************************************************]
#[* Your libxml2 does not provide a setGlobalState. ]
#[* You probably haven't patched it. Please read the `README`.]
#[* Compilation will go on anyway - I've installed some compatibility handlers.]
#[******************************************************])
	 #], [ `$XML2 --libs` ])

  #dnl # LIBS="$LIBS `$XML2 --libs`"
#fi

#dnl # Add libxslt parameters
#if test "$XSLTF" = "yes"; then
  #CPPFLAGS="$CPPFLAGS `$XSLT --cflags`"

  #dnl # Check for libxslt headers
  #AC_CHECK_HEADERS([libxslt/xslt.h libxslt/xsltInternals.h libxslt/transform.h libxslt/xsltutils.h],,
  		  #[ AC_MSG_ERROR([Some or all header files for libxslt are missing ]) ])

  #LIBS="$LIBS `$XSLT --libs`"

  #dnl # Check if exslt was asked
  #AC_ARG_ENABLE([exslt], AC_HELP_STRING([[--enable-exslt]], [Enable exslt extensions for libxslt. Default disabled ]),
	    #[ EXSLT="yes" ], [ EXSLT="no" ])


  #if test "$EXSLT" = "yes"; then
    #AC_CHECK_HEADERS([libexslt/exslt.h],, [ AC_MSG_ERROR([Could not find headers for exslt library ]) ])
    #AC_CHECK_LIB([exslt], [exsltLibraryVersion], [ LIBS="$LIBS" ], [ AC_MSG_ERROR([Could not link against libexslt ]) ])

    #AC_DEFINE(USE_EXSLT,,[libexslt is installed on the system and you want mod-yaslt to use it])

    #dnl #CPPFLAGS="$CPPFLAGS"
    #LIBS="$LIBS -lexslt"
  #fi
#fi



# Checks for header files.
AC_CHECK_HEADERS([inttypes.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset getline])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
