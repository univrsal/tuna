/**
 * This file is part of obs-studio
 * which is licensed under the GPL 2.0
 * See COPYING or https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 * https://github.com/obsproject/obs-studio/blob/master/UI/frontend-plugins/frontend-tools/auto-scene-switcher-win.cpp
 */

#include "window_helper.hpp"
#include <util/platform.h>
#include <windows.h>

using namespace std;

static bool GetWindowTitle(HWND window, string& title)
{
    size_t len = (size_t)GetWindowTextLengthW(window);
    wstring wtitle;

    wtitle.resize(len);
    if (!GetWindowTextW(window, &wtitle[0], (int)len + 1))
        return false;

    len = os_wcs_to_utf8(wtitle.c_str(), 0, nullptr, 0);
    title.resize(len);
    os_wcs_to_utf8(wtitle.c_str(), 0, &title[0], len + 1);
    return true;
}

static bool GetWindowExe(HWND window, string& exe)
{
    bool result = false;
    DWORD proc_id = 0;
    HANDLE h = NULL;
    GetWindowThreadProcessId(window, &proc_id);
    h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, proc_id);
    if (h) {
        DWORD l = 1024;
        size_t len = 1024;
        wchar_t buf[1024];
        result = QueryFullProcessImageNameW(h, NULL, buf, &l);
        if (result) {
            len = os_wcs_to_utf8(buf, 0, nullptr, 0);
            exe.resize(len);
            os_wcs_to_utf8(buf, 0, &exe[0], len + 1);
        }
        CloseHandle(h);
    }
    return result;
}

static bool WindowValid(HWND window)
{
    LONG_PTR styles, ex_styles;
    RECT rect;
    DWORD id;

    if (!IsWindowVisible(window))
        return false;
    GetWindowThreadProcessId(window, &id);
    if (id == GetCurrentProcessId())
        return false;

    GetClientRect(window, &rect);
    styles = GetWindowLongPtr(window, GWL_STYLE);
    ex_styles = GetWindowLongPtr(window, GWL_EXSTYLE);

    if (ex_styles & WS_EX_TOOLWINDOW)
        return false;
    if (styles & WS_CHILD)
        return false;

    return true;
}

void GetWindowList(vector<string>& windows)
{
    HWND window = GetWindow(GetDesktopWindow(), GW_CHILD);

    while (window) {
        string title;
        if (WindowValid(window) && GetWindowTitle(window, title))
            windows.emplace_back(title);
        window = GetNextWindow(window, GW_HWNDNEXT);
    }
}

void GetWindowAndExeList(vector<pair<string, string>>& list)
{
    HWND window = GetWindow(GetDesktopWindow(), GW_CHILD);

    while (window) {
        string title, exe;
        if (WindowValid(window) && GetWindowTitle(window, title) && GetWindowExe(window, exe)) {
            list.emplace_back(pair<string, string>(exe, title));
        }
        window = GetNextWindow(window, GW_HWNDNEXT);
    }
}
