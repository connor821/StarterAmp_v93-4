# Fender '68 Custom Deluxe Reverb (2013 reissue, two-channel Custom + Vintage) - schematic read

Source: `90a238eb-Fender_68_custom_deluxe_reverb.pdf` (16 pages, Fender service manual, Oct 2013, p/n 277-4000-000; downloaded from Manualslib). **Pages 1-11 and 13-14 have a text layer** (parts list, notes); the schematic pages are vector/raster drawings. I read: p5 circuit description (text), p6-9 parts lists (text), **p12 = main schematic** ("Service Diag. Comb (SCH), '68 Custom Deluxe Reverb", Dwg **7701464000 Rev B**, 28-JUN-13, sheet 1 of 3, revision B "PR 2845" 26-JUL-13; Fender Musical Instruments, Corona CA), p13 = sheet 2 of 3 (notes 10-18, tube list), p14 = PCB component layout (no values), p15-16 = export-only solid-state tremolo LDR replacement board (Dwg 0073579000) and its power supply, not modelled (they replace the LDR optocoupler on 230 V EU/UK units only).
Hand-checked at 600 dpi crops. The drawing prints reference designators (R1...R123, C1...C59), part values and **test points TP1...TP39 with DC/AC voltages**. Notes on sheet 1: (1) V1, 2, 4, 5 are 12AX7A/ECC83; (2) all resistors 1/4 W 5 % carbon film; (4) primary fuse F2A in 100-120 V models, F1A in 220-240 V; (5) all voltages to ground with DVM (>=1 Mohm AC, 10 Mohm DC), +/-20 %, standby on, reverb and vibrato switched off, other controls at 50 %, vibrato speed R42 max CW, **bias set for 24 mA of idle current in V7 and V8**, TP29-30 measured with a scope, TP6, 8, 9, 11 measured with 1 kHz sine into the CUSTOM channel only, other AC TPs with 1 kHz into the VINTAGE channel. Note 6: TP31/32 with x10 scope probe. Note 7 (last instances) and 8 (instances not used: C30, C37-57, P3-6, R73-116, S2, C5, C10, R12). Sheet 2 note 16: **C5, C10, R12 not installed on the '68 Custom PCB assemblies**. Note 17: R72 installed on solder side near R8 (bass).
Output test: 22 W min RMS into 8 ohm at 5 % THD, 1 kHz into Normal(Custom) channel J1, all Custom controls full CW, all Vintage controls full CCW, reverb and vibrato off. Spec (p4): input sensitivity 10 mV (high input) / 20 mV (low input) for 22 W, input impedance >1 Mohm, speaker 12 in 8 ohm 70 W (T5901AX60), 100 W power requirement, release PR 239.

## 1. Tube complement and heaters
| Ref | Tube (production, sheet 2 note 18) | Function |
|---|---|---|
| V1 | ECC83S-GT (12AX7A) | **CUSTOM** channel V1A (gain), V1B (recovery) |
| V2 | ECC83S-GT | **VINTAGE** channel V2A, V2B |
| V3 | ECC81S-GT (12AT7) | Reverb driver V3A+V3B paralleled |
| V4 | ECC83S-GT | V4A reverb recovery, V4B dry/wet mixer |
| V5 | ECC83S-ST | V5A tremolo oscillator, V5B modulator |
| V6 | ECC81S-GT (12AT7) | Phase inverter |
| V7, V8 | 6V6-S (schematic prints "6V6GT") | Push-pull output |
| V9 | 5AR4 | Rectifier (full-wave tube) |
- Heaters: 6.3 V (GRN) winding with **R70 100 ohm 1/2 W and R71 100 ohm 1/2 W** to ground (artificial CT), **F2 T5AL250V** fuse (XF2) in the heater line (5 A), pilot LA1 T47. Rectifier filament 5.4 VAC (YEL). Thermistor TH1 10 ohm 5 A (inrush), mains fuse F1 (2 A, 120 V), power xfmr T3 041316 (domestic 120 V), export 041317, OT T2 041318, reverb driver xfmr T1 037099, reverb pan 064063, OP1 037836, L1 choke 022707.

## 2. Stages

