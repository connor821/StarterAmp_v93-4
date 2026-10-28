# EHX compilation, pages 1-47: schematic read-out

Source: 'All Electro-Harmonix Effects.pdf', pages 1-47 (page images p-001..p-047 plus the pdftotext layer). Values come from the text layer cross-checked against the images. (?) marks anything unreadable or uncertain.

IMPORTANT FINDING: pages 1-47 are mostly NOT Electro-Harmonix Big Muff material. Only three circuits here are genuine EHX products (Small Stone p1, Soul Preacher p2, Bass Micro Synthesizer p26-31). The rest is a miscellany: Univox/Vox/Uni-Vibe era effects, hobby circuits, PAiA kits, a Theremin article, PCB layouts, and Marshall/Fender amp schematics. There is no Big Muff, Electric Mistress, Memory Man, Holy Grail, Screaming Bird, Hot Tubes, Soul Food, Mole, Nano, Hog's Foot, Bass Balls or Ram's Head in this range.

## Page index

| Page | Item | Notes |
|---|---|---|
| 1 | EH Small Stone Phaser, Issue J | EHX, OTA phaser (detailed below) |
| 2 | EH Soul Preacher (rev. 10/13/95 drawing) | EHX, compressor/sustainer (detailed) |
| 3 | "EM Stereo Spreader" | Electronic Musician project, not EHX; 5532 op-amp stereo widener |
| 4-9 | Theremin (Popular Electronics, Nov 1967, L. E. Garner Jr.) | Article text, PCB layout, parts list; RF beat-oscillator theremin. Not guitar-pedal relevant |
| 10 | Tone Booster (Everyday Electronics, Sept 1978) | 2-transistor treble booster (detailed) |
| 11 | Tone control circuits (bass/treble preamp; 709 op-amp version) | Synth-module era tone stacks |
| 12 | Controllable tremolo circuit | Phase-shift LFO + JFET (detailed) |
| 13 | Tube Distortion (Ron Black, Guitar Player Oct 1981) | 747 op-amp + 12AU7A at 12 V heater |
| 14 | TubeHead (PAiA 9305) | Op-amp + 12AX7 preamp, stereo, 4049 CMOS inverter charge-pump style supply |
| 15-16 | Tube Mic Pre (PAiA 9407) and mods | Mic preamp with 12AX7 + blend |
| 17 | UniVibe (Unicord model 905, c.1968) | Detailed |
| 18-22 | PCB layouts: "VIBE-A version 1.21" (18, 19 overlay/copper, 20 copper), "VIBE-B version 2.0" (21 overlay, 22 copper) | Univibe-clone boards; no schematic values worth recording |
| 23 | Univox Super-Fuzz | Detailed |
| 24 | Vox Tone Bender | Detailed |
| 25 | Vox Treble Booster (+ overdrive mod) | Detailed |
| 26-28 | Electro-Harmonix Bass Micro Synthesizer, 3 sheets (D. Cockerell design, rev 12/12/78, redrawn 28 Sep 1996) | Detailed |
| 29-31 | Same three sheets, second variant labelled "GUITAR" instead of "BASS" | Differences listed below |
| 32 | "Bob's Octavia" (Bob Starr) | LM386 + transformer octave-up |
| 33 | UniVibe lamp flasher circuit (Bob Starr) | Exar 2206 driving the lamp |
| 34-35 | Marshall 1987 (50 W) output stage/PSU and preamp | Amp, not an effect |
| 36 | Marshall 2204 preamp | Amp |
| 37-38 | Marshall 2205 output stage/PSU and preamp (with FX loop/reverb section) | Amp |
| 39 | Marshall 2210 output stage and PSU | Amp |
| 40-41 | Marshall 2550 (Silver Jubilee) and PSU | Amp |
| 42 | BI-PHASER (hobby, 8-stage JFET/4558 phaser) | Detailed |
| 43 | Fender Blues Deluxe (dwg 047019 rev F, 1994) | Amp |
| 44 | Fender 2x6L6GC amp, 7025 preamp, normal/bright inputs, handwritten "4x10" (title block not read) | Amp |
| 45 | Fender Champ 12 | Amp |
| 46-47 | Fender "Super 60" series tube amps (68/60 series, dwg 028921 rev E): preamp/reverb, then power/PSU | Amp |

---

## p.1 Electro-Harmonix Small Stone Phaser, Issue J

Circuit type: 4-stage all-pass phaser built from OTA stages, plus a 5th OTA as LFO. Supply 9 V (battery), V+ with a Vbias half-rail divider (15k / 10k with 33 uF filter).
Semiconductors: input buffer/gain stage 2N5088 (NPN) direct-coupled with 2N5087 (PNP); 'color' (feedback) switch stage with another 2N5087; five OTA ICs all house-marked EH1048 (equivalent to CA3094: CA3080-type OTA + Darlington follower).
Diodes: none shown.

