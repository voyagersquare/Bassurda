#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"
#include "DSP/Saturator.h"

using namespace ParamID;

struct BassurdaProcessor::Raw
{
    explicit Raw (juce::AudioProcessorValueTreeState& s) : apvts (s) {}

    std::atomic<float>* get (const char* id)
    {
        auto* p = apvts.getRawParameterValue (id);
        jassert (p != nullptr);
        return p;
    }

    juce::AudioProcessorValueTreeState& apvts;

#define BASSURDA_RAW(name, id) std::atomic<float>* name = get (id);
    BASSURDA_RAW (glideTime_, glideTime)  BASSURDA_RAW (glideMode_, glideMode)  BASSURDA_RAW (glideCurve_, glideCurve)
    BASSURDA_RAW (envMode_, envMode)      BASSURDA_RAW (bendRange_, bendRange)  BASSURDA_RAW (velocity_, velocity)
    BASSURDA_RAW (phaseRetrig_, phaseRetrig) BASSURDA_RAW (phaseStart_, phaseStart) BASSURDA_RAW (drift_, driftAmount)

    BASSURDA_RAW (o1On, osc1On) BASSURDA_RAW (o1Wave, osc1Wave) BASSURDA_RAW (o1Pw, osc1Pw) BASSURDA_RAW (o1Oct, osc1Oct)
    BASSURDA_RAW (o1Semi, osc1Semi) BASSURDA_RAW (o1Fine, osc1Fine) BASSURDA_RAW (o1Level, osc1Level)
    BASSURDA_RAW (o2On, osc2On) BASSURDA_RAW (o2Wave, osc2Wave) BASSURDA_RAW (o2Pw, osc2Pw) BASSURDA_RAW (o2Oct, osc2Oct)
    BASSURDA_RAW (o2Semi, osc2Semi) BASSURDA_RAW (o2Fine, osc2Fine) BASSURDA_RAW (o2Level, osc2Level)
    BASSURDA_RAW (o2Voices, osc2Voices) BASSURDA_RAW (o2Detune, osc2Detune)

    BASSURDA_RAW (subLevel_, subLevel) BASSURDA_RAW (subWave_, subWave) BASSURDA_RAW (subOct_, subOct)
    BASSURDA_RAW (subClean_, subClean) BASSURDA_RAW (clickLevel_, clickLevel) BASSURDA_RAW (clickTone_, clickTone)

    BASSURDA_RAW (pitchAmt_, pitchAmt) BASSURDA_RAW (pitchDecay_, pitchDecay) BASSURDA_RAW (pitchSub_, pitchSub)

    BASSURDA_RAW (fltType_, fltType) BASSURDA_RAW (fltCutoff_, fltCutoff) BASSURDA_RAW (fltReso_, fltReso)
    BASSURDA_RAW (fltEnv_, fltEnv) BASSURDA_RAW (fltKey_, fltKey) BASSURDA_RAW (fltDrive_, fltDrive)

    BASSURDA_RAW (fA, fAtk) BASSURDA_RAW (fD, fDec) BASSURDA_RAW (fS, fSus) BASSURDA_RAW (fR, fRel)
    BASSURDA_RAW (aA, aAtk) BASSURDA_RAW (aD, aDec) BASSURDA_RAW (aS, aSus) BASSURDA_RAW (aR, aRel)

    BASSURDA_RAW (lfoRate_, lfoRate) BASSURDA_RAW (lfoSync_, lfoSync) BASSURDA_RAW (lfoDiv_, lfoDiv)
    BASSURDA_RAW (lfoShape_, lfoShape) BASSURDA_RAW (lfoCutoff_, lfoCutoff) BASSURDA_RAW (lfoPitch_, lfoPitch)
    BASSURDA_RAW (lfoRetrig_, lfoRetrig)

