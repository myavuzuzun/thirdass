#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

#include "vst3/vst3_plugin.h"
#include "../dsp/ThirdAssDSP.hpp"
#include "../gui/WinD2DUI.hpp"
#include "ThirdAssVST3Editor.hpp"

class ThirdAssVST3Parameter : public VST3_Parameter {
public:
    ThirdAssVST3Parameter(int id, const char* nameStr, float defVal)
        : VST3_Parameter(nullptr, defVal), paramId(id) {
        std::memset(paramName, 0, sizeof(paramName));
        std::strncpy(paramName, nameStr, sizeof(paramName) - 1);
        name = paramName;
        short_name = paramName;
    }

    const char* displayText(float val, char* buf) override {
        const auto* defs = ThirdAss::getParameterDefs();
        float plain = defs[paramId].minVal + val * (defs[paramId].maxVal - defs[paramId].minVal);
        if (paramId == ThirdAss::PARAM_CAB_MODE) {
            const char* cabs[] = { "Direct", "8x10 SVT", "1x15 Port", "4x10 Punch", "2x12 Rock" };
            int c = std::clamp(static_cast<int>(plain), 0, 4);
            std::snprintf(buf, 64, "%s", cabs[c]);
        } else if (paramId == ThirdAss::PARAM_DRIVE_MODE) {
            const char* drives[] = { "Warm Tube", "Overdrive", "Fuzz Grit" };
            int d = std::clamp(static_cast<int>(plain), 0, 2);
            std::snprintf(buf, 64, "%s", drives[d]);
        } else if (defs[paramId].isInt) {
            std::snprintf(buf, 64, "%s", (plain > 0.5f) ? "On" : "Off");
        } else {
            std::snprintf(buf, 64, "%.1f %s", plain, defs[paramId].label);
        }
        return buf;
    }

private:
    int paramId;
    char paramName[64];
};

class ThirdAssVST3Descriptor : public VST3_Descriptor {
public:
    ThirdAssVST3Descriptor() {
        name = "Third Ass";
        author = "myavuzuzun";
        url = "https://github.com/myavuzuzun";
        email = "memtyovz@gmail.com";
        version_text = "1.2.0";
        version = 1200;
        is_synth = false;
        has_editor = true;
        can_send_midi = false;
        can_receive_midi = true;
        editor_width = ThirdAss::PluginUI::UI_WIDTH;
        editor_height = ThirdAss::PluginUI::UI_HEIGHT;
        num_inputs = 2;
        num_outputs = 2;

        static const uint8_t p_id[16] = { 0x33, 0x41, 0x53, 0x53, 0x00, 0x01, 0x4D, 0x59, 0x55, 0x5A, 0x55, 0x4E, 0x56, 0x53, 0x54, 0x33 };
        static const uint8_t e_id[16] = { 0x33, 0x41, 0x53, 0x53, 0x00, 0x02, 0x4D, 0x59, 0x55, 0x5A, 0x55, 0x4E, 0x45, 0x44, 0x49, 0x54 };
        std::memcpy(const_cast<uint8_t*>(plugin_id), p_id, 16);
        std::memcpy(const_cast<uint8_t*>(editor_id), e_id, 16);

        const auto* defs = ThirdAss::getParameterDefs();
        for (int i = 0; i < ThirdAss::NUM_PARAMS; ++i) {
            float range = defs[i].maxVal - defs[i].minVal;
            float normDef = (range > 0.0f) ? (defs[i].defaultVal - defs[i].minVal) / range : 0.0f;
            appendParameter(new ThirdAssVST3Parameter(i, defs[i].name, normDef));
        }
    }
};

class ThirdAssVST3Instance : public VST3_Instance {
public:
    ThirdAssVST3Instance() = default;
    ~ThirdAssVST3Instance() override = default;

    void on_prepare(float sampleRate) override {
        engine.setSampleRate(sampleRate);
    }

    void on_parameter(uint32_t index, float value, uint32_t /*mode*/) override {
        if (index < ThirdAss::NUM_PARAMS) {
            const auto* defs = ThirdAss::getParameterDefs();
            float denorm = defs[index].minVal + value * (defs[index].maxVal - defs[index].minVal);
            engine.setParameter(index, denorm);
        }
    }

    void on_process(VST3_ProcessContext* ctx) override {
        if (ctx->num_samples > 0 && ctx->inputs[0] && ctx->outputs[0]) {
            float* inR = ctx->inputs[1] ? ctx->inputs[1] : ctx->inputs[0];
            float* ins[2] = { ctx->inputs[0], inR };
            float* outR = ctx->outputs[1] ? ctx->outputs[1] : ctx->outputs[0];
            float* outs[2] = { ctx->outputs[0], outR };
            engine.process(ins, outs, ctx->num_samples);
        } else if (ctx->num_samples > 0) {
            if (ctx->outputs[0]) std::memset(ctx->outputs[0], 0, sizeof(float) * ctx->num_samples);
            if (ctx->outputs[1]) std::memset(ctx->outputs[1], 0, sizeof(float) * ctx->num_samples);
        }
    }

    VST3_Editor* on_openEditor(void* parent) override {
        return new ThirdAssVST3EditorWrapper(this, MDescriptor, parent, &engine);
    }

    void on_closeEditor(VST3_Editor* editor) override {
        delete editor;
    }

private:
    ThirdAss::ThirdAssEngine engine;
};

VST3_ENTRYPOINT(ThirdAssVST3Descriptor, ThirdAssVST3Instance)
