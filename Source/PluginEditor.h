#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OrganLookAndFeel.h"

//==============================================================================
/** Vertical peak meter: display range -inf (-60 floor) to +6 dB. */
class OutputLevelMeter final : public juce::Component
{
public:
    explicit OutputLevelMeter (APatheticSynthAudioProcessor& processorIn)
        : processor (processorIn) {}

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);

        g.setColour (OrganLookAndFeel::panelInset());
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (OrganLookAndFeel::goldDim().withAlpha (0.6f));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        auto meter = bounds.reduced (6.0f, 18.0f);
        const float topDb = 6.0f;
        const float bottomDb = -60.0f; // visual floor for -inf

        auto dbToY = [&] (float db) -> float
        {
            const float n = juce::jlimit (0.0f, 1.0f, (db - bottomDb) / (topDb - bottomDb));
            return meter.getBottom() - n * meter.getHeight();
        };

        // Track
        g.setColour (juce::Colour (0xff0a0806));
        g.fillRoundedRectangle (meter, 2.0f);

        const float levelDb = processor.getOutputPeakDb();
        const float clippedDb = juce::jmin (levelDb, topDb);
        const float y = (levelDb <= bottomDb + 0.01f) ? meter.getBottom() : dbToY (clippedDb);
        auto fill = meter.withTop (y);

        if (fill.getHeight() > 0.5f)
        {
            const bool hot = levelDb >= 0.0f;
            juce::ColourGradient grad (hot ? juce::Colour (0xffff3030) : OrganLookAndFeel::amberGlow(),
                                       fill.getX(), fill.getBottom(),
                                       hot ? juce::Colour (0xffffa000) : OrganLookAndFeel::amber(),
                                       fill.getX(), fill.getY(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (fill, 2.0f);
        }

        // Scale marks
        g.setFont (juce::FontOptions (9.0f));
        const float marks[] = { 6.0f, 0.0f, -6.0f, -12.0f, -24.0f, -48.0f };

        for (float db : marks)
        {
            const float my = dbToY (db);
            g.setColour (OrganLookAndFeel::goldDim().withAlpha (0.7f));
            g.drawHorizontalLine ((int) my, meter.getX(), meter.getRight());

            juce::String text = (db > 0.0f ? "+" : "") + juce::String ((int) db);
            g.setColour (OrganLookAndFeel::silkScreen());
            g.drawText (text, juce::Rectangle<float> (bounds.getX(), my - 6.0f,
                                                      bounds.getWidth(), 12.0f),
                        juce::Justification::centred, false);
        }

        // -inf label at bottom
        g.setColour (OrganLookAndFeel::silkScreen().withAlpha (0.85f));
        g.setFont (juce::FontOptions (8.0f));
        g.drawText ("-inf", bounds.removeFromBottom (14.0f), juce::Justification::centred, false);

        // Title
        g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        g.setColour (OrganLookAndFeel::gold());
        g.drawText ("OUT", bounds.removeFromTop (14.0f), juce::Justification::centred, false);
    }

private:
    APatheticSynthAudioProcessor& processor;
};

