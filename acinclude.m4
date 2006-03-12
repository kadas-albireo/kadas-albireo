dnl ------------------------------------------------------------------------
dnl Detect if this is a 64bit environment
dnl
dnl it sets:
dnl   _lib
dnl ------------------------------------------------------------------------
AC_DEFUN([AQ_CHECK_LIB64],
[
if test `echo ${libdir} | sed -e 's#.*lib64.*#64#'` = "64"; then
  _lib="lib64"
else
  _lib="lib"
fi
])

dnl ------------------------------------------------------------------------
dnl Detect GDAL/OGR
dnl
dnl use AQ_CHECK_GDAL to detect GDAL and OGR
dnl it sets:
dnl   GDAL_CFLAGS
dnl   GDAL_LDADD
dnl ------------------------------------------------------------------------

# Check for GDAL and OGR compiler and linker flags

AC_DEFUN([AQ_CHECK_GDAL],
[
AC_ARG_WITH([gdal],
  AC_HELP_STRING([--with-gdal=path],
    [Full path to 'gdal-config' script, e.g. '--with-gdal=/usr/local/bin/gdal-config']),
  [ac_gdal_config_path=$withval])

if test x"$ac_gdal_config_path" = x ; then
  ac_gdal_config_path=`which gdal-config`
fi

ac_gdal_config_path=`dirname $ac_gdal_config_path 2> /dev/null`
AC_PATH_PROG(GDAL_CONFIG, gdal-config, no, $ac_gdal_config_path)

if test x${GDAL_CONFIG} = xno ; then
  AC_MSG_ERROR([gdal-config not found! Supply it with --with-gdal=PATH])
else
  AC_MSG_CHECKING([for OGR in GDAL])
  if test x`$GDAL_CONFIG --ogr-enabled` = "xno" ; then
    AC_MSG_ERROR([GDAL must be compiled with OGR support and currently is not.])
  fi
  AC_MSG_RESULT(yes)
  AC_MSG_CHECKING([GDAL_CFLAGS])
  GDAL_CFLAGS=`$GDAL_CONFIG --cflags`
  AC_MSG_RESULT($GDAL_CFLAGS)

  AC_MSG_CHECKING([GDAL_LDADD])
  GDAL_LDADD=`$GDAL_CONFIG --libs`
  AC_MSG_RESULT($GDAL_LDADD)

  ac_gdalogr_version=`$GDAL_CONFIG --version`
  ac_gdalogr="yes"
fi

AC_SUBST(GDAL_CFLAGS)
AC_SUBST(GDAL_LDADD)
])

dnl ------------------------------------------------------------------------
dnl Detect GEOS
dnl
dnl use AQ_CHECK_GEOS to detect GEOS
dnl it sets:
dnl   GEOS_CFLAGS
dnl   GEOS_LDADD
dnl ------------------------------------------------------------------------

# Check for GEOS

AC_DEFUN([AQ_CHECK_GEOS],
[
AC_ARG_WITH([geos],
  AC_HELP_STRING([--with-geos=path],
    [Full path to 'geos-config' script, e.g. '--with-geos=/usr/local/bin/geos-config']),
  [ac_geos_config_path=$withval])

if test x"$ac_geos_config_path" = x ; then
  ac_geos_config_path=`which geos-config`
fi

ac_geos_config_path=`dirname $ac_geos_config_path 2> /dev/null`
AC_PATH_PROG(GEOS_CONFIG, geos-config, no, $ac_geos_config_path)

if test x${GEOS_CONFIG} = xno ; then
  AC_MSG_ERROR([geos-config not found! Supply it with --with-geos=PATH])
else
  ac_geos_version=`${GEOS_CONFIG} --version`
  if test `echo ${ac_geos_version} | sed -e 's#2\.[0-9].*#OK#'` != OK ; then
    AC_MSG_ERROR([Geos Version 2.x.x is needed, but you have $ac_geos_version!])
  else
    AC_MSG_CHECKING([GEOS_CFLAGS])
    GEOS_CFLAGS=`$GEOS_CONFIG --cflags`
    AC_MSG_RESULT($GEOS_CFLAGS)

    AC_MSG_CHECKING([GEOS_LDADD])
    GEOS_LDADD=`$GEOS_CONFIG --libs`
    AC_MSG_RESULT($GEOS_LDADD)

    ac_geos="yes"
  fi
fi

AC_SUBST(GEOS_CFLAGS)
AC_SUBST(GEOS_LDADD)
])

