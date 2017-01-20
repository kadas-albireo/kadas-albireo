#!/bin/sh

arch=${1:-x86_64}

if [ "$arch" == "i686" ]; then
    bits=32
elif [ "$arch" == "x86_64" ]; then
    bits=64
else
    echo "Error: unrecognized architecture $arch"
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
builddir="$srcdir/build_mingw"
installroot="$builddir/dist"
installprefix="$installroot/usr/$arch-w64-mingw32/sys-root/mingw"

# Cleanup
rm -rf "$installroot"

# Build
mkdir -p $builddir
(
    cd $builddir
    mingw$bits-cmake \
        -DQT_INCLUDE_DIRS_NO_SYSTEM=ON \
        -DWINDRES=$arch-w64-mingw32-windres \
        -DWITH_INTERNAL_QWTPOLAR=1 \
        -DWITH_GLOBE=1 \
        -DNATIVE_CRSSYNC_BIN=$srcdir/build/output/bin/crssync \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DQGIS_BIN_SUBDIR=bin \
        -DQGIS_LIBEXEC_SUBDIR=bin \
        -DQGIS_DATA_SUBDIR=share/qgis \
        -DQGIS_PLUGIN_SUBDIR=bin/qgisplugins \
        -DBINDINGS_GLOBAL_INSTALL=TRUE ..
)

mingw$bits-make -C$builddir -j8 DESTDIR="${installroot}" install

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
	mingw-objcopy --add-gnu-debuglink=$(basename $f.debug) --strip-unneeded $(basename $f) || :
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
    autoLinkDeps "$destdir/$name" "${2:-bin}" "${indent}  " || return 1
    if [ $withdebug ]; then
        [ -e "$MINGWROOT/$1.debug" ] && ln -sf "$MINGWROOT/$1.debug" "$destdir/$name.debug" || echo "Warning: missing $name.debug"
    fi
    return 0
}

function autoLinkDeps {
# Collects and links the dependencies of the specified binary
    for dep in $(mingw-objdump -p "$1" | grep "DLL Name" | awk '{print $3}'); do
        if ! isnativedll "$dep"; then
            # HACK fix incorrect libpq case
            dep=${dep/LIBPQ/libpq}
            linkDep bin/$dep "$2" "$3" || return 1
        fi
    done
    return 0
}

echo "Linking dependencies..."
for binary in $binaries; do
    autoLinkDeps $binary
done
linkDep bin/gdb.exe

# Additional qt4 dependencies
linkDep $(ls $MINGWROOT/bin/libssl-*.dll)
linkDep $(ls $MINGWROOT/bin/libcrypto-*.dll)
linkDep lib/qt4/plugins/imageformats/qgif4.dll  bin/imageformats
linkDep lib/qt4/plugins/imageformats/qico4.dll  bin/imageformats
linkDep lib/qt4/plugins/imageformats/qmng4.dll  bin/imageformats
linkDep lib/qt4/plugins/imageformats/qtga4.dll  bin/imageformats
linkDep lib/qt4/plugins/imageformats/qsvg4.dll  bin/imageformats
linkDep lib/qt4/plugins/imageformats/qtiff4.dll bin/imageformats
linkDep lib/qt4/plugins/imageformats/qjpeg4.dll bin/imageformats

# Install locale files
mkdir -p $installprefix/share/qt4/translations/
cp -a $MINGWROOT/share/qt4/translations/qt_*.qm  $installprefix/share/qt4/translations
