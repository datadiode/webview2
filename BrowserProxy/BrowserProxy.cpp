#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#pragma warning (disable:26812)
#pragma warning (disable:28251)

#include "platform.h"
#include "resource.h"
#include "shlwapi.h"
#include "shlobj.h"
#include "wininet.h"
#include "atlbase.h"
#include "atlcom.h"
#include "atlctl.h"
#include "atlstr.h"
#include "atlsafe.h"
#include "exdispid.h"
#include "wrl.h"
#include "WebView2.h"
#include "VoiceTrainerProxy.h"
#include "SettingsRequestHandler.h"
#include "BrowserProxy_i.h"
#include "BrowserProxy.h"

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_ThereEdgeWebBrowser, BrowserProxyModule)
END_OBJECT_MAP()

CComModule g_AtlModule;
HINSTANCE g_Instance;

void Log(const WCHAR *format, ...)
{
#ifdef THERE_LOGGING
    va_list args;
    va_start(args, format);

    WCHAR buff[1000];
    _vsnwprintf_s(buff, _countof(buff), format, args);

    FILE *file = nullptr;
    if (fopen_s(&file, "Debug.log", "a") == 0)
    {
        vfwprintf_s(file, format, args);
        fflush(file);
        fclose(file);
    }

    va_end(args);
#endif
}

void ReportError(HWND wnd, const WCHAR *format, ...)
{
    va_list args;
    va_start(args, format);

    WCHAR buff[1000];
    _vsnwprintf_s(buff, _countof(buff), format, args);

    va_end(args);

    MessageBox(wnd, buff, L"ThereEdge Error", MB_OK | MB_ICONERROR);
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_Instance = hInstance;
        g_AtlModule.Init(ObjectMap, (HINSTANCE)hInstance, &LIBID_BrowserProxyLib);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        g_AtlModule.Term();
    }
    return TRUE;
}

STDAPI DllCanUnloadNow()
{
    return g_AtlModule.DllCanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return g_AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
    return g_AtlModule.DllRegisterServer();
}

STDAPI DllUnregisterServer()
{
    return g_AtlModule.DllUnregisterServer();
}

STDAPI DllInstall(BOOL bInstall, _In_opt_ LPCWSTR pszCmdLine)
{
    HRESULT hr = E_FAIL;

    if (pszCmdLine != nullptr)
    {
        if (_wcsnicmp(pszCmdLine, L"user", 4) == 0)
            ATL::AtlSetPerUserRegistration(true);
    }

    if (bInstall)
    {
        hr = DllRegisterServer();
        if (FAILED(hr))
            DllUnregisterServer();
    }
    else
    {
        hr = DllUnregisterServer();
    }

    return hr;
}

BrowserProxyModule::BrowserProxyModule():
    m_url(),
    m_proxyVersion(),
    m_browserVersion(),
    m_userDataFolder(),
    m_browserEvents(),
    m_environment(),
    m_controller(),
    m_view(),
    m_newWindowDeferral(),
    m_newWindowArgs(),
    m_voiceTrainerProxy(),
    m_settingsRequestHandler(),
    m_navigationStartingToken(),
    m_navigationCompletedToken(),
    m_newWindowRequestedToken(),
    m_sourceChangedToken(),
    m_historyChangedToken(),
    m_documentTitleChangedToken(),
    m_webResourceRequestedToken(),
    m_webMessageReceivedToken(),
    m_windowCloseRequestedToken(),
    m_domContentLoadedToken(),
    m_downloadStartingToken(),
    m_ready(false),
    m_visible(true)
{
    m_bWindowOnly = TRUE;
}

BrowserProxyModule::~BrowserProxyModule()
{
    if (m_view != nullptr)
    {
        m_view->remove_NavigationStarting(m_navigationStartingToken);
        m_view->remove_NavigationCompleted(m_navigationCompletedToken);
        m_view->remove_NewWindowRequested(m_newWindowRequestedToken);
        m_view->remove_SourceChanged(m_sourceChangedToken);
        m_view->remove_HistoryChanged(m_historyChangedToken);
        m_view->remove_DocumentTitleChanged(m_documentTitleChangedToken);
        m_view->remove_WebResourceRequested(m_webResourceRequestedToken);
        m_view->remove_WebMessageReceived(m_webMessageReceivedToken);
        m_view->remove_WindowCloseRequested(m_windowCloseRequestedToken);
        m_view->remove_DOMContentLoaded(m_domContentLoadedToken);
        m_view->remove_DownloadStarting(m_downloadStartingToken);
    }

    if (m_controller != nullptr)
        m_controller->Close();

    if (m_voiceTrainerProxy != nullptr)
    {
        m_voiceTrainerProxy->Close();
        m_voiceTrainerProxy.Release();
    }

    if (m_settingsRequestHandler != nullptr)
        m_settingsRequestHandler.Release();
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    return S_OK;
}

bool BrowserProxyModule::InIDE()
{
    BOOL bUserMode = TRUE;
    return SUCCEEDED(GetAmbientUserMode(bUserMode)) && !bUserMode;
}

HRESULT BrowserProxyModule::OnDraw(ATL_DRAWINFO& di)
{
    if (InIDE())
    {
        RECT& rc = *(RECT*)di.prcBounds;
        Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);

        SetTextAlign(di.hdcDraw, TA_LEFT | TA_BASELINE);

        int x = 5;
        int y = 15;

        CComBSTR text;
        text += L"OpennessWebView2 v";
        text += m_proxyVersion;
        text += L" Control - Design Mode";

        MoveToEx(di.hdcDraw, x, y, nullptr);
        TextOut(di.hdcDraw, x, y, text, text.Length());
    }

    return S_OK;
}

