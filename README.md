# aPathetic Synth

A Windows VST3 + standalone software synthesizer with a 1980s home electronic organ aesthetic.

**Manufacturer:** aPathetic2  
**Version:** 1.0.1  
**Formats:** VST3 (64-bit), Standalone  

## Features

- Three oscillators (sine / saw / square / triangle) with level, semi, and fine tune  
- ADSR amp envelope + filter envelope  
- Low / high / band-pass filter with drive  
- Unison, glide, LFO (rate, depth, waveform, target)  
- Chorus, tempo-sync delay, reverb  
- On-screen keyboard, preset save/load/random, output meter  
- Organ-style UI (wood, ivory, gold, amber)  

## Requirements

- Windows 10/11 **64-bit**  
- A VST3 host (FL Studio, Ableton, Reaper, etc.) for the plugin format  
- [JUCE](https://juce.com/) (to build from source)  
- Visual Studio 2022 or 2026 with C++ desktop workload  

## Download (installer)

**[Download aPathetic Synth for Windows (setup)](https://github.com/aPathetic2/aPathetic-Synth/releases/latest)**

Or open the [Releases](https://github.com/aPathetic2/aPathetic-Synth/releases) page and get the latest setup `.exe`.

### Install

1. Run the setup executable (admin rights required).  
2. **Standalone** → `C:\Program Files\aPathetic Synth\`  
3. **VST3** → `C:\Program Files\Common Files\VST3\aPathetic Synth.vst3`  
4. Rescan plugins in your DAW if needed.  

Presets are stored per-user in:

`%APPDATA%\aPathetic Synth\Presets\`

## Build from source

1. Install JUCE and open `aPathetic Synth.jucer` in **Projucer**.  
2. Set the global JUCE modules path if needed, then save to refresh exporters.  
3. Open `Builds/VisualStudio2026/aPathetic Synth.sln` (or VS2022).  
4. Build **Release | x64** for Shared Code, Standalone Plugin, and VST3.  

Icon assets live in `Assets/` (`icon.png` / `icon.ico`).  

### Optional: Windows installer

1. Install [Inno Setup 6](https://jrsoftware.org/isinfo.php).  
2. Build Release standalone + VST3.  
3. From PowerShell:

```powershell
cd Installer
.\build-installer.ps1
```

Output: `Installer/Output/aPatheticSynth-Setup-<version>.exe`

## Project layout

```
Source/              Plugin + synth engine + UI
JuceLibraryCode/     Projucer-generated includes
Assets/              App icon
Installer/           Inno Setup script + icon assets
Tools/               Offline preset normaliser (dev helper)
Builds/              Visual Studio projects (build outputs gitignored)
```

## License

Copyright aPathetic2. All rights reserved unless otherwise stated.
