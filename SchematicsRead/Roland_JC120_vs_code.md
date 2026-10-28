# Roland JC-120 / JC-160: schematic read versus the plugin

Companion to `Roland_JC120.md` (same folder). Nothing in `Source/` or the existing docs was changed. All plugin facts below were confirmed by reading or grepping the files named; all JC numbers come from `Roland_JC120.md` (legible, derived or assumed as marked there).

## 1. Does the plugin model this amp?

**Yes in part. The premise "no model, apparently" is wrong.** Grep for `Jazz`, `Roland`, `Chorus`, `Tri-chorus`, `Dimension`, `Ensemble` and the solid-state clean amps found:

| Item | Where | What it is |
|---|---|---|
| **Jade Chorus**, amp model 10, "JC-120-style solid state" | `AMP_SOURCES.md` row 10; `MODELED_GEAR.md` line 31; `PluginProcessor.cpp` voicing rows 18 (Glass) and 19 (Warm), and the Funk row near line 559; `PluginEditor.cpp` line 1001 and 9751 (voicings Glass / Warm / Funk / Shimmer) | A hand-tuned amp ("My own tuning", AMP_SOURCES: "All R", i.e. read from the service notes for specs, power amp and channel 2 preamp, but "the clean stack is not" applied) |
| **Jade Chorus Distortion**, `jadeDist` (b39) | `PluginProcessor.h` `JadeDistStage` (line ~5302), run at `PluginProcessor.cpp` ~8028 | Gain stage into an asinh (Shockley-form) silicon clamp, after the tone EQ, only on amp 10 |
| JC "Clean" spring-reverb return | `CIRCUITS.md` line 255, `MODELED_GEAR.md` line 485 | Reverb style table row "JC-120 (1983)": op-amp plus emitter followers, current drive, recovery gain about 57 |
| **BbdChorus** (modes I, II, I+II, Dimension, Tri-chorus, Ensemble, Detune) | `PluginProcessor.h` line 3101; stand-alone call at `.cpp` ~6329 and modulation-slot type 12 at ~6419 | A generic mono BBD-style chorus; not tied to the JC (no mention of Roland in any chorus text) |
| **Jazz Box** (amp 32), **Bumble Lead** "Jazz" voicing | `PluginProcessor.cpp` lines 1079 and 349 | Different amps. Jazz Box is "a big, dark, solid-state jazz clean" that borrows nothing from the JC-120 schematic |

There is **no** string "Roland" in the Source tree (grep returned only docs). "Tri-chorus", "Dimension" and "Ensemble" occur only as BbdChorus voicing names and as the separate `WetEnsemble` class (reverb output), not as amp models.

What does not exist: a schematic-derived **clean** path (JFET input, passive TMB stack, volume with bright, flat solid-state power amp), a JC entry in the Tone Stack menu (`ToneStackTmb.h` `valuesFor` has types 0 to 11, none a JC; the default arm returns a generic set), the JC chorus/vibrato as a mode, and the stereo two-amp output.

## 2. What the existing docs say that the schematics contradict

(I did not edit those docs. These are the points where the notes differ from the eight pages.)

