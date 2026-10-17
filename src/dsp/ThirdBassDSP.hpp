#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "Biquad.hpp"

namespace ThirdBass {

enum ParameterID {
    PARAM_IN_GAIN = 0,
    PARAM_GATE_THRESH,
    PARAM_GATE_RELEASE,
    
    PARAM_DRIVE_ENABLE,
    PARAM_DRIVE_MODE,
    PARAM_DRIVE_AMOUNT,
    PARAM_DRIVE_TONE,
    PARAM_DRIVE_MIX,
    
    PARAM_EQ_ENABLE,
    PARAM_EQ_BASS,
    PARAM_EQ_LOW_MID,
    PARAM_EQ_HIGH_MID,
    PARAM_EQ_TREBLE,
    
    PARAM_SUB_ENABLE,
    PARAM_SUB_FREQ,
    PARAM_SUB_HARMONICS,
    PARAM_SUB_LEVEL,
    
    PARAM_COMP_ENABLE,
    PARAM_COMP_THRESH,
    PARAM_COMP_RATIO,
    PARAM_COMP_ATTACK,
    PARAM_COMP_RELEASE,
    PARAM_COMP_MAKEUP,
    
    PARAM_CHORUS_ENABLE,
    PARAM_CHORUS_RATE,
    PARAM_CHORUS_DEPTH,
    PARAM_CHORUS_CROSSOVER,
    PARAM_CHORUS_MIX,
    
    PARAM_DELAY_ENABLE,
    PARAM_DELAY_TIME,
    PARAM_DELAY_FEEDBACK,
    PARAM_DELAY_DAMP,
    PARAM_DELAY_MIX,
    
    PARAM_CAB_MODE,
    PARAM_LIMITER_ENABLE,
    PARAM_OUT_GAIN,
    PARAM_DRY_WET,
    
