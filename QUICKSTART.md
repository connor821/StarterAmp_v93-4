# Starter Amp v92: Quick Start

Ten minutes from install to a good tone. The full details are in `MANUAL.md`.

## 1. Build and open it (once)

```
xcode-select --install
brew install cmake
cmake -B build -G Xcode
cmake --build build --config Release
```

Open `build/StarterAmpV92_artefacts/Release/Standalone/Starter Amp v92.app`. The VST3 and AU versions are in the same `StarterAmpV92_artefacts/Release` folder (`VST3` and `AU`). The build does not install them for you: copy the `.vst3` to `~/Library/Audio/Plug-Ins/VST3` and the `.component` to `~/Library/Audio/Plug-Ins/Components`, then rescan plug-ins in your DAW. If the build prints errors, copy them as they are and send them to me.

**Before you play:** use headphones and keep the OUTPUT level low while you turn up the gain. The standalone app asks for your audio interface and microphone permission the first time.

## 2. Set your input

1. Open the app's audio settings (the Options button in the standalone window) and choose your interface and a small buffer (64 or 128 samples).
2. On the **INPUT** tab, **INPUT & GATE** panel, set the input menu: **Left** or **Right** for a guitar on one input, **Mono** to sum both.
3. Play. Raise **Gate** (the knob in the INPUT & GATE panel) until the noise between notes disappears, no further. The default (-65 dB) suits most guitars.

## 3. Pick a sound in 30 seconds

- Press **BROWSE** (top bar), choose the **Factory** tab and pick a preset, or use the `<` and `>` buttons to step through them. Presets are grouped by style (Djent and Modern Rhythm, Progressive Metal, Groove and Sludge, Melodic and Extreme, Classic and Low Gain, Clean and Ambient, Pitch and Octave).
- Presets are starting points. Change anything and press **SAVE** to keep your version under your own name.

## 4. Build a sound from scratch (the four things that matter)

Open the **AMP** tab.

1. **Pick the amp** (the top menu of the AMP panel). Fourteen electric amps plus an acoustic model. Quick guide:
   - Clean tones: **Clean**, **Glass Clean**, **Vintage Chime**
   - Crunch and classic rock: **Classic Rock**, **Low-Watt Combo**, **Boutique Lead**
   - Modern high gain: **Precision High-Gain**, **Tight Metal**, **Creamy Lead**, **Modern High-Gain**
   - Extreme styles (thall, industrial, doom, slam): **Digital Extreme**, **Doom Stack**
2. **Pick the voicing** (the second menu, "Voicing: ..."). Every amp has three. For the two new amps: *Precision High-Gain*: Tight, Lead, Crunch. *Digital Extreme*: Thall, Industrial, Cavern.
3. **Set Gain.** 0.4 to 0.6 is the sweet spot for most rhythm playing. Turn **AUTO LEVEL** on so changing amps or gain does not change the loudness.
4. **Shape the tone** with Bass, Low Mid, High Mid, Treble and Presence. 0 dB is each amp's own neutral point, not flat.

On a low-tuned guitar (drop D or lower, 7 or 8 strings) turn **Focus** up about 1 per semitone below E standard. It tightens the low end so the chugs stay clear. Modern high-gain amps also show a **MODERN METAL** panel: **Tight** (palm-mute clarity), **Sag**, **Chug Gate** and **Lead Boost**, with **Depth**, **Mid Freq**, **Mid Boost** and **Fizz Tamer** behind its ADV button.

## 5. Add a cab (the amp is not finished without one)

Open the **CAB** tab. **Block A** is on by default with a built-in cab model (speaker, mic, position, distance). For a thicker sound switch **Block B** on, pick a different mic, then press **ALIGN NOW**. For a more studio-like cab, try the speakers named Studio with the Flat mic, then raise **Box** a little and move **Tight** and **Bite**. You can also load your own impulse response (WAV or AIFF) with the Source menu on any block.

## 6. Add space

Open the **DELAY** and **POST FX** tabs: a delay and a reverb are ready. Raise the **Mix** knob on each to taste. A little reverb (0.1 to 0.2) makes a dry guitar sound more finished.

## 7. Everyday tools in the top bar

| Button | What it does |
|---|---|
| `<` `>` | Previous and next preset |
| BROWSE / SAVE | Open the preset browser (with search and tags) / save your sound |
| A, B, A/B | Two snapshots of your sound to compare; A/B swaps between them |
| UNDO / REDO | Step back and forward through your changes (also Cmd+Z) |
| Mode menu | Normal, Low Latency (no oversampling, least delay) or High Quality (4x) |
| TUNER | A chromatic tuner. The output is muted while it is open. |
| METRO | A metronome |
| MIDI | Foot controller and expression pedal mapping; right-click any knob for MIDI learn |
| ORDER | Rearrange the effects before the amp and after the cab |
| TAP / tempo box / HOST TEMPO | Tempo for the delays, tremolo and other synced effects. HOST TEMPO follows your DAW. |
| RIFF | Record up to 60 s of your playing and replay it through any tone you set |

## 8. Five starting points

| You want | Amp / voicing | Gain | Also do |
|---|---|---|---|
| Tight modern rhythm | Precision High-Gain / Tight | 0.5 | Tight 0.3, Gate -56 dB, Focus 2 for drop tunings, Cab: B on, ALIGN NOW |
| Singing lead | Precision High-Gain / Lead | 0.6 | Post-amp delay at a dotted-eighth note value, mix about 0.2; reverb mix about 0.14 |
| Thall chug | Digital Extreme / Thall | 0.6 | Focus 4, Tight 0.4, Chug Gate 0.4, Gate -52 dB |
| Industrial grind | Digital Extreme / Industrial | 0.55 | Mid Boost 0.3 (MODERN METAL, ADV), cab High Cut about 8 kHz to tame the fizz |
| Doom slam | Digital Extreme / Cavern, or Doom Stack / Wall | 0.55 | Focus 6, big reverb (0.2), Gate -50 dB |

## 9. If something is not right

| Problem | Try |
|---|---|
| No sound | Check the input menu (Left / Right / Mono), the audio settings, and that TUNER is closed |
| Too quiet or too loud | AUTO LEVEL on, then the OUTPUT panel's Level |
| Fizzy or harsh | Lower Gain; bring the cab's High Cut down to 6 to 8 kHz; lower Presence; Fizz Tamer (MODERN METAL panel, behind ADV) |
| Flubby low end | Raise Focus; raise Tight; try a lower Bass offset; check the cab's Low Cut (80 to 100 Hz) |
| Notes cut off | Lower the Gate threshold |
| Hum | PICKUP DE-NOISE on (match 50 or 60 Hz to your mains) |
| Clicks or crackles | Raise the buffer size; try Mode: Low Latency |
| Controls greyed out | They do nothing in the current setup; hover to see why |

Hover over any control for a description. Double-click a knob to reset it. Click the number under a knob to type a value.

## Finding your way around
- The ten tabs run in the order the sound travels: INPUT, GAIN, MOD, PITCH, AMP, CAB, EQ, DELAY, SCAPES, POST FX.
- **TIDY VIEW** (top right of the tabs) shrinks any pedal that is switched off into a small tile. Click **SWITCH ON** on a tile to use it. Turn TIDY VIEW off to see everything.
- **HELP** opens a short guide to the window.
- To change the order of effects, drag a block in the strip under the tabs.

## Matching a sound from a recording
On the **EQ** tab press **TONE LAB...**, load a clean recording of your guitar (DI) and a recording of the sound you want, then press **START MATCH**. When it finishes, press **APPLY TO MY SOUND** (and **UNDO** if you prefer the old one).