| Existing note | What these pages show |
|---|---|
| `CIRCUITS.md` b58 re-read: the 1979 front end is a "complementary bipolar pair (Q18 2SC1000, Q19 2SA493 ...)" on a board "AP-44" | Pages 5 to 8 show boards **AP-113 (CH-1) and AP-114 (CH-2)** with 2SK117GR JFET front ends. The b25 note ("JFET Q1, 1M gate resistor") agrees with these pages; the b58 "correction" does not. The b58 pot values (50k / 100k / 5k) and stack parts (18k, 0.0015 uF, 0.47 uF, 0.15 uF) do match AP-113 / AP-114 |
| b25: "Bass 250kA with 470k", "Distortion: JFET Q3 with D1/D2 back to back to ground through 33k and 15k" | Stack wiring is resolved (Roland_JC120.md section 3): bass pot is a rheostat in series, no 470k. Distortion: D1/D2 clamp the Q3 **gate** node after R17 33k; Q3 drain load is R23 15k (that is the 15k) |
| Spec table: "treble +/-35 or 37 dB, middle 0.6 to 0.8 dB, bass +/-17 to 18 dB, input sensitivity 11 mV (1979) or 15 mV (1983)" | Printed: 1983 treble 17 dB @ 10 kHz, middle 13 dB @ 350 Hz, bass 14 dB @ 50 Hz, sensitivity HIGH 20 mV; 1979 treble 38, middle 2, bass 18 dB; HIGH 22 mV (11 mV is the 10 kHz figure on the 1979 sheet) |
| "Middle barely changes tone" | True only for the 1979 stack at its spec setting (2 dB at 1 kHz). The 1983 middle control gives 13 dB at 350 Hz and the notch is 22 to 38 dB deep |
| b58: "none of the three standard tone-stack layouts the solver supports fits" | The wiring fits the plugin's existing 7-node netlist (section 3). Its response matches my independent solve to 0.1 dB at the checked settings |

## 3. A JC tone stack for `ToneStackTmb.h` (new types, not applied)

`Values` is `{ R1 treble pot, R2 bass pot, R3 mid pot, R4 slope, C1 treble cap, C2 bass cap, C3 mid cap, Rsrc, Rload, bassAudio, trebleAudio }`.

```
// 12  JC-120 (1983) CH-2: 250kB treble (linear), 250kA bass (audio), 10kB mid, 100k slope, 150 pF, 0.082 uF, 0.047 uF
//     driven by Q1's 12k drain load; loaded by Q2's bias network (R13 470k + R12/R11 about 110k = about 580k)
{ 250e3, 250e3, 10e3, 100e3, 150e-12, 0.082e-6, 0.047e-6, 12e3, 580e3, true, false }
// 13  JC-120 (1979) AP-113 / AP-114: 50kB treble, 100kA bass, 5kB mid, 18k slope, 1.5 nF, 0.47 uF, 0.15 uF
//     source about 150 ohm (source follower, ASSUMED), load 100k volume pot
{ 50e3, 100e3, 5e3, 18e3, 1.5e-9, 0.47e-6, 0.15e-6, 150.0, 100e3, true, false }
```
CH-1 (1983) differs from CH-2 only in the mid capacitor (0.056 uF) and the load (volume pot 1MB, assumed).

How well the plugin netlist reproduces my independent solve (dB at 50 / 100 / 350 / 1k / 3k / 10k Hz):

| Setting | 1983 mine | 1983 plugin netlist |
|---|---|---|
| 5/5/5 | -10.9 / -13.9 / -24.3 / -21.8 / -12.9 / -8.5 | -10.8 / -13.9 / -24.3 / -21.8 / -12.9 / -8.5 |
| 10/10/10 | -10.8 / -14.9 / -23.5 / -16.8 / -8.0 / -2.5 | identical |
| 5/5/0 | -11.1 / -14.2 / -27.4 / -24.3 / -13.7 / -9.1 | -10.9 / -14.1 / -27.4 / -24.3 / -13.7 / -9.1 |
| range checks (printed 17 / 14 / 13) | 17.2 / 14.0 / 13.2 | identical |

So for 1983 the existing solver is already right; only a menu entry and two `case` rows are needed. Two known gaps:

