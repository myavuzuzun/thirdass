# Build and Test Script for Third Ass VST2 on Windows x64
param(
    [switch]$Install = $false
)

$ErrorActionPreference = "Stop"

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host " Building Third Ass VST2 (Windows x64 Native)    " -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# Ensure output directories exist
if (-not (Test-Path "dist")) { New-Item -ItemType Directory -Path "dist" | Out-Null }
if (-not (Test-Path "bin")) { New-Item -ItemType Directory -Path "bin" | Out-Null }

# 1. Compile VST2 64-bit Dynamic Link Library (Ultra-Lightweight & Optimized)
Write-Host "[1/3] Compiling dist/ThirdAss.dll (Direct2D GPU GUI + 64-bit Audio DSP)..." -ForegroundColor Yellow
g++ -O3 -std=c++17 -Wall -Wextra -Iinclude -fno-math-errno -fno-trapping-math -static -shared -fno-rtti -fno-exceptions -ffunction-sections -fdata-sections -flto "-Wl,--gc-sections" -s src/vst2/ThirdBassVST2.cpp src/vst2/ThirdAss.def -ld2d1 -ldwrite -lole32 -lgdi32 -luser32 -o dist/ThirdAss.dll

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile dist/ThirdAss.dll"
    exit 1
}
$dllSize = (Get-Item "dist\ThirdAss.dll").Length
Write-Host "      -> dist/ThirdAss.dll built successfully! ($([math]::Round($dllSize/1KB, 1)) KB)" -ForegroundColor Green

# 2. Compile Standalone GUI Preview Executable
Write-Host "[2/3] Compiling bin/thirdbass_gui.exe (Standalone Direct2D preview)..." -ForegroundColor Yellow
g++ -O3 -std=c++17 -Wall -Wextra -Iinclude -fno-math-errno -fno-trapping-math -static -fno-rtti -fno-exceptions -ffunction-sections -fdata-sections -flto "-Wl,--gc-sections" -s src/standalone/gui_main.cpp -ld2d1 -ldwrite -lole32 -lgdi32 -luser32 -o bin/thirdbass_gui.exe

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile bin/thirdbass_gui.exe"
    exit 1
}
$exeSize = (Get-Item "bin\thirdbass_gui.exe").Length
Write-Host "      -> bin/thirdbass_gui.exe built successfully! ($([math]::Round($exeSize/1KB, 1)) KB)" -ForegroundColor Green

# 3. Run Verification Test Suite
Write-Host "[3/3] Running VST2 test suite..." -ForegroundColor Yellow
powershell -ExecutionPolicy Bypass -File .\test_vst.ps1

if ($LASTEXITCODE -ne 0) {
    Write-Error "Test suite failed!"
    exit 1
}

