#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
APatheticSynthAudioProcessorEditor::APatheticSynthAudioProcessorEditor (APatheticSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      keyboardComponent (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard),
      levelMeter (p)
{
    setLookAndFeel (&organLookAndFeel);
    // Main panel stays 1280x960 (4:3); extra width on the right is for the level meter only
    setSize (1280 + 72, 960);
    setResizable (false, false);

    titleLabel.setText ("aPathetic", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (26.0f).withStyle ("Bold"));
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setColour (juce::Label::textColourId, OrganLookAndFeel::gold());
    addAndMakeVisible (titleLabel);

    modelLabel.setText ("Electronic Organ Model AP-01", juce::dontSendNotification);
    modelLabel.setFont (juce::FontOptions (12.0f));
    modelLabel.setJustificationType (juce::Justification::centredLeft);
    modelLabel.setColour (juce::Label::textColourId, OrganLookAndFeel::silkScreen());
    addAndMakeVisible (modelLabel);

    // ---- Presets ----
    presetLabel.setText ("MEMORY", juce::dontSendNotification);
    presetLabel.setColour (juce::Label::textColourId, OrganLookAndFeel::silkScreen());
    presetLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (presetLabel);

    presetBox.setTextWhenNothingSelected ("- SELECT -");
    presetBox.setTextWhenNoChoicesAvailable ("EMPTY");
    presetBox.onChange = [this] { presetSelected(); };
    addAndMakeVisible (presetBox);

    setupTextButton (savePresetButton);
    savePresetButton.setTooltip ("Save the current sound as a preset");
    savePresetButton.onClick = [this] { savePresetClicked(); };

    setupTextButton (deletePresetButton);
    deletePresetButton.setTooltip ("Delete the selected preset");
    deletePresetButton.onClick = [this] { deletePresetClicked(); };

    setupTextButton (initPresetButton);
    initPresetButton.setTooltip ("Reset all parameters to defaults");
    initPresetButton.onClick = [this] { initPresetClicked(); };

    setupTextButton (randomPresetButton);
    randomPresetButton.setTooltip ("Randomise every setting (all 3 oscillators included)");
    randomPresetButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffe8a040));
    randomPresetButton.onClick = [this] { randomPresetClicked(); };

    refreshPresetList();

    auto& apvts = audioProcessor.getAPVTS();

    setupSectionLabel (oscillatorsSectionLabel, "TONE GENERATORS");
    for (int i = 0; i < 3; ++i)
        setupOscillator (oscillators[i], i + 1, apvts);

    setupSectionLabel (ampSectionLabel, "EXPRESSION  /  ENVELOPE");
    setupSectionLabel (filterSectionLabel, "TONE FILTER");
    setupSectionLabel (unisonSectionLabel, "CELESTE  /  ENSEMBLE");
    setupSectionLabel (lfoSectionLabel, "VIBRATO  /  MODULATION");
    setupSectionLabel (chorusSectionLabel, "CHORUS");
    setupSectionLabel (delaySectionLabel, "ECHO  /  DELAY");
    setupSectionLabel (reverbSectionLabel, "HALL  /  REVERB");

    setupSlider (attackSlider,  attackLabel,  "ATTACK");
    setupSlider (decaySlider,   decayLabel,   "DECAY");
    setupSlider (sustainSlider, sustainLabel, "SUSTAIN");
    setupSlider (releaseSlider, releaseLabel, "RELEASE");
    setupSlider (gainSlider,    gainLabel,    "VOLUME");
    setupSlider (glideSlider,   glideLabel,   "GLIDE");
    setupSlider (cutoffSlider,  cutoffLabel,  "CUTOFF");
    setupSlider (resonanceSlider, resonanceLabel, "RESO");
    setupSlider (driveSlider,   driveLabel,   "DRIVE");
    setupSlider (filtEnvAttackSlider,  filtEnvAttackLabel,  "F.ATK");
    setupSlider (filtEnvDecaySlider,   filtEnvDecayLabel,   "F.DEC");
    setupSlider (filtEnvSustainSlider, filtEnvSustainLabel, "F.SUS");
    setupSlider (filtEnvReleaseSlider, filtEnvReleaseLabel, "F.REL");
    setupSlider (filtEnvAmountSlider,  filtEnvAmountLabel,  "F.AMT");
    setupSlider (unisonVoicesSlider, unisonVoicesLabel, "VOICES");
    setupSlider (unisonDetuneSlider, unisonDetuneLabel, "DETUNE");
    setupSlider (unisonSpreadSlider, unisonSpreadLabel, "WIDTH");
    setupSlider (lfoRateSlider,  lfoRateLabel,  "SPEED");
    setupSlider (lfoDepthSlider, lfoDepthLabel, "DEPTH");
    setupSlider (chorusRateSlider,    chorusRateLabel,    "RATE");
    setupSlider (chorusDepthSlider,   chorusDepthLabel,   "DEPTH");
    setupSlider (chorusMixSlider,     chorusMixLabel,     "C.MIX");
    setupSlider (delayTimeSlider,     delayTimeLabel,     "TIME");
    setupSlider (delayFeedbackSlider, delayFeedbackLabel, "FDBK");
    setupSlider (delayMixSlider,      delayMixLabel,      "D.MIX");
    setupSlider (reverbSizeSlider,    reverbSizeLabel,    "SIZE");
    setupSlider (reverbDampingSlider, reverbDampingLabel, "DAMP");
    setupSlider (reverbWidthSlider,   reverbWidthLabel,   "WIDTH");
    setupSlider (reverbMixSlider,     reverbMixLabel,     "R.MIX");

    cutoffSlider.setTextValueSuffix (" Hz");
    cutoffSlider.setNumDecimalPlacesToDisplay (0);

    unisonVoicesSlider.setNumDecimalPlacesToDisplay (0);
    unisonDetuneSlider.setTextValueSuffix (" ct");
    unisonDetuneSlider.setNumDecimalPlacesToDisplay (1);

    lfoRateSlider.setTextValueSuffix (" Hz");
    lfoRateSlider.setNumDecimalPlacesToDisplay (2);
    lfoDepthSlider.setNumDecimalPlacesToDisplay (2);

    glideSlider.setTextValueSuffix (" s");
    glideSlider.setNumDecimalPlacesToDisplay (3);

    delayTimeSlider.setTextValueSuffix (" s");
    delayTimeSlider.setNumDecimalPlacesToDisplay (3);
    chorusRateSlider.setTextValueSuffix (" Hz");
    chorusRateSlider.setNumDecimalPlacesToDisplay (2);

    attackAttachment  = std::make_unique<SliderAttachment> (apvts, "attack",  attackSlider);
    decayAttachment   = std::make_unique<SliderAttachment> (apvts, "decay",   decaySlider);
    sustainAttachment = std::make_unique<SliderAttachment> (apvts, "sustain", sustainSlider);
    releaseAttachment = std::make_unique<SliderAttachment> (apvts, "release", releaseSlider);
    gainAttachment    = std::make_unique<SliderAttachment> (apvts, "gain",    gainSlider);
    cutoffAttachment  = std::make_unique<SliderAttachment> (apvts, "filterCutoff", cutoffSlider);
    resonanceAttachment = std::make_unique<SliderAttachment> (apvts, "filterResonance", resonanceSlider);
    driveAttachment = std::make_unique<SliderAttachment> (apvts, "drive", driveSlider);
    filtEnvAttackAttachment  = std::make_unique<SliderAttachment> (apvts, "filterEnvAttack",  filtEnvAttackSlider);
    filtEnvDecayAttachment   = std::make_unique<SliderAttachment> (apvts, "filterEnvDecay",   filtEnvDecaySlider);
    filtEnvSustainAttachment = std::make_unique<SliderAttachment> (apvts, "filterEnvSustain", filtEnvSustainSlider);
    filtEnvReleaseAttachment = std::make_unique<SliderAttachment> (apvts, "filterEnvRelease", filtEnvReleaseSlider);
    filtEnvAmountAttachment  = std::make_unique<SliderAttachment> (apvts, "filterEnvAmount",  filtEnvAmountSlider);

    filtEnvAmountSlider.textFromValueFunction = [] (double value)
    {
        return juce::String (juce::roundToInt (value * 100.0)) + " %";
    };
    filtEnvAmountSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        return text.retainCharacters ("0123456789.-").getDoubleValue() / 100.0;
    };
    filtEnvAmountSlider.updateText();

    unisonVoicesAttachment = std::make_unique<SliderAttachment> (apvts, "unisonVoices", unisonVoicesSlider);
    unisonDetuneAttachment = std::make_unique<SliderAttachment> (apvts, "unisonDetune", unisonDetuneSlider);
    unisonSpreadAttachment = std::make_unique<SliderAttachment> (apvts, "unisonSpread", unisonSpreadSlider);

    // Show 0-100% (parameter is stored 0-1); set after attachment so it sticks
    unisonSpreadSlider.textFromValueFunction = [] (double value)
    {
        return juce::String (juce::roundToInt (value * 100.0)) + " %";
    };
    unisonSpreadSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        return text.retainCharacters ("0123456789.-").getDoubleValue() / 100.0;
    };
    unisonSpreadSlider.updateText();
    lfoRateAttachment  = std::make_unique<SliderAttachment> (apvts, "lfoRate",  lfoRateSlider);
    lfoDepthAttachment = std::make_unique<SliderAttachment> (apvts, "lfoDepth", lfoDepthSlider);
    chorusRateAttachment  = std::make_unique<SliderAttachment> (apvts, "chorusRate",  chorusRateSlider);
    chorusDepthAttachment = std::make_unique<SliderAttachment> (apvts, "chorusDepth", chorusDepthSlider);
    chorusMixAttachment   = std::make_unique<SliderAttachment> (apvts, "chorusMix",   chorusMixSlider);
    delayTimeAttachment     = std::make_unique<SliderAttachment> (apvts, "delayTime",     delayTimeSlider);
    delayFeedbackAttachment = std::make_unique<SliderAttachment> (apvts, "delayFeedback", delayFeedbackSlider);
    delayMixAttachment      = std::make_unique<SliderAttachment> (apvts, "delayMix",      delayMixSlider);
    reverbSizeAttachment    = std::make_unique<SliderAttachment> (apvts, "reverbSize",    reverbSizeSlider);
    reverbDampingAttachment = std::make_unique<SliderAttachment> (apvts, "reverbDamping", reverbDampingSlider);
    reverbWidthAttachment   = std::make_unique<SliderAttachment> (apvts, "reverbWidth",   reverbWidthSlider);
    reverbMixAttachment     = std::make_unique<SliderAttachment> (apvts, "reverbMix",     reverbMixSlider);

    delaySyncButton.setTooltip ("Lock delay time to the host tempo (FL project BPM)");
    addAndMakeVisible (delaySyncButton);
    delaySyncAttachment = std::make_unique<ButtonAttachment> (apvts, "delaySync", delaySyncButton);

    delayDivisionLabel.setText ("NOTE", juce::dontSendNotification);
    delayDivisionLabel.setJustificationType (juce::Justification::centred);
    delayDivisionLabel.setColour (juce::Label::textColourId, OrganLookAndFeel::silkScreen());
    delayDivisionLabel.setFont (juce::FontOptions (10.0f));
    addAndMakeVisible (delayDivisionLabel);

    delayDivisionBox.addItemList (
        juce::StringArray { "1/1", "1/2", "1/4", "1/4.", "1/8", "1/8.", "1/8T", "1/16", "1/16.", "1/32" }, 1);
    addAndMakeVisible (delayDivisionBox);
    delayDivisionAttachment = std::make_unique<ComboAttachment> (apvts, "delayDivision", delayDivisionBox);

    fixedVelocityButton.setTooltip ("Play every note at full velocity");
    addAndMakeVisible (fixedVelocityButton);
    fixedVelocityAttachment = std::make_unique<ButtonAttachment> (apvts, "fixedVelocity", fixedVelocityButton);

    glideAttachment = std::make_unique<SliderAttachment> (apvts, "glideTime", glideSlider);
    glideLegatoButton.setTooltip ("Only glide when another note is already held (legato)");
    addAndMakeVisible (glideLegatoButton);
    glideLegatoAttachment = std::make_unique<ButtonAttachment> (apvts, "glideLegato", glideLegatoButton);

    filterTypeLabel.setText ("TYPE", juce::dontSendNotification);
    filterTypeLabel.setJustificationType (juce::Justification::centred);
    filterTypeLabel.setColour (juce::Label::textColourId, OrganLookAndFeel::silkScreen());
    filterTypeLabel.setFont (juce::FontOptions (10.0f));
    addAndMakeVisible (filterTypeLabel);

    filterTypeBox.addItemList (juce::StringArray { "Off", "Low Pass", "High Pass", "Band Pass" }, 1);
    addAndMakeVisible (filterTypeBox);
    filterTypeAttachment = std::make_unique<ComboAttachment> (apvts, "filterType", filterTypeBox);

    lfoWaveformLabel.setText ("SHAPE", juce::dontSendNotification);
    lfoWaveformLabel.setJustificationType (juce::Justification::centred);
    lfoWaveformLabel.setColour (juce::Label::textColourId, OrganLookAndFeel::silkScreen());
    lfoWaveformLabel.setFont (juce::FontOptions (10.0f));
    addAndMakeVisible (lfoWaveformLabel);

    lfoWaveformBox.addItemList (juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1);
    addAndMakeVisible (lfoWaveformBox);
    lfoWaveformAttachment = std::make_unique<ComboAttachment> (apvts, "lfoWaveform", lfoWaveformBox);

    lfoTargetLabel.setText ("DEST", juce::dontSendNotification);
    lfoTargetLabel.setJustificationType (juce::Justification::centred);
    lfoTargetLabel.setColour (juce::Label::textColourId, OrganLookAndFeel::silkScreen());
    lfoTargetLabel.setFont (juce::FontOptions (10.0f));
    addAndMakeVisible (lfoTargetLabel);

    lfoTargetBox.addItemList (juce::StringArray { "Off", "Filter", "Pitch", "Amplitude" }, 1);
    addAndMakeVisible (lfoTargetBox);
    lfoTargetAttachment = std::make_unique<ComboAttachment> (apvts, "lfoTarget", lfoTargetBox);

    lfoRetriggerButton.setTooltip ("Restart the LFO phase on each new note");
    addAndMakeVisible (lfoRetriggerButton);
    lfoRetriggerAttachment = std::make_unique<ButtonAttachment> (apvts, "lfoRetrigger", lfoRetriggerButton);

    styleKeyboard();
    // FL-style labeling: MIDI 60 = C5 (pitch unchanged). Range C2–C8 is MIDI 24–96,
    // so the on-screen board sits an octave lower than the old C3–C9 span.
    keyboardComponent.setAvailableRange (24, 96); // C2 – C8 with middle C = C5
    keyboardComponent.setOctaveForMiddleC (5);
    addAndMakeVisible (keyboardComponent);

    addAndMakeVisible (levelMeter);

    startTimerHz (30);
}

