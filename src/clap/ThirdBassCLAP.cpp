#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <memory>
#include <vector>
#include <cmath>
#include <X11/Xlib.h>
#include "../../include/clap/clap.h"
#include "../../include/clap/ext/gui.h"
#include "../dsp/ThirdBassDSP.hpp"
#include "../gui/CairoUI.hpp"

namespace {

static const char* s_features[] = {
    CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
    CLAP_PLUGIN_FEATURE_DISTORTION,
    nullptr
};

static const clap_plugin_descriptor_t s_descriptor = {
    CLAP_VERSION,
    "urn:antigravity:thirdbass",
    "Third Bass",
    "Antigravity Audio",
    "https://github.com/antigravity/3rdbass",
    "",
    "",
    "1.0.0",
    "Bass Multi-Effects, Amp Simulator & Psychoacoustic Sub Enhancer",
    s_features
};

class ThirdBassCLAPInstance {
public:
    ThirdBassCLAPInstance(const clap_host_t* h) : host(h) {
        plugin.desc = &s_descriptor;
        plugin.plugin_data = this;
        plugin.init = initCallback;
        plugin.destroy = destroyCallback;
        plugin.activate = activateCallback;
        plugin.deactivate = deactivateCallback;
        plugin.start_processing = startProcessingCallback;
        plugin.stop_processing = stopProcessingCallback;
        plugin.reset = resetCallback;
        plugin.process = processCallback;
        plugin.get_extension = getExtensionCallback;
        plugin.on_main_thread = onMainThreadCallback;

        engine.setSampleRate(44100.0f);
    }

    ~ThirdBassCLAPInstance() {
        closeUI();
    }

    clap_plugin_t* getPlugin() { return &plugin; }

    static bool CLAP_ABI initCallback(const clap_plugin_t* plugin) { return true; }
    static void CLAP_ABI destroyCallback(const clap_plugin_t* plugin) {
        if (plugin && plugin->plugin_data) {
            delete static_cast<ThirdBassCLAPInstance*>(plugin->plugin_data);
        }
    }

    static bool CLAP_ABI activateCallback(const clap_plugin_t* plugin, double sample_rate, uint32_t min_frames, uint32_t max_frames) {
        if (!plugin || !plugin->plugin_data) return false;
        auto* self = static_cast<ThirdBassCLAPInstance*>(plugin->plugin_data);
        self->engine.setSampleRate(static_cast<float>(sample_rate));
        return true;
    }

    static void CLAP_ABI deactivateCallback(const clap_plugin_t* plugin) {}
    static bool CLAP_ABI startProcessingCallback(const clap_plugin_t* plugin) { return true; }
    static void CLAP_ABI stopProcessingCallback(const clap_plugin_t* plugin) {}
    static void CLAP_ABI resetCallback(const clap_plugin_t* plugin) {}

    static clap_process_status CLAP_ABI processCallback(const clap_plugin_t* plugin, const clap_process_t* process) {
        if (!plugin || !plugin->plugin_data || !process) return CLAP_PROCESS_ERROR;
        auto* self = static_cast<ThirdBassCLAPInstance*>(plugin->plugin_data);

        // Handle parameter automation events
        const clap_input_events_t* inEvents = process->in_events;
        if (inEvents) {
            uint32_t numEvents = inEvents->size(inEvents);
            for (uint32_t i = 0; i < numEvents; ++i) {
                const clap_event_header_t* header = inEvents->get(inEvents, i);
                if (header->space_id == CLAP_CORE_EVENT_SPACE_ID && header->type == CLAP_EVENT_PARAM_VALUE) {
                    const auto* paramEvent = reinterpret_cast<const clap_event_param_value_t*>(header);
                    if (paramEvent->param_id < ThirdBass::NUM_PARAMS) {
                        self->engine.setParameter(paramEvent->param_id, static_cast<float>(paramEvent->value));
                    }
                }
            }
        }

        // Process audio
        if (process->audio_inputs_count > 0 && process->audio_outputs_count > 0) {
            const float* inL = process->audio_inputs[0].data32[0];
            const float* inR = (process->audio_inputs[0].channel_count > 1) ? process->audio_inputs[0].data32[1] : inL;
            float* outL = process->audio_outputs[0].data32[0];
            float* outR = (process->audio_outputs[0].channel_count > 1) ? process->audio_outputs[0].data32[1] : outL;

            const float* inputs[2] = { inL, inR };
            float* outputs[2] = { outL, outR };

            self->engine.process(inputs, outputs, process->frames_count);
        }

        return CLAP_PROCESS_CONTINUE;
    }

