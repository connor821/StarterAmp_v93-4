# Revision comparison: Vibroverb, Vibrolux Reverb, Princeton Reverb

Built from the individual read files in this folder. Every value carries the same confidence caveats as its source file; "(?)" = uncertain read. "Effect" tags: GAIN, COMP (compression/sag/headroom), TONE.
Source files: Vibroverb_6G16.md, Vibroverb_AA763.md, Vibroverb_AB763.md, Vibroverb_63Reissue.md, Vibroverb_64Custom.md; VibroluxReverb_AB568.md, VibroluxReverb_AA270.md; PrincetonReverb_AA1164.md, PrincetonReverb_AB1270.md.

---
## A. VIBROVERB: 6G16 -> AA763 -> AB763 -> '63 reissue -> '64 Custom reissue

### A1. Summary table (key values)
| Item | 6G16 (brown) | AA763 | AB763 (black) | '63 RI | '64 Custom RI |
|---|---|---|---|---|---|
| Preamp tubes | 7025 x3 (+PI 7025) | 7025 x3 | 7025 x3 | 12AX7A | 12AX7A |
| Reverb driver | 12AX7 (halves paralleled) | 12AT7 | 12AT7 | 12AT7 | 12AT7 |
| Phase inverter tube | 7025 | 12AT7 | 12AT7 | 12AX7A | 12AT7 |
| Rectifier | GZ34 | GZ34 | GZ34 | solid-state bridge | 5AR4 or solid-state (switch) |
| HV secondary (AC, each side) | 380 | 355 | 355 / 365 (?) | not printed | not printed |
| B+ at OT / plates | +440 V | +460 V | +460 V | +487 V CT, +436 V screens, plates +485 V | +455 V (ZV); TP1 +457 V |
| Screen R (each) | 470 ohm 1 W | 470 ohm 1 W | 470 ohm 1 W | 470 ohm 1 W | 470 ohm 1 W |
| Grid leaks (each) | 220K 5 % | 220K 5 % | 220K 5 % | 220K | 220K |
| Grid stoppers | none drawn | none drawn | 1500 ohm | none drawn | 1.5K |
| Bias (printed) | -62 V (?) | -55 V | -55 V | -59.2 V (TP32) | -60.2 V supply, -50 V set at TP36 |
| Cathode resistor | none (direct) | none | none | none | 1 ohm fusible (?) |
| Bias supply | 1K-1W trim, diode, 25 uF/50 V, 33K | 470 ohm 1 W, diode, 10K-L + 27K, 25 uF/50 V | same as AA763 | CR5 1N4003, 100 uF/100 V, R58 1K, 22 uF/63 V | 470 ohm 1 W, 1N4003, 100 uF/100 V, 2.4K, 10K trim + 27K |
| Global NFB | 10K to PI left bus (tail bus 4700 + 6800) | 820 ohm to bus (47 ohm to ground) | 820 ohm, 47 ohm | 10K to bus (R42 ~470 ohm to ground) | 820 ohm, 100 ohm to ground |
| PI tail | 6800 (bus->J), 820 (J->cathodes), +19 V cathodes | 27K (?), 470, +118 V | 22K, 470, +118 V | 6.8K, 820, +17.4 V | 22K, 470, +93.6 V |
| PI plate loads (top/bottom) | 82K / 100K (?) | 100K / 100K | 82K / 100K | 82K / 100K | 82K / 100K |
| Output speakers | two (2 jacks) | one + EXT | one + EXT | two 8 ohm | one 15" 8 ohm + EXT |
| Filter caps | 16 uF/450 V x5 | 20 uF/525 V x5 | 2x 70 uF/350 V (series, 220K equalisers) + 3x 20 uF/525 V | 22 uF/500 V x5 | 2x 220 uF/285 V series + 2x 47 uF/350 V series + 22 uF/500 V x2 |
| Dropping R after choke | B->C 10K, C->D 10K-1W | 1K (B->C), 4700 (C->D) | 1K, 4700 | 10K 1 W (Z->Y), 10K 1 W (Y->X) | 1K 1 W (ZV->YV), 4.7K 1 W (YV->XV) |
| Rail voltages | B +440, PI +250/260 | B +460, C +450, D +410 | same | Z +436, Y +358, X +302 | ZV +455, YV +443, XV +405 |

