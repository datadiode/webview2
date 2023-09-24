#pragma once

using namespace ATL;

void Log(const WCHAR *format, ...);

class CDispInvoke
{
public:
    static UINT const WM_DISPINVOKE = WM_APP + 1;
    CDispInvoke
    (
        IDispatch* p,
        DISPID dispidMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pdispparams,
        VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo,
        UINT* puArgErr
    )
        : p(p)
        , dispidMember(dispidMember)
        , riid(riid)
        , lcid(lcid)
        , wFlags(wFlags)
        , pdispparams(pdispparams)
        , pvarResult(pvarResult)
        , pexcepinfo(pexcepinfo)
        , puArgErr(puArgErr)
    {
    }
    LRESULT Invoke()
    {
        return p->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
    }
    HRESULT Invoke(HWND hwnd)
    {
        return static_cast<HRESULT>(SendMessage(hwnd, WM_DISPINVOKE, 0, reinterpret_cast<LPARAM>(this)));
    }
    static LRESULT OnDispInvoke(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return reinterpret_cast<CDispInvoke*>(lParam)->Invoke();
    }
private:
    IDispatch* const p;
    DISPID const dispidMember;
    REFIID riid;
    LCID const lcid;
    WORD const wFlags;
    DISPPARAMS* const pdispparams;
    VARIANT* const pvarResult;
    EXCEPINFO* const pexcepinfo;
    UINT* const puArgErr;
};

class BrowserProxyLicense
{
protected:
    static BOOL VerifyLicenseKey(BSTR bstr)
    {
        return wcscmp(bstr, L"fX5xiyLkhf1hCQ6gh9Zrsw") == 0;
    }
    static BOOL GetLicenseKey(DWORD, BSTR* pBstr)
    {
        *pBstr = SysAllocString(L"fX5xiyLkhf1hCQ6gh9Zrsw");
        return TRUE;
    }
    static BOOL IsLicenseValid() { return TRUE; }
};

