#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <mutex>
#include "../dsp/ThirdBassDSP.hpp"

static inline void thirdAssLog(const char* fmt, ...) {
    static const char* enableDebug = getenv("THIRD_ASS_DEBUG");
    if (!enableDebug) return;
    FILE* f = fopen("/tmp/thirdass_debug.log", "a");
    if (!f) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fclose(f);
}

static inline int s_xErrorHandler(Display* d, XErrorEvent* e) {
    char buf[256];
    XGetErrorText(d, e->error_code, buf, sizeof(buf));
    thirdAssLog("[X11 Error] code=%d (%s), req=%d, minor=%d, resID=%lu\n",
                e->error_code, buf, e->request_code, e->minor_code, (unsigned long)e->resourceid);
    return 0;
}

struct XThreadsInitializer {
    XThreadsInitializer() {
        XInitThreads();
        XSetErrorHandler(s_xErrorHandler);
    }
};
static XThreadsInitializer s_xThreadsInitializer;

// Self-contained Cairo C API declarations (links dynamically against libcairo.so.2)
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_pattern cairo_pattern_t;

typedef struct {
    double x_bearing, y_bearing, width, height, x_advance, y_advance;
} cairo_text_extents_t;

typedef enum {
    CAIRO_FORMAT_ARGB32 = 0,
    CAIRO_FORMAT_RGB24 = 1,
    CAIRO_FORMAT_A8 = 2,
    CAIRO_FORMAT_A1 = 3
} cairo_format_t;

extern "C" {
    cairo_surface_t* cairo_xlib_surface_create(Display* dpy, Drawable drawable, Visual* visual, int width, int height);
    cairo_surface_t* cairo_image_surface_create(cairo_format_t format, int width, int height);
    unsigned char* cairo_image_surface_get_data(cairo_surface_t* surface);
    int cairo_image_surface_get_stride(cairo_surface_t* surface);
    void cairo_surface_flush(cairo_surface_t* surface);
    void cairo_surface_destroy(cairo_surface_t* surface);
    cairo_t* cairo_create(cairo_surface_t* target);
    void cairo_destroy(cairo_t* cr);
    void cairo_save(cairo_t* cr);
    void cairo_restore(cairo_t* cr);
    void cairo_set_source_rgb(cairo_t* cr, double red, double green, double blue);
    void cairo_set_source_rgba(cairo_t* cr, double red, double green, double blue, double alpha);
    void cairo_rectangle(cairo_t* cr, double x, double y, double width, double height);
    void cairo_arc(cairo_t* cr, double xc, double yc, double radius, double angle1, double angle2);
    void cairo_fill(cairo_t* cr);
    void cairo_stroke(cairo_t* cr);
    void cairo_set_line_width(cairo_t* cr, double width);
    void cairo_set_line_cap(cairo_t* cr, int line_cap);
    void cairo_set_dash(cairo_t* cr, const double* dashes, int num_dashes, double offset);
    void cairo_move_to(cairo_t* cr, double x, double y);
    void cairo_line_to(cairo_t* cr, double x, double y);
    void cairo_curve_to(cairo_t* cr, double x1, double y1, double x2, double y2, double x3, double y3);
    void cairo_select_font_face(cairo_t* cr, const char* family, int slant, int weight);
    void cairo_set_font_size(cairo_t* cr, double size);
    void cairo_show_text(cairo_t* cr, const char* utf8);
    void cairo_text_extents(cairo_t* cr, const char* utf8, cairo_text_extents_t* extents);
    cairo_pattern_t* cairo_pattern_create_linear(double x0, double y0, double x1, double y1);
    void cairo_pattern_add_color_stop_rgba(cairo_pattern_t* pattern, double offset, double red, double green, double blue, double alpha);
    void cairo_set_source(cairo_t* cr, cairo_pattern_t* pattern);
    void cairo_pattern_destroy(cairo_pattern_t* pattern);
    void cairo_clip(cairo_t* cr);
    void cairo_reset_clip(cairo_t* cr);
    void cairo_new_path(cairo_t* cr);
    void cairo_close_path(cairo_t* cr);
    void cairo_paint(cairo_t* cr);
}

namespace ThirdBass {

struct Color {
    double r, g, b, a;
    Color(double red, double green, double blue, double alpha = 1.0)
        : r(red), g(green), b(blue), a(alpha) {}
};

struct KnobWidget {
    int paramId;
    float x, y, radius;
    std::string title;
    Color accentColor;
};

struct ButtonWidget {
    int paramId;
    float x, y, w, h;
    std::string text;
    float targetValue; // for multi-choice buttons
    bool isToggle;
    Color activeColor;
};

class PluginUI {
public:
    static constexpr int UI_WIDTH = 980;
    static constexpr int UI_HEIGHT = 560;

    PluginUI(ThirdBassEngine* eng) : engine(eng) {
        initLayout();
    }

    ~PluginUI() {
        closeWindow();
    }

    bool openWindow(Display* /*ignoredDpy*/, Window parentWin) {
        thirdAssLog("[CairoUI] openWindow called: parentWin=0x%lx\n", (unsigned long)parentWin);
        closeWindow();

        display = XOpenDisplay(nullptr);
        if (!display) {
            thirdAssLog("[CairoUI] XOpenDisplay failed!\n");
            return false;
        }

        parentWindow = parentWin;
        screen = DefaultScreen(display);

        XWindowAttributes pAttrs;
        memset(&pAttrs, 0, sizeof(pAttrs));
        int pDepth = DefaultDepth(display, screen);
        Visual* pVisual = DefaultVisual(display, screen);
        Colormap pColormap = DefaultColormap(display, screen);

        if (parentWindow != 0) {
            if (XGetWindowAttributes(display, parentWindow, &pAttrs)) {
                thirdAssLog("[CairoUI] Parent attrs: depth=%d, visual=%p, colormap=%lu, width=%d, height=%d\n",
                            pAttrs.depth, pAttrs.visual, (unsigned long)pAttrs.colormap, pAttrs.width, pAttrs.height);
                pDepth = pAttrs.depth;
                pVisual = pAttrs.visual;
                pColormap = pAttrs.colormap;
            } else {
                thirdAssLog("[CairoUI] XGetWindowAttributes on parentWin failed!\n");
            }
        }

        XSetWindowAttributes attr;
        attr.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask |
                          ButtonReleaseMask | PointerMotionMask | KeyPressMask;
        attr.background_pixel = BlackPixel(display, screen);
        attr.colormap = pColormap;
        attr.border_pixel = 0;

        Window root = RootWindow(display, screen);
        Window actualParent = (parentWindow != 0) ? parentWindow : root;

        unsigned long valmask = CWEventMask | CWBackPixel;
        if (pColormap != 0) {
            valmask |= CWColormap | CWBorderPixel;
        }

        window = XCreateWindow(display, actualParent, 0, 0, UI_WIDTH, UI_HEIGHT,
                               0, pDepth, InputOutput, pVisual,
                               valmask, &attr);
        if (!window) {
            thirdAssLog("[CairoUI] XCreateWindow failed!\n");
            XCloseDisplay(display);
            display = nullptr;
            return false;
        }
        thirdAssLog("[CairoUI] Window created: 0x%lx, actualParent=0x%lx\n", (unsigned long)window, (unsigned long)actualParent);

        // Set XEmbed property if embedded into a host container
        if (parentWindow != 0) {
            Atom atom_xembed = XInternAtom(display, "_XEMBED_INFO", False);
            unsigned long xembed_data[2] = { 0, 1 }; // Version 0, XEMBED_MAPPED (1)
            XChangeProperty(display, window, atom_xembed, atom_xembed, 32, PropModeReplace,
                            (unsigned char*)xembed_data, 2);
        }

        depth = pDepth;
        visual = pVisual;

        // Software-rendered ARGB32 image surface with subpixel antialiasing & zero XRender glyph issues
        cairoSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, UI_WIDTH, UI_HEIGHT);
        cairoContext = cairo_create(cairoSurface);
        gc = XCreateGC(display, window, 0, nullptr);

