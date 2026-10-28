# Fender '63 Vibroverb reissue - schematic read

Source: d6bdb8d7-fender_63_ri_vibroverb_schem.pdf (1 page, 3484x2264 px bitmap of a CAD drawing; Fender Musical Instruments, Brea CA; title "SCHEMATIC 63 VIBROVERB AMPLIFIER", drawing no. 036988, Rev A (released PR161, 8/6/96), drawn Wilkens).
This is the best-quality scan of the set (reference designators + numbered test points), but the CAD font renders the digit zero as a solid blob, so every value containing a zero (100K, 1M, 1.0, 10, 220K ...) had to be read from context. Values with an ambiguous digit are marked (?). Where two printed readings of the same node had to be reconciled, a current/voltage check was used (stated below).
Drawing notes (printed): 1) V1,2,4,5,6 are 12AX7A. 2) All resistors 1/4 W carbon film unless noted. 3) external line fuse 3 A in 100/120 V models, 1.6 A in 220/240 V (read "3 A", ratio check) . 4) internal line fuse 4 A / 2 A. 5) CP3,4,5,6 and C89 installed in 120 V models only. 6) all DC voltages measured to ground with a DVM (10 Mohm), +/-20 %. Test conditions: line 120.0 VAC 60 Hz, reverb and vibrato off, reverb and vibrato controls fully CCW, all volume and tone controls at 50 % rotation, no AC signal. AC test points TP1,2,4,5 measured with no signal at the bright input, normal input signal adjusted for 11.9 Vrms output (onset of clipping); a similar note for the bright channel (TP7...).
Output test (printed): 40 W RMS into 4 ohm resistive load at 7 % THD, measured at rated line voltage with 1 kHz sine input; TP21 = 11.9 Vrms into 4 ohm (that is about 35 W, the onset-of-clipping level).
Ground symbols: down-triangle. Letter nodes X, Y, Z, B+ are supply rails (see section 7); C- is the negative bias supply.

## 1. Tubes and heaters
| Ref | Type | Function |
|---|---|---|
| V1 (V1A, V1B) | 12AX7A | Normal channel stage 1 (V1A), stage 2 (V1B) |
| V2 (V2A, V2B) | 12AX7A | Bright channel stage 1 (V2A), stage 2 (V2B) |
| V3 | 12AT7 (as printed) | reverb driver, both halves in parallel, plate to T2 |
| V4 (V4A, V4B) | 12AX7A | V4A reverb recovery, V4B mixer/driver |
| V5 (V5A, V5B) | 12AX7A | vibrato (bias-modulating) oscillator V5A, buffer V5B |
| V6 (V6A, V6B) | 12AX7A | long-tail phase inverter |
| V7, V8 | 6L6GC | push-pull output |
Rectifier: solid-state bridge CR1-CR4 (see section 7). Heaters: T1 6.3 V winding (BRN/BRN) through sockets XV1-XV8 (pins 4/5 paralleled on 12AX7 sockets with pin 9 centre tap; XV7/XV8 are the 6L6 sockets pins 2/7); heater centre tap is made by R62 and R63, each 47 ohm 1/2 W, to ground; pilot lamp LA1 type T47 across the heater winding. Heater supply voltage not printed.

## 2. Preamp stages
### Normal channel (V1)
- Inputs J1 (jack 1) and J2 (jack 2): R1 68K from the jack-2 hot contact to the grid node, R2 68K from the grid node to the jack-1 hot contact, R3 1M from the jack-1 contact (and jack-1 side of R2) to ground. Printed AC test level 57 mVrms at TP1 (1 kHz).
- V1A (stage 1): plate load R4 100K 1/2 W to rail X; cathode R5 1.5K with C1 22 uF/25 V; TP3 cathode +1.49 V; plate TP2 +284 V (?, ambiguous 8) with 2.46 Vrms AC.
- Tone stack (section 3).
- V1B (stage 2): grid fed from volume wiper R10; cathode shared with the bright V2B cathode network (R24 820 ohm + C12 22 uF/25 V, node A), TP6 +1.97 V; plate +178 V (TP5), AC 2.66 Vrms; plate load: a resistor hidden under a black scan smear (value not readable) to rail X.
- V1B plate -> R35 220K -> mixer/coupling node M (see section 4).
### Bright channel (V2)
- Inputs J3 / J4: R12 68K, R13 68K, R14 1M (same as normal).
- V2A: R15 100K 1/2 W plate load to X; cathode R16 1.5K + C6 22 uF/25 V; TP9 cathode +1.47 V; plate TP8 +282 V (?), AC 9.46 Vrms; TP10 AC 98.2 mVrms (?).
- Tone stack as normal channel (C7 330 pF, R17 100K, C8 .1 uF, C9 .047 uF, R18 bass pot, R19 6.8K, R20 treble pot, C10 stack cap).
- Bright cap: C11 47 pF/1 kV across the volume pot R21 (1M 20 % A), fixed (no switch).
- V2B: cathode R24 820 ohm + C12 22 uF/25 V (+1.5 V node A); plate load R22 22K (1/2 W) in series with R23 82K (1/2 W) to X; plate TP11 +168 V (?), AC 3.41 Vrms.
- V2B plate (junction R22/R23) -> C17 .022 uF/400 V -> R32 470K -> mixer V4B grid node. V2B plate also -> C13 500 pF/1 kV -> reverb driver V3 grid.

