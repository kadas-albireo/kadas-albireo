Building with Microsoft Visual Studio
=====================================

This section describes the setup required to allow Visual Studio to be used to
build QGIS. 

See also https://schneide.wordpress.com/2013/03/26/building-visual-c-projects-with-cmake/
for an introduction for using with cmake and Visual Studio.


Visual C++ Express Edition
==========================

The free (as in free beer) Express Edition installer is available under:

  http://download.microsoft.com/download/c/d/7/cd7d4dfb-5290-4cc7-9f85-ab9e3c9af796/vc_web.exe

You also need the Windows SDK for Windows 7 and .NET Framework 4:

  http://download.microsoft.com/download/A/6/A/A6AC035D-DA3F-4F0C-ADA4-37C8E5D34E3D/winsdk_web.exe


Other tools and dependencies
============================

Download and install following packages:

  || Tool | Website |
  | CMake | http://www.cmake.org/files/v3.0/cmake-3.0.2-win32-x86.exe |
  | GNU flex, GNU bison and GIT | http://cygwin.com/setup-x86_64.exe |
  | OSGeo4W | http://download.osgeo.org/osgeo4w/osgeo4w-setup-x86_64.exe |

OSGeo4W does not only provide ready packages for the current QGIS release and
nightly builds of master, but also offers most of the dependencies needs to
build it.

* Select "Advanced installation"
* Add additional package source http://build.sourcepole.ch/osgeo4w
* Select both sources

Install package 'qgis-kadas-build' to install all required build dependencies.


Special dependencies of QGIS KADAS:

* GEOS with C++ bindings
* QJson: http://qjson.sourceforge.net/
* Exiv2: http://www.exiv2.org/download.html
  Build:
    cmake -G "Visual Studio 10 Win64" -DCMAKE_CONFIGURATION_TYPES=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=../exiv2
    cmake --build . --target install --config RelWithDebInfo
* QuaZIP: http://quazip.sourceforge.net/


Get the source code
===================

On the command prompt checkout the QGIS source from git to the source directory QGIS:

    git clone git@git.sourcepole.ch:sourcepole/qgis-enterprise.git
or
    git clone https://git.sourcepole.ch/sourcepole/qgis-enterprise.git
(don't forget setting the Proxy variables!)
  
    git checkout qgis_enterprise_15_vbs


Command line Build
==================

    cd qgis-enterprise\ms-windows\osgeo4w
    
    build-ci 15.01kadas 01 qgis-kadas x86_64
    
To skip some steps, you can create a files in the build directory:

    touch noclean
    touch skiptests
    touch repackage

Setting up the Visual Studio project with CMake
===============================================

To start a command prompt with an environment that both has the VC++ and the OSGeo4W
variables create the following batch file (assuming the above packages were
installed in the default locations):

	@echo off

	set VS90COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools\
	call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" amd64

	set INCLUDE=%INCLUDE%;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\include
	set LIB=%LIB%;C:\Program Files (x86)\Mycrosoft SDKs\Windows\v7.0A\lib

	set OSGEO4W_ROOT=C:\OSGeo4W64
	call "%OSGEO4W_ROOT%\bin\o4w_env.bat"
	path %PATH%;C:\Program Files (x86)\CMake\bin;c:\cygwin64\bin;C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE

	@set GRASS_PREFIX=%OSGEO4W_ROOT%/apps/grass/grass-6.4.3
	@set INCLUDE=%INCLUDE%;%OSGEO4W_ROOT%\include
	@set LIB=%LIB%;%OSGEO4W_ROOT%\lib;%OSGEO4W_ROOT%\lib

	@cmd

Start the batch file and on the command prompt and got to the QGIS source directory

Create a 'build' directory somewhere. This will be where all the build output
will be generated.

Now run cmake-gui (still from cmd) and in the Where is the source code:
box, browse to the top level QGIS directory.

In the Where to build the binaries: box, browse to the 'build' directory you
created.

If the path to bison and flex contains blanks, you need to use the short name
for the directory (i.e. C:\Program Files should be rewritten to
C:\Progra~n, where n is the number as shown in `dir /x C:\``).

Verify that the 'BINDINGS_GLOBAL_INSTALL' option is not checked, so that python
bindings are placed into the output directory when you run the INSTALL target.

Hit Configure to start the configuration and select Visual Studio 10 2010
and keep native compilers and click Finish.

The configuration should complete without any further questions and allow you to
click Generate.

Now close cmake-gui and continue on the command prompt by starting
devenv.  Use File / Open / Project/Solutions and open the
qgis-x.y.z.sln File in your project directory.

Change Solution Configuration from Debug to RelWithDebInfo (Release
with Debug Info)  or Release before you build QGIS using the ALL_BUILD
target (otherwise you need debug libraries that are not included).

After the build completed you should install QGIS using the INSTALL target.

Install QGIS by building the INSTALL project. By default this will install to
c:\Program Files\qgis<version> (this can be changed by changing the
CMAKE_INSTALL_PREFIX variable in cmake-gui). 

You will also either need to add all the dependency DLLs to the QGIS install
directory or add their respective directories to your PATH.


Packaging
=========

To create a standalone installer there is a perl script named 'creatensis.pl'
in 'qgis-enterprise/ms-windows/osgeo4w'.  It downloads all required packages from OSGeo4W
and repackages them into an installer using NSIS.

The script can be run on both Windows and Linux.

On Debian/Ubuntu you can just install the 'nsis' package.

NSIS for Windows can be downloaded at:

  http://nsis.sourceforge.net

And Perl for Windows (including other requirements like 'wget', 'unzip', 'tar'
and 'bzip2') is available at:

  http://cygwin.com


Packaging your own build of QGIS
================================

Assuming you have completed the above packaging step, if you want to include
your own hand built QGIS executables, you need to copy them in from your
windows installation into the ms-windows file tree created by the creatensis
script.

  cd ms-windows/
  rm -rf osgeo4w/unpacked/apps/qgis/*
  cp -r /tmp/qgis1.7.0/* osgeo4w/unpacked/apps/qgis/

Now create a package.

  ./quickpackage.sh

After this you should now have a nsis installer containing your own build 
of QGIS and all dependencies needed to run it on a windows machine.
