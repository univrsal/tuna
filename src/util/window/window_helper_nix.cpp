#include "window_helper.hpp"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <list>
#include <obs-module.h>
#include <string>
#include <unistd.h>
#include <util/platform.h>
#include <utility>
#include <vector>

/*
    https://github.com/obsproject/obs-studio/blob/master/plugins/linux-capture/xcompcap-helper.cpp
*/
using namespace std;
namespace x11util {
static Display* xdisplay = 0;

Display* disp()
{
    if (!xdisplay)
        xdisplay = XOpenDisplay(nullptr);

    return xdisplay;
}

bool ewmhIsSupported()
{
    Display* display = disp();
    Atom netSupportingWmCheck = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", true);
    Atom actualType;
    int format = 0;
    unsigned long num = 0, bytes = 0;
    unsigned char* data = nullptr;
    Window ewmh_window = 0;

    int status = XGetWindowProperty(display, DefaultRootWindow(display), netSupportingWmCheck, 0L, 1L, false, XA_WINDOW,
        &actualType, &format, &num, &bytes, &data);

    if (status == Success) {
        if (num > 0) {
            ewmh_window = ((Window*)data)[0];
        }
        if (data) {
            XFree(data);
            data = nullptr;
        }
    }

    if (ewmh_window) {
        status = XGetWindowProperty(display, ewmh_window, netSupportingWmCheck, 0L, 1L, false, XA_WINDOW, &actualType,
            &format, &num, &bytes, &data);
        if (status != Success || num == 0 || ewmh_window != ((Window*)data)[0]) {
            ewmh_window = 0;
        }
        if (status == Success && data) {
            XFree(data);
        }
    }

    return ewmh_window != 0;
}

list<Window> getTopLevelWindows()
{
    list<Window> res;

    if (!ewmhIsSupported()) {
        blog(LOG_WARNING, "Unable to query window list "
                          "because window manager "
                          "does not support extended "
                          "window manager Hints");
        return res;
    }

    Atom netClList = XInternAtom(disp(), "_NET_CLIENT_LIST", true);
    Atom actualType;
    int format;
    unsigned long num, bytes;
    Window* data = 0;

    for (int i = 0; i < ScreenCount(disp()); ++i) {
        Window rootWin = RootWindow(disp(), i);

        int status = XGetWindowProperty(disp(), rootWin, netClList, 0L, ~0L, false, AnyPropertyType, &actualType,
            &format, &num, &bytes, (uint8_t**)&data);

        if (status != Success) {
            blog(LOG_WARNING, "Failed getting root "
                              "window properties");
            continue;
        }

        for (unsigned long i = 0; i < num; ++i) {
            res.push_back(data[i]);
        }

        XFree(data);
    }

    return res;
}

string getWindowAtom(Window win, const char* atom)
{
    Atom netWmName = XInternAtom(disp(), atom, false);
    int n;
    char** list = 0;
    XTextProperty tp;
    string res = "unknown";

    XGetTextProperty(disp(), win, &tp, netWmName);

    if (!tp.nitems)
        XGetWMName(disp(), win, &tp);

    if (!tp.nitems)
        return "error";

    if (tp.encoding == XA_STRING) {
        res = (char*)tp.value;
    } else {
        int ret = XmbTextPropertyToTextList(disp(), &tp, &list, &n);

        if (ret >= Success && n > 0 && *list) {
            res = *list;
            XFreeStringList(list);
        }
    }

    char* conv = nullptr;
    if (os_mbs_to_utf8_ptr(res.c_str(), 0, &conv))
        res = conv;
    bfree(conv);

    XFree(tp.value);

    return res;
}

inline string getWindowName(Window win)
{
    return getWindowAtom(win, "_NET_WM_NAME");
}

inline string getWindowClass(Window win)
{
    return getWindowAtom(win, "WM_CLASS");
}

inline string getWindowExe(Window win)
{
    Atom windowPID = XInternAtom(disp(), "_NET_WM_PID", true);
    Atom actualType;
    int format;
    unsigned long num, bytes;
    unsigned char* propPID = nullptr;
    if (windowPID != None) {
        if (XGetWindowProperty(disp(), win, windowPID, 0, 1, False, XA_CARDINAL,
                &actualType, &format, &num, &bytes, &propPID)
            == Success) {
            if (propPID != nullptr) {
                int pidInt = *((int*)propPID);
                auto pid_str = "/proc/" + to_string(pidInt) + "/exe";
                XFree(propPID);
                char exe[1024];
                memset(exe, 0, 1024);
                if (readlink(pid_str.c_str(), exe, 1024) > 0) {
                    return exe;
                }
            }
        }
    }
    return "";
}

} // namespace x11util

void GetWindowList(vector<string>& windows)
{
    list<Window> top_level = x11util::getTopLevelWindows();
    for (const auto& window : top_level) {
        windows.emplace_back(x11util::getWindowName(window));
    }
}

void GetWindowAndExeList(vector<pair<string, string>>& list)
{
    auto top_level = x11util::getTopLevelWindows();
    for (const auto& window : top_level) {
        auto exe = x11util::getWindowExe(window);
        if (!exe.empty()) {
            list.emplace_back(pair<string, string>(exe,
                x11util::getWindowName(window)));
        }
    }
}
