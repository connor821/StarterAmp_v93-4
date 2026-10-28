# Schematic read: pages 48-94 of "All Electro-Harmonix Effects.pdf"

IMPORTANT: this part of the book is NOT mostly Electro-Harmonix. Pages 48-94 are a mixed collection (Fender/Marshall/Orange/Music Man amps, Boss, Ibanez/Maxon-family pedals, PAiA, DOD, MXR, hobbyist drawings). Only two sections are actual EHX: the Big Muff Pi (p80) and the EHX Bass Micro Synthesizer (p83-85). Pages were located by viewing every page image; page numbers below are PDF page numbers (p-NNN.png).

Reading confidence: values marked (?) were blurry or ambiguous. Resistors in ohms/k/M, caps in uF/nF/pF as printed.

---------------------------------------------------------------------
## 1. FULL-DETAIL ENTRIES (priority circuits)
---------------------------------------------------------------------

### 1.1 p80 - ELECTRO-HARMONIX BIG MUFF PI (original, hobbyist redraw, discrete 4-transistor)
Supply: 9 V battery, switched; DPDT-style bypass shown (in/out switch). Transistors: "SPT 87-103" originals (note says unknown, any high-gain NPN works; 4 NPN). Diodes: 4x 1N914-class, originals marked "525GY / 523GY" (silicon, 2 antiparallel pairs). No part numbers on schematic itself.
Topology: input buffer/boost -> sustain pot -> clipping stage 1 -> clipping stage 2 -> passive tone -> recovery stage -> volume.

Input: 39k (series, input jack) -> 1u -> Q1 base.
Q1 (boost stage, common emitter): base bias 100k to ground; collector-to-base feedback 470k in parallel with 500p; collector load 15k to +9V; emitter 100 ohm to ground. Collector -> 1u* coupling -> Sustain pot 100k (top end), lower end of pot to ground via 1k. Wiper -> 1uf* -> 8.2k -> Q2 base.
Q2 (clipping stage 1): base 100k to ground; feedback 470k || 500p collector-to-base; collector 15k to +9V; emitter 100 to ground. Feedback clipping branch: two antiparallel diodes in SERIES with a 0.1** cap between base and collector, in parallel with 470k||500p (as drawn the diode pair connects to the base node and the 0.1** cap connects the diode pair to the collector). Collector -> 1u* (electrolytic, + toward collector) -> 8.2k -> Q3 base.
Q3 (clipping stage 2): identical to Q2: 100k base to ground, 470k || 500p feedback, antiparallel diode pair + 0.1** series branch, 15k collector, 100 emitter.
Q3 collector goes DIRECTLY (no coupling cap drawn) into the tone network:
 Tone: 39k series from Q3 collector to tone pot top (with 0.01 from that point to ground = low-pass leg); 0.004 from Q3 collector to the bottom (high-pass leg) of the tone pot; pot 100k with lower end to ground... (pot ends: top = 39k/0.01 node, bottom = 0.004 node; wiper out). Wiper -> 0.1 -> Q4 base.
Q4 (recovery/output stage): base bias 390k from +9V, 100k to ground; collector load 10k to +9V; emitter 2.2k to ground. Collector -> 1u* -> Volume pot 100k -> output jack.
Notes printed: caps marked * (1u coupling) "reported to sound better at 0.1uF"; caps marked ** (0.1 in the diode branches) "better at 1.0uF". Authors say schematic is the original; diode/transistor types unknown. Coupling electrolytics 1u at input, after Q1, after Q2, after Q4.
Caveat for modelling: the series 0.1 cap with the diode pair is as drawn on this redraw; standard Triangle/Ram's Head documents show the diodes directly in the feedback loop with the 470k/500p. Treat this drawing as an "early-style redraw", not authoritative.
Key numbers for an "early" Big Muff build: gain-stage collector 15k, emitter 100, base 100k, feedback 470k || 500pF, inter-stage series 8.2k, tone 39k/0.01 + 0.004 + 100k pot, recovery 390k/100k/10k/2.2k, sustain 100k (+1k), volume 100k.

