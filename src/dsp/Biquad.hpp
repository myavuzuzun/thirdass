#pragma once
#include <cmath>
#include <algorithm>

namespace ThirdBass {

inline float flushDenormal(float val) {
    return (std::abs(val) < 1e-15f) ? 0.0f : val;
}

enum class FilterType {
    LowPass,
    HighPass,
    BandPass,
    Notch,
    Peak,
    LowShelf,
    HighShelf
};

class Biquad {
public:
    Biquad() {
        reset();
    }

    void reset() {
        z1 = 0.0f;
        z2 = 0.0f;
    }

    void setParams(FilterType type, float sampleRate, float frequency, float q, float gainDb = 0.0f) {
        if (sampleRate <= 0.0f) sampleRate = 44100.0f;
        // Clamp frequency between 10 Hz and Nyquist * 0.95
        float nyquist = sampleRate * 0.495f;
        frequency = std::clamp(frequency, 10.0f, nyquist);
        q = std::max(0.01f, q);

        float w0 = 2.0f * 3.14159265358979323846f * frequency / sampleRate;
        float cosW = std::cos(w0);
        float sinW = std::sin(w0);
        float alpha = sinW / (2.0f * q);
        float A = std::pow(10.0f, gainDb / 40.0f); // sqrt(10^(gainDb/20))

        float b0_t = 0.0f, b1_t = 0.0f, b2_t = 0.0f, a0_t = 1.0f, a1_t = 0.0f, a2_t = 0.0f;

        switch (type) {
            case FilterType::LowPass:
                b0_t = (1.0f - cosW) * 0.5f;
                b1_t = 1.0f - cosW;
                b2_t = (1.0f - cosW) * 0.5f;
                a0_t = 1.0f + alpha;
                a1_t = -2.0f * cosW;
                a2_t = 1.0f - alpha;
                break;

            case FilterType::HighPass:
                b0_t = (1.0f + cosW) * 0.5f;
                b1_t = -(1.0f + cosW);
                b2_t = (1.0f + cosW) * 0.5f;
                a0_t = 1.0f + alpha;
                a1_t = -2.0f * cosW;
                a2_t = 1.0f - alpha;
                break;

            case FilterType::BandPass:
                b0_t = alpha;
                b1_t = 0.0f;
                b2_t = -alpha;
                a0_t = 1.0f + alpha;
                a1_t = -2.0f * cosW;
                a2_t = 1.0f - alpha;
                break;

            case FilterType::Notch:
                b0_t = 1.0f;
                b1_t = -2.0f * cosW;
                b2_t = 1.0f;
                a0_t = 1.0f + alpha;
                a1_t = -2.0f * cosW;
                a2_t = 1.0f - alpha;
                break;

            case FilterType::Peak:
                b0_t = 1.0f + alpha * A;
                b1_t = -2.0f * cosW;
                b2_t = 1.0f - alpha * A;
                a0_t = 1.0f + alpha / A;
                a1_t = -2.0f * cosW;
                a2_t = 1.0f - alpha / A;
                break;

            case FilterType::LowShelf: {
                float sqrtA = std::sqrt(A);
                float aAlpha = 2.0f * sqrtA * alpha;
                b0_t = A * ((A + 1.0f) - (A - 1.0f) * cosW + aAlpha);
                b1_t = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosW);
                b2_t = A * ((A + 1.0f) - (A - 1.0f) * cosW - aAlpha);
                a0_t = (A + 1.0f) + (A - 1.0f) * cosW + aAlpha;
                a1_t = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosW);
                a2_t = (A + 1.0f) + (A - 1.0f) * cosW - aAlpha;
                break;
            }

            case FilterType::HighShelf: {
                float sqrtA = std::sqrt(A);
                float aAlpha = 2.0f * sqrtA * alpha;
                b0_t = A * ((A + 1.0f) + (A - 1.0f) * cosW + aAlpha);
                b1_t = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosW);
                b2_t = A * ((A + 1.0f) + (A - 1.0f) * cosW - aAlpha);
                a0_t = (A + 1.0f) - (A - 1.0f) * cosW + aAlpha;
                a1_t = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosW);
                a2_t = (A + 1.0f) - (A - 1.0f) * cosW - aAlpha;
                break;
            }
        }

        if (std::abs(a0_t) > 1e-12f) {
            float invA0 = 1.0f / a0_t;
            b0 = b0_t * invA0;
            b1 = b1_t * invA0;
            b2 = b2_t * invA0;
            a1 = a1_t * invA0;
            a2 = a2_t * invA0;
        }
    }

    inline float process(float in) {
        // Direct Form II Transposed
        float out = b0 * in + z1;
        z1 = b1 * in - a1 * out + z2;
        z2 = b2 * in - a2 * out;

        z1 = flushDenormal(z1);
        z2 = flushDenormal(z2);
        return out;
    }

private:
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;
};

} // namespace ThirdBass
