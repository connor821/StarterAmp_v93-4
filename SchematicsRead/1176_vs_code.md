# 1176 schematic / manual vs the plugin's FET compressor style

Analysis only; no code or existing documents were changed.

What was compared
- `MODELED_GEAR.md`: line 362 (b59 "Compressor audit"), line 705 (b36 "FET and Opto compressors"), plus the b59 follow-up table in `DATASHEET_AUDIT_b58.md` (line 238) and `CHANGELOG.md` b59.
- `CIRCUITS.md`: contains nothing about the 1176 or the FET compressor (the only compressor row is the MXR Dyna Comp, line 17), so there is nothing there to disagree with; the 1176 facts live in `MODELED_GEAR.md`, `DATASHEET_AUDIT_b58.md` and `reference/schematics/1176/MASONAUDIO_1176_build_guide_notes.md`.
- Code: `Source/PluginProcessor.h` compressor `process()` (around lines 2695 to 2960; style index 1 = FET) and the parameter layout in `PluginProcessor.cpp` (lines 2986 to 3000: Threshold -40 to 0 dB default -18; Ratio menu 1:1, 2:1, 3:1, 4:1, 6:1, 10:1, 20:1, Limit; Attack 0.05 to 50 ms default 5; Release 50 to 3500 ms default 250).
- Schematic numbers come from `1176_schematic_notes.md` (Rev K drawing plus the manual and factory test pages).

