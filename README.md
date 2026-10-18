# Starter Amp v93 (JUCE, macOS)

A hand-built guitar amp simulator (Standalone, VST3, AU): input and gate, pitch shifting, fuzz, compressor, overdrive, boost, wah, phaser, chorus, modulation, fifteen amp models (fourteen electric and an acoustic), four neural-capture (NAM) slots, four cab blocks, Post EQ, granular and shoegaze effects, delays and reverbs.

This README is organised by section (one section per part of the plugin). Other files: **MANUAL.md** (full user manual and control reference), **QUICKSTART.md** (first ten minutes), **MODELED_GEAR.md** (the real gear the parts are loosely inspired by; names appear only there), **CHANGELOG.md** (old per-version notes), **WINDOWS_BUILD.md** (the Windows beta build).

## Build
```
xcode-select --install
brew install cmake
cmake -B build -G Xcode
cmake --build build --config Release
```
App: `build/StarterAmpV93_artefacts/Release/Standalone/Starter Amp v93.app`

Use headphones, and keep Output low while you raise the gain knobs. If the build shows errors, paste them to whoever maintains this project.

## The interface
The window has ten pages, chosen with the tabs along the top or the signal-flow strip under them, in the order the signal travels. The open page is remembered with the session.

| Tab | What is on it |
|---|---|
| INPUT | INPUT & GATE (with the pickup de-noise), TRANSPOSE / FORMANT |
| GAIN | FUZZ, COMPRESSOR, OVERDRIVE, BOOST, BOOST EQ |
| MOD | WAH, PHASER, CHORUS, MOD FX A, MOD FX B |
| PITCH | PITCH PEDAL, OCTAVER, HARMONIZER |
| AMP | Tabs AMP A, AMP B (the parallel amp) and NAM (four neural-model slots); MODERN METAL and ACOUSTIC panels appear when the amp needs them |
| CAB | Sub-tab CAB BLOCKS (cab blocks A to D, CAB, mic position, IR alignment) and sub-tab SPEAKER SHAPING & EXPORT (per-block shaping, speaker dynamics, export as IR) |
| EQ | POST EQ and the spectrum |
| DELAY | PRE-AMP DELAY and POST-AMP DELAY (each with its effects panel) |
| SCAPES | GRANULAR CLOUD, TREMOLO / AUTO-PAN, WOBBLE, SWELL, LO-FI WARBLE, NOISE PEDAL |
| POST FX | REVERB (with reverb shaping), OUTPUT (meters, limiter), DOUBLER, WIDENER |

**Tidy view.** The TIDY VIEW button in the header collapses every pedal that is switched off into a small tile with its name, a one-line description and a SWITCH ON button, so a page only shows what you are using. Click SWITCH ON to bring the pedal back at full size. Switch TIDY VIEW off to see every pedal in full, faded when off.

**Help.** The HELP button opens a window with a short plain-language guide to the window.

**Reordering effects.** The signal-flow strip under the tabs shows the effects that can be moved. Drag a block left or right to change the order; blocks can move within their own group (before the amp, or after the cab). The ORDER window (MOVE UP / MOVE DOWN, or drag a row) does the same from a list.

**On/off switches.** Every effect has one. Lit (amber) = on. Switching a delay or the reverb off fades its mix out, so it does not click, and any tail dies away.

Using it:
- **Knobs:** drag up/down (or sideways) to change a value. **Double-click** a knob to reset it to its default. **Click the number** under a knob to type a value.
- **Menus** pick the style, model, ratio, note value and so on. Hover a menu, or an item in an open menu, for a short description.
- **Resizing:** drag the bottom-right corner. The whole window scales in proportion.

For developers (all in `PluginEditor.h` and `PluginEditor.cpp`):
- **Colours:** change the palette in the `ui` namespace at the top of `PluginEditor.h`.
- **Look of the knobs and buttons:** `AmpLookAndFeel` in `PluginEditor.cpp`.
- **Adding a control:** in the `StarterAmpEditor` constructor, call `addKnob (somePanel, "parameterId", "Label")`, or add a toggle or menu with `makeToggle` or `makeCombo` inside `somePanel.addRow (...)`. The `parameterId` is the ID used in `createParameterLayout` in `PluginProcessor.cpp`.
- **Adding a panel:** declare a `Panel` in `PluginEditor.h`, add it to the list near the top of the constructor, and add an `item (...)` line for it in the page definitions (search for `addFlowPage`) so it takes part in the flow layout and TIDY VIEW. If it has a description, add it to the map in `addFlowPage`.

## Signal chain
input -> **pickup de-noise** -> gate -> high-pass -> **transpose / formant** -> **noise pedal** -> **fuzz** -> **compressor** -> **wah** -> **phaser** -> **chorus** -> **pre-amp delay** -> **Mod FX slots set to "Before the amp"** -> **Lo-Fi Warble** -> **boost** -> **NAM pedal 1** -> **overdrive** -> **amp** (Amp A, or its NAM capture; Amp B in parallel, or its own capture) -> presence and tone stack -> **cab** (four blocks, each taking Amp A, Amp B or both) -> **NAM pedal 2** and the **Mod FX slots set to "After the cab"** -> **Post EQ**, **delay**, **reverb** and the rest of the post group, in the ORDER window's order -> output level -> **optional limiter**.

The order of the pre-amp group and the post-cab group can be changed by dragging (see above). Pitch effects (pitch pedal, octaver, harmonizer) sit in the pre-amp chain next to the modulation effects.

## Matching the amp volumes
Each amp model has a built-in output trim so they sit at about the same loudness. I set the trims by measuring, not by ear: I ran a plucked-note test signal through a model of the amp stage (preamp, sag, presence, tone stack and the built-in speaker low-pass, with the knobs at their defaults and Overdrive off) and compared the loudness with a rough K-weighted RMS measure.

The trims (in dB, the last number in each entry in `getVoicing`):

| Amp | Trim |
|---|---|
| Clean (sparkly) | +9.0 |
| Vintage Chime (class-A) | +1.5 |
| British Hot-Rod (tight) | -6.8 |
| Saturated Lead (smooth) | -3.8 |
| Modern High-Gain (scooped) | -1.4 |
| Modern High-Gain with Voicing on | -4.65 |
| Classic Rock (crunch) | -1.0 |
| Classic Rock with Voicing on (pushed) | -2.0 |
| Boutique Lead, Jazz | +2.8 |
| Boutique Lead, Rock | -1.4 |
| Tight Metal, Raw | -4.6 |
| Tight Metal, Modern | -3.5 |
| Low-Watt Combo, Soft | +3.8 |
| Low-Watt Combo, Cranked | +1.9 |
| Glass Clean, Glass | +13.1 |
| Glass Clean, Warm | +6.8 |
| Creamy Lead, Rhythm | -3.7 |
| Creamy Lead, Lead | -5.4 |
| Doom Stack, Stoner | -4.1 |
| Doom Stack, Wall | -2.2 |

the Saturated Lead trim was measured the same way. At the default Amp Gain (0.5) all six land within about 0.3 dB of each other in the model. Away from the middle they drift apart on purpose, because that is how the amps behave:
- **Clean** and **Vintage** get louder as you turn Amp Gain up (about 4 dB per 0.3 of the knob), since they have headroom and clip later. At Amp Gain 0.2 they are about 3 to 5 dB quieter than the high-gain models, and at 0.8 about 3 dB louder.
- **Hot-Rod**, **Saturated Lead** and **Modern** change by only a few dB across the whole knob, because they are already squashed by clipping and sag.

Limits: this is a measured match with a test signal, not a perception test. Your guitar, playing, the Overdrive, the Compressor, and a loaded cab IR (IRs vary a lot in level) will all shift it. If one amp still seems too loud or quiet to you, change its `trimDb`, or use the Output control.


## Noise gate and pickup de-noise
**Gate (dB)** works as before: raise it until the noise between notes disappears, and lower it if notes get cut off.

**Pickup De-Noise** is a new stage in front of the gate that cleans the signal first. It's off by default.

