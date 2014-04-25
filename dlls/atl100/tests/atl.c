/*
 * Copyright 2012 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS
#define CONST_VTABLE

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <exdisp.h>

#include <atlbase.h>
#include <mshtml.h>

#include <wine/test.h>

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { called_ ## func = FALSE; expect_ ## func = TRUE; } while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(FreezeEvents_TRUE);
DEFINE_EXPECT(FreezeEvents_FALSE);
DEFINE_EXPECT(DoVerb);
DEFINE_EXPECT(SetExtent);
DEFINE_EXPECT(GetExtent);
DEFINE_EXPECT(SetClientSite);
DEFINE_EXPECT(SetClientSite_NULL);
DEFINE_EXPECT(SetHostNames);
DEFINE_EXPECT(Close);
DEFINE_EXPECT(InPlaceObject_GetWindow);
DEFINE_EXPECT(SetObjectRects);
DEFINE_EXPECT(InPlaceDeactivate);
DEFINE_EXPECT(GetMiscStatus);
DEFINE_EXPECT(SetAdvise);
DEFINE_EXPECT(SetAdvise_NULL);
DEFINE_EXPECT(GetViewStatus);
DEFINE_EXPECT(OnAmbientPropertyChange_UNKNOWN);
DEFINE_EXPECT(Advise);
DEFINE_EXPECT(Unadvise);
DEFINE_EXPECT(Draw);
DEFINE_EXPECT(LockRunning);
DEFINE_EXPECT(SetSite);
DEFINE_EXPECT(SetSite_NULL);

static IOleClientSite *client_site;
static HWND container_hwnd, plugin_hwnd;
static LONG activex_refcnt;
static HRESULT ax_qi(REFIID, void**);
static BOOL g_isRunning;
static HRESULT SetSite_hres;

static LRESULT WINAPI plugin_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HBRUSH brush;
        RECT rect;
        HDC dc;

        GetClientRect(hwnd, &rect);

        dc = BeginPaint(hwnd, &ps);
        brush = CreateSolidBrush(RGB(255,0,0));
        SelectObject(dc, brush);
        Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
        DeleteObject(brush);
        EndPaint(hwnd, &ps);
        break;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void create_plugin_window(HWND parent, const RECT *rect)
{
    static const WCHAR plugin_testW[] =
        {'p','l','u','g','i','n',' ','t','e','s','t',0};
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        0,
        plugin_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        plugin_testW,
        NULL
    };

    RegisterClassExW(&wndclass);
    plugin_hwnd = CreateWindowW(plugin_testW, plugin_testW,
            WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN, rect->left, rect->top,
            rect->right-rect->left, rect->bottom-rect->top, parent, NULL, NULL, NULL);
}

static HRESULT WINAPI OleControl_QueryInterface(IOleControl *iface, REFIID riid, void **ppv)
{
    return ax_qi(riid, ppv);
}

static ULONG WINAPI OleControl_AddRef(IOleControl *iface)
{
    return ++activex_refcnt;
}

static ULONG WINAPI OleControl_Release(IOleControl *iface)
{
    return --activex_refcnt;
}

static HRESULT WINAPI OleControl_GetControlInfo(IOleControl *iface, CONTROLINFO *pCI)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_OnMnemonic(IOleControl *iface, MSG *mMsg)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_OnAmbientPropertyChange(IOleControl *iface, DISPID dispID)
{
    switch(dispID) {
    case DISPID_UNKNOWN:
        CHECK_EXPECT2(OnAmbientPropertyChange_UNKNOWN);
        break;
    default:
        ok(0, "unexpected call %d\n", dispID);
    }

    return S_OK;
}

static HRESULT WINAPI OleControl_FreezeEvents(IOleControl *iface, BOOL bFreeze)
{
    if(bFreeze)
        CHECK_EXPECT2(FreezeEvents_TRUE);
    else
        CHECK_EXPECT2(FreezeEvents_FALSE);
    return S_OK;
}

static const IOleControlVtbl OleControlVtbl = {
    OleControl_QueryInterface,
    OleControl_AddRef,
    OleControl_Release,
    OleControl_GetControlInfo,
    OleControl_OnMnemonic,
    OleControl_OnAmbientPropertyChange,
    OleControl_FreezeEvents
};

static IOleControl OleControl = { &OleControlVtbl };

static HRESULT WINAPI ViewObjectEx_QueryInterface(IViewObjectEx *iface, REFIID riid, void **ppv)
{
    return ax_qi(riid, ppv);
}

static ULONG WINAPI ViewObjectEx_AddRef(IViewObjectEx *iface)
{
    return ++activex_refcnt;
}

static ULONG WINAPI ViewObjectEx_Release(IViewObjectEx *iface)
{
    return --activex_refcnt;
}

static HRESULT WINAPI ViewObjectEx_Draw(IViewObjectEx *iface, DWORD dwDrawAspect, LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd,
        HDC hdcTargetDev, HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBoungs, BOOL (WINAPI*pfnContinue)(ULONG_PTR), ULONG_PTR dwContinue)
{
    CHECK_EXPECT(Draw);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_GetColorSet(IViewObjectEx *iface, DWORD dwDrawAspect, LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd,
        HDC hicTargetDev, LOGPALETTE **ppColorSet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_Freeze(IViewObjectEx *iface, DWORD dwDrawAspect, LONG lindex, void *pvAspect, DWORD *pdwFreeze)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_Unfreeze(IViewObjectEx *iface, DWORD dwFreeze)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_SetAdvise(IViewObjectEx *iface, DWORD aspects, DWORD advf, IAdviseSink *pAdvSink)
{
    if (pAdvSink)
        CHECK_EXPECT(SetAdvise);
    else
        CHECK_EXPECT(SetAdvise_NULL);

    ok(aspects == DVASPECT_CONTENT, "aspects = %x\n", aspects);
    ok(!advf, "advf = %x\n", advf);

    return S_OK;
}

static HRESULT WINAPI ViewObjectEx_GetAdvise(IViewObjectEx *iface, DWORD *pAspects, DWORD *pAdvf, IAdviseSink **ppAdvSink)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_GetExtent(IViewObjectEx *iface, DWORD dwDrawAspect, LONG lindex, DVTARGETDEVICE *ptd, LPSIZEL lpsizel)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_GetRect(IViewObjectEx *iface, DWORD dwAspect, LPRECTL pRect)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_GetViewStatus(IViewObjectEx *iface, DWORD *pdwStatus)
{
    CHECK_EXPECT(GetViewStatus);

    *pdwStatus = VIEWSTATUS_OPAQUE|VIEWSTATUS_SOLIDBKGND;
    return S_OK;
}

static HRESULT WINAPI ViewObjectEx_QueryHitPoint(IViewObjectEx *iface, DWORD dwAspect, LPCRECT pRectBounds, POINT ptlLoc,
        LONG lCloseHint, DWORD *pHitResult)
{
    trace("QueryHitPoint call ignored\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_QueryHitRect(IViewObjectEx *iface, DWORD dwAspect, LPCRECT pRectBounds, LPCRECT pRectLoc,
        LONG lCloseHint, DWORD *pHitResult)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_GetNaturalExtent(IViewObjectEx *iface, DWORD dwAspect, LONG lindex, DVTARGETDEVICE *ptd,
        HDC hicTargetDev, DVEXTENTINFO *pExtentIngo, LPSIZEL pSizel)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IViewObjectExVtbl ViewObjectExVtbl = {
    ViewObjectEx_QueryInterface,
    ViewObjectEx_AddRef,
    ViewObjectEx_Release,
    ViewObjectEx_Draw,
    ViewObjectEx_GetColorSet,
    ViewObjectEx_Freeze,
    ViewObjectEx_Unfreeze,
    ViewObjectEx_SetAdvise,
    ViewObjectEx_GetAdvise,
    ViewObjectEx_GetExtent,
    ViewObjectEx_GetRect,
    ViewObjectEx_GetViewStatus,
    ViewObjectEx_QueryHitPoint,
    ViewObjectEx_QueryHitRect,
    ViewObjectEx_GetNaturalExtent
};

static IViewObjectEx ViewObjectEx = { &ViewObjectExVtbl };



static HRESULT WINAPI OleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    return ax_qi(riid, ppv);
}

static ULONG WINAPI OleObject_AddRef(IOleObject *iface)
{
    return ++activex_refcnt;
}

static ULONG WINAPI OleObject_Release(IOleObject *iface)
{
    return --activex_refcnt;
}

static HRESULT WINAPI OleObject_SetClientSite(IOleObject *iface, IOleClientSite *pClientSite)
{
    if(!pClientSite) {
        CHECK_EXPECT(SetClientSite_NULL);
        return S_OK;
    }

    CHECK_EXPECT(SetClientSite);

    IOleClientSite_AddRef(pClientSite);
    client_site = pClientSite;
    return S_OK;
}

static HRESULT WINAPI OleObject_GetClientSite(IOleObject *iface, IOleClientSite **ppClientSite)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetHostNames(IOleObject *iface, LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    CHECK_EXPECT(SetHostNames);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    CHECK_EXPECT(Close);

    ok(dwSaveOption == OLECLOSE_NOSAVE, "dwSaveOption = %d\n", dwSaveOption);
    return S_OK;
}

static HRESULT WINAPI OleObject_SetMoniker(IOleObject *iface, DWORD dwWhichMoniker, IMoniker *pmk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMoniker(IOleObject *iface, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_InitFromData(IOleObject *iface, IDataObject *pDataObject, BOOL fCreation,
        DWORD dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetClipboardData(IOleObject *iface, DWORD dwReserved, IDataObject **ppDataObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_DoVerb(IOleObject *iface, LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
        LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    OLEINPLACEFRAMEINFO frame_info = {0xdeadbeef};
    IOleInPlaceUIWindow *ip_uiwindow;
    IOleInPlaceFrame *ip_frame;
    IOleInPlaceSiteEx *ip_site;
    RECT pos_rect, clip_rect;
    BOOL no_redraw;
    HWND hwnd;
    HRESULT hres;

    CHECK_EXPECT(DoVerb);

    ok(iVerb == OLEIVERB_INPLACEACTIVATE, "iVerb = %d\n", iVerb);
    ok(!lpmsg, "lpmsg != NULL\n");
    ok(pActiveSite != NULL, "pActiveSite == NULL\n");
    ok(!lindex, "lindex = %d\n", lindex);
    ok(lprcPosRect != NULL, "lprcPosRect == NULL\n");

    hres = IOleClientSite_QueryInterface(pActiveSite, &IID_IOleInPlaceSiteEx, (void**)&ip_site);
    ok(hres == S_OK, "Could not get IOleInPlaceSiteEx iface: %08x\n", hres);

    hres = IOleInPlaceSiteEx_CanInPlaceActivate(ip_site);
    ok(hres == S_OK, "CanInPlaceActivate failed: %08x\n", hres);

    no_redraw = 0xdeadbeef;
    SET_EXPECT(SetObjectRects);
    hres = IOleInPlaceSiteEx_OnInPlaceActivateEx(ip_site, &no_redraw, 0);
    todo_wine CHECK_CALLED(SetObjectRects);
    ok(hres == S_OK, "InPlaceActivateEx failed: %08x\n", hres);
    ok(no_redraw == 0xdeadbeef, "no_redraw = %x\n", no_redraw);

    hwnd = NULL;
    hres = IOleInPlaceSiteEx_GetWindow(ip_site, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
    ok(hwnd != NULL, "hwnd == NULL\n");
    ok(hwnd == hwndParent, "hwnd != hwndParent\n");

    create_plugin_window(hwnd, lprcPosRect);

    ip_frame = NULL;
    ip_uiwindow = NULL;
    frame_info.cb = sizeof(OLEINPLACEFRAMEINFO);
    hres = IOleInPlaceSiteEx_GetWindowContext(ip_site, &ip_frame, &ip_uiwindow, &pos_rect, &clip_rect, &frame_info);
    ok(hres == S_OK, "GetWindowContext failed: %08x\n", hres);
    ok(ip_frame != NULL, "ip_frame == NULL\n");
    todo_wine ok(ip_uiwindow != NULL, "ip_uiwindow == NULL\n");
    ok((IOleInPlaceUIWindow*)ip_frame != ip_uiwindow, "ip_frame == ip_uiwindow\n");
    ok(!memcmp(&pos_rect, lprcPosRect, sizeof(RECT)), "pos_rect != lpecPosRect\n");
    ok(!memcmp(&clip_rect, lprcPosRect, sizeof(RECT)), "clip_rect != lpecPosRect\n");
    ok(frame_info.cb == sizeof(frame_info), "frame_info.cb = %d\n", frame_info.cb);
    ok(!frame_info.fMDIApp, "frame_info.fMDIApp = %x\n", frame_info.fMDIApp);
    todo_wine ok(!frame_info.hwndFrame,"frame_info.hwndFrame = %p\n", frame_info.hwndFrame);
    todo_wine ok(frame_info.hwndFrame == container_hwnd, "frame_info.hwnd != container_hwnd\n");
    todo_wine ok(frame_info.haccel != NULL, "expected frame_info.haccel not NULL!\n");
    todo_wine ok(frame_info.cAccelEntries == 1, "frame_info.cAccelEntries %d\n", frame_info.cAccelEntries);

    IOleInPlaceFrame_Release(ip_frame);
    if (ip_uiwindow) IOleInPlaceUIWindow_Release(ip_uiwindow);

    IOleInPlaceSiteEx_Release(ip_site);

    hres = IOleClientSite_ShowObject(client_site);
    ok(hres == S_OK, "ShowObject failed: %08x\n", hres);

    return S_OK;
}

static HRESULT WINAPI OleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Update(IOleObject *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_IsUpToDate(IOleObject *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserClassID(IOleObject *iface, CLSID *pClsid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserType(IOleObject *iface, DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    CHECK_EXPECT(SetExtent);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    CHECK_EXPECT(GetExtent);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Advise(IOleObject *iface, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    CHECK_EXPECT(Advise);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Unadvise(IOleObject *iface, DWORD dwConnection)
{
    CHECK_EXPECT(Unadvise);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMiscStatus(IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    CHECK_EXPECT(GetMiscStatus);
    ok(dwAspect == DVASPECT_CONTENT, "dwAspect = %d\n", dwAspect);
    ok(pdwStatus != NULL, "pdwStatus == NULL\n");
    *pdwStatus = OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE
        |OLEMISC_INSIDEOUT|OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE;
    return S_OK;
}

static HRESULT WINAPI OleObject_SetColorScheme(IOleObject *iface, LOGPALETTE *pLogpal)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleObjectVtbl OleObjectVtbl = {
    OleObject_QueryInterface,
    OleObject_AddRef,
    OleObject_Release,
    OleObject_SetClientSite,
    OleObject_GetClientSite,
    OleObject_SetHostNames,
    OleObject_Close,
    OleObject_SetMoniker,
    OleObject_GetMoniker,
    OleObject_InitFromData,
    OleObject_GetClipboardData,
    OleObject_DoVerb,
    OleObject_EnumVerbs,
    OleObject_Update,
    OleObject_IsUpToDate,
    OleObject_GetUserClassID,
    OleObject_GetUserType,
    OleObject_SetExtent,
    OleObject_GetExtent,
    OleObject_Advise,
    OleObject_Unadvise,
    OleObject_EnumAdvise,
    OleObject_GetMiscStatus,
    OleObject_SetColorScheme
};

static IOleObject OleObject = { &OleObjectVtbl };

static HRESULT WINAPI OleInPlaceObject_QueryInterface(IOleInPlaceObjectWindowless *iface,
        REFIID riid, void **ppv)
{
    return ax_qi(riid, ppv);
}

static ULONG WINAPI OleInPlaceObject_AddRef(IOleInPlaceObjectWindowless *iface)
{
    return ++activex_refcnt;
}

static ULONG WINAPI OleInPlaceObject_Release(IOleInPlaceObjectWindowless *iface)
{
    return --activex_refcnt;
}

static HRESULT WINAPI OleInPlaceObject_GetWindow(IOleInPlaceObjectWindowless *iface,
        HWND *phwnd)
{
    CHECK_EXPECT2(InPlaceObject_GetWindow);

    ok(phwnd != NULL, "phwnd == NULL\n");

    *phwnd = plugin_hwnd;
    return *phwnd ? S_OK : E_UNEXPECTED;
}

static HRESULT WINAPI OleInPlaceObject_ContextSensitiveHelp(IOleInPlaceObjectWindowless *iface,
        BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObject_InPlaceDeactivate(IOleInPlaceObjectWindowless *iface)
{
    IOleInPlaceSite *ip_site;
    HRESULT hres;

    CHECK_EXPECT(InPlaceDeactivate);

    hres = IOleClientSite_QueryInterface(client_site, &IID_IOleInPlaceSite, (void**)&ip_site);
    ok(hres == S_OK, "Could not get IOleInPlaceSite iface: %08x\n", hres);

    hres = IOleInPlaceSite_OnInPlaceDeactivate(ip_site);
    ok(hres == S_OK, "OnInPlaceDeactivate failed: %08x\n", hres);

    IOleInPlaceSite_Release(ip_site);
    return S_OK;
}

static HRESULT WINAPI OleInPlaceObject_UIDeactivate(IOleInPlaceObjectWindowless *iface)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static HRESULT WINAPI OleInPlaceObject_SetObjectRects(IOleInPlaceObjectWindowless *iface,
        LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    CHECK_EXPECT(SetObjectRects);
    return S_OK;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_ReactivateAndUndo(IOleInPlaceObjectWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_OnWindowMessage(IOleInPlaceObjectWindowless *iface,
        UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lpResult)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_GetDropTarget(IOleInPlaceObjectWindowless *iface,
        IDropTarget **ppDropTarget)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceObjectWindowlessVtbl OleInPlaceObjectWindowlessVtbl = {
    OleInPlaceObject_QueryInterface,
    OleInPlaceObject_AddRef,
    OleInPlaceObject_Release,
    OleInPlaceObject_GetWindow,
    OleInPlaceObject_ContextSensitiveHelp,
    OleInPlaceObject_InPlaceDeactivate,
    OleInPlaceObject_UIDeactivate,
    OleInPlaceObject_SetObjectRects,
    OleInPlaceObjectWindowless_ReactivateAndUndo,
    OleInPlaceObjectWindowless_OnWindowMessage,
    OleInPlaceObjectWindowless_GetDropTarget
};

static IOleInPlaceObjectWindowless OleInPlaceObjectWindowless = { &OleInPlaceObjectWindowlessVtbl };

static HRESULT WINAPI OleObjectRunnable_QueryInterface(IRunnableObject *iface, REFIID riid, void **ppv)
{
    return ax_qi(riid, ppv);
}

static ULONG WINAPI OleObjectRunnable_AddRef(IRunnableObject *iface)
{
    return ++activex_refcnt;
}

static ULONG WINAPI OleObjectRunnable_Release(IRunnableObject *iface)
{
    return --activex_refcnt;
}

static HRESULT WINAPI OleObjectRunnable_GetRunningClass(IRunnableObject *iface, LPCLSID lpClsid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObjectRunnable_Run(IRunnableObject *iface, LPBINDCTX pbc)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static BOOL WINAPI OleObjectRunnable_IsRunning(IRunnableObject *iface)
{
    ok(0, "unexpected call\n");
    return g_isRunning;
}

static HRESULT WINAPI OleObjectRunnable_LockRunning(IRunnableObject *iface, BOOL fLock, BOOL fLastUnlockCloses)
{
    CHECK_EXPECT(LockRunning);
    return S_OK;
}

static HRESULT WINAPI OleObjectRunnable_SetContainedObject(IRunnableObject *iface, BOOL fContained)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IRunnableObjectVtbl OleObjectRunnableVtbl =
{
    OleObjectRunnable_QueryInterface,
    OleObjectRunnable_AddRef,
    OleObjectRunnable_Release,
    OleObjectRunnable_GetRunningClass,
    OleObjectRunnable_Run,
    OleObjectRunnable_IsRunning,
    OleObjectRunnable_LockRunning,
    OleObjectRunnable_SetContainedObject
};

static IRunnableObject OleObjectRunnable = { &OleObjectRunnableVtbl };

static HRESULT WINAPI ObjectWithSite_QueryInterface(IObjectWithSite *iface, REFIID riid, void **ppv)
{
    return ax_qi(riid, ppv);
}

static ULONG WINAPI ObjectWithSite_AddRef(IObjectWithSite *iface)
{
    return --activex_refcnt;
}

static ULONG WINAPI ObjectWithSite_Release(IObjectWithSite *iface)
{
    return --activex_refcnt;
}

static HRESULT WINAPI ObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *pUnkSite)
{
    IServiceProvider *sp;
    HRESULT hres;


    if (pUnkSite)
    {
        CHECK_EXPECT(SetSite);
        hres = IUnknown_QueryInterface(pUnkSite, &IID_IServiceProvider, (void**)&sp);
        ok(hres == S_OK, "Could not get IServiceProvider iface: %08x\n", hres);
        IServiceProvider_Release(sp);
    }
    else
        CHECK_EXPECT(SetSite_NULL);

    return SetSite_hres;
}

static HRESULT WINAPI ObjectWithSite_GetSite(IObjectWithSite *iface, REFIID riid, void **ppvSite)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IObjectWithSiteVtbl ObjectWithSiteVtbl = {
    ObjectWithSite_QueryInterface,
    ObjectWithSite_AddRef,
    ObjectWithSite_Release,
    ObjectWithSite_SetSite,
    ObjectWithSite_GetSite
};

static IObjectWithSite ObjectWithSite = { &ObjectWithSiteVtbl };

static HRESULT ax_qi(REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IOleControl)) {
        *ppv = &OleControl;
    }else if(IsEqualGUID(riid, &IID_IViewObject) || IsEqualGUID(riid, &IID_IViewObject2)
             || IsEqualGUID(riid, &IID_IViewObjectEx)) {
        *ppv = &ViewObjectEx;
    }else if(IsEqualGUID(riid, &IID_IOleObject)) {
        *ppv = &OleObject;
    }else if(IsEqualGUID(riid, &IID_IOleWindow) || IsEqualGUID(riid, &IID_IOleInPlaceObject)
              || IsEqualGUID(&IID_IOleInPlaceObjectWindowless, riid)) {
        *ppv = &OleInPlaceObjectWindowless;
    }else if(IsEqualGUID(riid, &IID_IRunnableObject)) {
            *ppv = &OleObjectRunnable;
    }else if(IsEqualGUID(riid, &IID_IObjectWithSite)) {
            *ppv = &ObjectWithSite;
    }else {
        trace("QI %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
    }

    if(!*ppv)
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static const GUID CLSID_Test =
    {0x178fc163,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
#define CLSID_TEST_STR "178fc163-0000-0000-0000-000000000046"

static const GUID CATID_CatTest1 =
    {0x178fc163,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x46}};
#define CATID_CATTEST1_STR "178fc163-0000-0000-0000-000000000146"

static const GUID CATID_CatTest2 =
    {0x178fc163,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x46}};
#define CATID_CATTEST2_STR "178fc163-0000-0000-0000-000000000246"

static BOOL is_process_limited(void)
{
    static BOOL (WINAPI *pOpenProcessToken)(HANDLE, DWORD, PHANDLE) = NULL;
    HANDLE token;

    if (!pOpenProcessToken)
    {
        HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");
        pOpenProcessToken = (void*)GetProcAddress(hadvapi32, "OpenProcessToken");
        if (!pOpenProcessToken)
            return FALSE;
    }

    if (pOpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        BOOL ret;
        TOKEN_ELEVATION_TYPE type = TokenElevationTypeDefault;
        DWORD size;

        ret = GetTokenInformation(token, TokenElevationType, &type, sizeof(type), &size);
        CloseHandle(token);
        return (ret && type == TokenElevationTypeLimited);
    }
    return FALSE;
}

static void test_winmodule(void)
{
    _AtlCreateWndData create_data[3];
    _ATL_WIN_MODULE winmod;
    void *p;
    HRESULT hres;

    winmod.cbSize = 0xdeadbeef;
    hres = AtlWinModuleInit(&winmod);
    ok(hres == E_INVALIDARG, "AtlWinModuleInit failed: %08x\n", hres);

    winmod.cbSize = sizeof(winmod);
    winmod.m_pCreateWndList = (void*)0xdeadbeef;
    winmod.m_csWindowCreate.LockCount = 0xdeadbeef;
    winmod.m_rgWindowClassAtoms.m_aT = (void*)0xdeadbeef;
    winmod.m_rgWindowClassAtoms.m_nSize = 0xdeadbeef;
    winmod.m_rgWindowClassAtoms.m_nAllocSize = 0xdeadbeef;
    hres = AtlWinModuleInit(&winmod);
    ok(hres == S_OK, "AtlWinModuleInit failed: %08x\n", hres);
    ok(!winmod.m_pCreateWndList, "winmod.m_pCreateWndList = %p\n", winmod.m_pCreateWndList);
    ok(winmod.m_csWindowCreate.LockCount == -1, "winmod.m_csWindowCreate.LockCount = %d\n",
       winmod.m_csWindowCreate.LockCount);
    ok(winmod.m_rgWindowClassAtoms.m_aT == (void*)0xdeadbeef, "winmod.m_rgWindowClassAtoms.m_aT = %p\n",
       winmod.m_rgWindowClassAtoms.m_aT);
    ok(winmod.m_rgWindowClassAtoms.m_nSize == 0xdeadbeef, "winmod.m_rgWindowClassAtoms.m_nSize = %d\n",
       winmod.m_rgWindowClassAtoms.m_nSize);
    ok(winmod.m_rgWindowClassAtoms.m_nAllocSize == 0xdeadbeef, "winmod.m_rgWindowClassAtoms.m_nAllocSize = %d\n",
       winmod.m_rgWindowClassAtoms.m_nAllocSize);

    InitializeCriticalSection(&winmod.m_csWindowCreate);

    AtlWinModuleAddCreateWndData(&winmod, create_data, (void*)0xdead0001);
    ok(winmod.m_pCreateWndList == create_data, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[0].m_pThis == (void*)0xdead0001, "unexpected create_data[0].m_pThis %p\n", create_data[0].m_pThis);
    ok(create_data[0].m_dwThreadID == GetCurrentThreadId(), "unexpected create_data[0].m_dwThreadID %x\n",
       create_data[0].m_dwThreadID);
    ok(!create_data[0].m_pNext, "unexpected create_data[0].m_pNext %p\n", create_data[0].m_pNext);

    AtlWinModuleAddCreateWndData(&winmod, create_data+1, (void*)0xdead0002);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[1].m_pThis == (void*)0xdead0002, "unexpected create_data[1].m_pThis %p\n", create_data[1].m_pThis);
    ok(create_data[1].m_dwThreadID == GetCurrentThreadId(), "unexpected create_data[1].m_dwThreadID %x\n",
       create_data[1].m_dwThreadID);
    ok(create_data[1].m_pNext == create_data, "unexpected create_data[1].m_pNext %p\n", create_data[1].m_pNext);

    AtlWinModuleAddCreateWndData(&winmod, create_data+2, (void*)0xdead0003);
    ok(winmod.m_pCreateWndList == create_data+2, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[2].m_pThis == (void*)0xdead0003, "unexpected create_data[2].m_pThis %p\n", create_data[2].m_pThis);
    ok(create_data[2].m_dwThreadID == GetCurrentThreadId(), "unexpected create_data[2].m_dwThreadID %x\n",
       create_data[2].m_dwThreadID);
    ok(create_data[2].m_pNext == create_data+1, "unexpected create_data[2].m_pNext %p\n", create_data[2].m_pNext);

    p = AtlWinModuleExtractCreateWndData(&winmod);
    ok(p == (void*)0xdead0003, "unexpected AtlWinModuleExtractCreateWndData result %p\n", p);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[2].m_pNext == create_data+1, "unexpected create_data[2].m_pNext %p\n", create_data[2].m_pNext);

    create_data[1].m_dwThreadID = 0xdeadbeef;

    p = AtlWinModuleExtractCreateWndData(&winmod);
    ok(p == (void*)0xdead0001, "unexpected AtlWinModuleExtractCreateWndData result %p\n", p);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
    ok(!create_data[0].m_pNext, "unexpected create_data[0].m_pNext %p\n", create_data[0].m_pNext);
    ok(!create_data[1].m_pNext, "unexpected create_data[1].m_pNext %p\n", create_data[1].m_pNext);

    p = AtlWinModuleExtractCreateWndData(&winmod);
    ok(!p, "unexpected AtlWinModuleExtractCreateWndData result %p\n", p);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
}

#define test_key_exists(a,b) _test_key_exists(__LINE__,a,b)
static void _test_key_exists(unsigned line, HKEY root, const char *key_name)
{
    HKEY key;
    DWORD res;

    res = RegOpenKeyA(root, key_name, &key);
    ok_(__FILE__,line)(res == ERROR_SUCCESS, "Could not open key %s\n", key_name);
    if(res == ERROR_SUCCESS)
        RegCloseKey(key);
}

#define test_key_not_exists(a,b) _test_key_not_exists(__LINE__,a,b)
static void _test_key_not_exists(unsigned line, HKEY root, const char *key_name)
{
    HKEY key;
    DWORD res;

    res = RegOpenKeyA(root, key_name, &key);
    ok_(__FILE__,line)(res == ERROR_FILE_NOT_FOUND, "Attempting to open %s returned %u\n", key_name, res);
    if(res == ERROR_SUCCESS)
        RegCloseKey(key);
}

static void test_regcat(void)
{
    unsigned char b;
    HRESULT hres;

    const struct _ATL_CATMAP_ENTRY catmap[] = {
        {_ATL_CATMAP_ENTRY_IMPLEMENTED, &CATID_CatTest1},
        {_ATL_CATMAP_ENTRY_REQUIRED, &CATID_CatTest2},
        {_ATL_CATMAP_ENTRY_END}
    };

    if (is_process_limited())
    {
        skip("process is limited\n");
        return;
    }

    hres = AtlRegisterClassCategoriesHelper(&CLSID_Test, catmap, TRUE);
    ok(hres == S_OK, "AtlRegisterClassCategoriesHelper failed: %08x\n", hres);

    test_key_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}");
    test_key_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}\\Implemented Categories\\{" CATID_CATTEST1_STR "}");
    test_key_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}\\Required Categories\\{" CATID_CATTEST2_STR "}");

    hres = AtlRegisterClassCategoriesHelper(&CLSID_Test, catmap, FALSE);
    ok(hres == S_OK, "AtlRegisterClassCategoriesHelper failed: %08x\n", hres);

    test_key_not_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}\\Implemented Categories");
    test_key_not_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}\\Required Categories");
    test_key_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}");

    ok(RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}") == ERROR_SUCCESS, "Could not delete key\n");

    hres = AtlRegisterClassCategoriesHelper(&CLSID_Test, NULL, TRUE);
    ok(hres == S_OK, "AtlRegisterClassCategoriesHelper failed: %08x\n", hres);

    test_key_not_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}");

    b = 10;
    hres = AtlGetPerUserRegistration(&b);
    ok(hres == S_OK, "AtlGetPerUserRegistration failed: %08x\n", hres);
    ok(!b, "AtlGetPerUserRegistration returned %x\n", b);
}

static void test_typelib(void)
{
    ITypeLib *typelib;
    HINSTANCE inst;
    size_t len;
    BSTR path;
    HRESULT hres;

    static const WCHAR scrrun_dll_suffixW[] = {'\\','s','c','r','r','u','n','.','d','l','l',0};
    static const WCHAR mshtml_tlb_suffixW[] = {'\\','m','s','h','t','m','l','.','t','l','b',0};

    inst = LoadLibraryA("scrrun.dll");
    ok(inst != NULL, "Could not load scrrun.dll\n");

    typelib = NULL;
    hres = AtlLoadTypeLib(inst, NULL, &path, &typelib);
    ok(hres == S_OK, "AtlLoadTypeLib failed: %08x\n", hres);
    FreeLibrary(inst);

    len = SysStringLen(path);
    ok(len > sizeof(scrrun_dll_suffixW)/sizeof(WCHAR)
       && lstrcmpiW(path+len-sizeof(scrrun_dll_suffixW)/sizeof(WCHAR), scrrun_dll_suffixW),
       "unexpected path %s\n", wine_dbgstr_w(path));
    SysFreeString(path);
    ok(typelib != NULL, "typelib == NULL\n");
    ITypeLib_Release(typelib);

    inst = LoadLibraryA("mshtml.dll");
    ok(inst != NULL, "Could not load mshtml.dll\n");

    typelib = NULL;
    hres = AtlLoadTypeLib(inst, NULL, &path, &typelib);
    ok(hres == S_OK, "AtlLoadTypeLib failed: %08x\n", hres);
    FreeLibrary(inst);

    len = SysStringLen(path);
    ok(len > sizeof(mshtml_tlb_suffixW)/sizeof(WCHAR)
       && lstrcmpiW(path+len-sizeof(mshtml_tlb_suffixW)/sizeof(WCHAR), mshtml_tlb_suffixW),
       "unexpected path %s\n", wine_dbgstr_w(path));
    SysFreeString(path);
    ok(typelib != NULL, "typelib == NULL\n");
    ITypeLib_Release(typelib);
}

static HRESULT WINAPI ConnectionPoint_QueryInterface(IConnectionPoint *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IConnectionPoint, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ConnectionPoint_AddRef(IConnectionPoint *iface)
{
    return 2;
}

static ULONG WINAPI ConnectionPoint_Release(IConnectionPoint *iface)
{
    return 1;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionInterface(IConnectionPoint *iface, IID *pIID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionPointContainer(IConnectionPoint *iface,
        IConnectionPointContainer **ppCPC)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static int advise_cnt;

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *pUnkSink,
                                             DWORD *pdwCookie)
{
    ok(pUnkSink == (IUnknown*)0xdead0000, "pUnkSink = %p\n", pUnkSink);
    *pdwCookie = 0xdeadbeef;
    advise_cnt++;
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD dwCookie)
{
    ok(dwCookie == 0xdeadbeef, "dwCookie = %x\n", dwCookie);
    advise_cnt--;
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_EnumConnections(IConnectionPoint *iface,
                                                      IEnumConnections **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IConnectionPointVtbl ConnectionPointVtbl =
{
    ConnectionPoint_QueryInterface,
    ConnectionPoint_AddRef,
    ConnectionPoint_Release,
    ConnectionPoint_GetConnectionInterface,
    ConnectionPoint_GetConnectionPointContainer,
    ConnectionPoint_Advise,
    ConnectionPoint_Unadvise,
    ConnectionPoint_EnumConnections
};

static IConnectionPoint ConnectionPoint = { &ConnectionPointVtbl };

static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
        REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IConnectionPointContainer, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    return 2;
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    return 1;
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface,
        IEnumConnectionPoints **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface,
        REFIID riid, IConnectionPoint **ppCP)
{
    ok(IsEqualGUID(riid, &CLSID_Test), "unexpected riid\n");
    *ppCP = &ConnectionPoint;
    return S_OK;
}

static const IConnectionPointContainerVtbl ConnectionPointContainerVtbl = {
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

static IConnectionPointContainer ConnectionPointContainer = { &ConnectionPointContainerVtbl };

static void test_cp(void)
{
    DWORD cookie = 0;
    HRESULT hres;

    hres = AtlAdvise(NULL, (IUnknown*)0xdeed0000, &CLSID_Test, &cookie);
    ok(hres == E_INVALIDARG, "expect E_INVALIDARG, returned %08x\n", hres);

    hres = AtlUnadvise(NULL, &CLSID_Test, 0xdeadbeef);
    ok(hres == E_INVALIDARG, "expect E_INVALIDARG, returned %08x\n", hres);

    hres = AtlAdvise((IUnknown*)&ConnectionPointContainer, (IUnknown*)0xdead0000, &CLSID_Test, &cookie);
    ok(hres == S_OK, "AtlAdvise failed: %08x\n", hres);
    ok(cookie == 0xdeadbeef, "cookie = %x\n", cookie);
    ok(advise_cnt == 1, "advise_cnt = %d\n", advise_cnt);

    hres = AtlUnadvise((IUnknown*)&ConnectionPointContainer, &CLSID_Test, 0xdeadbeef);
    ok(hres == S_OK, "AtlUnadvise failed: %08x\n", hres);
    ok(!advise_cnt, "advise_cnt = %d\n", advise_cnt);
}

static CLSID persist_clsid;

static HRESULT WINAPI Persist_QueryInterface(IPersist *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI Persist_AddRef(IPersist *iface)
{
    return 2;
}

static ULONG WINAPI Persist_Release(IPersist *iface)
{
    return 1;
}

static HRESULT WINAPI Persist_GetClassID(IPersist *iface, CLSID *pClassID)
{
    *pClassID = persist_clsid;
    return S_OK;
}

static const IPersistVtbl PersistVtbl = {
    Persist_QueryInterface,
    Persist_AddRef,
    Persist_Release,
    Persist_GetClassID
};

static IPersist Persist = { &PersistVtbl };

static HRESULT WINAPI ProvideClassInfo2_QueryInterface(IProvideClassInfo2 *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ProvideClassInfo2_AddRef(IProvideClassInfo2 *iface)
{
    return 2;
}

static ULONG WINAPI ProvideClassInfo2_Release(IProvideClassInfo2 *iface)
{
    return 1;
}

static HRESULT WINAPI ProvideClassInfo2_GetClassInfo(IProvideClassInfo2 *iface, ITypeInfo **ppTI)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProvideClassInfo2_GetGUID(IProvideClassInfo2 *iface, DWORD dwGuidKind, GUID *pGUID)
{
    ok(dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID, "unexpected dwGuidKind %x\n", dwGuidKind);
    *pGUID = DIID_DispHTMLBody;
    return S_OK;
}

static const IProvideClassInfo2Vtbl ProvideClassInfo2Vtbl = {
    ProvideClassInfo2_QueryInterface,
    ProvideClassInfo2_AddRef,
    ProvideClassInfo2_Release,
    ProvideClassInfo2_GetClassInfo,
    ProvideClassInfo2_GetGUID
};

static IProvideClassInfo2 ProvideClassInfo2 = { &ProvideClassInfo2Vtbl };
static BOOL support_classinfo2;

static HRESULT WINAPI Dispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IProvideClassInfo2, riid)) {
        if(!support_classinfo2)
            return E_NOINTERFACE;
        *ppv = &ProvideClassInfo2;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IPersist, riid)) {
        *ppv = &Persist;
        return S_OK;
    }

    ok(0, "unexpected riid: %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Dispatch_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI Dispatch_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI Dispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    ITypeLib *typelib;
    HRESULT hres;

    static const WCHAR mshtml_tlbW[] = {'m','s','h','t','m','l','.','t','l','b',0};

    ok(!iTInfo, "iTInfo = %d\n", iTInfo);
    ok(!lcid, "lcid = %x\n", lcid);

    hres = LoadTypeLib(mshtml_tlbW, &typelib);
    ok(hres == S_OK, "LoadTypeLib failed: %08x\n", hres);

    hres = ITypeLib_GetTypeInfoOfGuid(typelib, &IID_IHTMLElement, ppTInfo);
    ok(hres == S_OK, "GetTypeInfoOfGuid failed: %08x\n", hres);

    ITypeLib_Release(typelib);
    return S_OK;
}

static HRESULT WINAPI Dispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IDispatchVtbl DispatchVtbl = {
    Dispatch_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch Dispatch = { &DispatchVtbl };

static void test_source_iface(void)
{
    unsigned short maj_ver, min_ver;
    IID libid, iid;
    HRESULT hres;

    support_classinfo2 = TRUE;

    maj_ver = min_ver = 0xdead;
    hres = AtlGetObjectSourceInterface((IUnknown*)&Dispatch, &libid, &iid, &maj_ver, &min_ver);
    ok(hres == S_OK, "AtlGetObjectSourceInterface failed: %08x\n", hres);
    ok(IsEqualGUID(&libid, &LIBID_MSHTML), "libid = %s\n", wine_dbgstr_guid(&libid));
    ok(IsEqualGUID(&iid, &DIID_DispHTMLBody), "iid = %s\n", wine_dbgstr_guid(&iid));
    ok(maj_ver == 4 && min_ver == 0, "ver = %d.%d\n", maj_ver, min_ver);

    support_classinfo2 = FALSE;
    persist_clsid = CLSID_HTMLDocument;

    maj_ver = min_ver = 0xdead;
    hres = AtlGetObjectSourceInterface((IUnknown*)&Dispatch, &libid, &iid, &maj_ver, &min_ver);
    ok(hres == S_OK, "AtlGetObjectSourceInterface failed: %08x\n", hres);
    ok(IsEqualGUID(&libid, &LIBID_MSHTML), "libid = %s\n", wine_dbgstr_guid(&libid));
    ok(IsEqualGUID(&iid, &DIID_HTMLDocumentEvents), "iid = %s\n", wine_dbgstr_guid(&iid));
    ok(maj_ver == 4 && min_ver == 0, "ver = %d.%d\n", maj_ver, min_ver);

    persist_clsid = CLSID_HTMLStyle;

    maj_ver = min_ver = 0xdead;
    hres = AtlGetObjectSourceInterface((IUnknown*)&Dispatch, &libid, &iid, &maj_ver, &min_ver);
    ok(hres == S_OK, "AtlGetObjectSourceInterface failed: %08x\n", hres);
    ok(IsEqualGUID(&libid, &LIBID_MSHTML), "libid = %s\n", wine_dbgstr_guid(&libid));
    ok(IsEqualGUID(&iid, &IID_NULL), "iid = %s\n", wine_dbgstr_guid(&iid));
    ok(maj_ver == 4 && min_ver == 0, "ver = %d.%d\n", maj_ver, min_ver);
}

static void test_ax_win(void)
{
    DWORD ret, ret_size, i;
    HRESULT res;
    WNDCLASSEXW wcex;
    HWND hwnd;
    HANDLE hfile;
    IUnknown *control;
    static const WCHAR AtlAxWin100[] = {'A','t','l','A','x','W','i','n','1','0','0',0};
    static const WCHAR AtlAxWinLic100[] = {'A','t','l','A','x','W','i','n','L','i','c','1','0','0',0};
    static const WCHAR *cls_names[2] = {AtlAxWin100, AtlAxWinLic100};
    WNDPROC wndproc[2] = {NULL, NULL};
    static HMODULE hinstance = 0;
    static const WCHAR emptyW[] = {'\0'};
    static const WCHAR randomW[] = {'r','a','n','d','o','m','\0'};
    static const WCHAR progid1W[] = {'S','h','e','l','l','.','E','x','p','l','o','r','e','r','.','2','\0'};
    static const WCHAR clsid1W[] = {'{','8','8','5','6','f','9','6','1','-','3','4','0','a','-',
                                    '1','1','d','0','-','a','9','6','b','-',
                                    '0','0','c','0','4','f','d','7','0','5','a','2','}','\0'};
    static const WCHAR url1W[] = {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q',
                                  '.','o','r','g','/','t','e','s','t','s','/','w','i','n','e','h','q','_',
                                  's','n','a','p','s','h','o','t','/','\0'};
    static const WCHAR mshtml1W[] = {'m','s','h','t','m','l',':','<','h','t','m','l','>','<','b','o','d','y','>',
                                     't','e','s','t','<','/','b','o','d','y','>','<','/','h','t','m','l','>','\0'};
    static const WCHAR mshtml2W[] = {'M','S','H','T','M','L',':','<','h','t','m','l','>','<','b','o','d','y','>',
                                     't','e','s','t','<','/','b','o','d','y','>','<','/','h','t','m','l','>','\0'};
    static const WCHAR mshtml3W[] = {'<','h','t','m','l','>','<','b','o','d','y','>', 't','e','s','t',
                                     '<','/','b','o','d','y','>','<','/','h','t','m','l','>','\0'};
    static const WCHAR binW[] = {'b','i','n','\0'};
    static const char bin_body[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}; /* random binary data */
    WCHAR bin_fileW[MAX_PATH], tmp_pathW[MAX_PATH], file_uri1W[MAX_PATH] = {'f','i','l','e',':','/','/','/','\0'};

    ret = AtlAxWinInit();
    ok(ret, "AtlAxWinInit failed\n");

    hinstance = GetModuleHandleA(NULL);

    for (i = 0; i < 2; i++)
    {
        memset(&wcex, 0, sizeof(wcex));
        wcex.cbSize = sizeof(wcex);
        ret = GetClassInfoExW(hinstance, AtlAxWin100, &wcex);
        ok(ret, "AtlAxWin100 has not registered\n");
        ok(wcex.style == (CS_GLOBALCLASS | CS_DBLCLKS), "wcex.style %08x\n", wcex.style);
        wndproc[i] = wcex.lpfnWndProc;

        hwnd = CreateWindowW(cls_names[i], NULL, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = (IUnknown *)0xdeadbeef;
        res = AtlAxGetControl(hwnd, &control);
        todo_wine ok(res == E_FAIL, "AtlAxGetControl failed with res %08x\n", res);
        todo_wine ok(!control, "returned %p\n", control);
        if (control) IUnknown_Release(control);
        DestroyWindow(hwnd);

        hwnd = CreateWindowW(cls_names[i], emptyW, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = (IUnknown *)0xdeadbeef;
        res = AtlAxGetControl(hwnd, &control);
        todo_wine ok(res == E_FAIL, "AtlAxGetControl failed with res %08x\n", res);
        todo_wine ok(!control, "returned %p\n", control);
        if (control) IUnknown_Release(control);
        DestroyWindow(hwnd);

        hwnd = CreateWindowW(cls_names[i], randomW, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        todo_wine ok(!hwnd, "returned %p\n", hwnd);
        if(hwnd) DestroyWindow(hwnd);

        hwnd = CreateWindowW(cls_names[i], progid1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08x\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        hwnd = CreateWindowW(cls_names[i], clsid1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08x\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        hwnd = CreateWindowW(cls_names[i], url1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08x\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        /* test html stream with "MSHTML:" prefix */
        hwnd = CreateWindowW(cls_names[i], mshtml1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08x\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        hwnd = CreateWindowW(cls_names[i], mshtml2W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08x\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        /* test html stream without "MSHTML:" prefix */
        hwnd = CreateWindowW(cls_names[i], mshtml3W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        todo_wine ok(!hwnd, "returned %p\n", hwnd);
        if(hwnd) DestroyWindow(hwnd);

        if(winetest_interactive)
        {
            ret = GetTempPathW(MAX_PATH, tmp_pathW);
            ok(ret, "GetTempPath failed!\n");
            ret = GetTempFileNameW(tmp_pathW, binW, 0, bin_fileW);
            ok(ret, "GetTempFileName failed!\n");
            hfile = CreateFileW(bin_fileW, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
            ok(hfile != INVALID_HANDLE_VALUE, "failed to create file\n");
            ret = WriteFile(hfile, bin_body, sizeof(bin_body), &ret_size, NULL);
            ok(ret, "WriteFile failed\n");
            CloseHandle(hfile);
            lstrcatW(file_uri1W, bin_fileW);

            /* test file:// scheme, timeout on Windows 8 */
            hwnd = CreateWindowW(cls_names[i], file_uri1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
            ok(hwnd != NULL, "CreateWindow failed!\n");
            control = NULL;
            res = AtlAxGetControl(hwnd, &control);
            ok(res == S_OK, "AtlAxGetControl failed with res %08x\n", res);
            ok(control != NULL, "AtlAxGetControl failed!\n");
            IUnknown_Release(control);
            DestroyWindow(hwnd);

            ret = DeleteFileW(bin_fileW);
            ok(ret, "DeleteFile failed!\n");

            /* test file:// scheme on non-existent file : Windows 7 popup a dialog complaining file doesn't exist. */
            lstrcatW(file_uri1W, bin_fileW);
            hwnd = CreateWindowW(cls_names[i], file_uri1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
            ok(hwnd != NULL, "CreateWindow failed!\n");
            control = NULL;
            res = AtlAxGetControl(hwnd, &control);
            ok(res == S_OK, "AtlAxGetControl failed with res %08x\n", res);
            ok(control != NULL, "AtlAxGetControl failed!\n");
            IUnknown_Release(control);
            DestroyWindow(hwnd);
        }
    }

    ok(wndproc[0] == wndproc[1], "expected same proc!\n");
}

static ATOM register_class(void)
{
    WNDCLASSA wndclassA;

    wndclassA.style = 0;
    wndclassA.lpfnWndProc = DefWindowProcA;
    wndclassA.cbClsExtra = 0;
    wndclassA.cbWndExtra = 0;
    wndclassA.hInstance = GetModuleHandleA(NULL);
    wndclassA.hIcon = NULL;
    wndclassA.hCursor = LoadCursorA(NULL, (LPSTR)IDC_ARROW);
    wndclassA.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wndclassA.lpszMenuName = NULL;
    wndclassA.lpszClassName = "WineAtlTestClass";

    return RegisterClassA(&wndclassA);
}

static HWND create_container_window(void)
{
    return CreateWindowA("WineAtlTestClass", "Wine ATL Test Window", 0,
                              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, NULL, NULL, NULL, NULL);
}

static void test_AtlAxAttachControl(void)
{
    HWND hwnd = create_container_window();
    HRESULT hr;
    IUnknown *control, *container;
    LONG val;

    hr = AtlAxAttachControl(NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected AtlAxAttachControl to return E_INVALIDARG, got 0x%08x\n", hr);

    container = (IUnknown *)0xdeadbeef;
    hr = AtlAxAttachControl(NULL, NULL, &container);
    ok(hr == E_INVALIDARG, "Expected AtlAxAttachControl to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(container == (IUnknown *)0xdeadbeef,
       "Expected the output container pointer to be untouched, got %p\n", container);

    hr = AtlAxAttachControl(NULL, hwnd, NULL);
    ok(hr == E_INVALIDARG, "Expected AtlAxAttachControl to return E_INVALIDARG, got 0x%08x\n", hr);

    hr = CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                          &IID_IOleObject, (void **)&control);
    ok(hr == S_OK, "Expected CoCreateInstance to return S_OK, got 0x%08x\n", hr);

    if (FAILED(hr))
    {
        skip("Couldn't obtain a test IOleObject instance\n");
        return;
    }

    hr = AtlAxAttachControl(control, NULL, NULL);
    ok(hr == S_FALSE, "Expected AtlAxAttachControl to return S_FALSE, got 0x%08x\n", hr);

    container = NULL;
    hr = AtlAxAttachControl(control, NULL, &container);
    ok(hr == S_FALSE, "Expected AtlAxAttachControl to return S_FALSE, got 0x%08x\n", hr);
    ok(container != NULL, "got %p\n", container);
    IUnknown_Release(container);

    SetWindowLongW(hwnd, GWLP_USERDATA, 0xdeadbeef);
    hr = AtlAxAttachControl(control, hwnd, NULL);
    ok(hr == S_OK, "Expected AtlAxAttachControl to return S_OK, got 0x%08x\n", hr);
    val = GetWindowLongW(hwnd, GWLP_USERDATA);
    ok(val == 0xdeadbeef, "returned %08x\n", val);

    SetWindowLongW(hwnd, GWLP_USERDATA, 0xdeadbeef);
    container = NULL;
    hr = AtlAxAttachControl(control, hwnd, &container);
    ok(hr == S_OK, "Expected AtlAxAttachControl to return S_OK, got 0x%08x\n", hr);
    ok(container != NULL, "Expected not NULL!\n");
    val = GetWindowLongW(hwnd, GWLP_USERDATA);
    todo_wine ok(val == 0xdeadbeef, "Expected unchanged, returned %08x\n", val);

    IUnknown_Release(control);

    DestroyWindow(hwnd);
}

static void test_AtlAxAttachControl2(void)
{
    HWND hwnd = create_container_window();
    IUnknown *control = (IUnknown *)&OleObject;
    IUnknown *container;
    HRESULT hr;

    SET_EXPECT(GetMiscStatus);
    SET_EXPECT(SetClientSite);
    SET_EXPECT(Advise);
    SET_EXPECT(SetAdvise);
    SET_EXPECT(SetHostNames);
    SET_EXPECT(SetExtent);
    SET_EXPECT(GetExtent);
    SET_EXPECT(DoVerb);
    SET_EXPECT(LockRunning);
    SET_EXPECT(Draw);
    SET_EXPECT(SetSite);

    container = NULL;
    hr = AtlAxAttachControl(control, hwnd, &container);
    ok(hr == S_OK, "Expected AtlAxAttachControl to return S_OK, got 0x%08x\n", hr);
    ok(container != NULL, "Expected not NULL!\n");

    todo_wine CHECK_CALLED(GetMiscStatus);
    CHECK_CALLED(SetClientSite);
    todo_wine CHECK_CALLED(Advise);
    todo_wine CHECK_CALLED(SetAdvise);
    CHECK_CALLED(SetHostNames);
    CHECK_CALLED(SetExtent);
    todo_wine CHECK_CALLED(GetExtent);
    CHECK_CALLED(DoVerb);
    todo_wine CHECK_CALLED(LockRunning);
    todo_wine CHECK_CALLED(Draw);
    todo_wine CHECK_CALLED(SetSite);

    SET_EXPECT(SetAdvise_NULL);
    SET_EXPECT(Unadvise);
    SET_EXPECT(Close);
    SET_EXPECT(SetClientSite_NULL);
    SET_EXPECT(SetSite_NULL);

    DestroyWindow(hwnd);

    todo_wine CHECK_CALLED(SetAdvise_NULL);
    todo_wine CHECK_CALLED(Unadvise);
    CHECK_CALLED(Close);
    CHECK_CALLED(SetClientSite_NULL);
    todo_wine CHECK_CALLED(SetSite_NULL);
}

START_TEST(atl)
{
    if (!register_class())
        return;

    CoInitialize(NULL);

    test_winmodule();
    test_regcat();
    test_typelib();
    test_cp();
    test_source_iface();
    test_ax_win();
    test_AtlAxAttachControl();
    test_AtlAxAttachControl2();

    CoUninitialize();
}