    NUM_PARAMS
};

struct ParameterDef {
    const char* id;
    const char* name;
    const char* label;
    float minVal;
    float maxVal;
    float defaultVal;
    float step;
    bool isInt;
};

inline const ParameterDef* getParameterDefs() {
    static const ParameterDef defs[NUM_PARAMS] = {
        {"in_gain",         "Input Gain",      "dB", -24.0f,  24.0f,   0.0f, 0.1f,  false},
        {"gate_thresh",     "Gate Threshold",  "dB", -90.0f,   0.0f, -60.0f, 0.5f,  false},
        {"gate_release",    "Gate Release",    "ms",  10.0f, 500.0f, 100.0f, 1.0f,  false},
        
        {"drive_enable",    "Drive Enable",    "",      0.0f,   1.0f,   1.0f, 1.0f,  true},
        {"drive_mode",      "Drive Mode",      "",      0.0f,   2.0f,   0.0f, 1.0f,  true},
        {"drive_amount",    "Drive Amount",    "%",     0.0f, 100.0f,  25.0f, 0.5f,  false},
        {"drive_tone",      "Drive Tone",      "%",  -100.0f, 100.0f,   0.0f, 1.0f,  false},
        {"drive_mix",       "Drive Mix",       "%",     0.0f, 100.0f,  70.0f, 0.5f,  false},
        
        {"eq_enable",       "EQ Enable",       "",      0.0f,   1.0f,   1.0f, 1.0f,  true},
        {"eq_bass",         "Bass (80Hz)",     "dB", -15.0f,  15.0f,   2.0f, 0.1f,  false},
        {"eq_low_mid",      "Low Mid (350Hz)", "dB", -15.0f,  15.0f,   1.0f, 0.1f,  false},
        {"eq_high_mid",     "Hi Mid (1.8kHz)", "dB", -15.0f,  15.0f,   0.0f, 0.1f,  false},
        {"eq_treble",       "Treble (4.5kHz)", "dB", -15.0f,  15.0f,   1.0f, 0.1f,  false},
        
        {"sub_enable",      "Sub Enhancer",    "",      0.0f,   1.0f,   1.0f, 1.0f,  true},
        {"sub_freq",        "Sub Frequency",   "Hz",   40.0f, 120.0f,  65.0f, 1.0f,  false},
        {"sub_harmonics",   "Sub Harmonics",   "%",     0.0f, 100.0f,  40.0f, 0.5f,  false},
        {"sub_level",       "Sub Boost",       "%",     0.0f, 100.0f,  30.0f, 0.5f,  false},
        
        {"comp_enable",     "Comp Enable",     "",      0.0f,   1.0f,   1.0f, 1.0f,  true},
        {"comp_thresh",     "Comp Threshold",  "dB", -40.0f,   0.0f, -18.0f, 0.5f,  false},
        {"comp_ratio",      "Comp Ratio",      ":1",    1.0f,  12.0f,   4.0f, 0.1f,  false},
        {"comp_attack",     "Comp Attack",     "ms",    1.0f, 100.0f,  15.0f, 0.5f,  false},
        {"comp_release",    "Comp Release",    "ms",   20.0f,1000.0f, 150.0f, 1.0f,  false},
        {"comp_makeup",     "Comp Makeup",     "dB",    0.0f,  24.0f,   3.0f, 0.1f,  false},
        
        {"chorus_enable",   "Chorus Enable",   "",      0.0f,   1.0f,   0.0f, 1.0f,  true},
        {"chorus_rate",     "Chorus Rate",     "Hz",    0.1f,   5.0f,   0.8f, 0.05f, false},
        {"chorus_depth",    "Chorus Depth",    "%",     0.0f, 100.0f,  50.0f, 0.5f,  false},
        {"chorus_crossover","Chorus Crossover","Hz",   80.0f, 400.0f, 180.0f, 1.0f,  false},
        {"chorus_mix",      "Chorus Mix",      "%",     0.0f, 100.0f,  40.0f, 0.5f,  false},
        
        {"delay_enable",    "Delay Enable",    "",      0.0f,   1.0f,   0.0f, 1.0f,  true},
        {"delay_time",      "Delay Time",      "ms",   20.0f,1000.0f, 350.0f, 1.0f,  false},
        {"delay_feedback",  "Delay Feedback",  "%",     0.0f,  90.0f,  35.0f, 0.5f,  false},
        {"delay_damp",      "Delay Damping",   "Hz", 1000.0f,16000.0f, 4000.0f, 10.0f, false},
        {"delay_mix",       "Delay Mix",       "%",     0.0f, 100.0f,  25.0f, 0.5f,  false},
        
        {"cab_mode",        "Cabinet Model",   "",      0.0f,   4.0f,   1.0f, 1.0f,  true},
        {"limiter_enable",  "Peak Limiter",    "",      0.0f,   1.0f,   1.0f, 1.0f,  true},
        {"out_gain",        "Output Gain",     "dB", -24.0f,  12.0f,   0.0f, 0.1f,  false},
        {"dry_wet",         "Mix (Dry/Wet)",   "%",     0.0f, 100.0f, 100.0f, 0.5f,  false}
    };
    return defs;
}

struct Preset {
    const char* name;
    float params[NUM_PARAMS];
};

inline const std::vector<Preset>& getPresets() {
    static std::vector<Preset> presets;
    if (presets.empty()) {
        const ParameterDef* defs = getParameterDefs();
        
        auto createPreset = [&](const char* name) -> Preset {
            Preset p;
            p.name = name;
            for (int i = 0; i < NUM_PARAMS; ++i) {
                p.params[i] = defs[i].defaultVal;
            }
            return p;
        };

        // 1. Clean Studio DI
        {
            Preset p = createPreset("01: Clean Studio DI");
            p.params[PARAM_DRIVE_ENABLE] = 0.0f;
            p.params[PARAM_CAB_MODE] = 0.0f; // Direct
            p.params[PARAM_EQ_BASS] = 1.0f;
            p.params[PARAM_EQ_LOW_MID] = -1.0f;
            p.params[PARAM_EQ_TREBLE] = 1.5f;
            p.params[PARAM_COMP_THRESH] = -16.0f;
            p.params[PARAM_COMP_RATIO] = 3.0f;
            presets.push_back(p);
        }

        // 2. Vintage Tube Warmth
        {
            Preset p = createPreset("02: Vintage Tube Warmth");
            p.params[PARAM_DRIVE_ENABLE] = 1.0f;
            p.params[PARAM_DRIVE_MODE] = 0.0f; // Warm Tube
            p.params[PARAM_DRIVE_AMOUNT] = 35.0f;
            p.params[PARAM_CAB_MODE] = 2.0f; // 1x15 Portaflex
            p.params[PARAM_EQ_BASS] = 3.0f;
            p.params[PARAM_EQ_LOW_MID] = 2.0f;
            p.params[PARAM_SUB_ENABLE] = 1.0f;
            p.params[PARAM_SUB_HARMONICS] = 30.0f;
            presets.push_back(p);
        }

        // 3. 8x10 Rock Growl
        {
            Preset p = createPreset("03: 8x10 Rock Growl");
            p.params[PARAM_DRIVE_ENABLE] = 1.0f;
            p.params[PARAM_DRIVE_MODE] = 1.0f; // Overdrive
            p.params[PARAM_DRIVE_AMOUNT] = 45.0f;
            p.params[PARAM_DRIVE_TONE] = 20.0f;
            p.params[PARAM_CAB_MODE] = 1.0f; // 8x10 Fridge
            p.params[PARAM_EQ_BASS] = 2.0f;
            p.params[PARAM_EQ_HIGH_MID] = 3.5f;
            p.params[PARAM_COMP_THRESH] = -20.0f;
            presets.push_back(p);
        }

        // 4. Funky Slap & Pop
        {
            Preset p = createPreset("04: Funky Slap & Pop");
            p.params[PARAM_DRIVE_ENABLE] = 0.0f;
            p.params[PARAM_CAB_MODE] = 3.0f; // 4x10 Punch
            p.params[PARAM_EQ_BASS] = 4.0f;
            p.params[PARAM_EQ_LOW_MID] = -3.0f;
            p.params[PARAM_EQ_TREBLE] = 4.5f;
            p.params[PARAM_COMP_THRESH] = -22.0f;
            p.params[PARAM_COMP_RATIO] = 5.0f;
            p.params[PARAM_COMP_ATTACK] = 5.0f;
            p.params[PARAM_CHORUS_ENABLE] = 1.0f;
            p.params[PARAM_CHORUS_MIX] = 25.0f;
            presets.push_back(p);
        }

        // 5. Sub Heavy Dub
        {
            Preset p = createPreset("05: Sub Heavy Dub");
            p.params[PARAM_DRIVE_ENABLE] = 0.0f;
            p.params[PARAM_CAB_MODE] = 2.0f; // 1x15
            p.params[PARAM_EQ_BASS] = 6.0f;
            p.params[PARAM_EQ_HIGH_MID] = -4.0f;
            p.params[PARAM_EQ_TREBLE] = -6.0f;
            p.params[PARAM_SUB_ENABLE] = 1.0f;
            p.params[PARAM_SUB_LEVEL] = 75.0f;
            p.params[PARAM_SUB_HARMONICS] = 60.0f;
            p.params[PARAM_COMP_THRESH] = -24.0f;
            presets.push_back(p);
        }

        // 6. Motown 1x15 Soul
        {
            Preset p = createPreset("06: Motown 1x15 Soul");
            p.params[PARAM_DRIVE_ENABLE] = 1.0f;
            p.params[PARAM_DRIVE_MODE] = 0.0f;
            p.params[PARAM_DRIVE_AMOUNT] = 15.0f;
            p.params[PARAM_CAB_MODE] = 2.0f;
            p.params[PARAM_EQ_BASS] = 3.5f;
            p.params[PARAM_EQ_LOW_MID] = 2.5f;
            p.params[PARAM_EQ_TREBLE] = -3.0f;
            presets.push_back(p);
        }

        // 7. Modern Punch & Snap
        {
            Preset p = createPreset("07: Modern Punch & Snap");
            p.params[PARAM_DRIVE_ENABLE] = 1.0f;
            p.params[PARAM_DRIVE_MODE] = 0.0f;
            p.params[PARAM_DRIVE_AMOUNT] = 20.0f;
            p.params[PARAM_CAB_MODE] = 3.0f;
            p.params[PARAM_EQ_BASS] = 3.0f;
            p.params[PARAM_EQ_HIGH_MID] = 2.5f;
            p.params[PARAM_EQ_TREBLE] = 2.0f;
            p.params[PARAM_SUB_ENABLE] = 1.0f;
            p.params[PARAM_SUB_LEVEL] = 35.0f;
            presets.push_back(p);
        }

        // 8. Gritty Overdrive
        {
            Preset p = createPreset("08: Gritty Overdrive");
            p.params[PARAM_DRIVE_ENABLE] = 1.0f;
            p.params[PARAM_DRIVE_MODE] = 1.0f;
            p.params[PARAM_DRIVE_AMOUNT] = 65.0f;
            p.params[PARAM_DRIVE_TONE] = 15.0f;
            p.params[PARAM_DRIVE_MIX] = 85.0f;
            p.params[PARAM_CAB_MODE] = 1.0f;
            presets.push_back(p);
        }

        // 9. Fuzz Bass Monster
        {
            Preset p = createPreset("09: Fuzz Bass Monster");
            p.params[PARAM_DRIVE_ENABLE] = 1.0f;
            p.params[PARAM_DRIVE_MODE] = 2.0f; // Fuzz
            p.params[PARAM_DRIVE_AMOUNT] = 75.0f;
            p.params[PARAM_DRIVE_TONE] = 10.0f;
            p.params[PARAM_DRIVE_MIX] = 80.0f;
            p.params[PARAM_CAB_MODE] = 4.0f; // 2x12
            p.params[PARAM_EQ_BASS] = 4.0f;
            presets.push_back(p);
        }

        // 10. 80s Chorus Bass
        {
            Preset p = createPreset("10: 80s Chorus Bass");
            p.params[PARAM_DRIVE_ENABLE] = 0.0f;
            p.params[PARAM_CAB_MODE] = 3.0f;
            p.params[PARAM_CHORUS_ENABLE] = 1.0f;
            p.params[PARAM_CHORUS_RATE] = 1.2f;
            p.params[PARAM_CHORUS_DEPTH] = 70.0f;
            p.params[PARAM_CHORUS_MIX] = 55.0f;
            p.params[PARAM_EQ_TREBLE] = 3.0f;
            presets.push_back(p);
        }

        // 11. Ambient Space Bass
        {
            Preset p = createPreset("11: Ambient Space Bass");
            p.params[PARAM_DRIVE_ENABLE] = 0.0f;
            p.params[PARAM_CAB_MODE] = 0.0f;
            p.params[PARAM_CHORUS_ENABLE] = 1.0f;
            p.params[PARAM_CHORUS_RATE] = 0.6f;
            p.params[PARAM_CHORUS_DEPTH] = 80.0f;
            p.params[PARAM_CHORUS_MIX] = 45.0f;
            p.params[PARAM_DELAY_ENABLE] = 1.0f;
            p.params[PARAM_DELAY_TIME] = 450.0f;
            p.params[PARAM_DELAY_FEEDBACK] = 55.0f;
            p.params[PARAM_DELAY_MIX] = 40.0f;
            presets.push_back(p);
        }

        // 12. Tight Metal Pick
        {
            Preset p = createPreset("12: Tight Metal Pick");
            p.params[PARAM_GATE_THRESH] = -50.0f;
            p.params[PARAM_GATE_RELEASE] = 50.0f;
            p.params[PARAM_DRIVE_ENABLE] = 1.0f;
            p.params[PARAM_DRIVE_MODE] = 1.0f;
            p.params[PARAM_DRIVE_AMOUNT] = 55.0f;
            p.params[PARAM_DRIVE_TONE] = 35.0f;
            p.params[PARAM_CAB_MODE] = 4.0f; // 2x12 Rock
            p.params[PARAM_EQ_BASS] = 2.0f;
            p.params[PARAM_EQ_LOW_MID] = -2.0f;
            p.params[PARAM_EQ_HIGH_MID] = 4.0f;
            p.params[PARAM_EQ_TREBLE] = 2.5f;
            p.params[PARAM_COMP_THRESH] = -22.0f;
            p.params[PARAM_COMP_RATIO] = 6.0f;
            presets.push_back(p);
        }
    }
    return presets;
}

class ThirdBassEngine {
public:
    ThirdBassEngine() {
        params.resize(NUM_PARAMS);
        const ParameterDef* defs = getParameterDefs();
        for (int i = 0; i < NUM_PARAMS; ++i) {
            params[i] = defs[i].defaultVal;
        }
        setSampleRate(44100.0f);
    }

