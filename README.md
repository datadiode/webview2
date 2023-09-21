# OpennessWebView2 Control

[![StandWithUkraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/badges/StandWithUkraine.svg)](https://github.com/vshymanskyy/StandWithUkraine/blob/main/docs/README.md)

This ActiveX control embeds [Microsoft Edge WebView2](https://docs.microsoft.com/en-us/microsoft-edge/webview2/) into SIMATIC WinCC Runtime Advanced.

The [Microsoft Edge WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/) needs to be installed to use this control.

![html5test](https://github.com/datadiode/srellcom/assets/10423465/89f04c05-9aee-41d7-b9e8-c87aa1cddb1e)

```VBS
Sub VBFunction_1()
    HmiRuntime.Screens("Screen_1").ScreenItems("WebView2_1").Navigate "https://html5test.com"
End Sub
```
![OpennessWebView2](https://github.com/datadiode/srellcom/assets/10423465/6877b41e-69d2-4446-ba0a-ff7be0d4e999)

## Installation

### Regular installation procedure
![installation_procedure_with_shared_version_of_webview2_runtime](https://github.com/datadiode/srellcom/assets/10423465/8da4ca35-5b57-408c-89ba-10e8f46955d9)

### Installation procedure with fixed version of WebView2 Runtime
![installation_procedure_with_fixed_version_of_webview2_runtime](https://github.com/datadiode/srellcom/assets/10423465/94dfe15e-50d9-4691-b208-4fe5a4af29f6)

Fixed versions of the WebView2 Runtime can be obtained from [here](https://github.com/westinyang/WebView2RuntimeArchive/releases).

## Building from source

### Visual Studio

Install [Visual Studio 2022](https://visualstudio.microsoft.com/vs/) with the following:
* Workloads
  * Desktop development with C++
  * Python development
* Individual components
  * Windows 10 SDK (10.0.19041.0)
  * Python language support
  * Python 3 64-bit (3.9.13)
