@echo off
if /i not "%~x2" == ".cab" goto :register
expand.exe -f:* %2 .
echo [OpennessWebView2.dll] > OpennessWebView2.ini
echo BrowserExecutableFolder=%~dp0%~n2 >> OpennessWebView2.ini
:register
set options=
for %%x in (%*) do if /i "%%x" == "/autoextract" set options=/s
for %%x in (system32 syswow64) do if exist "%SystemRoot%\%%x" set SystemLeaf=%%x
"%SystemRoot%\%SystemLeaf%\regsvr32.exe" %options% OpennessWebView2.dll
del %0
