#include "BassurdaLookAndFeel.h"

using namespace juce;
namespace C = Colours_;

BassurdaLookAndFeel::BassurdaLookAndFeel()
{
    setColour (PopupMenu::backgroundColourId, C::lcdBg);
    setColour (PopupMenu::textColourId, C::cyan);
    setColour (PopupMenu::highlightedBackgroundColourId, C::cyanDim);
    setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (ComboBox::textColourId, C::cyan);
    setColour (Label::textColourId, C::textLight);
    setColour (Slider::textBoxTextColourId, C::cyan);
    setColour (Slider::textBoxBackgroundColourId, C::lcdBg);
    setColour (Slider::textBoxOutlineColourId, C::cyanDim);
    setColour (TooltipWindow::backgroundColourId, C::lcdBg);
    setColour (TooltipWindow::textColourId, C::cyan);
    setColour (TooltipWindow::outlineColourId, C::cyanDim);
    setColour (MidiKeyboardComponent::whiteNoteColourId, Colour (0xffd9d3c0));
    setColour (MidiKeyboardComponent::blackNoteColourId, Colour (0xff17160f));
    setColour (MidiKeyboardComponent::keySeparatorLineColourId, Colour (0xff4b483d));
    setColour (MidiKeyboardComponent::mouseOverKeyOverlayColourId, C::cyan.withAlpha (0.3f));
    setColour (MidiKeyboardComponent::keyDownOverlayColourId, C::cyan.withAlpha (0.7f));
    setColour (MidiKeyboardComponent::shadowColourId, Colours::black.withAlpha (0.4f));
    setColour (MidiKeyboardComponent::upDownButtonBackgroundColourId, C::bodyMid);
    setColour (MidiKeyboardComponent::upDownButtonArrowColourId, C::cyan);
}

Font BassurdaLookAndFeel::labelFont (float height, bool bold)
{
    Font f (height, bold ? Font::bold : Font::plain);
    return f.withExtraKerningFactor (0.08f);
}

const Image& BassurdaLookAndFeel::noiseTexture()
{
    static Image img = []
    {
        constexpr int size = 256;
        Image im (Image::ARGB, size, size, true);
        Random rng (1234);
        Image::BitmapData bd (im, Image::BitmapData::writeOnly);

        // fine grain + horizontal brushed streaks
        std::vector<float> streak (size);
        for (auto& s : streak)
            s = rng.nextFloat();

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                const float grain = rng.nextFloat();
                const float brush = 0.5f + 0.5f * std::sin ((float) y * 0.9f + streak[(size_t) x] * 0.6f) * streak[(size_t) y];
                const float v = 0.6f * grain + 0.4f * brush;
                const uint8 lum = (uint8) (v > 0.5f ? 255 : 0);
                const uint8 a = (uint8) jlimit (0, 255, (int) (std::abs (v - 0.5f) * 2.0f * 90.0f));
                bd.setPixelColour (x, y, Colour (lum, lum, lum, a));
            }
        }
        return im;
    }();
    return img;
}

void BassurdaLookAndFeel::drawTexture (Graphics& g, Rectangle<float> area, float alpha)
{
    Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (area.toNearestInt());
    g.setTiledImageFill (noiseTexture(), 0, 0, alpha);
    g.fillRect (area);
}

void BassurdaLookAndFeel::drawScrew (Graphics& g, Point<float> c, float r)
{
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - r + 0.8f, c.y - r + 1.2f, r * 2, r * 2);

    ColourGradient grad (Colour (0xffd8d2c0), c.x - r, c.y - r, Colour (0xff57534a), c.x + r, c.y + r, false);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);

    g.setColour (Colour (0xff3b3830));
    Path slot;
    slot.addRectangle (-r * 0.75f, -r * 0.14f, r * 1.5f, r * 0.28f);
    g.fillPath (slot, AffineTransform::rotation (0.7f).translated (c));
    Path slot2;
    slot2.addRectangle (-r * 0.14f, -r * 0.75f, r * 0.28f, r * 1.5f);
    g.fillPath (slot2, AffineTransform::rotation (0.7f).translated (c));
}

