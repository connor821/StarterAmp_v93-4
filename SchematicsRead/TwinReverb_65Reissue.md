# Fender '65 Twin Reverb Reissue - schematic read

Source: `6b809f01-Fender_65_twin_reverb_service.pdf` (9-page service manual, March 1991, p/n 021-7300-000 (120 V), 021-7350 (220 V), 021-7340 (240 V), 021-7370 (100 V)). Pages 1-7 have a text layer (parts list / BOM). The two drawings are scans: schematic dwg **037627 rev E** (rotated 180 degrees on the page; "SCHEMATIC 65 TWIN REVERB AMPLIFIER", Fender, Corona CA) and service diagram dwg **037632 rev D** (PCB layout and test-point locations). Values below come from the schematic; where the BOM (reference designators) was used to cross-check this is stated.
Schematic revision table: A "Released PR186" 08-MAR-91; B "ECO L02271" 08-JUL-91; C "EC 784" 11-SEP-91; D "EC 1388 / ECO L02897" 20-OCT-94; E "EC 1786" 16-OCT-96.
Schematic notes (printed): 1) V1, V2, V4, V5 are **12AX7A**; 2) resistors 1/4 W carbon film unless specified; 4) fuse 4 A slo-blo (100/120 V), 2 A (230/240 V); 5) S2 (ground switch) installed in 120 V units only; 6) all voltages measured to ground with a DVM (1 Mohm AC / 10 Mohm DC input), +/-20 %, line 120 VAC 60 Hz, bright switches/reverb/vibrato off, all Normal channel controls full CCW, all Vibrato channel controls 50 %, no signal; 7) AC voltages at TP16, TP17 with a scope and X10 probe; 8) **8200 pF cap (C41) installed in 230 V units only**; 9) instances not used: C29, CP5, CP6, CP32, CP35.
Test-point values in boxes (DC) and ovals (AC, test conditions) are as printed. "(?)" = not certain; "(computed)" = my arithmetic.

## 1. Tube complement and heater notes
| Ref | Tube | Function |
|---|---|---|
| V1A / V1B | 12AX7A | Normal channel stage 1 / 2 |
| V2A / V2B | 12AX7A | Vibrato channel stage 1 / 2 |
| V4B | 1/2 12AX7A | Mixer (vibrato channel + reverb return); note: label is V4B here, V4A in the AA270 |
| V4A | 1/2 12AX7A | Reverb recovery |
| V3 | 12AT7 | Reverb driver (triodes paralleled) |
| V5A / V5B | 12AX7A | Vibrato oscillator / modulator |
| V6A / V6B | 12AT7 | Phase inverter (long-tail) |
| V7, V8, V9, V10 | 6L6GC | Output (BOM: 6L6GC x4) |
| CR1-CR4 | 800 V 1 A diodes | Bridge rectifier (BOM 026730); CR5 1N4003 bias rectifier |
- Heater winding (T3 green leads CP13/CP14) feeds all filaments and the pilot lamp LA1 (T47). A **100 ohm "HUM ADJ." linear pot (R69)** is across the heater winding with its wiper to ground (hum balance; the AB763 had a grounded centre tap, the AA270 two fixed 100 ohm resistors).
- Mains: 120 VAC plug, power switch S1A/S1B, fuse F1 (4 A SB), thermistor TH1 (NTC, BOM KCC.C60) in series with the primary, ground switch S2 (120 V units).

