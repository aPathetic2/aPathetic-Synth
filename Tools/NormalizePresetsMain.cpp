/*
  Offline peak-normaliser for aPathetic Synth factory presets.
  Loads each preset, renders a test pattern, scales gain so peak ≈ 0 dB,
  and if still quiet at max gain, raises drive (user OK with tone change).
  Skips "aPathetic 1" and "aPathetic 2".
*/

#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"

#include <cmath>
#include <iostream>

namespace
{
constexpr double kSampleRate = 44100.0;
constexpr int    kBlockSize  = 512;
constexpr float  kTargetPeak = 1.0f;   // 0 dBFS
constexpr float  kTolerance  = 0.03f;  // ~0.26 dB (allows for 0.01 gain quantisation)
constexpr int    kMaxIters   = 8;
constexpr int    kDriveSearchSteps = 10;
constexpr float  kMaxGain    = 2.0f;   // must match PluginProcessor gain range

const juce::StringArray kSkipNames { "aPathetic 1", "aPathetic 2" };

float getParam (APatheticSynthAudioProcessor& p, const char* id)
{
    return p.getAPVTS().getRawParameterValue (id)->load();
}

void setParam (APatheticSynthAudioProcessor& p, const char* id, float actualValue, float minV, float maxV)
{
    if (auto* param = p.getAPVTS().getParameter (id))
    {
        const float clamped = juce::jlimit (minV, maxV, actualValue);
        param->setValueNotifyingHost (param->convertTo0to1 (clamped));
    }
}

float getGain  (APatheticSynthAudioProcessor& p) { return getParam (p, "gain"); }
float getDrive (APatheticSynthAudioProcessor& p) { return getParam (p, "drive"); }
void  setGain  (APatheticSynthAudioProcessor& p, float g) { setParam (p, "gain",  g, 0.0f, kMaxGain); }
void  setDrive (APatheticSynthAudioProcessor& p, float d) { setParam (p, "drive", d, 0.0f, 1.0f); }

void processSeconds (APatheticSynthAudioProcessor& p,
                     juce::AudioBuffer<float>& buffer,
                     float seconds,
                     float& peakOut,
                     const juce::MidiMessage* eventAtStart = nullptr)
{
    const int totalSamples = juce::jmax (1, (int) std::round (seconds * kSampleRate));
    int done = 0;

    while (done < totalSamples)
    {
        const int n = juce::jmin (kBlockSize, totalSamples - done);
        buffer.setSize (2, n, false, false, true);
        buffer.clear();

        juce::MidiBuffer midi;

        if (eventAtStart != nullptr && done == 0)
            midi.addEvent (*eventAtStart, 0);

        p.processBlock (buffer, midi);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* data = buffer.getReadPointer (ch);

            for (int i = 0; i < n; ++i)
                peakOut = juce::jmax (peakOut, std::abs (data[i]));
        }

        done += n;
    }
}

/** Render a multi-note test pattern; returns absolute peak. */
float measurePeak (APatheticSynthAudioProcessor& p)
{
    p.releaseResources();
    p.setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);
    p.prepareToPlay (kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer (2, kBlockSize);
    float peak = 0.0f;

    const int notes[] = { 36, 48, 60, 72 };

    for (int note : notes)
    {
        const auto noteOn  = juce::MidiMessage::noteOn  (1, note, (juce::uint8) 127);
        const auto noteOff = juce::MidiMessage::noteOff (1, note);

        processSeconds (p, buffer, 1.0f, peak, &noteOn);
        processSeconds (p, buffer, 0.05f, peak, &noteOff);
        processSeconds (p, buffer, 0.35f, peak);
    }

    // Chord stab (C3 E3 G3 C4)
    {
        juce::MidiBuffer chordOn;
        chordOn.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 127), 0);
        chordOn.addEvent (juce::MidiMessage::noteOn (1, 52, (juce::uint8) 127), 0);
        chordOn.addEvent (juce::MidiMessage::noteOn (1, 55, (juce::uint8) 127), 0);
        chordOn.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 127), 0);

        buffer.setSize (2, kBlockSize, false, false, true);
        buffer.clear();
        p.processBlock (buffer, chordOn);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* data = buffer.getReadPointer (ch);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
                peak = juce::jmax (peak, std::abs (data[i]));
        }

        processSeconds (p, buffer, 1.2f, peak);

        juce::MidiBuffer chordOff;
        chordOff.addEvent (juce::MidiMessage::noteOff (1, 48), 0);
        chordOff.addEvent (juce::MidiMessage::noteOff (1, 52), 0);
        chordOff.addEvent (juce::MidiMessage::noteOff (1, 55), 0);
        chordOff.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
        buffer.clear();
        p.processBlock (buffer, chordOff);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* data = buffer.getReadPointer (ch);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
                peak = juce::jmax (peak, std::abs (data[i]));
        }
    }

    processSeconds (p, buffer, 2.8f, peak);

    p.releaseResources();
    return peak;
}

bool shouldSkip (const juce::String& name)
{
    return kSkipNames.contains (name);
}