Input stage
- Input node: 0.0068 uF coupling cap to the 2N5088 base; 100k (to ground? ) at the input; 470k bias to Vbias.
- 2N5088 emitter 4.7k to ground; 2N5088 collector feeds 2N5087 base; 2N5087 collector loaded 10k; 15k and 10k bias network from V+ (values read from page text: 15k, 10k, 10k, 470k).
- Output coupling 10 uF into the phase-shift chain via 30k.

Phase-shift section (each stage identical, 4 times)
- OTA with 0.0068 uF timing cap, 27k (30k on the first stage) series input resistor, 1k from IC pin 3 (amp bias current set), 27k to Vbias, 10k to ground at the follower output (pin 6).
- Stages are linked by 27k resistors; the LFO current modulates the OTA bias current (pin 5), which sweeps the all-pass frequency.
- Between stages: 4.7k and 3.3k and 27k mixing resistors to the 'EFFECT' output node (0.1 uF + 470k load at output).
- Dry/wet mix: 270k + 0.1 uF taps (dry path), 27k and 3.3k with 4.7k to ground (wet path combination).

LFO and 'Color' switch
- 5th OTA with 33 uF, 1M (speed pot), 27k and 7.5k from V+ for rate range, 100 ohm output resistor feeding the four 'LFO' nodes.
- COLOR switch (DPDT) swaps an alternative feedback/mix path: 4.7k, 22k, 15k, 27k, 1k, 1.8k, 0.1 uF and a second 2N5087 emitter-follower in that block (exact wiring (?)).
- Switch: bypass (true bypass) / effect.

Key values to remember: all-pass caps 0.0068 uF x4; stage resistors 27k; LFO 33 uF with 1M; supply 9 V.

## p.2 Electro-Harmonix Soul Preacher (drawing rev. 10/13/95)

Circuit type: sustainer/compressor with FET gain-reduction. Supply 9 V (battery or AC adaptor), +4.5 V virtual ground, AC adaptor input is regulated by a pass transistor.
Semiconductors: Q1 input buffer NPN (type unknown, drawn as emitter follower), Q2 (PNP, drives rectifier node), J1 FET (JFET shunt gain element, type unknown), Q3 pass transistor from adaptor, Q4 half-rail follower, IC1 = 4558 dual op-amp (IC1a = main gain stage, IC1b = full-wave rectifier / control driver). Diodes 1N3666 x2 (germanium-type? 1N3666 is a Ge signal diode) in the rectifier path.

Values
- Input buffer: 680k to ground, 270k from +9V to base, 270k base to ground (bias), 4k7 emitter, 0.1 uF in; 1 uF 25V output; 100k to ground.
- 220k + 0.1 uF in series feed the J1 drain/op-amp input node; 0.1 uF into IC1a pin 2 path; 270k bias to +4.5 V on pin 3.
- IC1a feedback: 330 pF across, 1k + 10k + 100k linear pot (compression gain) in feedback; 10 uF 25V tantalum from feedback tap to ground.
- Output: 10 uF 25V tantalum, 10k series, S1 switchable 0.15 uF (treble shaping), 10k log volume pot.
- Control loop: 4558 B stage with 10k/10k/10k/10k resistors, 10 uF 16V; 47k + 1N3666 pair rectifier; 20k, 4M7, 1M, 1 uF 35V tantalum timing; Q2 provides gate-bias for J1.
- Supply: 220 ohm + 100 uF 25V, 15k, 0.1 uF, 3k9, 4k7 x2, 1 uF 25V, 10 uF 16V.
- All resistors carbon film 1/4W 5%; non-polarised caps mylar 50V 10%.

## p.10 Tone Booster (Everyday Electronics, Sept 1978)

Two transistor (NPN ZTX384 + PNP BC415P) treble-peaking booster at 9 V. R: 330k, 820k (bias), 4k7 x2, 2k2 x2 (twin-T-like), caps 0.1 uF, 22000 pF (22 nF), 100 pF, 0.1 uF x3, 150 uF supply decoupling. Peaks near 5 kHz.

## p.12 Controllable Tremolo Circuit

+18 V. HEP 251 (NPN) in a phase-shift oscillator (47k, 47k, 2k2, 1k, 25k Rate pot, 1 uF, 1 uF, 2 uF) -> 10k + 100k Depth -> 1M gate of HEP 801 JFET (input 0.5 uF, 5k6 drain, 47k source, 1 uF out). 4k7 collector, 0.1 uF decoupling.

