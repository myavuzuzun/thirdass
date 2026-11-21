#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <memory>

#include "vst3/vst3_plugin.h"
#include "../dsp/ThirdAssDSP.hpp"
#include "../gui/WinD2DUI.hpp"

class ThirdAssVST3EditorWrapper : public VST3_Editor {
public:
    ThirdAssVST3EditorWrapper(VST3_EditorListener* listener, VST3_Descriptor* desc, void* parent, ThirdAss::ThirdAssEngine* eng)
        : VST3_Editor(listener, desc, parent), engine(eng) {
        gui = std::make_unique<ThirdAss::PluginUI>(eng);
        gui->openWindow(parent);
    }

    ~ThirdAssVST3EditorWrapper() override {
        if (gui) {
            gui->closeWindow();
            gui.reset();
        }
    }

    void updateParameterFromHost(uint32_t /*index*/, float /*value*/) override {
        if (gui) {
            gui->redraw();
        }
    }

    ThirdAss::PluginUI* getUI() { return gui.get(); }

private:
    ThirdAss::ThirdAssEngine* engine = nullptr;
    std::unique_ptr<ThirdAss::PluginUI> gui;
};
