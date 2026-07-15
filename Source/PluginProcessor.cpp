#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
APatheticSynthAudioProcessor::APatheticSynthAudioProcessor()
    : AudioProcessor (BusesProperties()
                         .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout()),
      presetManager (apvts)
{
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new SynthVoice());

    synth.addSound (new SynthSound());
}

APatheticSynthAudioProcessor::~APatheticSynthAudioProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout APatheticSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 }, "Attack",
        juce::NormalisableRange<float> { 0.001f, 2.0f, 0.001f, 0.4f }, 0.05f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "decay", 1 }, "Decay",
        juce::NormalisableRange<float> { 0.001f, 2.0f, 0.001f, 0.4f }, 0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sustain", 1 }, "Sustain",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.8f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 }, "Release",
        juce::NormalisableRange<float> { 0.001f, 5.0f, 0.001f, 0.4f }, 0.3f));

    // Range goes above 1 so quiet patches can still peak at 0 dBFS after normalisation
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 }, "Gain",
        juce::NormalisableRange<float> { 0.0f, 2.0f, 0.01f }, 0.7f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "fixedVelocity", 1 }, "Fixed Velocity", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "glideTime", 1 }, "Glide Time",
        juce::NormalisableRange<float> { 0.0f, 2.0f, 0.001f, 0.4f }, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "glideLegato", 1 }, "Glide Legato", false));

    // ---- Three independent oscillators ----
    const juce::StringArray waveNames { "Sine", "Saw", "Square", "Triangle" };
    const float defaultLevels[3] = { 0.8f, 0.0f, 0.0f };
    const int defaultWaves[3] = { 1, 1, 2 }; // Saw, Saw, Square
    const float defaultSemis[3] = { 0.0f, 0.0f, -12.0f };

    for (int i = 1; i <= 3; ++i)
    {
        const auto n = juce::String (i);
        const auto idx = i - 1;

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "osc" + n + "Wave", 1 }, "Osc " + n + " Wave",
            waveNames, defaultWaves[idx]));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osc" + n + "Level", 1 }, "Osc " + n + " Level",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, defaultLevels[idx]));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osc" + n + "Semi", 1 }, "Osc " + n + " Semi",
            juce::NormalisableRange<float> { -24.0f, 24.0f, 1.0f }, defaultSemis[idx]));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osc" + n + "Fine", 1 }, "Osc " + n + " Fine",
            juce::NormalisableRange<float> { -50.0f, 50.0f, 0.1f }, 0.0f));
    }

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "filterType", 1 }, "Filter Type",
        juce::StringArray { "Off", "Low Pass", "High Pass", "Band Pass" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterCutoff", 1 }, "Cutoff",
        juce::NormalisableRange<float> { 20.0f, 20000.0f, 0.1f, 0.3f }, 8000.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterResonance", 1 }, "Resonance",
        juce::NormalisableRange<float> { 0.1f, 10.0f, 0.01f, 0.5f }, 0.707f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 }, "Drive",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.0f));

    // Filter envelope (modulates cutoff; amount is bipolar)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterEnvAttack", 1 }, "Filter Env Attack",
        juce::NormalisableRange<float> { 0.001f, 2.0f, 0.001f, 0.4f }, 0.01f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterEnvDecay", 1 }, "Filter Env Decay",
        juce::NormalisableRange<float> { 0.001f, 2.0f, 0.001f, 0.4f }, 0.25f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterEnvSustain", 1 }, "Filter Env Sustain",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterEnvRelease", 1 }, "Filter Env Release",
        juce::NormalisableRange<float> { 0.001f, 5.0f, 0.001f, 0.4f }, 0.2f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterEnvAmount", 1 }, "Filter Env Amount",
        juce::NormalisableRange<float> { -1.0f, 1.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float value, int /*maxLen*/)
            {
                return juce::String (juce::roundToInt (value * 100.0f)) + " %";
            })
            .withValueFromStringFunction ([] (const juce::String& text)
            {
                return text.retainCharacters ("0123456789.-").getFloatValue() / 100.0f;
            })));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "unisonVoices", 1 }, "Unison Voices", 1, 8, 1));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "unisonDetune", 1 }, "Unison Detune",
        juce::NormalisableRange<float> { 0.0f, 50.0f, 0.1f }, 10.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "unisonSpread", 1 }, "Unison Spread",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.5f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float value, int /*maxLen*/)
            {
                return juce::String (juce::roundToInt (value * 100.0f)) + " %";
            })
            .withValueFromStringFunction ([] (const juce::String& text)
            {
                return text.retainCharacters ("0123456789.-").getFloatValue() / 100.0f;
            })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lfoRate", 1 }, "LFO Rate",
        juce::NormalisableRange<float> { 0.05f, 20.0f, 0.01f, 0.4f }, 2.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lfoDepth", 1 }, "LFO Depth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "lfoWaveform", 1 }, "LFO Waveform",
        juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "lfoTarget", 1 }, "LFO Target",
        juce::StringArray { "Off", "Filter", "Pitch", "Amplitude" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "lfoRetrigger", 1 }, "LFO Retrigger", true));

    // ---- Delay (free time + optional host tempo sync) ----
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayTime", 1 }, "Delay Time",
        juce::NormalisableRange<float> { 0.01f, 2.0f, 0.001f, 0.4f }, 0.35f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayFeedback", 1 }, "Delay Feedback",
        juce::NormalisableRange<float> { 0.0f, 0.95f, 0.01f }, 0.35f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayMix", 1 }, "Delay Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "delaySync", 1 }, "Delay Sync", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "delayDivision", 1 }, "Delay Division",
        juce::StringArray { "1/1", "1/2", "1/4", "1/4.", "1/8", "1/8.", "1/8T", "1/16", "1/16.", "1/32" },
        4)); // default 1/8

    // ---- Chorus ----
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorusRate", 1 }, "Chorus Rate",
        juce::NormalisableRange<float> { 0.05f, 5.0f, 0.01f, 0.4f }, 0.8f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorusDepth", 1 }, "Chorus Depth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.4f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorusMix", 1 }, "Chorus Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.0f));

    // ---- Reverb ----
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbSize", 1 }, "Reverb Size",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbDamping", 1 }, "Reverb Damping",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbWidth", 1 }, "Reverb Width",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbMix", 1 }, "Reverb Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f }, 0.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String APatheticSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool APatheticSynthAudioProcessor::acceptsMidi() const
{
    return true;
}

