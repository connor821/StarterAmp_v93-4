# Fender Deluxe Model 26 - schematic read

Source: `dd644220-image.webp` (1200x807 image). Title "FENDER 'DELUXE' SCHEMATIC / MODEL 26". No date, revision or voltages. Hand-lettered. Caps "value-voltage" in uF (".02-600", ".05-200", "10-25" = 10 uF/25 V, "8-450", "16-450").

## 1. Tubes
Preamp **6SC7**; phase inverter **6N7**; output 2 x **6V6GT**; rectifier **5Y3GT**.

## 2. Preamp (6SC7, cathodes tied: **1500 ohm || 10-25** (10 uF/25 V) to ground - bypassed)
- Upper triode (jacks 1+2): jack 1 tip -> 7.5K, jack 2 tip -> 7.5K, joined; to the grid directly (no coupling cap, no grid leak drawn). Plate load **250K**.
- Lower triode (jack 3): jack -> direct to grid; jack line has **1 MEG** to ground. Plate load **250K**.
- The two 250K plate loads join at one point to the B+ rail (the 8-450 node feeding the 6N7 centre and plates).
- Plate -> **.02-600** -> volume pot (two x **1 MEG**, taper not printed); same Deluxe-style arrow-on-pot arrangement (wiper reading (?)); outputs tied at mix node -> 6N7 grid 1. No tone control.

## 3. Tone
No tone control. (Possible output-side "tone" - see 6.)

## 4. Tremolo
None.

## 5. Phase inverter 6N7 (twin triode, self-balancing)
- Common cathode: **5000 ohm || 10-25** (10 uF/25 V).
- Plate loads: **120K** and **120K** (each plate to a common B+ centre point, node tied to the 8-450 cap rail).
- Plate 1 -> **.05-600** -> upper 6V6 grid node; plate 2 -> **.05-200** -> lower 6V6 grid node.
- Grid divider (upper to lower): upper grid node -> **250K** -> tap (to 6N7 grid 2 by curved wire) -> **10K** -> grounded centre -> **250K** -> lower grid node. Tap fraction 10K/260K ~3.8% (derived). No grid stoppers.

## 6. Output stage
- 2 x 6V6GT push-pull, cathode bias **250 ohm 5 W || 25-25** (25 uF/25 V). Screens direct to screen node (no screen R). OT secondary: one **8 ohm** speaker ("8-ohm SPKR"). OT impedance not printed.
- Across the plates (upper plate to lower plate): a **2 MEG** potentiometer and **.05-200** capacitor in series (pot end at the upper plate, wiper to the cap, cap to the lower plate; pot's other end open) - a variable plate-to-plate RC (tone/treble-roll-off style); control name not printed.
- No NFB, no printed voltages.

## 7. Power supply
5Y3GT; [node 1 **16-450**: OT CT] -> **2.5K** -> [node 2 **8-450**: screens] -> **2.5K** -> [node 3 **8-450**: preamp plates and 6N7 centre]. (Resistor labels "2.5K", checked at high zoom.) AC switch, 2 AMP fuse, .05-600 to ground. No voltages.

## 8. Not legible / uncertain
- Wiper reading of volume arrows; jack-spring details; the control purpose of the 2 MEG/.05-200 pair (inferred from topology only); no values for tone voltage; OT impedance; all DC voltages absent.
