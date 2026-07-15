;==============================================================================
;  aPathetic Synth — Inno Setup script
;
;  Packages the Release standalone app + VST3 for distribution.
;
;  Prerequisites:
;    1. Build Release | x64 in Visual Studio (Shared Code, Standalone, VST3)
;    2. Install Inno Setup 6: https://jrsoftware.org/isinfo.php
;
;  Compile:
;    - Run:  .\build-installer.ps1
;    - Or open this file in Inno Setup Compiler and press Ctrl+F9
;    - Or:   ISCC.exe aPatheticSynth.iss
;
;  Output:
;    Installer\Output\aPatheticSynth-Setup-1.0.0.exe
;==============================================================================

#define MyAppName      "aPathetic Synth"
#define MyAppVersion   "1.0.0"
#define MyAppPublisher "Darren"
#define MyAppURL       "https://www.Darren.com"
#define MyAppExeName   "aPathetic Synth.exe"

; Release build outputs (relative to this .iss file)
#define BuildRoot      "..\Builds\VisualStudio2026\x64\Release"
#define StandaloneSrc  BuildRoot + "\Standalone Plugin\" + MyAppExeName
#define Vst3Src        BuildRoot + "\VST3\aPathetic Synth.vst3\*"

[Setup]
; Unique product ID — keep stable across versions so upgrades replace the old install
AppId={{A9E4C2B1-7F3D-4A8E-9C01-6B5D2E8F4A70}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
; 64-bit only (matches the VS x64 build)
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Program Files + Common Files\VST3 need elevation
PrivilegesRequired=admin
OutputDir=Output
OutputBaseFilename=aPatheticSynth-Setup-{#MyAppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; Custom icon for the setup.exe and wizard (organ-style knob + amber LED)
SetupIconFile=assets\apathetic-synth-icon.ico
; Uninstall entry / Start Menu can use the same art once copied into {app}
UninstallDisplayIcon={app}\apathetic-synth-icon.ico
UsePreviousAppDir=yes
DisableDirPage=no
CloseApplications=yes
RestartApplications=no
; LicenseFile=   ; optional: add a License.txt and uncomment

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; \
    GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "startmenu"; Description: "Create Start Menu shortcuts"; \
    GroupDescription: "{cm:AdditionalIcons}"; Flags: checkedonce

[Files]
; ---- Standalone application ----
; {app} = e.g. C:\Program Files\aPathetic Synth
Source: "{#StandaloneSrc}"; DestDir: "{app}"; Flags: ignoreversion

; App icon (shortcuts + Apps & Features uninstall display)
Source: "assets\apathetic-synth-icon.ico"; DestDir: "{app}"; Flags: ignoreversion

; ---- VST3 bundle (full Contents\x86_64-win structure) ----
; {commoncf64} = C:\Program Files\Common Files  on 64-bit Windows
Source: "{#Vst3Src}"; \
    DestDir: "{commoncf64}\VST3\aPathetic Synth.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; \
    IconFilename: "{app}\apathetic-synth-icon.ico"; Tasks: startmenu
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"; \
    IconFilename: "{app}\apathetic-synth-icon.ico"; Tasks: startmenu
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; \
    IconFilename: "{app}\apathetic-synth-icon.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; \
    Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; \
    Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: dirifempty; Name: "{app}"
