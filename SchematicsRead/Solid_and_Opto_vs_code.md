# Solid-state amps and the Opto Sustain compressor: what the schematics say against what the plugin does

Nothing in the plugin or in the existing docs was changed. This file reads the four new notes against the code. Line numbers are in `Source/` as the files stand now.

Sources and how much to trust them:
- **Randall RG100ES** (`Randall_RG100ES.md`): a legible factory-style schematic with a few annotated levels. Anything marked computed there is arithmetic on printed values, except where a JFET gm is assumed.
- **Valvestate handbook** (`Valvestate_8100_handbook.md`): prose only. No schematic. Only qualitative statements can be compared.
- **Matamp GT120** (`Matamp_GT120.md`): a redrawn schematic, values legible, no voltages.
- **Pigtronix Philosopher's Tone Micro** (`Pigtronix_PhilosophersTone.md`): a **third-party trace** (AION FX v1.0, 3/28/2025), values legible, the LDR curve is not given, so detector and timing numbers are modelled with stated assumptions.

Rule used: "agree" means the plugin's value or structure falls within the uncertainty of the read; "disagree" means it is outside it. Verdict "unverifiable" is used where the sheet gives nothing.

---

## A. Opto Sustain (`FxExtra::OptoSustainFx`, FxExtra.h 1144 to 1225; hooked up at PluginProcessor.cpp 6249 to 6262)

The plugin's own header (FxExtra.h 1139 to 1143) says every number is an estimate from public descriptions. That is borne out: the schematic contradicts the timing, the ratio and the treble, and agrees with the knee width, the sustain-to-threshold direction and the constant held level.

