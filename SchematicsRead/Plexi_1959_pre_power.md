# Marshall 1959 STD (Super Lead, 100 W, 4 x EL34) - preamp and power sheets read

- Source files: `1959prem.gif` (preamp, 1975x1352, clean CAD drawing) and `1959pwrm.gif` (output stage and PSU, 2071x1424, CAD drawing, a little lower contrast).
- Both drawn by "JIM MARSHALL PRODUCTS LTD., BLETCHLEY, MILTON KEYNES, ENGLAND", logo MARSHALL, copyright mark (C) in a circle.
- Preamp sheet title block: "1959 STD  Preamp"; file name printed "1987PRE.DGM"; revision table: **ISS. 4, DATE 10-5-88, DRN. CPV** (rows above it empty). Notes box printed: "PCB: JM80" / "V1,2: ECC83, filament supply shown on power stage diagram." / "JS3,4 chassis mounted and linked to board".
- Power sheet title block: "1959 STD  Output Stage & PSU"; file name printed "1959PWR.DGM"; revision table: **ISS. 5, DATE 7-12-88 (last digit blobbed: 88? 89? 86?), DRN. CPV**; **ISS. 4, DATE 10-5-88, CPV**.
- Reference designators printed. Capacitors: ".022u" etc. in uF; "470p", "220p", "100p", "47p" in pF; "50+50uF" = two 50 uF in one can. Resistors: "820n" = 820 ohm ("n" is the font's ohm sign); "2k7", "4K7", "5Kb" = 5k6 (the CAD font draws 6 as "b"; confirmed because "1k 5W" and "5Kb" are the same style and "5K6" is the standard stopper value); "1M", "100k" etc.
- Off-sheet nodes: preamp sheet ends X (HT), Y (to PI input via C8), Z (ground); power sheet starts X (via R30), Y (via C8), Z (ground).
- No DC voltage table or node voltages are printed on either sheet.

## 1. Tube complement
| Pos | Tube | Function |
|---|---|---|
| V1 (a,b) | ECC83 | Two input channels; V1a = Inputs II (Vol II), V1b = Inputs I (Vol I) |
| V2 (a,b) | ECC83 | V2a mixer / gain stage; V2b cathode follower |
| V3 | ECC83 | Long-tailed phase inverter |
| V4-V7 | EL34 (note printed "V4-7: EL34") | Four push-pull output tubes, 2 per side |
| Rectifier | 4 x 1N4007 (D2, D3, D4, D5) | Bridge |

## 2. Preamp (sheet 1959 STD Preamp, issue 4)
### Inputs
- Four input jacks: Inputs II = JS3 "Low" and JS1 "High"; Inputs I = JS4 "Low" and JS2 "High". Each jack is routed through **R4 68k** (JS3), **R3 68k** (JS1), **R8 68k** (JS4), **R7 68k** (JS2) to the V1 grid node of its half (V1a for Inputs II, V1b for Inputs I). Switch contact of the "High" jack normalled to the "Low" jack contact (drawn). **R2 1M** (Inputs II) and **R6 1M** (Inputs I) from the High-jack node to ground.
### V1a (Inputs II, Vol II)
- Plate load **R5 100k** to HT; coupling **C2 .022u** to the top of **VR1 1M log** (Vol II); no bright cap.
- Cathode **R1 820 ohm** in parallel with **C1 330u** (+ marked on the cathode side).
- VR1 wiper -> **R13 470k** -> mix node (V2a grid).
### V1b (Inputs I, Vol I)
- Plate load **R10 100k** to HT (supply node decoupled by R11/C19, below). Coupling **C5 .022u** to the top of **VR2 1M log** (Vol I).
- Cathode **R9 2k7** in parallel with **C3 .68u** (+ marked).
- **C4 100p** between the V1b plate and the V1b cathode (drawn from the plate node to the cathode node) - a plate-to-cathode local-feedback / treble roll-off cap.
- **C7 220p** from the VR2 top (after C5) to the VR2 wiper/R14 junction node (bright cap across the top of the pot).
- **R14 470k** from the VR2 wiper node to the mix node (V2a grid); **C6 470p** in parallel with R14 (from the wiper node to the mix node).
### Preamp supply
- **R11 10k 1W** from the X (HT) node (after R30 on the power sheet) to the preamp HT node; **C19 50+50u** (drawn as two electrolytics from the node to a common ground symbol) from that node to ground. V1a plate load R5, V1b plate load R10, V2a plate load R15 and V2b plate hang from this node (V1a/V2 nodes labelled X on the right).
### V2a (mixer / gain stage)
- Grid = junction of R13, R14 (and C6's far end). Cathode **R12 820 ohm** to ground, **no bypass cap drawn**. Plate load **R15 100k** to HT. Plate direct-coupled to V2b grid (drawn without a cap).
### V2b (cathode follower)
- Plate to HT node X directly. Cathode **R16 100k** to ground. Cathode output feeds the tone stack directly (no coupling cap).

## 3. Tone stack (plate of V2b cathode follower)
- **C9 470p** from the stack input (V2b cathode) to the top of the Treble pot VR3.
- **R17 33k** from the stack input down to node B.
- **C10 .022u** from node B to the junction VR3 bottom / VR5 top.
- **VR3 220k lin** (printed "220k lin"), Treble; wiper = output Y.
- **VR5 1M log** (printed "1M log"), Bass; drawn as a rheostat (wiper tied to the top end).
- **VR4 22k lin** (printed "22k lin"), Middle; between the bass pot bottom and ground; **C11 .022u** from node B (the R17 bottom node) to the Middle wiper.
- Output: Y -> C8 .022u (power sheet) -> PI.
- No master volume, no switches on this sheet.

## 4. Power sheet (1959 STD Output Stage & PSU, issue 5)
### Phase inverter V3 (ECC83)
- Input: **C8 .022u** from Y to the left (V3 first-half) grid; **R19 1M** from that grid to the junction J; **R21 1M** from J to the right grid; right grid also has **C13 .1u** from the right grid node to the NFB node N (ground-side of the feedback divider).
- Cathodes joined: **R18 470 ohm** from the common cathode to J. J to N via **R22 10k**.
- N to ground: **R25 4K7**. Presence: **VR6 22K** (variable resistor, wiper-end tied; printed "Presence") in series with **C15 .1u** from N to ground.
- Plate loads: **R20 82K** (left plate) and **R23 100k** (right plate), both to X (HT).
- **C16 47p** between the two plates.
- Coupling: **C12 .022u** from the left plate; **C14 .022u** from the right plate; each to a node feeding two grid stoppers.
### Output grid networks
- **R26 220k** and **R27 220k** (printed "220k") in series between the two driven nodes; their centre node is the bias node (goes to C17, R28, R29). Test points "Ca" (upper pair) and "Or" (lower pair) printed on the two nodes.
- Grid stoppers: **R33 5k6**, **R35 5k6** (upper pair V4, V5), **R34 5k6**, **R36 5k6** (lower pair V6, V7); printed "5Kb".
- Screens: **R37 1k 5W** (V4), **R39 1k 5W** (V5), **R38 1k 5W** (V6), **R40 1k 5W** (V7), each from the tube's screen to the screen supply, which is the HT node after the choke (the 1k's join on a line returning to the T3 choke output).
### Bias
- Negative bias line: **D1 1N4007** (orientation not resolved from the drawing) and **R32 27k** feeding a node with **C18 10u** (electrolytic, + mark) to ground; **RV1 22K** preset (variable resistor symbol with the wiper tied to the supply node); **R28 47k** and **R29 15K** from the bias node (grid-leak centre) down toward RV1 and the supply node; **C17 10u** from the bias node to ground. The exact interconnection of R28/R29/RV1 is as drawn but dense (?).
- The bias supply line runs along the bottom of the sheet to the lowest winding "Wh" of the mains transformer (the "Wh" tail next to the filament windings) (?). The bias winding voltage is not printed.
### Feedback
- **R24 100k** from the OT secondary tap wired "Ye" -> "Pur" (purple) to the NFB node N. The tap sits next to the 8 ohm ("Bn") terminal of the secondary and the "Bk" terminal; a short wire from the Ye contact goes to the Pur terminal (?). It is not the SEL2-switched terminal.
### Output transformer
- "OUTPUT TX  C2668". Primary: **Rd** (red) to V4/V5 plates, **Wh** (white) centre tap to HT after the choke, **Bn** (brown) to V6/V7 plates (printed "Rd", "Wh", "Bn", "Bn"). No primary impedance printed.
- Secondary taps printed: **Gn 16 ohm**, **Bn 8 ohm**, **Ye** (wire to the Pur terminal for the feedback), **Bk** (common) and **4 ohm**. SEL2 (impedance selector, three positions drawn: 16 / 8 / 4 ohm) goes to speaker jacks **JS5, JS6** ("Speakers").
- DI/slave-type output: **R41 2k7** from the Bk/4 ohm node down, **R42 100 ohm** to ground, and **R43 560 ohm** from the R41/R42 junction to jack **JS7** printed "D1" (probably "DI" = direct out) (?).
### Supply
- Mains transformer "T4145". Mains: **S1** (two-pole mains switch, dashed box), **F1 T2A** fuse (primary), mains inlet **MS1** with terminals **N E L**, a pilot lamp (circle) on the primary. **SEL1** voltage selector: **240V / 220V / 120V** taps (labels "240V", "220V", "120V"; windings "Bl" / "Rd" / "Or" / "Vi").
- HT winding: leads "Bl" (top), "Gy" (bottom), "Ye - C.T. unused" printed (the centre tap lead is not used). **S2** (two-pole, dashed link) is wired between the HT winding ends and the bridge (function not labelled) (?).
- Bridge: **D2, D3, D4, D5 = 4 x 1N4007**. After the bridge: **C21 50+50uF** (drawn as two electrolytics from the HT line to a common ground symbol, i.e. a dual can: two 50 uF in parallel on the same node), **HT fuse F2 T1A**, **T3 Choke** (inductance/resistance not printed), HT node feeding the OT centre tap and the screen resistors; **C20 50+50uF** from the post-choke node to ground; **R30 10k 1W** from that node to the X (preamp HT) line.
- Heaters: windings "FILAMENTS 6.3V~" (Bk, Gn, Wh leads; arrows to the tube heaters).
- B+ voltages: not printed on either sheet.

## 5. Not legible / uncertain
- Last digit of the issue 5 date (7-12-88?), D1 orientation, exact connection of R28/R29/RV1, S2 function, which winding feeds the bias, OT primary impedance, choke value, whether JS7 is "DI".