## 2. Stage by stage
### Normal channel (J1, J2)
- Input: J1 / J2 with **R1 = 68k, R2 = 68k** in series (V1A grid at the junction), **R3 = 1 Mohm** from the J1 end to ground; jack switches drawn (J2 tip ties around R1/R2; same arrangement as AB763).
- **V1A**: plate load **R4 100k 1/2 W** to X; cathode **R5 1.5k || C1 22 uF/25 V**; TP24 oval ("~12 mV, for output test only") on the grid. Plate goes directly to the tone stack.
- Tone stack (section 3). **Volume R10 1 Mohm "30A"**.
- **V1B**: plate load **R11 100k 1/2 W** to X; cathode shared node: **R23 820 ohm || C11 22 uF/25 V** (**TP5 +2.0 V**; shared with V2B).
- Output: plate -> **R34 220k** -> **C18 .047 uF/400 V** -> node N1; N1 -> **C20 .001 uF/400 V** -> PI top grid.
### Vibrato channel (J3, J4)
- Input as Normal (R12, R13 68k; R14 1M). **V2A**: plate load **R15 100k 1/2 W**; cathode **R16 1.5k || C6 22 uF/25 V**, **TP2 +1.9 V**. **V2B**: plate load **R22 100k 1/2 W**; cathode **R23 820 || C11 22 uF** shared with V1B (TP5 +2.0 V).
- **Volume R21 1 Mohm "30A"**.
- V2B plate -> **C12 .022 uF/400 V** -> node VO. VO -> **R32 3.3 Mohm || C17 10 pF/1 kV** -> V4B grid; VO -> **C13 500 pF/1 kV** -> V3 grid (reverb send, vibrato channel only; wire seen running along the board edge from node VO to C13).
### V4B mixer
- Plate load **R33 100k 1/2 W** to X. Cathode **+2.0 V (TP12)**, shared node with V4A: **R27 820 || C15 22 uF/25 V**.
- Grid node: R32/C17 from VO; **R31 220k** to ground; **R30 390k** to the reverb-return node RR (R30/R74 junction, see section 4).
- Plate -> **C19 .1 uF/400 V** -> node N2; N2 -> **R35 220k** -> N1 (PI feed, see section 5). N2 also goes to the top of the INTENSITY pot R64 (vibrato). TP15 oval "4.2 v" on the mixer plate (AC, test condition).

## 3. Tone stack (both channels; same values as AB763 and AA270)
Vibrato channel designators in brackets.
- Slope: **R6 100k** [R17] from plate node to common node. **C2 250 pF/1 kV** [C7] plate to Treble top.
- **Treble R7 250k "30A"** [R18]; **Bass R8 250k "10A"** [R19]; **Middle R9 10k "10A"** [R20].
- **C3 .1 uF/400 V** [C8] and **C4 .047 uF/400 V** [C9]: same positions as the AB763 (.1 to the Treble-bottom/Bass-top node, .047 to the Bass-bottom/Middle-top node).
- **Bright**: **C5 120 pF/1 kV** [C10] across the Volume top-to-wiper, switched by slide switch **S4** [S5].
- Pot codes: BOM gives 1M "30A", 250k "10A" (bass) and "30A" (treble), 10k "10A", 3M "30C", 50k "30C", 100k "B", 10k "B". Interpretation (assumed, not stated): A = audio, B = linear, C = reverse audio (the schematic labels reverb R29 "LIN" and SPEED "3M 10C"); the 10/30 prefix is not explained.
- No shift/boost switching.