| Quantity | Schematic read (Pigtronix trace) | Plugin (code location) | Verdict |
|---|---|---|---|
| Topology | **Feedback** compressor: the LDR sits in the feedback of an inverting stage (R7 2M2 / C3 33 pF / LDR), the LED is fed from that stage's own output through a full-wave bridge into a 2200 uF cap | Feed-forward: a 4 ms mean-square detector on the input, a gain computer, a gain applied to the input (1162, 1180 to 1197) | Different. A feedback loop gives a self-limiting, level-dependent attack and a softer effective ratio; the plugin approximates it with parameters (below) |
| Threshold span | Knee at the input level where the stage output reaches about 2.0 V rms (2.9 V peak): **-52 dBV rms at max sustain, -20.4 dBV rms at min sustain** (a 31.6 dB span); a 1 M dark LDR would move both up 10 dB | thr = -14 - 40 x sus dB (1158): a 40 dB span | Roughly agree at the ends (-54 vs -52; -14 vs -20.4 after allowing for the unknown dBFS to dBV mapping), 8 dB wider overall |
| Threshold law (knob taper) | Gain = 2.2 M / (2.7 k + pot) with a linear 100 kB pot, so thr(sus) = -52 + 20 log10(1 + 37 (1 - sus)): sus 0, 0.25, 0.5, 0.75, 0.9, 1 gives -20.4, -22.8, -26.2, -31.8, -38.6, -52.0 dB. Mid-travel is only **18 % of the way** along the span; the last quarter of the knob covers 20 dB | Linear in dB: mid-travel is 50 % of the span (-34 dB) | **Disagree** (back-loaded in the real pedal; the plugin is up to 8 dB more sensitive in the middle) |
| Level held after squeezing | IC1B output held at +4 to +8 dBV rms regardless of the sustain setting; at the blend the wet level is about 12.3 dB lower (small signal, after the clipper divider and IC2B) | makeup = 36 x sus (1159): "the level it holds the note at stays about the same" | Agree (concept) |
| Ratio | Static curves: output rises 4.1 dB for 40 dB more input (model A) or 3.9 dB for 35 dB (model B): **about 10:1, range 8 to 12:1** | slope 1 - 1/40 = 0.975, i.e. 40:1 (1160): output rises 1.0 dB over 40 dB | **Disagree**. Real is 3 to 4 dB more output growth over a 40 dB range. Slope should be about 0.9 |
| Knee width | About 8 dB (slope falls from 1 to about 0.1 between roughly -56 and -48 dBV at max sustain, model A) | 6 dB quadratic knee (1161) | Agree (within 2 dB) |
| Attack | Set by C4 2200 uF charging through R9 + source (about 57 ohm): **63 % in 40 to 41 ms when 10 dB or more over the knee, 118 ms (90 % in 172 ms) at 9 dB of gain reduction**. Level dependent | atkC 2 ms (1163) on the gain-reduction smoothers, plus the 4 ms detector (1162): 63 % in 2 ms, 90 % in 5 ms (computed from the code) | **Disagree**. The plugin is 20 to 60 times faster. The real pedal lets about 40 ms of the pick through before it clamps; the plugin removes the pick |
| Release shape | **Hyperbolic**: I(t) = I0 / (1 + t / tau0), tau0 = C4 x 0.05 V / I0 (0.41 s at I0 = 270 uA; 37 ms at 3 mA). Release is **faster after louder notes**: 50 % of the gain reduction gone in **0.59 s after -10 dBV, 1.9 s after -30 dBV, more than 5 s at 9 dB of reduction** | Two exponentials (1164, 1193): a fast half (90 ms) and a slow half of 0.45 s stretching to 0.95 s with depth (memory 1.2 s, 1165) | **Disagree on both size and direction**. Computed from the code: 50 % at **150 to 165 ms**, 25 % at 0.5 to 0.66 s, 10 % at 1.1 to 1.5 s, for 9, 22 and 40 dB of reduction. Real is 4 to 12 times slower at 50 %, and the plugin slows with depth while the real pedal speeds up |
| LDR own time constants | A few ms rise, tens of ms fall (assumed, no VTL5C6 data) | Not separate | Unverifiable |
| Dark-state high-frequency roll-off | C3 33 pF across the feedback gives a first-order low-pass at **fp = 2.19 kHz x 10^(GR/20)** where GR is the current gain reduction in dB: 2.2 kHz at 0 dB, 4.4 kHz at 6 dB, 6.9 kHz at 10 dB, 12 kHz at 15 dB, 22 kHz at 20 dB. Quiet notes and decay tails are dull; squeezed notes are bright | None | **Missing**. This is the biggest tonal effect in the circuit that the plugin lacks |
| Clipper after the compressor | R3 220k, then 3 x 1N914 each way across R6 100k: small-signal -8.7 dB, onset at about 1.65 to 1.8 V at the node (about 4.6 V peak out of IC1B), soft knee. Idle at the held level (2.2 to 3.6 V peak); it only acts on the **note-onset overshoot** the 40 ms attack allows, and on very hot input | A "Grit" layer: high-passed 150 Hz, gain 6 to 66, hard clip (first-order ADAA), 6 kHz low-pass, mixed in at 0.30 (1207 to 1215); plus a tanh ceiling of 2.0 on the main path (1201) | **Disagree in character**. The real clipper is soft, in series with the signal, not user controlled, and sits 2 to 6 dB above the held level; the plugin's ceiling (2.0 = +6 dBFS against a held level the comment puts at about -16 dB) is 22 dB above it and never engages. The Grit control has no counterpart on this schematic (it may come from other pedals) |
| Treble | A boost-only stage: 0 dB at one end, at the other end **+0.4 dB at 2.5 kHz, +1.3 dB at 5 kHz, +3.6 dB at 10 kHz, +7.3 dB at 20 kHz** (1 + R18 / (R20 + 1 / jwC8)) | A shelf at 2.5 kHz, plus or minus 12 dB (1167, 1172, 1221) | **Disagree** on sign, range and corner. The real treble is a gentle top-octave lift, never a cut |
| Blend | Dry is the IC1A buffer, wet is two inversions later (in phase), linear blend pot 10 kB | `in x (1 - blend) + y x blend` (1223), in phase | Agree |
| Sustain-dependent bass | C2 1 uF into 2.7 k to 102.7 k: high-pass **59 Hz at max sustain, 1.55 Hz at min** (wet path only) | None | Minor, missing |
| Output stage | C6 2.2 uF into volume 10 kA: 7.2 Hz; 150 ohm output resistor | Volume only | Negligible |

