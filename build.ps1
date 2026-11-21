# Build and Test Script for Third Ass (Windows x64 VeSTige & VST3 Architecture)
param(
    [switch]$Install = $false
)

$ErrorActionPreference = "Stop"

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host " Building Third Ass (Windows x64 VeSTige & VST3)  " -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# Ensure output directories exist
if (-not (Test-Path "dist")) { New-Item -ItemType Directory -Path "dist" | Out-Null }
if (-not (Test-Path "bin")) { New-Item -ItemType Directory -Path "bin" | Out-Null }

# 1. Compile 64-bit Dynamic Link Library via Open-Source VeSTige Architecture
Write-Host "[1/4] Compiling dist/ThirdAss.dll (Direct2D GPU GUI + 64-bit Audio DSP)..." -ForegroundColor Yellow
g++ -O3 -std=c++17 -Wall -Wextra -Iinclude -fno-math-errno -fno-trapping-math -static -shared -fno-rtti -fno-exceptions -ffunction-sections -fdata-sections -flto "-Wl,--gc-sections" -s src/vestige/ThirdAssPlugin.cpp src/vestige/ThirdAss.def -ld2d1 -ldwrite -lole32 -lgdi32 -luser32 -o dist/ThirdAss.dll

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile dist/ThirdAss.dll"
    exit 1
}
$dllSize = (Get-Item "dist\ThirdAss.dll").Length
Write-Host "      -> dist/ThirdAss.dll built successfully! ($([math]::Round($dllSize/1KB, 1)) KB)" -ForegroundColor Green

# 2. Compile VST3 Plugin (Bundle & Binary for Modern DAWs)
Write-Host "[2/4] Compiling dist/ThirdAss.vst3 (Modern VST3 + Direct2D GUI)..." -ForegroundColor Yellow
g++ -O3 -std=c++17 -Wall -Wextra -Iinclude -Iinclude/vst3 -Wno-unused-parameter -Wno-unused-private-field -fno-math-errno -fno-trapping-math -static -shared -fno-rtti -fno-exceptions -ffunction-sections -fdata-sections -flto "-Wl,--gc-sections" -s src/vst3/ThirdAssVST3.cpp src/vst3/ThirdAssVST3.def -ld2d1 -ldwrite -lole32 -lgdi32 -luser32 -o dist/ThirdAss_raw.vst3

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile VST3 plugin"
    exit 1
}

# Create standard VST3 bundle structure
if (Test-Path "dist\ThirdAss.vst3" -PathType Leaf) {
    Remove-Item "dist\ThirdAss.vst3" -Force
}
$vst3BundleDir = "dist\ThirdAss.vst3\Contents\x86_64-win"
if (-not (Test-Path $vst3BundleDir)) {
    New-Item -ItemType Directory -Path $vst3BundleDir -Force | Out-Null
}
Copy-Item "dist\ThirdAss_raw.vst3" "$vst3BundleDir\ThirdAss.vst3" -Force
Copy-Item "dist\ThirdAss_raw.vst3" "dist\ThirdAss_standalone.vst3" -Force
Remove-Item "dist\ThirdAss_raw.vst3" -Force -ErrorAction SilentlyContinue

$vst3Size = (Get-Item "$vst3BundleDir\ThirdAss.vst3").Length
Write-Host "      -> dist/ThirdAss.vst3 (Bundle) built successfully! ($([math]::Round($vst3Size/1KB, 1)) KB)" -ForegroundColor Green

# 3. Compile Standalone GUI Preview Executable
Write-Host "[3/4] Compiling bin/thirdass_gui.exe (Standalone Direct2D preview)..." -ForegroundColor Yellow
g++ -O3 -std=c++17 -Wall -Wextra -Iinclude -fno-math-errno -fno-trapping-math -static -fno-rtti -fno-exceptions -ffunction-sections -fdata-sections -flto "-Wl,--gc-sections" -s src/standalone/gui_main.cpp -ld2d1 -ldwrite -lole32 -lgdi32 -luser32 -o bin/thirdass_gui.exe

if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to compile bin/thirdass_gui.exe"
    exit 1
}
$exeSize = (Get-Item "bin\thirdass_gui.exe").Length
Write-Host "      -> bin/thirdass_gui.exe built successfully! ($([math]::Round($exeSize/1KB, 1)) KB)" -ForegroundColor Green

# 4. Compile and Run Verification Test Suites
Write-Host "[4/4] Running plugin verification test suites..." -ForegroundColor Yellow

# VeSTige verification
powershell -ExecutionPolicy Bypass -File .\test_vst.ps1
if ($LASTEXITCODE -ne 0) {
    Write-Error "VeSTige test suite failed!"
    exit 1
}

# VST3 verification
g++ -O3 -std=c++17 -Wall -Wextra -Iinclude -Iinclude/vst3 -Wno-unused-parameter -Wno-unused-private-field src/vst3/test_vst3.cpp -lgdi32 -luser32 -o bin/test_vst3.exe
if ($LASTEXITCODE -eq 0) {
    .\bin\test_vst3.exe
    if ($LASTEXITCODE -ne 0) {
        Write-Error "VST3 verification suite failed!"
        exit 1
    }
}

