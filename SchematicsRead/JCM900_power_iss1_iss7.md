# Marshall JCM900 Series 100 W power amp (CD0192) - issue 1 and issue 7 read

- Source files: `cd0192-100-iss1.pdf` (issue 1) and `cd0192-iss7.pdf` (issue 7); each 1 page (A3, vector drawings with no extractable text layer; read from 220 dpi renders). Clean CAD drawings, so values are read with high confidence unless marked (?).
- Drawing number printed: **CD0192** (both). Title block: "JCM900 Series / 100w Power Amp / Circuit Diagram" plus a variant line.
  - Issue 1: variant line "**ALL COUNTRIES**"; file name "**900-100.DGM**"; revision table: **ISS. 1 | DATE 31-10-89 | DRN. CPV** (only row). No "Models" line.
  - Issue 7: variant line "**NOT SEMKO**" (the plotter overprinted "NOT" and "SEMKO", reads "NOTSEMKO"); file name "**CD0192.DGM**"; revision table (top to bottom): **7 | 2-7-90 | CPV**, **6 | 16-5-90 | CPV**, **5 | 9-3-90 | CPV** (rows for issues 1-4 blank). Bottom-left text: "**Models: 2100, 2101, 4100, 4101 & 4102**".
- Company block: Marshall, "Jim Marshall (Products) Ltd., BLETCHLEY, MILTON KEYNES, ENGLAND", (C).
- Notes (issue 1, printed): 1. D1-5: 1N4007. 2. PCB: **JMP50A**. 3. Effects loop components shown on preamp cct. diag. 4. Output impedance select can be wired for either 8 & 16 ohm (normal) or 4 & 8 ohm.
- Notes (issue 7, printed): 1. D1-5: 1N4007. 2. PCB: **JMP50B**. 3. Effects loop components shown on preamp cct. diag. 4. Output impedance select can be wired for either 8 & 16 ohm (normal) or 4 & 8 ohm. 5. V3: ECC83; V4-7: EL34. 6. Undesignated components are off-PCB components. 7. All resistors are 1/4 w unless stated otherwise.
- Units: "22n" = 22 nF = 0.022 uF; "47p", "100p", "220p", "4n7", "1n", "2n2", "0.1u"; "10u 100v" electrolytics; "2x 50+50uF 500v".
- Wire colour codes printed at test points: Bl, Gn, Ye, Bn, Bk, Or, Pk, Wh, Rd, Vio.

## 1. Tube complement
V3 (V3a, V3b) ECC83 long-tailed phase inverter; V4, V5, V6, V7 EL34 (four, push-pull); no preamp tubes on this sheet (the heater-wiring drawing in issue 7 shows "3x ECC83" and "2x EL34 (A)", "2x EL34 (B)").

## 2. Phase inverter V3 (both issues)
- Input: from the preamp PCB via shielded "Twin Scnd from Front PCB" ("Input", "Rd" lead; screen); **C7 22n** to the V3a grid ("Gn" point).
- Cathodes joined ("Ye" point): **R19 470 ohm** to the junction; **R20 1M** from V3a grid to the junction; **R21 1M** from junction to the V3b grid; **C8 0.1u** from the V3b grid node to the feedback node ("Presence" line).
- **R18** from the junction down to the node on the "Presence" line: **10k in issue 7, 4k7 in issue 1**; below it **R6 4k7** to ground (earth to preamp PCB, "Bk" point). **C5** from the Presence line to the presence control: **MVs 0.1u, DRs NOT FITTED** (issue 7); **0.1u** (issue 1).
- Plate loads: **R22 82k** (V3a) with **C9 across it (issue 7: 100p; issue 1: 220p)**; **R23 100k** (V3b). Plates at the "Bl" points. **C10 between the plates: issue 7 100p; issue 1 47p**.
- Coupling: **C11 47n** (from V3a plate) and **C12 47n** (from V3b plate), each to a node that feeds the grid networks below.
- **R29 220k** and **R28 220k** in series (both issues): from the C11 node (upper) to the C12 node (lower) with centre node to the lower node chain, i.e. the two output-grid nodes are separated by two 220k (R29 between the upper node and the middle node, R28 between the middle and the lower node); the nodes feed the grids via the stoppers below.

## 3. Output stage
- Four EL34: V4, V5 (upper row), V7, V6 (lower row). Cross-wiring between tubes (diagonal wires) as drawn.
- Grid stoppers:
  - **Issue 7:** **R25, R24, R27, R26 each 1k5 in series with 2k2** (labels "1k5" and "2k2" on each leg, with a test point "Gn" / "Or" between the two resistors) => 3.7k each.
  - **Issue 1:** **R25, R24 = 22k** (V4 side, "Gn" wires) and **R27, R26 = 4k7** (V7 side, "Or" wires).
