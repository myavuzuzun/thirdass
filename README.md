# Third Bass (Linux Native VST / LV2 / CLAP)

A high-performance, native 64-bit Linux bass amplifier simulator and multi-effects processor designed as a modern replacement for the legacy Windows freeware plugin **3rd Bass** (by Fretted Synth Audio / Joseph DeHelian).

Built specifically for **REAPER** and **LMMS** on Linux without requiring Wine, bridges, or external dependencies.

## Graphical User Interface (GUI)

Third Bass features a custom, high-definition double-buffered hardware rack interface (980x560) rendered natively via Cairo / X11:

- **Contextual Signal-Flow Grouping**: Modules are grouped strictly by studio audio signal chain (Input/Gate -> Preamp & Drive -> 4-Band EQ on top row; Sub Resurrection -> Opto Compressor -> Crossover Chorus -> Stereo Delay -> Cabinet & Master on bottom row).
- **Dynamic 4-Band EQ Visualizer**: Recessed graphical spectrum analyzer showing real-time composite frequency curves across 20 Hz – 20 kHz with draggable frequency node markers.
- **Tube Filament Glow & Visualizers**: 12AX7 vacuum tube visualizer glows dynamically with Drive level; Crossover Chorus wave and Stereo Delay decay representations.
- **Real-Time LED Metering**:
  - Live Noise Gate status LED (`OPEN` vs `GATE`).
  - Opto Compressor Gain Reduction LED meter (`GR dB` ladder from 0 to -18 dB).
  - Stereo Master Output Peak VU meter with clip indication.
- **Studio Hardware Controls**:
  - **Vertical Drag**: Smooth adjustment across parameter ranges.
  - **Shift + Drag**: 10x fine-tuning precision.
  - **Mouse Wheel Scroll**: Turn any knob by scrolling up or down.
  - **Double-Click**: Instantly reset any knob to its factory default value.
  - **Preset Browser**: `<` and `>` quick navigation through all 12 factory presets.
  - **Bypass / Toggle Buttons**: Instant ON/OFF toggle per module with glowing illuminated states.

---

## Features & Signal Chain

Third Bass is tailored specifically to make electric bass guitars, synth basses, and low-end tracks punch through a mix with weight, grit, and clarity:

1. **Input Stage & Noise Gate**
   - Clean adjustable threshold (`-90 dB` to `0 dB`) and exponential release (`10 ms` to `500 ms`) to silence high-gain amplifier buzz, single-coil hum, and idle line noise.
2. **Preamp Drive & Saturation**
   - **Warm Tube**: Asymmetrical triode-style soft saturation with tube warmth and DC blocking.
   - **Overdrive**: Pushed symmetrical solid-state/tube crunch.
   - **Fuzz / Heavy Grit**: Aggressive clipping for modern distorted bass riffs.
   - **Tone / Pre-tilt Control**: Tightens the low-end contour before saturation.
   - **Drive Mix**: Blend parallel clean bass underneath the drive so you never lose the fundamental low-end punch.
3. **4-Band Classic Bass Tone Stack (EQ)**
   - **Bass (80 Hz)**: Low shelving filter (+/- 15 dB) for foundational weight.
   - **Low Mid (350 Hz)**: Peaking bell filter (+/- 15 dB) for body and warmth.
   - **High Mid (1.8 kHz)**: Peaking bell filter (+/- 15 dB) for finger pluck, pick attack, and growl.
   - **Treble (4.5 kHz)**: High shelving filter (+/- 15 dB) for string zing and modern clarity.
4. **Resurrection Psychoacoustic Sub-Harmonic Enhancer**
   - Signature sub-bass generator inspired by Christian Budde's Resurrection DSP.
   - Tunable fundamental tracking frequency (`40 Hz` to `120 Hz`).
   - Generates non-linear 2nd & 3rd virtual pitch harmonics ($2x^2-1$ and $4x^3-3x$) that fool the human ear into hearing deep sub-bass on small monitors, laptops, and headphones.
   - Sub Boost control adds pure sub-octave rumble.
5. **Bass Dynamics (Opto / Soft-Knee Compressor)**
   - Threshold (`-40 dB` to `0 dB`), Ratio (`1:1` to `12:1`), Attack (`1 ms` to `100 ms`), Release (`20 ms` to `1000 ms`), and Makeup Gain (`0 dB` to `24 dB`).
   - Smooth 4 dB soft-knee characteristic to even out dynamic slap spikes and fingerstyle variations.