    static void CLAP_ABI onMainThreadCallback(const clap_plugin_t* plugin) {
        if (!plugin || !plugin->plugin_data) return;
        auto* self = static_cast<ThirdBassCLAPInstance*>(plugin->plugin_data);
        if (self->ui && self->xDisplay) {
            while (XPending(self->xDisplay) > 0) {
                XEvent ev;
                XNextEvent(self->xDisplay, &ev);
                self->ui->handleEvent(&ev);
            }
            self->ui->idle();
        }
    }

    static const void* CLAP_ABI getExtensionCallback(const clap_plugin_t* plugin, const char* id) {
        if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &s_audioPorts;
        if (strcmp(id, CLAP_EXT_PARAMS) == 0) return &s_params;
        if (strcmp(id, CLAP_EXT_STATE) == 0) return &s_state;
        if (strcmp(id, CLAP_EXT_GUI) == 0) return &s_gui;
        return nullptr;
    }

    // Audio Ports Extension
    static uint32_t CLAP_ABI audioPortsCount(const clap_plugin_t* plugin, bool is_input) { return 1; }
    static bool CLAP_ABI audioPortsGet(const clap_plugin_t* plugin, uint32_t index, bool is_input, clap_audio_port_info_t* info) {
        if (index != 0) return false;
        info->id = 0;
        snprintf(info->name, sizeof(info->name), "%s", is_input ? "Audio In" : "Audio Out");
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->channel_count = 2;
        info->port_type = CLAP_PORT_STEREO;
        info->in_place_pair = 0;
        return true;
    }

    // Parameters Extension
    static uint32_t CLAP_ABI paramsCount(const clap_plugin_t* plugin) {
        return ThirdBass::NUM_PARAMS;
    }

    static bool CLAP_ABI paramsGetInfo(const clap_plugin_t* plugin, uint32_t param_index, clap_param_info_t* param_info) {
        if (param_index >= ThirdBass::NUM_PARAMS) return false;
        const auto* defs = ThirdBass::getParameterDefs();

        param_info->id = param_index;
        param_info->flags = CLAP_PARAM_IS_AUTOMATABLE;
        if (defs[param_index].isInt) param_info->flags |= CLAP_PARAM_IS_STEPPED;

        param_info->cookie = nullptr;
        snprintf(param_info->name, sizeof(param_info->name), "%s", defs[param_index].name);
        snprintf(param_info->module, sizeof(param_info->module), "Third Bass");

        param_info->min_value = defs[param_index].minVal;
        param_info->max_value = defs[param_index].maxVal;
        param_info->default_value = defs[param_index].defaultVal;
        return true;
    }

    static bool CLAP_ABI paramsGetValue(const clap_plugin_t* plugin, clap_id param_id, double* out_value) {
        if (!plugin || !plugin->plugin_data || param_id >= ThirdBass::NUM_PARAMS) return false;
        auto* self = static_cast<ThirdBassCLAPInstance*>(plugin->plugin_data);
        *out_value = self->engine.getParameter(param_id);
        return true;
    }

    static bool CLAP_ABI paramsValueToText(const clap_plugin_t* plugin, clap_id param_id, double value, char* out_buffer, uint32_t out_buffer_capacity) {
        if (param_id >= ThirdBass::NUM_PARAMS) return false;
        const auto* defs = ThirdBass::getParameterDefs();

        if (param_id == ThirdBass::PARAM_CAB_MODE) {
            const char* cabs[] = { "Direct / Bypass", "Classic 8x10 SVT", "Vintage 1x15 Portaflex", "Modern 4x10 Punch", "Aggressive 2x12 Rock" };
            int c = std::clamp(static_cast<int>(value), 0, 4);
            snprintf(out_buffer, out_buffer_capacity, "%s", cabs[c]);
        } else if (param_id == ThirdBass::PARAM_DRIVE_MODE) {
            const char* drives[] = { "Warm Tube", "Overdrive", "Fuzz Grit" };
            int d = std::clamp(static_cast<int>(value), 0, 2);
            snprintf(out_buffer, out_buffer_capacity, "%s", drives[d]);
        } else if (defs[param_id].isInt) {
            snprintf(out_buffer, out_buffer_capacity, "%s", (value > 0.5) ? "On" : "Off");
        } else {
            snprintf(out_buffer, out_buffer_capacity, "%.1f %s", value, defs[param_id].label);
        }
        return true;
    }

