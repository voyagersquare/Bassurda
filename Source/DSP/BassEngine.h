#pragma once

#include <array>
#include <atomic>
#include "DspUtils.h"
#include "Envelope.h"

namespace bdsp
{
    // Snapshot of every parameter the voice needs, filled once per block.
    struct EngineParams
    {
        // voice
        float glideMs = 80.0f;
        int   glideMode = 1;          // 0 off, 1 legato, 2 always
        int   glideCurve = 0;         // 0 linear (constant time), 1 exponential
        int   envMode = 1;            // 0 retrigger, 1 legato
        int   bendRange = 2;
        float velSens = 0.25f;
        bool  phaseRetrig = true;
        float phaseStart = 0.0f;      // 0..1
        float drift = 0.0f;

        struct Osc
        {
            bool  on = true;
            float wave = 2.0f, pw = 0.5f;
            int   oct = 0, semi = 0;
            float fine = 0.0f, level = 0.6f;
        } osc1, osc2;

        int   osc2Voices = 1;
        float osc2Detune = 0.25f;

        float subLevel = 0.8f;
        int   subWave = 0;            // 0 sine, 1 triangle
        int   subOct = 1;             // 0, -1, -2 (index)
        bool  subClean = true;
        float clickLevel = 0.0f, clickTone = 0.5f;

        float pitchAmt = 0.0f, pitchDecayMs = 60.0f;
        bool  pitchToSub = true;

        int   fltType = 1;            // LP12, LP24, BP12, HP12
        float cutoff = 1800.0f, reso = 0.15f, fltEnv = 0.35f, keyTrack = 0.5f, fltDrive = 0.0f;

        Adsr::Params filterEnv { 1.0f, 250.0f, 0.2f, 120.0f };
        Adsr::Params ampEnv    { 0.5f, 400.0f, 0.85f, 90.0f };

        float lfoRateHz = 2.0f;
        int   lfoShape = 0;
        float lfoToCutoff = 0.0f, lfoToPitch = 0.0f;
        bool  lfoRetrig = true;
    };

    // Monophonic bass voice with a note stack, glide, phase reset and a
    // separate "clean" sub-oscillator output that bypasses filter and
    // saturation so the fundamental stays phase-coherent and undistorted.
    class BassEngine
    {
    public:
        void prepare (double sampleRate);
        void reset();
        void setParams (const EngineParams& p);

        void noteOn (int note, float velocity);
        void noteOff (int note);
        void allNotesOff (bool hardStop);
        void pitchBend (int value14bit);

        // Renders the voice. `main` receives everything that goes to the
        // saturation stage; `sub` receives the clean sub (zeros if the sub
        // is routed through the filter instead). Buffers are overwritten.
        void render (float* main, float* sub, int numSamples) noexcept;

        bool isActive() const noexcept { return ampEnv.isActive(); }

        // GUI read-outs (lock-free)
        std::atomic<float> uiCurrentNote { -1.0f };
        std::atomic<float> uiAmpLevel { 0.0f };
        std::atomic<float> uiFilterLevel { 0.0f };
        std::atomic<bool>  uiGliding { false };
        std::atomic<int>   uiTriggerCount { 0 };

    private:
        void triggerVoice (float velocity);
        void setTargetNote (int note, bool glide);
        void resetPhases() noexcept;
        float lfoValue() noexcept;

        static constexpr int maxUnison = 7;

        double sr = 44100.0;
        EngineParams params;

        // note stack (most recent last), fixed storage: no allocation
        std::array<int, 128> held {};
        int numHeld = 0;
        int lastNote = -1;

        // pitch / glide
        double currentNote = 48.0, targetNote = 48.0, glideStep = 0.0;
        bool   gliding = false;
        float  bendSemis = 0.0f;
        int    bendRaw = 8192;

        // oscillators
        double phase1 = 0.0, phaseSub = 0.0;
        std::array<double, maxUnison> phase2 {};
        std::array<float, 3> driftValue {};
        std::array<float, 3> driftTarget {};
        int driftCounter = 0;

        // modulation
        Adsr ampEnv, filterEnv;
        float pitchEnv = 0.0f, pitchEnvCoef = 0.0f;
        double lfoPhase = 0.0;
        float lfoSH = 0.0f;
        float velocityGain = 1.0f, lastVelocity = 1.0f;

        // click transient
        float clickEnv = 0.0f, clickCoef = 0.0f, clickLp = 0.0f, clickHpState = 0.0f;
        Noise noise, clickNoise;

        // filter
        Svf svf1, svf2;
        Smoother cutoffSmooth, resoSmooth, osc1Gain, osc2Gain, subGain, driveSmooth;

        // de-click for phase resets while the voice is still sounding
        bool  pendingDeclick = false;
        float lastMain = 0.0f, lastSub = 0.0f;
        float declickMain = 0.0f, declickSub = 0.0f, declickCoef = 0.0f;
    };
}
