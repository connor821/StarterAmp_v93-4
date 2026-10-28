# Mesa/Boogie Mark IV (11-page set, files MK4BLOCK/PRE1-3/EQMK4/PWR/SW1-2, FU-3 footswitch)

Source images: `mk4-01..11.png`. Page 1 is a title card, page 2 the block diagram (1/10), pages 3-5 the preamp (sections 1-3), page 6 the graphic EQ, page 7 the power amp, page 8 power supplies, page 9 and 10 the LDR switching matrix, page 11 the FU-3 footswitch. All pages are clean, so nearly every value below is legible. Values marked (?) are not. Computed figures come from my own nodal solver of the plugin's tone-stack topology (taper not printed, so pots at linear position) and simple RC formulas; they are estimates.

Notes common to all pages: "Voltages measured no signal, 117 V AC mains, DC voltages may vary 20 %, resistors 1/2 W unless noted."

## 1. Tube complement

| Tube | Type | Stage |
|---|---|---|
| V1A | 12AX7 | Input stage (all channels) |
| V1B | 12AX7 | Second stage after the tone stack and gain pots |
| V2A | 12AX7 | Third stage (rhythm 1 / rhythm 2 output; also receives lead) |
| V2B | 12AX7 | Output / recovery stage (master and loop recovery), feeds the graphic EQ and output level |
| V3A, V3B | 12AX7 | Lead drive and lead stage (cascaded) |
| V4A, V4B | 12AT7 | Reverb driver and recovery |
| V5A, V5B | 12AX7A / 7025 | Phase inverter pair |
| V6, V7 | 5L6 (printed "5L6"; 6L6 family) | Output pair 1 (power amp, always on) |
| V8, V9 | 5L6 or 6CA7/EL34 | Output pair 2 (the pair switched Tri/Pent and Simul-Class) |
| Rectification | 1N4007 silicon diodes | |

Block diagram (page 2): Input -> V1A -> tone controls -> {R1 gain, R2 gain, LD gain} -> V1B -> (lead path: LD drive -> V3A -> V3B) -> V2A -> reverb tap (V4A -> reverb pan -> V4B) -> FX loop (normal or switchable) -> V2B -> output level -> graphic EQ -> power amp with presence control. Record out and slave out are taken from the EQ/power sections. This is a three-mode amp (Rhythm 1, Rhythm 2, Lead) with a footswitchable graphic EQ and an FX loop, all switched by LDRs (opto-isolators), see section 8.

## 2. Preamp, section 1 (page 3)

Supply markings: D rail 370 V (printed "370v", and "378v" appears on page 8), C rail 385 V.

### V1A (input stage)
- 12AX7, plate load R201 150K to D (370 V); plate 210 V printed.
- Grid: R391 1M to ground; input jack straight to grid (no grid stopper drawn).
- Cathode: R251 1.5K parallel with C321 0.47 uF (cathode 1.6 V printed). A second bypass C331 2.2 uF in series with R222 22K can be switched in (LDR7, R2): "R2 low boost on V1A" (LDR function 7). Bypass corner for 1.5K / 0.47 uF is about 226 Hz (1/(2 pi R C), ignoring rp). With the switched 22K / 2.2 uF path added, the cathode impedance below the corner only falls by roughly 6 % (22K is in parallel with 1.5K), so the boost is mild; estimated by hand, (?) not simulated.

### Tone stack and gain pots (after V1A)
Three stacks selected by LDRs: R1 (Rhythm 1), R2 (Rhythm 2 shares the bass/mid with R1 but has its own treble pot, see below) and LD (Lead). Printed values:

Rhythm 1 stack (lower, around R1 TRBL):
- R212 100K slope resistor (plate side), treble cap C12 250 pF, bass/mid caps C241 0.1 uF (treble bottom to bass top) and C231 0.047 uF (to mid), pots: treble 250K, bass 250K, mid 10K.
- C32 750 pF switched in parallel with C12 by LDR3 (the "R2 Fat" contact): the "fat" version of the treble cap is 1000 pF in total. R352 10M is a DC bleed across the switch.
- After the stack, R1 GAIN 1M, with R264 470K to ground and a 120 pF bright cap across the upper part of the gain pot (switchable by an LDR); "60K" is drawn in series in the R2 TRBL branch to the 0.1 uF cap.
- R2 has its own treble pot R2 TRBL 250K (LDR2 "R2 treble control on"), shared bass and mid pots, grounded through LDR6 ("R2 tone controls grnd") and RV1 (R1) for R1; R2 GAIN 1M.

