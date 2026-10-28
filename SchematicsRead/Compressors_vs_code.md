# Compressor documents versus what the plugin assumes

Compared: my notes (`LA2A_notes.md`, `CS3_notes.md`, `Ross_notes.md`) against `MODELED_GEAR.md` (sections "Compressor audit" b59 near line 362, "FET and Opto compressors" b36 at line 705, the Dyna and Sustain readings, "Thump style" b31, "Opto Sustain" b58, "Crush Limiter" b62), `CIRCUITS.md` (the Dyna Comp row), `DATASHEET_AUDIT_b58.md` (the "Compressor styles against the filed sources" table) and the actual numbers in `Source/PluginProcessor.h` (CompressorEffect, lines 2678 to 2900), `Source/CircuitModels.h` (Circuit::DynaComp, line 1060), `Source/FxExtra.h` (OptoSustainFx, CrushLimiterFx). No code or existing doc was changed.

Source tags: UA = Universal Audio LA-2A manual (`manual` and `doc70f05` are the same document, pixel-identical); TT = Teletronix 1966 manual with the 11-66 addendum (`preservation`).

How the plugin builds the "Opto" style (so the comparisons below make sense): style 2 of the Comp section is the shared feed-forward compressor with these settings: soft knee **14 dB**, attack = Attack knob x 2 with a **1.5 ms floor** (default Attack 5 ms, so 10 ms), level detector 3 ms, **two release stages** (each carrying half of the gain reduction): a fast stage with a 20 ms time constant (scaled by Release/250 ms) and a slow stage with time constant (0.11 + "held") x Release/250 ms, where "held" is a slow memory (1.2 s smoothing) of the gain reduction divided by 8 dB, clipped to 0 to 1. Ratio comes from the user's Ratio menu (1, 2, 3, 4, 6, 10, 20, Limit = 100:1). Default Comp Saturation is "Tube-like" (an even-harmonic stage that gets stronger with gain reduction); "Opto tube stage" is a half-strength version of it.

## 1. LA-2A (Opto style): where the documents and the code agree or disagree