6. **Bass Crossover Stereo Chorus**
   - Integrated Linkwitz-Riley 2nd-order crossover filter (default `180 Hz`).
   - **Sub-lows stay 100% mono, tight, and unmodulated** in the center (avoiding phase cancellation in the low end).
   - Low-mids, mids, and highs receive lush dual quadrature stereo LFO modulation (Left 0°, Right 90°).
7. **Stereo Analog / Tape Delay**
   - Up to 1000 ms stereo delay line with ping-pong stereo offset.
   - 1-pole high-cut damping filter (`1 kHz` to `16 kHz`) for vintage warm repeats.
8. **Speaker Cabinet Simulation**
   - **0: Direct / Bypass**: Clean studio DI sound.
   - **1: Classic 8x10 SVT Fridge**: Standard punchy Ampeg-style rock cabinet with 2.4 kHz upper-mid punch and 4.5 kHz speaker roll-off.
   - **2: Vintage 1x15 Portaflex**: Deep, round vintage Motown / reggae warmth with smooth rolled-off highs.
   - **3: Modern 4x10 Punch**: Fast transient response, scooped 500 Hz boxiness, and crisp horn extension up to 6.2 kHz.
   - **4: Aggressive 2x12 Rock**: Cutting 1.1 kHz - 2.6 kHz mid bite for heavy rock and metal pick bass.
9. **Master & Brickwall Peak Limiter**
   - Zero-latency soft-knee peak limiter ceiling at `-0.1 dBFS` prevents digital clipping.
   - Master volume and total Dry/Wet mix.

---

## 12 Factory Presets

1. **01: Clean Studio DI** - Pristine clean studio DI with gentle opto compression.
2. **02: Vintage Tube Warmth** - Warm tube preamp drive through a 1x15 Portaflex cabinet.
3. **03: 8x10 Rock Growl** - Punchy SVT 8x10 rock growl with mid bite.
4. **04: Funky Slap & Pop** - Scooped mids, boosted 80 Hz bass and 4.5 kHz treble with subtle chorus.
5. **05: Sub Heavy Dub** - Deep sub-harmonic resurrection with 1x15 low-end rumble.
6. **06: Motown 1x15 Soul** - Round, warm vintage bass with rolled-off treble.
7. **07: Modern Punch & Snap** - High-definition modern bass tone for versatile pop/rock mixes.
8. **08: Gritty Overdrive** - Saturated pushed overdrive with parallel dry blend.
9. **09: Fuzz Bass Monster** - Heavy fuzz saturation through 2x12 rock cab.
10. **10: 80s Chorus Bass** - Lush stereo chorus with clean mono sub-bass.
11. **11: Ambient Space Bass** - Atmospheric chorus and tape delay wash.
12. **12: Tight Metal Pick** - Fast noise gate, aggressive 2x12 cabinet, and cutting high-mid attack.

---

## Installation & Formats

This project builds 3 native Linux formats:
* **VST2 (`ThirdBass.so`)**: Installed to `~/.vst/` and `/home/yavuz/Documents/lmms/plugins/vst/` (compatible with LMMS VeSTige & Carla and REAPER).
* **LV2 (`ThirdBass.lv2`)**: Installed to `~/.lv2/` (recognized by REAPER and LMMS).
* **CLAP (`ThirdBass.clap`)**: Installed to `~/.clap/` (recognized by modern REAPER).

### Building and Installing

To compile and install:
```bash
make install
```

To run the verification test suite:
```bash
make test
```

---

## Using in REAPER

1. Open REAPER.
2. Open the FX window on any bass audio or virtual instrument track (click `FX`).
3. If REAPER hasn't scanned yet, press `F5` in the FX browser or go to `Options -> Preferences -> Plug-ins -> VST` and click `Re-scan`.
4. Type `Third Bass` in the search box.
5. Add `VST: Third Bass (Antigravity Audio)`, `LV2: Third Bass`, or `CLAP: Third Bass`.
6. Select any of the 12 factory presets from the preset dropdown, or tweak any of the 37 parameters!

---

## Using in LMMS

### Method 1: In the FX Mixer (Effect Processor)
1. Open the **FX Mixer** (`F8`).
2. Select your Bass channel.
3. Click **Add effect**.
4. In the effect search dialog, locate **Third Bass** (either via native VST or LV2 / Carla).
5. Load the plugin and dial in your tone.

### Method 2: As an Instrument / Audio Processor (VeSTige)
1. Drag **VeSTige** from the Instrument Plugins panel into the Song-Editor.
2. In the VeSTige instrument window, click the green folder icon.
3. Open `ThirdBass.so` from your VST folder: `/home/yavuz/Documents/lmms/plugins/vst/ThirdBass.so`.

---

## License

ISC License (Open-source, permissive).
Designed & built by Antigravity.