Lead stack (upper): R211 100K slope resistor, C11 250 pF, C31 750 pF (LDR1 "Lead Fat"; R351 10M bleed), C242 0.1 uF, C232 0.047 uF, treble 250K, bass 250K, mid 10K, and GAIN LD 1M (switched with LDR4 "Lead gain on").

Computed results (my solver, source about 44K = R201 150K in parallel with about 62K tube rp, load about 1.1M):
- Stack without Fat (250 pF): noon insertion loss about -18.5 dB (mean 100-5 kHz) and -19.0 dB at 1 kHz; notch about -26.6 dB near 400 Hz; with everything at 10 the minimum is -23.5 dB near 280 Hz and the maximum -4.4 dB.
- Stack with Fat (1000 pF): noon -14.1 dB mean and -11.5 dB at 1 kHz, notch -20.8 dB near 200 Hz; all-10 minimum -19.3 dB near 140 Hz. So the Fat switch moves the 1 kHz noon level by about 7.5 dB and the notch down an octave, a noticeable voicing change.
- The plugin's tone stack type 11 ("Mark IV bright") uses 1000 pF, i.e. the Fat state, not the standard state. See `Amps_batch2_vs_code.md`.

### V1B
- 12AX7, plate load R213 100K to D, plate 255 V; cathode R262 1.5K parallel with C343 15 uF (cathode 1.8 V printed); bypass corner about 7 Hz.
- Grid sees the output of the gain pots (R1/R2/LD).
- Output coupling C233 0.047 uF into V2A. Grid network at V2A: R214 100K to ground; R361 680K and R362 680K switched by LDR9 (L) and RV2 (R1, L) to the "lead drive" node; R102 3.3M in parallel with C2 20 pF around the coupling node from the lead circuit; C1 47 pF to ground. High-pass corner of C233 into about 100K: about 34 Hz.

### V2A
- 12AX7, plate load R216 100K to C (385 V), plate 257 V; cathode R253 1.6K (reads "1.6K", possibly 1.5K, (?)) with C344 15 uF, plus selectable C332 2.2 uF (RV1, R2/L) and R223 22K / RV2 (R2) alternate networks (the "mid gain" switch). Cathode 2.0 V.
- Output coupling C234 0.047 uF.

## 3. Preamp, section 2 (page 4): reverb and lead stages

### Lead stages (lower half)
- V3A 12AX7: plate R282 82K to C (385 V), plate 265 V; cathode R255 1.5K parallel with C333 2.2 uF (cathode 2 V), corner about 48 Hz; grid fed from the LD DRIVE 1M pot (LDR8 "lead drive on") with R261 470K to ground and C3 120 pF across; this is where the "from 680K at LDR 8" input enters.
- Coupling V3A to V3B: R275 270K in series with C222 0.02 uF into R105 68K to ground with C4 0.001 uF (1 nF) across it. Corner of 0.02 uF into 68K about 117 Hz (plus the 270K series, so about 86 Hz total, (?) by hand), and a divider: 68K / (68K + 270K) = about -14 dB (so V3A to V3B is heavily attenuated; a cold clipper or mild "gain-staging" attenuator as in other high-gain amps). The 1 nF cap rolls off lows and highs of the signal that reaches V3B (corner 1 / (2 pi (270K parallel 68K) 1 nF) about 3.0 kHz).
- V3B 12AX7: plate R276 270K to C with C23 500 pF across it (a plate bright/bypass), plate 220 V; cathode R234 3.3K parallel with C311 0.22 uF (cathode 2 V), corner about 220 Hz; LD BRT switch adds R226 22K in series with a bypass to ground ("LD BRT"). Output C236 0.047 uF into R216 100K parallel with C21 500 pF to ground; R323 220K parallel with C14 250 pF bridges the output coupling (a bright network) to the "to LDR 10 (11, 12)" Lead Output node.
- No clipping diodes anywhere; lead gain is all tube saturation, with V3A feeding V3B through the 14 dB divider.

### Reverb (upper half)
- V4A 12AT7 driver: grid R262 470K from the reverb input node and R325 220K to ground (both drawn); plate to a reverb transformer primary fed from B (448 V) through R403 22K/2 W, plate 320 V; cathode R331 820 parallel with C322 0.47 uF (4.5 V printed); C5 0.002 uF across the transformer.
- V4B 12AT7 recovery: grid R324 220K to ground; plate R111 150K/1 W to C (385 V), plate 80 V; cathode R106 330 parallel with C346 15 uF (1 V); output C213 0.005 uF to the REV LEVEL 100K pot.
- Reverb tank: three-spring (the drawing shows three parallel coil sections).

