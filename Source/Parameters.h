#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// All parameter IDs live here so DSP, GUI and presets share one source of truth.
namespace ParamID
{
    // Voice / performance
    inline constexpr const char* glideTime    = "glide_time";
    inline constexpr const char* glideMode    = "glide_mode";    // Off, Legato, Always
    inline constexpr const char* glideCurve   = "glide_curve";   // Linear, Exponential
    inline constexpr const char* envMode      = "env_mode";      // Retrigger, Legato
    inline constexpr const char* bendRange    = "bend_range";
    inline constexpr const char* velocity     = "velocity";
    inline constexpr const char* phaseRetrig  = "phase_retrig";
    inline constexpr const char* phaseStart   = "phase_start";
    inline constexpr const char* driftAmount  = "drift";

    // Oscillator A
    inline constexpr const char* osc1On     = "osc1_on";
    inline constexpr const char* osc1Wave   = "osc1_wave";
    inline constexpr const char* osc1Pw     = "osc1_pw";
    inline constexpr const char* osc1Oct    = "osc1_oct";
    inline constexpr const char* osc1Semi   = "osc1_semi";
    inline constexpr const char* osc1Fine   = "osc1_fine";
    inline constexpr const char* osc1Level  = "osc1_level";

    // Oscillator B (with unison for Reese-type basses)
    inline constexpr const char* osc2On     = "osc2_on";
    inline constexpr const char* osc2Wave   = "osc2_wave";
    inline constexpr const char* osc2Pw     = "osc2_pw";
    inline constexpr const char* osc2Oct    = "osc2_oct";
    inline constexpr const char* osc2Semi   = "osc2_semi";
    inline constexpr const char* osc2Fine   = "osc2_fine";
    inline constexpr const char* osc2Level  = "osc2_level";
    inline constexpr const char* osc2Voices = "osc2_voices";
    inline constexpr const char* osc2Detune = "osc2_detune";

    // Sub oscillator + transient click
    inline constexpr const char* subLevel   = "sub_level";
    inline constexpr const char* subWave    = "sub_wave";    // Sine, Triangle
    inline constexpr const char* subOct     = "sub_oct";     // 0, -1, -2
    inline constexpr const char* subClean   = "sub_clean";   // bypass filter + saturation
    inline constexpr const char* clickLevel = "click_level";
    inline constexpr const char* clickTone  = "click_tone";

    // Pitch envelope
    inline constexpr const char* pitchAmt   = "penv_amt";
    inline constexpr const char* pitchDecay = "penv_decay";
    inline constexpr const char* pitchSub   = "penv_sub";    // apply to sub osc too

    // Filter
    inline constexpr const char* fltType    = "flt_type";    // LP12, LP24, BP12, HP12
    inline constexpr const char* fltCutoff  = "flt_cutoff";
    inline constexpr const char* fltReso    = "flt_reso";
    inline constexpr const char* fltEnv     = "flt_env";
    inline constexpr const char* fltKey     = "flt_key";
    inline constexpr const char* fltDrive   = "flt_drive";

    // Filter envelope
    inline constexpr const char* fAtk = "fenv_a";
    inline constexpr const char* fDec = "fenv_d";
    inline constexpr const char* fSus = "fenv_s";
    inline constexpr const char* fRel = "fenv_r";

    // Amp envelope
    inline constexpr const char* aAtk = "aenv_a";
    inline constexpr const char* aDec = "aenv_d";
    inline constexpr const char* aSus = "aenv_s";
    inline constexpr const char* aRel = "aenv_r";

    // LFO
    inline constexpr const char* lfoRate    = "lfo_rate";
    inline constexpr const char* lfoSync    = "lfo_sync";
    inline constexpr const char* lfoDiv     = "lfo_div";
    inline constexpr const char* lfoShape   = "lfo_shape";
    inline constexpr const char* lfoCutoff  = "lfo_cutoff";
    inline constexpr const char* lfoPitch   = "lfo_pitch";
    inline constexpr const char* lfoRetrig  = "lfo_retrig";

    // Saturation
    inline constexpr const char* satType  = "sat_type";      // Warm, Tape, Hard, Fold
    inline constexpr const char* satDrive = "sat_drive";
    inline constexpr const char* satTone  = "sat_tone";
    inline constexpr const char* satMix   = "sat_mix";
    inline constexpr const char* satBand  = "sat_band";      // Full, Highs (multiband)
    inline constexpr const char* satXover = "sat_xover";     // crossover for "Highs"

    // Sample oscillator (load your own 808 / bass one-shot)
    inline constexpr const char* smpOn    = "smp_on";
    inline constexpr const char* smpLevel = "smp_level";
    inline constexpr const char* smpRoot  = "smp_root";
    inline constexpr const char* smpTune  = "smp_tune";
    inline constexpr const char* smpStart = "smp_start";
    inline constexpr const char* smpLoop  = "smp_loop";
    inline constexpr const char* smpPenv  = "smp_penv";

    // Kick duck (MIDI / beat triggered ducking)
    inline constexpr const char* duckMode    = "duck_mode";  // Off, Note, Ch 10, Beat
    inline constexpr const char* duckNote    = "duck_note";
    inline constexpr const char* duckDepth   = "duck_depth";
    inline constexpr const char* duckAttack  = "duck_atk";
    inline constexpr const char* duckHold    = "duck_hold";
    inline constexpr const char* duckRelease = "duck_rel";

    // Output
    inline constexpr const char* outGain   = "out_gain";
    inline constexpr const char* outClip   = "out_clip";
    inline constexpr const char* monoCut   = "low_cut";      // DC / infrasonic high-pass
    inline constexpr const char* width     = "width";        // stereo width of the highs
    inline constexpr const char* monoBelow = "mono_below";   // everything below stays mono
}

namespace ParamChoices
{
    inline const juce::StringArray glideMode  { "Off", "Legato", "Always" };
    inline const juce::StringArray glideCurve { "Linear", "Exp" };
    inline const juce::StringArray envMode    { "Retrig", "Legato" };
    inline const juce::StringArray subWave    { "Sine", "Triangle" };
    inline const juce::StringArray subOct     { "0", "-1", "-2" };
    inline const juce::StringArray fltType    { "LP12", "LP24", "BP12", "HP12" };
    inline const juce::StringArray lfoDiv     { "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                                                "1/4T", "1/8T", "1/16T", "1/4.", "1/8." };
    inline const juce::StringArray lfoShape   { "Sine", "Triangle", "Saw", "Square", "S&H" };
    inline const juce::StringArray satType    { "Warm", "Tape", "Hard", "Fold" };
    inline const juce::StringArray lowCut     { "Off", "20 Hz", "30 Hz", "40 Hz" };
    inline const juce::StringArray satBand    { "Full", "Highs" };
    inline const juce::StringArray duckMode   { "Off", "Note", "Ch 10", "Beat" };
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// LFO division in beats (quarter notes) for the lfoDiv choice index.
double lfoDivisionInBeats (int index);
