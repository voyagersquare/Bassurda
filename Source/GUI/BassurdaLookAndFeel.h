#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

namespace Colours_
{
    // Palette: gunmetal body, khaki-metal module plates, cyan LCD glow,
    // orange accents and red LEDs.
    const juce::Colour bodyDark   { 0xff1b1a17 };
    const juce::Colour bodyMid    { 0xff2c2a25 };
    const juce::Colour bodyLight  { 0xff3d3a33 };
    const juce::Colour plateTop   { 0xff9a957f };
    const juce::Colour plateBot   { 0xff76715f };
    const juce::Colour plateEdge  { 0xff4b483d };
    const juce::Colour engraved   { 0xff25241f };
    const juce::Colour lcdBg      { 0xff071113 };
    const juce::Colour lcdBg2     { 0xff0d1d20 };
    const juce::Colour lcdGrid    { 0xff12333a };
    const juce::Colour cyan       { 0xff3fd6ff };
    const juce::Colour cyanDim    { 0xff1a6a80 };
    const juce::Colour orange     { 0xffff9a2a };
    const juce::Colour red        { 0xffe8392b };
    const juce::Colour green      { 0xff7fd84a };
    const juce::Colour textLight  { 0xffe6e0cc };
    const juce::Colour textDim    { 0xff9b9686 };
}

class BassurdaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BassurdaLookAndFeel();

    // Shared drawing helpers ---------------------------------------------------
    static juce::Font labelFont (float height, bool bold = true);
    static const juce::Image& noiseTexture();
    static void drawTexture (juce::Graphics& g, juce::Rectangle<float> area, float alpha);
    static void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius);
    static void drawModulePlate (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title);
    static void drawLcdFrame (juce::Graphics& g, juce::Rectangle<float> area);
    static void drawEngravedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                                  juce::Justification just, float height);
    static void strokeGlow (juce::Graphics& g, const juce::Path& path, juce::Colour colour, float thickness);

    // LookAndFeel overrides -------------------------------------------------
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;

    juce::Label* createSliderTextBox (juce::Slider&) override;
};