## 4. Preamp, section 3 (page 5): V2B recovery, master and FX loop
- V2B 12AX7: plate R103 120K to C (385 V), plate 215 V; cathode R211 1K parallel with C345 15 uF (1.5 V printed). Feedback from the plate to the output level pot: R224 22K in series with C235 0.047 uF to the OUTPUT LEVEL 250K pot (this is the signal to the EQ-in, since output goes "TO EQ IN").
- V2B grid: R371 47K to ground; LOOP SELECT switch chooses normal or switchable loop; R107 5.6K at the send. LDR17 (FX return on), LDR18 (FX send on), LDR19 (FX bypass on).
- Master section: R372 47K from V2A (via C234 0.047 uF), R203 150K, R233 3.3K, R241 4.7K shunt (LDR 11x, R2 FX shunt) and three master pots, "R1 MSTR 100K", "R2 MSTR 100K", "LD MSTR 100K", switched by LDR16/LDR14/LDR13 respectively. These form the channel master voltage dividers into V2B.

## 5. Graphic EQ (page 6)
Five-band passive-inductor EQ around a transistor amplifier (Q1 MPSA63, Q2 MPSA70, Q3 MPSA70, Q4 MPSA20-5).
- Bands: 80 Hz, 240 Hz, 750 Hz, 2200 Hz, 6600 Hz; each a 50K pot into a series RLC: 80 Hz: 470 ohm, 1.0 H, 3.3 uF; 240 Hz: 470 ohm, 0.39 H, 0.47 uF; 750 Hz: 470 ohm, 0.22 H, 0.22 uF; 2200 Hz: 1K, 0.068 H, 0.15 uF; 6600 Hz: 1K, 0.033 H, 0.033 uF. R373 47K to ground via LDR16 ("graphic EQ on") from the bottom rail.
- Amplifier: C51 0.1 uF input, R263 470K, R392 1 meg, R220 22K, R236 3.3K, R235 3.3K, R229 22K, R230 3.3K, R237 3.3K, C41 10 pF, C42 10 pF, output C411 1 uF/50 V; supply "EQ SUPPLY" -30 V with C412 10 uF/63 V.
- The EQ has no inductor bypass when off: LDR16 simply removes the bottom ground so the sliders go flat.

## 6. Power amp (page 7)
- Phase inverter: V5A and V5B (12AX7A / 7025). Printed: R281 82K (V5B plate, 385 V), R101 91K (V5A plate, 385 V), R217 100K, R202 150K, R227 22K, R343 470, R226 22K, R231 330 parallel with C251 0.047 uF/200 V, R254 150, C245 0.1/400 V, C52 0.1 uF, C13 250 pF between the two plates, coupling C243 0.1/400 V and C244 0.1/400 V to the power tube grids. The exact topology (a long-tail-style paraphase with the V5A grid driven from the V5B cathode network) is (?) because several nodes are routed behind each other. "Pull silent" switch shorts the signal to ground.
- Output tubes: four octal tubes: V6, V7 (5L6 per the legend "V6,V7 = 5L6", i.e. 6L6 class) and V8, V9 (5L6 or 6CA7/EL34 per "V8,V9 = 5L6 or 6CA7/EL34").
  - V6 / V7 screens: R 470 ohm/2 W from B (448 V) to pin 4.
  - V8 / V9 screens: R 2.7K/2 W with a TRI./PENT. switch: triode puts the screen on the plate (pin 3), pentode through 2.7K to B.
  - Grids: pin 5 driven from the PI via 0.1 uF coupling caps; V6/V7 grid leaks R321 and R322 220K each to the -60 V bias line (E); V8 and V9 grids are connected through a further 220K to the V6/V7 grid node and 2.2M to ground each (so V8/V9 bias = -60 V x 2.2M / (2.2M + 0.22M) about -54.5 V, computed by me, and the signal on V8/V9 is the V6/V7 signal times about 0.91).
  - "CLASS A / SIMUL-CLASS" switch: ties the cathodes of V6/V7 to ground (Simul-Class: all four tubes conduct, V6/V7 biased as Class AB with the others) or lets them float. In Class A operation (switch open) V6/V7 are cut off by their floating cathodes. Which position is which on the drawing: the switch is drawn between the V6/V7 cathode node and ground (?) polarity.