### What I would do about it, ranked (no code was changed)

1. **Attack and release shape** (largest audible difference): attack about 40 ms at 10 dB or more over the knee (about 120 ms near the knee), release I(t) = I0 / (1 + t / tau0) with tau0 = 0.05 V x 2200 uF / I0, mapped to gain reduction through an LDR law such as R = 55 k x (I / 1 mA)^-0.9 (assumed, not on the sheet). The code's own release stretches with depth, the real one shortens: the sign of line 1193's `held` term should flip.
2. **Dark-state low-pass**: one first-order low-pass on the wet path with fc = 2190 Hz x 10^(GR/20), capped at 20 kHz, GR taken from the existing `gr` (line 1195). One line, exact from the schematic, and it is what makes the pedal "bloom".
3. **Ratio and threshold law**: slope 0.9 instead of 0.975 (1160); thr = -54 + 40 x log10(1 + 37 (1 - sus)) / log10(38) to keep the plugin's endpoints with the real back-loaded taper (1158). Both are one-liners.
4. **Clipper**: move the soft ceiling (1201) to about 4 dB above the held level and make it a soft symmetric knee (3 diode drops) so it bites on the onset overshoot (only matters once the attack in item 1 is slowed). Keep Grit as an optional extra, not as the pedal's character.
5. **Treble**: if fidelity is wanted, a boost-only top-octave shelf (0 to +7 dB at 20 kHz, about +3.6 dB at 10 kHz); otherwise keep the plus or minus 12 dB as a deliberate widening and say so in the docs.
6. Minor: sustain-dependent 1.5 to 59 Hz wet high-pass.

---

## B. Solid Metal (model 29): JFET voicings against the Randall RG100ES, hybrid voicings against the Valvestate handbook

Context found in the docs: AMP_BUILD_STATUS_b62.md line 91 says "Hybrid voicings follow the Valvestate; JFET voicings follow the Randall", and CIRCUITS.md (lines about 136 to 140) already records a read of the JFET sheet. The captures in CAPTURE_COMPARISON_b62.md (score for Randall 1985: 11.5 against 6.1; Randall 1987: 4.7 against 4.0; Valvestate 5.4 / 5.1 / 6.3 against 5.1 / 4.3 / 3.4) say the model is near the Randall 1987 and Valvestate head sounds and far from the 1985 amp.

### B1. Randall: where the code agrees

| Item | Schematic | Code | Verdict |
|---|---|---|---|
| Red channel clipper after all gain and before tone | Zener pair (labelled "4 V Z") after Q7 and before the master and tone stack: about +/-4.6 V | `hardMask` 4 = only stage index 2 clips hard for JFET Lead (PluginProcessor.cpp 1207); then the tone rows | Agree: hard, symmetric, after the gain stages |
| Gain of the lead channel | 72.5 dB implied by the specification (0.5 mV in, 2.1 V at the master input); 22.3 dB above the clean channel (6.5 mV) | JFET Lead gainMax {20, 18, 10} = 3600 = 71.1 dB (row at 1044) | Agree (within 1.4 dB) |
| Power amplifier sag | +/-40 V rails, 2 x 6000 uF: ripple only 2.0 V peak-to-peak at 80 W into 8 ohm and 3.4 V at 120 W into 4 ohm (computed). Transformer regulation not printed | sag 0.04 to 0.05 (rows 1030 to 1050) and no supply model (PowerSection.h 302 to 309 has no case 29) | Agree: almost no sag. Supply droop cannot be quantified from the sheet |
| Reverb ("Tilt" style) | Send pre-emphasis +20 dB (gain 11 at 154 Hz to 111 at 1.54 kHz), return tilt +40 dB (16 Hz to 1.59 kHz), return coupling C29 0.22 uF into the 2k reverb pot = **362 Hz** high-pass | SpringReturn style 6: high-pass 362 Hz, send shelf +12 dB at 1500 Hz, return shelves -8 dB (FxExtra.h 1099); MODELED_GEAR.md 487 calls it a "tamed version" | Agree. The 362 Hz corner is an exact match (1 / (2 pi x 2 k x 0.22 uF)). The steepness (60 dB real against about 20 dB) is a stated choice |