APatheticSynthAudioProcessorEditor::~APatheticSynthAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void APatheticSynthAudioProcessorEditor::styleKeyboard()
{
    keyboardComponent.setColour (juce::MidiKeyboardComponent::whiteNoteColourId,
                                 OrganLookAndFeel::ivory());
    keyboardComponent.setColour (juce::MidiKeyboardComponent::blackNoteColourId,
                                 juce::Colour (0xff1a120c));
    keyboardComponent.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,
                                 juce::Colour (0xff8a7a60));
    keyboardComponent.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                                 OrganLookAndFeel::amber().withAlpha (0.35f));
    keyboardComponent.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,
                                 OrganLookAndFeel::amber().withAlpha (0.55f));
    keyboardComponent.setColour (juce::MidiKeyboardComponent::shadowColourId,
                                 juce::Colours::black.withAlpha (0.4f));
}

void APatheticSynthAudioProcessorEditor::fitKeyboardToWidth (int pixelWidth)
{
    // Expand white-key width so C2-C8 fills the keyboard component edge-to-edge
    int whiteKeys = 0;
    constexpr int lowest = 36;  // C2
    constexpr int highest = 108; // C8

    for (int note = lowest; note <= highest; ++note)
        if (! juce::MidiMessage::isMidiNoteBlack (note))
            ++whiteKeys;

    if (whiteKeys > 0 && pixelWidth > 0)
        keyboardComponent.setKeyWidth ((float) pixelWidth / (float) whiteKeys);
}

