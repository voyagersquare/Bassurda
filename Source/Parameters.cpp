#include "Parameters.h"

namespace
{
    using APF  = juce::AudioParameterFloat;
    using APC  = juce::AudioParameterChoice;
    using APB  = juce::AudioParameterBool;
    using API  = juce::AudioParameterInt;
    using Range = juce::NormalisableRange<float>;

    juce::ParameterID pid (const char* id) { return { id, 1 }; }

    Range timeRange (float minMs, float maxMs, float centreMs)
    {
        Range r { minMs, maxMs };
        r.setSkewForCentre (centreMs);
        return r;
    }

    juce::AudioParameterFloatAttributes msAttr()
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel ("ms")
            .withStringFromValueFunction ([] (float v, int)
            {
                return v < 1000.0f ? juce::String (v, v < 10.0f ? 1 : 0) + " ms"
                                   : juce::String (v / 1000.0f, 2) + " s";
            });
    }

    juce::AudioParameterFloatAttributes percentAttr()
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; });
    }

    juce::AudioParameterFloatAttributes bipolarPercentAttr()
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction ([] (float v, int)
            {
                const int p = juce::roundToInt (v * 100.0f);
                return (p > 0 ? "+" : "") + juce::String (p) + " %";
            });
    }

    juce::AudioParameterFloatAttributes hzAttr()
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel ("Hz")
            .withStringFromValueFunction ([] (float v, int)
            {
                return v < 1000.0f ? juce::String (v, v < 100.0f ? 1 : 0) + " Hz"
                                   : juce::String (v / 1000.0f, 2) + " kHz";
            });
    }

    juce::AudioParameterFloatAttributes waveAttr()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int)
            {
                static const char* names[] = { "Sine", "Triangle", "Saw", "Square" };
                const int idx = juce::jlimit (0, 3, (int) std::round (v));
                if (std::abs (v - (float) idx) < 0.02f)
                    return juce::String (names[idx]);
                const int a = juce::jlimit (0, 2, (int) std::floor (v));
                return juce::String (names[a]) + ">" + names[a + 1] + " " + juce::String (juce::roundToInt ((v - a) * 100.0f)) + "%";
            });
    }

    void addOscillator (juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                        const char* on, const char* wave, const char* pw, const char* oct,
                        const char* semi, const char* fine, const char* level,
                        const juce::String& prefix, bool defaultOn, float defaultWave, int defaultOct, float defaultLevel)
    {
        layout.add (std::make_unique<APB> (pid (on), prefix + " On", defaultOn));
        layout.add (std::make_unique<APF> (pid (wave), prefix + " Wave", Range { 0.0f, 3.0f }, defaultWave, waveAttr()));
        layout.add (std::make_unique<APF> (pid (pw), prefix + " Pulse Width", Range { 0.05f, 0.95f }, 0.5f, percentAttr()));
        layout.add (std::make_unique<API> (pid (oct), prefix + " Octave", -3, 2, defaultOct));
        layout.add (std::make_unique<API> (pid (semi), prefix + " Semitone", -12, 12, 0));
        layout.add (std::make_unique<APF> (pid (fine), prefix + " Fine", Range { -100.0f, 100.0f }, 0.0f,
                                           juce::AudioParameterFloatAttributes().withLabel ("ct")
                                               .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " ct"; })));
        layout.add (std::make_unique<APF> (pid (level), prefix + " Level", Range { 0.0f, 1.0f }, defaultLevel, percentAttr()));
    }
}