        XMapWindow(display, window);
        XFlush(display);

        windowMapped = true;
        redraw();

        // Start dedicated GUI event thread for host-independent responsive rendering
        uiRunning = true;
        eventThread = std::thread([this]() {
            int idleCounter = 0;
            while (uiRunning && display) {
                while (display && XPending(display) > 0) {
                    XEvent ev;
                    XNextEvent(display, &ev);
                    handleEvent(&ev);
                }
                if (windowMapped) {
                    // 60 FPS during active knob drag/interaction, throttled 30 FPS when idle
                    if (++idleCounter >= 2 || activeKnobId >= 0) {
                        idleCounter = 0;
                        idle();
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        });

        thirdAssLog("[CairoUI] openWindow complete and event thread running\n");
        return true;
    }

    void closeWindow() {
        uiRunning = false;
        if (eventThread.joinable()) {
            eventThread.join();
        }

        std::lock_guard<std::mutex> lock(renderMutex);
        if (cairoContext) {
            cairo_destroy(cairoContext);
            cairoContext = nullptr;
        }
        if (cairoSurface) {
            cairo_surface_destroy(cairoSurface);
            cairoSurface = nullptr;
        }
        if (display && gc) {
            XFreeGC(display, gc);
            gc = nullptr;
        }
        if (display && window) {
            XUnmapWindow(display, window);
            XDestroyWindow(display, window);
            XFlush(display);
            window = 0;
        }
        if (display) {
            XCloseDisplay(display);
            display = nullptr;
        }
        windowMapped = false;
    }

    void show() {
        windowMapped = true;
        if (display && window) {
            XMapWindow(display, window);
            XFlush(display);
            redraw();
        }
    }

    void hide() {
        windowMapped = false;
        if (display && window) {
            XUnmapWindow(display, window);
            XFlush(display);
        }
    }

    void handleEvent(XEvent* ev) {
        if (!ev) return;

        switch (ev->type) {
            case Expose:
                if (ev->xexpose.count == 0) {
                    redraw();
                }
                break;

            case MapNotify:
                windowMapped = true;
                redraw();
                break;

            case UnmapNotify:
                windowMapped = false;
                break;

            case ConfigureNotify:
                redraw();
                break;

            case ButtonPress:
                onMouseDown(ev->xbutton.x, ev->xbutton.y, ev->xbutton.button, ev->xbutton.state, ev->xbutton.time);
                break;

            case ButtonRelease:
                onMouseUp(ev->xbutton.x, ev->xbutton.y, ev->xbutton.button);
                break;

            case MotionNotify:
                onMouseMove(ev->xmotion.x, ev->xmotion.y, ev->xmotion.state);
                break;

            default:
                break;
        }
    }

    void idle() {
        // Smoothly refresh animated meters and dynamic indicators
        redraw();
    }

    Window getWindow() const { return window; }

    void renderToContext(cairo_t* cr) {
        cairoContext = cr;
        render();
        cairoContext = nullptr; // do not hold reference
    }

    void redraw() {
        std::lock_guard<std::mutex> lock(renderMutex);
        if (!display || !window || !cairoContext) return;
        render();
        blitToWindow();
    }

private:
    void initLayout() {
        knobs.clear();
        buttons.clear();

        // High contrast, harmonious module color identity
        Color colGate(0.96, 0.62, 0.15);     // Warm Amber / Sunset Orange
        Color colDrive(0.93, 0.25, 0.25);    // Aggressive Crimson Red
        Color colEQ(0.12, 0.88, 0.84);       // Neon Cyan / Teal
        Color colSub(0.82, 0.38, 0.96);      // Psychoacoustic Purple / Magenta
        Color colComp(0.96, 0.78, 0.18);     // Warm Optical Gold
        Color colChorus(0.24, 0.72, 0.98);   // Electric Ocean Blue
        Color colDelay(0.22, 0.88, 0.52);    // Analog Emerald Green
        Color colCab(0.90, 0.72, 0.40);      // Champagne Bronze
        Color colMaster(0.92, 0.94, 0.98);   // Silver Titanium

        // -------------------------------------------------------------
        // ROW 1: PREAMP & TONE (y: 54 to 290)
        // -------------------------------------------------------------

        // Module 1: INPUT & GATE (x: 14, w: 180)
        knobs.push_back({PARAM_IN_GAIN, 58, 162, 26, "IN GAIN", colMaster});
        knobs.push_back({PARAM_GATE_THRESH, 140, 126, 20, "THRESH", colGate});
        knobs.push_back({PARAM_GATE_RELEASE, 140, 216, 20, "RELEASE", colGate});

        // Module 2: PREAMP DRIVE & TONE (x: 204, w: 290)
        buttons.push_back({PARAM_DRIVE_ENABLE, 448, 62, 38, 18, "ON", 1.0f, true, colDrive});
        // Mode selector pill buttons
        buttons.push_back({PARAM_DRIVE_MODE, 214, 88, 82, 22, "TUBE", 0.0f, false, colDrive});
        buttons.push_back({PARAM_DRIVE_MODE, 302, 88, 96, 22, "OVERDRIVE", 1.0f, false, colDrive});
        buttons.push_back({PARAM_DRIVE_MODE, 404, 88, 82, 22, "FUZZ", 2.0f, false, colDrive});
        // Large central drive knob with contour & blend
        knobs.push_back({PARAM_DRIVE_AMOUNT, 274, 192, 32, "DRIVE", colDrive});
        knobs.push_back({PARAM_DRIVE_TONE, 372, 156, 21, "CONTOUR", colDrive});
        knobs.push_back({PARAM_DRIVE_MIX, 438, 212, 21, "BLEND", colDrive});

        // Module 3: 4-BAND BASS TONE STACK (x: 504, w: 462)
        buttons.push_back({PARAM_EQ_ENABLE, 918, 62, 40, 18, "ON", 1.0f, true, colEQ});
        knobs.push_back({PARAM_EQ_BASS, 558, 126, 22, "BASS 80Hz", colEQ});
        knobs.push_back({PARAM_EQ_LOW_MID, 672, 126, 22, "LOW MID 350Hz", colEQ});
        knobs.push_back({PARAM_EQ_HIGH_MID, 786, 126, 22, "HI MID 1.8kHz", colEQ});
        knobs.push_back({PARAM_EQ_TREBLE, 900, 126, 22, "TREBLE 4.5kHz", colEQ});

        // -------------------------------------------------------------
        // ROW 2: PROCESSING & EFFECTS (y: 298 to 548)
        // -------------------------------------------------------------

        // Module 4: RESURRECTION SUB ENHANCER (x: 14, w: 180)
        buttons.push_back({PARAM_SUB_ENABLE, 146, 306, 40, 18, "ON", 1.0f, true, colSub});
        knobs.push_back({PARAM_SUB_FREQ, 58, 372, 21, "TRACK Hz", colSub});
        knobs.push_back({PARAM_SUB_HARMONICS, 142, 372, 21, "HARMONICS", colSub});
        knobs.push_back({PARAM_SUB_LEVEL, 100, 472, 23, "SUB BOOST", colSub});

        // Module 5: OPTO COMPRESSOR (x: 204, w: 242)
        buttons.push_back({PARAM_COMP_ENABLE, 398, 306, 40, 18, "ON", 1.0f, true, colComp});
        knobs.push_back({PARAM_COMP_THRESH, 248, 366, 20, "THRESH", colComp});
        knobs.push_back({PARAM_COMP_RATIO, 325, 366, 20, "RATIO", colComp});
        knobs.push_back({PARAM_COMP_ATTACK, 402, 366, 20, "ATTACK", colComp});
        knobs.push_back({PARAM_COMP_RELEASE, 278, 478, 20, "RELEASE", colComp});
        knobs.push_back({PARAM_COMP_MAKEUP, 370, 478, 20, "MAKEUP", colComp});

        // Module 6: CROSSOVER CHORUS (x: 456, w: 176)
        buttons.push_back({PARAM_CHORUS_ENABLE, 584, 306, 40, 18, "ON", 1.0f, true, colChorus});
        knobs.push_back({PARAM_CHORUS_RATE, 500, 368, 20, "RATE", colChorus});
        knobs.push_back({PARAM_CHORUS_DEPTH, 588, 368, 20, "DEPTH", colChorus});
        knobs.push_back({PARAM_CHORUS_CROSSOVER, 500, 468, 20, "HP CROSS", colChorus});
        knobs.push_back({PARAM_CHORUS_MIX, 588, 468, 20, "MIX", colChorus});

        // Module 7: ANALOG DELAY (x: 642, w: 176)
        buttons.push_back({PARAM_DELAY_ENABLE, 770, 306, 40, 18, "ON", 1.0f, true, colDelay});
        knobs.push_back({PARAM_DELAY_TIME, 686, 368, 20, "TIME", colDelay});
        knobs.push_back({PARAM_DELAY_FEEDBACK, 774, 368, 20, "FEEDBACK", colDelay});
        knobs.push_back({PARAM_DELAY_DAMP, 686, 468, 20, "DAMPING", colDelay});
        knobs.push_back({PARAM_DELAY_MIX, 774, 468, 20, "MIX", colDelay});

        // Module 8: CABINET & MASTER (x: 828, w: 138)
        buttons.push_back({PARAM_CAB_MODE, 836, 328, 122, 16, "DIRECT / BYPASS", 0.0f, false, colCab});
        buttons.push_back({PARAM_CAB_MODE, 836, 347, 122, 16, "8x10 SVT FRIDGE", 1.0f, false, colCab});
        buttons.push_back({PARAM_CAB_MODE, 836, 366, 122, 16, "1x15 PORTAFLEX", 2.0f, false, colCab});
        buttons.push_back({PARAM_CAB_MODE, 836, 385, 122, 16, "4x10 MODERN PUNCH", 3.0f, false, colCab});
        buttons.push_back({PARAM_CAB_MODE, 836, 404, 122, 16, "2x12 ROCK BITE", 4.0f, false, colCab});

        buttons.push_back({PARAM_LIMITER_ENABLE, 836, 425, 122, 17, "LIMITER ON", 1.0f, true, colMaster});
        knobs.push_back({PARAM_OUT_GAIN, 864, 478, 19, "OUT", colMaster});
        knobs.push_back({PARAM_DRY_WET, 930, 478, 19, "MIX", colMaster});
    }

    void render() {
        cairo_t* cr = cairoContext;
        if (!cr) return;

        // 1. Chassis Outer Background (Brushed dark titanium finish)
        cairo_pattern_t* bgPat = cairo_pattern_create_linear(0, 0, 0, UI_HEIGHT);
        cairo_pattern_add_color_stop_rgba(bgPat, 0.0, 0.12, 0.13, 0.15, 1.0);
        cairo_pattern_add_color_stop_rgba(bgPat, 0.5, 0.09, 0.10, 0.12, 1.0);
        cairo_pattern_add_color_stop_rgba(bgPat, 1.0, 0.07, 0.08, 0.09, 1.0);
        cairo_set_source(cr, bgPat);
        cairo_rectangle(cr, 0, 0, UI_WIDTH, UI_HEIGHT);
        cairo_fill(cr);
        cairo_pattern_destroy(bgPat);

        // Rack Chassis Border & Corner Screws
        cairo_set_source_rgb(cr, 0.22, 0.24, 0.28);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, 0.5, 0.5, UI_WIDTH - 1.0, UI_HEIGHT - 1.0);
        cairo_stroke(cr);

        drawRackScrew(cr, 10, 10);
        drawRackScrew(cr, UI_WIDTH - 10, 10);
        drawRackScrew(cr, 10, UI_HEIGHT - 10);
        drawRackScrew(cr, UI_WIDTH - 10, UI_HEIGHT - 10);

        // 2. Top Header Bar
        cairo_set_source_rgb(cr, 0.06, 0.06, 0.08);
        cairo_rectangle(cr, 0, 0, UI_WIDTH, 48);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.18, 0.20, 0.24);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, 0, 48);
        cairo_line_to(cr, UI_WIDTH, 48);
        cairo_stroke(cr);

