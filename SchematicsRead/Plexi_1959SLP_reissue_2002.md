# Marshall 1959SLP-01 (2002 reissue of the 1959 Super Lead, 100 W) - circuit diagram read

- Source file: `1959-01-60-02.pdf` (1 page, A3, vector CAD; no text layer except the print stamp; read from a 220 dpi render; high confidence unless marked (?)).
- Print stamp (top): "Printed: 02 Apr 2002 12:05" and file path "v:\st1\1959-01-60-02.dgm".
- Frame: grid zones A-H (columns) and 1-6 (rows) printed on the border. Side box rotated: "DWG.No. 1959-01-60-02".
- Title block (printed): DRAWN "M BALLINGER", DATE "5-3-02"; MODEL "**1959SLP-01**"; revision table: **ISS 2 | ECO NUMBER 2100 | DATE 28-3-02** (rows above blank); TITLE "CIRCUIT DIAG."; DWG.No "**1959-01-60-02**"; ISS "**2**". Company: "Marshall (script logo)", "MARSHALL AMPLIFICATION PLC, DENBIGH ROAD, BLETCHLEY, MILTON KEYNES, MK1 1DQ, TEL (01908) 375411 FAX (01908) 376118".
- Box notes printed: "MAINS FUSE: T3.15A 230V / T6.3A 110V"; "VOLTAGE SELECTOR" with taps 230, 220, 110; "MAINS INPUT" (L, E, N); "INDICATOR" (6V red lamp); "HUM BALANCE" (VR2); "BIAS ADJUST" (VR1); "H.T. FUSE T1A"; "IMPEDANCE SELECTOR"; "SPEAKER OUTPUTS"; "EFFECTS CARD" box (LEVEL, BYPASS, RETURN, SEND, +30V W1, 0V W2, W3, W4); "LOUDNESS 1", "LOUDNESS 2", "TREBLE", "BASS", "MIDDLE", "PRESENCE", "INPUTS 1", "INPUTS 2".
- Units: "22n" 0.022 uF, "3n3", "4n7", "100n", "680n", "330u" uF, "470p", "47p" pF; "820R", "470R", "100R" ohm; "K" kohm; pot codes "A1M" (log), "B250K", "B25K", "B5K" (linear), "20K" preset.
- No DC voltage table or node voltages printed.

## 1. Tube complement
V1 ECC83 (two halves: one per channel), V2 ECC83 (V2 first half = mixer/gain stage, second half = cathode follower, direct-coupled), V3 ECC83 (long-tailed phase inverter), V4, V5, V6, V7 EL34 (four, fixed bias), rectifier = bridge D2-D5 (silicon). Valve base pin numbers are printed (V1 pins 1/2/3 and 6/7/8, etc.). The heater X, Y, X marks show a 6.3 V centre-tapped feed (heater winding not drawn in detail).

## 2. Preamp
### Inputs
- INPUTS 1 and INPUTS 2, each with a HI and a LO jack. Each jack: **R 68K** in series ("FITTED TO JACK SOCKET"): R3 (INPUTS 2 HI), R4 (INPUTS 2 LO) feed V1 pin 2 (left half); R5 (INPUTS 1 HI), R6 (INPUTS 1 LO) feed V1 pin 7 (right half). **1M** from each input pair's junction to ground (two 1M, unnumbered).
### V1 (two halves)
- Left half (pins 1/2/3, Inputs 2, "Loudness 2"): plate load **R7 100K**; cathode **R2 820R** in parallel with **C2 330u** (+ side to cathode). Plate coupling **C3 22n** to the top of **Loudness 2 pot A1M**; no bright cap.
- Right half (pins 6/7/8, Inputs 1, "Loudness 1"): plate load **R8 100K**; cathode **R1 820R** in parallel with **C1 680n**. Plate coupling **C4 3n3** to the top of **Loudness 1 pot A1M**; **C5 4n7** from the pot top to the wiper (bright cap).
- Both cathodes are 820R; the two halves have different bypasses (330u vs 680n).
### Mixer V2 (first half, pins 6/7/8)
- Loudness 1 wiper -> **R9 470K** -> V2 pin 7 grid node; **C6 470p** across R9 (from the Loudness 1 wiper to the grid node).
- Loudness 2 wiper -> **R10 470K** -> the same grid node.
- Plate load **100K** (unnumbered, "100K"); cathode **R11 820R** in parallel with **C7 680n** (bypassed).
- Plate direct-coupled to the V2 second-half grid (pin 2).
### Cathode follower (V2 second half, pins 1/2/3)
- Plate to HT; cathode **R12 100K** to ground; output (pin 3 node "X") to the tone stack.
### Preamp supply
- **R14 10K 1W** with a **50uF+50uF 500V** electrolytic pair (V1 plate loads, node) and **R15 10K 1W** with a **50uF+50uF 500V** pair (V2 plate loads and V3 plate loads), in series back to the HT.