double lfoDivisionInBeats (int index)
{
    static const double beats[] = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                     2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0, 1.5, 0.75 };
    return beats[juce::jlimit (0, (int) std::size (beats) - 1, index)];
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace ParamID;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ---- Voice --------------------------------------------------------------
    layout.add (std::make_unique<APF> (pid (glideTime), "Glide Time", timeRange (0.0f, 3000.0f, 200.0f), 80.0f, msAttr()));
    layout.add (std::make_unique<APC> (pid (glideMode), "Glide Mode", ParamChoices::glideMode, 1));
    layout.add (std::make_unique<APC> (pid (glideCurve), "Glide Curve", ParamChoices::glideCurve, 0));
    layout.add (std::make_unique<APC> (pid (envMode), "Envelope Mode", ParamChoices::envMode, 1));
    layout.add (std::make_unique<API> (pid (bendRange), "Pitch Bend Range", 0, 24, 2));
    layout.add (std::make_unique<APF> (pid (velocity), "Velocity Sensitivity", Range { 0.0f, 1.0f }, 0.25f, percentAttr()));
    layout.add (std::make_unique<APB> (pid (phaseRetrig), "Phase Retrigger", true));
    layout.add (std::make_unique<APF> (pid (phaseStart), "Start Phase", Range { 0.0f, 360.0f }, 0.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("deg")
                                           .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + juce::String (juce::CharPointer_UTF8 ("\xc2\xb0")); })));
    layout.add (std::make_unique<APF> (pid (driftAmount), "Analog Drift", Range { 0.0f, 1.0f }, 0.0f, percentAttr()));

    // ---- Oscillators --------------------------------------------------------
    addOscillator (layout, osc1On, osc1Wave, osc1Pw, osc1Oct, osc1Semi, osc1Fine, osc1Level, "Osc A", true, 2.0f, 0, 0.6f);
    addOscillator (layout, osc2On, osc2Wave, osc2Pw, osc2Oct, osc2Semi, osc2Fine, osc2Level, "Osc B", false, 3.0f, 0, 0.5f);
    layout.add (std::make_unique<API> (pid (osc2Voices), "Osc B Unison", 1, 7, 1));
    layout.add (std::make_unique<APF> (pid (osc2Detune), "Osc B Detune", Range { 0.0f, 1.0f }, 0.25f, percentAttr()));

    // ---- Sub + click --------------------------------------------------------
    layout.add (std::make_unique<APF> (pid (subLevel), "Sub Level", Range { 0.0f, 1.0f }, 0.8f, percentAttr()));
    layout.add (std::make_unique<APC> (pid (subWave), "Sub Wave", ParamChoices::subWave, 0));
    layout.add (std::make_unique<APC> (pid (subOct), "Sub Octave", ParamChoices::subOct, 1));
    layout.add (std::make_unique<APB> (pid (subClean), "Clean Sub", true));
    layout.add (std::make_unique<APF> (pid (clickLevel), "Click Level", Range { 0.0f, 1.0f }, 0.0f, percentAttr()));
    layout.add (std::make_unique<APF> (pid (clickTone), "Click Tone", Range { 0.0f, 1.0f }, 0.5f, percentAttr()));

    // ---- Pitch envelope -----------------------------------------------------
    layout.add (std::make_unique<APF> (pid (pitchAmt), "Pitch Env Amount", Range { -24.0f, 48.0f }, 0.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("st")
                                           .withStringFromValueFunction ([] (float v, int) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " st"; })));
    layout.add (std::make_unique<APF> (pid (pitchDecay), "Pitch Env Decay", timeRange (1.0f, 2000.0f, 80.0f), 60.0f, msAttr()));
    layout.add (std::make_unique<APB> (pid (pitchSub), "Pitch Env to Sub", true));

    // ---- Filter -------------------------------------------------------------
    Range cutoffRange { 20.0f, 20000.0f };
    cutoffRange.setSkewForCentre (632.0f);   // geometric centre of 20 Hz .. 20 kHz
    layout.add (std::make_unique<APC> (pid (fltType), "Filter Type", ParamChoices::fltType, 1));
    layout.add (std::make_unique<APF> (pid (fltCutoff), "Cutoff", cutoffRange, 1800.0f, hzAttr()));
    layout.add (std::make_unique<APF> (pid (fltReso), "Resonance", Range { 0.0f, 1.0f }, 0.15f, percentAttr()));
    layout.add (std::make_unique<APF> (pid (fltEnv), "Filter Env Amount", Range { -1.0f, 1.0f }, 0.35f, bipolarPercentAttr()));
    layout.add (std::make_unique<APF> (pid (fltKey), "Key Tracking", Range { 0.0f, 1.0f }, 0.5f, percentAttr()));
    layout.add (std::make_unique<APF> (pid (fltDrive), "Filter Drive", Range { 0.0f, 1.0f }, 0.0f, percentAttr()));

    // ---- Envelopes ----------------------------------------------------------
    layout.add (std::make_unique<APF> (pid (fAtk), "Filter Attack",  timeRange (0.1f, 5000.0f, 150.0f), 1.0f, msAttr()));
    layout.add (std::make_unique<APF> (pid (fDec), "Filter Decay",   timeRange (1.0f, 5000.0f, 400.0f), 250.0f, msAttr()));
    layout.add (std::make_unique<APF> (pid (fSus), "Filter Sustain", Range { 0.0f, 1.0f }, 0.2f, percentAttr()));
    layout.add (std::make_unique<APF> (pid (fRel), "Filter Release", timeRange (1.0f, 5000.0f, 400.0f), 120.0f, msAttr()));

    layout.add (std::make_unique<APF> (pid (aAtk), "Amp Attack",  timeRange (0.1f, 5000.0f, 150.0f), 0.5f, msAttr()));
    layout.add (std::make_unique<APF> (pid (aDec), "Amp Decay",   timeRange (1.0f, 5000.0f, 400.0f), 400.0f, msAttr()));
    layout.add (std::make_unique<APF> (pid (aSus), "Amp Sustain", Range { 0.0f, 1.0f }, 0.85f, percentAttr()));
    layout.add (std::make_unique<APF> (pid (aRel), "Amp Release", timeRange (1.0f, 5000.0f, 400.0f), 90.0f, msAttr()));

    // ---- LFO ----------------------------------------------------------------
    Range lfoRange { 0.05f, 30.0f };
    lfoRange.setSkewForCentre (3.0f);
    layout.add (std::make_unique<APF> (pid (lfoRate), "LFO Rate", lfoRange, 2.0f, hzAttr()));
    layout.add (std::make_unique<APB> (pid (lfoSync), "LFO Sync", false));
    layout.add (std::make_unique<APC> (pid (lfoDiv), "LFO Division", ParamChoices::lfoDiv, 5));
    layout.add (std::make_unique<APC> (pid (lfoShape), "LFO Shape", ParamChoices::lfoShape, 0));
    layout.add (std::make_unique<APF> (pid (lfoCutoff), "LFO to Cutoff", Range { -1.0f, 1.0f }, 0.0f, bipolarPercentAttr()));
    layout.add (std::make_unique<APF> (pid (lfoPitch), "LFO to Pitch", Range { 0.0f, 12.0f, 0.0f, 0.5f }, 0.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("st")
                                           .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2) + " st"; })));
    layout.add (std::make_unique<APB> (pid (lfoRetrig), "LFO Retrigger", true));

    // ---- Saturation ---------------------------------------------------------
    layout.add (std::make_unique<APC> (pid (satType), "Saturation Type", ParamChoices::satType, 0));
    layout.add (std::make_unique<APF> (pid (satDrive), "Drive", Range { 0.0f, 1.0f }, 0.25f, percentAttr()));
    Range toneRange { 800.0f, 20000.0f };
    toneRange.setSkewForCentre (4000.0f);
    layout.add (std::make_unique<APF> (pid (satTone), "Saturation Tone", toneRange, 9000.0f, hzAttr()));
    layout.add (std::make_unique<APF> (pid (satMix), "Saturation Mix", Range { 0.0f, 1.0f }, 1.0f, percentAttr()));

    // ---- Output -------------------------------------------------------------
    layout.add (std::make_unique<APF> (pid (outGain), "Output Gain", Range { -24.0f, 12.0f }, -3.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")
                                           .withStringFromValueFunction ([] (float v, int) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; })));
    layout.add (std::make_unique<APB> (pid (outClip), "Soft Clipper", true));
    layout.add (std::make_unique<APC> (pid (monoCut), "Low Cut", ParamChoices::lowCut, 1));

    return layout;
}
