# Fender Bassman Amp AB165 (blackface, 2-channel, 2x6L6GC, silicon rectifier) - schematic read

- Source file: `5b41fbd8-Fender_Bassman_AB165.pdf` (1 page, scan; 150 dpi raster 4386x2715 used; resolution is good for most values but rotated labels are small)
- Printed title: `FENDER MODEL "BASSMAN-AMP AB165"` (followed by "P/O"), mark "D-FE" under the patents notice. Footer: "FENDER MUSICAL INSTRUMENTS, a division of Columbia Records Distribution Corp., FULLERTON, CALIFORNIA USA". Printed patents: "2817708, 2973681, 192859 and patents pending". No date.
- NOTICE printed: (1) Voltages read to ground with electronic voltmeter, values shown +/-20%. (2) All resistors 1/2 watt, 10% tolerance if not specified. (3) All capacitors at least 400 volt rating if not specified. (4) "When installing new 6L6's reset hum balance for min hum" (asterisk "*" marks the hum-balance pot).
- Box: "ON EXPORT MODEL  TR1-125P7DX"; "TR1-125P7D", "TR2-125C1A", "TR3-125A13A" (power transformer / choke / output transformer).
- Cap notation: ".1" = 0.1 uF; "250 PF"; "20-525" = 20 uF/525 V; "70-350" = 70 uF/350 V; "8-450V" = 8 uF/450 V.

## 1. Tube complement / heaters
| Pos | Tube | Function |
|---|---|---|
| Bass V1 | 7025 half | Bass-instrument channel gain stage 1 |
| Bass V2 | 7025 half | Bass channel gain stage 2 (after tone stack + volume) |
| Normal V1 | 7025 half | Normal channel gain stage 1 |
| Normal V2 | 7025 half | Normal channel stage 2 (after tone stack + volume) |
| V3 | 7025 (one triode) | Mixer/recovery gain stage after the two channels |
| PI | 12AT7 | Long-tailed phase inverter |
| Output | 2 x 6L6GC | Push-pull, fixed bias |
| Rectifier | Silicon diodes (3 in series per leg) | No rectifier tube |

(The schematic shows 3 "7025" envelope drawings: bass pair, normal pair, and V3; exact tube/half allocation as drawn: bass V1+V2 = one 7025, normal V1+V2 = one 7025, V3 = one 7025 (one half used).)
Heaters: "TO ALL HEATERS AND PILOT LIGHT" from TR1 6.3 V winding (center tap grounded as drawn); voltages not printed. AC: AC RECEPTACLE, GROUND SWITCH with **.047 uF / 600 V** cap, AC SWITCH, **2 AMP SLO-BLO** fuse.

## 2. Preamp stages
### Inputs (both channels, as drawn)
- Jacks 1 and 2: **68k** from each jack to the grid node (grid between two 68k), **1M** from the jack-1 node to ground; jack-2 switch contact tied to jack-1 node.

### BASS INSTRUMENT channel
**V1 (7025 half, bass)**: cathode **1500 ohm** in parallel with **25 uF** (labels "25/25" (?)), cathode **+1.6 V (?)** (reads "+1.6V"). Plate load **100k in parallel with .01 uF** to the +320 V node. Plate voltage **+220 V**.
- Tone stack on the plate: **250 pF** from plate to TREBLE pot top (**250K-A**); plate -> **100k** -> node X; X -> **.1** -> treble pot bottom / BASS pot top; X -> (down) node Y -> **.1** -> BASS pot bottom; node Y also -> **.1 uF** -> **DEEP** switch -> ground (switch closes the cap to ground). BASS pot **250K-A** (rheostat) between the two .1 caps' far ends; BASS pot bottom -> **6.8k (?)** -> ground. Treble wiper -> VOLUME pot **1M-A** top (volume bottom to ground); volume wiper -> V2 grid. (Pot part numbers like "2-15" printed next to values not legible.)
**V2 (7025 half, bass)**: cathode **1500 ohm || 25 uF**, +1.8 V. Plate load **100k** to +320 V; plate **+260 V**. Plate -> **220k** -> mixing node.

### NORMAL channel
**V1 (7025 half, normal)**: cathode **1500 ohm || 25 uF**, +1.7 V. Plate load **100k** to +320 V; plate voltage printed vertically on the plate line, not clearly readable (about +2xx V (?)).
- Tone stack: **250 pF** plate -> TREBLE pot (250K-A) top; plate -> **100k** -> node; node -> **.1** -> bass pot top / treble pot bottom; node -> **.047 uF** -> BASS pot bottom; BASS pot **250K-A** rheostat; BASS pot bottom -> **4.7k (?)** -> ground (label reads "4.7K"). Treble wiper -> VOLUME **1M-A** top; volume wiper -> V2 grid.
- **BRIGHT** switch: a cap (value reads **.001 uF (?)**, maybe .01) from the volume pot top (treble wiper) to the volume wiper via the switch.
**V2 (7025 half, normal)**: cathode **1500 ohm || 25 uF**, +1.9 V. Plate load **100k** to +320 V; plate **+260 V**. Plate -> **220k** -> mixing node.