## 3. Tone stack
- **C8 470p** from the stack input to the top of the Treble pot (**B250K**, "TREBLE"); **R13 33K** from the stack input to node B; **C9 22n** from node B to the treble-bottom / bass-top junction; **C10 22n** from node B to the Middle wiper; **A1M** Bass (rheostat); **B25K** Middle to ground.
- Treble wiper -> **W4** (to the effects card SEND); return from the card **W3** -> **C11 22n** -> V3 pin 2 grid. (When the effects card is not fitted the W3/W4 link is as drawn on the sheet: "EFFECTS CARD" box with level/bypass/return/send.)
- No master volume pot on this sheet.

## 4. Phase inverter V3 (ECC83: first half pins 1/2/3, second half 6/7/8)
- Input **C11 22n** to the pin 2 grid; **R16 1M** from pin 2 grid to the junction; **R19 1M** from the junction to the pin 7 grid; **R17 470R** from the common cathode to the junction; **R20 10K** from the junction to the NFB node; **C13 100n** from the pin 7 grid node to the NFB node.
- Plate loads: **R22 82K** (pin 1) and **R25 100K** (pin 6), both from the HT node N2; **C15 47p** between the plates.
- Coupling: **C14 22n** and **C16 22n** to the output grid lines; **R23 220K** and **R24 220K** in series between the two grid lines; their centre node is the bias line (to the C17 / R29 junction).

## 5. Output stage (four EL34)
- V4, V5, V6, V7: control grids (pin 5) through **1K5** grid stoppers; screens (pin 4) through **1K 5W** resistors from the HT bus; suppressor (pin 1) tied to the cathode line (as drawn) (?); cathodes (pin 8) to the ground line (earth symbol on the cathode bus) - fixed bias.
- HT bus feeds the **R26 10K 1W** and **R27 10K 1W** in series from the screen/HT bus to the preamp node N2 (as drawn (?)); preamp node then through R15 and R14 (section 2).
- Output transformer: primary centre tapped to HT; secondary with taps; "IMPEDANCE SELECTOR" with **16** and **8** (ohm) taps (a 4 ohm tap is not clearly drawn (?)); "SPEAKER OUTPUTS": two parallel jack pairs (T and S terminals). No primary impedance, no transformer type number printed.
- Feedback: **R21 47K** from the **16 ohm** secondary tap to the NFB node (the long wire across the top of the sheet).
- Presence: **B5K** pot from the NFB node to ground with **C12 100n** from the wiper (as drawn) (?); no other NFB-to-ground resistor (the 1959 R25 4K7 is not present).

## 6. Bias supply
- LT winding -> **R30 27K** -> **D1 1N4007** -> node with **C18 10u** to ground -> **R29 15K** -> node with **C17 10u** to ground (this node is the bias line to R23/R24) -> **R28 56K** -> **VR1 20K** ("BIAS ADJUST") preset to ground.

## 7. HT supply and mains
- Mains: L / E / N inlet ("MAINS INPUT"), mains fuse "F" (**T3.15A 230V / T6.3A 110V**), "MAINS" two-pole switch, "VOLTAGE SELECTOR" with taps **230 / 220 / 110**.
- HT winding: two ends through the **STANDBY** two-pole switch to the bridge **D2, D3, D4, D5** (type not printed); winding centre tap connected as drawn to the capacitor midpoint (?). HT fuse "F" **H.T. FUSE T1A** after the bridge.
- Reservoir: **50uF+50uF 500V** twice (four electrolytics: an upper pair and a lower pair, in series between HT and ground, centre node tied to the transformer centre tap/bridge as drawn) (?).
- Choke: drawn (inductor symbol, no value) after the reservoir; after it the HT node feeds the OT centre tap; **56K 1W** and **56K 1W** (two) in series with another **50uF+50uF 500V** pair each leg (a series divider/charge string, two pairs of 50+50uF) feeding **R33 47K** (printed with "10W") to "**W1**" (+30V supply to the effects card).
- Indicator lamp (6V red, X / N.C. / Y winding): **R31 100R**, **R32 100R** and **VR2 100R "HUM BALANCE"** (wiper to ground), connected across the heater winding.
- Valve heaters: X, Y, X marks on every valve base (centre-tapped 6.3 V heater supply; winding/voltage not drawn).
- B+ voltages not printed.

## 8. Not legible / uncertain
- Exact HT bridge / centre-tap / capacitor-midpoint connections, 4 ohm tap, the 56K/56K 1W chain role, whether C12 100n is wiper-to-ground, EL34 pin 1 connection, whether R26/R27 are series or parallel as drawn (series read).