    BASSURDA_RAW (satType_, satType) BASSURDA_RAW (satDrive_, satDrive) BASSURDA_RAW (satTone_, satTone) BASSURDA_RAW (satMix_, satMix)
    BASSURDA_RAW (outGain_, outGain) BASSURDA_RAW (outClip_, outClip) BASSURDA_RAW (lowCut_, monoCut) BASSURDA_RAW (width_, width) BASSURDA_RAW (monoBelow_, monoBelow)
    BASSURDA_RAW (satBand_, satBand) BASSURDA_RAW (satXover_, satXover)
    BASSURDA_RAW (smpOn_, smpOn) BASSURDA_RAW (smpLevel_, smpLevel) BASSURDA_RAW (smpRoot_, smpRoot)
    BASSURDA_RAW (smpTune_, smpTune) BASSURDA_RAW (smpStart_, smpStart) BASSURDA_RAW (smpLoop_, smpLoop)
    BASSURDA_RAW (smpPenv_, smpPenv)
    BASSURDA_RAW (duckMode_, duckMode) BASSURDA_RAW (duckNote_, duckNote) BASSURDA_RAW (duckDepth_, duckDepth)
    BASSURDA_RAW (duckAtk_, duckAttack) BASSURDA_RAW (duckHold_, duckHold) BASSURDA_RAW (duckRel_, duckRelease)
#undef BASSURDA_RAW
};

static inline int   toInt (std::atomic<float>* p)  { return (int) std::lround (p->load (std::memory_order_relaxed)); }
static inline float toF (std::atomic<float>* p)    { return p->load (std::memory_order_relaxed); }
static inline bool  toBool (std::atomic<float>* p) { return p->load (std::memory_order_relaxed) > 0.5f; }

//==============================================================================
BassurdaProcessor::BassurdaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BASSURDA", createParameterLayout())
{
    raw = std::make_unique<Raw> (apvts);
    formatManager.registerBasicFormats();
    for (auto& c : ccMap)
        c = -1;
}

BassurdaProcessor::~BassurdaProcessor() = default;

bool BassurdaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void BassurdaProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    uiSampleRate = newSampleRate;

    engine.prepare (sampleRate);
    updateEngineParams();

    oversampler.reset();
    oversampler.initProcessing ((size_t) samplesPerBlock);
    latency = (int) std::lround (oversampler.getLatencyInSamples());
    setLatencySamples (latency);

    // the clean sub skips the oversampled saturation, so delay it by exactly
    // the same amount to keep it phase-aligned with the rest of the sound
    subDelay.assign ((size_t) std::max (1, latency), 0.0f);
    subDelayPos = 0;

    mainBuffer.setSize (1, samplesPerBlock, false, true, true);
    subBuffer.setSize (1, samplesPerBlock, false, true, true);

    const double osRate = sampleRate * 4.0;
    toneFilter.reset();
    driveSmooth.prepare (osRate, 20.0f);
    mixSmooth.prepare (osRate, 20.0f);
    toneSmooth.prepare (osRate, 20.0f);
    gainSmooth.prepare (sampleRate, 20.0f);
    driveSmooth.snap (toF (raw->satDrive_));
    mixSmooth.snap (toF (raw->satMix_));
    toneSmooth.snap (toF (raw->satTone_));
    gainSmooth.snap (juce::Decibels::decibelsToGain (toF (raw->outGain_)));

    juce::dsp::ProcessSpec osSpec { osRate, (juce::uint32) samplesPerBlock * 4, 1 };
    xoverLow.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    xoverHigh.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
    xoverLow.prepare (osSpec);
    xoverHigh.prepare (osSpec);

    lowCutChoice = -1;
    lowCut.reset();
    dcX = dcY = 0.0f;
    dcCoef = (float) std::exp (-bdsp::twoPi * 5.0 / sampleRate);

    duck.prepare (sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 1 };
    widthSplit.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
    widthSplit.prepare (spec);
    widthDelay.assign ((size_t) std::max (1, (int) std::lround (sampleRate * 0.0071)), 0.0f);
    widthDelayPos = 0;
    widthSmooth.prepare (sampleRate, 30.0f);
    widthSmooth.snap (toF (raw->width_));
    midScratch.assign ((size_t) samplesPerBlock, 0.0f);
}

