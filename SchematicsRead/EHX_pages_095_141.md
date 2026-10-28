# EHX compilation, pages 95-141 (source: 'All Electro-Harmonix Effects.pdf')

Reading notes. Pages 95-141 are mostly NOT Electro-Harmonix designs: they are the miscellaneous/"other makers and hobbyist" tail of the book. Only a handful of true EH circuits are present (p95-97, 98, 108-110, 117, 124). Values were read from the page images and cross-checked against the pdftotext layer. "(?)" = unreadable or inferred. Schematics are hobbyist redraws (early-1990s net archive), so errors in the drawings are possible and are flagged where noticed.

Page index (PDF page -> circuit):
95-96 EH 16 Second Digital Delay | 97 EH boosters (LPB-1, Muff Fuzz early, Screaming Bird, Mole/Hog's Foot old and new) | 98 EH Fuzz-Wah | 99 Electra Distortion | 100 Ten-band graphic EQ | 101 Fender Blender | 102 Foxx Fuzz-Wah | 103 Fuzz 001 | 104 Distortion Booster | 105 Fuzz Face (Dallas Arbiter) | 106 Octaver-Fuzz (ETI) | 107 Green Ringer | 108-110 EH Guitar Micro Synthesizer | 111 Harmonic Sweetener | 112 TS-9 | 113 Hendrix Fuzz Face | 114 Jordan Boss-Tone | 115 JSH Fuzz | 116 Kay Tremolo | 117 EH Little Big Muff | 118 Maestro Fuzz | 119 Maestro Boomer 2 | 120 Maestro Boomerang | 121 Mosrite Fuzz-Rite | 122 PAiA Motion Filter | 123 Modified Tube Sound Fuzz | 124 "Muff Fuzz" (EH, op-amp) | 125 MXR Distortion+ | 126 MXR Hot Tubes | 127 Oberheim PS-1 Phase Shifter | 128 Olson New Sound | 129-132 Orange 125 Mk3 amp | 133 VC Panner | 134-135 ProCo Rat (two versions) | 136 The Bomb | 137 Discrete JFET and op-amp guitar preamps | 138 unlabeled large amp/mixer-style schematic | 139 Stage Center Reverb | 140 Marshall Shredmaster | 141 Simple Mixer.

---------------------------------------------------------------------
## A. TRUE ELECTRO-HARMONIX CIRCUITS (detailed)
---------------------------------------------------------------------

### A1. p117 - Electro-Harmonix Little Big Muff (original schematic, 9 V battery)
Page note: "diode and transistor types are unknown. Probably any high gain NPN and 1N914s work. Caps marked * reported to work great at 1.0 uF."
Topology: NPN input stage, TWO diode-clipping gain stages, switchable 2-way tone, NPN output/recovery stage with volume. Four NPN transistors total, all emitter-degenerated (NOT grounded-emitter like the classic Triangle/Ram's Head).

- Supply: 9 V, 470 uF main decoupling from rail. Collector loads fed straight off the rail (no per-stage RC decoupling).
- Input: 10 uF (+ toward the jack side) -> 33k series -> Q1 base. Q1 base bias: 43k to ground. 
- Stage 1 (Q1, input/boost): emitter 100 R to ground; collector load 8.2k to +9 V; feedback collector->base 470k in parallel with 560 pF. Output coupling 0.1 uF.
- Stage 2 (Q2, clipper): in via 10k series to Q2 base; base-to-ground 100k; emitter 150 R; collector 10k to +9 V; feedback 470k || 560 pF (collector to base). Clipping: antiparallel diode pair (two diodes, types unknown, 1N914 suggested) from base node (the junction before/after the 10k) to a series cap 0.1 uF* (polarised symbol, + on collector side) to the collector. I.e. diodes sit in the feedback loop with a DC-blocking cap in series. Stage output coupling 0.1 uF.
- Stage 3 (Q3, clipper): in via 8.2k series to Q3 base; base-to-ground 100k; emitter 150 R; collector 15k to +9 V; feedback 470k || 560 pF; antiparallel diode pair with 0.1 uF* series cap, same arrangement as stage 2. Collector goes straight (no coupling cap drawn) to the tone network.
- Tone (2-position selector switch, not a pot; Tone block fed from Q3 collector):
  - Node A (low-pass side): 39k series from the input, 0.01 uF to ground.
  - Node B (high-pass side): 0.004 uF series from the input, 22k to ground.
  - A and B joined by 100k. Switch selects A or B. Output of switch -> 0.1 uF -> Q4 base.
- Stage 4 (Q4, recovery/output): base bias 430k to +9 V and 100k to ground; emitter 6.8k to ground; collector 15k to +9 V; output 0.1 uF -> Volume 100k pot -> OUT. Power switching via input jack and a bypass switch (SPDT-style) drawn.
- Diodes: unknown/silicon signal assumed. Transistors: unknown NPN.
Relevance: this is a very different build from the classic 4-transistor BMP (emitter resistors, low base series R, no 100k||... classic tone stack, smaller supply). Fits "early/low-voltage Big Muff variant" request only loosely; the stage structure (boost + 2 clipping stages with diode pair + tone + recovery) is the Big Muff architecture.

### A2. p97 - "Electro Harmonix Boosters" (five small circuits on one page)

(a) LPB-1 / LPB-2 / EGO (single NPN booster), 9 V battery
- Q1 2N5133 (NPN). Base bias 430k to +9 V and 43k to ground. Input 0.1 uF to base. Emitter 390 R to ground. Collector load 10k to +9 V. Output 0.1 uF -> 100k volume pot to ground, wiper = OUT.
- Note on page: variant with BC239, 100k base-to-ground and 1M base-to-collector.

(b) Muff Fuzz / Little Muff Pi (early) (two-transistor fuzz), 9 V battery, 50 uF supply cap
- Q1, Q2: 2N5133, direct-coupled (Q1 collector straight to Q2 base). Input 0.1 uF to Q1 base. Q1: emitter to ground, collector load 100k to +9 V. Q2: emitter 2.7k to ground, collector load 10k to +9 V. Bias/feedback: Q1 base 100k to ground; 100k from Q2 emitter to Q1 base (DC feedback, sets operating point). 
- Output: Q2 collector -> 0.1 uF -> node with TWO diodes (opposite polarity, antiparallel) to the supply rail (AC ground via the 50 uF cap) -> 100k volume pot to ground. Diode type not given. Hard clipping at the output node, not in the feedback loop.
- This is the same circuit as the "Distortion Booster" on p104 (BC108s, see below).

(c) Screaming Bird / Tree: same as LPB-1 but with input and output caps 0.002 uF (treble booster). Q 2N5133; 430k, 10k, 43k, 390 R, 100k output pot.

(d) Mole / Hog's Foot (old version): 2N5133 booster with 430k/43k bias, 10k collector, 390 R emitter. Input: 27k in series, 3.3 uF (+), 10k pot to ground (input level/bass control), 0.1 uF to ground at the base node. Collector has 0.1 uF to +9 V rail (decoupling) and output 3.3 uF (+) coupling to Out.

(e) Hog's Foot (new): op-amp + 2N5087 treble-booster with two passive 3-stage RC low-passes (it is an active filter/"treble boost" built from op-amps; op-amps drawn as generic triangles, 9 V single supply, type (?)).
- Input 0.1 uF, 220k/220k bias divider (to +9 V and ground), op-amp 1 non-inverting buffer/gain stage: feedback 100k || 0.022 uF, 22k + 5 uF (+) to ground (gain network).
- Then a 3-section RC ladder: 10k, 100k, 1M series with shunt 0.22 uF, 0.02 uF, 2200 pF.
- Op-amp 2 (buffer/gain): feedback 270k from output to inverting input, 27k + 10 uF to ground (gain set), 100k to ground at inverting node.
- Second 3-section RC ladder: 10k, 100k, 1M series with 0.22 uF, 0.02 uF, 2200 pF to ground. 
- Output of second ladder feeds Q 2N5087 base/emitter follower: collector 27k to +9 V, then 1 uF (+) -> 100k pot to ground = OUT. Page also shows 9 V battery switch.
(The op-amp topology of this one is hard to be sure of from the drawing; treat as (?) for exact node connections.)

### A3. p98 - Electro Harmonix Fuzz-Wah (actually Fuzz-Wah/Volume), Model 3006, 9 V
- Fuzz section (op-amp, 1458 dual, 9 V single supply): stage 1 = inverting amp, input .022 uF + 100k, feedback 470k (gain ~4.7). Stage 2 = inverting, 10k input, feedback = antiparallel diodes D1, D2 (any signal diode), output 0.1 uF -> 100k pot to ground. Bias: 680k + 680k divider (9 V to ground) with 10 uF (+) to ground; same bias node feeds both + inputs.
- Same fuzz topology as p124 "Muff Fuzz" (op-amp). Note: S1 improved to give true bypass; S2 selects fuzz only / wah-volume only / fuzz into wah; S3 chooses volume or wah; S4 selects wah filter capacitor (0.0033, .01, .047, .1 uF (?), plus .0022 uF fixed); S5 sweep reverse.
- Wah section: Q1 and Q2 = 2N3565 (NPN silicon). Q1: emitter 470 R, collector 22k and 1.5k to +9 V, input from pedal pot (100k) via .022 uF/68k. Q2: emitter 10k, collector 1k. Inductor 500 mH (TDK 5103 in the Foxx version). Other: 100k, 33k, 470k (x2), 10 uF, .22 uF (x2), 68k. Wah pot 100k with reverse switch.

### A4. p95-96 - Electro-Harmonix 16 Second Digital Delay, model EH7875-2A (rev 12/9/95 drawing)
- Digital delay with analogue companding and filters. Op-amps: IC1 TL082, IC2 4558, IC4/IC11 dual 4558-type, IC9 = NE571 compander, IC7/IC8 unmarked (sample/hold or mux (?)), ADC0804 (A/D) and a D/A (unmarked) around a large RAM array (IC30-IC41), CD4049/IC12 clock logic (?), LM358 (IC10), 7812/7815/7805 regulators with separate +12 V, +10 V, +5 V, -15 V, -10 V rails.
- Transistors: Q1, Q4 2N5087; Q2, Q3 2N4859A (JFET analog switches for blend/bypass); D3, D4 6.8 V zeners; D1, D2 unmarked signal diodes.
- Analog path values (selected): input 0.1 uF + 1k, 150k to ground; IC1a buffer; 10 uF -> blend pot 10k lin; direct/delayed blend via JFET Q3 and 10k lin; delay path: IC2a/IC2b anti-aliasing filter (10k, 47k, 0.0068 uF, 0.047 uF...); IC9b compressor (NE571), IC11a gain stage with 47k feedback, 47 pF; IC7 sample-and-hold -> A/D; D/A -> IC8 -> IC4a reconstruction filter (8k2, 8k2, 680 pF, 0.033 uF) -> IC9a expander -> IC4b (47k, 10k, 0.0068 uF, 47 pF) -> back to blend. 
- Not relevant for amp sim; useful only as a reference for compander/filter delay topology.

### A5. p108-110 - Electro-Harmonix Guitar Micro Synthesizer (designer D. Cockerell, rev 12/12/78; drawn by F. Hartery, 1996)
- 3-sheet analog guitar synth: pre-amp (RC4558), squelch, full-wave rectifier, 700 Hz low-pass tracking filter, adaptive Schmitt trigger, CD4013 divide-by-two flip-flops for sub-octave, track-and-hold modulator (CA3094/EH1040 OTAs), VCF (CA3094 stages, 2N5087), VCA (2N5088 x3, CA3094), sweep generator with 100k lin pots (sweep rate, stop/start frequency, resonance, attack, sub-octave, octave, pre-gain), 1N914 diodes everywhere, 1N5235 6.8 V zeners, 2N4302 and 2N5087/2N5088 transistors, RC4558 op-amps.
- Power: external 24 Vac transformer, W02M bridge, 2N6110 pass transistors, +/-10 V (1N961B 10 V zener), 10 uF and 100 uF caps. LED on 5.6k.
- Too large to transcribe fully here; the schematic images are in pages 108-110 (125 dpi, low-contrast; many designators R1-R111, C1-C33 readable at zoom if ever needed). Not relevant to amp sim.

### A6. p124 - "Muff Fuzz", Electro Harmonix (op-amp build), 9 V
- 4558 dual op-amp, single supply with 4.5 V bias: 680k/680k divider with 10 uF (+) on the bias node.
- Stage 1 (inverting): input .01 uF + 100k to pin 6, feedback 470k. Gain ~4.7.
- Stage 2 (inverting): 10k input from stage 1 output, feedback = antiparallel D1, D2 ("select an element to taste"), output .1 uF -> 100k audio pot -> OUT. 
- Switching: 2-pole true-bypass style (S1).
- Same topology as p98 fuzz section. This is the kind of "op-amp Big Muff-like" build the project mentioned: op-amp gain stage + op-amp hard/soft clipper in feedback with diode pair, but without the Big Muff tone stack.

---------------------------------------------------------------------
## B. OTHER FUZZ / DISTORTION / OVERDRIVE (non-EH, detailed because relevant)
---------------------------------------------------------------------

### B1. p105 - Fuzz Face (Dallas Arbiter), 9 V, positive-ground PNP germanium
- Q1, Q2: PNP germanium AC128 or NKT275 (gain > 80 recommended). NPN silicon alternative: BC108C, with the 470 R changed to 330 R and the battery and all polarised caps reversed. 2N3906 said to work.
- Input: S1 -> 2.2 uF (+) -> Q1 base. Q1 emitter to (supply-) ground side. Q1 collector load 33k; Q1 collector connects directly to Q2 base. Q2 collector load 8.2k. 470 R* between supply rail and the 8.2k/33k node; output 0.1 uF (some units 0.047 uF) from that node to 500k volume pot to ground. Feedback 100k from Q2's lower terminal (emitter side) back to Q1 base. Q2 lower terminal goes to 1k "fuzz" pot to ground with wiper to 20 uF (+) shunt.
- Drawing caveat: the output cap in this redraw is shown taken from the supply side of the 8.2k resistor; on a real Fuzz Face the output is at Q2 collector. Treat the terminal naming (collector/emitter) of Q2 as uncertain (?). 

### B2. p113 - Jimi Hendrix Fuzz Face by Jim Dunlop (Model JH-2), 9 V
- Q1, Q2: MPSA18 (NPN silicon). Supply: 9 V battery -> 470 R -> 9.1 V zener 1N5239B (as drawn, probably on a 9 V rail it is acting as a ripple filter) with 100 uF 6 V (+) to ground. 
- Input: In -> 2.2 uF (+) -> 100 R -> Q1 base; 1 nF from base to ground; 180k from input node to ground. Q1: emitter ground; collector load 43k to rail; Q2 base directly from Q1 collector. Q2 collector load 10k; 330 R series from rail; output 0.01 uF -> 500k "level" pot to ground. Feedback 68k || 47 pF from Q2 emitter to Q1 base. Q2 emitter -> 1k "fuzz" pot to ground with 22 uF (+) to ground on the wiper.

### B3. p114 - Jordan Boss-Tone, 9 V
- Two NPN (2N2222 or 2N4124 or similar). Q1: collector-voltage-feedback single stage (560k collector-to-base, 560k + 18k bias network, 47 pF, 0.022 uF). Q2: buffer/second gain stage with 18k collector load. Output 0.022 uF; two 1N914/1N4148 diodes in antiparallel to ground after the output cap; 10k output pot. Input pot 100k then 0.022 uF, 150k base to ground.

### B4. p115 - JSH Fuzz (Model FZIII), 9 V
- Q1 BC238B (input buffer: 1M base bias, 10k emitter), 0.1 uF, 4.7k series, Q2 and Q3 BC239C. Q2 collector 33k to rail and direct-coupled to Q3 base. Q3 collector 6.8k to rail, 1n2 across; 150k feedback from Q3 emitter to Q2 base; 22 R in Q3 emitter then 1k fuzz pot with 22 uF (+) wiper cap. Output 0.1 uF; silicon diode pair (antiparallel) to ground; 50k log output pot.

### B5. p104 - "Distortion Booster", 9 V
- Q1, Q2 BC108, silicon or germanium diodes D1, D2 (antiparallel) to ground at the output. Same as p97 Muff Fuzz (early): input 0.01 uF, Q1 base 100k to ground, 100k feedback from Q2 emitter to Q1 base, Q1 collector load 100k, Q2 collector load 10k, Q2 emitter 3k3, output 0.1 uF. No volume pot.

### B6. p99 - Electra Distortion
- Single 2N3904 NPN, 9 V. Input 0.1 uF; 2.3M collector-to-base bias; collector load 4.7k; emitter 470 R; output 0.1 uF; TWO germanium diodes (antiparallel pair) shunt to ground at the output. Notes: germanium gives ~0.4 V signal and soft clipping; silicon gives ~1.4 V, more power and less distortion. Claimed to sound like a Tube Screamer. The diodes load the transistor and soften the clipping.

### B7. p103 - "Fuzz 001" (unknown commercial source), 9 V, 4558 dual op-amp
- Stage 1: input 0.1 uF, 100k, pin 6; feedback 100k. Bias: 1M (to +9 V) and 1M (to ground) with 10 uF (+) on the bias node (4.5 V).
- Stage 2: 10k series, 0.1 uF; feedback is a bundle: two 1n4xx diodes (labelled "1n42"?) antiparallel, 470 pF, 100k all in parallel. Output 0.1 uF -> 47k log pot.
- Same family as p124 op-amp fuzz but with gain-limited stage 1 (unity-ish) and 100k feedback in the clipper.

### B8. p112 - TS-9 Tube Screamer, 9 V (4558 dual op-amp)
- Input buffer: .02 uF + 1k -> 2SC1815 emitter follower (510k bias to 4.5 V, 10k emitter), then 1 uF (np) coupling, 10k bias to 4.5 V, 0.1 uF to a 4.7k + 0.047 uF network into the inverting input. Rail splitter: 10k/10k (62k/22k in the drawing's separate bypass block (?)), 47 uF, 100 uF decoupling, 10k.
- Clipping stage (op-amp 1, non-inverting): feedback = 51 pF || 2x 1N914 (antiparallel) || 51k + 500k "Drive" pot in series. Gain network: 4.7k + 0.047 uF to the 4.5 V bias node. 
- Tone/output stage (op-amp 2): 1k + 0.22 uF tant to 4.5 V at the filter input, then 20k "Tone" pot with 220 R + 0.22 uF tant, 1k feedback; output 1 uF (np) + 1k -> 100k "Level" pot.
- Output buffer: second 2SC1815 emitter follower, 510k/10k bias, 10 uF (+), 470 R, 100k.
- Switching: the drawing shows an electronic (FET) bypass: two 2SK30A JFETs, a 4-transistor flip-flop (56k, 1M, 1000 pF, 0.047 uF, 3 V zener, 3.6k, 100 R, 22 R, cr1/cr2 = 51 pF || 56k). Page note: resistors marked * (1M) might be 22k and 56k might be 10k. Transistors all 2SC1815; diodes silicon 1N914.
- Directly relevant as a screamer reference (op-amp clipper with 51k + 500k, 51 pF, 4.7k/0.047 uF, 1N914 pair; tone 20k + 220 R + 0.22 uF).

### B9. p125 - MXR Distortion+, 9 V, 741 op-amp (single supply 4.5 V bias)
- Bias: 1M/1M divider with 1 uF 25 V (+); input .001 uF to ground RF cap; .01 uF + 10k to + input (via the bias). Gain network at - input: .047 uF + 4.7k + 1M pot (gain pot, wired as variable resistor) to ground; feedback 1M (dotted, optional modification 8-10 pF for oscillation, and a 1M click-prevention resistor to ground from input).
- Output: 1 uF 35 V tant, 10k series, antiparallel diodes D1, D2 to ground (originally germanium 1N270; alternates 1N34, 1N4148, LEDs, or 1N34 array), .001 uF to ground, then 10k output pot. Bypass S1 (not true bypass) with LED on 2.2k.

### B10. p126 - MXR Hot Tubes Distortion, 9 V
- Dual op-amp (741 or 4558) buffer + gain stage then three CD4049 inverter stages used as clipping gain stages (Craig Anderton "Tube Sound Fuzz" cousin).
- Front: In -> 0.1 uF, 1M to ground; 120k; 47 pF; op-amp 1 buffer; op-amp 2: 20k + 20k, 75k || 22 pF feedback, 1 uF (+); supply: 9 V with 30 uF, 220k/220k divider with 1 uF to make a bias rail. 
- Inverters: stage 1 100k (?) pot-adjusted bias via 0.1 uF/8k2 in series, feedback 150k || 22 pF, 1M5 bias to ground; coupling 0.015 uF; stage 2 220k feedback, 2M2 bias; coupling 0.22 uF + 470k; stage 3 1M feedback; coupling 4.7 uF (+) + 15k; output pot "unknown 10K?". Page says two of the inverter stages have a DC offset in their bias points. Values marked unknown on page: 100k? pot (drive) and 10K? (level).

### B11. p123 - Modified Tube Sound Fuzz (M. Hammer; after Anderton), 9 V
- Op-amp buffer/gain (any low-noise dual) + 2 stages of CD4049 inverters + tone and output. Input 0.1 uF, 470k to ground; 2k2 + 10 uF (+); 47k || 510 pF*; 1500 pF* bright; 10k pot; 100k (stage input) and 100k feedback with 390 pF* each across the inverter stages; 0.1 uF interstage; dual-ganged 10k tone (100k, 3k9, 100k, 3k9, 4700 pF x2); output 10 uF (+) + 10k pot. *= suggested values, substitutions within 30%.

### B12. p121 - Mosrite Fuzz-Rite, 9 V (PNP? the drawing has + battery to rail, "TZ82" germanium)
- Two TZ82 transistors (germanium), 470k, .05 uF, .002 uF, 350k pot, 33k pot (fuzz/volume), 470k collector loads. Two-stage direct-ish fuzz with 0.05 uF input, .002 uF coupling between stages, 470k feedback resistors from each collector to base, 350k and 33k pots. Very simple "Tone Bender-era" germanium fuzz.

### B13. p118 - Maestro Fuzz (Fuzz-Tone; "Satisfaction"), 9 V
- Transistors and diode are Gibson house-numbered (991-002298 transistors; 919-004799 diode), probably germanium. Impedance matcher stage (33k, 470k, 0.01 uF, 0.002 uF, 3.3k), fuzz preamp stage (8.2k, 0.1 uF, 3.3k, 470k, 0.001 uF), fuzz amp with 100k, 0.047 uF, 47k, 330k, 470k, 0.0047 uF, 18k, 3.3M; squelcher (diode, 100k, 0.1 uF), panned 50k pots (fuzz amount), 33k resistors, 50k output pot. 3.3M and 100k shown near the squelcher transistor.

### B14. p134 and p135 - ProCo Rat Distortion (two versions drawn)
Common: LM308 op-amp, 9 V (adapter or battery) with 1N4001 reverse-protection, 4.5 V virtual ground from a divider, 1N4148 diode pair (hard clipping to ground after the op-amp, with 1 uF/4.7 uF coupling), 1.5k + 0.0033 uF + 100k log "Filter" pot (post-clipper low-pass), JFET output buffer + 100k log "Volume" pot.
- p134 version: input JFET buffer BF245A (1M to ground, .022 uF + 1k in front, source 1k with 1 uF (+) bootstrap), 100k + 1k + 1 nF at the + input (bias 4.5 V); op-amp gain network: 560 R + 4.7 uF (+) and 47 R + 2.2 uF (+) to ground from - input; feedback 150k log "Distortion" pot in parallel with 100 pF; compensation 30 pF; 1k + 4.7 uF (35 V) after the op-amp; diodes; 100k log filter pot + 1.5k + 0.0033 uF; output stage .022 uF + 1M + BF245A source follower (1k, 10 uF (+), 10k, 100k log volume). Supply: 47 R + 100 uF, 33k/33k divider with 47 uF, 0.001 uF decoupling.
- p135 "Type B Rev 7-81": no input JFET; bias at + input via 1M from 4.5 V; 0.022 uF + 1k input; 0.001 uF at input; same gain network (560 R + 4.7 uF 35 V, 47 R + 4.7 uF 35 V); feedback 100k log pot || 100 pF; 30 pF; op-amp output 1k, 4.7 uF (35 V), 1N4148 pair, 100k log filter + 1.5k + 0.0033 uF; output: 2N5458 (JFET) buffer with .022 uF + 1M bias, 10k source, 1 uF (50 V), 100k log volume. Supply: 100 R + 100 uF (16 V), 100k/100k divider with 1 uF 50 V, 0.001 uF. Current drain ~0.6 mA idle, ~1.6 mA full output.
- These are real ProCo Rat circuits (well-known) and the main useful item in the range for a Rat request.

### B15. p140 - Marshall Shredmaster (Dec 19 1994 drawing), sheet 1 of 4
- Image at 125 dpi is too small to read component values (only a few designators legible: 1N4148 clipping diodes, TL072 op-amps, BASS/TREB pots, 4.7M/12k/22nF etc., all (?)). Shows battery input, two op-amp stages, 1N4148 diode pair clipping (D8/D9), bass/treble/contour/gain/volume controls (labels). Not usable without a better rendering of this page.

### B16. p101 - Fender Blender (octave fuzz / distortion with octaving), 9 V
- Q1, Q2 2N3391A; Q3, Q4, Q5 2N3391; D1-D4 1N276 (germanium). Input 15k + 0.1 uF; Q1 emitter follower (47k, 1k5, 150k bias); Q2 gain stage (120k, 10k, 0.05 uF, 560k); blend pot 10k with 680 R; Q3 phase splitter (220k, 150k, 8k2 x2, 27k, 10 uF x2) feeding D1/D2 full-wave rectifier; Q4 clipper stage (100k, 10k, 27k, 820, 10 uF) with D3/D4 clipping; passive tone (47k, 27k, 125k tone pot, 3k9, 0.003 uF, 0.1 uF, 100k); Q5 output stage (150k, 10k, 18k, 820, 3k9, 0.001 uF, 10 uF x3, 50k pot); sustain/volume 100k pot, 0.1 uF out. Octave-up via rectification (blend control), rest is distortion.

### B17. p106 - "Guitar Effects Unit" Octaver-Fuzz (ETI Canada, Jan 1980), +/-9 V (two 9 V batteries)
- Q1 MPS6515 NPN input stage (680k base bias, 560 pF, 6.8k collector, 270 R emitter, 1 uF in/out). IC = 4558 dual: stage (a) inverting gain 10k/10k + 1M trim pot, 100k, 8.2k; precision full-wave rectifier with D1, D2 (1N4148) feeding stage (b) 10k/10k; fuzz via 39k + 820 R divider; "Fuzz" or "Struzz" (fuzz + octave) mix switch S3; 1k/1k divider. Fully rectified signal for octave-up fuzz.

### B18. p102 - Foxx Fuzz-Wah (Foxx; includes fuzz, octave (may copy Octavia), wah, later volume pedal)
- All transistors 2N3565-R249 (NPN silicon), all diodes germanium, electrolytics 10 uF 16 V. Fuzz and octave: 47k, 100k, 4.7k, 470k, 10k, 150k, 3300 pF, 1000 pF, 0.1 film, 50k tone and drive pots, 22k, 15k, 1k. Wah: 0.5 H inductor "5103 TDK", 68k, 33k, 1.5k, 100k, 22k, 470k x2, 0.22/0.01/0.047/0.0033/0.0047/0.1 uF select caps, 220 R, 1k, 100k wah pot (log). Four wah centre frequencies via rotary switch. Fuzz/octave possibly Octavia-like (note: in Octavia the rectifier is in the gain stage; not verified).

---------------------------------------------------------------------
## C. MODULATION / PHASER / TREMOLO (non-EH)
---------------------------------------------------------------------

### C1. p127 - Oberheim PS-1 Phase Shifter, +/-15 V
- Six identical all-pass stages (1458 dual op-amps; "all bipolars 2N3638A"): each stage = 100k input resistor, op-amp inverting, 100k feedback, 0.01 uF and 100k to ground at the + input with a 2N4302 JFET (selected, marked *) as the variable resistor to ground from the + input node. 
- Input: 0.33 uF + 10k -> 2N3638A buffer (33k bias) ; mix: 4.7k resistors from stage taps into the output stage, 1M summing resistors into two 2N4302 FET switches (bypass/mix), 4.7k/10k output. DC offset trim 10k (-15/+15) with 1M; "Off/Slow Phase" switch; click minimisation.
- LFO: 2N3638A with 100k/4.7k/1M/220k/10k RC timing selecting Medium and Fast speeds (220 uF (+), 4.7 uF), integrator op-amp stage with 0.33 uF, 150k/150k, 2N4302 and P1069C FETs, 10k, 33k, 1M -> triangle -> Osc. Ampl. trim (10k) -> 33k/33k -> FET Bias (10k trim) -> gate lines of all six FETs (shared bus). 
- Power: transformer, 4x 1N4002 bridge, 2 x 470 uF 25 V, 2 x 270 R, 2 x 1N965 15 V zener, +/-15 V.
- Adjustment: set FET bias until phasing is mid-range with Osc. Ampl. at minimum, set depth, set output offset.
- 6 stages, FET-based allpass, 100k/0.01 uF per stage. Phase 90 has 4 stages, 10k FET channel resistances, 0.01 uF caps, 4.7k feedback summing - this is the same family but different (bigger, +/-15 V, 1458).

### C2. p116 - Kay Tremolo Model T-1, 9 V
- 2SC828R/2SC828P transistors: single input amplifier stage, twin-T oscillator driving two transistors that load the signal ("repeat percussion" Vox style). Twin-T: 33k, 33k, 100k, 47k, 43k, 68k, 10k x2, 470k, 4K7, 1K2, 1K, 33 R, caps 0.1 uF, 0.01 uF, 0.22 uF, 4.7 uF x2, 10 uF. No depth control; speed via treadle pot. 

### C3. p122 - PAiA Motion Filter / Follower (model 5720), 9 V
- Envelope-follower filter: IC1 4136 quad op-amp (envelope detector, 1N4148 D1/D2; state-variable filter with 0.01 uF x2 and 4066 quad switch IC2), 2N4124, many 1M/500k/10k/100k values, 0.1 uF, 1 uF, 47k, 3M9 (sensitivity), 150 R, 100pF; offset trim; Cancel switch S1.

### C4. p111 - Harmonic Sweetener, +/-15 V
- TL074 quad (3 of 4 used): clipping section (red LEDs suggested), two op-amps in a sub/octave filter (1M, 4n7 x4 in the filter, 4k7*, 10k, 22k, 15k, 10 pF, 100 R/10 uF supply decoupling). Mods: 4k7* to 10k dual-gang pot (tunable filter), 1M** to 2.5M pot (drive).

### C5. p100 - Ten Band Graphic Equaliser (Fabian P. Hartery drawing; from Encycl. of Electronic Circuits Vol 2 / ETI)
- Per band: 10k, 100k, 10k and 1M, 1M with C1/C2 pair (gyrator-like), common TL082 op-amp, 100k, 2.4k, 3.3 uF. Centre frequency table: 32 Hz C1 180 nF/C2 18 nF; 64 Hz 100n/10n; 125 Hz 47n/4.7n; 250 Hz 22n/2.2n; 500 Hz 12n/1.2n; 1 kHz 5.6n/560p; 2 kHz 2.7n/270p; 4 kHz 1.5n/150p; 8 kHz 680p/68p; 16 kHz 360p/36p.

---------------------------------------------------------------------
## D. ONE-LINE ENTRIES (not studied in depth)
---------------------------------------------------------------------
- p107 Green Ringer (Dan Armstrong, 9/23/95 drawing): octave-up ring-mod-like octave effect with 2SC828, 2SA666 x2 and an unmarked three-stripe (green/blue/white) transistor used as dual diodes; 9 V; 160k, 18k, 10k, 22k, 47k, 66k x2, 560k, 6k2, 0.047 uF x3, 0.1 uF, 100 uF.
- p119 Maestro Boomer 2 (wah/volume, model EG-2), p120 Maestro Boomerang (wah, EG-1): two-transistor inductor wah (500 mH inductor, house-numbered transistors, 9 V; values 10k, 820k/620k, 1 uF, 25k, 1.5M, 48k, 56k/47k, 120 R, .047 uF, .1 uF, 6 uF, 8.2k, 4.7k, .01 uF).
- p128 Olson New Sound: single 2SB175 (PNP germanium) fuzz/booster on one 1.5 V cell (0.003 uF x2, 10k x3, 10 ohm?, 1 uF x2, 100k, 1M x2, 15k, 5k, 47k).
- p129-132 Orange 125 Mk3 amplifier: tube amp, 2x preamp channels (ECC83 x ~4, ECC82 driver, 4x EL34 power section, 3x BA127 rectifiers (?), reverb driver, tremolo, B+ 454 V and 392 V, grid bias -100 V/-40 V). Gain structure: 330 pF, 68 nF, 1.8 V cathode bias, passive treble/mid/bass, volume 1M log. Not an effect pedal; out of scope here but potentially useful as a tube preamp reference (see Gaps).
- p133 Voltage Controlled Panner (C. G. Forrest Cook 1994): dual 2N3904 matched pair VCA-style gain cells, NE5532, LM1458, +/-15 V; 10k, 330 R, 100 pF.
- p136 "The Bomb": acoustic fuzz: PVC tube with crystal mic and telephone speaker and two op-amp amps; no schematic values.
- p137 two guitar buffer/preamp circuits: discrete JFET 2N5457 preamp (Don Tillman; 220k, 6.8k, 2.2k, 51k, 100k, 10M, 5 uF, 15 uF, 22 uF, 1 uF, 10 uF; ~0.5 mA, gain 2) and LT1012 micro-power op-amp buffer (1M x2, 250k, 50, 1 uF, 0.1 uF).
- p138 large unlabeled schematic (landscape, drawn rotated, "D D"-style title block unreadable at this resolution); appears to be a mixer/amp section with transistors and op-amps; designators and values not legible at 125 dpi. Unidentified.
- p139 Stage Center Reverb (Craig Anderton, Guitar Player 1976): spring-reverb driver/receiver with a quad op-amp (4136 pinout), 33k, 0.22 uF x3, 0.02 uF x2, 10k, 22k, 50k x2, 1M, 4.7k x2, 0.01 uF, 220 pF, 2.2M, 470k*, 47k, 1N4001 x2 supply bypass; cancel switch; C1/C2 10-100 uF.
- p141 Simple Mixer: TL072/TL082 inverting summing mixer, 100k x several, 10k, 1M, 0.1 uF, 22 uF, 2.2 uF, 9 V (bipolar via +/-9 V).

---------------------------------------------------------------------
## Gaps and relevance (vs StarterAmp requests)
---------------------------------------------------------------------

What is NOT in pages 95-141 (so must come from other sources / the other halves of the book):
- Big Muff Pi, any version: NOT present. The closest items are p117 Little Big Muff and p97/p124 early Muff Fuzz. None of the following exist here: Triangle, Ram's Head, Civil War, Russian, Op-amp Big Muff, NYC reissue, 560 pF 'modern' build, Small Clone/Smooth/Gritty/Mid-forward/Heavy. The user's version list (Standard, Smooth, Gritty, Mid-forward, Heavy, early large-box, op-amp build, modern 560 pF build) cannot be sourced from here.
  - p117 Little Big Muff is a legitimate "small-box" Big Muff relative (3 gain stages with 470k || 560 pF feedback, 2 diode-clipper stages, 2-position tone). The 560 pF capacitance in the feedback is notable (same value as the modern-560 pF build option), but note it is across 470k as in classic Muffs (Triangle used 470k || 560 pF? / Ram's Head 470k || 470 pF in many reissues) - verify before basing a build on it.
  - p124 is the closest thing to an "op-amp Muff build" in the range, but it is an op-amp fuzz (gain 4.7 -> diode clipper), without the Big Muff tone stack or the transistor-based clipping; it would be only a loose reference.
  - p97 early Muff Fuzz/Little Muff Pi (2x 2N5133 direct-coupled, antiparallel diodes at output) is possibly useful as an "early/simple Muff" variant but is not a Big Muff.
- Small Stone / Electric Mistress / Memory Man / Holy Grail / Screaming Bird (only the LPB-style treble booster appears) / Soul Food / Hot Tubes (the one here is the MXR Hot Tubes, not EH's) / Bass Balls / Ram's Head: Small Stone, Electric Mistress, Memory Man, Holy Grail, Soul Food and Bass Balls are absent. Mole/Hog's Foot (old and new) and Screaming Bird/Tree are present on p97 but as simplistic boosters. The CE-1 (chorus ensemble) is also absent.
- Germanium transistor circuits that are present: Fuzz Face (p105, AC128/NKT275), Mosrite Fuzz-Rite (TZ82), Maestro Fuzz (house-numbered), Fender Blender (2N3391 and 1N276 germanium diodes), Olson (2SB175), Electra distortion (germanium diodes), MXR Distortion+ (1N270 germanium diodes), Foxx (germanium diodes). Tone Bender (Vox/Sola), Fuzz Factory (Zvex) and Klon-style overdrives are NOT present.

Which of these circuits are distinct from the user's list (new material for the plugin):
- Fuzz Face (p105 PNP germanium, p113 Hendrix JH-2 NPN MPSA18 with zener-regulated supply): directly useful for a Fuzz Face-style model; both are in this range. Drawing quirks noted (output coupling point and Q2 terminal labelling) so check against known Fuzz Face topology before use. Tone Bender-style: the Fuzz-Rite (p121) is a 2-transistor germanium fuzz with feedback resistors, not a Tone Bender; Jordan Boss-Tone (p114) and JSH Fuzz (p115) are collector-feedback 2-3 transistor fuzzes in the same genre.
- Rat: p134 (with JFET input buffer, LM308, 1N4148 pair, 150k log distortion pot, 100 pF, 30 pF compensation, 1.5k + 0.0033 uF filter) and p135 (Type B Rev 7-81) are real ProCo Rat circuits. Good coverage for a Rat request, including two versions.
- Screamer: p112 TS-9 (4558, 51 pF, 51k + 500k, 4.7k/0.047 uF, 1N914 pair, 20k tone, 100k level). Direct coverage for a screamer. The Marshall Shredmaster (p140) is a different overdrive but is illegible. MXR Distortion+ (p125) and p99 Electra Distortion (single 2N3904 with germanium diode clipping) give DS-1/Dist+-style alternatives; the Boss DS-1 itself is not present (the closest is the Distortion+ op-amp + diode-to-ground architecture, and the Rat).
- Op-amp fuzz family: p98, p103, p124 (two-stage 4558/1458 fuzz with antiparallel diode feedback clipper) - candidate for a minimal "op-amp Muff-like" distortion build option but not a Big Muff.
- Phase 90-style phaser: p127 Oberheim PS-1 (6-stage, 2N4302 FET all-pass, +/-15 V, LFO values) is the only phaser; it differs from Phase 90 (4 stages, 10k/15k...). Useful as a reference for FET all-pass phaser modeling, not a Phase 90 clone.
- Juno-style chorus / CE-1 / Small Stone: not present at all. (p95 delay uses companding and BBD-style/digital delay, not chorus.)
- Tube preamp references (p129-132 Orange 125 Mk3, ECC83 stages with 220k/100k/1M, 68 nF coupling, 1.8 V cathode bias, 392 V/454 V rails): not effects but could be a reference for tube preamp gain stages in the amp part of StarterAmp, if needed.
- Octave: Fender Blender (p101), ETI Octaver-Fuzz (p106), Green Ringer (p107), Foxx Octavia-like (p102), EH Micro Synth (p108-110, mostly irrelevant). A rectifier-type "octave-fuzz" is represented by p106 (op-amp precision rectifier + fuzz).
- Compressor/reverb/delay: no compressor in range. Spring reverb driver (p139) and the 16 s digital delay (p95-96) are present but distinct from amp-sim needs.

Illegible or unresolved items: p138 (unknown large schematic), p140 (Shredmaster), p108-110 (EH Micro Synth) have too many components to transcribe at 125 dpi; the Hog's Foot new (p97) op-amp node connections are uncertain; the Fuzz Face drawing quirk on p105/p113; p128 "10" value (10 ohm?) and p116 values are as read by OCR/visual and should be treated as approximate.