| Quantity | Documents | Plugin | Verdict |
|---|---|---|---|
| Release, first half | 0.06 s for 50 % release (UA spec, TT spec 9). UA theory: cell "releases quickly (40-80 ms) to about half its off resistance" | Model's 50 % release time, computed from the two stages at default Release = 250 ms: **29 ms** after a short squeeze (held = 0), **51 ms** at held = 0.5, **59 ms** at held = 1 (8 dB or more of remembered gain reduction) | Agrees at the long-squeeze end (59 ms against 60 ms). **Disagrees by about 2x after a short squeeze (29 ms)**. The code comment says "95 % of its half in 60 ms", which is a different thing from "50 % of the total in 60 ms". |
| Release, complete | 0.5 to 5 s depending on the previous reduction (UA, TT spec); TT circuit text says 1 to 15 s | 99 % release (computed): **0.43 s** (short) to **4.3 s** (held = 1) at default Release | **Agrees** with the 0.5 to 5 s spec (slightly short at the bottom). The TT "1 to 15 s" is not reached; the code comment says the 0.5 to 5 s figure was chosen on purpose. |
| Two-stage release shape | UA: fast to about half, then slow remainder over seconds; memory makes it slower after long or deep compression (UA page 10) | Two parallel stages, half of the reduction each; the slow stage lengthens with the memory (0.11 to 1.11 times 250 ms) | **Agrees in kind.** The doc's "memory" depends on how long **and how bright**; the code's depends on average gain reduction (8 dB = full), fine. |
| Attack | **TT: "essentially instantaneous (10 usec)"** (spec 8). **UA: "very fast attack time"** (no number). TT page 4: the cell is selected for minimum attack time | Attack knob x 2, floor **1.5 ms**, default **10 ms**. The code comment cites "a vendor page" (10 ms average) and the 1966 manual (instantaneous) and uses the vendor figure as the centre | **Disagrees with both documents I was given.** Neither UA nor TT says 10 ms; TT says 10 microseconds, which is 150 times faster than the 1.5 ms floor (the EL panel is the 10 usec part; the CdS cell is slower, but no document gives a number). The 10 ms is not supported by the three LA-2A documents. |
| Attack/release adjustable? | No: "the attack and release of the LA-2A are completely determined by the T4" (UA page 9), no controls | Attack and Release knobs exist for every style; Opto uses them as scale factors | Not a numeric disagreement, but the hardware has no such knobs. |
| Ratio, Limit | Horizontal after the knee: "the output increases less than 1 dB" for 20 dB more input (TT page 3); Limit = higher ratio (UA) | "Limit" = 100:1 (slope 0.99); 20:1 = slope 0.95 | **Agrees** (100:1 gives 0.2 dB for 20 dB; 20:1 gives 1 dB). |
| Ratio, Compress | about **3:1** ("100 % more input gives 30 % more output", TT-add); UA: "gentle, low ratio" | menu has 3:1 and 2:1 and 4:1 | **Agrees** (3:1 is a menu entry). The plugin does not tie the ratio to a Compress/Limit switch; the user picks it. |
| Knee / threshold shape | Breakaway at about **-30 dBm input**, curve flat by -20 dBm, so about **10 dB** of rounding ("gradually increases over the first 10 DB of input level rise", TT page 5); plot plateau about +5.3 to +5.5 dBm out | Soft knee **14 dB** wide (7 dB each side of the threshold, centred) | Same order; the code's knee is about 40 percent wider than the 10 dB the TT text gives. Not measured against Figure 1. |
| Threshold | Fixed; Peak Reduction changes side-chain gain, so it moves threshold and amount together (UA page 10) | Independent Threshold knob | Differs in control law, not in numbers. |
| Gain reduction range | up to 40 dB (UA, TT) | Not capped in the shared compressor (a ratio of 100:1 and a low threshold allow more) | No code limit at 40 dB; not checked whether the meter or the makeup stops there. |
| Detector | **None**: audio drives the EL panel directly, no rectifier or smoothing (TT page 4); side-chain frequency tilt via R37 up to about +10 dB at 15 kHz (TT) | A feed-forward level detector (3 ms), detector high-pass defaults to 80 Hz (menu: Off / 80 / 160 / 320 Hz) | Different structure. The doc's flat (or high-boosted) side-chain is not the same as a high-pass; default 80 Hz is not from these documents. |
| Feed-forward vs feedback | UA: "feed-back style"; TT schematic: side-chain taps the input transformer ahead of the attenuator (feed-forward) | Feed-forward | The documents disagree with each other; the plugin matches the TT drawing. |
| THD | **below 0.35 % at +10 dBm, below 0.75 % at +16 dBm** (UA, TT); under 0.5 % at 30 dB of gain reduction (TT); "no waveform or harmonic distortion" from the attenuator (TT intro) | Not tied to these numbers. Default Comp Saturation = "Tube-like": an asymmetric even-harmonic stage whose strength rises with gain reduction (k = 1 + 2 x depth); "Opto tube stage" halves the depth term | **Undetermined / probably much more distortion than the documents.** The documents describe a very clean unit (the tube stages are not driven harder when reduction rises, TT page 3). I did not measure the plugin's THD. |
| Frequency response | +/-0.1 dB, 30 Hz to 15 kHz | Not applicable to a gain model | n/a |
| Noise | 75 dB below +10 dBm (UA, TT spec); 70 dB (UA appendix) | Not modelled | n/a |
| Makeup / levels | gain 40 dB; +10 dBm nominal; +16 dBm peaks | Auto Level table measured on the plucked test signal; not tied to these | n/a |

