# Marshall JCM800 2203 STD (100 W master-volume lead) - preamp and power sheets read

- Source files: `2203prem.gif` (preamp, 2059x1416, CAD drawing) and `2203pwrm.gif` (output stage & PSU, 1979x1368, CAD drawing).
- Drawn by "JIM MARSHALL PRODUCTS LTD., BLETCHLEY, MILTON KEYNES, ENGLAND", copyright (C) mark.
- Preamp sheet: title block "2203 STD  Preamp"; file "2203PRE.DGM"; revision table: **ISS. 4, DATE 19-5-88 (the day digit prints "19"; the 1959 sheet prints "10-5-88"), DRN. CPV**; other rows empty. Notes box: "PCB: JM80" / "V1,2: ECC83, filament supply shown on power stage diagram." (no JS3,4 note).
- Power sheet: title block "2203 STD  Output Stage & PSU"; file "2203PWR.DGM" (blobbed); revision table, top to bottom: **ISS. 6, 7-12-88 (last digits blobbed: 88?), CPV**; **ISS. 5, 18-10-88 (blobbed "18-10-88" ?), CPV**; **ISS. 4, 19-5-88, CPV**. (Issue numbers read "6", "5", "4"; the 1959 power sheet has only 5 and 4 rows.)
- Units as the 1959 sheets: ".022u" uF; "470p" pF; "820n" = 820 ohm; "5Kb" = 5k6 (font draws 6 as b); "1n0" = 1 nF.
- Off-sheet nodes X (HT), Y (to PI via C9), Z (ground). No DC voltage table is printed on either sheet.

## 1. Tube complement
| Pos | Tube | Function |
|---|---|---|
| V1a, V1b | ECC83 | Gain stages; **Preamp Vol pot between V1a and V1b** |
| V2a | ECC83 | Third gain stage |
| V2b | ECC83 | Cathode follower, feeds the tone stack |
| V3 | ECC83 | Long-tailed phase inverter |
| V4-V7 | EL34 (printed "V4-7: EL34") | Push-pull output, 4 tubes |
| Rectifier | 4 x 1N4007 | Bridge |

## 2. Preamp (2203 STD, issue 4)
### Inputs
- Two jacks: **J1 "High"** and **J2 "Low"**. J1 -> **R3 68k** -> V1a grid. **R2 1M** from J1 contact node to ground. J1's second contact (switch) is wired to the "Inputs" ground/sleeve node (drawn left).
- J2 "Low": its contact line runs to the junction **R5 470k / C4 470p** (see below); J2's normalling contact is wired to **C3 .022u** which goes to the V1a plate. So with nothing in J2, V1a plate -> C3 -> J2 switch -> R5 // C4 -> Preamp Vol top; plugging J2 breaks V1a's plate path and feeds the input straight to the R5/C4 junction (V1a is bypassed) (as drawn; reading of the switch contacts (?)).
### V1a
- Plate load **R4 100k** to HT. Cathode **R1 2k7** in parallel with **C1 .68u** (+ side to the cathode). **C2 100p** from the V1a plate node to the V1a cathode node (plate-to-cathode, as in the 1959 V1b).
- Plate coupling: **C3 .022u** (via J2 switch) to **R5 470k // C4 470p**; the bottom of that pair is the top of **VR1 1M log "Preamp Vol"**.
- **C5 1n0** (1 nF) from the VR1 top to the VR1 wiper (bright cap across the top of the pot).
- VR1 wiper -> V1b grid directly (no grid stopper resistor drawn).
### V1b
- Plate load **R7 100k**. Cathode **R6 10k**, no bypass. Plate -> **C7 .022u** -> node A.
- **R10 470k** from node A to the V2a grid node, with **C8 470p** in parallel (across R10). **R11 470k** from the V2a grid node to ground. (So a 470k / 470k divider with a 470 pF treble bypass on the series half.)
### V2a
- Cathode **R9 820 ohm** to ground, no bypass. Plate load **R12 100k**. Plate direct-coupled to the V2b grid.
### V2b (cathode follower)
- Plate to HT. Cathode **R13 100k** to ground. Cathode feeds the tone stack directly.
### Preamp HT filtering
- **R8 10k 1W** from the X node to the preamp node; **C21 50+50u** (two electrolytics to a common ground symbol) to ground. R4, R7, R12 and the V2b plate hang from this node.