### 1.2 p83-85 - ELECTRO-HARMONIX BASS MICRO SYNTHESIZER (designed D. Cockerell, rev 12/12/78; drawn F. Hartery 28 Sep 1996; 3 sheets)
Analog bass-to-synth. Supply: external 24 VAC wall transformer -> bridge BR1 (W02M) -> C31 100u/35V -> +9 V / -10 V regulated rails via A1A/A1B (RC4558), Q5 2N6110 pass transistor (hot), D21 1N961B 10V zener, D22 1N914, R106 15K, R107 5.6K, R108 1K, R109 12K, R110 4.7K, R111 680 + LED D20; C32/C33 10u/25V.
Sheet 1 (p83): guitar input -> R2 4.7K / R1 68K -> A2A (RC4558) preamp; pots: Guitar R6 100K lin, R5 13K, preamp gain R3 10K/R4 470. 700 Hz low-pass filter (A2B): R7 2.2K, R8 3.3K, R9 33K, C1 0.1, C2 0.022, C3 0.0022. Smoothing filter / peak follower (A5B, 1N914), log amp A4A (4x 1N914 D4-D7), A4B, squelch (A5A, R14 100K pot, R15 3.3M), full-wave rectifier (A3B, D1/D2 1N914, R10-R13), A3A, adaptive stop detector (A7A), attack detector (A7B), square-wave shaper A6A, square-wave modulator A6B with Q1 2N5087, CA3094 (or EH1040) OTA A8. Octave pot R28 100K lin, trigger pot R31 100K lin.
Sheet 2 (p84): adaptive Schmitt trigger (A15A/B, A16A/B RC4558, 1N914s D12-D15), CD4013 flip-flops (A17A/B) for divide-by-2 (sub-octave), sub-octave track and hold modulator A18 (CA3094 / EH1040) with Q2 2N4302 JFET, VCF chain of four CA3094/EH1040 OTA stages A10, A11, A12 (resonance R86 100K lin, sweep rate R74 100K lin), VCA A14 (CA3094), Q3 2N5087, Q4-Q6 2N5088 (envelope/attack-delay, R101 100K lin), start/stop freq pots R65 R92 100K lin, zeners D17/D18 1N5235 6.8V.
Sheet 3 (p85): power supply (above).
Relevance: synth/VCF/sub-octave design; not directly useful for Big Muff/fuzz; reference for envelope-follower-driven VCF only.

### 1.3 p57 - JORDAN BossTone (fuzz/distortion, drawn by Aron Nelson 6/98; Jordan Electronics factory schematic minus switching)
Supply: +9V through R6 18K (decoupling, also Q2 emitter load).
Q1 NPN "0101" (germanium-era part labels; NPN), Q2 PNP "0102". Mod notes: Q1 = 2N2222 NPN, Q2 = 2N2904 (NTE-129) PNP.
Values: R1 100K "Attack" pot (input), C1 .02uF, C2 .02uF (input coupling; C2 from input/attack node to Q1 bias junction), R4 560K (to +9), R3 560K (base to collector-ish bias junction; R4/R3 divider feeding Q1 base), R2 15 ohm (Q1 emitter to ground), R5 18K (Q1 collector load to +9), C3 shown "50uF" but mod note says 47pF (collector-to-+9/feedback cap, ?), Q2 PNP emitter-follower (collector to ground, emitter to +9 via R6 18K), C4 .02uF to output node, D1 1N914 and D2 1N4148 antiparallel to ground (clipper), R7 100K "Volume" pot.
Tested mods: C1,C2,C4 = .022uF; C3 = 47pF; R2 = 150K (?); R7 = 10K pot.
Circuit type: two-transistor direct-coupled fuzz (Q1 gain stage with high-ohm bias, PNP follower) followed by diode clipping at the output: Bosstone-style fuzz. Distinct from Fuzz Face (no feedback pair here; second stage is a PNP follower).