dnl ------------------------------------------------------------------------                                                                             
dnl Detect QT
dnl
dnl use AQ_CHECK_QT to detect QT
dnl it sets:
dnl   QT_CXXFLAGS
dnl   QT_LDADD
dnl   QT_GUILINK
dnl   QASSISTANTCLIENT_LDADD
dnl ------------------------------------------------------------------------                                                                             

# Check for Qt compiler flags, linker flags, and binary packages

AC_DEFUN([AQ_CHECK_QT],
[
AC_REQUIRE([AC_PROG_CXX])
AC_REQUIRE([AC_PATH_X])

AC_MSG_CHECKING([QTDIR])
AC_ARG_WITH([qtdir], [  --with-qtdir=DIR        Qt installation directory [default=/usr/local]], QTDIR=$withval)
# Check that QTDIR is defined or that --with-qtdir given
if test x$QTDIR = x ; then
  QT_SEARCH=" /usr/lib/qt31 /usr/lib64/qt31 /usr/local/qt31 /usr/lib/qt3 /usr/lib64/qt3 /usr/local/qt3 /usr/lib/qt2 /usr/lib64/qt2 /usr/local/qt2 /usr/lib/qt /usr/lib64/qt /usr/local/qt /usr /usr/local"
  for i in $QT_SEARCH; do
    if test x$QTDIR = x; then
      if test -f $i/include/qt/qglobal.h -o -f $i/include/qglobal.h -o -f $i/include/qt3/qglobal.h; then
        QTDIR=$i
      fi
    fi
  done
fi
if test x$QTDIR = x ; then
  AC_MSG_ERROR([*** QTDIR must be defined, or --with-qtdir option given])
fi
AC_MSG_RESULT([$QTDIR])

# Change backslashes in QTDIR to forward slashes to prevent escaping
# problems later on in the build process, mainly for Cygwin build
# environment using MSVC as the compiler
# (include/Qt is used on Qt4)
# TODO: Use sed instead of perl
QTDIR=`echo $QTDIR | perl -p -e 's/\\\\/\\//g'`

# Check for QT includedir 
if test -f $QTDIR/include/qt/qglobal.h; then
  QTINC=$QTDIR/include/qt
  QTVERTEST=$QTDIR/include/qt
elif test -f $QTDIR/include/qt3/qglobal.h; then
  QTINC=$QTDIR/include/qt3
  QTVERTEST=$QTDIR/include/qt3
elif test -f $QTDIR/include/Qt/qglobal.h -a -f $QTDIR/src/corelib/global/qglobal.h; then
  # Windows: $QTDIR/include/Qt/qglobal.h includes $QTDIR/src/corelib/global/qglobal.h
  QTINC=$QTDIR/include
  QTVERTEST=$QTDIR/src/corelib/global/
elif test -f $QTDIR/include/Qt/qglobal.h; then
  QTINC=$QTDIR/include
  QTVERTEST=$QTDIR/include/Qt
elif test -f $QTDIR/lib/QtCore.framework/Headers/qglobal.h; then
  QTINC=$QTDIR/lib/QtCore.framework/Headers
  QTVERTEST=$QTDIR/lib/QtCore.framework/Headers
else
  QTINC=$QTDIR/include
  QTVERTEST=$QTDIR/include
fi

# Figure out which version of Qt we are using
AC_MSG_CHECKING([Qt version])
QT_VER=`grep 'define.*QT_VERSION_STR\W' $QTVERTEST/qglobal.h | perl -p -e 's/\D//g'`
case "${QT_VER}" in
  41*)
    QT_MAJOR="4"
    case "${host}" in
    *-darwin*)
      QT4_3SUPPORTINC=$QTDIR/lib/Qt3Support.framework/Headers
      QT4_COREINC=$QTDIR/lib/QtCore.framework/Headers
      QT4_GUIINC=$QTDIR/lib/QtGui.framework/Headers
      QT4_NETWORKINC=$QTDIR/lib/QtNetwork.framework/Headers
      QT4_OPENGLINC=$QTDIR/lib/QtOpenGL.framework/Headers
      QT4_SQLINC=$QTDIR/lib/QtSql.framework/Headers
      QT4_XMLINC=$QTDIR/lib/QtXml.framework/Headers
      QT4_SVGINC=$QTDIR/lib/QtSvg.framework/Headers
	  ;;
    *)
      QT4_3SUPPORTINC=$QTDIR/include/Qt3Support
      QT4_COREINC=$QTDIR/include/QtCore
      QT4_GUIINC=$QTDIR/include/QtGui
      QT4_NETWORKINC=$QTDIR/include/QtNetwork
      QT4_OPENGLINC=$QTDIR/include/QtOpenGL
      QT4_SQLINC=$QTDIR/include/QtSql
      QT4_XMLINC=$QTDIR/include/QtXml
      QT4_SVGINC=$QTDIR/include/QtSvg
      ;;
    esac
    QT4_DESIGNERINC=$QTDIR/include/QtDesigner
    QT4_DEFAULTINC=$QTDIR/mkspecs/default
    ;;
