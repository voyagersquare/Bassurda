#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "GUI/BassurdaLookAndFeel.h"
#include "GUI/Modules.h"

// Fixed-size design surface; the editor scales it to the window size.
class MainView : public juce::Component, private juce::Timer
{
public:
    static constexpr int designWidth  = 1200;
    static constexpr int designHeight = 1060;

    explicit MainView (BassurdaProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void refreshPresetBox();
    void stepPreset (int delta);
    void showPresetMenu();
    void savePresetDialog();
    void timerCallback() override;

    BassurdaProcessor& proc;
    int lastPresetCounter = -1;
    juce::Array<juce::File> userPresets;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, menuButton { "PRESET" };
    std::unique_ptr<juce::AlertWindow> saveWindow;

    OscModule oscA, oscB;
    SubModule sub;
    ScopeModule scope;
    FilterModule filter;
    EnvModule filterEnv, ampEnv;
    PitchModule pitch;
    VoiceModule voice;
    LfoModule lfo;
    SaturationModule saturation;
    SampleModule sample;
    DuckModule duck;
    OutputModule output;
    juce::MidiKeyboardComponent keyboard;
};

class BassurdaEditor : public juce::AudioProcessorEditor
{
public:
    explicit BassurdaEditor (BassurdaProcessor&);
    ~BassurdaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    BassurdaLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };
    MainView view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassurdaEditor)
};