### 1.4 p69 - THE PURPLE PEDAL (C. Steele, 4/98; Big Muff-type clone, 5 transistors)
Supply 9V battery; D7 red LED with 2.2K as power indicator only. Bypass: DPDT-ish switch.
Transistors: all high-gain NPN; author used Q1,Q4 = 2N5088, Q2,Q3,Q5 = 2N5089 (2N2222 also OK for Q2). Diodes: D1,D2 germanium 1N34A ("very high clipping", 1N914 may substitute); D3-D6 1N914.
Topology: Q1 input boost (15K(?)/18K(?) collector, 470K||470pF(?) feedback, 100K base to ground, 100 emitter, 1uF tantalum input after 24K(?) input resistor); Saturation pot 100K (+ 1K(?) to ground) between stages, switchable; Q2 clipping stage (D1/D2 antiparallel Ge diodes in feedback with .22uF tantalum series cap, 470K||470pF(?) feedback, 100K base, 100 emitter, 10K(?) collector, interstage 0.47uF tant + 8.2K); "Load Vol/Saturation" 100K pot on Q2 collector selects whether Q2 is wired; Q3 and Q4 clipping stages same pattern with D3/D4 and D5/D6 pairs, .22uF series caps, 8.2K inter-stage resistors, 0.47uF tantalum couplers; tone: 39K(?)+0.01uF low-pass, .0047uF(?) high-pass, 100K tone pot, 100K lower leg; Q5 recovery: 100K/100K base bias, 10K collector, 2.2K emitter, 0.47uF out to 100K volume pot.
Note says: "Basically a two-stage clipping circuit with an additional stage up front switched in for Load". All caps marked T are tantalum. Four clipping-style stages + boost = like a 5-transistor Muff with germanium option; shows the same "diode pair + series cap in the feedback branch" drawing as p80.

### 1.5 p94 - ROLAND DOUBLE BEAT (fuzz + wah; source: unknown hobbyist redraw)
Supply 9V battery. Three-section pedal: Fuzz (3x 2SC1000), Wah (2SC1000 + 2SC828).
Fuzz section: Q1 2SC1000: base 0.1 input, 470K base to ground, collector 820K to +9 and to Q2 base, emitter 1K2 to 22K to ground with 1uF bypass; 47pF collector-to-base. Q2 2SC1000 (Fuzz Face-style pair: 2K2 coupling, collector 120K, emitter 22K/22K, 470K feedback to Q1 emitter network, 1uF + 10K output (level) pot). Q3 2SC1000 third stage: 22K collector, 1M/220K base bias, 1K emitter, 0.01 coupling, 0.1 output; 3-position FUZZ TONE select: 330K only / 150K+250pF / 150K+0.0068 with 1M and 0.0068 shunt.
Wah section: Q4 2SC1000 (1K5, 20K, 470 emitter, 68K input, 47K/33K input divider, 0.01), Q5 2SC828 (470K x2, 0.22 x2, 150K pot sweep, 10K emitter, 0.01, 1K), 0.5 H inductor, 10uF, 100K.
Relevance: Fuzz Face-family 2-transistor feedback pair plus a third gain stage. Silicon (2SC1000 NPN).

---------------------------------------------------------------------
## 2. FULL-DETAIL ENTRIES (overdrive / distortion / op-amp clippers)
---------------------------------------------------------------------

### p72 - MXR Distortion + (hobbyist redraw, rotated)
Supply 9V battery (or AC adaptor). 741 op-amp (+ input from 1M/1M/1M 4.5V bias network with 1uF). Input: 0.001 to ground, 0.01 series, 10K, ... gain network: 4.7K + 1M pot (Distortion), 0.047uF to inverting input; feedback 1M with 8-10pF optional (dotted). Output: 1uF tantalum 35V, 10K, then diode pair D1/D2 (originals germanium 1N270, replaceable: 1N34s, 1N4148 for fuzzier, LEDs for more crunch) to ground, 0.001uF to ground, 10K Level pot. LED indicator with 2.2K via S1. Dotted: 1M click-prevention resistor and the 8-10pF feedback cap.
Type: classic op-amp gain stage + hard-clip diode pair (Rat/DS-1 family ancestor; no tone control).

### p92 - DOD Overdrive 250
Supply 9V battery. 741 op-amp non-inverting: input 0.01uF, 10k; bias 20K/20K/1M... divider with 10uF; feedback: 500K reverse log pot (gain) + 4K7 + 0.05uF to inverting input, 1M feedback; output 10uF -> 10K -> two antiparallel diodes to ground + 0.001uF -> 100K level pot. Text: "almost exactly the same as MXR Distortion+".

### p87 - Gretsch Controfuzz Model 7798
9V; two 748 op-amps (25pF compensation caps). Stage 1: gain 220K feedback / 100 + 10uF to ground (gain ~2200 at LF), input 0.1uF, 100K/100K bias, 220pF; output 10K -> 0.1uF -> 2x 1N4001 clipper (to ground) -> 220K -> 10K "Distort" pot (mixes distortion), -> 100K -> 0.47uF -> stage 2 (330K feedback, subtracts distortion from dry signal) -> 0.47uF -> 10K "Boost" pot.

