#include "BassEngine.h"

namespace bdsp
{
    void BassEngine::prepare (double sampleRate)
    {
        sr = sampleRate;
        ampEnv.prepare (sr);
        filterEnv.prepare (sr);

        cutoffSmooth.prepare (sr, 8.0f);
        resoSmooth.prepare (sr, 15.0f);
        osc1Gain.prepare (sr, 15.0f);
        osc2Gain.prepare (sr, 15.0f);
        subGain.prepare (sr, 15.0f);
        driveSmooth.prepare (sr, 15.0f);

        declickCoef = std::exp (-1.0f / (float) (0.0015 * sr));   // ~1.5 ms
        reset();
        setParams (params);
        cutoffSmooth.snap (params.cutoff);
        resoSmooth.snap (params.reso);
        osc1Gain.snap (params.osc1.on ? params.osc1.level : 0.0f);
        osc2Gain.snap (params.osc2.on ? params.osc2.level : 0.0f);
        subGain.snap (params.subLevel);
        driveSmooth.snap (params.fltDrive);
    }

    void BassEngine::reset()
    {
        ampEnv.reset();
        filterEnv.reset();
        svf1.reset();
        svf2.reset();
        numHeld = 0;
        gliding = false;
        pitchEnv = 0.0f;
        clickEnv = 0.0f;
        lastMain = lastSub = declickMain = declickSub = 0.0f;
        pendingDeclick = false;
        resetPhases();
        pendingDeclick = false;
        uiAmpLevel = 0.0f;
        uiCurrentNote = -1.0f;
    }

    void BassEngine::setParams (const EngineParams& p)
    {
        params = p;
        ampEnv.setParams (p.ampEnv);
        filterEnv.setParams (p.filterEnv);

        pitchEnvCoef = std::exp (-std::log (100.0f) / std::max (1.0f, (float) (p.pitchDecayMs * 0.001 * sr)));

        const float clickMs = 0.6f + (1.0f - p.clickTone) * 2.4f;
        clickCoef = std::exp (-1.0f / (float) (clickMs * 0.001 * sr));

        cutoffSmooth.setTarget (p.cutoff);
        resoSmooth.setTarget (p.reso);
        osc1Gain.setTarget (p.osc1.on ? p.osc1.level : 0.0f);
        osc2Gain.setTarget (p.osc2.on ? p.osc2.level : 0.0f);
        subGain.setTarget (p.subLevel);
        driveSmooth.setTarget (p.fltDrive);

        bendSemis = (float) (bendRaw - 8192) / 8192.0f * (float) p.bendRange;
    }

    // ---------------------------------------------------------------------
    // Note handling (monophonic, last-note priority)
    // ---------------------------------------------------------------------
    void BassEngine::noteOn (int note, float velocity)
    {
        // remove if already held, then push on top
        for (int i = 0; i < numHeld; ++i)
        {
            if (held[(size_t) i] == note)
            {
                for (int j = i; j < numHeld - 1; ++j)
                    held[(size_t) j] = held[(size_t) j + 1];
                --numHeld;
                break;
            }
        }

        const bool legatoTransition = numHeld > 0 && ampEnv.isActive() && ! ampEnv.isReleasing();

        if (numHeld < (int) held.size())
            held[(size_t) numHeld++] = note;

        bool glide = false;
        if (params.glideMode == 1)      glide = legatoTransition;
        else if (params.glideMode == 2) glide = lastNote >= 0;

        setTargetNote (note, glide);
        lastNote = note;

        const bool retrigger = ! (params.envMode == 1 && legatoTransition);
        if (retrigger)
            triggerVoice (velocity);
    }

    void BassEngine::noteOff (int note)
    {
        int idx = -1;
        for (int i = 0; i < numHeld; ++i)
            if (held[(size_t) i] == note) { idx = i; break; }

        if (idx < 0)
            return;

        const bool wasTop = idx == numHeld - 1;
        for (int j = idx; j < numHeld - 1; ++j)
            held[(size_t) j] = held[(size_t) j + 1];
        --numHeld;

        if (numHeld == 0)
        {
            ampEnv.noteOff();
            filterEnv.noteOff();
            return;
        }

        if (wasTop)
        {
            // fall back to the previously held note (classic mono trill)
            const int prev = held[(size_t) numHeld - 1];
            setTargetNote (prev, params.glideMode != 0);
            lastNote = prev;

            if (params.envMode == 0)
                triggerVoice (lastVelocity);
        }
    }