void APatheticSynthAudioProcessorEditor::setupSectionLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    label.setColour (juce::Label::textColourId, OrganLookAndFeel::gold());
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);
}

void APatheticSynthAudioProcessorEditor::setupSlider (juce::Slider& slider, juce::Label& label,
                                                      const juce::String& name, bool drawbarStyle)
{
    if (drawbarStyle)
    {
        slider.setSliderStyle (juce::Slider::LinearVertical);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 40, 12);
        slider.setName (name); // used by look-and-feel for drawbar colour
    }
    else
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 48, 12);
    }

    slider.setColour (juce::Slider::rotarySliderFillColourId, OrganLookAndFeel::amber());
    slider.setColour (juce::Slider::thumbColourId, OrganLookAndFeel::ivory());
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, OrganLookAndFeel::silkScreen());
    label.setFont (juce::FontOptions (10.0f));
    label.attachToComponent (&slider, false);
    addAndMakeVisible (label);
}

void APatheticSynthAudioProcessorEditor::setupTextButton (juce::TextButton& button)
{
    button.setColour (juce::TextButton::buttonColourId, OrganLookAndFeel::ivoryDark());
    button.setColour (juce::TextButton::buttonOnColourId, OrganLookAndFeel::amber());
    button.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff2a2010));
    button.setColour (juce::TextButton::textColourOnId, juce::Colour (0xff1a1000));
    addAndMakeVisible (button);
}

