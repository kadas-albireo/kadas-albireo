#!/bin/sh

arch=${1:-x86_64}
qt=${2:-qt5}

if [ "$arch" == "i686" ]; then
    bits=32
elif [ "$arch" == "x86_64" ]; then
    bits=64
else
    echo "Error: unrecognized architecture $arch"
    exit 1
fi

if [ "$qt" == "qt5" ]; then
  useqt5=1
elif [ "qt" == "qt4" ]; then
  useqt5=0
else
    echo "Error: unrecognized qt version $qt"
    exit 1
fi

# Note: This script is written to be used with the Fedora mingw environment
MINGWROOT=/usr/$arch-w64-mingw32/sys-root/mingw

optflags="-O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions --param=ssp-buffer-size=4 -fno-omit-frame-pointer"

# Halt on errors
set -e

export MINGW32_CFLAGS="$optflags"
export MINGW32_CXXFLAGS="$optflags"
export MINGW64_CFLAGS="$optflags"
export MINGW64_CXXFLAGS="$optflags"

srcdir="$(readlink -f "$(dirname "$(readlink -f "$0")")/..")"
builddir="$srcdir/build_mingw${bits}_${qt}"
installroot="$builddir/dist"
installprefix="$installroot/usr/$arch-w64-mingw32/sys-root/mingw"

# Cleanup
rm -rf "$installroot"

# Build
mkdir -p $builddir
(
  cd $builddir
  mingw$bits-cmake \
    -DFULL_RELEASE_NAME="KADAS Albireo" \
    -DRELEASE_NAME="KADAS" \
    -DRELEASE_VERSION="1.1" \
    -DENABLE_QT5=$useqt5 \
    -DQT_INCLUDE_DIRS_NO_SYSTEM=ON \
    -DWINDRES=$arch-w64-mingw32-windres \
    -DWITH_INTERNAL_QWTPOLAR=1 \
    -DWITH_GLOBE=1 \
    -DNATIVE_CRSSYNC_BIN=$(readlink -f $builddir)/native_crssync/crssync \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DQGIS_BIN_SUBDIR=bin \
    -DQGIS_LIBEXEC_SUBDIR=bin \
    -DQGIS_DATA_SUBDIR=share/qgis \
    -DQGIS_PLUGIN_SUBDIR=bin/qgisplugins \
    -DBINDINGS_GLOBAL_INSTALL=TRUE \
    -DPYUIC4_PROGRAM=/usr/bin/pyuic5 \
    -DPYRCC4_PROGRAM=/usr/bin/pyrcc5 \
    -DQUAZIP_LIBRARIES=$MINGWROOT/bin/libquazip5.dll \
    -DQUAZIP_INCLUDE_DIR=$MINGWROOT/include/quazip5 \
    -DQSCINTILLA_INCLUDE_DIR=$MINGWROOT/include/qt5 \
    -DPYTHON_EXECUTABLE=/usr/$arch-w64-mingw32/bin/python2 \
    ..
  )

# Compile native crssync
mkdir -p $builddir/native_crssync
(
cd $builddir/native_crssync
echo "Building native crssync..."
moc-qt5 $srcdir/src/core/qgsapplication.h > moc_qgsapplication.cpp
g++ $optflags -fPIC -o crssync $srcdir/src/crssync/main.cpp $srcdir/src/crssync/qgscrssync.cpp moc_qgsapplication.cpp $srcdir/src/core/qgsapplication.cpp -DCORE_EXPORT= -DCOMPILING_CRSSYNC -I$srcdir/src/core/ -I$srcdir/src/core/geometry -I$builddir $(pkg-config --cflags --libs Qt5Widgets gdal sqlite3 proj)
)

njobs=$(($(grep -c ^processor /proc/cpuinfo) * 3 / 2))
mingw$bits-make -C$builddir -j$njobs DESTDIR="${installroot}" install

binaries=$(find $installprefix -name '*.exe' -or -name '*.dll' -or -name '*.pyd')

# Strip debuginfo
for f in $binaries
do
    case $(mingw-objdump -h $f 2>/dev/null | egrep -o '(debug[\.a-z_]*|gnu.version)') in
        *debuglink*) continue ;;
        *debug*) ;;
        *gnu.version*)
        echo "WARNING: $(basename $f) is already stripped!"
        continue
        ;;
        *) continue ;;
    esac

    echo extracting debug info from $f
    mingw-objcopy --only-keep-debug $f $f.debug || :
    pushd $(dirname $f)
    keep_symbols=`mktemp`
    mingw-nm $f.debug --format=sysv --defined-only | awk -F \| '{ if ($4 ~ "Function") print $1 }' | sort > "$keep_symbols"
    mingw-objcopy --add-gnu-debuglink=`basename $f.debug` --strip-unneeded `basename $f` --keep-symbols="$keep_symbols" || :
    rm -f "$keep_symbols"
    popd
done

