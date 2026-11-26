#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"
#include "DSP/BassEngine.h"

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

    // GUI read-outs
    ScopeBuffer scope;
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> satActivity { 0.0f };    // how hard the saturator is being hit
    std::atomic<double> uiSampleRate { 44100.0 };
    bdsp::BassEngine& getEngine() noexcept { return engine; }

    // keyboard state for the on-screen keyboard (standalone friendliness)
    juce::MidiKeyboardState keyboardState;

private:
    void updateEngineParams();
    void handleMidi (const juce::MidiMessage& m);
    void processSaturation (float* main, int numSamples);

    // cached raw parameter pointers (audio thread reads only these)
    struct Raw;
    std::unique_ptr<Raw> raw;

    bdsp::BassEngine engine;
    bdsp::EngineParams engineParams;

    juce::dsp::Oversampling<float> oversampler { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };
    juce::AudioBuffer<float> mainBuffer, subBuffer;
    std::vector<float> subDelay;
    int subDelayPos = 0, latency = 0;

    bdsp::Svf toneFilter;
    bdsp::Smoother driveSmooth, mixSmooth, toneSmooth, gainSmooth;
    juce::dsp::IIR::Filter<float> lowCut;
    int lowCutChoice = -1;
    float dcX = 0.0f, dcY = 0.0f, dcCoef = 0.999f;

    double sampleRate = 44100.0;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassurdaProcessor)
};