## 4. Reverb and vibrato
**Reverb**
- **V3 12AT7** (both triodes in parallel): grid **R24 1M** to ground, **C13 500 pF/1 kV**; cathode **R25 2.2k 1 W || C14 22 uF/25 V**, **TP8 +7.8 V**; plates to reverb transformer **T1 (037099)** primary (red/blue leads), other end to Z (+440 V). T1 secondary (green/black) to the pan input at J8 (RCA, dual).
- **V4A** reverb recovery: grid from J8 output, **R26 220k** to ground (AA270 had 220k in series with .002 uF; AB763 220k only); plate load **R28 100k 1/2 W** to X; cathode shared **R27 820 || C15 22 uF**, +2.0 V (TP12); plate -> **C16 .0033 uF/400 V** -> **Reverb pot R29 100k LIN**; wiper -> **R74 82k** -> node RR. RR -> **R30 390k** -> V4B grid node (which has R31 220k to ground). RR also goes to J7 (jack wire "GRAY") so the reverb footswitch S6 can ground the return. This replaces the AB763's 470k/220k network (470k in series, 220k to ground, wired straight to the grid). Divider (computed, ignoring the 3.3M): wiper -> R74 82k + R30 390k = 472k in series, with R31 220k to ground gives 220k/(472k+220k) = 0.318 of the wiper signal at the grid, the same as the AB763 (470k + 220k, 0.318); the R74/R30 junction is only a tap for the reverb footswitch (R74 sits between the wiper and the tap).
- Pan: reverb pan p/n 064063 (BOM: "REVERB UNIT 4 SPRING 4AB3C1B"), shielded cable, IN/OUT RCA.
**Vibrato** (changed circuit vs AB763/AA270)
- **V5A oscillator**: plate load **R54 220k 1/2 W** to Z; plate -> **C24 .022 uF/400 V** -> P1 -> **C25 .01 uF** -> P2 -> **C26 .01 uF** -> P3 = grid. **R57 1M** from P2 and **R58 1M** from the grid line P3 to node NB. NB -> **R76 270k** to ground, **C37 10 uF/100 V** to ground, and via **R59** to the -65 V raw bias node; the footswitch jack (S7 via **R75 10k**, J7 white) can pull NB to ground. TP31: **-0.35 V (vibrato ON), -8.5 V (OFF)** (computed check: -65 V x 270k/(1.8M+270k) = -8.5 V). **SPEED**: P1 -> **R55 91k** -> **R56 3M "10C"** (variable resistor, wiper tied to its bottom end) -> ground. **C38 220 pF/1 kV** from the V5A plate line to the V5A cathode node (?). Cathode **R60 2.7k || C27 22 uF/25 V**; **TP32 +1.9 V ON / ~0 V OFF**.
- **V5B modulator**: grid from P2 (long wire). Plate -> node LMP; **R62 100k 1/2 W** from Z to the lamp pin 2; neon lamp (**OP1, optoisolator assy 037836**) between pins 2 and 1 (pin 1 = plate side); **R63 10M** across the lamp; **C40 .022 uF/630 V** from the plate node toward ground (?); cathode **R61 100k 1/2 W || C28 22 uF/25 V**; **TP33 +13 V ON / ~0 V OFF**. Photocell (OP1 pins 3/4): one end to ground, the other to the wiper side of **INTENSITY R64 50k "30C"**; the pot's top connects to N2, bottom to ground (as in the AB763: shunts the vibrato-channel signal at N2).
- Differences vs AB763: oscillator plate cap .022 (was .02); speed series R 91k (was 100k); NB reference through R59/R76 network and footswitch (was 2.2M to the raw bias node); C37 10 uF; cathode voltages now printed with ON/OFF; lamp is fed from R62 100k with 10M across the lamp (AB763: lamp+100k in series to B+, 10M from plate to B+).

## 5. Phase inverter: V6 12AT7 long-tail (AB763-style values)
- Top grid: **C20 .001 uF/400 V** from N1; **R37 1M** to node X2. Bottom grid: **C21 .1 uF/250 V** from the NFB node; **R38 1M** to X2.
- Cathodes (**TP18 +99 V**) -> **R41 470 ohm** -> X2 -> **R36 22k 1 W** -> NFB node -> **R40 100 ohm** -> ground (tail about 22.6k, computed).
- Plate loads: **R42 82k 1 W** (V6A), **R43 100k 1 W** (V6B), both from **Y (+428 V)**. DC plate voltages not printed (AC test values only).
- Plate outputs: **C22 .1 uF/630 V** and **C23 .1 uF/630 V** to the phase nodes. Grid returns: **R44 220k** and **R45 220k** in series between the phase nodes, centre node to C- (bias).
- **Global NFB**: **R39 820 ohm || C39 1500 pF/1 kV** from the T2 secondary hot (gray wire; TP23 shows 9.0 V AC) to the NFB node (R36/R40 junction). The 1500 pF is new (AB763/AA270: 820 only). No presence control.

