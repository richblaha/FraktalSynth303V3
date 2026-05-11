#pragma once
#include <JuceHeader.h>

// ═══════════════════════════════════════════════════════════════════
//  PresetManager
//  Speichert/laedt Presets als XML-Dateien im Dokumente-Ordner.
//  Unterstuetzt Next/Prev-Navigation und Datei-Export.
// ═══════════════════════════════════════════════════════════════════
class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& a)
        : apvts(a)
    {
        presetDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("Fraktal611").getChildFile("Presets");
        presetDir.createDirectory();
        scanPresets();
    }

    // ── Speichern ──────────────────────────────────────────────────
    void save(const juce::String& name)
    {
        auto file = presetDir.getChildFile(name + ".fk6");
        auto state = apvts.copyState();
        auto xml   = state.createXml();
        if (xml) xml->writeTo(file);
        currentName = name;
        scanPresets();
        currentIndex = presetFiles.indexOf(file);
    }

    // ── Laden aus Datei ────────────────────────────────────────────
    bool loadFromFile(const juce::File& file)
    {
        if (!file.existsAsFile()) return false;
        auto xml = juce::XmlDocument::parse(file);
        if (!xml) return false;
        auto tree = juce::ValueTree::fromXml(*xml);
        if (!tree.isValid()) return false;
        apvts.replaceState(tree);
        currentName  = file.getFileNameWithoutExtension();
        currentIndex = presetFiles.indexOf(file);
        return true;
    }

    // ── Navigation ─────────────────────────────────────────────────
    void loadNext()
    {
        if (presetFiles.isEmpty()) return;
        currentIndex = (currentIndex + 1) % presetFiles.size();
        loadFromFile(presetFiles[currentIndex]);
    }

    void loadPrev()
    {
        if (presetFiles.isEmpty()) return;
        currentIndex = (currentIndex - 1 + presetFiles.size()) % presetFiles.size();
        loadFromFile(presetFiles[currentIndex]);
    }

    juce::String getCurrentName() const { return currentName; }
    int           getNumPresets()  const { return presetFiles.size(); }

private:
    void scanPresets()
    {
        presetFiles.clear();
        presetDir.findChildFiles(presetFiles, juce::File::findFiles, false, "*.fk6");
        presetFiles.sort();
    }

    juce::AudioProcessorValueTreeState& apvts;
    juce::File         presetDir;
    juce::Array<juce::File> presetFiles;
    juce::String       currentName = "Init";
    int                currentIndex = -1;
};
