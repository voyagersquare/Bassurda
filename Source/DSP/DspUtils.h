#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>

namespace bdsp
{
    constexpr double twoPi = 6.283185307179586476925286766559;
    constexpr float  pi    = 3.14159265358979323846f;

    inline float midiToHz (double note) noexcept
    {
        return (float) (440.0 * std::exp2 ((note - 69.0) / 12.0));
    }

    // Classic 2-sample polyBLEP residual for band-limiting steps.
    inline float polyBlep (double t, double dt) noexcept
    {
        if (t < dt)
        {
            t /= dt;
            return (float) (t + t - t * t - 1.0);
        }
        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return (float) (t * t + t + t + 1.0);
        }
        return 0.0f;
    }

    inline double wrap01 (double p) noexcept
    {
        return p - std::floor (p);
    }

    // ---------------------------------------------------------------------
    // Basic waveforms. All of them start at phase 0 on a rising zero
    // crossing (except square, which cannot), so a phase-reset note starts
    // without a step and every hit has the same transient.
    // ---------------------------------------------------------------------
    inline float sine (double p) noexcept      { return (float) std::sin (twoPi * p); }

    inline float triangle (double p) noexcept
    {
        const double t = wrap01 (p + 0.25);
        return (float) (1.0 - 4.0 * std::abs (t - 0.5));
    }

    inline float sawBlep (double p, double dt) noexcept
    {
        const double t = wrap01 (p + 0.5);            // jump sits at p = 0.5
        return (float) (2.0 * t - 1.0) - polyBlep (t, dt);
    }

    inline float pulseBlep (double p, double dt, double pw) noexcept
    {
        float y = p < pw ? 1.0f : -1.0f;
        y += polyBlep (p, dt);
        y -= polyBlep (wrap01 (p - pw + 1.0), dt);
        return y - (float) (2.0 * pw - 1.0);          // remove DC for asymmetric pulses
    }

    // Morphing oscillator: 0 = sine, 1 = triangle, 2 = saw, 3 = square/pulse.
    inline float morphWave (double p, double dt, float morph, float pw) noexcept
    {
        auto shape = [&] (int i) -> float
        {
            switch (i)
            {
                case 0:  return sine (p);
                case 1:  return triangle (p);
                case 2:  return sawBlep (p, dt);
                default: return pulseBlep (p, dt, pw);
            }
        };

        morph = std::clamp (morph, 0.0f, 3.0f);
        const int   a    = std::min (2, (int) morph);
        const float frac = morph - (float) a;

        if (frac < 1.0e-4f)        return shape (a);
        if (frac > 1.0f - 1.0e-4f) return shape (a + 1);
        return shape (a) + frac * (shape (a + 1) - shape (a));
    }

    // Naive (non-band-limited) version, used only for drawing in the GUI.
    inline float morphWaveNaive (double p, float morph, float pw) noexcept
    {
        return morphWave (p, 1.0e-9, morph, pw);
    }

    // ---------------------------------------------------------------------
    // Zero-delay-feedback state variable filter (Cytomic / Simper).
    // DC gain of the low-pass output is exactly 1 at every resonance, so the
    // low end does not thin out when the resonance goes up.
    // ---------------------------------------------------------------------
    struct Svf
    {
        float ic1 = 0.0f, ic2 = 0.0f;
        float low = 0.0f, band = 0.0f, high = 0.0f;

        void reset() noexcept { ic1 = ic2 = low = band = high = 0.0f; }

        // g = tan(pi * fc / fs), k = 1/Q
        inline void process (float v0, float g, float k) noexcept
        {
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1;
            const float a3 = g * a2;
            const float v3 = v0 - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            low  = v2;
            band = v1;
            high = v0 - k * v1 - v2;
        }
    };

    // Linear-in-time parameter smoother (one-pole, ~ms time constant).
    struct Smoother
    {
        float current = 0.0f, target = 0.0f, coef = 0.0f;

        void prepare (double sampleRate, float timeMs) noexcept
        {
            coef = 1.0f - std::exp (-1.0f / (float) (timeMs * 0.001 * sampleRate));
        }
        void setTarget (float t) noexcept { target = t; }
        void snap (float v) noexcept      { current = target = v; }
        inline float next() noexcept      { current += coef * (target - current); return current; }
    };

    // Tiny, fast, deterministic noise source (xorshift32).
    struct Noise
    {
        uint32_t state = 0x9E3779B9u;
        inline float next() noexcept
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return (float) ((int32_t) state) * (1.0f / 2147483648.0f);
        }
    };
}