    void BassEngine::allNotesOff (bool hardStop)
    {
        numHeld = 0;
        ampEnv.noteOff();
        filterEnv.noteOff();
        if (hardStop)
            reset();
    }

    void BassEngine::pitchBend (int value14bit)
    {
        bendRaw = value14bit;
        bendSemis = (float) (bendRaw - 8192) / 8192.0f * (float) params.bendRange;
    }

    void BassEngine::setTargetNote (int note, bool glide)
    {
        targetNote = (double) note;

        // "Always" may glide from the previous note even out of silence;
        // otherwise only glide when there is something to glide from.
        const bool canGlide = glide && params.glideMs >= 0.5f && lastNote >= 0
                              && (ampEnv.isActive() || params.glideMode == 2);
        if (! canGlide)
        {
            currentNote = targetNote;
            gliding = false;
            return;
        }

        const double samples = std::max (1.0, params.glideMs * 0.001 * sr);
        glideStep = (targetNote - currentNote) / samples;
        gliding = std::abs (targetNote - currentNote) > 1.0e-6;
    }

    void BassEngine::triggerVoice (float velocity)
    {
        lastVelocity = std::clamp (velocity, 0.0f, 1.0f);
        velocityGain = 1.0f - params.velSens * (1.0f - lastVelocity);
        ampEnv.noteOn();
        filterEnv.noteOn();
        pitchEnv = 1.0f;
        clickEnv = 1.0f;
        clickNoise.state = 0x2545F491u;   // same burst on every hit
        clickLp = clickHpState = 0.0f;

        if (params.phaseRetrig)
            resetPhases();

        if (params.lfoRetrig)
            lfoPhase = 0.0;

        uiTriggerCount.fetch_add (1);
    }

    void BassEngine::resetPhases() noexcept
    {
        const double start = params.phaseStart;
        phase1   = start;
        phaseSub = start;

        // centre voice on the start phase, the others on fixed (golden ratio)
        // offsets: deterministic, so every hit is identical, but without the
        // big in-phase spike that identical phases would give.
        for (size_t v = 0; v < phase2.size(); ++v)
            phase2[v] = wrap01 (start + 0.6180339887 * (double) v);

        pendingDeclick = true;
    }

    float BassEngine::lfoValue() noexcept
    {
        const double p = lfoPhase;
        float v = 0.0f;
        switch (params.lfoShape)
        {
            case 0:  v = sine (p); break;
            case 1:  v = triangle (p); break;
            case 2:  v = (float) (1.0 - 2.0 * p); break;     // falling saw (wobble style)
            case 3:  v = p < 0.5 ? 1.0f : -1.0f; break;
            default: v = lfoSH; break;
        }

        lfoPhase += params.lfoRateHz / sr;
        if (lfoPhase >= 1.0)
        {
            lfoPhase -= std::floor (lfoPhase);
            lfoSH = noise.next();
        }
        return v;
    }