/**
  Drive soft-clips, so it boosts quiet signals toward ~0.85 but will not
  push an already-hot signal to 1.0. Binary-search drive, keep the setting
  that lands closest to target (may leave drive unchanged if it doesn't help).
*/
float boostWithDrive (APatheticSynthAudioProcessor& p, float currentPeak)
{
    const float startDrive = getDrive (p);

    if (startDrive >= 0.999f)
        return currentPeak;

    float bestDrive = startDrive;
    float bestPeak  = currentPeak;

    // Sweep drive upward; keep the value nearest 0 dBFS
    for (int i = 0; i <= kDriveSearchSteps; ++i)
    {
        const float d = startDrive + (1.0f - startDrive) * (float) i / (float) kDriveSearchSteps;
        setDrive (p, d);
        const float peak = measurePeak (p);

        if (std::abs (peak - kTargetPeak) < std::abs (bestPeak - kTargetPeak))
        {
            bestDrive = d;
            bestPeak  = peak;
        }

        // Once we can reach target, refine with a short binary search above previous step
        if (peak >= kTargetPeak - kTolerance && i > 0)
        {
            float lo = startDrive + (1.0f - startDrive) * (float) (i - 1) / (float) kDriveSearchSteps;
            float hi = d;

            for (int b = 0; b < 6; ++b)
            {
                const float mid = 0.5f * (lo + hi);
                setDrive (p, mid);
                const float midPeak = measurePeak (p);

                if (std::abs (midPeak - kTargetPeak) < std::abs (bestPeak - kTargetPeak))
                {
                    bestDrive = mid;
                    bestPeak  = midPeak;
                }

                if (midPeak < kTargetPeak)
                    lo = mid;
                else
                    hi = mid;
            }
            break;
        }
    }

    setDrive (p, bestDrive);
    std::cout << "  drive search -> drive=" << bestDrive << ", peak=" << bestPeak << "\n";
    return measurePeak (p);
}

bool normalizePreset (APatheticSynthAudioProcessor& p, const juce::String& name)
{
    if (! p.getPresetManager().loadPreset (name))
    {
        std::cout << "  FAIL: could not load\n";
        return false;
    }

    const float gainBefore  = getGain (p);
    const float driveBefore = getDrive (p);
    float peak = measurePeak (p);

    if (peak < 1.0e-6f)
    {
        std::cout << "  SKIP: silent (peak ~ 0)\n";
        return false;
    }

    float peakDb = juce::Decibels::gainToDecibels (peak, -100.0f);
    std::cout << "  initial peak=" << peak << " (" << peakDb << " dB)"
              << ", gain=" << gainBefore
              << ", drive=" << driveBefore << "\n";

    for (int iter = 0; iter < kMaxIters; ++iter)
    {
        if (std::abs (peak - kTargetPeak) <= kTolerance)
            break;

        if (peak > kTargetPeak)
        {
            // Too hot — reduce gain only (leave drive as-is for character)
            const float scale = kTargetPeak / peak;
            const float newG = juce::jlimit (0.01f, kMaxGain, getGain (p) * scale);
            setGain (p, newG);
            peak = measurePeak (p);
        }
        else
        {
            // Too quiet — raise gain first (up to kMaxGain), then drive
            const float scale = kTargetPeak / peak;
            const float g = getGain (p);
            const float newG = g * scale;

            if (newG <= kMaxGain + 1.0e-4f && g < kMaxGain - 1.0e-4f)
            {
                setGain (p, juce::jmin (kMaxGain, newG));
                peak = measurePeak (p);
            }
            else
            {
                setGain (p, kMaxGain);
                peak = measurePeak (p);

                if (peak < kTargetPeak - kTolerance)
                {
                    std::cout << "  gain maxed at " << kMaxGain << "; boosting drive...\n";
                    peak = boostWithDrive (p, peak);

                    // Drive may have overshot — trim gain back down
                    if (peak > kTargetPeak + kTolerance)
                    {
                        setGain (p, juce::jlimit (0.01f, kMaxGain, getGain (p) * (kTargetPeak / peak)));
                        peak = measurePeak (p);
                    }
                }
            }
        }

        peakDb = juce::Decibels::gainToDecibels (peak, -100.0f);
        std::cout << "  iter " << (iter + 1) << ": peak=" << peak
                  << " (" << peakDb << " dB)"
                  << ", gain=" << getGain (p)
                  << ", drive=" << getDrive (p) << "\n";

        if (peak < kTargetPeak - kTolerance
            && getGain (p) >= kMaxGain - 1.0e-3f
            && getDrive (p) >= 0.999f)
        {
            std::cout << "  cannot reach 0 dB (gain+drive maxed)\n";
            break;
        }
    }

    if (! p.getPresetManager().savePreset (name))
    {
        std::cout << "  FAIL: could not save\n";
        return false;
    }

    const float finalDb = juce::Decibels::gainToDecibels (peak, -100.0f);
    std::cout << "  saved  peak=" << peak << " (" << finalDb << " dB)"
              << ", gain=" << getGain (p) << " (was " << gainBefore << ")"
              << ", drive=" << getDrive (p) << " (was " << driveBefore << ")\n";
    return true;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    APatheticSynthAudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);

    auto names = processor.getPresetManager().getPresetNames();
    std::cout << "Found " << names.size() << " presets in "
              << PresetManager::getPresetDirectory().getFullPathName() << "\n\n";

    int ok = 0, skipped = 0, failed = 0;

    for (const auto& name : names)
    {
        if (shouldSkip (name))
        {
            std::cout << "[skip] " << name << "\n";
            ++skipped;
            continue;
        }

        std::cout << "[norm] " << name << "\n";

        if (normalizePreset (processor, name))
            ++ok;
        else
            ++failed;
    }

    std::cout << "\nDone. normalized=" << ok
              << " skipped=" << skipped
              << " failed=" << failed << "\n";

    return failed > 0 ? 1 : 0;
}
