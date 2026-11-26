#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"

// Base for every LCD screen: draws the bezel, grid and glass, polls a
// signature of the values it shows and only repaints when it changes.
class LcdDisplay : public juce::Component, private juce::Timer
{
public:
    LcdDisplay (BassurdaProcessor& p, juce::String footerText);
    void paint (juce::Graphics&) override;

    void setFooter (juce::String text) { footer = std::move (text); repaint(); }

protected:
    virtual void drawContent (juce::Graphics& g, juce::Rectangle<float> screen) = 0;
    virtual float signature() { return 0.0f; }     // repaint when this changes
    virtual bool alwaysAnimate() const { return false; }

    float value (const char* id) const;
    void drawGrid (juce::Graphics& g, juce::Rectangle<float> r, int cols, int rows) const;
    void drawTag (juce::Graphics& g, juce::Rectangle<float> screen, const juce::String& text,
                  juce::Justification just, juce::Colour colour = juce::Colours::transparentBlack) const;

    BassurdaProcessor& proc;
    juce::String footer;

private:
    void timerCallback() override;
    float lastSignature = -1.0e9f;
};

//==============================================================================
class WaveDisplay : public LcdDisplay
{
public:
    WaveDisplay (BassurdaProcessor& p, int oscIndex);
protected:
    void drawContent (juce::Graphics&, juce::Rectangle<float>) override;
    float signature() override;
private:
    int osc;
};

class SubDisplay : public LcdDisplay
{
public:
    explicit SubDisplay (BassurdaProcessor& p) : LcdDisplay (p, "SUB + CLICK") {}
protected:
    void drawContent (juce::Graphics&, juce::Rectangle<float>) override;
    float signature() override;
};

class EnvelopeDisplay : public LcdDisplay
{
public:
    EnvelopeDisplay (BassurdaProcessor& p, bool isAmp);
protected:
    void drawContent (juce::Graphics&, juce::Rectangle<float>) override;
    float signature() override;
private:
    bool amp;
};

class PitchEnvDisplay : public LcdDisplay
{
public:
    explicit PitchEnvDisplay (BassurdaProcessor& p) : LcdDisplay (p, "PITCH DROP") {}
protected:
    void drawContent (juce::Graphics&, juce::Rectangle<float>) override;
    float signature() override;
};

class FilterDisplay : public LcdDisplay
{
public:
    explicit FilterDisplay (BassurdaProcessor& p) : LcdDisplay (p, "FILTER") {}
protected:
    void drawContent (juce::Graphics&, juce::Rectangle<float>) override;
    float signature() override;
};

class SaturationDisplay : public LcdDisplay
{
public:
    explicit SaturationDisplay (BassurdaProcessor& p) : LcdDisplay (p, "SATURATION") {}
protected:
    void drawContent (juce::Graphics&, juce::Rectangle<float>) override;
    float signature() override;
};

class LfoDisplay : public LcdDisplay
{
public:
    explicit LfoDisplay (BassurdaProcessor& p) : LcdDisplay (p, "LFO") {}
protected:
    void drawContent (juce::Graphics&, juce::Rectangle<float>) override;
    float signature() override;
};

class ScopeDisplay : public LcdDisplay
{
public:
    explicit ScopeDisplay (BassurdaProcessor& p) : LcdDisplay (p, "OUTPUT SCOPE") {}
protected:
    void drawContent (juce::Graphics&, juce::Rectangle<float>) override;
    bool alwaysAnimate() const override { return true; }
private:
    std::array<float, 4096> buffer {};
};

class MeterDisplay : public LcdDisplay
{
public:
    explicit MeterDisplay (BassurdaProcessor& p) : LcdDisplay (p, {}) {}
protected:
    void drawContent (juce::Graphics&, juce::Rectangle<float>) override;
    bool alwaysAnimate() const override { return true; }
private:
    float held = 0.0f, smoothed = 0.0f;
};

class SampleDisplay : public LcdDisplay, public juce::FileDragAndDropTarget
{
public:
    explicit SampleDisplay (BassurdaProcessor& p) : LcdDisplay (p, "808 SAMPLE") {}
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
protected:
    void drawContent (juce::Graphics&, juce::Rectangle<float>) override;
    float signature() override;
private:
    bool dragOver = false;
};

class DuckDisplay : public LcdDisplay
{
public:
    explicit DuckDisplay (BassurdaProcessor& p) : LcdDisplay (p, "KICK DUCK") {}
protected:
    void drawContent (juce::Graphics&, juce::Rectangle<float>) override;
    float signature() override;
};
