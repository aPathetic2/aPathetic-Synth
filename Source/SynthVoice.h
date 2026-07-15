#pragma once

#include <JuceHeader.h>
#include "SynthSound.h"

//==============================================================================
class SynthVoice : public juce::SynthesiserVoice
{
public:
    enum class Waveform
    {
        Sine = 0,
        Saw,
        Square,
        Triangle
    };

    enum class FilterType
    {
        Off = 0,
        LowPass,
        HighPass,
        BandPass
    };

    enum class LfoTarget
    {
        Off = 0,
        FilterCutoff,
        Pitch,
        Amplitude
    };

    static constexpr int numOscillators = 3;
    static constexpr int maxUnison = 8;

    struct OscParams
    {
        Waveform waveform = Waveform::Saw;
        float level = 0.0f;
        float semitone = 0.0f;
        float fineCents = 0.0f;
    };

    SynthVoice()
    {
        adsrParams.attack  = 0.05f;
        adsrParams.decay   = 0.1f;
        adsrParams.sustain = 0.8f;
        adsrParams.release = 0.3f;
        adsr.setParameters (adsrParams);

        filterEnvParams.attack  = 0.01f;
        filterEnvParams.decay   = 0.25f;
        filterEnvParams.sustain = 0.0f;
        filterEnvParams.release = 0.2f;
        filterEnv.setParameters (filterEnvParams);

        // Default: only oscillator 1 is audible
        oscParams[0] = { Waveform::Saw, 0.8f, 0.0f, 0.0f };
        oscParams[1] = { Waveform::Saw, 0.0f, 0.0f, 0.0f };
        oscParams[2] = { Waveform::Square, 0.0f, -12.0f, 0.0f };
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SynthSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        const auto noteVelocity = fixedVelocity ? 1.0f : velocity;
        level = noteVelocity * 0.25;

        targetFrequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        baseFrequency = targetFrequency; // oscillator ratios are built around the target note

        const int heldBefore = notesHeld.load (std::memory_order_relaxed);
        const double lastHz = sharedLastFrequency.load (std::memory_order_relaxed);
        const bool doGlide = glideTimeSec > 0.001f
                          && lastHz > 0.0
                          && (! glideLegatoOnly || heldBefore > 0);

        currentFrequency = doGlide ? lastHz : targetFrequency;
        sharedLastFrequency.store (targetFrequency, std::memory_order_relaxed);
        notesHeld.fetch_add (1, std::memory_order_relaxed);
        countedAsHeld = true;

        setupLayers (true);

        filterL.reset();
        filterR.reset();
        updateFilter (filterCutoff);
        samplesUntilFilterUpdate = 0;

        if (lfoRetrigger)
            lfoPhase = 0.0;

        noteActive = true;
        adsr.noteOn();
        filterEnv.noteOn();
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        adsr.noteOff();
        filterEnv.noteOff();

        // Count key-up immediately so legato sees real held notes (not release tails)
        releaseHeldCount();

        if (! allowTailOff || ! adsr.isActive())
            finishVoice();
    }

    void pitchWheelMoved (int /*newValue*/) override {}
    void controllerMoved (int /*controllerNumber*/, int /*newValue*/) override {}

    void prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
    {
        currentSampleRate = sampleRate;
        adsr.setSampleRate (sampleRate);
        filterEnv.setSampleRate (sampleRate);
        updateLfoPhaseDelta();
        filterL.reset();
        filterR.reset();
        updateFilter (filterCutoff);
    }

