;NSIS Modern User Interface installation script for gKamus (http://gkamus.sourceforge.net)
;written by Ardhan Madras (ajhwb@knac.com)

;-------------------------------
;Definition

!define GKAMUS_VERSION  "0.3"
!define GKAMUS_PRODUCT_VERSION  "0.3.0.0"
!define GKAMUS_PACKAGE_DIR  "Z:\tmp\gkamus-0.3"
!define GKAMUS_SOURCE_DIR   "Z:\projects\gkamus-0.3\win32"
!define GKAMUS_REG_KEY  "Software\gKamus"
!define GKAMUS_UNINST_EXE   "gkamus-uninst.exe"
!define GKAMUS_UNINST_KEY   "Software\Microsoft\Windows\CurrentVersion\Uninstall\gKamus"

;gKamus 0.1 using Inno Setup Compiler
!define GKAMUS_OLD_UNINST_KEY   "Software\Microsoft\Windows\CurrentVersion\Uninstall\gKamus_is1"

;--------------------------------
;Include

  !include "MUI2.nsh"
  !include "WinVer.nsh"

;-------------------------------
;Function after installation

  ;Function .onGUIEnd
  ;  MessageBox MB_OK|MB_ICONINFORMATION "Untuk membuat gKamus lebih baik lagi, \
  ;      anda dapat berpartisipasi mengembangkan program ini, lihat file TODO \
  ;          untuk informasi lebih lanjut."
  ;  ExecShell "open" "$INSTDIR\share\doc\todo.txt"
  ;FunctionEnd

;--------------------------------
;General

  ;Name and file
  Name "gKamus ${GKAMUS_VERSION}"
  
  OutFile "gkamus-${GKAMUS_VERSION}-setup.exe"
  
  CRCCheck force
  
  ;Default compressor, solid lzma is the smallest size, but take times ;p
  ;SetCompressor /SOLID lzma
  
  ;Version
  VIAddVersionKey "FileDescription" "gKamus Installer (w/ GTK+ Integration)"
  VIProductVersion "${GKAMUS_PRODUCT_VERSION}"
  VIAddVersionKey "ProductName" "gKamus"
  VIAddVersionKey "FileVersion" "${GKAMUS_VERSION}"
  VIAddVersionKey "ProductVersion" "${GKAMUS_VERSION}"

  VIAddVersionKey "LegalCopyright" ""

  ;Default installation folder
  InstallDir "$PROGRAMFILES\gKamus"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "${GKAMUS_REG_KEY}" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel user

;--------------------------------
;Init function
;Check for another installer instances
Function .onInit

  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "mutex") i .r1 ?e'
  Pop $R0

  StrCmp $R0 0 +3
    MessageBox MB_OK|MB_ICONEXCLAMATION "Setup gKamus ${GKAMUS_VERSION} telah ada yang dijalankan."
    Abort

  ;Uninstall previous version if any
  ReadRegStr $0 HKLM "${GKAMUS_OLD_UNINST_KEY}" "UninstallString"
  StrCmp $0 "" done try_uninstall

  try_uninstall:
    MessageBox MB_YESNO|MB_ICONQUESTION "Setup mendeteksi versi gKamus yang lama telah ter-install, \
        uninstall versi ini? (Pastikan anda telah mem-backup data kamus yang lama)" IDYES Yes IDNO No
    Yes:
      ExecWait "$0 /SILENT" ;see Inno Setup Compiler manual
      DeleteRegKey HKLM "${GKAMUS_OLD_UNINST_KEY}"
      Goto done

    No:
      MessageBox MB_OK|MB_ICONEXCLAMATION "Untuk meng-install gKamus ${GKAMUS_VERSION}, \
        versi sebelumya harus di uninstall terlebih dahulu."
      Abort

  done:
  
FunctionEnd

;--------------------------------
;Interface Configuration

  !define MUI_ICON ".\gkamus.ico"
  !define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\win.bmp"
  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\win.bmp"
  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  ;Finish Page config
  !define MUI_COMPONENTSPAGE_SMALLDESC
  !define MUI_FINISHPAGE_NOAUTOCLOSE
  !define MUI_FINISHPAGE_RUN "$INSTDIR\gkamus.exe"
  !define MUI_FINISHPAGE_RUN_CHECKED

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "${GKAMUS_PACKAGE_DIR}\share\doc\copying.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "Indonesian" ;thx to Ariel825010106@yahoo.com

;--------------------------------
;Installer Sections

;Package
Section "!gKamus (w/ GTK+ 2.12.9)" SecPackage
  
  SectionIn 1 RO ;core program & must be installed, disable section modification
  
  SetOutPath "$INSTDIR"
  
  ;Package files
  File /r "${GKAMUS_PACKAGE_DIR}\*.*"
  
  ;Store installation folder
  WriteRegStr HKLM "${GKAMUS_REG_KEY}" "" $INSTDIR
  WriteRegStr HKLM "${GKAMUS_REG_KEY}" "Version" "${GKAMUS_VERSION}"
  ;Write uninstallation reg
  WriteRegStr HKLM "${GKAMUS_UNINST_KEY}" "UninstallString" "$INSTDIR\${GKAMUS_UNINST_EXE}"
  WriteRegStr HKLM "${GKAMUS_UNINST_KEY}" "DisplayName" "gKamus ${GKAMUS_VERSION}"
  WriteRegStr HKLM "${GKAMUS_UNINST_KEY}" "DisplayVersion" "${GKAMUS_VERSION}"
  WriteRegStr HKLM "${GKAMUS_UNINST_KEY}" "HelpLink" "http://gkamus.sourceforge.net"
  WriteRegDWORD HKLM "${GKAMUS_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${GKAMUS_UNINST_KEY}" "NoRepair" 1
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\${GKAMUS_UNINST_EXE}"

