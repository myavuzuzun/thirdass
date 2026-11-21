CXX ?= g++
CXXFLAGS ?= -O3 -std=c++17 -Wall -Wextra -Iinclude -fno-math-errno -fno-trapping-math -fno-rtti -fno-exceptions -ffunction-sections -fdata-sections -flto -s
WIN_LIBS = -ld2d1 -ldwrite -lole32 -lgdi32 -luser32
TARGET_DLL = dist/ThirdAss.dll
TARGET_VST3 = dist/ThirdAss.vst3
TARGET_GUI = bin/thirdass_gui.exe

LMMS_DIR = $(USERPROFILE)/Documents/lmms/plugins/vst
REAPER_USER_DIR = $(APPDATA)/REAPER/UserPlugins
VST_DIR = $(USERPROFILE)/Documents/VstPlugins
VST3_USER_DIR = $(LOCALAPPDATA)/Programs/Common/VST3

all: plugin vst3 gui test

plugin: $(TARGET_DLL)

$(TARGET_DLL): src/vestige/ThirdAssPlugin.cpp src/vestige/ThirdAss.def src/dsp/ThirdAssDSP.hpp src/dsp/Biquad.hpp src/gui/WinD2DUI.hpp include/vestige/vestige.h
	@if not exist dist mkdir dist
	$(CXX) $(CXXFLAGS) -static -shared -Wl,--gc-sections src/vestige/ThirdAssPlugin.cpp src/vestige/ThirdAss.def $(WIN_LIBS) -o $(TARGET_DLL)

vst3: $(TARGET_VST3)

$(TARGET_VST3): src/vst3/ThirdAssVST3.cpp src/vst3/ThirdAssVST3.def src/vst3/ThirdAssVST3Editor.hpp src/dsp/ThirdAssDSP.hpp src/gui/WinD2DUI.hpp include/vst3/vst3.h include/vst3/vst3_plugin.h
	@if not exist dist mkdir dist
	$(CXX) $(CXXFLAGS) -Iinclude/vst3 -Wno-unused-parameter -Wno-unused-private-field -static -shared -Wl,--gc-sections src/vst3/ThirdAssVST3.cpp src/vst3/ThirdAssVST3.def $(WIN_LIBS) -o $(TARGET_VST3)

gui: $(TARGET_GUI)

$(TARGET_GUI): src/standalone/gui_main.cpp src/dsp/ThirdAssDSP.hpp src/gui/WinD2DUI.hpp
	@if not exist bin mkdir bin
	$(CXX) $(CXXFLAGS) -static -Wl,--gc-sections src/standalone/gui_main.cpp $(WIN_LIBS) -o $(TARGET_GUI)

test: plugin vst3
	@powershell -ExecutionPolicy Bypass -File .\test_vst.ps1
	@$(CXX) $(CXXFLAGS) -Iinclude/vst3 -Wno-unused-parameter -Wno-unused-private-field src/vst3/test_vst3.cpp -lgdi32 -luser32 -o bin/test_vst3.exe
	@.\bin\test_vst3.exe

install: plugin vst3
	@powershell -ExecutionPolicy Bypass -File .\build.ps1 -Install

clean:
	@if exist dist rmdir /s /q dist
	@if exist bin rmdir /s /q bin

.PHONY: all plugin vst3 gui test install clean
