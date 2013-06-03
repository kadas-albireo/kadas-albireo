textreplace -std -t bin\@package@.bat
textreplace -std -t bin\@package@-browser.bat

if not %OSGEO4W_MENU_LINKS%==0 mkdir "%OSGEO4W_STARTMENU%"
if not %OSGEO4W_MENU_LINKS%==0 xxmklink "%OSGEO4W_STARTMENU%\QGIS Desktop (@version@).lnk"       "%OSGEO4W_ROOT%\bin\@package@.bat" " " \ "QGIS - Desktop GIS (@version@)" 1 "%OSGEO4W_ROOT%\apps\@package@\icons\QGIS.ico"
if not %OSGEO4W_MENU_LINKS%==0 xxmklink "%OSGEO4W_STARTMENU%\QGIS Browser (@version@).lnk"       "%OSGEO4W_ROOT%\bin\@package@-browser.bat" " " \ "QGIS - Desktop GIS (@version@)" 1 "%OSGEO4W_ROOT%\apps\@package@\icons\QGIS.ico"

if not %OSGEO4W_DESKTOP_LINKS%==0 xxmklink "%ALLUSERSPROFILE%\Desktop\QGIS Desktop (@version@).lnk" "%OSGEO4W_ROOT%\bin\@package@.bat" " " \ "QGIS - Desktop GIS (@version@)" 1 "%OSGEO4W_ROOT%\apps\@package@\icons\QGIS.ico"
if not %OSGEO4W_DESKTOP_LINKS%==0 xxmklink "%ALLUSERSPROFILE%\Desktop\QGIS Browser (@version@).lnk" "%OSGEO4W_ROOT%\bin\@package@-browser.bat" " " \ "QGIS - Desktop GIS (@version@)" 1 "%OSGEO4W_ROOT%\apps\@package@\icons\QGIS.ico"

set O4W_ROOT=%OSGEO4W_ROOT%
set OSGEO4W_ROOT=%OSGEO4W_ROOT:\=\\%
textreplace -std -t "%O4W_ROOT%\apps\@package@\bin\qgis.reg"
"%WINDIR%\regedit" /s "%O4W_ROOT%\apps\@package@\bin\qgis.reg"