## p.17 UniVibe (Unicord model 905, c.1968)

Transistors 2SC828 (and 2SC539 for the input stage). B+ rail with 22k/100k decoupling, four LDR phase-shift stages (CDS MXY-7BX4), caps 15 nF, 220 nF, 470 nF, 4.7 nF (4N7), 100k/4k7/47k/68k/1.2M/3.3k bias network, lamp driver 100k/47k/1k with 10u caps; 250k dual speed pot, 220k, 2.2M, 4.7k, 3.3k LFO phase-shift oscillator; 24 VAC / 115 VAC transformer, 1000 uF filter, 470 / 470 / 220 uF filter chain; chorus/vibrato switch (100k, 47k, 220k mixing). Supply: 24 VAC rectified, bulb 6.3 VAC.

## p.23 Univox Super-Fuzz (late 1960s / early 1970s)

Fuzz/octave: 2N2222 / 2N3391 high-gain NPN types. Input: 22k, 47k, 100k, 1k8 etc. Two-transistor gain stage, a drive pot (50k), a third transistor, then a differential pair with tied collectors acting as a full-wave rectifier (octave generation), 22k / 10k / 100k / 1k8 / 470. Back-to-back clipping diodes just before the normal/notch selector (originally germanium; silicon or LEDs possible). Notch filter: 0.001 uF with 10k and 22k, 47k, 0.1 uF (1 kHz notch). Level 50k, output transistor with 100k / 10k / 15k / 1k. 9 V battery, 100 uF/10V decoupling.

## p.24 Vox Tone Bender (Mk 1.5-like, two transistors)

SFT363 (Q1) + SFT337 (Q2) germanium (2N3906 listed as modern substitute; originals Ge). Positive ground, 9 V.
Values: .022 uF input, Q1 collector 10k to rail; Q2 base direct from Q1 collector; Q2 collector 8.2k with .032 uF output cap to 50k-100k volume pot; 1k in the supply; 47k feedback from Q2 emitter to Q1 base; Q2 emitter 1k pot (fuzz) + 8.2k to ground with 25 uF bypass on wiper.

## p.25 Vox Treble Booster

Single 2N2924 NPN at 9 V: 100k / 22k bias, 500 pF input cap (0.1 uF out), 2K2 + 2K2 collector, 1K emitter with 10 uF bypass. Modified overdrive version: input cap 0.01-0.022 uF, 500k pot on output.

## p.26-28 Electro-Harmonix Bass Micro Synthesizer (D. Cockerell, rev 12/12/78; redrawn by F. P. Hartery 28 Sep 1996)

Circuit type: monophonic analogue guitar/bass synthesizer: envelope-following VCA/VCF sweeps, square-wave octave and sub-octave generation by divide-by-two flip-flops.
Supply: dual +/-V from transformer (24 VAC secondary, W02M bridge, 35V/25V filter caps, 1N961B 10V zener for -V, 2N6110 regulators), 15k, 100 uF etc.
ICs: RC4558 (many sections), CA3094 (or EH1040) OTA/VCA x6 (A8 shaper, A10-A12 VCF, A13, A14 VCA, A18 sub-octave modulator), CD4013 dual D flip-flop, 2N5087 (Q1, Q3), 2N4302 JFET (Q2), 2N5088 x3 (Q4-Q6) attack/delay.
Diodes: 1N914 (many), 1N5235 6.8V zener x2.

Sheet 1 key values: input J1, R1 68k, R2 4.7k; pre-amp gain pot R6 100k lin with 13k/10k/470; 700 Hz low pass filter R7 2.2k, R8 3.3k, R9 33k with C1 0.1, C2 0.022, C3 0.0022 uF; full-wave rectifier R10-R13 (10k, 10k, 7.5k, 15k), 1N914 x2; squelch R14 100k, R15 3.3M, R16 27k, C5 0.0018; log amp 4x 1N914 (D4-D7), R24 1k, R25 22k, R23 47k; trigger R31 100k, R30 2.2k, R32 390k; octave pot R28 100k lin with R26 820, R27 4.7k, R29 8.2k; smoothing filter R19 560, R20 3.3k, R21 2.2k, R22 22k, caps 4.7 / 2.2 / 1.0 / 0.1 uF; square-wave shaper R17 2.7k, R18 27k, C6 1.0; square-wave modulator Q1 2N5087, R36 100k lin, R37 13k, R34 2.7k, R35 10k, C14 1.0, C15 0.047.
Sheet 2: adaptive Schmitt trigger (RC4558 x2, 220, 10k, 3.3k, 47k, 6.8k, 4.7 uF); S-R flip-flop and divide-by-two CD4013; sub-octave track-and-hold (A18); VCF (A10-A12, three-pole CA3094 stages, 0.0033 uF caps, 3.3k, 8.2k, 47k, 1k, resonance 100k lin); VCA A14; sweep rate 100k lin, start/stop frequency pots 100k lin, 6.8V zeners; attack/delay 100k lin with 2N5088 x3; 470/820/22k etc.
Sheet 3: power supply as above.
Variant p.29-31 ('GUITAR' version): C10 0.056 uF (was 0.1), added 0.47 cap, R~12k -> 2.2k on sheet 2; otherwise identical.