| Control | What it does |
|---|---|
| Pickup De-Noise | Switches the stage on |
| Hum Frequency | 60 Hz (US) or 50 Hz (EU) mains. Pick the one for where you play. |
| Hum Reduction | Narrow notch filters at the mains frequency and its first four harmonics (for 60 Hz: 60, 120, 180, 240 and 300 Hz), which is the buzz single-coil pickups pick up from lights, amps and wiring. 0 = no notching, 1 = full notches. |
| Hiss Reduction | A low-pass that closes when you aren't playing (between notes and in the tail of a decay) and opens quickly when you do. At 1 it closes down to about 1.5 kHz, so hiss and fizz in the gaps get rolled off. At 0 it does nothing. |

How the two parts work together with the gate:
- The de-noise stage comes **before** the gate, so the gate listens to the cleaned signal. Hum no longer holds it open, and you can set Gate (dB) lower without it chattering.
- Hiss Reduction follows the **Gate (dB)** threshold: the filter is fully closed at the threshold and fully open about 24 dB above it. Change the gate threshold and the hiss filter moves with it.

Tips:
- Start with Hum Frequency matching your mains, Hum Reduction about 0.7, Hiss Reduction about 0.4, and Gate about -65 dB.
- If the notes sound slightly thin, lower Hum Reduction. The notches are narrow (about 2.4 Hz wide at 60 Hz), but notes sitting right on a notch, such as near 120 Hz or 180 Hz, can lose a little body at full strength.
- If note tails sound dull or sizzle disappears too early, lower Hiss Reduction.
- It can't remove noise that's present while you play, or noise that isn't a steady hum. For that, lean on the gate and the hiss filter, or fix the source (a shielded cable, moving away from the monitor or dimmer).
- Hum often varies with where you stand. Turning toward or away from the source changes it, which is worth trying before reaching for the filters.


## Cab section
The cab page has four cab block panels (**A, B, C, D**), a **CAB** panel and the **IR ALIGNMENT** display.

### Cab blocks A to D (one panel each)
Only block A is on at first. Each block has an **ON** switch and a **Source** menu:
- **Built-in model:** the plugin builds the IR from the controls below (see "Built-in cab models").
- **IR file:** the big button loads a WAV or AIFF impulse response and shows the file name. IRs are normalised when loaded.

Controls in both modes:
- **INVERT** flips the IR's polarity (upside down).
- **Level** (-24 to +12 dB): how loud this block is in the mix.
- **Pan** (-1 to +1): places this block in the stereo field.
- **Delay** (0 to 10 ms): delays this block. This is what lines blocks up in time. 1 ms is about 34 cm of distance.
- **Low Cut** (20 to 500 Hz) and **High Cut** (2 to 20 kHz): filter this block on its own, before the blend. Off at the ends of their ranges.

The blocks are summed with equal-power scaling (each is multiplied by 1 divided by the square root of the number of blocks that are on), so turning on more blocks does not make things much louder. The old "Mix A / B" knob is gone; use each block's Level instead. Defaults: A = Modern Tight + Dynamic Bright; B = Modern Tight + Ribbon Smooth; C = Vintage Crunch + Dynamic Full; D = Smooth Vintage + Dynamic Presence, so switching a block on gives a different, usable sound at once.

### Built-in cab models
Choose a **Speaker** and a **Mic**, then set where the mic is:
- **Speakers:** Modern Tight, Vintage Crunch, Rock Bite, Smooth Vintage, Chimey Alnico, Vintage American, Hi-Fi Full. Then the metal-oriented ones (Modern Scoop to Mid Forward), and the fitted **Studio**, **Heavy**, **Rhythm** and **Compact** speakers in the Mix ready folder, plus four **Classic** speakers for older styles.
- **Mics:** Dynamic Bright, Dynamic Full, Dynamic Presence, Ribbon Smooth, Condenser Detailed, and Flat (no mic coloration).
- **Position** (0 to 1): 0 = centre of the speaker's dust cap (bright, direct), 1 = the edge of the cone (darker, warmer, with a small dip near 2.8 kHz).
- **Distance** (0 to 12 inches): close = more bass (proximity effect, strongest on the ribbon and weakest on the condenser), far = a little less top end. Distance also delays the IR by the real time of flight (about 3.5 samples per inch), so two slots at different distances need aligning, which is what ALIGN NOW is for.
- **Angle** (0 to 60 degrees): pointing the mic away from the cone loses highs, most on the dynamic mics.
- **Tight, Bite and Box**: Tight and Bite shape the low end and the upper-mid bite; Box adds a real cabinet's narrow resonances.
- **Cabinet** (a menu on each block): 16 types in folders (1x, 2x, 4x, Oversized), from 1x8 Small to 4x12 Closed. Open backs lose bass below about 100 Hz; closed cabs add a low thump around 100 Hz.
- **Room** (shared, 0 to 1) and **Room Type**: a decaying ambience after the direct sound. Original is the short dark ambience; Studio room, Large studio room and Living room are longer and modelled on room recordings.

**How honest this is:** these are models I built, not recordings. Each speaker and mic is a hand-made frequency response (a low roll-off, a low resonance, a few peaks and dips, a high roll-off, a little irregularity) based on the general character those parts are known for. I tuned them by ear, and I have not measured any real speaker or mic. The names say "-style" for that reason. They will not match a particular commercial IR. The response is turned into a minimum-phase impulse response (how a real speaker roughly behaves), 4096 samples long at 48 kHz, and loaded like a file IR. Use them as a starting point and trust your ears.

Tip: set two blocks to the same speaker with different mics (say Dynamic Bright close, Ribbon Smooth a few inches back), switch both on, press ALIGN NOW, and balance them with Level.

### CAB panel
- **CAB ON** turns the cab section on. Off = only the amp's own low-pass.
- **AUTO ALIGN** (on by default): every time you load an IR file with the button, if two or more blocks are on, they are aligned automatically. Sessions you reopen are not re-aligned, and changing a built-in model control does not realign (so your Delay knobs do not move while you tweak).
- **ALIGN NOW** runs the alignment again whenever you want.
- **Room**: the shared built-in model control described above (Cabinet is now on each block).
- **Spread**: pushes the blocks apart (A left, B right, C a little left, D a little right) on top of each block's own Pan, for a stereo cab from mono IRs. It needs two or more blocks on. If you plan to listen on a mono speaker, keep Spread and the Pans at 0.
- **Low Cut** (20 to 500 Hz) and **High Cut** (2 to 20 kHz): remove rumble and fizz after the cab. Off at the ends of their ranges.
- **Tilt** (-6 to +6 dB): makes the whole cab darker or brighter around 800 Hz.
- **Reso** (-6 to +9 dB) and **Reso Hz** (50 to 200 Hz): a low peak that adds (or removes) speaker-cabinet thump at the frequency you choose. 0 dB = off.
- **Air** (-6 to +6 dB): a gentle high shelf from about 6 kHz.
- The old "Built-in Cab" fallback menu (Amp default, 1x12 Open...) has been removed from the window. It only mattered when no IR was loaded, and the built-in models replace it. If both slots are set to "IR file" with no file loaded, the amp's own low-pass is used.
- The IR ALIGNMENT display shows the IRs' level, delay, polarity and mix, but not the pan and per-slot filters.

### Automatic phase alignment
With one block on there is nothing to align. When two mics record the same speaker, one hears it a little later than the other (about 1 ms for every 34 cm of extra distance). Mixing the two then makes some frequencies cancel, which sounds thin and hollow. Mic positions can also be effectively upside down relative to each other. Alignment fixes both.

How it works here (my own simple version of the usual cross-correlation method):
1. The first **active** block (lowest letter that is on) is the reference. Each other active block's first few thousand samples are compared with the reference through a band-pass (150 Hz to 3.5 kHz).
2. Each is slid against the reference by up to 5 ms either way, and the strongest match gives its time offset. If the strongest match is a negative one, that block is **inverted**.
3. Every block that matched is then given a **Delay** so that all arrive together, i.e. matching the latest one (the reference and the blocks that arrived earlier get delayed). The results are written into the Delay and INVERT controls, which you can adjust by hand.

If a block's best match is weak (below 0.15 on a scale where 1.0 is identical), it tells you and leaves that block's settings alone. That can happen with different speakers or very different mic positions.