LRESULT BrowserProxyModule::OnCreate(UINT, WPARAM, LPARAM, BOOL&)
{
    if (HRSRC source = FindResource(g_Instance, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION))
    {
        if (HGLOBAL resource = LoadResource(g_Instance, source))
        {
            if (void *data = LockResource(resource))
            {
                VS_FIXEDFILEINFO *version = nullptr;
                UINT size = 0;
                if (VerQueryValue(data, L"\\", (void**)&version, &size) && version != nullptr && size >= sizeof(VS_FIXEDFILEINFO))
                {
                    WCHAR value[100];
                    _snwprintf_s(value, _countof(value), L"%u.%u.%u.%u",
                                    version->dwFileVersionMS >> 16 & 0xFFFF,
                                    version->dwFileVersionMS & 0xFFFF,
                                    version->dwFileVersionLS >> 16 & 0xFFFF,
                                    version->dwFileVersionLS & 0xFFFF);
                    m_proxyVersion = value;
                }
            }
        }
    }

    if (!InIDE())
    {
        WCHAR executingFile[MAX_PATH];
        WCHAR executingFileFull[MAX_PATH];
        LPWSTR executingFileName = nullptr;

        GetModuleFileName(g_Instance, executingFile, _countof(executingFile));
        GetFullPathName(executingFile, _countof(executingFileFull), executingFileFull, &executingFileName);

        WCHAR executingHost[MAX_PATH];
        WCHAR executingHostFull[MAX_PATH];
        LPWSTR executingHostName = nullptr;

        GetModuleFileName(nullptr, executingHost, _countof(executingHost));
        GetFullPathName(executingHost, _countof(executingHostFull), executingHostFull, &executingHostName);

        WCHAR userDataFolder[MAX_PATH] = { 0 };
        WCHAR browserExecutableFolder[MAX_PATH] = { 0 };

        if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, userDataFolder)) && userDataFolder[0] != 0)
        {
            if (PathAppend(userDataFolder, executingFileName))
            {
                CreateDirectory(userDataFolder, nullptr);
                if (PathAppend(userDataFolder, executingHostName))
                {
                    CreateDirectory(userDataFolder, nullptr);
                    m_userDataFolder = userDataFolder;
                }
            }
        }

        PathRenameExtension(executingHost, L".ini");
        if (GetPrivateProfileString(executingFileName, L"BrowserExecutableFolder", nullptr, browserExecutableFolder, _countof(browserExecutableFolder), executingHost) == 0)
        {
            PathRenameExtension(executingFile, L".ini");
            GetPrivateProfileString(executingFileName, L"BrowserExecutableFolder", nullptr, browserExecutableFolder, _countof(browserExecutableFolder), executingFile);
        }

        HRESULT rc = CreateCoreWebView2EnvironmentWithOptions(browserExecutableFolder, m_userDataFolder, nullptr, this);
        if (FAILED(rc))
        {
            switch (rc)
            {
                case HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED):
                {
                    ReportError(m_hWnd, L"The Edge application path was used in the browser executable folder.");
                    break;
                }
                case HRESULT_FROM_WIN32(ERROR_INVALID_STATE):
                {
                    ReportError(m_hWnd, L"The specified options do not match the options of the WebViews that are currently running in the shared browser process.");
                    break;
                }
                case HRESULT_FROM_WIN32(ERROR_DISK_FULL):
                {
                    ReportError(m_hWnd, L"Too many previous WebView2 Runtime versions exist.");
                    break;
                }
                case HRESULT_FROM_WIN32(ERROR_PRODUCT_UNINSTALLED):
                {
                    ReportError(m_hWnd, L"The Webview depends upon an installed WebView2 Runtime version and it is uninstalled.");
                    break;
                }
                case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
                {
                    ReportError(m_hWnd, L"Could not find an Edge installation.");
                    break;
                }
                case HRESULT_FROM_WIN32(ERROR_FILE_EXISTS):
                {
                    ReportError(m_hWnd, L"The user data folder cannot be created because a file with the same name already exists.");
                    break;
                }
                case E_ACCESSDENIED:
                {
                    ReportError(m_hWnd, L"Unable to create the user data folder.");
                    break;
                }
                case E_FAIL:
                {
                    ReportError(m_hWnd, L"The Edge runtime is unable to start.");
                    break;
                }
                default:
                {
                    ReportError(m_hWnd, L"Unable to create a WebView2 environment: 0x%08lx", rc);
                    break;
                }
            }
        }
    }

    return 0;
}

LRESULT BrowserProxyModule::OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
    if (m_controller != nullptr)
        m_controller->Close();

    bHandled = FALSE;
    return 0;
}

LRESULT BrowserProxyModule::OnSize(UINT, WPARAM, LPARAM, BOOL&)
{
    if (m_controller)
    {
        RECT bounds;
        GetClientRect(&bounds);
        m_controller->put_Bounds(bounds);
    }
    return 0;
}