void BassurdaLookAndFeel::drawEngravedText (Graphics& g, const String& text, Rectangle<float> area,
                                            Justification just, float height)
{
    g.setFont (labelFont (height));
    g.setColour (Colours::white.withAlpha (0.22f));
    g.drawText (text, area.translated (0.0f, 1.0f), just, false);
    g.setColour (C::engraved);
    g.drawText (text, area, just, false);
}

void BassurdaLookAndFeel::drawModulePlate (Graphics& g, Rectangle<float> area, const String& title)
{
    // drop shadow
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (area.translated (0.0f, 3.0f).expanded (1.0f), 7.0f);

    // metal plate
    ColourGradient grad (C::plateTop, area.getX(), area.getY(), C::plateBot, area.getX(), area.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (area, 6.0f);

    {
        Graphics::ScopedSaveState s (g);
        Path clip;
        clip.addRoundedRectangle (area, 6.0f);
        g.reduceClipRegion (clip);
        g.setTiledImageFill (noiseTexture(), 0, 0, 0.35f);
        g.fillRect (area);
    }

    // bevel
    g.setColour (Colours::white.withAlpha (0.28f));
    g.drawLine (area.getX() + 6, area.getY() + 1.0f, area.getRight() - 6, area.getY() + 1.0f, 1.0f);
    g.setColour (C::plateEdge);
    g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);

    // title strip
    auto titleArea = area.withHeight (22.0f).reduced (18.0f, 0.0f);
    g.setColour (Colours::black.withAlpha (0.10f));
    g.fillRect (area.getX() + 2.0f, area.getY() + 21.0f, area.getWidth() - 4.0f, 1.0f);
    g.setColour (Colours::white.withAlpha (0.18f));
    g.fillRect (area.getX() + 2.0f, area.getY() + 22.0f, area.getWidth() - 4.0f, 1.0f);
    drawEngravedText (g, title, titleArea.translated (0.0f, 1.0f), Justification::centredLeft, 12.5f);

    const float sr = 3.2f;
    drawScrew (g, { area.getX() + 8.0f, area.getY() + 11.0f }, sr);
    drawScrew (g, { area.getRight() - 8.0f, area.getY() + 11.0f }, sr);
    drawScrew (g, { area.getX() + 8.0f, area.getBottom() - 8.0f }, sr);
    drawScrew (g, { area.getRight() - 8.0f, area.getBottom() - 8.0f }, sr);
}

void BassurdaLookAndFeel::drawLcdFrame (Graphics& g, Rectangle<float> area)
{
    // bezel
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (area.expanded (3.0f).translated (0.0f, 1.0f), 5.0f);
    ColourGradient bez (Colour (0xff2a2924), area.getX(), area.getY() - 3, Colour (0xff4a473e), area.getX(), area.getBottom() + 3, false);
    g.setGradientFill (bez);
    g.fillRoundedRectangle (area.expanded (3.0f), 5.0f);

    // screen
    ColourGradient scr (C::lcdBg2, area.getCentreX(), area.getCentreY(), C::lcdBg, area.getX(), area.getY(), true);
    g.setGradientFill (scr);
    g.fillRoundedRectangle (area, 3.0f);
}

