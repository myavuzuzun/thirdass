#include <cstdlib>
#include <cstring>
#include <memory>
#include <X11/Xlib.h>
#include "../../include/lv2/core/lv2.h"
#include "../../include/lv2/ui/ui.h"
#include "../dsp/ThirdBassDSP.hpp"
#include "../gui/CairoUI.hpp"

#define THIRD_BASS_UI_URI "urn:antigravity:thirdbass#ui"

namespace {

class ThirdBassLV2UIInstance {
public:
    ThirdBassLV2UIInstance(const char* bundle_path,
                           LV2UI_Write_Function write_fn,
                           LV2UI_Controller ctrl,
                           LV2UI_Widget* widget,
                           const LV2_Feature* const* features)
        : writeFunction(write_fn), controller(ctrl) {

        Window parentWindow = 0;
        for (int i = 0; features && features[i]; ++i) {
            if (strcmp(features[i]->URI, LV2_UI__parent) == 0) {
                parentWindow = (Window)(uintptr_t)features[i]->data;
                break;
            }
        }

        xDisplay = XOpenDisplay(nullptr);
        if (xDisplay && parentWindow) {
            ui = std::make_unique<ThirdBass::PluginUI>(&engine);
            ui->openWindow(xDisplay, parentWindow);
            if (widget) {
                *widget = (LV2UI_Widget)(uintptr_t)ui->getWindow();
            }
        }
    }

    ~ThirdBassLV2UIInstance() {
        if (ui) {
            ui->closeWindow();
            ui.reset();
        }
        if (xDisplay) {
            XCloseDisplay(xDisplay);
            xDisplay = nullptr;
        }
    }

    void portEvent(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer) {
        if (port_index >= 4 && port_index < 4 + ThirdBass::NUM_PARAMS && buffer) {
            int paramIdx = port_index - 4;
            float val = *static_cast<const float*>(buffer);
            engine.setParameter(paramIdx, val);
            if (ui) ui->redraw();
        }
    }

    int idle() {
        if (ui && xDisplay) {
            while (XPending(xDisplay) > 0) {
                XEvent ev;
                XNextEvent(xDisplay, &ev);
                ui->handleEvent(&ev);
            }
            ui->idle();
        }
        return 0;
    }

private:
    LV2UI_Write_Function writeFunction;
    LV2UI_Controller controller;
    ThirdBass::ThirdBassEngine engine;

    Display* xDisplay = nullptr;
    std::unique_ptr<ThirdBass::PluginUI> ui;
};

LV2UI_Handle ui_instantiate(const LV2UI_Descriptor* descriptor,
                            const char* plugin_uri,
                            const char* bundle_path,
                            LV2UI_Write_Function write_function,
                            LV2UI_Controller controller,
                            LV2UI_Widget* widget,
                            const LV2_Feature* const* features) {
    return new ThirdBassLV2UIInstance(bundle_path, write_function, controller, widget, features);
}

void ui_cleanup(LV2UI_Handle handle) {
    if (handle) {
        delete static_cast<ThirdBassLV2UIInstance*>(handle);
    }
}

void ui_port_event(LV2UI_Handle handle,
                   uint32_t port_index,
                   uint32_t buffer_size,
                   uint32_t format,
                   const void* buffer) {
    if (handle) {
        static_cast<ThirdBassLV2UIInstance*>(handle)->portEvent(port_index, buffer_size, format, buffer);
    }
}

int ui_idle(LV2UI_Handle handle) {
    if (handle) {
        return static_cast<ThirdBassLV2UIInstance*>(handle)->idle();
    }
    return 0;
}

const LV2UI_Idle_Interface idle_interface = {
    ui_idle
};

const void* ui_extension_data(const char* uri) {
    if (strcmp(uri, LV2_UI__idleInterface) == 0) {
        return &idle_interface;
    }
    return nullptr;
}

const LV2UI_Descriptor ui_descriptor = {
    THIRD_BASS_UI_URI,
    ui_instantiate,
    ui_cleanup,
    ui_port_event,
    ui_extension_data
};

} // namespace

extern "C" {

#ifndef LV2_SYMBOL_EXPORT
#if defined(__GNUC__)
#define LV2_SYMBOL_EXPORT __attribute__((visibility("default")))
#else
#define LV2_SYMBOL_EXPORT
#endif
#endif

LV2_SYMBOL_EXPORT const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index) {
    return (index == 0) ? &ui_descriptor : nullptr;
}

}
