# Fender Deluxe 5E3 - schematic + layout read

Source: `4626a600-fender_deluxe_5e3.pdf`, 2 pages (1494x929 / 1494x952 px 8-bit-indexed rasters). p1 "FENDER 'DELUXE' SCHEMATIC, MODEL 5E3", drafting code "F-EE". p2 "FENDER 'DELUXE' LAYOUT, MODEL 5E3" (F-EE) with "NOTICE: voltages read to ground with electronic voltmeter, values shown +/- 20%" - but **no voltages are actually printed** on the layout. No date or revision letter.

Caps printed "value-voltage" in uF (".1-400", ".0005" mica, ".005-600", "25-25" = 25 uF/25 V, "16-450").

## 1. Tube complement
V1 **12AY7** (dual triode, two channels); V2 **12AX7** (V2a gain stage, V2b cathodyne PI); output 2 x **6V6GT**; rectifier **5Y3GT**. 6.3 V heaters also run a pilot lamp.

## 2. Stages
**V1 = 12AY7 (both triodes share ONE cathode network: 820 ohm || 25-25 (25 uF/25 V) to ground).**
- Channel 1 (upper triode V1a), jacks 1 and 2: each jack tip -> **68K** in series to the grid node; **1 MEG** from the jack-1 line to ground (one 1 MEG per channel). Grid node connects direct to the grid (no coupling cap). Plate load **100K** to B+. Plate -> **.1-400** -> channel-1 volume pot.
- Channel 2 (lower triode V1b), jacks 1 and 2: identical 68K/68K/1 MEG arrangement, grid direct. Plate load **100K**. Plate -> **.1-400** -> channel-2 volume pot.
- Jack switching: arrows/springs drawn so that jack 2's spring is tied to jack 1's line and the 1 MEG (to ground) hangs on jack 1's side. Exact normalling is not determinable (?). Layout page shows one 1 MEG per channel at jack 1.
- Volume pots: two x **1 MEG** (taper not printed); drawn in the Deluxe style: coupling-cap line arrow lands on the pot (wiper symbol, (?)), one end grounded, the other end to the common mix node. No mixing resistors.
- Mix node -> V2a grid directly.

**Tone (channel 1 path only):** node at V1a's .1-400 output -> **.0005** (mica) -> 1 MEG tone pot top; pot bottom -> **.005** -> ground (layout: ".005 600"); wiper -> mix node.

**V2a (12AX7 first triode): gain stage.** Plate load **100K**; cathode **1500 ohm || 25-25** (25 uF/25 V). Grid = mix node. Plate -> coupling cap to V2b grid: schematic leaves it unlabeled; layout shows **.02 MFD 600 VOLTS** (read ".02 MFD / 600 V").

## 3. Tremolo
None. (A "GROUND SWITCH" selecting the .05-600 cap between the line leads is the only switch besides AC.)

## 4/5. Phase inverter V2b = cathodyne / split-load (12AX7 second triode)
- Grid: node after the .02 cap; **1 MEG** from the grid to the **cathode-resistor junction** (between 1500 ohm and 56K) - i.e. the grid is biased through the 1500 ohm.
- Plate load **56K** to B+ (node 3); cathode **1500 ohm + 56K** in series to ground. Cathode output tapped at the top of the 1500.
- Plate output -> **.1-400** -> upper 6V6 path; cathode output -> **.1-200** -> lower 6V6 path.
- Anomaly: the cathode-output wire crosses the B+ line of the 56K plate resistor with a solder dot drawn at the crossing; treated as a wire crossover (a connection would short the cathode output to B+) (?).

## 6. Output stage
- Each 6V6GT: grid stopper **1500 ohm**; the grid junctions have **220K** to the grid-return node (upper and lower, with common node grounded) i.e. 220K grid leaks to ground, one per tube.
- Cathode bias (shared): **250 ohm 5 W || 25-25** (25 uF/25 V) to ground. Screens direct to screen node (no screen R). OT CT to node 1. 
- Speaker: 8 ohm speaker jack + "EXTRA 8 ohm SPKR" jack (jack drawn at secondary). OT impedance not printed. **No NFB**.
- Printed voltages: none.

## 7. Power supply
- 5Y3GT. Chain: [node 1 **16-450**, OT CT/plates] -> **5000 ohm** -> [node 2 **16-450**, 6V6 screens] -> **22K** -> [node 3 **16-450**, V1, V2a, V2b plate loads (100K, 100K, 56K)]. Layout lists "5000" and "22K" as boxes between the caps and three "16 MFD 450 VOLTS" caps.
- Primary: AC switch, 2 AMP fuse, ground switch with **.05-600**; layout shows ".05 MFD 600 VOLTS" and "2 AMP fuse", pilot lite. 6.3 V winding "To all 6.3 volt filaments and pilot lite".
- No printed voltages. No part numbers.

## 8. Not legible / uncertain
- Wiper reading of volume pots; jack-spring normalling; wire-crossover vs junction at the V2b cathode-output/B+ crossing; schematic coupling cap V2a->V2b unlabeled (took .02-600 from layout, which reads ".02 MFD 600 VOLTS"); pot tapers; heater/OT details.

## Changes vs earlier Deluxes (3 lines)
- 5C3 -> 5E3: preamp 6SC7 -> 12AY7 (plate loads 250K -> 100K, cathodes grounded -> 820 ohm || 25 uF shared; 68K/1 MEG inputs instead of 7.5K/5 MEG), V2 6SC7 self-balancing PI -> 12AX7 gain stage + cathodyne PI (56K).
- 5C3 -> 5E3: output grids now 1500 ohm stoppers + 220K leaks (no 1 MEG/25K divider), NFB resistor removed, 5000 ohm/22K supply dropping replaces 10K/10K, ground switch + pilot lamp added.
- 5E3 has a layout page (no voltages); 5B3 is the only Deluxe revision here with printed voltages.
