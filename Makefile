ifeq ($(OS),Windows_NT)
CXX ?= g++
CXXFLAGS ?= -O3 -std=c++17 -Wall -Wextra -Iinclude -fno-math-errno -fno-trapping-math -fno-rtti -fno-exceptions -ffunction-sections -fdata-sections -flto -s
WIN_LIBS = -ld2d1 -ldwrite -lole32 -lgdi32 -luser32
TARGET_DLL = dist/ThirdAss.dll
TARGET_GUI = bin/thirdbass_gui.exe
LMMS_DIR = $(USERPROFILE)/Documents/lmms/plugins/vst
REAPER_USER_DIR = $(APPDATA)/REAPER/UserPlugins
VST_DIR = $(USERPROFILE)/Documents/VstPlugins

all: vst2 gui test

vst2: $(TARGET_DLL)

$(TARGET_DLL): src/vst2/ThirdBassVST2.cpp src/vst2/ThirdAss.def src/dsp/ThirdBassDSP.hpp src/dsp/Biquad.hpp src/gui/WinD2DUI.hpp
	@if not exist dist mkdir dist
	$(CXX) $(CXXFLAGS) -static -shared -Wl,--gc-sections src/vst2/ThirdBassVST2.cpp src/vst2/ThirdAss.def $(WIN_LIBS) -o $(TARGET_DLL)

gui: $(TARGET_GUI)

$(TARGET_GUI): src/standalone/gui_main.cpp src/dsp/ThirdBassDSP.hpp src/gui/WinD2DUI.hpp
	@if not exist bin mkdir bin
	$(CXX) $(CXXFLAGS) -static -Wl,--gc-sections src/standalone/gui_main.cpp $(WIN_LIBS) -o $(TARGET_GUI)

test: vst2
	@powershell -ExecutionPolicy Bypass -File .\test_vst.ps1

install: vst2
	@powershell -ExecutionPolicy Bypass -Command "if (-not (Test-Path '$(LMMS_DIR)')) { New-Item -ItemType Directory -Path '$(LMMS_DIR)' -Force | Out-Null }; if (-not (Test-Path '$(REAPER_USER_DIR)')) { New-Item -ItemType Directory -Path '$(REAPER_USER_DIR)' -Force | Out-Null }; if (-not (Test-Path '$(VST_DIR)')) { New-Item -ItemType Directory -Path '$(VST_DIR)' -Force | Out-Null }; Copy-Item '$(TARGET_DLL)' '$(LMMS_DIR)/ThirdAss.dll' -Force; Copy-Item '$(TARGET_DLL)' '$(REAPER_USER_DIR)/ThirdAss.dll' -Force; Copy-Item '$(TARGET_DLL)' '$(VST_DIR)/ThirdAss.dll' -Force; Write-Host 'Installed to LMMS, REAPER UserPlugins, and Documents/VstPlugins!'"

clean:
	rm -rf dist bin build

.PHONY: all vst2 gui test install clean

else
CXX ?= g++
CXXFLAGS ?= -O3 -fPIC -std=c++17 -Wall -Wextra -Iinclude -fvisibility=hidden -pthread -fno-math-errno -fno-trapping-math
CAIRO_LIB = /usr/lib/x86_64-linux-gnu/libcairo.so.2
UI_LIBS = $(CAIRO_LIB) -lX11 -pthread

DIST_DIR = dist
BUILD_DIR = build
BIN_DIR = bin

all: test vst2

test: $(BIN_DIR)/thirdbass_test
	@echo "Running DSP Test Suite..."
	@$(BIN_DIR)/thirdbass_test

$(BIN_DIR)/thirdbass_test: src/standalone/main.cpp src/dsp/ThirdBassDSP.hpp src/dsp/Biquad.hpp
	@mkdir -p $(BIN_DIR)
	$(CXX) -O3 -std=c++17 -Iinclude -fno-math-errno -fno-trapping-math src/standalone/main.cpp -o $(BIN_DIR)/thirdbass_test

vst2: $(DIST_DIR)/ThirdAss.so

$(DIST_DIR)/ThirdAss.so: src/vst2/ThirdBassVST2.cpp src/dsp/ThirdBassDSP.hpp src/dsp/Biquad.hpp src/gui/CairoUI.hpp
	@mkdir -p $(DIST_DIR)
	$(CXX) $(CXXFLAGS) -shared -Wl,-soname,ThirdAss.so src/vst2/ThirdBassVST2.cpp $(UI_LIBS) -o $(DIST_DIR)/ThirdAss.so

install: vst2
	@echo "Installing Third Ass VST2 plugin for REAPER and LMMS..."
	@mkdir -p ~/.vst /home/yavuz/Documents/lmms/plugins/vst
	cp $(DIST_DIR)/ThirdAss.so ~/.vst/
	cp $(DIST_DIR)/ThirdAss.so /home/yavuz/Documents/lmms/plugins/vst/
	@echo "Installation complete!"
	@echo "  -> REAPER VST2: ~/.vst/ThirdAss.so"
	@echo "  -> LMMS VST2:   /home/yavuz/Documents/lmms/plugins/vst/ThirdAss.so"

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR) $(BIN_DIR)

.PHONY: all test vst2 install clean
endif
