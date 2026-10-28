# Fender Princeton 5F2-A - schematic + layout read

Source: `51675a84-fender_princeton_5f2a.pdf`, 2 pages (1100x1017 / 1082x994 rasters). p1 "FENDER 'PRINCETON' SCHEMATIC, MODEL 5F2-A", code "K-EG", "NOTICE: all voltages read to ground with electronic voltmeter, values shown +/- 20%" (no voltages printed). Boxed parts table: **"POWER TRANS. - 66079; OUTPUT TRANS. - 265"**. "NOTE - all resistors 1/2 watt unless otherwise noted." p2 "FENDER 'PRINCETON' LAYOUT, MODEL 5F2-A" (no voltages either).

Caps "value-voltage" in uF (".02-400", ".0005", ".005", "25-25" = 25 uF/25 V, "8-450", "16-450").

## 1. Tubes
**12AX7** (V1a, V1b), **6V6GT** (single-ended), **5Y3GT**.

## 2. Stages
- Inputs: as Champ: jack 1 tip -> **68K** and jack 2 tip -> **68K** into a common grid node (V1a grid direct); **1M** from the jack-1 line to ground; jack-2 spring tied to jack-1 line (?).
- **V1a:** plate load **100K**; cathode **1500 ohm || 25-25** (25 uF/25 V) - bypassed. Plate -> **.02-400** -> node feeding both the volume pot and the tone pot wiper.
- **Volume:** **1 MEG LIN** (layout label "1 MEG. LIN."): top end at the signal node, bottom to ground, wiper -> V1b grid.
- **V1b:** plate load **100K**; cathode **1500 ohm, unbypassed**, carries NFB tap. Plate -> **.02-400** -> 6V6 grid.

## 3. Tone control
**1 MEG LIN** (layout label). Wiper to the signal node (same node as volume pot's hot end). One end -> **.005** -> ground; other end -> **.0005** -> V1b grid node (volume wiper). A treble-cut / bleed blend. Both pots are labelled linear on the layout.

## 4. Tremolo
None.

## 5. Phase inverter
None - single-ended.

## 6. Output stage
- 6V6GT: grid leak **220K** to ground (no stopper); cathode **470 ohm 1 W || 25-25** (25 uF/25 V); screen direct to B+ node 2; OT CT to B+ node 1. Speaker: one secondary, shorting speaker jack. OT part "265"; impedance not stated.
- **Global NFB:** **22K** from the OT secondary's ungrounded terminal to **V1b cathode** (1500 ohm to ground): 1500/(22K+1500) ~ 6.4% (derived).
- No printed voltages.

## 7. Power supply
5Y3GT; HV winding grounded CT; 5 V winding. B+: node 1 = **16-450 || 16-450** (two 16 uF caps both on the node, OT CT) -> **10K 1 W** -> node 2 **8-450** (6V6 screen) -> **22K** -> node 3 **8-450** (V1 plate loads). **3/4 AMP fuse** (printed "3/4 AMP. FUSE"), AC switch, **.05-600** from fuse junction to ground. Power transformer part number 66079. 6.3 V "to all 6.3 volt heaters and pilot light".

## 8. Not legible / uncertain
- Jack normalling; OT impedance; all DC voltages absent (notice only); (?) that both 16-450 caps sit on the same node (as drawn they do).

Differences vs Champ 5F1 (for reference): tone control added (.005/.0005), V1a cathode bypassed (25 uF), .02-400 (not -600) coupling caps, 470 ohm 1 W cathode R, 10K-1W, 3/4 A fuse, second 16-450.
