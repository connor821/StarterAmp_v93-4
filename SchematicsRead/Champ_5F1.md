# Fender Champ-Amp 5F1 - schematic + layout read

Source: `2e6ee341-fender_champ_5f1.pdf`, 2 pages (1306x994 / 1147x988 rasters). p1 "FENDER 'CHAMP-AMP' SCHEMATIC, MODEL 5F1", drafting code "K-EE". p2 "FENDER 'CHAMP-AMP' LAYOUT, MODEL 5F1" with "NOTICE: voltages read to ground with electronic voltmeter, values shown +/- 20%". No date/revision.

Caps "value-voltage" in uF (".02-600", "25-25" = 25 uF/25 V, "8-450", "16-450").

## 1. Tubes
**12AX7** (V1a, V1b), **6V6GT** (single-ended), **5Y3GT**.

## 2. Stages
- Inputs: jack 1 tip -> **68K**; jack 2 tip -> **68K**; both 68K to one grid node (V1a grid, direct, no coupling cap). **1 MEG** from the jack-1 line to ground. Jack-2's spring tied to jack-1's line (normalling, (?)). Effective DC grid return = 68K + 1 MEG.
- **V1a:** plate load **100K** to B+ node 3; cathode **1500 ohm to ground, unbypassed** (no bypass drawn). Plate -> **.02-600** -> volume pot.
- **Volume:** one pot **1 MEG** (layout: "1 MEG."; taper not stated): cap to the pot's top, bottom to ground, wiper -> V1b grid (conventional wiring).
- **V1b:** grid = volume wiper (no grid stopper). Plate load **100K** to B+ node 3. Cathode **1500 ohm to ground, unbypassed**, which also takes the NFB tap (below). Plate -> **.02-600** -> 6V6 grid.

## 3. Tone
None.

## 4. Tremolo
None.

## 5. Phase inverter
None - single-ended.

## 6. Output stage
- 6V6GT: grid leak **220K** to ground (no grid stopper); cathode bias **470 ohm || 25-25** (25 uF/25 V); screen directly to B+ node 2; plate to OT primary; OT CT to B+ node 1. OT: one secondary to a speaker jack (shorting) and speaker; impedance not printed.
- **Global NFB:** **22K** from the OT secondary's ungrounded terminal to **V1b's cathode** (with its 1500 ohm to ground): divider 22K : 1500 ohm (derived feedback fraction 1500/23500 ~ 6.4%).
- Printed DC voltages (layout, +/-20%): **6V6 cathode +18 V** ("+18 V"), node 1 / OT CT / 6V6 plate **+340 V**, screens / node 2 **+295 V**, preamp node 3 **+250 V**, **V1 plates +150 V, +150 V**, **V1 cathodes +1.5 V, +1.5 V**.
- Layout callouts: ".02 MFD 600 VOLTS" x2, 1500 x2, 100K x2, 68K x2, 220K, 470, 22K x2, 25 MFD 25 VOLTS, 16/8/8 MFD 450 VOLTS, 10K, 1 MEG pot.

## 7. Power supply
5Y3GT; HV winding with grounded CT; 5 V winding. B+: [node 1 **16-450**, OT CT, +340 V] -> **10K** -> [node 2 **8-450**, 6V6 screen, +295 V] -> **22K** -> [node 3 **8-450**, V1 plate loads, +250 V]. AC switch, **2 AMP fuse**, .05-600 to ground. 6.3 V "to all filaments and pilot lite". No transformer part numbers.

Derived checks: 18 V/470 ohm = 38 mA total; (340-295)/10K = 4.5 mA screen; (295-250)/22K = 2.0 mA preamp (matches 2 x (1.5 V/1500 ohm) = 2.0 mA).

## 8. Not legible / uncertain
- Jack normalling; pot taper; OT impedance; the "+150" vs "+160" last digit is read as 150 (consistent with 100K load); voltage tolerance is +/-20%.