    // ---------------------------------------------------------------------
    // Rendering
    // ---------------------------------------------------------------------
    void BassEngine::render (float* main, float* sub, int numSamples) noexcept
    {
        const bool silent = ! ampEnv.isActive() && clickEnv < 1.0e-5f
                            && std::abs (declickMain) < 1.0e-6f && std::abs (declickSub) < 1.0e-6f;
        if (silent)
        {
            std::fill (main, main + numSamples, 0.0f);
            std::fill (sub, sub + numSamples, 0.0f);
            lastMain = lastSub = 0.0f;
            // clear the filter memory so the next note starts from exactly
            // the same state (identical transient on every hit)
            svf1.reset();
            svf2.reset();
            uiAmpLevel = 0.0f;
            uiGliding = false;
            // keep smoothers converging so the next note starts on target
            cutoffSmooth.snap (cutoffSmooth.target);
            resoSmooth.snap (resoSmooth.target);
            osc1Gain.snap (osc1Gain.target);
            osc2Gain.snap (osc2Gain.target);
            subGain.snap (subGain.target);
            driveSmooth.snap (driveSmooth.target);
            return;
        }

        const double nyquistGuard = 0.45 * sr;

        // unison detune ratios (symmetric spread, up to +-40 cents)
        const int voices = std::clamp (params.osc2Voices, 1, maxUnison);
        std::array<double, maxUnison> ratio {};
        for (int v = 0; v < voices; ++v)
        {
            const double spread = voices > 1 ? ((double) v / (voices - 1)) * 2.0 - 1.0 : 0.0;
            ratio[(size_t) v] = std::exp2 (spread * params.osc2Detune * 40.0 / 1200.0);
        }
        const float unisonNorm = 1.0f / std::sqrt ((float) voices);

        const double glideExpCoef = 1.0 - std::exp (-std::log (100.0) / std::max (1.0, params.glideMs * 0.001 * sr));
        const float  driftCents   = params.drift * 12.0f;
        const float  subOctShift  = -12.0f * (float) params.subOct;

        // click filter coefficients
        const float clickLpCoef = 1.0f - std::exp (-bdsp::pi * 2.0f * (1500.0f + params.clickTone * 12000.0f) / (float) sr);
        const float clickHpCoef = 1.0f - std::exp (-bdsp::pi * 2.0f * 250.0f / (float) sr);

        for (int i = 0; i < numSamples; ++i)
        {
            // ---- envelopes -------------------------------------------------
            const float amp  = ampEnv.next();
            const float fenv = filterEnv.next();

            // ---- glide ---------------------------------------------------
            if (gliding)
            {
                if (params.glideCurve == 0)
                {
                    currentNote += glideStep;
                    if ((glideStep > 0.0 && currentNote >= targetNote) || (glideStep < 0.0 && currentNote <= targetNote))
                    {
                        currentNote = targetNote;
                        gliding = false;
                    }
                }
                else
                {
                    currentNote += (targetNote - currentNote) * glideExpCoef;
                    if (std::abs (targetNote - currentNote) < 0.0005)
                    {
                        currentNote = targetNote;
                        gliding = false;
                    }
                }
            }

            // ---- analog drift (slow random walk) ----------------------------
            if (params.drift > 0.0f)
            {
                if (--driftCounter <= 0)
                {
                    driftCounter = (int) (sr * 0.05);
                    for (auto& t : driftTarget)
                        t = noise.next();
                }
                for (size_t d = 0; d < driftValue.size(); ++d)
                    driftValue[d] += 0.0004f * (driftTarget[d] - driftValue[d]);
            }

            // ---- modulation ----------------------------------------------
            const float lfo = lfoValue();
            const float pe  = pitchEnv * params.pitchAmt;
            pitchEnv *= pitchEnvCoef;

            const double basePitch = currentNote + bendSemis + lfo * params.lfoToPitch;
            const double mainPitch = basePitch + pe;

            // ---- oscillator A --------------------------------------------
            float oscA = 0.0f;
            const float g1 = osc1Gain.next();
            {
                const double pitch = mainPitch + params.osc1.oct * 12 + params.osc1.semi + params.osc1.fine * 0.01
                                     + driftValue[0] * driftCents * 0.01;
                const double dt = std::min ((double) midiToHz (pitch), nyquistGuard) / sr;
                if (g1 > 1.0e-5f)
                    oscA = morphWave (phase1, dt, params.osc1.wave, params.osc1.pw);
                phase1 = wrap01 (phase1 + dt);
            }

            // ---- oscillator B (unison) --------------------------------------
            float oscB = 0.0f;
            const float g2 = osc2Gain.next();
            {
                const double pitch = mainPitch + params.osc2.oct * 12 + params.osc2.semi + params.osc2.fine * 0.01
                                     + driftValue[1] * driftCents * 0.01;
                const double baseDt = std::min ((double) midiToHz (pitch), nyquistGuard) / sr;
                for (int v = 0; v < voices; ++v)
                {
                    const double dt = baseDt * ratio[(size_t) v];
                    if (g2 > 1.0e-5f)
                        oscB += morphWave (phase2[(size_t) v], dt, params.osc2.wave, params.osc2.pw);
                    phase2[(size_t) v] = wrap01 (phase2[(size_t) v] + dt);
                }
                oscB *= unisonNorm;
            }

            // ---- sub oscillator ------------------------------------------
            const float gs = subGain.next();
            float subSig;
            {
                const double pitch = basePitch + (params.pitchToSub ? pe : 0.0f) + subOctShift
                                     + driftValue[2] * driftCents * 0.004;
                const double dt = std::min ((double) midiToHz (pitch), nyquistGuard) / sr;
                subSig = params.subWave == 0 ? sine (phaseSub) : triangle (phaseSub);
                phaseSub = wrap01 (phaseSub + dt);
            }
            subSig *= gs;

            // ---- filter --------------------------------------------------
            float x = oscA * g1 + oscB * g2 + (params.subClean ? 0.0f : subSig);

            const float drive = driveSmooth.next();
            if (drive > 0.001f)
            {
                const float dg = 1.0f + drive * 6.0f;
                x = std::tanh (x * dg) / std::sqrt (dg);
            }

            const float octaves = params.fltEnv * fenv * 7.0f
                                + params.keyTrack * (float) ((currentNote + bendSemis - 36.0) / 12.0)
                                + params.lfoToCutoff * lfo * 3.0f;
            const float fc = std::clamp (cutoffSmooth.next() * std::exp2 (octaves), 16.0f, (float) std::min (20000.0, nyquistGuard));
            const float g  = std::tan (bdsp::pi * fc / (float) sr);
            const float r  = resoSmooth.next();

            float y = 0.0f;
            switch (params.fltType)
            {
                case 0: // LP12
                {
                    const float k = std::max (0.02f, 1.4142f * (1.0f - r));
                    svf1.process (x, g, k);
                    y = svf1.low;
                    break;
                }
                case 1: // LP24: Butterworth pair, resonance on the 2nd stage
                {
                    svf1.process (x, g, 1.8478f);
                    svf2.process (svf1.low, g, std::max (0.02f, 0.7654f * (1.0f - r)));
                    y = svf2.low;
                    break;
                }
                case 2: // BP12 (unity peak)
                {
                    const float k = std::max (0.05f, 1.4142f * (1.0f - r));
                    svf1.process (x, g, k);
                    y = svf1.band * k;
                    break;
                }
                default: // HP12
                {
                    const float k = std::max (0.02f, 1.4142f * (1.0f - r));
                    svf1.process (x, g, k);
                    y = svf1.high;
                    break;
                }
            }

            // ---- click transient (filtered noise burst) ---------------------
            if (clickEnv > 1.0e-5f)
            {
                clickLp += clickLpCoef * (clickNoise.next() - clickLp);
                clickHpState += clickHpCoef * (clickLp - clickHpState);
                y += (clickLp - clickHpState) * clickEnv * params.clickLevel * 2.5f;
                clickEnv *= clickCoef;
            }

            const float vca = amp * velocityGain;
            float outMain = y * vca;
            float outSub  = params.subClean ? subSig * vca : 0.0f;

            // ---- de-click phase resets -------------------------------------
            if (pendingDeclick)
            {
                declickMain = lastMain - outMain;
                declickSub  = lastSub - outSub;
                pendingDeclick = false;
            }
            outMain += declickMain;
            outSub  += declickSub;
            declickMain *= declickCoef;
            declickSub  *= declickCoef;

            lastMain = outMain;
            lastSub  = outSub;

            main[i] = outMain;
            sub[i]  = outSub;
        }

        uiAmpLevel    = ampEnv.getLevel();
        uiFilterLevel = filterEnv.getLevel();
        uiCurrentNote = ampEnv.isActive() ? (float) currentNote : -1.0f;
        uiGliding     = gliding;
    }
}
