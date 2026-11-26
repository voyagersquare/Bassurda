#include "Modules.h"
#include "BassurdaLookAndFeel.h"

using namespace juce;
using namespace ParamID;
namespace C = Colours_;
using LAF = BassurdaLookAndFeel;

//==============================================================================
Knob::Knob (APVTS& state, const String& id, String displayName, Colour accent)
    : name (std::move (displayName)), paramId (id),
      proc (dynamic_cast<BassurdaProcessor*> (&state.processor)),
      attachment (state, id, slider)
{
    const auto& paramId_ = id;
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setDoubleClickReturnValue (true, slider.getDoubleClickReturnValue());
    if (auto* p = state.getParameter (paramId_))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    slider.setMouseDragSensitivity (220);
    slider.setVelocityBasedMode (false);
    if (! accent.isTransparent())
        slider.getProperties().set ("accent", accent.toString());
    slider.addMouseListener (this, false);
    slider.onValueChange = [this] { repaint(); };
    slider.setPopupMenuEnabled (false);
    slider.onRightClick = [this] { showMenu(); };
    addAndMakeVisible (slider);
}

void Knob::showMenu()
{
    if (proc == nullptr)
        return;

    PopupMenu m;
    const int cc = proc->getMidiCCFor (paramId);
    m.addSectionHeader (name + (cc >= 0 ? "  (CC " + String (cc) + ")" : String()));
    if (proc->isLearning (paramId))
        m.addItem (1, "Cancel MIDI Learn");
    else
        m.addItem (2, "MIDI Learn (move a controller)");
    m.addItem (3, "Clear MIDI CC", cc >= 0);
    m.addSeparator();
    m.addItem (4, "Reset to default");

    SafePointer<Knob> safe (this);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this), [safe] (int result)
    {
        if (safe == nullptr) return;
        auto* p = safe->proc;
        switch (result)
        {
            case 1: p->cancelMidiLearn(); break;
            case 2: p->startMidiLearn (safe->paramId); safe->startTimerHz (8); break;
            case 3: p->clearMidiMapping (safe->paramId); break;
            case 4: safe->slider.setValue (safe->slider.getDoubleClickReturnValue(), sendNotificationSync); break;
            default: break;
        }
        safe->repaint();
    });
}

void Knob::timerCallback()
{
    ++blink;
    if (proc == nullptr || ! proc->isLearning (paramId))
        stopTimer();
    repaint();
}

void Knob::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (15);
    const int s = jmin (r.getWidth(), r.getHeight());
    slider.setBounds (r.withSizeKeepingCentre (s, s));
}

void Knob::paint (Graphics& g)
{
    auto label = getLocalBounds().removeFromBottom (15).toFloat();
    const bool showValue = slider.isMouseOverOrDragging();
    const bool learning = proc != nullptr && isTimerRunning() && proc->isLearning (paramId);
    const int cc = proc != nullptr ? proc->getMidiCCFor (paramId) : -1;

    if (cc >= 0)
    {
        g.setFont (LAF::labelFont (8.5f));
        g.setColour (C::orange.darker (0.2f));
        g.drawText ("CC" + String (cc), getLocalBounds().removeFromTop (11).toFloat(), Justification::topRight, false);
    }

    if (learning)
    {
        auto box = label.withSizeKeepingCentre (jmin (label.getWidth(), 78.0f), 13.0f);
        g.setColour ((blink & 1) ? C::orange : C::orange.withAlpha (0.4f));
        g.fillRoundedRectangle (box, 2.0f);
        g.setColour (C::lcdBg);
        g.setFont (LAF::labelFont (10.0f));
        g.drawText ("LEARN", box, Justification::centred, false);
    }
    else if (showValue)
    {
        auto box = label.withSizeKeepingCentre (jmin (label.getWidth(), 78.0f), 13.0f);
        g.setColour (C::lcdBg);
        g.fillRoundedRectangle (box, 2.0f);
        g.setColour (C::cyan);
        g.setFont (LAF::labelFont (10.5f));
        g.drawFittedText (slider.getTextFromValue (slider.getValue()), box.toNearestInt(), Justification::centred, 1, 0.7f);
    }
    else
    {
        LAF::drawEngravedText (g, name, label, Justification::centred, 10.5f);
    }
}