### CUSTOM channel (V1)
- **Inputs**: J1 (high) and J2 (low); R1 68k and R2 68k, **R3 1 Mohm** to ground, then **R117 10k grid stopper** (new vs the '65) to V1A grid.
- **V1A**: plate load **R4 100k 1/2 W** to X (**+270 V**, TP4); cathode **R5 1.5k || C1 22 uF 25 V**, **+1.4 VDC (TP7)**; plate AC 464 mVAC (TP8).
- **Tone stack** (section 3).
- **V1B**: grid via **R118 10k** from **VOLUME R10 1 Mohm "30A"**; plate load **R11 100k 1/2 W** to X; **cathode shares R24 820 ohm || C11 22 uF 25 V** with V2B (+1.4 VDC TP10). **V1B and V2B plates are tied together** (manual p5, and visible on the drawing): the two 100k plate loads (R11 and R23) are in parallel (~50k). The '65's C5 .047 uF / R12 220k are **not installed**. Combined plate node then goes C12 .022 uF -> mixer (see V4B). No bright cap on either channel (C10 not installed).
### VINTAGE channel (V2)
- Inputs J3/J4, R13 68k, R14 68k, **R15 1 Mohm**, **R119 10k** grid stopper.
- **V2A**: plate load **R16 100k 1/2 W** to X; cathode **R17 1.5k || C6 22 uF 25 V**, **+1.4 VDC (TP13)**; plate 455 mVAC (TP14).
- **VOLUME R22 1 Mohm "30A"**, **R120 10k** stopper to V2B grid.
- **V2B**: plate load **R23 100k 1/2 W** to X; cathode R24 820 || C11 22 uF (shared, +1.4 V); plate AC 291 mVAC (TP16); grid 9.9 mVAC (TP15).
- Plate output -> **C12 .022 uF 400 V** -> mixer grid node.
Both channels feed the **same** mixer/reverb path (this is a major topology change versus blackface where Normal bypassed the reverb mixer).

## 3. Tone stacks
**Vintage (V2A)** - same as '65: C7 250 pF 1 kV (plate to treble pot top); R18 100k (plate to stack node); C8 .1 uF 400 V; C9 .047 uF 400 V; R21 6.8k to ground; TREBLE R19 250k "30A"; BASS R20 250k "10A"; VOLUME R22.
**Custom (V1A)** - R6 100k, C2 250 pF 1 kV, C3 .1 uF 400 V, **C4 .022 uF 400 V** (vs .047 uF), TREBLE R7 250k 30A, BASS R8 250k 10A, **R72 18k in series with R9 6.8k** to ground (total 24.8k vs 6.8k) - a "scooped"-type Custom stack. This is the voicing difference between channels.

## 4. Reverb
- **V3 12AT7 driver** (halves paralleled): grid via **C14 500 pF 1 kV** from the mixer-grid / C12 node, grid leak **R26 1 Mohm**, **R123 10k** stopper on one grid; cathodes **R27 2.2k 1 W || C15 22 uF 25 V**, **+7.4 VDC (TP18)**; plate to **T1 037099** primary (RED -> **Z, +408 V**), secondary to **J5** reverb pan input (pan 064063) via **ferrite bead** (0059848000) on the reverb cable. AC at plate 8 VAC (TP19).
- **Recovery V4A**: grid from the pan output via **R122 10k** stopper, **R28 220k** to ground, **C58 220 pF** from pan output to ground; cathode **R30 820 || C16 22 uF 25 V**, **+1.4 VDC (TP22)**; plate load **R29 100k 1/2 W** to X; plate -> **C17 .0033 uF 400 V** -> **R31 100k "B" REVERB pot** -> wiper -> **R32 1.5 Mohm** (vs 470k) -> V4B grid node. Footswitch (reverb) shorts the reverb pot output; **C59 220 pF** on the footswitch jack line.
- **Mixer V4B**: grid from V1B/V2B plates via C12 .022 uF -> **R25 3.3 Mohm || C13 10 pF 1 kV** -> **R121 10k** stopper -> V4B grid; **R33 220k** to ground at the wet/dry node; plate load **R34 100k 1/2 W** to X, plate -> **C18 .1 uF 400 V** -> **R35 220k** -> PI input node. Cathode V4B pin 8 to ground (own cathode: not shown with a resistor; drawn straight to ground (?)).
- Circuit description (p5): "V1B/V2B signal also couples to V4B intermediate gain stage; this is the dry signal which mixes with the wet reverb signal at the grid of V4B." Gain staging same as '65.

## 5. Tremolo
Identical to the '65: V5A oscillator (R43 220k, C21 .01, C22 .022, C20 .01, R39 1M, R40 1M, R37 390k, R38 2.2M, R41 100k, **R42 3M "10C" speed**, R44 2.7k || C23 22 uF, C19 4.7 uF 50 V); V5B modulator (R45 10M, R46 100k || C24 22 uF, R47 100k, OP1 037836, **R48 50k "30C" INTENSITY**). DC: TP27 +1.8 V on / ~0 V off, **TP28 +18 V on / ~0 V off** (vs +11 V on the '65), TP29 324 Vpk-pk. Speed 0.5-5 Hz (manual). Per manual p5-6 the circuit "modulates the input impedance of the non-inverting input of the tube power amp" (V6A side). Footswitch via R36 10k to C- (TP26 -0 V on / -7 V off). Intensity pot 30C (vs 80C on the '65).
Europe: LDR optocoupler replaced by a solid-state board (Dwg 0073579000) - same function.

## 6. Phase inverter V6 12AT7
Same as '65: C25 .001 uF; R49/R52 1M; R50 22k 1/2 W; R51 470 ohm; R53 47 ohm; C26 .1 uF 250 V; R54 82k 1 W (V6A), R55 100k 1 W (V6B) to **Y (+324 V)**. Cathode **+69 VDC (TP33)**; C27/C28 .1 uF 630 V; R56/R57 220k grid leaks to bias; R60/R61 1.5k stoppers. **R64 = 1.5k (not 820)**: **global NFB resistor 1.5k** from the OT secondary (GRY/GRN = 8 ohm tap node) to the NFB node (R53 47 ohm to ground): ratio 47/(1500+47) = 3.0 % (-30.4 dB) vs 5.4 % (-25.3 dB) at 820 ohm - **less negative feedback**.

## 7. Output
- V7, V8 6V6GT, fixed bias; **R62, R63 470 ohm 1 W flameproof screen resistors**; screens from the B+ (red OT CT), plates **+407 VDC (TP37)**; 47 VAC at plates (test conditions), OT **041318**, speaker **8 ohm** (J7 speaker, J8 ext speaker).
- **Bias**: C- rail **-50 VDC (TP5)** from T3 RED/BLU winding -> CR1 1N4003 -> R69 22 ohm (flameproof) -> **C36 100 uF 100 V** -> C-; **R58 10k "B" trim** (P18 gray to C-, wiper P19) with **R59 22k 1/2 W** (vs 10k) to ground; **-38 VDC at TP36** (grid bias; set so each 6V6 idles at 24 mA); C29 22 uF 63 V on the wiper rail. Adjustment range: -50 V down to -50*22/(10+22) = -34.4 V.
- OT primary impedance not printed (service parts: "XFMR OUTPUT 65 DELUXE REVERB" 0041318).

## 8. Supply
- **V9 5AR4**; TP1 **+415 VDC** at the rectifier cathode (before standby). Reservoir **C31 220 uF 100 V in series with C32 47 uF 500 V**, with **R65 100k 1/2 W** and **R66 470k 1 W** balancing resistors; S3 STANDBY; SP1/SP2 spark gaps; **L1 choke 022707**; **Z = TP2 +408 VDC** (C33 22 uF 500 V); **R67 10k flameproof 1 W** -> **Y = +324 V (TP3)** (C34 22 uF 500 V); **R68 10k flameproof 1 W** -> **X = +270 V (TP4)** (C35 22 uF 500 V).
- Manual p6 text states "415 V (B+), 406 V (Z), 324 V (Y), 270 V (X)". Drawing TP2 prints **+408**; the manual text says 406.
- Z feeds: V3 plate (T1), V5 stages, OT centre tap/screens (the OT RED lead is labelled B+ at TP37 +407). Y feeds V6 plate loads. X feeds all preamp plate loads (R4, R11, R16, R23, R29, R34).
- Hum / heaters: separate 5.4 VAC winding for 5AR4.

## 9. Not legible / uncertain
- V4B cathode wiring (own cathode to ground vs shared) - drawn to ground with no resistor as I read it.
- Whether R117-R123 (10k stoppers) are in the parts list: they appear on the schematic only; the manual's parts list (p6-8) lists "10k 1/4 W R36 only" under that part number - the stoppers may appear under another part number. The schematic is the authority here.
- Output transformer primary impedance and the power xfmr secondary voltages are not printed (only winding colours and TP voltages).
- The "30C"/"10C"/"10A"/"30A" taper codes are printed as codes.
- Page 14 is a PCB layout; pages 15-16 are the export solid-state tremolo.
