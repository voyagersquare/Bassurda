#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "GUI/BassurdaLookAndFeel.h"
#include "GUI/Modules.h"

// Fixed-size design surface; the editor scales it to the window size.
class MainView : public juce::Component
{
public:
    static constexpr int designWidth  = 1200;
    static constexpr int designHeight = 846;

    explicit MainView (BassurdaProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void refreshPresetBox();
    void stepPreset (int delta);

    BassurdaProcessor& proc;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };

    OscModule oscA, oscB;
    SubModule sub;
    ScopeModule scope;
    FilterModule filter;
    EnvModule filterEnv, ampEnv;
    PitchModule pitch;
    VoiceModule voice;
    LfoModule lfo;
    SaturationModule saturation;
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