//==============================================================================
ChoiceButtons::ChoiceButtons (APVTS& state, const String& paramId, StringArray labels, String cap)
    : caption (std::move (cap))
{
    auto* param = state.getParameter (paramId);
    jassert (param != nullptr);

    const int groupId = Random::getSystemRandom().nextInt (1 << 30) + 1;
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = buttons.add (new TextButton (labels[i]));
        b->setClickingTogglesState (false);
        b->setRadioGroupId (groupId);
        b->setConnectedEdges ((i > 0 ? Button::ConnectedOnLeft : 0) | (i < labels.size() - 1 ? Button::ConnectedOnRight : 0));
        b->onClick = [this, i] { if (attachment) attachment->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }

    attachment = std::make_unique<ParameterAttachment> (*param, [this] (float v) { update (v); });
    attachment->sendInitialUpdate();
}

void ChoiceButtons::update (float value)
{
    const int idx = roundToInt (value);
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState (i == idx, dontSendNotification);
}

void ChoiceButtons::resized()
{
    auto r = getLocalBounds();
    if (caption.isNotEmpty())
        r.removeFromLeft (jmin (58, r.getWidth() / 3));
    const int n = buttons.size();
    const int w = r.getWidth() / jmax (1, n);
    for (int i = 0; i < n; ++i)
        buttons[i]->setBounds (i == n - 1 ? r : r.removeFromLeft (w));
}

void ChoiceButtons::paint (Graphics& g)
{
    if (caption.isNotEmpty())
        LAF::drawEngravedText (g, caption, getLocalBounds().removeFromLeft (jmin (58, getWidth() / 3)).toFloat(),
                               Justification::centredLeft, 10.5f);
}

//==============================================================================
LedToggle::LedToggle (APVTS& state, const String& paramId, const String& buttonText, Colour led)
    : ToggleButton (buttonText), attachment (state, paramId, *this)
{
    getProperties().set ("ledColour", led.toString());
}

//==============================================================================
void ModulePanel::paint (Graphics& g)
{
    LAF::drawModulePlate (g, getLocalBounds().toFloat().reduced (2.0f), title);
}

void ModulePanel::layoutRow (Rectangle<int> area, std::initializer_list<Component*> items)
{
    const int n = (int) items.size();
    if (n == 0) return;
    const int w = area.getWidth() / n;
    int i = 0;
    for (auto* c : items)
    {
        c->setBounds (i == n - 1 ? area : area.removeFromLeft (w));
        ++i;
    }
}

//==============================================================================
OscModule::OscModule (BassurdaProcessor& p, int idx)
    : ModulePanel (idx == 0 ? "OSCILLATOR A" : "OSCILLATOR B"),
      index (idx), display (p, idx),
      onSwitch (p.apvts, idx == 0 ? osc1On : osc2On, "ON", C::cyan)
{
    auto& s = p.apvts;
    const bool a = idx == 0;
    wave  = std::make_unique<Knob> (s, a ? osc1Wave : osc2Wave, "WAVE");
    pw    = std::make_unique<Knob> (s, a ? osc1Pw : osc2Pw, "PW");
    level = std::make_unique<Knob> (s, a ? osc1Level : osc2Level, "LEVEL");
    oct   = std::make_unique<Knob> (s, a ? osc1Oct : osc2Oct, "OCT", C::orange);
    semi  = std::make_unique<Knob> (s, a ? osc1Semi : osc2Semi, "SEMI", C::orange);
    fine  = std::make_unique<Knob> (s, a ? osc1Fine : osc2Fine, "FINE", C::orange);
    if (! a)
    {
        voices = std::make_unique<Knob> (s, osc2Voices, "UNISON", C::green);
        detune = std::make_unique<Knob> (s, osc2Detune, "DETUNE", C::green);
        addAndMakeVisible (*voices);
        addAndMakeVisible (*detune);
    }

    for (auto* c : { (Component*) &display, (Component*) &onSwitch, (Component*) wave.get(), (Component*) pw.get(),
                     (Component*) level.get(), (Component*) oct.get(), (Component*) semi.get(), (Component*) fine.get() })
        addAndMakeVisible (c);
}

