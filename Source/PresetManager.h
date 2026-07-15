#pragma once

#include <JuceHeader.h>

//==============================================================================
/** Saves / loads APVTS state as XML presets in the user's app data folder. */
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& stateToManage)
        : apvts (stateToManage)
    {
        getPresetDirectory().createDirectory();
    }

    //==============================================================================
    static juce::File getPresetDirectory()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("aPathetic Synth")
                   .getChildFile ("Presets");
    }

    juce::StringArray getPresetNames() const
    {
        juce::StringArray names;
        const auto dir = getPresetDirectory();

        for (const auto& entry : juce::RangedDirectoryIterator (dir, false, "*.xml", juce::File::findFiles))
            names.add (entry.getFile().getFileNameWithoutExtension());

        names.sort (true);
        return names;
    }

    bool savePreset (const juce::String& name) const
    {
        const auto trimmed = name.trim();

        if (trimmed.isEmpty())
            return false;

        auto file = getPresetFile (trimmed);
        file.getParentDirectory().createDirectory();

        if (auto xml = apvts.copyState().createXml())
        {
            xml->setAttribute ("presetName", trimmed);
            return xml->writeTo (file);
        }

        return false;
    }

    bool loadPreset (const juce::String& name) const
    {
        const auto file = getPresetFile (name);

        if (! file.existsAsFile())
            return false;

        if (auto xml = juce::XmlDocument::parse (file))
        {
            // Accept either our APVTS type or any ValueTree-compatible root
            auto tree = juce::ValueTree::fromXml (*xml);

            if (! tree.isValid())
                return false;

            apvts.replaceState (tree);
            return true;
        }

        return false;
    }

    bool deletePreset (const juce::String& name) const
    {
        const auto file = getPresetFile (name);

        if (! file.existsAsFile())
            return false;

        return file.deleteFile();
    }

    /** Restores every parameter to its default value. */
    void loadInit()
    {
        for (auto* param : apvts.processor.getParameters())
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
                p->setValueNotifyingHost (p->getDefaultValue());

        currentPresetName = "Init";
    }

    /** Randomises every parameter, including all three oscillators. */
    void randomiseAll()
    {
        juce::Random random;

        for (auto* param : apvts.processor.getParameters())
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
                p->setValueNotifyingHost (random.nextFloat());

        // Guarantee at least one oscillator is loud enough to hear
        float maxLevel = 0.0f;
        juce::RangedAudioParameter* levelParams[3] = {};

        for (int i = 0; i < 3; ++i)
        {
            levelParams[i] = apvts.getParameter ("osc" + juce::String (i + 1) + "Level");

            if (levelParams[i] != nullptr)
                maxLevel = juce::jmax (maxLevel, levelParams[i]->getValue());
        }

        if (maxLevel < 0.25f && levelParams[0] != nullptr)
            levelParams[0]->setValueNotifyingHost (0.25f + random.nextFloat() * 0.75f);

        // Keep master gain from being near-silent
        if (auto* gain = apvts.getParameter ("gain"))
            if (gain->getValue() < 0.2f)
                gain->setValueNotifyingHost (0.2f + random.nextFloat() * 0.8f);

        currentPresetName = "Random";
    }

    juce::String getCurrentPresetName() const { return currentPresetName; }
    void setCurrentPresetName (const juce::String& name) { currentPresetName = name; }

private:
    static juce::String sanitiseFileName (const juce::String& name)
    {
        return name.trim()
                   .replaceCharacter ('/', '-')
                   .replaceCharacter ('\\', '-')
                   .replaceCharacter (':', '-')
                   .replaceCharacter ('*', '-')
                   .replaceCharacter ('?', '-')
                   .replaceCharacter ('"', '\'')
                   .replaceCharacter ('<', '-')
                   .replaceCharacter ('>', '-')
                   .replaceCharacter ('|', '-');
    }

    static juce::File getPresetFile (const juce::String& name)
    {
        return getPresetDirectory().getChildFile (sanitiseFileName (name) + ".xml");
    }

    juce::AudioProcessorValueTreeState& apvts;
    juce::String currentPresetName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
