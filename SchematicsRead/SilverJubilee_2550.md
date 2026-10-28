# Marshall Silver Jubilee 2550 STD (25/50 W; notes: also 2553 and 2558) - schematic read

- Source file: `2550amp.gif` (1 page, 1925x1355, clean CAD drawing; zoomed crops used).
- Title block (printed): "2550 STD" (DRAWING NUMBER), MARSHALL, "JIM MARSHALL PRODUCTS LTD., BLETCHLEY, MILTON KEYNES, ENGLAND", file "2550.DGM", copyright (C).
- Revision table (printed): **ISS. 2, DATE 6-6-88, DRN. CPV**; **ISS. 1, DATE (blank), DRN. ICE**.
- Notes box (printed): "Notes: V1,2,3: ECC83 / V4,5: EL34 / PCB: JM112D / See also PSU diagram". Handwritten: "Also 2553+2558". The PSU diagram is not on this sheet (the HT feed arrives on "To HT fuse" and the bias on "D").
- Output label: "OUTPUT 25/50W"; "TX: 784-139" (output transformer).
- Units: ".022" etc. uF; "470p", "47p", "4n7", "2n2", "1n" etc. = pF/nF; "10u"; "1K5", "2K7" = 1.5k, 2.7k; "5Kb" = 5k6 (font draws 6 as b); "K" = kohm; "R19 10 ohm".

## 1. Tube complement
| Pos | Tube | Function |
|---|---|---|
| V1A | ECC83 (V1,2,3 = ECC83) | First gain stage |
| V1B | ECC83 | Second gain stage |
| V2A | ECC83 | Third gain stage (lead/rhythm clipping stage) |
| V2B | ECC83 | Fourth stage: drives the tone stack from its plate (this sheet draws V2B as a gain stage with a 220K plate load and 470 ohm cathode; the tone stack hangs off its plate) |
| V3A, V3B | ECC83 | Long-tailed phase inverter |
| V4, V5 | EL34 | Push-pull output (V5 is drawn inverted) |
| Rectifier | on the PSU sheet (not shown); D8/D9/D10 1N4007 on this sheet | |

## 2. Preamp
### Input and V1A
- Input jack **J1** (switched); **C2 .047** series, **R25 68K** series to the grid; **R2 1M** from the jack side (after C2) to ground; **C33 47p** from the grid node to the cathode (read as drawn) (?).
- Plate load **R26 100K** to HT; cathode **R1 2K7** in parallel with **C1 .68u**.
- Plate coupling **C3 .022** to the top of **VR1 Gain 1M LOG**; **C5 .001** from the VR1 top to the VR1 wiper (bright cap across the top of the pot).
### V1B
- VR1 wiper -> **R27 100K** -> grid. **R6 220K** from the grid to the cathode line (cathode node). Cathode **R3 1K5** in parallel with **C4 10u** (+ marked).
- Plate load **R28 100K**; **C19 470p** across R28 (from the HT rail to the plate node, read as drawn) (?).
- Plate coupling **C20 .022**; the following node has **R7 220K** to ground and **R29 47K** to the clipping rail.
### Lead clipping and Lead Master
- Clipping rail (node after R29 47K) is shunted to ground by two diode strings: **LED3 + D1 + D2** (3 in series, one polarity: LED3, D1, D2 marked "3x 1N4007"? the label "3x 1N4007" sits beside D1/D2/D3 and "2x 1N4007" beside D4/D5) and **LED2 + D3** (the opposite polarity); **C21 1n** across the rail to ground.
- **VR2 Lead Master 220K lin** from the clipping rail to ground; wiper -> **R11 47K** -> V2A grid node.
- Rhythm/lead selection: **R8 10K** and **R10 100K** (R8 from the V1B line through the changeover contact (relay RL1: "Pull Boost Sw on VR3" operates the relay) to **R10 100K** to the V2A grid node). Rhythm clip: switch on VR1 ("Switch on VR1", "Pull Rhythm Clip") connects **R9 47K** from the V2A grid node to the node with **D4 / D5 (2x 1N4007, anti-parallel)** and **C6 2n2** to ground. (Exact contact wiring of the lead/rhythm changeover is drawn but dense (?).)
### V2A
- Grid node: R11 (47K, from Lead Master wiper), R10 (100K), R9 (47K), and **R12 1M** from the grid node to the V2A plate node (grid-to-plate resistor, read as drawn) (?).
- Plate load **R35 100K** to HT; cathode **R32 1K5** to ground, no bypass drawn.
- Plate -> **C24 .1** -> **R34 100K** -> series effects loop: **Send J3** (tip jack with **R33 10K** to ground and **R53 10 ohm**), **Return J4** (with **C25 .1** and **R36 1M** to ground), then to the V2B grid.
### V2B and tone stack (Treble 220k lin / Mid 100k log / Bass 1M log tandem)
- V2B: plate load **R38 220K**; cathode **R37 470** (unbypassed); grid fed from the return jack/C25.
- From the V2B plate: **C26 4n7** to node M; **C27 .1** to the top of **VR6 back (1M LOG)**, its wiper connected to node M; **R16 100K** from VR6-back's bottom to ground ("tandem" with the bass pot VR6 front).
- **C8 220p** from node M to the top of **VR4 Treble 220k LIN**.
- **R13 100K** from node M down to node N1; **C9 .1** from N1 to **R14 10K** to node N2 (N2 is joined to the treble-pot bottom / bass top node); **C10 .01** from N1 to the wiper of **VR5 Mid 100K log**; **C11 470p** from N2 to the VR5 top; VR5 bottom to ground.
- **C7 2n2** from N2 to ground; **R15 100K** from the treble-bottom node to the top of **VR6 front Bass 1M LOG** (rheostat, wiper tied to its top); VR6 front bottom to ground. VR6 front and back are mechanically tandem (dashed link).
- Treble wiper -> top of **VR3 Output Master 1M LOG**; VR3 wiper = output "B" -> PI via C28.
### Preamp supply
- **R49 10K 1W** with **C34 2x 50u** (two 50 uF electrolytics, + marked): feeds the V1A/V1B/V2A loads at the left rail; the V2A plate and V2B plate loads and the tone stack hang from the "A" rail at the right end of the top line.
### Footswitch / heater DC section (top-left strip)
- **FS1 / J2 FOOT SW**; **R30 330**, **R31 330**, **LED1** (indicator); **C23 22u**; **D9 1N4007** across the relay supply; **RL1** relay coil; **C22 470u** (+ marked); **D8 1N4007**; leads marked "TO HEATERS" (a DC supply derived from the heater winding for the relay/LED) (?). "Pull Boost Sw on VR3" operates the relay.

