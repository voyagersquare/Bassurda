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
      voice (p), lfo (p), saturation (p), sample (p), duck (p), output (p),
      keyboard (p.keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    for (auto* c : { (Component*) &oscA, (Component*) &oscB, (Component*) &sub, (Component*) &scope,
                     (Component*) &filter, (Component*) &filterEnv, (Component*) &ampEnv, (Component*) &pitch,
                     (Component*) &voice, (Component*) &lfo, (Component*) &saturation, (Component*) &output,
                     (Component*) &sample, (Component*) &duck, (Component*) &menuButton,
                     (Component*) &keyboard, (Component*) &presetBox, (Component*) &prevButton, (Component*) &nextButton })
        addAndMakeVisible (c);

    keyboard.setAvailableRange (24, 84);
    keyboard.setLowestVisibleKey (24);
    keyboard.setOctaveForMiddleC (3);
    keyboard.setWantsKeyboardFocus (false);

    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id <= 0) return;
        if (id < 1000)
        {
            proc.setCurrentProgram (id - 1);
        }
        else if (juce::isPositiveAndBelow (id - 1000, userPresets.size()))
        {
            if (! proc.loadUserPreset (userPresets[id - 1000]))
                AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "Bassurda", "Could not load this preset.");
        }
    };
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    menuButton.onClick = [this] { showPresetMenu(); };
    prevButton.setTooltip ("Previous preset");
    nextButton.setTooltip ("Next preset");
    menuButton.setTooltip ("Save, delete and manage your own presets");

    setSize (designWidth, designHeight);
    startTimerHz (5);
}

void MainView::refreshPresetBox()
{
    presetBox.clear (dontSendNotification);
    presetBox.addSectionHeading ("FACTORY");
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);

    userPresets = proc.getUserPresets();
    if (! userPresets.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("USER");
        for (int i = 0; i < userPresets.size(); ++i)
            presetBox.addItem (userPresets[i].getFileNameWithoutExtension(), 1000 + i);
    }

    // select the entry that matches the current preset name
    const auto name = proc.getCurrentPresetName();
    int selected = 0;
    if (proc.getCurrentProgram() >= 0 && proc.getProgramName (proc.getCurrentProgram()) == name)
        selected = proc.getCurrentProgram() + 1;
    else
        for (int i = 0; i < userPresets.size(); ++i)
            if (userPresets[i].getFileNameWithoutExtension() == name)
                selected = 1000 + i;

    if (selected > 0)
        presetBox.setSelectedId (selected, dontSendNotification);
    else
        presetBox.setText (name, dontSendNotification);
}

void MainView::timerCallback()
{
    const int c = proc.presetChangeCounter.load();
    if (c != lastPresetCounter)
    {
        lastPresetCounter = c;
        refreshPresetBox();
    }
}

void MainView::stepPreset (int delta)
{
    const int n = presetBox.getNumItems();
    if (n == 0) return;
    const int idx = presetBox.getSelectedItemIndex();
    const int next = idx < 0 ? 0 : (idx + delta + n) % n;
    presetBox.setSelectedItemIndex (next, sendNotificationSync);
}

void MainView::showPresetMenu()
{
    PopupMenu m;
    const auto current = proc.getCurrentPresetName();
    const auto currentFile = BassurdaProcessor::getUserPresetFolder().getChildFile (File::createLegalFileName (current) + ".bassurda");

    m.addItem (1, "Save preset as...");
    m.addItem (2, "Delete \"" + current + "\"", currentFile.existsAsFile());
    m.addSeparator();
    m.addItem (3, "Open preset folder");
    m.addItem (4, "Refresh list");

    SafePointer<MainView> safe (this);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&menuButton), [safe, currentFile] (int r)
    {
        if (safe == nullptr) return;
        if (r == 1) safe->savePresetDialog();
        if (r == 2)
        {
            currentFile.deleteFile();
            safe->refreshPresetBox();
        }
        if (r == 3)
        {
            BassurdaProcessor::getUserPresetFolder().createDirectory();
            BassurdaProcessor::getUserPresetFolder().revealToUser();
        }
        if (r == 4) safe->refreshPresetBox();
    });
}

void MainView::savePresetDialog()
{
    saveWindow = std::make_unique<AlertWindow> ("Save preset", "Name your sound:", MessageBoxIconType::NoIcon, this);
    saveWindow->addTextEditor ("name", proc.getCurrentPresetName());
    saveWindow->addButton ("Save", 1, KeyPress (KeyPress::returnKey));
    saveWindow->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));

    SafePointer<MainView> safe (this);
    saveWindow->enterModalState (true, ModalCallbackFunction::create ([safe] (int result)
    {
        if (safe == nullptr || safe->saveWindow == nullptr) return;
        const auto name = safe->saveWindow->getTextEditorContents ("name");
        safe->saveWindow->setVisible (false);
        if (result == 1)
        {
            if (safe->proc.saveUserPreset (name))
                safe->refreshPresetBox();
            else
                AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "Bassurda", "Could not save the preset.");
        }
    }), false);
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
        const int w = 410, h = 28;
        auto r = Rectangle<int> (0, 0, w, h).withCentre ({ designWidth / 2 + 10, 33 });
        prevButton.setBounds (r.removeFromLeft (30));
        menuButton.setBounds (r.removeFromRight (70).withTrimmedLeft (4));
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
    lfo.setBounds (row3.removeFromLeft (330));        row3.removeFromLeft (gap);
    saturation.setBounds (row3);
    area.removeFromTop (gap);

    auto row4 = area.removeFromTop (208);
    sample.setBounds (row4.removeFromLeft (470));     row4.removeFromLeft (gap);
    duck.setBounds (row4.removeFromLeft (470));       row4.removeFromLeft (gap);
    output.setBounds (row4);
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

    // start at a size that fits the screen (the window can be resized)
    double scale = 0.85;
    if (auto* display = Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        const auto area = display->userArea;
        scale = jmin (1.0, (area.getHeight() - 80) / (double) MainView::designHeight,
                      (area.getWidth() - 40) / (double) MainView::designWidth);
    }
    scale = jmax (0.5, scale);
    setSize (roundToInt (MainView::designWidth * scale), roundToInt (MainView::designHeight * scale));
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
