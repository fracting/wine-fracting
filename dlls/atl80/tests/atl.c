/*
 * Copyright 2013 Qian Hong for CodeWeavers
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
static const WCHAR fileW[] = {'f','i','l','e',':','/','/','/','\0'};
static const WCHAR html_fileW[] = {'t','e','s','t','.','h','t','m','l','\0'};
static const char html_str[] = "<html><body>test</body><html>";

static void test_ax_win(void)
{
    DWORD ret, ret_size, i;
    HRESULT res;
    WNDCLASSEXW wcex;
    HWND hwnd;
    HANDLE hfile;
    IUnknown *control;
    static const WCHAR AtlAxWin80[] = {'A','t','l','A','x','W','i','n','8','0',0};
    static const WCHAR AtlAxWinLic80[] = {'A','t','l','A','x','W','i','n','L','i','c','8','0',0};
    static const WCHAR *cls_names[2] = {AtlAxWin80, AtlAxWinLic80};
    WNDPROC wndproc[2] = {NULL, NULL};
    static HMODULE hinstance = 0;
    WCHAR file_uri1W[MAX_PATH], pathW[MAX_PATH];

    ret = AtlAxWinInit();
    ok(ret, "AtlAxWinInit failed\n");
    ret = AtlAxWinInit();
    todo_wine ok(ret, "AtlAxWinInit failed\n");

    hinstance = GetModuleHandleA(NULL);

    for (i = 0; i < 2; i++)
    {
        memset(&wcex, 0, sizeof(wcex));
        wcex.cbSize = sizeof(wcex);
        ret = GetClassInfoExW(hinstance, cls_names[i], &wcex);
        ok(ret, "%s has not registered\n", wine_dbgstr_w(cls_names[i]));
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

        ret = GetTempPathW(MAX_PATH, pathW);
        ok(ret, "GetTempPath failed!\n");
        lstrcatW(pathW, html_fileW);
        hfile = CreateFileW(pathW, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0);
        ok(hfile != INVALID_HANDLE_VALUE, "failed to create file\n");
        ret = WriteFile(hfile, html_str, sizeof(html_str), &ret_size, NULL);
        ok(ret, "WriteFile failed\n");
        CloseHandle(hfile);

        /* test C:// scheme, timeout on Windows 8 */
        hwnd = CreateWindowW(cls_names[i], pathW, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08x\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        /* test file:// scheme, timeout on Windows 8 */
        lstrcpyW(file_uri1W, fileW);
        lstrcatW(file_uri1W, pathW);
        hwnd = CreateWindowW(cls_names[i], file_uri1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08x\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        /* test file:// scheme on non-existent file : Windows 7 popup a dialog complaining file doesn't exist. */
        ret = DeleteFileW(pathW);
        ok(ret, "DeleteFile failed!\n");
        hwnd = CreateWindowW(cls_names[i], file_uri1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08x\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);
    }

    todo_wine ok(wndproc[0] != wndproc[1], "expected different proc!\n");
}

START_TEST(atl)
{
    CoInitialize(NULL);

    test_ax_win();

    CoUninitialize();
}
