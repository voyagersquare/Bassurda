#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"
#include "DSP/BassEngine.h"
#include "DSP/SampleData.h"

// Lock-free single-producer ring buffer that the GUI scope reads from.
struct ScopeBuffer
{
    static constexpr int size = 8192;
    std::array<float, size> data {};
    std::atomic<int> writePos { 0 };

    void push (const float* samples, int num) noexcept
    {
        int w = writePos.load (std::memory_order_relaxed);
        for (int i = 0; i < num; ++i)
        {
            data[(size_t) w] = samples[i];
            w = (w + 1) & (size - 1);
        }
        writePos.store (w, std::memory_order_release);
    }

    // Copies the most recent `num` samples (oldest first).
    void read (float* dest, int num) const noexcept
    {
        const int w = writePos.load (std::memory_order_acquire);
        int r = (w - num + size) & (size - 1);
        for (int i = 0; i < num; ++i)
        {
            dest[i] = data[(size_t) r];
            r = (r + 1) & (size - 1);
        }
    }
};

// Sidechain-style volume ducking triggered by MIDI notes or the host beat.
struct KickDuck
{
    void prepare (double sr) { sampleRate = sr; gain = 1.0f; stage = 0; numPending = 0; }

    void setParams (float depth, float attackMs, float holdMs, float releaseMs)
    {
        floorGain    = 1.0f - depth;
        attackSamps  = std::max (1.0f, (float) (attackMs * 0.001 * sampleRate));
        holdSamps    = std::max (0.0f, (float) (holdMs * 0.001 * sampleRate));
        releaseSamps = std::max (1.0f, (float) (releaseMs * 0.001 * sampleRate));
    }

    // schedule a trigger `delay` samples from the start of the current block
    void schedule (int delay)
    {
        if (numPending < (int) pending.size())
            pending[(size_t) numPending++] = delay;
    }

    // advance one sample (call once per sample, in order)
    inline float next() noexcept
    {
        for (int i = 0; i < numPending; ++i)
        {
            if (pending[(size_t) i] == 0)
            {
                stage = 1; t = 0.0f; startGain = gain;
                pending[(size_t) i] = pending[(size_t) --numPending];
                --i;
            }
            else
            {
                --pending[(size_t) i];
            }
        }

        switch (stage)
        {
            case 1:
                gain = startGain + (floorGain - startGain) * std::min (1.0f, t / attackSamps);
                if (++t >= attackSamps) { stage = 2; t = 0.0f; }
                break;
            case 2:
                gain = floorGain;
                if (++t >= holdSamps) { stage = 3; t = 0.0f; }
                break;
            case 3:
            {
                const float x = std::min (1.0f, t / releaseSamps);
                const float curve = 1.0f - (1.0f - x) * (1.0f - x);
                gain = floorGain + (1.0f - floorGain) * curve;
                if (++t >= releaseSamps) { stage = 0; gain = 1.0f; }
                break;
            }
            default:
                gain = 1.0f;
                break;
        }
        return gain;
    }

    double sampleRate = 44100.0;
    float gain = 1.0f, startGain = 1.0f, floorGain = 0.2f, t = 0.0f;
    float attackSamps = 48.0f, holdSamps = 0.0f, releaseSamps = 4800.0f;
    int stage = 0;
    std::array<int, 64> pending {};
    int numPending = 0;
};

class BassurdaProcessor : public juce::AudioProcessor
{
public:
    BassurdaProcessor();
    ~BassurdaProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Bassurda"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // ---- user presets (message thread) ------------------------------------
    static juce::File getUserPresetFolder();
    static juce::File getUserSampleFolder();
    juce::Array<juce::File> getUserPresets() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& file);
    juce::String getCurrentPresetName() const { return currentPresetName; }
    std::atomic<int> presetChangeCounter { 0 };

    // ---- sample oscillator (message thread) ---------------------------------
    // Loads a one-shot. A sample from outside Documents/Bassurda/Samples is
    // copied there first, so presets and projects keep working even if the
    // original file is moved or deleted.
    bool loadSample (const juce::File& file, bool autoTune = true);
    void clearSample();
    const bdsp::SampleData* getSampleForDisplay() const { return sampleData.get(); }
    juce::String getSampleName() const { return sampleFile.getFileNameWithoutExtension(); }
    std::atomic<int> sampleChangeCounter { 0 };

    // ---- MIDI learn --------------------------------------------------------
    void startMidiLearn (const juce::String& paramId);
    void cancelMidiLearn() { learnParam = -1; }
    void clearMidiMapping (const juce::String& paramId);
    int  getMidiCCFor (const juce::String& paramId) const;
    bool isLearning (const juce::String& paramId) const;

    // ---- GUI read-outs ------------------------------------------------------
    ScopeBuffer scope;
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> satActivity { 0.0f };
    std::atomic<float> duckGain { 1.0f };
    std::atomic<double> uiSampleRate { 44100.0 };
    bdsp::BassEngine& getEngine() noexcept { return engine; }

    juce::MidiKeyboardState keyboardState;

private:
    void updateEngineParams();
    void handleMidi (const juce::MidiMessage& m, int samplePos);
    void processSaturation (float* main, int numSamples);
    void scheduleBeatDucks (int numSamples);
    int  paramIndex (const juce::String& id) const;
    juce::ValueTree createStateTree (bool includeMidiMap);
    static juce::File copyIntoSampleFolder (const juce::File& file);
    void restoreStateTree (const juce::ValueTree& tree, bool includeMidiMap);

    struct Raw;
    std::unique_ptr<Raw> raw;

    bdsp::BassEngine engine;
    bdsp::EngineParams engineParams;

    // sample (swapped on the message thread under the lock)
    std::unique_ptr<bdsp::SampleData> sampleData;
    juce::File sampleFile;
    juce::SpinLock sampleLock;
    juce::AudioFormatManager formatManager;

    // MIDI learn: CC number -> parameter index (or -1)
    std::array<std::atomic<int>, 128> ccMap;
    std::atomic<int> learnParam { -1 };

    juce::dsp::Oversampling<float> oversampler { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };
    juce::AudioBuffer<float> mainBuffer, subBuffer;
    std::vector<float> subDelay;
    int subDelayPos = 0, latency = 0;

    bdsp::Svf toneFilter;
    bdsp::Smoother driveSmooth, mixSmooth, toneSmooth, gainSmooth;
    juce::dsp::LinkwitzRileyFilter<float> xoverLow, xoverHigh;
    juce::dsp::IIR::Filter<float> lowCut;
    int lowCutChoice = -1;
    float dcX = 0.0f, dcY = 0.0f, dcCoef = 0.999f;

    KickDuck duck;

    juce::dsp::LinkwitzRileyFilter<float> widthSplit;
    std::vector<float> widthDelay, midScratch;
    int widthDelayPos = 0;
    bdsp::Smoother widthSmooth;

    double sampleRate = 44100.0;
    int currentProgram = 0;
    juce::String currentPresetName { "Init Bass" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassurdaProcessor)
};