void APatheticSynthAudioProcessorEditor::setupOscillator (OscillatorControls& osc, int index,
                                                          juce::AudioProcessorValueTreeState& apvts)
{
    const auto n = juce::String (index);

    osc.titleLabel.setText ("OSC " + n, juce::dontSendNotification);
    osc.titleLabel.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
    osc.titleLabel.setColour (juce::Label::textColourId, OrganLookAndFeel::amberGlow());
    addAndMakeVisible (osc.titleLabel);

    osc.waveLabel.setText ("WAVE", juce::dontSendNotification);
    osc.waveLabel.setJustificationType (juce::Justification::centred);
    osc.waveLabel.setColour (juce::Label::textColourId, OrganLookAndFeel::silkScreen());
    osc.waveLabel.setFont (juce::FontOptions (11.0f));
    addAndMakeVisible (osc.waveLabel);

    osc.waveBox.addItemList (juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1);
    addAndMakeVisible (osc.waveBox);

    // Drawbar level: label is placed manually (not attached) so rows never overlap
    osc.levelSlider.setSliderStyle (juce::Slider::LinearVertical);
    osc.levelSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 48, 14);
    osc.levelSlider.setName (n); // used by look-and-feel for drawbar colour
    osc.levelSlider.setColour (juce::Slider::thumbColourId, OrganLookAndFeel::ivory());
    addAndMakeVisible (osc.levelSlider);

    osc.levelLabel.setText ("LEVEL", juce::dontSendNotification);
    osc.levelLabel.setJustificationType (juce::Justification::centred);
    osc.levelLabel.setColour (juce::Label::textColourId, OrganLookAndFeel::silkScreen());
    osc.levelLabel.setFont (juce::FontOptions (11.0f));
    addAndMakeVisible (osc.levelLabel);

    setupSlider (osc.semiSlider,  osc.semiLabel,  "SEMI");
    setupSlider (osc.fineSlider,  osc.fineLabel,  "FINE");

    osc.semiSlider.setNumDecimalPlacesToDisplay (0);
    osc.semiSlider.setTextValueSuffix (" st");
    osc.fineSlider.setNumDecimalPlacesToDisplay (1);
    osc.fineSlider.setTextValueSuffix (" ct");

    osc.waveAttachment  = std::make_unique<ComboAttachment>  (apvts, "osc" + n + "Wave",  osc.waveBox);
    osc.levelAttachment = std::make_unique<SliderAttachment> (apvts, "osc" + n + "Level", osc.levelSlider);
    osc.semiAttachment  = std::make_unique<SliderAttachment> (apvts, "osc" + n + "Semi",  osc.semiSlider);
    osc.fineAttachment  = std::make_unique<SliderAttachment> (apvts, "osc" + n + "Fine",  osc.fineSlider);
}

