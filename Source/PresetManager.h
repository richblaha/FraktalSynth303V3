#pragma once
#include <JuceHeader.h>

// ═══════════════════════════════════════════════════════════════════
//  PresetManager mit Kategorien
//
//  Ordnerstruktur:
//    ~/Documents/Fraktal611/Presets/
//      Bass/          ← Kategorie
//        DeepSub.fk6
//        WobbleBass.fk6
//      Pads/
//        AmbientPad.fk6
//      Leads/
//        ...
//
//  Presets ohne Unterordner landen in "Default"
// ═══════════════════════════════════════════════════════════════════
class PresetManager
{
public:
    struct Preset {
        juce::String category;
        juce::String name;
        juce::File   file;
    };

    explicit PresetManager(juce::AudioProcessorValueTreeState& a) : apvts(a)
    {
        presetDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("Fraktal611").getChildFile("Presets");
        presetDir.createDirectory();
        // Standard-Kategorien anlegen falls leer
        for (auto& cat : {"Bass","Pads","Leads","Keys","FX","Ambient","Drums","Default"})
            presetDir.getChildFile(cat).createDirectory();
        scanPresets();
    }

    // ── Alle Kategorien ────────────────────────────────────────────
    juce::StringArray getCategories() const
    {
        juce::StringArray cats;
        for (auto& p : allPresets)
            if (!cats.contains(p.category)) cats.add(p.category);
        cats.sort(false);
        return cats;
    }

    // ── Presets einer Kategorie ────────────────────────────────────
    juce::StringArray getPresetsInCategory(const juce::String& category) const
    {
        juce::StringArray names;
        for (auto& p : allPresets)
            if (p.category == category) names.add(p.name);
        names.sort(false);
        return names;
    }

    // ── Laden per Kategorie + Name ─────────────────────────────────
    bool load(const juce::String& category, const juce::String& name)
    {
        for (auto& p : allPresets)
            if (p.category==category && p.name==name)
                return loadFromFile(p.file);
        return false;
    }

    bool loadFromFile(const juce::File& file)
    {
        if (!file.existsAsFile()) return false;
        auto xml = juce::XmlDocument::parse(file);
        if (!xml) return false;
        auto tree = juce::ValueTree::fromXml(*xml);
        if (!tree.isValid()) return false;
        juce::String s1 = tree.getProperty("smp1Path","").toString();
        juce::String s2 = tree.getProperty("smp2Path","").toString();
        apvts.replaceState(tree);
        if (s1.isNotEmpty()) smp1Path=s1;
        if (s2.isNotEmpty()) smp2Path=s2;
        // Kategorie + Name aus Pfad ableiten
        currentCategory = file.getParentDirectory().getFileName();
        currentName     = file.getFileNameWithoutExtension();
        onSamplePathsLoaded(smp1Path, smp2Path);
        return true;
    }

    // ── Speichern ─────────────────────────────────────────────────
    void save(const juce::String& category, const juce::String& name)
    {
        auto catDir = presetDir.getChildFile(category);
        catDir.createDirectory();
        auto file = catDir.getChildFile(name + ".fk6");
        auto state = apvts.copyState();
        if (smp1Path.isNotEmpty()) state.setProperty("smp1Path",smp1Path,nullptr);
        if (smp2Path.isNotEmpty()) state.setProperty("smp2Path",smp2Path,nullptr);
        if (auto xml = state.createXml()) {
            juce::FileOutputStream stream(file);
            if (stream.openedOk()) { stream.setPosition(0); stream.truncate(); xml->writeTo(stream); }
        }
        currentCategory = category;
        currentName     = name;
        scanPresets();
    }

    // ── Navigation ────────────────────────────────────────────────
    void loadNext()
    {
        if (allPresets.isEmpty()) return;
        currentIndex = (currentIndex+1) % allPresets.size();
        loadFromFile(allPresets[currentIndex].file);
    }
    void loadPrev()
    {
        if (allPresets.isEmpty()) return;
        currentIndex = (currentIndex-1+allPresets.size()) % allPresets.size();
        loadFromFile(allPresets[currentIndex].file);
    }

    juce::String getCurrentName()     const { return currentName; }
    juce::String getCurrentCategory() const { return currentCategory; }
    int          getNumPresets()      const { return allPresets.size(); }

    std::function<void(const juce::String&,const juce::String&)>
        onSamplePathsLoaded = [](const juce::String&,const juce::String&){};

    juce::String smp1Path, smp2Path;

    void scanPresets()
    {
        allPresets.clear();
        currentIndex = -1;
        // Alle .fk6 Dateien rekursiv suchen
        juce::Array<juce::File> files;
        presetDir.findChildFiles(files, juce::File::findFiles, true, "*.fk6");
        for (auto& f : files) {
            Preset p;
            p.file     = f;
            p.name     = f.getFileNameWithoutExtension();
            // Kategorie = Elternordner (oder "Default" wenn direkt im Presets-Ordner)
            p.category = (f.getParentDirectory()==presetDir) ? "Default"
                          : f.getParentDirectory().getFileName();
            allPresets.add(p);
            if (p.category==currentCategory && p.name==currentName)
                currentIndex = allPresets.size()-1;
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::File    presetDir;
    juce::Array<Preset> allPresets;
    juce::String  currentName     = "Init";
    juce::String  currentCategory = "Default";
    int           currentIndex    = -1;
};