void BassurdaLookAndFeel::strokeGlow (Graphics& g, const Path& path, Colour colour, float thickness)
{
    g.setColour (colour.withAlpha (0.12f));
    g.strokePath (path, PathStrokeType (thickness * 4.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (colour.withAlpha (0.28f));
    g.strokePath (path, PathStrokeType (thickness * 2.2f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (colour);
    g.strokePath (path, PathStrokeType (thickness, PathStrokeType::curved, PathStrokeType::rounded));
}

//==============================================================================
void BassurdaLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height, float pos,
                                            float startAngle, float endAngle, Slider& slider)
{
    const auto bounds = Rectangle<int> (x, y, width, height).toFloat();
    const float size  = jmin (bounds.getWidth(), bounds.getHeight());
    const auto  c     = bounds.getCentre();
    const float r     = size * 0.5f - 1.0f;
    const bool  enabled = slider.isEnabled();
    const Colour accent = slider.getProperties().contains ("accent")
                              ? Colour::fromString (slider.getProperties()["accent"].toString())
                              : C::cyan;

    // ---- LED value arc -------------------------------------------------
    const float arcR  = r * 0.92f;
    const float arcW  = jmax (2.0f, r * 0.11f);
    Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (Colours::black.withAlpha (0.55f));
    g.strokePath (track, PathStrokeType (arcW + 2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colour (0xff10191b));
    g.strokePath (track, PathStrokeType (arcW, PathStrokeType::curved, PathStrokeType::rounded));

    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float zeroPos = bipolar ? (float) slider.valueToProportionOfLength (0.0) : 0.0f;
    const float a0 = startAngle + zeroPos * (endAngle - startAngle);
    const float a1 = startAngle + pos * (endAngle - startAngle);

    if (std::abs (a1 - a0) > 0.01f && enabled)
    {
        Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, jmin (a0, a1), jmax (a0, a1), true);
        g.setColour (accent.withAlpha (0.25f));
        g.strokePath (val, PathStrokeType (arcW * 2.2f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (val, PathStrokeType (arcW * 0.75f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // ---- knob body -------------------------------------------------------
    const float kr = r * 0.74f;
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillEllipse (c.x - kr + 1.0f, c.y - kr + 2.5f, kr * 2.0f, kr * 2.0f);

    // metal skirt
    ColourGradient skirt (Colour (0xff8c877a), c.x - kr, c.y - kr, Colour (0xff1f1e1a), c.x + kr * 0.6f, c.y + kr, false);
    g.setGradientFill (skirt);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);

    // grip ribs
    g.setColour (Colours::black.withAlpha (0.45f));
    const int ribs = 28;
    for (int i = 0; i < ribs; ++i)
    {
        const float a = (float) i / ribs * MathConstants<float>::twoPi;
        const auto p1 = c.getPointOnCircumference (kr * 0.84f, a);
        const auto p2 = c.getPointOnCircumference (kr * 0.99f, a);
        g.drawLine ({ p1, p2 }, 1.0f);
    }

    // black cap
    const float cr = kr * 0.78f;
    ColourGradient cap (Colour (0xff3c3b37), c.x - cr * 0.5f, c.y - cr, Colour (0xff060606), c.x + cr * 0.3f, c.y + cr, false);
    g.setGradientFill (cap);
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f, 1.0f);

    // concave highlight
    ColourGradient shine (Colours::white.withAlpha (0.16f), c.x, c.y - cr, Colours::transparentWhite, c.x, c.y, false);
    g.setGradientFill (shine);
    g.fillEllipse (c.x - cr * 0.8f, c.y - cr * 0.95f, cr * 1.6f, cr * 1.1f);

    // ---- indicator -------------------------------------------------------
    const float ang = a1;
    const auto  pA  = c.getPointOnCircumference (cr * 0.30f, ang);
    const auto  pB  = c.getPointOnCircumference (cr * 0.86f, ang);
    const Colour ind = enabled ? accent : C::textDim;
    g.setColour (ind.withAlpha (0.3f));
    g.drawLine ({ pA, pB }, jmax (2.0f, r * 0.12f) * 2.2f);
    g.setColour (ind);
    g.drawLine ({ pA, pB }, jmax (1.6f, r * 0.08f));
    g.fillEllipse (Rectangle<float> (r * 0.12f, r * 0.12f).withCentre (pB));
}

//==============================================================================
void BassurdaLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    auto area = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const Colour led = b.getProperties().contains ("ledColour")
                           ? Colour::fromString (b.getProperties()["ledColour"].toString())
                           : C::red;

    // housing
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (area.translated (0.0f, 1.0f), 4.0f);
    ColourGradient grad (Colour (0xff35332d), area.getX(), area.getY(), Colour (0xff1a1915), area.getX(), area.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (Colours::white.withAlpha (highlighted ? 0.18f : 0.09f));
    g.drawRoundedRectangle (area.reduced (0.5f), 4.0f, 1.0f);

    // LED
    const float d = jmin (area.getHeight() * 0.45f, 9.0f);
    auto ledArea = Rectangle<float> (d, d).withCentre ({ area.getX() + 6.0f + d * 0.5f, area.getCentreY() });
    if (on)
    {
        g.setColour (led.withAlpha (0.3f));
        g.fillEllipse (ledArea.expanded (d * 0.5f));
        g.setColour (led);
    }
    else
    {
        g.setColour (led.darker (2.6f));
    }
    g.fillEllipse (ledArea);
    g.setColour (Colours::white.withAlpha (on ? 0.6f : 0.15f));
    g.fillEllipse (ledArea.reduced (d * 0.3f).translated (-d * 0.12f, -d * 0.12f));

    // label
    auto textArea = area.withLeft (ledArea.getRight() + 5.0f).withTrimmedRight (4.0f);
    g.setFont (labelFont (jmin (11.0f, area.getHeight() * 0.55f)));
    g.setColour (on ? C::textLight : C::textDim);
    g.drawFittedText (b.getButtonText(), textArea.toNearestInt(), Justification::centredLeft, 1, 0.8f);
}

void BassurdaLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    auto area = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();

    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (area.translated (0.0f, 1.2f), 3.5f);

    ColourGradient grad (down ? Colour (0xff141412) : Colour (0xff3a3832), area.getX(), area.getY(),
                         down ? Colour (0xff24231f) : Colour (0xff191815), area.getX(), area.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (area, 3.5f);

    if (on)
    {
        auto inner = area.reduced (2.0f);
        g.setColour (C::lcdBg);
        g.fillRoundedRectangle (inner, 2.5f);
        g.setColour (C::cyan.withAlpha (0.18f));
        g.fillRoundedRectangle (inner, 2.5f);
        g.setColour (C::cyan.withAlpha (0.7f));
        g.drawRoundedRectangle (inner, 2.5f, 1.0f);
    }

    g.setColour (Colours::white.withAlpha (highlighted ? 0.2f : 0.1f));
    g.drawRoundedRectangle (area.reduced (0.5f), 3.5f, 1.0f);
}

Font BassurdaLookAndFeel::getTextButtonFont (TextButton&, int h)
{
    return labelFont (jmin (11.5f, (float) h * 0.55f));
}

void BassurdaLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.getToggleState() ? C::cyan : C::textDim);
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (3, 1), Justification::centred, 1, 0.7f);
}

//==============================================================================
void BassurdaLookAndFeel::drawComboBox (Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box)
{
    auto area = Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setColour (C::lcdBg);
    g.fillRoundedRectangle (area, 3.0f);
    g.setColour ((box.hasKeyboardFocus (true) || box.isMouseOver (true)) ? C::cyan.withAlpha (0.6f) : C::cyanDim);
    g.drawRoundedRectangle (area.reduced (0.5f), 3.0f, 1.0f);

    // folder glyph
    auto icon = Rectangle<float> (8.0f, height * 0.5f - 5.0f, 13.0f, 10.0f);
    g.setColour (C::cyan.withAlpha (0.75f));
    g.drawRect (icon, 1.0f);
    g.fillRect (icon.withHeight (2.5f).withWidth (6.0f).translated (0.0f, -2.0f));

    // arrow
    Path p;
    const float ax = (float) width - 14.0f, ay = (float) height * 0.5f;
    p.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (C::cyan);
    g.fillPath (p);
}

Font BassurdaLookAndFeel::getComboBoxFont (ComboBox&) { return labelFont (13.0f); }

void BassurdaLookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (26, 1, box.getWidth() - 46, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (Justification::centred);
}

Font BassurdaLookAndFeel::getPopupMenuFont() { return labelFont (14.0f, false); }

Label* BassurdaLookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (labelFont (11.0f));
    return l;
}