    void updateParameters (float attack, float decay, float sustain, float release,
                           float gainIn, bool fixedVelocityIn,
                           float glideTimeIn, bool glideLegatoIn,
                           const OscParams* newOscParams,
                           FilterType filterTypeIn, float cutoffIn, float resonanceIn, float driveIn,
                           float filtEnvAttack, float filtEnvDecay,
                           float filtEnvSustain, float filtEnvRelease, float filtEnvAmountIn,
                           int unisonVoicesIn, float unisonDetuneIn, float unisonSpreadIn,
                           float lfoRateIn, float lfoDepthIn, Waveform lfoWaveformIn,
                           LfoTarget lfoTargetIn, bool lfoRetriggerIn)
    {
        adsrParams.attack  = attack;
        adsrParams.decay   = decay;
        adsrParams.sustain = sustain;
        adsrParams.release = release;
        adsr.setParameters (adsrParams);

        filterEnvParams.attack  = filtEnvAttack;
        filterEnvParams.decay   = filtEnvDecay;
        filterEnvParams.sustain = filtEnvSustain;
        filterEnvParams.release = filtEnvRelease;
        filterEnv.setParameters (filterEnvParams);
        filterEnvAmount = juce::jlimit (-1.0f, 1.0f, filtEnvAmountIn);

        gain = gainIn;
        fixedVelocity = fixedVelocityIn;
        glideTimeSec = juce::jmax (0.0f, glideTimeIn);
        glideLegatoOnly = glideLegatoIn;

        bool oscChanged = false;

        for (int o = 0; o < numOscillators; ++o)
        {
            const auto& n = newOscParams[o];
            auto& cur = oscParams[o];

            if (cur.waveform != n.waveform
                || ! juce::approximatelyEqual (cur.level, n.level)
                || ! juce::approximatelyEqual (cur.semitone, n.semitone)
                || ! juce::approximatelyEqual (cur.fineCents, n.fineCents))
            {
                oscChanged = true;
            }

            cur = n;
        }

        const bool filterChanged = filterType != filterTypeIn
                                || ! juce::approximatelyEqual (filterCutoff, cutoffIn)
                                || ! juce::approximatelyEqual (filterResonance, resonanceIn);

        filterType = filterTypeIn;
        filterCutoff = cutoffIn;
        filterResonance = resonanceIn;
        driveAmount = juce::jlimit (0.0f, 1.0f, driveIn);

        // Static cutoff only when neither env nor LFO is modulating the filter
        if (filterChanged
            && lfoTarget != LfoTarget::FilterCutoff
            && juce::approximatelyEqual (filterEnvAmount, 0.0f))
        {
            updateFilter (filterCutoff);
        }

        const bool unisonChanged = unisonVoices != unisonVoicesIn
                                || ! juce::approximatelyEqual (unisonDetuneCents, unisonDetuneIn)
                                || ! juce::approximatelyEqual (unisonSpread, unisonSpreadIn);

        unisonVoices = juce::jlimit (1, maxUnison, unisonVoicesIn);
        unisonDetuneCents = unisonDetuneIn;
        unisonSpread = juce::jlimit (0.0f, 1.0f, unisonSpreadIn);

        if ((unisonChanged || oscChanged) && noteActive && baseFrequency > 0.0)
            setupLayers (false);

        const bool lfoRateChanged = ! juce::approximatelyEqual (lfoRate, lfoRateIn);
        lfoRate = lfoRateIn;
        lfoDepth = juce::jlimit (0.0f, 1.0f, lfoDepthIn);
        lfoWaveform = lfoWaveformIn;
        lfoTarget = lfoTargetIn;
        lfoRetrigger = lfoRetriggerIn;

        if (lfoRateChanged)
            updateLfoPhaseDelta();
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override
    {
        if (! noteActive)
            return;

        const auto numChannels = outputBuffer.getNumChannels();
        const float voiceScale = 1.0f / std::sqrt ((float) unisonVoices);

        float levelSum = 0.0f;
        for (int o = 0; o < numOscillators; ++o)
            levelSum += oscParams[o].level;

        const float oscScale = 1.0f / juce::jmax (1.0f, levelSum);

        while (--numSamples >= 0)
        {
            if (! adsr.isActive())
            {
                finishVoice();
                break;
            }

            advanceGlide();

            const auto lfo = (float) getLfoSample();
            advanceLfo();

            // Glide scales all oscillators relative to the target note
            double pitchRatio = (targetFrequency > 0.0) ? (currentFrequency / targetFrequency) : 1.0;

            if (lfoTarget == LfoTarget::Pitch && lfoDepth > 0.0f)
            {
                constexpr double maxSemitones = 2.0;
                pitchRatio *= std::pow (2.0, (double) (lfo * lfoDepth) * maxSemitones / 12.0);
            }

            float ampMod = 1.0f;
            if (lfoTarget == LfoTarget::Amplitude && lfoDepth > 0.0f)
                ampMod = juce::jmax (0.0f, 1.0f + lfo * lfoDepth);

            const auto ampEnv = adsr.getNextSample();
            const auto filtEnv = filterEnv.getNextSample();

            // Filter modulation: envelope (bipolar amount) + optional LFO
            if (filterType != FilterType::Off
                && (std::abs (filterEnvAmount) > 0.0001f
                    || (lfoTarget == LfoTarget::FilterCutoff && lfoDepth > 0.0f)))
            {
                if (--samplesUntilFilterUpdate <= 0)
                {
                    samplesUntilFilterUpdate = filterUpdateInterval;

                    constexpr float envMaxOctaves = 4.0f;
                    constexpr float lfoMaxOctaves = 3.0f;

                    float octaves = filtEnv * filterEnvAmount * envMaxOctaves;

                    if (lfoTarget == LfoTarget::FilterCutoff && lfoDepth > 0.0f)
                        octaves += lfo * lfoDepth * lfoMaxOctaves;

                    const float modCutoff = filterCutoff * std::pow (2.0f, octaves);
                    updateFilter (modCutoff);
                }
            }

            float left = 0.0f;
            float right = 0.0f;

            for (int u = 0; u < unisonVoices; ++u)
            {
                float mono = 0.0f;

                for (int o = 0; o < numOscillators; ++o)
                {
                    if (oscParams[o].level <= 0.0001f)
                        continue;

                    auto& phase = phases[u][o];
                    mono += (float) oscillatorSample (phase, oscParams[o].waveform) * oscParams[o].level;

                    phase += angleDeltas[u][o] * pitchRatio;

                    if (phase >= juce::MathConstants<double>::twoPi)
                        phase -= juce::MathConstants<double>::twoPi;
                    else if (phase < 0.0)
                        phase += juce::MathConstants<double>::twoPi;
                }

                left  += mono * pans[u].panL;
                right += mono * pans[u].panR;
            }

            const auto amp = (float) (level * ampEnv * gain * voiceScale * oscScale * ampMod);
            left  *= amp;
            right *= amp;

            if (filterType != FilterType::Off)
            {
                left  = filterL.processSingleSampleRaw (left);
                right = filterR.processSingleSampleRaw (right);
            }

            // Soft saturation after the filter (drive 0 = clean)
            if (driveAmount > 0.0001f)
            {
                left  = applyDrive (left);
                right = applyDrive (right);
            }

            if (numChannels >= 2)
            {
                outputBuffer.addSample (0, startSample, left);
                outputBuffer.addSample (1, startSample, right);

                for (int channel = 2; channel < numChannels; ++channel)
                    outputBuffer.addSample (channel, startSample, 0.5f * (left + right));
            }
            else if (numChannels == 1)
            {
                outputBuffer.addSample (0, startSample, 0.5f * (left + right));
            }

            ++startSample;
        }
    }

    using juce::SynthesiserVoice::renderNextBlock;

private:
    struct PanGains
    {
        float panL = 0.7071f;
        float panR = 0.7071f;
    };

    static double oscillatorSample (double angle, Waveform wave)
    {
        switch (wave)
        {
            case Waveform::Sine:
                return std::sin (angle);

            case Waveform::Saw:
                return (angle / juce::MathConstants<double>::pi) - 1.0;

            case Waveform::Square:
                return angle < juce::MathConstants<double>::pi ? 1.0 : -1.0;

            case Waveform::Triangle:
            {
                const auto t = angle / juce::MathConstants<double>::twoPi;
                return 4.0 * std::abs (t - 0.5) - 1.0;
            }

            default:
                return std::sin (angle);
        }
    }

    void releaseHeldCount()
    {
        if (! countedAsHeld)
            return;

        const int remaining = notesHeld.fetch_sub (1, std::memory_order_relaxed) - 1;
        if (remaining < 0)
            notesHeld.store (0, std::memory_order_relaxed);

        countedAsHeld = false;
    }

    void finishVoice()
    {
        releaseHeldCount();

        // Next note can glide from this pitch
        if (targetFrequency > 0.0)
            sharedLastFrequency.store (targetFrequency, std::memory_order_relaxed);

        clearCurrentNote();
        noteActive = false;
    }

    void advanceGlide()
    {
        if (targetFrequency <= 0.0)
            return;

        if (glideTimeSec <= 0.001f || currentSampleRate <= 0.0)
        {
            currentFrequency = targetFrequency;
            return;
        }

        // Exponential approach: roughly settles within glideTimeSec
        const double coeff = std::exp (-1.0 / ((double) glideTimeSec * currentSampleRate));
        currentFrequency = targetFrequency + (currentFrequency - targetFrequency) * coeff;

        if (std::abs (currentFrequency - targetFrequency) < 0.01)
            currentFrequency = targetFrequency;
    }

    void updateLfoPhaseDelta()
    {
        if (currentSampleRate <= 0.0)
            return;

        lfoPhaseDelta = (juce::jmax (0.01f, lfoRate) / currentSampleRate)
                        * juce::MathConstants<double>::twoPi;
    }

    void advanceLfo()
    {
        lfoPhase += lfoPhaseDelta;

        if (lfoPhase >= juce::MathConstants<double>::twoPi)
            lfoPhase -= juce::MathConstants<double>::twoPi;
    }

    double getLfoSample() const
    {
        return oscillatorSample (lfoPhase, lfoWaveform);
    }

    void setupLayers (bool resetPhases)
    {
        if (currentSampleRate <= 0.0 || baseFrequency <= 0.0)
            return;

        for (int u = 0; u < unisonVoices; ++u)
        {
            const float t = (unisonVoices == 1) ? 0.5f
                                                : (float) u / (float) (unisonVoices - 1);
            const float unisonCents = (t * 2.0f - 1.0f) * unisonDetuneCents;
            const double unisonRatio = std::pow (2.0, (double) unisonCents / 1200.0);

            for (int o = 0; o < numOscillators; ++o)
            {
                const double oscRatio = std::pow (2.0,
                    ((double) oscParams[o].semitone / 12.0)
                    + ((double) oscParams[o].fineCents / 1200.0));

                const double freq = baseFrequency * unisonRatio * oscRatio;
                angleDeltas[u][o] = (freq / currentSampleRate) * juce::MathConstants<double>::twoPi;

                if (resetPhases)
                {
                    phases[u][o] = juce::MathConstants<double>::twoPi
                                 * ((double) (u * numOscillators + o + 1)
                                    / (double) (unisonVoices * numOscillators + 1));
                }
            }

            const float pan = (unisonVoices == 1) ? 0.0f
                                                  : (t * 2.0f - 1.0f) * unisonSpread;
            const float panAngle = (pan * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi;
            pans[u].panL = std::cos (panAngle);
            pans[u].panR = std::sin (panAngle);
        }
    }

    float applyDrive (float sample) const
    {
        // 0 → clean, 1 → strong tanh saturation with mild makeup control
        const float driveGain = 1.0f + driveAmount * 18.0f;
        const float driven = std::tanh (sample * driveGain);

        // Keep overall level from exploding as drive increases
        const float norm = 1.0f / std::tanh (0.7f + driveAmount * 2.5f);
        return driven * norm * (1.0f - driveAmount * 0.15f);
    }

    void updateFilter (float cutoffHz)
    {
        if (currentSampleRate <= 0.0 || filterType == FilterType::Off)
            return;

        const auto nyquistLimit = (float) (currentSampleRate * 0.45);
        const auto safeCutoff = juce::jlimit (20.0f, nyquistLimit, cutoffHz);
        const auto safeQ = juce::jlimit (0.1f, 20.0f, filterResonance);

        juce::IIRCoefficients coeffs;

        switch (filterType)
        {
            case FilterType::LowPass:
                coeffs = juce::IIRCoefficients::makeLowPass (currentSampleRate, safeCutoff, safeQ);
                break;

            case FilterType::HighPass:
                coeffs = juce::IIRCoefficients::makeHighPass (currentSampleRate, safeCutoff, safeQ);
                break;

            case FilterType::BandPass:
                coeffs = juce::IIRCoefficients::makeBandPass (currentSampleRate, safeCutoff, safeQ);
                break;

            case FilterType::Off:
            default:
                return;
        }

        filterL.setCoefficients (coeffs);
        filterR.setCoefficients (coeffs);
    }

    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
    juce::ADSR filterEnv;
    juce::ADSR::Parameters filterEnvParams;
    juce::IIRFilter filterL, filterR;

    OscParams oscParams[numOscillators];
    double phases[maxUnison][numOscillators] {};
    double angleDeltas[maxUnison][numOscillators] {};
    PanGains pans[maxUnison];

    double currentSampleRate = 44100.0;
    double baseFrequency = 0.0;      // target note (for osc detune ratios)
    double targetFrequency = 0.0;    // note we are gliding toward
    double currentFrequency = 0.0;   // instantaneous pitched frequency
    double level = 0.0;
    bool noteActive = false;
    bool countedAsHeld = false;

    float gain = 0.7f;
    bool fixedVelocity = false;
    float glideTimeSec = 0.0f;
    bool glideLegatoOnly = false;

    // Shared across voices so successive notes can portamento
    inline static std::atomic<int> notesHeld { 0 };
    inline static std::atomic<double> sharedLastFrequency { 0.0 };

    FilterType filterType = FilterType::LowPass;
    float filterCutoff = 8000.0f;
    float filterResonance = 0.707f;
    float driveAmount = 0.0f;
    float filterEnvAmount = 0.0f;
    int samplesUntilFilterUpdate = 0;
    static constexpr int filterUpdateInterval = 16;

    int unisonVoices = 1;
    float unisonDetuneCents = 10.0f;
    float unisonSpread = 0.5f;

    double lfoPhase = 0.0;
    double lfoPhaseDelta = 0.0;
    float lfoRate = 2.0f;
    float lfoDepth = 0.0f;
    Waveform lfoWaveform = Waveform::Sine;
    LfoTarget lfoTarget = LfoTarget::Off;
    bool lfoRetrigger = true;
};