- Screen resistors: **R33, R34, R35, R36**: **issue 7: 2k2 4W**; **issue 1: 2k2 5W**.
- Output power select (**SEL2**, two-pole "High / Low", dashed link): the screen resistor pairs (R33/R34, R35/R36) go to the switch common; in "High" the common goes to the screen-supply HT line ("Rd") and in "Low" it is connected to the line to the "Wh"/"Ye" terminals at the bottom (as drawn) (?), which halves the active tube pair.
- Cathodes: Bn / Bk test points for the two pairs; cathodes to the common ground line (fixed bias); no cathode resistors.
- Bias: **PR1 22k** preset ("Bias Preset"), **R30 56k**, **R31 15k**, **R7 56k**, **C13 10u 100v**, **C14 10u 100v**, **D1** (in the "D1-5 1N4007" note), **C15 47n Class X** (same in both issues).
- Output transformer: issue 7 prints "**Output Transformer C3070**"; issue 1 prints "Output Transformer" only (no number). Primary: "Rd" (upper plates), "Bn" centre, "Wh" lower (plates); secondary taps **16 ohm, 8 ohm, 4 ohm** with **SEL1 "Output Impedance Select"** to **JS5, JS6 Speaker Sockets** (see note 4). Secondary also has "Gn", "Ye", "Vio" leads; no impedance numbers on the primary.
- Presence/feedback: **R17** from the "Vio" feedback tap to the Presence line: **issue 7: "27k - MVs, 100k - DRs"**; **issue 1: 100k**. **C6** across R17: **issue 7: "220p - MVs, 100p - DRs"**; **issue 1: 47p**.
- Fuse-fail indicators (both issues): **FS1 T500mA** and **FS2 T500mA**, each with **R8 100k / R9 100k** and **LED1 / LED2 "Fuse Fail"**.

## 4. HT supply
- Bridge **D2, D3, D4, D5 (1N4007)** from the "Bl" (HT winding) leads through the **Standby Switch** ("Bl"); reservoir **2x (50+50 uF) 500 V** (two cans of two 50 uF; printed "2x 50+50uF 500v"), star earth (centre) "Star Earth" with "Bk" and "Or" leads.
- HT line: **R32 10k 1W** ("Metal Oxide" in issue 7) with **R37 100 ohm: issue 7 4W, issue 1 5W** and **FS4 fuse: issue 7 "F3A Type 275 Picofuse"; issue 1 "T3A Picofuse"**. Issue 1 additionally shows the label "**LINK**" on the top wire (a jumper) at the top-left rail; issue 7 shows a plain wire.
- "HT to Preamp" tap after FS2 (both issues).
- B+ voltages: not printed.

## 5. Effects loop / DI circuit (same in both issues)
- **R16 10k**, **C4 47n**, **R15 10k // C3 4n7**, **R13 22k**, **R12 22k**, **C2 1n**, **C1 2n2**, **R14 33k**, **R5 100 ohm** (JS4 "Direct Out"), **R4 100 ohm** (JS3 "Record Out"). ("Effects loop components shown on preamp cct. diag.")

## 6. Mains / heaters
- **Issue 7:** mains inlet MS "Line / Earth / Neutral", label "220v~" and "Mains Inlet - Voltage as Marked on Rear Panel"; **FS3 "117v - T4A  234v T2A"**; mains switch; primaries linked: "**A - 117v, B - 234v**" ("Primaries linked for Line Voltage"); neon type "**1610INRB5NKR220V**"; mains transformer "**T5826**". Chassis connection: **0.1u 250v** in parallel with **R "MVs - 100 ohm, DRs - 22 ohm, 1/2w"**, and **100 ohm 1/2W** to the chassis. Heater fuses **FS6, FS7 "T6.3A 20mm"**, "Heater Fuses", to heater filaments (series/parallel wiring diagram: "Valve Heater Wiring Arrangement": 3x ECC83, 2x EL34 (A), 2x EL34 (B)). LT secondary to preamp PCB: **Wh 20v - Bk 0v - Wh 20v**.
- **Issue 1:** mains inlet Line/Earth/Neutral; **NTC thermistor**; **FS5 "Internal Fuse CSA Only  117v - TxA, 234v - TxA"**; **FS3** (TxA placeholder); same primary linking "A - 117v, B - 234v"; "Mains Transformer" (no number printed); "Voltage as marked on Read[sic] Panel"; heater fuses **FS6, FS7 "Heater Fuses - SEMKO & CSA only"** to valve heaters "**3.15v**" each side; LT secondary **Rd 20v - Bk 0v - Rd 20v**; direct chassis earth plus star earth (no 0.1u / 100 ohm network).

## 7. Not legible / uncertain
- Exact contact wiring of the High/Low screen switch (as drawn); which grid stopper belongs to which tube through the diagonal wires; whether "MVs" and "DRs" mean master-volume models and dual-reverb models (not expanded on the sheet); fuse values "TxA" in issue 1 (placeholders, not values).
