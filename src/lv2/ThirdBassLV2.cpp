#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
#include "../../include/lv2/core/lv2.h"
#include "../dsp/ThirdBassDSP.hpp"

#define THIRD_BASS_LV2_URI "urn:antigravity:thirdbass"

namespace {

enum PortIndex {
    PORT_AUDIO_IN_L = 0,
    PORT_AUDIO_IN_R = 1,
    PORT_AUDIO_OUT_L = 2,
    PORT_AUDIO_OUT_R = 3,
    PORT_PARAMS_START = 4
};

class ThirdBassLV2Instance {
public:
    ThirdBassLV2Instance(double sampleRate) {
        engine.setSampleRate(static_cast<float>(sampleRate));
        paramPorts.resize(ThirdBass::NUM_PARAMS, nullptr);
    }

    void connectPort(uint32_t port, void* dataLocation) {
        switch (port) {
            case PORT_AUDIO_IN_L:
                audioInL = static_cast<const float*>(dataLocation);
                break;
            case PORT_AUDIO_IN_R:
                audioInR = static_cast<const float*>(dataLocation);
                break;
            case PORT_AUDIO_OUT_L:
                audioOutL = static_cast<float*>(dataLocation);
                break;
            case PORT_AUDIO_OUT_R:
                audioOutR = static_cast<float*>(dataLocation);
                break;
            default:
                if (port >= PORT_PARAMS_START && port < PORT_PARAMS_START + ThirdBass::NUM_PARAMS) {
                    paramPorts[port - PORT_PARAMS_START] = static_cast<const float*>(dataLocation);
                }
                break;
        }
    }

    void activate() {
        // Ready for processing
    }

    void run(uint32_t sampleCount) {
        if (!audioInL || !audioOutL) return;

        // Sync parameters from control ports
        for (int i = 0; i < ThirdBass::NUM_PARAMS; ++i) {
            if (paramPorts[i] != nullptr) {
                float val = *paramPorts[i];
                if (val != engine.getParameter(i)) {
                    engine.setParameter(i, val);
                }
            }
        }

        const float* inR = audioInR ? audioInR : audioInL;
        float* outR = audioOutR ? audioOutR : audioOutL;

        const float* inputs[2] = { audioInL, inR };
        float* outputs[2] = { audioOutL, outR };

        engine.process(inputs, outputs, static_cast<int>(sampleCount));
    }

    void deactivate() {
    }

private:
    ThirdBass::ThirdBassEngine engine;
    const float* audioInL = nullptr;
    const float* audioInR = nullptr;
    float* audioOutL = nullptr;
    float* audioOutR = nullptr;
    std::vector<const float*> paramPorts;
};

LV2_Handle instantiate(const LV2_Descriptor* descriptor,
                       double sample_rate,
                       const char* bundle_path,
                       const LV2_Feature* const* features) {
    return new ThirdBassLV2Instance(sample_rate);
}

void connect_port(LV2_Handle instance, uint32_t port, void* data_location) {
    if (instance) {
        static_cast<ThirdBassLV2Instance*>(instance)->connectPort(port, data_location);
    }
}

void activate(LV2_Handle instance) {
    if (instance) {
        static_cast<ThirdBassLV2Instance*>(instance)->activate();
    }
}

void run(LV2_Handle instance, uint32_t sample_count) {
    if (instance) {
        static_cast<ThirdBassLV2Instance*>(instance)->run(sample_count);
    }
}

void deactivate(LV2_Handle instance) {
    if (instance) {
        static_cast<ThirdBassLV2Instance*>(instance)->deactivate();
    }
}

void cleanup(LV2_Handle instance) {
    if (instance) {
        delete static_cast<ThirdBassLV2Instance*>(instance);
    }
}

const void* extension_data(const char* uri) {
    return nullptr;
}

const LV2_Descriptor descriptor = {
    THIRD_BASS_LV2_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data
};

} // namespace

extern "C" {

#if defined(__GNUC__)
#define LV2_SYMBOL_EXPORT __attribute__((visibility("default")))
#else
#define LV2_SYMBOL_EXPORT
#endif

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
    return (index == 0) ? &descriptor : nullptr;
}

}
