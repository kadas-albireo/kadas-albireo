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