void BassurdaProcessor::updateEngineParams()
{
    auto& p = engineParams;
    auto& r = *raw;

    p.glideMs     = toF (r.glideTime_);
    p.glideMode   = toInt (r.glideMode_);
    p.glideCurve  = toInt (r.glideCurve_);
    p.envMode     = toInt (r.envMode_);
    p.bendRange   = toInt (r.bendRange_);
    p.velSens     = toF (r.velocity_);
    p.phaseRetrig = toBool (r.phaseRetrig_);
    p.phaseStart  = toF (r.phaseStart_) / 360.0f;
    p.drift       = toF (r.drift_);

    p.osc1 = { toBool (r.o1On), toF (r.o1Wave), toF (r.o1Pw), toInt (r.o1Oct), toInt (r.o1Semi), toF (r.o1Fine), toF (r.o1Level) };
    p.osc2 = { toBool (r.o2On), toF (r.o2Wave), toF (r.o2Pw), toInt (r.o2Oct), toInt (r.o2Semi), toF (r.o2Fine), toF (r.o2Level) };
    p.osc2Voices = toInt (r.o2Voices);
    p.osc2Detune = toF (r.o2Detune);

    p.subLevel   = toF (r.subLevel_);
    p.subWave    = toInt (r.subWave_);
    p.subOct     = toInt (r.subOct_);
    p.subClean   = toBool (r.subClean_);
    p.clickLevel = toF (r.clickLevel_);
    p.clickTone  = toF (r.clickTone_);

    p.pitchAmt     = toF (r.pitchAmt_);
    p.pitchDecayMs = toF (r.pitchDecay_);
    p.pitchToSub   = toBool (r.pitchSub_);

    p.fltType  = toInt (r.fltType_);
    p.cutoff   = toF (r.fltCutoff_);
    p.reso     = toF (r.fltReso_);
    p.fltEnv   = toF (r.fltEnv_);
    p.keyTrack = toF (r.fltKey_);
    p.fltDrive = toF (r.fltDrive_);

    p.filterEnv = { toF (r.fA), toF (r.fD), toF (r.fS), toF (r.fR) };
    p.ampEnv    = { toF (r.aA), toF (r.aD), toF (r.aS), toF (r.aR) };

    float rate = toF (r.lfoRate_);
    if (toBool (r.lfoSync_))
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm())
                    bpm = *b;
        rate = (float) (bpm / 60.0 / lfoDivisionInBeats (toInt (r.lfoDiv_)));
    }
    p.lfoRateHz   = rate;
    p.lfoShape    = toInt (r.lfoShape_);
    p.lfoToCutoff = toF (r.lfoCutoff_);
    p.lfoToPitch  = toF (r.lfoPitch_);
    p.lfoRetrig   = toBool (r.lfoRetrig_);

    p.smpOn    = toBool (r.smpOn_);
    p.smpLevel = toF (r.smpLevel_);
    p.smpRoot  = toInt (r.smpRoot_);
    p.smpTune  = toF (r.smpTune_);
    p.smpStart = toF (r.smpStart_);
    p.smpLoop  = toBool (r.smpLoop_);
    p.smpPenv  = toBool (r.smpPenv_);

    engine.setParams (p);

    duck.setParams (toF (r.duckDepth_), toF (r.duckAtk_), toF (r.duckHold_), toF (r.duckRel_));
}

void BassurdaProcessor::handleMidi (const juce::MidiMessage& m, int samplePos)
{
    // ---- kick duck triggers (these notes never reach the synth) ------------
    const int mode = toInt (raw->duckMode_);
    if (m.isNoteOnOrOff())
    {
        const bool isTrigger = (mode == 1 && m.getNoteNumber() == toInt (raw->duckNote_))
                            || (mode == 2 && m.getChannel() == 10);
        if (isTrigger)
        {
            // the audio is delayed by the oversampling latency (and the host
            // compensates for it), so delay the duck by the same amount
            if (m.isNoteOn())
                duck.schedule (samplePos + latency);
            return;
        }
    }

    // ---- MIDI learn / mapped CCs -------------------------------------------
    if (m.isController())
    {
        const int cc = m.getControllerNumber();
        if (const int learn = learnParam.exchange (-1); learn >= 0)
        {
            for (auto& c : ccMap)
                if (c.load() == learn)
                    c = -1;
            ccMap[(size_t) cc] = learn;
        }

        const int target = ccMap[(size_t) cc].load();
        auto& params = getParameters();
        if (juce::isPositiveAndBelow (target, params.size()))
        {
            params[target]->setValueNotifyingHost ((float) m.getControllerValue() / 127.0f);
            return;
        }
    }

    if (m.isNoteOn())
        engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())
        engine.pitchBend (m.getPitchWheelValue());
    else if (m.isAllNotesOff())
        engine.allNotesOff (false);
    else if (m.isAllSoundOff())
        engine.allNotesOff (true);
}