#  33*)
#    QT_MAJOR="3"
#    ;;
#  32*)
#    QT_MAJOR="3"
#    ;;
#  31*)
#    QT_MAJOR="3"
#    ;;
  *)
    AC_MSG_ERROR([*** Qt version 4.1 or higher is required])
    ;;
esac
AC_MSG_RESULT([$QT_VER ($QT_MAJOR)])

if test $QT_MAJOR = "3" ; then
  # Check that moc is in path
  AC_CHECK_PROG(MOC, moc, moc)
  if test x$MOC = x ; then
    AC_MSG_ERROR([*** moc must be in path])
  fi
  # uic is the Qt user interface compiler
  AC_CHECK_PROG(UIC, uic, uic)
  if test x$UIC = x ; then
    AC_MSG_ERROR([*** uic must be in path])
  fi
fi

if test $QT_MAJOR = "4" ; then
  # Hard code things for the moment

  # Check that moc is in path
  if test $cross_compiling = "yes" ; then # MinGW
      AC_CHECK_PROG(MOC, moc, moc)
      if test x$MOC = x ; then
	AC_MSG_ERROR([*** moc must be in path])
      fi
      # uic is the Qt user interface compiler
      AC_CHECK_PROG(UIC, uic, uic)
      if test x$UIC = x ; then
	AC_MSG_ERROR([*** uic must be in path])
      fi
      # check for rcc
      AC_CHECK_PROG(RCC, rcc, rcc)
      if test x$RCC = x ; then
	AC_MSG_ERROR([*** rcc must be in path])
      fi
  else
      AC_CHECK_PROG(MOC, moc, $QTDIR/bin/moc, , $QTDIR/bin)
      if test x$MOC = x ; then
	AC_MSG_ERROR([*** moc must be in path])
      fi
      # uic is the Qt user interface compiler
      AC_CHECK_PROG(UIC, uic, $QTDIR/bin/uic, , $QTDIR/bin)
      if test x$UIC = x ; then
	AC_MSG_ERROR([*** uic must be in path])
      fi
      # check for rcc
      AC_CHECK_PROG(RCC, rcc, $QTDIR/bin/rcc, , $QTDIR/bin)
      if test x$RCC = x ; then
	AC_MSG_ERROR([*** rcc must be in path])
      fi
  fi
fi

# qembed is the Qt data embedding utility.
# It is located in $QTDIR/tools/qembed, and must be compiled and installed
# manually, we'll let it slide if it isn't present
### AC_CHECK_PROG(QEMBED, qembed, qembed)
# Calculate Qt include path
if test $QT_MAJOR = "3" ; then
QT_CXXFLAGS="-I$QTINC"
fi
if test $QT_MAJOR = "4" ; then
QT_CXXFLAGS="-DQT3_SUPPORT -I$QT4_DEFAULTINC -I$QT4_3SUPPORTINC -I$QT4_COREINC -I$QT4_DESIGNERINC -I$QT4_GUIINC -I$QT4_NETWORKINC -I$QT4_OPENGLINC -I$QT4_SQLINC -I$QT4_XMLINC -I$QTINC -I$QT4_SVGINC"
fi
QT_IS_EMBEDDED="no"
# On unix, figure out if we're doing a static or dynamic link