class BrowserProxyModule: public CComObjectRootEx<CComSingleThreadModel>,
                          public CComControl<BrowserProxyModule>,
                          public IOleControlImpl<BrowserProxyModule>,
                          public IOleObjectImpl<BrowserProxyModule>,
                          public IOleInPlaceActiveObjectImpl<BrowserProxyModule>,
                          public IViewObjectExImpl<BrowserProxyModule>,
                          public IOleInPlaceObjectWindowlessImpl<BrowserProxyModule>,
                          public IPersistStorageImpl<BrowserProxyModule>,
                          public IPersistStreamInitImpl<BrowserProxyModule>,
                          public ISupportErrorInfoImpl<&IID_IThereEdgeWebBrowser>,
                          public IDispatchImpl<IThereEdgeWebBrowser2, &IID_IThereEdgeWebBrowser2, &LIBID_BrowserProxyLib>,
                          public IProvideClassInfo2Impl<&CLSID_ThereEdgeWebBrowser, &DIID_IThereEdgeWebBrowserEvents2, &LIBID_BrowserProxyLib>,
                          public CComCoClass<BrowserProxyModule, &CLSID_ThereEdgeWebBrowser>,
                          public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler,
                          public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
{
public:
    DECLARE_CLASSFACTORY2(BrowserProxyLicense)
    DECLARE_REGISTRY_RESOURCEID(IDR_BROWSERPROXY)

    DECLARE_OLEMISC_STATUS(OLEMISC_RECOMPOSEONRESIZE |
        OLEMISC_CANTLINKINSIDE |
        OLEMISC_INSIDEOUT |
        OLEMISC_ACTIVATEWHENVISIBLE |
        OLEMISC_SETCLIENTSITEFIRST)

    BrowserProxyModule();
    virtual ~BrowserProxyModule();

    BEGIN_COM_MAP(BrowserProxyModule)
        COM_INTERFACE_ENTRY(IThereEdgeWebBrowser)
        COM_INTERFACE_ENTRY2(IDispatch, IThereEdgeWebBrowser)
        COM_INTERFACE_ENTRY(IViewObjectEx)
        COM_INTERFACE_ENTRY(IViewObject2)
        COM_INTERFACE_ENTRY(IViewObject)
        COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY(IOleInPlaceObject)
        COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
        COM_INTERFACE_ENTRY(IOleControl)
        COM_INTERFACE_ENTRY(IOleObject)
        COM_INTERFACE_ENTRY(IPersistStorage)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
        COM_INTERFACE_ENTRY2(IProvideClassInfo, IProvideClassInfo2)
    END_COM_MAP()

    BEGIN_PROP_MAP(BrowserProxyModule)
        PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
        PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
    END_PROP_MAP()

    BEGIN_MSG_MAP(BrowserProxyModule)
        CHAIN_MSG_MAP(CComControl<BrowserProxyModule>)
        MESSAGE_HANDLER(CDispInvoke::WM_DISPINVOKE, CDispInvoke::OnDispInvoke)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    END_MSG_MAP()

    HRESULT IPersistStreamInit_Load(LPSTREAM, ATL_PROPMAP_ENTRY const*) { return S_OK; }

    bool InIDE();
    HRESULT OnDraw(ATL_DRAWINFO& di);

protected:
    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL&);

    STDMETHOD(Invoke)(
        DISPID dispidMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pdispparams,
        VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo,
        UINT* puArgErr)
    {
        return GetCurrentThreadId() != GetWindowThreadProcessId(m_hWnd, NULL) ?
            CDispInvoke(this, dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr).Invoke(m_hWnd) :
            IDispatchImpl::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
    }

    virtual HRESULT STDMETHODCALLTYPE SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj) override;
    virtual HRESULT STDMETHODCALLTYPE GoBack() override;
    virtual HRESULT STDMETHODCALLTYPE GoForward() override;
    virtual HRESULT STDMETHODCALLTYPE GoHome() override;
    virtual HRESULT STDMETHODCALLTYPE GoSearch() override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE Navigate(BSTR URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers) override;
    virtual HRESULT STDMETHODCALLTYPE Refresh() override;
    virtual HRESULT STDMETHODCALLTYPE Refresh2(VARIANT *Level) override;
    virtual HRESULT STDMETHODCALLTYPE Stop() override;
    virtual HRESULT STDMETHODCALLTYPE get_Application(IDispatch **ppDisp) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Parent(IDispatch **ppDisp) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Container(IDispatch **ppDisp) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Document(IDispatch **ppDisp) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_TopLevelContainer(VARIANT_BOOL *pBool) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Type(BSTR *Type) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Left(long *pl) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_Left(long Left) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Top(long *pl) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_Top(long Top) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Width(long *pl) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_Width(long Width) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Height(long *pl) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_Height(long Height) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_LocationName(BSTR *LocationName) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_LocationURL(BSTR *LocationURL) override;
    virtual HRESULT STDMETHODCALLTYPE get_Busy(VARIANT_BOOL *pBool) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE Quit() override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE ClientToWindow(int *pcx, int *pcy) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE PutProperty(BSTR Property, VARIANT vtValue) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE GetProperty(BSTR Property, VARIANT *pvtValue) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Name(BSTR *Name) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_HWND(SHANDLE_PTR *pHWND) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_FullName(BSTR *FullName) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Path(BSTR *Path) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Visible(VARIANT_BOOL *pBool) override;
    virtual HRESULT STDMETHODCALLTYPE put_Visible(VARIANT_BOOL Value) override;
    virtual HRESULT STDMETHODCALLTYPE get_StatusBar(VARIANT_BOOL *pBool) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_StatusBar(VARIANT_BOOL Value) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_StatusText(BSTR *StatusText) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_StatusText(BSTR StatusText) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_ToolBar(int *Value) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_ToolBar(int Value) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_MenuBar(VARIANT_BOOL *Value) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_MenuBar(VARIANT_BOOL Value) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_FullScreen(VARIANT_BOOL *pbFullScreen) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_FullScreen(VARIANT_BOOL bFullScreen) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE Navigate2(VARIANT *URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers) override;
    virtual HRESULT STDMETHODCALLTYPE QueryStatusWB(OLECMDID cmdID, OLECMDF *pcmdf) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE ExecWB(OLECMDID cmdID, OLECMDEXECOPT cmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE ShowBrowserBar(VARIANT *pvaClsid, VARIANT *pvarShow, VARIANT *pvarSize) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_ReadyState(READYSTATE *plReadyState) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Offline(VARIANT_BOOL *pbOffline) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_Offline(VARIANT_BOOL bOffline) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Silent(VARIANT_BOOL *pbSilent) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_Silent(VARIANT_BOOL bSilent) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_RegisterAsBrowser(VARIANT_BOOL *pbRegister) override;
    virtual HRESULT STDMETHODCALLTYPE put_RegisterAsBrowser(VARIANT_BOOL bRegister) override;
    virtual HRESULT STDMETHODCALLTYPE get_RegisterAsDropTarget(VARIANT_BOOL *pbRegister) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_RegisterAsDropTarget(VARIANT_BOOL bRegister) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_TheaterMode(VARIANT_BOOL *pbRegister) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_TheaterMode(VARIANT_BOOL bRegister) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_AddressBar(VARIANT_BOOL *Value) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_AddressBar(VARIANT_BOOL Value) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE get_Resizable(VARIANT_BOOL *Value) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE put_Resizable(VARIANT_BOOL Value) override {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, ICoreWebView2Environment *environment) override;
    virtual HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, ICoreWebView2Controller *controller) override;