### B2. Randall: where the code disagrees or is silent

| Item | Schematic | Code | Verdict |
|---|---|---|---|
| First-stage bass behaviour | Red channel: Q5 source 2.2k **bypassed by 0.47 uF**: a low shelf, zero at 154 Hz, pole at about 520 Hz, **step 20 log (1 + gm Rs) = 10.7 dB (gm 1.1 mS assumed; 13.4 dB at 1.7 mS)**. The *clean* channel's Q1 is the unbypassed one (flat gain, not bass-shy) | Comment at 1042 says the first stage is "unbypassed ... so it is bass-shy"; hp[0] = 170 Hz (row at 1044) | **Disagree**. The comment mislabels the channels (CIRCUITS.md line about 140 has it right). A 170 Hz high-pass cuts bass harder than the circuit, which never cuts more than about 11 to 13 dB. A low shelf of about -11 dB below 154 Hz would be closer |
| Diode pair at the master | D5/D6 (1N914) in anti-parallel, **switched in only by pulling the Treble knob** ("Sustain Boost": volume drops, "infinite sustain"). With it in, the pre-master node is clamped at about +/-0.6 V, 17.7 dB below the zeners, a very soft knee | Comment at 1042 to 1043 lists "the master-level diode pair" as part of JFET Lead; `hardMask` carries no second clip level | **Disagree** if the diode pair is meant as always on: it is a switched mode, not part of the standard lead sound |
| Power amp clipping | Hard, symmetric, rail-limited at about +/-37 V. At full master a hard-clipped red channel (+/-4.6 V, minus about 14 dB of stack, times a power-amp gain of 80 to 100) asks for about 70 to 90 V against 36 V available: **the power amp is itself 6 to 8 dB into hard clipping**; it stops at about -8 dB of master (computed) | Generic soft power saturation: `powerSample` = tanh(d s) / d x sqrt(d) with d = 1 + 5 drive^2 (PluginProcessor.cpp 2140 to 2149), applied at 7932 to 7968 for every model including 29 (no PowerSection family: PowerSection.h line 26 gives fam[29] = -1, so none of the derived modes 1 to 3 touch it, PluginProcessor.cpp 1712 to 1735); `pwrAdd` is 0 for the model-29 rows | **Disagree** (hardness and the always-overdriven condition at high master) |
| Output impedance | **Mixed voltage and current-sense feedback: Zout about 5.0 ohm, damping factor 1.5 at 8 ohm** (computed; the manual says 2 to 4). Gain 99 into 8 ohm, 123 into 16 ohm, 72 into 4 ohm (computed). Bass rolls off below about 40 Hz (-2.4 dB at 50 Hz, -6.3 dB at 20 Hz) | Row comments call the amp "stiff" (1026 for the hybrid, 1043 for JFET Lead); `ampReso` is a generic 105 Hz, Q 1.1, +6 dB x knob peak (5442), not model-specific | **Disagree** in direction. With a 5 ohm source and a speaker whose impedance rises from 8 to about 35 ohm at resonance (assumed), the 100 Hz level rises by 20 log((35 / 40) / (8 / 13)) = +3.1 dB: about `ampReso` 0.5. A stiff amp would give +0.1 dB |
| Presence | A **cut that the knob removes**: 2k rheostat in a shunt with 0.1 uF from the point after the stack, **before the effects loop and the power amp**. Knob at 0: first-order low-pass at about 1.6 kHz (-10.5 dB at 5 kHz, -16 dB at 10 kHz); knob at max: only about -3.5 dB at the top. Span 7 dB at 5 kHz, 12.5 dB at 10 kHz (computed) | Presence is a post-power shelf (osPresence, PluginProcessor.cpp 7979, after the transformer at 7972): +7 dB at 5 kHz at the top of the knob (rows 1044 to 1051) | Span agrees at 5 kHz (7 dB). **Position disagrees**: the real network shapes the signal before the power amp clips, so a dull setting also lessens the fizz of the clipped power stage |
| Tone stack | Passive: 50k treble (wiper output), 50KA bass rheostat, 2k mid, 0.002 / 0.22 / 0.2 uF, 4.7k, driven from a 10k master. Swings (computed): bass 16 dB at 50 Hz, mid 9.1 dB at 600 Hz, treble 14.3 dB at 10 kHz (manual 15 / 10 / 16). At 5 / 5 / 5 the response sits 3.4 dB above the 1 kHz level at 100 to 200 Hz and 3.7 to 4.6 dB above it from 3 kHz up, with the dip centred near 800 Hz (-19.2 dB against -15.2 at 100 Hz and -14.2 at 6.4 kHz) | None of the 13 Tone Stack menu entries is this network (PluginProcessor.cpp 3129 to 3135). The baked rows give JFET Lead: bass +3.5 dB at 100 Hz, low-mid -4 dB at 450 Hz, Q 0.7, high-mid +1 at 2200 Hz, treble +2 at 3800 Hz | Shape agrees in kind (a scoop about 4 to 5 dB deep with lows and highs lifted). **Scoop centre disagrees**: 450 Hz in the row against about 750 to 800 Hz on the sheet |
| Clean channel | **No clipper at all**; linear until the power amp runs out of rail, about 5.1 dB above the annotated 20 V (computed: 36 V against 20 V). Gain 50.9 dB (6 mV to 2.1 V) | "JFET Crunch (the clean channel pushed: soft JFET clipping only)" (1351 and 1377: hardMask 0) | Partly: the soft clip is plausible only with the master well down. At high master the hard power-amp clip arrives first. Also AMP_BUILD_STATUS_b62.md line 78 notes the clean captures stay linear to -6 dBFS, below the plugin's lowest Gain: consistent with this read |
| Clip level | 4 V zener label: about +/-4.6 V | CIRCUITS.md says 4.7 V zeners, about +/-5 V; comment at 1042 says 4.7 V | Within 0.7 dB, ignore |