Limits:
- It only changes Delay and Invert, never the IR files. Offsets are measured to the nearest 48 kHz sample; offsets over 5 ms are not searched. The delays can add up to 10 ms of lag, which the plugin does not report to the DAW as latency.
- All blocks are compared with the reference, not with each other, so two blocks that each match the reference are assumed to match each other.
- The best-sounding mix is not always the best-aligned one, so use your ears and the display.

### IR ALIGNMENT panel (the display)
- **Left plot:** the first 6 ms of each active block (A amber, B teal, C purple, D green), drawn with the same scale, with the current Delay and INVERT applied. When they are aligned, their first big peaks line up and point the same way.
- **Right plot:** the frequency response (60 Hz to 12 kHz) of each block (thin lines) and the two together as you hear them (white), after Delay, INVERT and Level. A comb-filtered mix shows narrow deep notches. A well-aligned mix is smoother. The vertical range is -30 to +10 dB around the average. The thin lines are only drawn with two or more blocks on. This ignores Spread, Low Cut, High Cut and Tilt.
- **Text:** what the last alignment found.

### Other notes
- IRs are normalised when loaded, so adjust **Level** or **Output (dB)** if the level jumps.
- The IR file paths are saved in the plugin state and reloaded with the session (if the files haven't moved).
- IRs are loaded with the silence at their start kept, because alignment depends on it.

Use IRs you have the right to use: ones you record yourself, free packs with suitable licenses, or ones you've bought. Don't use IRs taken from other companies' plugins.

## Phaser (with more options)
A classic phaser is four all-pass stages swept together by one LFO with a single Speed knob, mixed half and half with the dry sound. This phaser starts from that idea and adds options. It sits after the compressor and before the amp, so the overdrive and amp distort its sweep.

| Control | What it does |
|---|---|
| PHASER ON | Switch (off by default) |
| Stages | 2, 4 (the classic number, the default), 6, 8, 10 or 12 stages. More stages = more notches and a deeper, more vocal sound. |
| Shape | LFO shape: Sine, Triangle, or Random (a smoothed random sweep) |
| Rate | 0.05 to 10 Hz |
| Depth | How far the sweep travels (up to 2 octaves each way of the centre) |
| Centre | Where the sweep is centred, 80 Hz to 3 kHz |
| Feedback | -0.9 to +0.9. Positive = resonant peaks, negative = hollow notches, 0 = classic classic phaser sound. Output is reduced a little as feedback rises. |
| Mix | 0 to 1 (0.5 is the classic classic phaser sound) |
| Envelope | Lets how hard you play push the sweep, like an auto-wah. 0 = off. Set Rate low and Envelope high for a mostly envelope-driven sweep. |

What is classic and what is mine: the four stages, one shared sweep and 50/50 mix are the classic design. Everything else is added. The sweep range and shapes are hand-tuned, not copied from a circuit.

## Chorus (bucket-brigade style)
This chorus is two bucket-brigade-style delay lines swept in opposite directions by a slow triangle LFO, with three modes: I, II, and I+II. I have used commonly quoted figures as I remember them (about 0.5 Hz for I, 0.9 Hz for II, 9.75 Hz and shallow for I+II, delays of a few milliseconds). I have not measured a unit, so treat them as approximate.

| Control | What it does |
|---|---|
| CHORUS ON | Switch (off by default) |
| Mode | I, II or I + II (the classic modes), or Dimension, Tri-chorus, Ensemble or Detune (see below) |
| Mix | Dry/wet, 0.5 by default |
| Rate | Speeds the LFO up or down from the mode's rate (x0.25 to x4) |
| Depth | How far the delay sweeps (0 to 1.5, 1 = the mode's normal depth) |
| Tone | The darkness of the BBD lines (2 to 14 kHz low-pass, default 8 kHz) |
| Hiss | Optional BBD noise. 0 by default, because the amp after it adds gain |

**Chorus voicings.** Modes 1 to 3 are the classic bucket-brigade modes described above. Four more voicings use the same Mix, Rate, Depth, Tone and Hiss knobs:
- **Dimension:** two lines moving in opposite directions, very slow and shallow. It thickens and widens rather than sweeps; try Mix 0.3 to 0.5.
- **Tri-chorus:** three lines a third of a cycle apart: a smooth, wide, swirling chorus.
- **Ensemble:** three lines with a slow sweep and a faint fast shimmer on top, in the manner of a string machine.
- **Detune:** two longer lines that drift slowly in opposite directions, a few cents sharp and flat, like a doubled take.
Tested standalone: all four stay finite and bounded at full Mix. I have not listened to them.

Because it is in front of the amp, it is mono here: the two lines are summed rather than sent to left and right. The fixed delay of a few milliseconds also makes a comb filter, so some notes sound thinner than others. That is part of the sound.

## Compressor
A compressor before the amp. It has a set of ratio curves (including a 10:1 "Soft" curve that acts like a light-controlled compressor, and "Limit", a brickwall limiter), attack from about 50 microseconds to 50 ms, release up to a few seconds, a high-pass filter in the detector, and two distortion modes: **Tube-like** (mostly 2nd harmonic) and **Tape-like** (mostly 3rd harmonic). It is a simple design of my own, **not** a circuit model of any hardware.

**Style menu.** Standard is the original compressor. The other styles change the knee, the attack and release behaviour and (for some) the ratio curve, with the same Threshold, Ratio, Attack, Release and Mix controls:

| Style | Character | Measured in a standalone test (6:1, -24 dB threshold, Attack 5 ms, Release 250 ms) |
|---|---|---|
| FET | Fast, hard knee, snaps back after deep squeezes: punchy and forward | Attack 1.5 ms to settle, release about 96 ms |
| Opto | Soft knee, slow to grab; the release slows down the deeper and longer it has squeezed | Attack 6 ms, release about 430 ms |
| Variable-mu | Very soft knee and a gentler ratio curve: smooth, even glue | Attack 6 ms, release about 330 ms |
| Sustainer | A pedal-style squeeze: the first few ms of the pick gets through, then a long hold so notes bloom | Attack 4 ms, release about 460 ms |
| Bus | Light glue (4:1 at most, slow attack) that holds chords together | Attack 4 ms, release about 180 ms |

(The numbers are the time to get within 3 dB of settled and of recovered; the Release knob scales them all.)

**Saturation menu** (behind ADV). Off, Tube-like, Tape-like, Both and Transformer as before, plus four voiced to pair with the styles: **FET stage** (class-A solid state, odd-harmonic edge, pairs with FET), **Opto tube stage** (soft, warm, even harmonics, pairs with Opto), **Mu iron + tube** (tube stage plus low-end iron saturation, pairs with Variable-mu) and **Diode** (a rounded, symmetric diode-style softening, pairs with Sustainer). Nothing pairs automatically: you choose both menus. In the test the added second and third harmonics ranged from about -42 dB to -23 dB under the fundamental at a 30 dB-over-threshold squeeze.

It sits after the Noise and Fuzz pedals and before the pre-amp delay, overdrive and amp, so its saturation and level changes feed the amp. It is off by default.

| Control | What it does |
|---|---|
| COMP ON | Switches the compressor on. |
| Ratio (menu) | 1:1 (no compression, saturation only), 2:1, 3:1, 4:1, 6:1 (a good start), 10:1, **10:1 Soft**, 20:1, **Limit** (limiter). Soft has a soft knee, and its release gets slower the harder it is compressing. Limit is a hard-knee limiter. |
| Saturation (menu) | **Off**, **Dist 2 (tube)**, **Dist 3 (tape)** or **Dist 2+3**. Dist 2 is lopsided saturation that adds even harmonics. Dist 3 is symmetrical and adds odd harmonics. The amount grows with the amount of compression, like the real unit's. |
| Detector HP (menu) | Off, 80, 160 or 320 Hz. Filters low notes out of what the compressor listens to, so low palm mutes don't pump the whole sound. |
| Thresh | Level (dB) above which the compressor works. Lower = more compression. |
| Attack | 0.05 to 50 ms. Short attack catches the pick; longer lets the pick through. |
| Release | 50 ms to 3.5 s. How fast the gain recovers. |
| Output | Makeup gain, -12 to +18 dB. |
| Mix | Dry/wet blend for parallel compression. 1 = all compressed. |
| GR meter | Gain reduction in dB. |

Tips and limits:
- A good start for tight, even rhythm playing: Ratio 6:1, Thresh -20 dB, Attack 5 ms, Release 250 ms, Output +4 dB, Saturation Dist 2.
- For sustain on leads: Ratio 10:1 Opto, Thresh -25 dB, Release 600 ms, Mix 0.8.
- Heavy compression raises the noise floor of your guitar. The gate and pickup de-noise are before the compressor, so they help.
- The saturation is not oversampled. It is gentle, but pushing Dist 3 hard with high gain reduction can alias a little.
- The detector looks at the signal through the optional high-pass only. The real unit also has a high-mid emphasis in its detector, which is not in this version.

## Overdrive

The overdrive has two styles, chosen by the **Style** menu in the OVERDRIVE panel. **OVERDRIVE ON** switches it on, and **Volume** (dB) is shared by both. The panel's other knobs change with the style.

### Transparent style (the original)
Gain, Tone, Edge, Volume, as in earlier versions: a clean path blended with a gain path, a diode-style clipper, and a treble tone control.

### Tight style
A tight, modern overdrive made to push high-gain amps, with controls named Drive, Volume (Level), Bright, Gate and a 6-position **Attack** setting (a knob that clicks through 1 to 6) that tightens the low end and adds mid bite. The sound here is **my own approximation** of those ideas.

| Control | What it does here |
|---|---|
| Drive | Gain. Almost everything goes through the gain path, with only a little clean signal. Higher = more saturation. |
| Bright | Adds treble before the clipper (up to +9 dB at 3 kHz). It can sound harsh at low gain, so turn it down there. |
| Gate | A noise gate ahead of the gain. 0 = off. Higher = a higher threshold, for chopped, gated palm mutes. Too high will cut off quiet notes. |
| Attack (knob, 1 to 6) | Positions 1 to 6. Higher positions tighten the low end more (the high-pass on the gain path goes from 60 Hz to 520 Hz) and add more bite around 750 Hz (0 to 6.5 dB). |
| Volume | Output level in dB, shared with the Transparent style. |

It clips a little harder than the Transparent style, and the signal after the clipper is rolled off slightly (4 dB shelf at 1.8 kHz) for a smoother top.

Tips:
- Pedal-style boost for a high-gain amp: Drive low (about 0.1), Attack 4 or 5, Bright about 0.3, Volume up a few dB, Gate about 0.3.
- Raise Volume carefully. A boost into the amp can be much louder than the amp alone.
- Both styles are before the amp, so they push the amp's first stage.

## Delays
There are two completely separate delays, each with its own full set of controls. Both start with Mix at 0, so they're off until you raise it. Each repeat is darkened a little (low-pass at about 3.5 kHz), like an analog delay.

- **Pre-Amp Delay** sits before the overdrive and preamp. Its repeats go through the gain stages, so they compress and distort along with your playing, and they sound thicker and less distinct.
- **Post-Amp Delay** sits after the amp and cab, before the reverb. Its repeats are clean copies of the amped sound.

Each delay has these controls (in the plugin they're named "Pre-Amp Delay Mix", "Post-Amp Delay Mix", and so on):

| Control | What it does | Default (pre / post) |
|---|---|---|
| Mix | How much of the repeats you hear (0 = off) | 0 / 0 |
| Note | Free (ms) or a note value taken from the tempo | Free |
| Time (ms) | 20 to 1000 ms; used only when Note is Free | 250 / 380 |
| Feedback | How many repeats. Keep below about 0.8 for a clean decay. | 0.30 / 0.35 |
| Lo-Fi | 0 = clean repeats. Higher = darker, crunchier, more degraded repeats (lower sample rate and bit depth). | 0 / 0 |
| Tape Warble | 0 = steady. Higher = more tape-style pitch wobble: a slow wow, a faster flutter, and a little random drift. | 0 / 0 |

**Delay mode.** Each delay has a **mode** menu: **Digital** (clean and bright, 16 kHz, linear feedback), **Analog** (a bucket-brigade style: darker at 4.5 kHz with two poles, soft saturation, a little clock wobble), **Tape** (darker still at 3.2 kHz, stronger saturation, a lot of wow, flutter and drift) and **Lo-Fi** (dark at 2.6 kHz plus reduced sample rate and bit depth, and a little wobble). The mode sets the base character and the **Lo-Fi** and **Warble** knobs add on top of it (so Tape with Warble up is wobblier still). Each repeat is filtered again, so the repeats get darker and softer in the analog, tape and lo-fi modes; Digital keeps them bright. New sessions and old ones default to **Analog**, which is the closest to the single sound earlier versions had, but it is a little brighter/darker in places and not identical. These are hand-built approximations of those styles, not models of particular units.

**Lo-Fi and Warble** only affect that delay's repeats, never your dry signal. They stack: try Lo-Fi about 0.5 with Warble about 0.4 for a worn-tape sound, and keep Feedback around 0.4 to 0.5, since each repeat gets rougher.

Ideas for using both:
- A long, murky pre-amp delay (Time 500 ms, Lo-Fi 0.4, Mix 0.2) for a smeared wash that the amp distorts, plus a short post-amp slapback (Time 90 ms, Feedback 0.1, Mix 0.2) for definition.
- A rhythmic post-amp delay for the clear repeats, and a quiet pre-amp delay at a different time for density.
- The two delays no longer share settings, so Mix is no longer doubled the way it was in "Both" mode. Set each one to taste.

## Tone Lab (match a recording)
The EQ page has a **TONE LAB...** button. It matches your sound to a recording by comparing two audio files:
- **DI:** your guitar recorded clean, with no amp or effects.
- **Target:** the same kind of playing after the amp, pedals and cab you want to sound like.

It plays your DI through a private copy of the plugin and measures how close the result is to the target. Then it changes one setting at a time and keeps each change that gets closer: the amp model and voicing, gain, bass, low mid, high mid, treble and presence, then (if block A uses a built-in cab) the cab speaker, mic, cabinet and mic position and distance, then the compressor, overdrive and boost (on or off and how hard), and finally the nine Post EQ faders for whatever difference is left. It never ends up worse than where it started, and nothing changes in your sound until you press **APPLY TO MY SOUND** (**UNDO** puts it all back).

What it measures, over the whole file, so the two takes do not have to be the same notes or line up in time: the average tonal balance in 25 third-octave bands from 50 Hz to 12.7 kHz, the crest factor (how far the peaks stand above the average) and the dynamic spread (how much the level moves between quiet and loud). The window shows the target curve and your curve before and after, and the tonal difference in dB. It does not match loudness, reverb, delay or modulation, so use dry or lightly processed target recordings.

Tests (standalone, with a synthetic plucked-string DI, about 50 s per run, 263 renders): when the target was made with a known chain, the average tonal difference fell from 2.8 dB to 0.4 dB in one case and from 3.4 dB to 0.5 dB in another. It did not pick the same amp model that made the target (an amp, a cab and the EQ can give nearly the same balance), so it finds settings that sound close, not the original settings. I have not listened to the results.

The older **MATCH...** window (reference spectrum only, fitted to the Post EQ while you play) is still there.

## Post EQ
A 9-band EQ after the cab and before the delay and reverb, with a second mode that uses custom Q settings per band. The band choices and Q values are mine, picked by ear.

| Control | What it does |
|---|---|
| EQ ON | Turns the Post EQ on (off by default) |
| Mode | **Graphic**: all nine bands have the same medium Q (1.4). **Mod (custom Q)**: each band has its own Q, narrower in the mids and wider at the ends. |
| Faders | 63, 125, 250, 500 Hz, 1, 2, 4, 8, 16 kHz, each -12 to +12 dB |
| Q (Mod) | In Mod mode, scales every band's Q together (0.4 to 4, default 1.4 = the built-in values). It does nothing in Graphic mode. |
| Low Cut | High-pass filter, 20 to 500 Hz (off at 20 Hz), before the bands |
| High Cut | Low-pass filter, 2 to 20 kHz (off at 20 kHz), before the bands |
| Slope | 12 dB/oct or 24 dB/oct for both Low Cut and High Cut (Butterworth, default 24) |
| Level | Output level of the EQ, -12 to +12 dB, to make up for boosts or cuts |

Tips: cut 63 Hz and 16 kHz a little for a tighter, less fizzy sound; boost 2 kHz and 4 kHz for bite, or cut them to smooth. Bands at 0 dB cost nothing.

## Reverb
The reverb has a **Type** menu: **Classic** (the original simple reverb, the default), **Room**, **Hall**, **Plate**, **Ambient**, **Cathedral** and **Spring**. The new types use a feedback delay network I wrote (eight delay lines mixed through an orthogonal matrix, four all-pass filters to smear the input, per-line damping, and slow modulation on the lines to avoid metallic ringing). Roughly: Room is short and natural (about 0.5 s at the middle Size); Hall is large and smooth (about 2 s); Plate is dense and bright (about 1.5 s); Ambient is long, lush and slowly moving (about 3.5 s); Cathedral is huge and dark (about 5 s); Spring is short, boingy and a bit chirpy (about 1.5 s). The times scale with **Size**. These are hand-tuned approximations "in the character of" those names, not models of any real room or unit, and I have not listened to them, so tune Size, Damp, Pre-Dly and the cuts by ear. Size, Damp, Width, Freeze, Pre-Delay, ducking, shimmer and the filters work on every type. The Classic reverb is unchanged. The reverb also now skips its work entirely while it is off or at Mix 0 (CPU saving). Each type is level-trimmed to roughly the same loudness, but expect to adjust Mix. What follows describes the controls and the signal path (it was written for the Classic reverb).

Signal path: pre-delay -> reverb -> low cut / high cut -> mix.

| Control | What it does |
|---|---|
| Mix | Amount of reverb (0 = off, the default) |
| Decay | Decay time in seconds (0.1 to 30 s): how long the tail takes to fall by 60 dB. |
| Pre-Dly | 0 to 250 ms of delay before the reverb starts, which keeps the reverb from smearing your notes |
| Damp | How fast the highs die away in the tail |
| Width | 0 = mono reverb, 1 = full stereo |
| Low Cut | **High-pass filter on the reverb** (20 to 1000 Hz, default 100 Hz). Removes mud, and tames shimmer buildup in the lows |
| High Cut | Low-pass filter on the reverb (1 to 20 kHz, default 12 kHz) |
| Duck | Turns the reverb down while you play and lets it bloom when you stop. 0 = off |
| FREEZE | Holds the current reverb tail |
| Shimmer | Amount of pitch-shifted feedback (see below) |
| Tone | **Shimmer Tone**: how dark the shimmer feedback is (1.5 to 12 kHz, default 4.5 kHz). Lower = darker, softer shimmer |

### Shimmer
Switch **SHIMMER** on to turn the reverb into a shimmer reverb.

How it works: the reverb's output is fed back into its input through a pitch shifter. Each trip around the loop goes higher in pitch and fades, which gives the rising, choir-like tail. The direct signal still goes into the reverb unshifted. The Low Cut and High Cut sit **inside** that loop, so they also stop the lows building up and the highs getting harsh with every pass. That is the fix for shimmer brightness: raise Low Cut, lower High Cut or Tone.

The menu next to SHIMMER picks the interval (eight choices), and the Mode menu picks what is shifted: the tail, your playing, or both.

Tips:
- Start with Mix about 0.3, Size about 0.6, Shimmer about 0.4, then play single sustained notes.
- If it is too bright, try Low Cut 200 to 300 Hz and Tone 3 kHz.
- High Shimmer values make tails very long. It is stable by design (the feedback is limited and filtered), but it can get loud, so keep Output low.
- Switching Shimmer on or off while a tail is ringing cuts the shimmer part of the tail.
- The shifter is a basic delay-line type, so on its own it is a little grainy. The reverb smooths that out.
- The reverb now always runs one sample at a time, so it uses a bit more CPU than before in normal mode.
- Previously the reverb had fixed damping and no filters, so the sound with the new defaults is slightly different from v19 even with the new controls untouched.

## Amp models
**Amp Model** picks one of five amp models, **listed from least to most gain**. They share the same preamp machinery (cascaded clipping stages, each preceded by a high-pass) but with different numbers, so each has its own character. They are hand-tuned approximations of each amp's general character, **not** circuit models, so don't expect a match to the real amps. Treat them as starting points and adjust by ear.

| # | Model | Preamp gain at Amp Gain 0.5 | Character | How it's voiced |
|---|---|---|---|---|
| 1 | Clean (sparkly) | about 9 dB | Lots of headroom, sparkly, scooped mids | 2 gentle stages, low gain, little clipping. A bright boost before the stages, mids pulled back a little, speaker roll-off opened up to 8 kHz. |
| 2 | Vintage Chime (class-A) | about 22 dB | Chimey, jangly, upper-mid "honk", gritty when pushed | 2 stages with moderate gain, strongly asymmetric clipping (a Class A-style character), a 1.8 kHz push before the stages, high mids lifted around 2 kHz, medium sag. The real amp has a Cut control instead of Presence, so Presence here only adds a little air. |
| 3 | British Hot-Rod (tight) | about 42 dB | Tight, bright, saturated, strong upper mids | 3 stages with high cutoffs (tight lows), big gain range, a 1.3 kHz push before the stages, mid knob centred higher. |
| 4 | Saturated Lead (smooth) | about 45 dB | Smooth, thick, singing lead tones, forward upper mids, a lot of compression | 3 stages with moderately tight lows, a 2 kHz push before the stages, a hump in the high mids around 1.6 kHz, a little extra bass, strong sag for the long, creamy sustain. A touch less gain than the next amp, a bit more than the British amp. |
| 5 | Modern High-Gain (scooped) | about 45 dB | Big low end, scooped mids, spongy and heavy | 3 stages with looser lows, big gain range, extra bass, mids scooped about 5 dB, strong power-amp sag so loud notes squash and bloom. |

"Preamp gain" is the total gain of the model's stages at Amp Gain 0.5, from the numbers in `getVoicing`, and is how they were put in order. The Saturated Lead amp and the Modern High-Gain amp are close in gain (the Modern is a little higher). They differ more in tone than in how hard they clip. The numbers are my own, based on the general reputation of each kind of sound.

- **Amp Gain** sets how hard each model's stages are driven. On the Clean model, low settings stay clean, and the top of the range starts to break up.
- **Presence, Bass, Low Mid, High Mid, Treble**: see below. The tone knobs are added to each model's built-in offsets, so "0" isn't flat. It's that amp's neutral setting.
- Each model has its own level trim so they're roughly matched (see "Matching the amp volumes"). Adjust **Output (dB)** to taste.
- Switching models while playing may click briefly. That's the gain stages changing.
- To change a voicing, edit the numbers in `getVoicing` in `PluginProcessor.cpp`. Each field is commented.
- **Older sessions:** the amp list order changed in v19 (it used to be Clean, Hot-Rod, High-Gain, Chime). Sessions saved by older versions are translated automatically, so they still load with the same amp.

## Voicing switch (every amp)
The **Voicing** switch does something on every amp, and its label changes to say what (it does nothing in Acoustic mode). Off is the amp as it was. On gives:

| Amp | Switch on | What changes (all my own numbers, not measurements) |
|---|---|---|
| Clean (sparkly) | **Edge push** | A small old amp turned up: about 2x the gain range with more asymmetry (so it breaks up when you dig in), a push near 800 Hz, heavy sag, a thicker low mid, a darker top (treble -3 dB, speaker filter 6 kHz). |
| Vintage Chime (class-A) | **Treble boost** | A treble booster in front of the amp: much more gain, a +5 dB push near 2.6 kHz, thinner lows (bass -3 dB, tighter high-passes), more bite and top. |
| British Hot-Rod (tight) | **Heavy** | A modern compact heavy-rhythm voicing: more gain and compression (stage gains up to 23 / 17 / 6), a deep but controlled low end (the first stage lets the lows through at 95 Hz, the later stages clamp them harder at 175 and 125 Hz), bass +3.5 dB, a slight low-mid scoop (-1.5 dB at 420 Hz), a forward +2.5 dB upper-mid at 2.2 kHz for chunk and definition, and a tamed, less fizzy top (speaker filter 5.6 kHz). This is my own take on that kind of sound, not a model of any particular amp. |
| Saturated Lead (smooth) | **Lead** | More gain and compression for sustain, a +4.5 dB push at 1.1 kHz plus +3.5 dB at 1.4 kHz for a singing lead tone, a slightly lighter low end, a smoother top. It still responds to the Focus knob. |
| Modern High-Gain (scooped) | **Clarity** | Fills in the mids and tightens the lows (details below). |

These describe general reputations of these sounds, and I have not tuned any of them by ear. **Level trims:** the new voicings have more gain, so I estimated their trims (+7.5, 0.0, -7.5 and -5.6 dB for Clean, Chime, Hot-Rod and Lead) instead of measuring them like the original trims. They may be a few dB off, so adjust Output if the level jumps when you flip the switch. Edit entries 6 to 9 of the `getVoicing` table to change any of this.

### Clarity (Modern High-Gain) details
**Voicing switch on the Modern High-Gain amp** fills in the mids and tightens the amp. This is my own take on "more mids and more clarity":

| | Switch off (default) | Switch on |
|---|---|---|
| Low mids (400 Hz) | -4 dB | -1 dB |
| High mids | -3 dB at 1.2 kHz | +1.5 dB at 1.4 kHz |
| Bass shelf | +3 dB | +1.5 dB |
| Stage high-passes (tighter = less flub) | 70 / 100 / 80 Hz | 95 / 130 / 100 Hz |
| Power-amp sag | heavy | lighter |
| Push before the preamp | +2 dB at 3 kHz | +3 dB at 1.8 kHz |
| Speaker filter (no IR loaded) | 5.5 kHz | 5.8 kHz |
| Level trim | -2 dB | -3 dB (the extra mids sound louder) |

- It still stacks with your own Bass, Low Mid, High Mid, Treble and Presence settings.
- Flipping it while playing may click briefly, since the filters and gains change at once.
- Good for fast riffs and chords, where the scooped setting turns muddy. The off setting is better for big, hollow, rhythm-heavy tones.

## Amp Focus (Saturated Lead and Modern High-Gain)
The **Focus** knob is on the AMP panel (0 to 12, in 0.1 steps; think of it as "semitones below E standard": 2 = Drop D, 4 = Drop C, 5 = 7-string B, 10 = 8-string F#). It was called Tuning before v28 and works exactly the same; old sessions load unchanged. It only changes the Saturated Lead and Modern High-Gain models (both voicings); the other amps ignore it. 0 is exactly what you had before.

For lower tunings it does these things, scaled by how many semitones down the knob is:
- raises the pre-gain high-pass so low strings do not flub the gain stages (the tight-ness setting moves up in frequency);
- takes a little gain off the first stage, because low strings hit it harder and the stage compresses less cleanly;
- trims bass and low-mid in the tone stack so the low end does not get boomy or muddy;
- moves the power-amp/voicing mid emphasis up and adds a bit of it, so the notes keep definition;
- reduces power-amp sag a bit for a tighter feel.

The numbers are my own rules of thumb, not measurements of real amps, and I have not tested them by ear. Treat Focus as a starting point and set Bass, Mids and Gain to taste. If it sounds too thin, lower Focus or turn Bass up.

## Presence and the split mids
**Presence** (0 to 1) adds top-end sparkle and bite. It's a treble shelf placed after the clipping and sag, where a real amp's presence control acts in the power stage, so it brightens the amp without making the preamp distort differently. Each model has its own presence frequency and range:

| Model | Presence starts at | Full boost |
|---|---|---|
| Clean (sparkly) | 5.5 kHz | +8 dB |
| British Hot-Rod (tight) | 4.5 kHz | +9 dB |
| Modern High-Gain (scooped) | 4 kHz | +10 dB |
| Vintage Chime (class-A) | 6 kHz | +4 dB |

It sits above the **Treble** control, so the two work together: Treble for the overall brightness, Presence for the very top.

**Low Mid** and **High Mid** replace the old single Mid control, so you can shape the body and the bite separately:

| Model | Low Mid centre | High Mid centre |
|---|---|---|
| Clean (sparkly) | 400 Hz | 1.5 kHz |
| British Hot-Rod (tight) | 500 Hz | 1.8 kHz |
| Modern High-Gain (scooped) | 400 Hz | 1.2 kHz |
| Vintage Chime (class-A) | 500 Hz | 2 kHz |

- **Low Mid** controls body and "boxiness". Cutting it cleans up muddiness, and boosting it adds thickness.
- **High Mid** controls bite, pick attack, and how forward a lead sounds. Boosting it helps solos cut through a mix, and cutting it tames harshness.
- Each knob goes from -12 to +12 dB on top of the model's own offsets.
- All the frequencies and offsets are in the `getVoicing` table in `PluginProcessor.cpp`.

Tip: for a lead tone, try High Mid about +3, Low Mid about -2, and Presence around 0.5.

## Noise pedal
A pitch-shifting "noise generator" at the front of the chain, with a dissonant Clash layer and a ring modulator added. It is a digital octave pitch shifter with two momentary footswitches (octave up, double octave up) and four knobs named Panic, Chaos, Speed and Rise, made for harsh, dissonant tones. The knob roles below are **my own design**.

### How the panel is laid out
From top to bottom:
1. **Four footswitches:** OCT UP, 2 OCT UP, CLASH, RING. They can all be on together.
2. **A LATCH button under each switch.** Off (default) = momentary: the switch is on only while you hold the button down. On = latching: click once to turn the switch on, click again to turn it off. If you turn LATCH off while the switch is on, the switch lets go. Each switch has its own LATCH setting, saved with the project.
3. **Three tabs:** OCTAVE, CLASH, RING MOD. The tabs only choose which settings you see. A lit tab has an amber line under it; it doesn't turn the effect on.
4. **The settings for the open tab.** On the CLASH and RING MOD tabs, the menus come first, then the knobs.

**Rise** and **Speed** are shared, so they appear on every tab:

| Knob | What it does |
|---|---|
| Rise | Glide time. For the octave switches: how long the pitch takes to slide up and back down. For Clash and Ring Mod: how fast the layer fades in and out. Low = instant, high = a slow swell over up to about 1.5 seconds. |
| Speed | Rate of the wobble: the octave wobble and random jumps (about 0.3 to 12 Hz), the Clash detune (about 0.15 to 6 Hz), and the ring mod Wobble (about 0.15 to 6 Hz). |

With no switch on, the pedal passes your signal through untouched. A DAW can automate the switches, the LATCH settings and all the knobs.

### OCTAVE tab
- **OCT UP** shifts your signal up one octave while on. **2 OCT UP** shifts it up two octaves. If both are on, two octaves wins.
- **Panic:** depth of a smooth, warbling pitch wobble (up to about 1.5 semitones).
- **Chaos:** depth of random, stepped pitch jumps (up to about 7 semitones), for glitchy, dissonant sounds.
- Panic and Chaos only act while an octave switch is on. At 0 for both you get clean octaves.
- Start with Rise about 0.3, Speed about 0.4, Panic about 0.3, Chaos about 0.3, then hold a switch and play single notes. Push Chaos up for the noisy, dissonant end.

### CLASH tab
A pitch effect for modern metal and djent with two knobs, DRIVE and BLEND, and three scenes: Bright is tense and alien; Dark is darker and more sinister; Full is maximum dissonance, "three-note chaos". **The intervals and processing here are my own design.**

What it does here: while CLASH is on, your signal is pitch-shifted into one to three clashing voices above the note you play, each voice slowly and randomly detuned so it never settles. The shifted voices are blended with your dry signal, and the lows are filtered out of the wet signal so the low end stays tight.

| Scene (menu) | Voices above the played note | Tone |
|---|---|---|
| Bright | tritone (+6 semitones) and a flat ninth (+13) | bright |
| Dark | minor second (+1) and tritone (+6) | dark, with the highs rolled off |
| Full | minor second (+1), tritone (+6) and major seventh (+11), all at once | full dissonance |

- **Drive:** how hard the signal is pushed (into soft clipping) before the pitch engine. Higher = more aggressive, harmonically messier, and a wider detune wobble.
- **Blend:** wet/dry mix. At 0 you hear only your dry signal. At 1 you hear only the Clash voices.
- Start with Drive 0.4 and Blend 0.5. Play single notes, hold or latch CLASH, then try the three scenes. Blend is good to automate for transitions.
- It runs three pitch shifters all the time, so there is some extra CPU use. The shifters can only go up, so every voice is above the note you play.

### RING MOD tab
A ring modulator multiplies your signal by a carrier tone. Each note's harmonics are replaced by sum and difference tones, which gives bells, metallic clangs and robot voices. This is my own simple design, not a copy of a particular pedal.

- **Mode (menu):** *Ring* is plain multiplication, the classic sound where the original notes mostly disappear. *AM* is amplitude modulation: the original notes stay, with a shimmering, tremolo-like layer on top.
- **Wave (menu):** the carrier shape. *Sine* is the cleanest, *Square* is harsher and buzzier, *Triangle* is in between.
- **Freq:** the carrier frequency, 1 Hz to 5 kHz (default 330 Hz). About 1 to 20 Hz is a tremolo-like chop. About 50 to 500 Hz gives growl and metal. Higher gives bells and alien tones. Notes you play will sound out of tune with the carrier on purpose; that's the effect.
- **Mix:** wet/dry. At 0 you hear only your dry signal.
- **Wobble:** moves the carrier up and down by up to an octave, at the Speed rate. At 0 the carrier stays put.
- Try: Ring, Sine, Freq about 150, Mix 0.6 for a growl. Raise Freq to about 800 for bells. Switch to Square and add some Wobble for a sci-fi siren.
- The carrier isn't band-limited, so Square and Triangle at high frequencies alias a little. That is part of the sound here.

### Limits
- The pitch shifter is the same simple delay-line type as the shimmer reverb. It works best on single notes, and chords get grainy.
- While an octave or Clash switch is on, your signal is delayed by a few tens of milliseconds, and it isn't latency-compensated. This isn't noticeable when you tap the switch for effect.
- Because the pedal is before the overdrive and amp, the shifted and ring-modulated notes get distorted along with everything else.

## Fuzz pedal
Switch **Fuzz On** to put a fuzz in front of the overdrive and amp. **Fuzz Style** picks one of four, each a hand-built approximation of the general character of a classic pedal, not a model of its components, so don't expect an exact match.

| Style | Character | How it's built |
|---|---|---|
| Wall | Thick, sustained, wall-of-sound | Two cascaded clipping stages with filters between them, then a tone control: a blend of a low-pass (about 400 Hz) and a high-pass (about 1.9 kHz), so the mids dip when Tone is in the middle. |
| Tight | Tight, aggressive, mid-forward distortion | A high-gain op-amp-style stage that gives highs far more gain than lows and loses bandwidth as the gain rises, then hard diode-style clipping, then a low-pass. |
| Gated | Gated, spitty, "velcro" fuzz that can squeal | Asymmetric clipping with a starved bias (Gate) and positive feedback (Stability). |
| Buzzsaw | Thick, sawing, scooped-mid metal buzz | A heavy-metal pedal circuit (v93): two gain stages, op-amp and clipper, then a Low / High tone section. Low and High are about -20 to +20 dB; there is no Tone knob. |

| Control | What it does |
|---|---|
| Fuzz Gain | Wall: sustain. Tight: distortion. Gated: drive. Buzzsaw: distortion. |
| Fuzz Tone | 0 = dark, 1 = bright, for all styles (Buzzsaw has no Tone knob; it has Low and High). (On the Gated style, which has no tone control of its own, it's a simple low-pass after the fuzz.) |
| Fuzz Level (dB) | Output level of the pedal. |
| Fuzz Gate (Gated style) | Gated style only. 0 = normal sustained fuzz. Higher = the stage is starved, so notes cut off and sputter as they decay. |
| Fuzz Stability (Gated style) | Gated style only. 1 = stable. Lower = more positive feedback, giving squeals and howls, mostly as notes decay. |
| Buzzsaw Low / High | Buzzsaw style only. Boost the low band (thump) and the high band (bite). |
| Buzzsaw Mid / Soft / Hard / Blend / Invert | Buzzsaw only. Mid is the upper-mid band split from the original single tone knob. Soft and Hard take the two clipper pairs in or out (both on = the original). Blend mixes in a clean copy of your input; Invert flips it. |
| Boost Mix | Blends the boost with the dry input (1 = all boost). |

Tips:
- **Feedback read from a second schematic (v90):** the creamy lead amp power section in Circuit-derived mode now uses a heavier feedback family, with the supply values confirmed by a second schematic. Classic is unchanged.
- **Distortion circuit mode and a classic wah sweep (v91):** the Circuit Mode switch on the Overdrive panel now also works for the Distortion style. It solves a transistor booster (gain 56, soft lopsided limit), the op-amp gain stage (gain 1 + Rdist / 4.7k, bass corner 72 Hz at full gain), hard diode clipping after a 2.2k / 10 nF low-pass, and the passive tone network, from the published resistor and capacitor values. It has no clean path and is far louder than the hand-tuned Distortion (about 5.5 to 8 dB above bypass at Volume -6 dB), so use Auto Level. The Clipping menu picks the diode pair (Silicon, LED, Germanium, Asymmetric); Auto Level is measured for Silicon only. Off by default, so saved sessions sound the same. The Wah gets a Voice menu: Generic (the Heel and Toe knobs, as before) or Classic circuit (a fixed 450 Hz to 1.6 kHz sweep, the range in the published analysis of a classic wah). Off by default.
- **Classic is back (v94):** the amp Mode menus (Classic default, or Circuit-derived) and the pedal Circuit Mode switch (off = the original hand-tuned Transparent, Tight, Screamer, Distortion) return, so the circuit work is a choice, not forced. Classic matches v92 exactly in my render tests. See CHANGELOG.
- **Second drive and the pedal changes (v93):** a second OVERDRIVE block ("Drive 2", starts off) that stacks after the first (Series) or runs beside it (Parallel, each -3 dB). The Transparent style is now a Centaur-type circuit with Gain, Tone and Edge (I took "grip" to mean Edge). Clean and Treble boosts moved into the Boost section, and the Dyna compressor into the Comp section (Style "Dyna (OTA)"). (The v93 removal of the Circuit Mode switch and the amp Mode menus was reversed in v94, above.) A second fuzz block ("Fuzz 2") works the same way as Drive 2. CIRCUITS.md lists every circuit received and what was done with it. Details and my estimates are in MODELED_GEAR.md.
- **More real component values (v89):** Circuit-derived mode now takes preamp cathode corners, a bright cap and supply values from schematics I have read, for two more amps (the high-gain amp and the Mark-style creamy lead amp). The creamy lead amp's low end is tighter than before. Classic mode is unchanged. Details and what was not applied are in MODELED_GEAR.md.
- **Circuit-derived by default (v88):** new sessions and factory presets start in Circuit-derived; older sessions keep the mode they had. Component-value preamp voicing exists only for the American combo and 50 W British amps, the ones I have schematics for. At high gain the new default is up to about 5 dB quieter than Classic on a couple of amps (more sag), within about 2 dB on most.
- **Amp mode (v87):** the Circuit Amp, Stage Model and Derived menus (and the Dual voice switch, Power, Class and Rectifier menus and the Metal Sag knob) are folded into one "Mode" menu per amp. **Classic** is the original voicing. **Circuit-derived** uses the circuit-derived stage curves and works out the power section, supply droop, transformer saturation, feedback loop and preamp corners from circuit values; in my timing test it costs about the same CPU as Classic (roughly 2% of real time against 1.5 to 2%, on the build machine). **Full circuit** is the 50 W lead-amp simulation; it is only for that one amp and measured about 45% of real time, roughly 20 times the others, with no extra reported latency. Amp B has Classic and Circuit-derived. Master only shows in Full circuit. The Stack menu now reads As voiced, Neutral, then the seven passive stacks. Older sessions are moved across: Circuit Amp on becomes Full circuit; Stage Model or Derived on becomes Circuit-derived; Power Half/Low adds some Power Drive; Silicon rectifier and Metal Sag scale the Sag knob; American/British stack settings become the closest passive stack. These are close, not identical. From v88 the default is Circuit-derived; Classic remains selectable and is unchanged from before. Dual voice is removed (its effect was subtle).
- **Circuit-derived levels (v83):** next to the Stage Model menu, a "Derived" menu (Amp B has its own in the advanced rows). Off is the classic voicing, as before. "Power section" replaces the Feel switch's tone nudges and the Presence shelf with the response of the power amp worked out from its feedback network, presence control, output transformer and a speaker impedance model: more feedback is tighter and stiffer, the Presence knob works by cutting feedback at high frequencies (its effect is milder than the classic shelf, as on the real amps), and the speaker's bass resonance lifts the low end. "Power + supply" adds a supply droop with two time constants (the reservoir recovers in tens of milliseconds, the filter stages in a few hundred) and a little low-end saturation of the output transformer, with depth from each amp's supply values (the Sag knob scales it; Rectifier Silicon makes it stiffer). "Power + supply + voicing" also takes the preamp corner frequencies and the bright-cap shelf from component values, for the American-combo and 50 W British models. Levels are matched to classic on a test signal. It costs no extra CPU. Not available for the Acoustic, Glass Clean and Digital models. Default is Off, so older sessions are unchanged.
- **Power stage with feedback (v86):** in Circuit-derived mode, the output stage runs inside its negative-feedback loop. Tight feedback stays clean longer and then clips harder; loose feedback breaks up earlier and softer. The Feel switch no longer nudges the drive by itself in that mode. Effect is subtle at most settings, since the preamp clips first.
- **Two more passive stacks (v84):** "high-gain, 680 pF" and "high-gain, 500 pF" in the Stack menus, from a two-channel high-gain amp: the British lead stack with a 220k treble pot and 22k mid pot, and a different treble cap. The two differ slightly in brightness.
- **Optical tremolo (v83):** the Tremolo Shape menu has a new "Optical (bulb and photocell)" entry: the oscillator lights a lamp, the lamp lights a light-sensitive cell; rounded, a little lopsided and shallower at fast rates, like the vibrato of a classic combo.
- **Spring Drive (v83):** the reverb shaping panel has a Spring Drive knob that works with the Spring reverb type: it pushes the spring's driver (thin lows, soft lopsided clip on hard hits, rolled-off top). At 0 it is off.
- **Passive tone stacks (v81):** the Stack menu (and Amp B's) now has five extra entries that replace the amp's tone filters with a real passive bass / mid / treble stack solved from component values: American combo, British 50 W, British lead, American lead and chime (class A). The three knobs interact like the real ones: the bass control mostly sets how much low end gets through, the mid knob fills in a notch whose depth depends on the other two, and the treble knob sweeps a long way. The pots use their real tapers, so the middle of the Bass knob on the American stack is a lower setting than on the British ones. Bass, Treble and Mid (the average of Low Mid and High Mid) drive the stack; the amp's own tone shaping (the voicing offsets) is replaced. Levels are matched with all three at the middle. A true stack costs almost no CPU. Default is the existing Stack setting, so older sessions are unchanged. (v81 also fixes the Bass knob of the v79 Circuit Amp, which worked backwards.)
- **Stage model: circuit-derived (v80):** next to the Circuit Amp menu (and in Amp B's advanced rows) is a Stage Model menu. "circuit-derived" replaces the plain clipping in the preamp stages and the power stage with curves taken from simulations of real tube stages. Each gain stage is hard on one side (cutoff) and soft on the other (grid conduction), the size of the asymmetry depends on the stage type used by that amp model (low-gain clean stage, cold class-A stage, crunch stage with a big cathode resistor, hot stage, hot cascade stage) and on the Preamp Tube choice; grid blocking makes a loud note briefly pull the operating point down and recover over about 45 ms; and the power stage follows the power tube (As voiced picks the tube that amp model is built around; EL34, 6L6, 6V6, EL84 or KT88 override it) with a hotter curve when Bias is above zero. It costs almost no extra CPU. Default is classic, so older sessions are unchanged. It does not apply to the acoustic, glass clean and digital amp models, which keep their own shaping. Levels are matched to the classic shaping on a test signal; expect small differences.
- **Circuit Amp (v79, first one):** on the AMP page, under the Bright row, pick "Circuit Amp: Lead 50 (full circuit)". Instead of the amp model's preamp, tone filters and power stage, a whole tube amp is solved from its schematic: two cascaded gain stages with a bright volume control, a third gain stage, a cathode follower, the passive bass/mid/treble stack (the three controls interact, and the stack loads the stage before it), a master volume, a long-tail phase inverter, a pair of EL34 power tubes into an output transformer, negative feedback with a presence control, and a power supply that droops when you play loud. Gain, Bass, Mids (the average of Low Mid and High Mid), Treble, Presence, Master, Bias, Sag, the preamp tube, the power tube and the rectifier choice all act on the circuit. The amp model menu, Voicing, Stack, Feel, Watt, Class, Deep, Auto Level and the Metal controls are not used while it is on. It runs at twice the host rate (so it uses noticeably more CPU than the normal amps: about a third of a core on a modern laptop at 48 kHz) and the pedals in front of it still work. Default is Off, so older sessions sound exactly as before. Power tubes other than EL34 use approximate curves.
- **Wall, Circuit version (v77):** pick "Circuit (from the schematic)" in the Wall Version menu. It solves the real fuzz circuit: an input booster, two diode-limited gain stages with the real filter corners, the passive tone network and the output stage. Gain is Sustain, Tone is the real tone network (so the mids dip as the pot moves), Diodes, Op-Amp and Tone Bypass work on the circuit itself, and the Mids knob is not used. **Variants (v78):** "Circuit (variant B, 500 pF)" and "(variant C, 470 pF)" are the later build of the same fuzz (smaller coupling and diode-series capacitors, 390 ohm emitters, 12k collectors, a 20k tone resistor): tighter lows, lower gain, a shallower mid dip. "Circuit (op-amp build)" is a different, op-amp based design with a resonant low-pass, a sustain divider, an op-amp clipper with three diodes each way and a Muff-style tone network; Tone Bypass skips the tone network.
- **Overdrive Circuit mode (v77):** on the Tight and Screamer pages, a Circuit Mode switch (off by default) solves the real circuit instead of the hand-tuned model. Same wide Drive range; Clipping gains "None (op-amp rails)".
- **Wall:** Gain about 0.6, Tone about 0.5 (mids scooped), Level about -9 dB. Sweep Tone to hear the low-pass to high-pass blend.
- **Tight:** Gain about 0.5 for tight crunch, up to 0.8 and beyond for full-on distortion. Fuzz Tone around 0.5.
- **Gated:** Gain about 0.5, Gate 0.3 to 0.5, Stab 0.4 to 0.8. Gate works on the actual signal level, so it interacts with Gain and with your guitar's volume. If everything cuts out, lower Gate or raise Gain. Turn your guitar's volume down to hear it clean up and gate differently, as on the real pedal.
- The defaults for Gate (0.2) and Stab (0.8) are a mild starting point. Low Stab with high Gate gets wild.
- The Gated style doesn't have a Comp control.
- **Buzzsaw:** Gain about 0.6, Tone about 0.5, Low about 0.6, High about 0.5, Level about -9 dB. Raise Low and High together for a bigger smile curve; sweep Tone to move the hole in the mids. Keep the amp's gain modest.

Limits:
- The fuzz runs at 4x its own sample rate (2x unless Amp Quality is Best) to reduce aliasing, which adds a few samples of latency while it's on. It isn't compensated for, and it uses extra CPU.
- Switching Fuzz On or Off, or changing Style, while playing may click.
- It sits before the overdrive and the amp's preamp, so the fuzz gets distorted further. For a pure fuzz tone, switch **OD On** off and lower Amp Gain.


## Saved sessions
Everything added after a release defaults to neutral or off, so older sessions load and sound the same. Sessions saved before v73 reopen on the page that now holds their old page's content. Details of each older change are in CHANGELOG.md.

I have not run or listened to the full plugin on a Mac; parts of the DSP were tested on their own. If the build shows errors, paste them.