### p79 - Blue Clipper (generic)
1/2 4558 non-inverting: 0.047uF in, 20K/240K bias, feedback 150K with 240K + 4.7uF to ground (gain ~1.6x), output 4.7uF + 8.2K to antiparallel diodes to ground (D1, D2 "pick an element"), then 10K + 50K(?) pot with .033uF to ground. Supply 9V.

### p77 - PAiA Axe Grinder (model 5750, rev 10/28/95)
Op-amp distortion, 4136 quad op-amp + 4066 quad switch. IC1a input buffer (0.1uF in, 10M/10M bias, 4.5V reference). IC1b distortion stage: 0.1uF + 10K in, feedback 15pF + 100K + 500K pot (distortion intensity), clipping diodes D1/D2 1N914/1N4148 antiparallel in feedback loop (soft-clip feedback style). Tone pot 5K + 100K level pot, 0.1uF. IC1c clean path (12K, 100pF, 220K, 100K level) with 4066 switch; IC1d mixer (10K, 4.7K, 1K). Supply 9V with 10K/10K 4.5V divider, 10uF caps.
Relevance: op-amp feedback-diode clipper (TS-style soft clip) with blend of clean and distorted.

### p76 - Angry Beard III (Jamie Heilman)
Supply +9V. Op-amp (NE5534 original) non-inverting: 0.01uF + 10K input; 4.5V bias via 20K+20K+10uF and 1M; feedback 1M with emphasis switch: 0.22uF (Normal) / 0.022uF (High) in series with 330 ohm to ground. Output 10uF + 10K -> PNP germanium 2N44 emitter follower (1K collector to +9? as drawn, 0.0056uF + 100K emitter network, 0.22uF output cap) -> 10K pot out. Type: op-amp boost into germanium PNP stage (distortion/treble boost).

---------------------------------------------------------------------
## 3. FULL-DETAIL ENTRIES (modulation, delay, dynamics, filters)
---------------------------------------------------------------------

### p50 - Boss PH-1-type phaser (unnamed; board code MP-P01001A)
Supply 9V battery or external DC (D105 W03C, 1N5231B 5.1V zener D104 -> 4.5V mid rail, C126 100/10, C125 47/6.3). 
Parts: IC101-IC104 MC1458 (dual op-amps), Q101-Q104 2SK30AY (selected) JFETs as variable resistors, Q105 2SK44SP JFET (LFO/bias), Q106-108 2SC536SP, diodes MA150 (D101-102), RD5.6EB zener D103, LED LN21CPSL.
Phase stages: 4 all-pass stages (IC101/102/103 op-amps, each stage: 0.047uF (C102..C109 pairs), 10K R103/107/108/114/118/119/123, R104/109/115/120 150K, R106/111/116/121 30K, R105/110/117/122 100K to bias bus), feedback via VR102 10K "FEEDBACK" with R113 10K, C106 100pF, R112 10K; input buffer IC101 (1K R101, 510K R102, 0.1 C101); output IC101 inverter/summer 10K R135/R136/R132, 4.7K R134 + C117 100pF, 470 R137, C118 10/16, R138 10K, 22K R133. Bias: VR101 100K "BIAS", R125 200K, R126 240K. LFO: IC104 integrator/Schmitt (C113~114 47/10 x2, R128 82K, R129 22K, R130 100K, R127 24K, C115 0.01), 2-transistor multivibrator Q106/Q107 with R142/R143/R149/R150 56K, C122/123 1000pF, C124 1000pF; Speed pot 500KC, Width 100KB. Q108 via R140 1M, D103 5.6V zener, R141 510, R144 100. Output coupling C111 1/50 NP, C116 1/50 NP.
Type: JFET all-pass phaser (4-stage), similar family to MXR Phase 90 (4 JFET stages in Phase 90 vs here op-amp all-pass with JFET control). Very relevant for a "Phase 90 style" request in terms of topology, but op-amp-based not discrete.

