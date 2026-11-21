#include <iostream>
#include "../dsp/ThirdBassDSP.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../gui/WinD2DUI.hpp"

static LRESULT CALLBACK StandaloneHostWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    (void)nCmdShow;
    ThirdBass::ThirdBassEngine engine;
    ThirdBass::PluginUI ui(&engine);

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = StandaloneHostWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "ThirdAssStandaloneHostClass";
    RegisterClassExA(&wc);

    RECT rc = { 0, 0, ThirdBass::PluginUI::UI_WIDTH, ThirdBass::PluginUI::UI_HEIGHT };
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    HWND hostHWnd = CreateWindowExA(
        0,
        "ThirdAssStandaloneHostClass",
        "Third Ass - Bass Multi-Effects & Amp System (Direct2D Win64)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hostHWnd) {
        return 1;
    }

    ui.openWindow(hostHWnd);

    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    ui.closeWindow();
    return 0;
}

int main() {
    return WinMain(GetModuleHandle(nullptr), nullptr, nullptr, SW_SHOW);
}

#else

#include <X11/Xlib.h>
#include <unistd.h>
#include "../gui/CairoUI.hpp"

int main() {
    std::cout << "Starting Third Bass Standalone GUI preview...\n";
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) {
        std::cerr << "Failed to open X11 Display!\n";
        return 1;
    }

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    XSetWindowAttributes attr;
    attr.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask |
                      ButtonReleaseMask | PointerMotionMask | KeyPressMask;

    Window topWin = XCreateWindow(dpy, root, 100, 100,
                                  ThirdBass::PluginUI::UI_WIDTH,
                                  ThirdBass::PluginUI::UI_HEIGHT,
                                  0, DefaultDepth(dpy, screen), InputOutput,
                                  DefaultVisual(dpy, screen),
                                  CWEventMask, &attr);

    XStoreName(dpy, topWin, "Third Bass - Bass Multi-Effects & Amp System");
    XMapWindow(dpy, topWin);
    XFlush(dpy);

    ThirdBass::ThirdBassEngine engine;
    ThirdBass::PluginUI ui(&engine);
    ui.openWindow(dpy, topWin);

    // Render loop
    std::cout << "UI window created (980x560). Close window or press Ctrl+C to exit.\n";
    
    // Process initial events and take screenshot / run loop
    bool running = true;
    while (running) {
        while (XPending(dpy) > 0) {
            XEvent ev;
            XNextEvent(dpy, &ev);
            if (ev.type == DestroyNotify || (ev.type == KeyPress && XLookupKeysym(&ev.xkey, 0) == 0xff1b)) { // ESC
                running = false;
                break;
            }
            ui.handleEvent(&ev);
        }
        ui.idle();
        usleep(16000); // ~60 fps
    }

    ui.closeWindow();
    XDestroyWindow(dpy, topWin);
    XCloseDisplay(dpy);
    return 0;
}

#endif