    void setSampleRate(float sr) {
        if (sr <= 0.0f) sr = 44100.0f;
        sampleRate = sr;

        // Initialize delay buffer: 2 seconds
        int maxDelaySamples = static_cast<int>(sampleRate * 2.1f);
        delayBufferL.assign(maxDelaySamples, 0.0f);
        delayBufferR.assign(maxDelaySamples, 0.0f);
        delayWritePos = 0;

        // Initialize chorus buffer: 50 ms
        int maxChorusSamples = static_cast<int>(sampleRate * 0.06f) + 16;
        chorusBufferL.assign(maxChorusSamples, 0.0f);
        chorusBufferR.assign(maxChorusSamples, 0.0f);
        chorusWritePos = 0;
        chorusPhase = 0.0f;

        gateEnv = 0.0f;
        compEnv = 0.0f;
        driveDcBlockL = 0.0f;
        driveDcBlockR = 0.0f;
        delayDampStateL = 0.0f;
        delayDampStateR = 0.0f;

        updateFilterCoeffs();
    }

    void setParameter(int index, float val) {
        if (index < 0 || index >= NUM_PARAMS) return;
        const ParameterDef* defs = getParameterDefs();
        val = std::clamp(val, defs[index].minVal, defs[index].maxVal);
        if (defs[index].isInt) val = std::round(val);
        params[index] = val;
        dirtyFilters = true;
    }

