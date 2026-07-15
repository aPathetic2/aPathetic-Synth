#pragma once

#include <JuceHeader.h>
#include "SynthSound.h"
#include "SynthVoice.h"
#include "PresetManager.h"

//==============================================================================
class APatheticSynthAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    APatheticSynthAudioProcessor();
    ~APatheticSynthAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }
    PresetManager& getPresetManager() { return presetManager; }

    /** Peak level in dB for the GUI meter (approx -100 = silence / -inf). */
    float getOutputPeakDb() const noexcept;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    //==============================================================================
    void updateVoices();
    void prepareEffects (double sampleRate);
    void processChorus (juce::AudioBuffer<float>& buffer);
    void processDelay (juce::AudioBuffer<float>& buffer);
    void processReverb (juce::AudioBuffer<float>& buffer);
    void updateOutputMeter (const juce::AudioBuffer<float>& buffer);

    float getHostBpm() const;
    float getDelayTimeSeconds() const;
    static float syncDivisionToBeats (int divisionIndex);

    juce::Synthesiser synth;
    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presetManager;

    // Delay (stereo circular buffer)
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;

    // Chorus (stereo modulated delay)
    juce::AudioBuffer<float> chorusBuffer;
    int chorusWritePos = 0;
    double chorusLfoPhase = 0.0;

    // Reverb
    juce::Reverb reverb;

    // Output meter ballistics (linear peak, GUI converts to dB)
    std::atomic<float> outputPeakLinear { 0.0f };

    double currentSampleRate = 44100.0;
    static constexpr float maxDelaySeconds = 2.0f;
    static constexpr float maxChorusDelaySeconds = 0.05f; // 50 ms max

    static constexpr int numVoices = 8;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (APatheticSynthAudioProcessor)
};