### B3. Valvestate handbook against the Hybrid voicings (qualitative only)

| Statement in the handbook | Code | Verdict |
|---|---|---|
| Boost channel: OD1 "creamy, classic" and OD2 "screaming, Hi Gain", with the Gain "fattening" OD1 and going "from rock overdrive to full metallic thrash" on OD2; one ECC83 in the drive path | Voicings "Hybrid Classic OD" and "Hybrid Hi-Gain" (PluginProcessor.cpp 1025 to 1041): op-amp stages, LED clipping, one 12AX7 | Agree in structure |
| Power stage built "to simulate a valve power stage": Bi-Polar (8100 and others) or MOSFET "constant current" with a **high output impedance** and "speaker reaction similar to that found in valve equipment. Ringing harmonics, rich bass response and sensitivity" | Comment at 1026: "a stiff transistor power amp ... almost no sag"; no speaker-interaction model for model 29 (fam -1) beyond the generic `ampReso` | **Disagree in intent**: the handbook says the output stage is deliberately not stiff, and no number is given. The Randall read (Zout 5 ohm) is a different amp, so it cannot supply the figure. No quantity can be set from this document |
| **Contour**: "special mid band shift contour control", a sweeping mid-band voicing, ends labelled LOW and HIGH on the panel; "devastating" with the EQ | A fixed "contour (mid-scoop) network" baked into the row (1026); there is no Contour parameter (no Contour string in PluginProcessor.cpp or PluginEditor.cpp) | **Partial disagreement**: the real control sweeps; the plugin fixes it. Range and centre frequencies are not given |
| Normal channel: Clean ("sounds remain clean, even at high gain levels") and Crunch (gain adds distortion) | No normal-channel Valvestate voicing: the four voicings are two hybrid boost, JFET Lead, JFET Crunch (Randall) | Missing; AMP_BUILD_STATUS_b62.md line 78 also notes the clean channels sit below the plugin's lowest Gain |
| Effects loop after the preamp distortion, 0 dB / -20 dB level switch; Line Out "specially filtered" (speaker emulation) | Not modelled (no loop in the amp model) | Not relevant to the amp tone; the line-out filter shape is not given |

