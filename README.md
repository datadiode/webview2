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