- **All controls at zero:** mine -45 dB flat, plugin -25.6 to -46.9 dB (the plugin keeps a 1 ohm floor on the pot ends and its mid pot is a 3-terminal pot, mine a rheostat). Only matters at the extreme corner.
- **1979 bass:** with the 1979 spec conditions (treble 10, mid 0) the plugin netlist gives a bass range at 100 Hz of 8.2 dB against 19.0 dB for my solve and 18 dB printed; treble range 40.9 dB against 38.4 / 38 printed. The difference is the 100 ohm tail resistor under the mid pot (R11) which the plugin netlist has no field for. Case 13 therefore needs an extra field (a new `Rtail` or reuse of `Rn`'s sibling) before it reproduces the 1979 sheet. The 1983 1k series resistor (R158 / R43) is not needed: leaving it out changes nothing at the checked settings.

Insertion loss to compensate in the gain staging (derived, at 1 kHz, flat 5/5/5): 1983 -21.8 dB (the notch at 518 Hz is -26.5 dB, and 10 kHz is -8.5 dB); 1979 -13.8 dB (notch -17.9 dB at 436 Hz). With Middle at 10 the 1983 notch is -22 dB; at 0 it is -38 dB.

## 4. What a faithful "solid-state clean" JC model needs

Stage list, gains from the printed levels (1983 CH-2 unless marked; "derived" = my arithmetic):

| # | Stage | Gain / level | Numbers for the model |
|---|---|---|---|
| 1 | Input select | HIGH 19 mV, LOW 59 mV (-9.8 dB), 33k / 68k series, 1M bias | LOW = 0.32 x HIGH |
| 2 | Q1 JFET, 12k drain load, 47 pF across | **x9.5 (+19.5 dB)**: 19 mV to 180 mV | RF pole: 47 pF only; no tube-style bias shift |
| 3 | Passive TMB stack | **-21.8 dB at 1 kHz flat** (-17.5 dB printed at T10 B10 M0) | case 12 above |
| 4 | Recovery JFET Q2 and the branch to Volume | 24 mV in, 20 mV at the wiper (printed) | high input impedance, about 580k load on the stack |
| 5 | Volume 1MB with Bright C5 220 pF (CH-1: 330 pF) | bright boost 7.5 dB at 10 kHz at 50% volume, 12.7 dB at 25%, 3.5 dB at 75%, 0 at 100% (derived; printed spec 5 dB at centre) | cap across the top half of the pot; corner 1.45 kHz (CH-1 0.96 kHz) at the half-pot resistance |
| 6 | Q5 JFET, 12k load | x10 (+20 dB): 20 mV to 200 mV | |
| 7 | Q16 / Q15 output stage | x10.5 (+20.4 dB): to **2.1 V** | |
| 8 | Mix to MAIN IN | 2.1 V down to 0.8 V needed: about -8.4 dB | the exact mix network is not resolved (?) |
| 9 | Power amp | **x28 (+28.9 dB)** closed loop, corner 16 Hz (1983); x48 on the 1979 sheet (printed sensitivity implies about 31, unresolved) | rated 22 V rms at 0.8 V in |
| 10 | Output | +/-37 V rails (1983), +/-43 V (1979), 8 ohm per channel | clip at 32 to 34 V peak (1983): 0.2 to 0.8 dB over rated; 1979 about 2 dB |

Total gain (derived) HIGH input to speaker with volume at maximum and stack flat: 19 mV in, 19.5 dB, -21.8 dB, 0 dB recovery, +20 dB, +20.4 dB, -8.4 dB, +28.9 dB = +58.6 dB, i.e. 19 mV to about 16 V rms; the rated 22 V needs about 3 dB more, which is within the unresolved mixing network and stack-setting uncertainty. Flag as an estimate.

Behaviour the model must capture, which the current `AmpVoicing` row cannot:

1. **Clean until the rails.** Voicing rows 18 / 19 already have `sag = 0.0` and bias 0 (symmetric), which is right. The 1983 amp clips at rated power plus 0.5 dB; the plugin should clip the final stage hard at the rail ratio (about +/-33 V of 37 V) and not before. In the plugin's Glass voicing the gain range is `gainMin {0.8,0.8,1.0}` to `gainMax {2.0,1.6,1.0}` per stage, so the pre-power-amp stages never exceed 2 x. Real JFET stages swing about +/-1 V before clipping (Q1 at 12.1 to 13.6 V DC with a 180 mV level, i.e. 32 dB above the 19 mV sensitivity, derived). Both are consistent with "very clean".
2. **Supply ripple instead of sag:** with 3300 uF per rail at 2.5 A each, ripple is 6.3 V (60 Hz mains) to 7.6 V (50 Hz) peak-to-peak at full power on both channels (derived). That modulates the clip level at 120 / 100 Hz rather than sagging slowly; a model would add a 100 or 120 Hz term to the rail value, scaled by output current.
3. **Stack before volume:** the real order is input stage, stack, recovery, volume (with bright), then gain. In the plugin the shelf/peak EQ (`bassShelf`, `lowMidPeak`, `highMidPeak`, `trebleShelf`) runs before `jadeDist`; I did not check its position relative to the gain stages. For clean use the order is nearly harmless; for the Distortion channel it matters (below).
4. **Voicing shape:** the plugin's Glass voicing is `voiceHz 3200, +3 dB, Q 0.7`; bass shelf 100 Hz 0 dB; low-mid 400 Hz Q 0.8 at -1 dB; high-mid 2500 Hz +0.5 dB; treble shelf 4500 Hz +4.5 dB; presence 6000 Hz +9 dB; cab low-pass 9500 Hz. Against the stack at 5/5/5 (relative to the 100 Hz level, which is -13.9 dB): 350 Hz is **-10.4 dB**, 1 kHz -7.9 dB, 3 kHz +1.0 dB, 10 kHz +5.4 dB. The plugin's low-mid dip is -1 dB, so the real stack is far more scooped than the voicing row; either the row is deliberately milder or it was voiced by ear. The JC model of the 1983 stack should be used with the Tone Stack menu, not stacked on top of the present shelf/peak offsets.

## 5. Distortion channel: schematic versus `JadeDistStage`

| | Schematic (1983, derived with assumed gm 15 mS) | Plugin (`JadeDistStage`) |
|---|---|---|
| Position | after the stack, before Volume (clean branch joins the same node) | after the shelf/peak EQ, before `ampTrim` (volume position unknown) |
| Order of gain and clip | R17 33k then **diodes at the Q3 gate** (clamp +/-0.6 V at the input), then Q3 gain, then divider | gain first (`g * hp / vk`), then asinh clamp |
| Gain | Q3: 4.5 (13 dB) at pot minimum to about 40 (32 dB) at maximum; then -6 dB divider | 0 to 40 dB (`40 * amount^0.8`), then level make-up `-0.42 x gain` |
| High-pass | 97 Hz (C12 0.082 uF into 20k) | 90 Hz |
| Low-pass | Miller 100 pF: 8.8 kHz at low gain down to 1.2 kHz at high | 8 kHz at amount 0 falling to 2.8 kHz at amount 1 (`2800 + 5200 x (1 - amount)`) |
| Clip knee | silicon diode about 0.6 V into a stage with gain | `vk = 0.18` in plugin units |
| Control law | 10kC (reverse log): gain changes mostly in the first third | `amount^0.8` (near linear in dB) |

What agrees: hard symmetric silicon clip after the EQ, high-pass near 90 to 100 Hz, a darkening top end as the knob goes up. What differs: the real diodes are at the **input** of the gain stage, so the signal is limited to about +/-0.6 V and then amplified (output swing set by the JFET and its 15k load, not by the diodes); in the plugin the diodes come after the gain. This produces different behaviour: in the real circuit the clip is first and the gain stage's own saturation then rounds the result, and the output amplitude of the diode-limited signal is nearly independent of drive. The 1979 sheet has **no diode clip** in the signal path that I could find: Q5's saturation (gain 4.3 to 133) does the work (Roland_JC120.md section 5.2).

## 6. JC chorus compared with the existing BbdChorus modes

Plugin numbers are from `BbdChorus::process` / `processMulti`; JC numbers from `Roland_JC120.md` section 6 (1979 printed clock range with an **assumed** 512-stage MN3004; 1983 uses an MN3007, assumed 1024 stages).

| | JC-120 chorus (1979 / 1983) | Mode I | Mode II | Mode I+II | Dimension | Tri-chorus | Ensemble | Detune |
|---|---|---|---|---|---|---|---|---|
| LFO rate | **0.77 Hz** printed (1.3 s); computed range 0.47 to 1.30 Hz via trimmer; 1983: 0.83 Hz (1.2 s) | 0.513 Hz | 0.863 Hz | 9.75 Hz | 0.31 Hz | 0.55 Hz | 0.60 Hz (+6.3 Hz shimmer) | 0.18 Hz |
| Rate range with knob | none (service trimmer only) | x0.25 to x4 (0.13 to 2.05 Hz) stand-alone; modulation slot 0.02 to 20 x | same | same | same | same | same | same |
| LFO shape | triangle | triangle | triangle | triangle | sine | sine | sine | sine |
| Delay range (peak to peak) | **1.16 to 5.12 ms** (3.96 ms), centre 3.14 ms (1979); 1.28 to 6.14 ms (1983, 4.86 ms) | 1.66 to 5.35 ms (3.69 ms), centre 3.505 | same as I | 3.31 to 3.71 ms (0.4 ms) | 4.15 to 4.85 ms (0.7 ms) | 3.7 to 7.3 ms (3.6 ms) | 4.9 to 8.1 ms (3.2 ms) + 0.44 ms | 9.0 to 13.0 ms (4 ms) |
| Depth control | none in chorus (fixed); vibrato has a Depth pot (VR8 100kB) | `depth` x1.845 ms swing | same | x0.2 ms | x0.35 ms | x1.8 ms | x1.6 ms | x2.0 ms |
| Lines | **one** BBD line, one LFO; wet mixed with dry; dry reaches both amps (my reading), wet through the second amp (forum report; wiring not resolved) | two lines moving in **opposite** directions (+tri / -tri), averaged | same | same | two, opposite | three, 120 degrees apart | three + shimmer | two, opposite |
| Output | **stereo** (two power amps, two speakers) | **mono** (copied to both channels) | mono | mono | mono | mono | mono | mono |
| Mix | wet only in the second amp (or 100% wet in vibrato) | `mix` (0.5 default) | same | same | same | same | same | same |
| Filtering | pre-BBD low-pass about 23 kHz (Sallen-Key, derived); post-BBD two-section reconstruction filter -3 dB at about 6.3 kHz, -24 dB at 10 kHz (derived; topology reading flagged) | biquad low-pass `Tone` 2 to 14 kHz, default 8 kHz, one per line | same | same | same | same | same | same |
| Noise | none modelled; BBD hiss not shown on the pages | optional `Hiss` | same | same | same | same | same | same |
| Vibrato | separate path: sine oscillator, **1.8 to 9 Hz** (1979), 2.0 to 8.3 Hz (1983), clock 65 to 130 kHz = **1.97 to 3.94 ms**, 100% wet | not a mode | not a mode | not a mode | | | | |

Observations:

- **Mode II is the closest to the JC.** Rate 0.863 Hz against 0.83 Hz (1983) and 0.77 Hz (1979); delay span 3.69 ms against 3.96 ms (1979); both triangle LFOs. With the Rate knob (or mod-slot rate) at 0.89 the 1979 rate is hit exactly; depth 1.07 gives the 3.96 ms span, depth 1.32 the 1983 span of 4.86 ms (the depth parameter goes to 1.5). Centre delay differs: 3.505 ms against 3.14 ms (1979) or 3.71 ms (1983, arithmetic).
- **The averaging of two opposite-moving lines is not what the JC does.** With one wet line and the dry signal, the comb moves in one direction and produces a flanging-like pitch deviation in the wet; the plugin's opposite-line average cancels most of the net pitch modulation and produces mostly a comb-depth wobble. The JC wet pitch deviation is the derivative of the delay: peak delay slope = span x 2 x rate = 3.96 ms x 2 x 0.77 = 6.1 ms/s, i.e. about 0.6% pitch (10 cents) peak. In the plugin the sum of two opposite lines at equal weight has zero net first-order slope, which is the "soft widening" character that makes Mode I/II sound more like a Dimension D than like a JC.
- **Stereo behaviour is the largest difference.** The JC's "chorus" is a spatial effect created by feeding a modulated mono wet signal to one speaker and a mostly dry signal to the other (my reading, forum-supported, wiring unresolved); the plugin's chorus is mono in and mono out, copied to both channels, so Mix=0.5 sums dry and wet in the same channel. A faithful JC voicing needs a stereo path: L = dry, R = wet (mix 1.0). This also means no comb-filter notches in the acoustic dry-plus-wet sum at the player's position; the notches occur in the room instead.
- **The JC has no depth or rate controls for chorus.** The plugin exposes them; leave them at the JC values for a "JC Chorus" preset: mode II, rate x0.89, depth 1.07, Tone 6.3 kHz, mix 1.0 into the right channel only.
- **Vibrato mode is not available.** The nearest settings (Mode II, Mix 1.0, rate x2.1 to x10.4 for 1.8 to 9 Hz) still average two opposite lines, which removes most of the pitch modulation, so the result is not a vibrato. A true vibrato needs a single line, 100% wet, centre 2.95 ms, swing about 0.99 ms (1979), triangle or sine at 1.8 to 9 Hz. Pitch deviation at the fast end: 1.97 ms span x 2 x 9 Hz = 35 ms/s = 3.5% (60 cents) peak; at 1.8 Hz it is 0.7% (12 cents).
- **Delay vs clock.** A BBD's delay is N / (2 x f_clock). If the control voltage moves the clock linearly (assumed, not shown), the delay moves hyperbolically, slower at the long-delay end. The plugin sweeps delay linearly. At the extremes of 50 to 220 kHz this makes the real sweep spend more time at long delays; the effect on sound is small but audible on a pure tone.
- **Companding:** no compander IC (NE570 family) appears in the parts lists, unlike many later BBD units; the dark filtering at 6.3 kHz carries the "BBD sound".

## 7. Suggested work list (not done)

1. Add tone-stack types 12 (JC 1983) and 13 (JC 1979, needs an `Rtail` field), then set Jade Chorus to use type 12 by default and re-voice the shelf/peak offsets against the -21.8 dB stack loss.
2. Add a JC-specific clean-stage chain: Q1 x9.5, stack, Volume with bright (220 pF / 330 pF over a 1MB pot), Q5 x10 and output stage x10.5, power stage x28 with rail clip at +/-33 V and 100 / 120 Hz rail ripple.
3. Re-order `JadeDistStage` to clamp first (diodes at the gate after 33k), then gain (13 to 32 dB), then -6 dB; keep the 97 Hz high-pass.
4. Add a "JC" chorus mode: single wet line, fixed 0.77 Hz triangle, 1.16 to 5.12 ms, the 6.3 kHz reconstruction low-pass, dry in one channel and wet in the other; add a single-line vibrato mode at 1.8 to 9 Hz.
5. Get the BBD stage counts from the Panasonic MN3004 / MN3007 datasheets (assumed here as 512 and 1024) before treating the delay times as final.

## 8. Unresolved items that affect the comparison

- Stage counts (assumed), the 1983 clock-label meaning (period before or after the divide-by-two, see `Roland_JC120.md` section 6.2).
- Stereo routing of the wet signal (single line seen; which amp receives it is not resolved).
- 1979 power-amp feedback gain (48 read, 31 implied by the printed sensitivity).
- LFO amplitude (25 V pp printed against 17.6 to 19.7 V pp derived).
- Whether the clean branch is muted when Distortion is on (1983).
- Values of the volume position and the mix network between the 2.1 V output and the 0.8 V MAIN IN level.