bool APatheticSynthAudioProcessor::producesMidi() const
{
    return false;
}

bool APatheticSynthAudioProcessor::isMidiEffect() const
{
    return false;
}

double APatheticSynthAudioProcessor::getTailLengthSeconds() const
{
    // Amp/filter release + delay repeats + reverb tail
    return 12.0;
}

int APatheticSynthAudioProcessor::getNumPrograms()
{
    return 1;
}

int APatheticSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void APatheticSynthAudioProcessor::setCurrentProgram (int /*index*/) {}

const juce::String APatheticSynthAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void APatheticSynthAudioProcessor::changeProgramName (int /*index*/, const juce::String& /*newName*/) {}

//==============================================================================
void APatheticSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    synth.setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            voice->prepareToPlay (sampleRate, samplesPerBlock);
    }

    prepareEffects (sampleRate);
    updateVoices();
    juce::ignoreUnused (samplesPerBlock);
}

void APatheticSynthAudioProcessor::releaseResources()
{
    delayBuffer.setSize (0, 0);
    chorusBuffer.setSize (0, 0);
}

void APatheticSynthAudioProcessor::prepareEffects (double sampleRate)
{
    const int maxDelaySamples = juce::jmax (1, (int) std::ceil (sampleRate * (double) maxDelaySeconds) + 2);
    delayBuffer.setSize (2, maxDelaySamples);
    delayBuffer.clear();
    delayWritePos = 0;

    const int maxChorusSamples = juce::jmax (1, (int) std::ceil (sampleRate * (double) maxChorusDelaySeconds) + 8);
    chorusBuffer.setSize (2, maxChorusSamples);
    chorusBuffer.clear();
    chorusWritePos = 0;
    chorusLfoPhase = 0.0;

    reverb.setSampleRate (sampleRate);
    reverb.reset();
}

float APatheticSynthAudioProcessor::getHostBpm() const
{
    if (auto* head = getPlayHead())
    {
        if (auto position = head->getPosition())
        {
            if (auto bpm = position->getBpm())
                if (*bpm > 1.0 && *bpm < 400.0)
                    return (float) *bpm;
        }
    }

    return 120.0f; // standalone / no playhead
}