What the FET style assumes (from the code)
- Ratio menu mapped through `fetRatios` = 1, 2.4, 3.7, 5.1, 6.6, 9.9, 20.5, 100 (the menu's 4 = 5.1, 20 = 20.5; the "4 / 8 / 12 / 20" measurement of 5.1 / 8.2 / 11.5 / 20.5 is from the Mason Audio DIY Rev F build, not UREI).
- Knee 2 dB (0 for "Limit"); detector = fast peak follower on the squared signal, 50 us up / 2 ms down, with 3 dB removed so a sine reads the same as the old RMS window; single threshold control independent of ratio.
- Attack smoothing in the dB domain: tau = max(5 us, Attack knob x 0.25), so the knob's 0.05 to 50 ms spans 12.5 us to 12.5 ms (default 5 ms gives 1.25 ms).
- Release smoothing in the dB domain: tau = Release knob (50 to 3500 ms) x f with f = 0.5 + 0.9 x memory (memory = slow average of the gain reduction, time constant 1.2 s, saturating at 8 dB), so f runs 0.5 to 1.4 ("program dependent: fast after a short peak, about 3 times slower after a long squeeze").
- Saturation option "FET stage" = odd-harmonic tilted soft clip (k3 x 1.4); UI text calls it "Class-A solid-state output with a little extra odd-harmonic edge".

## 1. Agreements

| Item | Plugin | Schematic / manual | Verdict |
|---|---|---|---|
| Top ratio | 20.5:1 at the 20 button / menu 20 | 20:1 (test: +1 dB out for +20 dB in, +/-20 %) | agrees |
| 12:1 and 8:1 buttons | interpolated 9.9 / 6.6 around them (menu 10 -> 9.9, menu 6 -> 6.6) | 12:1 -> 1.66 dB out per 20 dB in (ratio 12), 8:1 -> 2.5 dB (ratio 8), each +/-20 % | within tolerance (menu values 10 and 6 are not unit buttons) |
| Attack floor | 5 us time constant | spec "<20 us for 100 % recovery"; manual 1.1: "a 50 kHz peak is fully stabilized within 1 cycle" (20 us). If 100 % recovery is about 4 time constants, tau is about 5 us | consistent |
| Release definition | exponential time constant, i.e. the 63 % point | spec "50 ms minimum, 1.1 s maximum (for 63 % recovery)"; derived tau from R and C: 59 ms (270k x 0.22 uF) to 1.16 s (5.27M x 0.22 uF) | same definition, same range |
| Peak, full-wave detection | fast peak follower | CR3/CR4 full-wave rectify the two phases of the control amplifier output; a peak (not RMS) detector | agrees |
| Detector band | detector high-pass off by default | the sidechain is flat: factory test shows the threshold unchanged at 20 Hz and 20 kHz; all couplings (C17 1 uF, C19/C20 6.8 uF) put the corner far below 20 Hz | agrees |
| Distortion rises with gain reduction | saturation depth = 0.2 + GR/15 | THD "increases with faster release at low frequencies"; with limiting it grows (spec 0.5 % in limiting vs 0.3 % without) | direction agrees |
| Odd-harmonic edge (Rev K) | odd-harmonic lean | Rev K output stage is a complementary push-pull Darlington pair (BD517/BD518) driving T2, symmetric, so odd harmonics | agrees for Rev F/G/K type units |
| Reference for the meter and gain reduction | n/a | GR meter calibrated so 10 dB of limiting reads -10 | no conflict |

## 2. Disagreements (numbers)

| Topic | Plugin | Schematic / manual | Size of the difference |
|---|---|---|---|
| Ratios 4:1 button | 5.1 (menu 4) | factory spec 4:1; test window 20 dB in -> 5 dB out +/-20 % = ratio 3.3 to 5.0. The 5.1 figure is a DIY build's measurement; it is just outside the factory window | plugin about 25 % steeper than nominal at 4:1 |
| Ratio menu entries 1:1, 2:1, 3:1 (and 2.4, 3.7 mapped values) | exist | the unit has no ratio below 4:1; 1:1 would be "no button" (limiter off via the attack-OFF position). The mapped 2.4 and 3.7 are extrapolation | no hardware basis below 4 |
| "Limit" | 100:1, knee 0 | all four buttons in is not a 100:1 limiter. From the divider: sidechain level -6.8 dB vs 4:1 -15.6 dB and 20:1 -1.9 dB, plus a low-threshold diode bias (-2.2 V) - an effective ratio around 11:1 by sidechain gain (derived, UA4706 block unread), with the lowest threshold of all. Folklore says it behaves more extremely (fast, distorted); this drawing does not state a ratio for it | likely much lower than 100:1 on paper (?) |
| Threshold vs ratio | one threshold, same for every ratio | threshold moves with the ratio: -30 dB (4:1), -26 (8:1), -25 (12:1), -24 (20:1) input for 1 dB of limiting, relative output +7, +8, +9, +10 dBm; derived from the diode bias ladder (4:1 about -1.95 V up to 20:1 about -6.3 V) | 6 dB spread across ratios not modelled; the audit already lists "SSL's threshold that moves with ratio" but not the 1176's |
| Fixed gain across ratios | none | with the same input and output settings the output level drops 1 dB (12:1), 1.5 dB (8:1), 2.5 dB (4:1) relative to the 20:1 setting (test p-37); hand notes: same input gives -2 dB GR at 12:1, -2.5 dB at 8:1, -4 dB at 4:1 when 20:1 gives -1 dB | not modelled |
| Knee | 2 dB for every ratio | thresholds are defined at 1 dB of limiting for 20, 12, 8:1 but 3 dB for 4:1 "because of the soft knee in the threshold circuit"; Fig 3-1 shows a markedly rounder 4:1 knee | the plugin's knee is probably right for the higher ratios and too hard for 4:1 (derived) |
| Attack knob range | 12.5 us to 12.5 ms (times 0.25), default 1.25 ms | 20 us to 800 us | top of the knob is 15 times slower than the real maximum; default is 1.6 times slower than real slowest |
| Release range | 50 to 3500 ms knob x f (0.5 to 1.4) = 25 ms to 4.9 s | 50 ms to 1.1 s (63 %) | top of the knob about 3 to 4 times beyond the real maximum; the minimum of 25 ms (50 ms x 0.5) is half the real minimum; default 250 ms x f = 125 to 350 ms is inside the real range |
| Release program dependence | f = 0.5 + 0.9 x (memory) slows release up to 2.8 x after a long squeeze; comment cites the manual | no program-dependent circuit: the release is a plain RC (release pot R78 5M + R79 270k, reservoir C27 0.22 uF) in the voltage domain. The manual pages read (pp. 11 to 19, 3.4.5) say nothing about fast release after transients and slow after sustained signals; they only discuss pumping/breathing | the comment's attribution to "the manual" is not supported by this manual; some program dependence exists physically, see section 3 |
| UI text vs code | editor text: "a release that snaps back after deep squeezes" | code does the opposite (f rises with memory: slower after a long squeeze); the physical unit also releases more slowly after deep squeezes, see section 3 | the tooltip contradicts the implementation |
| Output stage description | "Class-A solid-state output" (editor) | Rev K: push-pull complementary Darlington (class AB, DIY notes) driving a Lundahl LL5402; Rev A to D: transformer-coupled emitter-follower / class-A style with the UREI UA-5002 transformer. Both exist; the Class-A wording matches the older boards | wording is revision dependent |
| Amount of distortion | tanh saturation with k = 1 + 3 x depth x 1.4 on a signal pre-scaled by 3 | unit THD is tiny: spec under 0.5 % in limiting, about 0.03 to 0.25 % after trimming R16 (hand notes), 0.3 % maximum without the FET; the unit is clean | plugin saturation is far more colourful than the hardware; fine as a voicing but not a model |
| Threshold control | Threshold -40 to 0 dB | no threshold control: Input knob (before the FET, up to +45 dB) sets the level at the detector, Output knob after the sidechain tap. Sidechain is taken after Q1 and before the Output pot (feedback topology) | equivalent static behaviour with Input = threshold; Output does not change detection (matches the plugin's separate make-up) |

## 3. Behavioural points the plugin does not capture (from the circuit, derived)

1. **Feedback topology.** The control amp reads the preamp output, after the gain-reduction FET (Q1 is at the input, the preamp Q2-Q4 follows, R26 56k taps the preamp output). The plugin is a feed-forward static curve with smoothing. Consequences in the real unit: finite loop gain sets the ratio (4:1 to 20:1 are loop gains), overshoot on fast attacks (factory test measures "leading edge overshoot", p-36), and an input knob / ratio interaction.
2. **dB-domain release shape.** With the bus decaying as a single RC in voltage and the FET shunting through R6 27k, gain reduction in dB is roughly 20 x log10(1 + k x V). For deep reduction this decays as a constant dB per second ramp (about 8.7 dB per release time constant) and only rolls into an exponential in the last few dB. So deeper squeezes take proportionally longer to recover (about 10 dB of GR at 1.1 s tau is a ramp of around 1.3 s before the tail), and the first dB of recovery is slow in time but constant in rate. The plugin's first-order dB smoothing (time constant fixed) recovers fast at first and slows near the end - the opposite shape. A rate-limited release (as the RMS style already uses) would be closer; this is a derivation from the circuit, the BF245A's R_ds(Vgs) law is not in the documents (?).
3. **Attack/release tied to the same bus capacitor.** C27 0.22 uF is the only candidate reservoir; the attack resistance (470 ohm to 25.5k) with this C gives 103 us to 5.6 ms, not 20 to 800 us, so the attack network either has a smaller effective capacitance or the specification quotes a shorter measure; the plugin's 5 us to 12.5 ms range straddles both. (The schematic C22 0.022 uF across R76 470 is the fast-edge bypass.)
4. **Ratio changes the sidechain gain and the threshold together** (B section divider times A section bias). The plugin changes only the slope.
5. **Soft knee depends on ratio** (4:1 much softer).
6. **Meter reads GR with tracking within +/-1 dB** and "0 = 0 dB GR", while the plugin's displayed GR is `grDb x shownScale`; no conflict.
7. **Stereo link (1176SA)**: not modelled.
8. **Level scale.** Real unit: max input +20 dBu, gain 45 dB (47 dB measured), clip +24 dBm into 600 ohm; thresholds are -30 to -24 dBu at full Input. In a plugin the relevant mapping is dBFS; no tie exists, so threshold values are free.

## 4. Suggested alignments if the style is revisited (not applied)

- Ratio table for the FET style: use buttons 4 / 8 / 12 / 20 as 4, 8, 12, 20 (factory) or keep the 5.1 / 8.2 / 11.5 / 20.5 measurement but state that it is the DIY unit's, and note that the factory test window is +/-20 % so both are "in spec".
- Attack knob: map the full 0.05-50 ms knob range onto the real 20-800 us (as time to full effect) so the top of the knob is no longer 12.5 ms; or add a note that the style extends beyond the hardware.
- Release: cap the real-unit range at 50 ms to 1.1 s (tau), allow the extra range only as an extension; give f a basis in the dB-ramp behaviour (item 2) rather than a "memory" factor, and fix the tooltip ("snaps back").
- Add the ratio-dependent threshold offset (relative to 20:1 at -24 dB: 12:1 -1 dB, 8:1 -2 dB, 4:1 -6 dB) and the ratio-dependent output offset (-1, -1.5, -2.5 dB at equal input) if exact behaviour is wanted, and a wider knee (about 3 to 6 dB) for the 4:1 setting.
- Treat "Limit" as the all-buttons mode (sidechain level between 8:1 and 12:1 but with a low threshold) rather than as 100:1, or keep it as a plugin-only extension and say so.
- Add a CIRCUITS.md row for the UREI 1176LN (Rev K schematic and manual) listing what is applied and what is not.

## 5. Source locations for the figures above

- Ratio, threshold, attack, release, THD, S/N, gain: manual p-05 (printed page 2) and factory test pp. 35 to 37 (`urei/p-35.png`, `p-36.png`, `p-37.png`).
- Ratio divider and bias ladder: `misc/1176sch.png` upper middle (S1B..S4B, R2, R24, R25, R26, R23) and right of centre (S1A..S4A, R42-R44, R45, R46, R87); Rev D labels "4:1, 8:1, 12:1, 20:1" in `misc/schematD.png`.
- Attack/release/Q-bias: lower right of `misc/1176sch.png` (R76, R77, C22, R78, R79, R81-R83, C21, CR3, CR4).
- Power transformer, supply: `urei/p-58.png`, right side of the Rev K drawing.