void OscModule::resized()
{
    onSwitch.setBounds (titleBar().removeFromRight (52));
    auto r = content();
    display.setBounds (r.removeFromTop (78));
    r.removeFromTop (4);
    auto row1 = r.removeFromTop (r.getHeight() / 2);
    if (index == 0)
    {
        layoutRow (row1, { wave.get(), pw.get(), level.get() });
        layoutRow (r, { oct.get(), semi.get(), fine.get() });
    }
    else
    {
        layoutRow (row1, { wave.get(), pw.get(), level.get(), voices.get() });
        layoutRow (r, { oct.get(), semi.get(), fine.get(), detune.get() });
    }
}

//==============================================================================
SubModule::SubModule (BassurdaProcessor& p)
    : ModulePanel ("SUB / PUNCH"), display (p),
      level (p.apvts, subLevel, "SUB"),
      click (p.apvts, clickLevel, "CLICK", C::orange),
      tone (p.apvts, clickTone, "TONE", C::orange),
      wave (p.apvts, subWave, { "SINE", "TRI" }),
      oct (p.apvts, subOct, { "0", "-1", "-2" }),
      clean (p.apvts, subClean, "CLEAN", C::green)
{
    for (auto* c : { (Component*) &display, (Component*) &level, (Component*) &click, (Component*) &tone,
                     (Component*) &wave, (Component*) &oct, (Component*) &clean })
        addAndMakeVisible (c);
    clean.setTooltip ("Clean sub bypasses the filter and saturation, keeping the fundamental pure");
}

void SubModule::resized()
{
    clean.setBounds (titleBar().removeFromRight (70));
    auto r = content();
    display.setBounds (r.removeFromTop (78));
    r.removeFromTop (6);
    auto buttons = r.removeFromTop (22);
    wave.setBounds (buttons.removeFromLeft (buttons.getWidth() * 2 / 5).withTrimmedRight (6));
    oct.setBounds (buttons);
    r.removeFromTop (4);
    layoutRow (r, { &level, &click, &tone });
}

//==============================================================================
ScopeModule::ScopeModule (BassurdaProcessor& p) : ModulePanel ("SCOPE"), scope (p), meter (p)
{
    addAndMakeVisible (scope);
    addAndMakeVisible (meter);
}

void ScopeModule::resized()
{
    auto r = content();
    meter.setBounds (r.removeFromRight (30));
    r.removeFromRight (4);
    scope.setBounds (r);
}

//==============================================================================
FilterModule::FilterModule (BassurdaProcessor& p)
    : ModulePanel ("FILTER"), display (p),
      type (p.apvts, fltType, ParamChoices::fltType),
      cutoff (p.apvts, fltCutoff, "CUTOFF"),
      reso (p.apvts, fltReso, "RESO"),
      env (p.apvts, fltEnv, "ENV", C::orange),
      key (p.apvts, fltKey, "KEY"),
      drive (p.apvts, fltDrive, "DRIVE", C::red)
{
    for (auto* c : { (Component*) &display, (Component*) &type, (Component*) &cutoff, (Component*) &reso,
                     (Component*) &env, (Component*) &key, (Component*) &drive })
        addAndMakeVisible (c);
}

void FilterModule::resized()
{
    auto r = content();
    display.setBounds (r.removeFromTop (78));
    r.removeFromTop (5);
    type.setBounds (r.removeFromTop (22));
    r.removeFromTop (4);
    auto big = r.removeFromLeft (r.getWidth() / 4);
    cutoff.setBounds (big);
    layoutRow (r, { &reso, &env, &key, &drive });
}

