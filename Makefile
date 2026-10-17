CXX ?= g++
CXXFLAGS ?= -O3 -fPIC -std=c++17 -Wall -Wextra -Iinclude -fvisibility=hidden
CAIRO_LIB = /usr/lib/x86_64-linux-gnu/libcairo.so.2
UI_LIBS = $(CAIRO_LIB) -lX11

DIST_DIR = dist
BUILD_DIR = build
BIN_DIR = bin

all: test vst2 lv2 clap

test: $(BIN_DIR)/thirdbass_test
	@echo "Running DSP Test Suite..."
	@$(BIN_DIR)/thirdbass_test

$(BIN_DIR)/thirdbass_test: src/standalone/main.cpp src/dsp/ThirdBassDSP.hpp src/dsp/Biquad.hpp
	@mkdir -p $(BIN_DIR)
	$(CXX) -O3 -std=c++17 -Iinclude src/standalone/main.cpp -o $(BIN_DIR)/thirdbass_test

vst2: $(DIST_DIR)/ThirdBass.so

$(DIST_DIR)/ThirdBass.so: src/vst2/ThirdBassVST2.cpp src/dsp/ThirdBassDSP.hpp src/dsp/Biquad.hpp src/gui/CairoUI.hpp
	@mkdir -p $(DIST_DIR)
	$(CXX) $(CXXFLAGS) -shared -Wl,-soname,ThirdBass.so src/vst2/ThirdBassVST2.cpp $(UI_LIBS) -o $(DIST_DIR)/ThirdBass.so

lv2: $(DIST_DIR)/ThirdBass.lv2/ThirdBass.so $(DIST_DIR)/ThirdBass.lv2/ThirdBass_ui.so

$(DIST_DIR)/ThirdBass.lv2/ThirdBass.so: src/lv2/ThirdBassLV2.cpp src/dsp/ThirdBassDSP.hpp src/dsp/Biquad.hpp src/lv2/manifest.ttl src/lv2/ThirdBass.ttl
	@mkdir -p $(DIST_DIR)/ThirdBass.lv2
	$(CXX) $(CXXFLAGS) -shared -Wl,-soname,ThirdBass.so src/lv2/ThirdBassLV2.cpp -o $(DIST_DIR)/ThirdBass.lv2/ThirdBass.so
	cp src/lv2/manifest.ttl $(DIST_DIR)/ThirdBass.lv2/
	cp src/lv2/ThirdBass.ttl $(DIST_DIR)/ThirdBass.lv2/

$(DIST_DIR)/ThirdBass.lv2/ThirdBass_ui.so: src/lv2/ThirdBassUI_LV2.cpp src/dsp/ThirdBassDSP.hpp src/dsp/Biquad.hpp src/gui/CairoUI.hpp
	@mkdir -p $(DIST_DIR)/ThirdBass.lv2
	$(CXX) $(CXXFLAGS) -shared -Wl,-soname,ThirdBass_ui.so src/lv2/ThirdBassUI_LV2.cpp $(UI_LIBS) -o $(DIST_DIR)/ThirdBass.lv2/ThirdBass_ui.so

clap: $(DIST_DIR)/ThirdBass.clap

$(DIST_DIR)/ThirdBass.clap: src/clap/ThirdBassCLAP.cpp src/dsp/ThirdBassDSP.hpp src/dsp/Biquad.hpp src/gui/CairoUI.hpp
	@mkdir -p $(DIST_DIR)
	$(CXX) $(CXXFLAGS) -shared -Wl,-soname,ThirdBass.clap src/clap/ThirdBassCLAP.cpp $(UI_LIBS) -o $(DIST_DIR)/ThirdBass.clap

install: all
	@echo "Installing Third Bass plugins for REAPER and LMMS..."
	@mkdir -p ~/.vst ~/.clap ~/.lv2 /home/yavuz/Documents/lmms/plugins/vst
	cp $(DIST_DIR)/ThirdBass.so ~/.vst/
	cp $(DIST_DIR)/ThirdBass.so /home/yavuz/Documents/lmms/plugins/vst/
	rm -rf ~/.lv2/ThirdBass.lv2
	cp -r $(DIST_DIR)/ThirdBass.lv2 ~/.lv2/
	cp $(DIST_DIR)/ThirdBass.clap ~/.clap/
	@echo "Installation complete!"
	@echo "  -> VST2: ~/.vst/ThirdBass.so"
	@echo "  -> LMMS VST: /home/yavuz/Documents/lmms/plugins/vst/ThirdBass.so"
	@echo "  -> LV2:  ~/.lv2/ThirdBass.lv2/ (DSP & UI)"
	@echo "  -> CLAP: ~/.clap/ThirdBass.clap"

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR) $(BIN_DIR)

.PHONY: all test vst2 lv2 clap install clean