    static bool CLAP_ABI paramsTextToValue(const clap_plugin_t* plugin, clap_id param_id, const char* param_value_text, double* out_value) {
        if (param_id >= ThirdBass::NUM_PARAMS) return false;
        *out_value = atof(param_value_text);
        return true;
    }

    static void CLAP_ABI paramsFlush(const clap_plugin_t* plugin, const clap_input_events_t* in, const clap_output_events_t* out) {
        if (!plugin || !plugin->plugin_data || !in) return;
        auto* self = static_cast<ThirdBassCLAPInstance*>(plugin->plugin_data);

        uint32_t numEvents = in->size(in);
        for (uint32_t i = 0; i < numEvents; ++i) {
            const clap_event_header_t* header = in->get(in, i);
            if (header->space_id == CLAP_CORE_EVENT_SPACE_ID && header->type == CLAP_EVENT_PARAM_VALUE) {
                const auto* paramEvent = reinterpret_cast<const clap_event_param_value_t*>(header);
                if (paramEvent->param_id < ThirdBass::NUM_PARAMS) {
                    self->engine.setParameter(paramEvent->param_id, static_cast<float>(paramEvent->value));
                }
            }
        }
    }

    // State Extension
    static bool CLAP_ABI stateSave(const clap_plugin_t* plugin, const clap_ostream_t* stream) {
        if (!plugin || !plugin->plugin_data) return false;
        auto* self = static_cast<ThirdBassCLAPInstance*>(plugin->plugin_data);

        std::vector<float> values(ThirdBass::NUM_PARAMS);
        for (int i = 0; i < ThirdBass::NUM_PARAMS; ++i) {
            values[i] = self->engine.getParameter(i);
        }
        int64_t written = stream->write(stream, values.data(), values.size() * sizeof(float));
        return written == (int64_t)(values.size() * sizeof(float));
    }

    static bool CLAP_ABI stateLoad(const clap_plugin_t* plugin, const clap_istream_t* stream) {
        if (!plugin || !plugin->plugin_data) return false;
        auto* self = static_cast<ThirdBassCLAPInstance*>(plugin->plugin_data);

        std::vector<float> values(ThirdBass::NUM_PARAMS);
        int64_t read = stream->read(stream, values.data(), values.size() * sizeof(float));
        if (read == (int64_t)(values.size() * sizeof(float))) {
            for (int i = 0; i < ThirdBass::NUM_PARAMS; ++i) {
                self->engine.setParameter(i, values[i]);
            }
            if (self->ui) self->ui->redraw();
            return true;
        }
        return false;
    }

    // GUI Extension
    static bool CLAP_ABI guiIsApiSupported(const clap_plugin_t* plugin, const char* api, bool is_floating) {
        if (is_floating) return false;
        return (strcmp(api, CLAP_WINDOW_API_X11) == 0);
    }

    static bool CLAP_ABI guiGetPreferredApi(const clap_plugin_t* plugin, const char** api, bool* is_floating) {
        *api = CLAP_WINDOW_API_X11;
        *is_floating = false;
        return true;
    }

    static bool CLAP_ABI guiCreate(const clap_plugin_t* plugin, const char* api, bool is_floating) {
        return (strcmp(api, CLAP_WINDOW_API_X11) == 0 && !is_floating);
    }

    static void CLAP_ABI guiDestroy(const clap_plugin_t* plugin) {
        if (!plugin || !plugin->plugin_data) return;
        auto* self = static_cast<ThirdBassCLAPInstance*>(plugin->plugin_data);
        self->closeUI();
    }

    static bool CLAP_ABI guiSetScale(const clap_plugin_t* plugin, double scale) {
        return false;
    }

    static bool CLAP_ABI guiGetSize(const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) {
        *width = ThirdBass::PluginUI::UI_WIDTH;
        *height = ThirdBass::PluginUI::UI_HEIGHT;
        return true;
    }

    static bool CLAP_ABI guiCanResize(const clap_plugin_t* plugin) {
        return false;
    }

    static bool CLAP_ABI guiGetResizeHints(const clap_plugin_t* plugin, clap_gui_resize_hints_t* hints) {
        hints->can_resize_horizontally = false;
        hints->can_resize_vertically = false;
        return true;
    }

    static bool CLAP_ABI guiAdjustSize(const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) {
        *width = ThirdBass::PluginUI::UI_WIDTH;
        *height = ThirdBass::PluginUI::UI_HEIGHT;
        return true;
    }

    static bool CLAP_ABI guiSetSize(const clap_plugin_t* plugin, uint32_t width, uint32_t height) {
        return (width == ThirdBass::PluginUI::UI_WIDTH && height == ThirdBass::PluginUI::UI_HEIGHT);
    }