- Rails: A = 460 V (OT center tap, plate voltage), B = 448 V (screens), C = 385 V, D = 370 V (printed "378v" on page 8), E = -60 V bias, F = -30 V EQ.
- Output transformer: 8 ohm and 4 ohm taps, a FEEDBACK tap, a slave-out tap through a "slave level" pot on page 2, and a "Direct Out" tap through R 6.8K with a 10K direct level pot. Output transformer is the same for 4 tubes.
- NFB and presence: feedback comes from the OT secondary through C 0.1 uF into three switched presence pots (R1, R2, LD: 250K each, LDR22 / LDR21 / LDR20), with a 680 pF cap in the R2 leg (LDR21) and a 0.001 uF cap in the LD leg (LDR20). Then R232 330 ohm parallel with C22 500 pF in series with R104 56K parallel with C211 0.005 uF, into the V5A cathode node (R231 330 ohm parallel with C251 0.047 uF, R254 150 ohm to ground). Estimate of feedback ratio at the injection node: about 480 ohm / (480 ohm + 56K + presence R + 330 ohm): -41 dB at presence = 0, about -35 dB at 1 kHz because 56K is shunted by 0.005 uF; maximum presence (250K in series) lowers it further to about -50 dB. Plus tube-to-OT gain this makes a light feedback amp (estimated; node assignment (?)).
- Direct out: 6.8K, 10K direct level.

## 7. Supply (page 8)
- Mains: 117 V AC, 3 A slo-blo, MOV 130 V AC, export primary with 1.5 A slo-blo and taps (0, 100, 117, 220, 230, 240 V). Transformer #561136 (domestic), #561137 (export). Fan on the primary.
- HV: red 350 V AC secondary into 1N4007 D1 to D4 (bridge-like with the center tap), reservoir two 220 uF/300 V in series (about 110 uF/600 V in total) with 150K 1 W balancing resistors (that differs from the "220 uF reservoir" comment in the code), standby switch, choke, rail A 460 V at the reservoir side, B 448 V after the choke with 30 uF/500 V, then 5.6K/2 W to C 385 V (30 uF/500 V), 1K/2 W to D 370-378 V (30 uF/500 V). A 0.1 uF/600 V cap shown at rail A.
- Bias: blue 50 V AC through D14 and D15 (1N4007) with R345 470, R332 820, C432 220 uF/63 V, C421 47 uF/100 V, R256 1.5K, R239 3.3K, C431/C422, R381 82K, R382 47K ("omit export") giving E -60 V bias and F -30 V EQ supply.
- Low voltage: 6.3 V AC heater winding (V2-V5 and V6-V9 separated, "V1: 12 VDC pin 5, GRND pin 4" i.e. V1 heater on DC), R381/R382 100 ohm in the heater feeds, C441 to C443 8200 uF/16 V (printed "8200uf/16v"), VR12 (12.6 V), VR5 (5 V), C342 15 uF, D7/D8/D9 1N4007 and 1N5392 x2, 15 V to FTSW, 100 ohm R110.

## 8. LDR switching (pages 9, 10) and footswitch (page 11)
LDR function legend (page 9, 22 LDRs):
1 Lead Fat; 2 R2 Treble control on; 3 R2 Fat; 4 Lead gain on; 5 R1 gain on; 6 R2 tone controls ground; 7 R2 low boost on V1A; 8 Lead drive on; 9 R2 gain on; 10 Lead output on; 11 Lead output shunt R2; 11x R2 FX shunt (4.7K); 12 Lead output shunt R1; 13 Lead master on; 14 R2 master on; 15 R1 master on; 16 Graphic EQ on; 17 FX return on; 18 FX send on; 19 FX bypass on; 20 LD presence; 21 R2 presence; 22 R1 presence.
Each LDR is an LED lit through a 39 ohm resistor from a +12.6 V rail (R110 100 ohm/1 W, C341 15 uF/35 V). Mode select, 74LS138 decoder, 74LS14 hex Schmitt inverter, Q5 to Q8 transistors with 274K bases, diodes D1 to D6, relays 1 and 2; FU-3 footswitch uses a 74C922 keyboard encoder, 7805 regulator, LEDs and 330 ohm resistors. Wire color code: R1 yellow, R2 orange, LD red.

## 9. Gain structure
- Rhythm 1 and Rhythm 2: V1A -> stack -> V1B -> V2A -> (reverb tap) -> V2B -> master (R1 MSTR / R2 MSTR) -> graphic EQ -> power amp. Three gain stages before the master (V1A, V1B, V2A) plus V2B recovery; R2 is the hotter mode (more gain and the low-boost cathode network, tone controls grounded by LDR6).
- Lead: V1A -> stack (Lead Fat selectable) -> V1B -> LD DRIVE -> V3A -> V3B (through the -14 dB 270K / 68K divider) -> V2A -> V2B -> LD MSTR. This is the cascaded high-gain voicing; there are no clipping diodes.

## 10. Illegible or uncertain
- V2A cathode R (1.6K vs 1.5K), the exact PI node assignment on page 7, the polarity of the Class A / Simul-Class switch, rail D (370 vs 378 V), the R2 treble pot / "60K" / 120 pF wiring on page 3, and the exact LDR matrix polarity (pages 9-10).
