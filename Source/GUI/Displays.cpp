#include "Displays.h"
#include "BassurdaLookAndFeel.h"
#include "../DSP/DspUtils.h"
#include "../DSP/Saturator.h"

using namespace juce;
namespace C = Colours_;
using LAF = BassurdaLookAndFeel;

//==============================================================================
LcdDisplay::LcdDisplay (BassurdaProcessor& p, String footerText)
    : proc (p), footer (std::move (footerText))
{
    setOpaque (false);
    startTimerHz (30);
}

float LcdDisplay::value (const char* id) const
{
    if (auto* v = proc.apvts.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

void LcdDisplay::timerCallback()
{
    if (alwaysAnimate())
    {
        repaint();
        return;
    }
    const float s = signature();
    if (std::abs (s - lastSignature) > 1.0e-6f)
    {
        lastSignature = s;
        repaint();
    }
}

void LcdDisplay::drawGrid (Graphics& g, Rectangle<float> r, int cols, int rows) const
{
    g.setColour (C::lcdGrid);
    for (int i = 1; i < cols; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) cols;
        g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
    }
    for (int i = 1; i < rows; ++i)
    {
        const float y = r.getY() + r.getHeight() * (float) i / (float) rows;
        g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    }
}

void LcdDisplay::drawTag (Graphics& g, Rectangle<float> screen, const String& text, Justification just, Colour colour) const
{
    g.setFont (LAF::labelFont (10.5f));
    g.setColour (colour.isTransparent() ? C::cyan.withAlpha (0.85f) : colour);
    g.drawText (text, screen.reduced (5.0f, 2.0f).withHeight (13.0f), just, false);
}

void LcdDisplay::paint (Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (3.0f);
    LAF::drawLcdFrame (g, area);

    auto screen = area.reduced (2.0f);
    {
        Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (screen.toNearestInt());

        auto content = screen;
        if (footer.isNotEmpty())
        {
            auto foot = content.removeFromBottom (14.0f);
            g.setColour (C::cyanDim.withAlpha (0.35f));
            g.fillRect (foot);
            g.setColour (C::textLight.withAlpha (0.9f));
            g.setFont (LAF::labelFont (10.0f));
            g.drawText (footer, foot.reduced (5.0f, 0.0f), Justification::centredLeft, false);
        }

        drawContent (g, content);

        // scanlines + glass
        g.setColour (Colours::black.withAlpha (0.12f));
        for (float y = screen.getY(); y < screen.getBottom(); y += 2.0f)
            g.drawHorizontalLine ((int) y, screen.getX(), screen.getRight());
        ColourGradient glass (Colours::white.withAlpha (0.07f), screen.getX(), screen.getY(),
                              Colours::transparentWhite, screen.getX(), screen.getCentreY(), false);
        g.setGradientFill (glass);
        g.fillRect (screen.withHeight (screen.getHeight() * 0.5f));
    }
}

//==============================================================================
static Path wavePath (Rectangle<float> r, int points, const std::function<float (double)>& fn, float amp = 0.8f)
{
    Path p;
    for (int i = 0; i <= points; ++i)
    {
        const double t = (double) i / points;
        const float y = r.getCentreY() - fn (t) * r.getHeight() * 0.5f * amp;
        const float x = r.getX() + (float) t * r.getWidth();
        if (i == 0) p.startNewSubPath (x, y);
        else        p.lineTo (x, y);
    }
    return p;
}

WaveDisplay::WaveDisplay (BassurdaProcessor& p, int oscIndex)
    : LcdDisplay (p, oscIndex == 0 ? "OSC A  MORPH" : "OSC B  UNISON"), osc (oscIndex) {}

float WaveDisplay::signature()
{
    using namespace ParamID;
    return osc == 0 ? value (osc1Wave) + value (osc1Pw) * 10 + value (osc1On) * 100 + value (osc1Level) * 1000
                    : value (osc2Wave) + value (osc2Pw) * 10 + value (osc2On) * 100 + value (osc2Level) * 1000
                      + value (osc2Voices) * 1.0e4f + value (osc2Detune) * 1.0e5f;
}

void WaveDisplay::drawContent (Graphics& g, Rectangle<float> r)
{
    using namespace ParamID;
    drawGrid (g, r, 8, 4);

    const bool  on    = value (osc == 0 ? osc1On : osc2On) > 0.5f;
    const float wave  = value (osc == 0 ? osc1Wave : osc2Wave);
    const float pw    = value (osc == 0 ? osc1Pw : osc2Pw);
    const float level = value (osc == 0 ? osc1Level : osc2Level);
    const Colour col  = on ? C::cyan : C::cyanDim;

    auto plot = r.reduced (6.0f, 8.0f);
    if (osc == 1 && on)
    {
        const int voices = (int) value (osc2Voices);
        const float det  = value (osc2Detune);
        for (int v = 1; v < voices; ++v)
        {
            const double shift = (double) v * 0.6180339887;
            const double stretch = 1.0 + det * 0.05 * ((double) v / voices);
            auto p = wavePath (plot, 160, [&] (double t)
            { return bdsp::morphWaveNaive (bdsp::wrap01 (t * 2.0 * stretch + shift), wave, pw); }, 0.8f * jmax (0.2f, level));
            g.setColour (col.withAlpha (0.22f));
            g.strokePath (p, PathStrokeType (1.0f));
        }
    }

    auto p = wavePath (plot, 240, [&] (double t) { return bdsp::morphWaveNaive (bdsp::wrap01 (t * 2.0), wave, pw); },
                       0.8f * jmax (0.2f, level));
    LAF::strokeGlow (g, p, col, 1.6f);

    auto* param = proc.apvts.getParameter (osc == 0 ? osc1Wave : osc2Wave);
    drawTag (g, r, param != nullptr ? param->getCurrentValueAsText() : String(), Justification::topLeft);
    if (! on)
        drawTag (g, r, "OFF", Justification::topRight, C::red);
}

//==============================================================================
float SubDisplay::signature()
{
    using namespace ParamID;
    return value (subLevel) + value (subWave) * 10 + value (subOct) * 100 + value (clickLevel) * 1000
           + value (subClean) * 1.0e4f + value (clickTone) * 1.0e5f;
}

void SubDisplay::drawContent (Graphics& g, Rectangle<float> r)
{
    using namespace ParamID;
    drawGrid (g, r, 8, 4);

    const float level  = value (subLevel);
    const bool  tri    = value (subWave) > 0.5f;
    const int   oct    = (int) value (subOct);
    const float click  = value (clickLevel);
    const float cycles = oct == 0 ? 2.0f : (oct == 1 ? 1.0f : 0.5f);

    auto plot = r.reduced (6.0f, 8.0f);
    auto p = wavePath (plot, 200, [&] (double t)
    {
        const double ph = bdsp::wrap01 (t * cycles);
        float v = tri ? bdsp::triangle (ph) : bdsp::sine (ph);
        // the click burst at the very start of the note
        if (click > 0.0f && t < 0.08)
            v += click * (float) std::sin (t * 900.0) * (float) (1.0 - t / 0.08);
        return v;
    }, 0.8f * jmax (0.15f, level));
    LAF::strokeGlow (g, p, C::cyan, 1.8f);

    drawTag (g, r, String (tri ? "TRI" : "SINE") + "  OCT " + ParamChoices::subOct[oct], Justification::topLeft);
    if (value (subClean) > 0.5f)
        drawTag (g, r, "CLEAN", Justification::topRight, C::green);
    else
        drawTag (g, r, "TO FILTER", Justification::topRight, C::orange);
}

//==============================================================================
EnvelopeDisplay::EnvelopeDisplay (BassurdaProcessor& p, bool isAmp)
    : LcdDisplay (p, isAmp ? "AMP ENVELOPE" : "FILTER ENVELOPE"), amp (isAmp) {}

float EnvelopeDisplay::signature()
{
    using namespace ParamID;
    auto& e = proc.getEngine();
    const float live = amp ? e.uiAmpLevel.load() : e.uiFilterLevel.load();
    return amp ? value (aAtk) + value (aDec) * 3 + value (aSus) * 7 + value (aRel) * 11 + live * 1.0e5f
               : value (fAtk) + value (fDec) * 3 + value (fSus) * 7 + value (fRel) * 11 + live * 1.0e5f;
}

void EnvelopeDisplay::drawContent (Graphics& g, Rectangle<float> r)
{
    using namespace ParamID;
    drawGrid (g, r, 8, 4);

    const float a = value (amp ? aAtk : fAtk);
    const float d = value (amp ? aDec : fDec);
    const float s = value (amp ? aSus : fSus);
    const float rl = value (amp ? aRel : fRel);

    // sqrt time scaling keeps short and long settings readable
    const float wa = std::sqrt (a), wd = std::sqrt (d), wr = std::sqrt (rl), ws = 30.0f;
    const float total = wa + wd + ws + wr;
    auto plot = r.reduced (8.0f, 10.0f).withTrimmedTop (6.0f);
    const float sx = plot.getWidth() / total;
    const float x0 = plot.getX(), yb = plot.getBottom(), yt = plot.getY();
    const float ys = yb - s * plot.getHeight();

    Path p;
    p.startNewSubPath (x0, yb);
    p.lineTo (x0 + wa * sx, yt);
    const float xd = x0 + wa * sx;
    for (int i = 1; i <= 24; ++i)
    {
        const float t = (float) i / 24.0f;
        const float curve = std::exp (-4.6f * t);
        p.lineTo (xd + t * wd * sx, ys + (yt - ys) * curve);
    }
    const float xs = xd + wd * sx;
    p.lineTo (xs + ws * sx, ys);
    const float xr = xs + ws * sx;
    for (int i = 1; i <= 24; ++i)
    {
        const float t = (float) i / 24.0f;
        const float curve = std::exp (-4.6f * t) * (1.0f - t * 0.01f);
        p.lineTo (xr + t * wr * sx, yb + (ys - yb) * curve);
    }

    Path fill (p);
    fill.lineTo (plot.getRight(), yb);
    fill.closeSubPath();
    const Colour col = amp ? C::cyan : C::orange;
    g.setColour (col.withAlpha (0.10f));
    g.fillPath (fill);
    LAF::strokeGlow (g, p, col, 1.6f);

    // live level
    auto& e = proc.getEngine();
    const float live = amp ? e.uiAmpLevel.load() : e.uiFilterLevel.load();
    if (live > 0.001f)
    {
        const float y = yb - live * plot.getHeight();
        g.setColour (Colours::white.withAlpha (0.7f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
    }

    auto txt = [] (float ms) { return ms < 1000.0f ? String (ms, ms < 10.0f ? 1 : 0) : String (ms / 1000.0f, 1) + "s"; };
    drawTag (g, r, "A " + txt (a) + "  D " + txt (d) + "  S " + String (roundToInt (s * 100)) + "  R " + txt (rl),
             Justification::topLeft, col.withAlpha (0.9f));
}

//==============================================================================
float PitchEnvDisplay::signature()
{
    using namespace ParamID;
    return value (pitchAmt) + value (pitchDecay) * 100 + value (pitchSub) * 1.0e6f;
}

void PitchEnvDisplay::drawContent (Graphics& g, Rectangle<float> r)
{
    using namespace ParamID;
    drawGrid (g, r, 6, 4);

    const float amt   = value (pitchAmt);
    const float decay = value (pitchDecay);
    auto plot = r.reduced (8.0f, 12.0f).withTrimmedTop (4.0f);

    // fixed 400 ms window so the decay setting is visible
    const float window = 400.0f;
    const float zeroY  = plot.getY() + plot.getHeight() * (48.0f / 72.0f);
    const float scale  = plot.getHeight() / 72.0f;

    g.setColour (C::cyanDim);
    g.drawHorizontalLine ((int) zeroY, plot.getX(), plot.getRight());

    Path p;
    for (int i = 0; i <= 120; ++i)
    {
        const float t  = (float) i / 120.0f * window;
        const float st = amt * std::exp (-4.6f * t / jmax (1.0f, decay));
        const float x  = plot.getX() + (float) i / 120.0f * plot.getWidth();
        const float y  = zeroY - st * scale;
        if (i == 0) p.startNewSubPath (x, y);
        else        p.lineTo (x, y);
    }
    LAF::strokeGlow (g, p, C::orange, 1.6f);

    drawTag (g, r, (amt > 0 ? "+" : "") + String (amt, 1) + " st", Justification::topLeft, C::orange);
    drawTag (g, r, String (roundToInt (decay)) + " ms", Justification::topRight);
}

//==============================================================================
float FilterDisplay::signature()
{
    using namespace ParamID;
    auto& e = proc.getEngine();
    return value (fltType) + value (fltCutoff) * 0.01f + value (fltReso) * 1000 + value (fltEnv) * 1.0e4f
           + e.uiFilterLevel.load() * 3.0e4f + e.uiCurrentNote.load() * 7.0e3f + value (fltKey) * 1.1e5f;
}

void FilterDisplay::drawContent (Graphics& g, Rectangle<float> r)
{
    using namespace ParamID;
    drawGrid (g, r, 10, 4);

    const int   type   = (int) value (fltType);
    const float reso   = value (fltReso);
    auto& e = proc.getEngine();
    const float note   = e.uiCurrentNote.load();
    float fc = value (fltCutoff);
    if (note >= 0.0f)
        fc *= std::exp2 (value (fltEnv) * e.uiFilterLevel.load() * 7.0f + value (fltKey) * (note - 36.0f) / 12.0f);
    fc = jlimit (16.0f, 20000.0f, fc);

    const float minDb = -36.0f, maxDb = 24.0f;
    auto plot = r.reduced (4.0f, 6.0f).withTrimmedTop (8.0f);

    auto magnitude = [&] (float f)
    {
        const double w = f / fc;
        auto sec = [w] (double k, int mode) // 0 low, 1 band (unity peak), 2 high
        {
            const double re = 1.0 - w * w, im = k * w;
            const double den = std::sqrt (re * re + im * im);
            if (mode == 0) return 1.0 / den;
            if (mode == 1) return k * w / den;
            return w * w / den;
        };
        const double k12 = jmax (0.02, 1.4142 * (1.0 - reso));
        switch (type)
        {
            case 0:  return sec (k12, 0);
            case 1:  return sec (1.8478, 0) * sec (jmax (0.02, 0.7654 * (1.0 - reso)), 0);
            case 2:  return sec (jmax (0.05, k12), 1);
            default: return sec (k12, 2);
        }
    };

    Path p;
    const int n = 160;
    for (int i = 0; i <= n; ++i)
    {
        const float f  = 20.0f * std::pow (1000.0f, (float) i / n);
        const float db = jlimit (minDb, maxDb, (float) Decibels::gainToDecibels (magnitude (f), (double) minDb));
        const float x  = plot.getX() + (float) i / n * plot.getWidth();
        const float y  = jmap (db, maxDb, minDb, plot.getY(), plot.getBottom());
        if (i == 0) p.startNewSubPath (x, y);
        else        p.lineTo (x, y);
    }
    Path fill (p);
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.lineTo (plot.getX(), plot.getBottom());
    fill.closeSubPath();
    g.setColour (C::cyan.withAlpha (0.10f));
    g.fillPath (fill);
    LAF::strokeGlow (g, p, C::cyan, 1.6f);

    // cutoff marker
    const float cx = plot.getX() + std::log (fc / 20.0f) / std::log (1000.0f) * plot.getWidth();
    g.setColour (C::orange.withAlpha (0.6f));
    g.drawVerticalLine ((int) cx, plot.getY(), plot.getBottom());

    drawTag (g, r, ParamChoices::fltType[jlimit (0, 3, type)], Justification::topLeft);
    drawTag (g, r, fc < 1000 ? String (roundToInt (fc)) + " Hz" : String (fc / 1000.0f, 2) + " kHz", Justification::topRight, C::orange);
}

//==============================================================================
float SaturationDisplay::signature()
{
    using namespace ParamID;
    return value (satType) + value (satDrive) * 10 + value (satMix) * 100 + proc.satActivity.load() * 1000;
}

void SaturationDisplay::drawContent (Graphics& g, Rectangle<float> r)
{
    using namespace ParamID;
    drawGrid (g, r, 6, 4);

    const int   type  = (int) value (satType);
    const float drive = value (satDrive);
    const float mix   = value (satMix);
    const float dg    = bdsp::Saturator::driveToGain (drive);
    const float mk    = bdsp::Saturator::makeupGain (type, dg);

    auto plot = r.reduced (60.0f, 3.0f).withTrimmedTop (2.0f);

    g.setColour (C::cyanDim.withAlpha (0.6f));
    g.drawLine (plot.getX(), plot.getBottom(), plot.getRight(), plot.getY(), 1.0f);

    Path p;
    const int n = 160;
    for (int i = 0; i <= n; ++i)
    {
        const float x = -1.0f + 2.0f * (float) i / n;
        const float wet = (bdsp::Saturator::shape (type, x * dg) - bdsp::Saturator::shape (type, 0.0f)) * mk;
        const float y = jlimit (-1.2f, 1.2f, x + mix * (wet - x));
        const float px = jmap (x, -1.0f, 1.0f, plot.getX(), plot.getRight());
        const float py = jmap (y, -1.2f, 1.2f, plot.getBottom(), plot.getY());
        if (i == 0) p.startNewSubPath (px, py);
        else        p.lineTo (px, py);
    }

    const float act = jlimit (0.0f, 1.0f, proc.satActivity.load() * 0.25f);
    LAF::strokeGlow (g, p, C::orange.interpolatedWith (C::red, act), 1.6f + act * 1.2f);

    drawTag (g, r, ParamChoices::satType[jlimit (0, 3, type)], Justification::topLeft, C::orange);
    drawTag (g, r, "+" + String (drive * 30.0f, 1) + " dB", Justification::topRight);
}

//==============================================================================
float LfoDisplay::signature()
{
    using namespace ParamID;
    return value (lfoShape) + value (lfoRate) * 10 + value (lfoSync) * 1000 + value (lfoDiv) * 1.0e4f;
}

void LfoDisplay::drawContent (Graphics& g, Rectangle<float> r)
{
    using namespace ParamID;
    drawGrid (g, r, 8, 2);

    const int shape = (int) value (lfoShape);
    auto plot = r.reduced (6.0f, 8.0f).withTrimmedTop (6.0f);
    Random rng (42);
    float sh[8];
    for (auto& v : sh) v = rng.nextFloat() * 2.0f - 1.0f;

    auto p = wavePath (plot, 240, [&] (double t)
    {
        const double ph = bdsp::wrap01 (t * 2.0);
        switch (shape)
        {
            case 0:  return bdsp::sine (ph);
            case 1:  return bdsp::triangle (ph);
            case 2:  return (float) (1.0 - 2.0 * ph);
            case 3:  return ph < 0.5 ? 1.0f : -1.0f;
            default: return sh[jlimit (0, 7, (int) (t * 8.0))];
        }
    });
    LAF::strokeGlow (g, p, C::green, 1.5f);

    const bool sync = value (lfoSync) > 0.5f;
    const String rate = sync ? ParamChoices::lfoDiv[jlimit (0, ParamChoices::lfoDiv.size() - 1, (int) value (lfoDiv))]
                             : String (value (lfoRate), 2) + " Hz";
    drawTag (g, r, ParamChoices::lfoShape[jlimit (0, 4, shape)], Justification::topLeft, C::green);
    drawTag (g, r, rate, Justification::topRight);
}

//==============================================================================
void ScopeDisplay::drawContent (Graphics& g, Rectangle<float> r)
{
    drawGrid (g, r, 10, 6);
    g.setColour (C::cyanDim.withAlpha (0.8f));
    g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());

    proc.scope.read (buffer.data(), (int) buffer.size());

    auto& e = proc.getEngine();
    const float note = e.uiCurrentNote.load();
    const double srate = proc.uiSampleRate.load();

    // show ~3 cycles of the played note, triggered on a rising zero crossing
    int window = 1024;
    if (note >= 0.0f)
        window = jlimit (128, 2600, (int) (3.0 * srate / bdsp::midiToHz (note)));

    const int last = (int) buffer.size() - window - 1;
    int start = last;
    for (int i = last; i > 1; --i)
    {
        if (buffer[(size_t) i - 1] < 0.0f && buffer[(size_t) i] >= 0.0f)
        {
            start = i;
            break;
        }
    }

    auto plot = r.reduced (6.0f, 14.0f);
    Path p;
    for (int i = 0; i < window; ++i)
    {
        const float x = plot.getX() + (float) i / (float) (window - 1) * plot.getWidth();
        const float y = plot.getCentreY() - jlimit (-1.1f, 1.1f, buffer[(size_t) (start + i)]) * plot.getHeight() * 0.5f;
        if (i == 0) p.startNewSubPath (x, y);
        else        p.lineTo (x, y);
    }
    LAF::strokeGlow (g, p, C::cyan, 1.8f);

    if (note >= 0.0f)
    {
        const int n = roundToInt (note);
        const String name = MidiMessage::getMidiNoteName (n, true, true, 3);
        drawTag (g, r, name + "   " + String (bdsp::midiToHz (note), 1) + " Hz", Justification::topLeft);
    }
    else
    {
        drawTag (g, r, "PLAY A NOTE", Justification::topLeft, C::cyanDim);
    }

    if (e.uiGliding.load())
        drawTag (g, r, "GLIDE", Justification::topRight, C::orange);
}

//==============================================================================
void MeterDisplay::drawContent (Graphics& g, Rectangle<float> r)
{
    const float peak = proc.outputPeak.load();
    smoothed = jmax (peak, smoothed * 0.85f);
    held = jmax (smoothed, held * 0.985f);

    auto toY = [&] (float lin)
    {
        const float db = jlimit (-48.0f, 3.0f, Decibels::gainToDecibels (lin, -48.0f));
        return jmap (db, -48.0f, 3.0f, r.getBottom() - 3.0f, r.getY() + 3.0f);
    };

    auto bar = r.reduced (5.0f, 3.0f);
    const int segs = 26;
    const float segH = bar.getHeight() / segs;
    const float levelY = toY (smoothed);
    for (int i = 0; i < segs; ++i)
    {
        auto seg = Rectangle<float> (bar.getX(), bar.getBottom() - (i + 1) * segH, bar.getWidth(), segH - 1.5f);
        const float frac = (float) i / segs;
        const Colour col = frac > 0.9f ? C::red : (frac > 0.75f ? C::orange : C::cyan);
        const bool lit = seg.getBottom() > levelY;
        g.setColour (lit ? col : col.withAlpha (0.10f));
        g.fillRect (seg);
    }
    g.setColour (Colours::white.withAlpha (0.8f));
    g.drawHorizontalLine ((int) toY (held), bar.getX(), bar.getRight());
}
