Building KADAS Albireo from source
==================================

Build
-----

Clone or download source code:

    git clone https://github.com/sourcepole/kadas-albireo.git
    cd kadas-albireo

Get the Docker image for building:

    docker pull sourcepole/kadas-mingw-buildenv

Build within Docker container with attached source code:

    docker run -v $PWD:/workspace sourcepole/kadas-mingw-buildenv ms-windows/mingwbuild.sh

Windows:

    docker run -v %CD%:/workspace sourcepole/kadas-mingw-buildenv ms-windows/mingwbuild.sh

Tar within Docker container the portable App:

    docker run -v $PWD:/workspace sourcepole/kadas-mingw-buildenv tar -hcvf ./kadas.tar ./build_mingw64_qt5/dist/usr/x86_64-w64-mingw32/sys-root/mingw