    float getParameter(int index) const {
        if (index < 0 || index >= NUM_PARAMS) return 0.0f;
        return params[index];
    }

    void loadPreset(int presetIdx) {
        const auto& presets = getPresets();
        if (presetIdx >= 0 && presetIdx < (int)presets.size()) {
            for (int i = 0; i < NUM_PARAMS; ++i) {
                params[i] = presets[presetIdx].params[i];
            }
            dirtyFilters = true;
        }
    }

    void process(const float* const* inputs, float* const* outputs, int numFrames) {
        if (dirtyFilters) {
            updateFilterCoeffs();
            dirtyFilters = false;
        }

        const float* inL = inputs[0];
        const float* inR = (inputs[1] != nullptr) ? inputs[1] : inputs[0];
        float* outL = outputs[0];
        float* outR = (outputs[1] != nullptr) ? outputs[1] : outputs[0];

        // Cache parameters to local floats for speed
        float inGainLin = std::pow(10.0f, params[PARAM_IN_GAIN] / 20.0f);
        float gateThreshLin = std::pow(10.0f, params[PARAM_GATE_THRESH] / 20.0f);
        float gateRelCoeff = std::exp(-1.0f / (sampleRate * (params[PARAM_GATE_RELEASE] * 0.001f)));

        bool driveOn = params[PARAM_DRIVE_ENABLE] > 0.5f;
        int driveMode = static_cast<int>(params[PARAM_DRIVE_MODE]);
        float driveAmt = params[PARAM_DRIVE_AMOUNT] * 0.01f;
        float driveMix = params[PARAM_DRIVE_MIX] * 0.01f;

        bool eqOn = params[PARAM_EQ_ENABLE] > 0.5f;

        bool subOn = params[PARAM_SUB_ENABLE] > 0.5f;
        float subHarmAmt = params[PARAM_SUB_HARMONICS] * 0.01f;
        float subLevelAmt = params[PARAM_SUB_LEVEL] * 0.01f;

        bool compOn = params[PARAM_COMP_ENABLE] > 0.5f;
        float compThreshDb = params[PARAM_COMP_THRESH];
        float compRatio = params[PARAM_COMP_RATIO];
        float compAttCoeff = std::exp(-1.0f / (sampleRate * (params[PARAM_COMP_ATTACK] * 0.001f)));
        float compRelCoeff = std::exp(-1.0f / (sampleRate * (params[PARAM_COMP_RELEASE] * 0.001f)));
        float compMakeupLin = std::pow(10.0f, params[PARAM_COMP_MAKEUP] / 20.0f);

        bool chorusOn = params[PARAM_CHORUS_ENABLE] > 0.5f;
        float chorusDepth = params[PARAM_CHORUS_DEPTH] * 0.01f;
        float chorusMix = params[PARAM_CHORUS_MIX] * 0.01f;
        float chorusPhaseInc = 2.0f * 3.14159265358979323846f * params[PARAM_CHORUS_RATE] / sampleRate;

        bool delayOn = params[PARAM_DELAY_ENABLE] > 0.5f;
        float delayTimeMs = params[PARAM_DELAY_TIME];
        float delaySamples = delayTimeMs * 0.001f * sampleRate;
        float delayFb = params[PARAM_DELAY_FEEDBACK] * 0.01f;
        float delayMix = params[PARAM_DELAY_MIX] * 0.01f;
        float delayDampFreq = params[PARAM_DELAY_DAMP];
        float delayDampCoeff = std::exp(-2.0f * 3.14159265358979323846f * delayDampFreq / sampleRate);

        int cabMode = static_cast<int>(params[PARAM_CAB_MODE]);
        bool limiterOn = params[PARAM_LIMITER_ENABLE] > 0.5f;
        float outGainLin = std::pow(10.0f, params[PARAM_OUT_GAIN] / 20.0f);
        float dryWet = params[PARAM_DRY_WET] * 0.01f;

        int delayBufSize = delayBufferL.size();
        int chorusBufSize = chorusBufferL.size();

        for (int i = 0; i < numFrames; ++i) {
            float rawL = inL[i];
            float rawR = inR[i];

            // 1. Input Gain
            float sL = rawL * inGainLin;
            float sR = rawR * inGainLin;

            // 2. Noise Gate
            float peak = std::max(std::abs(sL), std::abs(sR));
            if (peak > gateEnv) {
                gateEnv = peak; // instant attack
            } else {
                gateEnv = gateEnv * gateRelCoeff + peak * (1.0f - gateRelCoeff);
            }
            gateEnv = flushDenormal(gateEnv);

            float gateGain = 1.0f;
            if (gateEnv < gateThreshLin) {
                float diff = (gateThreshLin - gateEnv) / std::max(1e-6f, gateThreshLin);
                gateGain = std::max(0.0f, 1.0f - diff * diff);
            }
            sL *= gateGain;
            sR *= gateGain;

            // Save clean copy for dry/wet & drive clean blend
            float preDriveL = sL;
            float preDriveR = sR;

            // 3. Preamp Saturation / Overdrive
            if (driveOn && driveAmt > 0.001f) {
                // Pre-tilt tone shaping
                float shapedL = driveToneFilterL.process(sL);
                float shapedR = driveToneFilterR.process(sR);

                // Drive gain multiplier: 1x to 15x
                float driveGain = 1.0f + driveAmt * 14.0f;
                float dL = shapedL * driveGain;
                float dR = shapedR * driveGain;

                if (driveMode == 0) {
                    // Warm Tube: Asymmetric soft clipping
                    auto tubeSat = [](float x) {
                        if (x > 0.0f) {
                            return std::tanh(x * 1.2f) * 0.833f;
                        } else {
                            float neg = -x;
                            return -(neg / (1.0f + neg * 0.6f));
                        }
                    };
                    dL = tubeSat(dL);
                    dR = tubeSat(dR);
                } else if (driveMode == 1) {
                    // Overdrive: Symmetrical cubic / tanh soft-clipping
                    auto odSat = [](float x) {
                        float absX = std::abs(x);
                        if (absX <= 1.0f) {
                            return x - (x * x * x) / 3.0f;
                        } else {
                            return (x > 0.0f) ? (2.0f / 3.0f) : (-2.0f / 3.0f);
                        }
                    };
                    dL = odSat(dL * 0.8f) * 1.25f;
                    dR = odSat(dR * 0.8f) * 1.25f;
                } else {
                    // Fuzz / Heavy Grit: Hard foldback & harmonic crunch
                    auto fuzzSat = [](float x) {
                        float clip = std::clamp(x, -1.2f, 1.2f);
                        return std::tanh(clip * 1.8f);
                    };
                    dL = fuzzSat(dL);
                    dR = fuzzSat(dR);
                }

                // DC blocker for asymmetric drive
                driveDcBlockL = dL - driveDcBlockL * 0.995f;
                driveDcBlockR = dR - driveDcBlockR * 0.995f;
                dL = dL - driveDcBlockL;
                dR = dR - driveDcBlockR;

                // Blend drive with pre-drive
                sL = preDriveL * (1.0f - driveMix) + dL * driveMix;
                sR = preDriveR * (1.0f - driveMix) + dR * driveMix;
            }

            // 4. Tone Stack (4-Band EQ)
            if (eqOn) {
                sL = eqBassL.process(sL);
                sL = eqLowMidL.process(sL);
                sL = eqHighMidL.process(sL);
                sL = eqTrebleL.process(sL);

                sR = eqBassR.process(sR);
                sR = eqLowMidR.process(sR);
                sR = eqHighMidR.process(sR);
                sR = eqTrebleR.process(sR);
            }

            // 5. Resurrection Psychoacoustic Sub-Harmonic Bass Enhancer
            if (subOn) {
                // Extract fundamental sub band
                float subL = subExtractL.process(sL);
                float subR = subExtractR.process(sR);

                // Generate 2nd & 3rd virtual pitch harmonics
                // Chebyshev T2(x) = 2x^2 - 1, T3(x) = 4x^3 - 3x
                auto genHarmonics = [](float x) {
                    float xClamped = std::clamp(x * 1.5f, -1.0f, 1.0f);
                    float h2 = 2.0f * xClamped * xClamped - 1.0f;
                    float h3 = 4.0f * xClamped * xClamped * xClamped - 3.0f * xClamped;
                    return h2 * 0.6f + h3 * 0.4f;
                };

                float harmL = subHarmFilterL.process(genHarmonics(subL));
                float harmR = subHarmFilterR.process(genHarmonics(subR));

                sL += harmL * subHarmAmt * 0.8f + subL * subLevelAmt;
                sR += harmR * subHarmAmt * 0.8f + subR * subLevelAmt;
            }

            // 6. Bass Compressor
            if (compOn) {
                float envIn = std::max(std::abs(sL), std::abs(sR));
                float inDb = (envIn > 1e-5f) ? (20.0f * std::log10(envIn)) : -100.0f;

                // Soft-knee curve (4 dB knee width)
                float knee = 4.0f;
                float grDb = 0.0f;
                float delta = inDb - compThreshDb;

                if (delta <= -knee * 0.5f) {
                    grDb = 0.0f;
                } else if (delta >= knee * 0.5f) {
                    grDb = (1.0f - 1.0f / compRatio) * delta;
                } else {
                    float k = (delta + knee * 0.5f) / knee;
                    grDb = (1.0f - 1.0f / compRatio) * (k * k * knee * 0.5f);
                }

                // Ballistic smoothing
                if (grDb > compEnv) {
                    compEnv = compEnv * compAttCoeff + grDb * (1.0f - compAttCoeff);
                } else {
                    compEnv = compEnv * compRelCoeff + grDb * (1.0f - compRelCoeff);
                }
                compEnv = flushDenormal(compEnv);

                float compGainLin = std::pow(10.0f, -compEnv / 20.0f) * compMakeupLin;
                sL *= compGainLin;
                sR *= compGainLin;
            }

            // 7. Bass Crossover Chorus
            if (chorusOn && chorusMix > 0.001f) {
                // Split lows and highs: keep low frequencies mono, solid and phase coherent
                float lowL = chorusCrossoverLowL.process(sL);
                float lowR = chorusCrossoverLowR.process(sR);
                float monoLow = (lowL + lowR) * 0.5f;

                float highL = chorusCrossoverHighL.process(sL);
                float highR = chorusCrossoverHighR.process(sR);

                // Write highs to circular chorus buffer
                chorusBufferL[chorusWritePos] = highL;
                chorusBufferR[chorusWritePos] = highR;

                // Dual quadrature LFO (L = 0 deg, R = 90 deg)
                float lfoL = (std::sin(chorusPhase) + 1.0f) * 0.5f;
                float lfoR = (std::sin(chorusPhase + 1.57079632679f) + 1.0f) * 0.5f;

                // Delay time: 5ms to 25ms
                float baseDelay = 0.008f * sampleRate;
                float modDepth = 0.012f * sampleRate * chorusDepth;

                auto readChorus = [&](const std::vector<float>& buf, float delaySamp) {
                    float rPos = (float)chorusWritePos - delaySamp;
                    while (rPos < 0.0f) rPos += chorusBufSize;
                    int i0 = (int)rPos;
                    int i1 = (i0 + 1) % chorusBufSize;
                    float frac = rPos - (float)i0;
                    return buf[i0] + frac * (buf[i1] - buf[i0]);
                };

                float wetHighL = readChorus(chorusBufferL, baseDelay + modDepth * lfoL);
                float wetHighR = readChorus(chorusBufferR, baseDelay + modDepth * lfoR);

                chorusWritePos = (chorusWritePos + 1) % chorusBufSize;
                chorusPhase += chorusPhaseInc;
                if (chorusPhase > 6.28318530718f) chorusPhase -= 6.28318530718f;

                sL = monoLow + (highL * (1.0f - chorusMix) + wetHighL * chorusMix);
                sR = monoLow + (highR * (1.0f - chorusMix) + wetHighR * chorusMix);
            }

            // 8. Stereo Delay
            if (delayOn && delayMix > 0.001f) {
                auto readDelay = [&](const std::vector<float>& buf, float delaySamp) {
                    float rPos = (float)delayWritePos - delaySamp;
                    while (rPos < 0.0f) rPos += delayBufSize;
                    int i0 = (int)rPos;
                    int i1 = (i0 + 1) % delayBufSize;
                    float frac = rPos - (float)i0;
                    return buf[i0] + frac * (buf[i1] - buf[i0]);
                };

                float delOutL = readDelay(delayBufferL, delaySamples);
                float delOutR = readDelay(delayBufferR, delaySamples * 1.08f); // slight stereo offset

                // Damping filter
                delayDampStateL = delOutL * (1.0f - delayDampCoeff) + delayDampStateL * delayDampCoeff;
                delayDampStateR = delOutR * (1.0f - delayDampCoeff) + delayDampStateR * delayDampCoeff;
                delayDampStateL = flushDenormal(delayDampStateL);
                delayDampStateR = flushDenormal(delayDampStateR);

                delayBufferL[delayWritePos] = sL + delayDampStateL * delayFb;
                delayBufferR[delayWritePos] = sR + delayDampStateR * delayFb;

                delayWritePos = (delayWritePos + 1) % delayBufSize;

                sL = sL * (1.0f - delayMix) + delOutL * delayMix;
                sR = sR * (1.0f - delayMix) + delOutR * delayMix;
            }

            // 9. Cabinet & Speaker Simulation
            if (cabMode > 0) {
                sL = cabLowL.process(sL);
                sL = cabMid1L.process(sL);
                sL = cabMid2L.process(sL);
                sL = cabHighL.process(sL);

                sR = cabLowR.process(sR);
                sR = cabMid1R.process(sR);
                sR = cabMid2R.process(sR);
                sR = cabHighR.process(sR);
            }

            // 10. Master Output Gain
            sL *= outGainLin;
            sR *= outGainLin;

            // 11. Brickwall Peak Limiter (Soft Knee at -0.1 dBFS)
            if (limiterOn) {
                auto softLimiter = [](float x) {
                    float ceiling = 0.988f; // ~ -0.1 dBFS
                    float absX = std::abs(x);
                    if (absX <= ceiling * 0.85f) {
                        return x;
                    } else {
                        // Smooth cubic soft knee saturation
                        float sign = (x >= 0.0f) ? 1.0f : -1.0f;
                        float norm = (absX - ceiling * 0.85f) / (ceiling * 0.15f);
                        norm = std::clamp(norm, 0.0f, 1.0f);
                        float compressed = ceiling * 0.85f + (ceiling * 0.15f) * (norm - norm * norm * 0.333f);
                        return sign * std::min(compressed, ceiling);
                    }
                };
                sL = softLimiter(sL);
                sR = softLimiter(sR);
            }

            // 12. Global Dry / Wet Blend
            outL[i] = rawL * (1.0f - dryWet) + sL * dryWet;
            outR[i] = rawR * (1.0f - dryWet) + sR * dryWet;
        }
    }

private:
    void updateFilterCoeffs() {
        // Drive tone tilt
        float driveTone = params[PARAM_DRIVE_TONE];
        driveToneFilterL.setParams(FilterType::Peak, sampleRate, 1200.0f, 0.8f, driveTone * 0.08f);
        driveToneFilterR.setParams(FilterType::Peak, sampleRate, 1200.0f, 0.8f, driveTone * 0.08f);

        // 4-Band Bass Tone Stack
        eqBassL.setParams(FilterType::LowShelf, sampleRate, 80.0f, 0.707f, params[PARAM_EQ_BASS]);
        eqBassR.setParams(FilterType::LowShelf, sampleRate, 80.0f, 0.707f, params[PARAM_EQ_BASS]);

        eqLowMidL.setParams(FilterType::Peak, sampleRate, 350.0f, 1.0f, params[PARAM_EQ_LOW_MID]);
        eqLowMidR.setParams(FilterType::Peak, sampleRate, 350.0f, 1.0f, params[PARAM_EQ_LOW_MID]);

        eqHighMidL.setParams(FilterType::Peak, sampleRate, 1800.0f, 1.2f, params[PARAM_EQ_HIGH_MID]);
        eqHighMidR.setParams(FilterType::Peak, sampleRate, 1800.0f, 1.2f, params[PARAM_EQ_HIGH_MID]);

        eqTrebleL.setParams(FilterType::HighShelf, sampleRate, 4500.0f, 0.707f, params[PARAM_EQ_TREBLE]);
        eqTrebleR.setParams(FilterType::HighShelf, sampleRate, 4500.0f, 0.707f, params[PARAM_EQ_TREBLE]);

        // Sub Enhancer Filters
        float subF = params[PARAM_SUB_FREQ];
        subExtractL.setParams(FilterType::BandPass, sampleRate, subF, 1.4f, 0.0f);
        subExtractR.setParams(FilterType::BandPass, sampleRate, subF, 1.4f, 0.0f);

        subHarmFilterL.setParams(FilterType::BandPass, sampleRate, subF * 2.2f, 1.2f, 0.0f);
        subHarmFilterR.setParams(FilterType::BandPass, sampleRate, subF * 2.2f, 1.2f, 0.0f);

        // Chorus Crossover
        float xOverF = params[PARAM_CHORUS_CROSSOVER];
        chorusCrossoverLowL.setParams(FilterType::LowPass, sampleRate, xOverF, 0.707f);
        chorusCrossoverLowR.setParams(FilterType::LowPass, sampleRate, xOverF, 0.707f);
        chorusCrossoverHighL.setParams(FilterType::HighPass, sampleRate, xOverF, 0.707f);
        chorusCrossoverHighR.setParams(FilterType::HighPass, sampleRate, xOverF, 0.707f);

        // Cabinet Filters
        int cabMode = static_cast<int>(params[PARAM_CAB_MODE]);
        switch (cabMode) {
            case 1: // Classic 8x10 SVT Fridge: punchy, tight low-end, 2.5kHz punch, 4.5kHz roll-off
                cabLowL.setParams(FilterType::HighPass, sampleRate, 52.0f, 0.8f);
                cabLowR.setParams(FilterType::HighPass, sampleRate, 52.0f, 0.8f);
                cabMid1L.setParams(FilterType::Peak, sampleRate, 400.0f, 1.2f, -2.5f);
                cabMid1R.setParams(FilterType::Peak, sampleRate, 400.0f, 1.2f, -2.5f);
                cabMid2L.setParams(FilterType::Peak, sampleRate, 2400.0f, 1.4f, 4.0f);
                cabMid2R.setParams(FilterType::Peak, sampleRate, 2400.0f, 1.4f, 4.0f);
                cabHighL.setParams(FilterType::LowPass, sampleRate, 4500.0f, 0.9f);
                cabHighR.setParams(FilterType::LowPass, sampleRate, 4500.0f, 0.9f);
                break;

            case 2: // Vintage 1x15 Portaflex: big warm bottom, round mids, soft top
                cabLowL.setParams(FilterType::LowShelf, sampleRate, 85.0f, 0.7f, 3.5f);
                cabLowR.setParams(FilterType::LowShelf, sampleRate, 85.0f, 0.7f, 3.5f);
                cabMid1L.setParams(FilterType::Peak, sampleRate, 750.0f, 1.0f, -3.0f);
                cabMid1R.setParams(FilterType::Peak, sampleRate, 750.0f, 1.0f, -3.0f);
                cabMid2L.setParams(FilterType::Peak, sampleRate, 1800.0f, 1.5f, 1.5f);
                cabMid2R.setParams(FilterType::Peak, sampleRate, 1800.0f, 1.5f, 1.5f);
                cabHighL.setParams(FilterType::LowPass, sampleRate, 3600.0f, 0.8f);
                cabHighR.setParams(FilterType::LowPass, sampleRate, 3600.0f, 0.8f);
                break;

            case 3: // Modern 4x10 Punch: scooped 500Hz, crisp 5.5kHz horn
                cabLowL.setParams(FilterType::HighPass, sampleRate, 42.0f, 0.7f);
                cabLowR.setParams(FilterType::HighPass, sampleRate, 42.0f, 0.7f);
                cabMid1L.setParams(FilterType::Peak, sampleRate, 500.0f, 1.5f, -4.0f);
                cabMid1R.setParams(FilterType::Peak, sampleRate, 500.0f, 1.5f, -4.0f);
                cabMid2L.setParams(FilterType::Peak, sampleRate, 3200.0f, 1.2f, 3.0f);
                cabMid2R.setParams(FilterType::Peak, sampleRate, 3200.0f, 1.2f, 3.0f);
                cabHighL.setParams(FilterType::LowPass, sampleRate, 6200.0f, 0.8f);
                cabHighR.setParams(FilterType::LowPass, sampleRate, 6200.0f, 0.8f);
                break;

            case 4: // Aggressive 2x12 Rock: intense mid bite, tight bottom
                cabLowL.setParams(FilterType::HighPass, sampleRate, 55.0f, 0.85f);
                cabLowR.setParams(FilterType::HighPass, sampleRate, 55.0f, 0.85f);
                cabMid1L.setParams(FilterType::Peak, sampleRate, 1100.0f, 1.6f, 3.5f);
                cabMid1R.setParams(FilterType::Peak, sampleRate, 1100.0f, 1.6f, 3.5f);
                cabMid2L.setParams(FilterType::Peak, sampleRate, 2600.0f, 1.5f, 2.0f);
                cabMid2R.setParams(FilterType::Peak, sampleRate, 2600.0f, 1.5f, 2.0f);
                cabHighL.setParams(FilterType::LowPass, sampleRate, 5000.0f, 0.9f);
                cabHighR.setParams(FilterType::LowPass, sampleRate, 5000.0f, 0.9f);
                break;

            default: // Bypass / Direct
                break;
        }
    }