### Mix / V3 (7025)
- Bass and normal 220k resistors join at the mix node. Mix node -> **.01 uF** -> V3 grid; V3 grid leak **470k** to ground.
- V3 cathode **1500 ohm || 25 uF (?)**, cathode about +2.x V (printed "+2.1V"? (?)).
- V3 plate load **100k** to the +390 V node; plate voltage **+250 V**.
- **470k** from V3 plate back to the mix node (drawn as a resistor from the mix node to the plate; local feedback/mix resistor).
- Plate -> **.1 uF** -> PI input node (same node receives the NFB via .1).

## 3. Tone stacks
Two independent passive stacks (one per channel) described above (bass stack with 250pF/100k/.1/.1/DEEP .1/6.8k(?), normal stack with 250pF/100k/.1/.047/4.7k(?)). Treble/bass/volume pots all listed as 250K-A / 250K-A / 1M-A ("-A" taper per label). Bright (normal channel) and Deep (bass channel) switches. No mid or presence control.

## 4. Reverb / tremolo / vibrato
None.

## 5. Phase inverter (12AT7, long-tailed pair)
- Upper grid: signal node (V3 coupling .1 and NFB .1).
- Grid resistors: **1M** from each grid to a junction node; junction -> **22k** to ground; junction -> **470 ohm** -> shared cathode node (cathode printed **+10.2 V (?)**, junction printed about +10 V). Lower grid coupled to ground (AC) by **.1-200** (0.1 uF/200 V) to the grounded line.
- Plate loads: **100k** (upper) and **100k** (lower), tolerance label "5%/1%" (?); the loads join at a center node fed from the **+415 V** node. Plate voltages: upper **+229 V**, lower **+220 V**.
- Coupling: **.022 uF** from each PI plate to its output-tube grid node.
- Also **220k** resistor from each PI plate to the corresponding 6L6 plate (see below).
- Note: the DC path of the tail (470 + 22k) does not appear to match the printed +10 V cathode voltage with ~3.8 mA total plate current; read as drawn (?).

## 6. Output stage
- 2 x **6L6GC**, push-pull, **fixed bias**, cathodes tied directly to ground (grounded cathode arrow).
- Grid stoppers: **1500 ohm** each (values read "1600"/"1500", assume 1.5k (?)), after the .022 coupling cap node.
- Grid leaks: **220k 5%** from each grid node to the bias bus ("-48 V").
- Screens: **470 ohm 1 W** each from screen to the screen supply node (choke output node, about +425 V).
- Printed on each 6L6: plate **+425 V**, screen **+425 V** (pins 3 and 4).
- **220k** from each 6L6 plate to its PI plate (plate-to-PI-plate feedback as drawn).
- Output transformer **TR3 (125A13A)**: primary center tap to the +425 V line before the choke; secondary feeds two speaker jacks/loads (two speaker symbols) plus **EXT. SPKR.** jack; no impedance/ratio printed.
- Global NFB: **47k** from the OT secondary hot side (same node as EXT. SPKR. jack) in series with **.1 uF** to the PI input node (same node as the V3 coupling cap). No presence control.
- Bias supply: TR1 tap -> **470 ohm 1 W** -> diode (conducts toward the winding; bias node negative) -> bias node with filter cap to ground (value not legible) -> **10K-L** potentiometer ("hum balance", marked "*"), with a resistor (value unreadable, ~15k (?)) from the pot's left end to ground; two **10k** resistors from the pot ends up to the grid-leak bus; **-48 V** label with arrow at the pot wiper feeding the bias bus. Printed bias **-48 V**.

## 7. Power supply
- TR1 HV winding with grounded center tap; each end through **3 silicon diodes in series** (labelled "320 V AC" per side) -> rectifier node printed **+425 V**.
- Filter at the rectifier output: two electrolytics in series, **70-350 each (70 uF/350 V)**, each shunted by a balancing resistor (value reads "?20K" (?)).
- STANDBY SWITCH, then +425 V node with **20-525** to ground (before the choke; OT center tap feeds from here).
- Choke **TR2 (125C1A)**; after the choke node (screens, +425 V printed) with **20-525** (reads "20-525/20-526").
- **1k 1W** -> **+415 V** node (**20-525**): PI plate loads.
- **4.7k 1W** -> **+390 V** node (**20-525**): V3 plate load (100k).
- **27k 1W** -> **+320 V** node (**8-450 V**, 8 uF/450 V): bass and normal V1/V2 plate loads (100k each).
- Printed voltages: +425 V (rectifier, and plates/screens), +415 V, +390 V, +320 V.

## 8. Not legible / uncertain
- Normal V1 plate voltage; bass V1 cathode voltage (1.6?).
- Bass-stack bottom resistor (6.8k?) and normal-stack bottom resistor (4.7k?).
- Cathode bypass cap voltage/values ("25" assumed 25 uF).
- Bright cap value (.001 or .01).
- Balancing resistor values across the 70-350 caps; bias filter cap value; the resistor from the hum pot to ground; exact wiring of the two 10k resistors around the hum pot.
- PI plate-load tolerances; 6L6 grid stopper (1.5k vs 1.6k).
- OT impedance/ratio; speaker impedances; heater voltages.