//==============================================================================
class APatheticSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit APatheticSynthAudioProcessorEditor (APatheticSynthAudioProcessor&);
    ~APatheticSynthAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    struct OscillatorControls
    {
        juce::Label titleLabel;
        juce::ComboBox waveBox;
        juce::Label waveLabel;
        juce::Slider levelSlider, semiSlider, fineSlider;
        juce::Label levelLabel, semiLabel, fineLabel;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> semiAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fineAttachment;
    };

    void timerCallback() override;
    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& name,
                      bool drawbarStyle = false);
    void setupSectionLabel (juce::Label& label, const juce::String& text);
    void setupTextButton (juce::TextButton& button);
    void setupOscillator (OscillatorControls& osc, int index, juce::AudioProcessorValueTreeState& apvts);
    void layoutOscillatorColumn (OscillatorControls& osc, juce::Rectangle<int> column);
    void layoutKnobRow (juce::Rectangle<int> row, std::initializer_list<juce::Component*> knobs);
    void styleKeyboard();
    void fitKeyboardToWidth (int pixelWidth);
    void paintWoodPanel (juce::Graphics& g, juce::Rectangle<float> area) const;
    void paintControlPanel (juce::Graphics& g, juce::Rectangle<float> area) const;
    void paintNameplate (juce::Graphics& g, juce::Rectangle<float> area) const;

    /** Black panel tightly wrapping a rectangle (already includes content). */
    static juce::Rectangle<float> makePanelBounds (juce::Rectangle<int> content,
                                                   int padX = 6, int padY = 4);

    void refreshPresetList();
    void savePresetClicked();
    void deletePresetClicked();
    void initPresetClicked();
    void randomPresetClicked();
    void presetSelected();
    void clearPresetSelection();

    APatheticSynthAudioProcessor& audioProcessor;
    OrganLookAndFeel organLookAndFeel;

    // Presets
    juce::Label presetLabel;
    juce::ComboBox presetBox;
    juce::TextButton savePresetButton { "SAVE" };
    juce::TextButton deletePresetButton { "DEL" };
    juce::TextButton initPresetButton { "INIT" };
    juce::TextButton randomPresetButton { "RANDOM" };
    bool ignorePresetBoxCallback = false;

    // Oscillators
    juce::Label oscillatorsSectionLabel;
    OscillatorControls oscillators[3];

    // Envelope / amp
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider, gainSlider;
    juce::Label  attackLabel,  decayLabel,  sustainLabel,  releaseLabel,  gainLabel;
    juce::Label  ampSectionLabel;

    juce::ToggleButton fixedVelocityButton { "FULL TOUCH" };
    juce::Slider glideSlider;
    juce::Label  glideLabel;
    juce::ToggleButton glideLegatoButton { "LEGATO" };

    // Filter + drive + filter envelope
    juce::Slider cutoffSlider, resonanceSlider, driveSlider;
    juce::Label  cutoffLabel,  resonanceLabel,  driveLabel;
    juce::ComboBox filterTypeBox;
    juce::Label    filterTypeLabel;
    juce::Label    filterSectionLabel;
    juce::Slider filtEnvAttackSlider, filtEnvDecaySlider, filtEnvSustainSlider,
                 filtEnvReleaseSlider, filtEnvAmountSlider;
    juce::Label  filtEnvAttackLabel,  filtEnvDecayLabel,  filtEnvSustainLabel,
                 filtEnvReleaseLabel,  filtEnvAmountLabel;

    // Unison
    juce::Slider unisonVoicesSlider, unisonDetuneSlider, unisonSpreadSlider;
    juce::Label  unisonVoicesLabel,  unisonDetuneLabel,  unisonSpreadLabel;
    juce::Label  unisonSectionLabel;

    // LFO
    juce::Slider lfoRateSlider, lfoDepthSlider;
    juce::Label  lfoRateLabel,  lfoDepthLabel;
    juce::ComboBox lfoWaveformBox, lfoTargetBox;
    juce::Label    lfoWaveformLabel, lfoTargetLabel;
    juce::ToggleButton lfoRetriggerButton { "RETRIG" };
    juce::Label  lfoSectionLabel;

    // Chorus + delay + reverb
    juce::Label  chorusSectionLabel;
    juce::Slider chorusRateSlider, chorusDepthSlider, chorusMixSlider;
    juce::Label  chorusRateLabel,  chorusDepthLabel,  chorusMixLabel;
    juce::Label  delaySectionLabel;
    juce::Slider delayTimeSlider, delayFeedbackSlider, delayMixSlider;
    juce::Label  delayTimeLabel,  delayFeedbackLabel,  delayMixLabel;
    juce::ToggleButton delaySyncButton { "SYNC" };
    juce::ComboBox delayDivisionBox;
    juce::Label    delayDivisionLabel;
    juce::Label  reverbSectionLabel;
    juce::Slider reverbSizeSlider, reverbDampingSlider, reverbWidthSlider, reverbMixSlider;
    juce::Label  reverbSizeLabel,  reverbDampingLabel,  reverbWidthLabel,  reverbMixLabel;

    juce::MidiKeyboardComponent keyboardComponent;
    OutputLevelMeter levelMeter;

    juce::Label titleLabel;
    juce::Label modelLabel;

    // Painted panel regions - updated in resized() so they hug the controls
    juce::Rectangle<float> nameplateBounds;
    juce::Rectangle<float> meterPanelBounds;
    juce::Rectangle<float> tonePanelBounds;
    juce::Rectangle<float> ampPanelBounds;
    juce::Rectangle<float> filterPanelBounds;
    juce::Rectangle<float> unisonPanelBounds;
    juce::Rectangle<float> lfoPanelBounds;
    juce::Rectangle<float> chorusPanelBounds;
    juce::Rectangle<float> delayPanelBounds;
    juce::Rectangle<float> reverbPanelBounds;
    juce::Rectangle<float> keyboardStripBounds;
    juce::Rectangle<float> keyboardWellBounds;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> decayAttachment;
    std::unique_ptr<SliderAttachment> sustainAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<ButtonAttachment> fixedVelocityAttachment;
    std::unique_ptr<SliderAttachment> glideAttachment;
    std::unique_ptr<ButtonAttachment> glideLegatoAttachment;
    std::unique_ptr<SliderAttachment> cutoffAttachment;
    std::unique_ptr<SliderAttachment> resonanceAttachment;
    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<ComboAttachment>  filterTypeAttachment;
    std::unique_ptr<SliderAttachment> filtEnvAttackAttachment;
    std::unique_ptr<SliderAttachment> filtEnvDecayAttachment;
    std::unique_ptr<SliderAttachment> filtEnvSustainAttachment;
    std::unique_ptr<SliderAttachment> filtEnvReleaseAttachment;
    std::unique_ptr<SliderAttachment> filtEnvAmountAttachment;
    std::unique_ptr<SliderAttachment> unisonVoicesAttachment;
    std::unique_ptr<SliderAttachment> unisonDetuneAttachment;
    std::unique_ptr<SliderAttachment> unisonSpreadAttachment;
    std::unique_ptr<SliderAttachment> lfoRateAttachment;
    std::unique_ptr<SliderAttachment> lfoDepthAttachment;
    std::unique_ptr<ComboAttachment>  lfoWaveformAttachment;
    std::unique_ptr<ComboAttachment>  lfoTargetAttachment;
    std::unique_ptr<ButtonAttachment> lfoRetriggerAttachment;
    std::unique_ptr<SliderAttachment> chorusRateAttachment;
    std::unique_ptr<SliderAttachment> chorusDepthAttachment;
    std::unique_ptr<SliderAttachment> chorusMixAttachment;
    std::unique_ptr<SliderAttachment> delayTimeAttachment;
    std::unique_ptr<SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr<SliderAttachment> delayMixAttachment;
    std::unique_ptr<ButtonAttachment> delaySyncAttachment;
    std::unique_ptr<ComboAttachment>  delayDivisionAttachment;
    std::unique_ptr<SliderAttachment> reverbSizeAttachment;
    std::unique_ptr<SliderAttachment> reverbDampingAttachment;
    std::unique_ptr<SliderAttachment> reverbWidthAttachment;
    std::unique_ptr<SliderAttachment> reverbMixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (APatheticSynthAudioProcessorEditor)
};