void BassurdaProcessor::scheduleBeatDucks (int numSamples)
{
    auto* ph = getPlayHead();
    if (ph == nullptr) return;
    auto pos = ph->getPosition();
    if (! pos || ! pos->getIsPlaying()) return;
    auto ppq = pos->getPpqPosition();
    auto bpm = pos->getBpm();
    if (! ppq || ! bpm || *bpm <= 0.0) return;

    const double ppqPerSample = *bpm / 60.0 / sampleRate;
    for (double beat = std::ceil (*ppq); ; beat += 1.0)
    {
        const int offset = (int) std::lround ((beat - *ppq) / ppqPerSample);
        if (offset >= numSamples) break;
        duck.schedule (offset + latency);
    }
}

void BassurdaProcessor::processSaturation (float* main, int numSamples)
{
    const int  type    = toInt (raw->satType_);
    const bool highsOnly = toInt (raw->satBand_) == 1;
    driveSmooth.setTarget (toF (raw->satDrive_));
    mixSmooth.setTarget (toF (raw->satMix_));
    toneSmooth.setTarget (toF (raw->satTone_));
    xoverLow.setCutoffFrequency (toF (raw->satXover_));
    xoverHigh.setCutoffFrequency (toF (raw->satXover_));

    float* channels[] = { main };
    juce::dsp::AudioBlock<float> block (channels, 1, (size_t) numSamples);
    auto osBlock = oversampler.processSamplesUp (block);
    float* os = osBlock.getChannelPointer (0);
    const int osN = (int) osBlock.getNumSamples();
    const float osRate = (float) (sampleRate * 4.0);

    float activity = 0.0f;
    for (int i = 0; i < osN; ++i)
    {
        const float drive = driveSmooth.next();
        const float mix   = mixSmooth.next();
        const float tone  = toneSmooth.next();

        const float dg     = bdsp::Saturator::driveToGain (drive);
        const float makeup = bdsp::Saturator::makeupGain (type, dg);

        const float dry  = os[i];
        float low = 0.0f, input = dry;
        if (highsOnly)
        {
            // Linkwitz-Riley split: the lows stay clean, only the highs are
            // driven. LP + HP of a LR4 pair sum back to flat magnitude.
            low   = xoverLow.processSample (0, dry);
            input = xoverHigh.processSample (0, dry);
        }

        const float driven = input * dg;
        activity = std::max (activity, std::abs (driven));
        float wet = (bdsp::Saturator::shape (type, driven) - bdsp::Saturator::shape (type, 0.0f)) * makeup;

        // tone: gentle 12 dB/oct low-pass on the wet signal tames fizz
        const float g = std::tan (bdsp::pi * std::min (tone, osRate * 0.45f) / osRate);
        toneFilter.process (wet, g, 1.4142f);
        wet = toneFilter.low;

        os[i] = low + input + mix * (wet - input);
    }

    oversampler.processSamplesDown (block);
    satActivity = activity;
}

void BassurdaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    updateEngineParams();

    if (mainBuffer.getNumSamples() < numSamples)
    {
        // host sent a bigger block than announced: stay safe
        mainBuffer.setSize (1, numSamples, false, false, true);
        subBuffer.setSize (1, numSamples, false, false, true);
        oversampler.initProcessing ((size_t) numSamples);
    }

    float* main = mainBuffer.getWritePointer (0);
    float* sub  = subBuffer.getWritePointer (0);

    if (toInt (raw->duckMode_) == 3)
        scheduleBeatDucks (numSamples);

    // ---- voice: sample-accurate MIDI -----------------------------------------
    {
        // never wait for the message thread: if a new sample is being swapped
        // in right now, render this block without the sample layer
        juce::SpinLock::ScopedTryLockType lock (sampleLock);
        engine.setSample (lock.isLocked() ? sampleData.get() : nullptr);

        int pos = 0;
        for (const auto metadata : midi)
        {
            const int eventPos = juce::jlimit (0, numSamples, metadata.samplePosition);
            if (eventPos > pos)
            {
                engine.render (main + pos, sub + pos, eventPos - pos);
                pos = eventPos;
            }
            handleMidi (metadata.getMessage(), eventPos);
        }
        if (pos < numSamples)
            engine.render (main + pos, sub + pos, numSamples - pos);

        engine.setSample (nullptr);
    }

    // ---- saturation (4x oversampled) -------------------------------------------
    processSaturation (main, numSamples);

    // ---- low cut setup ----------------------------------------------------------
    const int lc = toInt (raw->lowCut_);
    if (lc != lowCutChoice)
    {
        lowCutChoice = lc;
        static const float freqs[] = { 0.0f, 20.0f, 30.0f, 40.0f };
        if (lc > 0)
            lowCut.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, freqs[lc], 0.7071f);
        lowCut.reset();
    }

    // ---- sum, clean up, duck, gain, width, clip ------------------------------
    gainSmooth.setTarget (juce::Decibels::decibelsToGain (toF (raw->outGain_)));
    const bool clip = toBool (raw->outClip_);
    const bool ducking = toInt (raw->duckMode_) != 0;
    const bool stereo = buffer.getNumChannels() > 1;
    widthSmooth.setTarget (stereo ? toF (raw->width_) : 0.0f);
    widthSplit.setCutoffFrequency (toF (raw->monoBelow_));
    if ((int) midScratch.size() < numSamples)
        midScratch.resize ((size_t) numSamples);

    auto softClip = [] (float v)
    {
        // soft knee clipper: linear up to -2 dB, smooth ceiling at 0 dBFS
        constexpr float t = 0.8f;
        const float a = std::abs (v);
        return a > t ? std::copysign (t + (1.0f - t) * std::tanh ((a - t) / (1.0f - t)), v) : v;
    };

    float peak = 0.0f, minDuck = 1.0f;
    float* left  = main;
    float* right = sub;      // the sub buffer is reused for the right channel

    for (int i = 0; i < numSamples; ++i)
    {
        // delay clean sub by the oversampling latency
        float s = sub[i];
        if (latency > 0)
        {
            const float delayed = subDelay[(size_t) subDelayPos];
            subDelay[(size_t) subDelayPos] = s;
            subDelayPos = (subDelayPos + 1) % latency;
            s = delayed;
        }

        float x = main[i] + s;

        // DC blocker (asymmetric saturation creates DC)
        const float y = x - dcX + dcCoef * dcY;
        dcX = x;
        dcY = y;
        x = y;

        if (lowCutChoice > 0)
            x = lowCut.processSample (x);

        const float dg = duck.next();
        if (ducking)
        {
            x *= dg;
            minDuck = std::min (minDuck, dg);
        }

        x *= gainSmooth.next();

        // Stereo width: a delayed copy of the highs is added to the left and
        // subtracted from the right. It cancels completely in mono, and
        // everything below "mono below" stays dead centre.
        const float high = widthSplit.processSample (0, x);
        const float delayedHigh = widthDelay[(size_t) widthDelayPos];
        widthDelay[(size_t) widthDelayPos] = high;
        widthDelayPos = (widthDelayPos + 1) % (int) widthDelay.size();
        float side = delayedHigh * widthSmooth.next() * 0.8f;

        // clip the mid only, and let the side use just the headroom left, so
        // the mono sum is exactly the same as with zero width
        if (clip)
        {
            x = softClip (x);
            const float headroom = std::max (0.0f, 0.999f - std::abs (x));
            side = juce::jlimit (-headroom, headroom, side);
        }
        const float l = x + side, r = x - side;

        left[i]  = l;
        right[i] = r;
        midScratch[(size_t) i] = 0.5f * (l + r);
        peak = std::max (peak, std::max (std::abs (l), std::abs (r)));
    }

    if (stereo)
    {
        buffer.copyFrom (0, 0, left, numSamples);
        buffer.copyFrom (1, 0, right, numSamples);
        for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
            buffer.clear (ch, 0, numSamples);
    }
    else if (buffer.getNumChannels() == 1)
    {
        buffer.copyFrom (0, 0, left, numSamples);
    }

    scope.push (midScratch.data(), numSamples);
    outputPeak = std::max (peak, outputPeak.load() * 0.9f);
    duckGain = minDuck;
}