        // Branding Title
        cairo_select_font_face(cr, "Sans", 0, 1);
        cairo_set_font_size(cr, 22.0);
        cairo_set_source_rgb(cr, 0.14, 0.88, 0.84); // Radiant Cyan
        cairo_move_to(cr, 22, 32);
        cairo_show_text(cr, "THIRD ASS");

        cairo_set_font_size(cr, 10.0);
        cairo_set_source_rgb(cr, 0.55, 0.59, 0.66);
        cairo_move_to(cr, 172, 31);
        cairo_show_text(cr, "BASS MULTI-FX & AMP PROCESSOR");

        // Preset Selector Bar
        drawPresetBar(cr);

        // Top-right Master Level & Linux Badge
        drawTopRightStatus(cr);

        // 3. Draw Module Panels
        drawModuleBox(cr, 14, 54, 180, 236, "INPUT & GATE", Color(0.96, 0.62, 0.15));
        drawModuleBox(cr, 204, 54, 290, 236, "PREAMP DRIVE & TONE", Color(0.93, 0.25, 0.25));
        drawModuleBox(cr, 504, 54, 462, 236, "4-BAND BASS TONE STACK", Color(0.12, 0.88, 0.84));

        drawModuleBox(cr, 14, 298, 180, 250, "SUB RESURRECTION", Color(0.82, 0.38, 0.96));
        drawModuleBox(cr, 204, 298, 242, 250, "OPTO COMPRESSOR", Color(0.96, 0.78, 0.18));
        drawModuleBox(cr, 456, 298, 176, 250, "BASS CHORUS", Color(0.24, 0.72, 0.98));
        drawModuleBox(cr, 642, 298, 176, 250, "STEREO DELAY", Color(0.22, 0.88, 0.52));
        drawModuleBox(cr, 828, 298, 138, 250, "CABINET & OUT", Color(0.90, 0.72, 0.40));