### B4. Ranked list for Solid Metal (what is worth changing)

1. **Power stage for model 29**: give it a PowerSection family of its own (a new fifth row): hard symmetric rail clip at about +/-37 V against a stage gain of 80 to 100 (so the stage is already clipping 6 to 8 dB at full master), output impedance about 5 ohm (damping factor 1.5), plus `ampReso` of about 0.5 as the default for the JFET voicings. The code today pushes it through the generic soft tanh at a zero offset and makes it "stiff".
2. **Sustain Boost**: a switched soft diode clamp at about +/-0.6 V (17.7 dB below the zener clip) with the volume drop the manual describes, as a fifth voicing or a switch. Move the diode pair out of the default JFET Lead description.
3. **First stage**: replace the 170 Hz high-pass on the JFET Lead first stage (row at 1044) by a low shelf of about -11 dB below 154 Hz (13 dB if gm is 1.7 mS) and correct the comment at 1042.
4. **Presence before the power stage** for the JFET voicings: first-order low-pass at about 1.6 kHz at 0 rising to a -3.5 dB shelf at max (shape given above), so the clipped power stage works on a pre-shaped spectrum.
5. **Tone stack**: add the Randall passive stack to the menu (the nodal solution in my scratch scripts reproduces the manual's swings within 1 to 2 dB and the annotated -14.4 dB insertion loss to 0.5 dB), and move the scoop of the baked JFET rows from 450 to about 750 Hz.
6. **Hybrid**: expose Contour as a sweep and give the hybrid voicings a non-stiff output (a moderate `ampReso`), but only as a design decision: the handbook has no numbers.
7. **Clean range**: lower the lowest Gain of the clean voicings (already identified in AMP_BUILD_STATUS_b62.md); the Randall clean channel is linear up to the power amp's rails.

---

## C. Matt's Amp (model 24) against the Matamp GT120 sheet (Orange-style)

Model 24 is explicitly an average of three Orange sheets (MODELED_GEAR.md line 201), so it should not match the GT120 alone. The comparison below picks out what the GT120 read adds or contradicts.

| Item | GT120 read | Code | Verdict |
|---|---|---|---|
| Six-position plate cap | C21 0.1 uF, 4700, 2700, 1500, 820, 390 pF into a 1M log pot: high-pass corners 1.6, 34, 59, 106, 194, 408 Hz at full pot, 16, 339, 590, 1061, 1941, 4081 Hz at a 100 k effective load | "Heavy: bass-cut input cap about 70 Hz corner at the 10 nF position" (PluginProcessor.cpp 904); hp {72, 100, 85} (row at 905) | Agree in kind. The 59 Hz (2700 pF) and 106 Hz (1500 pF) positions bracket the 72 Hz row. The selector sits **before the second triode**, not the first (V1a runs flat to about 2 Hz) while the row puts 72 Hz in front of stage one |
| Stage count | V1a and V1b (both 2.4k / 50 uF, 220k loads, gain about 78 each, assumed), then a **third voltage stage V2a** (390k, unbypassed 1.5k cathode with the feedback, about 68 open loop), direct-coupled to the phase inverter | Graphic: 2 stages, Heavy and Bright: 3 | Agree for the last two. CIRCUITS.md ("V3 inverter ECC83 with 100k loads") omits V2a as a gain stage |
| Treble pole inside the loop | C11 680 pF from the V2a anode to the supply: first-order low-pass about 4.3 to 4.5 kHz (assumed rp 62.5k), -7.7 dB at 10 kHz, **inside the feedback loop** | Graphic comment: "the fixed top cap taking the fizz off" (row comment at 896); cabLowpass 5000 Hz | Agree in effect. Medium confidence in how C11 is connected |
| Tone stack | Two 1M log pots, 330 pF / 2200 pF, 100k, 27k, 0.01 uF: bass pot swing 24.9 dB at 60 Hz, treble pot 22 dB at 4 kHz and 29 dB at 8 kHz, **no mid control**; at 5 / 5 the response is 3.5 to 4.5 dB higher at 120 to 250 Hz than at 4 kHz | Graphic offsets: bass +2 dB at 130 Hz, low-mid +1.5 at 300 Hz, treble -3 dB at 3.2 kHz: a 5 dB low-to-high tilt | Agree. MODELED_GEAR.md line 201 says all three were solved |
| Presence and feedback | 24k || 330 pF from the 16 ohm tap to the V2a cathode; at low frequency beta = 4.6 % (5.9 % if the 5k pot is ignored). Presence is a 0.1 uF shunt on the cathode through a **5k LIN** pot: with the wiper at the cathode end beta falls 16 dB at 10 kHz and 7 dB at 3 kHz (computed). With the plugin's own loop gain for family 2 (3.65) that is +5.0 dB at 3 kHz and +9.4 dB at 10 kHz | presence 4200 to 5500 Hz, +4.5 to +8 dB (rows 897 to 920); derived mode uses the "British lead" family (PowerSection.h 26, fam[24] = 2) | Roughly agree. CIRCUITS.md calls the presence pot 1k; I read **5k LIN**. A "15k feedback resistor" in that note is the bias-supply resistor R23, not feedback |
| Supply | Bridge of eight 1N4007, 2 x 100 uF / 500 V (50 uF), 1 A fuse, choke, 2 x 33 uF / 500 V (16.5 uF). Preamp chain **R20 10k, R6 10k, R3 33k** with 16, 16, 10 uF. Screens through 1k / 5 W from the post-choke node, plates from the pre-choke node. No voltages printed | PowerSection.h 302 gives "33k and 33k dropping stages with 16 uF" | Small mismatch (this sheet is like the GRO 100 pattern). Effect on sag negligible (the dropping chain feeds the preamp only) |
| Component voicing | All first-stage values are printed (2.4k / 50 uF, 0.047 uF couplings) | `applyComponentVoicing` (PluginProcessor.cpp 1566 onward) has no branch for model 24 (or 29) | Not done: could be added from the sheet |
| Fixed bias | Negative supply (D9 1N4007, 27k, 15k, 10 uF / 150 V twice, 100k trim between 40 % and 100 % of the negative node), voltage not printed | `ampBias` generic | Unverifiable |

---

## D. Overall ranked list across all four sources

1. Opto: attack about 40 ms and the hyperbolic, level-dependent release (the timing is wrong by 4 to 60 times).
2. Opto: the level-dependent top-end roll-off fc = 2190 Hz x 10^(GR/20).
3. Solid Metal power stage: hard rail clip, Zout 5 ohm, `ampReso` about 0.5, already clipping at high master.
4. Opto: ratio 10:1 instead of 40:1, and the back-loaded sustain law.
5. Solid Metal: the switched Sustain Boost clamp, and the first-stage shelf instead of a 170 Hz high-pass.
6. Opto: soft series clipper relative to the held level, and a boost-only treble.
7. Presence before the power amp, and the Randall tone stack and scoop frequency.
8. Matt's Amp: component-voicing branch from the GT120 values, the selector before the second stage, the supply resistors.
9. Valvestate: Contour as a sweep, non-stiff output (needs numbers the handbook does not give).

## E. What could not be compared

- Valvestate: nothing numeric exists in the handbook.
- Randall: the clean-channel clip level (JFET headroom depends on a rail value that two sheet annotations disagree about: 24 V printed, about 28 V implied), transformer regulation, and the negative-feedback C34's role.
- Pigtronix: the VTL5C6 curve and timing (assumed models), the real LED and diode curves (assumed), and whether production values match the trace.
- GT120: all DC voltages, output transformer ratio, the L1 value, the exact switching of the three-wafer selector and the effects-loop jacks.