## 3. Phase inverter V3 and feedback
- **C28 .022** from B to the V3A grid; **R39 1M** from the V3A grid to the junction; **R41 1M** from the junction to the V3B grid; **C29 .1** from the V3B grid node to the NFB node (to ground side); **R40 470** from the common cathode to the junction; **R17 10K** from the junction to the NFB node.
- Plate loads: **R45 82K** (V3A) and **R46 100K** (V3B). Coupling: **C30 22n** and **C31 22n** (0.022u) to the output grid networks. **C15 47p** from the V3A-side grid node to the V3B-side grid node (between the two coupled lines).
- Grid leaks: **R21 220K** and **R? 220K** (lower, no designator read), the two in series between the two output grid lines; their centre node is the bias node. Bias network on this node: **R22 22K** preset (printed "R22 / RV1 22k", variable), **R47 15K**, **R23 56K**, **C16 10u**, **C17 10u** (+ marked) and **D10 1N4007** into the line "D" (bias supply from the PSU sheet).
- Grid stoppers: **R54 5k6** (V4, grid node labelled "Gn") and **R55 5k6** (V5, node "Org") (printed "5Kb").
- Feedback: **R44 100K** from the OT tap "Pur" (purple) to the NFB node; **R20 4k7** from the NFB node to ground; **VR7 Presence 22K LIN** with **C14 .1u** in series from the NFB node to the R19 end; **R19 10 ohm** to ground (C ground).
- DI: **R43 27K** from the Pur line to a node with **R18 680 ohm** to ground and **C13 47n** (blobbed "47n"), then **R42 680** and **C12 47n** to jack **J5 "D.I."**.
- PI supply: **R50 10k 1W** and **R51 10K 1W**, each followed by a **C35 50u** electrolytic (two C35 50u on the sheet), feeding the PI plate loads R45/R46 (order as drawn (?)).

## 4. Output stage
- Two **EL34 (V4, V5)**, class AB push-pull; cathodes of V4 and V5 joined and to the bias/ground line (fixed bias from "D").
- Screen resistors **R56 1K5W** (V4) and **R55 1K5W** (V5) (printed "1K5W", i.e. 1.5 k? or 1k 5W (?)). Note: the label "1K5W" is the same text style as "1K5" used for 1.5 k resistors elsewhere on the sheet (R3 "1K5", R32 "1K5"), so it is ambiguous between 1.5 kohm 5 W... and 1 kohm 5 W (?). The Marshall 1959/2203 sheets print "1k 5W" with a space.
- **S3 HI LOW SWITCH** (two-pole, three terminals per pole; labelled "H" and "L"): in H the screen resistors go to the HT node, in L the screen resistors go to the plates (read as drawn; the L contacts join the plate lines) (?).
- Output transformer "TX: 784-139" with T2 label: primary "RD" (V4 plate), "BN" (centre tap to the HT after the choke), "WH" (V5 plate); secondary taps **16 ohm**, **8 ohm** (via SEL2 to jacks J6 and J7), **4 ohm**, with "OR" common (to ground). Output label "OUTPUT 25/50W". No primary impedance printed.
- HT: "T3 choke" (no value printed) between the HT fuse feed ("To HT fuse") and the OT centre-tap / screen supply node. B+ voltages not printed; no voltage table.

## 5. Not legible / uncertain
- Duplicate designator: the sheet prints R55 on both the V5 grid stopper (5k6) and a screen resistor (1K5W); the screen resistors are read as R56 (V4) and R55 (V5) in section 4, so one R55 label may be misread.
- R6/R7 grid-leak tie points, exact wiring of the lead/rhythm relay and switch contacts, C19 and C33 connections, unlabelled lower 220K grid leak, R55/R56 "1K5W" meaning, S3 contact wiring, C13/C12 47n values, DI wiring, supply-resistor order.
