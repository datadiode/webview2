;Based on https://nsis.sourceforge.io/Examples/Modern%20UI/WelcomeFinish.nsi

  !include "MUI2.nsh"
  !include "FileFunc.nsh"
  !include "StrFunc.nsh"

  ${Using:StrFunc} StrLoc

;--------------------------------
;General

  ;Name and file
  Name "OpennessWebView2 Control"
  OutFile "OpennessWebView2.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES32\OpennessWebView2"

  RequestExecutionLevel admin

  ; OWV2_UNINSTALL_KEY is where information is stored for Add/Remove Programs
  !define OWV2_UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpennessWebView2"

  ; Set compression type for the installer data.
  SetCompressor /SOLID lzma

  ShowInstDetails show
  ShowUnInstDetails show

;--------------------------------
;Interface Settings

  !define MUI_ICON "..\BrowserProxy\EdgeWebView2.ico"
  !define MUI_UNICON "..\BrowserProxy\EdgeWebView2.ico"
  !define MUI_WELCOMEPAGE_TEXT "Version: 1.1.2.4$\r$\n$\r$\nThis ActiveX control embeds Microsoft Edge WebView2$\r$\ninto SIMATIC WinCC Runtime Advanced.$\r$\n$\r$\nThe Microsoft Edge WebView2 Runtime needs to be$\r$\ninstalled to use this control."

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  ;!insertmacro MUI_PAGE_LICENSE "${NSISDIR}\Docs\Modern UI\License.txt"
  ;!insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  ;!insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;https://nsis.sourceforge.io/Examples/silent.nsi
Function .onInit
  ${GetParameters} $R0
  ${GetOptionsS} $R0 "/autoextract" $0
  IfErrors +2 0
  SetSilent silent
  ClearErrors
FunctionEnd

;--------------------------------
;Installer Sections

Section "Install" SecInstall
  ; Set install directory
  SetOutPath "$INSTDIR"

  File "..\LICENSE.md"
  File "..\BrowserProxy\Release\OpennessWebView2.dll"
  File "..\BrowserProxy\Release\WebView2Loader.dll"

  ${GetParameters} $R0
  ${GetOptions} "/cab $R0" "/cab" $R1
  ${StrLoc} $0 $R1 ".cab" "<"
  StrCmp $0 "0" 0 CabFileEnd
    ${GetBaseName} $R1 $R0
    FileOpen $1 "$OUTDIR\OpennessWebView2.ini" w
    FileWrite $1 "[OpennessWebView2.dll]$\r$\nBrowserExecutableFolder=$OUTDIR\$R0$\r$\n"
    FileClose $1
    ExecWait '"expand.exe" -f:* "$R1" "$OUTDIR"'
  CabFileEnd:

  ; Create uninstaller
  WriteUninstaller "$OUTDIR\uninstall_OpennessWebView2.exe"

  ; Create Add/Remove programs keys
  WriteRegStr HKLM "${OWV2_UNINSTALL_KEY}" "DisplayName" "OpennessWebView2 Control"
  WriteRegStr HKLM "${OWV2_UNINSTALL_KEY}" "UninstallString" "$OUTDIR\uninstall_OpennessWebView2.exe"
  WriteRegDWORD HKLM "${OWV2_UNINSTALL_KEY}" "NoModify" 0x00000001
  WriteRegDWORD HKLM "${OWV2_UNINSTALL_KEY}" "NoRepair" 0x00000001

  ; Register the DLL
  RegDLL "$OUTDIR\OpennessWebView2.dll"
  
  IfErrors 0 ErrorsEnd
    DetailPrint "DLL registration failed!"
	Abort
  ErrorsEnd:
SectionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall" SecUninstall

  UnRegDLL "$INSTDIR\OpennessWebView2.dll"

  RMDir /r /REBOOTOK $INSTDIR

  DeleteRegKey HKLM "${OWV2_UNINSTALL_KEY}"

SectionEnd
