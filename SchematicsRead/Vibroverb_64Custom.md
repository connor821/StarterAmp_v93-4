# Fender '64 Vibroverb Custom (reissue) - schematic read

Source: ba417e5c-Reissue_64_Vibroverb_Custom_Schematic.pdf. Page 1 = "Service diagram, combined (schematic), '64 Vibroverb Custom Amplifier", drawing 0059708000 Rev A (PR532, 27-AUG-02, Fender Musical Instruments, Corona CA; drawn/engr. L. Lorenzen). Page 2 = PCB / chassis layout (test-point locations, pot and jack positions; no extra values). Vector CAD drawing, rendered at 300 dpi: text is crisp and every value below was read directly, except where marked (?).
Notes printed on drawing: all resistors in ohms 5 % 1/4 W unless noted; unpolarised caps 10 % or better, 50 V min (supply bypass 20 %); polarised caps 20 %, 50 V min. DC voltages are measured with a DVM (>=1 Mohm AC, 10 Mohm DC), +/-20 %; test conditions: rated line voltage, reverb pan 0064063000 connected at J5-J6, 8 ohm resistive load at J8, "BRIGHT" switches off, reverb and vibrato switched off with the footswitch, vibrato speed R42 at max CW, all other panel controls at 50 %, rectifier switch set to "TUBE", no input signal. Bias set to -50 VDC at TP36 (or 35-38 mV at both TP37 and TP38).
Naming: supply rails are ZV (after choke, power-stage plates/screens, reverb-driver transformer, vibrato), YV (PI plates, via R67), XV (preamp plates, via R68). A bias rail carries -60.2 V.
Ground symbol: hatched chassis ground only.

## 1. Tubes and heaters
| Ref | Type | Function |
|---|---|---|
| V1 (A,B) | 12AX7A | Normal channel stage 1, stage 2 |
| V2 (A,B) | 12AX7A | Vibrato channel stage 1, stage 2 |
| V3 (A,B) | 12AT7 | Reverb driver, halves paralleled |
| V4 (A,B) | 12AX7A | A = reverb recovery, B = reverb/vibrato mixer + driver |
| V5 (A,B) | 12AX7A | Vibrato oscillator (A) and driver (B) of the opto-coupler |
| V6 (A,B) | 12AT7 | Long-tail phase inverter |
| V7, V8 | 6L6GC | Push-pull output ("V7-A, V8-A" sections) |
| V9 | 5AR4 | Rectifier (selectable, see supply) |
Heaters: T2 GRN windings through fuse F3 T10A (slow); socket wiring shows 12AX7/12AT7 pin 4/5 (6.3 V) with pin 9 centre tap; R70, R71 100 ohm 1/2 W heater centre-tap resistors to ground (read "100", 1/2 W (?)); pilot lamp T47. 6L6 heater pins 2/7. Heater voltage not printed (6.3 V nominal).

