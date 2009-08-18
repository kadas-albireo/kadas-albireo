@echo off
set GRASS_VERSION=6.4.0svn

set VERSION=%1
set PACKAGE=%2
if "%VERSION%"=="" goto error
if "%PACKAGE%"=="" goto error

path %SYSTEMROOT%\system32;%SYSTEMROOT%;%SYSTEMROOT%\System32\Wbem;%PROGRAMFILES%\CMake 2.6\bin
set PYTHONPATH=

set VS90COMNTOOLS=%PROGRAMFILES%\Microsoft Visual Studio 9.0\Common7\Tools\
call "%PROGRAMFILES%\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" x86

if "%OSGEO4W_ROOT%"=="" set OSGEO4W_ROOT=%PROGRAMFILES%\OSGeo4W
if not exist "%OSGEO4W_ROOT%\bin\o4w_env.bat" goto error

call "%OSGEO4W_ROOT%\bin\o4w_env.bat"

set BUILDDIR=%CD%\build
REM set BUILDDIR=%TEMP%\qgis_unstable

set O4W_ROOT=%OSGEO4W_ROOT:\=/%
set LIB_DIR=%O4W_ROOT%

set DEVENV=
if exist "%DevEnvDir%\vcexpress.exe" set DEVENV=vcexpress
if exist "%DevEnvDir%\devenv.exe" set DEVENV=devenv
if "%DEVENV%"=="" goto error

PROMPT qgis%VERSION%$g 

set BUILDCONF=RelWithDebInfo
REM set BUILDCONF=Release

if not exist "%BUILDDIR%" mkdir %BUILDDIR%
if not exist "%BUILDDIR%" goto error

cd ..\..
set SRCDIR=%CD%

if "%BUILDDIR:~1,1%"==":" %BUILDDIR:~0,2%
cd %BUILDDIR%

if not exist build.log goto build

REM
REM try renaming the logfile to see if it's locked
REM

if exist build.tmp del build.tmp
if exist build.tmp goto error

ren build.log build.tmp
if exist build.log goto locked
if not exist build.tmp goto locked

ren build.tmp build.log
if exist build.tmp goto locked
if not exist build.log goto locked

goto build

:locked
echo Logfile locked
if exist build.tmp del build.tmp
goto error

:build
set LOG=%BUILDDIR%\build.log

echo Logging to %LOG%
echo BEGIN: %DATE% %TIME%>>%LOG% 2>&1
if errorlevel 1 goto error

set >buildenv.log

if exist CMakeCache.txt goto skipcmake

echo CMAKE: %DATE% %TIME%>>%LOG% 2>&1
if errorlevel 1 goto error

cmake -G "Visual Studio 9 2008" ^
        -D PEDANTIC=TRUE ^
        -D WITH_SPATIALITE=TRUE ^
        -D WITH_INTERNAL_SPATIALITE=TRUE ^
	-D CMAKE_CONFIGURATION_TYPE=%BUILDCONF% ^
	-D CMAKE_BUILDCONFIGURATION_TYPES=%BUILDCONF% ^
	-D GDAL_INCLUDE_DIR=%O4W_ROOT%/apps/gdal-16/include ^
	-D GDAL_LIBRARY=%O4W_ROOT%/apps/gdal-16/lib/gdal_i.lib ^
	-D PYTHON_EXECUTABLE=%O4W_ROOT%/bin/python.exe ^
	-D PYTHON_INCLUDE_PATH=%O4W_ROOT%/apps/Python25/include ^
	-D PYTHON_LIBRARY=%O4W_ROOT%/apps/Python25/libs/python25.lib ^
	-D SIP_BINARY_PATH=%O4W_ROOT%/apps/Python25/sip.exe ^
	-D GRASS_PREFIX=%O4W_ROOT%/apps/grass/grass-%GRASS_VERSION% ^
	-D QT_BINARY_DIR=%O4W_ROOT%/bin ^
	-D QT_LIBRARY_DIR=%O4W_ROOT%/lib ^
	-D QT_HEADERS_DIR=%O4W_ROOT%/include/qt4 ^
	-D QT_ZLIB_LIBRARY=%O4W_ROOT%/lib/zlib.lib ^
	-D QT_PNG_LIBRARY=%O4W_ROOT%/lib/libpng13.lib ^
	-D CMAKE_INSTALL_PREFIX=%O4W_ROOT%/apps/qgis-dev ^
	%SRCDIR%>>%LOG% 2>&1
if errorlevel 1 goto error

REM bail out if python or grass was not found
grep -Eq "^(Python not being built|Could not find GRASS)" %LOG%
if not errorlevel 1 goto error

:skipcmake

echo ZERO_CHECK: %DATE% %TIME%>>%LOG% 2>&1
%DEVENV% qgis%VERSION%.sln /Project ZERO_CHECK /Build %BUILDCONF% /Out %LOG%>>%LOG% 2>&1
if errorlevel 1 goto error 

echo ALL_BUILD: %DATE% %TIME%>>%LOG% 2>&1
%DEVENV% qgis%VERSION%.sln /Project ALL_BUILD /Build %BUILDCONF% /Out %LOG%>>%LOG% 2>&1
if errorlevel 1 goto error 

echo INSTALL: %DATE% %TIME%>>%LOG% 2>&1
%DEVENV% qgis%VERSION%.sln /Project INSTALL /Build %BUILDCONF% /Out %LOG%>>%LOG% 2>&1
if errorlevel 1 goto error

echo PACKAGE: %DATE% %TIME%>>%LOG% 2>&1

cd ..
copy postinstall.bat %OSGEO4W_ROOT%\etc\postinstall\qgis-dev.bat
copy preremove.bat %OSGEO4W_ROOT%\etc\preremove\qgis-dev.bat
copy qgis-dev.bat.tmpl %OSGEO4W_ROOT%\bin\qgis-dev.bat.tmpl

sed -e 's/%OSGEO4W_ROOT:\=\\\\\\\\%/@osgeo4w@/' %OSGEO4W_ROOT%\apps\qgis-dev\python\qgis\qgisconfig.py >%OSGEO4W_ROOT%\apps\qgis-dev\python\qgis\qgisconfig.py.tmpl
if errorlevel 1 goto error

del %OSGEO4W_ROOT%\apps\qgis-dev\python\qgis\qgisconfig.py

touch exclude

tar -C %OSGEO4W_ROOT% -cjf qgis-dev-%VERSION%-%PACKAGE%.tar.bz2 ^
	--exclude-from exclude ^
	apps/qgis-dev ^
	bin/qgis-dev.bat.tmpl ^
	etc/postinstall/qgis-dev.bat ^
	etc/preremove/qgis-dev.bat>>%LOG% 2>&1
if errorlevel 1 goto error

goto end

:error
echo BUILD ERROR %ERRORLEVEL%: %DATE% %TIME%
echo BUILD ERROR %ERRORLEVEL%: %DATE% %TIME%>>%LOG% 2>&1
if exist qgis-dev-%VERSION%-%PACKAGE%.tar.bz2 del qgis-dev-%VERSION%-%PACKAGE%.tar.bz2

:end
echo FINISHED: %DATE% %TIME% >>%LOG% 2>&1