//==============================================================================
// Factory presets (host program list)
//==============================================================================
int BassurdaProcessor::getNumPrograms()
{
    return (int) getFactoryPresets().size();
}

void BassurdaProcessor::setCurrentProgram (int index)
{
    const auto& presets = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;

    currentProgram = index;
    applyPreset (apvts, presets[(size_t) index]);
    currentPresetName = presets[(size_t) index].name;
    ++presetChangeCounter;
}

const juce::String BassurdaProcessor::getProgramName (int index)
{
    const auto& presets = getFactoryPresets();
    return juce::isPositiveAndBelow (index, (int) presets.size()) ? presets[(size_t) index].name : juce::String();
}

//==============================================================================
// User presets
//==============================================================================
juce::File BassurdaProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Bassurda").getChildFile ("Presets");
}

juce::File BassurdaProcessor::getUserSampleFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Bassurda").getChildFile ("Samples");
}

juce::File BassurdaProcessor::copyIntoSampleFolder (const juce::File& file)
{
    auto folder = getUserSampleFolder();
    if (file.isAChildOf (folder) || ! folder.createDirectory())
        return file;

    // reuse an identical copy; never overwrite a different file of the same name
    auto target = folder.getChildFile (file.getFileName());
    if (target.existsAsFile() && target.getSize() == file.getSize() && target.hasIdenticalContentTo (file))
        return target;
    if (target.existsAsFile())
        target = folder.getNonexistentChildFile (file.getFileNameWithoutExtension(), file.getFileExtension(), false);

    return file.copyFileTo (target) ? target : file;
}

juce::Array<juce::File> BassurdaProcessor::getUserPresets() const
{
    auto files = getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.bassurda");
    files.sort();
    return files;
}

bool BassurdaProcessor::saveUserPreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty())
        return false;

    auto folder = getUserPresetFolder();
    if (! folder.createDirectory())
        return false;

    currentPresetName = name.trim();
    auto tree = createStateTree (false);
    auto xml = tree.createXml();
    if (xml == nullptr || ! xml->writeTo (folder.getChildFile (clean + ".bassurda")))
        return false;

    ++presetChangeCounter;
    return true;
}

bool BassurdaProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    restoreStateTree (juce::ValueTree::fromXml (*xml), false);
    currentPresetName = file.getFileNameWithoutExtension();
    currentProgram = -1;
    ++presetChangeCounter;
    return true;
}

//==============================================================================
// Sample oscillator
//==============================================================================
bool BassurdaProcessor::loadSample (const juce::File& file, bool autoTune)
{
    const auto source = copyIntoSampleFolder (file);
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (source));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;

    const int maxLen = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 30.0));
    juce::AudioBuffer<float> buf ((int) reader->numChannels, maxLen);
    reader->read (&buf, 0, maxLen, 0, true, true);

    auto data = std::make_unique<bdsp::SampleData>();
    data->sampleRate = reader->sampleRate;
    data->samples.assign ((size_t) maxLen, 0.0f);
    const float chScale = 1.0f / (float) std::max (1, buf.getNumChannels());
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < maxLen; ++i)
            data->samples[(size_t) i] += buf.getSample (ch, i) * chScale;

    // normalise to -1 dBFS so every sample sits at the same level
    float pk = 0.0f;
    for (auto v : data->samples) pk = std::max (pk, std::abs (v));
    if (pk > 1.0e-6f)
        for (auto& v : data->samples) v *= 0.89f / pk;

    data->computePeaks();
    data->detectPitch();

    if (autoTune && data->detectedNote > 0.0f)
    {
        const int root = juce::jlimit (0, 127, (int) std::lround (data->detectedNote));
        const float cents = juce::jlimit (-100.0f, 100.0f, (data->detectedNote - (float) root) * 100.0f);
        auto set = [this] (const char* id, float v)
        {
            if (auto* p = apvts.getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 (v));
        };
        set (smpRoot, (float) root);
        set (smpTune, cents);
    }
    if (autoTune)
        if (auto* p = apvts.getParameter (smpOn))
            p->setValueNotifyingHost (1.0f);

    std::unique_ptr<bdsp::SampleData> old;
    {
        const juce::SpinLock::ScopedLockType lock (sampleLock);
        old = std::move (sampleData);
        sampleData = std::move (data);
    }
    sampleFile = source;
    ++sampleChangeCounter;
    return true;
}