# Optional installation
if ($Install -or $true) {
    Write-Host "==================================================" -ForegroundColor Cyan
    Write-Host " Installing VST2 Plugin for LMMS, REAPER & Windows " -ForegroundColor Cyan
    Write-Host "==================================================" -ForegroundColor Cyan

    $lmmsVstDir = "$env:USERPROFILE\Documents\lmms\plugins\vst"
    if (-not (Test-Path $lmmsVstDir)) {
        New-Item -ItemType Directory -Path $lmmsVstDir -Force | Out-Null
    }
    
    # Safe atomic copy if file is in use by LMMS
    try {
        Copy-Item "dist\ThirdAss.dll" "$lmmsVstDir\ThirdAss.dll" -Force
    } catch {
        Move-Item "$lmmsVstDir\ThirdAss.dll" "$lmmsVstDir\ThirdAss.dll.old" -Force -ErrorAction SilentlyContinue
        Copy-Item "dist\ThirdAss.dll" "$lmmsVstDir\ThirdAss.dll" -Force
    }
    Write-Host "  -> Installed to LMMS VeSTige: $lmmsVstDir\ThirdAss.dll" -ForegroundColor Green

    # Ensure VeSTige 1-click preset is installed
    $lmmsPresetDir = "$env:USERPROFILE\Documents\lmms\presets\VeSTige"
    if (-not (Test-Path $lmmsPresetDir)) {
        New-Item -ItemType Directory -Path $lmmsPresetDir -Force | Out-Null
    }
    $presetXml = @"
<?xml version="1.0"?>
<!DOCTYPE lmms-project>
<lmms-project version="30" creator="LMMS" type="instrumenttracksettings" creatorversion="1.3.0">
  <head/>
  <instrumenttracksettings name="Third Ass" solo="0" muted="0" mutedBeforeSolo="0" type="0">
    <instrumenttrack enabled="0" range_import="1" enablecc="0" basenote="57" pitch="0" usemasterpitch="1" pan="0" vol="100" pitchrange="1" keymap="0" lastkey="127" firstkey="0" mixch="0" scale="0">
      <instrument name="vestige">
        <vestige plugin="$($lmmsVstDir.Replace('\', '/'))/ThirdAss.dll"/>
      </instrument>
      <eldata fres="0.5" fwet="0" ftype="0" fcut="14000">
        <elvol lspd_denominator="4" lspd_syncmode="0" ctlenvamt="0" lshp="0" rel="0.1" lspd_numerator="4" lamt="0" sustain="0.5" lpdel="0" lspd="0.1" pdel="0" dec="0.5" amt="0" x100="0" att="0" hold="0.5" latt="0"/>
        <elcut lspd_denominator="4" lspd_syncmode="0" ctlenvamt="0" lshp="0" rel="0.1" lspd_numerator="4" lamt="0" sustain="0.5" lpdel="0" lspd="0.1" pdel="0" dec="0.5" amt="0" x100="0" att="0" hold="0.5" latt="0"/>
        <elres lspd_denominator="4" lspd_syncmode="0" ctlenvamt="0" lshp="0" rel="0.1" lspd_numerator="4" lamt="0" sustain="0.5" lpdel="0" lspd="0.1" pdel="0" dec="0.5" amt="0" x100="0" att="0" hold="0.5" latt="0"/>
      </eldata>
      <chordcreator chordrange="1" chord-enabled="0" chord="0"/>
      <arpeggiator arp-enabled="0" arpgate="100" arpcycle="0" arptime_denominator="4" arpmiss="0" arptime_syncmode="0" arptime="200" arpmode="0" arprepeats="1" arpskip="0" arptime_numerator="4" arp="0" arprange="1" arpdir="0"/>
      <fxchain enabled="0" numofeffects="0"/>
    </instrumenttrack>
  </instrumenttracksettings>
</lmms-project>
"@
    Set-Content -Path "$lmmsPresetDir\ThirdAss.xpf" -Value $presetXml
    Write-Host "  -> Installed LMMS 1-click preset: $lmmsPresetDir\ThirdAss.xpf" -ForegroundColor Green

    # Install for REAPER & Global VST Directories
    $vstGlobalDir = "$env:USERPROFILE\Documents\VstPlugins"
    if (-not (Test-Path $vstGlobalDir)) {
        New-Item -ItemType Directory -Path $vstGlobalDir -Force | Out-Null
    }
    Copy-Item "dist\ThirdAss.dll" "$vstGlobalDir\ThirdAss.dll" -Force
    Write-Host "  -> Installed to VstPlugins: $vstGlobalDir\ThirdAss.dll" -ForegroundColor Green

    $reaperUserPluginsDir = "$env:APPDATA\REAPER\UserPlugins"
    if (Test-Path "$env:APPDATA\REAPER") {
        if (-not (Test-Path $reaperUserPluginsDir)) {
            New-Item -ItemType Directory -Path $reaperUserPluginsDir -Force | Out-Null
        }
        Copy-Item "dist\ThirdAss.dll" "$reaperUserPluginsDir\ThirdAss.dll" -Force
        Write-Host "  -> Installed to REAPER UserPlugins: $reaperUserPluginsDir\ThirdAss.dll" -ForegroundColor Green

        # Configure reaper.ini vstpath64 if needed
        $reaperIni = "$env:APPDATA\REAPER\reaper.ini"
        if (Test-Path $reaperIni) {
            $iniContent = Get-Content $reaperIni -Raw
            if ($iniContent -match "vstpath64=(.*)") {
                $curPaths = $matches[1]
                if ($curPaths -notlike "*$vstGlobalDir*") {
                    $newPaths = "$vstGlobalDir;$curPaths"
                    $iniContent = $iniContent -replace "vstpath64=.*", "vstpath64=$newPaths"
                    Set-Content -Path $reaperIni -Value $iniContent
                    Write-Host "  -> Updated REAPER vstpath64 in $reaperIni" -ForegroundColor Green
                }
            }
        }

        # Update or register in REAPER cache reaper-vstplugins64.ini
        $reaperCacheIni = "$env:APPDATA\REAPER\reaper-vstplugins64.ini"
        $fileTimeHex = [System.IO.File]::GetLastWriteTimeUtc("$vstGlobalDir\ThirdAss.dll").ToFileTime().ToString("X16")
        $cacheEntry = "ThirdAss.dll=$fileTimeHex,859919187,Third Ass (myavuzuzun)!!!VSTi"
        if (Test-Path $reaperCacheIni) {
            $cacheLines = Get-Content $reaperCacheIni
            $found = $false
            $newLines = @()
            foreach ($line in $cacheLines) {
                if ($line -like "ThirdAss.dll=*") {
                    $newLines += $cacheEntry
                    $found = $true
                } else {
                    $newLines += $line
                }
            }
            if (-not $found) {
                $newLines += $cacheEntry
            }
            Set-Content -Path $reaperCacheIni -Value $newLines
            Write-Host "  -> Pre-registered cache in REAPER: $cacheEntry" -ForegroundColor Green
        }
    }
}

Write-Host "Build and installation complete!" -ForegroundColor Cyan
