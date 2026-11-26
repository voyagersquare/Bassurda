#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"
#include "Displays.h"

using APVTS = juce::AudioProcessorValueTreeState;

// Slider that hands right-clicks to a callback instead of dragging.
class LearnSlider : public juce::Slider
{
public:
    std::function<void()> onRightClick;
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick(); return; }
        Slider::mouseDown (e);
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (! e.mods.isPopupMenu()) Slider::mouseDrag (e); }
    void mouseUp (const juce::MouseEvent& e) override   { if (! e.mods.isPopupMenu()) Slider::mouseUp (e); }
};

// Rotary knob with an engraved name that turns into the live value while
// the mouse is over it or it is being dragged. Right-click: MIDI learn.
class Knob : public juce::Component, private juce::Timer
{
public:
    Knob (APVTS& state, const juce::String& paramId, juce::String displayName, juce::Colour accent = {});
    void resized() override;
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }

    LearnSlider slider;

private:
    void showMenu();
    void timerCallback() override;

    juce::String name, paramId;
    BassurdaProcessor* proc = nullptr;
    int blink = 0;
    APVTS::SliderAttachment attachment;
};

// Row of radio buttons bound to a choice parameter.
class ChoiceButtons : public juce::Component
{
public:
    ChoiceButtons (APVTS& state, const juce::String& paramId, juce::StringArray labels, juce::String caption = {});
    void resized() override;
    void setTooltip (const juce::String& tip) { for (auto* b : buttons) b->setTooltip (tip); }
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
    ChoiceButtons type, band;
    Knob drive, tone, mix, xover;
};

class SampleModule : public ModulePanel
{
public:
    explicit SampleModule (BassurdaProcessor&);
    void resized() override;
private:
    BassurdaProcessor& proc;
    SampleDisplay display;
    juce::TextButton loadButton { "LOAD" }, clearButton { "CLEAR" };
    LedToggle on, loop, penv;
    Knob level, root, tune, start;
    std::unique_ptr<juce::FileChooser> chooser;
};

class DuckModule : public ModulePanel
{
public:
    explicit DuckModule (BassurdaProcessor&);
    void resized() override;
private:
    DuckDisplay display;
    ChoiceButtons mode;
    Knob note, depth, attack, hold, release;
};

class OutputModule : public ModulePanel
{
public:
    explicit OutputModule (BassurdaProcessor&);
    void resized() override;
private:
    Knob gain, lowCut, width, monoBelow;
    LedToggle clip;
};