//==============================================================================
EnvModule::EnvModule (BassurdaProcessor& p, bool isAmp)
    : ModulePanel (isAmp ? "AMP ENVELOPE" : "FILTER ENVELOPE"), amp (isAmp), display (p, isAmp),
      a (p.apvts, isAmp ? aAtk : fAtk, "ATTACK", isAmp ? C::cyan : C::orange),
      d (p.apvts, isAmp ? aDec : fDec, "DECAY", isAmp ? C::cyan : C::orange),
      s (p.apvts, isAmp ? aSus : fSus, "SUSTAIN", isAmp ? C::cyan : C::orange),
      r (p.apvts, isAmp ? aRel : fRel, "RELEASE", isAmp ? C::cyan : C::orange)
{
    for (auto* c : { (Component*) &display, (Component*) &a, (Component*) &d, (Component*) &s, (Component*) &r })
        addAndMakeVisible (c);
    if (amp)
    {
        vel = std::make_unique<Knob> (p.apvts, velocity, "VELO");
        addAndMakeVisible (*vel);
    }
}

void EnvModule::resized()
{
    auto area = content();
    display.setBounds (area.removeFromTop (78));
    area.removeFromTop (6);
    if (amp) layoutRow (area, { &a, &d, &s, &r, vel.get() });
    else     layoutRow (area, { &a, &d, &s, &r });
}

//==============================================================================
PitchModule::PitchModule (BassurdaProcessor& p)
    : ModulePanel ("PITCH ENV"), display (p),
      amount (p.apvts, pitchAmt, "AMOUNT", C::orange),
      decay (p.apvts, pitchDecay, "DECAY", C::orange),
      toSub (p.apvts, pitchSub, "SUB", C::orange)
{
    for (auto* c : { (Component*) &display, (Component*) &amount, (Component*) &decay, (Component*) &toSub })
        addAndMakeVisible (c);
    toSub.setTooltip ("Apply the pitch drop to the sub oscillator too (classic 808 punch)");
}

void PitchModule::resized()
{
    toSub.setBounds (titleBar().removeFromRight (58));
    auto r = content();
    display.setBounds (r.removeFromTop (78));
    r.removeFromTop (6);
    layoutRow (r, { &amount, &decay });
}

//==============================================================================
VoiceModule::VoiceModule (BassurdaProcessor& p)
    : ModulePanel ("GLIDE / VOICE"),
      glide (p.apvts, glideTime, "GLIDE", C::orange),
      bend (p.apvts, bendRange, "BEND"),
      phase (p.apvts, phaseStart, "PHASE"),
      drift (p.apvts, driftAmount, "DRIFT", C::green),
      glideMode (p.apvts, ParamID::glideMode, { "OFF", "LEGATO", "ALWAYS" }, "GLIDE"),
      glideCurve (p.apvts, ParamID::glideCurve, { "LIN", "EXP" }, "CURVE"),
      envMode (p.apvts, ParamID::envMode, { "RETRIG", "LEGATO" }, "ENV"),
      retrig (p.apvts, phaseRetrig, "PHASE RESET", C::cyan)
{
    for (auto* c : { (Component*) &glide, (Component*) &bend, (Component*) &phase, (Component*) &drift,
                     (Component*) &glideMode, (Component*) &glideCurve, (Component*) &envMode, (Component*) &retrig })
        addAndMakeVisible (c);
    retrig.setTooltip ("Restart every oscillator at the same phase on each note: identical punch on every hit");
}

void VoiceModule::resized()
{
    auto r = content();
    layoutRow (r.removeFromTop (78), { &glide, &bend, &phase, &drift });
    r.removeFromTop (8);
    glideMode.setBounds (r.removeFromTop (22));
    r.removeFromTop (6);
    auto row = r.removeFromTop (22);
    glideCurve.setBounds (row.removeFromLeft (row.getWidth() / 2).withTrimmedRight (6));
    envMode.setBounds (row);
    r.removeFromTop (6);
    retrig.setBounds (r.removeFromTop (22).withWidth (120));
}

