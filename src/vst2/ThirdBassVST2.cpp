#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <memory>
#include <vector>
#include <algorithm>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../gui/WinD2DUI.hpp"
#else
#include <X11/Xlib.h>
#include "../gui/CairoUI.hpp"
#endif

#include "../../include/vst2/aeffect.h"
#include "../dsp/ThirdBassDSP.hpp"

struct ERect {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
};

class ThirdBassVST2Wrapper {
public:
    ThirdBassVST2Wrapper(audioMasterCallback master) : audioMaster(master) {
        memset(&cEffect, 0, sizeof(cEffect));
        cEffect.magic = kEffectMagic;
        cEffect.dispatcher = dispatcherCallback;
        cEffect.process = nullptr;
        cEffect.processReplacing = processReplacingCallback;
        cEffect.processDoubleReplacing = processDoubleReplacingCallback;
        cEffect.setParameter = setParameterCallback;
        cEffect.getParameter = getParameterCallback;
        cEffect.numPrograms = static_cast<int32_t>(ThirdBass::getPresets().size());
        cEffect.numParams = ThirdBass::NUM_PARAMS;
        cEffect.numInputs = 2;
        cEffect.numOutputs = 2;
        cEffect.flags = effFlagsHasEditor | effFlagsCanReplacing | effFlagsCanDoubleReplacing | effFlagsIsSynth;
        cEffect.uniqueID = 0x33415353; // '3ASS'
        cEffect.version = 1000;
        cEffect.object = this;

        editorRect.top = 0;
        editorRect.left = 0;
        editorRect.bottom = ThirdBass::PluginUI::UI_HEIGHT;
        editorRect.right = ThirdBass::PluginUI::UI_WIDTH;

        currentProgram = 0;
        engine.setSampleRate(44100.0f);
    }

    ~ThirdBassVST2Wrapper() {
        closeEditor();
    }

    AEffect* getAEffect() { return &cEffect; }

