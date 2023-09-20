@echo off
cd "%~dp0"
REM Read the version number from the resource file
for /f "tokens=1,2" %%G in (..\BrowserProxy\BrowserProxy.rc) do if "%%G" == "FILEVERSION" set FILEVERSION=%%H
echo FILEVERSION=%FILEVERSION:,=.%
REM Create the CAB
makecab /f OpennessWebView2.ddf
REM Create the SFX
"makesfx.exe" ^
/zip="disk1\OpennessWebView2.cab" ^
/sfx="OpennessWebView2.exe" ^
/title="OpennessWebView2 Control" ^
/website="https://github.com/datadiode/webview2" ^
/intro="Version: %FILEVERSION:,=.%\n\nThis ActiveX control embeds Microsoft Edge WebView2\ninto SIMATIC WinCC Runtime Advanced.\n\nThe Microsoft Edge WebView2 Runtime needs to be\ninstalled to use this control." ^
/runelevated ^
/overwrite ^
/exec="\"cmd.exe\" /c setup.bat $cmdline$" ^
/defaultpath="$programfiles$\OpennessWebView2"