        // Module-specific visual meters & graphics
        drawGateLed(cr, 142, 68);
        drawTubeVisual(cr, 372, 234);
        drawEQVisualizer(cr, 518, 176, 434, 98);
        drawSubHarmonicVisual(cr, 100, 400);
        drawCompGRMeter(cr, 218, 414, 214, 18);
        drawChorusVisual(cr, 544, 418);
        drawDelayVisual(cr, 730, 418);
        drawMasterOutputMeter(cr, 838, 524, 118, 14);

        // 4. Draw Buttons & Switches
        for (const auto& btn : buttons) {
            drawButton(cr, btn);
        }

        // 5. Draw Rotary Knobs
        for (const auto& knob : knobs) {
            drawKnob(cr, knob);
        }
    }

    void drawRackScrew(cairo_t* cr, float x, float y) {
        cairo_new_path(cr);
        cairo_set_source_rgb(cr, 0.15, 0.16, 0.18);
        cairo_arc(cr, x, y, 4.0, 0, 6.2831853);
        cairo_fill(cr);

        cairo_new_path(cr);
        cairo_set_source_rgb(cr, 0.30, 0.32, 0.36);
        cairo_set_line_width(cr, 0.8);
        cairo_arc(cr, x, y, 4.0, 0, 6.2831853);
        cairo_stroke(cr);

        // Slot
        cairo_new_path(cr);
        cairo_set_source_rgb(cr, 0.08, 0.09, 0.10);
        cairo_set_line_width(cr, 1.2);
        cairo_move_to(cr, x - 2.5, y - 1.0);
        cairo_line_to(cr, x + 2.5, y + 1.0);
        cairo_stroke(cr);
    }

    void drawTopRightStatus(cairo_t* cr) {
        // Master Output VU peak bar in header
        float outL = engine ? engine->getMeterOutPeakL() : 0.0f;
        float outR = engine ? engine->getMeterOutPeakR() : 0.0f;

        float bx = 760, by = 15, bw = 100, bh = 18;
        cairo_set_source_rgb(cr, 0.08, 0.09, 0.11);
        roundedRect(cr, bx, by, bw, bh, 3);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 0.20, 0.22, 0.26);
        cairo_set_line_width(cr, 1.0);
        roundedRect(cr, bx, by, bw, bh, 3);
        cairo_stroke(cr);

        // L bar
        drawMeterSegmentBar(cr, bx + 16, by + 3, bw - 20, 5, outL);
        // R bar
        drawMeterSegmentBar(cr, bx + 16, by + 10, bw - 20, 5, outR);

        cairo_select_font_face(cr, "Sans", 0, 1);
        cairo_set_font_size(cr, 7.5);
        cairo_set_source_rgb(cr, 0.50, 0.55, 0.62);
        cairo_move_to(cr, bx + 4, by + 8);
        cairo_show_text(cr, "L");
        cairo_move_to(cr, bx + 4, by + 15);
        cairo_show_text(cr, "R");

