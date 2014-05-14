/*
 * Copyright 2014 Piotr Caban for CodeWeavers
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

#define COBJMACROS

#include "oleacc_private.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oleacc);

typedef struct {
    IAccessible IAccessible_iface;

    LONG ref;
} Window;

static inline Window* impl_from_Window(IAccessible *iface)
{
    return CONTAINING_RECORD(iface, Window, IAccessible_iface);
}

static HRESULT WINAPI Window_QueryInterface(IAccessible *iface, REFIID riid, void **ppv)
{
    Window *This = impl_from_Window(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if(IsEqualIID(riid, &IID_IAccessible) ||
            IsEqualIID(riid, &IID_IDispatch) ||
            IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = iface;
        IAccessible_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI Window_AddRef(IAccessible *iface)
{
    Window *This = impl_from_Window(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %u\n", This, ref);
    return ref;
}

static ULONG WINAPI Window_Release(IAccessible *iface)
{
    Window *This = impl_from_Window(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %u\n", This, ref);

    if(!ref)
        heap_free(This);
    return ref;
}

static HRESULT WINAPI Window_GetTypeInfoCount(IAccessible *iface, UINT *pctinfo)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_GetTypeInfo(IAccessible *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%u %x %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_GetIDsOfNames(IAccessible *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s %p %u %x %p)\n", This, debugstr_guid(riid),
            rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_Invoke(IAccessible *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%x %s %x %x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accParent(IAccessible *iface, IDispatch **ppdispParent)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%p)\n", This, ppdispParent);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accChildCount(IAccessible *iface, LONG *pcountChildren)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%p)\n", This, pcountChildren);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accChild(IAccessible *iface,
        VARIANT varChildID, IDispatch **ppdispChild)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varChildID), ppdispChild);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accName(IAccessible *iface, VARIANT varID, BSTR *pszName)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszName);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accValue(IAccessible *iface, VARIANT varID, BSTR *pszValue)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accDescription(IAccessible *iface,
        VARIANT varID, BSTR *pszDescription)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszDescription);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accRole(IAccessible *iface, VARIANT varID, VARIANT *pvarRole)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pvarRole);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accState(IAccessible *iface, VARIANT varID, VARIANT *pvarState)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pvarState);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accHelp(IAccessible *iface, VARIANT varID, BSTR *pszHelp)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszHelp);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accHelpTopic(IAccessible *iface,
        BSTR *pszHelpFile, VARIANT varID, LONG *pidTopic)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%p %s %p)\n", This, pszHelpFile, debugstr_variant(&varID), pidTopic);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accKeyboardShortcut(IAccessible *iface,
        VARIANT varID, BSTR *pszKeyboardShortcut)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszKeyboardShortcut);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accFocus(IAccessible *iface, VARIANT *pvarID)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%p)\n", This, pvarID);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accSelection(IAccessible *iface, VARIANT *pvarID)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%p)\n", This, pvarID);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_get_accDefaultAction(IAccessible *iface,
        VARIANT varID, BSTR *pszDefaultAction)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszDefaultAction);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_accSelect(IAccessible *iface, LONG flagsSelect, VARIANT varID)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%x %s)\n", This, flagsSelect, debugstr_variant(&varID));
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_accLocation(IAccessible *iface, LONG *pxLeft,
        LONG *pyTop, LONG *pcxWidth, LONG *pcyHeight, VARIANT varID)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%p %p %p %p %s)\n", This, pxLeft, pyTop,
            pcxWidth, pcyHeight, debugstr_variant(&varID));
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_accNavigate(IAccessible *iface,
        LONG navDir, VARIANT varStart, VARIANT *pvarEnd)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%d %s %p)\n", This, navDir, debugstr_variant(&varStart), pvarEnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_accHitTest(IAccessible *iface,
        LONG xLeft, LONG yTop, VARIANT *pvarID)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%d %d %p)\n", This, xLeft, yTop, pvarID);
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_accDoDefaultAction(IAccessible *iface, VARIANT varID)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&varID));
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_put_accName(IAccessible *iface, VARIANT varID, BSTR pszName)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_variant(&varID), debugstr_w(pszName));
    return E_NOTIMPL;
}

static HRESULT WINAPI Window_put_accValue(IAccessible *iface, VARIANT varID, BSTR pszValue)
{
    Window *This = impl_from_Window(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_variant(&varID), debugstr_w(pszValue));
    return E_NOTIMPL;
}

static const IAccessibleVtbl WindowVtbl = {
    Window_QueryInterface,
    Window_AddRef,
    Window_Release,
    Window_GetTypeInfoCount,
    Window_GetTypeInfo,
    Window_GetIDsOfNames,
    Window_Invoke,
    Window_get_accParent,
    Window_get_accChildCount,
    Window_get_accChild,
    Window_get_accName,
    Window_get_accValue,
    Window_get_accDescription,
    Window_get_accRole,
    Window_get_accState,
    Window_get_accHelp,
    Window_get_accHelpTopic,
    Window_get_accKeyboardShortcut,
    Window_get_accFocus,
    Window_get_accSelection,
    Window_get_accDefaultAction,
    Window_accSelect,
    Window_accLocation,
    Window_accNavigate,
    Window_accHitTest,
    Window_accDoDefaultAction,
    Window_put_accName,
    Window_put_accValue
};

HRESULT create_window_object(HWND hwnd, const IID *iid, void **obj)
{
    Window *window;
    HRESULT hres;

    if(!IsWindow(hwnd))
        return E_FAIL;

    window = heap_alloc_zero(sizeof(Window));
    if(!window)
        return E_OUTOFMEMORY;

    window->IAccessible_iface.lpVtbl = &WindowVtbl;
    window->ref = 1;

    hres = IAccessible_QueryInterface(&window->IAccessible_iface, iid, obj);
    IAccessible_Release(&window->IAccessible_iface);
    return hres;
}