    static intptr_t dispatcherCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) {
        if (!effect || !effect->object) return 0;
        auto* self = static_cast<ThirdBassVST2Wrapper*>(effect->object);
        return self->dispatcher(opcode, index, value, ptr, opt);
    }

    static void processReplacingCallback(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames) {
        if (!effect || !effect->object) return;
        auto* self = static_cast<ThirdBassVST2Wrapper*>(effect->object);
        self->engine.process(inputs, outputs, sampleFrames);
    }

    static void processDoubleReplacingCallback(AEffect* effect, double** inputs, double** outputs, int32_t sampleFrames) {
        if (!effect || !effect->object) return;
        auto* self = static_cast<ThirdBassVST2Wrapper*>(effect->object);
        
        thread_local std::vector<float> inBufL, inBufR, outBufL, outBufR;
        if ((int)inBufL.size() < sampleFrames) {
            inBufL.resize(sampleFrames);
            inBufR.resize(sampleFrames);
            outBufL.resize(sampleFrames);
            outBufR.resize(sampleFrames);
        }

        for (int i = 0; i < sampleFrames; ++i) {
            inBufL[i] = static_cast<float>(inputs[0][i]);
            inBufR[i] = (inputs[1] != nullptr) ? static_cast<float>(inputs[1][i]) : inBufL[i];
        }

        const float* inPtrs[2] = { inBufL.data(), inBufR.data() };
        float* outPtrs[2] = { outBufL.data(), outBufR.data() };

        self->engine.process(inPtrs, outPtrs, sampleFrames);

        for (int i = 0; i < sampleFrames; ++i) {
            outputs[0][i] = static_cast<double>(outBufL[i]);
            if (outputs[1]) outputs[1][i] = static_cast<double>(outBufR[i]);
        }
    }

    static void setParameterCallback(AEffect* effect, int32_t index, float parameter) {
        if (!effect || !effect->object) return;
        auto* self = static_cast<ThirdBassVST2Wrapper*>(effect->object);
        if (index < 0 || index >= ThirdBass::NUM_PARAMS) return;

        const auto* defs = ThirdBass::getParameterDefs();
        float denorm = defs[index].minVal + parameter * (defs[index].maxVal - defs[index].minVal);
        self->engine.setParameter(index, denorm);

        if (self->ui) {
            self->ui->redraw();
        }
    }

    static float getParameterCallback(AEffect* effect, int32_t index) {
        if (!effect || !effect->object) return 0.0f;
        auto* self = static_cast<ThirdBassVST2Wrapper*>(effect->object);
        if (index < 0 || index >= ThirdBass::NUM_PARAMS) return 0.0f;

        const auto* defs = ThirdBass::getParameterDefs();
        float val = self->engine.getParameter(index);
        float range = defs[index].maxVal - defs[index].minVal;
        return (range > 0.0f) ? ((val - defs[index].minVal) / range) : 0.0f;
    }

    intptr_t dispatcher(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) {
        const auto* defs = ThirdBass::getParameterDefs();
        const auto& presets = ThirdBass::getPresets();

        switch (opcode) {
            case effOpen:
                return 0;

            case effClose:
                delete this;
                return 0;

            case effEditGetRect:
                if (ptr) {
                    *reinterpret_cast<ERect**>(ptr) = &editorRect;
                    return 1;
                }
                return 0;

            case effEditOpen:
                if (ptr) {
#if defined(_WIN32)
                    openEditor(ptr);
#else
                    Window parentWin = (Window)(uintptr_t)ptr;
                    openEditor(parentWin);
#endif
                    return 1;
                }
                return 0;

            case effEditClose:
                closeEditor();
                return 1;

            case 19: // effEditIdle
                if (ui) {
                    ui->idle();
                }
                return 1;

            case effSetProgram:
                if (value >= 0 && value < (intptr_t)presets.size()) {
                    currentProgram = static_cast<int32_t>(value);
                    engine.loadPreset(currentProgram);
                    if (ui) ui->redraw();
                }
                return 0;

            case effGetProgram:
                return currentProgram;

            case effSetProgramName:
                return 0;

            case effGetProgramName:
                if (ptr && currentProgram >= 0 && currentProgram < (int32_t)presets.size()) {
                    strncpy((char*)ptr, presets[currentProgram].name, 24);
                    ((char*)ptr)[24] = '\0';
                }
                return 1;

            case effGetProgramNameIndexed:
                if (ptr && index >= 0 && index < (int32_t)presets.size()) {
                    strncpy((char*)ptr, presets[index].name, 24);
                    ((char*)ptr)[24] = '\0';
                    return 1;
                }
                return 0;

            case effGetParamName:
                if (ptr && index >= 0 && index < ThirdBass::NUM_PARAMS) {
                    strncpy((char*)ptr, defs[index].name, 24);
                    ((char*)ptr)[24] = '\0';
                    return 1;
                }
                return 0;

            case effGetParamLabel:
                if (ptr && index >= 0 && index < ThirdBass::NUM_PARAMS) {
                    strncpy((char*)ptr, defs[index].label, 8);
                    ((char*)ptr)[8] = '\0';
                    return 1;
                }
                return 0;

            case effGetParamDisplay:
                if (ptr && index >= 0 && index < ThirdBass::NUM_PARAMS) {
                    float val = engine.getParameter(index);
                    if (index == ThirdBass::PARAM_CAB_MODE) {
                        const char* cabs[] = { "Direct", "8x10 SVT", "1x15 Port", "4x10 Punch", "2x12 Rock" };
                        int c = std::clamp(static_cast<int>(val), 0, 4);
                        snprintf((char*)ptr, 24, "%s", cabs[c]);
                    } else if (index == ThirdBass::PARAM_DRIVE_MODE) {
                        const char* drives[] = { "Warm Tube", "Overdrive", "Fuzz Grit" };
                        int d = std::clamp(static_cast<int>(val), 0, 2);
                        snprintf((char*)ptr, 24, "%s", drives[d]);
                    } else if (defs[index].isInt) {
                        snprintf((char*)ptr, 24, "%s", (val > 0.5f) ? "On" : "Off");
                    } else {
                        snprintf((char*)ptr, 24, "%.1f", val);
                    }
                    return 1;
                }
                return 0;

            case effSetSampleRate:
                engine.setSampleRate(opt);
                return 0;

            case effSetBlockSize:
                return 0;

            case effGetEffectName:
                if (ptr) {
                    strncpy((char*)ptr, "Third Ass", 32);
                    ((char*)ptr)[31] = '\0';
                    return 1;
                }
                return 0;

            case effGetVendorString:
                if (ptr) {
                    strncpy((char*)ptr, "myavuzuzun", 32);
                    ((char*)ptr)[31] = '\0';
                    return 1;
                }
                return 0;

            case effGetProductString:
                if (ptr) {
                    strncpy((char*)ptr, "Third Ass", 32);
                    ((char*)ptr)[31] = '\0';
                    return 1;
                }
                return 0;

            case effGetVendorVersion:
                return 1000;

            case 25: // effProcessEvents
                return 1;

            case effCanDo:
                if (ptr) {
                    if (strcmp((char*)ptr, "receiveVstEvents") == 0) return 1;
                    if (strcmp((char*)ptr, "receiveVstMidiEvent") == 0) return 1;
                    if (strcmp((char*)ptr, "sendVstEvents") == 0) return -1;
                    if (strcmp((char*)ptr, "1in1out") == 0) return 1;
                    if (strcmp((char*)ptr, "2in2out") == 0) return 1;
                }
                return 0;

            case effGetVstVersion:
                return 2400;

            default:
                return 0;
        }
    }

#if defined(_WIN32)
    void openEditor(void* parentWin) {
        closeEditor();
        ui = std::make_unique<ThirdBass::PluginUI>(&engine);
        ui->openWindow(parentWin);
    }
#else
    void openEditor(Window parentWin) {
        closeEditor();
        ui = std::make_unique<ThirdBass::PluginUI>(&engine);
        ui->openWindow(nullptr, parentWin);
    }
#endif

    void closeEditor() {
        if (ui) {
            ui->closeWindow();
            ui.reset();
        }
#if !defined(_WIN32)
        xDisplay = nullptr;
#endif
    }

private:
    AEffect cEffect;
    [[maybe_unused]] audioMasterCallback audioMaster;
    ThirdBass::ThirdBassEngine engine;
    int32_t currentProgram;
    ERect editorRect;

#if !defined(_WIN32)
    Display* xDisplay = nullptr;
#endif
    std::unique_ptr<ThirdBass::PluginUI> ui;
};

extern "C" {

#if defined(_WIN32)
#define VST_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define VST_EXPORT __attribute__((visibility("default")))
#else
#define VST_EXPORT
#endif

VST_EXPORT AEffect* VSTPluginMain(audioMasterCallback audioMaster) {
    if (!audioMaster) return nullptr;
    auto* wrapper = new ThirdBassVST2Wrapper(audioMaster);
    return wrapper->getAEffect();
}

#if !defined(_WIN32)
VST_EXPORT AEffect* main_vst(audioMasterCallback audioMaster) asm("main");
VST_EXPORT AEffect* main_vst(audioMasterCallback audioMaster) {
    return VSTPluginMain(audioMaster);
}
#endif

}