### A2. Preamp and tone, value by value
1. **Input network**: 6G16, AA763, AB763, '63 RI: 68K / 68K / 1M. '64 Custom: 100K / 100K / 1M. Effect: GAIN (~1 dB less source loading; slightly less Miller-capacitance high-frequency loss at the input, TONE).
2. **First-stage cathode**: 1500 ohm in every version. Bypass 25 uF (6G16, AA763, AB763) -> 22 uF/25 V ('63 RI, '64). Effect: negligible (low-frequency corner moves from 4.2 Hz to 4.8 Hz).
3. **Normal-channel stage-2 plate load**: 6G16 220K (to rail) -> 100K (AA763, AB763, '63 RI, '64). Effect: GAIN (lower mu-loading; ~3-4 dB less gain in the Normal channel), and TONE (less gain-related distortion in the stage).
4. **Bright/Vibrato-channel stage-2 plate load**: 6G16 22K + 82K series (junction feeds the reverb mixer path through .05 uF/470K) -> AA763/AB763 100K (single resistor, 270 V plate) -> '63 RI back to 22K + 82K (R22, R23) -> '64 Custom 100K. Effect: GAIN and reverb-feed level (the series resistor pair attenuates what the reverb driver and mixer see).
5. **Stage 1 plate voltage**: 6G16 +220 V; AA763/AB763 +280 V (normal) / +210 V (vibrato); '63 RI +284 V (?) / +282 V (?); '64 not printed. Effect: headroom of stage 1 (follows the B+ differences).
6. **Tone stack**: all versions use the same Fender "Blackface-type" stack topology: plate -> 100K -> stack node; top cap to treble pot; bass pot; 6.8K to ground.
   - Treble cap: 6G16 250 pF; AA763 250 pF (printed "230/250"); AB763 250 pF; '63 RI 330 pF (?); '64 250 pF. Effect: TONE (330 pF moves the treble corner lower than 250 pF).
   - Mid-series cap (stack node -> bass top): .1 uF in all.
   - Bass-bottom cap (stack node -> bass pot bottom): 6G16 .05 uF; AA763 .05 (?) normal, .033 vibrato (the two channels differ); AB763 .047 both; '63 RI .047; '64 Custom .022. Effect: TONE. The '64 Custom's .022 (about half) shifts the bass/mid response upward, which the schematic reader should model explicitly rather than assume .047.
   - Pots: all 250K treble, 250K bass, 1M volume. Taper letter: A (6G16, AA763, AB763, '63 RI), J as printed on '64 (same audio taper family). 6G16 and AA763 pots carry "10K TAP" / "2-35" codes whose meaning is not resolved.
   - Treble-pot / volume shunt cap (to ground): 6G16 about .005 (?); '63 RI about .0068 uF (?) (C5/C10); AA763, AB763, '64: none read. Effect: TONE (HF roll-off with treble turned down), (?).
7. **Bright cap**: 6G16 47 pF on the Bright channel only, fixed. AA763 and AB763: 120 pF on both channels, switched (BRIGHT switch). '63 RI: 47 pF/1 kV, bright channel only, fixed. '64 Custom: 120 pF with a switch (S3/S4) on both channels. Effect: TONE (and perceived gain at low volume).
8. **First-stage voltages**: cathode +1.8 V (6G16) vs +2.0 V (AA763) vs +1.49 V ('63 RI) vs +2.02 V ('64). Effect: small differences in operating point only; mostly measurement spread within +/-20 %.

### A3. Reverb and mixer
1. **Driver tube**: 12AX7 (6G16) -> 12AT7 (AA763 onward). Cathode: 1K 1 W + 25 uF (6G16) -> 2200 ohm + 25 uF (AA763, AB763, '63 RI, '64 22 uF). Effect: GAIN/drive (12AT7 drives the TR4 primary harder; more reverb drive at the same level). 
2. **Driver grid**: 500 pF from the second-stage plate in all versions; 1M grid leak. Same.
3. **Recovery**: cathode 1500 ohm + 25 uF (6G16, +1.5 V) -> 820 ohm + 25 uF (AA763, AB763, '63 RI, '64; V4A cathode shared with the mixer). Plate 100K, .003 uF to the REVERB pot (100K) in 6G16, AA763, AB763; '63 RI .0033 uF, REVERB pot 100K linear then R65 82K + R31 390K in the wiper path; '64 .0033 uF, 100K "HD" taper, R32 470K. Effect: GAIN of the recovery and the reverb return level; the 82K+390K ladder in '63 RI scales reverb return down.
4. **Mixer**: 6G16 has a mixer/return stage with 1M and 470K/.05 uF paths (wiring not fully traced); AA763/AB763 put 10 pF in parallel with 4.7M (AA763, (?)) / 1.5M (AB763, (?)) between the vibrato-channel output and the mixer grid, with 470K, 220K, .1 uF at the mixer plate; '63 RI uses R32 470K, R35 220K, R36 1M and .022 uF + 500 pF couplings; '64 Custom uses 3.3M || 10 pF, 470K, 220K, 10M bleeders (R77, R78) and the MOD switch S1. Effect: GAIN and dry/wet blend, and how much vibrato-channel signal reaches the PI directly.
5. **Reverb transformer / tank**: TR4 (125A20A/B) in 6G16, AA763, AB763; T2 037099 in the '63 RI and T1 0037099000 in the '64; ratio and tank impedance are not printed in any of them.

### A4. Vibrato / tremolo
1. **Type**: 6G16 = 12AX7 oscillator + follower with INTENSITY 250K-L injected into the bias line (bias modulation, no photocell). AA763 and AB763 = 12AX7 oscillator + driver for a neon/photocell pair, INTENSITY 50K-RA wired to the bias supply (bias modulation) - the photocell traced to the bias line in both (?). '63 RI = 12AX7A V5 oscillator + buffer (V5B) with INTENSITY 250K linear into the C- bias line (no opto; same idea as 6G16). '64 Custom = 12AX7A with opto-coupler OP1 (037836) and INTENSITY R48 50K, bias-rail modulation at -60.2 V with R79 2.4K.
2. **Speed**: 3M-RA + 100K in 6G16, AA763, AB763; '64 R42 3M CCWA + R41 100K. 
3. **Oscillator cathode**: 4700 ohm (6G16, +2.0 V) -> 2700 ohm (AA763, AB763 +2.3 V, '64 +2.02 V). Effect: oscillation frequency range/intensity.
4. **Supply of the oscillator plate**: 6G16 470K to B (+440 V); AA763/AB763 220K to B (+460 V); '63 RI 470K via R64 100K (?); '64 R43 220K to ZV. Effect: modulation depth and wave shape.
5. **Modulator/driver cathode**: AA763 56K (+5.6 V), AB763 100K (+12 V (?)), '64 R46 100K (+13.2 V); 6G16's right triode has a 470K load (a follower, not a lamp driver). Effect: depth at maximum intensity.

### A5. Phase inverter and power amp
1. **PI tube**: 7025 (6G16) -> 12AT7 (AA763, AB763, '64) -> 12AX7A ('63 RI). Effect: GAIN (12AT7 has lower mu, more current, lower drive impedance); the '63 RI returns to a high-mu tube, so PI drive is bigger but more compressed.
2. **PI tail network**: 6G16 6800 / 820 / 4700 (to ground) with 10K NFB; AA763 27K (?) / 470 / 47 ohm with 820 ohm NFB; AB763 22K / 470 / 47 with 820 ohm NFB; '63 RI 6.8K / 820 / R42 (about 470 ohm (?)) with 10K NFB (R41); '64 22K / 470 / 100 ohm (R53) with 820 ohm NFB (R64). Effect: COMP and TONE. Feedback divider ratios (computed from the printed values, not printed on the drawings): AB763/AA763 (820+47)/47 = 18.4 (about 25 dB); '64 (820+100)/100 = 9.2 (about 19 dB), i.e. roughly 6 dB less feedback than AB763; 6G16 (10K+4700)/4700 = 3.1 (about 10 dB) and '63 RI (10K+470)/470 = 22 (if R42 is 470 ohm, (?)). The 6G16 therefore has far less feedback than AB763, the '64 half as much as AB763; the '63 RI value hinges on the uncertain R42 read.
3. **PI plate loads**: AA763 100K / 100K (both 5 %); other versions 82K / 100K. Effect: PI balance (82K/100K compensates for a lower-gain half).
4. **Grid stoppers**: AB763 and '64 add 1500 ohm in each 6L6 grid; 6G16, AA763, '63 RI none drawn. Effect: TONE/stability at high frequencies when the power tubes clip (slightly softer top-end blocking distortion); negligible gain change.
5. **Bias**: -62 V (6G16), -55 V (AA763, AB763), -59.2 V ('63 RI), -50 V target ('64 Custom, with 35-38 mA per tube check). Effect: COMP (more negative bias = colder idle, more crossover, earlier sag). Plate voltage: 440 / 460 / 460 / 485 / 455 V.
6. **Cathode protection (new in '64 Custom)**: R72/R73 about 1 ohm fusible (?) and D4/D5 clamps; D2/D3 plate-to-cathode arc protection. Effect: no tonal effect in normal operation, 1 ohm adds a few mV of local feedback at 40 mA.
7. **Speaker**: 6G16 two speakers (two jacks); AA763/AB763 one speaker + EXT (impedance not printed); '63 RI two 8 ohm speakers (SPKR 1/2 shown as 8 ohm each); '64 one 15" 8 ohm with J8/J9. Effect: load matching of the OT.
8. **OT**: TR3 125A6A (6G16) -> 125A7A (AA763, AB763) -> 036966 (?) ('63 RI) -> 0041754000 ('64). Primary impedance not printed in any.

### A6. Supply differences
- 6G16: 380-0-380 V AC, 16 uF/450 V caps, rails 440 V. Sag is largest here (small caps, 10K+10K).
- AA763/AB763: 355 V per side, 20 uF/525 V caps, B +460 V, C +450 V (1K), D +410 V (4.7K). AB763 additionally has the 2x 70 uF/350 V + 220K 1 W series pair ahead of the choke (more reservoir; COMP: stiffer).
- '63 RI: solid-state bridge, 22 uF/500 V caps, B+ +487 V at CT before the choke, 10K 1 W dropping resistors. Higher no-load voltage but stiffer rectifier (less sag; COMP).
- '64 Custom: switchable TUBE (5AR4) / SOLID STATE (BYD33V) rectifier; big reservoir (2x220 uF/285 V series, about 110 uF) before the choke, then 47 uF x2 series (about 23.5 uF), 1K/4.7K dropping resistors; ZV +455 V. COMP: the only version with a bigger, switchable, stiffer supply.

---
## B. VIBROLUX REVERB: AB568 vs AA270 (sheet 1 rev F, sheet 2 with volume-boost)
| Item | AB568 | AA270 (S1 / S2) | Effect |
|---|---|---|---|
| Output stage | 6L6GC with cathode resistors 120 / 150 ohm 7 W (?) to ground and 25 uF/50 V (?); cathodes +10 V; grid bias line -41 V (?) | cathodes direct to ground, bias -48 V | COMP: AB568 is a hybrid (cathode + grid bias); AA270 is fixed bias. AB568 has more local current feedback and a lower effective stiffness at clipping. |
| Grid network | 1500 ohm stoppers, 100K leaks to bias line, grid-to-cathode-node caps (about 3000 pF (?)) | 1500 ohm stoppers, 100K leaks, 2000 pF (?) each from grid to ground | TONE: RF/high-frequency trimming differs. |
| Bias supply | 5U4GB tap -> 27K -> diode -> 10K-L pot with 25 uF/10 V (?) ; 10K/10K/15K divider; -30/-41 V | 1K -> diode -> 80 uF/75 V; 15K / 10K-L OUTPUT MATCHING / 3.3K; -48 V | COMP: bias level and ripple filtering. |
| B+ at plates | +460 V (OT CT +465 V) | +450 V (S1), +445 V after choke | COMP/headroom. |
| Rectifier / HV | 5U4GB, 345 V AC (?), +463 V | 5U4GB, 345 V AC, +465 V | same |
| Filter | 2x 16 uF/450 V + choke + 16 uF x3 with 4.7K / 10K 1 W | same caps; S2 uses 2.2K 1 W in place of 4.7K | COMP: S2 has less droop on the +375 V rail. |
| PI | grid leaks 1M; tail 22K / 470; 100 ohm to ground; 820 NFB; cathodes +40 V (?); plates 47K x2, +270 V | S1: 1M, 22K, 470; S2: 330K grid leaks, tail 680 ohm; cathodes +100 V; plates 47K | GAIN/headroom of PI; the 330K leaks lower the input impedance of the PI (less low-frequency retention, TONE). |
| Reverb driver cathode | 2200 ohm | S1 2200 ohm; S2 prints 680 ohm with a pencil note reading 25-25 and 2200 (cathode +6 V printed) | GAIN of reverb drive. |
| Reverb recovery | cathode 820 ohm, plate cap .005 uF (?) | cathode 820 ohm, .003 uF; S1 shows 220K only (S2: 220K || 2000 pF) | TONE: .003 less bass in the reverb return; 2000 pF rolls off some highs. |
| Vibrato | 12AX7, neon/photocell, INTENSITY 50K-RA tied to the bias divider (bias modulation); SPEED 3M-RA | Same oscillator; INTENSITY 50K-RA 021915 wiper to node A (preamp cathode) per S2: modulates preamp gain, not bias | GAIN: different tremolo character (preamp vs bias). |
| Extras | none | S2 adds volume-boost pedal, 4.7 ohm pilot R, thermal protector, export taps | |
| Speakers | two 8 ohm drawn (hand-written) | S1 handwritten 8 ohm x2 / "TOTAL LOAD 4 OHM"; S2 prints 4 ohm total | |

---
## C. PRINCETON REVERB: AA1164 vs AB1270 (rev C)
The two schematics are nearly identical. Only these differences were found:
1. **Rectifier**: GZ34 (AA1164) vs 5U4GB (AB1270). Both fed by 340 V AC per side and +420 V. COMP: the GZ34's lower internal resistance gives slightly less sag.
2. **Notice**: AA1164 "values +/-20 %", capacitors at least 400 V; AB1270 "+/-10 %", capacitors at least 600 V. No audio effect.
3. **Recovery grid**: AA1164 220K only; AB1270 220K in parallel with 2000 pF to ground. TONE: slight high-frequency loss in the reverb return (corner depends on the tank's output impedance, which is not printed).
4. **Reverb driver plate**: +400 V printed (AA1164) vs +395 V (AB1270): measurement spread.
5. **Vibrato oscillator plate**: AA1164 +260 V; AB1270 +268 V / +260 V (two overlapping values). Negligible.
6. **Bias supply**: AA1164 "100K 5 %" / 25-50; AB1270 100K 1 W, 0.5 A/600 V diode (020881), 50 uF/70 V; both end at -34 V.
7. **Part numbers**: AB1270 lists transformer P/Ns (022772 etc.) and the AB1270 layout adds pot part numbers; AA1164 does not.
8. **Heater centre tap**: AB1270 draws 100 ohm to ground from each end of the heater winding; not drawn on AA1164 (?).
Audio-circuit values (stage loads, tone stack, cathodyne PI, 6V6 bias, supply ladder 1K/18K/18K, 20 uF caps) are identical.

---
## D. Things to resolve before modelling
- The '64 Custom .022 uF bass cap and the 100K/100K input resistors are real (vector drawing) changes.
- AA763 vs AB763 bass cap (.05/.033 vs .047) and PI plate loads (100K/100K vs 82K/100K) are small-scan reads.
- Vibrato modulation target per amp (bias vs preamp cathode) should be confirmed against a clean AA270/AB763 drawing.