## 3. Tone stack (identical both channels, FMV-style Fender stack)
- C2/C7 330 pF/1 kV (printed "33?pF", a 3-3-zero reading; could be 390 pF (?)) from plate to treble pot top.
- R6/R17 100K from plate to stack node.
- C3/C8 .1 uF/400 V from stack node (R6 bottom) to bass pot top (pot printed "250K 10 % A" labelled R7/R18).
- C4/C9 .047 uF/400 V from stack node to bass pot bottom.
- R8/R19 6.8K from bass pot bottom to ground.
- Treble pot R9/R20: 250K 20 % A, with "50K TAP" printed (a tap at 50K from one end), wiper to the stack node (junction of bass pot top / C3).
- C5/C10 about .0068 uF/600 V (printed ".0?68 uF 600 V", first digits blob) from the treble pot's low end to ground (?).
- Volume pots R10/R21: 1M 20 % A (audio taper).
- Bass pot R7/R18: 250K 10 % A. Taper A means audio.

## 4. Reverb and vibrato
### Reverb driver
- V3 12AT7 both triodes in parallel; grid via C13 500 pF/1 kV from the bright V2B plate; R25 1M grid leak; cathode R26 2.2K 1 W + C14 22 uF/25 V, TP24 cathode +8.92 V.
- Plate: primary of T2 (RED to rail Z, BLU to the plates); TP23 plate +43x V (digit unclear), AC 95.2 Vrms (?) at the plate. T2 part no. 037099 (digits partly unreadable). Secondary GRN/BLK to CP25/CP26 -> BT1/BT2 -> REVERB PAN IN (white lead); no ratio or impedance printed; TP38 1.12 Vrms at the pan input (?).
### Reverb recovery
- Pan OUT (red lead) -> BT3 -> V4A grid; R27 220K grid to ground; BT4 sleeve to ground.
- V4A: cathode R28 1.5K + C15 22 uF/25 V (TP27 +1.47 V); plate load R29 100K 1/2 W to X; plate TP26 +292 V (?) (reads "+282"/"+292"), AC 144 mVrms; TP25 9.48 mVrms (?) at the pan output.
- Plate -> C16 .0033 uF (?) /400 V -> R30 REVERB pot 100K linear ("LIN") to ground; wiper -> R65 82K -> node -> R31 390K -> V4B grid node.
### Mixer / reverb mix stage (V4B)
- Grid node receives: reverb return (R65 82K + R31 390K), and bright channel via C17 .022 uF + R32 470K (the resistor ladder R32 / R31 / R65 appears in series from the C17 node down to the pot wiper, with the grid taken from the R32-R31 junction).
- V4B cathode R33 1.5K to ground, TP37 +1.47 V; plate load R34 100K 1/2 W to X; plate TP14 +263 V (?), AC 16.2 Vrms.
- Mix node M: V4B plate -> R36 1M -> M; normal V1B plate -> R35 220K -> M; M -> C18 (printed "500pF 1KV", compare AB763's .001 uF) -> PI grid V6A. TP15 5.6 Vpk-pk at M, TP16 5.2 Vpk-pk at the PI grid.
### Vibrato (bias modulation - not photocell) V5
- V5A oscillator (phase shift): grid network C29 .033 uF, C24 .033 uF, C26 about .068 uF (?), R49 and R50 about 300K each (printed "3??K", digits partly blob (?)); plate load R51 470K 1/2 W from node (rail X via R64 100K? 1/2 W (?), decoupled by C30 47 uF/350 V); cathode R52 4.7K + C25 22 uF/25 V (TP28 +1.58 V); plate TP29 about +158 V.
- V5B follower: grid from V5A plate; plate to Y/X-ish rail (TP30 about +303 V); cathode R55 100K 1/2 W to ground; TP31 about +158 V (cathode). Output from cathode: R56 22K -> node (C28 .1 uF/400 V to ground) -> C29b .1 uF/400 V -> R57 INTENSITY pot 250K linear -> C- bias line ("C-" arrow).
- SPEED: C27 about .0?? uF/400 V (digits blob) from the V5B cathode through R53 39K (?) and the SPEED pot (reverse-audio "RA", value blotted) in series with R54 1M to ground, feeds back to the oscillator network.
- TP32 -59.2 V is the bias line. Printed test: "VIBRATO TEST: vibrato on, speed and intensity full CW, 55 Vpk-pk sine at 8 Hz approx."
- Footswitch jack J5 (ring/tip/sleeve): S4 reverb, S5 vibrato (shorts to ground).

## 5. Phase inverter (V6, long-tail)
- Input via C18 to V6A grid; R38 1M grid leak (V6A grid to node J); R40 1M for V6B grid; C19 .1 uF/250 V from V6B grid to the left bus.
- R37 6.8K 1 W from node J to left bus (as read, "R37 6.8K 1W"); R39 820 ohm from J to cathodes; TP17 cathode +17.4 V. Left bus: R42 470 ohm (read "47?", checked with currents) to ground and R41 10K NFB resistor to the output transformer secondary. Cross-check: 17.4 V / (820+6.8K+470) = 2.1 mA tail, which matches the plate currents from R43/R44.
- Plate loads: R43 82K 1/2 W (top, V6A) and R44 100K 1/2 W (bottom, V6B) from rail Y (+358 V printed at TP35).
- Plates: TP18 +262 V (V6A), TP19 +260 V (V6B); AC 33.5 Vrms at TP18. C20 100 pF (?)/1 kV across the two plate outputs (read "C20 1??pF 1KV").
- Plate outputs -> C21 and C22 .1 uF/630 V -> V7/V8 grids.

## 6. Output stage
- V7, V8 6L6GC fixed bias. Grid leaks R45, R46 220K, no grid stoppers drawn; the two 220K meet at the bias node (C-), TP32 -59.2 V.
- Screens: R47, R48 470 ohm 1 W flameproof from rail Z (TP34, about +436 V); plates to the T3 primary ends (BRN, BLU), centre tap RED to B+ (TP33 +487 V, TP20 plate +485/+486 V printed on V7 plate). AC 266 Vrms (?) at V7 plate (TP20).
- T3 (part no. 036966 (?)): secondary BLK to ground side; speaker jacks J6 (speaker, two jacks) and J7 (EXT SPKR); drawn with SPKR 1 8 ohm and SPKR 2 8 ohm (two 8 ohm speakers).
- NFB: R41 10K (as read) from the BLK secondary (jack node) to the PI left bus, divided by R42 (about 470 ohm) to ground.
- Cathodes both to chassis ground.
- Bias supply: T1 BRN-BRN winding -> CR5 1N4003 -> C36 100 uF/100 V -> R58 1K 1/2 W -> C38 22 uF/63 V, with R59 (blotted "??K", 1/2 W) as load to ground; output "C-". No trimmer drawn on this sheet; the INTENSITY pot R57 feeds the vibrato signal into this line.

## 7. Supply
- T1 primary: S1A/S1B power switch, F1 3 A external (?), F2 4 A internal, TH1 thermistor (inrush), 120 VAC 60 Hz; CP6/C89 .01 uF/125 VAC and S2 ground switch (120 V models only). Export alternate primary table for 100/120/220/240 V.
- HV secondary RED/RED to bridge CR1-CR4 (printed "800V 1A" ?, type not named), STANDBY S3 in the DC return (CP35-CP36-CP37, i.e. in the negative lead of the filter caps), first filter C31 and C32 22 uF/500 V in parallel (about 44 uF), B+ node CP16/CP17 (TP33 +487 V) feeds the OT centre tap.
- Choke L1 (P/N 036496) from B+ to rail Z; C33 22 uF/500 V at Z (+436 V TP34); R60 10K 1 W flameproof to rail Y; C34 22 uF/500 V at Y (+358 V TP35, feeds PI plate loads and reverb/preamp via further resistors); R61 10K 1 W FP to rail X; C35 22 uF/500 V at X (+302 V TP36, preamp plates).
- Actual plate-load rails: preamp plates are fed from X (R4, R15, R34, R29...), PI from Y, reverb driver transformer from Z, screens from Z.
- Heater winding: separate secondary, see section 1.

## 8. Not legible / uncertain
- All digits with a zero were inferred; treat 100K/1M/10K/220K as reliable only where context confirms (stage plate loads and test-point currents agree).
- Normal V1B plate resistor (hidden by smudge), R59, C-supply parts R58/R59 values, SPEED pot value, vibrato capacitor values C26/C27 and resistors R49/R50/R53/R64.
- TP numbers: +284/+282/+168/+263/+292 are single-digit uncertain.
- C2 330 vs 390 pF, C5/C10 value, C20, C18 (500 pF vs .001 uF).
- Reverb pan and T2 impedances, T3 impedance taps and part numbers.