    static bool CLAP_ABI guiSetParent(const clap_plugin_t* plugin, const clap_window_t* window) {
        if (!plugin || !plugin->plugin_data || !window) return false;
        if (strcmp(window->api, CLAP_WINDOW_API_X11) != 0) return false;
        auto* self = static_cast<ThirdBassCLAPInstance*>(plugin->plugin_data);
        return self->openUI(window->x11);
    }

    static bool CLAP_ABI guiSetTransient(const clap_plugin_t* plugin, const clap_window_t* window) {
        return false;
    }

    static void CLAP_ABI guiSuggestTitle(const clap_plugin_t* plugin, const char* title) {}

    static bool CLAP_ABI guiShow(const clap_plugin_t* plugin) {
        if (!plugin || !plugin->plugin_data) return false;
        auto* self = static_cast<ThirdBassCLAPInstance*>(plugin->plugin_data);
        if (self->ui) {
            self->ui->redraw();
            return true;
        }
        return false;
    }

    static bool CLAP_ABI guiHide(const clap_plugin_t* plugin) {
        return true;
    }

    bool openUI(Window parentWin) {
        closeUI();
        xDisplay = XOpenDisplay(nullptr);
        if (xDisplay) {
            ui = std::make_unique<ThirdBass::PluginUI>(&engine);
            return ui->openWindow(xDisplay, parentWin);
        }
        return false;
    }

    void closeUI() {
        if (ui) {
            ui->closeWindow();
            ui.reset();
        }
        if (xDisplay) {
            XCloseDisplay(xDisplay);
            xDisplay = nullptr;
        }
    }

private:
    clap_plugin_t plugin;
    const clap_host_t* host;
    ThirdBass::ThirdBassEngine engine;

    Display* xDisplay = nullptr;
    std::unique_ptr<ThirdBass::PluginUI> ui;

    static const clap_plugin_audio_ports_t s_audioPorts;
    static const clap_plugin_params_t s_params;
    static const clap_plugin_state_t s_state;
    static const clap_plugin_gui_t s_gui;
};

const clap_plugin_audio_ports_t ThirdBassCLAPInstance::s_audioPorts = {
    audioPortsCount,
    audioPortsGet
};

const clap_plugin_params_t ThirdBassCLAPInstance::s_params = {
    paramsCount,
    paramsGetInfo,
    paramsGetValue,
    paramsValueToText,
    paramsTextToValue,
    paramsFlush
};

const clap_plugin_state_t ThirdBassCLAPInstance::s_state = {
    stateSave,
    stateLoad
};

const clap_plugin_gui_t ThirdBassCLAPInstance::s_gui = {
    guiIsApiSupported,
    guiGetPreferredApi,
    guiCreate,
    guiDestroy,
    guiSetScale,
    guiGetSize,
    guiCanResize,
    guiGetResizeHints,
    guiAdjustSize,
    guiSetSize,
    guiSetParent,
    guiSetTransient,
    guiSuggestTitle,
    guiShow,
    guiHide
};

// Factory
static uint32_t CLAP_ABI pluginFactoryGetPluginCount(const clap_plugin_factory_t* factory) {
    return 1;
}

static const clap_plugin_descriptor_t* CLAP_ABI pluginFactoryGetPluginDescriptor(const clap_plugin_factory_t* factory, uint32_t index) {
    return (index == 0) ? &s_descriptor : nullptr;
}

static const clap_plugin_t* CLAP_ABI pluginFactoryCreatePlugin(const clap_plugin_factory_t* factory, const clap_host_t* host, const char* plugin_id) {
    if (strcmp(plugin_id, s_descriptor.id) != 0) return nullptr;
    auto* instance = new ThirdBassCLAPInstance(host);
    return instance->getPlugin();
}

static const clap_plugin_factory_t s_pluginFactory = {
    pluginFactoryGetPluginCount,
    pluginFactoryGetPluginDescriptor,
    pluginFactoryCreatePlugin
};

static bool CLAP_ABI entryInit(const char* plugin_path) { return true; }
static void CLAP_ABI entryDeinit() {}
static const void* CLAP_ABI entryGetFactory(const char* factory_id) {
    if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) return &s_pluginFactory;
    return nullptr;
}

} // namespace

extern "C" {

#if defined(__GNUC__)
#define CLAP_EXPORT __attribute__((visibility("default")))
#else
#define CLAP_EXPORT
#endif

CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    CLAP_VERSION,
    entryInit,
    entryDeinit,
    entryGetFactory
};

}
