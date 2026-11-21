CXX ?= g++
CXXFLAGS ?= -O3 -std=c++17 -Wall -Wextra -Iinclude -fno-math-errno -fno-trapping-math -fno-rtti -fno-exceptions -ffunction-sections -fdata-sections -flto -s
WIN_LIBS = -ld2d1 -ldwrite -lole32 -lgdi32 -luser32
TARGET_DLL = dist/ThirdAss.dll
TARGET_GUI = bin/thirdass_gui.exe
LMMS_DIR = $(USERPROFILE)/Documents/lmms/plugins/vst
REAPER_USER_DIR = $(APPDATA)/REAPER/UserPlugins
VST_DIR = $(USERPROFILE)/Documents/VstPlugins

all: plugin gui test

plugin: $(TARGET_DLL)

$(TARGET_DLL): src/vestige/ThirdAssPlugin.cpp src/vestige/ThirdAss.def src/dsp/ThirdAssDSP.hpp src/dsp/Biquad.hpp src/gui/WinD2DUI.hpp include/vestige/vestige.h
	@if not exist dist mkdir dist
	$(CXX) $(CXXFLAGS) -static -shared -Wl,--gc-sections src/vestige/ThirdAssPlugin.cpp src/vestige/ThirdAss.def $(WIN_LIBS) -o $(TARGET_DLL)

gui: $(TARGET_GUI)

$(TARGET_GUI): src/standalone/gui_main.cpp src/dsp/ThirdAssDSP.hpp src/gui/WinD2DUI.hpp
	@if not exist bin mkdir bin
	$(CXX) $(CXXFLAGS) -static -Wl,--gc-sections src/standalone/gui_main.cpp $(WIN_LIBS) -o $(TARGET_GUI)

test: plugin
	@powershell -ExecutionPolicy Bypass -File .\test_vst.ps1

install: plugin
	@powershell -ExecutionPolicy Bypass -Command "if (-not (Test-Path '$(LMMS_DIR)')) { New-Item -ItemType Directory -Path '$(LMMS_DIR)' -Force | Out-Null }; if (-not (Test-Path '$(REAPER_USER_DIR)')) { New-Item -ItemType Directory -Path '$(REAPER_USER_DIR)' -Force | Out-Null }; if (-not (Test-Path '$(VST_DIR)')) { New-Item -ItemType Directory -Path '$(VST_DIR)' -Force | Out-Null }; Copy-Item '$(TARGET_DLL)' '$(LMMS_DIR)/ThirdAss.dll' -Force; Copy-Item '$(TARGET_DLL)' '$(REAPER_USER_DIR)/ThirdAss.dll' -Force; Copy-Item '$(TARGET_DLL)' '$(VST_DIR)/ThirdAss.dll' -Force; Write-Host 'Installed to LMMS, REAPER UserPlugins, and Documents/VstPlugins!'"

clean:
	@if exist dist rmdir /s /q dist
	@if exist bin rmdir /s /q bin

.PHONY: all plugin gui test install clean