## 2. Preamp
### Normal channel (J1 = NORMAL 1, J2 = NORMAL 2)
- Input network: J2 -> R1 100k -> grid node; grid node -> R2 100k -> J1 contact; R3 1M from the J1 contact to ground (same cross-connect arrangement as the old Fender 2-jack scheme). R1/R2 are 100k, not 68k.
- V1-A (stage 1, "Normal pre"): plate load R4 100k 1/2 W to XV; cathode R5 1.5k with C1 22 uF/25 V bypass; TP7 +2.02 VDC cathode; AC test TP6 20 mVAC (grid), TP8 1.15 VAC (plate) at 1 kHz.
- Tone stack (section 3).
- V1-B (stage 2): grid from volume wiper (R10 1M-J); plate load R11 100k 1/2 W to XV; cathode R74 1.5k + C38 22 uF/25 V, TP10 +2.12 VDC; TP9 32 mVAC, TP11 1.50 VAC.
- V1-B plate -> C5 .047 uF/400 V -> node with R77 10M to ground -> MOD switch S1-C pin 8; pin 7 -> R12 220k -> mix node. As drawn ("MOD" position) pole 8 is on pin 9 (not wired), i.e. the Normal channel's path through R12 is open in the MOD position and closed (8-7) in the other position (?). I cannot confirm from the sheet which position is "stock".
### Vibrato channel (J3 = VIBRATO 1, J4 = VIBRATO 2)
- Input: R13 100k, R14 100k, R15 1M (same as normal).
- V2-A: plate load R16 100k 1/2 W to XV; cathode R17 2.7k + C6 22 uF/25 V; MOD switch S1-A switches R75 3.3k in parallel with R17 (stock): TP13 +2.70 VDC in MOD, +2.06 VDC stock. Plate AC 1.15 VAC (TP14), grid 20 mVAC (TP12).
- Tone stack (same as normal; R18-R22).
- V2-B: plate load R23 100k 1/2 W to XV; cathode R24 2.7k with C11 .68 uF/250 V bypass (the 22 uF/25 V C39 bypass and R76 3.3k parallel branch is switched by S1-B): TP16 +2.70 VDC MOD / +2.06 VDC stock. TP15 32 mVAC, TP17 1.9 VAC.
- V2-B plate -> C12 .022 uF/400 V -> V4-B grid node (R25 3.3M || C13 10 pF/1 kV in the line; R33 220k to ground and R32 470k to the reverb pot wiper at that grid node). V2-B plate also -> C14 500 pF/1 kV -> reverb driver V3 grids.

