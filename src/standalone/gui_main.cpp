#include <iostream>
#include <X11/Xlib.h>
#include <unistd.h>
#include "../dsp/ThirdBassDSP.hpp"
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
