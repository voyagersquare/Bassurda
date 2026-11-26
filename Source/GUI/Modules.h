#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"
#include "Displays.h"

using APVTS = juce::AudioProcessorValueTreeState;

// Rotary knob with an engraved name that turns into the live value while
// the mouse is over it or it is being dragged.
class Knob : public juce::Component
{
public:
    Knob (APVTS& state, const juce::String& paramId, juce::String displayName, juce::Colour accent = {});
    void resized() override;
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }

    juce::Slider slider;

private:
    juce::String name;
    APVTS::SliderAttachment attachment;
};

// Row of radio buttons bound to a choice parameter.
class ChoiceButtons : public juce::Component
{
public:
    ChoiceButtons (APVTS& state, const juce::String& paramId, juce::StringArray labels, juce::String caption = {});
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    void update (float value);

    juce::String caption;
    juce::OwnedArray<juce::TextButton> buttons;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

// LED toggle bound to a bool parameter.
class LedToggle : public juce::ToggleButton
{
public:
    LedToggle (APVTS& state, const juce::String& paramId, const juce::String& text, juce::Colour led = juce::Colour (0xffe8392b));
private:
    APVTS::ButtonAttachment attachment;
};

// Base: draws the textured module plate with an engraved title.
class ModulePanel : public juce::Component
{
public:
    explicit ModulePanel (juce::String t) : title (std::move (t)) {}
    void paint (juce::Graphics&) override;

protected:
    juce::Rectangle<int> content() const { return getLocalBounds().reduced (10, 0).withTrimmedTop (26).withTrimmedBottom (12); }
    juce::Rectangle<int> titleBar() const { return getLocalBounds().withHeight (22).reduced (18, 2); }
    static void layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items);

    juce::String title;
};

//==============================================================================
class OscModule : public ModulePanel
{
public:
    OscModule (BassurdaProcessor&, int index);
    void resized() override;
private:
    int index;
    WaveDisplay display;
    LedToggle onSwitch;
    std::unique_ptr<Knob> wave, pw, level, oct, semi, fine, voices, detune;
};

class SubModule : public ModulePanel
{
public:
    explicit SubModule (BassurdaProcessor&);
    void resized() override;
private:
    SubDisplay display;
    Knob level, click, tone;
    ChoiceButtons wave, oct;
    LedToggle clean;
};

class ScopeModule : public ModulePanel
{
public:
    explicit ScopeModule (BassurdaProcessor&);
    void resized() override;
private:
    ScopeDisplay scope;
    MeterDisplay meter;
};

class FilterModule : public ModulePanel
{
public:
    explicit FilterModule (BassurdaProcessor&);
    void resized() override;
private:
    FilterDisplay display;
    ChoiceButtons type;
    Knob cutoff, reso, env, key, drive;
};

class EnvModule : public ModulePanel
{
public:
    EnvModule (BassurdaProcessor&, bool amp);
    void resized() override;
private:
    bool amp;
    EnvelopeDisplay display;
    Knob a, d, s, r;
    std::unique_ptr<Knob> vel;
};

class PitchModule : public ModulePanel
{
public:
    explicit PitchModule (BassurdaProcessor&);
    void resized() override;
private:
    PitchEnvDisplay display;
    Knob amount, decay;
    LedToggle toSub;
};

class VoiceModule : public ModulePanel
{
public:
    explicit VoiceModule (BassurdaProcessor&);
    void resized() override;
private:
    Knob glide, bend, phase, drift;
    ChoiceButtons glideMode, glideCurve, envMode;
    LedToggle retrig;
};

class LfoModule : public ModulePanel
{
public:
    explicit LfoModule (BassurdaProcessor&);
    void resized() override;
private:
    LfoDisplay display;
    ChoiceButtons shape;
    Knob rate, div, toCutoff, toPitch;
    LedToggle sync, retrig;
};

class SaturationModule : public ModulePanel
{
public:
    explicit SaturationModule (BassurdaProcessor&);
    void resized() override;
private:
    SaturationDisplay display;
    ChoiceButtons type;
    Knob drive, tone, mix;
};

class OutputModule : public ModulePanel
{
public:
    explicit OutputModule (BassurdaProcessor&);
    void resized() override;
private:
    Knob gain, lowCut;
    LedToggle clip;
};
