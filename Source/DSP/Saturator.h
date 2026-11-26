#pragma once

#include "DspUtils.h"

namespace bdsp
{
    // Waveshapers used by the saturation stage. They are evaluated at 4x
    // oversampling inside the processor. "Warm" is deliberately asymmetric:
    // even harmonics (2nd, 4th) are what makes a bass audible on phone and
    // laptop speakers that cannot reproduce the fundamental.
    struct Saturator
    {
        enum Type { warm = 0, tape, hard, fold };

        static inline float shape (int type, float x) noexcept
        {
            switch (type)
            {
                case warm:
                {
                    constexpr float bias = 0.25f;
                    static const float biasOut = std::tanh (bias);
                    return std::tanh (x + bias) - biasOut;
                }
                case tape:
                {
                    const float a = std::abs (x);
                    const float y = (1.0f - std::exp (-a * 1.2f)) / 1.2f * 1.25f;
                    return x < 0.0f ? -y * 0.95f : y;   // slight asymmetry
                }
                case hard:
                    return std::clamp (x, -1.0f, 1.0f);
                default: // fold
                    return std::sin (x);
            }
        }

        // drive 0..1 -> 0..+30 dB input gain
        static inline float driveToGain (float drive) noexcept
        {
            return std::pow (10.0f, drive * 30.0f / 20.0f);
        }

        // rough loudness compensation so turning drive up does not just
        // make everything louder
        static inline float makeupGain (int type, float driveGain) noexcept
        {
            if (type == fold)
                return 0.9f;
            const float probe = 0.5f;
            const float out = std::abs (shape (type, driveGain * probe) - shape (type, 0.0f));
            return out > 1.0e-6f ? std::min (4.0f, probe / out) : 1.0f;
        }
    };
}