void APatheticSynthAudioProcessorEditor::layoutOscillatorColumn (OscillatorControls& osc,
                                                                 juce::Rectangle<int> column)
{
    // Compact vertical column for side-by-side OSC 1/2/3
    column = column.reduced (4, 2);
    osc.titleLabel.setBounds (column.removeFromTop (16));

    osc.waveLabel.setBounds (column.removeFromTop (12));
    osc.waveBox.setBounds (column.removeFromTop (22).reduced (2, 0));
    column.removeFromTop (4);

    auto body = column;
    auto levelCol = body.removeFromLeft (juce::jmax (48, body.getWidth() / 3)).reduced (2, 0);
    osc.levelLabel.setBounds (levelCol.removeFromTop (12));
    osc.levelSlider.setBounds (levelCol.reduced (8, 0));

    auto knobs = body.reduced (2, 0);
    const int halfH = knobs.getHeight() / 2;
    auto semiArea = knobs.removeFromTop (halfH).reduced (2, 2);
    auto fineArea = knobs.reduced (2, 2);
    osc.semiSlider.setBounds (semiArea.withTrimmedTop (12));
    osc.fineSlider.setBounds (fineArea.withTrimmedTop (12));
}

void APatheticSynthAudioProcessorEditor::layoutKnobRow (juce::Rectangle<int> row,
                                                        std::initializer_list<juce::Component*> knobs)
{
    if (knobs.size() == 0)
        return;

    const int w = row.getWidth() / (int) knobs.size();

    for (auto* c : knobs)
    {
        if (c != nullptr)
            c->setBounds (row.removeFromLeft (w).reduced (3, 2).withTrimmedTop (12));
    }
}

//==============================================================================
void APatheticSynthAudioProcessorEditor::paintWoodPanel (juce::Graphics& g, juce::Rectangle<float> area) const
{
    juce::ColourGradient wood (OrganLookAndFeel::woodLight(), area.getX(), area.getY(),
                               OrganLookAndFeel::woodDark(), area.getRight(), area.getBottom(), false);
    g.setGradientFill (wood);
    g.fillRect (area);

    // Grain lines
    g.setColour (juce::Colours::black.withAlpha (0.06f));
    for (float y = area.getY(); y < area.getBottom(); y += 3.0f)
        g.drawHorizontalLine ((int) y, area.getX(), area.getRight());

    g.setColour (juce::Colours::white.withAlpha (0.04f));
    for (float y = area.getY() + 1.0f; y < area.getBottom(); y += 7.0f)
        g.drawHorizontalLine ((int) y, area.getX(), area.getRight());
}

void APatheticSynthAudioProcessorEditor::paintControlPanel (juce::Graphics& g, juce::Rectangle<float> area) const
{
    // Recessed black plastic panel
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (area.translated (0.0f, 2.0f), 6.0f);

    juce::ColourGradient panel (juce::Colour (0xff2a221c), area.getX(), area.getY(),
                                OrganLookAndFeel::panelBlack(), area.getX(), area.getBottom(), false);
    g.setGradientFill (panel);
    g.fillRoundedRectangle (area, 6.0f);

    g.setColour (OrganLookAndFeel::goldDim().withAlpha (0.45f));
    g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.2f);

    // Inner bevel highlight
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (area.reduced (2.0f), 5.0f, 1.0f);
}

void APatheticSynthAudioProcessorEditor::paintNameplate (juce::Graphics& g, juce::Rectangle<float> area) const
{
    juce::ColourGradient plate (juce::Colour (0xff4a3828), area.getX(), area.getY(),
                                juce::Colour (0xff2a1c12), area.getX(), area.getBottom(), false);
    g.setGradientFill (plate);
    g.fillRoundedRectangle (area, 4.0f);

    g.setColour (OrganLookAndFeel::gold());
    g.drawRoundedRectangle (area.reduced (1.0f), 4.0f, 1.5f);
    g.setColour (OrganLookAndFeel::goldDim().withAlpha (0.5f));
    g.drawRoundedRectangle (area.reduced (3.5f), 3.0f, 1.0f);
}