float APatheticSynthAudioProcessor::syncDivisionToBeats (int divisionIndex)
{
    switch (juce::jlimit (0, 9, divisionIndex))
    {
        case 0:  return 4.0f;          // 1/1
        case 1:  return 2.0f;          // 1/2
        case 2:  return 1.0f;          // 1/4
        case 3:  return 1.5f;          // 1/4 dotted
        case 4:  return 0.5f;          // 1/8
        case 5:  return 0.75f;         // 1/8 dotted
        case 6:  return 1.0f / 3.0f;   // 1/8 triplet
        case 7:  return 0.25f;         // 1/16
        case 8:  return 0.375f;        // 1/16 dotted
        case 9:  return 0.125f;        // 1/32
        default: return 0.5f;
    }
}

float APatheticSynthAudioProcessor::getDelayTimeSeconds() const
{
    const bool sync = apvts.getRawParameterValue ("delaySync")->load() > 0.5f;

    if (! sync)
        return apvts.getRawParameterValue ("delayTime")->load();

    const int division = juce::roundToInt (apvts.getRawParameterValue ("delayDivision")->load());
    const float beats = syncDivisionToBeats (division);
    const float bpm = getHostBpm();
    const float seconds = beats * (60.0f / bpm);

    return juce::jlimit (0.01f, maxDelaySeconds, seconds);
}

bool APatheticSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void APatheticSynthAudioProcessor::updateVoices()
{
    const auto attack  = apvts.getRawParameterValue ("attack")->load();
    const auto decay   = apvts.getRawParameterValue ("decay")->load();
    const auto sustain = apvts.getRawParameterValue ("sustain")->load();
    const auto release = apvts.getRawParameterValue ("release")->load();
    const auto gain    = apvts.getRawParameterValue ("gain")->load();
    const auto fixedVelocity = apvts.getRawParameterValue ("fixedVelocity")->load() > 0.5f;
    const auto glideTime = apvts.getRawParameterValue ("glideTime")->load();
    const auto glideLegato = apvts.getRawParameterValue ("glideLegato")->load() > 0.5f;
    const auto filterTypeIdx = static_cast<int> (apvts.getRawParameterValue ("filterType")->load());
    const auto filterCutoff = apvts.getRawParameterValue ("filterCutoff")->load();
    const auto filterResonance = apvts.getRawParameterValue ("filterResonance")->load();
    const auto drive = apvts.getRawParameterValue ("drive")->load();
    const auto filterEnvAttack  = apvts.getRawParameterValue ("filterEnvAttack")->load();
    const auto filterEnvDecay   = apvts.getRawParameterValue ("filterEnvDecay")->load();
    const auto filterEnvSustain = apvts.getRawParameterValue ("filterEnvSustain")->load();
    const auto filterEnvRelease = apvts.getRawParameterValue ("filterEnvRelease")->load();
    const auto filterEnvAmount  = apvts.getRawParameterValue ("filterEnvAmount")->load();
    const auto unisonVoices = juce::roundToInt (apvts.getRawParameterValue ("unisonVoices")->load());
    const auto unisonDetune = apvts.getRawParameterValue ("unisonDetune")->load();
    const auto unisonSpread = apvts.getRawParameterValue ("unisonSpread")->load();
    const auto lfoRate = apvts.getRawParameterValue ("lfoRate")->load();
    const auto lfoDepth = apvts.getRawParameterValue ("lfoDepth")->load();
    const auto lfoWaveIdx = static_cast<int> (apvts.getRawParameterValue ("lfoWaveform")->load());
    const auto lfoTargetIdx = static_cast<int> (apvts.getRawParameterValue ("lfoTarget")->load());
    const auto lfoRetrigger = apvts.getRawParameterValue ("lfoRetrigger")->load() > 0.5f;

    SynthVoice::OscParams oscs[SynthVoice::numOscillators];

    for (int o = 0; o < SynthVoice::numOscillators; ++o)
    {
        const auto n = juce::String (o + 1);
        const auto waveIdx = static_cast<int> (apvts.getRawParameterValue ("osc" + n + "Wave")->load());

        oscs[o].waveform  = static_cast<SynthVoice::Waveform> (juce::jlimit (0, 3, waveIdx));
        oscs[o].level     = apvts.getRawParameterValue ("osc" + n + "Level")->load();
        oscs[o].semitone  = apvts.getRawParameterValue ("osc" + n + "Semi")->load();
        oscs[o].fineCents = apvts.getRawParameterValue ("osc" + n + "Fine")->load();
    }

    const auto filterType = static_cast<SynthVoice::FilterType> (
        juce::jlimit (0, 3, filterTypeIdx));
    const auto lfoWaveform = static_cast<SynthVoice::Waveform> (
        juce::jlimit (0, 3, lfoWaveIdx));
    const auto lfoTarget = static_cast<SynthVoice::LfoTarget> (
        juce::jlimit (0, 3, lfoTargetIdx));

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            voice->updateParameters (attack, decay, sustain, release, gain, fixedVelocity,
                                     glideTime, glideLegato,
                                     oscs,
                                     filterType, filterCutoff, filterResonance, drive,
                                     filterEnvAttack, filterEnvDecay, filterEnvSustain,
                                     filterEnvRelease, filterEnvAmount,
                                     unisonVoices, unisonDetune, unisonSpread,
                                     lfoRate, lfoDepth, lfoWaveform, lfoTarget, lfoRetrigger);
    }
}

