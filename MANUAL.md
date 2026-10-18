# Starter Amp v93: User Manual

Starter Amp is a guitar amp simulator and effects rack built from hand-written DSP (no neural captures needed, although it can also load neural amp models). It runs as a standalone app, a VST3 and an AU plug-in. This manual covers every part of it. If you only want to get playing, read `QUICKSTART.md` first.

**An honest note up front.** Every amp, pedal and cab here is my own approximation built from the general reputation of each kind of sound. None of it is a circuit model, a capture or a copy of any particular product, and the names say "-style" or describe a character for that reason. I have tested it with measurements and a test harness, but I have not run it or listened to it on a Mac. Treat every setting in this manual as a starting point and trust your ears. Where the manual says "measured", it means a number from a test signal, not a listening result.

---

## Contents

1. Installing and building
2. First launch and audio setup
3. A tour of the window
4. Signal flow
5. The pages, one by one
   - INPUT
   - GAIN
   - MOD
   - PITCH
   - AMP
   - CAB
   - EQ
   - DELAY
   - SCAPES
   - POST FX
   - Reordering effects
6. The amps in depth (all fifteen, with all four voicings)
7. Presets, A/B, undo and the browser
8. MIDI and controllers
9. Tuner, metronome, riff recorder and tone matching
10. Gain staging and levels (how Auto Level works)
11. Recipes by style
12. Quality, CPU and latency
13. Troubleshooting
14. Known limits
15. Appendix A: amp and voicing table
16. Appendix B: control reference (every control, from the plug-in's own hover help)

---

## 1. Installing and building

You need a Mac with Xcode's command-line tools, CMake and an internet connection (the first configure downloads the JUCE framework, version 8.0.4).

```
xcode-select --install
brew install cmake
cmake -B build -G Xcode
cmake --build build --config Release
```

The products are in `build/StarterAmpV72_artefacts/Release/`:

| Format | Where | Install by |
|---|---|---|
| Standalone | `Standalone/Starter Amp v93.app` | Double-click, or drag to Applications |
| VST3 | `VST3/Starter Amp v93.vst3` | Copy to `~/Library/Audio/Plug-Ins/VST3` |
| AU | `AU/Starter Amp v93.component` | Copy to `~/Library/Audio/Plug-Ins/Components` |

The build does not copy the plug-ins for you. After copying, rescan plug-ins in your DAW. A self-built, unsigned plug-in may be blocked the first time: right-click it in Finder and choose Open, or allow it in System Settings, Privacy and Security.

If the build prints errors, copy them exactly and send them to me.

**Where your files live.** Presets are files named `Name.starteramp` in `Documents/StarterAmp/Presets`. Copy them to share or back them up. Impulse responses, neural models and tone-match references are remembered by location, so keep them in a stable folder.

## 2. First launch and audio setup

1. Use headphones, and keep the OUTPUT level low while you raise the gain knobs.
2. **Standalone:** open the Options button at the top left of the window, choose your audio interface, set the buffer to 64 or 128 samples, and allow microphone access when macOS asks (it is the audio input permission).
3. **Plug-in:** put it on a guitar track with the input from your interface. A mono track is enough.
4. In the plug-in, choose the **input channel** (INPUT tab, INPUT & GATE panel): Left, Right or Mono (L + R). The result always goes to both sides.
5. Set the **Gate** just high enough to silence the noise between notes. With a noisy single-coil guitar, switch on **PICKUP DE-NOISE**, choose 60 Hz (US) or 50 Hz (EU) to match your mains, and start with Hum about 0.7 and Hiss about 0.4.

Resize the window by dragging its bottom-right corner. It scales in proportion.

## 3. A tour of the window

### The top bar

| Control | What it does |
|---|---|
| Logo and version | The version label is for reference when you report a problem |
| `<` `>` and the preset menu | Step through factory presets, then your own; the menu lists them by style |
| BROWSE | The preset browser: Factory and User tabs, a search box (names and tags) and a tag filter |
| SAVE | Saves the whole sound as a preset. It asks for a name and tags. Saving an existing name replaces it |
| A, B, A/B | Two snapshots for comparing. A/B swaps them, and the level is matched so the louder one does not win |
| UNDO, REDO | Step through your changes (Cmd+Z works too) |
| Mode menu | **Normal** (each quality menu as you set it), **Low Latency** (no oversampling, least delay) or **High Quality** (4x everywhere) |
| TUNER | A chromatic tuner (A = 440 Hz) over the page. The output is muted while it is open |
| METRO | A metronome with its own panel |
| MIDI | Controller mapping and the MIDI window (see section 8) |
| ORDER | Rearrange the effects before the amp and after the cab (see section 5, Reordering effects) |
| TAP, tempo box, HOST TEMPO | The tempo that synced delays, tremolo, modulation and other time-based settings follow. HOST TEMPO follows your DAW's tempo |
| RIFF | The riff recorder (section 9) |

### The page tabs and the signal-flow strip

Ten page tabs run under the top bar: INPUT, GAIN, MOD, PITCH, AMP, CAB, EQ, DELAY, SCAPES and POST FX, in signal order. Next to them are two buttons: **TIDY VIEW** (switched-off pedals collapse into small tiles with a SWITCH ON button; turn it off to see every pedal in full, faded when off) and **HELP** (a plain-language guide to the window). The strip under them shows the whole chain from IN to OUT. A lit dot means that effect is on. Click a block's name to jump to its page, or click its dot to switch the effect on or off. **Drag a block left or right to change the order of the effects**; the strip and the ORDER window always agree.

### The look of the panels

The window is styled like a pedalboard and an amp stack:

- **Pedals** (fuzz, compressor, overdrive, wah, phaser, chorus, delays and so on) are coloured enclosures with a title band, a small line-art illustration, a screw in each top corner and a faint watermark of the same art. The colour and art are decoration only. They help you find a pedal at a glance.
- **Amp panels** take on the look of the amp you choose: tolex, piping, faceplate and grille cloth change with the model. The AMP page also shows a large illustration of the selected amp head on a four-speaker cab, which follows the model (and Amp B's model on the Amp B tab).
- **Rack panels** (input, EQ, output and so on) are plain dark cards.
- The accent colour of knobs and highlights follows the amp you are playing.

### Controls

- **Knobs:** drag up and down (or sideways) to change them. **Double-click** to reset to the default. **Click the number** under a knob to type a value.
- **Hover** over any control for a description. The hover help also names the current menu choice and explains why a control is greyed out.
- **Greyed-out controls** do nothing in the current setup (for example, a delay's tap level with Taps off). The control still works, and the hover help tells you what to switch on first.
- **ON switches:** every effect has one. A lit (coloured) switch means on. A panel whose switch is off fades, but its controls still work so you can dial it in first. Switching a delay or reverb off fades its mix out so it does not click.
- **ADV button** (top right of many panels): Basic / Advanced. With ADV off, only the main controls show. With it on, everything shows. Hidden controls keep their values and still work. The setting is remembered with the session.
- **Right-click a knob** for MIDI learn and clearing a mapping.

## 4. Signal flow

```
IN -> input select -> pickup de-noise -> gate -> high-pass -> transpose
   -> noise pedal -> [pre-amp group, reorderable: fuzz, compressor, wah, phaser, chorus, pre-amp delay]
   -> (Mod FX slots set to "before the amp") -> lo-fi warble -> boost (with its EQ)
   -> NAM pedal 1 -> overdrive -> AMP A (or its neural model) [+ AMP B in parallel]
   -> tone stack -> cab blocks A to D -> NAM pedal 2 -> (Mod FX slots set to "after the cab")
   -> [post group, reorderable: Post EQ, textures, doubler, post-amp delay, reverb, widener]
   -> output level -> optional limiter -> OUT
```

Fixed stages (the ORDER window cannot move them): noise pedal, transpose, pitch pedal, octaver, lo-fi warble, boost, NAM pedals, overdrive, the amps and the cab. "Textures" in the ORDER window means tremolo, wobble, the swell fade and the granular cloud together.

## 5. The pages, one by one

The ten pages are in signal order. Each section says what the page is for and how to use it. Appendix B lists every knob, switch and menu with its hover text. On every page, a pedal that is switched off collapses to a small tile while **TIDY VIEW** is on (see section 3).

### INPUT
Getting a clean, quiet signal into the rest of the chain.

**INPUT & GATE.** Input menu (Left, Right, Mono), input level, **Gate** threshold in dB, and the pickup de-noise (hum notches at the mains frequency and its harmonics, plus a low-pass that closes in the gaps between notes to hide hiss). De-noise sits before the gate, so hum no longer holds the gate open and you can set the gate lower without chatter. If the notes sound thin, lower Hum; if tails sound dull, lower Hiss.

**TRANSPOSE / FORMANT.** A pitch shifter at the very start of the chain, so everything after it (fuzz, amp) plays the shifted notes. **Fast** (about 14 ms) or **Smooth** (about 27 ms, better for low strings and down-shifts). Formant Lock shifts the formant the opposite way so the tone stays natural. Choose 6, 7 or 8-string tuning in the string-range menu and the shifters follow the lower strings. It reports its delay to your DAW. Best on single notes.

### GAIN
Everything that pushes or squashes the signal before the amp, left to right in the order they run: fuzz, compressor, overdrive, boost. Use only the ones you need.

**FUZZ.** One streamlined pedal with several styles:
- **Wall** is the thick, sustained wall of sound; **Tight** (op-amp) is mid-forward and aggressive; **Buzzsaw** is scooped with a sawing buzz; **Face** is round and cleans up with your guitar's volume; **Bender** is raspy and fuzzy; **Factory** is the velcro-and-gating fuzz that squeals and sputters. **Nuke** is a wave-shaper fuzz: Gain drives the signal through a curve that **Shape** morphs between five families, **Crush** adds bit and sample-rate reduction, and Tone is a resonant output filter. **Crusher** is a bit-crusher with drive in front: **Bits**, **Rate**, Gain and Tone. (An older Gated style is kept only so old sessions load the same; it is greyed out in the menu.)
- Always visible: ON, style, Auto Level, the Op-Amp switch and a version menu where the style has one, and the main knobs (Gain, Tone, Level, Mix, Mids).
- Behind ADV: germanium and silicon choices, bias starve, input and compression controls, drift, hiss, and the buffer and tone-pass switches, depending on the style.

**COMPRESSOR.** Pick a **Style** (Standard, FET, Opto, Variable-mu, Sustainer or Bus; see the README for what each does) and a matching **Saturation** (behind ADV: FET stage, Opto tube stage, Mu iron + tube or Diode, plus the older Tube-like, Tape-like, Both and Transformer). Threshold, ratio (1:1 up to Limit, including a soft-knee 10:1), attack, release, output and mix, with a gain-reduction readout and an Auto Level that restores the level the compression takes away. Behind ADV: a sidechain high-pass (80, 160 or 320 Hz) and saturation (tube-like, tape-like, both or transformer).

**OVERDRIVE.** A versatile overdrive in front of the amp. Styles: **Transparent**, **Tight**, **Open** (high or low range), **Screamer** (the classic mid-hump style) and **Distortion** (hard clipping) and **Tube Driver** (v64: a full-range, warm, creamy tube-style drive that keeps much of the clean signal at low Drive and turns thick and compressed when pushed; knobs Drive, Bass and Treble, with Bright behind ADV). Each shares Gain, Tone, Edge and Volume, with extra controls behind ADV (clipping diodes: silicon, LED, germanium, asymmetric; voicing: Classic, Mild, Open; bass and mid controls on the Screamer; a mid switch on the Distortion). **Auto Level** keeps the loudness the same as with the pedal off, so Volume becomes a trim around that level. The overdrive feeds both amps, and Amp B has a switch to choose whether it uses it. **Circuit mode (v77, Tight and Screamer):** a switch that replaces the hand-tuned model with the real circuit, solved from its published resistor, capacitor, diode and op-amp values. The Drive range stays as wide as before (it now reaches lower and higher than the stock pedal), and Bass, Mid, Mid Freq, Edge, Attack, Bright and Tone keep their ranges, with their default positions acting as the stock circuit. Clipping gets a fifth choice, **None (op-amp rails)**, where the op-amp itself runs out of swing instead of diodes; Tight uses the Clipping menu in this mode too. It is **off by default**, so old sessions sound exactly as before. Auto Level has its own measured table for the circuit modes. It is based on schematics, not on listening tests against hardware.

#### Boost and boost EQ
**BOOST** is a clean-to-saturated boost in front of the overdrive and the amp, with a **Tighten** high-pass and five saturation modes. **BOOST EQ** is a four-band parametric EQ that runs with it, handy for shaping what hits the amp (a low cut for tightness, a mid push for lead).

### MOD
Sweeps, wobbles and choruses.

**WAH** (manual, auto, or LFO sweep; MIDI expression pedal CC 11 moves the position by default), **PHASER** (with more options than the classic four-stage one), **CHORUS** (bucket-brigade style.

Two identical slots (**A** and **B**). Each can be a flanger, through-zero flanger, vibrato, Uni-V style, rotary, auto-pan or harmonic tremolo. Each slot chooses whether it runs **before the amp** or **after the cab**.

### PITCH
**PITCH PEDAL** (an expression pedal: heel and toe pitch with modes such as octave up, two octaves, octave down and a custom mode; the MIDI pitch wheel moves it by default), **OCTAVER**, and the **HARMONIZER** with Interval or Scale modes and three tracking choices: Single note (fast, for leads), Chords (follows up to six notes, about 85 ms of delay) and Chords fast (about 43 ms).

### AMP
The AMP page has three tabs: **AMP A**, **AMP B** (the parallel second amp) and **NAM** (neural model slots). On the right is the amp illustration.

**AMP A panel, always visible:**
- **Model menu** (fourteen electric amps plus Acoustic). Section 6 describes each one.
- **Voicing menu:** four positions per amp, named for the amp ("Voicing: Thall").
- **AUTO LEVEL:** holds this amp's loudness the same for every Gain setting, every amp and every voicing.
- **BRIGHT:** a treble lift that fades out as Gain rises, like a vintage bright switch.
- **Tone stack** (As voiced, Neutral, or one of seven passive stacks), **Feel** (Loose, Normal, Tight power-amp feedback) and **Mode** (Classic, Circuit-derived, Full circuit; see Amp mode below).
- **Knobs:** Gain, Bass, Low Mid, High Mid, Treble, Presence, and Focus on the amps that use it.

**Behind ADV:** the preamp tube (12AX7, 12AT7, 12AY7), the power tube (As voiced, EL34, 6L6, 6V6, EL84, KT88), DEEP (a low-end boost), the amp's oversampling quality (Best 4x, Balanced 2x, Lowest latency), and the knobs Bias, Power Drive, Resonance and Sag.

**Focus** tightens the low end and trims bass and gain so low-tuned guitars stay clear. Turn it up by about 1 for each semitone below E standard. It works on Saturated Lead, Modern High-Gain, Tight Metal, Creamy Lead, Precision High-Gain and Digital Extreme. (Digital Extreme is already very tight, so Focus tightens it about half as strongly as the others.)

**MODERN METAL panel** (it appears under the amp for Hot-Rod, Saturated Lead, Modern High-Gain, Tight Metal, Creamy Lead, Precision High-Gain and Digital Extreme):
- **Tight:** raises the high-passes between the gain stages (up to about 2.8x) so low palm mutes stay clear and fast.
- **Sag:** power-amp squash and bloom. The middle is the amp's own, 0 is stiff, 1 is double.
- **Chug Gate:** a fast gate after the preamp that cuts the gain's hiss between palm mutes. Too high cuts quiet notes.
- **LEAD BOOST:** more gain in the first stages, a little more mid push and sag, level-trimmed to match, for solos.
- Behind ADV: **Depth** (a low shelf below about 90 Hz), **Mid Freq**, **Mid Boost** and **Fizz Tamer** (rolls off the top from 12 kHz to about 3.8 kHz after the preamp).

**AMP B** has the same controls for a second amp that runs in parallel on the same signal, with its own faceplate, **A Level** and **B Level** knobs and an **AMP B ON** switch. A common use is a tight high-gain amp with a cleaner amp underneath. Each cab block chooses Amp A, Amp B or both. Amp B shares Focus and the overdrive with Amp A.

**NAM tab:** four slots (Amp A, Amp B, Pedal 1, Pedal 2), each with its own LOAD MODEL and CLEAR. A loaded neural model replaces that amp's preamp and power stage. The session remembers the file's location, not a copy.

**ACOUSTIC / PIEZO** (when the model is Acoustic): body shape (Dreadnought, Jumbo, Parlor/OM, Nylon), body amount, piezo fix (removes the nasal quack and glassy edge), preamp warmth, squeeze compression, sparkle and warmth. It replaces the amp and cab.

### CAB
The CAB page has two sub-tabs along its top: **CAB BLOCKS** (the four blocks, the mic position view, the CAB panel and the IR alignment display) and **SPEAKER SHAPING & EXPORT** (per-block shaping, speaker dynamics, export as an impulse response).

Four cab blocks (**A** to **D**), a shared **CAB** panel and the **IR ALIGNMENT** display. Only block A is on at first.

- Each block has an **ON** switch and a **Source** menu: a built-in model, or an impulse-response file (WAV or AIFF). IRs are normalised when loaded.
- **Built-in model:** choose a speaker (Modern Tight, Vintage Crunch, Rock Bite, Smooth Vintage, Chimey Alnico, Vintage American, Hi-Fi Full), a mic (Dynamic Bright, Dynamic Full, Dynamic Presence, Ribbon Smooth, Condenser Detailed, Flat), a cabinet (16 types in 1x, 2x, 4x and Oversized folders, from 1x8 Small to 4x12 Closed), and the mic's Position (centre to edge), Distance (0 to 12 inches, with proximity effect) and Angle. Since v67 there are also fitted "Studio", "Heavy", "Rhythm", "Compact" and "Classic" speakers (best with the Flat mic and the 4x12 Closed cabinet) and three knobs per block: **Tight** (low end), **Bite** (2.4 and 3.8 kHz) and **Box** (adds the narrow resonances of a real cabinet; 0 = smooth, as before).
- Per block: Level, Pan, Delay (up to 10 ms), INVERT, Low Cut and High Cut, and which amp it takes (A, B or both).
- **IR LIBRARY**: a window for folders of IR files. ADD FOLDER, pick a file to see its curve and a comparison with the closest built-in speaker, tick AUDITION to hear it in place of your cab blocks, and SEND TO A to D to load it. HIDE DUPLICATES hides files within 1 dB of an earlier one. Favourites and hidden files are remembered.
- **REALISTIC** (CAB page): when on, Position and Distance change the sound by the amount real recordings show, which is gentler than before. Older sessions open with it off.
- **Seven more mics and three more speakers** are in the Mic menu's Measured folder and the Speaker menu's Classic and Raw (bold) folders. Raw Big Low and Raw Bright Cap are deliberately bold.
- **Ten knobs per block**: Position, Distance, Angle, Tight, Bite, Box, Level, Pan, **Delay** (manual alignment, 0 to 10 ms) and **Tilt** (-12 to +12 dB, brighter or darker). Low Cut and High Cut are on the block's advanced page.
- **Alignment:** with two or more blocks on, AUTO ALIGN measures how late each mic is and whether it is upside down, and writes Delay and INVERT for you. **ALIGN NOW** repeats it. The display shows the first 6 ms of each block and the combined frequency response, where a comb-filtered mix shows deep narrow notches.
- **CAB panel:** CAB ON, AUTO ALIGN, ALIGN NOW, Room, Spread (stereo width from mono IRs), Low Cut, High Cut, Tilt, Reso and Reso Hz (cabinet thump), and Air.

Tip: two blocks on the same speaker with different mics (a dynamic close, a ribbon a few inches back), aligned with ALIGN NOW, give a fuller sound than either alone. Use IRs you have the right to use.

#### Speaker shaping and export
**BLOCK SHAPING** (a per-block EQ, resonance and phase), **SPEAKER DYNAMICS** (the speaker's compression and break-up behaviour, plus, **Tame** and **Motion**) and **EXPORT CAB AS IR** (writes the current cab as an impulse-response file you can use elsewhere).


SPEAKER DYNAMICS has a mode menu and three knobs:
- **Mode:** Tight (modern) and Vintage (spongy) are the original behaviours. **Real: tight** and **Real: vintage** add a cone that stiffens (its bass resonance rises and fades as it moves), a top that fades while the lows work hard, and a mid breakup that leaves the lows alone. Amount scales all of it.
- **Amount:** how much the speaker changes with level. 0 is off.
- **Tame:** turns down resonances (fizz, honk, boxiness) only while they stick out of the surrounding spectrum, after the cab, so it works on built-in cabs and loaded impulse responses. 0 is off. Start around 0.3 to 0.5. Behind ADV, **Listen to cuts** plays what Tame removes, so you can check it is taking out only what you want.
- **Motion:** slow random movement in the cab's tone, different left and right and bigger the harder you play, so the cab does not sound frozen. 0 is off. Behind ADV, **Rate** sets how fast it moves.

Tip: for a loaded impulse response that sounds a little harsh on some notes, try Tame 0.4 first; for a stiff, static sound, add a little Motion (0.2 to 0.4) and a Real mode at Amount 0.3 to 0.5.

### EQ
**TONE LAB...** matches your whole sound to a recording. Load a clean DI of your guitar and a recording of the sound you want, press START MATCH (about a minute), look at the before and after curves, then press APPLY TO MY SOUND (UNDO reverts). Tick or untick what it may change: other amps, cab speakers and mics, compressor/overdrive/boost, and the final Post EQ fit. See the README (Tone Lab) for what it measures and its limits.

**POST EQ:** low cut, nine bands and high cut, with a live curve. Drag a fader; double-click to return a band to 0 dB. The **SPECTRUM** panel shows what you are hearing (pre-EQ or output, or both). **MATCH...** (tone matching): load a reference recording (the first 90 seconds are measured), press LISTEN, play for about ten seconds, then APPLY TO EQ. The plug-in fits the nine bands by least squares so your tone moves toward the reference; Strength scales the correction and RESET EQ flattens the faders.

### DELAY
**PRE-AMP DELAY** (a mono delay with ducking, modulation, diffusion, hold and taps, placed before the amp for ambient and rhythmic textures). **POST-AMP DELAY** (note values and tap tempo, Forward or Reverse, ping-pong and dual modes, ducking, modulation, diffusion, hold and taps). The two delays are independent. The pre-amp delay runs before the overdrive and amp; the post-amp delay runs near the end of the chain. The hover help on each Mode menu describes Digital, Analog, Tape and Lo-Fi.

### SCAPES
Shoegaze, ambient and special-effect tools.

Shoegaze and ambient tools: **GRANULAR CLOUD** (with scale, grain shape, scan and tone), **TREMOLO / AUTO-PAN**, **WOBBLE** (tremolo-arm style vibrato) and **SWELL** (a volume fade-in that turns a pick attack into a violin-like swell).

**NOISE PEDAL.** A pitch-mangling pedal for harsh, dissonant sounds: octave up, two octaves up, Clash (clashing pitch-shifted voices) and a ring modulator. The four footswitches are momentary by default (on only while held); light **LATCH** to make one latching. NOISE PEDAL ON is a master switch. The tabs (OCTAVE, CLASH, RING MOD) only choose which settings you see.

**LO-FI WARBLE.** A worn, wobbly tape-style pedal: pitch warble (Rate, Depth, shape), Drift, Tone, Dirt (soft clipping plus bit and sample-rate crushing), Noise, Dropouts, Mix and Level. Set Depth, Drift, Dirt and Noise to 0 for a nearly transparent pass-through.

### POST FX
**POST-AMP DELAY** (note values and tap tempo, Forward or Reverse, ping-pong and dual modes, ducking, modulation, diffusion, hold and taps), **REVERB** (several types, with a **Shimmer** mode and its own controls), **REVERB SHAPING**, **DOUBLER** (double-tracking), **WIDENER** and **OUTPUT** (level, a limiter with ceiling and release, input and output peak meters with clip lights, and a MONO check switch).


### Reordering effects
Drag a block in the signal-flow strip under the tabs to move it; blocks move within their own group. The **ORDER** window (header) does the same from a list: select an effect and press MOVE UP or MOVE DOWN (or drag a row); RESET ORDER restores the default. Two groups can be reordered: before the amp (fuzz, compressor, wah, phaser, chorus, pre-amp delay) and after the cab (Post EQ, textures, doubler, post-amp delay, reverb, widener).

## 6. The amps in depth

All fourteen electric amps share the same machinery: cascaded clipping stages, each preceded by a high-pass (tighter lows when the cutoffs are higher), a pre-gain mid push, a power-amp stage with sag, a presence control, and a four-band tone stack. They differ in their numbers. The model menu lists them in the order below, and each has three voicings.

### Reading the amp list

- **Gain** is how hard the stages are driven. On the high-gain amps, the top of the Gain knob (above about 55%) adds extra gain for saturation, sustain and compression.
- **Tone knobs** add to each amp's built-in tone shape. 0 dB is the amp's neutral setting, not flat. The American, British and Neutral tone stack choices change how much of that built-in shape is used.
- **Auto Level** is calibrated per amp and per voicing (section 10).

### The newest amp: Master-Volume Rock

An 80s hard-rock amp built around a master volume: a cascaded preamp that is tight, bitey and compressed. It has no Modern Metal controls or Focus.
- **Stock:** the classic sound: three gain stages, a tight low end, a forward upper mid and a bright, edgy top. It is exactly the sound that was Classic Rock's third voicing before the Master-Volume Rock amp existed.
- **Modded:** the same kind of amp with a modification for more gain: an extra cascaded stage, a tighter low end, a cut in the lower mids and a hotter upper mid. It is smoother, more compressed and more saturated, with a lot of sustain.
- **Cranked:** the master volume wide open. The preamp is gentler (two stages) and the power stage does the clipping, so it is looser, saggier, darker and louder.

Sessions and presets saved with Classic Rock on its third voicing load as this amp's Stock voicing, so they sound the same.

### Precision High-Gain and Digital Extreme

**Precision High-Gain (tight, modern).** A modern boutique high-gain amp: very tight and articulate, a firm low end, a scooped low mid, a focused upper mid and a smooth, detailed top. It has the Modern Metal panel and Focus. It is built to stay clear under fast palm-muted rhythm and to sing on leads, in the spirit of modern boutique high-gain heads and the tones of contemporary progressive players.
- **Tight:** the main high-gain channel. Three stages, tight lows, a scooped low mid, a 1.8 kHz bite and moderate sag so held notes bloom a little.
- **Lead:** more gain and sustain, a vocal mid hump near 1.3 kHz, more sag and a rounder top so single notes sing.
- **Crunch:** an open, dynamic, lower-gain channel (two stages) that cleans up with your guitar's volume knob. It compresses much less than the other two (measured: about 5 dB of gain compression against about 10 dB).

**Digital Extreme (brutal, digital).** A modern digital-style amp for extreme styles: thall and djent, deathcore and metalcore, industrial and doom. It has very high gain, almost no sag, hard and symmetrical clipping, and extreme tightness. It has the Modern Metal panel and Focus.
- **Thall:** ultra-tight (the highest high-passes of any amp), a firm sub-low, a deep cut at about 350 Hz and a sharp 2.4 kHz edge. Chugs stay pinned and clear at low tunings. It is the most upper-mid-forward of the three (measured).
- **Industrial:** a harsh, gritty, wide-open voicing with fuzzier clipping, some even harmonics, strong upper mids and more top-end fizz, for aggressive, abrasive riffs, synth-like leads and sound-design guitar. Pair it with the cab's High Cut and Fizz Tamer to control the top.
- **Cavern:** a huge, slow, dark wall with giant lows, a smooth rolled-off top and a lot of sag and headroom behind the clipping. Built for 7 and 8-string, drop-tuned doom and slam. Raise Focus as you tune down.

### The fourth voicings

| Amp | 4th voicing | Character |
|---|---|---|
| Clean | Studio | Hi-fi: more headroom, smoother low mid, brighter top, less sag |
| Vintage Chime | Jangle | Brighter, more open, less compression |
| British Hot-Rod | Mid push | Forward nasal upper mid, less bass |
| Saturated Lead | Sustain | More gain and sag, rolled-off top |
| Modern High-Gain | Chunk | Tighter, firm lows, forward upper mid |
| Classic Rock | Bright | More gain and presence, a little tighter |
| Boutique Lead | Blues | Earlier breakup, forward upper mids, more sag |
| Tight Metal | Buzz | Tighter, more gain, scooped low mid, edgy top |
| Low-Watt Combo | Gritty | Cranked, dark, saggy breakup |
| Glass Clean | Shimmer | Very bright, scooped and glassy |
| Creamy Lead | Singing | Smoother, rounder, vocal |
| Doom Stack | Fuzz | Fuzzy, dark, very saturated, more bias |
| Precision High-Gain | Polished | Changed in v77: starts from the open Crunch channel instead of Tight. Hi-fi, clear and dynamic, with more headroom, less sag and a bright, extended top; it cleans up as you play softer |
| Digital Extreme | Lead | The scoop filled in, a touch less tight |
| Master-Volume Rock | Boosted | Boosted preamp: tighter, more gain, forward upper mid |

Each starts from one of the amp's first three voicings and is level-matched to the amp's first voicing at Gain 0.5.

### Voicing and level notes

Each voicing has its own level trim and its own Auto Level row, measured so the amps sit at about the same loudness at the default Gain. Appendix A lists every amp and voicing.

### Amp-wide switches

| Switch | Choices | What it changes |
|---|---|---|
| Tone stack | As voiced, Neutral, passive stacks | Neutral: most of the amp's own tone shape removed. The passive stacks are solved from component values (see Passive tone stacks) |
| Feel | Loose, Normal, Tight | Power-amp feedback. Loose: more sag, fuller looser lows, less presence. Tight: less sag, tighter lows, more presence |
| DEEP | on / off | A low-end boost of about 2.5 dB into the tone stack and first stage |
| Preamp tube | 12AX7, 12AT7, 12AY7 | Gain and softness: 12AX7 is highest gain and crispest, 12AY7 is lowest and roundest |
| Power tube | As voiced, EL34, 6L6, 6V6, EL84, KT88 | EL34 crunchy mids; 6L6 tight with headroom; 6V6 soft with early breakup; EL84 bright and chimey; KT88 big and tight |

All switches start neutral, so they only change the sound when you move them. Each has a measured level compensation so it changes feel and tone, not loudness. The effects are subtle on clean amps and clearer as the amp is pushed.

## 7. Presets, A/B, undo and the browser

- A **preset** is the whole sound: every knob, switch and menu, the effect order, and the paths of loaded IRs and neural models. It does not include your MIDI mappings (a session keeps those). Input, quality, tempo, limiter and mono settings are kept when you load a factory preset.
- **Factory presets** are grouped by style and level-matched by measurement. Choosing one resets the sound to defaults first, so nothing from the previous tone is left behind. They are starting points and I have not listened to them. Factory presets cannot be deleted; edit one and SAVE it under a new name.
- **SAVE** asks for a name and tags. Saving a name that exists replaces it without a second prompt.
- **BROWSE** shows Factory and User tabs, a search box and a tag filter. In the User tab you can rename, re-tag and delete your presets. A damaged file shows in red and can only be deleted.
- **A/B:** edit a sound, press B, change something, then press A/B to compare. The level is matched so the louder version does not seem better.
- **UNDO / REDO** step through your changes, including preset loads.
- **Sessions:** your DAW saves the plug-in's state with the project. Sessions made with older versions load and sound the same, because new controls start neutral or off.

## 8. MIDI and controllers

- **Program Change n loads factory preset n,** counting from 0 in the order of the Presets menu.
- **CC 0 to 127 and pitch bend** can each move one parameter over its full range. Switches treat 64 and above as on. A mapping can be inverted.
- **Starting mappings:** CC 11 (expression pedal) moves the wah position; the pitch wheel moves the pitch pedal. DEFAULTS in the MIDI window restores them.
- **Learn:** right-click a knob, choose MIDI learn, and move a controller. For switches and menus, open the MIDI window, choose the parameter and press LEARN.
- **The MIDI window** has a channel filter (all by default), the mapping list with INVERT and REMOVE, and a line showing the last message received, which is the quickest way to check a controller is arriving.
- Mappings and the channel are saved with the session, not with presets. A parameter has one controller and a controller drives one parameter.
- Not available: MIDI output, bank select, note handling, a separate range per mapping. The plug-in does not expose its presets to the host as programs.

## 9. Tuner, metronome, riff recorder and tone matching

- **TUNER:** chromatic, A = 440 Hz. It mutes the output while open.
- **METRO:** a metronome that follows the tempo box.
- **RIFF:** records up to 60 seconds in stereo. **DI** records your dry guitar before the chain, so playback goes through whatever amp and effects you have now, which is the best way to audition tones hands-free. **Full tone** records the finished output. It can loop, export 24-bit WAV and import WAV. Riffs are not saved with the session.
- **Tone matching** is on the EQ page (see above).

## 10. Gain staging and levels

- **Auto Level (amp):** each amp and voicing has a measured table of its output level at each Gain setting. With AUTO LEVEL on, the plug-in adds the difference to hit a common target, so changing the gain changes the sound, not the loudness. The tables were measured with a plucked-note test signal and a flat amp (overdrive off, no cab), so they are accurate within roughly 1 to 2 dB. Your guitar, your playing, the overdrive, the compressor and the cab (IRs vary a lot in level) will shift it. Expect small differences between voicings and between amps at the same Gain.
- **Auto Level (overdrive, compressor, fuzz):** the same idea, so switching a pedal on does not make the whole chain louder.
- **Level trims:** every amp and voicing has a built-in output trim so the amps sit at about the same loudness at the default Gain. Away from the middle they drift apart on purpose, the way the amps behave: clean amps get louder as you turn the gain up, high-gain amps change by only a few dB.
- **OUTPUT panel:** Level, an optional limiter (Ceiling and Release) and peak meters with clip lights. Click a clip light to clear it. The limiter adds a short look-ahead only when it is on.
- **Watch the order.** A boost with Tighten before a high-gain amp is the quickest way to tighten and focus it. A noise gate set too high chops note tails; the Chug Gate on the MODERN METAL panel acts after the preamp and is better for killing gain hiss between palm mutes.

## 11. Recipes by style

These are starting points. Set the cab (Block A on, and B on with a different mic, then ALIGN NOW) before you judge any of them.

| Style | Amp / voicing | Settings to try |
|---|---|---|
| Progressive rhythm | Precision High-Gain / Tight | Gain 0.5, Tight 0.3, Focus to match your tuning, Gate about -56 dB, a short plate reverb |
| Progressive lead | Precision High-Gain / Lead | Gain 0.6, Lead boost off (the voicing already adds gain), post-amp delay at a dotted eighth, reverb 0.14 |
| Clean-ish crunch | Precision High-Gain / Crunch | Gain 0.4, roll your guitar's volume knob down to clean up |
| Djent and thall | Digital Extreme / Thall | Gain 0.6, Focus about 4, Tight 0.4, Chug Gate 0.4, Gate about -52 dB, very little reverb |
| Metalcore with synths | Digital Extreme / Industrial | Gain 0.55, Mid Boost about 0.3, cab High Cut 7 to 9 kHz, a pre-amp delay or Mod FX for movement |
| Industrial and aggressive | Digital Extreme / Industrial | Raise Gain, Fizz Tamer up a little, a Distortion-style overdrive in front for grit |
| Doom, slam and 8-string | Digital Extreme / Cavern | Gain 0.55, Focus 6, Tight low, big reverb, cab Block A on a 4x12 |
| Stoner and sludge | Doom Stack / Stoner | Gain 0.55, a fuzz in front (Wall style), Power Drive up |
| Classic rock | Classic Rock / Crunch, or Master-Volume Rock / Stock | Gain 0.5, a little room and spring-style reverb |
| Funk and clean | Glass Clean / Funk | Gain 0.4, compressor on, a little chorus |
| Ambient | Glass Clean / Warm, Granular and Shimmer on the SCAPES / POST FX pages | Low gain, long delay and reverb |
| Acoustic with a piezo | Acoustic | Body Dreadnought, Piezo Fix 0.6, a touch of Squeeze and room |

## 12. Quality, CPU and latency

- **Mode menu (top bar):** Normal, Low Latency (no oversampling, zero added delay in the amp), High Quality (4x oversampling for the amp and fuzz).
- **Amp Quality** (behind ADV on the amp panel): Best (4x), Balanced (2x, about half the CPU, the default) or Lowest latency. Oversampling reduces the fizz and aliasing of heavy distortion.
- **Latency:** Lowest latency is 0 samples, Balanced is a few samples, Best is a few more. Transpose, the harmonizer's chord modes and the limiter add their own delay, which is reported to your DAW.
- If you hear crackles, raise the buffer size first, then lower the quality. Turning off unused effects saves CPU because idle effects stop processing.

## 13. Troubleshooting

| Symptom | What to check |
|---|---|
| No sound | Input menu (Left, Right or Mono), audio settings, TUNER open (it mutes), CAB ON (with it off, only a basic low-pass is used), OUTPUT level |
| Hum or buzz | PICKUP DE-NOISE on, mains frequency set right, then the gate; turning toward or away from the source often changes hum |
| Hiss with high gain | Raise Gate a little; use Chug Gate; Fizz Tamer; lower Gain |
| Fizz | Lower Gain; cab High Cut 6 to 8 kHz; lower Presence; use 4x oversampling; try the Cavern or Crunch voicings |
| Flubby or boomy low end | Raise Focus and Tight; lower Bass; cab Low Cut about 80 to 100 Hz; Boost with Tighten before the amp |
| Notes cut off or sputter | Lower Gate; lower Chug Gate |
| Too quiet or too loud after changing amp | Turn AUTO LEVEL on |
| Click when switching amp or effect order | Normal: the gain stages or signal path change at a block boundary. Do it between notes |
| A control seems to do nothing | It may be greyed out: hover to see why. Some controls only apply to certain amps or styles |
| A saved session sounds different | New controls start neutral, so it should not. Tell me the version it was saved in |
| Build errors | Copy them exactly and send them to me |

## 14. Known limits

- Everything is a hand-built approximation. It is not a model of any specific product, and I have not run it or listened to it on a Mac.
- The built-in cab models are hand-made responses, not recordings of real speakers or mics. For a particular sound, use IRs you own or a neural capture.
- The pitch features work best on single notes. Low notes (below about 100 Hz) and down-shifts are rougher in the Fast mode; use Smooth.
- Auto Level is a measured match with a test signal, not a perception test.
- Not built: a built-in chorus in the Glass Clean amp (use the Chorus pedal), a graphic EQ in the Creamy Lead amp (use the Boost EQ or Post EQ), MIDI output, and non-WAV IRs.

---

## Appendix A: amp and voicing table

Voicing positions 1, 2 and 3 are the three entries of the Voicing menu. Positions 1 and 2 are the two voicings the switch has always had; position 3 arrived in v60 (and for the two newest amps, all three are new in v61). Descriptions are by character; none is a model of a specific product.

| # | Amp | Voicing 1 | Voicing 2 | Voicing 3 | Character (voicing 4 is listed in the next table) |
|---|---|---|---|---|---|
| 0 | Clean (sparkly) | Sparkly | Edge push | Boutique | Lots of headroom, scooped mids, sparkly top. Edge push is a small old amp turned up. Boutique has fuller mids and more compression |
| 1 | Vintage Chime (class-A) | Chime | Treble boost | Normal | Chimey, jangly class-A upper-mid honk, gritty when pushed. Normal is darker, rounder and thicker, without the top boost |
| 2 | British Hot-Rod (tight) | Tight | Heavy | Brown | Tight, bright, saturated, strong upper mids. Heavy has deep controlled lows and more gain. Brown is thicker, saggier and mid-rich |
| 3 | Saturated Lead (smooth) | Smooth | Lead | Crunch | Smooth, thick, singing lead with forward upper mids and lots of compression. Crunch is a low-gain, open channel |
| 4 | Modern High-Gain (scooped) | Scooped | Clarity | Vintage | Big lows, scooped mids, spongy sag. Clarity fills the scoop and tightens. Vintage is looser and fatter with a softer attack |
| 5 | Classic Rock (crunch) | Crunch | Pushed | Jumped | Open, mid-forward, loose and dynamic. Pushed is a hard-rock lead tone. Jumped is both channels linked: fatter, hotter, more sag, a softer top |
| 6 | Boutique Lead (smooth, singing) | Jazz | Rock | Lead | Clean headroom that turns into a smooth vocal lead when pushed. Lead is the boosted mode with more compression |
| 7 | Tight Metal (raw to modern) | Raw | Modern | Crunch | Tight, aggressive three-stage high gain. Modern has more gain, tighter lows and a smoother top. Crunch is a low-gain rhythm channel |
| 8 | Low-Watt Combo (spongy breakup) | Soft | Cranked | Four-ten | Small combo that breaks up early and soft with heavy sag. Cranked is spitty and thicker. Four-ten (changed in v77) is a clearer, fuller-bodied voicing: more headroom, bigger lows, a slightly scooped low mid, a brighter top and less sag |
| 9 | Glass Clean (solid-state) | Glass | Warm | Funk | Stiff solid-state clean with huge headroom and no sag. Warm is rounder and darker. Funk is tight, percussive and scooped |
| 10 | Creamy Lead (tight, mid-hump) | Rhythm | Lead | Vintage | Tight, creamy high gain with a mid hump. Rhythm is tight and punchy. Lead has a big mid hump and sustain. Vintage is lower gain with thick creamy mids |
| 11 | Doom Stack (thick, loose) | Stoner | Wall | Bass stack | Thick, loose and heavy with a huge low end. Stoner is fuzzy and saturated. Wall is huge, deep and slow. Bass stack has massive low mids and a grind only when pushed |
| 12 | Precision High-Gain (tight, modern) | Tight | Lead | Crunch | Modern boutique high gain: tight, articulate, clear. Lead (changed in v77) is a modern articulate lead: more gain, tighter and brighter than Tight, a focused upper-mid push near 2 kHz and little sag, so single notes stay defined rather than creamy. Crunch is open and dynamic |
| 13 | Digital Extreme (brutal, digital) | Thall | Industrial | Cavern | Brutal, very tight digital-style high gain with almost no sag. Industrial is harsh and fizzy. Cavern is huge, slow and dark |
| 14 | Master-Volume Rock (80s hard rock) | Stock | Modded | Cranked | An 80s master-volume hard-rock amp. Stock is the classic sound (the old Classic Rock "Master volume" voicing). Modded adds a gain stage: tighter, smoother, more saturated. Cranked has the master wide open: loose, saggy, loud |
| - | Acoustic (piezo to acoustic) | - | - | - | A chain that makes a piezo pickup sound like a miked acoustic guitar. It replaces the amp and cab |

(The # column is the position in the Amp B list. Both amp menus are grouped into folders by gain level: Clean, Low / mid gain and High gain; Amp A also lists Acoustic first. Open a folder to pick an amp.)

### How the new amps compare (measured with a plucked test signal, Amp Gain 0.5, overdrive off)

| Amp / voicing | Gain compression | Share of energy 0.5 to 1.5 kHz | 1.5 to 3.5 kHz | 3.5 to 9 kHz |
|---|---|---|---|---|
| Precision / Tight | about 9.9 dB | 13% | 7.5% | 1.3% |
| Precision / Crunch | about 5.2 dB | 12% | 9.7% | 0.7% |
| Digital / Thall | about 10.6 dB | 22% | 10.9% | 3.4% |
| Digital / Industrial | about 10.8 dB | 13% | 12.1% | 6.9% |
| Digital / Cavern | about 10.4 dB | 5.8% | 3.2% | 0.3% |

Thall has the most mid and upper-mid energy, Industrial the most top-end, and Cavern the least of both (it keeps almost everything below 500 Hz). The test uses low notes only, so it says little about sag and feel, which I cannot measure with it.

---

## Appendix B: control reference

Generated from the plug-in's own hover help. Grouped by page and panel. Knobs show the label on the plug-in and the parameter ID (used by MIDI learn and presets).


### INPUT page

#### INPUT & GATE

- **Menu**: Which input channel feeds the plugin. Left or Right uses just that channel (a guitar plugged into one side of an interface); Mono sums both. The result goes to both sides. Choices: Input: Left ; Input: Right ; Input: Mono (L + R).
- **PICKUP DE-NOISE** (switch): Pickup de-noise: removes mains hum with notch filters and closes a low-pass in the gaps between notes to hide hiss. It sits before the gate.
- **Menu**: The mains frequency where you live (50 or 60 Hz), so the hum notches line up with your hum. Choices: 60 Hz (US) ; 50 Hz (EU).
- **Input** (`inGain`): Input level (dB) before everything else. Set it so your guitar is strong but not clipping; the gate, pedals and amp all react to it.
- **Gate** (`gate`): Noise gate threshold (dB). Signal quieter than this is turned down, which cuts noise between notes. Raise it to gate more; too high will chop quiet notes.
- **Hum** (`deNoiseHum`): (see above)
- **Hiss** (`deNoiseHiss`): (see above)

#### TRANSPOSE / FORMANT

- **TRANSPOSE ON** (switch): Transpose on/off. It is the first effect in the chain, so everything after it (fuzz, overdrive, amp) plays the shifted notes. While it is on it adds a small delay, which the plugin reports to your DAW.
- **Menu**: Fast has less delay (about 14 ms); Smooth adds about 27 ms and is better for low strings and for shifting down. Works best on single notes. (The Mode menu in the header can override this.) Choices: Fast (~14 ms delay) ; Smooth (~27 ms delay).
- **FORMANT LOCK** (switch): Formant Lock: automatically shifts the formant the opposite way to the transpose, so the tone stays close to the original instead of getting chipmunk-bright (up) or dark (down).
- **Menu**: Which guitar this is. The Transpose, Octaver and Pitch pedal all use it. 6-string is the default and sounds as before. 7-string and 8-string tune the pitch shifters to follow the lower strings (down to a low B, or a low F#) and add a little delay for it; use them only if you play those instruments. Choices: 6-string ; 7-string ; 8-string.
- **Transpose** (`tpSemis`): Shift the pitch by whole semitones (-24 to +24).
- **Fine** (`tpFine`): Fine pitch shift in cents (hundredths of a semitone), for detuning.
- **Formant** (`tpFormant`): Formant shift: moves the tone's emphasis up or down. This is an EQ-style approximation (it cuts and re-adds emphasis around two regions), not a true formant resynthesis. Plus the Lock amount if Formant Lock is on.
- **Mix** (`tpMix`): Blend between your original notes and the shifted ones. Around 0.5 gives a harmony or detune.

### GAIN page

#### FUZZ

- **FUZZ ON** (switch): Fuzz pedal on/off. It sits before the compressor, overdrive and amp.
- **Menu**: Wall (thick, sustained wall of sound), Tight (tight, aggressive, mid-forward), Gated (old, replaced by Factory), Buzzsaw (scooped mids, boosted lows and highs, thick sawing metal buzz), Face (round, two-transistor, cleans up with the guitar's volume), Bender (raspy, two or three transistors) or Factory (gated, with Comp, that can squeal). Face, Bender and Factory have a Germanium / silicon switch. Hand-built approximations, not circuit models. Choices: Wall (sustained) ; Tight (op-amp) ; Gated (velcro) ; Buzzsaw (scooped) ; Face (round, germanium or silicon) ; Bender (raspy, 2 or 3 stage) ; Factory (gated, squealing).
- **AUTO LEVEL** (switch): Auto level: a slow level follower keeps the fuzz about as loud as the clean guitar going into it, whatever the Gain and style. The Level knob then works relative to its -9 dB starting point. It holds its setting during silence.
- **GERMANIUM** (switch): Face, Bender and Factory styles. On = germanium: soft, warm, round, with a lot of lopsided clipping and a leaky, sputtery feel. Off = silicon: tighter, brighter, harder-edged, with more gain and a less compressed sound.
- **OP-AMP** (switch): Wall style only. Op-amp mode: the stages run out of swing and then clip hard, for a tighter, brighter, harder, more modern fuzz.
- **Menu**: Wall style only. Standard (the original sound), Smooth (deeper mid scoop, a smoother top), Gritty (thick, a bit rough, more low end), Mid-forward (more bite and mids, tighter bass) or Heavy (more gain, fuzzy huge lows). Choices: Standard ; Smooth (deep scoop) ; Gritty (thick) ; Mid-forward (bite) ; Heavy (fuzzy lows).
- **Menu**: Bender style only. Early: two transistors, thick, woolly mids. Classic: three transistors, raspy and cutting. Late: three transistors with a tone sweep, tighter low end and more fizz. Choices: Early (2 stage) ; Classic (3 stage) ; Late (3 stage, tone sweep).
- **Menu**: Wall style only. The clipping diodes: Silicon (the original), LED (more headroom, louder, more open, less compression) or Germanium (clips early, softer, more compressed). Choices: Silicon ; LED (open) ; Germanium (soft).
- **TONE BYPASS** (switch): Wall style only. Bypasses the tone control for the full, unfiltered fuzz.
- **BUFFER** (switch): Face, Bender and Factory styles. A buffer in front, so the pickup is not loaded: a brighter, tighter, clearer sound. Off is the authentic woolly, pickup-loaded sound (the top end is softer and the fuzz reacts more to your guitar).
- **OCTAVE UP** (switch): An octave-up rectifier in front of the fuzz (full-wave rectified): a sputtering, ringing octave-up fuzz. Works best on single notes, high on the neck.
- **Gain** (`fuzzGain`): Amount of fuzz. Wall: sustain. Tight: distortion. Gated: drive. Buzzsaw: distortion. Face and Bender: the fuzz amount (the Face's Fuzz knob, the Bender's Attack). Factory: drive.
- **Tone** (`fuzzTone`): 0 = dark, 1 = bright (the same direction for all styles). On Buzzsaw it is the colour sweep: it moves the scooped mid band up in pitch. On Wall it is the Muff tone control (a scooped middle). On Face, Bender and Factory it is a top-end roll-off.
- **Level** (`fuzzLevel`): Output level of the fuzz pedal (dB). Fuzz can be loud, so turn this down if the amp is overloaded.
- **Mix** (`fuzzMix`): Blend between your clean guitar (0) and the fuzz (1). Around 0.3 to 0.6 keeps the note defined under the fuzz. The clean part is delayed to match the fuzz, so the two stay in phase.
- **Gate** (`fuzzGate`): Gated and Factory styles. Higher = the stage is starved, so notes cut off and sputter as they die away.
- **Stab** (`fuzzStab`): Gated and Factory styles. 1 = stable; lower = more feedback, giving squeals and howls as notes decay.
- **Starve** (`fuzzStarve`): Face and Bender styles. A weak, dying battery: less headroom, a gap at the zero crossing, and a gated, sputtering, spitty sound. 0 is a fresh battery.
- **Mids** (`fuzzMuffMids`): Wall style only. The middle of the sound: 0.5 is the original. Lower scoops the mids further, higher fills them back in for a more forward fuzz.
- **Low** (`buzzLow`): Buzzsaw style only. Boosts the low end (thump and weight).
- **High** (`buzzHigh`): Buzzsaw style only. Boosts the highs (bite and cut).
- **Mid** (`buzzMid`), **Soft clip** (`buzzSoft`), **Hard clip** (`buzzHard`), **Blend** (`buzzBlend`), **Invert blend** (`buzzPol`): Buzzsaw only (v94, from a modded HM-2 schematic). Mid is the upper-mid band split from the single tone knob; Soft and Hard are the two clipper pairs (both on = the original); Blend mixes in a clean copy of your input and Invert flips it.
- **Comp** (`fuzzComp`): Factory style only. Compression ahead of the fuzz: higher squashes the input, so quiet notes get louder and the sound sustains more.
- **Guitar Vol** (`fuzzInput`): Face, Bender and Factory styles. The volume knob on your guitar: turn it down and the fuzz cleans up, the way a fuzz pedal reacts to your guitar. 1 is full.
- **Drift** (`fuzzDrift`): Temperature drift: the gain and bias slowly wander, the way a germanium fuzz changes as it warms up. 0 is rock steady.
- **Hiss** (`fuzzHiss`): Transistor hiss in front of the fuzz. It gets louder as you turn the fuzz up. 0 is silent.

#### COMPRESSOR

- **COMP ON** (switch): Compressor on/off. It sits after the noise and fuzz pedals and before the amp.
- **Menu**: How hard it squeezes: 1:1 (saturation only) up to 20:1, 10:1 Soft (slow-release, light-controlled feel) and Limit (a hard limiter). Choices: 1:1 ; 2:1 ; 3:1 ; 4:1 ; 6:1 ; 10:1 ; 10:1 Soft ; 20:1 ; Limit.
- **Menu**: The kind of compressor. Standard is the original. FET is fast and punchy, Opto is smooth and slow to let go, Variable-mu is gentle glue, Sustainer is a pedal-style squeeze that lets the pick through and holds notes, Bus is light glue for chords. Choices: Standard ; FET ; Opto ; Variable-mu ; Sustainer ; Bus.
- **AUTO LEVEL** (switch): Auto level: adds the makeup gain for you, based on the Threshold, Ratio and Mix (a table I measured with a plucked test signal). The Output knob then trims on top of it, with 0 dB meaning 'about as loud as the compressor off'.
- **Menu**: Gentle saturation that grows with compression: tube-like (even harmonics), tape-like (odd harmonics), both, Transformer (low end only), or one of the four voiced for the compressor styles: FET stage, Opto tube stage, Mu iron + tube, Diode. Choices: Off ; Tube-like ; Tape-like ; Both ; Transformer.
- **Menu**: High-pass on what the compressor listens to (Off, 80, 160, 320 Hz), so low palm mutes don't pump the whole sound. Choices: Off ; 80 Hz ; 160 Hz ; 320 Hz.
- **Thresh** (`compThresh`): Level (dB) above which the compressor works. Lower = more compression.
- **Attack** (`compAttack`): How fast it reacts (0.05-50 ms). Short catches the pick attack; longer lets it through.
- **Release** (`compRelease`): How fast the gain recovers (50 ms-3.5 s).
- **Output** (`compOutput`): Makeup gain (dB) to bring the level back up after compressing.
- **Mix** (`compMix`): Dry/wet blend for parallel compression. 1 = all compressed.
- **Bloom** (`compBloom`): Makes the main compressor work harder and let go more slowly, so notes squeeze down and then swell back up (up to 2.5x the gain reduction and 3x the release). 0 = off.
- **Smack** (`compSmack`): Level of a second, parallel compressor that runs alongside the main one. It is fast, heavy and transparent, with the lows lifted, so it adds low-end impact and thickens quiet notes without squashing the attack. 0 = off.
- **Clarity** (`compClarity`): A tilt EQ on the compressor's output. Turn right for more highs and less lows (clarity, bite); turn left for warmth and thickness. 0 = flat.

#### OVERDRIVE

- **OVERDRIVE ON** (switch): Overdrive pedal on/off. It sits before the amp and pushes its first stage.
- **Menu**: Transparent (smooth, open), Tight (tight, modern, gated), Open (a dynamic, amp-like overdrive that cleans up with your pick attack, with a range switch), Screamer (a versatile mid-hump drive with clipping, voicing, Bass and Mid controls) or Distortion (a hard-clipped distortion with a clipping choice and a Mids switch). Hand-built approximations, not circuit models. Choices: Transparent ; Tight ; Open ; Screamer ; Distortion.
- **AUTO LEVEL** (switch): Auto level: sets the overdrive's loudness to match the guitar without it (measured through the Clean amp, so it is the pedal's own level change). The Volume knob then works relative to its -6 dB starting point. Turn it off when you want the overdrive as a boost into the amp.
- **HIGH RANGE** (switch): Open style only. High: more gain and a fuller, more compressed drive. Low: lighter, with more headroom and a clearer, more dynamic feel.
- **Menu**: Screamer and Distortion: what clips the signal. Silicon is the classic sound; LED has more headroom (louder, more open); Germanium clips early and soft; Asymmetric clips the two halves differently, which adds even harmonics and a more amp-like feel. Choices: Silicon (classic) ; LED (open) ; Germanium (soft) ; Asymmetric.
- **Menu**: Screamer only. Classic: the familiar mid hump and gain. Mild: leaner, with less gain and no hump. Open: brighter, with more gain and a looser low end. Choices: Classic (mid hump) ; Mild (lean) ; Open (bright).
- **Menu**: Screamer and Distortion: what clips the signal. Silicon is the classic sound; LED has more headroom (louder, more open); Germanium clips early and soft; Asymmetric clips the two halves differently, which adds even harmonics and a more amp-like feel. Choices: Silicon (classic) ; LED (open) ; Germanium (soft) ; Asymmetric.
- **Menu**: Distortion only: Scoop is the stock sound (a mid dip, scooped and fizzy), Flat leaves the mids alone, Boost adds a mid hump for a thicker, more lead-like tone. Choices: Scoop (stock) ; Flat ; Boost.
- **Gain** (`odGain`): Amount of drive. Transparent and Screamer blend a clean path with the gain path; Tight and Distortion send almost everything through the gain path.
- **Tone** (`odTone`): Treble control after the clipper (darker to brighter). On the Distortion it tilts the highs around 1.2 kHz.
- **Edge** (`odEdge`): Transparent, Screamer and Distortion styles: extra treble fed into the clipper for more bite (shown as Bright on the Screamer and Distortion).
- **Drive** (`odGain`): Amount of drive. Transparent and Screamer blend a clean path with the gain path; Tight and Distortion send almost everything through the gain path.
- **Bright** (`odTone`): Treble control after the clipper (darker to brighter). On the Distortion it tilts the highs around 1.2 kHz.
- **Gate** (`odGate`): Tight style: a noise gate ahead of the gain, for chopped palm mutes. 0 = off. Too high cuts quiet notes.
- **Attack** (`odAttack`): Tight style: positions 1-6. Higher tightens the low end more and adds more bite around 750 Hz.
- **Drive** (`odGain`): Amount of drive. Transparent and Screamer blend a clean path with the gain path; Tight and Distortion send almost everything through the gain path.
- **Drive** (`odGain`): Amount of drive. Transparent and Screamer blend a clean path with the gain path; Tight and Distortion send almost everything through the gain path.
- **Dist** (`odGain`): Amount of drive. Transparent and Screamer blend a clean path with the gain path; Tight and Distortion send almost everything through the gain path.
- **Tone** (`odTone`): Treble control after the clipper (darker to brighter). On the Distortion it tilts the highs around 1.2 kHz.
- **Tone** (`odTone`): Treble control after the clipper (darker to brighter). On the Distortion it tilts the highs around 1.2 kHz.
- **Mid** (`odMid`): Screamer only: cuts or boosts the mid hump before the clipper. Centre = the voicing's own hump.
- **Tone** (`odTone`): Treble control after the clipper (darker to brighter). On the Distortion it tilts the highs around 1.2 kHz.
- **Volume** (`odVolume`): Output level of the overdrive (dB). A boost into the amp can be much louder than the amp alone.
- **Bass** (`odBass`): Screamer only: how much low end goes into the clipper. Low = tight and mid-focused (the classic sound); high = fuller and thicker.
- **Mid Freq** (`odMidFreq`): Screamer only: where the mid control works, from about 350 Hz (honky, vocal) up to about 2 kHz (nasal, cutting).
- **Bright** (`odEdge`): Transparent, Screamer and Distortion styles: extra treble fed into the clipper for more bite (shown as Bright on the Screamer and Distortion).
- **Bright** (`odEdge`): Transparent, Screamer and Distortion styles: extra treble fed into the clipper for more bite (shown as Bright on the Screamer and Distortion).

#### BOOST

- **BOOST ON** (switch): Switches the Boost on. It sits after the Lo-Fi pedal and before the NAM pedal, the overdrive and the amp: tighten filter, gain, saturation and a 4-band EQ, in that order (the EQ can go before the saturation instead).
- **Menu**: The saturation character: Clean (no saturation, just gain), EP (warm, bass-heavy and soft, like a small tube preamp), Console (transformer-style: smooth, the lows saturate first), Tube (an asymmetric stage with even harmonics) Treble (a treble booster: thin lows, strong upper mids, germanium-style grit), Mid (a mid-forward push for leads) or FET (a transparent solid-state stage). Choices: Clean ; EP ; Console ; Tube ; Treble ; Mid ; FET.
- **Menu**: Mid hump: a low cut plus a mid hump around 720 Hz. Low cut: only the low cut, nothing else. Focus: a low cut plus a push around 1.5 kHz for pick attack. Choices: Mid hump ; Low cut ; Focus.
- **Gain** (`bsGain`): How much the boost lifts the signal before the saturation, in dB. More gain pushes harder into the saturation, and into the amp after it.
- **Saturation** (`bsSat`): Pick another mode.
- **Tighten** (`bsTight`): The tighten filter: cuts the low end that makes a high-gain amp flabby. 0 = off; turning it up raises the cut from 40 Hz to 300 Hz. The style menu picks its character.
- **Level** (`bsLevel`): The boost's output level, in dB.

#### BOOST EQ  (4-BAND PARAMETRIC)

- **EQ ON** (switch): Switches the 4-band EQ on. Bands set to 0 dB cost nothing.
- **Menu**: Before saturation: the EQ shapes what hits the saturation (cut lows to keep it tight, boost mids to make it sing). After saturation: it shapes the finished sound. Choices: Before saturation ; After saturation.
- **Low Freq** (`bsLoHz`): Low shelf frequency.
- **Low Gain** (`bsLoDb`): Low shelf gain, in dB.
- **Mid 1 Freq** (`bsM1Hz`): Band 1 centre frequency (a bell, 100 Hz to 2 kHz).
- **Mid 1 Gain** (`bsM1Db`): Band 1 gain, in dB.
- **Mid 1 Q** (`bsM1Q`): Band 1 width. Low Q = broad, high Q = narrow.
- **Mid 2 Freq** (`bsM2Hz`): Band 2 centre frequency (a bell, 500 Hz to 8 kHz).
- **Mid 2 Gain** (`bsM2Db`): Band 2 gain, in dB.
- **Mid 2 Q** (`bsM2Q`): Band 2 width. Low Q = broad, high Q = narrow.
- **High Freq** (`bsHiHz`): High shelf frequency.
- **High Gain** (`bsHiDb`): High shelf gain, in dB.

### MOD page

#### WAH

- **WAH ON** (switch): Wah pedal on/off. It sits after the compressor and before the phaser, chorus, overdrive and amp.
- **Menu**: Manual: you set the Position (automate it from your DAW). Auto: Envelope: playing harder opens the wah. Auto: LFO: it sweeps by itself. Choices: Manual (Position) ; Auto: Envelope ; Auto: LFO.
- **Menu**: Free uses the LFO Rate knob. A note value locks the wah sweep to the tempo (one note length per cycle). Choices: Free (Hz) ; 1/2 ; Dotted 1/4 ; 1/4 ; 1/4 triplet ; Dotted 1/8 ; 1/8 ; 1/8 triplet ; Dotted 1/16 ; 1/16 ; 1/16 triplet.
- **Position** (`wahPos`): Filter position, 0 = heel (low, bassy) to 1 = toe (high, trebly). In Envelope mode it's the resting position; in LFO mode it's the centre of the sweep.
- **Heel** (`wahHeel`): Lowest centre frequency of the wah sweep.
- **Toe** (`wahToe`): Highest centre frequency of the wah sweep.
- **Resonance** (`wahQ`): Resonance: how sharp and vocal the wah peak is. Higher = more nasal and pointed.
- **Mix** (`wahMix`): Wet/dry. 1 = fully wah; lower blends in your dry signal for a subtler effect.
- **Sensitivity** (`wahSens`): (see above)
- **Decay** (`wahDecay`): (see above)
- **LFO Rate** (`wahRate`): (see above)
- **LFO Depth** (`wahDepth`): (see above)

#### PHASER

- **PHASER ON** (switch): Phaser on/off. It sits after the wah and before the chorus and amp.
- **Menu**: Number of all-pass stages (2-12). More stages = more notches and a deeper, more complex sweep. Choices: 2 stages ; 4 stages (classic) ; 6 stages ; 8 stages ; 10 stages ; 12 stages.
- **Menu**: Shape of the sweep: sine, triangle or random. Choices: Sine ; Triangle ; Random.
- **Menu**: Free uses the Rate knob. A note value locks the phaser sweep to the tempo (one note length per cycle). Choices: Free (Hz) ; 1/2 ; Dotted 1/4 ; 1/4 ; 1/4 triplet ; Dotted 1/8 ; 1/8 ; 1/8 triplet ; Dotted 1/16 ; 1/16 ; 1/16 triplet.
- **Rate** (`phaserRate`): Speed of the sweep (Hz).
- **Depth** (`phaserDepth`): How far the sweep travels.
- **Centre** (`phaserCenter`): Centre frequency the sweep moves around.
- **Feedback** (`phaserFeedback`): Feeds the output back in for a sharper, more resonant sweep. Negative values give a hollower sound.
- **Mix** (`phaserMix`): Wet/dry blend. About 0.5 is the classic phaser sound.
- **Envelope** (`phaserEnv`): How much your playing level also moves the sweep (envelope follower).

#### CHORUS

- **CHORUS ON** (switch): Chorus on/off. It sits after the phaser, before the overdrive and amp.
- **Menu**: Chorus voicing. I, II and I+II are the classic modes (I+II is a fast, vibrato-like shimmer). Dimension, Tri-chorus, Ensemble and Detune are the wider, slower voicings. Choices: I ; II ; I + II ; Dimension ; Tri-chorus ; Ensemble ; Detune.
- **Mix** (`chorusMix`): Wet/dry blend of the chorus.
- **Rate** (`chorusRate`): Multiplies the speed of the mode's built-in sweep.
- **Depth** (`chorusDepth`): Multiplies how far the mode's sweep moves the pitch.
- **Tone** (`chorusTone`): Low-pass on the chorus signal (Hz). Lower = darker, warmer.
- **Hiss** (`chorusHiss`): Adds the faint hiss of the analog circuit. 0 = silent.

#### MOD FX A

- **MOD FX A ON** (switch): Switches Mod FX A on.
- **Menu**: The effect: Flanger, Through-zero flanger (the dry signal is delayed 4 ms too, which adds that fixed delay), Vibrato (pitch wobble only), Uni-V (a throbbing phase-vibrato), Rotary (a rotating horn and drum), Auto-pan (a tremolo if the signal is mono) or Harmonic tremolo (lows and highs pulse in opposite directions). Choices: Flanger ; Through-zero flanger ; Vibrato ; Uni-V ; Rotary ; Auto-pan ; Harmonic tremolo.
- **Menu**: Before the amp: the effect hits the amp's input (and its gain). After the cab: it is applied to the finished sound, in stereo. Choices: Before the amp ; After the cab.
- **Menu**: The LFO wave: Sine, Triangle or Soft square. Choices: Sine ; Triangle ; Soft square.
- **Menu**: Free uses the Rate knob. A note value locks the speed to the tempo (one note length per cycle). Choices: Free (Hz) ; 1/2 ; Dotted 1/4 ; 1/4 ; 1/4 triplet ; Dotted 1/8 ; 1/8 ; 1/8 triplet ; Dotted 1/16 ; 1/16 ; 1/16 triplet.
- **Rate** (`mfARate`): How fast the effect moves, in Hz (used when Note is Free). For the Rotary this is the horn speed: about 0.8 Hz is slow, 6.7 Hz is fast; the drum follows more slowly, like the real thing.
- **Depth** (`mfADepth`): How far the effect sweeps.
- **Feedback** (`mfAFeedback`): Flangers: positive rings, negative sounds hollow. Other types ignore it.
- **Manual** (`mfAManual`): Flangers: the centre delay time (0.3 to 7.8 ms): low = bright and jet-like, high = deep and slow. Rotary: the balance between horn (high) and drum (low). Other types ignore it.
- **Tone** (`mfATone`): Rolls off the top end of the effect (not the dry signal).
- **Mix** (`mfAMix`): Dry to wet. Vibrato and Uni-V sound like a pure vibrato at 100%; lower values blend in the dry signal for a flanger/chorus-like character.

#### MOD FX B

- **MOD FX B ON** (switch): Switches Mod FX B on.
- **Menu**: The effect: Flanger, Through-zero flanger (the dry signal is delayed 4 ms too, which adds that fixed delay), Vibrato (pitch wobble only), Uni-V (a throbbing phase-vibrato), Rotary (a rotating horn and drum), Auto-pan (a tremolo if the signal is mono) or Harmonic tremolo (lows and highs pulse in opposite directions). Choices: Flanger ; Through-zero flanger ; Vibrato ; Uni-V ; Rotary ; Auto-pan ; Harmonic tremolo.
- **Menu**: Before the amp: the effect hits the amp's input (and its gain). After the cab: it is applied to the finished sound, in stereo. Choices: Before the amp ; After the cab.
- **Menu**: The LFO wave: Sine, Triangle or Soft square. Choices: Sine ; Triangle ; Soft square.
- **Menu**: Free uses the Rate knob. A note value locks the speed to the tempo (one note length per cycle). Choices: Free (Hz) ; 1/2 ; Dotted 1/4 ; 1/4 ; 1/4 triplet ; Dotted 1/8 ; 1/8 ; 1/8 triplet ; Dotted 1/16 ; 1/16 ; 1/16 triplet.
- **Rate** (`mfBRate`): How fast the effect moves, in Hz (used when Note is Free). For the Rotary this is the horn speed: about 0.8 Hz is slow, 6.7 Hz is fast; the drum follows more slowly, like the real thing.
- **Depth** (`mfBDepth`): How far the effect sweeps.
- **Feedback** (`mfBFeedback`): (see above)
- **Manual** (`mfBManual`): (see above)
- **Tone** (`mfBTone`): Rolls off the top end of the effect (not the dry signal).
- **Mix** (`mfBMix`): Dry to wet. Vibrato and Uni-V sound like a pure vibrato at 100%; lower values blend in the dry signal for a flanger/chorus-like character.

### PITCH page

#### PITCH PEDAL  (EXPRESSION)

- **PITCH PEDAL ON** (switch): Pitch pedal on/off. While on it adds a small processing delay (about 14 to 27 ms, set by the Transpose quality menu), which is reported to your DAW.
- **Menu**: What the pedal does. Octave up / Two octaves up / Octave down / Two octaves down: the pedal sweeps from your normal pitch (Position 0) to that shift (Position 1). Dive bomb: a falling pitch drop (Position 0 = none, 1 = deepest). Bend: -12 to +12 semitones (centre = no shift). Custom: you set the heel and toe shifts. Choices: Octave up ; Two octaves up ; Octave down ; Two octaves down ; Dive bomb ; Bend (-12 to +12) ; Custom (heel and toe).
- **Position** (`wowPos`): The pedal position (0 = heel, 1 = toe). Automate it from your DAW, or from a controller mapped to this knob, to play the pedal.
- **Heel** (`wowHeel`): Custom mode: the pitch shift (semitones) when the pedal is at the heel.
- **Toe** (`wowToe`): Custom mode: the pitch shift (semitones) when the pedal is at the toe.
- **Glide** (`wowGlide`): How quickly the pitch follows the pedal position (ms). Short is snappy; long gives a smooth slide.
- **Mix** (`wowMix`): Wet/dry mix. 1 = only the shifted sound (like a real pedal); lower blends in the dry guitar for harmony effects.

#### OCTAVER

- **OCTAVER ON** (switch): Octaver on/off. Two shifted voices blended with the dry guitar. While on it adds a small processing delay, which is reported to your DAW.
- **Menu**: Pitch of voice A: two octaves down, one octave down, one octave up, or two octaves up. Choices: 2 octaves down ; 1 octave down ; 1 octave up ; 2 octaves up.
- **Menu**: Pitch of voice B: two octaves down, one octave down, one octave up, or two octaves up. Choices: 2 octaves down ; 1 octave down ; 1 octave up ; 2 octaves up.
- **Voice A** (`octALvl`): Level of voice A.
- **Voice B** (`octBLvl`): Level of voice B. 0 turns it off.
- **Dry** (`octDry`): Level of the unshifted guitar.
- **Tone** (`octTone`): Darkens or brightens the shifted voices (0 = dark and smooth, 1 = bright).

#### HARMONIZER

- **HARMONIZER ON** (switch): Switches the harmonizer on: two extra voices that follow the note you play. It adds a little delay (shown to your DAW).
- **Menu**: Before the amp: the harmony is mono and goes through the amp's gain with your playing. After the cab: the harmony is added to the finished sound, in stereo, with a pan for each voice. Choices: Before the amp ; After the cab.
- **Menu**: Interval: each voice is a fixed number of semitones away. Scale: each voice is a number of steps up or down the scale you choose, so the harmony stays in key (it listens to the note you are playing). Choices: Interval ; Scale.
- **Menu**: Single note: tracks one note at a time (fast, about 14 ms), best for leads and single-note lines. Chords: follows several notes at once and moves each by its own amount (about 90 ms of delay). Chords (fast): the same with less delay (about 45 ms), but it cannot tell low notes apart as well, so it suits upper-string voicings. Choices: Single note ; Chords ; Chords (fast).
- **Menu**: The key of the song, for Scale mode. Choices: C ; C# ; D ; D# ; E ; F ; F# ; G ; G# ; A ; A# ; B.
- **Menu**: The scale the harmony stays in, for Scale mode. Choices: Major ; Natural minor ; Dorian ; Mixolydian ; Harmonic minor ; Major pentatonic ; Minor pentatonic.
- **A Interval** (`hmAInt`): (see above)
- **A Step** (`hmADeg`): Voice A, in Scale mode: how many scale steps from your note (2 = a third up, 4 = a fifth up, 7 = an octave up, -2 = a third down).
- **A Level** (`hmALvl`): How loud voice A is. 0 = off.
- **A Pan** (`hmAPan`): Choose After the cab for stereo.
- **Dry** (`hmDry`): The level of your own playing next to the harmony.
- **Tone** (`hmTone`): Rolls off the top of the harmony voices. Lower = softer and darker.
- **B Interval** (`hmBInt`): (see above)
- **B Step** (`hmBDeg`): Voice B, in Scale mode: how many scale steps from your note.
- **B Level** (`hmBLvl`): How loud voice B is. 0 = off.
- **B Pan** (`hmBPan`): Choose After the cab for stereo.
- **Glide** (`hmGlide`): How quickly the voices slide to a new note. Short = snappy, long = a smooth slide between notes.

### AMP page

#### AMP

- **Menu**: Which amp: Clean (sparkly), Vintage Chime (class-A), British Hot-Rod (tight), Saturated Lead (smooth), Modern High-Gain (scooped), Classic Rock (crunch), Boutique Lead (smooth, singing), Tight Metal (raw to modern), Low-Watt Combo (spongy breakup), Glass Clean (solid-state), Creamy Lead (tight, mid-hump), Doom Stack (thick, loose), Precision High-Gain (a modern boutique high-gain amp), Digital Extreme (a modern digital-style amp for extreme styles), or Acoustic (piezo to acoustic). Choices: Acoustic (piezo to acoustic) ; Clean (sparkly) ; Vintage Chime (class-A) ; British Hot-Rod (tight) ; Saturated Lead (smooth) ; Modern High-Gain (scooped) ; Classic Rock (crunch) ; Boutique Lead (smooth, singing) ; Tight Metal (raw to modern) ; Low-Watt Combo (spongy breakup) ; Glass Clean (solid-state) ; Creamy Lead (tight, mid-hump) ; Doom Stack (thick, loose) ; Precision High-Gain (tight, modern) ; Digital Extreme (brutal, digital).
- **Menu**: The amp's voicing, with four positions per amp (listed in section 6). Clean: Sparkly, Edge push (a small old amp turned up) or Boutique (fuller mids, more compression). Vintage Chime: Chime, Treble boost or Normal (a darker, rounder, thicker channel without the top boost). Hot-Rod: Tight, Heavy (deep but controlled lows, more gain, forward upper mids) or Brown (a thicker, saggier, mid-rich crunch). Saturated Lead: Smooth, Lead or Crunch (a low-gain channel). Modern High-Gain: Scooped, Clarity or Vintage (spongy, looser, fatter lows). Classic Rock: Crunch, Pushed or Jumped (both channels linked: fatter, hotter, more sag). Boutique Lead: Jazz, Rock or Lead (the boosted lead mode). Tight Metal: Raw, Modern or Crunch (a low-gain rhythm channel). Low-Watt Combo: Soft, Cranked or Four-ten (a 4x10-style voicing with more headroom and bigger lows). Glass Clean: Glass, Warm or Funk (tight, percussive and scooped). Creamy Lead: Rhythm, Lead or Vintage (an early-style sound with lower gain and thick creamy mids). Doom Stack: Stoner, Wall or Bass stack (massive low mids, a cleaner top, a grind only when pushed). Precision High-Gain: Tight (the main high-gain channel), Lead (more gain, mid hump, sustain) or Crunch (an open, lower-gain channel). Digital Extreme: Thall (ultra-tight, scooped, hard clipping), Industrial (harsh, gritty, fizzy upper mids) or Cavern (huge, slow, dark, for drop tunings). Master-Volume Rock: Stock (the classic 80s master-volume sound), Modded (an extra gain stage: tighter, smoother, more saturated) or Cranked (master wide open: loose, saggy, loud). Does nothing in Acoustic mode. Choices: Voicing: Tight ; Voicing: Heavy ; Voicing: Brown.
- **AUTO LEVEL** (switch): Auto level: this amp's output is held at the same loudness for every setting of the Gain knob and for every amp model and voicing, so changing the gain changes the sound, not the volume. The level comes from a table I measured, so it adds no processing. Overdrive and Modern Metal settings can still change the loudness a little.
- **BRIGHT** (switch): Bright switch: a treble lift that fades out as the Gain goes up (like the bright switch on a vintage amp). Most noticeable at low gain.
- **Menu**: The tone stack: as voiced (the amp's own), Neutral (most of the amp's own tone shape taken out) or one of seven passive stacks solved from component values. Bass, Low Mid, High Mid and Treble still work on top.
- **Menu**: How much negative feedback the power amp has. Loose: sags and blooms, fuller and looser lows, less presence. Normal: the amp as voiced. Tight: stiff and fast, tighter lows, more presence. Choices: Loose feel (low feedback) ; Normal feel ; Tight feel (high feedback).
- **Menu**: The preamp tube: 12AX7 (high gain, tight and crisp), 12AT7 (medium) or 12AY7 (low gain, softer and rounder). Choices: 12AX7 (high gain) ; 12AT7 (medium) ; 12AY7 (low gain).
- **Menu**: The power tubes: As voiced (the amp's own), EL34 (crunchy mids), 6L6 (tight, clean headroom), 6V6 (soft, early breakup, lots of sag), EL84 (bright, chimey) or KT88 (big and tight). Changes the feel, tone and how soon the power stage breaks up. Choices: As voiced ; EL34 ; 6L6 ; 6V6 ; EL84 ; KT88.
- **DEEP** (switch): A low-end boost into the first stage and the tone stack: more weight and thump.
- **Menu**: Oversampling for the amp's distortion. Best (4x) sounds smoothest but uses the most CPU and adds the most delay. Balanced (2x) is about half the CPU. Lowest latency has no added delay and the least CPU, but high-gain settings can sound slightly fizzier. Choices: Best (4x, more CPU) ; Balanced (2x) ; Lowest latency (no oversampling).
- **Gain** (`ampGain`): Amount of preamp gain for the chosen amp. The top of the knob (above about 55%) adds extra gain on top of the old range, most on the high-gain amps, for heavier modern metal and djent saturation; the lower part is unchanged.
- **Bass** (`bass`): Bass control (dB), added to the amp's own voicing.
- **Low Mid** (`lowMid`): Low-mid control (dB), around 400-500 Hz.
- **High Mid** (`highMid`): High-mid control (dB), around 1.2-2 kHz.
- **Treble** (`treble`): Treble control (dB).
- **Presence** (`presence`): Power-amp presence: adds treble after the clipping, like a real amp's presence knob.
- **Focus** (`ampTune`): Focus: tightens the low end and trims bass and gain so low-tuned guitars stay clear. Turn it up as your tuning gets lower (about 1 per semitone below E standard). Only the Saturated Lead, Modern High-Gain, Tight Metal, Creamy Lead, Precision High-Gain and Digital Extreme amps use it.
- **Bias** (`ampBias`): Power-tube bias. Negative: hotter, with more bite and a tighter feel. Positive: colder, with a looser, crossover-style sound and more gap in the attack. 0 is the amp's own.
- **Power Drive** (`ampPower`): Power-amp drive: pushes the output stage into saturation for more squash and sizzle. 0 is off.
- **Resonance** (`ampReso`): Resonance: low-end thump from the power amp and speaker interaction (around 100 Hz). 0 is off.
- **Sag** (`ampSagAmt`): How far the power supply droops under loud notes (squash and bloom). 0.5 is the amp's own, 0 is stiff, 1 is double.

#### ACOUSTIC / PIEZO  (used when Amp Model = Acoustic)

- **Menu**: The size and shape of the acoustic guitar body being imitated: Dreadnought, Jumbo (bigger, lower), Parlor / OM (smaller, more midrange) or Nylon classical (softer top). Choices: Body: Dreadnought ; Body: Jumbo ; Body: Parlor / OM ; Body: Nylon classical.
- **Body** (`acBodyAmt`): How strongly the body resonances are applied. 0 = just the piezo signal.
- **Piezo Fix** (`acFix`): Removes the nasal 'quack' (about 2.3 kHz) and glassy edge (about 4.8 kHz) of a piezo pickup and fills in the hollow low mids.
- **Preamp** (`acPreamp`): Soft saturation for warmth, like a tube preamp. 0 = clean.
- **Squeeze** (`acSqueeze`): Gentle compression for an even, polished acoustic level. 0 = off.
- **Sparkle** (`acSparkle`): High shelf at 5 kHz (dB): adds or removes sparkle.
- **Warmth** (`acWarmth`): Low shelf at 200 Hz (dB): adds or removes warmth.
- **Room** (`acRoom`): Short early reflections for a sense of room and stereo width.
- **Level** (`acLevel`): Output level of the acoustic section (dB).

#### MODERN METAL

- **LEAD BOOST** (switch): Lead boost: more gain in the first stages, a little more mid push and sag, with the level trimmed to match. For solos.
- **Tight** (`mtTight`): Raises the high-passes between the gain stages (up to about 2.8 times), so low palm mutes stay clear and fast.
- **Depth** (`mtDepth`): Low-end depth: a shelf below about 90 Hz after the preamp (up to +6 dB) for a bigger thump.
- **Mid Freq** (`mtMidHz`): Slides the pre-gain mid push up or down in frequency. The middle position is the amp's own.
- **Mid Boost** (`mtMid`): How much extra mid push goes in before the gain stages (up to +8 dB), for a more focused, aggressive mid.
- **Fizz Tamer** (`mtFizz`): Rolls off the top after the preamp (from 12 kHz down to about 3.8 kHz) to remove fizz without dulling the pick attack as much as turning Presence down.
- **Sag** (`mtSag`): Power-amp sag: how much loud notes squash and bloom. The middle position is the amp's own; 0 is stiff, 1 is double.
- **Chug Gate** (`mtChug`): A fast noise gate after the preamp that cuts the gain's hiss between palm mutes. 0 = off. Too high cuts quiet notes.

#### BRITISH HOT-ROD

#### AMP B  (PARALLEL)

- **Menu**: Which electric amp Amp B is. Use a different one from Amp A for layered tones, such as a tight high-gain with a cleaner amp underneath. Choices: Clean (sparkly) ; Vintage Chime (class-A) ; British Hot-Rod (tight) ; Saturated Lead (smooth) ; Modern High-Gain (scooped) ; Classic Rock (crunch) ; Boutique Lead (smooth, singing) ; Tight Metal (raw to modern) ; Low-Watt Combo (spongy breakup) ; Glass Clean (solid-state) ; Creamy Lead (tight, mid-hump) ; Doom Stack (thick, loose) ; Precision High-Gain (tight, modern) ; Digital Extreme (brutal, digital).
- **Menu**: Amp B's voicing: the same four-position switch as Amp A's, with the names for Amp B's model. Choices: Voicing: Sparkly ; Voicing: Edge push ; Voicing: Boutique.
- **AMP B ON** (switch): Turns on the parallel Amp B. It gets the same signal as Amp A (after the overdrive pedal) and runs its own preamp and tone stack. Each cab block then picks Amp A, Amp B or both. Uses about as much CPU as Amp A again. Not available with the Acoustic amp.
- **AUTO LEVEL** (switch): Auto level for Amp B: same as Amp A's. Holds Amp B at the same loudness for every setting of its Gain knob and every model. B Level still works on top of it.
- **FED BY OVERDRIVE** (switch): Lets the overdrive pedal feed Amp B too (it always feeds Amp A). Turn it off to hear Amp B without the pedal's boost.
- **BRIGHT** (switch): Amp B bright switch: a treble lift that fades out as the Gain goes up.
- **Menu**: Amp B tone stack: as voiced, Neutral or a passive stack.
- **Menu**: Amp B feedback feel: Loose, Normal or Tight. Choices: Loose feel (low feedback) ; Normal feel ; Tight feel (high feedback).
- **Menu**: Amp B preamp tube: 12AX7 (high gain), 12AT7 (medium) or 12AY7 (low gain). Choices: 12AX7 (high gain) ; 12AT7 (medium) ; 12AY7 (low gain).
- **Menu**: Amp B power tubes: As voiced, EL34, 6L6, 6V6, EL84 or KT88. Choices: As voiced ; EL34 ; 6L6 ; 6V6 ; EL84 ; KT88.
- **DEEP** (switch): Amp B low-end boost.
- **Gain** (`ampBGain`): Amp B's preamp gain.
- **Bass** (`ampBBass`): Amp B bass (dB).
- **Low Mid** (`ampBLowMid`): Amp B low mid (dB).
- **High Mid** (`ampBHighMid`): Amp B high mid (dB).
- **Treble** (`ampBTreble`): Amp B treble (dB).
- **Presence** (`ampBPresence`): Amp B presence: treble on its power stage.
- **Tight** (`ampBTight`): Amp B's tightness: raises the high-passes between its gain stages (Hot-Rod, Lead and High-Gain only).
- **A Level** (`ampALevel`): (see above)
- **B Level** (`ampBLevel`): (see above)
- **Bias** (`ampBBias`): Amp B power-tube bias. Negative: hotter and tighter. Positive: colder and looser.
- **Power Drive** (`ampBPower`): Amp B power-amp drive: more output-stage saturation. 0 is off.
- **Resonance** (`ampBReso`): Amp B resonance: low-end thump around 100 Hz. 0 is off.
- **Sag** (`ampBSagAmt`): How far Amp B's power supply droops under loud notes. 0.5 is the amp's own, 0 is stiff, 1 is double.

#### NAM: AMP A  (in place of Amp A's preamp and power stage)

- **ON** (switch): Uses the loaded neural capture (.nam file) in place of Amp A's preamp and power stage. The overdrive pedal still feeds it, and the tone stack and cab come after it. Amp B can still run beside it.
- **LOAD MODEL...** (switch): Loads a neural capture (.nam file) into the Amp A slot. WaveNet and LSTM captures are supported. The model file stays where it is: the session remembers its location.
- **CLEAR** (switch): Unloads the Amp A model.
- **MATCH LOUDNESS** (switch): Evens out the volume between models using the loudness stored in the file, so different captures sit at similar levels. Off: the raw level of the model.
- **Input** (`namIn`): How hard the signal drives the model, in dB. Captures are made at a particular input level: raise this for more gain and breakup, lower it for a cleaner, quieter sound.
- **Output** (`namOut`): The model's output level, in dB.
- **Bass** (`bass`): Bass control (dB), added to the amp's own voicing.
- **Low Mid** (`lowMid`): Low-mid control (dB), around 400-500 Hz.
- **High Mid** (`highMid`): High-mid control (dB), around 1.2-2 kHz.
- **Treble** (`treble`): Treble control (dB).

#### NAM: AMP B  (in place of Amp B's preamp and power stage)

- **ON** (switch): Uses Amp B's own neural capture in place of Amp B's preamp and power stage. Amp B must also be switched on (AMP B ON on the Amp B tab).
- **LOAD MODEL...** (switch): Loads a neural capture (.nam file) into the Amp B slot. WaveNet and LSTM captures are supported. The model file stays where it is: the session remembers its location.
- **CLEAR** (switch): Unloads the Amp B model.
- **MATCH LOUDNESS** (switch): Evens out the volume between models using the loudness stored in the file.
- **Input** (`namBIn`): How hard the signal drives the Amp B model, in dB.
- **Output** (`namBOut`): The Amp B model's output level, in dB.
- **Bass** (`ampBBass`): Amp B bass (dB).
- **Low Mid** (`ampBLowMid`): Amp B low mid (dB).
- **High Mid** (`ampBHighMid`): Amp B high mid (dB).
- **Treble** (`ampBTreble`): Amp B treble (dB).

#### NAM: PEDAL 1  (before the overdrive and the amp)

- **ON** (switch): Runs the pedal 1 capture (a drive, compressor or other effect), before the overdrive and the amp.
- **LOAD MODEL...** (switch): Loads a neural capture (.nam file) into the pedal 1 slot. WaveNet and LSTM captures are supported. The model file stays where it is: the session remembers its location.
- **CLEAR** (switch): Unloads the pedal 1 model.
- **MATCH LOUDNESS** (switch): Evens out the volume between models using the loudness stored in the file.
- **Input** (`nmP1In`): How hard the signal drives the pedal 1 model, in dB.
- **Output** (`nmP1Out`): The pedal 1 model's output level, in dB.
- **Mix** (`nmP1Mix`): Dry to wet. The dry signal is delayed to match the model's own delay, so the blend stays in phase.

#### NAM: PEDAL 2  (after the cab)

- **ON** (switch): Runs the pedal 2 capture (a drive, compressor or other effect), after the cab.
- **LOAD MODEL...** (switch): Loads a neural capture (.nam file) into the pedal 2 slot. WaveNet and LSTM captures are supported. The model file stays where it is: the session remembers its location.
- **CLEAR** (switch): Unloads the pedal 2 model.
- **MATCH LOUDNESS** (switch): Evens out the volume between models using the loudness stored in the file.
- **STEREO: ONE MODEL PER SIDE** (switch): Off: the stereo signal is summed to mono for one model, whose output goes to both sides (less CPU). On: a second copy of the model runs, one for each side, so a stereo signal stays stereo (about twice the CPU).
- **Input** (`nmP2In`): How hard the signal drives the pedal 2 model, in dB.
- **Output** (`nmP2Out`): The pedal 2 model's output level, in dB.
- **Mix** (`nmP2Mix`): Dry to wet. The dry signal is delayed to match the model's own delay, so the blend stays in phase.

### CAB page

#### CAB BLOCK A

- **ON** (switch): Turns this cab block on or off. Up to four blocks play together.
- **Menu**: Built-in model: the plugin builds the cab from speaker, mic and position. IR file: load your own WAV or AIFF impulse response. Choices: Built-in model ; IR file.
- **INVERT** (switch): Flips this block's polarity (upside down). Alignment sets this automatically when blocks are out of phase.
- **Menu**: Which amp feeds this cab block: Amp A, Amp B, or both added together (about 3 dB quieter each, so the sum is not much louder). Only matters while Amp B is on. Choices: Amp A ; Amp B ; Amp A + B.
- **Menu**: Speaker for the built-in model. The first seven are general purpose; the last six (Modern Scoop, Tight Ceramic, Dark Thump, Hi-Def Light Cone, Smooth Lead, Mid Forward) are aimed at modern metal and djent. Hand-built approximations, not measurements. Choices: Modern Tight ; Vintage Crunch ; Rock Bite ; Smooth Vintage ; Chimey Alnico ; Vintage American ; Hi-Fi Full ; Modern Scoop ; Tight Ceramic ; Dark Thump ; Hi-Def Light Cone ; Smooth Lead ; Mid Forward.
- **Menu**: Mic for the built-in model: six general mics, six aimed at heavier playing (Dynamic Aggressive, Dynamic Punch, Dynamic Scoop, Ribbon Dark, Condenser Warm, and a dynamic + ribbon blend), or Flat for no mic colouring. Choices: Dynamic Bright ; Dynamic Full ; Dynamic Presence ; Ribbon Smooth ; Condenser Detailed ; Flat (no mic) ; Dynamic Aggressive ; Dynamic Punch ; Dynamic Scoop ; Ribbon Dark ; Condenser Warm ; Blend (dynamic + ribbon).
- **Menu**: The cabinet shape used by this block's built-in model: open or closed back, 1x12 to 4x12, straight or angled front, or an oversized ported 4x12 (deeper, tighter lows and cleaner low mids, for downtuned playing). Open backs lose bass; closed cabs add thump near 100 Hz. Choices: 1x12 Open ; 2x12 Open ; 2x12 Closed ; 4x12 Closed ; 4x12 Straight ; 4x12 Angled ; 4x12 Open Back ; Oversized 4x12 Straight (ported) ; Oversized 4x12 Angled (ported).
- **Load IR A...** (switch): Load a WAV or AIFF impulse response for this cab block (used when Source is IR file).
- **Position** (`cabPosA`): Mic position: 0 = centre of the speaker's dust cap (bright, direct), 1 = the cone's edge (darker, warmer).
- **Distance** (`cabDistA`): Mic distance (inches): close = more bass (proximity effect), far = a little less top and a later arrival.
- **Angle** (`cabAngA`): Mic angle (degrees): pointing the mic off-axis loses highs.
- **Level** (`cabLevelA`): Level of this cab block in the mix (dB).
- **Pan** (`cabPanA`): Stereo position of this cab block.
- **Delay** (`cabDelayA`): Delays this block (0-10 ms), used to line up blocks in time. 1 ms is about 34 cm of distance.
- **Low Cut** (`cabLowCutA`): Low cut on this block alone, before the blend.
- **High Cut** (`cabHighCutA`): High cut on this block alone, before the blend.

#### CAB BLOCK B

- **ON** (switch): Turns this cab block on or off. Up to four blocks play together.
- **Menu**: Built-in model: the plugin builds the cab from speaker, mic and position. IR file: load your own WAV or AIFF impulse response. Choices: Built-in model ; IR file.
- **INVERT** (switch): Flips this block's polarity (upside down). Alignment sets this automatically when blocks are out of phase.
- **Menu**: Which amp feeds this cab block: Amp A, Amp B, or both added together (about 3 dB quieter each, so the sum is not much louder). Only matters while Amp B is on. Choices: Amp A ; Amp B ; Amp A + B.
- **Menu**: Speaker for the built-in model. The first seven are general purpose; the last six (Modern Scoop, Tight Ceramic, Dark Thump, Hi-Def Light Cone, Smooth Lead, Mid Forward) are aimed at modern metal and djent. Hand-built approximations, not measurements. Choices: Modern Tight ; Vintage Crunch ; Rock Bite ; Smooth Vintage ; Chimey Alnico ; Vintage American ; Hi-Fi Full ; Modern Scoop ; Tight Ceramic ; Dark Thump ; Hi-Def Light Cone ; Smooth Lead ; Mid Forward.
- **Menu**: Mic for the built-in model: six general mics, six aimed at heavier playing (Dynamic Aggressive, Dynamic Punch, Dynamic Scoop, Ribbon Dark, Condenser Warm, and a dynamic + ribbon blend), or Flat for no mic colouring. Choices: Dynamic Bright ; Dynamic Full ; Dynamic Presence ; Ribbon Smooth ; Condenser Detailed ; Flat (no mic) ; Dynamic Aggressive ; Dynamic Punch ; Dynamic Scoop ; Ribbon Dark ; Condenser Warm ; Blend (dynamic + ribbon).
- **Menu**: The cabinet shape used by this block's built-in model: open or closed back, 1x12 to 4x12, straight or angled front, or an oversized ported 4x12 (deeper, tighter lows and cleaner low mids, for downtuned playing). Open backs lose bass; closed cabs add thump near 100 Hz. Choices: 1x12 Open ; 2x12 Open ; 2x12 Closed ; 4x12 Closed ; 4x12 Straight ; 4x12 Angled ; 4x12 Open Back ; Oversized 4x12 Straight (ported) ; Oversized 4x12 Angled (ported).
- **Load IR B...** (switch): Load a WAV or AIFF impulse response for this cab block (used when Source is IR file).
- **Position** (`cabPosB`): Mic position: 0 = centre of the speaker's dust cap (bright, direct), 1 = the cone's edge (darker, warmer).
- **Distance** (`cabDistB`): Mic distance (inches): close = more bass (proximity effect), far = a little less top and a later arrival.
- **Angle** (`cabAngB`): Mic angle (degrees): pointing the mic off-axis loses highs.
- **Level** (`cabLevelB`): Level of this cab block in the mix (dB).
- **Pan** (`cabPanB`): Stereo position of this cab block.
- **Delay** (`cabDelayB`): Delays this block (0-10 ms), used to line up blocks in time. 1 ms is about 34 cm of distance.
- **Low Cut** (`cabLowCutB`): Low cut on this block alone, before the blend.
- **High Cut** (`cabHighCutB`): High cut on this block alone, before the blend.

#### CAB BLOCK C

- **ON** (switch): Turns this cab block on or off. Up to four blocks play together.
- **Menu**: Built-in model: the plugin builds the cab from speaker, mic and position. IR file: load your own WAV or AIFF impulse response. Choices: Built-in model ; IR file.
- **INVERT** (switch): Flips this block's polarity (upside down). Alignment sets this automatically when blocks are out of phase.
- **Menu**: Which amp feeds this cab block: Amp A, Amp B, or both added together (about 3 dB quieter each, so the sum is not much louder). Only matters while Amp B is on. Choices: Amp A ; Amp B ; Amp A + B.
- **Menu**: Speaker for the built-in model. The first seven are general purpose; the last six (Modern Scoop, Tight Ceramic, Dark Thump, Hi-Def Light Cone, Smooth Lead, Mid Forward) are aimed at modern metal and djent. Hand-built approximations, not measurements. Choices: Modern Tight ; Vintage Crunch ; Rock Bite ; Smooth Vintage ; Chimey Alnico ; Vintage American ; Hi-Fi Full ; Modern Scoop ; Tight Ceramic ; Dark Thump ; Hi-Def Light Cone ; Smooth Lead ; Mid Forward.
- **Menu**: Mic for the built-in model: six general mics, six aimed at heavier playing (Dynamic Aggressive, Dynamic Punch, Dynamic Scoop, Ribbon Dark, Condenser Warm, and a dynamic + ribbon blend), or Flat for no mic colouring. Choices: Dynamic Bright ; Dynamic Full ; Dynamic Presence ; Ribbon Smooth ; Condenser Detailed ; Flat (no mic) ; Dynamic Aggressive ; Dynamic Punch ; Dynamic Scoop ; Ribbon Dark ; Condenser Warm ; Blend (dynamic + ribbon).
- **Menu**: The cabinet shape used by this block's built-in model: open or closed back, 1x12 to 4x12, straight or angled front, or an oversized ported 4x12 (deeper, tighter lows and cleaner low mids, for downtuned playing). Open backs lose bass; closed cabs add thump near 100 Hz. Choices: 1x12 Open ; 2x12 Open ; 2x12 Closed ; 4x12 Closed ; 4x12 Straight ; 4x12 Angled ; 4x12 Open Back ; Oversized 4x12 Straight (ported) ; Oversized 4x12 Angled (ported).
- **Load IR C...** (switch): Load a WAV or AIFF impulse response for this cab block (used when Source is IR file).
- **Position** (`cabPosC`): Mic position: 0 = centre of the speaker's dust cap (bright, direct), 1 = the cone's edge (darker, warmer).
- **Distance** (`cabDistC`): Mic distance (inches): close = more bass (proximity effect), far = a little less top and a later arrival.
- **Angle** (`cabAngC`): Mic angle (degrees): pointing the mic off-axis loses highs.
- **Level** (`cabLevelC`): Level of this cab block in the mix (dB).
- **Pan** (`cabPanC`): Stereo position of this cab block.
- **Delay** (`cabDelayC`): Delays this block (0-10 ms), used to line up blocks in time. 1 ms is about 34 cm of distance.
- **Low Cut** (`cabLowCutC`): Low cut on this block alone, before the blend.
- **High Cut** (`cabHighCutC`): High cut on this block alone, before the blend.

#### CAB BLOCK D

- **ON** (switch): Turns this cab block on or off. Up to four blocks play together.
- **Menu**: Built-in model: the plugin builds the cab from speaker, mic and position. IR file: load your own WAV or AIFF impulse response. Choices: Built-in model ; IR file.
- **INVERT** (switch): Flips this block's polarity (upside down). Alignment sets this automatically when blocks are out of phase.
- **Menu**: Which amp feeds this cab block: Amp A, Amp B, or both added together (about 3 dB quieter each, so the sum is not much louder). Only matters while Amp B is on. Choices: Amp A ; Amp B ; Amp A + B.
- **Menu**: Speaker for the built-in model. The first seven are general purpose; the last six (Modern Scoop, Tight Ceramic, Dark Thump, Hi-Def Light Cone, Smooth Lead, Mid Forward) are aimed at modern metal and djent. Hand-built approximations, not measurements. Choices: Modern Tight ; Vintage Crunch ; Rock Bite ; Smooth Vintage ; Chimey Alnico ; Vintage American ; Hi-Fi Full ; Modern Scoop ; Tight Ceramic ; Dark Thump ; Hi-Def Light Cone ; Smooth Lead ; Mid Forward.
- **Menu**: Mic for the built-in model: six general mics, six aimed at heavier playing (Dynamic Aggressive, Dynamic Punch, Dynamic Scoop, Ribbon Dark, Condenser Warm, and a dynamic + ribbon blend), or Flat for no mic colouring. Choices: Dynamic Bright ; Dynamic Full ; Dynamic Presence ; Ribbon Smooth ; Condenser Detailed ; Flat (no mic) ; Dynamic Aggressive ; Dynamic Punch ; Dynamic Scoop ; Ribbon Dark ; Condenser Warm ; Blend (dynamic + ribbon).
- **Menu**: The cabinet shape used by this block's built-in model: open or closed back, 1x12 to 4x12, straight or angled front, or an oversized ported 4x12 (deeper, tighter lows and cleaner low mids, for downtuned playing). Open backs lose bass; closed cabs add thump near 100 Hz. Choices: 1x12 Open ; 2x12 Open ; 2x12 Closed ; 4x12 Closed ; 4x12 Straight ; 4x12 Angled ; 4x12 Open Back ; Oversized 4x12 Straight (ported) ; Oversized 4x12 Angled (ported).
- **Load IR D...** (switch): Load a WAV or AIFF impulse response for this cab block (used when Source is IR file).
- **Position** (`cabPosD`): Mic position: 0 = centre of the speaker's dust cap (bright, direct), 1 = the cone's edge (darker, warmer).
- **Distance** (`cabDistD`): Mic distance (inches): close = more bass (proximity effect), far = a little less top and a later arrival.
- **Angle** (`cabAngD`): Mic angle (degrees): pointing the mic off-axis loses highs.
- **Level** (`cabLevelD`): Level of this cab block in the mix (dB).
- **Pan** (`cabPanD`): Stereo position of this cab block.
- **Delay** (`cabDelayD`): Delays this block (0-10 ms), used to line up blocks in time. 1 ms is about 34 cm of distance.
- **Low Cut** (`cabLowCutD`): Low cut on this block alone, before the blend.
- **High Cut** (`cabHighCutD`): High cut on this block alone, before the blend.

#### CAB

- **CAB ON** (switch): Cab section on/off. Off = only the amp's own low-pass. (Not used in Acoustic mode.)
- **AUTO ALIGN** (switch): Automatically align the cab blocks (time and polarity) whenever you load an IR file with two or more blocks on.
- **ALIGN NOW** (switch): Line the active cab blocks up with the first one: sets each block's Delay and Invert from the measured offset. Blocks that match poorly are left alone.
- **Room** (`cabRoom`): A short, dark, decaying ambience added to the built-in cab models.
- **Spread** (`cabSpread`): Pushes the cab blocks apart in the stereo field (A left, B right, C a little left, D a little right) on top of each block's Pan. Needs two or more blocks.
- **Low Cut** (`cabLowCut`): Cab-wide low cut after the cab, to remove rumble.
- **High Cut** (`cabHighCut`): Cab-wide high cut after the cab, to remove fizz.
- **Tilt** (`cabTilt`): Makes the whole cab darker or brighter around 800 Hz.
- **Reso** (`cabReso`): A low resonance peak (dB) for speaker-cabinet thump, at the Reso Hz frequency. 0 = off.
- **Reso Hz** (`cabResoHz`): Frequency of the cab resonance peak.
- **Air** (`cabAir`): Gentle high shelf from about 6 kHz (dB).

#### IR ALIGNMENT

#### MIC POSITION

#### BLOCK SHAPING  (per cab block)

- **BLOCK A** (switch): Show the shaping controls of cab block A. This doesn't turn the block on.
- **BLOCK B** (switch): Show the shaping controls of cab block B. This doesn't turn the block on.
- **BLOCK C** (switch): Show the shaping controls of cab block C. This doesn't turn the block on.
- **BLOCK D** (switch): Show the shaping controls of cab block D. This doesn't turn the block on.
- **Low** (`cabEq1A`): Low shelf on this block (about 110 Hz), in dB.
- **Low Mid** (`cabEq2A`): Low-mid peak on this block (about 330 Hz), in dB.
- **Mid** (`cabEq3A`): Mid peak on this block (about 1 kHz), in dB.
- **High Mid** (`cabEq4A`): High-mid peak on this block (about 2.8 kHz), in dB.
- **High** (`cabEq5A`): High shelf on this block (about 6.5 kHz), in dB.
- **Reso** (`cabRsoA`): Extra low-frequency resonance on this block, at the shared Reso Hz, in dB.
- **Phase** (`cabPhsA`): All-pass phase shift on this block: moves the phase around 700 Hz to 5.6 kHz without changing the tone. Use it to tune how blocks combine.
- **Low** (`cabEq1B`): Low shelf on this block (about 110 Hz), in dB.
- **Low Mid** (`cabEq2B`): Low-mid peak on this block (about 330 Hz), in dB.
- **Mid** (`cabEq3B`): Mid peak on this block (about 1 kHz), in dB.
- **High Mid** (`cabEq4B`): High-mid peak on this block (about 2.8 kHz), in dB.
- **High** (`cabEq5B`): High shelf on this block (about 6.5 kHz), in dB.
- **Reso** (`cabRsoB`): Extra low-frequency resonance on this block, at the shared Reso Hz, in dB.
- **Phase** (`cabPhsB`): All-pass phase shift on this block: moves the phase around 700 Hz to 5.6 kHz without changing the tone. Use it to tune how blocks combine.
- **Low** (`cabEq1C`): Low shelf on this block (about 110 Hz), in dB.
- **Low Mid** (`cabEq2C`): Low-mid peak on this block (about 330 Hz), in dB.
- **Mid** (`cabEq3C`): Mid peak on this block (about 1 kHz), in dB.
- **High Mid** (`cabEq4C`): High-mid peak on this block (about 2.8 kHz), in dB.
- **High** (`cabEq5C`): High shelf on this block (about 6.5 kHz), in dB.
- **Reso** (`cabRsoC`): Extra low-frequency resonance on this block, at the shared Reso Hz, in dB.
- **Phase** (`cabPhsC`): All-pass phase shift on this block: moves the phase around 700 Hz to 5.6 kHz without changing the tone. Use it to tune how blocks combine.
- **Low** (`cabEq1D`): Low shelf on this block (about 110 Hz), in dB.
- **Low Mid** (`cabEq2D`): Low-mid peak on this block (about 330 Hz), in dB.
- **Mid** (`cabEq3D`): Mid peak on this block (about 1 kHz), in dB.
- **High Mid** (`cabEq4D`): High-mid peak on this block (about 2.8 kHz), in dB.
- **High** (`cabEq5D`): High shelf on this block (about 6.5 kHz), in dB.
- **Reso** (`cabRsoD`): Extra low-frequency resonance on this block, at the shared Reso Hz, in dB.
- **Phase** (`cabPhsD`): All-pass phase shift on this block: moves the phase around 700 Hz to 5.6 kHz without changing the tone. Use it to tune how blocks combine.

#### SPEAKER DYNAMICS

- **Menu**: Tight (modern): quick and controlled. Vintage (spongy): slower, softer and more compressed. Choices: Tight (modern) ; Vintage (spongy).
- **Amount** (`cabDyn`): Speaker dynamics: how much the speakers compress and break up at high levels (bass limiting, cone breakup, heating and a duller top). 0 is off, which is the previous sound.

#### EXPORT CAB AS IR

- **EXPORT WAV...** (switch): Choose where to save the WAV. The cab blocks must be on and have an IR; file IRs are exported at their analysed length.

### EQ page

#### POST EQ

- **EQ ON** (switch): Post EQ on/off. It sits after the cab, before the textures, delays and reverb.
- **Menu**: Graphic: fixed-width bands. Mod: the Q knob changes every band's width together. Choices: Graphic ; Mod (custom Q).
- **Menu**: How steep the low cut and high cut are (12 or 24 dB per octave). Choices: 12 dB/oct ; 24 dB/oct.
- **AUTO LEVEL** (switch): Auto level: a slow level follower keeps the Post EQ's output at the loudness of its input, so boosting or cutting bands changes the tone, not the volume. The Level knob still trims on top of it.
- **MATCH...** (switch): Tone matching: opens a window where you load a reference recording, play your guitar, and have the Post EQ fitted so your tone's overall balance moves toward the reference.
- **Q (Mod)** (`eqQ`): Mod mode: width of all bands (higher = narrower).
- **Level** (`eqLevel`): Overall level after the EQ (dB).
- **Low Cut** (`eqLowCut`): Post EQ high-pass (Hz): removes everything below this.
- **High Cut** (`eqHighCut`): Post EQ low-pass (Hz): removes everything above this.

#### SPECTRUM  (what you are hearing)

- **OUTPUT** (switch): Show the final output spectrum.
- **PRE-EQ** (switch): Show the spectrum just before the Post EQ (after the amp and cab).
- **BOTH** (switch): Show both, so you can see what the Post EQ changes.
- **TILT** (switch): Tilts the display up by 4.5 dB per octave so a typical guitar sound looks flat. Off = true spectrum.
- **HOLD** (switch): Draws a thin line that holds recent peaks and slowly falls.

### DELAY page

#### PRE-AMP DELAY

- **DELAY ON** (switch): Pre-amp delay on/off. Its repeats go through the overdrive and amp, so they get compressed and distorted.
- **Menu**: Delay character. Digital: clean and bright. Analog: darker, bucket-brigade style, with soft saturation and a slight wobble. Tape: dark and saturated with wow, flutter and drift. Lo-Fi: dark with reduced sample rate and bit depth. The Lo-Fi and Warble knobs add to the mode. Choices: Digital ; Analog ; Tape ; Lo-Fi.
- **Menu**: Free (ms) uses the Time knob. A note value sets the delay time from the tempo instead (and greys out Time). Choices: Free (ms) ; 1/2 ; Dotted 1/4 ; 1/4 ; 1/4 triplet ; Dotted 1/8 ; 1/8 ; 1/8 triplet ; Dotted 1/16 ; 1/16 ; 1/16 triplet.
- **Mix** (`preDelayMix`): How much of the repeats you hear. 0 = off.
- **Time** (`preDelayTime`): Delay time in milliseconds (used when Note is Free).
- **Feedback** (`preDelayFeedback`): How many repeats. Keep it below about 0.8 for a clean decay.
- **Lo-Fi** (`preDelayLofi`): Darker, crunchier, more degraded repeats (lower sample rate and bit depth). Only affects the repeats.
- **Warble** (`preDelayWarble`): Tape-style pitch wobble on the repeats: a slow wow, a faster flutter and a little drift.

#### PRE-AMP DELAY: DUCK, MODULATION, DIFFUSION, HOLD, TAPS

- **HOLD** (switch): Hold the delay. Freeze: the delay stops listening and loops what it holds. Infinite: the repeats never die away, but new playing still joins in. Switch it off to let the repeats fade again.
- **Menu**: Freeze: the delay loops what it holds and ignores new playing. Infinite: the repeats never die away, and new playing keeps adding to them. Choices: Freeze ; Infinite.
- **Menu**: Multi-tap: extra repeats at fractions of the delay time, ahead of the main repeat. The main repeat and the feedback stay at the full time. Not available as a separate sound in Reverse. Choices: Off ; Half + full ; Thirds ; Quarters ; Off-beat ; Dotted cluster.
- **Duck** (`preDelayDuck`): Ducking: the repeats of the pre-amp delay dip while you play and swell back when you stop, so they stay out of the way of your notes. 0 = off.
- **Duck Release** (`preDelayDuckRel`): (see above)
- **Mod Depth** (`preDelayModDepth`): Slowly wobbles the delay time, so the repeats shimmer and drift like a chorus. 0 = off.
- **Mod Rate** (`preDelayModRate`): (see above)
- **Diffusion** (`preDelayDiffuse`): Smears each repeat, so the echoes bloom into a soft wash instead of staying clean and distinct. 0 = off.
- **Tap Level** (`preDelayTapLevel`): (see above)

#### POST-AMP DELAY

- **DELAY ON** (switch): Post-amp delay on/off. Its repeats are clean copies of the amped sound, before the reverb.
- **Menu**: Delay character. Digital: clean and bright. Analog: darker, bucket-brigade style, with soft saturation and a slight wobble. Tape: dark and saturated with wow, flutter and drift. Lo-Fi: dark with reduced sample rate and bit depth. The Lo-Fi and Warble knobs add to the mode. Choices: Digital ; Analog ; Tape ; Lo-Fi.
- **Menu**: Free (ms) uses the Time knob. A note value sets the delay time from the tempo instead (and greys out Time). Choices: Free (ms) ; 1/2 ; Dotted 1/4 ; 1/4 ; 1/4 triplet ; Dotted 1/8 ; 1/8 ; 1/8 triplet ; Dotted 1/16 ; 1/16 ; 1/16 triplet.
- **Menu**: How the repeats are spread across the stereo field. Stereo: both sides echo together. Ping-Pong: the repeats bounce between left and right. Dual: the right side runs at a different time to the left. Needs a stereo signal; the pre-amp delay is always mono. Choices: Stereo (same time) ; Ping-Pong (left first) ; Ping-Pong (right first) ; Dual (right offset).
- **Menu**: Forward: normal repeats. Reverse: each repeat is played backwards, swelling up to the delay time. Combine it with Ping-Pong in the menu above for reversed echoes bouncing between the sides. (Warble is ignored in Reverse.) Choices: Forward ; Reverse.
- **Mix** (`postDelayMix`): How much of the repeats you hear. 0 = off.
- **Time** (`postDelayTime`): Delay time in milliseconds (used when Note is Free).
- **Feedback** (`postDelayFeedback`): How many repeats. Keep it below about 0.8 for a clean decay.
- **Lo-Fi** (`postDelayLofi`): Darker, crunchier, more degraded repeats (lower sample rate and bit depth). Only affects the repeats.
- **Warble** (`postDelayWarble`): Tape-style pitch wobble on the repeats: a slow wow, a faster flutter and a little drift.
- **R Offset** (`postDelayOffset`): Dual mode only. How much longer the right delay is than the left: 0 = the same, 0.5 = 1.5x (a dotted feel against the left), 1 = 2x.
- **Spread** (`postDelaySpread`): How far apart the left and right repeats are. 1 = hard apart, 0 = both in the centre. Useful for a softer ping-pong.

#### POST-AMP DELAY: DUCK, MODULATION, DIFFUSION, HOLD, TAPS

- **HOLD** (switch): Hold the delay. Freeze: the delay stops listening and loops what it holds. Infinite: the repeats never die away, but new playing still joins in. Switch it off to let the repeats fade again.
- **Menu**: Freeze: the delay loops what it holds and ignores new playing. Infinite: the repeats never die away, and new playing keeps adding to them. Choices: Freeze ; Infinite.
- **Menu**: Multi-tap: extra repeats at fractions of the delay time, ahead of the main repeat. The main repeat and the feedback stay at the full time. Not available as a separate sound in Reverse. Choices: Off ; Half + full ; Thirds ; Quarters ; Off-beat ; Dotted cluster.
- **Duck** (`postDelayDuck`): Ducking: the repeats of the post-amp delay dip while you play and swell back when you stop, so they stay out of the way of your notes. 0 = off.
- **Duck Release** (`postDelayDuckRel`): (see above)
- **Mod Depth** (`postDelayModDepth`): Slowly wobbles the delay time, so the repeats shimmer and drift like a chorus. 0 = off.
- **Mod Rate** (`postDelayModRate`): (see above)
- **Diffusion** (`postDelayDiffuse`): Smears each repeat, so the echoes bloom into a soft wash instead of staying clean and distinct. 0 = off.
- **Tap Level** (`postDelayTapLevel`): (see above)

### SCAPES page

#### GRANULAR CLOUD

- **GRAINS ON** (switch): Granular cloud on/off. It records a 4-second loop and plays back lots of tiny overlapping grains. It sits after the Post EQ.
- **FREEZE** (switch): Stops recording, so the cloud keeps playing whatever was in the loop. Play something, then freeze it.
- **Menu**: Snaps the random grain pitches to a scale (instead of any pitch). It works from Pitch Rand of 1 st or more: Pitch Rand sets how far up and down the grains can go. Free = no snapping. Choices: Free ; Octaves ; Fifths ; Major pentatonic ; Minor pentatonic ; Major ; Minor ; Whole tone.
- **Menu**: The fade of each grain. Smooth: even fade in and out. Swell: slow fade in, quick end (grains swell). Pluck: sharp start, long decay. Flat: almost no fade (percussive). Choices: Smooth ; Swell ; Pluck ; Flat.
- **Menu**: Free uses the Size knob. A note value makes every grain exactly that long at the tempo. Choices: Size: Free (ms) ; Size: 1/2 ; Size: Dotted 1/4 ; Size: 1/4 ; Size: 1/4 triplet ; Size: Dotted 1/8 ; Size: 1/8 ; Size: 1/8 triplet ; Size: Dotted 1/16 ; Size: 1/16 ; Size: 1/16 triplet.
- **Menu**: Free uses the Density knob. A note value starts one grain per note length, so the cloud pulses with the tempo. Choices: Rate: Free (/s) ; Rate: 1/2 ; Rate: Dotted 1/4 ; Rate: 1/4 ; Rate: 1/4 triplet ; Rate: Dotted 1/8 ; Rate: 1/8 ; Rate: 1/8 triplet ; Rate: Dotted 1/16 ; Rate: 1/16 ; Rate: 1/16 triplet.
- **Menu**: Free uses the Position knob. A note value reads grains from exactly that many beats back in the loop. Choices: Pos: Free (ms) ; Pos: 1/2 ; Pos: Dotted 1/4 ; Pos: 1/4 ; Pos: 1/4 triplet ; Pos: Dotted 1/8 ; Pos: 1/8 ; Pos: 1/8 triplet ; Pos: Dotted 1/16 ; Pos: 1/16 ; Pos: 1/16 triplet.
- **Size** (`granSize`): Grain length (ms). Short = grainy and glitchy; long = smooth and pad-like.
- **Density** (`granDensity`): How many grains start each second. More = thicker cloud.
- **Position** (`granPos`): How far back in the loop grains start (ms).
- **Spray** (`granSpray`): Scatters the grain start points further back, up to 1.5 seconds.
- **Pitch** (`granPitch`): Pitch of the grains in semitones. +12 with long grains gives a shimmer-like octave-up cloud.
- **Scan** (`granScan`): Slowly sweeps the point the grains are taken from, back and forth through the last 1.2 seconds, so the texture keeps evolving. 0 = off.
- **Pitch Rand** (`granPitchRand`): Random pitch variation per grain (up to +-12 semitones) for a detuned, shimmery wash.
- **Reverse** (`granReverse`): Chance that a grain plays backwards.
- **Width** (`granWidth`): How far grains are scattered left and right.
- **Feedback** (`granFeedback`): Feeds the cloud back into the recording, so it evolves and builds.
- **Tone** (`granTone`): Rolls off the highs of the cloud. Fully up = no filtering.
- **Mix** (`granMix`): Crossfade between your dry signal (0) and the cloud (1).

#### TREMOLO / AUTO-PAN

- **TREMOLO ON** (switch): Tremolo / auto-pan on/off. It sits after the Post EQ.
- **Menu**: Tremolo: both sides pulse together. Stereo: the left and right sides alternate (an auto-pan feel). Choices: Tremolo (both sides) ; Stereo (sides alternate).
- **Menu**: Shape of the pulse: sine (smooth), triangle, or soft square (choppy). Choices: Sine ; Triangle ; Square (soft).
- **Menu**: Free uses the Rate knob. A note value locks the speed to the tempo (one note length per cycle). Choices: Free (Hz) ; 1/2 ; Dotted 1/4 ; 1/4 ; 1/4 triplet ; Dotted 1/8 ; 1/8 ; 1/8 triplet ; Dotted 1/16 ; 1/16 ; 1/16 triplet.
- **Rate** (`tremRate`): Speed in Hz (used when Note is Free).
- **Depth** (`tremDepth`): How deep the volume dips. 1 = down to silence.

#### WOBBLE  (TREMOLO-ARM VIBRATO)

- **WOBBLE ON** (switch): Wobble on/off: pitch vibrato like a loosely held tremolo arm.
- **Menu**: Free uses the Rate knob. A note value locks the wobble to the tempo (one note length per cycle). Choices: Free (Hz) ; 1/2 ; Dotted 1/4 ; 1/4 ; 1/4 triplet ; Dotted 1/8 ; 1/8 ; 1/8 triplet ; Dotted 1/16 ; 1/16 ; 1/16 triplet.
- **Rate** (`wobRate`): Speed of the vibrato (Hz).
- **Depth** (`wobDepth`): How far the pitch moves. Small values give a few cents of drift; high values sound seasick.
- **Drift** (`wobDrift`): Adds a slow random drift to the pitch, like an unsteady hand.
- **Mix** (`wobMix`): 1 = pure vibrato; lower blends in the dry signal for a chorus-like effect.

#### SWELL  (VOLUME FADE-IN)

- **SWELL ON** (switch): Swell on/off: automatically fades in each new note (volume swell / slow gear).
- **Menu**: Free uses the Fade-In knob. A note value makes each swell last exactly that note length at the tempo. Choices: Fade: Free (ms) ; Fade: 1/2 ; Fade: Dotted 1/4 ; Fade: 1/4 ; Fade: 1/4 triplet ; Fade: Dotted 1/8 ; Fade: 1/8 ; Fade: 1/8 triplet ; Fade: Dotted 1/16 ; Fade: 1/16 ; Fade: 1/16 triplet.
- **Fade-In** (`swellAttack`): How long each note takes to fade in (ms).
- **Sensitivity** (`swellSens`): How easily it detects a new note. Higher catches softer notes; lower ignores them.

#### LO-FI WARBLE

- **LO-FI ON** (switch): Switches the Lo-Fi Warble pedal on. It sits just before the overdrive.
- **Menu**: The shape of the pitch warble: Sine (smooth), Triangle, or Random (a new random pitch each cycle, like a worn tape). Choices: Sine ; Triangle ; Random.
- **Rate** (`lfRate`): How fast the pitch warbles, in Hz.
- **Depth** (`lfDepth`): How far the pitch swings. Low = gentle chorus-like movement, high = a seasick vibrato.
- **Drift** (`lfDrift`): Random pitch wander and a little fast flutter, like an unsteady tape transport.
- **Tone** (`lfTone`): The top end of the worn sound: turn down for dull and muffled, up for brighter.
- **Dirt** (`lfDirt`): Crunch from soft clipping plus bit and sample-rate crushing. High settings sound gritty and digital.
- **Noise** (`lfNoise`): Hiss and crackle, like an old recording.
- **Dropouts** (`lfDropouts`): Random short dips in level, like a worn tape losing contact. 0 = none; more = more often and deeper.
- **Mix** (`lfMix`): Dry to wet. At 100% you hear only the warbling signal (pure vibrato); lower values blend in the clean signal for a chorus.
- **Level** (`lfLevel`): The pedal's output level, in dB.

#### NOISE PEDAL

- **NOISE PEDAL ON** (switch): Master switch for the Noise pedal. When off, none of its footswitches do anything.
- **OCT UP** (switch): Octave up while held (or latched on). The pedal is a pitch shifter for harsh, dissonant sounds.
- **2 OCT UP** (switch): Two octaves up while held (or latched on). Wins over OCT UP if both are on.
- **CLASH** (switch): Adds clashing pitch-shifted voices above your note (the scene menu picks which). Hold or latch it on.
- **RING** (switch): Ring modulator on while held or latched: replaces your harmonics with metallic sum and difference tones.
- **LATCH** (switch): Off: the OCT UP switch is on only while held. On: click once to turn it on, again to turn it off.
- **LATCH** (switch): Off: the 2 OCT UP switch is on only while held. On: click once to turn it on, again to turn it off.
- **LATCH** (switch): Off: the CLASH switch is on only while held. On: click once to turn it on, again to turn it off.
- **LATCH** (switch): Off: the RING switch is on only while held. On: click once to turn it on, again to turn it off.
- **OCTAVE** (switch): Show the octave settings (Panic, Chaos). This doesn't turn the effect on.
- **CLASH** (switch): Show the Clash settings (scene, Drive, Blend). This doesn't turn the effect on.
- **RING MOD** (switch): Show the ring modulator settings. This doesn't turn the effect on.
- **Menu**: Which clashing intervals Clash uses: Bright (tritone + flat ninth), Dark (minor second + tritone), Full (all three, maximum dissonance). Choices: Bright (tritone) ; Dark (minor 2nd) ; Full (all three).
- **Menu**: Ring = plain multiplication (original notes mostly disappear). AM = amplitude modulation (original notes stay, with a shimmering layer). Choices: Ring ; AM.
- **Menu**: Shape of the ring modulator's carrier: Sine is cleanest, Square buzzier, Triangle in between. Choices: Sine ; Square ; Triangle.
- **Rise** (`noiseRise`): Glide time. For the octaves: how long the pitch slides up and down. For Clash and Ring Mod: how fast they fade in and out. Low = instant, high = a slow swell.
- **Speed** (`noiseSpeed`): Rate of the wobble and random jumps: the octave wobble, the Clash detune, and the ring mod's Wobble.
- **Panic** (`noisePanic`): Octave pages: depth of a smooth, warbling pitch wobble (up to about 1.5 semitones).
- **Chaos** (`noiseChaos`): Octave pages: depth of random, stepped pitch jumps (up to about 7 semitones) for glitchy, dissonant sounds.
- **Drive** (`noiseNuke`): How hard the signal is pushed into soft clipping before the Clash pitch engine. Higher = more aggressive and messier.
- **Blend** (`noiseSicken`): Clash wet/dry mix. 0 = only your dry signal, 1 = only the Clash voices.
- **Freq** (`ringFreq`): Carrier frequency, 1 Hz to 5 kHz. About 1-20 Hz is a tremolo chop, 50-500 Hz growl and metal, higher gives bells.
- **Mix** (`ringMix`): Ring modulator wet/dry. 0 = only your dry signal.
- **Wobble** (`ringWobble`): Moves the carrier up and down by up to an octave, at the Speed rate. 0 = steady.

### POST FX page

#### REVERB

- **REVERB ON** (switch): Reverb on/off. It's the last effect before the output level.
- **SHIMMER** (switch): Shimmer on/off: pitch-shifted sound fed through the reverb for a rising, ethereal wash.
- **Menu**: The interval the shimmer is shifted by, from an octave down to two octaves up, including two-voice choices such as octave + fifth. Choices: +12 (octave) ; +7 (fifth) ; +24 (two octaves) ; -12 (octave down) ; +12 & +7 (octave + fifth) ; +12 & -12 (up and down) ; +5 (fourth) ; +19 (octave + fifth up).
- **FREEZE** (switch): Holds the reverb tail so it sustains indefinitely.
- **Menu**: Reverb character. Classic is the original simple reverb. Room is short and natural, Hall is large and smooth, Plate is bright and dense, Ambient is long, lush and slowly moving, Cathedral is huge and dark, Spring is short and boingy, Soft Plate is dense and gently moving, Tumble has echoes that tumble over each other, Abyss is huge, dark and drifting. All are hand-tuned approximations. Size, Damp and Width work on every type. Choices: Classic ; Room ; Hall ; Plate ; Ambient ; Cathedral ; Spring ; Soft Plate ; Tumble ; Abyss.
- **Menu**: Free uses the Pre-Dly knob. A note value sets the pre-delay to that note length (up to 250 ms). Choices: Pre-dly: Free (ms) ; Pre-dly: 1/2 ; Pre-dly: Dotted 1/4 ; Pre-dly: 1/4 ; Pre-dly: 1/4 triplet ; Pre-dly: Dotted 1/8 ; Pre-dly: 1/8 ; Pre-dly: 1/8 triplet ; Pre-dly: Dotted 1/16 ; Pre-dly: 1/16 ; Pre-dly: 1/16 triplet.
- **Menu**: What gets pitch-shifted. Feedback: the reverb's own tail, so the shimmer blooms and climbs with every trip around the loop. Input: only your playing, shifted once into the reverb, for a clean octave pad that does not build up. Both: the two together. Choices: Feedback (blooming) ; Input (no build-up) ; Both.
- **Mix** (`reverbMix`): Wet/dry amount of reverb.
- **Decay** (`reverbSize`): Decay time in seconds (how long the tail takes to fade by 60 dB), from 0.1 to 30 s. Longer decays also use a bigger space. The Classic type tops out around 12 s; the other types go all the way to 30 s.
- **Pre-Dly** (`reverbPreDelay`): Gap before the reverb starts (ms). Longer keeps the dry note clearer.
- **Damp** (`reverbDamp`): How quickly the highs die away in the tail. Higher = darker, softer tail.
- **Width** (`reverbWidth`): Stereo width of the reverb.
- **Low Cut** (`reverbLowCut`): Removes lows from the reverb (Hz) so it doesn't get muddy.
- **High Cut** (`reverbHighCut`): Removes highs from the reverb (Hz) for a darker wash.
- **Duck** (`reverbDuck`): Lowers the reverb while you play and lets it bloom when you stop.
- **Shimmer** (`shimmerAmount`): How much pitch-shifted sound is fed into the reverb.
- **Tone** (`shimmerTone`): Tone (Hz) of the shimmer signal: lower = darker.
- **Detune** (`shimmerDetune`): Splits the shimmer into two voices a few cents either side of the pitch, each drifting slowly. Adds width and a choir-like softness, and hides the grain of the pitch shifter. 0 = one steady voice.

#### REVERB SHAPING

- **Menu**: Smooth: any pitch in between. Semitones: whole notes. Fifths and octaves: only fifths and octaves. Octaves: only octaves. Choices: Smooth ; Semitones ; Fifths and octaves ; Octaves.
- **Menu**: The pitch of the Symphony layer. Choices: Octave up ; Octave down ; Fifth up ; Two octaves up.
- **Diffuse** (`reverbDiffuse`): Pick another reverb type.
- **Space** (`reverbSpace`): Pick another reverb type.
- **Mod** (`reverbMod`): Adds slow movement: a gentle three-voice chorus on the reverb's output, and a slight sway in the tail. 0 = none. Works on every type.
- **Drag** (`reverbDrag`): Pick another reverb type.
- **Bloom** (`reverbBloom`): Delays the build-up: the reverb swells in behind each note instead of starting at once, like a slow-attack pad. 0 = off. Works on every type.
- **Symphony** (`reverbSymph`): Adds a pitch-shifted layer to the reverb: a second voice in the tail an octave up, an octave down, a fifth up or two octaves up (see the menu). 0 = none. Works on every type.

#### OUTPUT

- **LIMITER** (switch): Final limiter on/off. It sits after the Output level and stops the signal going over the ceiling, to avoid clipping. With look-ahead it adds 1 ms of delay (2 ms in High Quality, none in Low Latency), which is reported to your DAW.
- **MONO** (switch): Sums the output to mono (both sides get the same signal). Use it to check how a wide sound folds down on a mono system.
- **Level** (`outGain`): Final output level (dB). Keep it low while you raise gain knobs.
- **Ceiling** (`limCeil`): The highest level the limiter lets through (dB). It limits sample peaks, not true peaks, so leave a little headroom (-1 dB is a safe start).
- **Release** (`limRel`): How fast the limiter lets go after a peak (ms). Short = louder but can sound pumpy or gritty; long = smoother.

#### DOUBLER  (DOUBLE-TRACKING)

- **DOUBLER ON** (switch): Doubler on/off: imitates double-tracking by adding two slightly delayed, drifting copies spread left and right. It sits after the textures, before the post-amp delay.
- **Time** (`dblTime`): Base delay of the two copies (ms), 5 to 60. About 15-30 ms sounds like a second take; longer starts to sound like an echo; below about 10 ms it turns into a thick, flanger-like thickening.
- **Variation** (`dblVar`): How much the copies drift in time at random, like two takes that never line up exactly.
- **Spread** (`dblSpread`): How far the two copies are pushed left and right. 0 = both in the middle.
- **Mix** (`dblMix`): How much of the doubled copies you hear against the original.

#### WIDENER

- **WIDENER ON** (switch): Widener on/off. It sits after the reverb, just before the output level. It needs a stereo signal (from the doubler, stereo cab blocks or the reverb) to have anything to widen.
- **Menu**: Three flavours of the widening (my own design): Classic, Desktop (a little more low-mid body) and Rack (subtler). Choices: Wide: Classic ; Wide: Desktop ; Wide: Rack.
- **Wide** (`widWow`): How much the stereo image is widened: the left-right difference signal is lifted and made louder.
- **Space** (`widSpace`): Smears the difference signal in time and phase for a bigger, more diffuse space. Too much can sound unfocused.
- **Bass Lift** (`widBass`): Bass Lift: adds harmonics of the low notes so bass is audible on small speakers. 0 = off.
- **Bass Freq** (`widBassHz`): The bass frequency Bass Lift works on.
- **Mono Bass** (`widMono`): Frequencies below this are kept in the centre (taken out of the widened part), so the bass stays solid and mono-compatible.
- **Mix** (`widMix`): Wet/dry blend of the widener.
- **Level** (`widLevel`): Level after the widener (dB), to compensate for loudness changes.

## Amp mode (v87, default Circuit-derived since v88)

Each amp has one Mode menu (Amp A on the row under Bright; Amp B in the same place on its tab).

- **Classic:** the amp as voiced by ear. Feel and Presence nudge the tone; the sag has one time constant.
- **Circuit-derived:** the preamp stages and power stage clip with curves taken from tube-circuit simulation (grid blocking, power-tube shapes), and the power section is worked out from circuit values: the feedback loop with the output stage inside it, the Presence control as cut feedback, output transformer and speaker impedance, a two-part supply droop (the Sag knob scales it), gentle output-transformer saturation, and preamp corner frequencies and bright cap from component values (American combo and 50 W British models). Levels are matched to Classic on a test signal. In my timing test it costs about the same CPU as Classic. The Acoustic, Glass Clean and Digital models ignore it.
- **Full circuit (Lead 50, heavy):** Amp A only. A complete simulation of one 50 watt master-volume lead amp. The Master knob appears. Gain is the preamp volume; Bass, Treble and Mids (the average of Low Mid and High Mid) act like the real controls and interact; Bias, Sag, the preamp tube and the power tube change the circuit. It runs at twice the host rate and measured about 45% of real time, roughly 20 times the CPU of the other modes. The amp model menu, Voicing, Stack, Feel, Deep, Auto Level and the Metal controls are ignored while it is on. Pedals before the amp and the cab section work as usual.

What v87 removed: Power (Full/Half/Low), Class, Rectifier, Dual voice and the Metal panel's Sag knob (Power Drive and Sag cover the same ground), and the American and British entries of the Stack menu (the passive stacks cover them).

## Circuit-derived levels (v83, now the Circuit-derived mode)

On the AMP page, next to the Circuit Amp and Stage Model menus, the Derived menu chooses how much of the amp comes from circuit values instead of by-ear numbers. Amp B has its own in the advanced rows.

- **Derived: off (classic):** the amp as voiced. The Feel switch and Presence knob nudge the tone.
- **Derived: power section:** the nudges are replaced by the response of the power amp: the negative-feedback network, the presence control, the output transformer and the speaker's impedance (including its bass resonance). Feel sets how much feedback there is (loose, normal, tight). Presence is milder than the classic shelf: on the real amps it only cuts feedback at the top.
- **Derived: power + supply:** adds a supply droop with a fast part (the reservoir, tens of milliseconds) and a slow part (the filter stages, a few hundred), and a gentle rounding of loud low notes (output transformer). The Sag knob scales it, Power (Half, Low) increases it and Rectifier: Silicon stiffens it.
- **Derived: power + supply + voicing:** also uses the corner frequencies of the cathode bypass and coupling capacitors, and the bright capacitor's shelf, worked out from component values. Only the American combo and 50 W British models have this; for the others it is the same as the level below. It can sound looser in the low end than classic, because the real corners are low.

Levels are matched to classic on a test signal. CPU is unchanged. The Acoustic, Glass Clean and Digital models ignore it.

## Optical tremolo and Spring Drive (v83)

Tremolo Shape has an Optical entry (bulb and photocell). The pulse is rounded and lopsided, and gets shallower at fast rates, as in an optical vibrato circuit. Spring Drive (reverb shaping panel) only acts on the Spring reverb type: it pushes the spring's driver, so hard notes splash and drip, the lows thin out and the top rolls off. At 0 it is off.

## Circuit Amp (v79, now the Full circuit mode)

On the AMP page, the row under Bright holds the Circuit Amp menu. "Circuit Amp: Lead 50 (full circuit)" replaces the amp model with a full simulation of a 50 watt master-volume lead amp: preamp stages, bright volume, tone stack, master volume, phase inverter, EL34 power tubes, output transformer and a sagging supply.

- Gain is the preamp volume. Master (next to Presence) sets how hard the power tubes are driven; for the classic sound turn Gain up and Master down, then raise Master for more power-stage grit and sag.
- Bass, Treble and Mids work like the real controls, so they affect each other; Mids is the average of Low Mid and High Mid.
- Bias, Sag, the preamp tube, the power tube and the rectifier choice (Silicon = stiff supply) change the circuit itself.
- It uses more CPU than the normal amps. If your computer struggles, turn it off or raise the audio buffer size.
- The amp model menu, Voicing, Stack, Feel, Watt, Class, Deep, Auto Level and the Metal controls are ignored while it is on. Pedals before the amp and the cab section work as usual.
- Off (the default) leaves everything as it was.

## Stage Model (v80, now part of the Circuit-derived mode)

On the AMP page, next to the Circuit Amp menu, the Stage Model menu chooses how the preamp stages and power stage clip. "classic" is the original. "circuit-derived" uses clipping curves taken from tube-circuit simulation: each gain stage clips differently on its two sides (more even harmonics, more like a real tube), loud notes briefly pull the stage down and recover, and the power stage follows the Power Tube menu. The choice of amp model decides which kind of stage each amp uses. Amp B has its own Stage Model menu in its advanced rows. It does not use more CPU. The acoustic, glass clean and digital amp models ignore it.

## Passive tone stacks (v81)

The Stack menu on the AMP page (and Amp B's) has five "Passive stack" entries: American combo, British 50 W, British lead, American lead and chime. Unlike the older Stack settings, which only nudge four EQ filters, these solve a real bass / mid / treble circuit from its component values.

- Bass, Treble and Mid (the average of Low Mid and High Mid) work like the real knobs and interact: more treble thins the mids, a low Mid knob digs a deep notch, and the Bass knob changes how much low end gets through at all.
- At the middle of all three knobs the level is matched to the other stacks. The American stacks sound scooped at the middle; the British ones are flatter with a dip around 500 to 800 Hz.
- The amp model's own tone shaping is not used while a passive stack is on.
- No extra CPU to speak of.

## Distortion circuit mode and wah voice (v91)
Overdrive panel, Distortion style: switch on CIRCUIT MODE to use the circuit-based Distortion. It has no clean path and is loud, so turn on Auto Level. Wah panel: the Voice menu picks Generic (your Heel and Toe) or Classic circuit (a fixed 450 Hz to 1.6 kHz sweep).

## Stacking two drives, and the pedal changes (v93)
**Drive 2.** The GAIN page has a second OVERDRIVE block, "DRIVE 2", a full copy of the first (same styles, knobs and Auto Level). It starts off. Its **Route** menu picks how it sits with Drive 1 when both are on: **Series** (Drive 1 feeds Drive 2, which feeds the amp) or **Parallel** (both get the same input and their outputs are added, each at -3 dB). With only one drive on it simply feeds the amp, and Drive 2 alone sounds the same as Drive 1 alone with the same settings (checked bit-identical). Using both adds the delay of one more oversampling pass (a few samples); it is not compensated.

**Fuzz 2.** The Fuzz section also has a second block ("FUZZ 2", starts off), a full copy of the first with a **Route** menu: Series (Fuzz 1 into Fuzz 2) or Parallel (both from the same input, added at -3 dB each; two fuzzes added together can partly cancel and sound hollow). Both sit in the Fuzz position of the effect order. Real fuzzes depend on what drives them, so a fuzz behind another fuzz is a model of the circuit, not a promise of what the real pedals do together.

**Style menu (Drive 1 and 2):** Transparent, Tight, Open, Screamer, Distortion, Tube Driver, Op-Amp Distortion, Stack Drive. Transparent is now a Centaur-type circuit (below). The Circuit Mode switch is back (v94, off by default): off, Transparent, Tight, Screamer and Distortion are the original hand-tuned voicings; on, they are the circuit models. Op-Amp Distortion and Stack Drive are always circuit models; Open and Tube Driver are hand-tuned. Grip works in both modes.

**Transparent.** Gain fades the clean path out as the clipped path comes in; Tone is the treble shelf; **Edge** is the extra top end into the clipper (I took "grip" to mean Edge, since there is no knob called Grip; tell me if you meant something else). Several values in it are my estimates until you send the LC_VODA readings (list in MODELED_GEAR).

**Boost section.** Mode **Clean** is now an op-amp boost (0 to 26 dB) and mode **Treble** is a one-transistor booster whose Sat knob moves the input capacitor (5 to 47 nF). **Compressor.** Style **Dyna (OTA)** is a squashing pedal compressor: Threshold sets how hard it squeezes, Attack, Release, Output and Mix work as labelled; ratio, attack and release curves are my estimates.

**Amps.** The Mode menus are back (v94): Classic (the original voicing, the default) or Circuit-derived. The Full-circuit mode and its Master knob stay removed. Sessions saved in v93 carry no Mode, so they load as Classic.

Wah panel: the Voice menu picks Generic (your Heel and Toe) or Classic circuit (a fixed 450 Hz to 1.6 kHz sweep).