//==============================================================================
void APatheticSynthAudioProcessorEditor::refreshPresetList()
{
    ignorePresetBoxCallback = true;

    const auto previous = presetBox.getText();
    presetBox.clear (juce::dontSendNotification);

    const auto names = audioProcessor.getPresetManager().getPresetNames();
    int id = 1;

    for (const auto& name : names)
        presetBox.addItem (name, id++);

    if (previous.isNotEmpty())
    {
        const auto index = names.indexOf (previous);

        if (index >= 0)
            presetBox.setSelectedItemIndex (index, juce::dontSendNotification);
    }
    else
    {
        const auto current = audioProcessor.getPresetManager().getCurrentPresetName();
        const auto index = names.indexOf (current);

        if (index >= 0)
            presetBox.setSelectedItemIndex (index, juce::dontSendNotification);
    }

    ignorePresetBoxCallback = false;
}

void APatheticSynthAudioProcessorEditor::savePresetClicked()
{
    auto* aw = new juce::AlertWindow ("Save Memory",
                                      "Enter a name for this registration:",
                                      juce::MessageBoxIconType::QuestionIcon);

    const auto suggested = audioProcessor.getPresetManager().getCurrentPresetName().isNotEmpty()
                         ? audioProcessor.getPresetManager().getCurrentPresetName()
                         : "My Preset";

    aw->addTextEditor ("name", suggested, "Name:");
    aw->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    aw->enterModalState (true, juce::ModalCallbackFunction::create (
        [this, aw] (int result)
        {
            if (result == 1)
            {
                const auto name = aw->getTextEditorContents ("name").trim();

                if (name.isNotEmpty())
                {
                    if (audioProcessor.getPresetManager().savePreset (name))
                    {
                        audioProcessor.getPresetManager().setCurrentPresetName (name);
                        refreshPresetList();

                        ignorePresetBoxCallback = true;
                        for (int i = 0; i < presetBox.getNumItems(); ++i)
                        {
                            if (presetBox.getItemText (i) == name)
                            {
                                presetBox.setSelectedItemIndex (i, juce::dontSendNotification);
                                break;
                            }
                        }
                        ignorePresetBoxCallback = false;
                    }
                    else
                    {
                        juce::AlertWindow::showMessageBoxAsync (
                            juce::MessageBoxIconType::WarningIcon,
                            "Save failed",
                            "Could not save the preset. Check the name and try again.");
                    }
                }
            }

            delete aw;
        }), true);
}

void APatheticSynthAudioProcessorEditor::deletePresetClicked()
{
    const auto name = presetBox.getText();

    if (name.isEmpty())
        return;

    juce::AlertWindow::showOkCancelBox (
        juce::MessageBoxIconType::WarningIcon,
        "Delete Memory",
        "Delete preset \"" + name + "\"?",
        "Delete",
        "Cancel",
        this,
        juce::ModalCallbackFunction::create ([this, name] (int result)
        {
            if (result != 0)
            {
                if (audioProcessor.getPresetManager().deletePreset (name))
                {
                    if (audioProcessor.getPresetManager().getCurrentPresetName() == name)
                        audioProcessor.getPresetManager().setCurrentPresetName ({});

                    refreshPresetList();
                }
            }
        }));
}

void APatheticSynthAudioProcessorEditor::initPresetClicked()
{
    audioProcessor.getPresetManager().loadInit();
    clearPresetSelection();
}

void APatheticSynthAudioProcessorEditor::randomPresetClicked()
{
    audioProcessor.getPresetManager().randomiseAll();
    clearPresetSelection();
}

void APatheticSynthAudioProcessorEditor::clearPresetSelection()
{
    ignorePresetBoxCallback = true;
    presetBox.setSelectedItemIndex (-1, juce::dontSendNotification);
    ignorePresetBoxCallback = false;
}

void APatheticSynthAudioProcessorEditor::presetSelected()
{
    if (ignorePresetBoxCallback)
        return;

    const auto name = presetBox.getText();

    if (name.isEmpty())
        return;

    if (audioProcessor.getPresetManager().loadPreset (name))
        audioProcessor.getPresetManager().setCurrentPresetName (name);
}

//==============================================================================
juce::Rectangle<float> APatheticSynthAudioProcessorEditor::makePanelBounds (juce::Rectangle<int> content,
                                                                            int padX, int padY)
{
    return content.expanded (padX, padY).toFloat();
}

void APatheticSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    paintWoodPanel (g, bounds);

    g.setColour (OrganLookAndFeel::woodDark().withAlpha (0.55f));
    g.fillRect (0.0f, 0.0f, 10.0f, bounds.getHeight());
    g.fillRect (bounds.getWidth() - 10.0f, 0.0f, 10.0f, bounds.getHeight());

    if (! nameplateBounds.isEmpty())
        paintNameplate (g, nameplateBounds);

    if (! tonePanelBounds.isEmpty())   paintControlPanel (g, tonePanelBounds);
    if (! ampPanelBounds.isEmpty())    paintControlPanel (g, ampPanelBounds);
    if (! filterPanelBounds.isEmpty()) paintControlPanel (g, filterPanelBounds);
    if (! unisonPanelBounds.isEmpty()) paintControlPanel (g, unisonPanelBounds);
    if (! lfoPanelBounds.isEmpty())    paintControlPanel (g, lfoPanelBounds);
    if (! chorusPanelBounds.isEmpty()) paintControlPanel (g, chorusPanelBounds);
    if (! delayPanelBounds.isEmpty())  paintControlPanel (g, delayPanelBounds);
    if (! reverbPanelBounds.isEmpty()) paintControlPanel (g, reverbPanelBounds);
    if (! meterPanelBounds.isEmpty())  paintControlPanel (g, meterPanelBounds);

    if (! keyboardStripBounds.isEmpty())
    {
        juce::ColourGradient strip (OrganLookAndFeel::woodMid(),
                                    keyboardStripBounds.getX(), keyboardStripBounds.getY(),
                                    OrganLookAndFeel::woodDark(),
                                    keyboardStripBounds.getX(), keyboardStripBounds.getBottom(), false);
        g.setGradientFill (strip);
        g.fillRoundedRectangle (keyboardStripBounds, 2.0f);
        g.setColour (OrganLookAndFeel::goldDim().withAlpha (0.4f));
        g.drawRoundedRectangle (keyboardStripBounds, 2.0f, 1.0f);
    }

    if (! keyboardWellBounds.isEmpty())
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (keyboardWellBounds, 3.0f);
    }
}