## 3. Tone stack (identical both channels)
- C2/C7 250 pF/1 kV from plate to treble pot top; R6/R18 100k from plate to stack node; C3/C8 .1 uF/400 V from stack node to bass pot top; C4/C9 .022 uF/400 V from stack node to bass pot bottom; R9/R21 6.8k from bass pot bottom to ground.
- Treble pot R7/R19 250k J; bass pot R8/R20 250k A; volume pot R10/R22 1M J ("J" taper letter as printed; "A" for bass). The treble wiper goes to the volume pot top; bass pot wiper is tied to its top (as drawn: wiper to top of bass pot, forming a variable resistor).
- BRIGHT: S3/S4 switch with C29/C10 120 pF/1 kV across the volume pot (top to wiper); the bright switch is a real switch (drawn open in the schematic = "off").
- No resistor-cap shunt across treble pot (no C-to-ground cap as on the '63 reissue).

## 4. Reverb and vibrato
### Reverb driver (V3, 12AT7 both halves parallel)
- Grids from C14 500 pF/1 kV (from V2-B plate); R26 1M grid leak to ground. Cathode R27 2.2k 1 W + C15 22 uF/63 V; TP19 +8.55 VDC.
- Plate to REVERB XFMR T1 (P/N 0037099000): RED primary end to ZV, BLU to the V3 plates; secondary GRN -> J5 REVERB OUT tip (the jacks are labelled from the pan's perspective: J5 = REVERB OUT is the driver output = pan input), BLK to ground. TP20 plate 43.0 VAC. Transformer ratio and tank impedance are not printed; the 0064063000 pan is the connected reverb pan.
### Reverb recovery (V4-A)
- J6 REVERB IN (pan output) -> V4-A grid; R28 220k to ground. TP22 >1.0 mVAC (excitation dependent).
- Cathode R30 820 + C16 22 uF/25 V (TP23 +2.01 VDC; V4-B cathode joins this node); plate R29 100k 1/2 W to XV; plate AC TP24 >30 mVAC (note 8).
- Plate -> C17 .0033 uF/400 V -> R31 100k HD REVERB pot to ground (HD taper); wiper -> R32 470k -> V4-B grid node. Node "RVB" at the plate also goes to the footswitch jack J7 through C41 220 pF/500 V and C42 220 pF/500 V (RF/pop caps; footswitch shorts reverb or vibrato).
### Mixer (V4-B)
- Grid node: C12 .022 uF (from V2-B plate) via R25 3.3M || C13 10 pF, R32 470k (reverb return), R33 220k to ground. Plate load R34 100k 1/2 W to XV; plate AC 2.9 VAC (TP26), grid 90 mVAC (TP25). Plate -> C18 .1 uF/400 V -> node with R78 10M to ground -> R35 220k -> mix node -> C25 .001 uF/400 V -> PI.
- Mix node also receives the Normal channel via R12 220k (S1-C). MOD switch S1-D: pole 11 (C18 / R35 / R78 node) arm drawn to pin 12 (unwired); pin 10 goes to the -60 V bias line near R48/R79 (as traced; the purpose is unclear (?)).
### Vibrato (opto-coupler tremolo, V5 12AX7A)
- Oscillator V5-A: three-section RC phase-shift ladder from plate: C22 .022 uF/400 V, C21 .01 uF/400 V and C20 .01 uF/400 V with R39 1M, R40 1M, R37 390k, R38 2.2M; grid node on R40; plate R43 220k 1/2 W to ZV (330 Vpk-pk at TP30); cathode R44 2.7k + C23 22 uF/25 V (TP28 +2.02 VDC). SPEED R42 3M CCWA (variable resistor from the C21/C22 node to ground in series with R41 100k). 
- The oscillator ladder low end goes to node VIB through R36 10k, with C19 4.7 uF (polarised) to ground and R37 390k to ground: TP27 -8.60 VDC (a decoupled, divided bias supply); the VIB line returns to the footswitch (vibrato on/off).
- Driver V5-B: grid fed from the oscillator plate node (TP30 side); plate load R45 10M 1/2 W from ZV; cathode R46 100k 1/2 W + C24 22 uF/25 V (TP29 +13.2 VDC); the plate drives optocoupler OP1 (037836, lamp side with R47 100k 1/2 W from ZV; waveform at TP31: about +30 V to -70 V swing, 60 ms period, 20 ms dwell as drawn).
- OP1 photoresistor: one end ground, the other to R48 INTENSITY 50k CCWJ wiper; R48 top is on the bias rail (-60.2 V through R79 2.4k; this is the bias-modulation point), so tremolo modulates the 6L6 grid bias (bias-wobble type).

## 5. Phase inverter (V6, 12AT7 long-tail)
- C25 .001 uF/400 V from the mix node to V6-A grid; R49 1M and R52 1M grid leaks to node J; V6-B grid via C26 .1 uF/250 V to the NFB bus; R53 100 ohm from NFB bus to ground; R64 820 ohm from the NFB bus to the speaker secondary (via J8/J9 contacts); R50 22k 1 W from NFB bus to node J; R51 470 ohm from J to the cathodes (TP34 +93.6 VDC at cathodes).
- Plate loads: R54 82k 1/2 W (V6-A) and R55 100k 1/2 W (V6-B) from YV. DC plate voltages not printed; AC: TP35 13.7 VAC (V6-A), TP36 14.7 VAC (V6-B). TP32 1.4 VAC grid A, TP33 540 mVAC grid B.
- C27, C28 .1 uF/630 V to the 6L6 grids via R60/R61 1.5k.
- NFB: R64 820 ohm from speaker secondary to the bus with R53 100 ohm to ground (NFB divider (820+100)/100; as in the old AB763 with 820/47). Note that the divider ratio differs: here 100 ohm replaces the AB763's 47 ohm.

## 6. Output stage
- 2x 6L6GC, fixed bias, push-pull. Grid stoppers R60, R61 1.5k; grid leaks R56, R57 220k from grid line to the bias node (TP37 labelled -50 VDC at the node, adjust R58 10k BIAS ADJUST with R59 27k 1/2 W to ground at the bottom end).
- Screens: R62, R63 470 ohm 1 W from ZV (+455 V printed at TP2/TP40). Plates TP40 +455 VDC (and 80 VAC); TP41 80 VAC.
- Cathodes: each cathode through R72/R73 (read "Fp1" 1 W; a 1 ohm fusible resistor is consistent with TP38 +38 mVDC / 35-38 mA bias; not certain, (?)) to ground, with D4, D5 1N4006 clamps from the cathode nodes to ground. D2, D3 (type "R3000") from each plate to its cathode node (plate-to-cathode arc protection). Test points TP38, TP39 +38 mVDC at the cathodes.
- Output transformer T3 (P/N 0041754000): primary BLU - RED (CT, to B+ ZV) - BRN; secondary taps GRN, GREEN/YELLOW, WHITE, BLK (ground). Speaker jacks J8 MAIN SPKR (8 ohm) and J9 EXT SPKR (8 ohm) with switched contacts; a 15" speaker, P/N 0059660000, shown (8 ohm). Impedance selection with a single 8 ohm load; TP42 1.25 VAC on the NFB tap.
- Output test (printed): 40 W minimum RMS into 8 ohm resistive at 5 % THD, 1 kHz into Normal channel J1, all normal controls full CW, vibrato channel controls full CCW, reverb/vibrato off, with tube rectifier.
- Global NFB: R64 820 ohm from T3 secondary (jack/GRN-side node) to the PI bus with R53 100 ohm to ground.
- Bias supply: T2 winding (RED/BLU) -> R69 470 ohm 1 W -> D1 1N4003 -> C36 100 uF/100 V -> R79 2.4k -> bias rail TP5 -60.2 VDC; R58 10k trimmer and R59 27k set the grid bias node to about -50 V.

## 7. Supply
- Mains: IEC inlet, S6 power switch, F1 (3 A 250 V fast-blow at 100-120 V; 1.6 A 250 V at 220-240 V), C30 .1 uF 250 VAC across the primary, RT1 CL60-11 inrush thermistor; T2 120 V (P/N 0056919000) with 100/230/240 V wiring variants shown.
- HV: rectifier switch S5 selects TUBE (V9 5AR4) or SOLID STATE (D6, D7 BYD33V; full-wave centre-tap with the RED / RED windings); F2 T5A in the YEL (5AR4 filament) line; C37 .047 uF/630 V from rectifier output to ground; TP1 +457 VDC at the rectifier output.
- STANDBY switch S2 (GRY wire) then first filter node (the 'B+' node, i.e. the OT centre-tap feed on this sheet, is labelled on the line between S2 and the choke): C31, C32 220 uF/285 V in series with R65, R66 Fr100k 2 W equalisers (about 110 uF at about 570 V), then choke L1 (P/N 0036486000) to ZV (TP2 +455 VDC). ZV filter: C40, C33 47 uF/350 V in series with R80, R81 Fr100k 2 W (about 23.5 uF).
- ZV -> R67 1k 1 W -> YV (TP3 +443 VDC), C34 22 uF/500 V; YV -> R68 4.7k 1 W -> XV (TP4 +405 VDC), C35 22 uF/500 V.
- Printed B+: TP1 +457, TP2/ZV +455 (plates and screens +455 at TP40), TP3/YV +443, TP4/XV +405. Bias -60.2 V (TP5), -50 V at the grid node (TP37 label), -8.60 V vibrato bias (TP27).

## 8. Not legible / uncertain
- Which MOD switch position is "stock": S1-A/B cathode network voltages are printed for MOD and STOCK, but S1-C and S1-D connectivity in the non-drawn position is inferred (?).
- R72/R73 value ("Fp1" fusible resistor; 1 ohm assumed from the 38 mV bias reading).
- Reverb transformer ratio, pan impedance, output transformer tap impedance, heater CT resistor wattage.
- Exact source of the VIB line (footswitch wiring), and exact driver wiring of OP1 lamp.