//==============================================================================
LfoModule::LfoModule (BassurdaProcessor& p)
    : ModulePanel ("LFO"), display (p),
      shape (p.apvts, lfoShape, { "SIN", "TRI", "SAW", "SQR", "S&H" }),
      rate (p.apvts, lfoRate, "RATE", C::green),
      div (p.apvts, lfoDiv, "SYNC DIV", C::green),
      toCutoff (p.apvts, lfoCutoff, "> CUTOFF", C::green),
      toPitch (p.apvts, lfoPitch, "> PITCH", C::green),
      sync (p.apvts, lfoSync, "SYNC", C::green),
      retrig (p.apvts, lfoRetrig, "RETRIG", C::green)
{
    for (auto* c : { (Component*) &display, (Component*) &shape, (Component*) &rate, (Component*) &div,
                     (Component*) &toCutoff, (Component*) &toPitch, (Component*) &sync, (Component*) &retrig })
        addAndMakeVisible (c);
}

void LfoModule::resized()
{
    auto t = titleBar();
    retrig.setBounds (t.removeFromRight (70));
    t.removeFromRight (4);
    sync.setBounds (t.removeFromRight (60));

    auto r = content();
    display.setBounds (r.removeFromTop (62));
    r.removeFromTop (5);
    shape.setBounds (r.removeFromTop (22));
    r.removeFromTop (3);
    layoutRow (r, { &rate, &div, &toCutoff, &toPitch });
}

//==============================================================================
SaturationModule::SaturationModule (BassurdaProcessor& p)
    : ModulePanel ("SATURATION"), display (p),
      type (p.apvts, satType, { "WARM", "TAPE", "HARD", "FOLD" }),
      band (p.apvts, satBand, { "FULL", "HIGHS" }),
      drive (p.apvts, satDrive, "DRIVE", C::red),
      tone (p.apvts, satTone, "TONE", C::orange),
      mix (p.apvts, satMix, "MIX", C::orange),
      xover (p.apvts, satXover, "X-OVER", C::green)
{
    for (auto* c : { (Component*) &display, (Component*) &type, (Component*) &band, (Component*) &drive,
                     (Component*) &tone, (Component*) &mix, (Component*) &xover })
        addAndMakeVisible (c);
    band.setTooltip ("HIGHS: saturate only above the crossover, the low end stays clean");
}

void SaturationModule::resized()
{
    auto r = content();
    display.setBounds (r.removeFromTop (62));
    r.removeFromTop (5);
    auto row = r.removeFromTop (22);
    band.setBounds (row.removeFromRight (row.getWidth() / 3).withTrimmedLeft (8));
    type.setBounds (row);
    r.removeFromTop (3);
    layoutRow (r, { &drive, &tone, &mix, &xover });
}

//==============================================================================
SampleModule::SampleModule (BassurdaProcessor& p)
    : ModulePanel ("SAMPLE OSC"), proc (p), display (p),
      on (p.apvts, smpOn, "ON", C::cyan),
      loop (p.apvts, smpLoop, "LOOP", C::cyan),
      penv (p.apvts, smpPenv, "P.ENV", C::orange),
      level (p.apvts, smpLevel, "LEVEL"),
      root (p.apvts, smpRoot, "ROOT", C::orange),
      tune (p.apvts, smpTune, "TUNE", C::orange),
      start (p.apvts, smpStart, "START", C::orange)
{
    for (auto* c : { (Component*) &display, (Component*) &loadButton, (Component*) &clearButton, (Component*) &on,
                     (Component*) &loop, (Component*) &penv, (Component*) &level, (Component*) &root,
                     (Component*) &tune, (Component*) &start })
        addAndMakeVisible (c);

    loadButton.setTooltip ("Pick a built-in Bassurda sample or load your own one-shot (WAV/AIFF/FLAC). The root note is detected automatically.");
    penv.setTooltip ("Apply the pitch envelope to the sample too");
    loadButton.onClick = [this]
    {
        PopupMenu m;
        m.addSectionHeader ("BASSURDA SAMPLES");
        const auto names = BassurdaProcessor::getFactorySampleNames();
        for (int i = 0; i < names.size(); ++i)
            m.addItem (100 + i, names[i].upToLastOccurrenceOf ("_", false, false).replaceCharacter ('_', ' '));
        m.addSeparator();
        m.addItem (1, "Load from file...");

        SafePointer<SampleModule> safe (this);
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&loadButton), [safe, names] (int r)
        {
            if (safe == nullptr) return;
            if (r >= 100)
            {
                safe->proc.loadFactorySample (names[r - 100]);
                return;
            }
            if (r != 1) return;
            safe->chooser = std::make_unique<FileChooser> ("Load an 808 / bass sample", File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
            safe->chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                                        [safe] (const FileChooser& fc)
                                        {
                                            if (safe != nullptr && fc.getResult().existsAsFile())
                                                safe->proc.loadSample (fc.getResult());
                                        });
        });
    };
    clearButton.onClick = [this] { proc.clearSample(); };
}

