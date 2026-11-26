#include "PluginEditor.h"
#include "Presets.h"

using namespace juce;
namespace C = Colours_;
using LAF = BassurdaLookAndFeel;

//==============================================================================
MainView::MainView (BassurdaProcessor& p)
    : proc (p),
      oscA (p, 0), oscB (p, 1), sub (p), scope (p),
      filter (p), filterEnv (p, false), ampEnv (p, true), pitch (p),
      voice (p), lfo (p), saturation (p), output (p),
      keyboard (p.keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    for (auto* c : { (Component*) &oscA, (Component*) &oscB, (Component*) &sub, (Component*) &scope,
                     (Component*) &filter, (Component*) &filterEnv, (Component*) &ampEnv, (Component*) &pitch,
                     (Component*) &voice, (Component*) &lfo, (Component*) &saturation, (Component*) &output,
                     (Component*) &keyboard, (Component*) &presetBox, (Component*) &prevButton, (Component*) &nextButton })
        addAndMakeVisible (c);

    keyboard.setAvailableRange (24, 84);
    keyboard.setLowestVisibleKey (24);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setWantsKeyboardFocus (false);

    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0 && idx != proc.getCurrentProgram())
            proc.setCurrentProgram (idx);
    };
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    prevButton.setTooltip ("Previous preset");
    nextButton.setTooltip ("Next preset");

    setSize (designWidth, designHeight);
}

void MainView::refreshPresetBox()
{
    presetBox.clear (dontSendNotification);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), dontSendNotification);
}

void MainView::stepPreset (int delta)
{
    const int n = proc.getNumPrograms();
    const int idx = (proc.getCurrentProgram() + delta + n) % n;
    proc.setCurrentProgram (idx);
    presetBox.setSelectedItemIndex (idx, dontSendNotification);
}

void MainView::paint (Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // body
    ColourGradient body (C::bodyMid, 0.0f, 0.0f, C::bodyDark, 0.0f, bounds.getBottom(), false);
    g.setGradientFill (body);
    g.fillRect (bounds);
    LAF::drawTexture (g, bounds, 0.22f);

    // header plate
    auto header = bounds.removeFromTop (66.0f).reduced (8.0f, 6.0f);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (header.translated (0.0f, 2.0f), 6.0f);
    ColourGradient hg (C::bodyLight, header.getX(), header.getY(), C::bodyDark, header.getX(), header.getBottom(), false);
    g.setGradientFill (hg);
    g.fillRoundedRectangle (header, 6.0f);
    LAF::drawTexture (g, header, 0.25f);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (header.reduced (0.5f), 6.0f, 1.0f);
    LAF::drawScrew (g, { header.getX() + 9.0f, header.getCentreY() }, 3.5f);
    LAF::drawScrew (g, { header.getRight() - 9.0f, header.getCentreY() }, 3.5f);

    // emblem: a cyan sine dropping into an orange sub wave inside a ring
    const Point<float> ec { header.getX() + 44.0f, header.getCentreY() };
    const float er = 19.0f;
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillEllipse (ec.x - er, ec.y - er + 1.5f, er * 2, er * 2);
    ColourGradient eg (Colour (0xff4a4740), ec.x, ec.y - er, Colour (0xff151411), ec.x, ec.y + er, false);
    g.setGradientFill (eg);
    g.fillEllipse (ec.x - er, ec.y - er, er * 2, er * 2);
    g.setColour (C::red);
    g.drawEllipse (ec.x - er + 2, ec.y - er + 2, er * 2 - 4, er * 2 - 4, 2.0f);
    Path wave;
    for (int i = 0; i <= 40; ++i)
    {
        const float t = (float) i / 40.0f;
        const float x = ec.x - er * 0.65f + t * er * 1.3f;
        const float y = ec.y + std::sin (t * MathConstants<float>::twoPi * 1.0f) * er * 0.45f * (1.0f - t * 0.4f);
        if (i == 0) wave.startNewSubPath (x, y);
        else        wave.lineTo (x, y);
    }
    LAF::strokeGlow (g, wave, C::orange, 2.0f);

    // logo text
    auto logo = header.withX (header.getX() + 74.0f).withWidth (300.0f);
    g.setFont (LAF::labelFont (30.0f).withExtraKerningFactor (0.22f));
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawText ("BASSURDA", logo.translated (1.5f, 2.0f), Justification::centredLeft, false);
    ColourGradient lg (C::textLight, logo.getX(), logo.getY() + 10, Colour (0xffb7ae94), logo.getX(), logo.getBottom() - 10, false);
    g.setGradientFill (lg);
    g.drawText ("BASSURDA", logo, Justification::centredLeft, false);

    // tag line on the right
    auto right = header.removeFromRight (300.0f).reduced (24.0f, 0.0f);
    g.setFont (LAF::labelFont (11.0f));
    g.setColour (C::orange);
    g.drawText ("HIP-HOP BASS SYNTH", right.removeFromTop (right.getHeight() * 0.5f), Justification::bottomRight, false);
    g.setColour (C::textDim);
    g.drawText ("MONO  /  SUB  /  GLIDE  /  DRIVE", right, Justification::topRight, false);

    // preset LCD bezel
    auto presetArea = presetBox.getBounds().getUnion (prevButton.getBounds()).getUnion (nextButton.getBounds()).toFloat();
    LAF::drawLcdFrame (g, presetArea.expanded (3.0f));
}