SectionEnd

;Source code
Section "Kode Program (Dev-C++)" SecSourceCode

  SetOutPath "$INSTDIR\share\src"
  
  File /r "${GKAMUS_SOURCE_DIR}\*.*"

SectionEnd

;Start menu Shortcut
Section "Start Menu Shortcut" SecStartmenuShortcut
  
  SetOutPath "$INSTDIR"
  
  ${If} ${IsNT}
    ;set to "All Users",  %ALLUSERSPROFILE% \Desktop if we are in NT
    SetShellVarContext all
  ${Endif}

  CreateDirectory "$SMPROGRAMS\gKamus"
  
  CreateShortcut "$SMPROGRAMS\gKamus\gKamus.lnk" "$INSTDIR\gkamus.exe" "" "" "" "" "" "Kamus Bahasa Inggris - Indonesia"
  CreateShortcut "$SMPROGRAMS\gKamus\Readme.lnk" "$INSTDIR\share\doc\readme.txt"
  CreateShortcut "$SMPROGRAMS\gKamus\Changelog.lnk" "$INSTDIR\share\doc\changelog.txt"
  CreateShortcut "$SMPROGRAMS\gKamus\Lisensi.lnk" "$INSTDIR\share\doc\copying.txt"
  CreateShortcut "$SMPROGRAMS\gKamus\Kode Program.lnk" "$INSTDIR\share\src"
                 
SectionEnd

;Desktop shortcut

Section "Desktop Shortcut" SecDesktopShortcut
  
  SetOutPath "$INSTDIR"
  
  ${If} ${IsNT}
    SetShellVarContext all
  ${Endif}
  
  CreateShortcut "$DESKTOP\gKamus.lnk" "$INSTDIR\gkamus.exe" "" "" "" "" "" "Kamus Bahasa Inggris - Indonesia"

SectionEnd

;--------------------------------
;Section Descriptions

  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecPackage} "gKamus dengan integrasi GTK+ 2.12.9 runtime"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecSourceCode} "Kode program gKamus untuk Dev-C++ IDE"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecStartmenuShortcut} "Buat shortcut di Start Menu"
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktopShortcut} "Buat shortcut di Desktop"
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ${If} ${IsNT}
    SetShellVarContext all
  ${Endif}
  
  ;Shortcut
  RMDir /r "$SMPROGRAMS\gKamus"
  Delete "$DESKTOP\gKamus.lnk"

  ;Registry
  DeleteRegKey HKLM "${GKAMUS_REG_KEY}"
  DeleteRegKey HKLM "${GKAMUS_UNINST_KEY}"
  
  ;File removal
  RMDir /r "$INSTDIR\etc"
  RMDir /r "$INSTDIR\lib"
  RMDir /r "$INSTDIR\share\doc"
  RMDir /r "$INSTDIR\share\pixmaps"
  RMDir /r "$INSTDIR\share\src"
  RMDir /r "$INSTDIR\share\themes"
  RMDir /r "$INSTDIR\share\xml"
  Delete "$INSTDIR\share\data\gkamus.ui"
  Delete "$INSTDIR\share\data\irregular-verbs"
  Delete "$INSTDIR\freetype6.dll"
  Delete "$INSTDIR\${GKAMUS_UNINST_EXE}"
  Delete "$INSTDIR\gkamus.exe"
  Delete "$INSTDIR\iconv.dll"
  Delete "$INSTDIR\intl.dll"
  Delete "$INSTDIR\jpeg62.dll"
  Delete "$INSTDIR\libatk-1.0-0.dll"
  Delete "$INSTDIR\libcairo-2.dll"
  Delete "$INSTDIR\libfontconfig-1.dll"
  Delete "$INSTDIR\libfreetype-6.dll"
  Delete "$INSTDIR\libgdk-win32-2.0-0.dll"
  Delete "$INSTDIR\libgdk_pixbuf-2.0-0.dll"
  Delete "$INSTDIR\libglib-2.0-0.dll"
  Delete "$INSTDIR\libgmodule-2.0-0.dll"
  Delete "$INSTDIR\libgobject-2.0-0.dll"
  Delete "$INSTDIR\libgtk-win32-2.0-0.dll"
  Delete "$INSTDIR\libpango-1.0-0.dll"
  Delete "$INSTDIR\libpangocairo-1.0-0.dll"
  Delete "$INSTDIR\libpangoft2-1.0-0.dll"
  Delete "$INSTDIR\libpangowin32-1.0-0.dll"
  Delete "$INSTDIR\libpng12.dll"
  Delete "$INSTDIR\librsvg-2-2.dll"
  Delete "$INSTDIR\libtiff3.dll"
  Delete "$INSTDIR\libxml2.dll"
  Delete "$INSTDIR\zlib1.dll"
  
  MessageBox MB_YESNO|MB_ICONQUESTION "Apakah semua file kamus akan dihapus juga?" IDYES Yes IDNO done
  
  Yes:
    ;RMDir /r "$INSTDIR" ;DANGER!!!
    RMDir /r "$INSTDIR\share"
    Goto done
    
  done:
  
SectionEnd