void SampleModule::resized()
{
    auto t = titleBar();
    on.setBounds (t.removeFromRight (52));

    auto r = content();
    auto left = r.removeFromLeft (r.getWidth() * 55 / 100);
    display.setBounds (left.removeFromTop (100));
    left.removeFromTop (6);
    auto row = left.removeFromTop (24);
    const int w = row.getWidth() / 4;
    loadButton.setBounds (row.removeFromLeft (w).reduced (2, 0));
    clearButton.setBounds (row.removeFromLeft (w).reduced (2, 0));
    loop.setBounds (row.removeFromLeft (w).reduced (2, 0));
    penv.setBounds (row.reduced (2, 0));

    r.removeFromLeft (6);
    auto top = r.removeFromTop (r.getHeight() / 2);
    layoutRow (top, { &level, &root });
    layoutRow (r, { &tune, &start });
}

//==============================================================================
DuckModule::DuckModule (BassurdaProcessor& p)
    : ModulePanel ("KICK DUCK"), display (p),
      mode (p.apvts, duckMode, { "OFF", "NOTE", "CH 10", "BEAT" }),
      note (p.apvts, duckNote, "NOTE", C::red),
      depth (p.apvts, duckDepth, "DEPTH", C::red),
      attack (p.apvts, duckAttack, "ATTACK", C::red),
      hold (p.apvts, duckHold, "HOLD", C::red),
      release (p.apvts, duckRelease, "RELEASE", C::red)
{
    for (auto* c : { (Component*) &display, (Component*) &mode, (Component*) &note, (Component*) &depth,
                     (Component*) &attack, (Component*) &hold, (Component*) &release })
        addAndMakeVisible (c);
    mode.setTooltip ("NOTE: the chosen MIDI note ducks the bass (and is not played). "
                     "CH 10: any note on MIDI channel 10. BEAT: every beat of the host tempo.");
}

void DuckModule::resized()
{
    auto r = content();
    auto left = r.removeFromLeft (r.getWidth() * 42 / 100);
    display.setBounds (left.removeFromTop (100));
    left.removeFromTop (6);
    mode.setBounds (left.removeFromTop (24));

    r.removeFromLeft (6);
    auto top = r.removeFromTop (r.getHeight() / 2);
    layoutRow (top, { &note, &depth, &attack });
    layoutRow (r, { &hold, &release });
}

//==============================================================================
OutputModule::OutputModule (BassurdaProcessor& p)
    : ModulePanel ("OUTPUT"),
      gain (p.apvts, outGain, "GAIN"),
      lowCut (p.apvts, monoCut, "LOW CUT"),
      width (p.apvts, ParamID::width, "WIDTH", C::green),
      monoBelow (p.apvts, ParamID::monoBelow, "MONO <", C::green),
      clip (p.apvts, outClip, "CLIP", C::red)
{
    for (auto* c : { (Component*) &gain, (Component*) &lowCut, (Component*) &width, (Component*) &monoBelow, (Component*) &clip })
        addAndMakeVisible (c);
    width.slider.setTooltip ("Stereo width of the upper harmonics. Mono-compatible: it cancels when summed to mono");
    monoBelow.slider.setTooltip ("Everything below this frequency stays mono (the sub never moves)");
}

void OutputModule::resized()
{
    clip.setBounds (titleBar().removeFromRight (62));
    auto r = content();
    auto top = r.removeFromTop (r.getHeight() / 2);
    layoutRow (top, { &gain, &lowCut });
    layoutRow (r, { &width, &monoBelow });
}