        // 64-BIT DSP Badge
        float tagX = 878, tagY = 15, tagW = 82, tagH = 18;
        cairo_set_source_rgba(cr, 0.14, 0.88, 0.84, 0.10);
        roundedRect(cr, tagX, tagY, tagW, tagH, 3);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 0.14, 0.88, 0.84, 0.40);
        cairo_set_line_width(cr, 1.0);
        roundedRect(cr, tagX, tagY, tagW, tagH, 3);
        cairo_stroke(cr);

        cairo_set_font_size(cr, 8.5);
        cairo_set_source_rgb(cr, 0.14, 0.88, 0.84);
        cairo_move_to(cr, tagX + 11, tagY + 12.5);
        cairo_show_text(cr, "LINUX 64-BIT");
    }

    void drawMeterSegmentBar(cairo_t* cr, float x, float y, float w, float h, float peakLin) {
        float peakDb = (peakLin > 1e-5f) ? (20.0f * std::log10(peakLin)) : -60.0f;
        float norm = std::clamp((peakDb + 48.0f) / 48.0f, 0.0f, 1.0f);

        int numSegs = 14;
        float segW = (w - (numSegs - 1) * 1.5f) / numSegs;
        int activeCount = (int)std::round(norm * numSegs);

        for (int i = 0; i < numSegs; ++i) {
            float sx = x + i * (segW + 1.5f);
            bool lit = (i < activeCount);
            if (i < 9) { // Green (-48 to -12 dB)
                cairo_set_source_rgba(cr, 0.20, 0.85, 0.40, lit ? 0.95 : 0.15);
            } else if (i < 12) { // Yellow (-12 to -3 dB)
                cairo_set_source_rgba(cr, 0.96, 0.80, 0.20, lit ? 0.95 : 0.15);
            } else { // Red (-3 to 0 dB+)
                cairo_set_source_rgba(cr, 0.95, 0.25, 0.25, lit ? 1.0 : 0.20);
            }
            cairo_rectangle(cr, sx, y, segW, h);
            cairo_fill(cr);
        }
    }

    void drawPresetBar(cairo_t* cr) {
        float bx = 410, by = 10, bw = 330, bh = 28;

        // Container box
        cairo_set_source_rgb(cr, 0.09, 0.10, 0.12);
        roundedRect(cr, bx, by, bw, bh, 4);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.22, 0.25, 0.30);
        cairo_set_line_width(cr, 1.0);
        roundedRect(cr, bx, by, bw, bh, 4);
        cairo_stroke(cr);

        // Prev Arrow Button (<)
        cairo_set_source_rgb(cr, 0.14, 0.16, 0.20);
        roundedRect(cr, bx + 2, by + 2, 28, bh - 4, 3);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 0.70, 0.75, 0.82);
        cairo_set_font_size(cr, 13.0);
        cairo_move_to(cr, bx + 11, by + 19);
        cairo_show_text(cr, "<");

        // Next Arrow Button (>)
        cairo_set_source_rgb(cr, 0.14, 0.16, 0.20);
        roundedRect(cr, bx + bw - 30, by + 2, 28, bh - 4, 3);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 0.70, 0.75, 0.82);
        cairo_set_font_size(cr, 13.0);
        cairo_move_to(cr, bx + bw - 21, by + 19);
        cairo_show_text(cr, ">");

        // Preset Name Text
        const auto& presets = getPresets();
        std::string pName = "01: Clean Studio DI";
        if (currentPresetIdx >= 0 && currentPresetIdx < (int)presets.size()) {
            pName = presets[currentPresetIdx].name;
        }

        cairo_set_source_rgb(cr, 0.95, 0.96, 0.98);
        cairo_set_font_size(cr, 12.0);
        cairo_select_font_face(cr, "Sans", 0, 1);

        cairo_text_extents_t ext;
        cairo_text_extents(cr, pName.c_str(), &ext);
        float tx = bx + (bw - ext.width) * 0.5f;
        cairo_move_to(cr, tx, by + 18.5f);
        cairo_show_text(cr, pName.c_str());
    }

    void drawModuleBox(cairo_t* cr, float x, float y, float w, float h, const char* title, Color accent) {
        // Module card background with soft recessed gradient
        cairo_pattern_t* cardPat = cairo_pattern_create_linear(x, y, x, y + h);
        cairo_pattern_add_color_stop_rgba(cardPat, 0.0, 0.14, 0.15, 0.17, 1.0);
        cairo_pattern_add_color_stop_rgba(cardPat, 1.0, 0.10, 0.11, 0.12, 1.0);
        cairo_set_source(cr, cardPat);
        roundedRect(cr, x, y, w, h, 6);
        cairo_fill(cr);
        cairo_pattern_destroy(cardPat);

        // Card outer border
        cairo_set_source_rgb(cr, 0.22, 0.24, 0.28);
        cairo_set_line_width(cr, 1.0);
        roundedRect(cr, x, y, w, h, 6);
        cairo_stroke(cr);

        // Header accent vertical marker
        cairo_set_source_rgb(cr, accent.r, accent.g, accent.b);
        roundedRect(cr, x + 8, y + 8, 4, 13, 2);
        cairo_fill(cr);

        // Header Title
        cairo_select_font_face(cr, "Sans", 0, 1);
        cairo_set_font_size(cr, 10.5);
        cairo_set_source_rgb(cr, 0.88, 0.90, 0.95);
        cairo_move_to(cr, x + 18, y + 19);
        cairo_show_text(cr, title);

        // Thin separator rule below header
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.05);
        cairo_move_to(cr, x + 8, y + 26);
        cairo_line_to(cr, x + w - 8, y + 26);
        cairo_stroke(cr);
    }

    void drawGateLed(cairo_t* cr, float x, float y) {
        float gateGain = engine ? engine->getMeterGateGain() : 1.0f;
        bool open = (gateGain > 0.4f);

        cairo_new_path(cr);
        cairo_arc(cr, x, y, 4.0, 0, 6.2831853);
        if (open) {
            cairo_set_source_rgb(cr, 0.25, 0.92, 0.45); // Active Bright Green
        } else {
            cairo_set_source_rgb(cr, 0.85, 0.45, 0.15); // Gating Amber/Red
        }
        cairo_fill(cr);

        cairo_select_font_face(cr, "Sans", 0, 1);
        cairo_set_font_size(cr, 8.5);
        cairo_set_source_rgb(cr, 0.60, 0.65, 0.72);
        cairo_move_to(cr, x + 8, y + 3);
        cairo_show_text(cr, open ? "OPEN" : "GATE");
    }

    void drawTubeVisual(cairo_t* cr, float cx, float cy) {
        float driveVal = engine ? engine->getParameter(PARAM_DRIVE_AMOUNT) * 0.01f : 0.25f;
        bool driveOn = engine ? (engine->getParameter(PARAM_DRIVE_ENABLE) > 0.5f) : true;

        // Vacuum tube bulb outline
        cairo_new_path(cr);
        cairo_set_source_rgba(cr, 0.25, 0.28, 0.32, 0.6);
        cairo_set_line_width(cr, 1.0);
        roundedRect(cr, cx - 14, cy - 16, 28, 30, 8);
        cairo_stroke(cr);

        // Glowing filaments inside bulb
        float intensity = driveOn ? (0.25f + driveVal * 0.75f) : 0.1f;
        cairo_new_path(cr);
        cairo_set_source_rgba(cr, 0.98, 0.40 * intensity, 0.15 * intensity, intensity);
        cairo_set_line_width(cr, 2.0);
        cairo_move_to(cr, cx - 5, cy + 8);
        cairo_line_to(cr, cx - 5, cy - 6);
        cairo_line_to(cr, cx - 1, cy - 9);
        cairo_line_to(cr, cx - 1, cy + 8);

        cairo_move_to(cr, cx + 1, cy + 8);
        cairo_line_to(cr, cx + 1, cy - 9);
        cairo_line_to(cr, cx + 5, cy - 6);
        cairo_line_to(cr, cx + 5, cy + 8);
        cairo_stroke(cr);

        // Filament label
        cairo_set_font_size(cr, 7.5);
        cairo_set_source_rgb(cr, 0.50, 0.54, 0.60);
        cairo_move_to(cr, cx - 11, cy + 22);
        cairo_show_text(cr, "12AX7");
    }

    void drawEQVisualizer(cairo_t* cr, float x, float y, float w, float h) {
        // Dark recessed display frame
        cairo_set_source_rgb(cr, 0.06, 0.07, 0.09);
        roundedRect(cr, x, y, w, h, 4);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.18, 0.21, 0.26);
        cairo_set_line_width(cr, 1.0);
        roundedRect(cr, x, y, w, h, 4);
        cairo_stroke(cr);

        // Center 0 dB baseline
        float zeroY = y + h * 0.5f;
        cairo_new_path(cr);
        double dashPattern[] = {3.0, 3.0};
        cairo_set_dash(cr, dashPattern, 2, 0.0);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.14);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, x, zeroY);
        cairo_line_to(cr, x + w, zeroY);
        cairo_stroke(cr);

        // +/- 6 dB reference lines
        float plus6Y = zeroY - 18.0f;
        float minus6Y = zeroY + 18.0f;
        cairo_move_to(cr, x, plus6Y);
        cairo_line_to(cr, x + w, plus6Y);
        cairo_move_to(cr, x, minus6Y);
        cairo_line_to(cr, x + w, minus6Y);
        cairo_stroke(cr);

        // Reset dash
        cairo_set_dash(cr, nullptr, 0, 0.0);

        // Frequency grid markers (100 Hz, 1 kHz, 10 kHz)
        auto freqToX = [&](float freq) {
            float minF = 20.0f, maxF = 20000.0f;
            float norm = std::log10(freq / minF) / std::log10(maxF / minF);
            return x + norm * w;
        };

        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.06);
        for (float f : {100.0f, 1000.0f, 10000.0f}) {
            float fx = freqToX(f);
            cairo_move_to(cr, fx, y);
            cairo_line_to(cr, fx, y + h);
            cairo_stroke(cr);
        }

        cairo_select_font_face(cr, "Sans", 0, 0);
        cairo_set_font_size(cr, 7.5);
        cairo_set_source_rgb(cr, 0.40, 0.44, 0.50);
        cairo_move_to(cr, freqToX(100.0f) + 2, y + 10);
        cairo_show_text(cr, "100Hz");
        cairo_move_to(cr, freqToX(1000.0f) + 2, y + 10);
        cairo_show_text(cr, "1kHz");
        cairo_move_to(cr, freqToX(10000.0f) + 2, y + 10);
        cairo_show_text(cr, "10kHz");

        // Compute 4-Band EQ Response Curve
        bool eqOn = engine ? (engine->getParameter(PARAM_EQ_ENABLE) > 0.5f) : true;
        float gBass = engine ? engine->getParameter(PARAM_EQ_BASS) : 0.0f;
        float gLowMid = engine ? engine->getParameter(PARAM_EQ_LOW_MID) : 0.0f;
        float gHiMid = engine ? engine->getParameter(PARAM_EQ_HIGH_MID) : 0.0f;
        float gTreb = engine ? engine->getParameter(PARAM_EQ_TREBLE) : 0.0f;

        const int numPts = 80;
        std::vector<std::pair<float, float>> curvePts;
        curvePts.reserve(numPts);

        float minF = 20.0f, maxF = 20000.0f;
        for (int i = 0; i < numPts; ++i) {
            float t = (float)i / (float)(numPts - 1);
            float freq = minF * std::pow(maxF / minF, t);
            float px = x + t * w;

            float gainDb = 0.0f;
            if (eqOn) {
                // Low shelf approx (80 Hz)
                gainDb += gBass / (1.0f + std::pow(freq / 80.0f, 2.0f));
                // Low mid peak (350 Hz)
                float d1 = (freq - 350.0f) / 180.0f;
                gainDb += gLowMid / (1.0f + d1 * d1);
                // Hi mid peak (1.8 kHz)
                float d2 = (freq - 1800.0f) / 900.0f;
                gainDb += gHiMid / (1.0f + 1.2f * d2 * d2);
                // Treble shelf approx (4.5 kHz)
                float fRatio = freq / 4500.0f;
                gainDb += gTreb * (fRatio * fRatio) / (1.0f + fRatio * fRatio);
            }

            float py = zeroY - gainDb * 3.0f;
            py = std::clamp(py, y + 2.0f, y + h - 2.0f);
            curvePts.push_back({px, py});
        }

        // Draw translucent fill beneath the curve down to zero baseline
        cairo_new_path(cr);
        cairo_move_to(cr, curvePts[0].first, zeroY);
        for (const auto& pt : curvePts) {
            cairo_line_to(cr, pt.first, pt.second);
        }
        cairo_line_to(cr, curvePts.back().first, zeroY);
        cairo_close_path(cr);

        cairo_pattern_t* fillPat = cairo_pattern_create_linear(x, y, x, y + h);
        cairo_pattern_add_color_stop_rgba(fillPat, 0.0, 0.12, 0.88, 0.84, 0.22);
        cairo_pattern_add_color_stop_rgba(fillPat, 1.0, 0.12, 0.88, 0.84, 0.02);
        cairo_set_source(cr, fillPat);
        cairo_fill(cr);
        cairo_pattern_destroy(fillPat);

        // Draw EQ Curve Line
        cairo_new_path(cr);
        cairo_set_source_rgba(cr, 0.12, 0.88, 0.84, eqOn ? 0.95 : 0.40);
        cairo_set_line_width(cr, 2.2);
        for (size_t i = 0; i < curvePts.size(); ++i) {
            if (i == 0) cairo_move_to(cr, curvePts[i].first, curvePts[i].second);
            else cairo_line_to(cr, curvePts[i].first, curvePts[i].second);
        }
        cairo_stroke(cr);

        // Control Node Dots at center frequencies (80Hz, 350Hz, 1.8kHz, 4.5kHz)
        if (eqOn) {
            float nodeFreqs[] = {80.0f, 350.0f, 1800.0f, 4500.0f};
            float nodeGains[] = {gBass, gLowMid, gHiMid, gTreb};
            for (int k = 0; k < 4; ++k) {
                float nx = freqToX(nodeFreqs[k]);
                float ny = zeroY - nodeGains[k] * 3.0f;
                ny = std::clamp(ny, y + 4.0f, y + h - 4.0f);

                cairo_new_path(cr);
                cairo_arc(cr, nx, ny, 3.5, 0, 6.2831853);
                cairo_set_source_rgb(cr, 0.95, 0.98, 1.0);
                cairo_fill(cr);

                cairo_new_path(cr);
                cairo_arc(cr, nx, ny, 3.5, 0, 6.2831853);
                cairo_set_source_rgb(cr, 0.12, 0.88, 0.84);
                cairo_set_line_width(cr, 1.2);
                cairo_stroke(cr);
            }
        }
    }

    void drawSubHarmonicVisual(cairo_t* cr, float cx, float cy) {
        float subOn = engine ? (engine->getParameter(PARAM_SUB_ENABLE) > 0.5f) : true;
        float harmAmt = engine ? engine->getParameter(PARAM_SUB_HARMONICS) * 0.01f : 0.4f;

        // Visual spectrum bars (Sub, 2nd, 3rd)
        float barW = 8.0f;
        float heights[] = {18.0f, 14.0f * (0.3f + harmAmt * 0.7f), 10.0f * (0.2f + harmAmt * 0.8f)};
        float offsets[] = {-16.0f, 0.0f, 16.0f};
        const char* labels[] = {"SUB", "2ND", "3RD"};

        for (int i = 0; i < 3; ++i) {
            float bx = cx + offsets[i] - barW * 0.5f;
            float bh = heights[i];
            float by = cy - bh * 0.5f;

            cairo_set_source_rgba(cr, 0.82, 0.38, 0.96, subOn ? 0.85 : 0.20);
            roundedRect(cr, bx, by, barW, bh, 2);
            cairo_fill(cr);

            cairo_set_font_size(cr, 7.0);
            cairo_set_source_rgb(cr, 0.50, 0.54, 0.60);
            cairo_move_to(cr, bx - 1.5f, cy + 13.0f);
            cairo_show_text(cr, labels[i]);
        }
    }

    void drawCompGRMeter(cairo_t* cr, float x, float y, float w, float h) {
        float grDb = engine ? engine->getMeterCompGR() : 0.0f;
        bool compOn = engine ? (engine->getParameter(PARAM_COMP_ENABLE) > 0.5f) : true;

        // Meter background box
        cairo_set_source_rgb(cr, 0.07, 0.08, 0.10);
        roundedRect(cr, x, y, w, h, 3);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.20, 0.22, 0.26);
        cairo_set_line_width(cr, 1.0);
        roundedRect(cr, x, y, w, h, 3);
        cairo_stroke(cr);

        // GR Label
        cairo_select_font_face(cr, "Sans", 0, 1);
        cairo_set_font_size(cr, 8.5);
        cairo_set_source_rgb(cr, 0.65, 0.70, 0.78);
        cairo_move_to(cr, x + 6, y + 12.5f);
        cairo_show_text(cr, "GR dB");

        // 10 Segments (from 0 dB down to -20 dB)
        float startX = x + 44.0f;
        float barW = w - 50.0f;
        int numSegs = 10;
        float segW = (barW - (numSegs - 1) * 2.0f) / numSegs;

        // Active reduction (from 0 to 20 dB)
        float norm = compOn ? std::clamp(grDb / 18.0f, 0.0f, 1.0f) : 0.0f;
        int activeCount = (int)std::round(norm * numSegs);

        for (int i = 0; i < numSegs; ++i) {
            float sx = startX + i * (segW + 2.0f);
            bool lit = (i < activeCount);
            if (i < 5) {
                cairo_set_source_rgba(cr, 0.96, 0.78, 0.18, lit ? 0.95 : 0.15); // Goldenrod
            } else if (i < 8) {
                cairo_set_source_rgba(cr, 0.96, 0.55, 0.15, lit ? 0.95 : 0.15); // Amber
            } else {
                cairo_set_source_rgba(cr, 0.95, 0.25, 0.25, lit ? 1.00 : 0.20); // Red
            }
            cairo_rectangle(cr, sx, y + 3, segW, h - 6);
            cairo_fill(cr);
        }
    }

    void drawChorusVisual(cairo_t* cr, float cx, float cy) {
        float mix = engine ? engine->getParameter(PARAM_CHORUS_MIX) * 0.01f : 0.4f;
        bool chorusOn = engine ? (engine->getParameter(PARAM_CHORUS_ENABLE) > 0.5f) : true;

        // Subtle dual stereo wave
        cairo_new_path(cr);
        cairo_set_source_rgba(cr, 0.24, 0.72, 0.98, chorusOn ? (0.2f + mix * 0.7f) : 0.15f);
        cairo_set_line_width(cr, 1.4);
        for (int i = -16; i <= 16; ++i) {
            float px = cx + i;
            float py = cy + std::sin((float)i * 0.35f) * 6.0f;
            if (i == -16) cairo_move_to(cr, px, py);
            else cairo_line_to(cr, px, py);
        }
        cairo_stroke(cr);
    }

    void drawDelayVisual(cairo_t* cr, float cx, float cy) {
        float mix = engine ? engine->getParameter(PARAM_DELAY_MIX) * 0.01f : 0.25f;
        bool delayOn = engine ? (engine->getParameter(PARAM_DELAY_ENABLE) > 0.5f) : true;

        // Echo decay dots
        float offsets[] = {-14.0f, -5.0f, 4.0f, 13.0f};
        float radii[] = {4.0f, 3.2f, 2.4f, 1.8f};
        for (int i = 0; i < 4; ++i) {
            cairo_new_path(cr);
            float a = delayOn ? (mix * (1.0f - i * 0.22f)) : 0.10f;
            cairo_set_source_rgba(cr, 0.22, 0.88, 0.52, a);
            cairo_arc(cr, cx + offsets[i], cy, radii[i], 0, 6.2831853);
            cairo_fill(cr);
        }
    }

    void drawMasterOutputMeter(cairo_t* cr, float x, float y, float w, float h) {
        float outL = engine ? engine->getMeterOutPeakL() : 0.0f;
        float outR = engine ? engine->getMeterOutPeakR() : 0.0f;

        cairo_set_source_rgb(cr, 0.06, 0.07, 0.09);
        roundedRect(cr, x, y, w, h, 2);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.20, 0.22, 0.26);
        cairo_set_line_width(cr, 1.0);
        roundedRect(cr, x, y, w, h, 2);
        cairo_stroke(cr);

        float barH = (h - 4.0f) * 0.5f;
        drawMeterSegmentBar(cr, x + 14, y + 2, w - 18, barH, outL);
        drawMeterSegmentBar(cr, x + 14, y + 2 + barH + 1, w - 18, barH, outR);

        cairo_select_font_face(cr, "Sans", 0, 1);
        cairo_set_font_size(cr, 7.0);
        cairo_set_source_rgb(cr, 0.55, 0.60, 0.68);
        cairo_move_to(cr, x + 3, y + 6.5f);
        cairo_show_text(cr, "L");
        cairo_move_to(cr, x + 3, y + 12.5f);
        cairo_show_text(cr, "R");
    }

    void drawKnob(cairo_t* cr, const KnobWidget& knob) {
        float val = engine->getParameter(knob.paramId);
        const auto* defs = getParameterDefs();
        float minV = defs[knob.paramId].minVal;
        float maxV = defs[knob.paramId].maxVal;
        float norm = (maxV > minV) ? std::clamp((val - minV) / (maxV - minV), 0.0f, 1.0f) : 0.0f;

        float r = knob.radius;
        float cx = knob.x;
        float cy = knob.y;

        // Outer Arc Track (270 degrees total sweep, from 135 deg to 405 deg)
        float startAngle = 2.35619449f; // 135 deg
        float endAngle = 7.06858347f;   // 405 deg
        float currentAngle = startAngle + norm * (endAngle - startAngle);

        // Dark track arc
        cairo_new_path(cr);
        cairo_set_source_rgba(cr, 0.06, 0.07, 0.09, 1.0);
        cairo_set_line_width(cr, 4.0);
        cairo_set_line_cap(cr, 1); // Round cap
        cairo_arc(cr, cx, cy, r + 4.0, startAngle, endAngle);
        cairo_stroke(cr);

        // Active glowing colored arc
        cairo_new_path(cr);
        cairo_set_source_rgba(cr, knob.accentColor.r, knob.accentColor.g, knob.accentColor.b, 0.95);
        cairo_set_line_width(cr, 4.0);
        cairo_set_line_cap(cr, 1);
        cairo_arc(cr, cx, cy, r + 4.0, startAngle, currentAngle);
        cairo_stroke(cr);

        // Metallic inner knob body
        cairo_new_path(cr);
        cairo_pattern_t* knobPat = cairo_pattern_create_linear(cx - r, cy - r, cx + r, cy + r);
        cairo_pattern_add_color_stop_rgba(knobPat, 0.0, 0.25, 0.27, 0.31, 1.0);
        cairo_pattern_add_color_stop_rgba(knobPat, 1.0, 0.12, 0.13, 0.15, 1.0);
        cairo_set_source(cr, knobPat);
        cairo_arc(cr, cx, cy, r, 0, 6.2831853);
        cairo_fill(cr);
        cairo_pattern_destroy(knobPat);

        // Knob rim bevel
        cairo_new_path(cr);
        cairo_set_source_rgb(cr, 0.34, 0.37, 0.43);
        cairo_set_line_width(cr, 1.2);
        cairo_arc(cr, cx, cy, r, 0, 6.2831853);
        cairo_stroke(cr);

        // Direction Indicator pointer line
        float indX = cx + (r - 2.5f) * std::cos(currentAngle);
        float indY = cy + (r - 2.5f) * std::sin(currentAngle);
        float indStartX = cx + (r * 0.35f) * std::cos(currentAngle);
        float indStartY = cy + (r * 0.35f) * std::sin(currentAngle);

        cairo_new_path(cr);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_set_line_width(cr, 2.2);
        cairo_set_line_cap(cr, 1);
        cairo_move_to(cr, indStartX, indStartY);
        cairo_line_to(cr, indX, indY);
        cairo_stroke(cr);

        // Parameter Title Label (above)
        cairo_select_font_face(cr, "Sans", 0, 1);
        cairo_set_font_size(cr, 9.0);
        cairo_set_source_rgb(cr, 0.72, 0.75, 0.80);
        cairo_text_extents_t ext;
        cairo_text_extents(cr, knob.title.c_str(), &ext);
        cairo_move_to(cr, cx - ext.width * 0.5f, cy - r - 8.0f);
        cairo_show_text(cr, knob.title.c_str());

        // Value String (below)
        char valBuf[32];
        if (defs[knob.paramId].isInt) {
            snprintf(valBuf, sizeof(valBuf), "%d", (int)std::round(val));
        } else if (defs[knob.paramId].label[0] != '\0') {
            snprintf(valBuf, sizeof(valBuf), "%.1f %s", val, defs[knob.paramId].label);
        } else {
            snprintf(valBuf, sizeof(valBuf), "%.1f", val);
        }

        cairo_set_font_size(cr, 8.5);
        cairo_set_source_rgb(cr, 0.55, 0.58, 0.64);
        cairo_text_extents(cr, valBuf, &ext);
        cairo_move_to(cr, cx - ext.width * 0.5f, cy + r + 15.0f);
        cairo_show_text(cr, valBuf);
    }

    void drawButton(cairo_t* cr, const ButtonWidget& btn) {
        bool isActive = false;
        if (btn.paramId >= 0) {
            float val = engine->getParameter(btn.paramId);
            if (btn.isToggle) {
                isActive = (val > 0.5f);
            } else {
                isActive = (std::abs(val - btn.targetValue) < 0.1f);
            }
        }

        // Pill button background
        if (isActive) {
            cairo_pattern_t* activePat = cairo_pattern_create_linear(btn.x, btn.y, btn.x, btn.y + btn.h);
            cairo_pattern_add_color_stop_rgba(activePat, 0.0, btn.activeColor.r, btn.activeColor.g, btn.activeColor.b, 0.95);
            cairo_pattern_add_color_stop_rgba(activePat, 1.0, btn.activeColor.r * 0.72, btn.activeColor.g * 0.72, btn.activeColor.b * 0.72, 0.95);
            cairo_set_source(cr, activePat);
            roundedRect(cr, btn.x, btn.y, btn.w, btn.h, 4);
            cairo_fill(cr);
            cairo_pattern_destroy(activePat);

            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        } else {
            cairo_set_source_rgb(cr, 0.09, 0.10, 0.12);
            roundedRect(cr, btn.x, btn.y, btn.w, btn.h, 4);
            cairo_fill(cr);

            cairo_set_source_rgb(cr, 0.24, 0.26, 0.30);
            cairo_set_line_width(cr, 1.0);
            roundedRect(cr, btn.x, btn.y, btn.w, btn.h, 4);
            cairo_stroke(cr);

            cairo_set_source_rgb(cr, 0.58, 0.62, 0.68);
        }

        // Button text centered
        cairo_select_font_face(cr, "Sans", 0, 1);
        cairo_set_font_size(cr, 9.0);
        cairo_text_extents_t ext;
        cairo_text_extents(cr, btn.text.c_str(), &ext);
        cairo_move_to(cr, btn.x + (btn.w - ext.width) * 0.5f, btn.y + (btn.h + ext.height) * 0.5f - 1.0f);
        cairo_show_text(cr, btn.text.c_str());
    }

    void onMouseDown(int mx, int my, int mouseButton, unsigned int state, unsigned long clickTime) {
        // Double click detection
        bool isDoubleClick = (clickTime - lastClickTime < 350);
        lastClickTime = clickTime;

        // Mouse Wheel Scroll adjustments
        if (mouseButton == 4 || mouseButton == 5) {
            float direction = (mouseButton == 4) ? 1.0f : -1.0f;
            for (const auto& knob : knobs) {
                float dx = mx - knob.x;
                float dy = my - knob.y;
                if (dx * dx + dy * dy <= (knob.radius + 10.0f) * (knob.radius + 10.0f)) {
                    const auto* defs = getParameterDefs();
                    float range = defs[knob.paramId].maxVal - defs[knob.paramId].minVal;
                    float step = range * 0.02f;
                    if (defs[knob.paramId].isInt) step = 1.0f;
                    float cur = engine->getParameter(knob.paramId);
                    float nextV = std::clamp(cur + direction * step, defs[knob.paramId].minVal, defs[knob.paramId].maxVal);
                    engine->setParameter(knob.paramId, nextV);
                    redraw();
                    return;
                }
            }
            return;
        }

        if (mouseButton != 1) return; // Left click

        // Check Preset Bar prev/next buttons
        float bx = 410, by = 10, bw = 330, bh = 28;
        if (my >= by && my <= by + bh) {
            // Prev button (<)
            if (mx >= bx && mx <= bx + 30) {
                const auto& presets = getPresets();
                currentPresetIdx = (currentPresetIdx - 1 + (int)presets.size()) % (int)presets.size();
                engine->loadPreset(currentPresetIdx);
                redraw();
                return;
            }
            // Next button (>)
            if (mx >= bx + bw - 30 && mx <= bx + bw) {
                const auto& presets = getPresets();
                currentPresetIdx = (currentPresetIdx + 1) % (int)presets.size();
                engine->loadPreset(currentPresetIdx);
                redraw();
                return;
            }
        }

        // Check Buttons
        for (const auto& btn : buttons) {
            if (btn.paramId >= 0 && mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) {
                if (btn.isToggle) {
                    float cur = engine->getParameter(btn.paramId);
                    engine->setParameter(btn.paramId, (cur > 0.5f) ? 0.0f : 1.0f);
                } else {
                    engine->setParameter(btn.paramId, btn.targetValue);
                }
                redraw();
                return;
            }
        }

        // Check Knobs
        for (const auto& knob : knobs) {
            float dx = mx - knob.x;
            float dy = my - knob.y;
            if (dx * dx + dy * dy <= (knob.radius + 10.0f) * (knob.radius + 10.0f)) {
                if (isDoubleClick) {
                    // Reset to factory default value!
                    const auto* defs = getParameterDefs();
                    engine->setParameter(knob.paramId, defs[knob.paramId].defaultVal);
                    redraw();
                    return;
                }
                activeKnobId = knob.paramId;
                dragStartY = my;
                dragStartVal = engine->getParameter(knob.paramId);
                return;
            }
        }
    }

    void onMouseMove(int mx, int my, unsigned int state) {
        (void)mx;
        if (activeKnobId >= 0) {
            float deltaY = dragStartY - my;
            const auto* defs = getParameterDefs();
            float range = defs[activeKnobId].maxVal - defs[activeKnobId].minVal;

            // Shift key for high precision fine-tuning
            bool fine = (state & ShiftMask);
            float sensitivity = fine ? 0.001f : 0.006f;

            float newVal = dragStartVal + deltaY * range * sensitivity;
            newVal = std::clamp(newVal, defs[activeKnobId].minVal, defs[activeKnobId].maxVal);
            if (defs[activeKnobId].isInt) newVal = std::round(newVal);

            engine->setParameter(activeKnobId, newVal);
            redraw();
        }
    }

    void onMouseUp(int mx, int my, int mouseButton) {
        (void)mx; (void)my; (void)mouseButton;
        activeKnobId = -1;
    }

    void blitToWindow() {
        if (!display || !window || !cairoSurface || !gc) return;
        cairo_surface_flush(cairoSurface);
        unsigned char* data = cairo_image_surface_get_data(cairoSurface);
        int stride = cairo_image_surface_get_stride(cairoSurface);
        XImage* img = XCreateImage(display, visual, depth, ZPixmap, 0,
                                   (char*)data, UI_WIDTH, UI_HEIGHT, 32, stride);
        if (img) {
            XPutImage(display, window, gc, img, 0, 0, 0, 0, UI_WIDTH, UI_HEIGHT);
            img->data = nullptr; // prevent XDestroyImage from freeing Cairo's pixel data
            XDestroyImage(img);
            XFlush(display);
        }
    }

    void roundedRect(cairo_t* cr, double x, double y, double w, double h, double r) {
        cairo_new_path(cr);
        cairo_arc(cr, x + w - r, y + r, r, -1.5707963, 0);
        cairo_arc(cr, x + w - r, y + h - r, r, 0, 1.5707963);
        cairo_arc(cr, x + r, y + h - r, r, 1.5707963, 3.14159265);
        cairo_arc(cr, x + r, y + r, r, 3.14159265, 4.71238898);
        cairo_close_path(cr);
    }

    ThirdBassEngine* engine = nullptr;
    Display* display = nullptr;
    Window parentWindow = 0;
    Window window = 0;
    GC gc = nullptr;
    int screen = 0;
    Visual* visual = nullptr;
    int depth = 0;

    cairo_surface_t* cairoSurface = nullptr;
    cairo_t* cairoContext = nullptr;

    std::vector<KnobWidget> knobs;
    std::vector<ButtonWidget> buttons;

    int activeKnobId = -1;
    int dragStartY = 0;
    float dragStartVal = 0.0f;
    int currentPresetIdx = 0;
    unsigned long lastClickTime = 0;

    std::thread eventThread;
    std::atomic<bool> uiRunning{false};
    std::atomic<bool> windowMapped{false};
    std::mutex renderMutex;
};

} // namespace ThirdBass
