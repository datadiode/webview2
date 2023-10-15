@echo off
cd "%~dp0"
set ducible.exe="..\UnityDoorstop-BSIPA\BuildUtils\ducible.exe"
for /f "tokens=*" %%G in ('where git.exe') do set git.exe="%%G"
set touch.exe=%git.exe:cmd\git.exe=usr\bin\touch.exe%
REM Read the version number from the resource file
for /f "tokens=1,2" %%G in (..\BrowserProxy\BrowserProxy.rc) do if "%%G" == "FILEVERSION" set FILEVERSION=%%H
echo FILEVERSION=%FILEVERSION:,=.%
%ducible.exe% ..\BrowserProxy\Release\OpennessWebView2.dll
for /f delims^=^"^ tokens^=2 %%G in (OpennessWebView2.ddf) do %touch.exe% -t 20230%FILEVERSION:,=% %~dp0%%G
set FILEVERSION=%FILEVERSION:,0=,%
REM Create the ZIP
del "OpennessWebView2.zip"
"%ProgramFiles%\7-zip\7z.exe" a -mx9 "OpennessWebView2.zip" "..\BrowserProxy\Release\*.dll" "..\LICENSE.md" "setup.hta" "demo.hta"
REM Create the CAB
REM makecab /f OpennessWebView2.ddf
REM Create the SFX
"makesfx.exe" ^
/zip="OpennessWebView2.zip" ^
/sfx="OpennessWebView2.exe" ^
/title="OpennessWebView2 Control" ^
/website="https://github.com/datadiode/webview2" ^
/intro="Version: %FILEVERSION:,=.%\n\nThis ActiveX control embeds Microsoft Edge WebView2\ninto SIMATIC WinCC Runtime Advanced.\n\nThe Microsoft Edge WebView2 Runtime needs to be\ninstalled to use this control." ^
/runelevated ^
/overwrite ^
/exec="\"$sysdir$\mshta.exe\" \"$targetdir$\setup.hta?$dialogplacement$\" $cmdline$" ^
/defaultpath="$programfiles$\OpennessWebView2"
