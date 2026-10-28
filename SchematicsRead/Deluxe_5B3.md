# Fender Deluxe 5B3 - schematic + layout read

Source files: `bc34e824-fender_deluxe_5b3.pdf` and `d31e878c-fender_deluxe_5b3_1.pdf`. **They are byte-identical (same MD5 d24769e3...)**, so one read covers both. 2 pages (grayscale scans ~150 dpi): p1 "DELUXE AMP 5B3 SCHEMATIC" (title prefix "FENDER MODEL"), p2 "DELUXE AMP 5B3 LAYOUT" (with printed DC voltages and part callouts). Typed-style title, hand-lettered parts; tube pin numbers printed. No date/revision code.

Caps printed "value-voltage" in uF (".05-600" = 0.05 uF/600 V, ".0005" = 500 pF, "25-25" = 25 uF/25 V, "20-450" = 20 uF/450 V).

## 1. Tube complement
Preamp V1 6SC7 (pins: plate 5 / grid 4 for upper triode, plate 2 / grid 3 for lower, cathodes pin 6); phase inverter V2 6SC7; output 2 x 6V6GT; rectifier 5Y3GT.

## 2. Preamp stages (V1 6SC7; both cathodes tied and grounded directly, no cathode R/C)
- **V1a (upper) channel 1, jacks 1+2:** jack 1 tip -> **50K** -> node; jack 2 tip -> **50K** -> same node; node -> **.05-600** -> grid (pin 4). Grid leak **5 MEG** to ground. Plate (pin 5) load **250K** to B+.
- **V1b (lower) channel 2, jack 3:** jack -> .05-600 -> grid (pin 3) directly. Grid leak **5 MEG** to ground. Plate (pin 2) load **250K**.
- Both plate loads tie to one B+ line which runs to the filter node at the left end of the rail (the first 20 uF node on the schematic, = the lowest B+ node, +250 V on layout).
- Each plate -> **.05-600** -> its volume pot. Volume pots: two x **1 MEG**, taper not printed. Drawn like the 5C3: coupling-cap line arrow lands on the pot (wiper symbol), one pot end to ground, the other end to the common mix node (no mixing resistors). (?) see 5C3 note on the wiper reading.
- Mix node -> PI grid 1 (pin 4).

## 3. Tone control (channel 1 only)
Node after V1a's .05-600 -> **.0005** -> top of **1 MEG** tone pot; pot lower end -> **.01** (10 nF, voltage not printed on schematic) -> ground; wiper -> mix node. On the layout the pot with the power switch is labelled "1 MEG SWITCH" (?: whether it is the tone pot cannot be confirmed from the picture) and a ".01 MFD 600 VOLTS" and a "0.0005 MFD" part are drawn at the top of the layout.

## 4. Tremolo/vibrato
None.

## 5. Phase inverter V2 (6SC7, self-balancing/paraphase, shared cathode)
- Cathode (pin 6, both triodes): **2500 ohm** || **25-25** (25 uF/25 V) to ground.
- Plate loads **250K** (pin 5) and **250K** (pin 2) to the same B+ node (+250 V node).
- Plate (pin 5) -> **.05-600** -> upper 6V6 grid (pin 5); plate (pin 2) -> **.05-600** -> lower 6V6 grid.
- Output-grid network (upper to lower): upper grid node -> **250K** -> tap (to PI grid 2, pin 3, by curved wire) -> **7000 ohm** (printed "7000") -> centre node (grounded) -> **250K** -> lower grid node. (Tap fraction 7000/257000 = ~2.7%, derived.)
- Layout page additionally shows a small cap across the PI socket between the two plate pins, text near-illegible: reads approximately ".000? MFD / 1000 VOLTS" (best guess ".0001 uF/1000 V = 100 pF) (?). It is NOT drawn on the schematic page.

## 6. Output stage
- 2 x 6V6GT push-pull, cathode bias: **250 ohm 5 W** || **25-25** (25 uF/25 V) to ground (cathode pin 8, both). No grid stoppers drawn. Screens (pin 4) go direct to the screen node. Plates (pin 3) to OT. 
- Voltages (layout): plate/CT node **+350 V** (read "+350", (?) vs 450 - looks like 350), screens **+280 V**, cathode **+18 V**.
- OT: one secondary to one speaker symbol; no speaker jack drawn, no impedance printed. **No NFB loop and no extra-speaker jack on this revision.**

## 7. Power supply
- 5Y3GT; layout text on the power transformer box: "POWER TRANSFORMER, 80 MILS (mA), SECONDARY VOLTAGE 700CT" (rotated, upside-down text; read as 700 V CT, 80 mA).
- Chain: [node 1 **20-450**: OT centre tap, +350 V] -> **10K** -> [node 2 **20-450**: screens, +280 V] -> **10K** -> [node 3 **20-450**: preamp + PI plate loads, +250 V]. Layout shows the same three 20 MFD 450 V caps and two "10K" resistors (layout box text blurry, "10 OHM"-looking; schematic says 10K).
- AC: AC switch, 2 AMP fuse, .05-600 from fuse junction to ground. 6.3 V "TO ALL 6.3 VOLT FILAMENTS".

## Printed DC voltages (layout page, "+/-" tolerance not printed on this page)
| Node | V |
|---|---|
| OT centre-tap / 6V6 plates | +350 (?) |
| 6V6 screens | +280 (?) (could be +260; read as 280) |
| Preamp/PI B+ | +250 |
| 6V6 cathode | +18 |
| PI plates (both) | +150, +150 |
| PI cathode | +2.1 (?) |
| V1 plates (both) | +92, +92 |
| V1 grids | "-9" / "-.9" (?) - likely -0.9 V (grid-leak bias) |
Derived sanity checks: 18 V / 250 ohm = 72 mA total cathode current; (250-150)/250K = 0.4 mA per PI triode, 2 x 0.4 mA x 2500 ohm = 2.0 V (matches +2.1).

## 8. Not legible / uncertain
- Plate-to-plate cap value on PI socket (layout): partly legible only.
- "+350", "+280", "+2.1" and the "-.9" signs/decimal.
- Which pot carries the switch; resistor values on the jack side in the layout (schematic says 50K).
- Wiper reading of volume arrows (as in 5C3).
- Pot tapers, OT impedance: not printed.

## Changes between revisions
- 5A3 (file) -> 5B3: input isolating resistors 7.5K (inherited art) -> 50K; PI/grid divider 250K/7.5K/250K -> 250K/7000/250K (nearly the same ratio); filter caps 16-450 -> 20-450; tone cap .005 -> .01; plate-to-plate PI cap appears only on the 5B3 layout (and on the typed 5A3 sheet).
- 5B3 -> 5C3: inputs 50K -> 7.5K, input caps .05-600 -> .1-200, tone cap .01 -> .005, filter caps 20 -> 16 uF.
- 5B3 -> 5C3: grid divider becomes 1 MEG/25K/250K and a 1 MEG NFB resistor (secondary -> upper grid) plus an extra speaker jack appear; 5B3 is the only one of the trio with a layout page and printed voltages.
