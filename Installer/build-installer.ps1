#Requires -Version 5.1
<#
.SYNOPSIS
  Builds the aPathetic Synth setup executable with Inno Setup.

.DESCRIPTION
  Checks that Release standalone + VST3 exist, locates ISCC.exe, and compiles
  aPatheticSynth.iss. Output goes to Installer\Output\.
#>

$ErrorActionPreference = 'Stop'
$InstallerDir = $PSScriptRoot
$IssFile      = Join-Path $InstallerDir 'aPatheticSynth.iss'
$BuildRoot    = Join-Path $InstallerDir '..\Builds\VisualStudio2026\x64\Release' | Resolve-Path -ErrorAction SilentlyContinue

function Find-ISCC {
    $candidates = @(
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
        "${env:ProgramFiles}\Inno Setup 6\ISCC.exe"
        "${env:LocalAppData}\Programs\Inno Setup 6\ISCC.exe"
        "${env:ProgramFiles(x86)}\Inno Setup 5\ISCC.exe"
    )
    foreach ($p in $candidates) {
        if ($p -and (Test-Path -LiteralPath $p)) { return $p }
    }
    # PATH fallback
    $cmd = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

Write-Host '=== aPathetic Synth installer ===' -ForegroundColor Cyan

if (-not (Test-Path -LiteralPath $IssFile)) {
    throw "Script not found: $IssFile"
}

# ---- Verify Release builds ----
if (-not $BuildRoot) {
    throw "Release build folder not found. Build Release|x64 first.`nExpected under: Builds\VisualStudio2026\x64\Release"
}

$standalone = Join-Path $BuildRoot 'Standalone Plugin\aPathetic Synth.exe'
$vst3Root   = Join-Path $BuildRoot 'VST3\aPathetic Synth.vst3'
$vst3Binary = Join-Path $vst3Root 'Contents\x86_64-win\aPathetic Synth.vst3'

if (-not (Test-Path -LiteralPath $standalone)) {
    throw "Standalone missing:`n  $standalone`nBuild Release | x64 Standalone Plugin first."
}
if (-not (Test-Path -LiteralPath $vst3Binary)) {
    throw "VST3 missing:`n  $vst3Binary`nBuild Release | x64 VST3 first."
}

Write-Host "Standalone : $standalone"
Write-Host "VST3       : $vst3Root"

# ---- Find Inno Setup compiler ----
$iscc = Find-ISCC
if (-not $iscc) {
    throw @"
Inno Setup 6 not found (ISCC.exe).

Install from: https://jrsoftware.org/isinfo.php
Then re-run this script, or compile aPatheticSynth.iss from the Inno IDE.
"@
}
Write-Host "ISCC       : $iscc"

# ---- Compile ----
Write-Host "`nCompiling..." -ForegroundColor Cyan
& $iscc $IssFile
if ($LASTEXITCODE -ne 0) {
    throw "ISCC failed with exit code $LASTEXITCODE"
}

$outDir = Join-Path $InstallerDir 'Output'
$setup  = Get-ChildItem -Path $outDir -Filter 'aPatheticSynth-Setup-*.exe' -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if ($setup) {
    Write-Host "`nSuccess:" -ForegroundColor Green
    Write-Host "  $($setup.FullName)"
    Write-Host "  $([math]::Round($setup.Length / 1MB, 2)) MB"
} else {
    Write-Host "`nCompile finished; check Installer\Output\" -ForegroundColor Yellow
}