protected:
    HRESULT OnNavigationStarting(ICoreWebView2 *sender,  ICoreWebView2NavigationStartingEventArgs *args);
    HRESULT OnNavigationCompleted(ICoreWebView2 *sender, ICoreWebView2NavigationCompletedEventArgs *args);
    HRESULT OnNewWindowRequested(ICoreWebView2 *sender, ICoreWebView2NewWindowRequestedEventArgs *args);
    HRESULT OnSourceChanged(ICoreWebView2 *sender, ICoreWebView2SourceChangedEventArgs *args);
    HRESULT OnHistoryChanged(ICoreWebView2 *sender);
    HRESULT OnDocumentTitleChanged(ICoreWebView2 *sender);
#ifdef THERE
    HRESULT OnWebResourceRequested(ICoreWebView2 *sender, ICoreWebView2WebResourceRequestedEventArgs *args);
    HRESULT OnWebMessageReceived(ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args);
#endif
    HRESULT OnWindowCloseRequested(ICoreWebView2 *sender);
    HRESULT OnDOMContentLoaded(ICoreWebView2 *sender, ICoreWebView2DOMContentLoadedEventArgs *args);
    HRESULT OnDownloadStarting(ICoreWebView2 *sender, ICoreWebView2DownloadStartingEventArgs *args);
    HRESULT Navigate();
    HRESULT InvokeBrowserEvent(DISPID id, DISPPARAMS &args, VARIANT *result = nullptr);
    HRESULT SetVisibility(BOOL visible);
    HRESULT ForwardCookie(ICoreWebView2CookieManager *cookieManager, const WCHAR *url,
                          const WCHAR *name, const WCHAR *domain, const WCHAR *path);
    HRESULT ApplyScript(ICoreWebView2 *view, LONG id);
    HRESULT SetDeferral(ICoreWebView2NewWindowRequestedEventArgs *args);
    HRESULT ProcessDeferral();

protected:
    CComBSTR                                           m_url;
    CComBSTR                                           m_proxyVersion;
    CComBSTR                                           m_browserVersion;
    CComBSTR                                           m_userDataFolder;
    CComPtr<IThereEdgeWebBrowserEvents2>               m_browserEvents;
    CComPtr<ICoreWebView2Environment>                  m_environment;
    CComPtr<ICoreWebView2Controller3>                  m_controller;
    CComPtr<ICoreWebView2_4>                           m_view;
    CComPtr<ICoreWebView2Deferral>                     m_newWindowDeferral;
    CComPtr<ICoreWebView2NewWindowRequestedEventArgs>  m_newWindowArgs;
#ifdef THERE
    CComPtr<VoiceTrainerProxy>                         m_voiceTrainerProxy;
    CComPtr<SettingsRequestHandler>                    m_settingsRequestHandler;
#endif
    EventRegistrationToken                             m_navigationStartingToken;
    EventRegistrationToken                             m_navigationCompletedToken;
    EventRegistrationToken                             m_newWindowRequestedToken;
    EventRegistrationToken                             m_sourceChangedToken;
    EventRegistrationToken                             m_historyChangedToken;
    EventRegistrationToken                             m_documentTitleChangedToken;
#ifdef THERE
    EventRegistrationToken                             m_webResourceRequestedToken;
    EventRegistrationToken                             m_webMessageReceivedToken;
#endif
    EventRegistrationToken                             m_windowCloseRequestedToken;
    EventRegistrationToken                             m_domContentLoadedToken;
    EventRegistrationToken                             m_downloadStartingToken;
    BOOL                                               m_ready;
    BOOL                                               m_visible;
};
