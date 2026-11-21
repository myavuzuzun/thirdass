#pragma once

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <cmath>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include "../dsp/ThirdAssDSP.hpp"

#if defined(_MSC_VER)
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "ole32.lib")
#endif

namespace ThirdAss {

struct D2DColor {
    float r, g, b, a;
    D2DColor(float red = 0.0f, float green = 0.0f, float blue = 0.0f, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
    D2D1_COLOR_F toD2D() const { return D2D1::ColorF(r, g, b, a); }
};

struct KnobWidget {
    int paramId;
    float x, y, radius;
    std::string title;
    std::wstring wTitle;
    D2DColor accentColor;
};

struct ButtonWidget {
    int paramId;
    float x, y, w, h;
    std::string text;
    std::wstring wText;
    float targetValue; // for multi-choice buttons
    bool isToggle;
    D2DColor activeColor;
};

class PluginUI {
public:
    static constexpr int UI_WIDTH = 980;
    static constexpr int UI_HEIGHT = 560;

    PluginUI(ThirdAssEngine* eng) : engine(eng) {
        initLayout();
        initDirect2D();
    }

    ~PluginUI() {
        closeWindow();
        cleanupDirect2D();
    }

    bool openWindow(void* parentWin) {
        closeWindow();
        parentHWnd = static_cast<HWND>(parentWin);

        HINSTANCE hInstance = GetModuleHandle(nullptr);
        registerWindowClass(hInstance);

        hWnd = CreateWindowExA(
            0,
            "ThirdAssPluginWindow",
            "Third Ass",
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0, UI_WIDTH, UI_HEIGHT,
            parentHWnd,
            nullptr,
            hInstance,
            this
        );

        if (!hWnd) {
            return false;
        }

        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        createDeviceResources();
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);

        // 60 FPS animation/meter timer
        SetTimer(hWnd, TIMER_ID, 16, nullptr);

        windowMapped = true;
        return true;
    }

    void closeWindow() {
        if (hWnd) {
            KillTimer(hWnd, TIMER_ID);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
            DestroyWindow(hWnd);
            hWnd = nullptr;
        }
        discardDeviceResources();
        windowMapped = false;
        parentHWnd = nullptr;
    }

    void redraw() {
        if (hWnd && windowMapped) {
            InvalidateRect(hWnd, nullptr, FALSE);
        }
    }

    void idle() {
        if (hWnd && windowMapped) {
            // Check if any meter has active movement
            float curOutL = engine ? engine->getMeterOutPeakL() : 0.0f;
            float curOutR = engine ? engine->getMeterOutPeakR() : 0.0f;
            float curComp = engine ? engine->getMeterCompGR() : 0.0f;
            float curGate = engine ? engine->getMeterGateGain() : 1.0f;

            if (std::abs(curOutL - prevOutL) > 0.005f ||
                std::abs(curOutR - prevOutR) > 0.005f ||
                std::abs(curComp - prevComp) > 0.1f ||
                std::abs(curGate - prevGate) > 0.05f ||
                activeKnobId >= 0) {
                prevOutL = curOutL;
                prevOutR = curOutR;
                prevComp = curComp;
                prevGate = curGate;
                redraw();
            }
        }
    }

    HWND getHWnd() const { return hWnd; }

private:
    static constexpr UINT_PTR TIMER_ID = 1001;

    ThirdAssEngine* engine = nullptr;
    HWND parentHWnd = nullptr;
    HWND hWnd = nullptr;
    bool windowMapped = false;

    // Direct2D / DirectWrite Core
    ID2D1Factory* pD2DFactory = nullptr;
    IDWriteFactory* pDWriteFactory = nullptr;
    ID2D1HwndRenderTarget* pRenderTarget = nullptr;
    ID2D1SolidColorBrush* pSolidBrush = nullptr;
    ID2D1StrokeStyle* pRoundStrokeStyle = nullptr;

    // Cached GPU Gradient Brush for subtle EQ fill
    ID2D1LinearGradientBrush* pEqGradBrush = nullptr;

    // DirectWrite Text Formats
    IDWriteTextFormat* pFormatTitle = nullptr;
    IDWriteTextFormat* pFormatSubtitle = nullptr;
    IDWriteTextFormat* pFormatPreset = nullptr;
    IDWriteTextFormat* pFormatModule = nullptr;
    IDWriteTextFormat* pFormatKnobTitle = nullptr;
    IDWriteTextFormat* pFormatKnobVal = nullptr;
    IDWriteTextFormat* pFormatButton = nullptr;
    IDWriteTextFormat* pFormatSmall = nullptr;
    IDWriteTextFormat* pFormatTiny = nullptr;

    std::vector<KnobWidget> knobs;
    std::vector<ButtonWidget> buttons;

    int activeKnobId = -1;
    int dragStartY = 0;
    float dragStartVal = 0.0f;
    int currentPresetIdx = 0;

    float prevOutL = 0.0f;
    float prevOutR = 0.0f;
    float prevComp = 0.0f;
    float prevGate = 1.0f;

    std::mutex renderMutex;