case "${host}" in
  *-cygwin)
    AC_DEFINE_UNQUOTED(WIN32, "", Defined if on Win32 platform)
    echo "$QTDIR/lib/qt-mt$QT_VER.lib"
    if test -f "$QTDIR/lib/qt-mt$QT_VER.lib" ; then
      QT_LIB="qt-mt$QT_VER.lib"
      QT_IS_STATIC="no"
      QT_IS_MT="yes"
         
    elif test -f "$QTDIR/lib/qt$QT_VER.lib" ; then
      QT_LIB="qt$QT_VER.lib"
      QT_IS_STATIC="no"
      QT_IS_MT="no"
    elif test -f "$QTDIR/lib/qt.lib" ; then
      QT_LIB="qt.lib"
      QT_IS_STATIC="yes"
      QT_IS_MT="no"
    elif test -f "$QTDIR/lib/qt-mt.lib" ; then
      QT_LIB="qt-mt.lib" 
      QT_IS_STATIC="yes"
      QT_IS_MT="yes"
    fi
    ;;
  *-darwin*)
    # determin static or dynamic -- prefer dynamic
    QT_IS_DYNAMIC=`ls $QTDIR/lib/libqt*.dylib $QTDIR/lib/QtCore.framework/QtCore 2> /dev/null`
    if test "x$QT_IS_DYNAMIC" = x;  then
      QT_IS_STATIC=`ls $QTDIR/lib/libqt*.a 2> /dev/null`
      if test "x$QT_IS_STATIC" = x; then
        QT_IS_STATIC="no"
        AC_MSG_ERROR([*** Couldn't find any Qt libraries in $QTDIR/lib])
      else
        QT_IS_STATIC="yes"
      fi
    else
      QT_IS_STATIC="no"
    fi
    # set link parameters based on shared/mt libs or static lib
    if test "x`ls $QTDIR/lib/libqt.a* 2> /dev/null`" != x ; then
      QT_LIB="-lqt"
      QT_IS_MT="no"
    elif test "x`ls $QTDIR/lib/libqt-mt.*.dylib 2> /dev/null`" != x ; then
      QT_LIB="-lqt-mt"
      QT_IS_MT="yes"
    elif test "x`ls $QTDIR/lib/libqt.*.dylib 2> /dev/null`" != x ; then
      QT_LIB="-lqt"
      QT_IS_MT="no"
    elif test "x`ls $QTDIR/lib/libqte.* 2> /dev/null`" != x ; then
      QT_LIB="-lqte"
      QT_IS_MT="no"
      QT_IS_EMBEDDED="yes"
    elif test "x`ls $QTDIR/lib/libqte-mt.* 2> /dev/null`" != x ; then
      QT_LIB="-lqte-mt"
      QT_IS_MT="yes"
      QT_IS_EMBEDDED="yes"
    elif test "x`ls $QTDIR/lib/QtCore.framework/QtCore 2> /dev/null`" != x ; then
      QT_LIB="-Xlinker -F$QTDIR/lib -framework QtCore -framework Qt3Support -framework QtGui -framework QtNetwork -framework QtXml -framework QtSvg"
      QT_CXXFLAGS="-DQT3_SUPPORT -F$QTDIR/lib -I$QT4_DEFAULTINC -I$QT4_3SUPPORTINC -I$QT4_COREINC -I$QT4_DESIGNERINC -I$QT4_GUIINC -I$QT4_NETWORKINC -I$QT4_OPENGLINC -I$QT4_SQLINC -I$QT4_XMLINC -I$QT4_SVGINC"
      QT_IS_MT="yes"
    fi
    ;;
  *)
    # determin static or dynamic -- prefer dynamic
    QT_IS_DYNAMIC=`ls $QTDIR/${_lib}/libqt*.so $QTDIR/${_lib}/libQtCore.so /usr/lib/libQtCore.so 2> /dev/null`
    if test "x$QT_IS_DYNAMIC" = x;  then
      QT_IS_STATIC=`ls $QTDIR/${_lib}/libqt*.a $QTDIR/${_lib}/libQtCore.a 2> /dev/null`
      if test "x$QT_IS_STATIC" = x; then
        QT_IS_STATIC="no"
        AC_MSG_ERROR([*** Couldn't find any Qt libraries in $QTDIR/${_lib}])
      else
        QT_IS_STATIC="yes"
      fi
    else
      QT_IS_STATIC="no"
    fi
    # set link parameters based on shared/mt libs or static lib
    if test "x`ls $QTDIR/${_lib}/libqt.a* 2> /dev/null`" != x ; then
      QT_LIB="-lqt"
      QT_IS_MT="no"
    elif test "x`ls $QTDIR/${_lib}/libqt-mt.so* 2> /dev/null`" != x ; then
      QT_LIB="-lqt-mt"
      QT_IS_MT="yes"
    elif test "x`ls $QTDIR/${_lib}/libqt.so* 2> /dev/null`" != x ; then
      QT_LIB="-lqt"
      QT_IS_MT="no"
    elif test "x`ls $QTDIR/${_lib}/libqte.* 2> /dev/null`" != x ; then
      QT_LIB="-lqte"
      QT_IS_MT="no"
      QT_IS_EMBEDDED="yes"
    elif test "x`ls $QTDIR/${_lib}/libqte-mt.* 2> /dev/null`" != x ; then
      QT_LIB="-lqte-mt"
      QT_IS_MT="yes"
      QT_IS_EMBEDDED="yes"
    elif test "x`ls $QTDIR/${_lib}/libQtCore.* /usr/lib/libQtCore.* 2> /dev/null`" != x ; then
      QT_LIB="-lQtCore -lQt3Support -lQtGui -lQtNetwork -lQtSvg"
QT_CXXFLAGS="-DQT3_SUPPORT -I$QT4_DEFAULTINC -I$QT4_3SUPPORTINC -I$QT4_COREINC -I$QT4_DESIGNERINC -I$QT4_GUIINC -I$QT4_NETWORKINC -I$QT4_OPENGLINC -I$QT4_SQLINC -I$QT4_XMLINC -I$QTINC -I$QT4_SVGINC"
      QT_IS_MT="yes"
    fi
    ;;
esac

AC_MSG_CHECKING([if Qt is static])
AC_MSG_RESULT([$QT_IS_STATIC])
AC_MSG_CHECKING([if Qt is multithreaded])
AC_MSG_RESULT([$QT_IS_MT])
AC_MSG_CHECKING([if Qt is embedded])
AC_MSG_RESULT([$QT_IS_EMBEDDED])

QT_GUILINK=""
QASSISTANTCLIENT_LDADD="-lqassistantclient"
case "${host}" in
  *-mingw*)
     QT_LIBS="-lQtCore4 -lQt3Support4 -lQtGui4 -lQtNetwork4 -lQtXml4 -lQtSvg4"
    ;;
  *irix*)
    QT_LIBS="$QT_LIB"
    if test $QT_IS_STATIC = yes ; then
    QT_LIBS="$QT_LIBS -L$x_libraries -lXext -lX11 -lm -lSM -lICE"
    fi
    ;;

  *linux*)
    QT_LIBS="$QT_LIB"
    if test $QT_IS_STATIC = yes && test $QT_IS_EMBEDDED = no; then
      QT_LIBS="$QT_LIBS -L$x_libraries -lXext -lX11 -lm -lSM -lICE -ldl -ljpeg"
    fi
    ;;

  *freebsd*)
    QT_LIBS="$QT_LIB"
    if test $QT_IS_STATIC = yes && test $QT_IS_EMBEDDED = no; then
      QT_LIBS="$QT_LIBS -L$x_libraries -lXext -lX11 -lm -lSM -lICE -ldl -ljpeg -lpthread"
   else
      QT_LIBS="$QT_LIBS -lpthread"
    fi
    ;;

  *darwin*)
    QT_LIBS="$QT_LIB"
    if test $QT_IS_STATIC = yes && test $QT_IS_EMBEDDED = no; then
      QT_LIBS="$QT_LIBS -L$x_libraries -lXext -lX11 -lm -lSM -lICE -ldl -ljpeg"
    fi
    ;;

  *osf*) 
    # Digital Unix (aka DGUX aka Tru64)
    QT_LIBS="$QT_LIB"
    if test $QT_IS_STATIC = yes ; then
      QT_LIBS="$QT_LIBS -L$x_libraries -lXext -lX11 -lm -lSM -lICE"
    fi
    ;;

  *solaris*)
    QT_LIBS="$QT_LIB"
    if test $QT_IS_STATIC = yes ; then
      QT_LIBS="$QT_LIBS -L$x_libraries -lXext -lX11 -lm -lSM -lICE -lresolv -lsocket -lnsl"
    fi
    ;;

  *win*)
    # linker flag to suppress console when linking a GUI app on Win32
    QT_GUILINK="/subsystem:windows"
    if test $QT_MAJOR = "3" ; then
      if test $QT_IS_MT = yes ; then
        QT_LIBS="/nodefaultlib:libcmt"
      else
        QT_LIBS="/nodefaultlib:libc"
      fi
    fi

    if test $QT_IS_STATIC = yes ; then
      QT_LIBS="$QT_LIBS $QT_LIB kernel32.lib user32.lib gdi32.lib comdlg32.lib ole32.lib shell32.lib imm32.lib advapi32.lib wsock32.lib winspool.lib winmm.lib netapi32.lib"
      if test $QT_MAJOR = "3" ; then
        QT_LIBS="$QT_LIBS qtmain.lib"
      fi
    else
      QT_LIBS="$QT_LIBS $QT_LIB"        
      if test $QT_MAJOR = "3" ; then
        QT_CXXFLAGS="$QT_CXXFLAGS -DQT_DLL"
        QT_LIBS="$QT_LIBS qtmain.lib qui.lib user32.lib netapi32.lib"
      fi
    fi
    QASSISTANTCLIENT_LDADD="qassistantclient.lib"
    ;;
