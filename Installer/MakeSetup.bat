@echo off
REM Read the version number from the resource file
for /F "tokens=1,2" %%G in (%~dp0..\BrowserProxy\BrowserProxy.rc) do if "%%G" == "FILEVERSION" set FILEVERSION=%%H
echo FILEVERSION=%FILEVERSION:,=.%
REM Create the ZIP
del "%~dp0OpennessWebView2.zip"
"%ProgramFiles%\7-zip\7z.exe" a -mx9 "%~dp0OpennessWebView2.zip" "%~dp0..\BrowserProxy\Release\*.dll" "%~dp0..\LICENSE.md" "%~dp0setup.bat"
REM Create the SFX
"%~dp0makesfx.exe" ^
/zip="%~dp0OpennessWebView2.zip" ^
/sfx="%~dp0OpennessWebView2.exe" ^
/title="OpennessWebView2 Control" ^
/website="https://github.com/datadiode/webview2" ^
/intro="Version: %FILEVERSION:,=.%\n\nThis ActiveX control embeds Microsoft Edge WebView2\ninto SIMATIC WinCC Runtime Advanced.\n\nThe Microsoft Edge WebView2 Runtime needs to be\ninstalled to use this control." ^
/runelevated ^
/overwrite ^
/exec="\"cmd.exe\" /c setup.bat $cmdline$" ^
/defaultpath="$programfiles$\OpennessWebView2"