Things the documents give that the code has no counterpart for: the Compress/Limit switch parts (not given anywhere), the R37 side-chain frequency control, the stereo link, and the doc's 0.5 to 5 s complete release as the only place the slow stage's maximum comes from.

Things the code says about its own sources that I could not confirm from the three documents: "a vendor page says 10 ms" and "1 to 15 s in a vendor page" (the 1 to 15 s is in TT page 4; the 10 ms is in none of the three).

## 2. The other optical style: Opto Sustain (Pigtronix Philosopher's Tone idea)

Not an LA-2A model (MODELED_GEAR b58: "from public descriptions only, every number is mine"). Against the LA-2A documents: attack 2 ms fixed (docs: 10 usec EL panel, "very fast"); release 90 ms fast half and 0.45 s slow half stretching to 0.95 s with squeeze memory (time constants; 99 % release about 2 s to 4.4 s, so inside the LA-2A's 0.5 to 5 s but with a faster first half than the LA-2A's 60 ms-to-50 % requirement would give: 90 ms tau puts the 50 % point near 62 ms for the fast half alone); slope 40:1 (docs: Limit is horizontal, 100:1 or more). Consistent in spirit, no number conflicts that matter.

## 3. Dyna (OTA) versus the Ross notes and the CS-3 notes

The plugin's Dyna model (Circuit::DynaComp, from the ElectroSmash Dyna Comp analysis per MODELED_GEAR b37) assumes: transistors pull a **10 uF** capacitor down, **150k** recharges it (**1.5 s**), detector smoothing **1M / 10 nF = 10 ms**, an input high-pass, a high-frequency lift (about 1 dB at 300 Hz to 4 dB at 10 kHz), +8 dB output trim, and gain linear in the capacitor voltage. The ratio, attack, release, and K are estimates (MODELED_GEAR table: "the page gives no ratio, attack or release").

| Item | Ross schematic (my read) | Plugin Dyna | Verdict |
|---|---|---|---|
| Gain element | CA3080 OTA, control current into pin 5 through VR1 + R12 27k | OTA, gain linear in the capacitor voltage (the code comment says "the transconductance amp runs out of range" with a tanh) | Agrees in kind. |
| Reservoir | **C16 10 uF** | 10 uF | **Agrees.** |
| Recharge | **R18 150k**, so R x C = **1.5 s** | 150k, 1.5 s | **Agrees.** |
| Smoothing | **C14 0.01 uF x R17 1M = 10 ms** (and C13 x R16 for the inverted half) | 1M / 10 nF = 10 ms | **Agrees.** |
| Detector | Full wave, two transistors (Q2, Q3) from a phase splitter Q5, feedback (Q5 follows the OTA output) | Two rectifier transistors, "the detector listens to the cell's output" | **Agrees.** |
| Defaults | fixed, no knobs | **Attack knob default 5 ms, Release default 250 ms** (the code feeds the knobs in as the 10 ms and 1.5 s; min 0.1 ms and 5 ms) | **Disagrees at the defaults**: the circuit's 10 ms and 1.5 s are the real fixed values, the plugin's default knobs are 5 ms and 250 ms (the knobs can reach 3.5 s). |
| OTA load / tone | OTA output is loaded by **150k with 1 nF**: a lowpass pole near **1.06 kHz**; OTA input pin 3 has a **15k (1 uF) with 0.01 uF** shunt network; input trimmer 2k | A shelf (+3 dB at 3 kHz) and two high-passes (32 Hz, 90 Hz) | The HF lift in the code was taken from the ElectroSmash page, not from this schematic; this circuit has a 1 kHz output pole and a 15k/0.01 uF branch that I did not turn into a curve. Cannot say they agree. |
| Sustain control | VR1 500k linear in series with 27k into pin 5: current 0.3 mA to about 16 uA | Sensitivity K = 2 + 60 x sens | Different parametrisation; no numbers to compare. |
| Ratio | No ratio spec | Menu ratio "applies to every style except Dyna" | n/a |