esac

if test x"$QT_IS_EMBEDDED" = "xyes" ; then
  QT_CXXFLAGS="-DQWS $QT_CXXFLAGS"
fi

if test x"$QT_IS_MT" = "xyes" ; then
  QT_CXXFLAGS="$QT_CXXFLAGS -D_REENTRANT -DQT_THREAD_SUPPORT"
fi

case "${host}" in
  *-mingw*)
    QT_LDADD="-L$QTDIR/${_lib} $QT_LIBS"
    ;;
  *)
    QT_LDADD="-L$QTDIR/${_lib} $QT_LIBS"
    ;;
esac

if test x$QT_IS_STATIC = xyes ; then
  OLDLIBS="$LIBS"
  LIBS="$QT_LDADD"
  AC_CHECK_LIB(Xft, XftFontOpen, QT_LDADD="$QT_LDADD -lXft")
  LIBS="$LIBS"
fi

AC_MSG_CHECKING([QT_CXXFLAGS])
AC_MSG_RESULT([$QT_CXXFLAGS])
AC_MSG_CHECKING([QT_LDADD])
AC_MSG_RESULT([$QT_LDADD])

AC_SUBST(QT_CXXFLAGS)
AC_SUBST(QT_LDADD)
AC_SUBST(QT_GUILINK)
AC_SUBST(QASSISTANTCLIENT_LDADD)
AC_SUBST(QTDIR)
])