    float sampleRate = 44100.0f;
    std::vector<float> params;
    bool dirtyFilters = true;

    // Filters
    Biquad driveToneFilterL, driveToneFilterR;
    Biquad eqBassL, eqBassR;
    Biquad eqLowMidL, eqLowMidR;
    Biquad eqHighMidL, eqHighMidR;
    Biquad eqTrebleL, eqTrebleR;

    Biquad subExtractL, subExtractR;
    Biquad subHarmFilterL, subHarmFilterR;

    Biquad chorusCrossoverLowL, chorusCrossoverLowR;
    Biquad chorusCrossoverHighL, chorusCrossoverHighR;

    Biquad cabLowL, cabLowR;
    Biquad cabMid1L, cabMid1R;
    Biquad cabMid2L, cabMid2R;
    Biquad cabHighL, cabHighR;

    // State
    float gateEnv = 0.0f;
    float compEnv = 0.0f;
    float driveDcBlockL = 0.0f;
    float driveDcBlockR = 0.0f;
    float delayDampStateL = 0.0f;
    float delayDampStateR = 0.0f;

    // Delay buffers
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int delayWritePos = 0;

    // Chorus buffers
    std::vector<float> chorusBufferL;
    std::vector<float> chorusBufferR;
    int chorusWritePos = 0;
    float chorusPhase = 0.0f;
};

} // namespace ThirdBass
