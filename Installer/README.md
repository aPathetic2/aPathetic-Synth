# aPathetic Synth installer

Creates a single Windows setup executable that installs:

| Component   | Install location |
|------------|-------------------|
| Standalone | `C:\Program Files\aPathetic Synth\` |
| VST3       | `C:\Program Files\Common Files\VST3\aPathetic Synth.vst3` |
| Shortcuts  | Start Menu (optional desktop icon) |
| Uninstall  | Windows Apps & Features |

Presets stay in each user’s `%APPDATA%\aPathetic Synth\Presets\` (created when they save sounds). They are **not** part of the installer.

## Prerequisites

1. **Release build** of the plugin (Visual Studio, `Release | x64`):
   - Shared Code  
   - Standalone Plugin → `Builds\VisualStudio2026\x64\Release\Standalone Plugin\aPathetic Synth.exe`  
   - VST3 → `Builds\VisualStudio2026\x64\Release\VST3\aPathetic Synth.vst3\`

2. **[Inno Setup 6](https://jrsoftware.org/isinfo.php)** (free)

## Build the setup file

```powershell
cd "D:\darre\Documents\JUCE\Projects\aPathetic Synth\Installer"
.\build-installer.ps1
```

Or open `aPatheticSynth.iss` in the Inno Setup Compiler and press **Ctrl+F9**.

Output:

```text
Installer\Output\aPatheticSynth-Setup-1.1.1.exe
```

Distribute that `.exe` only (users do not need Visual Studio or Inno Setup).

The setup uses the organ-style icon in `assets\apathetic-synth-icon.ico` (wood panel, rotary knob, amber LED). To change it, replace that `.ico` and re-run `build-installer.ps1`.

## Version bumps

Edit these lines at the top of `aPatheticSynth.iss`:

```iss
#define MyAppVersion   "1.1.1"
```

The setup filename becomes `aPatheticSynth-Setup-<version>.exe`. Keep `AppId` the same across versions so Windows treats updates as upgrades of the same product.

## Notes

- **Admin rights** are required (writes under Program Files and Common Files).
- **64-bit only**, matching the x64 VST3/standalone build.
- Recipients may need the [Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) if the standalone fails to start on a clean PC (most DAW machines already have it).
- After install, rescan plugins in the DAW if the VST3 does not appear immediately.