dnl ------------------------------------------------------------------------
dnl
dnl improved Qt4 check
dnl - uses pkgconfig by default
dnl - can be overridden by -with-qtdir=....
dnl
dnl ------------------------------------------------------------------------

AC_DEFUN([AQ_CHECK_QT4],[
    
  AC_ARG_WITH([qtdir], AC_HELP_STRING([--with-qtdir=DIR],[Qt4 installation directory]),
              QTDIR="$withval", QTDIR="")

  QT_MIN_VER=4.1.0
  
  if test "x$QTDIR" == "x" ; then
  
    dnl ---------------------------------------------------------------------------
    dnl we will use PKGCONFIG, check that all needed Qt4 components are there
    dnl ---------------------------------------------------------------------------
    
    PKG_CHECK_MODULES(QTCORE, QtCore     >= $QT_MIN_VER)
    PKG_CHECK_MODULES(QTGUI,  QtGui      >= $QT_MIN_VER)
    PKG_CHECK_MODULES(QT3SUP, Qt3Support >= $QT_MIN_VER)
    PKG_CHECK_MODULES(QTNET,  QtNetwork  >= $QT_MIN_VER)
    PKG_CHECK_MODULES(QTXML,  QtXml      >= $QT_MIN_VER)
    PKG_CHECK_MODULES(QTSVG,  QtSvg      >= $QT_MIN_VER)

    dnl check for Qt binaries needed for compilation: moc,uic,rcc
    dnl (we could also check for moc and uic versions)
    
    AC_CHECK_PROG(MOC, moc, moc)
    if test x$MOC = x ; then
      AC_MSG_ERROR([*** moc must be in path])
    fi
    AC_CHECK_PROG(UIC, uic, uic)
    if test x$UIC = x ; then
      AC_MSG_ERROR([*** uic must be in path])
    fi
    AC_CHECK_PROG(RCC, rcc, rcc)
    if test x$RCC = x ; then
      AC_MSG_ERROR([*** rcc must be in path])
    fi

    dnl set and display variables
    
    QT_CXXFLAGS="-DQT3_SUPPORT $QTCORE_CFLAGS $QTGUI_CFLAGS $QT3SUP_CFLAGS $QTNET_CFLAGS $QTXML_CFLAGS $QTSVG_CFLAGS"
    AC_MSG_CHECKING([QT_CXXFLAGS])
    AC_MSG_RESULT([$QT_CXXFLAGS])
    AC_SUBST([$QT_CXXFLAGS])

    QT_LDADD="$QTCORE_LIBS $QTGUI_LIBS $QT3SUP_LIBS $QTNET_LIBS $QTXML_LIBS $QTSVG_LIBS"
    AC_MSG_CHECKING([QT_LDADD])
    AC_MSG_RESULT([$QT_LDADD])
    AC_SUBST([$QT_LDADD])

    QTDIR="no_qtdir"
    AC_SUBST([$QTDIR])

    QT_MAJOR=4

  else
  
    dnl ---------------------------------------------------------------------------
    dnl let's use old code for detection
    dnl it needs cleanups since there is still Qt3 detection stuff
    dnl ---------------------------------------------------------------------------
    AQ_CHECK_QT

    dnl ---------------------------------------------------------------------------
    dnl Qt/Mac check (install everything into application bundle)
    dnl ---------------------------------------------------------------------------
    if test x$QTDIR != x -a -f "$QTDIR/mkspecs/default/Info.plist.app"; then
      have_qtmac=yes
      bundle_suffix=$PACKAGE.app/Contents/MacOS
      if test `expr "$prefix" : ".*$bundle_suffix$"` -eq 0; then
        prefix="$prefix/$bundle_suffix"
      fi
    fi

  fi
  
  dnl do we still need this? [MD]
  dnl Assume for the moment that Qt4 installations will still compile against uic3
  dnl AM_CONDITIONAL([NO_UIC_IMPLEMENTATIONS], [test "$QT_MAJOR" = "4"])
  AM_CONDITIONAL([NO_UIC_IMPLEMENTATIONS], [test "$QT_MAJOR" = "0"])
  AM_CONDITIONAL([HAVE_QT3], [test "$QT_MAJOR" = "3"])
  AM_CONDITIONAL([HAVE_QT4], [test "$QT_MAJOR" = "4"])

  AM_CONDITIONAL([HAVE_QTMAC], [test "$have_qtmac" = "yes"])
])