    void registerWindowClass(HINSTANCE hInstance) {
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXA wc = {};
            wc.cbSize = sizeof(WNDCLASSEXA);
            wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = WndProcStatic;
            wc.hInstance = hInstance;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.lpszClassName = "ThirdAssPluginWindow";
            RegisterClassExA(&wc);
            registered = true;
        }
    }

    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        PluginUI* pThis = reinterpret_cast<PluginUI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (pThis) {
            return pThis->handleMessage(hwnd, uMsg, wParam, lParam);
        }
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }

    LRESULT handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_PAINT: {
                PAINTSTRUCT ps;
                BeginPaint(hwnd, &ps);
                render();
                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_ERASEBKGND:
                return 1; // Double buffering handled by Direct2D

            case WM_TIMER:
                if (wParam == TIMER_ID) {
                    idle();
                }
                return 0;

            case WM_LBUTTONDOWN: {
                int mx = GET_X_LPARAM(lParam);
                int my = GET_Y_LPARAM(lParam);
                SetCapture(hwnd);
                onMouseDown(mx, my, false);
                return 0;
            }

            case WM_LBUTTONDBLCLK: {
                int mx = GET_X_LPARAM(lParam);
                int my = GET_Y_LPARAM(lParam);
                onMouseDown(mx, my, true);
                return 0;
            }

            case WM_MOUSEMOVE: {
                int mx = GET_X_LPARAM(lParam);
                int my = GET_Y_LPARAM(lParam);
                bool shiftDown = (wParam & MK_SHIFT) != 0;
                onMouseMove(mx, my, shiftDown);
                return 0;
            }

            case WM_LBUTTONUP: {
                ReleaseCapture();
                onMouseUp();
                return 0;
            }

            case WM_MOUSEWHEEL: {
                int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ScreenToClient(hwnd, &pt);
                onMouseWheel(pt.x, pt.y, delta);
                return 0;
            }

            case WM_DESTROY:
                discardDeviceResources();
                return 0;
        }
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }

    void initDirect2D() {
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory));

        if (pD2DFactory) {
            D2D1_STROKE_STYLE_PROPERTIES strokeProps = D2D1::StrokeStyleProperties(
                D2D1_CAP_STYLE_ROUND,
                D2D1_CAP_STYLE_ROUND,
                D2D1_CAP_STYLE_ROUND,
                D2D1_LINE_JOIN_ROUND
            );
            pD2DFactory->CreateStrokeStyle(strokeProps, nullptr, 0, &pRoundStrokeStyle);
        }

        if (pDWriteFactory) {
            auto createFmt = [&](LPCWSTR family, float size, DWRITE_FONT_WEIGHT weight, IDWriteTextFormat** ppFmt, DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING) {
                pDWriteFactory->CreateTextFormat(
                    family, nullptr, weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                    size, L"en-us", ppFmt
                );
                if (*ppFmt) {
                    (*ppFmt)->SetTextAlignment(align);
                    (*ppFmt)->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                }
            };

            createFmt(L"Segoe UI", 22.0f, DWRITE_FONT_WEIGHT_BOLD, &pFormatTitle, DWRITE_TEXT_ALIGNMENT_LEADING);
            createFmt(L"Segoe UI", 10.0f, DWRITE_FONT_WEIGHT_NORMAL, &pFormatSubtitle, DWRITE_TEXT_ALIGNMENT_LEADING);
            createFmt(L"Segoe UI", 12.0f, DWRITE_FONT_WEIGHT_BOLD, &pFormatPreset, DWRITE_TEXT_ALIGNMENT_CENTER);
            createFmt(L"Segoe UI", 10.5f, DWRITE_FONT_WEIGHT_BOLD, &pFormatModule, DWRITE_TEXT_ALIGNMENT_LEADING);
            createFmt(L"Segoe UI", 9.0f, DWRITE_FONT_WEIGHT_BOLD, &pFormatKnobTitle, DWRITE_TEXT_ALIGNMENT_CENTER);
            createFmt(L"Segoe UI", 8.5f, DWRITE_FONT_WEIGHT_NORMAL, &pFormatKnobVal, DWRITE_TEXT_ALIGNMENT_CENTER);
            createFmt(L"Segoe UI", 9.0f, DWRITE_FONT_WEIGHT_BOLD, &pFormatButton, DWRITE_TEXT_ALIGNMENT_CENTER);
            createFmt(L"Segoe UI", 7.5f, DWRITE_FONT_WEIGHT_NORMAL, &pFormatSmall, DWRITE_TEXT_ALIGNMENT_LEADING);
            createFmt(L"Segoe UI", 7.0f, DWRITE_FONT_WEIGHT_NORMAL, &pFormatTiny, DWRITE_TEXT_ALIGNMENT_CENTER);
        }
    }

    void cleanupDirect2D() {
        discardDeviceResources();
        if (pRoundStrokeStyle) { pRoundStrokeStyle->Release(); pRoundStrokeStyle = nullptr; }
        if (pFormatTiny) { pFormatTiny->Release(); pFormatTiny = nullptr; }
        if (pFormatSmall) { pFormatSmall->Release(); pFormatSmall = nullptr; }
        if (pFormatButton) { pFormatButton->Release(); pFormatButton = nullptr; }
        if (pFormatKnobVal) { pFormatKnobVal->Release(); pFormatKnobVal = nullptr; }
        if (pFormatKnobTitle) { pFormatKnobTitle->Release(); pFormatKnobTitle = nullptr; }
        if (pFormatModule) { pFormatModule->Release(); pFormatModule = nullptr; }
        if (pFormatPreset) { pFormatPreset->Release(); pFormatPreset = nullptr; }
        if (pFormatSubtitle) { pFormatSubtitle->Release(); pFormatSubtitle = nullptr; }
        if (pFormatTitle) { pFormatTitle->Release(); pFormatTitle = nullptr; }
        if (pDWriteFactory) { pDWriteFactory->Release(); pDWriteFactory = nullptr; }
        if (pD2DFactory) { pD2DFactory->Release(); pD2DFactory = nullptr; }
    }

    void createDeviceResources() {
        if (!pRenderTarget && hWnd && pD2DFactory) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            D2D1_SIZE_U size = D2D1::SizeU(
                std::max<UINT32>(1, rc.right - rc.left),
                std::max<UINT32>(1, rc.bottom - rc.top)
            );

            D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                0, 0,
                D2D1_RENDER_TARGET_USAGE_NONE,
                D2D1_FEATURE_LEVEL_DEFAULT
            );

            D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
                hWnd, size, D2D1_PRESENT_OPTIONS_IMMEDIATELY
            );

            HRESULT hr = pD2DFactory->CreateHwndRenderTarget(rtProps, hwndProps, &pRenderTarget);
            if (SUCCEEDED(hr) && pRenderTarget) {
                pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
                pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pSolidBrush);

                // Subtle Monochrome EQ Fill Gradient Brush
                D2D1_GRADIENT_STOP stopsEq[2] = {
                    { 0.0f, D2D1::ColorF(0.92f, 0.94f, 0.97f, 0.12f) },
                    { 1.0f, D2D1::ColorF(0.92f, 0.94f, 0.97f, 0.01f) }
                };
                ID2D1GradientStopCollection* pStopsEq = nullptr;
                if (SUCCEEDED(pRenderTarget->CreateGradientStopCollection(stopsEq, 2, &pStopsEq))) {
                    D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES linProps = D2D1::LinearGradientBrushProperties(
                        D2D1::Point2F(518, 176), D2D1::Point2F(518, 176 + 98)
                    );
                    pRenderTarget->CreateLinearGradientBrush(linProps, pStopsEq, &pEqGradBrush);
                    pStopsEq->Release();
                }
            }
        }
    }

    void discardDeviceResources() {
        if (pEqGradBrush) { pEqGradBrush->Release(); pEqGradBrush = nullptr; }
        if (pSolidBrush) { pSolidBrush->Release(); pSolidBrush = nullptr; }
        if (pRenderTarget) { pRenderTarget->Release(); pRenderTarget = nullptr; }
    }

    void initLayout() {
        knobs.clear();
        buttons.clear();

        // Monochrome Flat Design Theme Palette
        D2DColor colMonoArc(0.90f, 0.92f, 0.96f);   // Crisp Off-White/Silver active knob arc
        D2DColor colMonoBtn(0.92f, 0.94f, 0.97f);   // High-contrast inverted active button

        auto addKnob = [&](int id, float x, float y, float r, const char* title, const D2DColor& col) {
            std::string s(title);
            std::wstring ws(s.begin(), s.end());
            knobs.push_back({id, x, y, r, s, ws, col});
        };

        auto addButton = [&](int id, float x, float y, float w, float h, const char* text, float targetVal, bool isToggle, const D2DColor& col) {
            std::string s(text);
            std::wstring ws(s.begin(), s.end());
            buttons.push_back({id, x, y, w, h, s, ws, targetVal, isToggle, col});
        };

        // -------------------------------------------------------------
        // ROW 1: PREAMP & TONE (y: 54 to 290)
        // -------------------------------------------------------------

        // Module 1: INPUT & GATE (x: 14, w: 180)
        addKnob(PARAM_IN_GAIN, 58, 162, 26, "IN GAIN", colMonoArc);
        addKnob(PARAM_GATE_THRESH, 140, 126, 20, "THRESH", colMonoArc);
        addKnob(PARAM_GATE_RELEASE, 140, 216, 20, "RELEASE", colMonoArc);

        // Module 2: PREAMP DRIVE & TONE (x: 204, w: 290)
        addButton(PARAM_DRIVE_ENABLE, 448, 62, 38, 18, "ON", 1.0f, true, colMonoBtn);
        addButton(PARAM_DRIVE_MODE, 214, 88, 82, 22, "TUBE", 0.0f, false, colMonoBtn);
        addButton(PARAM_DRIVE_MODE, 302, 88, 96, 22, "OVERDRIVE", 1.0f, false, colMonoBtn);
        addButton(PARAM_DRIVE_MODE, 404, 88, 82, 22, "FUZZ", 2.0f, false, colMonoBtn);
        addKnob(PARAM_DRIVE_AMOUNT, 274, 192, 32, "DRIVE", colMonoArc);
        addKnob(PARAM_DRIVE_TONE, 372, 156, 21, "CONTOUR", colMonoArc);
        addKnob(PARAM_DRIVE_MIX, 438, 212, 21, "BLEND", colMonoArc);

        // Module 3: 4-BAND BASS TONE STACK (x: 504, w: 462)
        addButton(PARAM_EQ_ENABLE, 918, 62, 40, 18, "ON", 1.0f, true, colMonoBtn);
        addKnob(PARAM_EQ_BASS, 558, 126, 22, "BASS 80Hz", colMonoArc);
        addKnob(PARAM_EQ_LOW_MID, 672, 126, 22, "LOW MID 350Hz", colMonoArc);
        addKnob(PARAM_EQ_HIGH_MID, 786, 126, 22, "HI MID 1.8kHz", colMonoArc);
        addKnob(PARAM_EQ_TREBLE, 900, 126, 22, "TREBLE 4.5kHz", colMonoArc);

        // -------------------------------------------------------------
        // ROW 2: PROCESSING & EFFECTS (y: 298 to 548)
        // -------------------------------------------------------------

        // Module 4: RESURRECTION SUB ENHANCER (x: 14, w: 180)
        addButton(PARAM_SUB_ENABLE, 146, 306, 40, 18, "ON", 1.0f, true, colMonoBtn);
        addKnob(PARAM_SUB_FREQ, 58, 372, 21, "TRACK Hz", colMonoArc);
        addKnob(PARAM_SUB_HARMONICS, 142, 372, 21, "HARMONICS", colMonoArc);
        addKnob(PARAM_SUB_LEVEL, 100, 472, 23, "SUB BOOST", colMonoArc);

        // Module 5: OPTO COMPRESSOR (x: 204, w: 242)
        addButton(PARAM_COMP_ENABLE, 398, 306, 40, 18, "ON", 1.0f, true, colMonoBtn);
        addKnob(PARAM_COMP_THRESH, 248, 366, 20, "THRESH", colMonoArc);
        addKnob(PARAM_COMP_RATIO, 325, 366, 20, "RATIO", colMonoArc);
        addKnob(PARAM_COMP_ATTACK, 402, 366, 20, "ATTACK", colMonoArc);
        addKnob(PARAM_COMP_RELEASE, 278, 478, 20, "RELEASE", colMonoArc);
        addKnob(PARAM_COMP_MAKEUP, 370, 478, 20, "MAKEUP", colMonoArc);

        // Module 6: CROSSOVER CHORUS (x: 456, w: 176)
        addButton(PARAM_CHORUS_ENABLE, 584, 306, 40, 18, "ON", 1.0f, true, colMonoBtn);
        addKnob(PARAM_CHORUS_RATE, 500, 368, 20, "RATE", colMonoArc);
        addKnob(PARAM_CHORUS_DEPTH, 588, 368, 20, "DEPTH", colMonoArc);
        addKnob(PARAM_CHORUS_CROSSOVER, 500, 468, 20, "HP CROSS", colMonoArc);
        addKnob(PARAM_CHORUS_MIX, 588, 468, 20, "MIX", colMonoArc);

        // Module 7: ANALOG DELAY (x: 642, w: 176)
        addButton(PARAM_DELAY_ENABLE, 770, 306, 40, 18, "ON", 1.0f, true, colMonoBtn);
        addKnob(PARAM_DELAY_TIME, 686, 368, 20, "TIME", colMonoArc);
        addKnob(PARAM_DELAY_FEEDBACK, 774, 368, 20, "FEEDBACK", colMonoArc);
        addKnob(PARAM_DELAY_DAMP, 686, 468, 20, "DAMPING", colMonoArc);
        addKnob(PARAM_DELAY_MIX, 774, 468, 20, "MIX", colMonoArc);

        // Module 8: CABINET & MASTER (x: 828, w: 138)
        addButton(PARAM_CAB_MODE, 836, 328, 122, 16, "DIRECT / BYPASS", 0.0f, false, colMonoBtn);
        addButton(PARAM_CAB_MODE, 836, 347, 122, 16, "8x10 SVT FRIDGE", 1.0f, false, colMonoBtn);
        addButton(PARAM_CAB_MODE, 836, 366, 122, 16, "1x15 PORTAFLEX", 2.0f, false, colMonoBtn);
        addButton(PARAM_CAB_MODE, 836, 385, 122, 16, "4x10 MODERN PUNCH", 3.0f, false, colMonoBtn);
        addButton(PARAM_CAB_MODE, 836, 404, 122, 16, "2x12 ROCK BITE", 4.0f, false, colMonoBtn);

        addButton(PARAM_LIMITER_ENABLE, 836, 425, 122, 17, "LIMITER ON", 1.0f, true, colMonoBtn);
        addKnob(PARAM_OUT_GAIN, 864, 478, 19, "OUT", colMonoArc);
        addKnob(PARAM_DRY_WET, 930, 478, 19, "MIX", colMonoArc);
    }

    void render() {
        std::lock_guard<std::mutex> lock(renderMutex);
        if (!pRenderTarget) {
            createDeviceResources();
            if (!pRenderTarget) return;
        }

        pRenderTarget->BeginDraw();

        // 1. Chassis Outer Background (Matte Dark Flat Slate)
        setBrushColor(D2DColor(0.075f, 0.080f, 0.090f));
        pRenderTarget->FillRectangle(D2D1::RectF(0, 0, UI_WIDTH, UI_HEIGHT), pSolidBrush);

        // Chassis 1px Border
        setBrushColor(D2DColor(0.18f, 0.19f, 0.22f));
        pRenderTarget->DrawRectangle(D2D1::RectF(0.5f, 0.5f, UI_WIDTH - 0.5f, UI_HEIGHT - 0.5f), pSolidBrush, 1.0f);

        // 2. Top Header Bar (Flat minimalist)
        setBrushColor(D2DColor(0.055f, 0.058f, 0.065f));
        pRenderTarget->FillRectangle(D2D1::RectF(0, 0, UI_WIDTH, 48), pSolidBrush);

        setBrushColor(D2DColor(0.16f, 0.17f, 0.19f));
        pRenderTarget->DrawLine(D2D1::Point2F(0, 48), D2D1::Point2F(UI_WIDTH, 48), pSolidBrush, 1.0f);

        // Branding Title
        setBrushColor(D2DColor(0.96f, 0.97f, 0.98f)); // Crisp Off-White
        drawDWriteText(L"THIRD ASS", 22, 10, 140, 36, pFormatTitle);

        setBrushColor(D2DColor(0.50f, 0.53f, 0.58f));
        drawDWriteText(L"BASS MULTI-PROCESSOR", 172, 13, 230, 36, pFormatSubtitle);

        // Preset Selector Bar
        drawPresetBar();

        // Top-right Master Level & Windows Badge
        drawTopRightStatus();

        // 3. Draw Module Panels (Clean Flat Cards)
        drawModuleBox(14, 54, 180, 236, L"INPUT & GATE");
        drawModuleBox(204, 54, 290, 236, L"PREAMP DRIVE & TONE");
        drawModuleBox(504, 54, 462, 236, L"4-BAND BASS TONE STACK");

        drawModuleBox(14, 298, 180, 250, L"SUB RESURRECTION");
        drawModuleBox(204, 298, 242, 250, L"OPTO COMPRESSOR");
        drawModuleBox(456, 298, 176, 250, L"BASS CHORUS");
        drawModuleBox(642, 298, 176, 250, L"STEREO DELAY");
        drawModuleBox(828, 298, 138, 250, L"CABINET & OUT");

        // Module-specific visual meters & graphics
        drawGateLed(142, 68);
        drawTubeVisual(372, 234);
        drawEQVisualizer(518, 176, 434, 98);
        drawSubHarmonicVisual(100, 400);
        drawCompGRMeter(218, 414, 214, 18);
        drawChorusVisual(544, 418);
        drawDelayVisual(730, 418);
        drawMasterOutputMeter(838, 524, 118, 14);

        // 4. Draw Buttons & Switches
        for (const auto& btn : buttons) {
            drawButton(btn);
        }

        // 5. Draw Rotary Knobs
        for (const auto& knob : knobs) {
            drawKnob(knob);
        }

        HRESULT hr = pRenderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            discardDeviceResources();
        }
    }

    void setBrushColor(const D2DColor& c) {
        if (pSolidBrush) {
            pSolidBrush->SetColor(c.toD2D());
        }
    }

    void drawTopRightStatus() {
        float outL = engine ? engine->getMeterOutPeakL() : 0.0f;
        float outR = engine ? engine->getMeterOutPeakR() : 0.0f;

        float bx = 760, by = 15, bw = 100, bh = 18;
        setBrushColor(D2DColor(0.08f, 0.085f, 0.095f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(bx, by, bx + bw, by + bh), 2, 2), pSolidBrush);

        setBrushColor(D2DColor(0.18f, 0.19f, 0.22f));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(bx, by, bx + bw, by + bh), 2, 2), pSolidBrush, 1.0f);

        // L bar
        drawMeterSegmentBar(bx + 16, by + 3, bw - 20, 5, outL);
        // R bar
        drawMeterSegmentBar(bx + 16, by + 10, bw - 20, 5, outR);

        setBrushColor(D2DColor(0.50f, 0.54f, 0.60f));
        drawDWriteText(L"L", bx + 4, by + 2, 10, 8, pFormatSmall);
        drawDWriteText(L"R", bx + 4, by + 9, 10, 8, pFormatSmall);

        // VeSTige x64 Badge (Monochrome Minimalist)
        float tagX = 878, tagY = 15, tagW = 82, tagH = 18;
        setBrushColor(D2DColor(0.10f, 0.105f, 0.12f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(tagX, tagY, tagX + tagW, tagY + tagH), 2, 2), pSolidBrush);

        setBrushColor(D2DColor(0.20f, 0.21f, 0.24f));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(tagX, tagY, tagX + tagW, tagY + tagH), 2, 2), pSolidBrush, 1.0f);

        setBrushColor(D2DColor(0.70f, 0.74f, 0.80f));
        drawDWriteText(L"VeSTige x64", tagX, tagY + 1, tagW, tagH - 2, pFormatSmall, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    void drawMeterSegmentBar(float x, float y, float w, float h, float peakLin) {
        float peakDb = (peakLin > 1e-5f) ? (8.685889638f * std::log(peakLin)) : -60.0f;
        float norm = std::clamp((peakDb + 48.0f) / 48.0f, 0.0f, 1.0f);

        int numSegs = 14;
        float segW = (w - (numSegs - 1) * 1.5f) / numSegs;
        int activeCount = static_cast<int>(std::round(norm * numSegs));

        for (int i = 0; i < numSegs; ++i) {
            float sx = x + i * (segW + 1.5f);
            bool lit = (i < activeCount);
            if (i < 10) {
                // Cool Silver-Gray (-48 to -6 dB)
                setBrushColor(D2DColor(0.72f, 0.75f, 0.82f, lit ? 0.95f : 0.12f));
            } else if (i < 13) {
                // Crisp White (-6 to 0 dB)
                setBrushColor(D2DColor(0.96f, 0.97f, 1.00f, lit ? 0.95f : 0.15f));
            } else {
                // Subtle Red Clip (> 0 dB)
                setBrushColor(D2DColor(0.92f, 0.32f, 0.32f, lit ? 1.0f : 0.20f));
            }
            pRenderTarget->FillRectangle(D2D1::RectF(sx, y, sx + segW, y + h), pSolidBrush);
        }
    }

    void drawPresetBar() {
        float bx = 410, by = 10, bw = 330, bh = 28;

        setBrushColor(D2DColor(0.10f, 0.105f, 0.12f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(bx, by, bx + bw, by + bh), 3, 3), pSolidBrush);

        setBrushColor(D2DColor(0.20f, 0.21f, 0.24f));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(bx, by, bx + bw, by + bh), 3, 3), pSolidBrush, 1.0f);

        // Prev Arrow Button (<)
        setBrushColor(D2DColor(0.14f, 0.15f, 0.17f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(bx + 2, by + 2, bx + 28, by + bh - 2), 2, 2), pSolidBrush);
        setBrushColor(D2DColor(0.22f, 0.24f, 0.28f));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(bx + 2, by + 2, bx + 28, by + bh - 2), 2, 2), pSolidBrush, 1.0f);
        setBrushColor(D2DColor(0.78f, 0.82f, 0.88f));
        drawDWriteText(L"<", bx + 2, by + 2, 26, bh - 4, pFormatPreset);

        // Next Arrow Button (>)
        setBrushColor(D2DColor(0.14f, 0.15f, 0.17f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(bx + bw - 28, by + 2, bx + bw - 2, by + bh - 2), 2, 2), pSolidBrush);
        setBrushColor(D2DColor(0.22f, 0.24f, 0.28f));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(bx + bw - 28, by + 2, bx + bw - 2, by + bh - 2), 2, 2), pSolidBrush, 1.0f);
        setBrushColor(D2DColor(0.78f, 0.82f, 0.88f));
        drawDWriteText(L">", bx + bw - 28, by + 2, 26, bh - 4, pFormatPreset);

        // Preset Name Text
        const auto& presets = getPresets();
        std::wstring pName = L"01: Clean Studio DI";
        if (currentPresetIdx >= 0 && currentPresetIdx < static_cast<int>(presets.size())) {
            const char* nameStr = presets[currentPresetIdx].name;
            pName.assign(nameStr, nameStr + strlen(nameStr));
        }

        setBrushColor(D2DColor(0.92f, 0.94f, 0.97f));
        drawDWriteText(pName.c_str(), bx + 32, by + 2, bw - 64, bh - 4, pFormatPreset);
    }

    void drawModuleBox(float x, float y, float w, float h, const wchar_t* title, const D2DColor& accent = D2DColor(0.72f, 0.75f, 0.82f)) {
        // Flat solid background
        setBrushColor(D2DColor(0.105f, 0.110f, 0.125f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), 4, 4), pSolidBrush);

        // Thin flat border
        setBrushColor(D2DColor(0.18f, 0.19f, 0.22f));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), 4, 4), pSolidBrush, 1.0f);

        // Minimalist subtle header accent marker (monochrome silver pill)
        setBrushColor(accent);
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x + 10, y + 8, x + 13, y + 20), 1.5f, 1.5f), pSolidBrush);

        // Header Title
        setBrushColor(D2DColor(0.85f, 0.88f, 0.93f));
        drawDWriteText(title, x + 20, y + 6, w - 28, 18, pFormatModule);

        // Hairline rule below header
        setBrushColor(D2DColor(0.15f, 0.16f, 0.18f));
        pRenderTarget->DrawLine(D2D1::Point2F(x + 8, y + 26), D2D1::Point2F(x + w - 8, y + 26), pSolidBrush, 1.0f);
    }

    void drawGateLed(float x, float y) {
        float gateGain = engine ? engine->getMeterGateGain() : 1.0f;
        bool open = (gateGain > 0.4f);

        if (open) {
            setBrushColor(D2DColor(0.92f, 0.94f, 0.97f)); // Bright White/Silver when audio passes
        } else {
            setBrushColor(D2DColor(0.20f, 0.22f, 0.26f)); // Muted Dark when gating
        }
        pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), 3.5f, 3.5f), pSolidBrush);

        setBrushColor(D2DColor(0.60f, 0.64f, 0.70f));
        drawDWriteText(open ? L"OPEN" : L"GATE", x + 8, y - 6, 36, 12, pFormatSmall);
    }

    void drawTubeVisual(float cx, float cy) {
        float driveVal = engine ? engine->getParameter(PARAM_DRIVE_AMOUNT) * 0.01f : 0.25f;
        bool driveOn = engine ? (engine->getParameter(PARAM_DRIVE_ENABLE) > 0.5f) : true;

        // Flat minimalist valve schematic outline
        setBrushColor(D2DColor(0.22f, 0.24f, 0.28f));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(cx - 14, cy - 16, cx + 14, cy + 14), 7, 7), pSolidBrush, 1.0f);

        // Valve base pin line
        pRenderTarget->DrawLine(D2D1::Point2F(cx - 9, cy + 14), D2D1::Point2F(cx + 9, cy + 14), pSolidBrush, 1.5f);

        // Internal element / filament (Monochrome intensity scale)
        float intensity = driveOn ? (0.35f + driveVal * 0.65f) : 0.15f;
        setBrushColor(D2DColor(0.95f * intensity, 0.96f * intensity, 1.0f * intensity));

        // Minimalist dual filament hairpin
        pRenderTarget->DrawLine(D2D1::Point2F(cx - 4, cy + 8), D2D1::Point2F(cx - 4, cy - 6), pSolidBrush, 1.8f);
        pRenderTarget->DrawLine(D2D1::Point2F(cx - 4, cy - 6), D2D1::Point2F(cx - 1, cy - 9), pSolidBrush, 1.8f);
        pRenderTarget->DrawLine(D2D1::Point2F(cx - 1, cy - 9), D2D1::Point2F(cx - 1, cy + 8), pSolidBrush, 1.8f);

        pRenderTarget->DrawLine(D2D1::Point2F(cx + 1, cy + 8), D2D1::Point2F(cx + 1, cy - 9), pSolidBrush, 1.8f);
        pRenderTarget->DrawLine(D2D1::Point2F(cx + 1, cy - 9), D2D1::Point2F(cx + 4, cy - 6), pSolidBrush, 1.8f);
        pRenderTarget->DrawLine(D2D1::Point2F(cx + 4, cy - 6), D2D1::Point2F(cx + 4, cy + 8), pSolidBrush, 1.8f);

        // Label
        setBrushColor(D2DColor(0.50f, 0.54f, 0.60f));
        drawDWriteText(L"12AX7", cx - 18, cy + 16, 36, 12, pFormatSmall, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    void drawEQVisualizer(float x, float y, float w, float h) {
        // Deep matte display frame
        setBrushColor(D2DColor(0.08f, 0.085f, 0.095f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), 3, 3), pSolidBrush);

        setBrushColor(D2DColor(0.18f, 0.19f, 0.22f));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), 3, 3), pSolidBrush, 1.0f);

        // Center 0 dB baseline & +/- 6 dB reference lines
        float zeroY = y + h * 0.5f;
        float plus6Y = zeroY - 18.0f;
        float minus6Y = zeroY + 18.0f;

        setBrushColor(D2DColor(1.0f, 1.0f, 1.0f, 0.10f));
        pRenderTarget->DrawLine(D2D1::Point2F(x, zeroY), D2D1::Point2F(x + w, zeroY), pSolidBrush, 1.0f);

        setBrushColor(D2DColor(1.0f, 1.0f, 1.0f, 0.05f));
        pRenderTarget->DrawLine(D2D1::Point2F(x, plus6Y), D2D1::Point2F(x + w, plus6Y), pSolidBrush, 1.0f);
        pRenderTarget->DrawLine(D2D1::Point2F(x, minus6Y), D2D1::Point2F(x + w, minus6Y), pSolidBrush, 1.0f);

        auto freqToX = [&](float freq) {
            float minF = 20.0f, maxF = 20000.0f;
            float norm = std::log10(freq / minF) / std::log10(maxF / minF);
            return x + norm * w;
        };

        // Frequency grid markers (100 Hz, 1 kHz, 10 kHz)
        setBrushColor(D2DColor(1.0f, 1.0f, 1.0f, 0.04f));
        for (float f : {100.0f, 1000.0f, 10000.0f}) {
            float fx = freqToX(f);
            pRenderTarget->DrawLine(D2D1::Point2F(fx, y), D2D1::Point2F(fx, y + h), pSolidBrush, 1.0f);
        }

        setBrushColor(D2DColor(0.40f, 0.44f, 0.50f));
        drawDWriteText(L"100Hz", freqToX(100.0f) + 2, y + 2, 36, 12, pFormatSmall);
        drawDWriteText(L"1kHz", freqToX(1000.0f) + 2, y + 2, 36, 12, pFormatSmall);
        drawDWriteText(L"10kHz", freqToX(10000.0f) + 2, y + 2, 36, 12, pFormatSmall);

        // Compute 4-Band EQ Response Curve
        bool eqOn = engine ? (engine->getParameter(PARAM_EQ_ENABLE) > 0.5f) : true;
        float gBass = engine ? engine->getParameter(PARAM_EQ_BASS) : 0.0f;
        float gLowMid = engine ? engine->getParameter(PARAM_EQ_LOW_MID) : 0.0f;
        float gHiMid = engine ? engine->getParameter(PARAM_EQ_HIGH_MID) : 0.0f;
        float gTreb = engine ? engine->getParameter(PARAM_EQ_TREBLE) : 0.0f;

        constexpr int numPts = 80;
        std::array<D2D1_POINT_2F, numPts> curvePts;

        float minF = 20.0f, maxF = 20000.0f;
        for (int i = 0; i < numPts; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(numPts - 1);
            float freq = minF * std::pow(maxF / minF, t);
            float px = x + t * w;

            float gainDb = 0.0f;
            if (eqOn) {
                gainDb += gBass / (1.0f + std::pow(freq / 80.0f, 2.0f));
                float d1 = (freq - 350.0f) / 180.0f;
                gainDb += gLowMid / (1.0f + d1 * d1);
                float d2 = (freq - 1800.0f) / 900.0f;
                gainDb += gHiMid / (1.0f + 1.2f * d2 * d2);
                float fRatio = freq / 4500.0f;
                gainDb += gTreb * (fRatio * fRatio) / (1.0f + fRatio * fRatio);
            }

            float py = zeroY - gainDb * 3.0f;
            py = std::clamp(py, y + 2.0f, y + h - 2.0f);
            curvePts[i] = D2D1::Point2F(px, py);
        }

        // Draw translucent monochrome gradient fill beneath curve
        ID2D1PathGeometry* pFillGeom = nullptr;
        if (SUCCEEDED(pD2DFactory->CreatePathGeometry(&pFillGeom))) {
            ID2D1GeometrySink* pSink = nullptr;
            if (SUCCEEDED(pFillGeom->Open(&pSink))) {
                pSink->BeginFigure(D2D1::Point2F(curvePts[0].x, zeroY), D2D1_FIGURE_BEGIN_FILLED);
                for (const auto& pt : curvePts) {
                    pSink->AddLine(pt);
                }
                pSink->AddLine(D2D1::Point2F(curvePts.back().x, zeroY));
                pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
                pSink->Close();
                pSink->Release();

                if (pEqGradBrush) {
                    pRenderTarget->FillGeometry(pFillGeom, pEqGradBrush);
                }
            }
            pFillGeom->Release();
        }

        // Draw EQ curve line (crisp off-white)
        setBrushColor(D2DColor(0.92f, 0.94f, 0.97f, eqOn ? 0.95f : 0.30f));
        for (size_t i = 1; i < curvePts.size(); ++i) {
            pRenderTarget->DrawLine(curvePts[i - 1], curvePts[i], pSolidBrush, 2.0f);
        }

        // Control Node Dots at center frequencies (80Hz, 350Hz, 1.8kHz, 4.5kHz)
        if (eqOn) {
            float nodeFreqs[] = {80.0f, 350.0f, 1800.0f, 4500.0f};
            float nodeGains[] = {gBass, gLowMid, gHiMid, gTreb};
            for (int k = 0; k < 4; ++k) {
                float nx = freqToX(nodeFreqs[k]);
                float ny = zeroY - nodeGains[k] * 3.0f;
                ny = std::clamp(ny, y + 4.0f, y + h - 4.0f);

                setBrushColor(D2DColor(1.0f, 1.0f, 1.0f));
                pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(nx, ny), 3.5f, 3.5f), pSolidBrush);

                setBrushColor(D2DColor(0.10f, 0.11f, 0.13f));
                pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(nx, ny), 3.5f, 3.5f), pSolidBrush, 1.0f);
            }
        }
    }

    void drawSubHarmonicVisual(float cx, float cy) {
        float subOn = engine ? (engine->getParameter(PARAM_SUB_ENABLE) > 0.5f) : true;
        float harmAmt = engine ? engine->getParameter(PARAM_SUB_HARMONICS) * 0.01f : 0.4f;

        float barW = 8.0f;
        float heights[] = {18.0f, 14.0f * (0.3f + harmAmt * 0.7f), 10.0f * (0.2f + harmAmt * 0.8f)};
        float offsets[] = {-16.0f, 0.0f, 16.0f};
        const wchar_t* labels[] = {L"SUB", L"2ND", L"3RD"};

        for (int i = 0; i < 3; ++i) {
            float bx = cx + offsets[i] - barW * 0.5f;
            float bh = heights[i];
            float by = cy - bh * 0.5f;

            setBrushColor(D2DColor(0.85f, 0.88f, 0.93f, subOn ? 0.85f : 0.20f));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(bx, by, bx + barW, by + bh), 2, 2), pSolidBrush);

            setBrushColor(D2DColor(0.50f, 0.54f, 0.60f));
            drawDWriteText(labels[i], bx - 6, cy + 12, barW + 12, 10, pFormatTiny);
        }
    }

    void drawCompGRMeter(float x, float y, float w, float h) {
        float grDb = engine ? engine->getMeterCompGR() : 0.0f;
        bool compOn = engine ? (engine->getParameter(PARAM_COMP_ENABLE) > 0.5f) : true;

        setBrushColor(D2DColor(0.08f, 0.085f, 0.095f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), 2, 2), pSolidBrush);

        setBrushColor(D2DColor(0.18f, 0.19f, 0.22f));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), 2, 2), pSolidBrush, 1.0f);

        setBrushColor(D2DColor(0.60f, 0.64f, 0.70f));
        drawDWriteText(L"GR dB", x + 6, y + 2, 36, h - 4, pFormatSmall);

        float startX = x + 44.0f;
        float barW = w - 50.0f;
        int numSegs = 10;
        float segW = (barW - (numSegs - 1) * 2.0f) / numSegs;

        float norm = compOn ? std::clamp(grDb / 18.0f, 0.0f, 1.0f) : 0.0f;
        int activeCount = static_cast<int>(std::round(norm * numSegs));

        for (int i = 0; i < numSegs; ++i) {
            float sx = startX + i * (segW + 2.0f);
            bool lit = (i < activeCount);
            if (i < 5) {
                setBrushColor(D2DColor(0.72f, 0.75f, 0.82f, lit ? 0.95f : 0.12f));
            } else if (i < 8) {
                setBrushColor(D2DColor(0.88f, 0.90f, 0.96f, lit ? 0.95f : 0.15f));
            } else {
                setBrushColor(D2DColor(1.0f, 1.0f, 1.0f, lit ? 1.0f : 0.18f));
            }
            pRenderTarget->FillRectangle(D2D1::RectF(sx, y + 3, sx + segW, y + h - 3), pSolidBrush);
        }
    }

    void drawChorusVisual(float cx, float cy) {
        float mix = engine ? engine->getParameter(PARAM_CHORUS_MIX) * 0.01f : 0.4f;
        bool chorusOn = engine ? (engine->getParameter(PARAM_CHORUS_ENABLE) > 0.5f) : true;

        setBrushColor(D2DColor(0.85f, 0.88f, 0.93f, chorusOn ? (0.2f + mix * 0.7f) : 0.15f));
        for (int i = -16; i < 16; ++i) {
            float x1 = cx + i;
            float y1 = cy + std::sin(static_cast<float>(i) * 0.35f) * 6.0f;
            float x2 = cx + (i + 1);
            float y2 = cy + std::sin(static_cast<float>(i + 1) * 0.35f) * 6.0f;
            pRenderTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), pSolidBrush, 1.4f);
        }
    }

    void drawDelayVisual(float cx, float cy) {
        float mix = engine ? engine->getParameter(PARAM_DELAY_MIX) * 0.01f : 0.25f;
        bool delayOn = engine ? (engine->getParameter(PARAM_DELAY_ENABLE) > 0.5f) : true;

        float offsets[] = {-14.0f, -5.0f, 4.0f, 13.0f};
        float radii[] = {3.5f, 2.8f, 2.2f, 1.6f};
        for (int i = 0; i < 4; ++i) {
            float a = delayOn ? (mix * (1.0f - i * 0.22f)) : 0.10f;
            setBrushColor(D2DColor(0.85f, 0.88f, 0.93f, a));
            pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx + offsets[i], cy), radii[i], radii[i]), pSolidBrush);
        }
    }

    void drawMasterOutputMeter(float x, float y, float w, float h) {
        float outL = engine ? engine->getMeterOutPeakL() : 0.0f;
        float outR = engine ? engine->getMeterOutPeakR() : 0.0f;

        setBrushColor(D2DColor(0.08f, 0.085f, 0.095f));
        pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), 2, 2), pSolidBrush);

        setBrushColor(D2DColor(0.18f, 0.19f, 0.22f));
        pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), 2, 2), pSolidBrush, 1.0f);

        float barH = (h - 4.0f) * 0.5f;
        drawMeterSegmentBar(x + 14, y + 2, w - 18, barH, outL);
        drawMeterSegmentBar(x + 14, y + 2 + barH + 1, w - 18, barH, outR);

        setBrushColor(D2DColor(0.50f, 0.54f, 0.60f));
        drawDWriteText(L"L", x + 3, y + 1, 10, barH, pFormatTiny);
        drawDWriteText(L"R", x + 3, y + 2 + barH, 10, barH, pFormatTiny);
    }

    void drawKnob(const KnobWidget& knob) {
        float val = engine ? engine->getParameter(knob.paramId) : 0.0f;
        const auto* defs = getParameterDefs();
        float minV = defs[knob.paramId].minVal;
        float maxV = defs[knob.paramId].maxVal;
        float norm = (maxV > minV) ? std::clamp((val - minV) / (maxV - minV), 0.0f, 1.0f) : 0.0f;

        float r = knob.radius;
        float cx = knob.x;
        float cy = knob.y;

        float startAngle = 2.35619449f; // 135 deg in rad
        float totalSweep = 4.71238898f; // 270 deg in rad
        float currentAngle = startAngle + norm * totalSweep;

        // 1. Sleek Flat Outer Track Arc (Thin, modern, 2.0px)
        drawArc(cx, cy, r + 4.0f, startAngle, totalSweep, D2DColor(0.18f, 0.19f, 0.22f), 2.0f);

        // 2. Active Flat Value Arc (Crisp Off-White/Silver, 2.4px)
        if (norm > 0.005f) {
            drawArc(cx, cy, r + 4.0f, startAngle, norm * totalSweep, knob.accentColor, 2.4f);
        }

        // 3. Flat Matte Inner Knob Body
        bool isKnobActive = (activeKnobId == knob.paramId);
        setBrushColor(isKnobActive ? D2DColor(0.16f, 0.17f, 0.20f) : D2DColor(0.13f, 0.14f, 0.16f));
        pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), pSolidBrush);

        // Thin 1.0px Rim
        setBrushColor(isKnobActive ? D2DColor(0.32f, 0.35f, 0.42f) : D2DColor(0.22f, 0.24f, 0.28f));
        pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), pSolidBrush, 1.0f);

        // 4. Direction Indicator Pointer Line (Clean, high-precision white)
        float indX = cx + (r - 2.5f) * std::cos(currentAngle);
        float indY = cy + (r - 2.5f) * std::sin(currentAngle);
        float indStartX = cx + (r * 0.32f) * std::cos(currentAngle);
        float indStartY = cy + (r * 0.32f) * std::sin(currentAngle);

        setBrushColor(D2DColor(1.0f, 1.0f, 1.0f));
        pRenderTarget->DrawLine(D2D1::Point2F(indStartX, indStartY), D2D1::Point2F(indX, indY), pSolidBrush, 2.0f, pRoundStrokeStyle);

        // 5. Parameter Title Label (above)
        setBrushColor(D2DColor(0.60f, 0.63f, 0.68f));
        drawDWriteText(knob.wTitle.c_str(), cx - 45, cy - r - 17.0f, 90, 14, pFormatKnobTitle);

        // 6. Parameter Value Readout (below)
        wchar_t valBuf[32];
        if (defs[knob.paramId].isInt) {
            swprintf(valBuf, 32, L"%d", static_cast<int>(std::round(val)));
        } else if (defs[knob.paramId].label[0] != '\0') {
            swprintf(valBuf, 32, L"%.1f %hs", val, defs[knob.paramId].label);
        } else {
            swprintf(valBuf, 32, L"%.1f", val);
        }

        setBrushColor(D2DColor(0.85f, 0.88f, 0.93f));
        drawDWriteText(valBuf, cx - 45, cy + r + 5.0f, 90, 14, pFormatKnobVal);
    }

    void drawArc(float cx, float cy, float radius, float startAngle, float sweepAngle, const D2DColor& color, float strokeWidth) {
        if (sweepAngle < 0.005f) return;

        float endAngle = startAngle + sweepAngle;
        float startX = cx + radius * std::cos(startAngle);
        float startY = cy + radius * std::sin(startAngle);
        float endX = cx + radius * std::cos(endAngle);
        float endY = cy + radius * std::sin(endAngle);

        ID2D1PathGeometry* pGeom = nullptr;
        if (SUCCEEDED(pD2DFactory->CreatePathGeometry(&pGeom))) {
            ID2D1GeometrySink* pSink = nullptr;
            if (SUCCEEDED(pGeom->Open(&pSink))) {
                pSink->BeginFigure(D2D1::Point2F(startX, startY), D2D1_FIGURE_BEGIN_HOLLOW);
                pSink->AddArc(D2D1::ArcSegment(
                    D2D1::Point2F(endX, endY),
                    D2D1::SizeF(radius, radius),
                    0.0f,
                    D2D1_SWEEP_DIRECTION_CLOCKWISE,
                    (sweepAngle > 3.14159265f) ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL
                ));
                pSink->EndFigure(D2D1_FIGURE_END_OPEN);
                pSink->Close();
                pSink->Release();

                setBrushColor(color);
                pRenderTarget->DrawGeometry(pGeom, pSolidBrush, strokeWidth, pRoundStrokeStyle);
            }
            pGeom->Release();
        }
    }

    void drawButton(const ButtonWidget& btn) {
        bool isActive = false;
        if (btn.paramId >= 0 && engine) {
            float val = engine->getParameter(btn.paramId);
            if (btn.isToggle) {
                isActive = (val > 0.5f);
            } else {
                isActive = (std::abs(val - btn.targetValue) < 0.1f);
            }
        }

        if (isActive) {
            // Active: High-contrast inverted flat white/silver with dark charcoal text
            setBrushColor(btn.activeColor);
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h), 3, 3), pSolidBrush);

            setBrushColor(D2DColor(1.0f, 1.0f, 1.0f));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h), 3, 3), pSolidBrush, 1.0f);

            setBrushColor(D2DColor(0.08f, 0.09f, 0.11f));
        } else {
            // Inactive: Flat dark charcoal card with muted silver text
            setBrushColor(D2DColor(0.12f, 0.125f, 0.14f));
            pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h), 3, 3), pSolidBrush);

            setBrushColor(D2DColor(0.20f, 0.22f, 0.25f));
            pRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h), 3, 3), pSolidBrush, 1.0f);

            setBrushColor(D2DColor(0.60f, 0.64f, 0.70f));
        }

        drawDWriteText(btn.wText.c_str(), btn.x, btn.y, btn.w, btn.h, pFormatButton);
    }

    void drawDWriteText(const wchar_t* text, float x, float y, float w, float h, IDWriteTextFormat* format, DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING) {
        if (!pRenderTarget || !format || !text) return;
        format->SetTextAlignment(align);
        D2D1_RECT_F layoutRect = D2D1::RectF(x, y, x + w, y + h);
        pRenderTarget->DrawText(text, static_cast<UINT32>(wcslen(text)), format, layoutRect, pSolidBrush);
    }

    void onMouseDown(int mx, int my, bool isDoubleClick) {
        if (!engine) return;

        // Check Preset Bar prev/next buttons
        float bx = 410, by = 10, bw = 330, bh = 28;
        if (my >= by && my <= by + bh) {
            // Prev button (<)
            if (mx >= bx && mx <= bx + 30) {
                const auto& presets = getPresets();
                currentPresetIdx = (currentPresetIdx - 1 + static_cast<int>(presets.size())) % static_cast<int>(presets.size());
                engine->loadPreset(currentPresetIdx);
                redraw();
                return;
            }
            // Next button (>)
            if (mx >= bx + bw - 30 && mx <= bx + bw) {
                const auto& presets = getPresets();
                currentPresetIdx = (currentPresetIdx + 1) % static_cast<int>(presets.size());
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

    void onMouseMove(int mx, int my, bool shiftDown) {
        (void)mx;
        if (activeKnobId >= 0 && engine) {
            float deltaY = static_cast<float>(dragStartY - my);
            const auto* defs = getParameterDefs();
            float range = defs[activeKnobId].maxVal - defs[activeKnobId].minVal;

            float sensitivity = shiftDown ? 0.001f : 0.006f;
            float newVal = dragStartVal + deltaY * range * sensitivity;
            newVal = std::clamp(newVal, defs[activeKnobId].minVal, defs[activeKnobId].maxVal);
            if (defs[activeKnobId].isInt) newVal = std::round(newVal);

            engine->setParameter(activeKnobId, newVal);
            redraw();
        }
    }

    void onMouseUp() {
        activeKnobId = -1;
    }

    void onMouseWheel(int mx, int my, int delta) {
        if (!engine) return;
        float direction = (delta > 0) ? 1.0f : -1.0f;
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
    }
};

} // namespace ThirdAss

#endif // _WIN32