# Optional installation
if ($Install -or $true) {
    Write-Host "==================================================" -ForegroundColor Cyan
    Write-Host " Installing Plugins for LMMS, REAPER & Modern DAWs" -ForegroundColor Cyan
    Write-Host "==================================================" -ForegroundColor Cyan

    # 1. LMMS VeSTige
    $lmmsVstDir = "$env:USERPROFILE\Documents\lmms\plugins\vst"
    if (-not (Test-Path $lmmsVstDir)) {
        New-Item -ItemType Directory -Path $lmmsVstDir -Force | Out-Null
    }
    try {
        Copy-Item "dist\ThirdAss.dll" "$lmmsVstDir\ThirdAss.dll" -Force
    } catch {
        Move-Item "$lmmsVstDir\ThirdAss.dll" "$lmmsVstDir\ThirdAss.dll.old" -Force -ErrorAction SilentlyContinue
        Copy-Item "dist\ThirdAss.dll" "$lmmsVstDir\ThirdAss.dll" -Force
    }
    Write-Host "  -> Installed to LMMS VeSTige: $lmmsVstDir\ThirdAss.dll" -ForegroundColor Green

    # LMMS 1-click preset
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
        <elcut lspd_denominator="4" lspd_syncmode="0" ctlenvamt="0" lshp="0" rel="0.1" lspd_numerator="4" lamt="0" sustain="0.5" lpdel="0" lshp="0" rel="0.1" lspd_numerator="4" lamt="0" sustain="0.5" lpdel="0"/>
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

    # 2. VST3 Standard Locations
    # Common Files VST3
    $systemVst3Dir = "$env:CommonProgramFiles\VST3"
    $systemVst3Bundle = "$systemVst3Dir\ThirdAss.vst3\Contents\x86_64-win"
    try {
        if (-not (Test-Path $systemVst3Bundle)) {
            New-Item -ItemType Directory -Path $systemVst3Bundle -Force | Out-Null
        }
        Copy-Item "$vst3BundleDir\ThirdAss.vst3" "$systemVst3Bundle\ThirdAss.vst3" -Force
        Copy-Item "$vst3BundleDir\ThirdAss.vst3" "$systemVst3Dir\ThirdAss.vst3" -Force
        Write-Host "  -> Installed VST3 to System: $systemVst3Dir\ThirdAss.vst3" -ForegroundColor Green
    } catch {
        Write-Host "  (System Common Files VST3 requires admin; falling back to User VST3)" -ForegroundColor Yellow
    }

    # User-level VST3 (Scanned by all modern DAWs: Cubase, REAPER, Studio One, FL Studio, Ableton)
    $userVst3Dir = "$env:LOCALAPPDATA\Programs\Common\VST3"
    $userVst3Bundle = "$userVst3Dir\ThirdAss.vst3\Contents\x86_64-win"
    if (-not (Test-Path $userVst3Bundle)) {
        New-Item -ItemType Directory -Path $userVst3Bundle -Force | Out-Null
    }
    Copy-Item "$vst3BundleDir\ThirdAss.vst3" "$userVst3Bundle\ThirdAss.vst3" -Force
    Copy-Item "$vst3BundleDir\ThirdAss.vst3" "$userVst3Dir\ThirdAss.vst3" -Force
    Write-Host "  -> Installed VST3 to User Common: $userVst3Dir\ThirdAss.vst3" -ForegroundColor Green

    # 3. Global Documents\VstPlugins
    $vstGlobalDir = "$env:USERPROFILE\Documents\VstPlugins"
    if (-not (Test-Path $vstGlobalDir)) {
        New-Item -ItemType Directory -Path $vstGlobalDir -Force | Out-Null
    }
    Copy-Item "dist\ThirdAss.dll" "$vstGlobalDir\ThirdAss.dll" -Force
    Copy-Item "$vst3BundleDir\ThirdAss.vst3" "$vstGlobalDir\ThirdAss.vst3" -Force
    Write-Host "  -> Installed DLL & VST3 to: $vstGlobalDir" -ForegroundColor Green

    # 4. REAPER UserPlugins
    $reaperUserPluginsDir = "$env:APPDATA\REAPER\UserPlugins"
    if (Test-Path "$env:APPDATA\REAPER") {
        if (-not (Test-Path $reaperUserPluginsDir)) {
            New-Item -ItemType Directory -Path $reaperUserPluginsDir -Force | Out-Null
        }
        Copy-Item "dist\ThirdAss.dll" "$reaperUserPluginsDir\ThirdAss.dll" -Force
        Copy-Item "$vst3BundleDir\ThirdAss.vst3" "$reaperUserPluginsDir\ThirdAss.vst3" -Force
        Write-Host "  -> Installed to REAPER UserPlugins: $reaperUserPluginsDir\ThirdAss.dll & ThirdAss.vst3" -ForegroundColor Green

        # Ensure vstpath64 in reaper.ini includes our paths
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

        # Update cache in reaper-vstplugins64.ini
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

Write-Host "Build, packaging, and installation complete!" -ForegroundColor Cyan