## p.32 Bob's Octavia (adapted from Bob Starr)

LM386 (9 V) + Radio Shack 273-1380 transformer T1 (used as frequency doubler with two 1N914 as full-wave rectifier); 50k gain pot with 2.2k, 0.05 uF in, 220 uF coupling, 100k, 50k level pot, 0.05 uF out.

## p.33 UniVibe Lamp Flasher circuit (Bob Starr)

Exar 2206 function generator at 9 V, 10 uF, 5.1k x2, 57k, 33 uF, 1k, 100k lin speed pot, 1 uF; 2N2222 x2 + 2N3568 lamp driver, 470, 10k, 47k, 100k lamp-bias adjust, 20k depth, 1k, 10 ohm; lamp 6 V 0.025 A.

## p.42 BI-PHASER (hobby)

Two 4-stage phaser chains with BF245A JFETs as variable resistors (22k drain resistors, 47 nF caps in the first chain, 15 nF in the second), 4558 op-amps, 10k and 100 pF per stage, depth 100k lin pots, 0.22 uF, 22k, 47k, 741 LFO (speed 500k lin, 10k, 10 nF, 150k, 470k, 100k, 3M9, 1M trim, 250k, 10 uF), dual supply 9-18 V; JFET matching test: 1k, 15 V.

## Amps (names only)
- p.34-41 Marshall 1987, 2204, 2205, 2210, 2550 and PSUs (+ photocopy noise).
- p.43 Fender Blues Deluxe, p.44 Fender 2x6L6GC amp (7025 preamp, normal/bright), p.45 Fender Champ 12, p.46-47 Fender 60-series.

---

## Gaps and relevance (for StarterAmp)

What this range does NOT give us:
- Nothing for any Big Muff version (Standard, Smooth, Gritty, Mid-forward, Heavy, early large-box, op-amp build, 560 pF modern). No diode-pair clipping stages, no Muff tone stack, no 2N5088/BC239 Muff transistor biasing. Need pages beyond 47 of the book for these.
- No Fuzz Face (p.24 Vox Tone Bender is the nearest relative, similar 2-Ge-transistor feedback topology but it is a Tone Bender MK-ish circuit and not a Fuzz Face). No Fuzz Factory. Super-Fuzz (p.23) is a different fuzz/octave topology with notch filter.
- No Rat, Klon-style, Tube Screamer, DS-1. The only overdrive-like content is p.25 (modded Vox treble booster, crunchy distortion), p.13 (op-amp + 12AU7 tube distortion), p.14-15 (PAiA tube preamps).
- No Phase 90 style phaser. Closest: p.42 Bi-Phaser (JFET/4558 all-pass chain, eight stages) and p.17 UniVibe (4-stage LDR phaser, model 905). Small Stone (p.1) is an OTA phaser, usable for a Small Stone request.
- No Juno-style chorus and no CE-1. The UniVibe (p.17) has a chorus/vibrato output switch (chorus = phase-shift taps mixed with dry) but is not a BBD chorus.
- No flanger, delay, reverb pedal, Memory Man, Mistress.

Useful for StarterAmp requests:
- Small Stone (p.1): directly relevant to a 'Small Stone / EHX phaser' request: 4 OTA all-pass stages, 0.0068 uF, 27k, 5th OTA LFO, color switch feedback. Treat as the highest-value circuit in this range.
- Soul Preacher (p.2): a compressor/sustainer reference with JFET gain-reduction and op-amp rectifier; useful if a compressor module is added.
- Bass Micro Synth (p.26-31): octave/sub-octave and filter-sweep synth. Interesting for an octave effect but heavy to implement.
- Univox Super-Fuzz (p.23): octave-up fuzz with Ge diode clipping and 1 kHz notch; distinct from Big Muff.
- Vox Tone Bender (p.24) and Treble Booster (p.25): simple, Ge/Si boosters and fuzz references.
- UniVibe (p.17) and Bi-Phaser (p.42): phaser references (4-stage and 8-stage) if a Phase 90 style phaser is needed.
- Marshall and Fender amp schematics (p.34-47) are amp-model references and are not EHX.