void APatheticSynthAudioProcessorEditor::resized()
{
    // Main controls keep the original 4:3 region; meter is an extra strip on the right
    constexpr int keyboardHeight = 72;
    constexpr int stripHeight = 3;
    constexpr int gap = 6;
    constexpr int meterWidth = 64;

    auto full = getLocalBounds().reduced (12, 10);

    auto meterStrip = full.removeFromRight (meterWidth);
    full.removeFromRight (6); // gap between controls and meter
    meterPanelBounds = makePanelBounds (meterStrip, 2, 2);
    levelMeter.setBounds (meterStrip.reduced (4, 4));

    auto area = full;

    // ---- Nameplate ----
    {
        auto nameBlock = area.removeFromTop (48);
        nameplateBounds = nameBlock.toFloat();

        auto nameInner = nameBlock.reduced (10, 4);
        auto top = nameInner.removeFromTop (28);
        titleLabel.setBounds (top.removeFromLeft (170));

        randomPresetButton.setBounds (top.removeFromRight (70).reduced (2, 2));
        initPresetButton.setBounds (top.removeFromRight (44).reduced (2, 2));
        deletePresetButton.setBounds (top.removeFromRight (44).reduced (2, 2));
        savePresetButton.setBounds (top.removeFromRight (48).reduced (2, 2));
        presetBox.setBounds (top.removeFromRight (150).reduced (2, 2));
        // Extra width so "MEMORY" is not ellipsised under Windows 125%/150% DPI scaling
        presetLabel.setBounds (top.removeFromRight (88));

        modelLabel.setBounds (nameInner.withTrimmedLeft (4));
    }

    area.removeFromTop (gap);

    // ---- Keyboard at bottom ----
    auto keyboardBlock = area.removeFromBottom (keyboardHeight + stripHeight);
    keyboardStripBounds = keyboardBlock.removeFromTop (stripHeight).toFloat();
    keyboardWellBounds = keyboardBlock.toFloat();
    keyboardComponent.setBounds (keyboardBlock);
    fitKeyboardToWidth (keyboardBlock.getWidth());

    area.removeFromBottom (gap);

    const int bandH = area.getHeight();
    const int toneH = (int) (bandH * 0.34f);
    const int midH  = (int) (bandH * 0.36f);

    // ===== Band 1: three oscillators side by side =====
    {
        auto toneBand = area.removeFromTop (toneH);
        tonePanelBounds = makePanelBounds (toneBand, 4, 3);
        auto inner = toneBand.reduced (8, 4);

        oscillatorsSectionLabel.setBounds (inner.removeFromTop (14));
        inner.removeFromTop (2);

        const int colW = inner.getWidth() / 3;
        for (int i = 0; i < 3; ++i)
            layoutOscillatorColumn (oscillators[i], inner.removeFromLeft (colW));
    }

    area.removeFromTop (gap);

    // ===== Band 2: Expression | Filter =====
    {
        auto midBand = area.removeFromTop (midH);
        auto left = midBand.removeFromLeft (midBand.getWidth() / 2).reduced (2, 0);
        auto right = midBand.reduced (2, 0);

        {
            ampPanelBounds = makePanelBounds (left, 4, 3);
            auto inner = left.reduced (8, 4);
            ampSectionLabel.setBounds (inner.removeFromTop (14));
            inner.removeFromTop (2);

            auto knobs = inner.removeFromTop (inner.getHeight() - 44);
            layoutKnobRow (knobs, {
                &attackSlider, &decaySlider, &sustainSlider,
                &releaseSlider, &gainSlider, &glideSlider
            });

            auto toggles = inner.reduced (4, 2);
            const int tw = toggles.getWidth() / 2;
            fixedVelocityButton.setBounds (toggles.removeFromLeft (tw).reduced (4, 4));
            glideLegatoButton.setBounds (toggles.reduced (4, 4));
        }

        {
            filterPanelBounds = makePanelBounds (right, 4, 3);
            auto inner = right.reduced (8, 4);
            filterSectionLabel.setBounds (inner.removeFromTop (14));
            inner.removeFromTop (2);

            auto topRow = inner.removeFromTop (inner.getHeight() / 2);
            const int topW = topRow.getWidth() / 4;
            cutoffSlider.setBounds (topRow.removeFromLeft (topW).reduced (2, 2).withTrimmedTop (12));
            resonanceSlider.setBounds (topRow.removeFromLeft (topW).reduced (2, 2).withTrimmedTop (12));
            driveSlider.setBounds (topRow.removeFromLeft (topW).reduced (2, 2).withTrimmedTop (12));
            auto typeCol = topRow.reduced (4, 2);
            filterTypeLabel.setBounds (typeCol.removeFromTop (12));
            filterTypeBox.setBounds (typeCol.removeFromTop (24));

            layoutKnobRow (inner, {
                &filtEnvAttackSlider, &filtEnvDecaySlider, &filtEnvSustainSlider,
                &filtEnvReleaseSlider, &filtEnvAmountSlider
            });
        }
    }

    area.removeFromTop (gap);

    // ===== Band 3: Unison | LFO | Chorus | Delay | Reverb =====
    {
        auto fxBand = area;
        const int colW = fxBand.getWidth() / 5;

        auto c0 = fxBand.removeFromLeft (colW);
        auto c1 = fxBand.removeFromLeft (colW);
        auto c2 = fxBand.removeFromLeft (colW);
        auto c3 = fxBand.removeFromLeft (colW);
        auto c4 = fxBand;

        {
            unisonPanelBounds = makePanelBounds (c0, 3, 3);
            auto inner = c0.reduced (4, 4);
            unisonSectionLabel.setBounds (inner.removeFromTop (14));
            inner.removeFromTop (2);
            layoutKnobRow (inner, { &unisonVoicesSlider, &unisonDetuneSlider, &unisonSpreadSlider });
        }

        {
            lfoPanelBounds = makePanelBounds (c1, 3, 3);
            auto inner = c1.reduced (4, 4);
            lfoSectionLabel.setBounds (inner.removeFromTop (14));
            inner.removeFromTop (2);

            auto knobArea = inner.removeFromTop (inner.getHeight() * 50 / 100);
            layoutKnobRow (knobArea, { &lfoRateSlider, &lfoDepthSlider });

            auto comboRow = inner.removeFromTop (40).reduced (1, 2);
            const int cw = comboRow.getWidth() / 2;
            auto shapeCol = comboRow.removeFromLeft (cw).reduced (1, 0);
            lfoWaveformLabel.setBounds (shapeCol.removeFromTop (12));
            lfoWaveformBox.setBounds (shapeCol.removeFromTop (22));
            auto destCol = comboRow.reduced (1, 0);
            lfoTargetLabel.setBounds (destCol.removeFromTop (12));
            lfoTargetBox.setBounds (destCol.removeFromTop (22));

            lfoRetriggerButton.setBounds (inner.reduced (4, 2));
        }

        {
            chorusPanelBounds = makePanelBounds (c2, 3, 3);
            auto inner = c2.reduced (4, 4);
            chorusSectionLabel.setBounds (inner.removeFromTop (14));
            inner.removeFromTop (2);
            layoutKnobRow (inner, { &chorusRateSlider, &chorusDepthSlider, &chorusMixSlider });
        }

        {
            delayPanelBounds = makePanelBounds (c3, 3, 3);
            auto inner = c3.reduced (4, 4);
            delaySectionLabel.setBounds (inner.removeFromTop (14));
            inner.removeFromTop (2);

            auto knobs = inner.removeFromTop (inner.getHeight() * 55 / 100);
            layoutKnobRow (knobs, { &delayTimeSlider, &delayFeedbackSlider, &delayMixSlider });

            auto syncRow = inner.removeFromTop (24).reduced (2, 2);
            delaySyncButton.setBounds (syncRow);

            auto divCol = inner.reduced (2, 2);
            delayDivisionLabel.setBounds (divCol.removeFromTop (12));
            delayDivisionBox.setBounds (divCol.removeFromTop (22));
        }

        {
            reverbPanelBounds = makePanelBounds (c4, 3, 3);
            auto inner = c4.reduced (4, 4);
            reverbSectionLabel.setBounds (inner.removeFromTop (14));
            inner.removeFromTop (2);
            layoutKnobRow (inner, {
                &reverbSizeSlider, &reverbDampingSlider, &reverbWidthSlider, &reverbMixSlider
            });
        }
    }
}
void APatheticSynthAudioProcessorEditor::timerCallback()
{
    keyboardComponent.repaint();
    levelMeter.repaint();
}