# Collect dependencies
function isnativedll {
    # If the import library exists but not the dynamic library, the dll ist most likely a native one
    local lower=${1,,}
    [ ! -e $MINGWROOT/bin/$1 ] && [ -f $MINGWROOT/lib/lib${lower/%.*/.a} ] && return 0;
    return 1;
}

function linkDep {
# Link the specified binary dependency and it's dependencies
    local indent=$3
    local destdir="$installprefix/${2:-bin}"
    local name="$(basename $1)"
    test -e "$destdir/$name" && return 0
    test -e "$destdir/qgisplugins/$name" && return 0
    echo "${indent}${1}"
    [ ! -e "$MINGWROOT/$1" ] && (echo "Error: missing $MINGWROOT/$1"; return 1)
    mkdir -p "$destdir" || return 1
    ln -sf "$MINGWROOT/$1" "$destdir/$name" || return 1
    autoLinkDeps "$destdir/$name" "${indent}  " || return 1
    [ -e "$MINGWROOT/$1.debug" ] && ln -sf "$MINGWROOT/$1.debug" "$destdir/$name.debug" || echo "Warning: missing $name.debug"
    return 0
}

function autoLinkDeps {
# Collects and links the dependencies of the specified binary
    for dep in $(mingw-objdump -p "$1" | grep "DLL Name" | awk '{print $3}'); do
        if ! isnativedll "$dep"; then
            # HACK fix incorrect libpq case
            dep=${dep/LIBPQ/libpq}
            linkDep bin/$dep bin "$2" || return 1
        fi
    done
    return 0
}

echo "Linking dependencies..."
for binary in $binaries; do
    autoLinkDeps $binary
done
linkDep bin/gdb.exe

linkDep $(ls $MINGWROOT/bin/libssl-*.dll | sed "s|$MINGWROOT/||")
linkDep $(ls $MINGWROOT/bin/libcrypto-*.dll | sed "s|$MINGWROOT/||")
# Additional qt4 dependencies
if [ "$qt" == "qt4" ]; then
  linkDep lib/qt4/plugins/imageformats/qgif4.dll  bin/imageformats
  linkDep lib/qt4/plugins/imageformats/qico4.dll  bin/imageformats
  linkDep lib/qt4/plugins/imageformats/qmng4.dll  bin/imageformats
  linkDep lib/qt4/plugins/imageformats/qtga4.dll  bin/imageformats
  linkDep lib/qt4/plugins/imageformats/qsvg4.dll  bin/imageformats
  linkDep lib/qt4/plugins/imageformats/qtiff4.dll bin/imageformats
  linkDep lib/qt4/plugins/imageformats/qjpeg4.dll bin/imageformats

  mkdir -p $installprefix/share/qt4/translations/
  cp -a $MINGWROOT/share/qt4/translations/qt_*.qm  $installprefix/share/qt4/translations
elif [ "$qt" == "qt5" ]; then
  linkDep lib/qt5/plugins/imageformats/qgif.dll  bin/imageformats
  linkDep lib/qt5/plugins/imageformats/qicns.dll bin/imageformats
  linkDep lib/qt5/plugins/imageformats/qico.dll  bin/imageformats
  linkDep lib/qt5/plugins/imageformats/qjp2.dll  bin/imageformats
  linkDep lib/qt5/plugins/imageformats/qjpeg.dll bin/imageformats
  linkDep lib/qt5/plugins/imageformats/qtga.dll  bin/imageformats
  linkDep lib/qt5/plugins/imageformats/qtiff.dll bin/imageformats
  linkDep lib/qt5/plugins/imageformats/qwbmp.dll bin/imageformats
  linkDep lib/qt5/plugins/imageformats/qwebp.dll bin/imageformats
  linkDep lib/qt5/plugins/imageformats/qsvg.dll  bin/imageformats
  linkDep lib/qt5/plugins/platforms/qwindows.dll bin/platforms
  linkDep lib/qt5/plugins/printsupport/windowsprintersupport.dll bin/printsupport

  mkdir -p $installprefix/share/qt5/translations/
  cp -a $MINGWROOT/share/qt5/translations/qt_*.qm  $installprefix/share/qt5/translations
  cp -a $MINGWROOT/share/qt5/translations/qtbase_*.qm  $installprefix/share/qt5/translations
fi

# Install python libs
(
cd $MINGWROOT
SAVEIFS=$IFS
IFS=$(echo -en "\n\b")
for file in $(find lib/python2.7 -type f); do
    mkdir -p "$installprefix/$(dirname $file)"
    ln -sf "$MINGWROOT/$file" "$installprefix/$file"
done
IFS=$SAVEIFS
)

# Osg plugins
ln -sf $MINGWROOT/bin/osgPlugins-3.5.5 $installprefix/bin/osgPlugins-3.5.5

# Data files
mkdir -p $installprefix/share/
ln -sf /usr/share/gdal $installprefix/share/gdal