void MainView::resized()
{
    // header: preset browser in the middle
    {
        const int w = 330, h = 28;
        auto r = Rectangle<int> (0, 0, w, h).withCentre ({ designWidth / 2 + 10, 33 });
        prevButton.setBounds (r.removeFromLeft (30));
        nextButton.setBounds (r.removeFromRight (30));
        presetBox.setBounds (r.reduced (3, 0));
    }

    auto area = getLocalBounds().reduced (8, 0);
    area.removeFromTop (70);
    const int gap = 6;

    auto row1 = area.removeFromTop (262);
    oscA.setBounds (row1.removeFromLeft (282));   row1.removeFromLeft (gap);
    oscB.setBounds (row1.removeFromLeft (352));   row1.removeFromLeft (gap);
    sub.setBounds (row1.removeFromLeft (250));    row1.removeFromLeft (gap);
    scope.setBounds (row1);
    area.removeFromTop (gap);

    auto row2 = area.removeFromTop (232);
    filter.setBounds (row2.removeFromLeft (372));    row2.removeFromLeft (gap);
    filterEnv.setBounds (row2.removeFromLeft (262)); row2.removeFromLeft (gap);
    ampEnv.setBounds (row2.removeFromLeft (322));    row2.removeFromLeft (gap);
    pitch.setBounds (row2);
    area.removeFromTop (gap);

    auto row3 = area.removeFromTop (208);
    voice.setBounds (row3.removeFromLeft (352));      row3.removeFromLeft (gap);
    lfo.setBounds (row3.removeFromLeft (300));        row3.removeFromLeft (gap);
    saturation.setBounds (row3.removeFromLeft (300)); row3.removeFromLeft (gap);
    output.setBounds (row3);
    area.removeFromTop (gap);

    keyboard.setBounds (area.removeFromTop (52));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f);   // 36 white keys: C1..C6
}

//==============================================================================
BassurdaEditor::BassurdaEditor (BassurdaProcessor& p)
    : AudioProcessorEditor (p), view (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (view);

    setResizable (true, true);
    const double ratio = (double) MainView::designWidth / MainView::designHeight;
    getConstrainer()->setFixedAspectRatio (ratio);
    setResizeLimits (MainView::designWidth / 2, MainView::designHeight / 2,
                     MainView::designWidth * 2, MainView::designHeight * 2);

    // start a bit smaller than the design size so it fits laptop screens
    setSize (roundToInt (MainView::designWidth * 0.85), roundToInt (MainView::designHeight * 0.85));
}

BassurdaEditor::~BassurdaEditor()
{
    setLookAndFeel (nullptr);
}

void BassurdaEditor::paint (Graphics& g)
{
    g.fillAll (C::bodyDark);
}

void BassurdaEditor::resized()
{
    const float scale = (float) getWidth() / (float) MainView::designWidth;
    view.setTransform (AffineTransform::scale (scale));
    view.setBounds (0, 0, MainView::designWidth, MainView::designHeight);
}
