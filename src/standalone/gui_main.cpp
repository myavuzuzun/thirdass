#include <iostream>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "../dsp/ThirdBassDSP.hpp"
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
