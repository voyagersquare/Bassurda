#include "Presets.h"
#include "Parameters.h"

using namespace ParamID;

namespace
{
    using Values = std::vector<std::pair<juce::String, float>>;

    // Shared starting point for 808-style patches: sub only, routed through
    // the saturator, filter wide open.
    Values eightOhEight (Values extra)
    {
        Values v { { osc1On, 0 }, { subLevel, 1.0f }, { subOct, 0 }, { subWave, 0 }, { subClean, 0 },
                   { fltCutoff, 20000.0f }, { fltReso, 0.0f }, { fltEnv, 0.0f }, { fltKey, 0.0f },
                   { velocity, 0.1f } };
        v.insert (v.end(), extra.begin(), extra.end());   // later entries override earlier ones
        return v;
    }
}

const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets
    {
        { "Init Bass", {} },

        // ================= 808s: trap ========================================
        { "808 Boom", eightOhEight ({
            { pitchAmt, 12.0f }, { pitchDecay, 45.0f },
            { aAtk, 0.3f }, { aDec, 1800.0f }, { aSus, 0.55f }, { aRel, 350.0f },
            { glideMode, 1 }, { glideTime, 60.0f },
            { satType, 0 }, { satDrive, 0.3f }, { satTone, 7000.0f },
            { clickLevel, 0.15f }, { clickTone, 0.6f } }) },

        { "Trap Slide 808", eightOhEight ({
            { pitchAmt, 7.0f }, { pitchDecay, 30.0f },
            { aAtk, 0.2f }, { aDec, 2500.0f }, { aSus, 0.7f }, { aRel, 400.0f },
            { glideMode, 2 }, { glideTime, 140.0f }, { glideCurve, 1 },
            { satType, 1 }, { satDrive, 0.55f }, { satTone, 6000.0f }, { clickLevel, 0.1f } }) },

        { "Distorted 808", eightOhEight ({
            { pitchAmt, 14.0f }, { pitchDecay, 50.0f },
            { aAtk, 0.2f }, { aDec, 1500.0f }, { aSus, 0.5f }, { aRel, 300.0f },
            { glideMode, 1 }, { glideTime, 90.0f },
            { satType, 2 }, { satDrive, 0.7f }, { satTone, 5000.0f }, { satMix, 0.85f },
            { clickLevel, 0.2f }, { outGain, -6.0f } }) },

        { "Long 808 Tail", eightOhEight ({
            { pitchAmt, 9.0f }, { pitchDecay, 60.0f },
            { aAtk, 0.3f }, { aDec, 4000.0f }, { aSus, 0.85f }, { aRel, 900.0f },
            { glideMode, 1 }, { glideTime, 110.0f }, { glideCurve, 1 },
            { satType, 0 }, { satDrive, 0.3f }, { satBand, 1 }, { satXover, 110.0f }, { satTone, 8000.0f } }) },

        { "Short 808 Knock", eightOhEight ({
            { pitchAmt, 18.0f }, { pitchDecay, 25.0f },
            { aAtk, 0.1f }, { aDec, 280.0f }, { aSus, 0.0f }, { aRel, 80.0f },
            { envMode, 0 }, { glideMode, 0 },
            { satType, 2 }, { satDrive, 0.4f }, { satTone, 7000.0f },
            { clickLevel, 0.35f }, { clickTone, 0.7f }, { velocity, 0.3f } }) },

        { "Hard 808 Grit", eightOhEight ({
            { pitchAmt, 12.0f }, { pitchDecay, 40.0f },
            { aAtk, 0.2f }, { aDec, 2000.0f }, { aSus, 0.6f }, { aRel, 300.0f },
            { glideMode, 1 }, { glideTime, 70.0f },
            { satType, 2 }, { satDrive, 0.85f }, { satTone, 4500.0f },
            { clickLevel, 0.15f }, { outGain, -7.0f } }) },

        { "Fuzzy Fold 808", eightOhEight ({
            { pitchAmt, 10.0f }, { pitchDecay, 45.0f },
            { aAtk, 0.3f }, { aDec, 2200.0f }, { aSus, 0.65f }, { aRel, 350.0f },
            { glideMode, 1 }, { glideTime, 90.0f },
            { satType, 3 }, { satDrive, 0.5f }, { satBand, 1 }, { satXover, 100.0f }, { satTone, 6500.0f },
            { outGain, -4.0f } }) },

        { "Rage 808", eightOhEight ({
            { osc1On, 1 }, { osc1Wave, 3.0f }, { osc1Oct, 1 }, { osc1Level, 0.25f },
            { fltCutoff, 2500.0f }, { fltType, 1 },
            { pitchAmt, 12.0f }, { pitchDecay, 35.0f },
            { aAtk, 0.2f }, { aDec, 2500.0f }, { aSus, 0.7f }, { aRel, 300.0f },
            { glideMode, 2 }, { glideTime, 120.0f },
            { satType, 2 }, { satDrive, 0.75f }, { satTone, 5000.0f }, { outGain, -4.5f } }) },

        { "Memphis Phonk 808", eightOhEight ({
            { osc1On, 1 }, { osc1Wave, 1.0f }, { osc1Oct, 1 }, { osc1Level, 0.2f },
            { pitchAmt, 10.0f }, { pitchDecay, 55.0f },
            { aAtk, 0.3f }, { aDec, 1600.0f }, { aSus, 0.5f }, { aRel, 250.0f },
            { glideMode, 2 }, { glideTime, 100.0f },
            { satType, 1 }, { satDrive, 0.6f }, { satTone, 3500.0f }, { driftAmount, 0.3f }, { outGain, -2.5f } }) },

        { "Plugg Sine", eightOhEight ({
            { pitchAmt, 5.0f }, { pitchDecay, 15.0f },
            { aAtk, 0.5f }, { aDec, 1200.0f }, { aSus, 0.75f }, { aRel, 200.0f },
            { glideMode, 1 }, { glideTime, 40.0f },
            { satType, 0 }, { satDrive, 0.15f }, { satTone, 9000.0f } }) },

        { "Trap Triangle Sub", eightOhEight ({
            { subWave, 1 }, { subClean, 1 },
            { pitchAmt, 6.0f }, { pitchDecay, 30.0f },
            { aAtk, 0.3f }, { aDec, 2000.0f }, { aSus, 0.7f }, { aRel, 300.0f },
            { glideMode, 1 }, { glideTime, 80.0f }, { satMix, 0.0f } }) },

        // ================= 808s: drill =======================================
        { "Drill Glide", eightOhEight ({
            { pitchAmt, 5.0f }, { pitchDecay, 25.0f },
            { aAtk, 0.2f }, { aDec, 3000.0f }, { aSus, 0.8f }, { aRel, 250.0f },
            { glideMode, 2 }, { glideTime, 220.0f }, { glideCurve, 0 },
            { satType, 3 }, { satDrive, 0.35f }, { satTone, 5500.0f }, { satMix, 0.7f } }) },

        { "UK Drill Slide", eightOhEight ({
            { pitchAmt, 3.0f }, { pitchDecay, 20.0f },
            { aAtk, 0.2f }, { aDec, 3500.0f }, { aSus, 0.85f }, { aRel, 220.0f },
            { glideMode, 2 }, { glideTime, 80.0f }, { glideCurve, 1 },
            { satType, 0 }, { satDrive, 0.45f }, { satBand, 1 }, { satXover, 120.0f }, { satTone, 7000.0f },
            { clickLevel, 0.08f } }) },

        { "NY Drill Bounce", eightOhEight ({
            { pitchAmt, 7.0f }, { pitchDecay, 30.0f },
            { aAtk, 0.2f }, { aDec, 600.0f }, { aSus, 0.3f }, { aRel, 160.0f },
            { glideMode, 2 }, { glideTime, 60.0f }, { glideCurve, 0 },
            { satType, 1 }, { satDrive, 0.45f }, { satTone, 6000.0f }, { clickLevel, 0.12f } }) },

        { "Dark Drill Reese", {
            { osc1Wave, 2.0f }, { osc1Level, 0.35f }, { osc2On, 1 }, { osc2Wave, 2.0f }, { osc2Level, 0.5f },
            { osc2Voices, 7 }, { osc2Detune, 0.5f }, { subLevel, 0.9f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 500.0f }, { fltReso, 0.15f }, { fltEnv, 0.1f }, { fltKey, 0.4f },
            { lfoRate, 0.2f }, { lfoCutoff, 0.15f }, { lfoRetrig, 0 },
            { aAtk, 2.0f }, { aDec, 1200.0f }, { aSus, 0.9f }, { aRel, 250.0f },
            { glideMode, 2 }, { glideTime, 160.0f },
            { satType, 0 }, { satDrive, 0.4f }, { satBand, 1 }, { satXover, 140.0f } } },

        // ================= sub / phone friendly ==============================
        { "Clean Sub",
          { { osc1On, 0 }, { subLevel, 1.0f }, { subOct, 0 }, { subClean, 1 },
            { aAtk, 1.0f }, { aDec, 500.0f }, { aSus, 1.0f }, { aRel, 60.0f },
            { glideMode, 1 }, { glideTime, 50.0f }, { satMix, 0.0f }, { outGain, -2.0f } } },

        { "Phone Speaker Grit",
          { { osc1Wave, 3.0f }, { osc1Pw, 0.35f }, { osc1Level, 0.45f }, { osc1Oct, 1 },
            { subLevel, 0.9f }, { subOct, 1 }, { subClean, 1 },
            { fltType, 1 }, { fltCutoff, 700.0f }, { fltReso, 0.1f }, { fltEnv, 0.25f },
            { satType, 1 }, { satDrive, 0.65f }, { satTone, 4500.0f }, { satMix, 0.9f } } },

        // ================= boom bap / lo-fi ==================================
        { "Lo-Fi Boom Bap",
          { { osc1Wave, 1.0f }, { osc1Level, 0.7f }, { subLevel, 0.6f }, { subOct, 1 },
            { fltType, 0 }, { fltCutoff, 600.0f }, { fltReso, 0.1f }, { fltEnv, 0.2f },
            { aAtk, 3.0f }, { aDec, 700.0f }, { aSus, 0.5f }, { aRel, 180.0f },
            { glideMode, 0 }, { driftAmount, 0.4f }, { velocity, 0.5f },
            { satType, 1 }, { satDrive, 0.3f }, { satTone, 3500.0f } } },

        { "Boom Bap Pluck",
          { { osc1Wave, 2.0f }, { osc1Level, 0.7f }, { subLevel, 0.55f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 220.0f }, { fltReso, 0.2f }, { fltEnv, 0.5f },
            { fDec, 250.0f }, { fSus, 0.0f },
            { aAtk, 1.0f }, { aDec, 500.0f }, { aSus, 0.0f }, { aRel, 150.0f },
            { envMode, 0 }, { glideMode, 0 }, { velocity, 0.6f }, { driftAmount, 0.2f },
            { satType, 0 }, { satDrive, 0.2f } } },

        // ================= west coast ========================================
        { "G-Funk Moog",
          { { osc1Wave, 2.6f }, { osc1Level, 0.75f }, { subLevel, 0.6f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 420.0f }, { fltReso, 0.35f }, { fltEnv, 0.45f }, { fltKey, 0.6f },
            { fAtk, 4.0f }, { fDec, 380.0f }, { fSus, 0.3f }, { fRel, 200.0f },
            { aAtk, 2.0f }, { aDec, 600.0f }, { aSus, 0.8f }, { aRel, 150.0f },
            { glideMode, 2 }, { glideTime, 90.0f }, { glideCurve, 1 },
            { lfoRate, 5.0f }, { lfoPitch, 0.12f },
            { satType, 1 }, { satDrive, 0.25f }, { driftAmount, 0.3f } } },

        { "West Coast Glide",
          { { osc1Wave, 3.0f }, { osc1Pw, 0.3f }, { osc1Level, 0.6f }, { subLevel, 0.7f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 900.0f }, { fltReso, 0.4f }, { fltEnv, 0.25f }, { fltKey, 0.5f },
            { aAtk, 3.0f }, { aDec, 800.0f }, { aSus, 0.85f }, { aRel, 200.0f },
            { glideMode, 2 }, { glideTime, 150.0f }, { glideCurve, 1 },
            { lfoRate, 5.5f }, { lfoPitch, 0.2f }, { lfoRetrig, 0 },
            { satType, 0 }, { satDrive, 0.2f }, { driftAmount, 0.25f } } },

        // ================= reese / wide ======================================
        { "Reese Pressure",
          { { osc1Wave, 2.0f }, { osc1Level, 0.5f }, { osc2On, 1 }, { osc2Wave, 2.0f }, { osc2Level, 0.6f },
            { osc2Voices, 5 }, { osc2Detune, 0.35f }, { subLevel, 0.7f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 900.0f }, { fltReso, 0.2f }, { fltEnv, 0.15f }, { fltKey, 0.3f },
            { lfoRate, 0.3f }, { lfoCutoff, 0.2f }, { lfoRetrig, 0 },
            { aAtk, 3.0f }, { aDec, 800.0f }, { aSus, 0.9f }, { aRel, 180.0f },
            { satType, 0 }, { satDrive, 0.35f }, { satTone, 8000.0f } } },

        { "Grime Square",
          { { osc1Wave, 3.0f }, { osc1Level, 0.6f }, { subLevel, 0.8f }, { subOct, 1 },
            { fltType, 0 }, { fltCutoff, 1200.0f }, { fltReso, 0.1f }, { fltEnv, 0.2f },
            { aAtk, 0.3f }, { aDec, 400.0f }, { aSus, 0.7f }, { aRel, 60.0f },
            { glideMode, 0 }, { envMode, 0 },
            { satType, 2 }, { satDrive, 0.35f }, { satBand, 1 }, { satXover, 150.0f } } },

        // ================= other styles ======================================
        { "Punch Square",
          { { osc1Wave, 3.0f }, { osc1Level, 0.55f }, { subLevel, 0.8f }, { subOct, 1 }, { subWave, 1 },
            { fltType, 0 }, { fltCutoff, 500.0f }, { fltReso, 0.25f }, { fltEnv, 0.5f },
            { fDec, 120.0f }, { fSus, 0.15f },
            { pitchAmt, 24.0f }, { pitchDecay, 12.0f }, { pitchSub, 0 },
            { aAtk, 0.2f }, { aDec, 300.0f }, { aSus, 0.6f }, { aRel, 80.0f },
            { clickLevel, 0.3f }, { clickTone, 0.8f } } },

        { "Rubber Pluck",
          { { osc1Wave, 2.0f }, { subLevel, 0.6f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 180.0f }, { fltReso, 0.5f }, { fltEnv, 0.6f },
            { fDec, 180.0f }, { fSus, 0.0f },
            { aAtk, 0.5f }, { aDec, 350.0f }, { aSus, 0.0f }, { aRel, 120.0f },
            { envMode, 0 }, { glideMode, 0 }, { satDrive, 0.2f } } },

        { "Acid Squelch",
          { { osc1Wave, 2.0f }, { osc1Level, 0.75f }, { subLevel, 0.4f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 250.0f }, { fltReso, 0.75f }, { fltEnv, 0.7f }, { fltKey, 0.5f },
            { fDec, 200.0f }, { fSus, 0.05f },
            { aAtk, 0.5f }, { aDec, 300.0f }, { aSus, 0.8f }, { aRel, 60.0f },
            { glideMode, 1 }, { glideTime, 60.0f }, { envMode, 1 }, { velocity, 0.5f },
            { satType, 0 }, { satDrive, 0.3f } } },

        { "Wobble Lane",
          { { osc1Wave, 2.0f }, { osc2On, 1 }, { osc2Wave, 3.0f }, { osc2Oct, -1 }, { osc2Level, 0.4f },
            { subLevel, 0.75f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 300.0f }, { fltReso, 0.45f }, { fltEnv, 0.0f },
            { lfoSync, 1 }, { lfoDiv, 5 }, { lfoShape, 0 }, { lfoCutoff, 0.6f },
            { satType, 0 }, { satDrive, 0.4f } } },

        { "Synthwave Pulse",
          { { osc1Wave, 3.0f }, { osc1Pw, 0.45f }, { osc1Level, 0.6f }, { osc2On, 1 }, { osc2Wave, 2.0f },
            { osc2Fine, 8.0f }, { osc2Level, 0.35f }, { subLevel, 0.5f }, { subOct, 1 },
            { fltType, 0 }, { fltCutoff, 1200.0f }, { fltReso, 0.2f }, { fltEnv, 0.3f },
            { fDec, 200.0f }, { fSus, 0.3f },
            { aAtk, 1.0f }, { aDec, 300.0f }, { aSus, 0.6f }, { aRel, 120.0f },
            { envMode, 0 }, { glideMode, 0 }, { satType, 1 }, { satDrive, 0.2f } } },

        { "Deep House Pluck",
          { { osc1Wave, 1.0f }, { osc1Level, 0.6f }, { subLevel, 0.8f }, { subOct, 1 },
            { fltType, 1 }, { fltCutoff, 350.0f }, { fltReso, 0.15f }, { fltEnv, 0.35f },
            { fDec, 150.0f }, { fSus, 0.0f },
            { aAtk, 0.5f }, { aDec, 280.0f }, { aSus, 0.0f }, { aRel, 120.0f },
            { envMode, 0 }, { glideMode, 0 }, { velocity, 0.4f }, { satDrive, 0.15f } } },
    };

    return presets;
}

void applyPreset (juce::AudioProcessorValueTreeState& apvts, const FactoryPreset& preset)
{
    for (auto* p : apvts.processor.getParameters())
    {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            // Kick-duck settings belong to the mix and the sample tuning to the
            // loaded sample, so presets leave them alone (the sample layer
            // itself is switched off unless the preset turns it on).
            const auto& id = rp->paramID;
            if (id.startsWith ("duck_") || (id.startsWith ("smp_") && id != ParamID::smpOn))
                continue;
            rp->setValueNotifyingHost (rp->getDefaultValue());
        }
    }

    for (const auto& [id, value] : preset.values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
}