## 3. Tone stack and master volume
- **C10 470p** from the V2b cathode to the Treble pot top; **R15 33k** from the cathode node to node B; **C11 .022u** from node B to the treble-bottom / bass-top junction; **C12 .022u** from node B to the Middle wiper.
- **VR3 220k lin** Treble (printed "220k lin"); **VR5 1M log** Bass (rheostat, wiper tied to the top end; printed "1M log"); **VR4 22k lin** Middle (printed "22k lin").
- Treble wiper -> a wire link printed **"link 'R14'"** (a fitted-link/ jumper position labelled R14) -> top of **VR2 1M log "Master Vol"**; VR2 wiper = Y (to the PI via C9); VR2 bottom to ground. So the master volume is **after** the tone stack.

## 4. Power sheet (2203 STD Output Stage & PSU, issue 6)
(Reference designators renumbered relative to the 1959 sheet; the circuit reads identical except presence pot.)
### Phase inverter V3 (ECC83)
- **C9 .022u** from Y to the left grid; **R17 1M** from that grid to junction J; **R19 1M** from J to the right grid; **C15 .1u** from the right grid node to the NFB node N.
- **R16 470 ohm** from the common cathode to J; **R20 10k** from J to N; **R23 4K7** from N to ground.
- **VR6 22k LIN** presence (printed "22K LIN") with **C17 .1u** in series from N to ground.
- Plate loads: **R18 82K**, **R21 100k**, to X. **C18 47p** between the plates.
- Plate coupling: **C14 .022u** and **C16 .022u**.
### Output stage
- Grid networks: **R24 220k** and **R25 220k** in series; centre = bias node. Test points "Ca" and "Or".
- Grid stoppers: **R31, R33, R32, R34 = 5k6** (printed "5Kb").
- Screen resistors: **R35, R37, R36, R38 = 1k 5W** (printed "1k 5W").
- Four EL34 (V4-V7). No cathode resistors drawn (fixed bias).
- Bias: **D1 1N4007**, **R30 27k**, **C20 10u** (electrolytic), **RV1 22K** preset, **R26 47k**, **R27 15K**, **C19 10u** (same arrangement as the 1959; connection detail as drawn is dense (?)). The bias line returns to the "Wh" lead at the bottom of the mains transformer's low-voltage winding (voltage not printed) (?).
- Feedback: **R22 100k** from the OT "Ye/Pur" tap to N (same arrangement as the 1959).
### Output transformer
- "OUTPUT TX  C2668"; primary leads "Rd", "Wh" (centre tap to HT), "Bn", "Bn"; secondary taps **Gn 16 ohm**, **Bn 8 ohm**, **Ye** (to Pur), **Bk**, **4 ohm**; SEL2 selects 16 / 8 / 4 ohm to speaker jacks **JS3, JS4** ("Speakers"). DI output: **R39 2k7**, **R40 100 ohm**, **R41 560 ohm** to jack **JS5** printed "DI".
### Supply
- Mains transformer "T4145"; **SEL1** 240V / 220V (printed "228V"? blobbed "220V") / 120V; **S1** mains switch; **F1 T2A**; pilot lamp; MS1 socket N E L; **S2** (two-pole, dashed link) at the HT winding; "Ye - C.T. unused" printed.
- Bridge **D2-D5 4 x 1N4007**; **C23 50+50uF** (dual can to ground); **F2 HT fuse T1A**; **T3 Choke**; **C22 50+50uF** at the post-choke node; **R29 10k 1W** to X.
- Heaters: windings "FILAMENTS 6.3V~" (Bk, Gn, Wh).
- B+ voltages not printed.

## 5. Not legible / uncertain
- J2 switch contact wiring (preamp), meaning of "link 'R14'", dates of issues 5 and 6, D1 orientation, bias network exact wiring, choke inductance, OT primary impedance, whether "220V" or "228V" on SEL1.