void APatheticSynthAudioProcessor::processChorus (juce::AudioBuffer<float>& buffer)
{
    const float rate  = apvts.getRawParameterValue ("chorusRate")->load();
    const float depth = apvts.getRawParameterValue ("chorusDepth")->load();
    const float mix   = apvts.getRawParameterValue ("chorusMix")->load();

    if (mix <= 0.0001f || chorusBuffer.getNumSamples() == 0 || currentSampleRate <= 0.0)
        return;

    const int bufferSize = chorusBuffer.getNumSamples();
    const int numSamples = buffer.getNumSamples();
    const bool stereo = buffer.getNumChannels() >= 2;

    // Centre delay ~8 ms, modulation up to ~5 ms
    const float baseDelayMs = 8.0f;
    const float modMs = 0.5f + depth * 5.0f;
    const double twoPi = juce::MathConstants<double>::twoPi;
    const double phaseInc = twoPi * (double) rate / currentSampleRate;

    auto* cL = chorusBuffer.getWritePointer (0);
    auto* cR = chorusBuffer.getNumChannels() > 1 ? chorusBuffer.getWritePointer (1) : cL;
    auto* outL = buffer.getWritePointer (0);
    auto* outR = stereo ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const float dryL = outL[i];
        const float dryR = stereo ? outR[i] : dryL;

        cL[chorusWritePos] = dryL;
        cR[chorusWritePos] = dryR;

        const float lfoL = (float) std::sin (chorusLfoPhase);
        const float lfoR = (float) std::sin (chorusLfoPhase + juce::MathConstants<double>::halfPi);

        const float delayMsL = baseDelayMs + lfoL * modMs;
        const float delayMsR = baseDelayMs + lfoR * modMs;

        auto readAt = [bufferSize] (const float* buf, int writePos, float delaySamples) -> float
        {
            float readPos = (float) writePos - delaySamples;
            while (readPos < 0.0f)
                readPos += (float) bufferSize;

            const int i0 = (int) readPos;
            const int i1 = (i0 + 1) % bufferSize;
            const float frac = readPos - (float) i0;
            return buf[i0] * (1.0f - frac) + buf[i1] * frac;
        };

        const float dSamplesL = juce::jlimit (1.0f, (float) bufferSize - 2,
                                              delayMsL * 0.001f * (float) currentSampleRate);
        const float dSamplesR = juce::jlimit (1.0f, (float) bufferSize - 2,
                                              delayMsR * 0.001f * (float) currentSampleRate);

        const float wetL = readAt (cL, chorusWritePos, dSamplesL);
        const float wetR = readAt (cR, chorusWritePos, dSamplesR);

        outL[i] = dryL * (1.0f - mix) + wetL * mix;
        if (stereo)
            outR[i] = dryR * (1.0f - mix) + wetR * mix;

        if (++chorusWritePos >= bufferSize)
            chorusWritePos = 0;

        chorusLfoPhase += phaseInc;
        if (chorusLfoPhase >= twoPi)
            chorusLfoPhase -= twoPi;
    }
}