## 6. Output stage
- **4 x 6L6GC** (V7/V8 and V9/V10, pairs in parallel); cathodes to ground; **R46-R49 1.5k** grid stoppers; **R50-R53 470 ohm 1 W flameproof** screen resistors (BOM "RESCC 1W 470 ohm 10%" or RES FP) from node Z. Plates to T2 outer primary leads (BRN, BLU), centre tap (RED, "B+") to the pre-choke node (**+442 V, TP27**); screens at Z (**+440 V, TP28** / box "+440 TP21" near the tubes, reading +440 or +448 (?)).
- Bias: fixed negative (C-): **-50 V (TP26)**; supply below. Bias range (computed): about -43.5 V to -59.7 V.
- Output transformer **T2 (037612)** secondary: black/green to **J5 SPEAKER** (closed-circuit jack) and **J6 EXT SPEAKER** (open-circuit jack). **Two 8 ohm 12 in Jensen replica speakers (037617) wired in parallel (4 ohm total)**; "OUTPUT TEST: 85 watts minimum RMS into a 4 ohm resistive load at 5 % THD". Primary impedance not printed.
- No negative-feedback tap other than R39/C39 above.

## 7. Supply
- **T3 (037610, 120 V domestic)** HV winding (red leads CP11/CP12, voltage not printed) -> **bridge CR1-CR4 (4 x 800 V 1 A)**. **C41 8200 pF/1 kV** across the HV winding (230 V units only, note 8; drawn on this sheet).
- **Standby switch S3 is in the negative (return) leg** of the bridge to ground, not in the positive rail.
- Reservoir: **C30 and C31, 220 uF/285 V each, in series** (110 uF effective, computed), each with **R65 / R66 220k 1/2 W** balancing resistor (BOM: CAPAE 285V 220uF 20%).
- B+ (CP21/CP22/CP23): **+442 V (TP27)** feeds the OT centre tap and the choke **L1 (036486)**.
- After L1: **Z = +440 V (TP28)**, **C32 22 uF/500 V**: screens, V3/T1 (reverb), V5 (vibrato plates).
- Z -> **R67 1k 1 W (flameproof)** -> **Y = +428 V (TP29)**, **C33 22 uF/500 V**: PI plate loads.
- Y -> **R68 4.7k 1 W (flameproof)** -> **X = +393 V (TP30)**, **C34 22 uF/500 V**: preamp plate loads (R4, R11, R15, R22, R28, R33) and the mixer/recovery stages. The droppers match the AB763 (1k / 4.7k).
- Return: B+GND, Z_GND, Y_GND, X_GND joined; **R73 10 ohm** to "control PCB ground".
- **Bias supply**: T3 brown winding (CP9/CP10) -> **CR5 1N4003** -> raw node **-65 V (TP25)**, **C35 100 uF/100 V**; **R70 3.3k 1/2 W** -> **R71 10k LIN "BIAS ADJ."** (wiper = C-) -> **R72 27k** to ground; **C36 22 uF/63 V** on C-; wiper reads **-50 V (TP26)**. R59 (1.8 Mohm on the schematic; BOM lists R59 as **2.2 Mohm**, see section 8) and R75/R76 feed the vibrato network from the raw node.
- Sag sources: solid-state bridge (no tube drop); reservoir 110 uF effective (computed; AB763 and AA270 have 35 uF effective: 2 x 70 uF in series), so less ripple-induced sag than the earlier amps; choke L1 is in the screen/preamp path only; the 1k and 4.7k dropper chain is unchanged from the AB763. Plates are fed from the pre-choke node (+442 V). Transformer regulation is the main sag source (secondary voltage and DCR not printed).

## 8. Not legible / uncertain
- **R59: schematic reads "1.8 Mohm"; BOM lists R59 as 2.2 Mohm (RESCF 1/4 W 2.2M).** Both printed values given; the computed -8.5 V (OFF) check matches the schematic's 1.8 M (2.2 M would give -7.1 V).
- Pot taper letter meanings and the 10/30 prefix (assumed A audio, B linear, C reverse audio).
- C38 220 pF connection (plate to cathode of V5A?) and the C40 .022 uF connection are read from a dense area (?).
- Plate DC voltages of the preamp, PI and output tubes are not printed (only cathode / supply / bias test points and AC values).
- TP21 duplicated on the drawing (one DC box "+440"/"+448" (?) and AC ovals "108 v" at the OT primary ends).
- Waveform at TP35 (lamp) is drawn: levels about 428 V and 340 V (?), timing not read.
- HV and bias winding AC voltages, OT primary impedance: not printed. Export transformer wiring (P/N 049181) not read.