LRESULT BrowserProxyModule::OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL&)
{
    return 1;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::GoBack()
{
    if (m_view == nullptr)
        return E_NOT_VALID_STATE;

    BOOL value = false;
    if (FAILED(m_view->get_CanGoBack(&value)))
        return E_FAIL;

    if (value && FAILED(m_view->GoBack()))
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::GoForward()
{
    if (m_view == nullptr)
        return E_NOT_VALID_STATE;

    BOOL value = false;
    if (FAILED(m_view->get_CanGoForward(&value)))
        return E_FAIL;

    if (value && FAILED(m_view->GoForward()))
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::GoHome()
{
    if (m_view == nullptr)
        return E_NOT_VALID_STATE;

    m_url = L"https://webapps.prod.there.com/therecentral/there_central.xml?fromClient=1";

    if (FAILED(Navigate()))
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::Navigate(BSTR URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    m_url = URL;

    if (FAILED(Navigate()))
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::Refresh()
{
    if (m_view == nullptr)
        return E_NOT_VALID_STATE;

    if (FAILED(m_view->Reload()))
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::Refresh2(VARIANT *Level)
{
    return Refresh();
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::Stop()
{
    if (m_view == nullptr)
        return E_NOT_VALID_STATE;

    if (FAILED(m_view->Stop()))
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::get_LocationURL(BSTR *LocationURL)
{
    if (LocationURL == nullptr)
        return E_INVALIDARG;

    if (m_view == nullptr)
        return E_NOT_VALID_STATE;

    WCHAR *url = nullptr;
    if (FAILED(m_view->get_Source(&url)) || url == nullptr)
        return E_FAIL;

    *LocationURL = SysAllocString(url);
    CoTaskMemFree(url);

    if (*LocationURL == nullptr)
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::get_Visible(VARIANT_BOOL *pBool)
{
    if (pBool == nullptr)
        return E_INVALIDARG;

    *pBool = m_visible ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::put_Visible(VARIANT_BOOL Value)
{
    if (FAILED(SetVisibility(Value != VARIANT_FALSE)))
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::Navigate2(VARIANT *URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    if (URL == nullptr || URL->vt != VT_BSTR)
        return E_INVALIDARG;

    m_url = URL->bstrVal;

    if (FAILED(Navigate()))
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::get_RegisterAsBrowser(VARIANT_BOOL *pbRegister)
{
    if (pbRegister == nullptr)
        return E_INVALIDARG;

    *pbRegister = VARIANT_TRUE;
    return S_OK;

}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::put_RegisterAsBrowser(VARIANT_BOOL bRegister)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::Invoke(HRESULT errorCode, ICoreWebView2Environment *environment)
{
    if (environment == nullptr)
        return E_INVALIDARG;

    {
        WCHAR *version = nullptr;
        if (FAILED(environment->get_BrowserVersionString(&version)) || version == nullptr)
            return E_FAIL;

        m_browserVersion = version;
        CoTaskMemFree(version);
    }

    m_environment = environment;
    m_environment->CreateCoreWebView2Controller(m_hWnd, this);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE BrowserProxyModule::Invoke(HRESULT errorCode, ICoreWebView2Controller *controller)
{
    if (controller == nullptr)
        return E_INVALIDARG;

    if (FAILED(controller->QueryInterface(&m_controller)) || m_controller == nullptr)
        return E_FAIL;

    CComPtr<ICoreWebView2> view;
    if (FAILED(m_controller->get_CoreWebView2(&view)) || view == nullptr)
        return E_FAIL;

    if (FAILED(view->QueryInterface(&m_view)) || m_view == nullptr)
        return E_FAIL;

    CComPtr<ICoreWebView2Settings> settings;
    if (FAILED(m_view->get_Settings(&settings)) || settings == nullptr)
        return E_FAIL;

    settings->put_AreDevToolsEnabled(true);
    settings->put_AreDefaultContextMenusEnabled(true);
    settings->put_AreDefaultScriptDialogsEnabled(true);
    settings->put_IsBuiltInErrorPageEnabled(true);
    settings->put_IsStatusBarEnabled(false);
    settings->put_IsZoomControlEnabled(true);
    settings->put_AreHostObjectsAllowed(false);
    settings->put_IsScriptEnabled(true);
    settings->put_IsWebMessageEnabled(false);

    RECT bounds;
    GetClientRect(&bounds);
    m_controller->put_Bounds(bounds);

    m_controller->put_ShouldDetectMonitorScaleChanges(false);
    m_controller->put_RasterizationScale(1.0);
    m_controller->put_IsVisible(m_visible);

    m_view->AddWebResourceRequestedFilter(L"https://webapps.prod.there.com/*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT);

    m_view->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
        [this](ICoreWebView2 *sender, ICoreWebView2NavigationStartingEventArgs *args) -> HRESULT
        {
            return OnNavigationStarting(sender, args);
        }
    ).Get(), &m_navigationStartingToken);

    m_view->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>(
        [this](ICoreWebView2 *sender, ICoreWebView2NavigationCompletedEventArgs *args) -> HRESULT
        {
            return OnNavigationCompleted(sender, args);
        }
    ).Get(), &m_navigationCompletedToken);

    m_view->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>(
        [this](ICoreWebView2 *sender, ICoreWebView2NewWindowRequestedEventArgs *args) -> HRESULT
        {
            return OnNewWindowRequested(sender, args);
        }
    ).Get(), &m_newWindowRequestedToken);

    m_view->add_SourceChanged(Callback<ICoreWebView2SourceChangedEventHandler>(
        [this](ICoreWebView2 *sender, ICoreWebView2SourceChangedEventArgs *args) -> HRESULT
        {
            return OnSourceChanged(sender, args);
        }
    ).Get(), &m_sourceChangedToken);

    m_view->add_HistoryChanged(Callback<ICoreWebView2HistoryChangedEventHandler>(
        [this](ICoreWebView2 *sender, IUnknown *args) -> HRESULT
        {
            return OnHistoryChanged(sender);
        }
    ).Get(), &m_historyChangedToken);

    m_view->add_DocumentTitleChanged(Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
        [this](ICoreWebView2 *sender, IUnknown *args) -> HRESULT
        {
            return OnDocumentTitleChanged(sender);
        }
    ).Get(), &m_documentTitleChangedToken);

    m_view->add_WebResourceRequested(Callback<ICoreWebView2WebResourceRequestedEventHandler>(
        [this](ICoreWebView2 *sender, ICoreWebView2WebResourceRequestedEventArgs *args) -> HRESULT
        {
            return OnWebResourceRequested(sender, args);
        }
    ).Get(), &m_webResourceRequestedToken);

    m_view->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [this](ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args) -> HRESULT
        {
            return OnWebMessageReceived(sender, args);
        }
    ).Get(), &m_webMessageReceivedToken);

    m_view->add_WindowCloseRequested(Callback<ICoreWebView2WindowCloseRequestedEventHandler>(
        [this](ICoreWebView2 *sender, IUnknown *args) -> HRESULT
        {
            return OnWindowCloseRequested(sender);
        }
    ).Get(), &m_windowCloseRequestedToken);

    m_view->add_DOMContentLoaded(Callback<ICoreWebView2DOMContentLoadedEventHandler>(
        [this](ICoreWebView2 *sender, ICoreWebView2DOMContentLoadedEventArgs *args) -> HRESULT
        {
            return OnDOMContentLoaded(sender, args);
        }
    ).Get(), &m_domContentLoadedToken);

    m_view->add_DownloadStarting(Callback<ICoreWebView2DownloadStartingEventHandler>(
        [this](ICoreWebView2 *sender, ICoreWebView2DownloadStartingEventArgs *args) -> HRESULT
        {
            return OnDownloadStarting(sender, args);
        }
    ).Get(), &m_downloadStartingToken);

    ProcessDeferral();

    if (FAILED(Navigate()))
        return E_FAIL;

    return S_OK;
}

HRESULT BrowserProxyModule::OnNavigationStarting(ICoreWebView2 *sender,  ICoreWebView2NavigationStartingEventArgs *args)
{
    if (sender == nullptr || args == nullptr)
        return E_INVALIDARG;

    CComVariant vurl = m_url;
    CComVariant vflags = (LONG)0;
    CComVariant vframe = L"";
    CComVariant vpost = L"";
    CComVariant vheaders = L"";
    VARIANT_BOOL vcancel = VARIANT_FALSE;

    VARIANTARG vargs[7];
    vargs[0].vt = VT_BOOL | VT_BYREF;
    vargs[0].pboolVal = &vcancel;
    vargs[1].vt = VT_VARIANT | VT_BYREF;
    vargs[1].pvarVal = &vheaders;
    vargs[2].vt = VT_VARIANT | VT_BYREF;
    vargs[2].pvarVal = &vpost;
    vargs[3].vt = VT_VARIANT | VT_BYREF;
    vargs[3].pvarVal = &vframe;
    vargs[4].vt = VT_VARIANT | VT_BYREF;
    vargs[4].pvarVal = &vflags;
    vargs[5].vt = VT_VARIANT | VT_BYREF;
    vargs[5].pvarVal = &vurl;
    vargs[6].vt = VT_DISPATCH;
    vargs[6].pdispVal = static_cast<IDispatch*>(this);

    DISPPARAMS params;
    params.rgvarg = vargs;
    params.cArgs = _countof(vargs);
    params.cNamedArgs = 0;

    if (FAILED(InvokeBrowserEvent(DISPID_BEFORENAVIGATE2, params)))
        return E_FAIL;

    CComPtr<ICoreWebView2Settings> settings;
    if (FAILED(sender->get_Settings(&settings)) || settings == nullptr)
        return E_FAIL;

    settings->put_IsWebMessageEnabled(false);

    if (m_voiceTrainerProxy != nullptr)
    {
        m_voiceTrainerProxy->Close();
        m_voiceTrainerProxy.Release();
    }

    if (m_settingsRequestHandler != nullptr)
        m_settingsRequestHandler.Release();

    if (vcancel != VARIANT_FALSE)
    {
        args->put_Cancel(true);
        return S_OK;
    }

    if (VoiceTrainerProxy::Validate(m_url))
    {
        CComPtr<VoiceTrainerProxy> voiceTrainerProxy(new VoiceTrainerProxy());
        if (voiceTrainerProxy != nullptr && SUCCEEDED(voiceTrainerProxy->Init(m_hWnd, sender)))
        {
            m_voiceTrainerProxy = voiceTrainerProxy;
            settings->put_IsWebMessageEnabled(true);
        }
    }

    if (SettingsRequestHandler::Validate(m_url))
    {
        CComPtr<SettingsRequestHandler> settingsRequestHandler(new SettingsRequestHandler(m_environment, m_proxyVersion));
        if (settingsRequestHandler != nullptr)
        {
            m_settingsRequestHandler = settingsRequestHandler;
            settings->put_IsWebMessageEnabled(true);
        }
    }

    return S_OK;
}

HRESULT BrowserProxyModule::OnNavigationCompleted(ICoreWebView2 *sender, ICoreWebView2NavigationCompletedEventArgs *args)
{
    if (sender == nullptr || args == nullptr)
        return E_INVALIDARG;

    BOOL success = false;
    args->get_IsSuccess(&success);

    if (success)
    {
        m_ready = true;

        CComVariant vurl = m_url;

        VARIANTARG vargs[2];
        vargs[0].vt = VT_VARIANT | VT_BYREF;
        vargs[0].pvarVal = &vurl;
        vargs[1].vt = VT_DISPATCH;
        vargs[1].pdispVal = static_cast<IDispatch*>(this);

        DISPPARAMS params;
        params.rgvarg = vargs;
        params.cArgs = _countof(vargs);
        params.cNamedArgs = 0;

        if (FAILED(InvokeBrowserEvent(DISPID_DOCUMENTCOMPLETE, params)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT BrowserProxyModule::OnNewWindowRequested(ICoreWebView2 *sender, ICoreWebView2NewWindowRequestedEventArgs *args)
{
    if (sender == nullptr || args == nullptr)
        return E_INVALIDARG;

    IDispatch *vdispatch = nullptr;
    VARIANT_BOOL vcancel = VARIANT_FALSE;

    VARIANTARG vargs[2];
    vargs[0].vt = VT_BOOL | VT_BYREF;
    vargs[0].pboolVal = &vcancel;
    vargs[1].vt = VT_DISPATCH | VT_BYREF;
    vargs[1].ppdispVal = &vdispatch;

    DISPPARAMS params;
    params.rgvarg = vargs;
    params.cArgs = _countof(vargs);
    params.cNamedArgs = 0;

    if (FAILED(InvokeBrowserEvent(DISPID_NEWWINDOW2, params)))
        return E_FAIL;

    if (vcancel)
    {
        if (FAILED(args->put_Handled(true)))
            return E_FAIL;

        return S_OK;
    }

    if (vdispatch != nullptr)
    {
        CComPtr<IThereEdgeWebBrowser2> browser;
        vdispatch->QueryInterface(&browser);
        vdispatch->Release();

        if (browser == nullptr)
            return E_FAIL;

        BrowserProxyModule *module = dynamic_cast<BrowserProxyModule*>(browser.p);
        if (module == nullptr)
            return E_FAIL;

        if (FAILED(module->SetDeferral(args)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT BrowserProxyModule::OnSourceChanged(ICoreWebView2 *sender, ICoreWebView2SourceChangedEventArgs *args)
{
    if (sender == nullptr || args == nullptr)
        return E_INVALIDARG;

    {
        WCHAR *url = nullptr;
        if (FAILED(sender->get_Source(&url)) || url == nullptr)
            return E_FAIL;

        m_url = url;
        CoTaskMemFree(url);
    }

    CComVariant vurl = m_url;

    VARIANTARG vargs[2];
    vargs[0].vt = VT_VARIANT | VT_BYREF;
    vargs[0].pvarVal = &vurl;
    vargs[1].vt = VT_DISPATCH;
    vargs[1].pdispVal = static_cast<IDispatch*>(this);

    DISPPARAMS params;
    params.rgvarg = vargs;
    params.cArgs = _countof(vargs);
    params.cNamedArgs = 0;

    if (FAILED(InvokeBrowserEvent(DISPID_NAVIGATECOMPLETE2, params)))
        return E_FAIL;

    return S_OK;
}

HRESULT BrowserProxyModule::OnHistoryChanged(ICoreWebView2 *sender)
{
    if (sender == nullptr)
        return E_INVALIDARG;

    BOOL value;

    if (SUCCEEDED(sender->get_CanGoBack(&value)))
    {
        VARIANTARG vargs[2];
        vargs[0].vt = VT_BOOL;
        vargs[0].boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
        vargs[1].vt = VT_I4;
        vargs[1].lVal = CSC_NAVIGATEBACK;

        DISPPARAMS params;
        params.rgvarg = vargs;
        params.cArgs = _countof(vargs);
        params.cNamedArgs = 0;

        if (FAILED(InvokeBrowserEvent(DISPID_COMMANDSTATECHANGE, params)))
            return E_FAIL;
    }

    if (SUCCEEDED(sender->get_CanGoForward(&value)))
    {
        VARIANTARG vargs[2];
        vargs[0].vt = VT_BOOL;
        vargs[0].boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
        vargs[1].vt = VT_I4;
        vargs[1].lVal = CSC_NAVIGATEFORWARD;

        DISPPARAMS params;
        params.rgvarg = vargs;
        params.cArgs = _countof(vargs);
        params.cNamedArgs = 0;

        if (FAILED(InvokeBrowserEvent(DISPID_COMMANDSTATECHANGE, params)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT BrowserProxyModule::OnDocumentTitleChanged(ICoreWebView2 *sender)
{
    if (sender == nullptr)
        return E_INVALIDARG;

    CComBSTR btitle;
    {
        WCHAR *title = nullptr;
        if (FAILED(sender->get_DocumentTitle(&title)) || title == nullptr)
            return E_FAIL;

        btitle = title;
        CoTaskMemFree(title);
    }

    VARIANTARG vargs[1];
    vargs[0].vt = VT_BSTR;
    vargs[0].bstrVal = btitle;

    DISPPARAMS params;
    params.rgvarg = vargs;
    params.cArgs = _countof(vargs);
    params.cNamedArgs = 0;

    if (FAILED(InvokeBrowserEvent(DISPID_TITLECHANGE, params)))
        return E_FAIL;

    return S_OK;
}

HRESULT BrowserProxyModule::OnWebResourceRequested(ICoreWebView2 *sender, ICoreWebView2WebResourceRequestedEventArgs *args)
{
    if (sender == nullptr || args == nullptr)
        return E_INVALIDARG;

    ICoreWebView2WebResourceRequest *request = nullptr;
    if (FAILED(args->get_Request(&request)) || request == nullptr)
        return E_FAIL;

    CComBSTR burl;
    {
        WCHAR *url = nullptr;
        if (FAILED(request->get_Uri(&url)) || url == nullptr)
            return E_FAIL;

        burl = url;
        CoTaskMemFree(url);
    }

    WCHAR host[40] = {0};
    URL_COMPONENTS components;
    ZeroMemory(&components, sizeof(components));
    components.dwStructSize = sizeof(components);
    components.lpszHostName = host;
    components.dwHostNameLength = _countof(host);

    if (InternetCrackUrl(burl, 0, ICU_DECODE, &components))
    {
        if (wcscmp(host, L"webapps.prod.there.com") == 0)
        {
            CComPtr<ICoreWebView2CookieManager> cookieManager;
            if (FAILED(m_view->get_CookieManager(&cookieManager)) || cookieManager == nullptr)
                return E_FAIL;

            static WCHAR *domain = L".prod.there.com";
            static WCHAR *path = L"/";

            ForwardCookie(cookieManager, burl, L"av", domain, path);
            ForwardCookie(cookieManager, burl, L"doid", domain, path);
            ForwardCookie(cookieManager, burl, L"ticket", domain, path);
            ForwardCookie(cookieManager, burl, L"ticketssl", domain, path);
            ForwardCookie(cookieManager, burl, L"ticketwlt", domain, path);
            ForwardCookie(cookieManager, burl, L"tv", domain, path);
        }
    }

    if (m_settingsRequestHandler != nullptr && SettingsRequestHandler::Validate(burl))
        return m_settingsRequestHandler->HandleRequest(burl, args, m_hWnd);

    return S_OK;
}

HRESULT BrowserProxyModule::OnWebMessageReceived(ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args)
{
    if (sender == nullptr || args == nullptr)
        return E_INVALIDARG;

    CComBSTR burl;
    {
        WCHAR *url = nullptr;
        if (FAILED(args->get_Source(&url)) || url == nullptr)
            return E_FAIL;

        burl = url;
        CoTaskMemFree(url);
    }

    CComBSTR bcommand;
    CComBSTR bpath;
    CComBSTR bquery;
    {
        WCHAR *command = nullptr;
        if (FAILED(args->TryGetWebMessageAsString(&command)) || command == nullptr)
            return E_FAIL;

        WCHAR *query = wcschr(command, L'?');
        if (query != nullptr)
        {
            *query = 0;
             query++;
        }
        else
        {
            query = command + wcslen(command);
        }

        WCHAR *path = wcschr(command, L'/');
        if (path != nullptr)
        {
            *path = 0;
             path++;
        }
        else
        {
            path = command + wcslen(command);
        }

        bcommand = command;
        bpath = path;
        bquery = query;
        CoTaskMemFree(command);
    }

    if (m_voiceTrainerProxy != nullptr && VoiceTrainerProxy::Validate(burl) && _wcsicmp(bcommand, L"voicetrainer") == 0)
        m_voiceTrainerProxy->ProcessMessage(bpath, bquery);

    if (m_settingsRequestHandler != nullptr && SettingsRequestHandler::Validate(burl) && _wcsicmp(bcommand, L"settings") == 0)
        m_settingsRequestHandler->ProcessMessage(bpath, bquery);

    return S_OK;
}

HRESULT BrowserProxyModule::OnWindowCloseRequested(ICoreWebView2 *sender)
{
    if (sender == nullptr)
        return E_INVALIDARG;

    VARIANT_BOOL vcancel = VARIANT_FALSE;

    VARIANTARG vargs[2];
    vargs[0].vt = VT_BOOL | VT_BYREF;
    vargs[0].pboolVal = &vcancel;
    vargs[1].vt = VT_BOOL;
    vargs[0].boolVal = VARIANT_TRUE;

    DISPPARAMS params;
    params.rgvarg = vargs;
    params.cArgs = _countof(vargs);
    params.cNamedArgs = 0;

    if (FAILED(InvokeBrowserEvent(DISPID_WINDOWCLOSING, params)))
        return E_FAIL;

    return S_OK;
}

HRESULT BrowserProxyModule::OnDOMContentLoaded(ICoreWebView2 *sender, ICoreWebView2DOMContentLoadedEventArgs *args)
{
    if (sender == nullptr || args == nullptr)
        return E_INVALIDARG;

    CComBSTR burl;
    {
        WCHAR *url = nullptr;
        if (FAILED(sender->get_Source(&url)) || url == nullptr)
            return E_FAIL;

        burl = url;
        CoTaskMemFree(url);
    }

    WCHAR host[40] = {0};
    URL_COMPONENTS components;
    ZeroMemory(&components, sizeof(components));
    components.dwStructSize = sizeof(components);
    components.lpszHostName = host;
    components.dwHostNameLength = _countof(host);

    if (InternetCrackUrl(burl, 0, ICU_DECODE, &components))
    {
        if (wcscmp(host, L"webapps.prod.there.com") == 0)
            ApplyScript(sender, IDR_COUPLING);
    }

    return S_OK;
}

HRESULT BrowserProxyModule::OnDownloadStarting(ICoreWebView2 *sender, ICoreWebView2DownloadStartingEventArgs *args)
{
    if (sender == nullptr || args == nullptr)
        return E_INVALIDARG;

    CComBSTR burl;
    {
        CComPtr<ICoreWebView2DownloadOperation> operation;
        if (FAILED(args->get_DownloadOperation(&operation)) || operation == nullptr)
            return E_FAIL;

        WCHAR *url = nullptr;
        if (FAILED(operation->get_Uri(&url)) || url == nullptr)
            return E_FAIL;

        burl = url;
        CoTaskMemFree(url);
    }

    CComVariant vurl = burl;
    CComVariant vflags = (LONG)0;
    CComVariant vframe = L"";
    CComVariant vpost = L"";
    CComVariant vheaders = L"";
    VARIANT_BOOL vcancel = VARIANT_FALSE;

    VARIANTARG vargs[7];
    vargs[0].vt = VT_BOOL | VT_BYREF;
    vargs[0].pboolVal = &vcancel;
    vargs[1].vt = VT_VARIANT | VT_BYREF;
    vargs[1].pvarVal = &vheaders;
    vargs[2].vt = VT_VARIANT | VT_BYREF;
    vargs[2].pvarVal = &vpost;
    vargs[3].vt = VT_VARIANT | VT_BYREF;
    vargs[3].pvarVal = &vframe;
    vargs[4].vt = VT_VARIANT | VT_BYREF;
    vargs[4].pvarVal = &vflags;
    vargs[5].vt = VT_VARIANT | VT_BYREF;
    vargs[5].pvarVal = &vurl;
    vargs[6].vt = VT_DISPATCH;
    vargs[6].pdispVal = static_cast<IDispatch*>(this);

    DISPPARAMS params;
    params.rgvarg = vargs;
    params.cArgs = _countof(vargs);
    params.cNamedArgs = 0;

    if (FAILED(InvokeBrowserEvent(DISPID_BEFORENAVIGATE2, params)))
        return E_FAIL;

    if (vcancel != VARIANT_FALSE)
    {
        args->put_Cancel(true);
        return S_OK;
    }

    return S_OK;
}

HRESULT BrowserProxyModule::Navigate()
{
    if (m_url.Length() == 0)
        return S_OK;

    if (m_view == nullptr)
        return S_OK;

    if (FAILED(m_view->Navigate(m_url)))
    {
        if (wcsncmp(m_url, L"http://", 7) == 0 || wcsncmp(m_url, L"https://", 8) == 0)
            return E_FAIL;

        CComBSTR burl = L"http://";
        if (FAILED(burl.Append(m_url)))
            return E_FAIL;

        m_url = burl;

        if (FAILED(m_view->Navigate(m_url)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT BrowserProxyModule::InvokeBrowserEvent(DISPID id, DISPPARAMS &params, VARIANT *result)
{
    if (m_browserEvents == nullptr)
        return E_FAIL;

    if (FAILED(m_browserEvents->Invoke(id, DIID_IThereEdgeWebBrowserEvents2, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, result, nullptr, nullptr)))
       return E_FAIL;

    return S_OK;
}

HRESULT BrowserProxyModule::SetVisibility(BOOL visible)
{
    if (visible == m_visible)
        return S_OK;

    m_visible = visible;

    if (m_controller != nullptr)
        m_controller->put_IsVisible(visible);

    return S_OK;
}

HRESULT BrowserProxyModule::ForwardCookie(ICoreWebView2CookieManager *cookieManager, const WCHAR *url,
                                          const WCHAR *name, const WCHAR *domain, const WCHAR *path)
{
    WCHAR buff[1000];
    DWORD size;

    size = _countof(buff);
    if (!InternetGetCookie(url, name, buff, &size))
        return E_FAIL;

    const WCHAR *value = nullptr;
    {
        DWORD skip = wcslen(name) + 1; // name=
        if (size < skip)
            return E_FAIL;

        value = buff + skip;
    }

    CComPtr<ICoreWebView2Cookie> cookie;
    if (FAILED(cookieManager->CreateCookie(name, value, domain, path, &cookie)) || cookie == nullptr)
        return E_FAIL;

    if (FAILED(cookieManager->AddOrUpdateCookie(cookie)))
        return E_FAIL;

    return S_OK;
}

HRESULT BrowserProxyModule::ApplyScript(ICoreWebView2 *view, LONG id)
{
    HRSRC source = FindResource(g_Instance, MAKEINTRESOURCE(id), MAKEINTRESOURCE(TEXTFILE));
    if (source == nullptr)
        return E_FAIL;

    DWORD dsize = SizeofResource(g_Instance, source);
    if (dsize == 0)
        return E_FAIL;

    HGLOBAL resource = LoadResource(g_Instance, source);
    if (resource == nullptr)
        return E_FAIL;

    const CHAR *data = static_cast<CHAR*>(LockResource(resource));
    if (data == nullptr)
        return E_FAIL;

    CComBSTR text(dsize + 1);
    if (text.m_str == nullptr)
        return E_FAIL;

    DWORD tsize = MultiByteToWideChar(CP_UTF8, 0, data, dsize, text.m_str, dsize);
    if (tsize != dsize)
        return E_FAIL;

    text[dsize] = 0;

    if (FAILED(view->ExecuteScript(text, Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
        [this](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT
        {
            return S_OK;
        }
    ).Get())))
        return E_FAIL;

    return S_OK;
}

HRESULT BrowserProxyModule::SetDeferral(ICoreWebView2NewWindowRequestedEventArgs *args)
{
    if (FAILED(args->GetDeferral(&m_newWindowDeferral)))
        return E_FAIL;

    m_newWindowArgs = args;

    return S_OK;
}

HRESULT BrowserProxyModule::ProcessDeferral()
{
    if (m_newWindowDeferral == nullptr || m_newWindowArgs == nullptr || m_view == nullptr)
        return S_FALSE;

    if (FAILED(m_newWindowArgs->put_NewWindow(m_view)))
        return E_FAIL;

    if (FAILED(m_newWindowArgs->put_Handled(true)))
        return E_FAIL;

    CComPtr<ICoreWebView2WindowFeatures> features;
    if (SUCCEEDED(m_newWindowArgs->get_WindowFeatures(&features)))
    {
        BOOL hasPosition = false;
        if (SUCCEEDED(features->get_HasPosition(&hasPosition)) && hasPosition)
        {
            UINT32 left;
            if (SUCCEEDED(features->get_Left(&left)))
            {
                VARIANTARG vargs[1];
                vargs[0].vt = VT_I4;
                vargs[0].lVal = left;

                DISPPARAMS params;
                params.rgvarg = vargs;
                params.cArgs = _countof(vargs);
                params.cNamedArgs = 0;

                InvokeBrowserEvent(DISPID_WINDOWSETLEFT, params);
            }

            UINT32 top;
            if (SUCCEEDED(features->get_Top(&top)))
            {
                VARIANTARG vargs[1];
                vargs[0].vt = VT_I4;
                vargs[0].lVal = top;

                DISPPARAMS params;
                params.rgvarg = vargs;
                params.cArgs = _countof(vargs);
                params.cNamedArgs = 0;

                InvokeBrowserEvent(DISPID_WINDOWSETTOP, params);
            }
        }

        BOOL hasSize = false;
        if (SUCCEEDED(features->get_HasSize(&hasSize)) && hasSize)
        {
            UINT32 width;
            if (SUCCEEDED(features->get_Width(&width)))
            {
                VARIANTARG vargs[1];
                vargs[0].vt = VT_I4;
                vargs[0].lVal = width;

                DISPPARAMS params;
                params.rgvarg = vargs;
                params.cArgs = _countof(vargs);
                params.cNamedArgs = 0;

                InvokeBrowserEvent(DISPID_WINDOWSETWIDTH, params);
            }

            UINT32 height;
            if (SUCCEEDED(features->get_Height(&height)))
            {
                VARIANTARG vargs[1];
                vargs[0].vt = VT_I4;
                vargs[0].lVal = height;

                DISPPARAMS params;
                params.rgvarg = vargs;
                params.cArgs = _countof(vargs);
                params.cNamedArgs = 0;

                InvokeBrowserEvent(DISPID_WINDOWSETHEIGHT, params);
            }
        }

        BOOL hasMenuBar = false;
        if (SUCCEEDED(features->get_ShouldDisplayMenuBar(&hasMenuBar)))
        {
            VARIANTARG vargs[1];
            vargs[0].vt = VT_BOOL;
            vargs[0].boolVal = hasMenuBar ? VARIANT_TRUE : VARIANT_FALSE;

            DISPPARAMS params;
            params.rgvarg = vargs;
            params.cArgs = _countof(vargs);
            params.cNamedArgs = 0;

            InvokeBrowserEvent(DISPID_ONMENUBAR, params);
        }

        BOOL hasToolbar = false;
        if (SUCCEEDED(features->get_ShouldDisplayToolbar(&hasToolbar)))
        {
            VARIANTARG vargs[1];
            vargs[0].vt = VT_BOOL;
            vargs[0].boolVal = hasToolbar ? VARIANT_TRUE : VARIANT_FALSE;

            DISPPARAMS params;
            params.rgvarg = vargs;
            params.cArgs = _countof(vargs);
            params.cNamedArgs = 0;

            InvokeBrowserEvent(DISPID_ONTOOLBAR, params);
        }

        BOOL hasStatus = false;
        if (SUCCEEDED(features->get_ShouldDisplayStatus(&hasStatus)))
        {
            VARIANTARG vargs[1];
            vargs[0].vt = VT_BOOL;
            vargs[0].boolVal = hasStatus ? VARIANT_TRUE : VARIANT_FALSE;

            DISPPARAMS params;
            params.rgvarg = vargs;
            params.cArgs = _countof(vargs);
            params.cNamedArgs = 0;

            InvokeBrowserEvent(DISPID_ONSTATUSBAR, params);
        }
    }

    m_newWindowDeferral->Complete();

    m_newWindowDeferral.Release();
    m_newWindowArgs.Release();

    return S_OK;
}