void APatheticSynthAudioProcessor::processDelay (juce::AudioBuffer<float>& buffer)
{
    const float delayTime = getDelayTimeSeconds();
    const float feedback  = juce::jlimit (0.0f, 0.95f, apvts.getRawParameterValue ("delayFeedback")->load());
    const float mix       = apvts.getRawParameterValue ("delayMix")->load();

    if (mix <= 0.0001f || delayBuffer.getNumSamples() == 0 || currentSampleRate <= 0.0)
        return;

    const int bufferSize = delayBuffer.getNumSamples();
    const int delaySamples = juce::jlimit (1, bufferSize - 1,
                                           (int) std::round (delayTime * (float) currentSampleRate));

    const int numSamples = buffer.getNumSamples();
    const bool stereo = buffer.getNumChannels() >= 2;

    auto* delayL = delayBuffer.getWritePointer (0);
    auto* delayR = delayBuffer.getNumChannels() > 1 ? delayBuffer.getWritePointer (1) : delayL;
    auto* outL = buffer.getWritePointer (0);
    auto* outR = stereo ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        int readPos = delayWritePos - delaySamples;
        if (readPos < 0)
            readPos += bufferSize;

        const float delayedL = delayL[readPos];
        const float delayedR = delayR[readPos];

        const float dryL = outL[i];
        const float dryR = stereo ? outR[i] : dryL;

        outL[i] = dryL * (1.0f - mix) + delayedL * mix;
        if (stereo)
            outR[i] = dryR * (1.0f - mix) + delayedR * mix;

        delayL[delayWritePos] = dryL + delayedL * feedback;
        delayR[delayWritePos] = dryR + delayedR * feedback;

        if (++delayWritePos >= bufferSize)
            delayWritePos = 0;
    }
}

void APatheticSynthAudioProcessor::processReverb (juce::AudioBuffer<float>& buffer)
{
    const float size    = apvts.getRawParameterValue ("reverbSize")->load();
    const float damping = apvts.getRawParameterValue ("reverbDamping")->load();
    const float width   = apvts.getRawParameterValue ("reverbWidth")->load();
    const float mix     = apvts.getRawParameterValue ("reverbMix")->load();

    if (mix <= 0.0001f)
        return;

    juce::Reverb::Parameters params;
    params.roomSize   = size;
    params.damping    = damping;
    params.width      = width;
    params.wetLevel   = mix;
    params.dryLevel   = 1.0f - mix * 0.85f; // keep some dry even at high mix
    params.freezeMode = 0.0f;
    reverb.setParameters (params);

    if (buffer.getNumChannels() >= 2)
        reverb.processStereo (buffer.getWritePointer (0),
                              buffer.getWritePointer (1),
                              buffer.getNumSamples());
    else if (buffer.getNumChannels() == 1)
        reverb.processMono (buffer.getWritePointer (0), buffer.getNumSamples());
}

void APatheticSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output (synths generate audio from MIDI, not from input)
    buffer.clear();

    updateVoices();

    // Merge hardware MIDI + on-screen keyboard notes into the buffer
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    // Render the synthesiser
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // Master FX: chorus -> delay -> reverb
    processChorus (buffer);
    processDelay (buffer);
    processReverb (buffer);

    updateOutputMeter (buffer);
}

void APatheticSynthAudioProcessor::updateOutputMeter (const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* data = buffer.getReadPointer (ch);

        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax (peak, std::abs (data[i]));
    }

    // Fast attack, slower release so the bar is readable
    const float previous = outputPeakLinear.load (std::memory_order_relaxed);
    const float next = (peak > previous) ? peak
                                         : previous * 0.92f + peak * 0.08f;
    outputPeakLinear.store (next, std::memory_order_relaxed);
}

float APatheticSynthAudioProcessor::getOutputPeakDb() const noexcept
{
    const float linear = outputPeakLinear.load (std::memory_order_relaxed);

    if (linear <= 1.0e-6f)
        return -100.0f; // treat as -inf for the meter floor

    return juce::Decibels::gainToDecibels (linear, -100.0f);
}

//==============================================================================
bool APatheticSynthAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* APatheticSynthAudioProcessor::createEditor()
{
    return new APatheticSynthAudioProcessorEditor (*this);
}

//==============================================================================
void APatheticSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void APatheticSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new APatheticSynthAudioProcessor();
}
