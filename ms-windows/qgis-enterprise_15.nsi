!include "MUI2.nsh"
!include "FileFunc.nsh"

Name "QGIS Enterprise 15"
!define INSTALLATIONNAME "QGIS Enterprise 15"
OutFile "qgis_enterprise_15_setup.exe"

InstallDir $PROGRAMFILES\QGIS_Enterprise_15

!define MUI_ABORTWARNING
!define MUI_ICON ".\Installer-Files\QGIS.ico"
!define MUI_UNICON ".\Installer-Files\QGIS.ico"
!define MUI_HEADERIMAGE_BITMAP_NOSTETCH ".\Installer-Files\InstallHeaderImage.bmp"
!define MUI_HEADERIMAGE_UNBITMAP_NOSTRETCH ".\Installer-Files\UnInstallHeaderImage.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP ".\Installer-Files\WelcomeFinishPage.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP ".\Installer-Files\UnWelcomeFinishPage.bmp"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE ".\Installer-Files\LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section ""

  SetOutPath "$INSTDIR"
  File /r .\qgis_enterprise_15\*.*

;Uninstaller
  WriteUninstaller $INSTDIR\uninstall.exe
  
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${INSTALLATIONNAME}" "DisplayName" "QGIS Enterprise 15"

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${INSTALLATIONNAME}" "UninstallString" "$INSTDIR\uninstall.exe"

  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${INSTALLATIONNAME}" "NoModify" 1

  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${INSTALLATIONNAME}" "NoRepair" 1

  WriteRegStr HKEY_CURRENT_USER "Software\Sourcepole\QGIS Enterprise 15\Qgis" "showTips" "false"
  


;Desktop shortcut
  Delete "$DESKTOP\QGIS Enterprise 15.lnk"
  SetOutPath "$INSTDIR\bin"
  CreateShortCut "$DESKTOP\QGIS Enterprise 15.lnk" "$INSTDIR\bin\qgis.bat" "" "$INSTDIR\apps\qgis\icons\QGIS.ico"
SectionEnd

Section "Uninstall"
  Delete $INSTDIR\uninstall.exe
  RMDir /r "$INSTDIR"
  Delete "$DESKTOP\QGIS Enterprise 15.lnk"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${INSTALLATIONNAME}"
SectionEnd

Section "install"
  WriteRegStr HKCR ".qgs" "" "QGIS Enterprise 15.qgs"
  WriteRegStr HKCR "QGIS Enterprise 15.qgs\DefaultIcon" "" "$INSTDIR\apps\qgis\icons\QGIS.ico"
  WriteRegStr HKCR "QGIS Enterprise.qgs\shell\open\command" "" '"$INSTDIR\bin\qgis.bat" "%1"'
  ${RefreshShellIcons}
SectionEnd 

Function .onInit
  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${INSTALLATIONNAME}" "UninstallString"
  StrCmp $R0 "" done
  
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "${INSTALLATIONNAME} is already installed. $\n Click 'OK' to remove the \
  previous version or 'Cancel' to cancel this upgrade." \
  IDOK uninst
  Abort

 uninst:
  Exec $INSTDIR\uninstall.exe
 done:
FunctionEnd