void BassurdaProcessor::clearSample()
{
    std::unique_ptr<bdsp::SampleData> old;
    {
        const juce::SpinLock::ScopedLockType lock (sampleLock);
        old = std::move (sampleData);
    }
    sampleFile = juce::File();
    ++sampleChangeCounter;
}

//==============================================================================
// MIDI learn
//==============================================================================
int BassurdaProcessor::paramIndex (const juce::String& id) const
{
    const auto& params = getParameters();
    for (int i = 0; i < params.size(); ++i)
        if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*> (params[i]))
            if (p->paramID == id)
                return i;
    return -1;
}

void BassurdaProcessor::startMidiLearn (const juce::String& paramId) { learnParam = paramIndex (paramId); }

bool BassurdaProcessor::isLearning (const juce::String& paramId) const
{
    const int l = learnParam.load();
    return l >= 0 && l == paramIndex (paramId);
}

void BassurdaProcessor::clearMidiMapping (const juce::String& paramId)
{
    const int idx = paramIndex (paramId);
    for (auto& c : ccMap)
        if (c.load() == idx)
            c = -1;
}

int BassurdaProcessor::getMidiCCFor (const juce::String& paramId) const
{
    const int idx = paramIndex (paramId);
    if (idx < 0) return -1;
    for (int cc = 0; cc < 128; ++cc)
        if (ccMap[(size_t) cc].load() == idx)
            return cc;
    return -1;
}

//==============================================================================
// State
//==============================================================================
juce::ValueTree BassurdaProcessor::createStateTree (bool includeMidiMap)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("presetName", currentPresetName, nullptr);
    state.setProperty ("samplePath", sampleFile.getFullPathName(), nullptr);

    if (includeMidiMap)
    {
        juce::StringArray map;
        const auto& params = getParameters();
        for (int cc = 0; cc < 128; ++cc)
        {
            const int idx = ccMap[(size_t) cc].load();
            if (juce::isPositiveAndBelow (idx, params.size()))
                if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*> (params[idx]))
                    map.add (juce::String (cc) + "=" + p->paramID);
        }
        state.setProperty ("midiMap", map.joinIntoString (";"), nullptr);
    }
    return state;
}

void BassurdaProcessor::restoreStateTree (const juce::ValueTree& tree, bool includeMidiMap)
{
    apvts.replaceState (tree);
    currentProgram    = tree.getProperty ("program", -1);
    currentPresetName = tree.getProperty ("presetName", "Init Bass").toString();

    const auto path = tree.getProperty ("samplePath").toString();
    juce::File file (juce::File::isAbsolutePath (path) ? path : juce::String());
    // the preset may come from another computer: look for the sample by name
    // in this computer's Bassurda sample folder
    if (path.isNotEmpty() && ! file.existsAsFile())
        file = getUserSampleFolder().getChildFile (juce::File::createFileWithoutCheckingPath (path).getFileName());

    if (path.isEmpty())
        clearSample();
    else if (file.existsAsFile() && file != sampleFile)
        loadSample (file, false);
    else if (! file.existsAsFile())
        clearSample();   // missing sample: better silence than the wrong sound

    if (includeMidiMap)
    {
        for (auto& c : ccMap)
            c = -1;
        for (const auto& entry : juce::StringArray::fromTokens (tree.getProperty ("midiMap").toString(), ";", ""))
        {
            const int cc = entry.upToFirstOccurrenceOf ("=", false, false).getIntValue();
            const int idx = paramIndex (entry.fromFirstOccurrenceOf ("=", false, false));
            if (juce::isPositiveAndBelow (cc, 128) && idx >= 0)
                ccMap[(size_t) cc] = idx;
        }
    }
    ++presetChangeCounter;
}

void BassurdaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = createStateTree (true).createXml())
        copyXmlToBinary (*xml, destData);
}

void BassurdaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            restoreStateTree (juce::ValueTree::fromXml (*xml), true);
}

juce::AudioProcessorEditor* BassurdaProcessor::createEditor()
{
    return new BassurdaEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassurdaProcessor();
}