# Configure path for the GNU Scientific Library
# Christopher R. Gabriel <cgabriel@linux.it>, April 2000


AC_DEFUN([AM_PATH_GSL],
[
AC_ARG_WITH(gsl-prefix,[  --with-gsl-prefix=PFX   Prefix where GSL is installed (optional)],
            gsl_prefix="$withval", gsl_prefix="")
AC_ARG_WITH(gsl-exec-prefix,[  --with-gsl-exec-prefix=PFX Exec prefix where GSL is installed (optional)],
            gsl_exec_prefix="$withval", gsl_exec_prefix="")
AC_ARG_ENABLE(gsltest, [  --disable-gsltest       Do not try to compile and run a test GSL program],
		    , enable_gsltest=yes)

  if test "x${GSL_CONFIG+set}" != xset ; then
     if test "x$gsl_prefix" != x ; then
         GSL_CONFIG="$gsl_prefix/bin/gsl-config"
     fi
     if test "x$gsl_exec_prefix" != x ; then
        GSL_CONFIG="$gsl_exec_prefix/bin/gsl-config"
     fi
  fi

  AC_PATH_PROG(GSL_CONFIG, gsl-config, no)
  min_gsl_version=ifelse([$1], ,0.2.5,$1)
  AC_MSG_CHECKING(for GSL - version >= $min_gsl_version)
  no_gsl=""
  if test "$GSL_CONFIG" = "no" ; then
    no_gsl=yes
  else
    GSL_CFLAGS=`$GSL_CONFIG --cflags`
    GSL_LIBS=`$GSL_CONFIG --libs`

    gsl_major_version=`$GSL_CONFIG --version | \
           sed 's/^\([[0-9]]*\).*/\1/'`
    if test "x${gsl_major_version}" = "x" ; then
       gsl_major_version=0
    fi

    gsl_minor_version=`$GSL_CONFIG --version | \
           sed 's/^\([[0-9]]*\)\.\{0,1\}\([[0-9]]*\).*/\2/'`
    if test "x${gsl_minor_version}" = "x" ; then
       gsl_minor_version=0
    fi

    gsl_micro_version=`$GSL_CONFIG --version | \
           sed 's/^\([[0-9]]*\)\.\{0,1\}\([[0-9]]*\)\.\{0,1\}\([[0-9]]*\).*/\3/'`
    if test "x${gsl_micro_version}" = "x" ; then
       gsl_micro_version=0
    fi

    if test "x$enable_gsltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GSL_CFLAGS"
      LIBS="$LIBS $GSL_LIBS"

      rm -f conf.gsltest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* my_strdup (const char *str);

char*
my_strdup (const char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (void)
{
  int major = 0, minor = 0, micro = 0;
  int n;
  char *tmp_version;

  system ("touch conf.gsltest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_gsl_version");

  n = sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) ;

  if (n != 2 && n != 3) {
     printf("%s, bad version string\n", "$min_gsl_version");
     exit(1);
   }

   if (($gsl_major_version > major) ||
      (($gsl_major_version == major) && ($gsl_minor_version > minor)) ||
      (($gsl_major_version == major) && ($gsl_minor_version == minor) && ($gsl_micro_version >= micro)))
    {
      exit(0);
    }
  else
    {
      printf("\n*** 'gsl-config --version' returned %d.%d.%d, but the minimum version\n", $gsl_major_version, $gsl_minor_version, $gsl_micro_version);
      printf("*** of GSL required is %d.%d.%d. If gsl-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If gsl-config was wrong, set the environment variable GSL_CONFIG\n");
      printf("*** to point to the correct copy of gsl-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      exit(1);
    }
}

],, no_gsl=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_gsl" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$GSL_CONFIG" = "no" ; then
       echo "*** The gsl-config script installed by GSL could not be found"
       echo "*** If GSL was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the GSL_CONFIG environment variable to the"
       echo "*** full path to gsl-config."
     else
       if test -f conf.gsltest ; then
        :
       else
          echo "*** Could not run GSL test program, checking why..."
          CFLAGS="$CFLAGS $GSL_CFLAGS"
          LIBS="$LIBS $GSL_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GSL or finding the wrong"
          echo "*** version of GSL. If it is not finding GSL, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GSL was incorrectly installed"
          echo "*** or that you have moved GSL since it was installed. In the latter case, you"
          echo "*** may want to edit the gsl-config script: $GSL_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
#     GSL_CFLAGS=""
#     GSL_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GSL_CFLAGS)
  AC_SUBST(GSL_LIBS)
  rm -f conf.gsltest
])

