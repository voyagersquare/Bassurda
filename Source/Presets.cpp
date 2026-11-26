#include "Presets.h"
#include "Parameters.h"

using namespace ParamID;

const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets
    {
        { "Init Bass", {} },

        { "808 Boom",
          { { osc1On, 0 }, { subLevel, 1.0f }, { subOct, 0 }, { subWave, 0 }, { subClean, 0 },
            { fltType, 1 }, { fltCutoff, 20000.0f }, { fltReso, 0.0f }, { fltEnv, 0.0f }, { fltKey, 0.0f },
            { pitchAmt, 12.0f }, { pitchDecay, 45.0f },
            { aAtk, 0.3f }, { aDec, 1800.0f }, { aSus, 0.55f }, { aRel, 350.0f },
            { glideMode, 1 }, { glideTime, 60.0f },
            { satType, 0 }, { satDrive, 0.3f }, { satTone, 7000.0f }, { satMix, 1.0f },
            { clickLevel, 0.15f }, { clickTone, 0.6f }, { velocity, 0.15f } } },

        { "Trap Slide 808",
          { { osc1On, 0 }, { subLevel, 1.0f }, { subOct, 0 }, { subClean, 0 },
            { fltCutoff, 20000.0f }, { fltReso, 0.0f }, { fltEnv, 0.0f }, { fltKey, 0.0f },
            { pitchAmt, 7.0f }, { pitchDecay, 30.0f },
            { aAtk, 0.2f }, { aDec, 2500.0f }, { aSus, 0.7f }, { aRel, 400.0f },
            { glideMode, 2 }, { glideTime, 140.0f }, { glideCurve, 1 },
            { satType, 1 }, { satDrive, 0.55f }, { satTone, 6000.0f },
            { clickLevel, 0.1f }, { velocity, 0.1f } } },

        { "Distorted 808",
          { { osc1On, 0 }, { subLevel, 1.0f }, { subOct, 0 }, { subClean, 0 },
            { fltCutoff, 20000.0f }, { fltReso, 0.0f }, { fltEnv, 0.0f }, { fltKey, 0.0f },
            { pitchAmt, 14.0f }, { pitchDecay, 50.0f },
            { aAtk, 0.2f }, { aDec, 1500.0f }, { aSus, 0.5f }, { aRel, 300.0f },
            { glideMode, 1 }, { glideTime, 90.0f },
            { satType, 2 }, { satDrive, 0.7f }, { satTone, 5000.0f }, { satMix, 0.85f },
            { clickLevel, 0.2f }, { outGain, -6.0f } } },

        { "Drill Glide",
          { { osc1On, 0 }, { subLevel, 1.0f }, { subOct, 0 }, { subClean, 0 },
            { fltCutoff, 20000.0f }, { fltReso, 0.0f }, { fltEnv, 0.0f }, { fltKey, 0.0f },
            { pitchAmt, 5.0f }, { pitchDecay, 25.0f },
            { aAtk, 0.2f }, { aDec, 3000.0f }, { aSus, 0.8f }, { aRel, 250.0f },
            { glideMode, 2 }, { glideTime, 220.0f }, { glideCurve, 0 },
            { satType, 3 }, { satDrive, 0.35f }, { satTone, 5500.0f }, { satMix, 0.7f } } },

        { "Clean Sub",
          { { osc1On, 0 }, { subLevel, 1.0f }, { subOct, 0 }, { subClean, 1 },
            { aAtk, 1.0f }, { aDec, 500.0f }, { aSus, 1.0f }, { aRel, 60.0f },
            { glideMode, 1 }, { glideTime, 50.0f }, { satMix, 0.0f }, { outGain, -2.0f } } },

        { "G-Funk Moog",
          { { osc1Wave, 2.6f }, { osc1Level, 0.75f }, { subLevel, 0.6f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 420.0f }, { fltReso, 0.35f }, { fltEnv, 0.45f }, { fltKey, 0.6f },
            { fAtk, 4.0f }, { fDec, 380.0f }, { fSus, 0.3f }, { fRel, 200.0f },
            { aAtk, 2.0f }, { aDec, 600.0f }, { aSus, 0.8f }, { aRel, 150.0f },
            { glideMode, 2 }, { glideTime, 90.0f }, { glideCurve, 1 },
            { lfoRate, 5.0f }, { lfoPitch, 0.12f },
            { satType, 1 }, { satDrive, 0.25f }, { driftAmount, 0.3f } } },

        { "Reese Pressure",
          { { osc1Wave, 2.0f }, { osc1Level, 0.5f }, { osc2On, 1 }, { osc2Wave, 2.0f }, { osc2Level, 0.6f },
            { osc2Voices, 5 }, { osc2Detune, 0.35f }, { subLevel, 0.7f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 900.0f }, { fltReso, 0.2f }, { fltEnv, 0.15f }, { fltKey, 0.3f },
            { lfoRate, 0.3f }, { lfoCutoff, 0.2f }, { lfoRetrig, 0 },
            { aAtk, 3.0f }, { aDec, 800.0f }, { aSus, 0.9f }, { aRel, 180.0f },
            { satType, 0 }, { satDrive, 0.35f }, { satTone, 8000.0f } } },

        { "Rubber Pluck",
          { { osc1Wave, 2.0f }, { subLevel, 0.6f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 180.0f }, { fltReso, 0.5f }, { fltEnv, 0.6f },
            { fDec, 180.0f }, { fSus, 0.0f },
            { aAtk, 0.5f }, { aDec, 350.0f }, { aSus, 0.0f }, { aRel, 120.0f },
            { envMode, 0 }, { glideMode, 0 }, { satDrive, 0.2f } } },

        { "Phone Speaker Grit",
          { { osc1Wave, 3.0f }, { osc1Pw, 0.35f }, { osc1Level, 0.45f }, { osc1Oct, 1 },
            { subLevel, 0.9f }, { subOct, 1 }, { subClean, 1 },
            { fltType, 1 }, { fltCutoff, 700.0f }, { fltReso, 0.1f }, { fltEnv, 0.25f },
            { satType, 1 }, { satDrive, 0.65f }, { satTone, 4500.0f }, { satMix, 0.9f } } },

        { "Wobble Lane",
          { { osc1Wave, 2.0f }, { osc2On, 1 }, { osc2Wave, 3.0f }, { osc2Oct, -1 }, { osc2Level, 0.4f },
            { subLevel, 0.75f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 300.0f }, { fltReso, 0.45f }, { fltEnv, 0.0f },
            { lfoSync, 1 }, { lfoDiv, 5 }, { lfoShape, 0 }, { lfoCutoff, 0.6f },
            { satType, 0 }, { satDrive, 0.4f } } },

        { "Punch Square",
          { { osc1Wave, 3.0f }, { osc1Level, 0.55f }, { subLevel, 0.8f }, { subOct, 1 }, { subWave, 1 },
            { fltType, 0 }, { fltCutoff, 500.0f }, { fltReso, 0.25f }, { fltEnv, 0.5f },
            { fDec, 120.0f }, { fSus, 0.15f },
            { pitchAmt, 24.0f }, { pitchDecay, 12.0f }, { pitchSub, 0 },
            { aAtk, 0.2f }, { aDec, 300.0f }, { aSus, 0.6f }, { aRel, 80.0f },
            { clickLevel, 0.3f }, { clickTone, 0.8f } } },
    };

    return presets;
}

void applyPreset (juce::AudioProcessorValueTreeState& apvts, const FactoryPreset& preset)
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : preset.values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
}
