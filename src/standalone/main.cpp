#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <iomanip>
#include "../dsp/ThirdBassDSP.hpp"

int main(int argc, char** argv) {
    std::cout << "===================================================\n";
    std::cout << " Third Bass (Linux Native VST / LV2 / CLAP Engine) \n";
    std::cout << " DSP Verification & Benchmarking Test Suite         \n";
    std::cout << "===================================================\n\n";

    ThirdBass::ThirdBassEngine engine;
    const float sampleRate = 44100.0f;
    engine.setSampleRate(sampleRate);

    // 1. Generate test audio: 1 second of a low-E bass note (E1 ~ 41.2 Hz) + harmonics
    const int numSamples = static_cast<int>(sampleRate * 1.5f);
    std::vector<float> inputL(numSamples);
    std::vector<float> inputR(numSamples);
    std::vector<float> outputL(numSamples);
    std::vector<float> outputR(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        float t = (float)i / sampleRate;
        // Plucked envelope decay
        float env = std::exp(-t * 2.5f);
        // Fundamental (41.2 Hz) + 2nd (82.4 Hz) + 3rd (123.6 Hz) + 4th (164.8 Hz)
        float sig = std::sin(2.0f * 3.14159f * 41.203f * t) * 0.7f
                  + std::sin(2.0f * 3.14159f * 82.407f * t) * 0.4f
                  + std::sin(2.0f * 3.14159f * 123.61f * t) * 0.25f
                  + std::sin(2.0f * 3.14159f * 164.81f * t) * 0.15f;
        inputL[i] = sig * env * 0.8f;
        inputR[i] = inputL[i];
    }

    const auto& presets = ThirdBass::getPresets();
    std::cout << "Testing all " << presets.size() << " Factory Presets:\n";
    std::cout << "---------------------------------------------------\n";

    bool allPassed = true;

    for (size_t p = 0; p < presets.size(); ++p) {
        engine.loadPreset(static_cast<int>(p));

        const float* inPtrs[2] = { inputL.data(), inputR.data() };
        float* outPtrs[2] = { outputL.data(), outputR.data() };

        // Process in 128-sample buffer chunks
        const int blockSize = 128;
        for (int offset = 0; offset < numSamples; offset += blockSize) {
            int curFrames = std::min(blockSize, numSamples - offset);
            const float* curIn[2] = { inPtrs[0] + offset, inPtrs[1] + offset };
            float* curOut[2] = { outPtrs[0] + offset, outPtrs[1] + offset };
            engine.process(curIn, curOut, curFrames);
        }

        // Analyze output
        float peakL = 0.0f, peakR = 0.0f;
        float rmsL = 0.0f, rmsR = 0.0f;
        bool hasNaN = false;

        for (int i = 0; i < numSamples; ++i) {
            float oL = outputL[i];
            float oR = outputR[i];
            if (std::isnan(oL) || std::isinf(oL) || std::isnan(oR) || std::isinf(oR)) {
                hasNaN = true;
            }
            peakL = std::max(peakL, std::abs(oL));
            peakR = std::max(peakR, std::abs(oR));
            rmsL += oL * oL;
            rmsR += oR * oR;
        }
        rmsL = std::sqrt(rmsL / numSamples);
        rmsR = std::sqrt(rmsR / numSamples);

        float peakDbL = (peakL > 1e-6f) ? (20.0f * std::log10(peakL)) : -120.0f;
        float rmsDbL = (rmsL > 1e-6f) ? (20.0f * std::log10(rmsL)) : -120.0f;

        bool passed = (!hasNaN) && (peakL <= 1.0f) && (rmsL > 0.001f);
        if (!passed) allPassed = false;

        std::cout << "Preset " << std::setw(2) << p + 1 << ": " 
                  << std::left << std::setw(24) << presets[p].name
                  << " Peak: " << std::right << std::setw(6) << std::fixed << std::setprecision(1) << peakDbL << " dBFS, "
                  << " RMS: " << std::setw(6) << rmsDbL << " dBFS | "
                  << (passed ? "[PASS]" : "[FAIL]") << "\n";
    }

    std::cout << "---------------------------------------------------\n";
    if (allPassed) {
        std::cout << "SUCCESS: All presets and DSP blocks verified with clean peak levels and zero NaN/inf!\n";
    } else {
        std::cout << "ERROR: Verification failed on one or more tests!\n";
        return 1;
    }

    return 0;
}