dnl Python
dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/ax_python.html
dnl
AC_DEFUN([AX_PYTHON],
[
AC_ARG_WITH([python],
AC_HELP_STRING([--with-python],
[Include Python scripting support ]))

if test x"$with_python" = "x"; then
AC_MSG_RESULT( Not using python )
    ac_use_python=no
else
AC_MSG_CHECKING(for python build information)
   for python in python2.4 python2.3 python2.2 python2.1 python; do
   AC_CHECK_PROGS(PYTHON_BIN, [$python])
   ax_python_bin=$PYTHON_BIN
   if test x$ax_python_bin != x; then
      AC_CHECK_LIB($ax_python_bin, main, ax_python_lib=$ax_python_bin, ax_python_lib=no)
      if test `echo ${host} | grep '.*-darwin.*'`; then
        python_prefix=/System/Library/Frameworks/Python.framework/*/
      fi
      AC_CHECK_HEADER([$ax_python_bin/Python.h],
      [[ax_python_header=`locate $python_prefix$ax_python_bin/Python.h | sed -e s,/Python.h,,`]],
      ax_python_header=no)
      if test $ax_python_lib != no; then
        if test $ax_python_header != no; then
          break;
        fi
      fi
   fi
   done
   if test x$ax_python_bin = x; then
      ax_python_bin=no
   fi
   if test x$ax_python_header = x; then
      ax_python_header=no
   fi
   if test x$ax_python_lib = x; then
      ax_python_lib=no
   fi
 
   AC_MSG_RESULT([  results of the Python check:])
   AC_MSG_RESULT([    Binary:      $ax_python_bin])
   AC_MSG_RESULT([    Library:     $ax_python_lib])
   AC_MSG_RESULT([    Include Dir: $ax_python_header])
   AC_MSG_RESULT([    Have python: $ac_use_python])
 
   if test x$ax_python_header != xno; then
     PYTHON_INCLUDE_DIR=-I$ax_python_header
     AC_SUBST(PYTHON_INCLUDE_DIR)
   fi
   if test x$ax_python_lib != xno; then
     PYTHON_LIB=-l$ax_python_lib
     AC_SUBST(PYTHON_LIB)
   fi
  if test x$ax_python_header != xno; then
  dnl  & x$ax_python_lib != xno; then
    ac_use_python=yes
    HAVE_PYTHON=-DHAVE_PYTHON
  fi 
fi

AM_CONDITIONAL([USE_PYTHON], [test "$ac_use_python" = "yes"])
])
dnl
