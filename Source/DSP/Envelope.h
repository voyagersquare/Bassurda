#pragma once

#include <cmath>
#include <algorithm>

namespace bdsp
{
    // ADSR with a linear attack (precise, punchy) and exponential decay and
    // release that are tuned to land exactly on their target after the set
    // time (they aim slightly past the target and stop there), so "80 ms
    // release" really means the note is silent after 80 ms.
    class Adsr
    {
    public:
        enum class Stage { idle, attack, decay, sustain, release };

        struct Params
        {
            float attackMs = 1.0f, decayMs = 300.0f, sustain = 0.8f, releaseMs = 100.0f;
        };

        void prepare (double sr) noexcept { sampleRate = sr; reset(); }
        void reset() noexcept             { stage = Stage::idle; level = 0.0f; }

        void setParams (const Params& p) noexcept
        {
            params = p;
            attackInc = 1.0f / std::max (1.0f, (float) (p.attackMs * 0.001 * sampleRate));
            decayCoef   = coefFor (p.decayMs);
            releaseCoef = coefFor (p.releaseMs);
        }

        void noteOn() noexcept  { stage = Stage::attack; }   // starts from the current level: no click
        void noteOff() noexcept { if (stage != Stage::idle) stage = Stage::release; }

        bool isActive() const noexcept   { return stage != Stage::idle; }
        bool isReleasing() const noexcept { return stage == Stage::release; }
        Stage getStage() const noexcept  { return stage; }
        float getLevel() const noexcept  { return level; }

        inline float next() noexcept
        {
            switch (stage)
            {
                case Stage::idle:
                    break;

                case Stage::attack:
                    level += attackInc;
                    if (level >= 1.0f) { level = 1.0f; stage = Stage::decay; }
                    break;

                case Stage::decay:
                {
                    const float target = params.sustain - overshoot * (1.0f - params.sustain);
                    level = target + (level - target) * decayCoef;
                    if (level <= params.sustain) { level = params.sustain; stage = Stage::sustain; }
                    break;
                }

                case Stage::sustain:
                    // follow sustain changes smoothly while the key is held
                    level += 0.002f * (params.sustain - level);
                    break;

                case Stage::release:
                {
                    const float target = -overshoot;
                    level = target + (level - target) * releaseCoef;
                    if (level <= 0.0f) { level = 0.0f; stage = Stage::idle; }
                    break;
                }
            }
            return level;
        }

    private:
        static constexpr float overshoot = 0.01f;

        float coefFor (float ms) const noexcept
        {
            const double samples = std::max (1.0, ms * 0.001 * sampleRate);
            // time to go from 1 to 0 when aiming at -overshoot
            const double tau = samples / std::log ((1.0 + overshoot) / overshoot);
            return (float) std::exp (-1.0 / tau);
        }

        double sampleRate = 44100.0;
        Params params;
        Stage stage = Stage::idle;
        float level = 0.0f;
        float attackInc = 1.0f, decayCoef = 0.0f, releaseCoef = 0.0f;
    };
}