### p49 - Boss-style 7-band bass graphic EQ (board PCB-BE-0101A; likely GEB-7 bass EQ)
Parts: NJM4558D (IC1-4), 2SC2458GR, 2SK118(O), 2SK44P, 1S1588 diodes, RD5.1EB, W03C. Supply 9V with 4.5V rail. 7 bands: 63Hz, 125Hz, 250Hz, 500Hz, 1kHz, 2.2kHz, 5kHz with 20K(G) sliders + LEVEL; per-band gyrator-less circuits: 120K, 330, plus cap values (0.033, 0.018, 0.0068, 0.0039, 0.0018, 820p, 390p). Input buffer with JFET switching (Q2/Q3, 2SK44P, 1M bias), LED, electronic bypass using flip-flop (Q5-Q8 2SC2458GR, 1000pF, 1M, 56K, 66K, 62K, 22K). Not circuit-critical for amp-sim.

### p58 - Compressor/limiter (Ibanez/Maxon-family; title not printed)
Supply 9V. IC1 NJM4558DV (4.7K, 47K, 6.8K, 0.0047, 0.022), IC2 NJM13600 OTA (selected, red), IC3 LM358; Q1 2SC2785K buffer; Q2 K381B JFET (release time); diodes 1S1555, RD5.1EB2 zener. Controls: Threshold 10KB (with 180 ohm, 56K), Release time 33K+2MA pot, output. LED with 1.2K and RD5.6EB1 zener. Compressor via OTA gain control with rectifier (D7-D9 1S1555). R values: 100K, 5.1K, 220K, 1K, 100 ohm, 22K VR1 47KB trimmer.

### p59 - Octaver (Ibanez/Maxon family; title not printed)
Supply 9V. ICs: 4558 (a,d), TL062 (b,c), 4013 (e) flip-flops (octave divider), NJM13600 selected (f, OTA/vca). Controls: Effect level 100KB, Direct level 100KB, Polarity switch, sub-octave via 4013 divide-by-2 from a squaring comparator (IC3c TL062, 47K, 220K, 1M). Filters: 22K/330K/0.01/0.0022 low-pass (IC6), 3.3K, 220P, 0.022.

### p60 - Parametric EQ (Ibanez/Maxon family PQ-style; title not printed)
Supply 9V. ICs: TX-429D (IC1, IC2, IC3; selected photo/FET module), NJM4558DV (IC4,5,6,7). Controls: GAIN boost/cut 100KB, BAND-WIDTH 100KA, FREQUENCY 100KB. Active state-variable bandpass: 100K, 150K (R6/R7), 91K R12, 220 ohm R5/R8, 0.015 C8/C9, 0.01 C2-C5, 1M R1-R4, 470K R41, 51K R14, 68K R18. Input buffer 10K, 0.01, 1M; D5/D6 1S1555; IC3 TX-429D as input stage JFET pair.

### p61 - 4-stage phaser (Ibanez/Maxon family PT-style; title not printed)
Supply 9V. IC1-IC4 TX-429D (selected) variable resistance modules, IC5-IC7 NJM4558DV, IC8 TL062 (LFO). Four all-pass stages: each 10K, 10K, 0.01, 0.0047, 110K, 1M (R2-R5), feedback R11 33K, INTENSITY pot 100KB, SPEED 500KC (4.7K, 150K, 0.01 C14, 15uF C16, 68K, 150K, 470K). R15/R24 15K, output 10K, 1K5.
Relevance: another Phase 90-like (4-stage) option with op-amps.

### p62 - Preamp/EQ with limiter-style JFET feedback (Ibanez/Maxon family; code "KLM690"; title not printed)
Supply 9V. IC1 TL072a, Q2 2SK30A (input JFET buffer, 22K source, 10K), Q3 2SC2785K, Q1 2SK30A (JFET in feedback network of IC1 controlled by rectified output: D3, 1M, 0.047, 100K) giving gain-limiting; Gain 1MC pot (with 1.8K, 1uF), feedback R8 330K; treble 200KB/bass 200KA pots with 220p, 0.14, 0.022, 100K, 10K; second stage IC1b: 1M, 4.7M, 0.022, output 1K, OUT LEVEL 10KB, 470 ohm / 47K output. Not a distortion; clean compressor-EQ.