CS-3 versus the plugin's "Sustainer" style (style 4: knee 8 dB, attack floor 4 ms, slope = 1 - 1/(1 + (ratio - 1) x 1.5), release factor 1.5 to 2.5 times the Release knob, i.e. 375 to 625 ms at default):
- CS-3 envelope reservoir C8 **4.7 uF**; recharge path Attack pot 0 to 250k plus R2 22k: **0.10 s to 1.28 s** (derived, and uncertain which knob end is which). The plugin's 375 to 625 ms default sits inside that range. The plugin's attack floor of 4 ms matches my derived few-ms detector attack (a rough estimate, 2 to 7 ms, not a spec).
- CS-3 detector is full-wave, high-passed at about **339 Hz** on both arms (47k x 0.01 uF), fed from the VCA output (feedback). The plugin's detector high-pass menu is Off / 80 / 160 / 320 Hz (default 80 Hz); 320 Hz is the nearest to the CS-3's 339 Hz.
- CS-3 tone stage: treble boost/cut, up to +9.9 dB at high frequency, corner about 1.25 kHz. No equivalent in the Sustainer style (the style has no such tone control).
- Neither CS-3 nor Ross provides a ratio, so the plugin's Sustainer slope is unverified against them. The service notes give no gain-law or ratio figures.

## 4. Thump and Crush Limiter

- **Thump (b31)**: "own design, nothing matched to a unit" (MODELED_GEAR). It is the Standard compressor with Bloom (up to 2.5x the gain reduction, up to 3x slower release), Smack (a parallel compressor) and Clarity (a tilt EQ). None of the documents I read (LA-2A, CS-3, Ross) has numbers that bear on it. Nothing to compare. (Bloom's slower, deeper squeeze is conceptually like the LA-2A's memory but is not derived from it.)
- **Crush Limiter (b62)**: modelled on the Soundtoys Devil-Loc from its public manual (attack about 1.3 ms, release about 1.7 s, 0.85 s on Fast, infinite ratio with a 3 dB soft knee, Crush 0 to +36 dB into the detector, release stretching about 13 times at full Crush). No relation to the three documents. Loosely, its 1.7 s single-stage release sits inside the LA-2A's 0.5 to 5 s complete-release range and its infinite ratio matches the LA-2A Limit curve (horizontal), but it has no fast first stage (the LA-2A has 60 ms to 50 %) and an attack of 1.3 ms against the LA-2A's "essentially instantaneous". Not a disagreement, since it is not claimed to be an LA-2A.

## 5. Short list of the disagreements worth acting on (not acted on)

1. **Opto attack**: code floor 1.5 ms, default 10 ms; TT says 10 microseconds, UA says "very fast"; no document says 10 ms.
2. **Opto fast stage**: with a short squeeze the 50 % release is about 29 ms in the model against 60 ms in both documents (correct only after 8 dB or more of memory).
3. **Opto THD**: documents say below 0.35 % (+10 dBm) and below 0.75 % (+16 dBm); the plugin's default Comp Saturation adds even harmonics that grow with gain reduction. Not measured.
4. **Opto knee**: 14 dB against about 10 dB from the TT text.
5. **Dyna defaults**: knobs default to 5 ms and 250 ms; the real circuit (Ross schematic, and the ElectroSmash numbers the code cites) is 10 ms and 1.5 s.
6. **Compress/Limit**: the documents give about 3:1 and horizontal; the plugin offers both as menu entries (3:1 and Limit/100:1) but has no switch that moves the Opto style between them; the real switch's parts are not documented in these files.

Agreements: complete-release range (0.43 to 4.3 s against 0.5 to 5 s), the two-stage release with signal-dependent memory, the ratios (3:1 and Limit), the Dyna reservoir, recharge and smoothing values (10 uF, 150k, 1M with 10 nF), and the full-wave transistor detector.