### p81 - Boss Slow Gear SG-1 (attack delay/swell; hobbyist redraw)
Supply 9V + 4.5V (22K/22K divider, 30u/10u). Q: 2SC932 (x2 buffer in/out), 2SC1815 elsewhere, 2SK30A JFET (VCA). Sections: JFET VCA with 220K/470K/3.3K/47K/10K trimmer 25K(?); Attack 20K pot; Sensitivity 100K pot; trigger op-amp (220K, 1M, 3.9K, 1u, 390K, 0.001, 0.022), envelope smoothing, discrete multivibrator (47p, 56K, 100K, 470K), cancel switch. Large caps 1u/47u/30u.

### p91 - DOD FX75 Flanger (10-15-84), drawn F. Hartery 11/20/94
Supply 9Vdc battery (1N4001 protection). Parts: TL022C dual op-amps (U1A/B, U2A/B, U3A/B), MN3101 clock generator, MN3007 1024-stage BBD (5.12-51.2 ms delay), CD4007, Q1 J113 JFET (input?), Q2/Q6 2N5089, Q3 2N4125, Q4 2N4124, Q5 J113 (LFO/bypass), diodes 1N4148 (many, e.g. in compander-free path, limiter). Controls: Regeneration 500K, Width 100K, Speed 500K, Manual 100K (P1), caps 0.047, 0.001, 0.0063, 0.022, 120pF. Input 470K, 0.01, 10uF; momentary switch to engage.
Type: BBD flanger (Boss BF-2 style). Mono in, stereo out via U1B/U2B-type outputs. 

### p89 - DOD Compressor 280A
9V; LM358 x2; VTL5C2 LED/LDR vactrol; Q1,Q2 NP4124 (NPN) as full-wave current source for LED; 500K gain (sensitivity) pot, 0.05uF, 22K, 3M feedback, 4.7K, 220K x2, 47uF. Author notes missing 100K at second opamp (-) input.

### p90 - DOD Envelope Filter 440
9V battery; TL022 dual op-amps; vactrol (LED + LDR pair): LED driven via 1N4148 detector from op-amp; filter: state-variable-type with 220K, 22K, 100K, 0.02uF x2, 430K, 0.05uF, 10uF; controls 100K RL (level), 100K "D" (decay?), 22K, 100K, 4.7uF, 1uF, 10K, 0.01/0.1.

### p88 - Dunlop Cry Baby wah (hobbyist redraw)
9V; Q1, Q2 2N3904 (original 2N3904-era; German original uses MPSA...), D1 1N4148; inductor 660mH; caps .001, .0011, .01, .01, 4.7uF; R: 68K, 1.5K, 470, 22K, 470K x2, 1K, 10K, 33K, 82K, 1K supply. True bypass S1 DPDT.

### p82 - Frequency Brighteners (Delton Horn, "Music Synthesizers"; TAB Books 1984)
(a) High frequency brightener: 2N3904, 120K/62K bias, 6.8K collector, 0.22uF in, 0.47uF out, 5K intensity pot, 0.15uF, 1K. (b) All-signal brightener: 2N3904, 47K, 1M, 100K bias, 1uF in/out, 22K, 1.8K, 100K gain pot, 0.01uF, 0.1uF, 15K, 100K pots, 0.0047uF, 0.047uF, 4.7K, 2.2K, 10K trimmer.

---------------------------------------------------------------------
## 4. ONE-LINE ENTRIES (non-priority)
---------------------------------------------------------------------
- p48: Fender Studio 85 amplifier schematic (Musical Instruments, Brea CA, 9-2-87; Drawing 028051 rev B): solid-state, op-amp preamp, power amp.
- p51: Marshall JCM900 100W power amp (ECC83 driver, 4x EL34, circuit diagram CD0109), same book as p52-56.
- p52: Marshall JCM900 50W power amp (models 2500/2501/2502/4500/4501/4502; 2x EL34, CD0112).
- p53: Marshall JCM900 100W power amp (models 2100/2101/4100/4101/4102; 4x EL34, CD0192).
- p54-55: Marshall JCM900 master-volume preamp (models 2500/2501/2502/2100/2101; ECC83 stages, op-amp stages; CD0109).
- p56: Marshall JCM900 dual reverb preamp (4500/4501/4502/4100/4101/4102; CD0189) - low-quality scan.
- p63, p65: Marshall 2205 STD preamp (JCM800 split-channel master volume, 1988) - duplicates; p64, p66: 2205 STD output stage & PSU (2x ECC83, 6L6/EL34 pair).
- p67: Music Man RD-50 amplifier (5004 board, 12AX7, 6L6GC, TL072, 4016B).
- p68: Orange Amplifier Graphic MK II 120W (1972; 4x EL34/KT88-type).
- p70: Robin Tomtlund reverb driver/receiver (TL071, BD139/BD140 +/-15V driver, LF351 receiver) 1999.
- p71: Mu-Tron III (1973, above S/N 02050) - image is only 334x261 px; unreadable at this resolution.
- p73: Simple 3-band tone control (Graff, Popular Electronics): op-amp Baxandall-style 3-band; caps 0.05, 0.005, 0.022, 1uF, 10uF; pots 100K x2, 500K; 11K, 3.6K, 1.8K.
- p74: PAiA 9303 Four-Band EQ (rev 9/21/95; 5532 x4): bands 35-680Hz, 150-3kHz, 450-8.5kHz, 750-15kHz.
- p75: PCB artwork only (no component values).
- p78: Fender Bassman 5F6 (Tweed) - 12AX7 x3, 5881/6L6 pair, GZ34 rectifier.
- p86: Ultra-Clean 9Vdc power supply (Rick Barker): LM317T.
- p93: Dual preamp & A/B box (Rick Barker, 5532 x2).

---------------------------------------------------------------------
## 5. Gaps and relevance
---------------------------------------------------------------------
Pages 48-94 contain NO Small Stone, Electric Mistress, Memory Man, Holy Grail, Screaming Bird, Hot Tubes, Soul Food, Mole, Nano, Hog's Foot, Bass Balls or Ram's Head schematics; and no Rat, Klon-style, Tube Screamer, DS-1, CE-1, Juno-chorus, or Fuzz Face / Tone Bender / Fuzz Factory pedals. The only EHX circuits are the Big Muff Pi (p80) and Bass Micro Synthesizer (p83-85).

Relevance to StarterAmp:
- Big Muff variants (Standard/Smooth/Gritty/Mid-forward/Heavy; early large-box; op-amp build; modern 560 pF build): p80 is directly relevant as the "original" four-transistor reference (15k collectors, 470k||500pF feedback, 8.2k inter-stage, 39k/0.01 + 0.004 passive tone with 100k pot, 390k/100k/10k/2.2k recovery stage, 1u couplings). It does NOT supply the 560 pF modern values, the op-amp build, the heavy/mid-forward variants, nor the exact Triangle/Ram's Head values (coupling caps in this redraw are suspect, diodes as drawn are in series with 0.1 caps). The Purple Pedal (p69) adds a 5-transistor "extra boost stage + Ge diode option" variant. The Bosstone (p57) is another 2-transistor fuzz.
- Fuzz Face / Tone Bender / Fuzz Factory: only the Bosstone (p57, 2-transistor fuzz with PNP follower), Roland Double Beat (p94, Fuzz Face-style 2-transistor feedback pair + third stage, silicon 2SC1000) and Angry Beard III germanium PNP stage (p76) are even loosely related. No true Fuzz Face, Tone Bender or Fuzz Factory schematic here.
- Rat-style: only generic op-amp + hard-clip diodes (MXR Distortion+ p72, DOD Overdrive 250 p92, Gretsch Controfuzz p87, Axe Grinder p77) - the common ancestor to Rat/DS-1 but without Rat's tone/filter and without the DS-1/Rat-specific values.
- Screamer / Klon: no direct match; Axe Grinder (p77) has TS-like feedback-diode clipping and Blue Clipper (p79) is a plain clipper.
- Phase 90 style: p50 (Boss PH-1-type JFET all-pass phaser, 4 stages, with full component values) and p61 (4-stage Ibanez/Maxon-style phaser) are the best candidates for a phaser model; p50 is the most readable.
- Chorus (Juno-style CE-1/Small Stone): none here. The DOD FX75 flanger (p91) shows MN3101/MN3007 BBD topology with values (flanger, not chorus).
- Other useful: Cry Baby wah (p88), compressor circuits (p58, p89), octaver (p59), DOD envelope filter (p90), parametric EQ (p60) and PAiA 4-band EQ (p74) for tone-stack/EQ modules.

Suggestion: the remaining EHX content (Small Stone, Memory Man, Mistress, etc.) is in other page ranges of the book (pages 1-47 and 95-141 were not part of this reading).
